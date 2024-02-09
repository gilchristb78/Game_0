// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Buildin.generated.h"

UCLASS()
class SURVIVALGAME_API ABuildin : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABuildin();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Buildin")
	class UStaticMeshComponent* BuildingMesh;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	void DestroyBuilding(FHitResult HitResult);

};
