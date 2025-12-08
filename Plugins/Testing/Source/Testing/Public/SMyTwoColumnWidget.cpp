#include "SMyTwoColumnWidget.h"

#include "TexturePresetAsset.h"
#include "TexturePresetLibrary.h"
#include "TexturePresetUserData.h"

#include "Engine/Texture2D.h"
#include "Engine/Texture.h"
#include "Engine/AssetUserData.h"

#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Modules/ModuleManager.h"
#include "AssetRegistry/AssetRegistryModule.h"

#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"

#include "Editor.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "Misc/MessageDialog.h"
#include "UObject/UObjectGlobals.h" 
#include "UObject/UnrealType.h"

// ---------- Construct / Destruct ----------

void SMyTwoColumnWidget::Construct(const FArguments& InArgs)
{
	ActiveTab = ENavigationTab::Files;
	bPendingPresetChange = false;

	InitializePropertyWatcher();

	// Use injected DetailsView if provided, otherwise create one
	if (InArgs._DetailsViewWidget.IsValid())
	{
		DetailsView = InArgs._DetailsViewWidget;
	}
	else
	{
		FPropertyEditorModule& PropertyEditorModule =
			FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

		FDetailsViewArgs DetailsArgs;
		DetailsArgs.bUpdatesFromSelection = false;
		DetailsArgs.bAllowSearch = true;
		DetailsArgs.bHideSelectionTip = true;
		DetailsArgs.bLockable = false;

		DetailsView = PropertyEditorModule.CreateDetailView(DetailsArgs);
	}

	RefreshTextureList();
	RefreshPresetList();

	ChildSlot
		[
			SNew(SSplitter)
				+ SSplitter::Slot()
				.Value(0.35f)
				[
					BuildLeftColumn()
				]
				+ SSplitter::Slot()
				.Value(0.65f)
				[
					BuildRightColumn()
				]
		];
}

SMyTwoColumnWidget::~SMyTwoColumnWidget()
{
	ShutdownPropertyWatcher();
}

// ---------- Keyboard: Ctrl+S ----------

FReply SMyTwoColumnWidget::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::S && InKeyEvent.IsControlDown())
	{
		return OnSaveButtonClicked();
	}

	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

// ---------- UI Construction ----------

TSharedRef<SWidget> SMyTwoColumnWidget::BuildLeftColumn()
{
	return
		SNew(SBorder)
		.Padding(4.f)
		[
			SNew(SVerticalBox)

				// Tabs: Files / Presets (now on the LEFT)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SSegmentedControl<ENavigationTab>)
						.Value(this, &SMyTwoColumnWidget::GetActiveTab)
						.OnValueChanged(this, &SMyTwoColumnWidget::OnTabChanged)
						+ SSegmentedControl<ENavigationTab>::Slot(ENavigationTab::Files)
						.Text(NSLOCTEXT("TextureManager", "FilesTab", "Files"))
						+ SSegmentedControl<ENavigationTab>::Slot(ENavigationTab::Presets)
						.Text(NSLOCTEXT("TextureManager", "PresetsTab", "Presets"))
				]

				// Body switched by tab: Files list / Presets list
				+ SVerticalBox::Slot()
				.FillHeight(1.f)
				.Padding(0.f, 4.f)
				[
					SNew(SWidgetSwitcher)
						.WidgetIndex_Lambda([this]()
							{
								return (ActiveTab == ENavigationTab::Files) ? 0 : 1;
							})

						// Files list
						+ SWidgetSwitcher::Slot()
						[
							BuildFilesList()
						]

						// Presets list
						+ SWidgetSwitcher::Slot()
						[
							BuildPresetsList()
						]
				]
		];
}

TSharedRef<SWidget> SMyTwoColumnWidget::BuildFilesList()
{
	SAssignNew(TextureListView, SListView<FTextureItem>)
		.ListItemsSource(&TextureItems)
		.SelectionMode(ESelectionMode::Single)
		.OnGenerateRow(this, &SMyTwoColumnWidget::GenerateTextureRow)
		.OnSelectionChanged(this, &SMyTwoColumnWidget::OnTextureSelected);

	return TextureListView.ToSharedRef();
}

TSharedRef<SWidget> SMyTwoColumnWidget::BuildPresetsList()
{
	SAssignNew(PresetListView, SListView<FPresetItem>)
		.ListItemsSource(&PresetItems)
		.SelectionMode(ESelectionMode::Single)
		.OnGenerateRow(this, &SMyTwoColumnWidget::GeneratePresetRow)
		.OnSelectionChanged(this, &SMyTwoColumnWidget::OnPresetSelected);

	return PresetListView.ToSharedRef();
}

TSharedRef<SWidget> SMyTwoColumnWidget::BuildRightColumn()
{
	return
		SNew(SBorder)
		.Padding(4.f)
		[
			SNew(SVerticalBox)

				// Current preset + Save
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 4.f)
				[
					SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
								.Text(NSLOCTEXT("TextureManager", "CurrentPresetLabel", "Current Preset"))
						]
						+ SHorizontalBox::Slot()
						.FillWidth(1.f)
						.Padding(4.f, 0.f)
						.VAlign(VAlign_Center)
						[
							SAssignNew(PresetComboBox, STextComboBox)
								.OptionsSource(&PresetLabels)
								.OnSelectionChanged(this, &SMyTwoColumnWidget::OnPresetComboChanged)
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SButton)
								.Text(NSLOCTEXT("TextureManager", "SaveButton", "Save"))
								.OnClicked(this, &SMyTwoColumnWidget::OnSaveButtonClicked)
						]
				]

				// Single DetailsView (behavior controlled by ActiveTab + selection)
				+ SVerticalBox::Slot()
				.FillHeight(1.f)
				.Padding(0.f, 4.f)
				[
					DetailsView.ToSharedRef()
				]
		];
}

// ---------- Row generators ----------

TSharedRef<ITableRow> SMyTwoColumnWidget::GenerateTextureRow(FTextureItem Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	FText Label = FText::FromString(
		Item.IsValid() ? Item->GetName() : TEXT("<invalid>"));

	return
		SNew(STableRow<FTextureItem>, OwnerTable)
		[
			SNew(STextBlock).Text(Label)
		];
}

TSharedRef<ITableRow> SMyTwoColumnWidget::GeneratePresetRow(FPresetItem Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	FText Label = FText::FromString(TEXT("<Preset>"));

	if (Item.IsValid())
	{
		const UTexturePresetAsset* Preset = Item.Get();
		if (Preset)
		{
			if (!Preset->PresetName.IsNone())
			{
				Label = FText::FromName(Preset->PresetName);
			}
			else
			{
				Label = FText::FromString(Preset->GetName());
			}
		}
	}

	return
		SNew(STableRow<FPresetItem>, OwnerTable)
		[
			SNew(STextBlock).Text(Label)
		];
}

// ---------- Tabs & refresh ----------

ENavigationTab SMyTwoColumnWidget::GetActiveTab() const
{
	return ActiveTab;
}

void SMyTwoColumnWidget::OnTabChanged(ENavigationTab NewTab)
{
	ActiveTab = NewTab;
	SyncSelectionToDetails();
}

void SMyTwoColumnWidget::RefreshTextureList()
{
	TextureItems.Reset();

#if WITH_EDITOR
	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	FARFilter Filter;
	Filter.ClassPaths.Add(UTexture2D::StaticClass()->GetClassPathName());
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Add(FName(TEXT("/Game")));

	TArray<FAssetData> Assets;
	AssetRegistryModule.Get().GetAssets(Filter, Assets);

	for (const FAssetData& Data : Assets)
	{
		if (UTexture2D* Texture = Cast<UTexture2D>(Data.GetAsset()))
		{
			TextureItems.Add(Texture);
		}
	}
#endif

	if (TextureListView.IsValid())
	{
		TextureListView->RequestListRefresh();
	}
}

void SMyTwoColumnWidget::RefreshPresetList()
{
	PresetItems.Reset();
	PresetChoices.Reset();
	PresetLabels.Reset();

	// Index 0 = <NONE>
	NonePresetOption = MakeShared<FString>(TEXT("<NONE>"));
	PresetLabels.Add(NonePresetOption);
	PresetChoices.Add(nullptr);

#if WITH_EDITOR
	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	FARFilter Filter;
	Filter.ClassPaths.Add(UTexturePresetAsset::StaticClass()->GetClassPathName());
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Add(FName(TEXT("/Game")));

	TArray<FAssetData> Assets;
	AssetRegistryModule.Get().GetAssets(Filter, Assets);

	for (const FAssetData& Data : Assets)
	{
		if (UTexturePresetAsset* Preset = Cast<UTexturePresetAsset>(Data.GetAsset()))
		{
			PresetItems.Add(Preset);      // for list view

			PresetChoices.Add(Preset);    // for combo
			const FString Label =
				!Preset->PresetName.IsNone()
				? Preset->PresetName.ToString()
				: Preset->GetName();

			PresetLabels.Add(MakeShared<FString>(Label));
		}
	}
#endif

	if (PresetComboBox.IsValid())
	{
		PresetComboBox->RefreshOptions();
	}

	if (PresetListView.IsValid())
	{
		PresetListView->RequestListRefresh();
	}

	// Re-select current preset in combo if any
	SelectPresetInCombo(SelectedPreset.Get());
}

bool SMyTwoColumnWidget::PromptForPresetName(const FString& DefaultName, FString& OutName, const FString& FixedPath)
{
#if WITH_EDITOR
	// Default result
	OutName = DefaultName;

	TSharedRef<SWindow> Dialog =
		SNew(SWindow)
		.Title(NSLOCTEXT("TextureManager", "PresetNameWindowTitle", "Save Texture Preset"))
		.SizingRule(ESizingRule::Autosized)
		.SupportsMinimize(false)
		.SupportsMaximize(false)
		.AutoCenter(EAutoCenter::PreferredWorkArea);

	TSharedPtr<SEditableTextBox> NameTextBox;
	bool bConfirmed = false;

	const FText PathText = FText::FromString(FixedPath);
	TWeakPtr<SWindow> WeakDialog = Dialog;

	Dialog->SetContent(
		SNew(SBorder)
		.Padding(8.f)
		[
			SNew(SVerticalBox)

				// Locked path (display only)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 8.f)
				[
					SNew(STextBlock)
						.Text(FText::Format(
							NSLOCTEXT("TextureManager", "PresetPathLabel", "Path: {0}"),
							PathText))
				]

				// Name label
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 4.f)
				[
					SNew(STextBlock)
						.Text(NSLOCTEXT("TextureManager", "PresetNameLabel", "Preset Name:"))
				]

				// Name textbox
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(NameTextBox, SEditableTextBox)
						.Text(FText::FromString(DefaultName))
						.MinDesiredWidth(300.f)
				]

				// OK / Cancel buttons
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Right)
				.Padding(0.f, 8.f, 0.f, 0.f)
				[
					SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(4.f, 0.f)
						[
							SNew(SButton)
								.Text(NSLOCTEXT("TextureManager", "OK", "OK"))
								.OnClicked_Lambda([WeakDialog, &bConfirmed]()
									{
										bConfirmed = true;
										if (WeakDialog.IsValid())
										{
											WeakDialog.Pin()->RequestDestroyWindow();
										}
										return FReply::Handled();
									})
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(4.f, 0.f)
						[
							SNew(SButton)
								.Text(NSLOCTEXT("TextureManager", "Cancel", "Cancel"))
								.OnClicked_Lambda([WeakDialog]()
									{
										if (WeakDialog.IsValid())
										{
											WeakDialog.Pin()->RequestDestroyWindow();
										}
										return FReply::Handled();
									})
						]
				]
		]
	);

	if (GEditor)
	{
		GEditor->EditorAddModalWindow(Dialog);
	}

	if (!bConfirmed || !NameTextBox.IsValid())
	{
		return false; // cancelled
	}

	OutName = NameTextBox->GetText().ToString().TrimStartAndEnd();
	return !OutName.IsEmpty();
#else
	return false;
#endif
}

// ---------- Selection handling ----------

void SMyTwoColumnWidget::OnTextureSelected(FTextureItem Item, ESelectInfo::Type SelectInfo)
{
	SelectedTexture = Item;

	if (!Item.IsValid())
	{
		SelectedPreset = nullptr;
		ClearDetails();
		SelectPresetInCombo(nullptr);
		return;
	}

	UTexture2D* Texture = Item.Get();
	if (!Texture)
	{
		SelectedPreset = nullptr;
		ClearDetails();
		SelectPresetInCombo(nullptr);
		return;
	}

	// Look up assigned preset via user data
	UTexturePresetUserData* UserData =
		Cast<UTexturePresetUserData>(
			Texture->GetAssetUserDataOfClass(UTexturePresetUserData::StaticClass()));

	UTexturePresetAsset* Preset = UserData ? UserData->AssignedPreset : nullptr;
	SelectedPreset = Preset;

	ActiveTab = ENavigationTab::Files;
	SyncSelectionToDetails();
}

void SMyTwoColumnWidget::OnPresetSelected(FPresetItem Item, ESelectInfo::Type SelectInfo)
{
	SelectedPreset = Item;

	if (ActiveTab == ENavigationTab::Presets && DetailsView.IsValid())
	{
		if (SelectedPreset.IsValid())
		{
			DetailsView->SetObject(SelectedPreset.Get());
		}
		else
		{
			DetailsView->SetObject(nullptr);
		}
	}

	SelectPresetInCombo(SelectedPreset.Get());
}

void SMyTwoColumnWidget::SyncSelectionToDetails()
{
	if (!DetailsView.IsValid())
	{
		return;
	}

	if (ActiveTab == ENavigationTab::Files)
	{
		UTexture2D* Texture = SelectedTexture.Get();

		// NEW BEHAVIOR:
		//  - If a file is selected, always show its properties in the details panel.
		//  - The combo box still reflects whether a real preset is assigned or <NONE>.
		//
		//  Conceptually:
		//    <NONE> = "this file uses its own default/import settings",
		//    but we DO NOT attach a UTexturePresetAsset for that.
		if (Texture)
		{
			DetailsView->SetObject(Texture);
		}
		else
		{
			DetailsView->SetObject(nullptr);
		}
	}
	else // Presets tab
	{
		if (SelectedPreset.IsValid())
		{
			DetailsView->SetObject(SelectedPreset.Get());
		}
		else
		{
			DetailsView->SetObject(nullptr);
		}
	}

	// Combo box: nullptr preset still maps to index 0 => "<NONE>"
	SelectPresetInCombo(SelectedPreset.Get());
}

void SMyTwoColumnWidget::SelectPresetInCombo(UTexturePresetAsset* Preset)
{
	int32 FoundIndex = 0; // default to <NONE>

	for (int32 Index = 0; Index < PresetChoices.Num(); ++Index)
	{
		if (PresetChoices[Index].Get() == Preset)
		{
			FoundIndex = Index;
			break;
		}
	}

	if (PresetLabels.IsValidIndex(FoundIndex))
	{
		CurrentPresetOption = PresetLabels[FoundIndex];
	}
	else if (PresetLabels.Num() > 0)
	{
		CurrentPresetOption = PresetLabels[0];
	}
	else
	{
		CurrentPresetOption.Reset();
	}

	if (PresetComboBox.IsValid())
	{
		PresetComboBox->SetSelectedItem(CurrentPresetOption);
	}
}

void SMyTwoColumnWidget::ShowPresetDetails(UTexturePresetAsset* Preset)
{
	if (!DetailsView.IsValid())
	{
		return;
	}

	if (Preset)
	{
		DetailsView->SetObject(Preset);
	}
	else
	{
		DetailsView->SetObject(nullptr);
	}
}

void SMyTwoColumnWidget::ClearDetails()
{
	if (DetailsView.IsValid())
	{
		DetailsView->SetObject(nullptr);
	}
}

// ---------- Combo change ----------

void SMyTwoColumnWidget::OnPresetComboChanged(TSharedPtr<FString> NewSelection,ESelectInfo::Type SelectInfo)
{
	if (!NewSelection.IsValid())
	{
		return;
	}

	// Find index of selection by pointer (not by string contents)
	const int32 Index = PresetLabels.Find(NewSelection);
	if (Index == INDEX_NONE)
	{
		return;
	}

	UTexturePresetAsset* NewPreset = nullptr;
	if (PresetChoices.IsValidIndex(Index))
	{
		NewPreset = PresetChoices[Index].Get();
	}

	UTexture2D* Texture = SelectedTexture.Get();
	if (!Texture)
	{
		return;
	}

#if WITH_EDITOR
	if (!NewPreset)
	{
		// <NONE> selected: clear preset assignment but keep the texture's
		// current values as its "default" state.
		Texture->Modify();
		Texture->RemoveUserDataOfClass(UTexturePresetUserData::StaticClass());
		Texture->PostEditChange();
		Texture->MarkPackageDirty();

		SelectedPreset = nullptr;
		bPendingPresetChange = false;

		// Force the details panel to show the texture in its unassigned state
		if (ActiveTab == ENavigationTab::Files && DetailsView.IsValid())
		{
			DetailsView->SetObject(nullptr);
			DetailsView->SetObject(Texture);
		}

		// Combo box should stay on <NONE>
		SelectPresetInCombo(nullptr);
		return;
	}

	// Real preset selected:
	// 1) Attach preset to texture via user data
	// 2) Apply all preset settings to the texture
	TexturePresetLibrary::AssignPresetToTexture(NewPreset, Texture);
	TexturePresetLibrary::ApplyToTexture(NewPreset, Texture);
#endif

	SelectedPreset = NewPreset;
	bPendingPresetChange = false;

	// Files tab: show the texture's *updated* values immediately
	if (ActiveTab == ENavigationTab::Files && DetailsView.IsValid())
	{
		DetailsView->SetObject(nullptr);
		DetailsView->SetObject(Texture);
	}
	else
	{
		// Presets tab: keep showing the preset asset itself
		SyncSelectionToDetails();
	}
}

// ---------- Save button (and Ctrl+S) ----------

FReply SMyTwoColumnWidget::OnSaveButtonClicked()
{
#if !WITH_EDITOR
	return FReply::Handled();
#else
	// We always need a selected texture as the "source of truth"
	if (!SelectedTexture.IsValid())
	{
		return FReply::Handled();
	}

	UTexture2D* Texture = SelectedTexture.Get();
	if (!Texture)
	{
		return FReply::Handled();
	}

	// If nothing has changed there is nothing to save
	if (!bPendingPresetChange)
	{
		return FReply::Handled();
	}

	const FString DefaultPath = TEXT("/Game/TexturePresets");

	// Find the preset currently assigned to this texture (if any)
	UTexturePresetUserData* PresetUserData =
		Cast<UTexturePresetUserData>(
			Texture->GetAssetUserDataOfClass(UTexturePresetUserData::StaticClass()));

	UTexturePresetAsset* CurrentPreset = PresetUserData ? PresetUserData->AssignedPreset : nullptr;

	FString ChosenName;

	// ----------------------------
	// Case 1: No preset yet -> always create a new preset
	// ----------------------------
	if (!CurrentPreset)
	{
		const FString DefaultName = FString::Printf(TEXT("%s_Preset"), *Texture->GetName());

		// Ask the user for the preset name (path is locked)
		if (!PromptForPresetName(DefaultName, ChosenName, DefaultPath))
		{
			// User cancelled
			return FReply::Handled();
		}

		const FName AssetName(*ChosenName);

		if (UTexturePresetAsset* NewPreset =
			TexturePresetLibrary::CreatePresetAssetFromTexture(Texture, DefaultPath, AssetName))
		{
			// Also store the friendly display name
			NewPreset->PresetName = AssetName;

			TexturePresetLibrary::AssignPresetToTexture(NewPreset, Texture);
			SelectedPreset = NewPreset;
		}

		RefreshPresetList();
		SelectPresetInCombo(SelectedPreset.Get());
		bPendingPresetChange = false;
		return FReply::Handled();
	}

	// ----------------------------
	// Case 2: Texture already has a preset
	// Ask if we overwrite or create a new one
	// ----------------------------

	TArray<UTexture2D*> LinkedTextures =
		TexturePresetLibrary::GetAllTexturesUsingPreset(CurrentPreset);

	const FText Message = FText::Format(
		NSLOCTEXT("TexturePreset", "OverwriteOrNew",
			"The texture '{0}' is using preset '{1}'.\n"
			"This preset is currently used by {2} texture(s).\n\n"
			"Yes = Overwrite preset for all textures.\n"
			"No = Create a new preset only for this texture.\n"
			"Cancel = Do nothing."),
		FText::FromString(Texture->GetName()),
		FText::FromString(
			CurrentPreset->PresetName.IsNone()
			? CurrentPreset->GetName()
			: CurrentPreset->PresetName.ToString()),
		FText::AsNumber(LinkedTextures.Num()));

	const EAppReturnType::Type Response =
		FMessageDialog::Open(
			EAppMsgType::YesNoCancel,
			Message,
			NSLOCTEXT("TexturePreset", "SavePresetTitle", "Save Preset"));

	if (Response == EAppReturnType::Yes)
	{
		// Overwrite existing preset using its current name.
		// No extra "name" window here; we keep the original preset name.
		TexturePresetLibrary::UpdatePresetFromTexture(CurrentPreset, Texture);

		// Re-apply to all linked textures so they pick up the new settings
		for (UTexture2D* Other : LinkedTextures)
		{
			if (Other)
			{
				TexturePresetLibrary::ApplyToTexture(CurrentPreset, Other);
			}
		}

		SelectedPreset = CurrentPreset;
	}
	else if (Response == EAppReturnType::No)
	{
		// Create a brand-new preset only for this texture
		// This is where we show the "name your new preset" window.
		const FString DefaultName = FString::Printf(TEXT("%s_Custom"), *Texture->GetName());

		if (!PromptForPresetName(DefaultName, ChosenName, DefaultPath))
		{
			// User cancelled
			return FReply::Handled();
		}

		const FName AssetName(*ChosenName);

		if (UTexturePresetAsset* NewPreset =
			TexturePresetLibrary::CreatePresetAssetFromTexture(Texture, DefaultPath, AssetName))
		{
			NewPreset->PresetName = AssetName;

			TexturePresetLibrary::AssignPresetToTexture(NewPreset, Texture);
			SelectedPreset = NewPreset;
		}
	}
	// Cancel -> do nothing

	RefreshPresetList();
	SelectPresetInCombo(SelectedPreset.Get());
	bPendingPresetChange = false;

	return FReply::Handled();
#endif
}

// ---------- Property change watching ----------

void SMyTwoColumnWidget::OnAnyPropertyChanged(UObject* Object, FPropertyChangedEvent& Event)
{
	// Only care about changes on the currently selected texture
	if (!SelectedTexture.IsValid())
	{
		return;
	}

	if (Object != SelectedTexture.Get())
	{
		return;
	}

	// We have unsaved changes for this texture.
	// Do NOT touch combo-box labels here; it should always show the
	// texture's current preset or <NONE>.
	bPendingPresetChange = true;
}

void SMyTwoColumnWidget::InitializePropertyWatcher()
{
#if WITH_EDITOR
	if (!PropertyChangedHandle.IsValid())
	{
		PropertyChangedHandle =
			FCoreUObjectDelegates::OnObjectPropertyChanged.AddRaw(
				this, &SMyTwoColumnWidget::OnAnyPropertyChanged);
	}
#endif
}

void SMyTwoColumnWidget::ShutdownPropertyWatcher()
{
#if WITH_EDITOR
	if (PropertyChangedHandle.IsValid())
	{
		FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(PropertyChangedHandle);
		PropertyChangedHandle.Reset();
	}
#endif
}

void SMyTwoColumnWidget::AddImportedTexture(UTexture2D* NewTexture)
{
	if (!NewTexture)
	{
		return;
	}

	FTextureItem NewItem = NewTexture;

	// Avoid duplicates
	for (const FTextureItem& Existing : TextureItems)
	{
		if (Existing == NewItem)
		{
			return; // already tracked
		}
	}

	TextureItems.Add(NewItem);

	if (TextureListView.IsValid())
	{
		TextureListView->RequestListRefresh();
	}
}

void SMyTwoColumnWidget::SetSelectedTexture(UTexture2D* NewTexture)
{
	if (!NewTexture)
	{
		SelectedTexture = nullptr;
		SelectedPreset = nullptr;
		ClearDetails();
		SelectPresetInCombo(nullptr);
		return;
	}

	FTextureItem Item = NewTexture;

	// Ensure it's in the list
	bool bFound = false;
	for (const FTextureItem& Existing : TextureItems)
	{
		if (Existing == Item)
		{
			bFound = true;
			break;
		}
	}
	if (!bFound)
	{
		TextureItems.Add(Item);
		if (TextureListView.IsValid())
		{
			TextureListView->RequestListRefresh();
		}
	}

	// Drive the same path as a real user-click
	if (TextureListView.IsValid())
	{
		TextureListView->SetSelection(Item, ESelectInfo::Direct);
		TextureListView->RequestScrollIntoView(Item);
	}
	else
	{
		// Fallback if list view not yet constructed
		OnTextureSelected(Item, ESelectInfo::Direct);
	}
}