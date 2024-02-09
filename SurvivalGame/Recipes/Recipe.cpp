// Fill out your copyright notice in the Description page of Project Settings.


#include "Recipes/Recipe.h"
#include "Components/InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h"

URecipe::URecipe()
{
	RepKey = 0;

}

void URecipe::MarkDirtyForReplication()
{
	++RepKey;
}

bool URecipe::AddInputFromClass(TSubclassOf<class UItem> ItemClass, const int32 Quantity)
{

	UItem* InputItem = NewObject<UItem>(Owner, ItemClass);
	InputItem->SetQuantity(Quantity);
	InputItem->MarkDirtyForReplication();
	InputItems.Add(InputItem);
	MarkDirtyForReplication();
	return true;

}

bool URecipe::AddOutputFromClass(TSubclassOf<class UItem> ItemClass, const int32 Quantity)
{
	UItem* OutputItem = NewObject<UItem>(Owner, ItemClass);
	OutputItem->SetQuantity(Quantity);
	OutputItem->MarkDirtyForReplication();
	OutputItems.Add(OutputItem);
	MarkDirtyForReplication();
	return true;
}

void URecipe::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(URecipe, RepKey);
	DOREPLIFETIME(URecipe, InputItems);
	DOREPLIFETIME(URecipe, OutputItems);
	DOREPLIFETIME(URecipe, Owner);
}

bool URecipe::IsSupportedForNetworking() const
{
	return true;
}

void URecipe::OnRep_Items()
{
	if (Owner->HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("updateServerRRR"));
	}
	else
		UE_LOG(LogTemp, Warning, TEXT("updateclientRRR"));
	OnRecipeModified.Broadcast();
}

void URecipe::SetInputItems(const TArray<class UItem*>& NewInputItems)
{
	InputItems = NewInputItems;
	MarkDirtyForReplication();
}

void URecipe::SetOutputItems(const TArray<class UItem*>& NewOutputItems)
{
	OutputItems = NewOutputItems;
	MarkDirtyForReplication();
}
