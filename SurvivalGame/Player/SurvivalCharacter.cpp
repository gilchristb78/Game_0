// Fill out your copyright notice in the Description page of Project Settings.


#include "SurvivalCharacter.h"
#include "DrawDebugHelpers.h"
#include "Net/UnrealNetwork.h"
#include "Build/BuildingVisual.h"
#include "Build/Builder.h"
#include "Build/DataTypes.h"
#include "Camera/CameraComponent.h"
#include "Player/SurvivalPlayerController.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/InventoryComponent.h"
#include "Components/InteractionComponent.h"
#include "Components/CraftingComponent.h"
#include "Items/EquippableItem.h"
#include "Items/ClothingItem.h"
#include"Materials/MaterialInstance.h"
#include "World/Pickup.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/DamageType.h"
#include "Weapons/MeleeDamage.h"
#include "Kismet/GameplayStatics.h"
#include "SurvivalGame.h"

#define LOCTEXT_NAMESPACE "SurvivalCharacter"

// Sets default values
ASurvivalCharacter::ASurvivalCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>("SpringArmComponent");
	SpringArmComponent->SetupAttachment(GetMesh(), FName("CameraSocket"));
	SpringArmComponent->TargetArmLength = 0.f;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>("CameraComponent");
	CameraComponent->SetupAttachment(SpringArmComponent);
	CameraComponent->bUsePawnControlRotation = true;

	HelmetMesh = PlayerMeshes.Add(EEquippableSlot::EIS_Helmet, CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HelmetMesh")));
	ChestMesh = PlayerMeshes.Add(EEquippableSlot::EIS_Chest, CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ChestMesh")));
	LegsMesh = PlayerMeshes.Add(EEquippableSlot::EIS_Legs, CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("LegsMesh")));
	FeetMesh = PlayerMeshes.Add(EEquippableSlot::EIS_Feet, CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FeetMesh")));
	VestMesh = PlayerMeshes.Add(EEquippableSlot::EIS_Vest, CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("VestMesh")));
	HandsMesh = PlayerMeshes.Add(EEquippableSlot::EIS_Hands, CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HandsMesh")));
	BackpackMesh = PlayerMeshes.Add(EEquippableSlot::EIS_Backpack, CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BackpackMesh")));

	for (auto& PlayerMesh : PlayerMeshes)
	{
		USkeletalMeshComponent* MeshComponent = PlayerMesh.Value;
		MeshComponent->SetupAttachment(GetMesh());
		MeshComponent->SetMasterPoseComponent(GetMesh());
	}

	PlayerMeshes.Add(EEquippableSlot::EIS_Head, GetMesh());

	PlayerInventory = CreateDefaultSubobject<UInventoryComponent>("PlayerInventory");
	PlayerInventory->SetCapacity(50);
	PlayerInventory->SetWeightCapacity(80.f);

	PlayerCrafter = CreateDefaultSubobject<UCraftingComponent>("PlayerCrafter");
	PlayerCrafter->InputInventory = PlayerInventory;
	PlayerCrafter->OutputInventory = PlayerInventory;

	LootPlayerInteraction = CreateDefaultSubobject<UInteractionComponent>("PlayerInteraction");
	LootPlayerInteraction->InteractableActionText = LOCTEXT("LootPlayerText", "Loot");
	LootPlayerInteraction->InteractableNameText = LOCTEXT("LootPlayerName", "Player");
	LootPlayerInteraction->SetupAttachment(GetRootComponent());
	LootPlayerInteraction->SetActive(false, true);
	LootPlayerInteraction->bAutoActivate = false;

	InteractionCheckFrequency = 0.f;
	InteractionCheckDistance = 1000.f;

	MaxHealth = 100.f;
	Health = MaxHealth;


	GetMesh()->SetOwnerNoSee(true);

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

	bInBuildMode = false;

	CurrentBuildingIndex = 0;

}

// Called when the game starts or when spawned
void ASurvivalCharacter::BeginPlay()
{
	Super::BeginPlay();

	LootPlayerInteraction->OnInteract.AddDynamic(this, &ASurvivalCharacter::BeginLootingPlayer);

	//Try to display the players platform name on their loot card
	if (APlayerState* PS = GetPlayerState())
	{
		LootPlayerInteraction->SetInteractableNameText(FText::FromString(PS->GetPlayerName()));
	}
	
	for (auto& PlayerMesh : PlayerMeshes)
	{
		NakedMeshes.Add(PlayerMesh.Key, PlayerMesh.Value->SkeletalMesh);
	}

	if (BuildingClass)
	{
		Builder = GetWorld()->SpawnActor<ABuilder>(BuildingClass);
		Builder->SetBuildingType(Buildings[CurrentBuildingIndex]);
	}
	
}

void ASurvivalCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASurvivalCharacter, LootSource);
	DOREPLIFETIME(ASurvivalCharacter, Killer);

	DOREPLIFETIME_CONDITION(ASurvivalCharacter, Health, COND_OwnerOnly);
}

bool ASurvivalCharacter::IsInteracting() const
{
	return GetWorldTimerManager().IsTimerActive(TimerHandle_Interact);
}

float ASurvivalCharacter::GetRemainingInteractTime() const
{
	return GetWorldTimerManager().GetTimerRemaining(TimerHandle_Interact);
}

void ASurvivalCharacter::UseItem(UItem* Item)
{
	if (GetLocalRole() < ROLE_Authority && Item)
	{
		ServerUseItem(Item);
	}

	if (HasAuthority())
	{
		UEquippableItem* EquippableItem = Cast<UEquippableItem>(Item);
		if (PlayerInventory && !PlayerInventory->FindItem(Item))
		{
			return;
		}
	}

	if (Item)
	{
		Item->Use(this);
	}
}

void ASurvivalCharacter::UseItemEquip(UItem* Item, int32 from)
{
	if (GetLocalRole() < ROLE_Authority && Item)
	{
		ServerUseItemEquip(Item, from);
	}

	if (HasAuthority())
	{
		UEquippableItem* EquippableItem = Cast<UEquippableItem>(Item);
		if (PlayerInventory && !PlayerInventory->FindItem(Item))
		{
			return;
		}
	}

	if (Item)
	{
		PlayerInventory->SetCapacity(PlayerInventory->GetCapacity() + 1);
		PlayerInventory->Swap(from, PlayerInventory->GetCapacity() - 1);
		Item->Use(this);
	}
}

void ASurvivalCharacter::UseItemUnequip(UItem* Item, int32 to)
{
	if (GetLocalRole() < ROLE_Authority && Item)
	{
		ServerUseItemUnequip(Item, to);
	}

	if (HasAuthority())
	{
		if (PlayerInventory && !PlayerInventory->FindItem(Item))
		{
			return;
		}
		
	}

	if (Item)
	{
		int32 from = PlayerInventory->FindItemIndex(Item);
		PlayerInventory->Swap(from, to);
		PlayerInventory->SetCapacity(PlayerInventory->GetCapacity() - 1);
		Item->Use(this);
	}
		
}

void ASurvivalCharacter::ServerUseItemUnequip_Implementation(UItem* Item, int32 to)
{
	UseItemUnequip(Item, to);
}

void ASurvivalCharacter::ServerUseItemEquip_Implementation(UItem* Item, int32 from)
{
	UseItemEquip(Item, from);
}


void ASurvivalCharacter::DropItem(UItem* Item, const int32 Quantity)
{
	if (PlayerInventory && Item && PlayerInventory->FindItem(Item))
	{
		if (GetLocalRole() < ROLE_Authority)
		{
			ServerDropItem(Item, Quantity);
			return;
		}

		if (HasAuthority())
		{
			const int32 ItemQuantity = Item->GetQuantity();
			const int32 DroppedQuantity = PlayerInventory->ConsumeItem(Item, Quantity);

			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.bNoFail = true;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

			FVector SpawnLocation = GetActorLocation();
			SpawnLocation.Z -= GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

			FTransform SpawnTransform(GetActorRotation(), SpawnLocation);

			ensure(PickupClass);
			
			APickup* Pickup = GetWorld()->SpawnActor<APickup>(PickupClass, SpawnTransform, SpawnParams);
			Pickup->InitializePickup(Item->GetClass(), DroppedQuantity);
			
		}
	}
}

void ASurvivalCharacter::ItemAddedToInventory(UItem* Item)
{
}

void ASurvivalCharacter::ItemRemovedFromInventory(UItem* Item)
{
}

bool ASurvivalCharacter::EquipItem(UEquippableItem* Item)
{
	EquippedItems.Add(Item->Slot, Item);
	OnEquippedItemsChanged.Broadcast(Item->Slot, Item);
	return true;
}

bool ASurvivalCharacter::UnEquipItem(UEquippableItem* Item)
{
	if (Item)
	{
		if (EquippedItems.Contains(Item->Slot))
		{
			if (Item == *EquippedItems.Find(Item->Slot))
			{
				EquippedItems.Remove(Item->Slot);
				OnEquippedItemsChanged.Broadcast(Item->Slot, nullptr);
				return true;
			}
		}
	}
	return false;
}

void ASurvivalCharacter::EquipClothing(UClothingItem* Clothing)
{
	if (USkeletalMeshComponent* ClothingMesh = GetSlotSkeletalMeshComponent(Clothing->Slot))
	{
		ClothingMesh->SetSkeletalMesh(Clothing->Mesh);
		ClothingMesh->SetMaterial(ClothingMesh->GetMaterials().Num() - 1, Clothing->MaterialInstance);
	}
}

void ASurvivalCharacter::UnEquipClothing(const EEquippableSlot Slot)
{
	if (USkeletalMeshComponent* EquippableMesh = GetSlotSkeletalMeshComponent(Slot))
	{
		if (USkeletalMesh* BodyMesh = *NakedMeshes.Find(Slot))
		{
			EquippableMesh->SetSkeletalMesh(BodyMesh);

			//Put the materials back on the body mesh (Since gear may have applied a different material)
			for (int32 i = 0; i < BodyMesh->Materials.Num(); ++i)
			{
				if (BodyMesh->Materials.IsValidIndex(i))
				{
					EquippableMesh->SetMaterial(i, BodyMesh->Materials[i].MaterialInterface);
				}
			}
		}
		else
		{
			//For some gear like backpacks, there is no naked mesh
			EquippableMesh->SetSkeletalMesh(nullptr);
		}
	}
}

void ASurvivalCharacter::UnEquipEquipment(const EEquippableSlot Slot)
{
	if (EquippedItems.Find(Slot))
	{
		EquippedItems.Remove(Slot);
	}
}

void ASurvivalCharacter::ServerDropItem_Implementation(class UItem* Item, const int32 Quantity)
{
	DropItem(Item, Quantity);
}

bool ASurvivalCharacter::ServerDropItem_Validate(class UItem* Item, const int32 Quantity)
{
	return true;
}


void ASurvivalCharacter::ServerUseItem_Implementation(UItem* Item)
{
	UseItem(Item);
}

bool ASurvivalCharacter::ServerUseItem_Validate(UItem* Item)
{
	return true;
}

USkeletalMeshComponent* ASurvivalCharacter::GetSlotSkeletalMeshComponent(const EEquippableSlot Slot)
{
	if (PlayerMeshes.Contains(Slot))
	{
		return *PlayerMeshes.Find(Slot);
	}
	return nullptr;
}

float ASurvivalCharacter::ModifyHealth(const float Delta)
{
	const float OldHealth = Health;

	Health = FMath::Clamp<float>(Health + Delta, 0.f, MaxHealth);

	return Health - OldHealth;
}

void ASurvivalCharacter::OnRep_Health(float OldHealth)
{
	OnHealthModified(Health - OldHealth);
}

void ASurvivalCharacter::StartFire()
{
	BeginMeleeAttack();
}

void ASurvivalCharacter::StopFire()
{
}

void ASurvivalCharacter::BeginMeleeAttack()
{
	if (GetWorld()->TimeSince(LastMeleeAttackTime) > MeleeAttackMontage->GetPlayLength())
	{
		FHitResult Hit;
		FCollisionShape Shape = FCollisionShape::MakeSphere(15.f);

		FVector StartTrace = CameraComponent->GetComponentLocation();
		FVector EndTrace = (CameraComponent->GetComponentRotation().Vector() * MeleeAttackDistance) + StartTrace;
	
		FCollisionQueryParams QueryParams = FCollisionQueryParams("MeleeSweep", false, this);

		PlayAnimMontage(MeleeAttackMontage);

		if (GetWorld()->SweepSingleByChannel(Hit, StartTrace, EndTrace, FQuat(), COLLISION_WEAPON, Shape, QueryParams))
		{
			UE_LOG(LogTemp, Warning, TEXT("hit a punch"));

			if (ASurvivalCharacter* HitPlayer = Cast<ASurvivalCharacter>(Hit.GetActor()))
			{
				if (ASurvivalPlayerController* PC = Cast<ASurvivalPlayerController>(GetController()))
				{
					PC->OnHitPlayer();
				}
			}
		}

		ServerProcessMeleeHit(Hit);

		LastMeleeAttackTime = GetWorld()->GetTimeSeconds();
	}
}

void ASurvivalCharacter::ServerProcessMeleeHit_Implementation(const FHitResult& MeleeHit)
{
	

	if (GetWorld()->TimeSince(LastMeleeAttackTime) > MeleeAttackMontage->GetPlayLength() && (GetActorLocation() - MeleeHit.ImpactPoint).Size() <= MeleeAttackDistance)
	{
		MulticastPlayMeleeFX();

		UGameplayStatics::ApplyPointDamage(MeleeHit.GetActor(), MeleeAttackDamage, (MeleeHit.TraceStart - MeleeHit.TraceEnd).GetSafeNormal(), MeleeHit, GetController(), this, UMeleeDamage::StaticClass());

	}
	LastMeleeAttackTime = GetWorld()->GetTimeSeconds();
}

void ASurvivalCharacter::MulticastPlayMeleeFX_Implementation()
{
	if (!IsLocallyControlled())
	{
		PlayAnimMontage(MeleeAttackMontage);
	}
}

void ASurvivalCharacter::Suicide(FDamageEvent const& DamageEvent, const AActor* DamageCauser)
{
	Killer = this;
	OnRep_Killer();
}

void ASurvivalCharacter::KilledByPlayer(FDamageEvent const& DamageEvent, class ASurvivalCharacter* Character, const AActor* DamageCauser)
{
	Killer = Character;
	OnRep_Killer();
}

void ASurvivalCharacter::OnRep_Killer()
{
	SetLifeSpan(20.f);

	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	GetMesh()->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
	SetReplicateMovement(false);


	LootPlayerInteraction->Activate();

	if (HasAuthority())
	{
		TArray<UEquippableItem*> EquippedInvItems;
		EquippedItems.GenerateValueArray(EquippedInvItems);

		for (auto& Equippable : EquippedInvItems)
		{
			Equippable->SetEquipped(false);
		}
	}

	if (IsLocallyControlled())
	{
		SpringArmComponent->TargetArmLength = 500.f;
		bUseControllerRotationPitch = true;

		if (ASurvivalPlayerController* PC = Cast<ASurvivalPlayerController>(GetController()))
		{
			PC->ShowDeathScreen(Killer);
		}
	}
}


void ASurvivalCharacter::SpawnBuilding()
{
	if (bInBuildMode && Builder)
	{
		Builder->SpawnBuilding();
	}
}

void ASurvivalCharacter::DestroyBuildingInstance()
{
	if (bInBuildMode && Builder)
	{
		Builder->DestroyBuilding(PerformLineTrace());
	}
}

void ASurvivalCharacter::SelectHotbar(float val)
{
	if (val != 0.f)
	{
		if (val > 0.f)
		{
			SelectedHotbarIndex == 9 ? SelectedHotbarIndex = 0 : SelectedHotbarIndex++;
			SelectedRecipeIndex == PlayerCrafter->GetRecipes().Num() - 1 ? SelectedRecipeIndex = 0 : SelectedRecipeIndex++;
		}
		else
		{
			SelectedHotbarIndex == 0 ? SelectedHotbarIndex = 9 : SelectedHotbarIndex--;
			SelectedRecipeIndex == 0 ? SelectedRecipeIndex = PlayerCrafter->GetRecipes().Num() - 1 : SelectedRecipeIndex--;
		}
		PlayerInventory->OnInventoryUpdated.Broadcast();
		PlayerCrafter->OnCraftingUpdated.Broadcast();
	}
}

void ASurvivalCharacter::SelectRecipe(float val)
{
	if (val != 0.f && PlayerCrafter)
	{
		if (val > 0.f)
		{
			SelectedRecipeIndex == PlayerCrafter->GetRecipes().Num() - 1 ? SelectedRecipeIndex = 0 : SelectedRecipeIndex++;
		}
		else
		{
			SelectedRecipeIndex == 0 ? SelectedRecipeIndex = PlayerCrafter->GetRecipes().Num() - 1 : SelectedRecipeIndex--;
		}
		PlayerCrafter->OnCraftingUpdated.Broadcast();
	}
}

void ASurvivalCharacter::MoveForward(float Val)
{
	if (Val != 0.f)
	{
		AddMovementInput(GetActorForwardVector(), Val);
	}
}

void ASurvivalCharacter::MoveRight(float Val)
{
	if (Val != 0.f)
	{
		AddMovementInput(GetActorRightVector(), Val);
	}
}

void ASurvivalCharacter::LookUp(float Val)
{
	if (Val != 0.f)
	{
		AddControllerPitchInput(Val);
	}
}

void ASurvivalCharacter::Turn(float Val)
{
	if (Val != 0.f)
	{
		AddControllerYawInput(Val);
	}
}

void ASurvivalCharacter::StartCrouching()
{
	Crouch();
}

void ASurvivalCharacter::StopCrouching()
{
	UnCrouch();
}

FHitResult ASurvivalCharacter::PerformLineTrace(float Distance, bool DrawDebug)
{
	FVector Start = CameraComponent->GetComponentLocation();
	FVector End = Start + CameraComponent->GetForwardVector() * Distance;

	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params);

	if (DrawDebug)
	{
		DrawDebugLine(GetWorld(), Start, End, FColor::Red);
	}

	return HitResult;
}



void ASurvivalCharacter::SetBuildMode(bool Enabled)
{
	bInBuildMode = Enabled;
	if (Builder)
	{
		Builder->SetActorHiddenInGame(!bInBuildMode);
	}
}

void ASurvivalCharacter::CycleBuildingMsesh()
{
	if (Builder && bInBuildMode)
	{
		if (++CurrentBuildingIndex == Buildings.Num())
			CurrentBuildingIndex = 0;
		Builder->SetBuildingType(Buildings[CurrentBuildingIndex]);
		//Builder->Set();
	}
}


// Called every frame
void ASurvivalCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GetWorld()->TimeSince(InteractionData.LastInteractionCheckTime) > InteractionCheckFrequency)
	{
		PerformInteractionCheck();
	}
	
	if (bInBuildMode && Builder)
	{
		Builder->SetBuildPosition(PerformLineTrace(650.0f, false), GetActorQuat());
	}

}

void ASurvivalCharacter::Restart()
{
	Super::Restart();

	if (ASurvivalPlayerController* PC = Cast<ASurvivalPlayerController>(GetController()))
	{
		PC->ShowIngameUI();
	}
}

float ASurvivalCharacter::TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);

	const float DamageDealt = ModifyHealth(-Damage);

	if (Health <= 0.f)
	{
		if (ASurvivalCharacter* KillerCharacter = Cast<ASurvivalCharacter>(DamageCauser->GetOwner()))
		{
			KilledByPlayer(DamageEvent, KillerCharacter, DamageCauser);
		}
		else
		{
			Suicide(DamageEvent, DamageCauser);
		}
	}


	return DamageDealt;
}

void ASurvivalCharacter::SetLootSource(UInventoryComponent* NewLootSource)
{
	if (NewLootSource && NewLootSource->GetOwner())
	{
		NewLootSource->GetOwner()->OnDestroyed.AddUniqueDynamic(this, &ASurvivalCharacter::OnLootSourceOwnerDestroyed);
	}

	if (HasAuthority())
	{
		if (NewLootSource)
		{
			if (ASurvivalCharacter* Character = Cast<ASurvivalCharacter>(NewLootSource->GetOwner()))
			{
				Character->SetLifeSpan(120.f);
			}
		}
		LootSource = NewLootSource;
		OnRep_LootSource();
	}
	else
	{
		ServerSetLootSource(NewLootSource);
	}
}

bool ASurvivalCharacter::IsLooting() const
{
	return LootSource != nullptr;
}



void ASurvivalCharacter::BeginLootingPlayer(ASurvivalCharacter* Character)
{
	if (Character)
	{
		Character->SetLootSource(PlayerInventory);
	}
}

void ASurvivalCharacter::OnLootSourceOwnerDestroyed(AActor* DestroyedActor)
{
	if (HasAuthority() && LootSource && DestroyedActor == LootSource->GetOwner())
	{
		ServerSetLootSource(nullptr);
	}
}

void ASurvivalCharacter::OnRep_LootSource()
{
	if (ASurvivalPlayerController* PC = Cast<ASurvivalPlayerController>(GetController()))
	{
		if (PC->IsLocalController())
		{
			if (LootSource)
			{
				PC->ShowLootMenu(LootSource);
			}
			else
			{
				PC->HideLootMenu();
			}

		}
	}
}

void ASurvivalCharacter::LootItem(UItem* ItemToGive)
{
	if (HasAuthority())
	{
		if (PlayerInventory && LootSource && ItemToGive && LootSource->HasItem(ItemToGive->GetClass(), ItemToGive->GetQuantity()))
		{
			const FItemAddResult AddResult = PlayerInventory->TryAddItem(ItemToGive);

			if (AddResult.AmountGiven > 0)
			{
				LootSource->ConsumeItem(ItemToGive, AddResult.AmountGiven);
			}
			else
			{
				//Tell player why they couldn't loot the item
				if (ASurvivalPlayerController* PC = Cast<ASurvivalPlayerController>(GetController()))
				{
					PC->ClientShowNotification(AddResult.ErrorText);
				}
			}
		}
	}
	else
	{
		ServerLootItem(ItemToGive);
	}
}

void ASurvivalCharacter::LootItemAt(UItem* ItemToGive, int32 from, int32 to)
{
	if (HasAuthority())
	{
		if (PlayerInventory && LootSource && ItemToGive && LootSource->HasItem(ItemToGive->GetClass(), ItemToGive->GetQuantity()))
		{
			LootSource->SwapItem(PlayerInventory->SwapItem(ItemToGive, to),from);
		}
	}
	else
	{
		ServerLootItemAt(ItemToGive, from, to);
	}
}

void ASurvivalCharacter::ServerLootItemAt_Implementation(UItem* ItemToGive, int32 from, int32 to)
{
	LootItemAt(ItemToGive, from, to);
}

void ASurvivalCharacter::DepositItemAt(UItem* ItemToGive, int32 from, int32 to)
{
	if (HasAuthority())
	{
		if (PlayerInventory && LootSource && ItemToGive && PlayerInventory->HasItem(ItemToGive->GetClass(), ItemToGive->GetQuantity()))
		{
			PlayerInventory->SwapItem(LootSource->SwapItem(ItemToGive, to), from);
		}
	}
	else
	{
		ServerDepositItemAt(ItemToGive, from, to);
	}
}

void ASurvivalCharacter::SwapItem(UItem* ItemToSwap, int32 from, int32 to)
{
	if (HasAuthority())
	{
		if (ItemToSwap && ItemToSwap->OwningInventory && ItemToSwap->OwningInventory->HasItem(ItemToSwap->GetClass(), ItemToSwap->GetQuantity()))
		{
			ItemToSwap->OwningInventory->SwapItem(ItemToSwap->OwningInventory->SwapItem(ItemToSwap, to), from);
		}
	}
	else
	{
		ServerSwapItem(ItemToSwap, from, to);
	}
}

void ASurvivalCharacter::ServerSwapItem_Implementation(UItem* ItemToSwap, int32 from, int32 to)
{
	SwapItem(ItemToSwap, from, to);
}

void ASurvivalCharacter::ServerDepositItemAt_Implementation(UItem* ItemToGive, int32 from, int32 to)
{
	DepositItemAt(ItemToGive, from, to);
}

void ASurvivalCharacter::DepositItem(UItem* ItemToGive)
{
	if (HasAuthority())
	{
		if (PlayerInventory && LootSource && ItemToGive && PlayerInventory->HasItem(ItemToGive->GetClass(), ItemToGive->GetQuantity()))
		{
			const FItemAddResult AddResult = LootSource->TryAddItem(ItemToGive); //TODO

			if (AddResult.AmountGiven > 0)
			{
				PlayerInventory->ConsumeItem(ItemToGive, AddResult.AmountGiven);
			}
			else
			{
				//Tell player why they couldn't loot the item
				if (ASurvivalPlayerController* PC = Cast<ASurvivalPlayerController>(GetController()))
				{
					PC->ClientShowNotification(AddResult.ErrorText);
				}
			}
		}
	}
	else
	{
		ServerDepositItem(ItemToGive);
	}
}

void ASurvivalCharacter::ServerDepositItem_Implementation(UItem* ItemToLoot)
{
	DepositItem(ItemToLoot);
}

bool ASurvivalCharacter::ServerDepositItem_Validate(UItem* ItemToLoot)
{
	return true;
}

void ASurvivalCharacter::ServerLootItem_Implementation(UItem* ItemToLoot)
{
	LootItem(ItemToLoot);
}

bool ASurvivalCharacter::ServerLootItem_Validate(UItem* ItemToLoot)
{
	return true;
}

void ASurvivalCharacter::ServerSetLootSource_Implementation(UInventoryComponent* NewLootSource)
{
	SetLootSource(NewLootSource);
}

bool ASurvivalCharacter::ServerSetLootSource_Validate(UInventoryComponent* NewLootSource)
{
	return true;
}

void ASurvivalCharacter::PerformInteractionCheck()
{

	if (GetController() == nullptr)
	{
		return;
	}

	InteractionData.LastInteractionCheckTime = GetWorld()->GetTimeSeconds();

	FVector EyesLoc;
	FRotator EyesRot;

	GetController()->GetPlayerViewPoint(EyesLoc, EyesRot);

	FVector TraceStart = EyesLoc;
	FVector TraceEnd = (EyesRot.Vector() * InteractionCheckDistance) + TraceStart;
	FHitResult TraceHit;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(TraceHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		//Check if we hit an interactable object
		if (TraceHit.GetActor())
		{
			if (UInteractionComponent* InteractionComponent = Cast<UInteractionComponent>(TraceHit.GetActor()->GetComponentByClass(UInteractionComponent::StaticClass())))
			{
				float Distance = (TraceStart - TraceHit.ImpactPoint).Size();
				if (InteractionComponent != GetInteractable() && Distance <= InteractionComponent->InteractionDistance)
				{
					FoundNewInteractable(InteractionComponent);
				}
				else if (Distance > InteractionComponent->InteractionDistance && GetInteractable())
				{
					CouldntFindInteractable();
				}

				return;
			}
		}
	}

	CouldntFindInteractable();

}

void ASurvivalCharacter::CouldntFindInteractable()
{
	if (GetWorldTimerManager().IsTimerActive(TimerHandle_Interact))
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_Interact);
	}

	//Tell the interactable we've stopped focusing on it, and clear the current interactable
	if (UInteractionComponent* Interactable = GetInteractable())
	{
		Interactable->EndFocus(this);

		if (InteractionData.bInteractHeld)
		{
			EndInteract();
		}
	}

	InteractionData.ViewedInteractionComponent = nullptr;
}

void ASurvivalCharacter::FoundNewInteractable(UInteractionComponent* Interactable)
{
	EndInteract();

	if (UInteractionComponent* OldInteractable = GetInteractable())
	{
		OldInteractable->EndFocus(this);
	}

	InteractionData.ViewedInteractionComponent = Interactable;
	Interactable->BeginFocus(this);
	
}

void ASurvivalCharacter::BeginInteract()
{
	if(!HasAuthority())
	{
		ServerBeginInteract();
	}

	/**As an optimization, the server only checks that we're looking at an item once we begin interacting with it.
	This saves the server doing a check every tick for an interactable Item. The exception is a non-instant interact.
	In this case, the server will check every tick for the duration of the interact*/
	if (HasAuthority())
	{
		PerformInteractionCheck();
	}

	InteractionData.bInteractHeld = true;

	if (UInteractionComponent* Interactable = GetInteractable())
	{
		Interactable->BeginInteract(this);

		if (FMath::IsNearlyZero(Interactable->InteractionTime))
		{
			Interact();
		}
		else
		{
			GetWorldTimerManager().SetTimer(TimerHandle_Interact, this, &ASurvivalCharacter::Interact, Interactable->InteractionTime, false);
		}
	}
	
}

void ASurvivalCharacter::EndInteract()
{
	if (!HasAuthority())
	{
		ServerEndInteract();
	}

	InteractionData.bInteractHeld = false;

	GetWorldTimerManager().ClearTimer(TimerHandle_Interact);

	if (UInteractionComponent* Interactable = GetInteractable())
	{
		Interactable->EndInteract(this);
	}
}

void ASurvivalCharacter::Interact()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_Interact);

	if (UInteractionComponent* Interactable = GetInteractable())
	{
		Interactable->Interact(this);
	}
}

void ASurvivalCharacter::ServerEndInteract_Implementation()
{
	EndInteract();
}

bool ASurvivalCharacter::ServerEndInteract_Validate()
{
	return true;
}

void ASurvivalCharacter::ServerBeginInteract_Implementation()
{
	BeginInteract();
}

bool ASurvivalCharacter::ServerBeginInteract_Validate()
{
	return true;
}


// Called to bind functionality to input
void ASurvivalCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ASurvivalCharacter::StartFire);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ASurvivalCharacter::StopFire);

	PlayerInputComponent->BindAxis("MoveForward", this, &ASurvivalCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASurvivalCharacter::MoveRight);
	
	PlayerInputComponent->BindAxis("LookUp", this, &ASurvivalCharacter::LookUp);
	PlayerInputComponent->BindAxis("Turn", this, &ASurvivalCharacter::Turn);

	PlayerInputComponent->BindAxis("ScrollSelect", this, &ASurvivalCharacter::SelectHotbar);
	PlayerInputComponent->BindAxis("SelectRecipe", this, &ASurvivalCharacter::SelectRecipe);


	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &ASurvivalCharacter::BeginInteract);
	PlayerInputComponent->BindAction("Interact", IE_Released, this, &ASurvivalCharacter::EndInteract);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ASurvivalCharacter::StartCrouching);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &ASurvivalCharacter::StopCrouching);

}

#undef LOCTEXT_NAMESPACE
