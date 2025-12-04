// Copyright Epic Games, Inc. All Rights Reserved.

#include "tedtCommands.h"

#define LOCTEXT_NAMESPACE "FtedtModule"

void FtedtCommands::RegisterCommands()
{
	UI_COMMAND(PluginAction, "tedt", "Execute tedt action", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
