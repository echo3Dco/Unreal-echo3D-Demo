// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

//need our "public" types
#include "EchoStructsFwd.h"
//#include "EchoConnection.h"

#include "EchoStringConstants.h"
//EchoHelperLib

//TODO: rename to EchoHelperUtil?

/*
class EchoHelperLib
*/

//this will look like a "class"
namespace EchoHelperLib
{	
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//Endpoint
	
	//Public
	//FString GenerateEchoStorageRequestURL(const FEchoConnection &fromConnection, const FString &storageId);
	FString GenerateEchoStorageRequestURL(const FEchoConnection &fromConnection, const FEchoFile &storage);
	FString GenerateEchoDatabaseRequestURL(const FEchoConnection &fromConnection, const TArray<FString> &forEntries = EchoStringConstants::EmptyStringArray, const TArray<FString> &forTags = EchoStringConstants::EmptyStringArray);

	//Use this to convert raw response content to a string later on (instead of passing strings around directly!)
	FString StringFromRawContent(const TArray<uint8> &source);

	FString ResolveQueryDebugInfo(const FEchoQueryDebugInfo &queryDebugState); //meant to be used by blueprint to print otherwise opaque-ish debug info
	//FString StringFromRawContent(const TArray<uint8> &source, int32 length);
	
	//Private
	//extern const bool s_hitDevelopmentEndpoint;
	//FString AppendQueryArg(const FString &base, const FString &argName, const FString &argValue, bool isFirstArg = false);
	//FString GenerateEchoBaseURL(bool bDev, const FEchoConnection &fromConnection);
	
	//Echo Development Only
	void SetUseDevelopmentEndpoint(bool setUseDevEndpoint);
	bool GetUseDevelopmentEndpoint();
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//Delegate Helpers
	class DelegateStatuses
	{
	public:
		enum DelegateStatus
		{
			Unknown = -1,
			Callable = 0,
			NeverBound = 1,
			Invalid = 2
		};
	};
	typedef DelegateStatuses::DelegateStatus DelegateStatus;

	struct DelegateStatusHelper
	{
	public:
		bool bCallable;
		bool bNeverBound;
		bool bError;	//not callable and not neverBound
		DelegateStatus actualStatus;
	};

	template<class T>
	DelegateStatusHelper CheckDelegateStatusNoisy(const T &forDelegate, const FString &customMessage = EchoStringConstants::EmptyString)
	{
		DelegateStatus foundStatus = ResolveDelegateStatus(forDelegate);
		return CheckDelegateStatusNoisyFromStatus(foundStatus, customMessage);
	}

	//this appears to be unusable for non-dynamic delegates?
	template<class T>
	DelegateStatus ResolveDelegateStatus(const T &forDelegate)
	{
		DelegateStatus retVal = DelegateStatuses::Unknown;
		static const T emptyDelegate;
		if (emptyDelegate == forDelegate)
		{
			return DelegateStatuses::NeverBound;
		}
		/*
		//TODO: figure out how to pick one of these for maximum paranoia using template deduction stuffs? SFINAE? or https://en.cppreference.com/w/cpp/types/enable_if ???
		//else if (forDelegate.TryGetBoundFunctionName() == NAME_None)
		*/
		else if (forDelegate.GetFunctionName() == NAME_None) 
		{
			//should this be a separate case?
			return DelegateStatuses::NeverBound;
		}
		
		return forDelegate.IsBound() ? DelegateStatuses::Callable : DelegateStatuses::Invalid;
	}

	//private	
	DelegateStatusHelper CheckDelegateStatusNoisyFromStatus(DelegateStatus foundStatus, const FString &customMessage = EchoStringConstants::EmptyString);
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///Time stuff

	//float GetCurrentTime();
	double GetCurrentTime();

	//float GetCurrentTimeFloat()
	//float GetCurrentTime_BP()
	FEchoTimeMeasurement GetCurrentTime_BP();
};