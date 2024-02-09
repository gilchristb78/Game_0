// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DataTypes.h"
#include "GameFramework/Actor.h"
#include "Builder.generated.h"

class ABuildin;

UCLASS()
class SURVIVALGAME_API ABuilder : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABuilder();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	//current mesh of the build preview
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Building")
	class UStaticMeshComponent* BuildPreviewMesh;

	//Red material used when you cant build
	UPROPERTY(EditDefaultsOnly, Category = "Building")
	UMaterialInstance* MaterialFalse;

	//Green material used when you can build
	UPROPERTY(EditDefaultsOnly, Category = "Building")
	UMaterialInstance* MaterialTrue;

	//the current type of building selected
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Building")
	FBuildingType CurrentBuildingType;

	//the current building the preview is snapped to or nullptr
	ABuildin* SnappedBuilding;

	bool bCanSpawn;

public:	

	void SetBuildPosition(const FHitResult& HitResult, const FQuat& Rotation);
	void SpawnBuilding();
	void DestroyBuilding(const FHitResult& HitResult);
	void SetBuildingType(FBuildingType buildingType);


};
