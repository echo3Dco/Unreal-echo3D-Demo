// Fill out your copyright notice in the Description page of Project Settings.


#include "EchoConnection.h"
#include "Misc/DefaultValueHelper.h"

#include "EchoStringConstants.h"

//for urlencode
#include "GenericPlatform/GenericPlatformHttp.h"

//const FString EchoStringConstants::EmptyFString;
const FString EchoStringConstants::EmptyString;
const FString EchoStringConstants::NullString("null");
const FString EchoStringConstants::BadStringValue("_this_is_a_bad_string_value_");


const FString EchoStringConstants::TrueText("true");
const FString EchoStringConstants::FalseText("false");

const FString EchoStringConstants::DefaultXKey(TEXT(".x"));
const FString EchoStringConstants::DefaultYKey(TEXT(".y"));
const FString EchoStringConstants::DefaultZKey(TEXT(".z"));

//const TArray<FString> EchoStringConstants::EmptyFStringArray;
const TArray<FString> EchoStringConstants::EmptyStringArray;

///////////////////////////////////////////////////////////////////

//static const FString DefaultXKey, DefaultYKey, DefaultZKey;

bool StringUtil::TryParseInt32(const FString &input, int32 &result)
{
	return FDefaultValueHelper::ParseInt(input, result);
}

bool StringUtil::TryParseFloat(const FString &input, float &result)
{
	return FDefaultValueHelper::ParseFloat(input, result);
}

bool StringUtil::TryParseBool(const FString &input, bool &result)
{
	//TODO: case insensitive comparison instead?
	if ((input == "true")||(input == "TRUE")||(input == "True"))
	{
		result = true;
		return true;
	}
	if ((input == "false")||(input == "FALSE")||(input == "False"))
	{
		result = false;
		return true;
	}
	return false;
}

//not implemented
//template<typename T>
//bool StringUtil::TryParseTyped(const FString &input, T &result, const T &defaultValue);
		
template<>
bool StringUtil::TryParseTyped<int32>(const FString &input, int32 &result, const int32 &defaultValue)
{
	if (!TryParseInt32(input, result))
	{
		result = defaultValue;
		return false;
	}
	return true;
}

template<>
bool StringUtil::TryParseTyped<float>(const FString &input, float &result, const float &defaultValue)
{
	if (!TryParseFloat(input, result))
	{
		result = defaultValue;
		return false;
	}
	return true;
}

template<>
bool StringUtil::TryParseTyped<bool>(const FString &input, bool &result, const bool &defaultValue)
{
	if (!TryParseBool(input, result))
	{
		result = defaultValue;
		return false;
	}
	return true;
}

template<>
FString StringUtil::TryWriteTyped<FString>(const FString &obj)
{
	return obj;
}


template<>
FString StringUtil::TryWriteTyped<int32>(const int32 &obj)
{
	//FDefaultValueHelper::
	return FString::Printf(TEXT("%d"), obj);
}

template<>
FString StringUtil::TryWriteTyped<float>(const float &obj)
{
	//FDefaultValueHelper::
	return FString::Printf(TEXT("%f"), obj);
}

template<>
FString StringUtil::TryWriteTyped<bool>(const bool &obj)
{
	//FDefaultValueHelper::
	//return FString::Printf(TEXT("%f"), obj);
	return obj ? EchoStringConstants::TrueText : EchoStringConstants::FalseText;
}

//TODO: should this be in some kind of echostringutil.cpp??

FString StringUtil::UrlEncode(const FString &input)
{
	return FGenericPlatformHttp::UrlEncode(input);
}

FString IdentifiersOnlyWithChar(const FString &source, TCHAR ReplaceWithChar = '_')
{
	FString ret = source;
	for(size_t i=0; i<source.Len(); i++)
	{
		TCHAR ch = ret[i];
		//if ((ch < 'a')||(ch >'z')) && ((ch < 'A')||(ch
		
		//FAIL//bool bValidIdentifier = ((ch >= 'a')&&(ch <= 'z')) || ((ch >= 'A')||(ch <= 'Z')) || ((ch >= '0')&&(ch <= '9')) || (ch == '_');
		bool bValidIdentifier = 
							   ((ch >= 'a')&&(ch <= 'z')) 
							|| ((ch >= 'A')&&(ch <= 'Z')) 
							|| ((ch >= '0')&&(ch <= '9')) 
							|| (ch == '_')
			;

		//TODO: check not past '\0'???
		if (!bValidIdentifier)
		{
			ret[i] = ReplaceWithChar;//'_'; //hack replace with underscore
		}
	}
	return ret;
}

FString StringUtil::IdentifiersOnly(const FString &source)
{
	return IdentifiersOnlyWithChar(source);
}


//////////////////////

FString FEchoRawQueryResult::GetContentAsString() const
{
	return EchoHelperLib::StringFromRawContent(contentBlob);
}

//////////////////////

template<typename T>
T ReadAdditionalDataTyped(const FEchoAdditionalData &thisData, const FString &key, T defaultValue = {})
//T ReadAdditionalDataTyped(const FString &key, T defaultValue = 0) const
{
	//or should we use the json object's parse? are floats in json allowed to be like 3.0f?
	if (!thisData.HasField(key))
	{
		return defaultValue;
	}
	const FString &str = thisData.ReadStringRef(key);
	T ret = {};
	if (!StringUtil::TryParseTyped<T>(str, ret, defaultValue))
	{
		//init once per type - note that this constant is PER TEMPLATE instantiation
		const static FString typeName(typeid(T).name());

		UE_LOG(LogTemp, Warning, TEXT("ReadFloat('%s'): Unable to parse as %s string: '%s'"), *key, *typeName, *str);
		ret = defaultValue;
	}
	return ret;
}

template<typename T>
void WriteAdditionalDataTyped(FEchoAdditionalData &thisData, const FString &key, const T &writeValue)
{
	//Q: what if this inherently can fail?
	FString writeString = StringUtil::TryWriteTyped<T>(writeValue);
	thisData.WriteStringRaw(key, writeString);
}

bool FEchoAdditionalData::ReadBool(const FString &key, bool defaultValue) const
{
	return ReadAdditionalDataTyped<bool>(*this, key, defaultValue);
}

int32 FEchoAdditionalData::ReadInt(const FString &key, int32 defaultValue) const
{
	return ReadAdditionalDataTyped<int32>(*this, key, defaultValue);
}

	
float FEchoAdditionalData::ReadFloat(const FString &key, float defaultValue) const
{
	return ReadAdditionalDataTyped<float>(*this, key, defaultValue);
}

void FEchoAdditionalData::WriteBool(const FString &key, bool value)
{
	WriteAdditionalDataTyped<bool>(*this, key, value);
}

void FEchoAdditionalData::WriteFloat(const FString &key, float value)
{
	WriteAdditionalDataTyped<float>(*this, key, value);
}

void FEchoAdditionalData::WriteInt(const FString &key, int32 value)
{
	WriteAdditionalDataTyped<int32>(*this, key, value);
}

//////////////////////////

//these need to be in the cpp file so they can use circularly referenced types
FEchoImportConfigPin FEchoImportConfig::Pin() const
{
	return FEchoImportConfigPin(*this);
}

FEchoImportMeshConfigPin FEchoImportMeshConfig::Pin() const
{
	return FEchoImportMeshConfigPin(*this);
}
