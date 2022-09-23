#pragma once
#include "Containers/UnrealString.h"
//TODO: does this really want ECHO3D_API?
/**
 * some string constants used practically everywhere
**/
struct ECHO3D_API EchoStringConstants
{
	//GENERATED_BODY()
public:
	/** an empty string**/
	static const FString EmptyString;

	/**
	 * sometimes (often?) a "null" string is more informative than an empty string, especially to indicate something is null
	**/
	static const FString NullString;

	/**
	 * For when a string should not be used/should be obvious for debugging - pass when you want it to be VERY obvious if something is using a string when it should not be!
	**/
	static const FString BadStringValue; 

	/** constant for true boolean **/
	static const FString TrueText;

	/** constant for false boolean **/
	static const FString FalseText;

	//keys for simplevec3 names
	static const FString DefaultXKey;
	static const FString DefaultYKey;
	static const FString DefaultZKey;

	/** readonly zero-length array of strings used as a placeholder for entries/tags**/
	static const TArray<FString> EmptyStringArray;
};