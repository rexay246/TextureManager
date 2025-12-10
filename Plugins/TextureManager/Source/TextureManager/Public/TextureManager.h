// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include "Subsystems/ImportSubsystem.h"
#include "Editor.h"
#include <SMyTwoColumnWidget.h>

class FTextureManagerModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	void OnAssetImported(UFactory* Factory, UObject* Imported);
	virtual void ShutdownModule() override;
	TSharedRef<SDockTab> OnSpawnPluginTab(const FSpawnTabArgs& Args);
	void RegisterMenus();
	void OnAssetsPreDelete(const TArray<UObject*>& Assets);

	UPROPERTY()
	UObject* SomeObjectToEdit = nullptr;

private:
	TSharedPtr< SMyTwoColumnWidget > ManagerWidget;
};
