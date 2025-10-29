#include "PlayerCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"

    APlayerCharacter::APlayerCharacter()
    {
        PrimaryActorTick.bCanEverTick = true;

        // Camera boom settings
        CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
        CameraBoom->SetupAttachment(RootComponent);
        CameraBoom->TargetArmLength = 375.f;
        CameraBoom->bUsePawnControlRotation = true;
        CameraBoom->bEnableCameraLag = true;
        CameraBoom->CameraLagSpeed = 10.f;
        CameraBoom->bEnableCameraRotationLag = true;
        CameraBoom->CameraRotationLagSpeed = 10.f;
        CameraBoom->bDoCollisionTest = true;

        // Follow camera
        FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
        FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
        FollowCamera->FieldOfView = 110.f;
        FollowCamera->bUsePawnControlRotation = false;

        // Configurable spawn pitch
        CameraSpawnPitch = -20.f;
        InitialCameraSpawnPitch = CameraSpawnPitch;

        // Movement
        bIsRunning = false;
        RotationSpeed = 10.f;
        WalkSpeed = 300.f;
        SprintSpeed = 600.f;

        // Sensitivity
        SensitivityMultiplier = 0.75f;
        VerticalSensitivityMultiplier = 0.75f;

        // Character rotation settings
        bUseControllerRotationYaw = false;
        bUseControllerRotationPitch = false;
        bUseControllerRotationRoll = false;

        GetCharacterMovement()->bOrientRotationToMovement = false;
        GetCharacterMovement()->bUseControllerDesiredRotation = false;

        // Slopes
        WalkableSlopeAngle = 40.f;
        StepOffset = 30.f;
        GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
        GetCharacterMovement()->AirControl = CustomAirControl;
        GetCharacterMovement()->GravityScale = CustomGravityScale;
    }

    void APlayerCharacter::BeginPlay()
    {
        Super::BeginPlay();

        GetCharacterMovement()->SetWalkableFloorAngle(WalkableSlopeAngle);
        GetCharacterMovement()->MaxStepHeight = StepOffset;
        GetCharacterMovement()->PerchRadiusThreshold = 10.f;
        GetCharacterMovement()->bEnablePhysicsInteraction = true;
        GetCharacterMovement()->bEnableScopedMovementUpdates = true;
        GetCharacterMovement()->BrakingDecelerationWalking = 1800.f;
        GetCharacterMovement()->GroundFriction = 8.f;

        CameraBoom->SetRelativeRotation(FRotator(CameraSpawnPitch, 0.f, 0.f));

        if (APlayerController* PC = Cast<APlayerController>(GetController()))
        {
            if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
                ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
            {
                Subsystem->AddMappingContext(InputMapping, 0);
            }
        }
    }

    void APlayerCharacter::Tick(float DeltaTime)
    {
        Super::Tick(DeltaTime);

        const bool bIsGrounded = GetCharacterMovement()->IsMovingOnGround();
        const bool bIsFalling = GetCharacterMovement()->IsFalling();

        if (bIsCrouching && bIsFalling){
            bIsProning = false;
            return;
        }
        else if (bIsProning && bIsFalling)
        {
            bIsProning = false;
            bIsInProneTransition = false;
            bIsCrouching = true;
            return;
        }
        else if(bIsCrouching || bIsProning)
        {
            return;
        }
        
        // Update jump buffer
        if (bJumpInputQueued)
        {
            JumpBufferTimer -= DeltaTime;
            if (JumpBufferTimer <= 0.f)
            {
                bJumpInputQueued = false;
            }
        }
        
        if (bIsGrounded)
        {
            JumpCount = 0;
            bJumpPending = false;
            bIsFlipping = false;
        }

        // Trigger jump
        if (bIsGrounded && bJumpInputQueued && !bJumpPending)
        {
            bJumpPending = true;
            bJumpInputQueued = false;
            JumpCount++;

            bIsJumping = true;
        }

        // Trigger flip (double jump)
        if (!bIsGrounded && bAllowDoubleJump && JumpCount < 2 && bJumpInputQueued && !bJumpPending)
        {
            bIsFlipping = true;
            bIsJumping = true;
            bJumpPending = true;
            bJumpInputQueued = false;
            JumpCount++;
        }
        
        if (bIsJumping && bIsFalling)
        {
            bIsJumping = false;
        }
    }



    void APlayerCharacter::Move(const FInputActionValue& Value)
    {
        FVector2D Input = Value.Get<FVector2D>();
        MovementInput = Input;

        if (!Controller || bIsInProneTransition)
            return;

        if (bIsDancing)
        {
            if (!Input.IsNearlyZero())
            {
                bIsDancing = false;
            }
            else
            {
                return;
            }
        }

        FRotator CameraRot = FollowCamera->GetComponentRotation();
        CameraRot.Pitch = 0.f;
        CameraRot.Roll = 0.f;

        const FVector Forward = FRotationMatrix(CameraRot).GetUnitAxis(EAxis::X);
        const FVector Right = FRotationMatrix(CameraRot).GetUnitAxis(EAxis::Y);

        FVector MoveInput = Forward * Input.Y + Right * Input.X;

        if (MoveInput.IsNearlyZero())
            return;

        AddMovementInput(MoveInput.GetSafeNormal(), 1.f);

        FRotator DesiredRot = MoveInput.Rotation();

        if (Input.Y < 0.f)
        {
            DesiredRot.Yaw += 180.f;
        }

        FRotator Current = GetActorRotation();
        FRotator TargetYawOnly(0.f, DesiredRot.Yaw, 0.f);
        FRotator NewRot = FMath::RInterpTo(Current, TargetYawOnly, GetWorld()->GetDeltaSeconds(), RotationSpeed);
        SetActorRotation(NewRot);

        if (bIsProning)
        {
            GetCharacterMovement()->MaxWalkSpeed = ProneSpeed;
        }
        else if (bIsCrouching)
        {
            GetCharacterMovement()->MaxWalkSpeed = CrouchSpeed;
        }
        else if (bIsRunning && Input.Y >= 0.f)
        {
            GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
        }
        else
        {
            GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
        }
    }

    void APlayerCharacter::Look(const FInputActionValue& Value)
    {
        const FVector2D LookInput = Value.Get<FVector2D>();
        if (!LookInput.IsNearlyZero())
        {
            AddControllerYawInput(LookInput.X * SensitivityMultiplier);
            AddControllerPitchInput(LookInput.Y * VerticalSensitivityMultiplier);

            FRotator ControlRot = GetControlRotation();
            ControlRot.Pitch = FMath::ClampAngle(
                ControlRot.Pitch,
                InitialCameraSpawnPitch - 15.f,
                InitialCameraSpawnPitch + 45.f
            );
            GetController()->SetControlRotation(ControlRot);
        }
    }

    void APlayerCharacter::RunPressed()
    {
        bIsRunning = true;

        if (MovementInput.Y >= 0.f)
        {
            if (GetCharacterMovement()->MaxWalkSpeed != SprintSpeed)
            {
                GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
            }
        }
    }

    void APlayerCharacter::RunReleased()
    {
        bIsRunning = false;

        if (GetCharacterMovement()->MaxWalkSpeed != WalkSpeed)
        {
            GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
        }
    }

    void APlayerCharacter::Dance()
    {
        bIsDancing = true;
    }

    void APlayerCharacter::QueueJumpInput()
    {
        if (bIsDancing || bIsCrouching || bIsProning){return;}
        bJumpInputQueued = true;
        JumpBufferTimer = JumpBufferTime;
    }

    void APlayerCharacter::ApplyJumpForce()
    {
        LaunchCharacter(FVector(0.f, 0.f, JumpForce), false, true);
        bJumpPending = false;
    }

    void APlayerCharacter::TriggerFlip()
    {
        bIsFlipping = true;
        bIsJumping = true;
        bJumpPending = true;
        JumpCount++;
        LaunchCharacter(FVector(0.f, 0.f, FlipJumpForce), false, true);
    }

    void APlayerCharacter::EndFlip()
    {
        bIsFlipping = false;
        bIsJumping = false;
    }

    void APlayerCharacter::ToggleCrouch()
    {
        if (bIsProning || IsRunning() || IsJumping() || IsFlipping() || GetCharacterMovement()->IsFalling() || bIsInProneTransition)
        {
            return;
        }

        UCapsuleComponent* Capsule = GetCapsuleComponent();
        UMeshComponent* PlayerMesh = GetMesh();

        if (bIsCrouching)
        {
            if (CanStandUp())
            {
                float OldHeight = Capsule->GetUnscaledCapsuleHalfHeight();
                float NewHeight = StandCapsuleHalfHeight;

                Capsule->SetCapsuleHalfHeight(NewHeight, true);
                PlayerMesh->SetRelativeLocation(FVector(0.f, 0.f, -NewHeight));

                bIsCrouching = false;
                GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
            }
        }
        else
        {
            float NewHeight = CrouchCapsuleHalfHeight;

            Capsule->SetCapsuleHalfHeight(NewHeight, true); // sweep enabled
            PlayerMesh->SetRelativeLocation(FVector(0.f, 0.f, -NewHeight));

            bIsCrouching = true;
            GetCharacterMovement()->MaxWalkSpeed = CrouchSpeed;
        }
    }



    void APlayerCharacter::ToggleProne()
    {
        if (bIsInProneTransition || IsRunning() || IsJumping() || IsFlipping() || GetCharacterMovement()->IsFalling())
        {
            return;
        }

        UCapsuleComponent* Capsule = GetCapsuleComponent();
        UMeshComponent* PlayerMesh = GetMesh();

        if (bIsProning)
        {
            if (CanCrouchUpFromProne())
            {
                float OldHeight = Capsule->GetUnscaledCapsuleHalfHeight();
                float NewHeight = CrouchCapsuleHalfHeight;
                float Delta = NewHeight - OldHeight;
                
                FVector UpOffset(0.f, 0.f, 2.0f);
                FHitResult Hit;
                AddActorWorldOffset(UpOffset, true, &Hit);
                
                FVector DownOffset(0.f, 0.f, -Delta * 0.95f + CustomCapsuleCrouchOffset);
                AddActorWorldOffset(DownOffset, true, &Hit);

                Capsule->SetCapsuleHalfHeight(NewHeight, true);
                PlayerMesh->SetRelativeLocation(FVector(0.f, 0.f, -NewHeight));

                bIsProning = false;
                bIsCrouching = true;
                bIsInProneTransition = true;
                GetCharacterMovement()->MaxWalkSpeed = CrouchSpeed;
            }
        }
        else if (bIsCrouching)
        {
            float OldHeight = Capsule->GetUnscaledCapsuleHalfHeight();
            float NewHeight = ProneCapsuleHalfHeight;
            float Delta = OldHeight - NewHeight;
            
            FVector DownOffset(0.f, 0.f, Delta * 0.95f + CustomCapsuleProneOffset);
            FHitResult Hit;
            AddActorWorldOffset(DownOffset, true, &Hit);

            Capsule->SetCapsuleHalfHeight(NewHeight, true);
            PlayerMesh->SetRelativeLocation(FVector(0.f, 0.f, -NewHeight));

            bIsProning = true;
            bIsInProneTransition = true;
            GetCharacterMovement()->MaxWalkSpeed = ProneSpeed;
        }
    }



    bool APlayerCharacter::CanStandUp() const
    {
        FVector Start = GetActorLocation() + FVector(0.f, 0.f, CrouchCapsuleHalfHeight);
        float CheckDistance = (StandCapsuleHalfHeight - CrouchCapsuleHalfHeight) - CeilingCheckOffset;
        FVector End = Start + FVector(0.f, 0.f, CheckDistance);
        float Radius = GetCapsuleComponent()->GetScaledCapsuleRadius();

        FCollisionQueryParams Params;
        Params.AddIgnoredActor(this);

        return !GetWorld()->SweepTestByChannel(Start, End, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(Radius), Params);
    }

    bool APlayerCharacter::CanCrouchUpFromProne() const
    {
        FVector Start = GetActorLocation() + FVector(0.f, 0.f, ProneCapsuleHalfHeight);
        float CheckDistance = (CrouchCapsuleHalfHeight - ProneCapsuleHalfHeight) - CeilingCheckOffset;
        FVector End = Start + FVector(0.f, 0.f, CheckDistance);
        float Radius = GetCapsuleComponent()->GetScaledCapsuleRadius();

        FCollisionQueryParams Params;
        Params.AddIgnoredActor(this);

        return !GetWorld()->SweepTestByChannel(Start, End, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(Radius), Params);
    }

    void APlayerCharacter::StartProneTransition()
    {
        bIsInProneTransition = true;
    }

    void APlayerCharacter::EndProneTransition()
    {
        bIsInProneTransition = false;
    }

    void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
    {
        Super::SetupPlayerInputComponent(PlayerInputComponent);

        if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
        {
            EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Move);
            EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Look);
            EIC->BindAction(RunAction, ETriggerEvent::Started, this, &APlayerCharacter::RunPressed);
            EIC->BindAction(RunAction, ETriggerEvent::Completed, this, &APlayerCharacter::RunReleased);
            EIC->BindAction(DanceAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Dance);
            EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &APlayerCharacter::QueueJumpInput);
            EIC->BindAction(CrouchAction, ETriggerEvent::Started, this, &APlayerCharacter::ToggleCrouch);
            EIC->BindAction(ProneAction, ETriggerEvent::Started, this, &APlayerCharacter::ToggleProne);
        }
    }
