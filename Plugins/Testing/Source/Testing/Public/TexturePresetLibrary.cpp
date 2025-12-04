// TexturePresetLibrary.cpp
#include "TexturePresetLibrary.h"

void UTexturePresetLibrary::CaptureFromTexture(UTexturePresetAsset* PresetAsset, UTexture2D* Texture)
{
    if (!PresetAsset || !Texture) return;

    auto& Out = PresetAsset->Settings;

    Out.LODGroup = Texture->LODGroup;
    Out.CompressionSettings = Texture->CompressionSettings;
    Out.bSRGB = Texture->SRGB;
    Out.bUseAlpha = Texture->HasAlphaChannel();
    Out.bFlipGreenChannel = Texture->bFlipGreenChannel; // if exposed

    // etc. Add any settings you care about.
}

void UTexturePresetLibrary::ApplyToTexture(UTexturePresetAsset* PresetAsset, UTexture2D* Texture)
{
    if (!PresetAsset || !Texture) return;

    const auto& In = PresetAsset->Settings;

    Texture->Modify();

    Texture->LODGroup = In.LODGroup;
    Texture->CompressionSettings = In.CompressionSettings;
    Texture->SRGB = In.bSRGB;
    Texture->bFlipGreenChannel = In.bFlipGreenChannel;
    // You might need to re-compress or re-init resources depending on which fields change.

    Texture->PostEditChange();
    Texture->MarkPackageDirty();
}
