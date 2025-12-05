// TexturePresetAsset.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture.h"
#include "TexturePresetAsset.generated.h"

USTRUCT(BlueprintType)
struct FTexturePresetSettings
{
    GENERATED_BODY()

    // -------- Core texture group / compression --------

    UPROPERTY(EditAnywhere, Category = "Texture Preset")
    TEnumAsByte<TextureGroup> LODGroup = TEXTUREGROUP_World;

    UPROPERTY(EditAnywhere, Category = "Texture Preset")
    int32 LODBias = 0;

    UPROPERTY(EditAnywhere, Category = "Texture Preset")
    TEnumAsByte<TextureCompressionSettings> CompressionSettings = TC_Default;

    UPROPERTY(EditAnywhere, Category = "Texture Preset")
    int32 CompressionQuality = 0; // maps to Texture->CompressionQuality

    UPROPERTY(EditAnywhere, Category = "Texture Preset")
    bool bSRGB = true;

    // This is effectively “does the source have alpha?”, not strictly editable;
    // we keep it as metadata for now.
    UPROPERTY(EditAnywhere, Category = "Texture Preset")
    bool bUseAlpha = false;

    UPROPERTY(EditAnywhere, Category = "Texture Preset")
    bool bFlipGreenChannel = false;

    // -------- Filtering & addressing --------

    UPROPERTY(EditAnywhere, Category = "Texture Preset")
    TEnumAsByte<TextureFilter> Filter = TF_Default;

    UPROPERTY(EditAnywhere, Category = "Texture Preset")
    TEnumAsByte<TextureAddress> AddressX = TA_Wrap;

    UPROPERTY(EditAnywhere, Category = "Texture Preset")
    TEnumAsByte<TextureAddress> AddressY = TA_Wrap;

    UPROPERTY(EditAnywhere, Category = "Texture Preset")
    TEnumAsByte<TextureAddress> AddressZ = TA_Wrap;

    UPROPERTY(EditAnywhere, Category = "Texture Preset")
    int32 MaxAnisotropy = 8;

    // -------- Mips / resolution --------

    UPROPERTY(EditAnywhere, Category = "Texture Preset")
    TEnumAsByte<TextureMipGenSettings> MipGenSettings = TMGS_FromTextureGroup;

    UPROPERTY(EditAnywhere, Category = "Texture Preset")
    int32 MaxTextureSize = 0; // 0 = no override

    UPROPERTY(EditAnywhere, Category = "Texture Preset")
    uint8 NumCinematicMipLevels = 0;

    UPROPERTY(EditAnywhere, Category = "Texture Preset")
    bool bPreserveBorder = false;

    // Downscale settings (LOD bias / editor preview)
    UPROPERTY(EditAnywhere, Category = "Texture Preset")
    float DownscaleFactor = 1.0f;

    //UPROPERTY(EditAnywhere, Category = "Texture Preset")
    //TEnumAsByte<ETextureDownscaleOptions> DownscaleOptions = ETextureDownscaleOptions::Default;

    // -------- Streaming / residency --------

    UPROPERTY(EditAnywhere, Category = "Texture Preset")
    bool NeverStream = false;

    UPROPERTY(EditAnywhere, Category = "Texture Preset")
    bool bForceMiplevelsToBeResident = false;

    UPROPERTY(EditAnywhere, Category = "Texture Preset")
    float StreamingDistanceMultiplier = 1.0f;

    // -------- Virtual texture --------

    UPROPERTY(EditAnywhere, Category = "Texture Preset")
    bool VirtualTextureStreaming = false;
};


UCLASS(BlueprintType)
class UTexturePresetAsset : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "Texture Preset")
    FName PresetName;

    UPROPERTY(EditAnywhere, Category = "Texture Preset")
    FTexturePresetSettings Settings;
};
