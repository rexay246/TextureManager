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
#include "Widgets/Input/SSearchBox.h"

#include "Editor.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "Misc/MessageDialog.h"
#include "UObject/UObjectGlobals.h" 
#include "UObject/UnrealType.h"

#include "Engine/Engine.h"

// ---------- Helper ------------------------

UTexture2D* SMyTwoColumnWidget::CloneTextureTransient(UTexture2D* Source)
{
	if (!Source) return nullptr;

	UTexture2D* Clone = DuplicateObject<UTexture2D>(
		Source,
		GetTransientPackage(),
		FName(*FString::Printf(TEXT("%s_Copy"), *Source->GetName()))
	);

	Clone->ClearFlags(RF_Standalone | RF_Public);
	Clone->SetFlags(RF_Transient);
	return Clone;
}

static UTexturePresetAsset* ClonePresetTransient(UTexturePresetAsset* Source)
{
	if (!Source) return nullptr;

	UTexturePresetAsset* Clone = DuplicateObject<UTexturePresetAsset>(
		Source,
		GetTransientPackage(),
		FName(*FString::Printf(TEXT("%s_Copy"), *Source->GetName()))
	);

	Clone->ClearFlags(RF_Standalone | RF_Public);
	Clone->SetFlags(RF_Transient);
	return Clone;
}

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

	DetailsView->OnFinishedChangingProperties().AddRaw(
		this, &SMyTwoColumnWidget::OnDetailsPropertyChanged
	);
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

				// Search Box (changes with tab)
				+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(2)
					[
						SNew(SWidgetSwitcher)
							.WidgetIndex_Lambda([this]()
								{
									return (ActiveTab == ENavigationTab::Files) ? 0 : 1;
								})

							// Files Search
							+ SWidgetSwitcher::Slot()
							[
								SAssignNew(FilesSearchBox, SSearchBox)
									.HintText(FText::FromString("Search Files..."))
									.OnTextChanged(this, &SMyTwoColumnWidget::OnFilesSearchChanged)
							]

							// Presets Search
							+ SWidgetSwitcher::Slot()
							[
								SAssignNew(PresetsSearchBox, SSearchBox)
									.HintText(FText::FromString("Search Presets..."))
									.OnTextChanged(this, &SMyTwoColumnWidget::OnPresetsSearchChanged)
							]
					]

				// Filter Preset dropdown
				+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(2)
					[
						SAssignNew(PresetFilterComboBox, STextComboBox)
							.OptionsSource(&FilterPresetLabels)
							.InitiallySelectedItem(CurrentFilterOption)
							.OnSelectionChanged(this, &SMyTwoColumnWidget::OnFilterComboChange)
							.Visibility(this, &SMyTwoColumnWidget::IsFilesChosen)
					]

				// Presets Save
				+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(2)
					.HAlign(HAlign_Center)
					[
						SNew(SButton)
							.Text(NSLOCTEXT("TextureManager", "NewButton", "Make New Preset"))
							.OnClicked(this, &SMyTwoColumnWidget::OnPresetNewButtonClicked)
							.Visibility_Lambda([this]()
								{
									return (ActiveTab != ENavigationTab::Files)
										? EVisibility::Visible
										: EVisibility::Collapsed;
								})
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
		.ListItemsSource(&FilteredTextureItems)
		.SelectionMode(ESelectionMode::Multi)
		.OnGenerateRow(this, &SMyTwoColumnWidget::GenerateTextureRow)
		.OnSelectionChanged(this, &SMyTwoColumnWidget::OnTextureSelected);

	return TextureListView.ToSharedRef();
}

TSharedRef<SWidget> SMyTwoColumnWidget::BuildPresetsList()
{
	SAssignNew(PresetListView, SListView<FPresetItem>)
		.ListItemsSource(&FilteredPresetItems)
		.SelectionMode(ESelectionMode::Multi)
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
								.IsEnabled(this, &SMyTwoColumnWidget::IsPresetComboEnabled)
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							//SNew(SButton)
							//	.Text(NSLOCTEXT("TextureManager", "SaveButton", "Save"))
							//	.OnClicked(this, &SMyTwoColumnWidget::OnSaveButtonClicked)
							SNew(SWidgetSwitcher)
								.WidgetIndex_Lambda([this]()
									{
										return (ActiveTab == ENavigationTab::Files) ? 0 : 1;
									})

								// Files Save
								+ SWidgetSwitcher::Slot()
								[
									SNew(SButton)
										.Text(NSLOCTEXT("TextureManager", "SaveButton", "Save"))
										.OnClicked(this, &SMyTwoColumnWidget::OnSaveButtonClicked)
										.IsEnabled_Lambda([this]()
											{
												return bPendingPresetChange || bPendingPropertyChange;
											})

								]

								// Presets Save
								+ SWidgetSwitcher::Slot()
								[
									SNew(SButton)
										.Text(NSLOCTEXT("TextureManager", "SaveButton", "Save"))
										.OnClicked(this, &SMyTwoColumnWidget::OnPresetSaveButtonClicked)
								]
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
	
	if (ActiveTab == ENavigationTab::Files) {
		if (SelectedPreset.Get()) {
			auto Preset = SelectedPreset.Get();
			for (auto Texture : Preset->Files) {
				TexturePresetLibrary::ApplyToTexture(Preset, Texture);
				Texture->PostEditChange();
			}
		}
	}
	else {
		if (SelectedTexture.Get()) {
			auto PTexture = SelectedTexture.Get();
			UTexturePresetUserData* UserData =
				Cast<UTexturePresetUserData>(
					PTexture->GetAssetUserDataOfClass(UTexturePresetUserData::StaticClass()));
			UTexturePresetAsset* Preset = UserData ? UserData->AssignedPreset : nullptr;
			for (auto Texture : Preset->Files) {
				TexturePresetLibrary::ApplyToTexture(Preset, Texture);
				Texture->PostEditChange();
			}
		}
	}

	SelectedPreset = nullptr;
	SelectedTexture = nullptr;
	TextureListView->ClearSelection();
	PresetListView->ClearSelection();
	SyncSelectionToDetails();
}

void SMyTwoColumnWidget::RefreshTextureList()
{
	//TextureItems.Reset();
	AllTextureItems.Reset();
	FilteredTextureItems.Reset();

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
			//TextureItems.Add(Texture);
			AllTextureItems.Add(Texture);
			FilteredTextureItems.Add(Texture);
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
	//PresetItems.Reset();
	AllPresetItems.Reset();
	FilteredPresetItems.Reset();

	PresetChoices.Reset();
	PresetLabels.Reset();

	FilterPresetLabels.Reset();
	FilterPresetChoices.Reset();

	// Index 0 = <NONE>
	NonePresetOption = MakeShared<FString>(TEXT("<NONE>"));
	PresetLabels.Add(NonePresetOption);
	PresetChoices.Add(nullptr);

	AllPresetOption = MakeShared<FString>(TEXT("All"));
	FilterPresetLabels.Add(AllPresetOption);
	FilterPresetLabels.Add(NonePresetOption);
	FilterPresetChoices.Add(nullptr);
	FilterPresetChoices.Add(nullptr);

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
			//PresetItems.Add(Preset);      // for list view
			AllPresetItems.Add(Preset);
			FilteredPresetItems.Add(Preset);
			PresetChoices.Add(Preset);    // for combo
			FilterPresetChoices.Add(Preset);
			const FString Label =
				!Preset->PresetName.IsNone()
				? Preset->PresetName.ToString()
				: Preset->GetName();

			PresetLabels.Add(MakeShared<FString>(Label));
			FilterPresetLabels.Add(MakeShared<FString>(Label));
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
	CurrentFilterOption = FilterPresetLabels[FilterIndex];
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
	if (SelectedTexture.Get()) {
		auto PTexture = SelectedTexture.Get();
		UTexturePresetUserData* UserData =
			Cast<UTexturePresetUserData>(
				PTexture->GetAssetUserDataOfClass(UTexturePresetUserData::StaticClass()));
		UTexturePresetAsset* Preset = UserData ? UserData->AssignedPreset : nullptr;
		for (auto Texture : Preset->Files) {
			TexturePresetLibrary::ApplyToTexture(Preset, Texture);
			Texture->PostEditChange();
		}
	}

	SelectedTexture = Item;
	bPendingPresetChange = false;
	bPendingPropertyChange = false;

	TArray<FTextureItem> SelectedItems;

	if (TextureListView.IsValid())
	{
		TextureListView->GetSelectedItems(SelectedItems);
	}

	const int32 NumSelected = SelectedItems.Num();
	if (NumSelected > 1) {
		bPendingPresetChange = true;
	}

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
	if (!Preset) {
		bPendingPresetChange = true;
	}
	TexturePresetLibrary::ApplyToTexture(Preset, Texture);
	Texture->PostEditChange();

	ActiveTab = ENavigationTab::Files;
	SyncSelectionToDetails();
}

void SMyTwoColumnWidget::OnPresetSelected(FPresetItem Item, ESelectInfo::Type SelectInfo)
{
	if (SelectedPreset.Get()) {
		auto Preset = SelectedPreset.Get();
		for (auto Texture : Preset->Files) {
			TexturePresetLibrary::ApplyToTexture(Preset, Texture);
			Texture->PostEditChange();
		}
	}

	SelectedPreset = Item;

	if (ActiveTab == ENavigationTab::Presets && DetailsView.IsValid())
	{
		if (SelectedPreset.IsValid())
		{
			PreviewPreset = ClonePresetTransient(SelectedPreset.Get());
			DetailsView->SetObject(PreviewPreset);
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
		 
		//TexturePresetLibrary::ApplyToTexture(SelectedPreset.Get(), Texture);
		if (Texture)
		{
			PreviewTexture = CloneTextureTransient(Texture);
			DetailsView->SetObject(PreviewTexture);
			//DetailsView->SetObject(Texture);
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
			PreviewPreset = ClonePresetTransient(SelectedPreset.Get());
			DetailsView->SetObject(PreviewPreset);
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

		//Texture->Modify();
		//Texture->RemoveUserDataOfClass(UTexturePresetUserData::StaticClass());
		//Texture->PostEditChange();
		//Texture->MarkPackageDirty();

		SelectedPreset = nullptr;
		bPendingPresetChange = true;

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

	if (SelectInfo == ESelectInfo::Direct) {
		return;
	}

	// Real preset selected:
	// 1) Attach preset to texture via user data
	// 2) Apply all preset settings to the texture
	 
	//TexturePresetLibrary::AssignPresetToTexture(NewPreset, Texture);

	//Texture->Modify();
	//TexturePresetLibrary::AssignPresetToTexture(NewPreset, Texture);
	TexturePresetLibrary::ApplyToTexture(NewPreset, Texture);
	Texture->PostEditChange();
	//TexturePresetLibrary::ApplyToTexture(NewPreset, Texture);

#endif

	SelectedPreset = NewPreset;
	bPendingPresetChange = true;

	// Files tab: show the texture's *updated* values immediately
	if (ActiveTab == ENavigationTab::Files && DetailsView.IsValid())
	{
		DetailsView->SetObject(nullptr);
		PreviewTexture = CloneTextureTransient(Texture);
		DetailsView->SetObject(PreviewTexture);
		//DetailsView->SetObject(Texture);
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
	SCOPE_CYCLE_COUNTER(STAT_TextureManager_OnSaveButtonClicked);

#if !WITH_EDITOR
	return FReply::Handled();
#else
	// We always need a selected texture as the "source of truth"
	if (!SelectedTexture.IsValid())
	{
		return FReply::Handled();
	}

	// Collect selected textures from the list view
	TArray<FTextureItem> SelectedItems;

	if (TextureListView.IsValid())
	{
		TextureListView->GetSelectedItems(SelectedItems);
	}

	UTexture2D* Texture = PreviewTexture;
	if (!Texture)
	{
		return FReply::Handled();
	}

	if (SelectedItems.Num() == 0 && SelectedTexture.IsValid())
	{
		SelectedItems.Add(SelectedTexture);
	}

	if (SelectedItems.Num() == 0)
	{
		// Nothing to apply to
		return FReply::Handled();
	}

	const FString DefaultPath = TEXT("/Game/TexturePresets");

	// Find the preset currently assigned to this texture (if any)
	UTexturePresetUserData* PresetUserData = 
		Cast<UTexturePresetUserData>(
			Texture->GetAssetUserDataOfClass(UTexturePresetUserData::StaticClass()));

	UTexturePresetAsset* CurrentPreset = SelectedPreset.Get(); //PresetUserData ? PresetUserData->AssignedPreset : nullptr;

	// If nothing has changed there is nothing to save
	if (!bPendingPresetChange && CurrentPreset && !bPendingPropertyChange)
	{
		return FReply::Handled();
	}

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

			//TexturePresetLibrary::AssignPresetToTexture(NewPreset, Texture);
			SelectedPreset = NewPreset;
			SaveFiles(SelectedItems);
		}

		RefreshPresetList();
		SelectPresetInCombo(SelectedPreset.Get());
		bPendingPresetChange = false;
		return FReply::Handled();
	}

	// ----------------------------
	// Changing Presets
	// ----------------------------

	TArray<UTexture2D*> LinkedTextures =
		TexturePresetLibrary::GetAllTexturesUsingPreset(CurrentPreset);

	if (bPendingPropertyChange) {
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
					Other->Modify();
					TexturePresetLibrary::AssignPresetToTexture(CurrentPreset, Other);
					TexturePresetLibrary::ApplyToTexture(CurrentPreset, Other);
					Other->PostEditChange();
				}
			}

			SelectedPreset = CurrentPreset;

			SelectedTexture.Get()->Modify();
			TexturePresetLibrary::AssignPresetToTexture(SelectedPreset.Get(), SelectedTexture.Get());
			TexturePresetLibrary::ApplyToTexture(SelectedPreset.Get(), SelectedTexture.Get());
			SelectedTexture.Get()->PostEditChange();
			//SaveFiles(SelectedItems);
		}
		else if (Response == EAppReturnType::No)
		{
			// Create a brand-new preset only for this texture
			// This is where we show the "name your new preset" window.
			const FString DefaultName = FString::Printf(TEXT("%s_Custom"), *Texture->GetName());

			if (PromptForPresetName(DefaultName, ChosenName, DefaultPath))
			{
				const FName AssetName(*ChosenName);

				if (UTexturePresetAsset* NewPreset =
					TexturePresetLibrary::CreatePresetAssetFromTexture(Texture, DefaultPath, AssetName))
				{
					NewPreset->PresetName = AssetName;

					SelectedPreset = NewPreset;
					//SaveFiles(SelectedItems);
					SelectedTexture.Get()->Modify();		
					TexturePresetLibrary::AssignPresetToTexture(SelectedPreset.Get(), SelectedTexture.Get());
					TexturePresetLibrary::ApplyToTexture(SelectedPreset.Get(), SelectedTexture.Get());
					SelectedTexture.Get()->PostEditChange();
				}
			}
		}
		// Cancel -> do nothing
		bPendingPropertyChange = false;
	}

	// ----------------------------
	// Changing the Preset
	// ----------------------------
	if (bPendingPresetChange) {
		SaveFiles(SelectedItems);
		bPendingPresetChange = false;
	}

	RefreshPresetList();
	SelectPresetInCombo(SelectedPreset.Get());
	return FReply::Handled();

#endif
}

FReply SMyTwoColumnWidget::OnPresetSaveButtonClicked()
{
	SCOPE_CYCLE_COUNTER(STAT_TextureManager_OnPresetSaveButtonClicked);

	UTexturePresetAsset* CurrentPreset = SelectedPreset.Get();

	TArray<UTexture2D*> LinkedTextures = CurrentPreset->Files;
		//TexturePresetLibrary::GetAllTexturesUsingPreset(CurrentPreset);

	const FText Message = FText::Format(
		NSLOCTEXT("TexturePreset", "OverwriteOrNew",
			"This preset is currently used by {0} texture(s).\n\n"
			"Yes = Save preset for all textures.\n"
			"No = Do nothing."),
		FText::AsNumber(LinkedTextures.Num()));

	const EAppReturnType::Type Response =
		FMessageDialog::Open(
			EAppMsgType::YesNo,
			Message,
			NSLOCTEXT("TexturePreset", "SavePresetTitle", "Save Preset"));

	if (Response == EAppReturnType::Yes)
	{
		TexturePresetLibrary::CopyProperties(PreviewPreset, CurrentPreset);
		CurrentPreset->MarkPackageDirty();

		for (UTexture2D* Other : LinkedTextures)
		{
			if (Other)
			{			
				Other->Modify();
				TexturePresetLibrary::AssignPresetToTexture(CurrentPreset, Other);
				TexturePresetLibrary::ApplyToTexture(CurrentPreset, Other);
				Other->PostEditChange();
			}
		}
	}

	return FReply::Handled();
}

FReply SMyTwoColumnWidget::OnPresetNewButtonClicked()
{
	FString ChosenName;
	const FString DefaultPath = TEXT("/Game/TexturePresets");
	const FString DefaultName = FString::Printf(TEXT("New_Copy"));

	// Ask the user for the preset name (path is locked)
	if (!PromptForPresetName(DefaultName, ChosenName, DefaultPath))
	{
		// User cancelled
		return FReply::Handled();
	}

	const FName AssetName(*ChosenName);

	if (UTexturePresetAsset* NewPreset =
		TexturePresetLibrary::CreatePresetAsset(DefaultPath, AssetName))
	{
	}

	RefreshPresetList();
	SelectPresetInCombo(SelectedPreset.Get());
	bPendingPresetChange = false;
	return FReply::Handled();
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
	//bPendingPresetChange = true;
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
	for (const FTextureItem& Existing : AllTextureItems)
	{
		if (Existing == NewItem)
		{
			return; // already tracked
		}
	}

	//TextureItems.Add(NewItem);
	AllTextureItems.Add(NewItem);
	FilteredTextureItems.Add(NewItem);

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
	for (const FTextureItem& Existing : AllTextureItems)
	{
		if (Existing == Item)
		{
			bFound = true;
			break;
		}
	}
	if (!bFound)
	{
		//TextureItems.Add(Item);
		AllTextureItems.Add(Item);
		FilteredTextureItems.Add(Item);
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

void SMyTwoColumnWidget::OnFilterComboChange(TSharedPtr<FString> NewSelection, ESelectInfo::Type)
{
	if (!NewSelection.IsValid())
	{
		return;
	}

	// Find index of selection by pointer (not by string contents)
	const int32 Index = FilterPresetLabels.Find(NewSelection);
	if (Index == INDEX_NONE)
	{
		return;
	}

	CurrentFilterOption = NewSelection;
	FilteredTextureItems.Empty();

	UTexturePresetAsset* FoundPreset =
		TexturePresetLibrary::FindPresetByName(*CurrentFilterOption);


	for (const FTextureItem& Item : AllTextureItems)
	{
		UTexturePresetUserData* UserData = Cast<UTexturePresetUserData>(
			Item->GetAssetUserDataOfClass(UTexturePresetUserData::StaticClass()));

		if (!Item.IsValid()) continue;

		if (FoundPreset == nullptr && NewSelection == NonePresetOption && (UserData == nullptr)) {
			FilteredTextureItems.Add(Item);
		}
		else if (FoundPreset != nullptr && (UserData != nullptr && UserData->AssignedPreset == FoundPreset))
		{
			FilteredTextureItems.Add(Item);
		}
		else if (FoundPreset == nullptr && NewSelection == AllPresetOption) {
			FilteredTextureItems.Add(Item);
		}
	}

	if (TextureListView.IsValid())
		TextureListView->RequestListRefresh();
}

void SMyTwoColumnWidget::OnFilesSearchChanged(const FText& InText)
{
	FilesSearchQuery = InText.ToString();
	FilteredTextureItems.Empty();

	for (const FTextureItem& Item : AllTextureItems)
	{
		UTexturePresetUserData* UserData = Cast<UTexturePresetUserData>(
			Item->GetAssetUserDataOfClass(UTexturePresetUserData::StaticClass()));

		if (!Item.IsValid()) continue;

		if (FilesSearchQuery.IsEmpty() || Item->GetName().Contains(FilesSearchQuery))
		{
			FilteredTextureItems.Add(Item);
		}
	}

	if (TextureListView.IsValid())
		TextureListView->RequestListRefresh();
}

void SMyTwoColumnWidget::OnPresetsSearchChanged(const FText& InText)
{
	PresetsSearchQuery = InText.ToString();
	FilteredPresetItems.Empty();

	for (const FPresetItem& Item : AllPresetItems)
	{
		if (!Item.IsValid()) continue;

		const FString Label = !Item->PresetName.IsNone()
			? Item->PresetName.ToString()
			: Item->GetName();

		if (PresetsSearchQuery.IsEmpty() || Label.Contains(PresetsSearchQuery))
		{
			FilteredPresetItems.Add(Item);
		}
	}

	if (PresetListView.IsValid())
		PresetListView->RequestListRefresh();
}

void SMyTwoColumnWidget::SaveFiles(TArray<FTextureItem> SelectedItems) {
	// If multiple, confirm with the user
	if (SelectedItems.Num() > 1)
	{
		UTexturePresetAsset* Preset = SelectedPreset.Get();
		const FString PresetLabel = Preset
			? (!Preset->PresetName.IsNone()
				? Preset->PresetName.ToString()
				: Preset->GetName())
			: TEXT("<Preset>");

		const FString Msg = FString::Printf(
			TEXT("Apply preset \"%s\" to %d textures?"),
			*PresetLabel,
			SelectedItems.Num());

		const EAppReturnType::Type Result = FMessageDialog::Open(
			EAppMsgType::YesNo,
			FText::FromString(Msg));

		if (Result != EAppReturnType::Yes)
		{
			return;
		}
	}

	// Apply preset to all selected textures
	for (const FTextureItem& Item : SelectedItems)
	{
		if (Item.IsValid())
		{
			UTexture2D* NewTexture = Item.Get();
			NewTexture->Modify();
			TexturePresetLibrary::AssignPresetToTexture(SelectedPreset.Get(), NewTexture);
			TexturePresetLibrary::ApplyToTexture(SelectedPreset.Get(), NewTexture);
			NewTexture->PostEditChange();
			//NewTexture->MarkPackageDirty();
		}
	}
}

void SMyTwoColumnWidget::OnDetailsPropertyChanged(const FPropertyChangedEvent& Event)
{
	bPendingPropertyChange = true;
	if (ActiveTab == ENavigationTab::Presets) {
		auto Preset = PreviewPreset;
		for (auto Texture : Preset->Files) {
			TexturePresetLibrary::ApplyToTexture(Preset, Texture);
			Texture->PostEditChange();
		}
	}
	else {
		auto PTexture = PreviewTexture;
		UTexturePresetAsset* Copy = ClonePresetTransient(SelectedPreset.Get());;
		TexturePresetLibrary::CaptureFromTexture(Copy, PTexture);
		for (auto Texture : Copy->Files) {
			TexturePresetLibrary::ApplyToTexture(Copy, Texture);
			Texture->PostEditChange();
		}
	}
}