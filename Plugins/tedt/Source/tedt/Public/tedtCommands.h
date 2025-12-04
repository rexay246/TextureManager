// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Framework/Commands/Commands.h"
#include "tedtStyle.h"

class FtedtCommands : public TCommands<FtedtCommands>
{
public:

	FtedtCommands()
		: TCommands<FtedtCommands>(TEXT("tedt"), NSLOCTEXT("Contexts", "tedt", "tedt Plugin"), NAME_None, FtedtStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > PluginAction;
};
