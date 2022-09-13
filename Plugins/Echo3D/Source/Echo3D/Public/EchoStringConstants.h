#pragma once
#include "Containers/UnrealString.h"

struct ECHO3D_API EchoStringConstants
{
	//GENERATED_BODY()
public:
	//static const FString EmptyFString; //deprecated
	static const FString EmptyString;

	/**
	 * sometimes (often?) a "null" string is more informative than an empty string, especially to indicate something is null
	**/
	static const FString NullString;

	/**
	 * For when a string should not be used/should be obvious for debugging - pass when you want it to be VERY obvious if something is using a string when it should not be!
	**/
	static const FString BadStringValue; 

	static const FString TrueText;
	static const FString FalseText;

	static const FString DefaultXKey;
	static const FString DefaultYKey;
	static const FString DefaultZKey;

	//static const TArray<FString> EmptyFStringArray;//deprecated
	static const TArray<FString> EmptyStringArray;
};