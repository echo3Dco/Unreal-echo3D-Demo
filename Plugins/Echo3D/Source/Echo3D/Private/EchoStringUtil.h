#pragma once

#include "CoreMinimal.h"
#include "Containers/UnrealString.h"
#include "EchoStringConstants.h"

struct StringUtil
{
public:
	/*
	static const FString EmptyFString;
	static const FString TrueText;
	static const FString FalseText;

	static const FString DefaultXKey, DefaultYKey, DefaultZKey;
	*/
	static bool TryParseFloat(const FString &input, float &result);
	static bool TryParseBool(const FString &input, bool &result);
	static bool TryParseInt32(const FString &input, int32 &result);

	template<typename T>
	static bool TryParseTyped(const FString &input, T &result, const T &defaultValue);

	template<typename T>
	static FString TryWriteTyped(const T &obj);


	static FString UrlEncode(const FString &input);


	static FString IdentifiersOnly(const FString &source);

	static const FString &BoolToString(bool value) { return value ? EchoStringConstants::TrueText : EchoStringConstants::FalseText; }
};