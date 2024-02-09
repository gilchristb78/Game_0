// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Recipe.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRecipeModified);

/**
 * 
 */
UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class SURVIVALGAME_API URecipe : public UObject
{
	GENERATED_BODY()
	
	

public:


	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Replicated, Category = "Recipe")
	AActor * Owner;

	URecipe();


	UPROPERTY(Replicated)
	int32 RepKey;

	UFUNCTION(BlueprintPure, Category = "Recipe")
	FORCEINLINE TArray<class UItem*> GetInputItems() const { return InputItems; }

	UFUNCTION(BlueprintPure, Category = "Recipe")
	FORCEINLINE TArray<class UItem*> GetOutputItems() const { return OutputItems; }

	UFUNCTION(BlueprintCallable, Category = "Recipe")
	void SetInputItems(const TArray<class UItem*>& NewInputItems);

	UFUNCTION(BlueprintCallable, Category = "Recipe")
	void SetOutputItems(const TArray<class UItem*>& NewOutputItems);

	/**Mark the object as needing replication. We must call this internally after modifying any replicated properties*/
	void MarkDirtyForReplication();

	UPROPERTY(BlueprintAssignable)
	FOnRecipeModified OnRecipeModified;

	UFUNCTION(BlueprintCallable, Category = "Recipe")
	bool AddInputFromClass(TSubclassOf<class UItem> ItemClass, const int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category = "Recipe")
	bool AddOutputFromClass(TSubclassOf<class UItem> ItemClass, const int32 Quantity = 1);

	UPROPERTY(ReplicatedUsing = OnRep_Items, EditDefaultsOnly, BlueprintReadWrite, Category = "Recipe")
	TArray<class UItem*> InputItems;

	UPROPERTY(ReplicatedUsing = OnRep_Items, EditDefaultsOnly, BlueprintReadWrite, Category = "Recipe")
	TArray<class UItem*> OutputItems;
	
protected:

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool IsSupportedForNetworking() const override;

private:

	UFUNCTION()
	void OnRep_Items();
};
