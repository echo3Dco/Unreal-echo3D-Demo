// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

//Note: FEchoAssetRequestArray, FEchoMemoryAssetArray are only in EchoStructsFwd since they're typedefs

//This contains a bunch of basic "echo" types used in various places.
//TODO: rename to something like EchoTypes.h or something?

#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"

//TODO: rename to EchoTypesFwd.h or something?
#include "EchoStructsFwd.h"

#include "EchoStringConstants.h"
#include "UObject/StrongObjectPtr.h"
//for uint64 format specifier macro PRIu64
//#include <stdint.h>
#include <inttypes.h>

#include "EchoConnection.generated.h"


#ifndef PRIu64
	#define ECHO_FORMAT_ARG_UINT64 "%" "llu"
	#define ECHO_FORMAT_ARG_HEX64_WITH0x "%#" PRIx64
#else
	#define ECHO_FORMAT_ARG_UINT64 "%" PRIu64
	#define ECHO_FORMAT_ARG_HEX64_WITH0x "%#" PRIx64
#endif

//some forward declarations
class UEchoHologramBaseTemplate;
class UEchoMaterialBaseTemplate;

/**
 * the information to connect to your echo project. 
 */
USTRUCT(BlueprintType)
struct ECHO3D_API FEchoConnection
{
	GENERATED_BODY()

public:
	FEchoConnection() = default;

	FEchoConnection(const FString &setApiKey, const FString &setSecurityKey = EchoStringConstants::EmptyString)
	 : apiKey(setApiKey)
	 , securityKey(setSecurityKey)
	{
	}

	/**
	 * API key. this grab from your echo project
	**/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EchoConnection")
	FString apiKey;

	/**
	 * Security key. often referred to as "secKey" elsewhere - grab from your echo web project settings
	 * this is needed if you have the setting to require it in your echo web project settings, but is probably a good idea
	**/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EchoConnection")
	FString securityKey;
};

/**
 * holds information about the results of an echo request. mostly for debugging information
**/
USTRUCT(BlueprintType)
struct ECHO3D_API FEchoRequestResultInfo
{
	GENERATED_BODY()

	/**
	 * a sequential number used to keep track (in print statements) of which output is coming from which query (which can otherwise be super confusing if your project imports a bunch of holograms at once
	 * 
	 * mostly useful for debugging. no guarantees on how it will be generated in the future
	**/
	uint64 queryNumber;
};


USTRUCT(BlueprintType)
struct ECHO3D_API FEchoTimeMeasurement
{
	GENERATED_BODY()

	//opaque value to blueprint since can't handle double (s)
	UPROPERTY()
	double raw;
};


USTRUCT(BlueprintType)
struct ECHO3D_API FEchoFile
{
	GENERATED_BODY()

	//actual name
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|EchoField")
	FString storageId;

	//"friendly" name
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|EchoField")
	FString filename;

	FEchoFile() = default;
	~FEchoFile() = default;
	FEchoFile(const FString &setStorageId, const FString &setFilename)
	 : storageId( setStorageId ), filename( setFilename )
	{
	}

	bool IsEmpty() const
	{
		return storageId.IsEmpty();
	}
};


USTRUCT(BlueprintType)
struct ECHO3D_API FEchoModelInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|EchoField")
	FEchoFile model;

	//TODO: do i need to de-allocate these?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|EchoField")
	TArray<FEchoFile> textures;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|EchoField")
	FEchoFile material;
};


USTRUCT(BlueprintType)
struct ECHO3D_API FEchoAdditionalData
{
	GENERATED_BODY()

public: //for clarity, structs public by default
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|EchoField")
	TMap<FString, FString> additionalData;

	bool HasField(const FString &key) const
	{
		return additionalData.Contains(key);
	}

	//TODO: should this return by value?
	const FString &ReadStringRef(const FString &key, const FString &defaultValue = EchoStringConstants::EmptyString) const
	{
		if (!HasField(key))
		{
			//missCount++;
			return defaultValue;
		}
		const FString *found = additionalData.Find(key);
		if (found != nullptr)
		{
			return *found;
		}
		return defaultValue;
	}

	void WriteStringRaw(const FString &key, const FString &value)
	{
		if (!HasField(key))
		{
			additionalData.Add(key, value);
		}
		else
		{
			additionalData[key] = value;
		}
	}

	FString ReadString(const FString &key, const FString &defaultValue =  EchoStringConstants::EmptyString) const
	{
		return ReadStringRef(key, defaultValue);
	}

	bool ReadBool(const FString &key, bool defaultValue = false) const;
	int32 ReadInt(const FString &key, int32 defaultValue = 0) const;
	float ReadFloat(const FString &key, float defaultValue = 0.0f) const;

	FVector ReadVec3(const FString &xKey, const FString &yKey, const FString &zKey, const FVector &defaultValue = FVector::ZeroVector) const
	{
		//TODO: make a tryread with better failure detection?
		FVector ret;
		ret.X = ReadFloat(xKey, defaultValue.X);
		ret.Y = ReadFloat(xKey, defaultValue.Y);
		ret.Z = ReadFloat(xKey, defaultValue.Z);
		return ret;
	}

	void WriteString(const FString &key, const FString &value)
	{
		WriteStringRaw(key, value);
	}

	void WriteBool(const FString &key, bool value);
	void WriteFloat(const FString &key, float value);
	void WriteInt(const FString &key, int32 value);
	

	void WriteVec3(const FString &xKey, const FString &yKey, const FString &zKey, const FVector &value)
	{
		//TODO: subkey with name.xyz??
		//TODO: make a tryread with better failure detection?
		//FVector ret;
		WriteFloat(xKey, value.X);
		WriteFloat(xKey, value.Y);
		WriteFloat(xKey, value.Z);
		//return ret;
	}
};

USTRUCT(BlueprintType)
struct ECHO3D_API FEchoHologramInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|EchoField")
	FString hologramId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|EchoField")
	FEchoModelInfo modelInfo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|EchoField")
	FEchoAdditionalData additionalData;
};

//TODO: should this just be opaque?
USTRUCT(BlueprintType)
struct ECHO3D_API FEchoQueryDebugInfo
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Echo3D|EchoField")
	FString query;

	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Echo3D|EchoField")
	uint64 queryNumber;
};

USTRUCT(BlueprintType)
struct ECHO3D_API FEchoRawQueryResult
{
	GENERATED_BODY()

	//UPROPERTY(BlueprintReadOnly, Category = "Echo3D|EchoField")
	//FString sourceQuery;

	UPROPERTY(BlueprintReadOnly, Category = "Echo3D|EchoField")
	FEchoQueryDebugInfo debugInfo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|EchoField")
	bool bSuccess;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|EchoField")
	TArray<uint8> contentBlob; //FEchoBlob

	FString GetContentAsString() const;
	/*
	{
		return EchoHelperLib::StringFromRawContent(contentBlob);
	}
	*/

	FEchoRawQueryResult() = default;
	~FEchoRawQueryResult() = default;

	//FEchoRawQueryResult(const FString &setSource, bool setSuccess, const FEchoBlob &setContentBlob)
	FEchoRawQueryResult(const FEchoQueryDebugInfo &setDebugInfo, bool setSuccess, const FEchoBlob &setContentBlob)
	 : debugInfo(setDebugInfo), bSuccess(setSuccess), contentBlob( setContentBlob )
	{
	}
};

USTRUCT(BlueprintType)
struct ECHO3D_API FEchoHologramQueryResult
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|EchoField")
	bool isValid = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|EchoField")
	bool hadWarnings = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|EchoField")
	int32 numMissing = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|EchoField")
	TArray<FEchoHologramInfo> holograms;

};


UENUM() 
enum class EEchoAssetSource: uint8
{
	EchoAssetSource_Unknown = 0,
	EchoAssetSource_FromRequest,
	EchoAssetSource_FromAsset,
};

USTRUCT(BlueprintType)
struct ECHO3D_API FEchoAssetDebugInfo
{
	GENERATED_BODY()

	EEchoAssetSource fromSource;
	FEchoQueryDebugInfo sourceQuery;
};


UENUM()
enum class EEchoAssetType : uint8
{
	EEchoAsset_Unknown = 0,
	EEchoAsset_Mesh = 1,
	EEchoAsset_Material = 2,
	EEchoAsset_Texture = 3,
	EEchoAsset_Animation = 4,
};

//TODO: asset query counter?

USTRUCT(BlueprintType)
struct ECHO3D_API FEchoMemoryAsset
{
	GENERATED_BODY()

	//TODO: initialize to some kind of magic synthetic source?
	FEchoAssetDebugInfo assetDebugInfo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|EchoField")
	FEchoFile fileInfo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|EchoField")
	TArray<uint8> blob; //FEchoBlob
	
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|EchoField")
	EEchoAssetType assetType;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|EchoField")
	bool bHaveContent;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|EchoField")
	//float cachedAt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|EchoField")
	FEchoTimeMeasurement cachedAt;

	//double cachedAt;
};

USTRUCT(BlueprintType)
struct ECHO3D_API FEchoAssetRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|EchoField")
	FEchoFile fileInfo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|EchoField")
	EEchoAssetType assetType = EEchoAssetType::EEchoAsset_Unknown;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|EchoField")
	bool bAllowCache = true;
};

USTRUCT(BlueprintType)
struct ECHO3D_API FEchoMeshVariants
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo|MeshMaterial")
	bool needVertexColors;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo|MeshMaterial")
	int32 meshFormatVersion; //provided to easily flag things that need to be updated when the version changes

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo|MeshMaterial")
	//bool needSkeleton;
};

typedef struct FEchoImportConfigPin;

USTRUCT(BlueprintType)
struct ECHO3D_API FEchoImportConfig
{
	GENERATED_BODY()

	FEchoImportConfig()
	 : WorldContextObject( nullptr )
	 , importTitle( EchoStringConstants::BadStringValue )
	 //, hologram
	 , hologramTemplate( nullptr )
	 , instanceState( nullptr )
	 //, connection
	{
	}

	/**
	 * our world context object
	**/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EchoImportConfig")
	UObject *WorldContextObject;

	/**
	 * a string to help identify the import in question easily. mostly useful for debugging
	**/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EchoImportConfig")
	FString importTitle;
	
	/**
	 * the hologram this is being applied to
	**/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EchoImportConfig")
	FEchoHologramInfo hologram;

	//TODO: does this need to be a weak reference?
	/**
	 * the template we resolved to
	**/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EchoImportConfig")
	const UEchoHologramBaseTemplate *hologramTemplate;

	//TODO: who owns this state?
	/**
	 * mutable state possibly used by the template. retrieved from the template
	**/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EchoImportConfig")
	UEchoTemplateState *instanceState;

	/**
	 * the connection info to use. mostly here to keep function signatures shorter, but I might break it out again in the future since its technically its own thing
	**/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EchoImportConfig")
	FEchoConnection connection;

	FEchoImportConfigPin Pin() const;
};




//TODO: possibly deprecate this? or have some kind of UObject config created by the template and passed back to it?
//TODO: does reachability work for ustructs???
/**
 * used to pin the an import config from garbage collection
 * TODO: figure out how to addrefernce or something instead of a bunch of heavy tstrongobjectptrs?
 * TODO: alternatively use weak pointers and recover later?
**/
struct FEchoImportConfigPin
{
	FEchoImportConfig pinnedConfig;
	TStrongObjectPtr<UObject> WorldContextObject;
	TStrongObjectPtr<const UEchoHologramBaseTemplate> hologramTemplate;
	TStrongObjectPtr<UEchoTemplateState> instanceState;
	int nullMask;

	FEchoImportConfigPin(const FEchoImportConfig &fromConfig)
	 : pinnedConfig( fromConfig )
	 , WorldContextObject( fromConfig.WorldContextObject )
	 , hologramTemplate( fromConfig.hologramTemplate )
	 , instanceState( fromConfig.instanceState )
	 , nullMask( 0 )
	{
		nullMask = GetNullMask(pinnedConfig);
		/*
		pinnedConfig = fromConfig;
		WorldContextObject = TStrongObjectPtr<UObject>( fromConfig.WorldContextObject );
		hologramTemplate = fromConfig.hologramTemplate;
		instanceState = fromConfig.instanceState;
		*/
	}

	bool Get(FEchoImportConfig &ret) const
	{
		ret = pinnedConfig; //copy non-objects

		//special handling for uobjects
		ret.WorldContextObject = this->WorldContextObject.Get();
		ret.hologramTemplate = this->hologramTemplate.Get();
		ret.instanceState = this->instanceState.Get();
		bool match = (nullMask == GetNullMask(ret));
		return match;
		//return true; //HACK TODO: check weak pointers???
	}
	
	int GetNullMask(const FEchoImportConfig &config) const
	{
		return 0
			| (((config.WorldContextObject != nullptr ) ? 1 : 0) << 0)
			| (((config.hologramTemplate != nullptr ) ? 1 : 0) << 1)
			| (((config.instanceState != nullptr ) ? 1 : 0) << 2)
		;
	}

	~FEchoImportConfigPin()
	{
		//probably not needed but paranoia
		WorldContextObject = nullptr;
		hologramTemplate= nullptr;
		instanceState = nullptr;
		nullMask = 0;
	}
};


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Mesh Stuff

//should this move to what should be called echotypes.h?(echoconnection.h)
//struct CustomMeshConfig
USTRUCT(BlueprintType)
struct ECHO3D_API FEchoCustomMeshImportSettings
{
	GENERATED_BODY();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EchoMesh|EchoField")
	float uniformScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EchoMesh|EchoField")
	bool bInvertWinding;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EchoMesh|EchoField")
	bool bFixAxes;

	//TODO: maybe use 100x?
	FEchoCustomMeshImportSettings()
	 : FEchoCustomMeshImportSettings(1.0f)
	{
	}

	FEchoCustomMeshImportSettings(float setUniformScale)
	 : FEchoCustomMeshImportSettings(setUniformScale, false, false)
	{
	}

	FEchoCustomMeshImportSettings(float setUniformScale, bool bSetInvertWinding, bool bSetFixAxes)
	 : uniformScale( setUniformScale ), bInvertWinding( bSetInvertWinding ), bFixAxes( bSetFixAxes )
	{
	}
	
	void Reset()
	{
		uniformScale = 1;
		bInvertWinding = false;
		bFixAxes = true;
	}
};

typedef struct FEchoImportMeshConfigPin;
//TODO: split this up, probably into FEchoImportMaterialConfig and 
//TODO: how to ensure we've kept this struct and its contents from being garbage collected????
USTRUCT(BlueprintType)
//struct ECHO3D_API FEchoImportMaterialConfig
struct ECHO3D_API FEchoImportMeshConfig
{
	GENERATED_BODY()

	/**
	 * our world context object
	**/
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EchoImportConfig")
	//UObject *WorldContextObject;

	/**
	 * our material template
	**/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EchoMaterials")
	//TWeakObjectPtr<const UEchoMaterialBaseTemplate> *materialTemplate;
	//TWeakObjectPtr<const UEchoImportMaterialTemplate> materialTemplate;
	//TWeakObjectPtr<const UEchoMaterialBaseTemplate> materialTemplate;
	const UEchoMaterialBaseTemplate *materialTemplate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EchoMesh")
	FEchoCustomMeshImportSettings meshImportSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EchoMesh")
	TWeakObjectPtr<AActor> actor;
	 
	//This should probably at best be a tsubclass of something. it probalby wants to be deprecated/removed
	//is this actually sane?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EchoMesh")
	UClass *spawnClass;
	//TStrongObjectPtr<UClass> spawnClass; 
	//TWeakClassPtr<UClass> spawnClass; 
	//TStrongObjectPtr<UClass> 
	//TWeakClassPtr<UClass> spawnClass; 
	//TWeakObjectPtr<UClass> spawnClass; 

	//TWeakObjectPtr<UClass> spawnClass;
	//TStrongObjectPtr<UClass> spawnClass;
	//TWeakObjectPtr<UClass> spawnClass;
	//TWeakClassPtr<UClass> spawnClass;
	//TODO: custom scene componenet classes?
	
	/*
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EchoMesh")
	AActor *actor;
	 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EchoMesh")
	UClass *spawnClass;
	*/
	FEchoImportMeshConfigPin Pin() const;
};

//TODO: possibly store these in our engine subsystem directly when requested??
struct FEchoImportMeshConfigPin
{
	//TODO: do some of these just want to be weak and we can store if they were stale?
	FEchoImportMeshConfig pinnedConfig;
	//TStrongObjectPtr<UObject> WorldContextObject;
	//TStrongObjectPtr<const UEchoHologramBaseTemplate> hologramTemplate;
	//TStrongObjectPtr<UEchoTemplateState> instanceState;
	TStrongObjectPtr<const UEchoMaterialBaseTemplate> materialTemplate;
	TStrongObjectPtr<UClass> spawnClass;
	int nullMask;

	FEchoImportMeshConfigPin(const FEchoImportMeshConfig &fromConfig)
	 : pinnedConfig( fromConfig )
	 , materialTemplate( fromConfig.materialTemplate )
	 , spawnClass( fromConfig.spawnClass )
	 , nullMask( 0 )
	{
		nullMask = GetNullMask(pinnedConfig);
	}

	bool Get(FEchoImportMeshConfig &ret) const
	{
		ret = pinnedConfig; //copy non-objects

		//special handling for uobjects
		ret.materialTemplate = this->materialTemplate.Get();
		ret.spawnClass = this->spawnClass.Get();
		bool match = (nullMask == GetNullMask(ret));
		return match;
	}
	
	int GetNullMask(const FEchoImportMeshConfig &config) const
	{
		return 0
			| (((config.materialTemplate != nullptr ) ? 1 : 0) << 0)
			| (((config.spawnClass != nullptr ) ? 1 : 0) << 1)
		;
	}

	~FEchoImportMeshConfigPin()
	{
		//probably not needed but paranoia
		materialTemplate = nullptr;
		spawnClass = nullptr;
		nullMask = 0;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

USTRUCT(BlueprintType)
struct ECHO3D_API FEchoConstructActorResult
{
	GENERATED_BODY();
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EchoConstructActor")
	AActor *actor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EchoConstructActor")
	UActorComponent *userComponent;

	FEchoConstructActorResult()
	: actor( nullptr ) 
	, userComponent( nullptr )
	{
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Delegate definitions

DECLARE_DELEGATE_OneParam(FEchoRequestHandler, const FEchoRawQueryResult&);
//DECLARE_DELEGATE_OneParam(FEchoBinaryRequestHandler, const FEchoRawBinaryQueryResult&);
	
//original
//DECLARE_DYNAMIC_DELEGATE_ThreeParams(FEchoHologramQueryHandler, const FEchoConnection&, connection, const FString&, blueprintKey, const FEchoHologramQueryResult&, holoQuery);
DECLARE_DYNAMIC_DELEGATE_ThreeParams(FEchoHologramQueryHandler, const FEchoConnection&, connection, const UEchoHologramBaseTemplate *, hologramTemplate, const FEchoHologramQueryResult&, holoQuery);
	
DECLARE_DYNAMIC_DELEGATE_OneParam(FEchoMemoryAssetCallbackBP, const FEchoMemoryAsset &, asset);
DECLARE_DELEGATE_OneParam(FEchoMemoryAssetCallback, const FEchoMemoryAsset & /*, asset*/);

//DECLARE_DELEGATE_OneParam(FEchoMemoryAssetArrayCallback, const FEchoMemoryAssetArray &);
DECLARE_DELEGATE_OneParam(FEchoMemoryAssetArrayCallback, const FEchoMemoryAssetArray &);

DECLARE_DYNAMIC_DELEGATE(FEchoCallback);