// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DataTypes.h"
#include "Building.generated.h"


class UInstancedStaticMeshComponent;

UCLASS()
class SURVIVALGAME_API ABuilding : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABuilding();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Building")
	UInstancedStaticMeshComponent* FoundationInstanceMesh;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Building")
	UInstancedStaticMeshComponent* WallInstanceMesh;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Building")
	UInstancedStaticMeshComponent* CeilingInstanceMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Building")
	TArray<UInstancedStaticMeshComponent*> InstanceMeshs;

	TArray<FName> Sockets;

	TArray<FInstanceSocketCheck> InstanceSocketsCheck;


	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	bool isValidSocket(UInstancedStaticMeshComponent* HitComponent, int32 HitIndex, const FName& FilterChar, const FName& SocketName);

public:

	UFUNCTION(BlueprintCallable, Category = "Building")
	void DestroyInstance(const FBuildingSocketData& BuildingSocketData);


	//given an instance, index of component in that system and socket name, return the transform to that socket.
	UFUNCTION(BlueprintCallable, Category = "Building")
	FTransform GetInstancedSocketTransform(UInstancedStaticMeshComponent* InstancedComponent, int32 InstanceIndex, const FName& SocketName, bool& Success);

	FBuildingSocketData GetHitSocketTransform(const FHitResult& HitResult, const FName& FilterChar, float ValidHitDistance = 25.0f);

	void AddInstance(const FBuildingSocketData& BuildingSocketData, EBuildType BuildType);

};
