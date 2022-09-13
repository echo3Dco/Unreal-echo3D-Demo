// Fill out your copyright notice in the Description page of Project Settings.

#include "EchoHelperLib.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "EchoConnection.h"

//for time
//#include "GenericPlatform/GenericPlatformTime.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////// ENDPOINT STUFF ////////////////////////////////////////////////////////////
namespace EchoHelperLib
{
	//TODO: replace this with something more complex later on if desired
	//const bool s_hitDevelopmentEndpoint = true;
	//bool s_hitDevelopmentEndpoint_ = true;
	bool s_hitDevelopmentEndpoint_ = false; //lets get into the correct mindset now instead of at some future point where we forget to set this false
	
	void SetUseDevelopmentEndpoint(bool setUseDevEndpoint)
	{
		s_hitDevelopmentEndpoint_ = setUseDevEndpoint;
		//TODO: figure out how best to reset static settings after exiting PIE?
		//useDevelopmentEndpoint = bSetUseDevEndpoint;
		if (s_hitDevelopmentEndpoint_)
		{
			//warning
			//UE_LOG(LogTemp, Warning, TEXT("Development endpoint activated. This will persist across application runs until the editor is closed or this is called with false."));
			UE_LOG(LogTemp, Warning, TEXT("Development endpoint activated."));
		}
	}

	bool GetUseDevelopmentEndpoint()
	{
		return s_hitDevelopmentEndpoint_;
	}

	const FString urlPrefix("https://");

	const FString devDomain("api.echo3d.dev");
	const FString prodDomain("api.echo3d.co");
	
	const FString urlMainEndpoint("/query");
	const FString urlArgNameAPIKey("key");
	const FString urlArgSecKey("secKey");
	
	
	const FString urlArgTagArray("tags");
	const FString urlArgEntryArray("entries");
	const FString urlArgEntrySingle("entry");

	const FString urlArgStorageId("file");
	
	const bool useDebugFilename = false;//deprecated does not work with echo console anymore
	//const bool useDebugFilename = true;
	const FString urlArgDebugFilename("debug_filename"); //for debugging

	//const bool useSrcParam = false;
	const bool useSrcParam = true;
	const FString urlArgSrc("src");
	const FString urlForcedValueSrc("UnrealSDK");
	
	//deprecated - appears to be no longer supported - maybe pass extra data along in a side channel?
	//const bool useDebugNonce = true;
	const bool useDebugNonce = false; //DEPRECATED - will be fully removed later on
	const FString urlArgDebugNonce("debug_nonce");
	int urlNonceCounter = 0;

	//FString AppendQueryArg(const FString &base, const FString &argName, const FString &argValue, bool isFirstArg = false)
	FString AppendQueryArg(const FString &base, const FString &argName, const FString &argValue, bool isFirstArg = false);

	FString AppendQueryArg(const FString &base, const FString &argName, const FString &argValue, bool isFirstArg)
	{
		//TODO: ensure things are url encoded?
		//TODO: do we really need base or can we just append the addition?
		return base + (isFirstArg ? "?" : "&") + argName + "=" + argValue;
	}

	//Q: what does &src do anyways?

	FString GenerateEchoBaseURL(bool bDev, const FEchoConnection &fromConnection)
	{
		const FString &usingDomain = bDev ? devDomain : prodDomain;
		FString ret = urlPrefix + usingDomain + urlMainEndpoint;
		ret = AppendQueryArg(ret, urlArgNameAPIKey, fromConnection.apiKey, true);
		if (!fromConnection.securityKey.IsEmpty())
		{
			ret = AppendQueryArg(ret, urlArgSecKey, fromConnection.securityKey);
		}
		return ret;
	}

	//#define TRY_APPEND_SRC_PARAM(url)
	#define TRY_APPEND_TRACKING_PARAMS(url) \
		if (useSrcParam) \
		{ \
			url = AppendQueryArg( url , urlArgSrc, urlForcedValueSrc); \
		} \
		if (useDebugNonce) \
		{ \
			url = AppendQueryArg(url, urlArgDebugNonce, FString::FromInt(urlNonceCounter++)); \
		}
	//////////////////////////////
	
	//FString GenerateEchoStorageRequestURL(const FString &base, const FString &storageId)
	//FString GenerateEchoStorageRequestURL(const FEchoConnection &fromConnection, const FString &storageId)
	FString GenerateEchoStorageRequestURL(const FEchoConnection &fromConnection, const FEchoFile &storage)
	{
		FString url = GenerateEchoBaseURL(GetUseDevelopmentEndpoint(), fromConnection);
		url = AppendQueryArg(url, urlArgStorageId, storage.storageId);
		if (useDebugFilename)
		{
			
			url = AppendQueryArg(url, urlArgDebugFilename, StringUtil::UrlEncode(storage.filename));
		}
		TRY_APPEND_TRACKING_PARAMS(url);
		return url;
	}
	
	FString GenerateRequestCSV(const FString &base, const FString &argName, const TArray<FString> &forArray)
	{
		if (forArray.Num() < 1)
		{
			//omit arg
			return base;
		}
		
		//FString ret = base;
		FString argString = "";
		bool bFirst = true;
		for(const FString &entry: forArray)
		{
			if (!bFirst)
			{
				argString += ", ";
			}
			bFirst = false;
			argString  += entry;
		}

		if (argString.IsEmpty())
		{
			return base;
		}
		else
		{
			//argString = FGenericPlatformHttp::UrlEncode(argString);
			return AppendQueryArg(base, argName, argString);
		}
		//return ret;
	}

	FString GenerateEchoDatabaseRequestURL(const FEchoConnection &fromConnection, const TArray<FString> &forEntries, const TArray<FString> &forTags)
	{
		FString ret = GenerateEchoBaseURL(GetUseDevelopmentEndpoint(), fromConnection);
		int numEntries = forEntries.Num();
		if ((numEntries <= 1)&&(forTags.Num() == 0))
		{
			if (numEntries == 0)
			{
				//no extra args
			}
			else
			{
				ret = AppendQueryArg(ret, urlArgEntrySingle, forEntries[0]);
			}
		}
		else
		{
			ret = GenerateRequestCSV(ret, urlArgEntryArray, forEntries);			
			ret = GenerateRequestCSV(ret, urlArgTagArray, forTags);			
		}

		TRY_APPEND_TRACKING_PARAMS(ret);
		return ret;
	}

	FString ResolveQueryDebugInfo(const FEchoQueryDebugInfo &queryDebugState)
	{
		//ECHO_FORMAT_ARG_UINT64
		//return FString::Format(TEXT("Query[" ECHO_FORMAT_ARG_UINT64 "]: %s"), queryDebugState.queryNumber, *queryDebugState.query);
		return FString::Printf(TEXT("Query[" ECHO_FORMAT_ARG_UINT64 "]: %s"), queryDebugState.queryNumber, *queryDebugState.query);
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//FString StringFromRawContent(const TArray<uint8> &source)//, int32 length = -1)
	FString StringFromRawContent(const FEchoBlob &source)//, int32 length = -1)
	{
		//source.Num(
		//based on WinHttpResponse.cpp. Source is a non-null terminated binary blob
		FUTF8ToTCHAR TCHARData(reinterpret_cast<const ANSICHAR*>(source.GetData()), source.Num());
		//FString ret = FString(TCHARData.Length(), TCHARData.Get());
		//ret += '\0';
		//return ret;
		return FString(TCHARData.Length(), TCHARData.Get());
	}

};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////// DELEGATE STUFF ////////////////////////////////////////////////////////////
namespace EchoHelperLib
{
	//DelegateStatusHelper CheckDelegateStatusNoisyImpl(DelegateStatus foundStatus, const FString &customMessage = EchoStringConstants::EmptyString)
	DelegateStatusHelper CheckDelegateStatusNoisyFromStatus(DelegateStatus foundStatus, const FString &customMessage)
	{
		DelegateStatusHelper ret;
		ret.bCallable = false;
		ret.bError = true;
		ret.actualStatus = foundStatus;
		ret.bNeverBound = false;

		//"DelegateStatus is Invalid"
		#define E3D_LOCAL_STATUS_LOG_HELPER(baseMsg) \
			if (customMessage.IsEmpty()) \
			{ \
				UE_LOG(LogTemp, Error, TEXT( baseMsg ) ); \
			} \
			else \
			{ \
				UE_LOG(LogTemp, Error, TEXT( baseMsg ": %s") , *customMessage); \
			}

		#define E3D_LOCAL_STATUS_LOG_HELPER_ARGS(baseMsg, ...) \
			if (customMessage.IsEmpty()) \
			{ \
				UE_LOG(LogTemp, Error, TEXT( baseMsg ), ## __VA_ARGS__ ); \
			} \
			else \
			{ \
				UE_LOG(LogTemp, Error, TEXT( baseMsg ": %s"), ## __VA_ARGS__ , *customMessage); \
			}
		/*
		//needs VA_OPT support =(
		{ \
				UE_LOG(LogTemp, Error, TEXT( baseMsg ), __VA_OPT__(,) __VA_ARGS__ ); \
			} \
			else \
			{ \
				UE_LOG(LogTemp, Error, TEXT( baseMsg ": %s") __VA_OPT__(,) __VA_ARGS__ , *customMessage); \
			}
	
		*/
		/*
			{ \
				UE_LOG(LogTemp, Error, TEXT( baseMsg ), __VA_ARGS__ ); \
			} \
			else \
			{ \
				UE_LOG(LogTemp, Error, TEXT( baseMsg ": %s"), __VA_ARGS__ , *customMessage); \
			}

		*/
		//UE_LOG(LogTemp, LogError, TEXT( baseMsg ), __VA_OPT__(,) __VA_ARGS__ );
		//UE_LOG(LogTemp, LogError, TEXT( baseMsg ": %s") __VA_OPT__(,) __VA_ARGS__ , *customMessage);

		//UE_LOG(LogTemp, LogError, TEXT( baseMsg ) __VA_OPT__(,) __VA_ARGS__ );
		//UE_LOG(LogTemp, LogError, TEXT( baseMsg ": %s"), *customMessage __VA_OPT__(,) __VA_ARGS__);
		switch(foundStatus)
		{
			case DelegateStatuses::Callable:
				ret.bCallable = true;
				ret.bError = false;
				break;

			case DelegateStatuses::NeverBound:
				ret.bNeverBound = true;
				ret.bError = false;
				break;

			case DelegateStatuses::Invalid:
				E3D_LOCAL_STATUS_LOG_HELPER("DelegateStatus is Invalid");
				break;		
		
			case DelegateStatuses::Unknown:
				E3D_LOCAL_STATUS_LOG_HELPER("DelegateStatus is Invalid");
				//UE_LOG(LogTemp, LogError, TEXT("DelegateStatus is Unknown!: %s"), *customMessage);
				break;		
			default:
				E3D_LOCAL_STATUS_LOG_HELPER_ARGS("Unhandled DelegateStatus = %d", (int)foundStatus);
				//UE_LOG(LogTemp, LogError, TEXT("Unhandled DelegateStatus = %d"), (int)foundStatus);
				break;
		}

		#undef E3D_LOCAL_STATUS_LOG_HELPER

		return ret;
	}


	/*
	//template<TBaseDynamicDelegate<>
	DelegateStatus ResolveDelegateStatus(const T &forDelegate)
	{
	
		//GetFunctionName
	}
	*/
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////// TIME STUFF ////////////////////////////////////////////////////////////
namespace EchoHelperLib
{
	//TODO: maybe make a complex helper struct/union?
	
	//::Crying:: ::Sob:: blueprint doesn't support double so we need to convert this stuff to floats =(
	double GetCurrentTimeInternal()
	{
		return FPlatformTime::Seconds();;
		//return 0; //not implemented yet
		//return GenericPlatformTime::Seconds();
	}

	const double zeroTime = GetCurrentTimeInternal();
	double GetCurrentTime()
	{
		//TODO: also watch for overflow etc since the time point is ill defined.
		double nowTime = GetCurrentTimeInternal();
		double diff = nowTime - zeroTime;
		
		//horrible hack
		//return (float)diff;
		return diff;
	}

	FEchoTimeMeasurement GetCurrentTime_BP()
	{
		//TODO: either deprecate this or somehow wrap it in some functionality that is blueprint friendly?
		//return (float)GetCurrentTime();
		double now = GetCurrentTime();
		FEchoTimeMeasurement ret;
		ret.raw = now;
		return ret;
	}
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////// ???????? STUFF ////////////////////////////////////////////////////////////
