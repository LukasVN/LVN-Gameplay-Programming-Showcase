
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputActionValue.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "PlayerCharacter.generated.h"

UCLASS()
class MECHANICS_TEST_LVN_API APlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	APlayerCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaTime) override;
	
	// Player functions
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void RunPressed();
	void Dance();
	void RunReleased();

	// Jump system functions
	void QueueJumpInput();

	bool bIsRunning = false;
	bool bIsDancing = false;
	bool bIsJumping = false;
	bool bIsFlipping = false;
	bool bIsCrouching = false;
	bool bIsProning = false;
	
	// Input properties
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputMappingContext* InputMapping;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* RunAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* DanceAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* JumpAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* CrouchAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* ProneAction;

	// Camera components and properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	float SensitivityMultiplier;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	float VerticalSensitivityMultiplier;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	float CameraSpawnPitch;
	float InitialCameraSpawnPitch;

	// Movement properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
	float RotationSpeed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
	float WalkSpeed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
	float SprintSpeed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
	float WalkableSlopeAngle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
	float StepOffset;

	// Jump General properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumping")
	float JumpForce = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumping")
	float FlipJumpForce = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumping")
	float CustomAirControl = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumping")
	float CustomGravityScale = 2.f;

	// Jump Extra properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumping")
	float JumpBufferTime = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumping")
	bool bAllowDoubleJump = true;

	UPROPERTY(BlueprintReadOnly, Category = "Jumping")
	bool bJumpInputQueued = false;

	UPROPERTY(BlueprintReadOnly, Category = "Jumping")
	bool bJumpPending = false;

	UPROPERTY(BlueprintReadOnly, Category = "Jumping")
	int32 JumpCount = 0;

	float JumpBufferTimer = 0.1f;

	// Crouch properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crouch")
	float CrouchSpeed = 200.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crouch")
	float CrouchCapsuleHalfHeight = 44.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crouch")
	float StandCapsuleHalfHeight = 88.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crouch")
	float CustomCapsuleCrouchOffset = -40.f;

	//Prone properties

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prone")
	float ProneCapsuleHalfHeight = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prone")
	float CeilingCheckOffset = 5.f;

	UPROPERTY(BlueprintReadOnly, Category = "Prone")
	bool bIsInProneTransition = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prone")
	float ProneSpeed = 125.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prone")
	float CustomCapsuleProneOffset = -20.f;

	
private:
	FVector2D MovementInput;

public:

	// Notify methods
	void ApplyJumpForce(); // Called when Jumping Animation leaves the ground
	void TriggerFlip(); // Called at the start of Flip Animation
	void EndFlip(); // Called at the end of Flip Animation
	void ToggleCrouch();
	void ToggleProne();
	
	// Getters for movement input and states
	UFUNCTION(BlueprintCallable, Category="Movement")
	float GetForwardInput() const { return MovementInput.Y; }

	UFUNCTION(BlueprintCallable, Category="Movement")
	bool IsRunning() const { return bIsRunning; }

	UFUNCTION(BlueprintCallable, Category="Movement")
	bool IsDancing() const { return bIsDancing; }

	UFUNCTION(BlueprintCallable, Category="Jumping")
	bool IsJumping() const { return bIsJumping; }

	UFUNCTION(BlueprintCallable, Category="Jumping")
	bool IsFlipping() const { return bIsFlipping; }

	UFUNCTION(BlueprintCallable, Category="Crouching")
	bool IsPlayerCrouching() const { return bIsCrouching; }

	UFUNCTION(BlueprintCallable, Category="Proning")
	bool IsPlayerProning() const { return bIsProning; }

	UFUNCTION(BlueprintCallable, Category = "Crouch")
	bool CanStandUp() const;

	UFUNCTION(BlueprintCallable, Category = "Prone")
	bool CanCrouchUpFromProne() const;

	UFUNCTION(BlueprintCallable, Category = "Prone")
	void StartProneTransition();

	UFUNCTION(BlueprintCallable, Category = "Prone")
	void EndProneTransition();

	
};
