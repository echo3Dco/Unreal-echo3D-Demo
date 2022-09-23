// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EchoConnection.h"
#include "EchoAdditionalDataLibrary.generated.h"

/**
 * a blueprint function library for working with hologram meta data since USTRUCT(s) cant expose UFUNCTION(s) to blueprint
 */
UCLASS()
class ECHO3D_API UEchoAdditionalDataLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	//subject: apparently self is a reserved name in unreal

	/**
	 * does it have the field
	**/
	UFUNCTION(BlueprintPure, Category = "Echo3D|Config")
	static bool HasField(const FEchoAdditionalData &subject, const FString &key)
	{
		return subject.HasField(key);
	}

	/**
	 * read a field as a string
	**/
	UFUNCTION(BlueprintPure, Category = "Echo3D|Config")
	static FString ReadString(const FEchoAdditionalData &subject, const FString &key, const FString &defaultValue)
	{
		return subject.ReadStringRef(key, defaultValue);
	}

	/**
	 * read a field as a bool
	**/
	UFUNCTION(BlueprintPure, Category = "Echo3D|Config")
	static bool ReadBool(const FEchoAdditionalData &subject, const FString &key, bool defaultValue = false)
	{
		return subject.ReadBool(key, defaultValue);
	}

	/**
	 * read a field as an int32 (signed)
	**/
	UFUNCTION(BlueprintPure, Category = "Echo3D|Config")
	static int ReadInt(const FEchoAdditionalData &subject, const FString &key, int defaultValue = 0)
	{
		return subject.ReadInt(key, defaultValue);
	}

	/**
	 * read a field as a float32
	**/
	UFUNCTION(BlueprintPure, Category = "Echo3D|Config")
	static float ReadFloat(const FEchoAdditionalData &subject, const FString &key, float defaultValue = 0.0f)
	{
		return subject.ReadFloat(key, defaultValue);
	}

	/**
	 * read three fields as float32 to get a vector3
	**/
	UFUNCTION(BlueprintPure, Category = "Echo3D|Config")
	static FVector ReadVec3(const FEchoAdditionalData &subject, const FString &xKey, const FString &yKey, const FString &zKey, const FVector &defaultValue = FVector::ZeroVector)
	{
		return subject.ReadVec3(xKey, yKey, zKey, defaultValue);
	}

	/**
	 * read 3 fields with names based on a base name
	**/
	UFUNCTION(BlueprintPure, Category = "Echo3D|Config")
	static FVector ReadVec3Simple(const FEchoAdditionalData &subject, const FString &keyBase, const FVector &defaultValue = FVector::ZeroVector)
	{
		return subject.ReadVec3(keyBase + EchoStringConstants::DefaultXKey, keyBase + EchoStringConstants::DefaultYKey, keyBase + EchoStringConstants::DefaultZKey, defaultValue);
	}

	//blueprint only seems to like bool, int32, float, and string ... essentially

	/**
	 * store a string value to a field
	**/
	UFUNCTION(BlueprintCallable, Category = "Echo3D|Config")
	static void WriteString(
		UPARAM(ref) FEchoAdditionalData &subject, 
		const FString &key, const FString &value)
	{
		subject.WriteString(key, value);
	}

	/**
	 * store a bool to a field
	**/
	UFUNCTION(BlueprintCallable, Category = "Echo3D|Config")
	static void WriteBool(
		UPARAM(ref) FEchoAdditionalData &subject,
		const FString &key, bool value)
	{
		subject.WriteBool(key, value);
	}

	/**
	 * write an int32 to a field
	**/
	UFUNCTION(BlueprintCallable, Category = "Echo3D|Config")
	static void WriteInt(
		UPARAM(ref) FEchoAdditionalData &subject, 
		const FString &key, int32 value)
	{
		subject.WriteInt(key, value);
	}

	/**
	 * store a float32 to a field
	**/
	UFUNCTION(BlueprintCallable, Category = "Echo3D|Config")
	static void WriteFloat(
		UPARAM(ref) FEchoAdditionalData &subject, 
		const FString &key, float value)
	{
		subject.WriteFloat(key, value);
	}

	/**
	 * write a vec3 to 3 float32 fields
	**/
	UFUNCTION(BlueprintCallable, Category = "Echo3D|Config")
	static void WriteVec3(
		UPARAM(ref) FEchoAdditionalData &subject, 
		const FString &xKey, const FString &yKey, const FString &zKey, FVector value)
	{
		subject.WriteVec3(xKey, yKey, zKey, value);
	}

	/**
	 * write a vec3 to 3 float32 fields with a common base name
	**/
	UFUNCTION(BlueprintPure, Category = "Echo3D|Config")
	static void WriteVec3Simple(
		UPARAM(ref) FEchoAdditionalData &subject, 
		const FString &keyBase, FVector writeValue)
	{
		subject.WriteVec3(keyBase + EchoStringConstants::DefaultXKey, keyBase + EchoStringConstants::DefaultYKey, keyBase + EchoStringConstants::DefaultZKey, writeValue);
	}
};
