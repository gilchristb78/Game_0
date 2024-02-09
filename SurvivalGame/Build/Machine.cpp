// Fill out your copyright notice in the Description page of Project Settings.


#include "Build/Machine.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/InteractionComponent.h"
#include "Components/InventoryComponent.h"
#include "Player/SurvivalCharacter.h"

#define LOCTEXT_NAMESPACE "Machine"

AMachine::AMachine()
{

	LootInteraction = CreateDefaultSubobject<UInteractionComponent>("LootInteraction");
	LootInteraction->InteractableActionText = LOCTEXT("LootActorText", "Loot");
	LootInteraction->InteractableNameText = LOCTEXT("LootActorName", "Chest");
	LootInteraction->SetupAttachment(GetRootComponent());

	Inventory = CreateDefaultSubobject<UInventoryComponent>("Inventory");
	Inventory->SetCapacity(40); //TODO
	Inventory->SetWeightCapacity(80.f);

	
}

void AMachine::BeginPlay()
{
	Super::BeginPlay();

	LootInteraction->OnInteract.AddDynamic(this, &AMachine::OnInteract);


}

void AMachine::OnInteract(ASurvivalCharacter* Character)
{
	if (Character)
	{
		Character->SetLootSource(Inventory);
	}
}



#undef LOCTEXT_NAMESPACE
