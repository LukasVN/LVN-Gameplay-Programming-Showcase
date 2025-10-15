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
    CameraBoom->TargetArmLength = 300.f;
    CameraBoom->bUsePawnControlRotation = true;

    CameraBoom->bEnableCameraLag = true;
    CameraBoom->CameraLagSpeed = 10.f;
    CameraBoom->bEnableCameraRotationLag = true;
    CameraBoom->CameraRotationLagSpeed = 10.f;
    CameraBoom->bDoCollisionTest = true;

    // Follow camera
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    // Configurable spawn pitch
    CameraSpawnPitch = -20.f;
    InitialCameraSpawnPitch = CameraSpawnPitch;

    // Movement
    bIsRunning = false;
    RotationSpeed = 10.f;
    WalkSpeed   = 300.f;
    SprintSpeed = 600.f;

    //Sensitivity
    SensitivityMultiplier = 0.75f;
    VerticalSensitivityMultiplier = 0.75f;

    // Character rotation settings
    bUseControllerRotationYaw = false;
    bUseControllerRotationPitch = false;
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = false;
    GetCharacterMovement()->bUseControllerDesiredRotation = false;

    // Default values for slopes
    WalkableSlopeAngle = 40.f;
    StepOffset = 30.f;
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    
}

void APlayerCharacter::BeginPlay()
{
    Super::BeginPlay();

    //Smoothing for slopes and stairs
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
    const FVector Right   = FRotationMatrix(CameraRot).GetUnitAxis(EAxis::Y);
    
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
            InitialCameraSpawnPitch + 60.f
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
    }
}
