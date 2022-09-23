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
	
	/**
	 * gets the content of a response as a null-terminated String
	 * we don't automatically do this since the query could be expensive for a huge blob and possibly not even meant to represent a string (for example model files are binary blobs)
	**/
	UFUNCTION(BlueprintPure, Category = "Echo3D")
	static FString GetContentAsString(const FEchoRawQueryResult &rawBinary)
	{
		//We can't actually make UFUNCTION(s) on UStructs so we'll just write this horrible wrapper
		return rawBinary.GetContentAsString(); //LOL
	}

	/**
	 * get the content length of a raw result blob. blueprint doesn't understand uint8_t and we cant add UFUNCTIONs to structs
	**/
	UFUNCTION(BlueprintPure, Category = "Echo3D")
	static int32 GetContentLength(const FEchoRawQueryResult &rawBinary)
	{
		//We can't actually make UFUNCTION(s) on UStructs so we'll just write this horrible wrapper
		return rawBinary.contentBlob.Num();
	}

	/**
	 * convert a time measurement to a flow. this is kinda a horrible cludge
	**/
	UFUNCTION(BlueprintPure, Category = "Echo3D")
	static float ConvertTimeMeasurementToFloat(const FEchoTimeMeasurement &timeMeasurement)
	{
		//We can't actually make UFUNCTION(s) on UStructs so we'll just write this horrible wrapper
		//return rawBinary.contentBlob.Num();
		return (float)timeMeasurement.raw;
	}

	/**
	 * the time elapsed between two times
	 * a slightly less horrible way to work with time measurements in blueprint
	**/
	UFUNCTION(BlueprintPure, Category = "Echo3D")
	static FEchoTimeMeasurement TimeMeasurementBetween(const FEchoTimeMeasurement &startTime, const FEchoTimeMeasurement &endTime)
	{
		FEchoTimeMeasurement ret;
		ret.raw = endTime.raw - startTime.raw;
		return ret;
	}

	/**
	 * convert query debug info into a string for debugging purposes
	**/
	UFUNCTION(BlueprintPure, Category = "Echo3D|Debug")
	static FString ResolveQueryDebugInfo(const FEchoQueryDebugInfo &queryDebugState);
};
