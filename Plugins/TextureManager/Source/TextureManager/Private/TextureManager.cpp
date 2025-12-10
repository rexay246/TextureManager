// Copyright Epic Games, Inc. All Rights Reserved.

#include "TextureManager.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "SMyTwoColumnWidget.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "FTextureManagerModule"

void FTextureManagerModule::StartupModule()
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
            this, &FTextureManagerModule::OnAssetImported
        );

        UE_LOG(LogTemp, Warning, TEXT("Delegate bound successfully"));
    }

    // Editor Tab
    //SomeObjectToEdit = NewObject<UObject>(GetTransientPackage(), UObject::StaticClass());

    FGlobalTabmanager::Get()->RegisterNomadTabSpawner("TextureManager",
        FOnSpawnTab::CreateRaw(this, &FTextureManagerModule::OnSpawnPluginTab))
       .SetMenuType(ETabSpawnerMenuType::Hidden)
       .SetDisplayName(FText::FromString("Texture Manager"));

    UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FTextureManagerModule::RegisterMenus));
}

void FTextureManagerModule::OnAssetImported(UFactory* Factory, UObject* Imported)
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
        UClass* type = Imported->GetClass();
        if (Imported->IsA<UTexture2D>()) {
            UE_LOG(LogTemp, Warning, TEXT("     --> This is a texture2D"));
        }

        // Set the imported object to be shown in the details panel
        SomeObjectToEdit = Imported;


        // Check if the tab is already open. If not, spawn it.
        if (!FGlobalTabmanager::Get()->FindExistingLiveTab(FName("TextureManager")))
        {
            // Open the tab if it isn't already opened
            FGlobalTabmanager::Get()->TryInvokeTab(FName("TextureManager"));
        }

        // If widget already exists, notify it of new texture (optional)
        if (ManagerWidget.IsValid())
        {
            //ManagerWidget->SetSelectedTexture(Cast<UTexture2D>(Imported));
            // 1) Add it to the Files list (this refreshes the view)
            ManagerWidget->AddImportedTexture(Cast<UTexture2D>(Imported));

            // 2) Also make it the active selection / show it in details panel
            ManagerWidget->SetSelectedTexture(Cast<UTexture2D>(Imported));
        }
    }
}

TSharedRef<SDockTab> FTextureManagerModule::OnSpawnPluginTab(const FSpawnTabArgs& Args)
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
    DetailsView->SetObject(nullptr);

    // Build your custom two-column layout widget
    TSharedRef<SMyTwoColumnWidget> TwoColumnWidget =
        SNew(SMyTwoColumnWidget)
        .DetailsViewWidget(DetailsView);

    ManagerWidget = TwoColumnWidget;

    /*if (SomeObjectToEdit && SomeObjectToEdit->IsA<UTexture2D>())
    {
        ManagerWidget->SetSelectedTexture(Cast<UTexture2D>(SomeObjectToEdit));
    }*/

    // Return as a Nomad tab
    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            TwoColumnWidget
        ];
}

void FTextureManagerModule::RegisterMenus()
{
    UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");

    FToolMenuSection& Section = Menu->AddSection("TextureTools", FText::FromString("Texture Tools"));

    Section.AddMenuEntry(
        "Texture Manager",
        FText::FromString("Texture Manager"),
        FText::FromString("Manage texture presets"),
        FSlateIcon(),
        FUIAction(FExecuteAction::CreateLambda([]()
            {
                FGlobalTabmanager::Get()->TryInvokeTab(FName("TextureManager"));
            }))
    );
}

void FTextureManagerModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner("TextureManager");
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FTextureManagerModule, TextureManager)