// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

//Note: FEchoAssetRequestArray, FEchoMemoryAssetArray are only in EchoStructsFwd since they're typedefs

/**
 * This contains a bunch of basic "echo" structs and types used in various places.
 * its poorly named after the first such struct added.
**/
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

/**
 * time measurement opaque to blueprint but with a blueprintfunctionlibrary elsewhere to make sense of it
 * not particularly useful at the moment
**/
USTRUCT(BlueprintType)
struct ECHO3D_API FEchoTimeMeasurement
{
	GENERATED_BODY()

	//opaque value to blueprint since can't handle double (s)
	UPROPERTY()
	double raw;
};

/**
 * information about a "storage".
 * used both to make the storage request and as debugging info
**/
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

/**
 * contains information about a model determined from its model json.
 * not super useful anymore other than the model file since everything now uses GLBs
**/
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

/**
 * a wrapper for the additional meta data that can be configured in the echo website for each entry
**/
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

/**
 * information about a hologram
**/
USTRUCT(BlueprintType)
struct ECHO3D_API FEchoHologramInfo
{
	GENERATED_BODY()

	/**
	 * the echo "id" of the hologram
	**/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|EchoField")
	FString hologramId;

	/**
	 * our parsed modelInfo
	**/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|EchoField")
	FEchoModelInfo modelInfo;

	/**
	 * our meta data
	**/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|EchoField")
	FEchoAdditionalData additionalData;
};

//TODO: should this just be opaque?
/**
 * some debug information about a hologram
**/
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

	/**
	 * holder for debug info. the idea is to eventually hide this in production-y builds or not set it if certain debug flags are not set for performance reasons... maybe
	**/
	UPROPERTY(BlueprintReadOnly, Category = "Echo3D|EchoField")
	FEchoQueryDebugInfo debugInfo;

	/**
	 * if the query was a success. unlike for the http connection this is that we have non-zero data as well not just we made a tcp connection
	**/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|EchoField")
	bool bSuccess;

	/**
	 * a blob with our results.
	**/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|EchoField")
	TArray<uint8> contentBlob; //FEchoBlob

	/**
	 * helper to correctly convert to FString. potentially very expensive especially on typically non-string things like huge model files or textures
	**/
	FString GetContentAsString() const;
	
	FEchoRawQueryResult() = default;
	~FEchoRawQueryResult() = default;

	FEchoRawQueryResult(const FEchoQueryDebugInfo &setDebugInfo, bool setSuccess, const FEchoBlob &setContentBlob)
	 : debugInfo(setDebugInfo), bSuccess(setSuccess), contentBlob( setContentBlob )
	{
	}
};

/**
 * a bunch of parsed holograms from a hologram query and some error/warning data
**/
USTRUCT(BlueprintType)
struct ECHO3D_API FEchoHologramQueryResult
{
	GENERATED_BODY()
	
	/** was the input query valid - carried forward here **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|EchoField")
	bool isValid = false;

	/** if we had warnings processing the json **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|EchoField")
	bool hadWarnings = false;

	/** how many holograms we omitted from the results because we could not understand them **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|EchoField")
	int32 numMissing = 0;

	/** the array of holograms we parsed **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|EchoField")
	TArray<FEchoHologramInfo> holograms;

};

/**
 * helper enum to track the source of an asset. not really used atm. meant to help identify if an asset came from a request or from within another asset (like an embedded texture etc).
 * not very useful atm
**/
UENUM() 
enum class EEchoAssetSource: uint8
{
	EchoAssetSource_Unknown = 0,
	EchoAssetSource_FromRequest,
	EchoAssetSource_FromAsset,
};

/**
 * some debug info about an asset
**/
USTRUCT(BlueprintType)
struct ECHO3D_API FEchoAssetDebugInfo
{
	GENERATED_BODY()

	EEchoAssetSource fromSource;
	FEchoQueryDebugInfo sourceQuery;
};

/**
 * a list of asset types. not super useful atm
**/
UENUM()
enum class EEchoAssetType : uint8
{
	EEchoAsset_Unknown = 0,
	EEchoAsset_Mesh = 1,
	EEchoAsset_Material = 2,
	EEchoAsset_Texture = 3,
	EEchoAsset_Animation = 4,
};

/**
 * an asset as a blob in memory
**/
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

/**
 * an asset that we can request
**/
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

/**
 * which echo mesh variant of a material are we requesting?
 * used with EchoMaterialBaseTemplate's EvalMaterial
**/
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
//TODO: rename to hologramConfig or importHologramConfig or something like that
/**
 * all the general import information for a template in one argument
**/
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
	//UObject *WorldContextObject;
	TWeakObjectPtr<UObject> WorldContextObject;

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

	bool IsStale() const
	{
		return (WorldContextObject.IsStale());
	}
};

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
	 , WorldContextObject( fromConfig.WorldContextObject.Get() )
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
/**
 * some specific mesh import settings
**/
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

/**
 * mesh/model import arguments in one place
**/
USTRUCT(BlueprintType)
struct ECHO3D_API FEchoImportMeshConfig
{
	GENERATED_BODY()

	/**
	 * our material template
	**/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EchoMaterials")
	const UEchoMaterialBaseTemplate *materialTemplate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EchoMesh")
	FEchoCustomMeshImportSettings meshImportSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EchoMesh")
	TWeakObjectPtr<AActor> actor;

	FEchoImportMeshConfigPin Pin() const;
};

//TODO: possibly store these in our engine subsystem directly when requested??
/**
 * helper meant ot pin a meshconfig struct's contents across a delayed lambda
**/
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
	 //, spawnClass( fromConfig.spawnClass )
	 , nullMask( 0 )
	{
		nullMask = GetNullMask(pinnedConfig);
	}

	bool Get(FEchoImportMeshConfig &ret) const
	{
		ret = pinnedConfig; //copy non-objects

		//special handling for uobjects
		ret.materialTemplate = this->materialTemplate.Get();
		//ret.spawnClass = this->spawnClass.Get();
		bool match = (nullMask == GetNullMask(ret));
		return match;
	}
	
	int GetNullMask(const FEchoImportMeshConfig &config) const
	{
		return 0
			| (((config.materialTemplate != nullptr ) ? 1 : 0) << 0)
			//| (((config.spawnClass != nullptr ) ? 1 : 0) << 1)
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

/**
 * the result of an actor template
**/
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

DECLARE_DYNAMIC_DELEGATE_ThreeParams(FEchoHologramQueryHandler, const FEchoConnection&, connection, const UEchoHologramBaseTemplate *, hologramTemplate, const FEchoHologramQueryResult&, holoQuery);
	
DECLARE_DYNAMIC_DELEGATE_OneParam(FEchoMemoryAssetCallbackBP, const FEchoMemoryAsset &, asset);
DECLARE_DELEGATE_OneParam(FEchoMemoryAssetCallback, const FEchoMemoryAsset & /*, asset*/);

DECLARE_DELEGATE_OneParam(FEchoMemoryAssetArrayCallback, const FEchoMemoryAssetArray &);

DECLARE_DYNAMIC_DELEGATE(FEchoCallback);
