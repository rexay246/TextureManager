// TexturePresetLibrary.h
#pragma once

#include "CoreMinimal.h"

class UTexture2D;
class UTexturePresetAsset;
class UTexturePresetUserData;

// Simple namespace, no UObject / UHT involved
namespace TexturePresetLibrary
{
	// Copy current texture settings into the preset asset
	void CaptureFromTexture(UTexturePresetAsset* PresetAsset, UTexture2D* Texture);

	// Apply preset settings onto a texture
	void ApplyToTexture(UTexturePresetAsset* PresetAsset, UTexture2D* Texture);

	// Create a new preset asset (under PackagePath) from the texture's current settings
	UTexturePresetAsset* CreatePresetAssetFromTexture(
		UTexture2D* Texture,
		const FString& PackagePath,
		FName PresetName
	);

	// Create a new preset asset (under PackagePath)
	UTexturePresetAsset* CreatePresetAsset(
		const FString& PackagePath,
		FName PresetName
	);

	// Attach preset to texture via UTexturePresetUserData::AssignedPreset
	void AssignPresetToTexture(UTexturePresetAsset* PresetAsset, UTexture2D* Texture);
	UTexturePresetAsset* FindPresetByName(const FString& InPresetName, const FString& SearchRootPath = TEXT("/Game"));

	TArray<UTexture2D*> GetAllTexturesUsingPreset(UTexturePresetAsset* PresetAsset);
	void UpdatePresetFromTexture(UTexturePresetAsset* PresetAsset, UTexture2D* Texture);

	void CopyProperties(UTexturePresetAsset* AssetIn, UTexturePresetAsset* AssetOut);
}
