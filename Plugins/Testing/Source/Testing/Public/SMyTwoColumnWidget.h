#pragma once

#include "Widgets/SCompoundWidget.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/STextComboBox.h"
#include "Widgets/Input/SSegmentedControl.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SBoxPanel.h"        // SVerticalBox / SHorizontalBox
#include "Widgets/SNullWidget.h"
#include "Styling/AppStyle.h"
#include "UObject/WeakObjectPtrTemplates.h"

class IDetailsView;
class UTexture2D;
// Later you can swap this back to a real preset type:
// class UTexturePresetAsset;

// Simple enum for which tab on the left is active
enum class ENavigationTab : uint8
{
    Files,
    Presets
};

class SMyTwoColumnWidget : public SCompoundWidget
{
public:

    SLATE_BEGIN_ARGS(SMyTwoColumnWidget)
        : _DetailsViewWidget(nullptr)
        {
        }
        SLATE_ARGUMENT(TSharedPtr<IDetailsView>, DetailsViewWidget)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs)
    {
        DetailsView = InArgs._DetailsViewWidget;
        ActiveTab = ENavigationTab::Files;

        // Preset combo options
        RefreshPresetOptions();

        ChildSlot
            [
                SNew(SSplitter)

                    // Left column: navigation
                    + SSplitter::Slot()
                    .Value(0.30f)
                    [
                        BuildLeftColumn()
                    ]

                    // Right column: header + details view
                    + SSplitter::Slot()
                    .Value(0.70f)
                    [
                        BuildRightColumn()
                    ]
            ];
    }

    // Called by module when a texture is imported / selected
    void SetSelectedTexture(UTexture2D* InTexture)
    {
        SelectedTexture = InTexture;

        if (DetailsView.IsValid())
        {
            DetailsView->SetObject(SelectedTexture.Get());
        }

        ActiveTab = ENavigationTab::Files;
        RefreshPresetOptionsFromSelection();
    }

private:

    // ---------- Data ----------

    ENavigationTab ActiveTab = ENavigationTab::Files;

    // Right side details
    TSharedPtr<IDetailsView> DetailsView;

    // Files list
    using FTextureItem = TWeakObjectPtr<UTexture2D>;
    TArray<FTextureItem> TextureItems;
    TSharedPtr< SListView<FTextureItem> > TextureListView;

    // Presets list
    // For now, use UObject* as the preset type; later replace with your UTexturePresetAsset
    using FPresetItem = TWeakObjectPtr<UObject>;
    TArray<FPresetItem> PresetItems;
    TSharedPtr< SListView<FPresetItem> > PresetListView;

    // Current selection
    FTextureItem SelectedTexture;
    FPresetItem  SelectedPreset;

    // Preset dropdown
    TArray<TSharedPtr<FString>> PresetOptions;
    TSharedPtr<FString> CurrentPresetOption;
    TSharedPtr<STextComboBox> PresetComboBox;

    // ---------- UI construction ----------

    TSharedRef<SWidget> BuildLeftColumn()
    {
        return
            SNew(SBorder)
            .Padding(4)
            .BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
            [
                SNew(SVerticalBox)

                    // Tabs: Files / Presets
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(2)
                    [
                        SNew(SSegmentedControl<ENavigationTab>)
                            .Value(this, &SMyTwoColumnWidget::GetActiveTab)
                            .OnValueChanged(this, &SMyTwoColumnWidget::OnTabChanged)
                            + SSegmentedControl<ENavigationTab>::Slot(ENavigationTab::Files)
                            .Text(FText::FromString(TEXT("Files")))
                            + SSegmentedControl<ENavigationTab>::Slot(ENavigationTab::Presets)
                            .Text(FText::FromString(TEXT("Presets")))
                    ]

                // Body (depending on tab)
                + SVerticalBox::Slot()
                    .FillHeight(1.f)
                    .Padding(2)
                    [
                        SNew(SBox)
                            [
                                SNew(SWidgetSwitcher)
                                    .WidgetIndex(this, &SMyTwoColumnWidget::GetWidgetIndex)

                                    // Files List
                                    + SWidgetSwitcher::Slot()
                                    [
                                        BuildFilesList()
                                    ]

                                    // Presets List
                                    + SWidgetSwitcher::Slot()
                                    [
                                        BuildPresetsList()
                                    ]
                            ]
                    ]
            ];
    }

    TSharedRef<SWidget> BuildFilesList()
    {
        // You’ll populate TextureItems from your manager / import events
        TextureListView =
            SNew(SListView<FTextureItem>)
            .ListItemsSource(&TextureItems)
            .OnGenerateRow(this, &SMyTwoColumnWidget::GenerateTextureRow)
            .OnSelectionChanged(this, &SMyTwoColumnWidget::OnTextureSelected)
            .SelectionMode(ESelectionMode::Single);

        return TextureListView.ToSharedRef();
    }

    TSharedRef<SWidget> BuildPresetsList()
    {
        // You’ll populate PresetItems from your preset manager
        PresetListView =
            SNew(SListView<FPresetItem>)
            .ListItemsSource(&PresetItems)
            .OnGenerateRow(this, &SMyTwoColumnWidget::GeneratePresetRow)
            .OnSelectionChanged(this, &SMyTwoColumnWidget::OnPresetSelected)
            .SelectionMode(ESelectionMode::Single);

        return PresetListView.ToSharedRef();
    }

    TSharedRef<SWidget> BuildRightColumn()
    {
        return
            SNew(SBorder)
            .Padding(4)
            .BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
            [
                SNew(SVerticalBox)

                    // Header: preset dropdown + buttons
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0, 0, 0, 4)
                    [
                        BuildHeaderBar()
                    ]

                    // Details View
                    + SVerticalBox::Slot()
                    .FillHeight(1.f)
                    [
                        DetailsView.IsValid()
                            ? DetailsView.ToSharedRef()
                            : SNullWidget::NullWidget
                    ]
            ];
    }

    TSharedRef<SWidget> BuildHeaderBar()
    {
        return
            SNew(SHorizontalBox)

            // Label
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(0, 0, 4, 0)
            [
                SNew(STextBlock)
                    .Text(FText::FromString(TEXT("Current Preset:")))
            ]

            // Preset dropdown
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SAssignNew(PresetComboBox, STextComboBox)
                    .OptionsSource(&PresetOptions)
                    .InitiallySelectedItem(CurrentPresetOption)
                    .OnSelectionChanged(this, &SMyTwoColumnWidget::OnPresetComboChanged)
            ]

            // Apply button
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(4, 0)
            [
                SNew(SButton)
                    .Text(FText::FromString(TEXT("Apply")))
                    .OnClicked(this, &SMyTwoColumnWidget::OnApplyPresetClicked)
                    .IsEnabled(this, &SMyTwoColumnWidget::CanApplyPreset)
            ]

            // Save button
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(4, 0)
            [
                SNew(SButton)
                    .Text(FText::FromString(TEXT("Save As Preset")))
                    .OnClicked(this, &SMyTwoColumnWidget::OnSaveAsPresetClicked)
                    .IsEnabled(this, &SMyTwoColumnWidget::CanSaveAsPreset)
            ]

            // Spacer
            + SHorizontalBox::Slot()
            .FillWidth(1.f);
    }

    // ---------- Helpers / callbacks ----------

    ENavigationTab GetActiveTab() const { return ActiveTab; }

    int32 GetWidgetIndex() const
    {
        return (ActiveTab == ENavigationTab::Files) ? 0 : 1;
    }

    void OnTabChanged(ENavigationTab NewTab)
    {
        ActiveTab = NewTab;

        if (DetailsView.IsValid())
        {
            if (ActiveTab == ENavigationTab::Files && SelectedTexture.IsValid())
            {
                DetailsView->SetObject(SelectedTexture.Get());
            }
            else if (ActiveTab == ENavigationTab::Presets && SelectedPreset.IsValid())
            {
                DetailsView->SetObject(SelectedPreset.Get());
            }
        }
    }

    TSharedRef<ITableRow> GenerateTextureRow(
        FTextureItem Item,
        const TSharedRef<STableViewBase>& OwnerTable)
    {
        FText Label = FText::FromString(
            Item.IsValid() ? Item->GetName() : TEXT("<invalid>"));

        return
            SNew(STableRow<FTextureItem>, OwnerTable)
            [
                SNew(STextBlock).Text(Label)
            ];
    }

    void OnTextureSelected(FTextureItem Item, ESelectInfo::Type)
    {
        SelectedTexture = Item;
        ActiveTab = ENavigationTab::Files;

        if (DetailsView.IsValid())
        {
            DetailsView->SetObject(SelectedTexture.Get());
        }

        RefreshPresetOptionsFromSelection();
    }

    TSharedRef<ITableRow> GeneratePresetRow(
        FPresetItem Item,
        const TSharedRef<STableViewBase>& OwnerTable)
    {
        // Until you have a real UTexturePresetAsset, just display a placeholder name.
        FText Label = FText::FromString(TEXT("<Preset>"));

        if (Item.IsValid())
        {
            Label = FText::FromString(Item->GetName());
        }

        return
            SNew(STableRow<FPresetItem>, OwnerTable)
            [
                SNew(STextBlock).Text(Label)
            ];
    }

    void OnPresetSelected(FPresetItem Item, ESelectInfo::Type)
    {
        SelectedPreset = Item;
        ActiveTab = ENavigationTab::Presets;

        if (DetailsView.IsValid() && SelectedPreset.IsValid())
        {
            DetailsView->SetObject(SelectedPreset.Get());
        }

        RefreshPresetOptions();
    }

    void RefreshPresetOptions()
    {
        PresetOptions.Empty();

        // TODO: query your FTexturePresetManager here
        // For now, just stub:
        PresetOptions.Add(MakeShared<FString>(TEXT("Custom")));
        CurrentPresetOption = PresetOptions[0];

        if (PresetComboBox.IsValid())
        {
            PresetComboBox->RefreshOptions();
            PresetComboBox->SetSelectedItem(CurrentPresetOption);
        }
    }

    void RefreshPresetOptionsFromSelection()
    {
        // TODO: read AssignedPreset from SelectedTexture’s user data,
        // and update CurrentPresetOption accordingly.
        RefreshPresetOptions();
    }

    void OnPresetComboChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type)
    {
        CurrentPresetOption = NewSelection;
        // Later: look up the corresponding preset asset and set SelectedPreset
    }

    bool CanApplyPreset() const
    {
        return SelectedTexture.IsValid() && SelectedPreset.IsValid();
    }

    FReply OnApplyPresetClicked()
    {
        if (CanApplyPreset())
        {
            // TODO: UTexturePresetLibrary::ApplyToTexture(SelectedPreset, SelectedTexture)
        }
        return FReply::Handled();
    }

    bool CanSaveAsPreset() const
    {
        return SelectedTexture.IsValid();
    }

    FReply OnSaveAsPresetClicked()
    {
        if (CanSaveAsPreset())
        {
            // TODO: FTexturePresetManager::Get().CreatePresetFromTexture(SelectedTexture);
        }
        return FReply::Handled();
    }
};