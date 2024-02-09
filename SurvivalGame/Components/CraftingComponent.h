// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CraftingComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCraftingUpdated);


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SURVIVALGAME_API UCraftingComponent : public UActorComponent
{
	GENERATED_BODY()

	friend URecipe;

public:	
	// Sets default values for this component's properties
	UCraftingComponent();

	UPROPERTY(BlueprintAssignable, Category = "Crafting")
	FOnCraftingUpdated OnCraftingUpdated;

	UFUNCTION(BlueprintCallable, Category = "Crafting")
	URecipe* AddRecipe(class URecipe* recipe);

	UFUNCTION(BlueprintCallable, Category = "Crafting")
	bool HasMaterials(class URecipe* recipe);

	UFUNCTION(BlueprintCallable, Category = "Crafting")
	bool Craft(class URecipe* recipe);

	UFUNCTION(Server, Reliable)
	void ServerCraft(class URecipe* recipe);


	UFUNCTION(BlueprintPure, Category = "Crafting")
	FORCEINLINE TArray<class URecipe*> GetRecipes() const { return Recipes; }

	UPROPERTY(BlueprintReadOnly, Category = "Crafting", meta = (ExposeOnSpawn = true))
	class UInventoryComponent* InputInventory;

	UPROPERTY(BlueprintReadOnly, Category = "Crafting", meta = (ExposeOnSpawn = true))
	class UInventoryComponent* OutputInventory;

	/**needed this because the pickups use a blueprint base class*/
	UPROPERTY(EditDefaultsOnly, Category = "Item")
	TSubclassOf<class APickup> PickupClass;


protected:

	UPROPERTY(ReplicatedUsing = OnRep_Crafting)
	TArray<class URecipe*> Recipes;

	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags) override;


private:

	UFUNCTION()
	void OnRep_Crafting();

	void MarkArrayDirtyForReplication();

	UPROPERTY(Replicated)
	int32 ReplicatedRecipesKey = 0;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
