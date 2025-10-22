#include "PlayerCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"
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

        if (!Controller)
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

        if (bIsRunning && Input.Y >= 0.f)
        {
            if (GetCharacterMovement()->MaxWalkSpeed != SprintSpeed)
            {
                GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
            }
        }
        else
        {
            if (GetCharacterMovement()->MaxWalkSpeed != WalkSpeed)
            {
                GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
            }
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
                InitialCameraSpawnPitch + 30.f
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
        if (bIsDancing){return;}
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
        }
    }
