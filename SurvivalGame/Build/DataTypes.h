#pragma once

#include "DataTypes.Generated.h"


class UInstancedStaticMeshComponent;

UENUM(BlueprintType)
enum class EBuildType : uint8
{
	Foundation	UMETA(DisplayName = "Foundation"),
	Wall		UMETA(DisplayName = "Wall"),
	Ceiling		UMETA(DisplayName = "Ceiling"),
	Count
};

const FString BuildString[] = { "Foundation", "Wall", "Ceiling"};

USTRUCT(BlueprintType)
struct FBuildingVisualType
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Building")
	class UStaticMesh* BuildingMesh;
	
	UPROPERTY(EditDefaultsOnly, Category = "Building")
	TSubclassOf<class ABuilding> BuildingClass;

	UPROPERTY(EditDefaultsOnly, Category = "Building")
	EBuildType BuildType;

	UPROPERTY(EditDefaultsOnly, Category = "Building")
	bool bInstanced;

	UPROPERTY(EditDefaultsOnly, Category = "Building")
	FName FilterChar;
};

USTRUCT(BlueprintType)
struct FBuildingType
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Building")
	class UStaticMesh* BuildingMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Building")
	TSubclassOf<class ABuildin> BuildingClass;

	UPROPERTY(EditDefaultsOnly, Category = "Building")
	bool bInstanced;

	UPROPERTY(EditDefaultsOnly, Category = "Building")
	FName FilterChar;
};


USTRUCT(BlueprintType)
struct FBuildingSocketData
{
	GENERATED_BODY()

	UInstancedStaticMeshComponent* InstancedComponent;
	int32 index;
	FName SocketName;
	FTransform SocketTransform;
};

USTRUCT(BlueprintType)
struct FSocketInformation
{
	GENERATED_BODY()
	FName SocketName;
	bool bSocketInUse = false;

};

USTRUCT(BlueprintType)
struct FBuildIndexSockets
{
	GENERATED_BODY()

	int32 Index;
	TArray<FSocketInformation> SocketsInformation;

};

USTRUCT(BlueprintType)
struct FInstanceSocketCheck
{
	GENERATED_BODY()

	UInstancedStaticMeshComponent* InstancedComponent;
	TArray< FBuildIndexSockets> InstanceSocketInformation;

};


