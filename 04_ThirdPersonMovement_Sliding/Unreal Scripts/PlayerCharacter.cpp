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

        // Handle airborne crouch/prone transitions
        if (bIsCrouching && bIsFalling)
        {
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
        else if (bIsCrouching || bIsProning)
        {
            return;
        }

        // Sliding logic
        if (bIsSliding)
        {
            FVector GroundNormal = FVector::UpVector;
            FHitResult Hit;
            FVector Start = GetActorLocation();
            FVector End = Start - FVector(0.f, 0.f, 150.f);

            bool bIsDownhillAligned = false;
            float SlideExitSpeedThreshold = MinSlideSpeed;
            float Alignment = 0.f;

            if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility))
            {
                GroundNormal = Hit.Normal;

                float Incline = FVector::DotProduct(GroundNormal, FVector::UpVector);
                float SlopeAngle = FMath::RadiansToDegrees(FMath::Acos(Incline));

                FVector DownhillDir = FVector::CrossProduct(GroundNormal, FVector::CrossProduct(FVector::UpVector, GroundNormal)).GetSafeNormal();
                Alignment = FVector::DotProduct(SlideVelocity.GetSafeNormal(), DownhillDir);

                // Uncomment for slope and align debugging
                //DrawDebugString(GetWorld(), Start, FString::Printf(TEXT("Slope: %.1f° Align: %.2f"), SlopeAngle, Alignment), nullptr, FColor::White, 0.1f);

                // Downhill boost
                if (SlopeAngle > 10.f && Alignment < 0.5f)
                {
                    float BoostScale = FMath::Clamp((1.f - Alignment) * (SlopeAngle / 30.f), 0.5f, 2.5f);
                    SlideVelocity += DownhillDir * RampBoostSpeed * DeltaTime * BoostScale;

                    //UE_LOG(LogTemp, Log, TEXT("Ramp boost applied: Slope %.1f°, Align %.2f"), SlopeAngle, Alignment);
                    bIsDownhillAligned = true;
                }

                // Uphill penalty
                if (Alignment > 0.f)
                {
                    float UphillPenalty = FMath::Clamp(Alignment, 0.2f, 1.f);
                    SlideVelocity *= 1.f - UphillPenalty * 0.5f;

                    //UE_LOG(LogTemp, Log, TEXT("Uphill penalty applied: Align %.2f, Scale %.2f"), Alignment, 1.f - UphillPenalty * 0.5f);
                }

                // Adjust exit threshold for downhill
                SlideExitSpeedThreshold = bIsDownhillAligned ? MinSlideSpeed * 0.5f : MinSlideSpeed;

                // Flat boost scaled and interpolated
                float FlatBoostScale = FMath::Clamp(1.f - Alignment, 0.f, 1.f);
                FVector TargetFlatBoost = GetActorForwardVector() * FlatSlideBoost * FlatBoostScale;
                SlideVelocity = FMath::VInterpTo(SlideVelocity, TargetFlatBoost, DeltaTime, 3.0f);
            }

            // Clamp max slide speed
            SlideVelocity = SlideVelocity.GetClampedToMaxSize(MaxSlideSpeed);
            GetCharacterMovement()->MaxWalkSpeed = MaxSlideSpeed;

            // Apply movement
            AddMovementInput(SlideVelocity.GetSafeNormal(), SlideVelocity.Size() * DeltaTime);

            // Track airborne slide time
            if (!bIsGrounded)
            {
                SlideFallTimer += DeltaTime;
            }
            else
            {
                SlideFallTimer = 0.f;
            }

            // Grace period timer
            if (SlideStartTimer > 0.f)
            {
                SlideStartTimer -= DeltaTime;
            }

            // Exit slide conditions
            bool bShouldExitSlide = SlideVelocity.Size() < SlideExitSpeedThreshold && !bIsDownhillAligned;

            if (SlideStartTimer <= 0.f && (bShouldExitSlide || SlideFallTimer > SlideFallGraceTime || !bIsRunning))
            {
                ExitSlide();
                return;
            }

            
            SlideVelocity = FMath::VInterpTo(SlideVelocity, FVector::ZeroVector, DeltaTime, SlideFriction);
        }

        // Jump buffer
        if (bJumpInputQueued)
        {
            JumpBufferTimer -= DeltaTime;
            if (JumpBufferTimer <= 0.f)
            {
                bJumpInputQueued = false;
            }
        }

        // Grounded jump reset
        if (bIsGrounded)
        {
            JumpCount = 0;
            bJumpPending = false;
            bIsFlipping = false;
        }

        // Buffered jump
        if (bIsGrounded && bJumpInputQueued && !bJumpPending)
        {
            bJumpPending = true;
            bJumpInputQueued = false;
            JumpCount++;
            bIsJumping = true;
        }

        // Double jump
        if (!bIsGrounded && bAllowDoubleJump && JumpCount < 2 && bJumpInputQueued && !bJumpPending)
        {
            bIsFlipping = true;
            bIsJumping = true;
            bJumpPending = true;
            bJumpInputQueued = false;
            JumpCount++;
        }

        // Reset jump flag when falling
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

        if (bIsSliding)
        {
            return;
        }
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

    void APlayerCharacter::TryStartSlide()
    {
        // Check if already sliding or in a conflicting state
        if (bIsSliding || bIsCrouching || bIsProning || !IsRunning() || MovementInput.Size() < 0.1f || GetForwardInput() < 0.f || GetCharacterMovement()->Velocity.Size() <= 0.f)
            return;

        // Check if character is grounded OR close enough to the ground to allow mid-air slide
        float GroundDistance = GetGroundDistance();

        if (!GetCharacterMovement()->IsMovingOnGround() && GroundDistance > SlideAirThreshold)
            return;

        // Begin slide
        bIsSliding = true;
        SlideStartTimer = 0.2f;
        SlideVelocity = GetActorForwardVector() * SlideSpeed;

        float NewHeight = ProneCapsuleHalfHeight;
        GetCapsuleComponent()->SetCapsuleHalfHeight(NewHeight, true);
        GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -NewHeight));
    }

    void APlayerCharacter::ExitSlide()
    {
        bIsSliding = false;

        const bool bCanStand = CanStandUp();
        const bool bCanCrouch = CanCrouchUpFromProne();

        UCapsuleComponent* Capsule = GetCapsuleComponent();
        USkeletalMeshComponent* PlayerMesh = GetMesh();

        if (bCanStand && bCanCrouch)
        {
            Capsule->SetCapsuleHalfHeight(StandCapsuleHalfHeight, true);
            PlayerMesh->SetRelativeLocation(FVector(0.f, 0.f, -StandCapsuleHalfHeight));
            bIsCrouching = false;
            bIsProning = false;
            GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
        }
        else if (bCanCrouch)
        {
            Capsule->SetCapsuleHalfHeight(CrouchCapsuleHalfHeight, true);
            PlayerMesh->SetRelativeLocation(FVector(0.f, 0.f, -CrouchCapsuleHalfHeight));
            bIsCrouching = true;
            bIsProning = false;
            GetCharacterMovement()->MaxWalkSpeed = CrouchSpeed;
        }
        else
        {
            Capsule->SetCapsuleHalfHeight(ProneCapsuleHalfHeight, true);
            PlayerMesh->SetRelativeLocation(FVector(0.f, 0.f, -ProneCapsuleHalfHeight));
            bIsCrouching = true;
            bIsProning = true;
            GetCharacterMovement()->MaxWalkSpeed = ProneSpeed;
        }
    }

    float APlayerCharacter::GetGroundDistance() const
    {
        FVector Start = GetActorLocation();
        FVector End = Start - FVector(0.f, 0.f, 200.f); // Trace 200 units downward

        FHitResult Hit;
        FCollisionQueryParams Params;
        Params.AddIgnoredActor(this);

        bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
        DrawDebugLine(GetWorld(), Start, End, bHit ? FColor::Green : FColor::Red, false, 1.0f, 0, 2.0f);
        
        if (bHit)
        {
            return (Start - Hit.Location).Size();
        }

        return MAX_FLT; 
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

    void APlayerCharacter::HandleCrouchOrSlidePressed()
    {
        if (bIsProning || IsJumping() || IsFlipping() || bIsInProneTransition)
            return;

        float GroundDistance = GetGroundDistance();
        const bool bNearGround = GroundDistance <= SlideAirThreshold;
        const bool bCanSlide = !bIsSliding && IsRunning() && MovementInput.Size() > 0.1f;

        if (bCanSlide && (GetCharacterMovement()->IsMovingOnGround() || bNearGround))
        {
            TryStartSlide(); // Start slide (held behavior)
        }
        else if (GetCharacterMovement()->IsMovingOnGround())
        {
            // Toggle crouch
            UCapsuleComponent* Capsule = GetCapsuleComponent();
            USkeletalMeshComponent* PlayerMesh = GetMesh();

            if (bIsCrouching)
            {
                if (CanStandUp())
                {
                    Capsule->SetCapsuleHalfHeight(StandCapsuleHalfHeight, true);
                    PlayerMesh->SetRelativeLocation(FVector(0.f, 0.f, -StandCapsuleHalfHeight));
                    bIsCrouching = false;
                    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
                }
            }
            else
            {
                Capsule->SetCapsuleHalfHeight(CrouchCapsuleHalfHeight, true);
                PlayerMesh->SetRelativeLocation(FVector(0.f, 0.f, -CrouchCapsuleHalfHeight));
                bIsCrouching = true;
                GetCharacterMovement()->MaxWalkSpeed = CrouchSpeed;
            }
        }
    }



    void APlayerCharacter::HandleCrouchReleased()
    {
        if (bIsSliding)
        {
            ExitSlide();
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
            EIC->BindAction(CrouchAction, ETriggerEvent::Started, this, &APlayerCharacter::HandleCrouchOrSlidePressed);
            EIC->BindAction(CrouchAction, ETriggerEvent::Completed, this, &APlayerCharacter::HandleCrouchReleased);
            EIC->BindAction(ProneAction, ETriggerEvent::Started, this, &APlayerCharacter::ToggleProne);
        }
    }
