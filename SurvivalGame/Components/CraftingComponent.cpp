// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/CraftingComponent.h"
#include "Net/UnrealNetwork.h"
#include "Camera/CameraComponent.h"
#include "Items/Item.h"
#include "Components/InventoryComponent.h"
#include "Recipes/Recipe.h"
#include "Components/CapsuleComponent.h"
#include "World/Pickup.h"
#include "Engine/ActorChannel.h"

// Sets default values for this component's properties
UCraftingComponent::UCraftingComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
	// ...
}

void UCraftingComponent::MarkArrayDirtyForReplication()
{
	++ReplicatedRecipesKey;
	OnRep_Crafting(); // Ensure OnRep is called to trigger replication
}

URecipe* UCraftingComponent::AddRecipe(URecipe* recipe)
{
	if (GetOwner() && GetOwner()->HasAuthority())
 	{
		URecipe* NewRecipe = NewObject<URecipe>(GetOwner(), recipe->GetClass());
		NewRecipe->Owner = GetOwner();
		NewRecipe->SetInputItems(recipe->GetInputItems());
		NewRecipe->SetOutputItems(recipe->GetOutputItems());
		NewRecipe->MarkDirtyForReplication();
		Recipes.Add(NewRecipe);

		ReplicatedRecipesKey++;
		

		return NewRecipe;
	}
	return nullptr;
}

bool UCraftingComponent::HasMaterials(URecipe* recipe)
{
	if(!Recipes.Contains(recipe))
		return false;
	
	if (!InputInventory)
		return false;

	for (auto& item : recipe->GetInputItems())
	{
		if(!InputInventory->HasItem(item->GetClass(), item->GetQuantity()))
			return false;
	}

	return true;
}

bool UCraftingComponent::Craft(URecipe* recipe)
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		ServerCraft(recipe);
		return false;
	}

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		if (!HasMaterials(recipe) || !OutputInventory)
			return false;

		for (auto& item : recipe->GetInputItems()) //take away mats
		{
			InputInventory->ConsumeItemClass(item->GetClass(), item->GetQuantity());
		}


		for (auto& item : recipe->GetOutputItems())
		{
			FItemAddResult ar = OutputInventory->TryAddItemFromClass(item->GetClass(), item->GetQuantity());


			
			
			if ((ar.Result == EItemAddResult::IAR_NoItemsAdded) || (ar.Result == EItemAddResult::IAR_SomeItemsAdded))
			{
				int32 ItemQuantity = item->GetQuantity();

				if (ar.Result == EItemAddResult::IAR_SomeItemsAdded)
				{
					ItemQuantity = ar.AmountToGive - ar.AmountGiven;
				}

				FActorSpawnParameters SpawnParams;
				SpawnParams.Owner = GetOwner();
				SpawnParams.bNoFail = true;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

				FVector SpawnLocation = GetOwner()->GetActorLocation();

				FTransform SpawnTransform(GetOwner()->GetActorRotation(), SpawnLocation);


				APickup* Pickup = GetWorld()->SpawnActor<APickup>(PickupClass, SpawnTransform, SpawnParams);
				Pickup->InitializePickup(item->GetClass(), ItemQuantity);
			}

		}
		return true;
	}
	return false;
}

void UCraftingComponent::ServerCraft_Implementation(URecipe* recipe)
{
	Craft(recipe);
}




// Called when the game starts
void UCraftingComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

void UCraftingComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCraftingComponent, Recipes);
	DOREPLIFETIME(UCraftingComponent, ReplicatedRecipesKey);
}

bool UCraftingComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	//Check if the array of items needs to replicate
	//if (Channel->KeyNeedsToReplicate(0, ReplicatedRecipesKey))
	//{
		for (auto& Recipe : Recipes)
		{
			if (Recipe)
			{
				if (Channel->KeyNeedsToReplicate(Recipe->GetUniqueID(), Recipe->RepKey))
				{
					bWroteSomething |= Channel->ReplicateSubobject(Recipe, *Bunch, *RepFlags);
				}
			
				for(auto& Item : Recipe->GetInputItems())
				{
					if (Channel->KeyNeedsToReplicate(Item->GetUniqueID(), Item->RepKey))
					{
						bWroteSomething |= Channel->ReplicateSubobject(Item, *Bunch, *RepFlags);
					}
				}

				for (auto& Item : Recipe->GetOutputItems())
				{
					if (Channel->KeyNeedsToReplicate(Item->GetUniqueID(), Item->RepKey))
					{
						bWroteSomething |= Channel->ReplicateSubobject(Item, *Bunch, *RepFlags);
					}
				}
			}
		}
	//}

	return bWroteSomething;
}

void UCraftingComponent::OnRep_Crafting()
{
	if (GetOwner()->HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("updateServer"));
	}
	else
		UE_LOG(LogTemp, Warning, TEXT("updateclient"));
	OnCraftingUpdated.Broadcast();
}


// Called every frame
void UCraftingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

