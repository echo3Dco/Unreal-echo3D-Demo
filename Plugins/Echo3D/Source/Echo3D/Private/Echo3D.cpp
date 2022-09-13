// Copyright Epic Games, Inc. All Rights Reserved.

#include "Echo3D.h"
#include "EchoStructsFwd.h"

#include <type_traits>

#define LOCTEXT_NAMESPACE "FEcho3DModule"

void FEcho3DModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	//Compile time sanity checks
	static_assert(std::is_same<FEchoBlob, TArray<uint8>>::value, "FEchoBlob no longer TArray<uint8>: you need to update some UHT stuff that are TArray<uint8> when they wanted to be FEchoBlob. We can't use FEchoBlob there since UTH doesn't understand typedefs");
}

void FEcho3DModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FEcho3DModule, Echo3D)