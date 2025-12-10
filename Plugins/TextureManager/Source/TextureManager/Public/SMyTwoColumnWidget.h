#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Input/STextComboBox.h"
#include "Widgets/Input/SSegmentedControl.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "Stats/Stats.h"

class IDetailsView;
class UTexture2D;
class UTexturePresetAsset;
struct FPropertyChangedEvent;

DECLARE_STATS_GROUP(TEXT("TextureManager"), STATGROUP_TextureManager, STATCAT_Advanced);

DECLARE_CYCLE_STAT(TEXT("TextureManager|OnSaveButtonClicked"),
	STAT_TextureManager_OnSaveButtonClicked, STATGROUP_TextureManager);

DECLARE_CYCLE_STAT(TEXT("TextureManager|OnPresetSaveButtonClicked"),
	STAT_TextureManager_OnPresetSaveButtonClicked, STATGROUP_TextureManager);

// Which "mode" the right side is in
enum class ENavigationTab : uint8
{
	Files,
	Presets
};

class SMyTwoColumnWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMyTwoColumnWidget) : _DetailsViewWidget(nullptr)
		{}
		// Optional: let the module pass in an existing details view
		SLATE_ARGUMENT(TSharedPtr<IDetailsView>, DetailsViewWidget)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SMyTwoColumnWidget() override;

	// For Ctrl+S
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

	// --- Public API used by Testing.cpp ---
	// Add a newly imported texture to the "Files" list
	void AddImportedTexture(UTexture2D* NewTexture);

	// Programmatically select a texture (as if the user clicked it)
	void SetSelectedTexture(UTexture2D* NewTexture);

	// Ask the user for a preset name; returns false if user cancels
	bool PromptForPresetName(const FString& DefaultName, FString& OutName, const FString& FixedPath);

private:
	// ---------- Types ----------
	using FTextureItem = TWeakObjectPtr<UTexture2D>;
	using FPresetItem = TWeakObjectPtr<UTexturePresetAsset>;

	// ---------- State ----------

	ENavigationTab ActiveTab = ENavigationTab::Files;

	// Right-side details view
	TSharedPtr<IDetailsView> DetailsView;

	// Files list (left column)
	//TArray<FTextureItem> TextureItems;
	TArray<FTextureItem> AllTextureItems;
	TArray<FTextureItem> FilteredTextureItems;
	TSharedPtr<SListView<FTextureItem>> TextureListView;

	// Presets list (right / Presets tab)
	//TArray<FPresetItem> PresetItems;
	TArray<FPresetItem> AllPresetItems;
	TArray<FPresetItem> FilteredPresetItems;
	TSharedPtr<SListView<FPresetItem>> PresetListView;

	// Current selection
	FTextureItem SelectedTexture;
	FPresetItem  SelectedPreset;

	UTexture2D* PreviewTexture = nullptr;
	UTexturePresetAsset* PreviewPreset = nullptr;

	// Combo box options:
	// Index 0 = <NONE>, nullptr preset
	TArray<TWeakObjectPtr<UTexturePresetAsset>> PresetChoices;
	TArray<TSharedPtr<FString>> PresetLabels;
	TSharedPtr<FString> NonePresetOption;
	TSharedPtr<FString> CurrentPresetOption;
	TSharedPtr<STextComboBox> PresetComboBox;

	TArray<TWeakObjectPtr<UTexturePresetAsset>> FilterPresetChoices;
	TArray<TSharedPtr<FString>> FilterPresetLabels;
	int FilterIndex = 0;
	TSharedPtr<FString> AllPresetOption;
	TSharedPtr<FString> CurrentFilterOption;
	TSharedPtr<STextComboBox> PresetFilterComboBox;

	TSharedPtr<SSearchBox> FilesSearchBox;
	TSharedPtr<SSearchBox> PresetsSearchBox;
	FString FilesSearchQuery;
	FString PresetsSearchQuery;

	// Dirty flag when the selected texture's settings diverge from its preset
	bool bPendingPresetChange = false;
	bool bPendingPropertyChange = false;

	// For global property-change watching
	FDelegateHandle PropertyChangedHandle;

	// ---------- UI construction ----------

	TSharedRef<SWidget> BuildLeftColumn();
	TSharedRef<SWidget> BuildRightColumn();
	TSharedRef<SWidget> BuildFilesList();
	TSharedRef<SWidget> BuildPresetsList();

	TSharedRef<ITableRow> GenerateTextureRow(FTextureItem Item, const TSharedRef<STableViewBase>& OwnerTable);
	TSharedRef<ITableRow> GeneratePresetRow(FPresetItem Item, const TSharedRef<STableViewBase>& OwnerTable);

	// ---------- Tab / selection logic ----------

	ENavigationTab GetActiveTab() const;
	void OnTabChanged(ENavigationTab NewTab);

	void RefreshTextureList();
	void RefreshPresetList();

	void OnTextureSelected(FTextureItem Item, ESelectInfo::Type SelectInfo);
	void OnPresetSelected(FPresetItem Item, ESelectInfo::Type SelectInfo);

	void SyncSelectionToDetails();
	void SelectPresetInCombo(UTexturePresetAsset* Preset);
	void ShowPresetDetails(UTexturePresetAsset* Preset);
	void ClearDetails();

	// ---------- Combo + Save ----------

	void OnPresetComboChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	FReply OnSaveButtonClicked();
	FReply OnPresetSaveButtonClicked();
	FReply OnPresetNewButtonClicked();

	// ---------- Change detection ----------

	void OnAnyPropertyChanged(UObject* Object, FPropertyChangedEvent& Event);
	void InitializePropertyWatcher();
	void ShutdownPropertyWatcher();

	void OnFilterComboChange(TSharedPtr<FString> NewSelection, ESelectInfo::Type);
	void OnFilesSearchChanged(const FText& InText);
	void OnPresetsSearchChanged(const FText& InText);

	void SaveFiles(TArray<FTextureItem> SelectedItems);

	EVisibility IsFilesChosen() const
	{
		return ActiveTab == ENavigationTab::Files ? EVisibility::Visible : EVisibility::Collapsed;
	}

	bool IsPresetComboEnabled() const
	{
		return ActiveTab == ENavigationTab::Files;
	}

	UTexture2D* CloneTextureTransient(UTexture2D* Source);
	void OnDetailsPropertyChanged(const FPropertyChangedEvent& Event);
};