// Fill out your copyright notice in the Description page of Project Settings.


#include "Build/Buildin.h"

// Sets default values
ABuildin::ABuildin()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	BuildingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BuildingMesh"));
	RootComponent = BuildingMesh;
}

// Called when the game starts or when spawned
void ABuildin::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ABuildin::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ABuildin::DestroyBuilding(FHitResult HitResult)
{
	//TODO drop mats / give mats
	Destroy();
}

