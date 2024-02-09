// Fill out your copyright notice in the Description page of Project Settings.


#include "Build/Building.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"
#include "DrawDebugHelpers.h"

// Sets default values
ABuilding::ABuilding()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	FoundationInstanceMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("FoundationInstanceMesh"));
	WallInstanceMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("WallInstanceMesh"));
	CeilingInstanceMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("CeilingInstanceMesh"));
	RootComponent = FoundationInstanceMesh;

	SetReplicates(false);
}

// Called when the game starts or when spawned
void ABuilding::BeginPlay()
{
	Super::BeginPlay();
	
	FInstanceSocketCheck FoundationInstanceSocket;
	FoundationInstanceSocket.InstancedComponent = FoundationInstanceMesh;
	FInstanceSocketCheck WallInstanceSocket;
	WallInstanceSocket.InstancedComponent = WallInstanceMesh;
	FInstanceSocketCheck CeilingInstanceSocket;
	CeilingInstanceSocket.InstancedComponent = CeilingInstanceMesh;
	InstanceSocketsCheck.Add(FoundationInstanceSocket);
	InstanceSocketsCheck.Add(WallInstanceSocket);
	InstanceSocketsCheck.Add(CeilingInstanceSocket);

	//FoundationInstanceMesh->AddInstance(FTransform());
	FBuildingSocketData BuildingSocketData;
	BuildingSocketData.index = 0;
	BuildingSocketData.InstancedComponent = FoundationInstanceMesh;
	BuildingSocketData.SocketName = NAME_None;
	BuildingSocketData.SocketTransform = GetActorTransform();
	AddInstance(BuildingSocketData, EBuildType::Foundation);

	Sockets = FoundationInstanceMesh->GetAllSocketNames();
	Sockets.Append(WallInstanceMesh->GetAllSocketNames());
	Sockets.Append(CeilingInstanceMesh->GetAllSocketNames());

	
	/*
	FTransform modifier = FTransform();

	for (int i = 1; i < 3; i++)
	{
		FVector MeshLocation = modifier.GetLocation();
		MeshLocation.Z = 250 * i;
		modifier.SetLocation(MeshLocation);

		FoundationInstanceMesh->AddInstance(modifier);
	}
	*/
	
}


void ABuilding::DestroyInstance(const FBuildingSocketData& BuildingSocketData)
{
	if (BuildingSocketData.InstancedComponent)
	{
		BuildingSocketData.InstancedComponent->RemoveInstance(BuildingSocketData.index);
	}

}

FTransform ABuilding::GetInstancedSocketTransform(UInstancedStaticMeshComponent* InstancedComponent, int32 InstanceIndex, const FName& SocketName, bool& Success)
{
	Success = true;
	if (InstancedComponent && InstancedComponent->IsValidInstance(InstanceIndex))
	{
		FTransform InstanceTransform = FTransform();
		InstancedComponent->GetInstanceTransform(InstanceIndex, InstanceTransform, true);
		FTransform ActorTransfor = GetTransform();
		FTransform InstanceTransformRelative = UKismetMathLibrary::MakeRelativeTransform(InstanceTransform, ActorTransfor); //handle rotation
		FTransform SocketTransform = InstancedComponent->GetSocketTransform(SocketName, RTS_Component);
		InstanceTransform = SocketTransform * InstanceTransform;// *GetActorTransform();


		DrawDebugString(GetWorld(), InstanceTransform.GetLocation(), SocketName.ToString(), nullptr, FColor::White, 0.1f);
		DrawDebugSphere(GetWorld(), InstanceTransform.GetLocation(), 5.0f, 10, FColor::Red);
		FTransform Temp;
		InstancedComponent->GetInstanceTransform(InstanceIndex, Temp, true);
		DrawDebugSphere(GetWorld(), Temp.GetLocation(), 5.0f, 15, FColor::Blue);
		
		
		
		return InstanceTransform;
	}

	Success = false;
	return FTransform();
}


bool ABuilding::isValidSocket(UInstancedStaticMeshComponent* HitComponent, int32 HitIndex, const FName& FilterChar, const FName& SocketName)
{
	for (FInstanceSocketCheck& InstanceSocketCheck : InstanceSocketsCheck)
	{
		if (InstanceSocketCheck.InstancedComponent == HitComponent)
		{
			for (const FBuildIndexSockets& BuildIndexSockets : InstanceSocketCheck.InstanceSocketInformation)
			{
				if (BuildIndexSockets.Index == HitIndex)
				{
					for (const FSocketInformation& SocketInformation : BuildIndexSockets.SocketsInformation)
					{
						if (SocketInformation.SocketName == SocketName && SocketInformation.bSocketInUse)
						{
							return false;
						}
					}
				}
			}
		}
	}
	return HitComponent->DoesSocketExist(SocketName) && SocketName.ToString().StartsWith(FilterChar.ToString());
}


FBuildingSocketData ABuilding::GetHitSocketTransform(const FHitResult& HitResult, const FName& FilterChar, float ValidHitDistance)
{
	FBuildingSocketData SocketData = FBuildingSocketData();
	UInstancedStaticMeshComponent* HitComponent = Cast<UInstancedStaticMeshComponent>(HitResult.Component); 
	int32 HitIndex = HitResult.Item;

	if (HitIndex != -1)
	{
		bool bIsSuccessful = false;
		for (const FName& SocketName : Sockets)
		{
			if (isValidSocket(HitComponent, HitIndex, FilterChar,SocketName))
			{
				FTransform SocketTransform = GetInstancedSocketTransform(HitComponent, HitIndex, SocketName, bIsSuccessful);

				if (FVector::Distance(SocketTransform.GetLocation(), HitResult.Location) <= ValidHitDistance)
				{
					//UE_LOG(LogTemp, Warning, TEXT("Valid Hit on Socket: %s"), *SocketName.ToString());
					SocketData.index = HitIndex;
					SocketData.InstancedComponent = HitComponent;
					SocketData.SocketName = SocketName;
					SocketData.SocketTransform = SocketTransform;
					return SocketData;
				}
			}
			
		}

		
	}

	return SocketData;
}

void ABuilding::AddInstance(const FBuildingSocketData& BuildingSocketData, EBuildType BuildType)
{
	for (FInstanceSocketCheck& InstanceSocket : InstanceSocketsCheck)
	{
		if (InstanceSocket.InstancedComponent == BuildingSocketData.InstancedComponent)
		{
			bool bFoundMatchingIndex = false;
			for (FBuildIndexSockets& IndexSocket : InstanceSocket.InstanceSocketInformation)
			{
				if (IndexSocket.Index == BuildingSocketData.index)
				{
					bFoundMatchingIndex = true;
					for (FSocketInformation& SocketInformation : IndexSocket.SocketsInformation)
					{
						if (SocketInformation.SocketName == BuildingSocketData.SocketName)
						{
							SocketInformation.bSocketInUse = true;
							break;
						}
					}
					
					break;
				}
			}
			
			if (!bFoundMatchingIndex)
			{
				FBuildIndexSockets BuildIndexSockets;
				BuildIndexSockets.Index = BuildingSocketData.index;
				FSocketInformation SocketInformation;
				for (const FName& SocketName : InstanceSocket.InstancedComponent->GetAllSocketNames())
				{
					SocketInformation.SocketName = SocketName;
					SocketInformation.bSocketInUse = SocketName.IsEqual(BuildingSocketData.SocketName);
					BuildIndexSockets.SocketsInformation.Add(SocketInformation);
				}
				InstanceSocket.InstanceSocketInformation.Add(BuildIndexSockets);
			}
			
		}
	}

	//InstanceMeshs[static_cast<uint8>(BuildType)]->AddInstanceWorldSpace(BuildingSocketData.SocketTransform);

	switch(BuildType)
	{
		case EBuildType::Foundation: 
			FoundationInstanceMesh->AddInstanceWorldSpace(BuildingSocketData.SocketTransform);
			break;
		case EBuildType::Wall:
			WallInstanceMesh->AddInstanceWorldSpace(BuildingSocketData.SocketTransform);
			break;
		case EBuildType::Ceiling:
			CeilingInstanceMesh->AddInstanceWorldSpace(BuildingSocketData.SocketTransform);
			break; 
	}
	
}


