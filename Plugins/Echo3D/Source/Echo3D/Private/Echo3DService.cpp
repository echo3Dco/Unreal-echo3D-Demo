// Fill out your copyright notice in the Description page of Project Settings.


#include "Echo3DService.h"

//#include "Json.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializerMacros.h"

#include "Runtime/Online/HTTP/Public/HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

#include "UObject/StrongObjectPtr.h"
#include "UObject/WeakObjectPtrTemplates.h"

#include "EchoHelperLib.h"
#include "EchoStringUtil.h"

#include "EchoMeshService.h"

//needed to get UWorld:
#include "Engine/GameInstance.h"

//hack for rotation behavior:
#include "GameFramework/RotatingMovementComponent.h"

#include "Engine/World.h"

#include "EchoBaseActor.h"


//TODO: maybe define LogEcho?

//probably should just be in query part
#define ECHO_QUERY_LOG_PREFIX "QUERY[" ECHO_FORMAT_ARG_UINT64 "]: "

//#include "GenericPlatform/GenericPlatform.h"

//for SIG MACRO
//#include "Misc/GeneratedTypeName.h"

//epic hack to disable optimizations
//#pragma optimize( "", off )

//TODO: make some kind of debug-only struct that has info like sourceQuery, but which we can omit for high performance builds?
//TODO: turn optimize back on
//TODO: clean up the JSON handling stuff *A LOT*
//TODO: maybe only crash on null WCO if missing?

#pragma optimize( "", off )
namespace
{
	//TODO: move this helper stuff to EchoHelperLib or something similar?
	//maybe echohelperlib and make an internal header for some of it?

	typedef TSharedPtr<FJsonObject> JO;
	
	typedef TSharedPtr<FJsonValue> JV;

	
	typedef TMap<FString, TSharedPtr<FJsonValue>> JMap;

	typedef TPair<FString, JV> JE;
	
	const FString dbField("db");
	const FString kHologram("hologram");
	const FString kAdditionalData("additionalData");

	enum class UseWhichAdditionalStorage : uint8
	{
		UseDefaultFormat = 0,
		UseObjFormat = 1,
		UseFBXFormat = 2,
		UseGLBFormat = 3,
	};
	
	//const bool useObjStorage = false;
	//const bool useObjStorage = true;
	//const bool useDefaultStorage = true;
	
	const FString kDefaultModelFilename("filename");
	const FString kDefaultModelStorage("storageID");

	const FString kObjModelStorage("objHologramStorageID");
	const FString kObjModelFilename("objHologramStorageFilename");

	const FString kFBXModelStorage("fbxHologramStorageID");
	const FString kFBXModelFilename("fbxHologramStorageFilename");

	const FString kGLBModelStorage("glbHologramStorageID");
	const FString kGLBModelFilename("glbHologramStorageFilename");

	
	const FString kTextureNames("textureFilenames");
	const FString kTextureStorages("textureStorageIDs");

	const UseWhichAdditionalStorage defaultStorageTypeToUse = UseWhichAdditionalStorage::UseGLBFormat;
	struct StorageMappingHelper
	{
		bool bAdditionalData;
		FString filenameKey;
		FString storageIDKey;
	};
	const TArray<StorageMappingHelper> storageMappingHelperArray = {
		StorageMappingHelper{ false, kDefaultModelFilename, kDefaultModelStorage },
		StorageMappingHelper{ true, kObjModelFilename, kObjModelStorage},
		StorageMappingHelper{ true, kFBXModelFilename, kFBXModelStorage},
		StorageMappingHelper{ true, kGLBModelFilename, kGLBModelStorage},
	};

	const StorageMappingHelper &LookupStorageHelper(UseWhichAdditionalStorage whichType)
	{
		//static_assert(
		uint32 i = (uint32)whichType;
		//TODO: is this cast actually the way we want to go about it?
		if ((i<0)||(i>=(uint32)storageMappingHelperArray.Num()))
		{
			UE_LOG(LogTemp, Error, TEXT("LookupStorageHelper: INVALID whichType=%d"), i);
			return storageMappingHelperArray[0];
		}
		else
		{
			return storageMappingHelperArray[i];
		}
	}

	const StorageMappingHelper &GetDefaultStorageLookupHelper()
	{
		return LookupStorageHelper(defaultStorageTypeToUse);
	}


	bool IsValidStorage(const FEchoFile &file)
	{
		return !file.storageId.IsEmpty();
	}

	bool IsValidRequest(const FEchoAssetRequest &req)
	{
		return IsValidStorage(req.fileInfo);
		//return !req.fileInfo.storageId.IsEmpty();
	}

	const bool hackFixRotation = true;
}

bool TryResolve(const JO *jo, const FJsonObject *&ret)
{
	bool ok = (jo != nullptr);
	ret = nullptr;

	if (jo == nullptr)
	{
		return false;
	}

	const FJsonObject *objValue = jo->Get();
	if (objValue == nullptr)
	{
		return false;
	}
	ret = objValue;

	if (!ok)
	{
		ret = nullptr;
	}
	return ok;
}

bool TryResolveObject(const FJsonObject *obj, const FString &key, const FJsonObject *&ret)
{
	bool ok = (obj != nullptr);
	ret = nullptr;
	if (ok)
	{
		const JO *ptr = nullptr;
		ok = ok && obj->TryGetObjectField(key, ptr);
		ok = ok && TryResolve(ptr, ret);
		ok = ok && (ret != nullptr);
	}

	if (!ok)
	{
		ret = nullptr;
	}
	return ok;
}


FEchoFile ParseStorage(const FJsonObject *obj, const FString &storageKey, const FString &filenameKey, bool &ok)
{
	FEchoFile ret;
	if (!ok)
	{
		return ret;
	}

	if (obj == nullptr)
	{
		ok = false;
		return ret;
	}

	if (!obj->TryGetStringField(storageKey, ret.storageId))
	{
		ok = false;
		return ret;
	}

	//not required unless specified
	if (!filenameKey.IsEmpty())
	{
		//TODO: add flag to make optional?
		if (!obj->TryGetStringField(filenameKey, ret.filename))
		{
			ok = false;
			return ret;
		}	
	}
	ok = true;
	return ret;
}


//TODO: maybe make a method?
//bool ParseOneHologram(const FString &label, const FJsonObject *obj, FEchoHologramInfo &result, bool &warnHere, const FString &sourceQuery)
bool ParseOneHologram(const FString &label, const FJsonObject *obj, FEchoHologramInfo &result, bool &warnHere, const FEchoQueryDebugInfo &queryDebugState)
{
	warnHere = false;
	
	//UE_LOG(LogTemp, Error, TEXT("ParseOneHologram(%s): " msg "\nQuery=%s"), *label, *sourceQuery);
	#define LOG_ERROR_IF(msg) \
		if (!ok) \
		{ \
			UE_LOG(LogTemp, Error, TEXT("ParseOneHologram(%s): " msg "\nQuery=%s"), *label, *queryDebugState.query); \
		}
	
	
	#define READ_AND_CHECK(condExpr, msg) \
		if (ok) \
		{ \
			ok = ok && condExpr; \
			LOG_ERROR_IF(msg); \
		}

	bool ok = (obj != nullptr);
	//TODO: is additionalData *ACTUALLY* required?
	//TODO: partial holograms if can't get full data??

	if (ok)
	{
		READ_AND_CHECK( 
			obj->TryGetStringField("id", result.hologramId),
			"missing .id string field"
		);
		const FJsonObject *hologramObj = nullptr;
		const FJsonObject *additionalObj = nullptr;
				
		READ_AND_CHECK( 
			TryResolveObject(obj, kHologram, hologramObj),
			"missing .hologram object field"
		);
				
		if (ok)
		{
			//needed for fbx storage id (also for additionalData)
			READ_AND_CHECK( 
				TryResolveObject(obj, kAdditionalData, additionalObj),
				"missing .additonalData object field"
			);
		}

		if (ok)
		{
			{
				const StorageMappingHelper &helper = GetDefaultStorageLookupHelper();
				const FJsonObject *whichJsonObj = helper.bAdditionalData ? additionalObj : hologramObj;
				//result.modelInfo.model = ParseStorage(hologramObj, kDefaultModelFilename, kDefaultModelStorage, ok);
				//FAIL-wrong-backwards://result.modelInfo.model = ParseStorage(whichJsonObj, helper.filenameKey, helper.storageIDKey, ok);
				//result.modelInfo.model = ParseStorage(whichJsonObj, helper.storageIDKey, helper.filenameKey, ok);
				bool modelOk = true;
				result.modelInfo.model = ParseStorage(whichJsonObj, helper.storageIDKey, helper.filenameKey, modelOk);
				if (!modelOk)
				{
					//TODO: maybe print what the storage type to use is called
					if (AEcho3DService::GetVerboseMode())
					{
						UE_LOG(LogTemp, Warning, TEXT("WARN: Query[%d] no default whichtype found (filename key: %s, storage key: %s), trying default keys! hologram=%s, defaultType=%d"), 
							queryDebugState.queryNumber,
							*helper.filenameKey, *helper.storageIDKey,
							*result.hologramId, (int)defaultStorageTypeToUse
						);
					}
					modelOk = true;
					const StorageMappingHelper &helper2 = LookupStorageHelper(UseWhichAdditionalStorage::UseDefaultFormat);
					const FJsonObject *whichJsonObj2 = helper2.bAdditionalData ? additionalObj : hologramObj;
					result.modelInfo.model = ParseStorage(whichJsonObj2, helper2.storageIDKey, helper2.filenameKey, modelOk);
				}
				ok = ok && modelOk;
			}
			LOG_ERROR_IF("missing model storage or filename");

			if (ok)
			{
				//Note: not all models have this field
						
				FJsonSerializableArray texNames;
				FJsonSerializableArray texStorages;
				bool bHaveTextures = true;
				bHaveTextures = bHaveTextures && hologramObj->TryGetStringArrayField(kTextureNames, texNames);
				bHaveTextures = bHaveTextures && hologramObj->TryGetStringArrayField(kTextureStorages, texStorages);
				if (bHaveTextures)
				{
					const int lenStorage = texStorages.Num();
					const int lenNames = texNames.Num();
					
					if (lenNames > lenStorage)
					{
						//this would be very unexpected and probably mean the json meaning changed.
						UE_LOG(LogTemp, Error, TEXT("texture names count > texture storeages count: %d > %d. hologramId=%s"), lenNames, lenStorage, *label);
					}

					if (lenNames != lenStorage)
					{
						UE_LOG(LogTemp, Warning, TEXT("ParseOneHologram: No lenNames(%d) != lenStorage(%d). hologramId=%s"), lenNames, lenStorage, *label);
						warnHere = true;
					}

					for(int i=0; i<lenStorage; i++)
					{
						FString storageId, filename;
						storageId = texStorages[i];
						if (lenNames > i)
						{
							filename = texNames[i];
						}
						FEchoFile file(storageId, filename);
						result.modelInfo.textures.Add(file);
					}
				}
				else
				{
					//not so useful now that we're using GLB
					//note that this check is technically incorrect since we're conflating which storage property to use with which kind of format we expect, and in particular 
					// this makes no sense for the "default" storages, or if we fell back to some other storage type
					if (defaultStorageTypeToUse != UseWhichAdditionalStorage::UseGLBFormat)
					{
						UE_LOG(LogTemp, Warning, TEXT("ParseOneHologram: No textures assigned. hologramId=%s"), *label);
						warnHere = true;
					}
				}

				//Additional data dict
				if (ok)
				{
					if (additionalObj != nullptr)
					{
						for(const JE &row: additionalObj->Values)
						{
							//TODO: maybe do more sanity checks later?
							result.additionalData.additionalData.Add(row.Key, row.Value.Get()->AsString());
						}
					}
				}
			}
		}
	}

	#undef READ_AND_CHECK
	#undef LOG_ERROR_IF
	return ok;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Begin Class Implementation

//TODO: which service to use?
bool AEcho3DService::bVerboseMode = false;
bool AEcho3DService::bDebugTemplateOperations = false;
bool AEcho3DService::logFailedQueries = false;
bool AEcho3DService::hideDefaultUnwantedWarnings = true;

TArray<FEchoCallback> AEcho3DService::queuedPostInitTasks;
TWeakObjectPtr<UWorld> AEcho3DService::lastPlayWorld = nullptr;
bool AEcho3DService::bInitCalled = false;

TMap<FString, TStrongObjectPtr<const UEchoHologramBaseTemplate>> AEcho3DService::templates;
TStrongObjectPtr<const UEchoHologramBaseTemplate> AEcho3DService::defaultTemplate;

//should this be Warning instead?
#define HINT_SHOWERRORS() \
		if (!GetLogFailedQueries()) \
		{ \
			UE_LOG(LogTemp, Error, TEXT("\tCall with SetLogFailedQueries(true) to print responses")); \
		}

//FEchoHologramQueryResult AEcho3DService::ParseEchoHologramQuery_Impl(const FString &sourceQuery, bool bSuccess, const FString &response)
FEchoHologramQueryResult AEcho3DService::ParseEchoHologramQuery_Impl(const FEchoQueryDebugInfo &queryDebugState, bool bSuccess, const FString &response)
{
	FEchoHologramQueryResult ret;
	ret.isValid = false;
	if (!bSuccess)
	{
		//UE_LOG(LogTemp, Error, TEXT("ParseEchoHologramQuery: rawQuery was failed query!"));
		//UE_LOG(LogTemp, Error, TEXT("ParseEchoHologramQuery: The query[" ECHO_FORMAT_ARG_UINT64 "] failed. No holograms will be decoded."), queryDebugState.queryNumber);
		UE_LOG(LogTemp, Error, TEXT(ECHO_QUERY_LOG_PREFIX "ParseEchoHologramQuery: The query failed. No holograms will be decoded."), queryDebugState.queryNumber);
		//HINT_SHOWERRORS();
		return ret;
	}

	//based in part on https://www.tomlooman.com/unreal-engine-async-blueprint-http-json/
	// Deserialize object
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<>::Create(response);
	FJsonSerializer::Deserialize(JsonReader, JsonObject);
		
	bool okOuter = true;
	bool warns = false;
	const JO *db = nullptr;
	ret.hadWarnings = false;
	ret.numMissing = 0;
		
	//this is paranoid and horrible?
	okOuter = okOuter && JsonObject->TryGetObjectField(dbField, db) && (db != nullptr);
	int numGot = 0;
	int numTried = 0;
	if (okOuter)
	{
		const JMap &dbData = db->Get()->Values;
		ret.holograms.Reserve(dbData.Num());
		for(const JE &hologramEntry: dbData)
		{
			numTried++;
			bool ok = true;

			FEchoHologramInfo newEntry;
			const FJsonObject *obj = nullptr;
			const JV jv = hologramEntry.Value;
			const JO *jo = nullptr;
			bool bGotToParseOneHologramOK = false;
			ok = ok && (jv != nullptr);
			if (ok)
			{
				FJsonValue *fjv = jv.Get();
				ok = ok && (fjv != nullptr);
				if (ok)
				{
					ok = ok && fjv->TryGetObject(jo);
				}
				//TODO: is there a way to get line numbers, or at least character positions from FJsonObject(s)??
				ok = ok && (jo != nullptr);
				if (ok)
				{
					ok = ok && TryResolve(jo, obj);
					bGotToParseOneHologramOK = ok;
					bool warnThis = false;
					ok = ok && ParseOneHologram(hologramEntry.Key, obj, newEntry, warnThis, queryDebugState);//rawQuery.sourceQuery);
					warns = warns || warnThis;
					if (!ok)
					{
						UE_LOG(LogTemp, Error, TEXT("ParseEchoHologramQuery: failed to parse hologram: %s"), *hologramEntry.Key);
						ret.numMissing++;
						warns = true;
						continue; //keep trying
					}
					if (warnThis)
					{
						UE_LOG(LogTemp, Warning, TEXT("ParseEchoHologramQuery: Warning parsing a hologram: %s"), *hologramEntry.Key);
						warns = true;
					}
					ret.holograms.Add(newEntry);
					numGot++;
				}
			}
			if (!bGotToParseOneHologramOK)
			{
				UE_LOG(LogTemp, Error, TEXT("ParseEchoHologramQuery: failed to get to parse one hologram key=%s"), *hologramEntry.Key);
			}
		}
		if (numTried != numGot)
		{
			warns = true;
			UE_LOG(LogTemp, Warning, TEXT("ParseEchoHologramQuery: failed to parse %d holograms. Tried: %d, Parsed: %d"), numTried-numGot, numTried, numGot);
			//TODO: is valid?
			
		}
	}
	
	ret.hadWarnings = warns;
		
	if (!okOuter)
	{
		//UE_LOG(LogTemp, Error, TEXT("ParseEchoHologramQuery: Parse failed! Query=%s"), *sourceQuery);
		//UE_LOG(LogTemp, Error, TEXT("ParseEchoHologramQuery: Parse failed! Query[" ECHO_FORMAT_ARG_UINT64 "]=%s"), queryDebugState.queryNumber, *queryDebugState.query);
		UE_LOG(LogTemp, Error, TEXT(ECHO_QUERY_LOG_PREFIX "ParseEchoHologramQuery: Parse failed! Query=%s"), queryDebugState.queryNumber, *queryDebugState.query);
		ret.isValid = false;
		return ret;
	}

	if (warns)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseEchoHologramQuery: Some Warnings!"));
	}

	if (ret.numMissing > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseEchoHologramQuery: Failed to parse %d holograms!"), ret.numMissing);
	}

	ret.isValid = true;
	return ret;
}

//#pragma optimize( "", on )
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//WCO dependent functionality

//const char* wcoCheckError = Label ": WCO was nullptr"; 
//UE_LOG(LogTemp, Error, TEXT( wcoCheckError ) )); 
//TODO: maybe inline the _ECHO_PRETTY_FUNCTION_ instead of a Label argument?
/*
#define CHECK_AND_CAPTURE_WCO( wcoWeak, WorldContextObject, Label ) \
	if (WorldContextObject == nullptr) \
	{ \
		UE_LOG(LogTemp, Error, TEXT( "%s: WCO was nullptr" ), Label ); \
	} \
	const TWeakObjectPtr<const UObject> wcoWeak = WorldContextObject;


#define RESOLVE_WCO( wcoWeak, WorldContextObject, Label ) \
	const UObject *WorldContextObject = nullptr; \
	if (wcoWeak.IsStale()) \
	{ \
		UE_LOG(LogTemp, Error, TEXT( "%s: WCO Destroyed!" ), Label ); \
		return; \
	} \
	WorldContextObject = wcoWeak.Get();
*/
//UE_LOG(LogTemp, Error, TEXT( "%s: WCO was nullptr" ), TEXT( _ECHO_PRETTY_FUNCTION_ ) );
//UE_LOG(LogTemp, Error, TEXT( "%s: WCO Destroyed!" ), TEXT( _ECHO_PRETTY_FUNCTION_ ) ); 
//const FString check_and_capture_wco_FuncName(_ECHO_PRETTY_FUNCTION_); 
//_ECHO_PRETTY_FUNCTION_FSTRING_
//UE_LOG(LogTemp, Error, TEXT( "%s: WCO was nullptr" ), *FString(_ECHO_PRETTY_FUNCTION_) ); 
//_ECHO_PRETTY_FUNCTION_FSTRING_

//const TWeakObjectPtr<const UObject> wcoWeak = WorldContextObject;
#define CHECK_AND_CAPTURE_WCO( wcoWeak, WorldContextObject, Label) \
	if (WorldContextObject == nullptr) \
	{ \
		UE_LOG(LogTemp, Error, TEXT( "%s: WCO was nullptr" ), TEXT(Label) ); \
	} \
	const TWeakObjectPtr<UObject> wcoWeak = WorldContextObject;

//const UObject *WorldContextObject = nullptr;
#define RESOLVE_WCO( wcoWeak, WorldContextObject, Label ) \
	UObject *WorldContextObject = nullptr; \
	if (wcoWeak.IsStale()) \
	{ \
		UE_LOG(LogTemp, Error, TEXT( "%s: WCO Destroyed!" ), TEXT(Label)); \
		return; \
	} \
	WorldContextObject = wcoWeak.Get();

#ifdef WITH_CHECK
	#define CHECK_CONDITION_WCO_LOWLEVEL(wco) (wco->IsValidLowLevel())
#else
	#define CHECK_CONDITION_WCO_LOWLEVEL(wco) (false)
#endif

#define CHECK_WCO_VALID( wco, Label ) \
	if (( wco == nullptr) || CHECK_CONDITION_WCO_LOWLEVEL( wco )) \
	{ \
		UE_LOG(LogTemp, Error, TEXT( "%s: NULL or invalid WCO" ), TEXT(Label)); \
	}

/////////////////////////////////////////////////////////////////
void AEcho3DService::HandleDefaultMetaData(AActor *actor, const FEchoHologramInfo &hologram)
{
	//const bool bConvert//???
	float scale = 1;
	float x = 0;
	float y = 0;
	float z = 0;
	float rotateDir = 0;
	float xAngle = 0;
	float yAngle = 0;
	float zAngle = 0;

	const FString kX("x");
	const FString kY("y");
	const FString kZ("z");

	const FString kScale("scale");
	const FString kDirection("direction");
	
	const FString kXAngle("xAngle");
	const FString kYAngle("yAngle");
	const FString kZAngle("zAngle");
	FTransform transform = actor->GetTransform();
	//FQuat applyRotation = FQuat::Identity;

	{
		float currentScale = 1;
		currentScale = hologram.additionalData.ReadFloat(kScale, currentScale);
		transform.SetScale3D( transform.GetScale3D() * currentScale);
	}
	{
		FVector pos = transform.GetLocation();
		x = pos.X;
		y = pos.Y;
		z = pos.Z;
		x = hologram.additionalData.ReadFloat(kX, x);
		y = hologram.additionalData.ReadFloat(kY, y);
		z = hologram.additionalData.ReadFloat(kZ, z);
		pos = FVector(x, y, z);
		transform.SetLocation(pos);
	}
	{
		FQuat quatCurrent = transform.GetRotation();
		FVector euler(0,0,0);
		//NB: unreal expects these in degrees
		xAngle = euler.X;
		yAngle = euler.Y;
		zAngle = euler.Z;
		xAngle = hologram.additionalData.ReadFloat(kXAngle, xAngle);
		yAngle = hologram.additionalData.ReadFloat(kYAngle, yAngle);
		zAngle = hologram.additionalData.ReadFloat(kZAngle, zAngle);
		euler = FVector(xAngle, yAngle, zAngle);
		FQuat quatRotateOuter = FQuat::MakeFromEuler(euler);
		FQuat finalRot = quatRotateOuter * quatCurrent;
		transform.SetRotation(finalRot);
	}
	{
		const float RotationRatePerSecond = 45.0f;//degrees/s
		FString strDirection = "";
		strDirection = hologram.additionalData.ReadString(kDirection, strDirection);
		if (strDirection == "left")
		{
			rotateDir = -1;//cw
		}
		else if (strDirection == "right")
		{
			rotateDir = +1;//ccw?
		}

		float rotation = rotateDir * RotationRatePerSecond;
		if (rotation != 0)
		{
			UE_LOG(LogTemp, Error, TEXT("NOTE: ADDING ROTATOR!"));
			USceneComponent *rootComp = actor->GetRootComponent();
			//UE_LOG(LogTemp, Error, TEXT("TODO: implement a component to perform direction rotation! rotation: %f"), rotation);
			URotatingMovementComponent *rotComp = NewObject<URotatingMovementComponent>(rootComp);
			//+rotation would be CW
			rotComp->RotationRate = FRotator(0, rotation, 0); //roll
			//rotComp->AttachTo //not found?!
			//applyRotation = FQuat::MakeFromEuler(FVector(0, 0, rotation));
			rotComp->RegisterComponent();
		}
	}
	actor->SetActorTransform(transform);
	//actor->SetActorRotation(applyRotation);
	//actor->set
}

//bool TryPushStorageIfValid(FEchoAssetRequestArray &requests, const FEchoFile &file, bool bSilent)
bool TryPushStorageIfValid(FEchoAssetRequestArray &requests, const FEchoAssetRequest &request, bool bSilent)
{
	//if (!IsValidStorage(file))
	if (!IsValidRequest(request))
	{
		if (!bSilent)
		{
			UE_LOG(LogTemp, Error, TEXT("Tried pushing invalid storage request! type=%i"), request.assetType);
		}
		return false;
	}
	requests.Push(request);
	return true;
}

void AEcho3DService::SetDefaultTemplateByClass(TSubclassOf<UEchoHologramBaseTemplate> setDefaultClass)
{
	//what if setDefaultClass is null? lets just do this and warn if we get a class holding nullptr for now - they can cal setDefaultTemplate null if they want
	if (setDefaultClass == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("SetDefaultTemplateByClass called with nullptr for class"));
		ClearDefaultTemplate();
		return;
	}
	SetDefaultTemplateImpl(ResolveTemplateFromClass(setDefaultClass), false);
}

void AEcho3DService::SetDefaultTemplateImpl(const UEchoHologramBaseTemplate *setDefaultTemplate, bool bNullExpected)
{
	bool bNullMatched = (setDefaultTemplate == nullptr) == bNullExpected;
	if (!bNullMatched)
	{
		UE_LOG(LogTemp, Error, TEXT("SetDefaultTemplateImpl: null case unexpected! setDefaultTemplate: %p, expected null? %s"), (void*)setDefaultTemplate, *StringUtil::BoolToString(bNullMatched));
	}
	defaultTemplate = TStrongObjectPtr<const UEchoHologramBaseTemplate>(setDefaultTemplate);
}

const UEchoHologramBaseTemplate *AEcho3DService::ResolveTemplateAndCreateConfig(
	UObject *WorldContextObject, const FEchoConnection &usingConnection, const FEchoHologramInfo &hologramInfo,
	const UEchoHologramBaseTemplate *hologramTemplate,
	FEchoImportConfig &outputConfig
)
{
	//FEchoImportConfig ret;
	outputConfig.WorldContextObject = WorldContextObject;	
	outputConfig.importTitle = StringUtil::IdentifiersOnly( hologramInfo.modelInfo.model.filename + "_" + hologramInfo.hologramId );
	//outputConfig.inputBlueprintKey = blueprintKey;
	//outputConfig.finalTemplateKey = blueprintKey;
	outputConfig.hologram = hologramInfo;
	outputConfig.connection = usingConnection;
	outputConfig.instanceState = nullptr;
	outputConfig.hologramTemplate = hologramTemplate;//will replace later
		
	
	//outputConfig.hologramTemplate = nullptr; //no template found/resolved

	//const UEchoHologramBaseTemplate *userTemplate = hologramTemplate;
	if (hologramTemplate != nullptr)
	{
		//hologramTemplate = hologramTemplate->ResolveTemplate(WorldContextObject, outputConfig);
		hologramTemplate = hologramTemplate->ResolveTemplate(outputConfig);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ResolveTemplateAndCreateConfig: importing '%s': null hologramTemplate recieved"), *outputConfig.importTitle);
	}
	//userTemplate = GetTemplate(blueprintKey);
	/*
	//now handled in ResolveTemplate, though not recursively
	if (userTemplate != nullptr)
	{
		UEchoHologramBaseTemplate *userTemplate2 = nullptr;
		//TODO: custom resovler function?
		while(!userTemplate->userTemplateKey.IsEmpty())
		{
			FString newTemplateKey = hologramInfo.additionalData.ReadString(userTemplate->userTemplateKey);
			outputConfig.finalTemplateKey = newTemplateKey;
			if (!newTemplateKey.IsEmpty())
			{
				//UE_LOG(
				//userTemplate = nullptr;
				userTemplate2 = GetTemplate(newTemplateKey);
				if (userTemplate2 != nullptr)
				{
						
					userTemplate = userTemplate2;
					continue;
				}
			}
			break;
		}
	}
	*/

	//NB: print before abstract test so we can still see the results
	{
		//TODO: can getclass ever return null?
		if (bDebugTemplateOperations)
		{
			const FString &templateNameFound = (hologramTemplate != nullptr) ? hologramTemplate->GetClass()->GetName() : EchoStringConstants::NullString;
			UE_LOG(LogTemp, Error, TEXT("DEDUCED TEMPLATE FOR Hologram: %s (model %s): %s"), *hologramInfo.hologramId, *hologramInfo.modelInfo.model.filename, *templateNameFound);
		}
	}

	//const UEchoHologramBaseTemplate *retUserTemplate = userTemplate;
	//if (retUserTemplate != nullptr)
	/*
	if (hologramTemplate != nullptr)
	{
		//retUserTemplate = retUserTemplate->ResolveTemplate(WorldContextObject, ret);
		hologramTemplate = hologramTemplate ->ResolveTemplate(ret);
		if (outputConfig.importTitle.IsEmpty()) //sanity check
		{
			UE_LOG(LogTemp, Error, TEXT("Warning: ResolveTemplate appears to have trashed our importConfig!"));
		}
	}
	*/
	//ensure we dont build from an "abstract" template:
	if (hologramTemplate != nullptr)
	{
		if (hologramTemplate->IsAbstractTemplate())//abstractTemplate)//isAbstractTemplate))
		{
			//TODO: need to provide a way to track down these resolutions for debugging sanity?
			//TODO: probably fail here?
			//UE_LOG(LogTemp, Error, TEXT("ERROR: Tried to instantiate an abstract echo template: %s via %s"), *userTemplate.templateKey, *lastVia);
			//UE_LOG(LogTemp, Error, TEXT("ERROR: Tried to instantiate an abstract echo template: %s via %s importing hologram %s"), *hologramTemplate->templateKey, *outputConfig.finalTemplateKey, *outputConfig.importTitle);
			//UE_LOG(LogTemp, Error, TEXT("ERROR: Tried to instantiate an abstract echo template: %s via %s importing hologram %s"), *hologramTemplate->GetTemplateDebugString(), *outputConfig.importTitle);
			UE_LOG(LogTemp, Error, TEXT("ERROR: Tried to instantiate an abstract echo template: %s importing hologram %s"), *hologramTemplate->GetTemplateDebugString(), *outputConfig.importTitle);
			hologramTemplate = nullptr;
		}
	}


	if (outputConfig.importTitle.IsEmpty()) //sanity check
	{
		UE_LOG(LogTemp, Error, TEXT("Warning: Something appears to have trashed our importConfig!"));
	}
	outputConfig.hologramTemplate = hologramTemplate; //GAH
	
	/*
	if (userTemplate != nullptr)
	{
		if (userTemplate->abstractTemplate)
		{
			UE_LOG(LogTemp, Error, TEXT("ILLEGAL: Hologram template is an abstract Template!: importing: %s : template: %s"), *importTitle, *userTemplate->GetClass()->GetName());
			userTemplate = nullptr; //fail
		}
	}
	*/
	return hologramTemplate;
	//return retUserTemplate;
	//ret.hologramTemplate = userTemplate;
	//return ret;
}

/*
void AEcho3DService::ResolveActorTemplate(UObject *WorldContextObject, FEchoActorTemplateResolution &result, FEchoImportConfig importConfig, const UEchoHologramBaseTemplate *hologramTemplate)
{
	//actorResolutionsBase
	result.Reset(); //zero all

	//hologram template shall already be resolved.
	//TODO: perhaps double check for abstractness here?
	result.hologramTemplate = hologramTemplate;//importConfig.hologramTemplate;//.Get();
	
	if (result.hologramTemplate != nullptr)
	{
		if (result.hologramTemplate->abstractTemplate)
		{
			UE_LOG(LogTemp, Error, TEXT("ABSTRACT TEMPLATE: %s in hologram: %s"), *result.hologramTemplate->GetTemplateName(), *importConfig.importTitle);
			result.hologramTemplate = nullptr; //force null
		}
	}

	if (result.hologramTemplate != nullptr)
	{
		result.actorTemplate = result.hologramTemplate->ResolveActorTemplate(WorldContextObject, importConfig);
		if (result.actorTemplate != nullptr)
		{
			result.actorTemplate = result.actorTemplate->ResolveTemplate(WorldContextObject, importConfig);
			//could now be null
			if (result.actorTemplate != nullptr)
			{
				result.meshTemplate = result.actorTemplate->ResolveMeshTemplate(WorldContextObject, result, importConfig);
				if (result.meshTemplate != nullptr)
				{
					result.meshTemplate = result.meshTemplate->ResolveTemplate(WorldContextObject, result, importConfig);
				}
				//etc.

			}
		}
	}
}
*/

void AEcho3DService::ProcessOneHologram(
	UObject *WorldContextObjectArg, 
	const FEchoConnection &connection, 
	//const FString &blueprintKey, 
	const UEchoHologramBaseTemplate *hologramTemplate,
	const FEchoHologramInfo &hologram, 
	UObject *useExisting, UClass *constructClass
)
{
	//static const FString ThisFunc(TEXT("AEcho3DService::ProcessOneHologram"));
	CHECK_WCO_VALID(WorldContextObjectArg, "AEcho3DService::ProcessOneHologram");
	//FString finalTemplateKey = EchoStringConstants::BadStringValue;
	//TODO: allow process with more errors allowed?
	//UE_LOG(LogTemp, Error, TEXT("TODO: ProcessHologram<%s>: {%s}: model: %s"), *blueprintKey, *hologram.hologramId, *hologram.modelInfo.model.storageId);

	//TODO: download dependency chain for assets?
	//TODO: generate asset dependencies??
	
	FEchoImportConfig importConfig;
	//const UEchoHologramBaseTemplate *hologramTemplate = ResolveTemplateAndCreateConfig(WorldContextObjectArg, importConfig, blueprintKey, hologram);
	//const UEchoHologramBaseTemplate *hologramTemplate = 
	hologramTemplate = ResolveTemplateAndCreateConfig(WorldContextObjectArg, connection, hologram, hologramTemplate, importConfig);
	//hologramTemplate->ExecTemplate(WorldContextObjectArg, importConfig, hologramTemplate);
	if (hologramTemplate == nullptr)
	{
		//TODO: only if allow default template fallbacks?
		if (defaultTemplate.Get() != nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("Importing %s: no template matched. using default template"), *importConfig.importTitle);
			hologramTemplate = defaultTemplate.Get();
		}
	}

	if (hologramTemplate != nullptr)
	{
		//hologramTemplate->ExecTemplate(WorldContextObjectArg, importConfig, connection, useExisting, constructClass);
		hologramTemplate->ExecTemplate(importConfig);
	}
	else
	{
		//we literally cannot proceed without a template anymore, which to be honest is probably correct
		//UE_LOG(LogTemp, Error, TEXT("Importing %s: NO TEMPLATE MATCHED %s - and no default template! falling back to default behavior"), *importConfig.importTitle, *importConfig.finalTemplateKey);
		//UE_LOG(LogTemp, Error, TEXT("Importing %s: NO TEMPLATE MATCHED %s - and no default template! falling back to default behavior"), *importConfig.importTitle, *importConfig.finalTemplateKey);
		//UE_LOG(LogTemp, Error, TEXT("Importing %s: NO TEMPLATE MATCHED and no default template! falling back to default behavior"), *importConfig.importTitle);
		UE_LOG(LogTemp, Error, TEXT("Importing %s: NO TEMPLATE MATCHED and no default template! - discarding hologram"), *importConfig.importTitle);
		//, *importConfig.finalTemplateKey);
		//AEcho3DService::ProcessActorHologramDefault(WorldContextObjectArg, importConfig, nullptr, connection, nullptr, nullptr, nullptr);
		//AEcho3DService::ProcessActorHologramDefault(WorldContextObjectArg, importConfig, nullptr, connection, nullptr, nullptr, nullptr);
	}
	
}

/*
//see templates
//void AEcho3DService::ProcessActorHologramDefault(UObject *WorldContextObjectArg, FEchoImportConfig &importConfig, const UEchoHologramBaseTemplate *hologramTemplate, const FEchoConnection &connection, const FString &blueprintKey, const FEchoHologramInfo &hologram, UObject *useExisting, UClass *constructClass)
void AEcho3DService::ProcessActorHologramDefault(
	UObject *WorldContextObjectArg, 
	FEchoImportConfig &importConfig, const UEchoHologramBaseTemplate *hologramTemplate, 
	const FEchoConnection &connection, 
	UObject *useExistingActor, 
	UClass *constructActorClass,
	TSubclassOf<UActorComponent> customUserActorComponent
)
{
	CHECK_AND_CAPTURE_WCO( wcoWeak, WorldContextObjectArg, "AEcho3DService::ProcessOneHologram");

	if (hologramTemplate == nullptr)
	{
		//be loud since this is *probably* unwanted
		UE_LOG(LogTemp, Error, TEXT("WARNING: null hologram template processing %s"), *importConfig.importTitle);
	}

	//TODO: make this less coupled to actor templates?
	FEchoActorTemplateResolution actorResolutionsBase;
	ResolveActorTemplate(WorldContextObjectArg, actorResolutionsBase, importConfig, hologramTemplate);
	
	TWeakObjectPtr<AActor> weakUseExistingAsActor( Cast<AActor>( useExistingActor ) );
	if ((useExistingActor != nullptr) && (weakUseExistingAsActor.Get() == nullptr)) //warn about failed cast
	{
		UE_LOG(LogTemp, Error, TEXT("importing %s: UseExisting is non-null but also not an aactor in AEcho3DService::ProcessOneHologram! is a: %s"), *importConfig.importTitle, *useExistingActor->GetClass()->GetName());
	}

	//UClass *constructActorClass is an arugment
	auto constructClassAsActor = TSubclassOf<AActor>(constructActorClass);
	TSoftClassPtr<AActor> softConstructActorClass( constructClassAsActor );
	if ((constructActorClass != nullptr) && (constructClassAsActor.Get() == nullptr))
	{
		UE_LOG(LogTemp, Error, TEXT("importing %s: ConstructActorClass is non-null but also not a TSubclassOf<AActor> in AEcho3DService::ProcessOneHologram! is a: %s"), *importConfig.importTitle, *constructActorClass->GetName());
	}

	//Do I need to worry about GC with UClass instances?
	TSoftClassPtr<UActorComponent> softUserComponent( customUserActorComponent );
	
	//TODO: make exec do all this stuffs? 
	//alternatively call a pre-exec here that can decide what to do (or do this by default if actually want assets?) or post this step and pre-download of assets?
	FEchoAssetRequestArray requests;
	{
		//generate asset requests

		const FEchoHologramInfo &hologram = importConfig.hologram;
		bool bAllowCached = true;
		const bool LoudError = false;
		const bool SilentError = true;

		TryPushStorageIfValid(requests, FEchoAssetRequest{hologram.modelInfo.model, EEchoAssetType::EEchoAsset_Mesh, bAllowCached}, LoudError);
	
		//silent error - we usually don't have actual materials specified
		TryPushStorageIfValid(requests, FEchoAssetRequest{hologram.modelInfo.material, EEchoAssetType::EEchoAsset_Material, bAllowCached}, SilentError);
		for(const auto &texFile: hologram.modelInfo.textures)
		{
			TryPushStorageIfValid(requests, FEchoAssetRequest{texFile, EEchoAssetType::EEchoAsset_Texture, bAllowCached}, LoudError);
		}

		if (actorResolutionsBase.actorTemplate != nullptr)
		{
			actorResolutionsBase.actorTemplate->ResolveAssets(WorldContextObjectArg, importConfig, requests);
		}

		//TODO: warn if empty requests
		//TODO: deal with requests remapping here?
	}

	//TODO: use embedded non-gltf textures for cupcake test model to test non-standard assets by requesting/evaluating them and using them in the material eval function?

	//TODO: only optionally resolve non-mesh requests initially?
	// or based on guessed asset type based on extension??
	//TODO: log total http requests and timing etc?? concurrent requests?? do we share one http object/manager??
	
	//TODO: simplify this lambda's capture list
	
	const FEchoImportConfig importConfigCopy = importConfig; //copy for lambda - mutable since can modify the lambda copy later on
	const FEchoConnection connectionCopy = connection;

	auto lambda = 
		[wcoWeak, weakUseExistingAsActor, softConstructActorClass, softUserComponent, connectionCopy, importConfigCopy, actorResolutionsBase]
		(const FEchoMemoryAssetArray &assets)
	{
		RESOLVE_WCO( wcoWeak, WorldContextObject, "AEcho3DService::ProcessOneHologram:[lambda]" );
		
		//TODO: .execActor here???//.exec here??
		//TODO: maybe .prepare above???

		if (weakUseExistingAsActor.IsStale())
		{
			UE_LOG(LogTemp, Error, TEXT("importing %s: existingActor was GC'd!"), *importConfigCopy.importTitle);
			return;//fail
		}

		AActor *useActor = nullptr;
		useActor = weakUseExistingAsActor.Get();
		
		FEchoImportConfig importConfigCopyInner = importConfigCopy;
		if (useActor == nullptr)
		{
			//no existing actor, try to create one
			const FString &hologramName = importConfigCopy.hologram.modelInfo.model.filename; //this is absolutely horrible
			useActor = ConstructBaseActor(WorldContextObject, hologramName, importConfigCopyInner, actorResolutionsBase, softConstructActorClass.Get(), TSubclassOf<UActorComponent>(softUserComponent.Get()));
			if (useActor == nullptr)
			{
				UE_LOG(LogTemp, Error, TEXT("Importing %s: ConstructBaseActor returned null!"), *importConfigCopyInner.importTitle);
				return;
			}
		}
		else
		{
			if (!softConstructActorClass.IsNull())
			{
				//sanity check
				//if (!softConstructActorClass.Get()->IsA( useActor->GetClass() ))
				if (!softConstructActorClass.Get()->IsChildOf( useActor->GetClass() )) //check if useActor is NOT A SUBCLASS (note the !) of the provided class
				{
					//TODO: can GetClass() ever return null??
					UE_LOG(LogTemp, Error, TEXT("Importing %s: provided a construct actor class with an existing actor but the types do not match - existing is an %s, but specified asset %s"), *importConfigCopyInner.importTitle, *useActor->GetClass()->GetName(), *softConstructActorClass.GetAssetName());
				}
			}
		}
		UEchoMeshService::Assemble(useActor, connectionCopy, assets, nullptr, importConfigCopyInner, actorResolutionsBase);
		
		//TODO: change this to an afterfunc in the template and make the default do this instead:
		HandleDefaultMetaData(useActor, importConfigCopy.hologram);

	};
		

	//TODO: build/modify requested assets here?
	FEchoMemoryAssetArrayCallback myDelegate;
	myDelegate.BindLambda(lambda);
	RequestAssets(connection, requests, myDelegate);
}
*/

//TODO: make this all a UObject and provide virtual methods to overload it for users?
//AActor *AEcho3DService::ConstructBaseActor(
FEchoConstructActorResult AEcho3DService::ConstructBaseActor(
	//UObject *WorldContextObject, const FString &setLabel, FEchoImportConfig &importConfig, FEchoActorTemplateResolution importTemplate, TSubclassOf<AActor> spawnClassOverride, TSubclassOf<UActorComponent> addUserClass
	const FEchoImportConfig &importConfig,
	const FString &setLabel, 
	TSubclassOf<AActor> spawnClassOverride, 
	TSubclassOf<UActorComponent> addUserClass,
	AActor *useExistingActor
)
{
	FEchoConstructActorResult ret;
	ret.actor = nullptr;
	ret.userComponent = nullptr;

	//UE_LOG(LogTemp, Warning, TEXT("TODO: implement Echo3DService::ConstructBaseObject"));
	UObject *WorldContextObject = importConfig.WorldContextObject.Get();
	if (importConfig.WorldContextObject.IsStale())
	{
		UE_LOG(LogTemp, Error, TEXT("ConstructBaseActor: WCO is stale!"));
		return ret;
	}
	if (WorldContextObject == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("ConstructBaseActor: WorldContextObject is nullptr!"));
		return ret;
	}

	//sanity check -- maybe nuke me later?
	if (!WorldContextObject->IsValidLowLevel())
	{
		UE_LOG(LogTemp, Error, TEXT("ConstructBaseObject: WorldContextObject is !IsValidLowLevel"));
		return ret;
	}



	//TODO: pass around a worldcontextobject thingy?
	//UWorld *world = GetWorld();
	UWorld *world = WorldContextObject->GetWorld();//GameInstance::GetWorld();

	
	const UEchoHologramBaseTemplate *hologramTemplate = importConfig.hologramTemplate;
	//if (importTemplate.hologramTemplate.IsStale())
	//if (importTemplate.hologramTemplate.IsStale())
	//if (importConfig.hologramTemplate == nullptr)
	if (hologramTemplate == nullptr)
	{
		//FAILURE - if this happens normally other than across enter/exit PIE boundaries then something is wrong
		//UE_LOG(LogTemp, Error, TEXT("hologramTemplate was destroyed. weak reference is no longer valid: %s"), *importConfig.finalTemplateKey);
		UE_LOG(LogTemp, Error, TEXT("ConstructBaseActor: hologramTemplate is null importing: %s"), *importConfig.importTitle);
		return ret;
	}
	//userTemplate = importConfig.hologramTemplate.Get();
	//const UEchoHologramBaseTemplate *userTemplate = importTemplate.hologramTemplate.Get();
	//const UEchoHologramBaseTemplate *hologramTemplate = importTemplate.hologramTemplate;//.Get();

	//TODO: This seems sus given all the stuff I did refactoring the evaltemplate stuff!!!
	if (world != nullptr)
	{
		UClass *forClass = spawnClassOverride.Get();
		if (forClass == nullptr)
		{
			/*
			if (hologramTemplate != nullptr)
			{
				//forClass = userTemplate->baseObjectClass;
				forClass = hologramTemplate->GetBaseObjectClass(); //TODO: should this be handled in the template implementation?
			}
			*/
	
			if (forClass == nullptr)
			{
				//hack
				//forClass = AActor::StaticClass();
				forClass = AEchoBaseActor::StaticClass();
			}
		}

		/*
		const UEchoImportActorTemplate *actorTemplate = nullptr;
		if (hologramTemplate != nullptr)
		{
			actorTemplate = hologramTemplate->ResolveActorTemplate(WorldContextObject, importConfig);
		}
		*/

		
		AActor *myActor = nullptr;
		myActor = useExistingActor;
		/*
		if (actorTemplate != nullptr)
		{
			myActor = actorTemplate->EvalActor(WorldContextObject, importConfig, myTransform);
			//NOTE: myActor might be null in which case we *STILL* need to do the fallback strategy
		}
		*/
		
		//TODO: provide the option to be based on an actor in echo default stuff via here?
		
		//fallback strategy
		if (myActor == nullptr)
		{
			FTransform myTransform;
			myTransform.SetIdentity();
		
			//myTransform.SetTranslation(FVector(0, 0, 200));//200cm up
			myActor = world->SpawnActor<AActor>(forClass, myTransform);
		}

		//check for failure
		if (myActor == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("SpawnActor returned nullptr!"));
			return ret;
		}

		ret.actor = myActor;
		
		AEchoBaseActor *echoActor = Cast<AEchoBaseActor>(myActor);
		if (echoActor != nullptr) //might reasonably fail, in which case we just don't do magic inspector behavior
		{
			//TODO: configure this stuff in echo3dservice if possible?
			//echoActor->importConfig = importConfig;
			//these are the probably-safe properties of importConfig:
			echoActor->hologram = importConfig.hologram;
			echoActor->hologramTemplate = importConfig.hologramTemplate;
			echoActor->importTitle = importConfig.importTitle;

			echoActor->hologramId = importConfig.hologram.hologramId;
		}

		//ensure we have a valid root component to attach stuff to
		// TODO: do we still want to do this with a custom actor template?
		USceneComponent *sceneBase = myActor->GetRootComponent();
		if (sceneBase == nullptr)
		{
			//This seems like a weird bunch of code
			//UE_LOG(LogTemp, Error, TEXT("NO Scene Base!"));//expected for AActor, handle gracefully
			sceneBase = NewObject<USceneComponent>(myActor, USceneComponent::StaticClass());
			sceneBase->SetupAttachment(myActor->GetRootComponent());
			sceneBase->RegisterComponent();
			if (hackFixRotation)
			{
				//its HORRIBLE but not worse than unity and Should make everything CORRECT? (Hopefully! ...famous last words!)
				//sceneBase->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f)); //HACK
				//sceneBase->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f)); //HACK
				sceneBase->SetRelativeRotation(FRotator(0.0f, 0.0f, -90.0f)); //HACK // THIRD TIMES THE CHARM! ???
			}
			myActor->SetRootComponent(sceneBase);
		}

		//TODO: check for and init if ACTOR is an echo template interface with the importConfig?

		//myActor->SetActorLocation(FVector(0,0,250));
		
		//spawn custom user class
		//if (userTemplate != nullptr)
		{
			//UClass *userCompClass = userTemplate->baseUserComponent;
			//UClass *userCompClass = userTemplate->GetBaseUserComponent();
			//UClass *userCompClass = addUserClass;
			TSubclassOf<UActorComponent> userCompClass = addUserClass;

			if (userCompClass != nullptr)
			{
			
				if (userCompClass != nullptr)
				{
					UActorComponent *userComponent = NewObject<UActorComponent>(myActor, userCompClass);
					
					USceneComponent *userSceneComp = Cast<USceneComponent>(userComponent);
					if (userSceneComp != nullptr)
					{
						userSceneComp->SetupAttachment(sceneBase);
					}
					
					userComponent->RegisterComponent();

					ret.userComponent = userComponent;
					//UEchoComponent
					//TODO: check for and init if CUSTOM_COMPONENT is an echo template interface with the importConfig?
				}
			}
		}
		//wow this rename can crash stuff
		//myActor->Rename(TEXT("EchoSpawnedBaseObjectActor"));
			
		#if WITH_EDITOR
			//set the label in the world outliner. This is only possible in the editor.
			if (!setLabel.IsEmpty())
			{
				myActor->SetActorLabel(setLabel);
			}
		#endif

		//return myActor;
		//return ret;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("GetWorld() returned nullptr!"));
		//return nullptr;
	}
	return ret;
}

//void AEcho3DService::ProcessAllHolograms(UObject *WorldContextObject, const FEchoConnection &connection, const FString &blueprintKey, const TArray<FEchoHologramInfo> &holograms, UClass *constructClass)
void AEcho3DService::ProcessAllHolograms(UObject *WorldContextObject, const FEchoConnection &connection, const UEchoHologramBaseTemplate *hologramTemplate, const TArray<FEchoHologramInfo> &holograms, UClass *constructClass)
{
	if (holograms.Num() < 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("ProcessAllHolograms: empty holograms array"));
	}
	for(const FEchoHologramInfo &hologram: holograms)
	{
		//ProcessOneHologram(WorldContextObject, connection, blueprintKey, hologram, nullptr, constructClass);
		ProcessOneHologram(WorldContextObject, connection, hologramTemplate, hologram, nullptr, constructClass);
	}
}

//void AEcho3DService::DefaultHologramHandler(UObject *WorldContextObject, const FEchoConnection &connection, const FString &blueprintKey, const FEchoHologramQueryResult &result)
void AEcho3DService::DefaultHologramHandler(UObject *WorldContextObject, const FEchoConnection &connection, const UEchoHologramBaseTemplate *hologramTemplate, const FEchoHologramQueryResult &result)
{
	if (!result.isValid)
	{
		//UE_LOG(LogTemp, Error, TEXT("Invalid response to hologram query! -- call SetLogFailedQueries(true) to print responses"));
		//UE_LOG(LogTemp, Error, TEXT("Invalid response to hologram query!"));
		UE_LOG(LogTemp, Warning, TEXT("DefaultHologramHandler: Invalid response to hologram query! No holograms processed."));
		//HINT_SHOWERRORS();
		return;
	}
	//ProcessAllHolograms(WorldContextObject, connection, blueprintKey, result.holograms, nullptr);
	ProcessAllHolograms(WorldContextObject, connection, hologramTemplate, result.holograms, nullptr);
}

void AEcho3DService::RequestHolograms(UObject *WorldContextObjectArg, const FEchoConnection &connection, const FString &templateKey, const TArray<FString> &forEntries, const TArray<FString> &forTags, const FEchoHologramQueryHandler &callback)
{
	if (!templates.Contains(templateKey))
	{
		UE_LOG(LogTemp, Warning, TEXT("RequestHolograms: no template currently matches key '%s'"), *templateKey);
	}
	//TODO: maybe warn if key not yet bound?
	RequestHologramsHelper(WorldContextObjectArg, connection, RequestHologramsArgHelper(templateKey), forEntries, forTags, callback);
}

void AEcho3DService::RequestHologramsTemplate(UObject *WorldContextObjectArg, const FEchoConnection &connection, const UEchoHologramBaseTemplate *templateInstance, const TArray<FString> &forEntries, const TArray<FString> &forTags, const FEchoHologramQueryHandler &callback)
{
	if (templateInstance == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("RequestHologramsTemplate: provided templateInstance is nullptr"));
	}
	RequestHologramsHelper(WorldContextObjectArg, connection, RequestHologramsArgHelper(templateInstance), forEntries, forTags, callback);
}

//void AEcho3DService::RequestHologramsTemplateClass(UObject *WorldContextObjectArg, const FEchoConnection &connection, TSoftClassPtr<UEchoHologramBaseTemplate> templateClass, const TArray<FString> &forEntries, const TArray<FString> &forTags, const FEchoHologramQueryHandler &callback)
void AEcho3DService::RequestHologramsTemplateClass(UObject *WorldContextObjectArg, const FEchoConnection &connection, TSubclassOf<UEchoHologramBaseTemplate> templateClass, const TArray<FString> &forEntries, const TArray<FString> &forTags, const FEchoHologramQueryHandler &callback)
{
	if (templateClass == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("RequestHologramsTemplate: provided templateInstance is nullptr"));
	}
	RequestHologramsHelper(WorldContextObjectArg, connection, RequestHologramsArgHelper(templateClass), forEntries, forTags, callback);
}

void AEcho3DService::RequestHologramsHelper(UObject *WorldContextObjectArg, const FEchoConnection &connection, RequestHologramsArgHelper requestedTemplateArg, const TArray<FString> &forEntries, const TArray<FString> &forTags, const FEchoHologramQueryHandler &callback)
{
	CHECK_AND_CAPTURE_WCO( wcoWeak, WorldContextObjectArg, "AEcho3DService::RequestHolograms" );
	const auto callbackStatus = EchoHelperLib::CheckDelegateStatusNoisy(callback, "RequestAllHolograms:");
	if (callbackStatus.bError)
	{
		UE_LOG(LogTemp, Error, TEXT("RequestAllHolograms: bad delegate provided. skipping request"));
		//Q: return false?
		return;
	}

	//NOTE: its easier to just pass this bool than figure out how to bind a member function to a dynamic delegate =(
	const bool bDoDefault = callbackStatus.bNeverBound;
	FEchoHologramQueryHandler callbackCopy = callback;
	
	FString url =EchoHelperLib::GenerateEchoDatabaseRequestURL(connection, forEntries, forTags);

	//FString blueprintKeyCopy = blueprintKey;
	RequestHologramsArgHelper requestedTemplateArgCopy = requestedTemplateArg;
	FEchoConnection connectionCopy = connection;
	FEchoRequestHandler handler;
	//handler.BindLambda([this, connectionCopy, blueprintKeyCopy, callbackCopy, bDoDefault](const FEchoRawQueryResult &rawQuery)
	//handler.BindLambda([wcoWeak, connectionCopy, blueprintKeyCopy, callbackCopy, bDoDefault](const FEchoRawQueryResult &rawQuery)
	handler.BindLambda([wcoWeak, connectionCopy, requestedTemplateArgCopy, callbackCopy, bDoDefault](const FEchoRawQueryResult &rawQuery)
	{
		RESOLVE_WCO( wcoWeak, WorldContextObject, "AEcho3DService::RequestHolograms:[Lambda]" );
		FEchoHologramQueryResult holoQuery;
		holoQuery = ParseEchoHologramQuery(rawQuery);
		const UEchoHologramBaseTemplate *foundTemplate = nullptr;
		switch(requestedTemplateArgCopy.whichCase)
		{
			case EWhichRequestHologramsCase::UseKey:
				foundTemplate = GetTemplate(requestedTemplateArgCopy.viaKey);
				break;

			case EWhichRequestHologramsCase::UseInstance:
				foundTemplate = requestedTemplateArgCopy.viaInstance.Get();
				if (requestedTemplateArgCopy.viaInstance.IsStale())
				{
					UE_LOG(LogTemp, Error, TEXT("RequestHologramsTemplate: template instance was destroyed in the interim"));
				}
				break;

			case EWhichRequestHologramsCase::UseClass:
				//TODO: should these actually be strong references since they need to be resolved?
				//foundTemplate = InstantiateTemplateFromClass(requestedTemplateArgCopy.viaClass.Get());
				foundTemplate = ResolveTemplateFromClass(requestedTemplateArgCopy.viaClass.Get());

				//if (requestedTemplateArgCopy.viaClass.IsPending
				if (foundTemplate == nullptr)
				{
					UE_LOG(LogTemp, Error, TEXT("RequestHologramsTemplateClass: failed to resolve the template class into a template instance"));
				}
				break;

			default:
				UE_LOG(LogTemp, Error, TEXT("RequestHologramsHelper: unhandled case %d. Failing request"), requestedTemplateArgCopy.whichCase);
				return;
		}

		if (bDoDefault)
		{
			DefaultHologramHandler(WorldContextObject, connectionCopy, foundTemplate, holoQuery);
		}
		else if (callbackCopy.IsBound())
		{
			//callbackCopy.Execute(connectionCopy, blueprintKeyCopy, holoQuery);
			callbackCopy.Execute(connectionCopy, foundTemplate, holoQuery);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("RequestAllHolograms: unbound callback delegate!"));
		}
	});
	DoEchoRequest(url, handler);
}

//No more WCO functions
#undef CHECK_AND_CAPTURE_WCO
#undef RESOLVE_WCO


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Request type functions



namespace
{
	//TODO: add a "no-overhead" optimized version?
	/*struct QueryDebugInfo
	{
		FString query;
		uint64 queryNumber;
	};*/
	std::atomic<uint64> echo_queryCounter = 0;//std::atomic might be overkill, but if we ever thread stuff this will be important
	FEchoQueryDebugInfo CreateQueryDebugInfo(const FString &requestQuery)
	{
		uint64 myCounter = echo_queryCounter++;
		return FEchoQueryDebugInfo{requestQuery, myCounter};
	}

	//debug settings
	//const bool logAllQueryEvents = true;
	const bool logAllQueryEvents = false;

	bool shouldLogVerboseQueryEvent()
	{
		return AEcho3DService::GetVerboseMode() || logAllQueryEvents;
	}
};

//void AEcho3DService::DoEchoRequest(const FString &requestURL, const FEchoRequestHandler &callback)
FEchoRequestResultInfo AEcho3DService::DoEchoRequest(const FString &requestURL, const FEchoRequestHandler &callback)
{
	const FEchoQueryDebugInfo queryDebugState = CreateQueryDebugInfo(requestURL); //capture debug state
	const FEchoRequestResultInfo ReturnError = FEchoRequestResultInfo{(uint64)-1};
	if (!callback.IsBound())
	{
		UE_LOG(LogTemp, Warning, TEXT(ECHO_QUERY_LOG_PREFIX "Unbound callback provided to DoEchoRequest: %s"), queryDebugState.queryNumber, *queryDebugState.query);
		// *requestURL);
		return ReturnError;
	}
	
	// Create HTTP Request
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetVerb("GET");
	//this is super sus
	//HttpRequest->SetHeader("Content-Type", "application/json");
	HttpRequest->SetURL(requestURL);
	
	//FString urlCopy = requestURL;
	
	FEchoRequestHandler callbackCopy = callback;
	
	//queryDebugState.queryNumber is a replacement for appending debug_nonce which is no longer supported. This way is also cleaner, anyways.
	//TODO: maybe not have query number as debug but as something returned that can be cancelled/queried??
	if (bVerboseMode)
	{
		//THIS IS CONFUSING TO SHOW AS AN ERROR
		
		//PRIu64
		//static_assert( TEXT(ECHO_FORMAT_ARG_UINT64) == L"%llu", "WTF");
		//UE_LOG(LogTemp, Error, TEXT("[VERBOSEMODE} GET[" ECHO_FORMAT_ARG_UINT64 "]: %s"), queryDebugState.queryNumber, *requestURL);
		//const TCHAR* desu2 = TEXT("[VERBOSEMODE} GET[" ECHO_FORMAT_ARG_UINT64 "]: %s");
		//UE_LOG(LogTemp, Error, TEXT("[VERBOSEMODE} GET[" ECHO_FORMAT_ARG_UINT64 "]: %s"), queryDebugState.queryNumber, *queryDebugState.query);
		UE_LOG(LogTemp, Display, TEXT(ECHO_QUERY_LOG_PREFIX "GET: %s"), queryDebugState.queryNumber, *queryDebugState.query);
	}
	/*
	else
	{
		//UE_LOG(LogTemp, Log, TEXT("GET[" ECHO_FORMAT_ARG_UINT64 "]: %s"), queryDebugState.queryNumber,  *requestURL);
		//UE_LOG(LogTemp, Log, TEXT("GET[" ECHO_FORMAT_ARG_UINT64 "]: %s"), queryDebugState.queryNumber, *queryDebugState.query);
		UE_LOG(LogTemp, Log, TEXT(ECHO_QUERY_LOG_PREFIX "GET: %s"), queryDebugState.queryNumber, *queryDebugState.query);
	}*/
	
	/*if (logAllQueryEvents)
	{
		
	}
	*/

	//HttpRequest->OnProcessRequestComplete().BindLambda([callback, urlCopy, callbackCopy](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
	HttpRequest->OnProcessRequestComplete().BindLambda([callback, queryDebugState, callbackCopy](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
	{
		//if (logAllQueryEvents)
		if (shouldLogVerboseQueryEvent())
		{
			//UE_LOG(LogTemp, Log, TEXT("GOT[%llu]: %s"), queryDebugState.queryNumber, *queryDebugState.query);
			UE_LOG(LogTemp, Log, TEXT( ECHO_QUERY_LOG_PREFIX "GOT: %s"), queryDebugState.queryNumber, *queryDebugState.query);
		}
		//UE_LOG(LogTemp, Log, TEXT("GOTTEN: %s"), *urlCopy);
		//Note: get URL from requestPtr if desired, that way we never get the string from an old node
		//I might be over thinking this, but I'm worried about re-entrant behavior
		//TODO: just capture the fstring? meh this is still fine 
			
		//per IHttpRequest.h - bSuccess means " indicates whether or not the request was able to connect successfully"
		// ie, fails if we get a connection refused etc.
		// notably, 400/404 type errors are a "success" so we need to detect that
		int32 responseCode = -1;
		if (bSuccess)
		{
			if (Response != nullptr)
			{
				responseCode = Response->GetResponseCode();

				//TODO: should we accept any "OK" type status not just 200?
				bSuccess = bSuccess && (responseCode == EHttpResponseCodes::Ok); //ensure got a 200 message
			}
			else
			{
				//Paranoia, should probably never occur
				bSuccess = false;
				UE_LOG(LogTemp, Error, TEXT("QUERY[]: Null Response object!"));
			}
		}

		//TODO: should we capture this when making the call or just check the global setting when the query resturns?
		
			if (!bSuccess)
			{
				//TODO: add a quiet mode?
				//#define ECHO_INTERNAL_LOG_BASE_ERROR "Query failed! \n\tquery=%s\n\tresponseCode=%d"
				#define ECHO_INTERNAL_LOG_BASE_ERROR ECHO_QUERY_LOG_PREFIX "Query failed! \n\tQueryId: " ECHO_FORMAT_ARG_UINT64 "\n\tquery=%s\n\tresponseCode=%d"
				//FString desu = ECHO_INTERNAL_LOG_BASE_ERROR("TEST");
				//UE_LOG(LogTemp, Error, TEXT("TEST: %s"), *desu);
				if (logFailedQueries)
				{
					//UE_LOG(LogTemp, Error, TEXT("Query failed! responseCode=%d, query=%s, responseBody=\n\t%s\r\n\r\n"), responseCode, *urlCopy, *EchoHelperLib::StringFromRawContent(ECHO_GET_RESPONSE_BLOB_SAFE(Response)));
					FString respString = EchoHelperLib::StringFromRawContent(ECHO_GET_RESPONSE_BLOB_SAFE(Response));
					auto desu = (*respString);
					//UE_LOG(LogTemp, Error, TEXT("ADDR: 0x%x"), desu);
					//UE_LOG(LogTemp, Error, ECHO_INTERNAL_LOG_BASE_ERROR ("\n\tresponseBody=\n\t%s\r\n\r\n"), responseCode, *urlCopy, *respString);
					//FFS//UE_LOG(LogTemp, Error, TEXT("Query failed! \n\tquery=%s\n\tresponseCode=%d" "\n\tresponseBody=\n\t%s\r\n\r\n"), responseCode, *urlCopy, *respString);
					//UE_LOG(LogTemp, Error, TEXT("Query failed! \n\tquery=%s\n\tresponseCode=%d" "\n\tresponseBody=\n\t%s\r\n\r\n"), responseCode, *urlCopy, *respString);
					//UE_LOG(LogTemp, Error, TEXT(ECHO_INTERNAL_LOG_BASE_ERROR "\n\tresponseBody=\n\t%s\r\n\r\n"), *urlCopy, responseCode, *respString);
					//UE_LOG(LogTemp, Error, TEXT(ECHO_INTERNAL_LOG_BASE_ERROR "\n\tresponseBody=%s\r\n\r\n"), *urlCopy, responseCode, *respString);
					UE_LOG(LogTemp, Error, TEXT(ECHO_INTERNAL_LOG_BASE_ERROR "\n\tresponseBody=%s\r\n\r\n"), queryDebugState.queryNumber, queryDebugState.queryNumber, *queryDebugState.query, responseCode, *respString);
				}
				else
				{
					//UE_LOG(LogTemp, Error, TEXT("Query failed! responseCode=%d, query=%s"), responseCode, *urlCopy);
					//UE_LOG(LogTemp, Error, TEXT(ECHO_INTERNAL_LOG_BASE_ERROR (""), responseCode, *urlCopy);
					//UE_LOG(LogTemp, Error, ECHO_INTERNAL_LOG_BASE_ERROR (""), responseCode, *urlCopy);
					//UE_LOG(LogTemp, Error, TEXT("Query failed! \n\tquery=%s\n\tresponseCode=%d"), responseCode, *urlCopy);
					//UE_LOG(LogTemp, Error, TEXT(ECHO_INTERNAL_LOG_BASE_ERROR), *urlCopy, responseCode);
					UE_LOG(LogTemp, Error, TEXT(ECHO_INTERNAL_LOG_BASE_ERROR), queryDebugState.queryNumber, queryDebugState.queryNumber, *queryDebugState.query, responseCode);
					HINT_SHOWERRORS();
				}
				#undef ECHO_INTERNAL_LOG_BASE_ERROR
			}
			
		//TODO: maybe store more detailed info somewhere? but probably not?
		//TODO: use some kind of request nonce?/request id for cancelling or mapping responses to requests??
		if (callback.IsBound())
		{
			//callback.Execute( FEchoRawQueryResult(urlCopy, bSuccess, ECHO_GET_RESPONSE_BLOB_SAFE( Response ) ) );
			//callback.Execute( FEchoRawQueryResult(queryDebugState.query, bSuccess, ECHO_GET_RESPONSE_BLOB_SAFE( Response ) ) );
			callback.Execute( FEchoRawQueryResult(queryDebugState, bSuccess, ECHO_GET_RESPONSE_BLOB_SAFE( Response ) ) );
		}
		else
		{
			//error because we checked at the start
			//UE_LOG(LogTemp, Error, TEXT("Unbound callback found in lambda for DoEchoRequest: %s"), *urlCopy);
			UE_LOG(LogTemp, Error, TEXT("QUERY[" ECHO_FORMAT_ARG_UINT64 "]: Unbound callback found in lambda for DoEchoRequest: %s"), queryDebugState.queryNumber, *queryDebugState.query);
		}
	});

	// Handle actual request
	HttpRequest->ProcessRequest();

	return FEchoRequestResultInfo{queryDebugState.queryNumber};
}


void AEcho3DService::RequestAssetBP(const FEchoConnection &connection, const FEchoFile &storage, EEchoAssetType expectedType, bool bAllowCache, const FEchoMemoryAssetCallbackBP &callback)
{
	//TODO: is it sane to request an asset in order to cache it later? probably a bad idea?
	//TODO: handle assets in such a way that we multicast on ready or something not create a new request?

	const auto callbackInfo = EchoHelperLib::CheckDelegateStatusNoisy(callback);
	const bool bWasCallable = callbackInfo.bCallable;
	if (!bWasCallable)
	{
		UE_LOG(LogTemp, Warning, TEXT("RequestAssetBP: delegate not bound!"));
	}
	
	FEchoMemoryAssetCallback wrapper;
	FEchoMemoryAssetCallbackBP callbackCopy = callback;
	wrapper.BindLambda([callbackCopy, bWasCallable](const FEchoMemoryAsset &result)
	{
		if (!callbackCopy.IsBound())
		{
			if (bWasCallable)
			{
				//warn if was previously bound
				UE_LOG(LogTemp, Error, TEXT("RequestAssetBP wrapper unable to resolve callback!"));
			}
			return;
		}
		callbackCopy.Execute(result);
	});
	RequestAsset(connection, storage, expectedType, bAllowCache, wrapper);
}

//TODO: maybe nuke bAllowCache param or make this take an FEchoAssetRequest?
//TODO: add some kind of cache?	
//TODO: a storageId only version?
//TODO: refactor to FEchoAssetRequest ??
void AEcho3DService::RequestAsset(const FEchoConnection &connection, const FEchoFile &storage, EEchoAssetType expectedType, bool bAllowCache, const FEchoMemoryAssetCallback &callback)
{
	if (storage.storageId.IsEmpty())
	{
		//log is mostly so user can put a breakpoint here
		UE_LOG(LogTemp, Error, TEXT("WARNING: Requesting empty storage. this will fail."));
	}
	//lol add asset completion listeners?
	//FString url = EchoHelperLib::GenerateEchoStorageRequestURL(connection, storage.storageId);
	FString url = EchoHelperLib::GenerateEchoStorageRequestURL(connection, storage);
	EEchoAssetType expectedTypeCopy = expectedType;
	FEchoMemoryAssetCallback callbackCopy = callback;
	const bool bWasBound = callback.IsBound();
	FEchoFile storageCopy = storage;
	FEchoRequestHandler callbackAdaptor;

	if (!bWasBound)
	{
		UE_LOG(LogTemp, Warning, TEXT("RequestAsset: callback was not bound!"));
	}

	callbackAdaptor.BindLambda([expectedTypeCopy, callbackCopy, bWasBound, storageCopy](const FEchoRawQueryResult &rawQuery)
	{
		FEchoMemoryAsset asset;
		asset.fileInfo = storageCopy; 
		asset.bHaveContent = rawQuery.bSuccess && (rawQuery.contentBlob.Num() > 0);
		asset.blob = std::move(rawQuery.contentBlob); //is this actually sane?
		asset.assetType = expectedTypeCopy;
		asset.cachedAt = EchoHelperLib::GetCurrentTime_BP();//TODO: possibly from server response value?
		asset.assetDebugInfo = FEchoAssetDebugInfo{EEchoAssetSource::EchoAssetSource_FromRequest, rawQuery.debugInfo};
		//TODO: cache asset

		if (!asset.bHaveContent)
		{
			//something probably went wrong, log error message
			//UE_LOG(LogTemp, Error, TEXT(ECHO_QUERY_LOG_PREFIX "No content in storage request response: filename='%s', storageId='%s'"), rawQuery.debugInfo.queryNumber, *storageCopy.filename, *storageCopy.storageId);
			UE_LOG(LogTemp, Error, TEXT(ECHO_QUERY_LOG_PREFIX "No content in storage request response!" "\n\tRequestId: " ECHO_FORMAT_ARG_UINT64 "\n\tRequestURL: %s\n\tFilename='%s'\n\tStorageId='%s'"), rawQuery.debugInfo.queryNumber, rawQuery.debugInfo.queryNumber, *rawQuery.debugInfo.query, *storageCopy.filename, *storageCopy.storageId);
		}

		if (!callbackCopy.IsBound())
		{
			if (bWasBound)
			{
				UE_LOG(LogTemp, Log, TEXT("RequestAsset lambda: callback no longer bound!"));
			}
			return;
		}
		callbackCopy.Execute(asset);
	});
	const FEchoRequestResultInfo queryInfo = AEcho3DService::DoEchoRequest(url, callbackAdaptor);
	//if (logAllQueryEvents)
	if (shouldLogVerboseQueryEvent())
	{
		//UE_LOG(LogTemp, Error, TEXT(ECHO_QUERY_LOG_PREFIX "Requesting Storage: filename='%s', storageId='%s'"), queryInfo.queryNumber, *storageCopy.filename, *storageCopy.storageId);
		//UE_LOG(LogTemp, Log, TEXT(ECHO_QUERY_LOG_PREFIX "Requesting Storage: " "\n\tRequestId: " ECHO_FORMAT_ARG_UINT64 "\n\tRequestURL: %s\n\tFilename='%s'\n\tStorageId='%s'"), rawQuery.debugInfo.queryNumber, rawQuery.debugInfo.queryNumber, *rawQuery.debugInfo.query, *storageCopy.filename, *storageCopy.storageId);
		////
		///UE_LOG(LogTemp, Log, TEXT(ECHO_QUERY_LOG_PREFIX "Requesting Storage: " "\n\tRequestId: " ECHO_FORMAT_ARG_UINT64 "\n\tRequestURL: %s\n\tFilename='%s'\n\tStorageId='%s'"), queryInfo.queryNumber, queryInfo.queryNumber, *storageCopy.filename, *storageCopy.storageId);
		UE_LOG(LogTemp, Log, TEXT(ECHO_QUERY_LOG_PREFIX "Requested Storage: " "\n\tFilename='%s'\n\tStorageId='%s'"), queryInfo.queryNumber, *storageCopy.filename, *storageCopy.storageId);
	}
}

//Q: echo::later?
void AEcho3DService::RequestAssets(const FEchoConnection &connection, const FEchoAssetRequestArray &requests, const FEchoMemoryAssetArrayCallback &afterCallback)
{
	const bool bWasBound = afterCallback.IsBound();
	if (!bWasBound)
	{
		UE_LOG(LogTemp, Warning, TEXT("Passed unbound after func to RequestAssets"));
	}
	//TSharedRef<TArray<FEchoMemoryAsset>> resultsRef( TArray<FEchoMemoryAsset>() ); //how does this give syntax errors later on?
	TSharedRef<TArray<FEchoMemoryAsset>> resultsRef(new TArray<FEchoMemoryAsset>() );//TODO: ensure this is deleting pointer?

	if (requests.Num() < 1)
	{
		//TODO: some kind of .later helper
		//TODO: execute "After" instead of immediately for correctness
		if (afterCallback.IsBound())
		{
			afterCallback.Execute(resultsRef.Get());
		}
		return;
	}

	//sanity check for debugging
	{
		int numInvalid = 0;
		//for(const FEchoAssetRequest &request: requests)
		for(int32 i=0; i<requests.Num(); i++)
		{
			const FEchoAssetRequest &request = requests[i];
			if (!IsValidRequest(request))
			{
				UE_LOG(LogTemp, Error, TEXT("Invalid Storage Request!\n\tIndex=%d\n\tFilename: '%s'\n\tStorageId: '%s'\n\tAssetType: %i"), i, *request.fileInfo.filename, *request.fileInfo.storageId, request.assetType);
				//bAnyInvalidRequest = true;
				numInvalid++;
			}
		}
		//if (bAnyInvalidRequest)
		if (numInvalid>0)
		{
			UE_LOG(LogTemp, Error, TEXT("%d invalid storage request(s) were detected in RequestAssets!"), numInvalid);
		}
	}
	//Assert requests.Num() > 0
	FEchoAssetRequestArray requestsCopy = requests; //capture array
	FEchoMemoryAssetArrayCallback afterFuncCopy = afterCallback;
	FEchoConnection connectionCopy = connection;
	
	//TSharedRef<FEchoMemoryAssetCallback> visitorCallbackRef( new FEchoMemoryAssetCallback() );
	TSharedRef<FEchoMemoryAssetCallback> visitorCallbackRef = MakeShared<FEchoMemoryAssetCallback>();
	// new FEchoMemoryAssetCallback() );
	visitorCallbackRef.Get().BindLambda([resultsRef, visitorCallbackRef, afterFuncCopy, requestsCopy, connectionCopy, bWasBound](const FEchoMemoryAsset &asset)
	{
		TArray<FEchoMemoryAsset> &results = resultsRef.Get();
		results.Push(asset);
		int numAssetsRecieved = results.Num();

		if (shouldLogVerboseQueryEvent())
		{
			UE_LOG(LogTemp, Display, TEXT("GOT ASSET: Filename:'%s' - %d / %d -- Storage='%s'"), *asset.fileInfo.filename, numAssetsRecieved, requestsCopy.Num(), *asset.fileInfo.storageId);
		}
		if (numAssetsRecieved >= requestsCopy.Num())
		{
			//finished
			if (!afterFuncCopy.IsBound())
			{
				if (bWasBound)
				{
					UE_LOG(LogTemp, Error, TEXT("RequestAssets::[lambda]: afterCallback delegate became unbound!"));
				}
				return;
			}
			afterFuncCopy.Execute(results);
			return;
		}
		else
		{
			//request next asset
			{
				int numResults = results.Num();
				const FEchoFile &requestFile = requestsCopy[numResults].fileInfo;
				if (shouldLogVerboseQueryEvent())
				{
					//UE_LOG(LogTemp, Warning, TEXT("Requesting %d / %d: '%s' @ '%s'"), numResults+1, requestsCopy.Num(), *requestFile.filename, *requestFile.storageId);
					UE_LOG(LogTemp, Display, TEXT("Requesting %d / %d: '%s' @ '%s'"), numResults+1, requestsCopy.Num(), *requestFile.filename, *requestFile.storageId);
				}
				AEcho3DService::RequestAsset(connectionCopy, requestFile, requestsCopy[numResults].assetType, requestsCopy[numResults].bAllowCache, visitorCallbackRef.Get());
			}
		}
		
	});
	
	//TODO: maybe do this part with a MACRO?
	{
		TArray<FEchoMemoryAsset> &results = resultsRef.Get();
		int numResults = results.Num();
		const FEchoFile &requestFile = requestsCopy[numResults].fileInfo;
		if (shouldLogVerboseQueryEvent())
		{
			UE_LOG(LogTemp, Warning, TEXT("Requesting %d / %d: '%s' @ '%s'"), numResults+1, requestsCopy.Num(), *requestFile.filename, *requestFile.storageId);
		}
		AEcho3DService::RequestAsset(connectionCopy, requestFile, requestsCopy[numResults].assetType, requestsCopy[numResults].bAllowCache, visitorCallbackRef.Get());
	}
}

//////////////////////////////////////////////////////////////////////////////


void AEcho3DService::SetUseDevelopmentEndpoint(bool bSetUseDevEndpoint)
{
	//forward to internals
	EchoHelperLib::SetUseDevelopmentEndpoint(bSetUseDevEndpoint);
}

bool AEcho3DService::GetUseDevelopmentEndpoint()
{
	//forward to internals
	return EchoHelperLib::GetUseDevelopmentEndpoint();
}

void AEcho3DService::BeginInit(const UObject *WorldContextObject)
{
	CheckForNewRun(WorldContextObject);
}

void AEcho3DService::ResetState()//const UObject *WorldContextObject)
{
	//TODO: reset other subsystems?
	SetUseDevelopmentEndpoint(false);
	templates.Reset();
	defaultTemplate = nullptr;
	bVerboseMode = false;
	logFailedQueries = false;
	bDebugTemplateOperations = false;
	hideDefaultUnwantedWarnings = true;

	bInitCalled = false;
	//TODO: how to safely reset queued tasks???
	//TODO: going to just assume they're stale or reset when the world changed when queued
	//CheckForNewRun(WorldContextObject);
}

void AEcho3DService::FinishInit(const UObject *WorldContextObject)
{
	//TODO: finish other subsystems?
	CheckForNewRun(WorldContextObject);
	if (!AEcho3DService::bInitCalled)
	{
		AEcho3DService::bInitCalled = true;
		UE_LOG(LogTemp, Warning, TEXT("FinishInit called!"));
		for(const FEchoCallback &callback: queuedPostInitTasks)
		{
			//callback
			if (callback.IsBound())
			{
				callback.ExecuteIfBound();
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("FinishInit: callback no longer bound!"));//paranoia
			}
		}
		queuedPostInitTasks.Reset();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Already Called FinishInit this run!"));
	}
}

void AEcho3DService::QueueTaskAfterInit(const UObject *WorldContextObject, const FEchoCallback &afterInit)
{
	CheckForNewRun(WorldContextObject);
	if (!afterInit.IsBound())
	{
		UE_LOG(LogTemp, Error, TEXT("QueueTaskAfterInit: afterInit callback not bound!")); //warn during first call
		return;
	}
	
	if (AEcho3DService::bInitCalled)
	{
		if (afterInit.IsBound())
		{
			afterInit.ExecuteIfBound();
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("QueueTaskAFterInit: afterInit not bound!"));//paranoia
		}
	}
	else
	{
		queuedPostInitTasks.Add(afterInit);
	}
}

void AEcho3DService::CheckForNewRun(const UObject *WorldContextObject)
{
	if (WorldContextObject == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("CheckForNewRun: null WCO!"));
		return;
	}
	UWorld *newWorld = WorldContextObject->GetWorld();
	if (newWorld == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("CheckForNewRun: WCO's GetWorld was nullptr!"));
		return;
	}
	if ((!lastPlayWorld.IsValid())||(lastPlayWorld.Get()==nullptr)||(lastPlayWorld.Get() != newWorld))
	{
		lastPlayWorld = newWorld; //isekai
		OnStaticReset();
	}
	//nothing changed?
}

void AEcho3DService::OnStaticReset()
{
	UE_LOG(LogTemp, Display, TEXT("[========================================== OnStaticInit ==========================================]"));
	bInitCalled = false;
	queuedPostInitTasks.Reset();
	ResetState();
}


void AEcho3DService::SetHideDefaultUnwantedWarnings(bool setHideDefaultUnwantedWarnings)
{
	hideDefaultUnwantedWarnings = setHideDefaultUnwantedWarnings;
	UEchoMeshService::SetHideDefaultUnwantedWarnings(setHideDefaultUnwantedWarnings);
}

#pragma optimize( "", on )