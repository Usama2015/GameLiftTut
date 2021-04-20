// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuWidget.generated.h"


class UWebBrowser;
/**
 * 
 */
UCLASS()
class GAMELIFTTUT_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UMainMenuWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;


private:
	UPROPERTY()
		FString LoginUrl;

	UPROPERTY()
		UWebBrowser* WebBrowser;
};
