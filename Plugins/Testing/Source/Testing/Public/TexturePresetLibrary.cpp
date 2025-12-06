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

		FTexturePresetSettings& Out = PresetAsset->Settings;

		// --- Core ---
		Out.TextureGroup = Texture->LODGroup;
		Out.LODBias = Texture->LODBias;
		Out.CompressionSettings = Texture->CompressionSettings;
		//Out.CompressionQuality = Texture->CompressionQuality;
		Out.bSRGB = Texture->SRGB;

		Out.bUseAlpha = Texture->HasAlphaChannel(); // informational
		Out.bFlipGreenChannel = Texture->bFlipGreenChannel;

		// --- Filter / addressing ---
		Out.Filter = Texture->Filter;
		Out.XTilingMethod = Texture->AddressX;
		Out.YTilingMethod = Texture->AddressY;
		//Out.AddressZ = Texture->AddressZ;
		//Out.MaxAnisotropy = Texture->MaxAnisotropy;

		// --- Mips / size ---
		Out.MipGenSettings = Texture->MipGenSettings;
		Out.MaxTextureSize = Texture->MaxTextureSize;
		Out.NumCinematicMipLevels = Texture->NumCinematicMipLevels;
		Out.bPreserveBorder = Texture->bPreserveBorder;
		//Out.DownscaleFactor = Texture->DownscaleFactor;
		//Out.DownscaleOptions = Texture->DownscaleOptions;

		// --- Streaming / residency ---
		Out.NeverStream = Texture->NeverStream;
		Out.bForceMiplevelsToBeResident = Texture->bForceMiplevelsToBeResident;
		//Out.StreamingDistanceMultiplier = Texture->StreamingDistanceMultiplier;

		// --- Virtual texturing ---
		Out.VirtualTextureStreaming = Texture->VirtualTextureStreaming;

		// Add more fields here if you decide to extend the struct later.
	}


	void ApplyToTexture(UTexturePresetAsset* PresetAsset, UTexture2D* Texture)
	{
		if (!PresetAsset || !Texture) return;

		const FTexturePresetSettings& In = PresetAsset->Settings;

		Texture->Modify();

		// --- Core ---
		Texture->LODGroup = In.TextureGroup;
		Texture->LODBias = In.LODBias;
		Texture->CompressionSettings = In.CompressionSettings;
		//Texture->CompressionQuality = In.CompressionQuality;
		Texture->SRGB = In.bSRGB;
		Texture->bFlipGreenChannel = In.bFlipGreenChannel;

		// NOTE: bUseAlpha is informational; you can’t actually force a texture
		// to “have alpha” without changing its source/import. We don’t apply it.

		// --- Filter / addressing ---
		Texture->Filter = In.Filter;
		Texture->AddressX = In.XTilingMethod;
		Texture->AddressY = In.YTilingMethod;
		//Texture->AddressZ = In.AddressZ;
		//Texture->MaxAnisotropy = In.MaxAnisotropy;

		// --- Mips / size ---
		Texture->MipGenSettings = In.MipGenSettings;
		Texture->MaxTextureSize = In.MaxTextureSize;
		Texture->NumCinematicMipLevels = In.NumCinematicMipLevels;
		Texture->bPreserveBorder = In.bPreserveBorder;
		//Texture->DownscaleFactor = In.DownscaleFactor;
		//Texture->DownscaleOptions = In.DownscaleOptions;

		// --- Streaming / residency ---
		Texture->NeverStream = In.NeverStream;
		Texture->bForceMiplevelsToBeResident = In.bForceMiplevelsToBeResident;
		//Texture->StreamingDistanceMultiplier = In.StreamingDistanceMultiplier;

		// --- Virtual texturing ---
		Texture->VirtualTextureStreaming = In.VirtualTextureStreaming;

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

		if (!UserData->AssignedPreset->Files.Find(Texture))
			UserData->AssignedPreset->Files.Remove(Texture);
		UserData->AssignedPreset = PresetAsset;
		UserData->AssignedPreset->Files.Add(Texture);

		Texture->MarkPackageDirty();
		Texture->PostEditChange();
#endif
	}

	UTexturePresetAsset* FindPresetByName( const FString& InPresetName, const FString& SearchRootPath /*= TEXT("/Game")*/)
	{
#if WITH_EDITOR
		if (InPresetName.IsEmpty())
		{
			return nullptr;
		}

		// Load Asset Registry
		FAssetRegistryModule& AssetRegistryModule =
			FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

		// Build a filter for all UTexturePresetAsset under SearchRootPath
		FARFilter Filter;
		Filter.ClassPaths.Add(UTexturePresetAsset::StaticClass()->GetClassPathName());
		Filter.bRecursiveClasses = true;

		const FName RootPathName(*SearchRootPath);
		Filter.PackagePaths.Add(RootPathName);
		Filter.bRecursivePaths = true;

		TArray<FAssetData> PresetAssets;
		AssetRegistryModule.Get().GetAssets(Filter, PresetAssets);

		const FName TargetName(*InPresetName);

		for (const FAssetData& AssetData : PresetAssets)
		{
			UTexturePresetAsset* Preset = Cast<UTexturePresetAsset>(AssetData.GetAsset());
			if (!Preset)
			{
				continue;
			}

			// Match either the PresetName property or the asset object name
			const bool bMatchByPresetName =
				!Preset->PresetName.IsNone() && (Preset->PresetName == TargetName);

			const bool bMatchByObjectName =
				Preset->GetFName() == TargetName;

			if (bMatchByPresetName || bMatchByObjectName)
			{
				return Preset;
			}
		}
#endif

		return nullptr;
	}

}