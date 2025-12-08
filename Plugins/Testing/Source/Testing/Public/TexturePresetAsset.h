// TexturePresetAsset.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture.h"
#include "Engine/TextureDefines.h"

#include "Math/Color.h"
#include "TexturePresetAsset.generated.h"

USTRUCT(BlueprintType)
struct FTexturePresetSettings
{
    GENERATED_BODY()

    // Level of Detail

    UPROPERTY(EditAnywhere, Category = "Level of Detail")
    TEnumAsByte<TextureMipGenSettings> MipGenSettings = TMGS_FromTextureGroup;

    UPROPERTY(EditAnywhere, Category = "Level of Detail")
    int32 LODBias = 0;

    UPROPERTY(EditAnywhere, Category = "Level of Detail")
    TEnumAsByte<TextureGroup> TextureGroup = TEXTUREGROUP_World;

    UPROPERTY(EditAnywhere, Category = "Level of Detail\Advanced")
    bool bPreserveBorder = false;

    UPROPERTY(EditAnywhere, Category = "Level of Detail\Advanced")
    float DownscaleFactor = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Level of Detail\Advanced")
    uint8 NumCinematicMipLevels = 0;

    UPROPERTY(EditAnywhere, Category = "Level of Detail\Advanced")
    bool NeverStream = false;

    UPROPERTY(EditAnywhere, Category = "Level of Detail\Advanced")
    bool bForceMiplevelsToBeResident = false;

    // Compression

    UPROPERTY(EditAnywhere, Category = "Compression")
    bool bUseAlpha = false;

    UPROPERTY(EditAnywhere, Category = "Compression")
    TEnumAsByte<TextureCompressionSettings> CompressionSettings = TC_Default;

    UPROPERTY(EditAnywhere, Category = "Compression\Advanced")
    int32 MaxTextureSize = 0; // 0 = no override

    UPROPERTY(EditAnywhere, Category = "Compression\Advanced")
    int32 CompressionQuality = 0; // maps to Texture->CompressionQuality

    // Texture

    UPROPERTY(EditAnywhere, Category = "Texture")
    bool bSRGB = true;

    UPROPERTY(EditAnywhere, Category = "Texture\Advanced")
    TEnumAsByte<TextureAddress> XTilingMethod = TA_Wrap;

    UPROPERTY(EditAnywhere, Category = "Texture\Advanced")
    TEnumAsByte<TextureAddress> YTilingMethod = TA_Wrap;

    UPROPERTY(EditAnywhere, Category = "Texture\Advanced")
    TEnumAsByte<TextureAddress> ZTilingMethod = TA_Wrap;

    UPROPERTY(EditAnywhere, Category = "Texture\Advanced")
    bool bFlipGreenChannel = false;

    UPROPERTY(EditAnywhere, Category = "Texture\Advanced")
    TEnumAsByte<TextureFilter> Filter = TF_Default;

    // Adjustments

    UPROPERTY(EditAnywhere, Category = "Texture\Adjustments")
    float Brightness = 1;

    UPROPERTY(EditAnywhere, Category = "Texture\Adjustments")
    float BrightnessCurve = 1;

    UPROPERTY(EditAnywhere, Category = "Texture\Adjustments")
    float Vibrance = 0;

    UPROPERTY(EditAnywhere, Category = "Texture\Adjustments")
    float Saturation = 1;

    UPROPERTY(EditAnywhere, Category = "Texture\Adjustments")
    float RGBCurve = 1;

    UPROPERTY(EditAnywhere, Category = "Texture\Adjustments")
    float Hue = 0;

    UPROPERTY(EditAnywhere, Category = "Texture\Adjustments")
    float MinAlpha = 0;

    UPROPERTY(EditAnywhere, Category = "Texture\Adjustments")
    float MaxAlpha = 1;

    UPROPERTY(EditAnywhere, Category = "Texture\Adjustments")
    bool ChromaKeyTexture = false;

    UPROPERTY(EditAnywhere, Category = "Texture\Adjustments")
    float ChromaKeyThreshold = 0.003f;

    UPROPERTY(EditAnywhere, Category = "Texture\Adjustments")
	FColor ChromaKeyColor = FColor::Magenta;

    // Other

    UPROPERTY(EditAnywhere, Category = "Other")
    bool VirtualTextureStreaming = false;
};


UCLASS(BlueprintType)
class UTexturePresetAsset : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "Texture Preset")
    FName PresetName;

    UPROPERTY(EditAnywhere, Category = "Texture Preset", meta = (ShowOnlyInnerProperties))
    FTexturePresetSettings Settings;

    UPROPERTY(VisibleAnywhere, Category = "Files")
    TArray<UTexture2D*> Files;
};
