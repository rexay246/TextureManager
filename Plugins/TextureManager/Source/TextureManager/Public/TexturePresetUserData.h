#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetUserData.h"
#include "TexturePresetAsset.h"
#include "TexturePresetUserData.generated.h"

UCLASS()
class UTexturePresetUserData : public UAssetUserData
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "Texture Preset")
    TObjectPtr<UTexturePresetAsset> AssignedPreset;
};
