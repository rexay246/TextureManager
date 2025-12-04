// TexturePresetAsset.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture.h"
#include "TexturePresetAsset.generated.h"

USTRUCT(BlueprintType)
struct FTexturePresetSettings
{
    GENERATED_BODY()

    // You decide which ones matter for v1
    UPROPERTY(EditAnywhere, Category = "Texture Preset")
    TEnumAsByte<TextureGroup> LODGroup = TEXTUREGROUP_World;

    UPROPERTY(EditAnywhere, Category = "Texture Preset")
    TEnumAsByte<TextureCompressionSettings> CompressionSettings = TC_Default;

    UPROPERTY(EditAnywhere, Category = "Texture Preset")
    bool bSRGB = true;

    UPROPERTY(EditAnywhere, Category = "Texture Preset")
    bool bUseAlpha = false;

    UPROPERTY(EditAnywhere, Category = "Texture Preset")
    bool bFlipGreenChannel = false;

    // Add more as needed...
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
