// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EchoConnection.h"
#include "Materials/Material.h"

//#include "EchoImporter.h"

#include "EchoMeshService.generated.h"

//from echoimporter
typedef struct FFinalReturnData;
typedef struct FEchoImportMaterialData;

//const UEchoMaterialBaseTemplate *templateForMaterials

/**
 * The factory for meshes and other 3D content/assets.
 * This might eventually get split up and only retain the 3d mesh part
 */
UCLASS()
class ECHO3D_API UEchoMeshService : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * the current most recent mesh variant version.
	 * supports only hasVertexColors presently
	**/
	static const int32 MeshVariantFormatVersion_01 = 1; 

	/**
	 * Attaches a mesh later on (async) from the specified echo3d storage content.
	**/
	UFUNCTION(BlueprintCallable, Category = "EchoMesh")
	//static void AttachMeshFromStorage(AActor *actor, const FEchoConnection &connection, const FString &storageId, UClass *spawnClass, const FEchoImportConfig &config, const FEchoActorTemplateResolution &actorConfig);
	//TODO: accept an echoconnection instead of an import config??
	static void AttachMeshFromStorage(
		const FEchoImportConfig &importConfig, 
		const FEchoImportMeshConfig &meshConfig,
		const FString &storageId
	);

	/*
		//AActor *actor, const FEchoConnection &connection, const FString &storageId, UClass *spawnClass, 
		AActor *actor, const FString &storageId,
		UClass *spawnClass,  //Do we still want this?
		const FEchoImportConfig &importConfig
		//, const FEchoActorTemplateResolution &actorConfig
	);
	*/

	/**
	 * Attaches a mesh given an asset. 
	 * we might actually want to pass the asset list/map around too?
	 * This is generally expected to (but not required to) complete immediately unless some additional assets must be downloaded.
	**/
	//UFUNCTION(BlueprintCallable, Category = "EchoMesh", DisplayName="AttachMeshFromAsset")
	UFUNCTION(BlueprintCallable, Category = "EchoMesh")
	static void AttachMeshFromAsset(
		const FEchoImportConfig &importConfig, 
		const FEchoImportMeshConfig &meshConfig,
		//const FEchoImportMaterialConfig &matConfig,
		const FEchoMemoryAsset &asset
	);
		
		//const FEchoCustomMeshImportSettings &applyMeshImportSettings, 
		//AActor *actor, 
		//const FEchoConnection &connection, 
		//UClass *spawnClass
		//, const FEchoActorTemplateResolution &actorConfig
	//);

	//TODO: Should this even be public? its unclear from the name how its different from just AttachMeshFromAsset
	/**
	 * Assemble 3d content from already downloaded assets. 
	 * TODO: maybe per-mesh asset run attachmesh??
	**/
	//static void Assemble(AActor *actor, const FEchoConnection &connection, const FEchoMemoryAssetArray &assets, UClass *spawnClass, const FEchoImportConfig &config, const FEchoActorTemplateResolution &actorConfig);
	UFUNCTION(BlueprintCallable, Category = "EchoMesh")
	static void AttachMeshFromAssetArray(
		const FEchoImportConfig &importConfig, 
		const FEchoImportMeshConfig &meshConfig,
		//const TArray<const FEchoMemoryAsset> &assetList
		const TArray<FEchoMemoryAsset> &assetList
		/*
		const FEchoCustomMeshImportSettings *applyMeshImportSettings, 
		const FEchoImportConfig &config, 
		const TArray<const FEchoMemoryAsset> &assetList, 
		AActor *actor, 
		//const FEchoConnection &connection, 
		
		UClass *spawnClass
		*/
		//AActor *actor, const FEchoConnection &connection, const FEchoMemoryAssetArray &assets, UClass *spawnClass, const FEchoImportConfig &config, const FEchoActorTemplateResolution &actorConfig
	);

	/**
	 * if you reimplement a material handler or need to handle the material variants based on the mesh configuration, you should call this
	 * since the EchoMeshVariant struct can be changed in the future, user should call this with their supported version so things can be flagged in the future if the variant changes.
	**/
	UFUNCTION(BlueprintCallable, Category = "EchoMesh|TemplateHelpers")
	static bool CheckSupportedMeshVersion(const FString &callContext, const FEchoMeshVariants &forMeshVariant, int32 maxSupportedMeshVersion);

	/**
	 * default material function designed for M_EchoMaster
	 * returns a UMaterialInstanceDynamic since its easier to modify later on. 
	 * Feel free to call this and modify the results to suit your needs. 
	**/
	static UMaterialInstanceDynamic *ParseOneMaterialDefault(
		UObject *WorldContextObject,
		//const FEchoImportConfig &importConfig, 
		const FString &importTitle, 
		//const FEchoActorTemplateResolution &actorConfig,
		const FFinalReturnData & materialResults, 
		const FEchoImportMaterialData &forMaterial,
		const FEchoMeshVariants &forMeshVariant,
		UMaterial *overrideMaterialMaster = nullptr
	);

	/**
	 * Sets the fallback master material to something
	 * eventually this will probably want to be where you can throw an "error shader PRO" shader for easy recognition OR just a safe fallback
	**/
	UFUNCTION(BlueprintCallable, Category = "EchoMesh")
	static void BindDefaultMaterial(UMaterial *setDefaultMat);//hack

	//TODO: rename this to something like "Fallback texture"
	/**
	 * Temporary workaround for not yet supporting texture loading. Might add something like this later on for side channel injection of textures though.
	**/
	static void BindDebugTexture(const FName &texProperty, UTexture2D *setTexture);
	
	/**
	 * if true, will print verbose information when setting up a material for debugging purposes
	**/
	UFUNCTION(BlueprintCallable, Category = "EchoMesh|Debug")
	static void SetMeshImporterDebugPrintMaterialInfo(bool setDebugPrintMaterialInfo)
	{
		debugPrintMaterialInfo = setDebugPrintMaterialInfo;
	}

	/**
	 * sets fallback mesh import settings
	**/
	UFUNCTION(BlueprintCallable, Category = "EchoMesh")
	static void SetDefaultMeshImportSettings(const FEchoCustomMeshImportSettings &useSettings);
	
	/**
	 * DEBUG: loads a mesh from a file. this is mostly meant as a debugging tool
	**/
	UFUNCTION(BlueprintCallable, Category = "EchoMesh")
	//static bool DebugLoadMeshFromFile(AActor *actor, const FString &localFilename, UClass *spawnClass, const FEchoImportConfig &config, const FEchoActorTemplateResolution &actorConfig)
	static bool DebugLoadMeshFromFile(
		const FEchoImportConfig &importConfig, 
		const FEchoImportMeshConfig &meshConfig,
		const FString &localFilename
		//const FEchoCustomMeshImportSettings &applyMeshImportSettings, 
		//const UEchoMaterialTemplate *materialTemplate,
		//AActor *actor, 
		//UClass *spawnClass
		
		//const FEchoImportConfig &config, 
		//AActor *actor, const FString &localFilename, UClass *spawnClass
		//, const FEchoActorTemplateResolution &actorConfig
	)
	{
		FEchoMemoryAsset asset;
		asset.bHaveContent = false;
		asset.assetType = EEchoAssetType::EEchoAsset_Unknown;
		//bool ok = DebugLoadAssetFromFile(localFilename, EEchoAssetType::EEchoAsset_Mesh, asset, config);
		bool ok = DebugLoadAssetFromFile(localFilename, EEchoAssetType::EEchoAsset_Mesh, asset);
		if (!ok)
		{
			UE_LOG(LogTemp, Error, TEXT("EchoMeshService::DebugLoadMeshFromFile: unable to load asset from %s"), *localFilename);
			//TODO: should we be returning some kind of UMeshComponent?
			return false;
		}
		//AttachMeshFromAsset(actor, FEchoConnection(), asset, spawnClass, nullptr, config, actorConfig);
		//AttachMeshFromAsset(importConfig, asset, applyMeshSettings, materialTemplate, actor, spawnClass);
		AttachMeshFromAsset(importConfig, meshConfig, asset);//, applyMeshSettings, materialTemplate, actor, spawnClass);
		//actor, FEchoConnection(), asset, spawnClass, nullptr, config, actorConfig);
		return true;
	}

	/** 
	 * DEBUG: loads an asset from a local file. this is mostly meant as a debugging tool and provides a bridge between such debug assets and other functions that rely on "assets"
	**/
	UFUNCTION(BlueprintCallable, Category = "EchoMesh")
	static bool DebugLoadAssetFromFile(const FString &localFilename, EEchoAssetType assetType, FEchoMemoryAsset &result);

protected:
	/////various blueprint specific variations that are protected to "hide" them from C++ users since they're just wrappers for blueprint
	
	/**
	 * Attaches a mesh given an asset. 
	 * we might actually want to pass the asset list/map around too?
	 * This is generally expected to (but not required to) complete immediately unless some additional assets must be downloaded.
	 * 
	 * This is a blueprint wrapper for the C++ version which
	 * 
	 * @param connection: because might concievably need additional assets
	**/
	
	/*
	UFUNCTION(BlueprintCallable, Category = "EchoMesh", DisplayName="AttachMeshFromAsset")
	//static void AttachMeshFromAsset_BP(AActor *actor, const FEchoConnection &connection, const FEchoMemoryAsset &asset, UClass *spawnClass, const FEchoImportConfig &config, const FEchoActorTemplateResolution &actorConfig)
	static void AttachMeshFromAsset_BP(
		const FEchoImportConfig &config, 
		const FEchoMemoryAsset &asset, 
		const FEchoCustomMeshImportSettings &importSettings,
		AActor *actor, UClass *spawnClass
		//, const FEchoActorTemplateResolution &actorConfig
	)
	{
		//AttachMeshFromAsset(actor, connection, asset, spawnClass, nullptr, config, actorConfig);
		//AttachMeshFromAsset(config, asset, asset, spawnClass, nullptr, config, actorConfig);
		AttachMeshFromAsset(config, asset, asset, spawnClass, nullptr, config, actorConfig);
	}
	*/
	/*
	//does this variation need to exist? - no we can easily remake it later though!
	UFUNCTION(BlueprintCallable, Category = "EchoMesh")
	static void AttachMeshFromAsset_BPCustomSettings(AActor *actor, const FEchoConnection &connection, const FEchoMemoryAsset &asset, UClass *spawnClass, const FEchoCustomMeshImportSettings &applyMeshImportSettings, const FEchoImportConfig &config, const FEchoActorTemplateResolution &actorConfig)
	{
		AttachMeshFromAsset(actor, connection, asset, spawnClass, &applyMeshImportSettings, config, actorConfig);
	}
	*/

	/**
	 * Temporary workaround for not yet supporting texture loading. Might add something like this later on for side channel injection of textures though.
	**/
	UFUNCTION(BlueprintCallable, Category = "EchoMesh", DisplayName="BindDebugTexture")
	static void BindDebugTexture_BP(FName texName, UTexture2D *setTexture)
	{
		BindDebugTexture(texName, setTexture);
	}

private:
	//State
	static TWeakObjectPtr<UMaterial> defaultMaterial;
	static FEchoCustomMeshImportSettings defaultMeshSettings;
	static TMap<FName, TWeakObjectPtr<UTexture2D>> debugTextureBindings;
	static bool debugPrintMaterialInfo;
};
