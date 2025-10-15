
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

	//Player functions
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void RunPressed();
	void Dance();
	void RunReleased();

	bool bIsRunning = false;
	bool bIsDancing = false;

	//Getters for movement input and states
	UFUNCTION(BlueprintCallable, Category="Movement")
	float GetForwardInput() const { return MovementInput.Y; }

	UFUNCTION(BlueprintCallable, Category="Movement")
	bool IsRunning() const { return bIsRunning; }

	UFUNCTION(BlueprintCallable, Category="Movement")
	bool IsDancing() const { return bIsDancing; }

	//Input properties
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


	//Camera components and properties
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

	//Movement properties
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
	
private:
	FVector2D MovementInput;
};
