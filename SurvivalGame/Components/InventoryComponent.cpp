// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/InventoryComponent.h"
#include "Items/Item.h"
#include "Items/EquippableItem.h"
#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h"

#define LOCTEXT_NAMESPACE "Inventory"

// Sets default values for this component's properties
UInventoryComponent::UInventoryComponent()
{
	SetIsReplicatedByDefault(true);
	InventoryeNameText = FText::FromString("Inventory");
}

int32 UInventoryComponent::GetAmount()
{
	int32 amount = 0;

	for (auto& Item : Items)
	{
		if (Item && Item->ShouldShowInInventory())
			amount++;
	}
	return amount;
}

FItemAddResult UInventoryComponent::TryAddItem(UItem* Item)
{
	return TryAddItem_Internal(Item);
}

FItemAddResult UInventoryComponent::TryAddItemFromClass(TSubclassOf<class UItem> ItemClass, const int32 Quantity)
{
	UItem* Item = NewObject<UItem>(GetOwner(), ItemClass);
	Item->SetQuantity(Quantity);
	return TryAddItem_Internal(Item);
}

UItem* UInventoryComponent::SwapItem(UItem* Item, int32 index)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		
		UItem* ReturnItem = nullptr;

		if (Items[index])
		{
			ReturnItem = NewObject<UItem>(GetOwner(), Items[index]->GetClass());
			ReturnItem->SetQuantity(Items[index]->GetQuantity());
			
		}

		UItem* NewItemPointer = Item;
		
		
		UItem* NewItem = nullptr;
		if (Item)
		{
			NewItem = NewObject<UItem>(GetOwner(), Item->GetClass());
			NewItem->SetQuantity(Item->GetQuantity());
			NewItem->OwningInventory = this;
			//NewItem->AddedToInventory(this);
		}

		
		UEquippableItem* Test = Cast<UEquippableItem>(Item);

		if (Test)
		{
			Cast<UEquippableItem>(NewItem)->SetEquipped(false);
		}
			
		
		Items.Insert(NewItem, index);
		Items.RemoveAt(index + 1);
		
		if(NewItem)
			NewItem->MarkDirtyForReplication();

		return ReturnItem;
	}
	return nullptr;
}

bool UInventoryComponent::Swap(int32 from, int32 to)
{
	Items.Swap(from, to);
	return true;
}

int32 UInventoryComponent::ConsumeItem(UItem* Item)
{
	if (Item)
	{
		ConsumeItem(Item, Item->GetQuantity());
	}
	return 0;
}

int32 UInventoryComponent::ConsumeItem(UItem* Item, const int32 Quantity)
{
	if (GetOwner() && GetOwner()->HasAuthority() && Item)
	{
		const int32 RemoveQuantity = FMath::Min(Quantity, Item->GetQuantity());

		//We shouldn't have a negative amount of the item after the drop
		ensure(!(Item->GetQuantity() - RemoveQuantity < 0));

		//We now have zero of this item, remove it from the inventory
		Item->SetQuantity(Item->GetQuantity() - RemoveQuantity);

		if (Item->GetQuantity() <= 0)
		{
			RemoveItem(Item);
		}
		else
		{
			ClientRefreshInventory();
		}

		return RemoveQuantity;
	}

	return 0;
}

int32 UInventoryComponent::ConsumeItemClass(TSubclassOf<class UItem> ItemClass, const int32 Quantity)
{
	if (GetOwner() && GetOwner()->HasAuthority() && ItemClass)
	{
		int32 QuantityLeftToRemove = Quantity;

		TArray<UItem*> MatchingItems = FindItemsByClass(ItemClass);

		for (UItem* Item : MatchingItems)
		{
			int32 RemoveQuantity = FMath::Min(QuantityLeftToRemove, Item->GetQuantity());
			
			QuantityLeftToRemove -= RemoveQuantity;

			ensure(!(Item->GetQuantity() - RemoveQuantity < 0));

			Item->SetQuantity(Item->GetQuantity() - RemoveQuantity);

			if (Item->GetQuantity() <= 0)
			{
				RemoveItem(Item);
			}
			else
			{
				ClientRefreshInventory();
			}
		}

		return QuantityLeftToRemove == 0;

	}

	return 0;
}

bool UInventoryComponent::RemoveItem(UItem* Item)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		if (Item)
		{
			int32 index = Items.Find(Item);
			if (index == -1)
				return false;
			Items.RemoveAt(index);
			//Items.RemoveSingle(Item);
			Items.Insert(nullptr, index);

			ReplicatedItemsKey++;

			return true;
		}
	}

	return false;
}

bool UInventoryComponent::HasItem(TSubclassOf<class UItem> ItemClass, const int32 Quantity) const
{
	TArray<UItem*> ItemsToFind = FindItemsByClass(ItemClass);
	if (ItemsToFind.Num() > 0)
	{
		int32 Items_Quantity = 0;

		for (UItem* Item : ItemsToFind)
		{
			Items_Quantity += Item->GetQuantity();
		}

		return Items_Quantity >= Quantity;
	}
	return false;
}

UItem* UInventoryComponent::FindItem(UItem* Item) const
{
	if (Item)
	{
		for (auto& InvItem : Items)
		{
			if (InvItem && InvItem == Item)
			{
				return InvItem;
			}
		}
	}
	return nullptr;
}

int32 UInventoryComponent::FindItemIndex(UItem* Item) const
{
	for (int i = 0; i < Items.Num(); i++)
	{
		if (Items[i] && Items[i] == Item)
		{
			return i;
		}
	}
	return -1;
}

UItem* UInventoryComponent::FindItemByClass(TSubclassOf<class UItem> ItemClass) const
{
	for (auto& InvItem : Items)
	{
		if (InvItem && InvItem->GetClass() == ItemClass)
		{
			return InvItem;
		}
	}
	return nullptr;
}

TArray<UItem*> UInventoryComponent::FindItemsByClass(TSubclassOf<class UItem> ItemClass) const
{
	TArray<UItem*> ItemsOfClass;

	for (auto& InvItem : Items)
	{
		if (InvItem && InvItem->GetClass()->IsChildOf(ItemClass))
		{
			ItemsOfClass.Add(InvItem);
		}
	}

	return ItemsOfClass;
}

float UInventoryComponent::GetCurrentWeight() const
{
	float Weight = 0.f;

	for (auto& Item : Items)
	{
		if (Item)
		{
			Weight += Item->GetStackWeight();
		}
	}

	return Weight;
}

void UInventoryComponent::SetWeightCapacity(const float NewWeightCapacity)
{
	WeightCapacity = NewWeightCapacity;
	OnInventoryUpdated.Broadcast();
}

void UInventoryComponent::SetCapacity(const int32 NewCapacity)
{
	Capacity = NewCapacity;
	UpdateArray();
	OnInventoryUpdated.Broadcast();
}


void UInventoryComponent::ClientRefreshInventory_Implementation()
{
	OnInventoryUpdated.Broadcast();
}




void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	UpdateArray();
}

void UInventoryComponent::UpdateArray()
{
	if (Capacity > Items.Num())
	{
		for (int i = Items.Num(); i < Capacity; i++)
			Items.Add(nullptr);
	}
	if (Capacity < Items.Num())
	{
		for (int i = Items.Num() - 1; i >= 0 && Items.Num() > Capacity; i--)
		{
			if (!Items[i])
			{
				Items.RemoveAt(i);
			}
			//Items.RemoveSingle(nullptr);
		}
	}
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UInventoryComponent, Items);
}

bool UInventoryComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	//Check if the array of items needs to replicate
	if (Channel->KeyNeedsToReplicate(0, ReplicatedItemsKey))
	{
		for (auto& Item : Items)
		{
			if (Item)
			{
				if (Channel->KeyNeedsToReplicate(Item->GetUniqueID(), Item->RepKey))
				{
					bWroteSomething |= Channel->ReplicateSubobject(Item, *Bunch, *RepFlags);
				}
			}
		}
	}

	return bWroteSomething;
}

UItem* UInventoryComponent::AddItem(UItem* Item)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		AActor* Owner = GetOwner();
		UItem* NewItem = NewObject<UItem>(Owner, Item->GetClass());
		NewItem->SetQuantity(Item->GetQuantity());
		NewItem->OwningInventory = this;
		

		int32 index = Items.Find(nullptr);
		if (index == -1)
		{
			Items.Add(NewItem);
		}
		else
		{
			Items.Insert(NewItem, index);
			Items.RemoveAt(index + 1);
		}
			

		NewItem->AddedToInventory(this);
		NewItem->MarkDirtyForReplication();

		return NewItem;
	}

	return nullptr;
}


void UInventoryComponent::OnRep_Items()
{
	OnInventoryUpdated.Broadcast();
}

FItemAddResult UInventoryComponent::TryAddItem_Internal(UItem* Item)
{

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		const int32 AddAmount = Item->GetQuantity();
		int32 ActualAddAmount = AddAmount;

		if (!FMath::IsNearlyZero(Item->Weight))
		{
			const int32 WeightMaxAddAmount = FMath::FloorToInt((WeightCapacity - GetCurrentWeight()) / Item->Weight);
			ActualAddAmount = FMath::Min(AddAmount, WeightMaxAddAmount);

			if (ActualAddAmount <= 0)	//if you cant add any because of weight
			{
				return FItemAddResult::AddedNone(Item->GetQuantity(), LOCTEXT("StackWeightFullText", "Couldn't add {ItemName}, too much weight."));
			}
		}

		if (Item->bStackable)
		{
			//Somehow the items quantity went over the max stack size. This shouldn't ever happen
			ensure(Item->GetQuantity() <= Item->MaxStackSize);

			TArray<UItem*> ExistingItems = FindItemsByClass(Item->GetClass());
			if (ExistingItems.Num() > 0)
			{
				for (auto& ExistingItem : ExistingItems)
				{
					if (!(ExistingItem->GetQuantity() == ExistingItem->MaxStackSize))//found a partial stack
					{
						FText ErrorText;

						if (!FMath::IsNearlyZero(Item->Weight))
						{
							if (ActualAddAmount < AddAmount)	//can only add some because of weight
							{
								ErrorText = FText::Format(LOCTEXT("InventoryTooMuchWeightText", "Couldn't add entire stack of {ItemName} to Inventory."), Item->ItemDisplayName);
								return tryAddItemMultipleStacks_Internal(ExistingItem, Item, ActualAddAmount, AddAmount, ErrorText);
							}

						}
						return tryAddItemMultipleStacks_Internal(ExistingItem, Item, AddAmount, AddAmount, ErrorText); //weights fine
					}
				}

				if (GetAmount() + 1 > GetCapacity())
				{
					return FItemAddResult::AddedNone(Item->GetQuantity(), LOCTEXT("InventoryCapacityFullText", "Couldn't add {ItemName} to Inventory. Inventory is full."));
				}
				AddItem(Item);
				return FItemAddResult::AddedAll(AddAmount);
			}
			else //dont have the item in inventory yet
			{
				if (GetAmount() + 1 > GetCapacity())	//check if we can add a new item
				{
					return FItemAddResult::AddedNone(Item->GetQuantity(), LOCTEXT("InventoryCapacityFullText", "Couldn't add {ItemName} to Inventory. Inventory is full."));
				}

				AddItem(Item);
				return FItemAddResult::AddedAll(AddAmount);
			}
		}
		else //not bStackable, add as new slot
		{
			ensure(Item->GetQuantity() == 1);	

			if (GetAmount() + 1 > GetCapacity())
			{
				return FItemAddResult::AddedNone(Item->GetQuantity(), LOCTEXT("InventoryCapacityFullText", "Couldn't add {ItemName} to Inventory. Inventory is full."));
			}

			AddItem(Item);
			return FItemAddResult::AddedAll(AddAmount);
		}
	}

	check(false);
	return FItemAddResult::AddedNone(-1, LOCTEXT("ErrorMessage",""));
	
}

FItemAddResult UInventoryComponent::tryAddItemMultipleStacks_Internal(class UItem* ExistingItem, UItem* Item, float QuantityToAdd, float totalTryAdd, FText ErrorText)
{

	const int32 ExistingItemMaxAddAmount = ExistingItem->MaxStackSize - ExistingItem->GetQuantity();
	ExistingItem->SetQuantity(ExistingItem->GetQuantity() + FMath::Min(ExistingItemMaxAddAmount, QuantityToAdd));
	
	if (ExistingItemMaxAddAmount < QuantityToAdd) {

		if (GetAmount() + 1 > GetCapacity())
		{
			OnRep_Items();
			return FItemAddResult::AddedSome(QuantityToAdd, QuantityToAdd - ExistingItemMaxAddAmount, LOCTEXT("InventoryCapacityFullText", "Couldn't add {ItemName} to Inventory. Inventory is full."));
		}

		UItem* NewItem = NewObject<UItem>(GetOwner(), Item->GetClass()); 
		NewItem->SetQuantity(QuantityToAdd - ExistingItemMaxAddAmount);
		AddItem(NewItem);
		OnRep_Items();
	}

	if (QuantityToAdd == totalTryAdd)
	{
		return FItemAddResult::AddedAll(QuantityToAdd);
	}

	return FItemAddResult::AddedSome(QuantityToAdd - ExistingItemMaxAddAmount, QuantityToAdd, ErrorText);
}



#undef LOCTEXT_NAMESPACE