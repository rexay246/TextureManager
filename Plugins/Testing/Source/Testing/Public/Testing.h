// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include "Subsystems/ImportSubsystem.h"
#include "Editor.h"

class FTestingModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	void OnAssetImported(UFactory* Factory, UObject* Imported);
	virtual void ShutdownModule() override;
};
