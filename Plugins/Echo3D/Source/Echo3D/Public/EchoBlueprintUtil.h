// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EchoBlueprintUtil.generated.h"

/**
 * Helper functions for blueprint / util libary for random stuff
 */
UCLASS()
class ECHO3D_API UEchoBlueprintUtil : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//static FString ConvertResponseContentToTextString(const FEchoRawBinaryQueryResult &rawBinary)
	
	/**
	 * gets the content of a response as a null-terminated String
	**/
	UFUNCTION(BlueprintPure, Category = "Echo3D")
	//static FString GetContentAsString(const FEchoRawBinaryQueryResult &rawBinary)
	static FString GetContentAsString(const FEchoRawQueryResult &rawBinary)
	{
		//We can't actually make UFUNCTION(s) on UStructs so we'll just write this horrible wrapper
		return rawBinary.GetContentAsString(); //LOL
	}

	UFUNCTION(BlueprintPure, Category = "Echo3D")
	static int32 GetContentLength(const FEchoRawQueryResult &rawBinary)
	{
		//We can't actually make UFUNCTION(s) on UStructs so we'll just write this horrible wrapper
		return rawBinary.contentBlob.Num();
	}

	UFUNCTION(BlueprintPure, Category = "Echo3D")
	static float ConvertTimeMeasurementToFloat(const FEchoTimeMeasurement &timeMeasurement)
	{
		//We can't actually make UFUNCTION(s) on UStructs so we'll just write this horrible wrapper
		//return rawBinary.contentBlob.Num();
		return (float)timeMeasurement.raw;
	}

	UFUNCTION(BlueprintPure, Category = "Echo3D")
	static FEchoTimeMeasurement TimeMeasurementBetween(const FEchoTimeMeasurement &startTime, const FEchoTimeMeasurement &endTime)
	{
		FEchoTimeMeasurement ret;
		ret.raw = endTime.raw - startTime.raw;
		return ret;
		//We can't actually make UFUNCTION(s) on UStructs so we'll just write this horrible wrapper
		//return rawBinary.contentBlob.Num();
		//return (float)timeMeasurement.raw;
	}

	UFUNCTION(BlueprintPure, Category = "Echo3D|Debug")
	static FString ResolveQueryDebugInfo(const FEchoQueryDebugInfo &queryDebugState);
};
