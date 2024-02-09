// Fill out your copyright notice in the Description page of Project Settings.


#include "Build/BuildingVisual.h"
#include "Build/Building.h"
#include "Build/Machine.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"

// Sets default values
ABuildingVisual::ABuildingVisual()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	BuildMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BuildMeshComponent"));
	RootComponent = BuildMesh;

	BuildingTypeIndex = 0;
	bMaterialisTrue = false;
}

// Called when the game starts or when spawned
void ABuildingVisual::BeginPlay()
{
	Super::BeginPlay();
	SetActorHiddenInGame(true);
	if (BuildingTypes[BuildingTypeIndex].BuildingMesh)
	{
		BuildMesh->SetStaticMesh(BuildingTypes[BuildingTypeIndex].BuildingMesh);
	}

	if (MaterialTrue)
	{
		bMaterialisTrue = true;
		BuildMesh->SetMaterial(0, MaterialTrue);
	}

}

void ABuildingVisual::SetBuildPosition(const FHitResult& HitResult, const FQuat& Rotation)
{
	SetActorHiddenInGame(!HitResult.bBlockingHit);
	if (HitResult.bBlockingHit)
	{
		InteractingBuilding = Cast<ABuilding>(HitResult.GetActor());
		if (InteractingBuilding)
		{

			SocketData = InteractingBuilding->GetHitSocketTransform(HitResult, BuildingTypes[BuildingTypeIndex].FilterChar);
			if (!SocketData.SocketTransform.Equals(FTransform()))
			{
				SetActorTransform(SocketData.SocketTransform);
				if (MaterialTrue && !bMaterialisTrue)
				{
					bMaterialisTrue = true;
					BuildMesh->SetMaterial(0, MaterialTrue);
				}
					
				return;
			}

			if (MaterialFalse && bMaterialisTrue)
			{
				bMaterialisTrue = false;
				BuildMesh->SetMaterial(0, MaterialFalse);
			}
			//UE_LOG(LogTemp, Warning, TEXT("Hit Index %d"), HitIndex);
		}

		SetActorLocation(HitResult.Location);
		SetActorRotation(Rotation);
		
	}
	else
	{
		
		InteractingBuilding = nullptr;
	}
}

void ABuildingVisual::SpawnBuilding()
{
	if (BuildingTypes[BuildingTypeIndex].BuildingClass && !IsHidden())
	{
		if (InteractingBuilding && bMaterialisTrue)
		{
			InteractingBuilding->AddInstance(SocketData, BuildingTypes[BuildingTypeIndex].BuildType);
		}
		else 
		{
			GetWorld()->SpawnActor<ABuilding>(BuildingTypes[BuildingTypeIndex].BuildingClass, GetActorTransform());
		}
		
	}
}

void ABuildingVisual::SetMeshTo(EBuildType BuildType)
{
	for (const FBuildingVisualType& Building : BuildingTypes)
	{
		if (Building.BuildType == BuildType)
		{
			BuildMesh->SetStaticMesh(Building.BuildingMesh);
			return;
		}
	}
}

void ABuildingVisual::CycleMesh()
{
	BuildingTypeIndex == BuildingTypes.Num() - 1 ? BuildingTypeIndex = 0 : BuildingTypeIndex++;

	if (BuildingTypes[BuildingTypeIndex].BuildingMesh)
	{
		BuildMesh->SetStaticMesh(BuildingTypes[BuildingTypeIndex].BuildingMesh);
	}
}

void ABuildingVisual::DestroyInstance(const FHitResult& HitResult)
{
	if (InteractingBuilding)
	{
		if (UInstancedStaticMeshComponent* InstancedStaticMeshComponent = Cast<UInstancedStaticMeshComponent>(HitResult.GetComponent()))
		{
			FBuildingSocketData BuildingSocketData;
			BuildingSocketData.InstancedComponent = InstancedStaticMeshComponent;
			BuildingSocketData.index = HitResult.Item;
			InteractingBuilding->DestroyInstance(BuildingSocketData);
		}
		
	}
}
