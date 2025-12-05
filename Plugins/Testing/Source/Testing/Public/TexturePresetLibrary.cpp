#include "TexturePresetLibrary.h"

#include "TexturePresetAsset.h"
#include "TexturePresetUserData.h"
#include "Engine/Texture.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "Modules/ModuleManager.h"
#include "UObject/Package.h"
#endif

namespace TexturePresetLibrary
{
	void CaptureFromTexture(UTexturePresetAsset* PresetAsset, UTexture2D* Texture)
	{
		if (!PresetAsset || !Texture) return;

		auto& Out = PresetAsset->Settings;

		Out.LODGroup = Texture->LODGroup;
		Out.CompressionSettings = Texture->CompressionSettings;
		Out.bSRGB = Texture->SRGB;
		Out.bUseAlpha = Texture->HasAlphaChannel();
		Out.bFlipGreenChannel = Texture->bFlipGreenChannel; // if exposed

		// Add more fields from FTexturePresetSettings as needed.
	}

	void ApplyToTexture(UTexturePresetAsset* PresetAsset, UTexture2D* Texture)
	{
		if (!PresetAsset || !Texture) return;

		const auto& In = PresetAsset->Settings;

		Texture->Modify();

		Texture->LODGroup = In.LODGroup;
		Texture->CompressionSettings = In.CompressionSettings;
		Texture->SRGB = In.bSRGB;
		Texture->bFlipGreenChannel = In.bFlipGreenChannel;

		Texture->PostEditChange();
		Texture->MarkPackageDirty();
	}

	UTexturePresetAsset* CreatePresetAssetFromTexture(
		UTexture2D* Texture,
		const FString& PackagePath,
		FName PresetName)
	{
#if WITH_EDITOR
		if (!Texture)
		{
			return nullptr;
		}

		// Default path if none provided
		const FString SanitizedPath = PackagePath.IsEmpty()
			? TEXT("/Game/TexturePresets")
			: PackagePath;

		if (PresetName.IsNone())
		{
			PresetName = *FString::Printf(TEXT("%s_Preset"), *Texture->GetName());
		}

		const FString FullPackageName = SanitizedPath / PresetName.ToString();

		UPackage* Package = CreatePackage(*FullPackageName);
		if (!Package)
		{
			return nullptr;
		}

		Package->FullyLoad();

		UTexturePresetAsset* NewPreset =
			NewObject<UTexturePresetAsset>(
				Package,
				UTexturePresetAsset::StaticClass(),
				PresetName,
				RF_Public | RF_Standalone | RF_Transactional);

		if (!NewPreset)
		{
			return nullptr;
		}

		// Give it a logical display name
		NewPreset->PresetName = PresetName;

		// Fill from current texture settings
		CaptureFromTexture(NewPreset, Texture);

		// Register in asset registry
		FAssetRegistryModule& AssetRegistryModule =
			FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		AssetRegistryModule.AssetCreated(NewPreset);

		Package->MarkPackageDirty();

		return NewPreset;
#else
		return nullptr;
#endif
	}

	void AssignPresetToTexture(UTexturePresetAsset* PresetAsset, UTexture2D* Texture)
	{
#if WITH_EDITOR
		if (!PresetAsset || !Texture)
		{
			return;
		}

		Texture->Modify();

		UTexturePresetUserData* UserData = Cast<UTexturePresetUserData>(
			Texture->GetAssetUserDataOfClass(UTexturePresetUserData::StaticClass()));

		if (!UserData)
		{
			UserData = NewObject<UTexturePresetUserData>(
				Texture,
				UTexturePresetUserData::StaticClass());
			Texture->AddAssetUserData(UserData);
		}

		UserData->AssignedPreset = PresetAsset;

		Texture->MarkPackageDirty();
		Texture->PostEditChange();
#endif
	}
}