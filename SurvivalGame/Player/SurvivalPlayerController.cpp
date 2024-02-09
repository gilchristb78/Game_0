// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/SurvivalPlayerController.h"

void ASurvivalPlayerController::ClientShowNotification_Implementation(const FText& Message)
{
	ShowNotification(Message);
}

ASurvivalPlayerController::ASurvivalPlayerController()
{
}
