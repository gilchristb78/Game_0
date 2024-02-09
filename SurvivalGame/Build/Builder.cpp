// Fill out your copyright notice in the Description page of Project Settings.


#include "Build/Builder.h"
#include "Build/Buildin.h"

// Sets default values
ABuilder::ABuilder()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	BuildPreviewMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BuildPreviewMeshCoponent"));
	RootComponent = BuildPreviewMesh;

	bCanSpawn = true;
}

// Called when the game starts or when spawned
void ABuilder::BeginPlay()
{
	Super::BeginPlay();
	SetActorHiddenInGame(true);

	if(MaterialTrue)
		BuildPreviewMesh->SetMaterial(0,MaterialTrue);
	
}

void ABuilder::SetBuildPosition(const FHitResult& HitResult, const FQuat& Rotation)
{
	SetActorHiddenInGame(!HitResult.bBlockingHit);
	if (HitResult.bBlockingHit)
	{
		//did we hit a Building?
		SnappedBuilding = Cast<ABuildin>(HitResult.GetActor());

		//are we overlapping (dont place)
		TArray<UPrimitiveComponent*> overlaps;
		BuildPreviewMesh->GetOverlappingComponents(overlaps); 

		if (overlaps.Num() == 0)
		{
			if (MaterialTrue)
			{
				BuildPreviewMesh->SetMaterial(0, MaterialTrue);
				bCanSpawn = true;
			}
				
		}
		else
		{
			if (MaterialFalse)
			{
				BuildPreviewMesh->SetMaterial(0, MaterialFalse);
				bCanSpawn = false;
			}
				
		}


		//set loc and rot
		SetActorLocation(HitResult.Location);
		SetActorRotation(Rotation);
	}
	else
	{
		//didnt hit anything (even floor) likely outside range
		SnappedBuilding = nullptr;
	}
}

void ABuilder::SpawnBuilding()
{
	if (CurrentBuildingType.BuildingClass && !IsHidden() && bCanSpawn)
	{
		GetWorld()->SpawnActor<ABuildin>(CurrentBuildingType.BuildingClass, GetActorTransform());
	}
}

void ABuilder::DestroyBuilding(const FHitResult& HitResult)
{
	if (SnappedBuilding)
	{
		SnappedBuilding->DestroyBuilding(HitResult);
	}
}

void ABuilder::SetBuildingType(FBuildingType buildingType)
{
	CurrentBuildingType = buildingType;
	if (buildingType.BuildingMesh)
	{
		BuildPreviewMesh->SetStaticMesh(buildingType.BuildingMesh); 
	}
}

