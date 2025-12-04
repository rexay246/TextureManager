// Copyright Epic Games, Inc. All Rights Reserved.

#include "Testing.h"

#define LOCTEXT_NAMESPACE "FTestingModule"

void FTestingModule::StartupModule()
{
    UE_LOG(LogTemp, Warning, TEXT("Testing module loaded"));

    if (!GEditor)
    {
        UE_LOG(LogTemp, Error, TEXT("GEditor IS NULL!"));
        return;
    }

    UImportSubsystem* ImportSubsystem = GEditor->GetEditorSubsystem<UImportSubsystem>();

    if (!ImportSubsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("ImportSubsystem is NULL!"));
        return;
    }

    ImportSubsystem->OnAssetPostImport.AddRaw(
        this, &FTestingModule::OnAssetImported
    );

    UE_LOG(LogTemp, Warning, TEXT("Delegate bound successfully"));
}

void FTestingModule::OnAssetImported(UFactory* Factory, UObject* Imported)
{
    UE_LOG(LogTemp, Warning, TEXT("IMPORT FIRED for %s"),
        Imported ? *Imported->GetName() : TEXT("<null>"));

    if (Imported->IsA<UTexture>())
    {
        UE_LOG(LogTemp, Warning, TEXT("     --> This is a texture"));
    }
}

void FTestingModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FTestingModule, Testing)