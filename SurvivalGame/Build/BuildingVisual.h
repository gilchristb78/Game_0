// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DataTypes.h"
#include "GameFramework/Actor.h"
#include "BuildingVisual.generated.h"

class ABuilding;
class UMaterialInstance;

UCLASS()
class SURVIVALGAME_API ABuildingVisual : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABuildingVisual();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Building")
	class UStaticMeshComponent* BuildMesh;

	

	uint8 BuildingTypeIndex;

	UPROPERTY(EditDefaultsOnly, Category = "Building")
	UMaterialInstance* MaterialFalse;

	UPROPERTY(EditDefaultsOnly, Category = "Building")
	UMaterialInstance* MaterialTrue;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Building")
	TArray<FBuildingVisualType> BuildingTypes;

	bool bMaterialisTrue;

	ABuilding* InteractingBuilding;

	FBuildingSocketData SocketData;

	void SetMeshTo(EBuildType BuildType);


public:

	void SetBuildPosition(const FHitResult& HitResult, const FQuat& Rotation);
	void SpawnBuilding();
	void DestroyInstance(const FHitResult& HitResult);
	void CycleMesh();
};
