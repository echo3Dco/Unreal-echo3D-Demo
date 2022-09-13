#pragma once

//is this include actually sane?
#include "Containers/ContainersFwd.h"
//feels like overkill?:
//#include "CoreMinimal.h"

typedef struct FEchoConnection;

typedef struct FEchoTimeMeasurement;

typedef struct FEchoFile;
typedef struct FEchoModelInfo;

//stringutil "struct" omitted??
typedef struct StringUtil;

typedef struct FEchoAdditionalData;
typedef struct FEchoHologramInfo;

typedef struct FEchoQueryDebugInfo;
typedef struct FEchoRawQueryResult;
typedef struct FEchoHologramQueryResult;


typedef enum class EEchoAssetType : uint8;

typedef struct FEchoMemoryAsset;
typedef struct FEchoAssetRequest;

//templates stuff
typedef struct FEchoImportConfig;
typedef struct FEchoImportMeshConfig;

//not technically a struct but needed
typedef class UEchoTemplateState;
//typedef class UEchoHologramBaseTemplateTemplate;
//typedef class UEchoMaterialBaseTemplate;


//Note: need to use TArray version for UHT stuff in blueprint callable header declarations
using FEchoAssetRequestArray = TArray<FEchoAssetRequest>;
using FEchoMemoryAssetArray = TArray<FEchoMemoryAsset>;
//using FEchoMemoryAssetArray = TArray<const FEchoMemoryAsset>;
using FEchoBlob = TArray<uint8>;

using FMyBlobNickname = TArray<uint8>;



//NOTE: apparently can't put the delegates here or UHT can't find them =(

//TODO: is there a way to static assert the type is sane here?
#define ECHO_GET_RESPONSE_BLOB_SAFE(Response) \
	(const FEchoBlob &)((Response != nullptr) ? Response->GetContent() : FEchoBlob())

/*
//based on #include "Misc/GeneratedTypeName.h"
#if defined(_MSC_VER) && !defined(__clang__)
	//VS
	//#define _ECHO_PRETTY_FUNCTION_ __FUNCSIG__
	#define _ECHO_PRETTY_FUNCTION_ __FUNCSIG__
#else
	#ifdef __PRETTY_FUNCTION__
		//GCC
		#define _ECHO_PRETTY_FUNCTION_ __PRETTY_FUNCTION__
	#else
		//should compile everywhere, but might be ugly and implementation dependent
		#define _ECHO_PRETTY_FUNCTION_ __func__
	#endif
#endif
*/
#define _ECHO_PRETTY_FUNCTION_ __func__
//#define _ECHO_PRETTY_FUNCTION_FSTRING_ *FString(_ECHO_PRETTY_FUNCTION_)
#define _ECHO_PRETTY_FUNCTION_FSTRING_ FString(_ECHO_PRETTY_FUNCTION_)
