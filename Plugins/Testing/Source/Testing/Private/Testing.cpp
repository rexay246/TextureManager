// Copyright Epic Games, Inc. All Rights Reserved.

#include "Testing.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "SMyTwoColumnWidget.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "FTestingModule"

void FTestingModule::StartupModule()
{
    // Load the Importing
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

    // Editor Tab
    //SomeObjectToEdit = NewObject<UObject>(GetTransientPackage(), UObject::StaticClass());

    FGlobalTabmanager::Get()->RegisterNomadTabSpawner("MyDetailsTab",
        FOnSpawnTab::CreateRaw(this, &FTestingModule::OnSpawnPluginTab))
        .SetDisplayName(FText::FromString("My Details Window"));
}

void FTestingModule::OnAssetImported(UFactory* Factory, UObject* Imported)
{
    UE_LOG(LogTemp, Warning, TEXT("IMPORT FIRED for %s"),
        Imported ? *Imported->GetName() : TEXT("<null>"));

    if (!Imported)
    {
        UE_LOG(LogTemp, Error, TEXT("Imported object is NULL!"));
        return;
    }

    // Check if the imported asset is a texture (or any other asset type)
    if (Imported->IsA<UTexture>())
    {
        UE_LOG(LogTemp, Warning, TEXT("     --> This is a texture"));

        // Set the imported object to be shown in the details panel
        SomeObjectToEdit = Imported;

        // Check if the tab is already open. If not, spawn it.
        if (!FGlobalTabmanager::Get()->FindExistingLiveTab(FName("MyDetailsTab")))
        {
            // Open the tab if it isn't already opened
            FGlobalTabmanager::Get()->TryInvokeTab(FName("MyDetailsTab"));
        }

        // If widget already exists, notify it of new texture (optional)
        if (ManagerWidget.IsValid())
        {
            ManagerWidget->SetSelectedTexture(Cast<UTexture2D>(Imported));
        }
    }
}

TSharedRef<SDockTab> FTestingModule::OnSpawnPluginTab(const FSpawnTabArgs& Args)
{
    // 1. Load Property Editor module
    FPropertyEditorModule& PropEditorModule =
        FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

    // 2. Create the details view (same widget used by Batch Edit window)
    FDetailsViewArgs ViewArgs;
    ViewArgs.bAllowSearch = true;
    ViewArgs.bShowPropertyMatrixButton = false;
    ViewArgs.bHideSelectionTip = false;
    ViewArgs.bShowOptions = true;
    ViewArgs.bShowScrollBar = true;

    TSharedPtr<IDetailsView> DetailsView = PropEditorModule.CreateDetailView(ViewArgs);

    // Example: Set the object to show in the details panel (replace with yours)
    if (SomeObjectToEdit)
    {
        DetailsView->SetObject(SomeObjectToEdit);
    }

    // 3. Build your custom two-column layout widget
    TSharedRef<SMyTwoColumnWidget> TwoColumnWidget =
        SNew(SMyTwoColumnWidget)
        .DetailsViewWidget(DetailsView);

    // 4. Return as a Nomad tab
    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            TwoColumnWidget
        ];
}

void FTestingModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FTestingModule, Testing)