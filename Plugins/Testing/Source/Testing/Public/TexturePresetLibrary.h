// TexturePresetLibrary.h
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TexturePresetAsset.h"
#include "Engine/Texture2D.h"
#include "TexturePresetLibrary.generated.h"

UCLASS()
class UTexturePresetLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintCallable, Category = "Texture Preset")
    static void CaptureFromTexture(UTexturePresetAsset* PresetAsset, UTexture2D* Texture);

    UFUNCTION(BlueprintCallable, Category = "Texture Preset")
    static void ApplyToTexture(UTexturePresetAsset* PresetAsset, UTexture2D* Texture);
};
