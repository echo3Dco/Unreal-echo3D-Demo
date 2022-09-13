// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EchoConnection.h"
#include "EchoAdditionalDataLibrary.generated.h"

/**
 * 
 */
UCLASS()
class ECHO3D_API UEchoAdditionalDataLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()


	//static const FString DefaultXKey, DefaultYKey, DefaultZKey;
public:
	//subject: apparently self is a reserved name in unreal

	UFUNCTION(BlueprintPure, Category = "Echo3D|Config")
	static bool HasField(const FEchoAdditionalData &subject, const FString &key)
	{
		return subject.HasField(key);
	}

	//static FString ReadString_BP(const FString &key, const FString &defaultValue)
	UFUNCTION(BlueprintPure, Category = "Echo3D|Config")
	static FString ReadString(const FEchoAdditionalData &subject, const FString &key, const FString &defaultValue)
	{
		return subject.ReadStringRef(key, defaultValue);
	}

	UFUNCTION(BlueprintPure, Category = "Echo3D|Config")
	static bool ReadBool(const FEchoAdditionalData &subject, const FString &key, bool defaultValue = false)
	{
		return subject.ReadBool(key, defaultValue);
	}

	UFUNCTION(BlueprintPure, Category = "Echo3D|Config")
	static int ReadInt(const FEchoAdditionalData &subject, const FString &key, int defaultValue = 0)
	{
		return subject.ReadInt(key, defaultValue);
	}

	UFUNCTION(BlueprintPure, Category = "Echo3D|Config")
	static float ReadFloat(const FEchoAdditionalData &subject, const FString &key, float defaultValue = 0.0f)
	{
		return subject.ReadFloat(key, defaultValue);
	}

	UFUNCTION(BlueprintPure, Category = "Echo3D|Config")
	static FVector ReadVec3(const FEchoAdditionalData &subject, const FString &xKey, const FString &yKey, const FString &zKey, const FVector &defaultValue = FVector::ZeroVector)
	{
		return subject.ReadVec3(xKey, yKey, zKey, defaultValue);
	}

	UFUNCTION(BlueprintPure, Category = "Echo3D|Config")
	static FVector ReadVec3Simple(const FEchoAdditionalData &subject, const FString &keyBase, const FVector &defaultValue = FVector::ZeroVector)
	{
		return subject.ReadVec3(keyBase + EchoStringConstants::DefaultXKey, keyBase + EchoStringConstants::DefaultYKey, keyBase + EchoStringConstants::DefaultZKey, defaultValue);
	}

	//blueprint only seems to like bool, int32, float, and string ... essentially

	UFUNCTION(BlueprintCallable, Category = "Echo3D|Config")
	static void WriteString(
		UPARAM(ref) FEchoAdditionalData &subject, 
		const FString &key, const FString &value)
	{
		subject.WriteString(key, value);
	}

	UFUNCTION(BlueprintCallable, Category = "Echo3D|Config")
	static void WriteBool(
		UPARAM(ref) FEchoAdditionalData &subject,
		const FString &key, bool value)
	{
		subject.WriteBool(key, value);
	}

	UFUNCTION(BlueprintCallable, Category = "Echo3D|Config")
	static void WriteFloat(
		UPARAM(ref) FEchoAdditionalData &subject, 
		const FString &key, float value)
	{
		subject.WriteFloat(key, value);
	}

	UFUNCTION(BlueprintCallable, Category = "Echo3D|Config")
	//static void WriteInt(FEchoAdditionalData &subject, const FString &key, int32 value)
	static void WriteInt(
		UPARAM(ref) FEchoAdditionalData &subject, 
		const FString &key, int32 value)
	{
		subject.WriteInt(key, value);
	}

	UFUNCTION(BlueprintCallable, Category = "Echo3D|Config")
	static void WriteVec3(
		UPARAM(ref) FEchoAdditionalData &subject, 
		const FString &xKey, const FString &yKey, const FString &zKey, FVector value)
	{
		subject.WriteVec3(xKey, yKey, zKey, value);
	}

	UFUNCTION(BlueprintPure, Category = "Echo3D|Config")
	static void WriteVec3Simple(
		UPARAM(ref) FEchoAdditionalData &subject, 
		const FString &keyBase, FVector writeValue)
	{
		//subject.WriteVec3(keyBase + DefaultXKey, keyBase + DefaultYKey, keyBase + DefaultZKey, writeValue);
		subject.WriteVec3(keyBase + EchoStringConstants::DefaultXKey, keyBase + EchoStringConstants::DefaultYKey, keyBase + EchoStringConstants::DefaultZKey, writeValue);
	}
};
