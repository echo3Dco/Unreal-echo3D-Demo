// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EchoConnection.h"
#include "Materials/Material.h"

#include "EchoMeshService.generated.h"

//from echoimporter
typedef struct FFinalReturnData;
typedef struct FEchoImportMaterialData;

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

	//TODO: accept an echoconnection instead of an import config??
	/**
	 * Attaches a mesh later on (async) from the specified echo3d storage content.
	**/
	UFUNCTION(BlueprintCallable, Category = "EchoMesh")
	static void AttachMeshFromStorage(
		const FEchoImportConfig &importConfig, 
		const FEchoImportMeshConfig &meshConfig,
		const FString &storageId
	);

	/**
	 * Attaches a mesh given an asset. 
	 * we might actually want to pass the asset list/map around too?
	 * This is generally expected to (but not required to) complete immediately unless some additional assets must be downloaded.
	**/
	UFUNCTION(BlueprintCallable, Category = "EchoMesh")
	static void AttachMeshFromAsset(
		const FEchoImportConfig &importConfig, 
		const FEchoImportMeshConfig &meshConfig,
		const FEchoMemoryAsset &asset
	);

	/**
	 * Assemble 3d content from already downloaded assets. 
	 * TODO: maybe per-mesh asset run attachmesh??
	**/
	UFUNCTION(BlueprintCallable, Category = "EchoMesh")
	static void AttachMeshFromAssetArray(
		const FEchoImportConfig &importConfig, 
		const FEchoImportMeshConfig &meshConfig,
		const TArray<FEchoMemoryAsset> &assetList
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
		const FString &importTitle, 
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
	static void SetMeshServiceDebugPrintMaterialInfo(bool setDebugPrintMaterialInfo)
	{
		debugPrintMaterialInfo = setDebugPrintMaterialInfo;
	}

	static bool GetMeshServiceDebugPrintMaterialInfo()
	{
		return debugPrintMaterialInfo;
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
	static bool DebugLoadMeshFromFile(
		const FEchoImportConfig &importConfig, 
		const FEchoImportMeshConfig &meshConfig,
		const FString &localFilename
	)
	{
		FEchoMemoryAsset asset;
		asset.bHaveContent = false;
		asset.assetType = EEchoAssetType::EEchoAsset_Unknown;
		bool ok = DebugLoadAssetFromFile(localFilename, EEchoAssetType::EEchoAsset_Mesh, asset);
		if (!ok)
		{
			UE_LOG(LogTemp, Error, TEXT("EchoMeshService::DebugLoadMeshFromFile: unable to load asset from %s"), *localFilename);
			//TODO: should we be returning some kind of UMeshComponent?
			return false;
		}
		AttachMeshFromAsset(importConfig, meshConfig, asset);
		return true;
	}

	/** 
	 * DEBUG: loads an asset from a local file. this is mostly meant as a debugging tool and provides a bridge between such debug assets and other functions that rely on "assets"
	**/
	UFUNCTION(BlueprintCallable, Category = "EchoMesh")
	static bool DebugLoadAssetFromFile(const FString &localFilename, EEchoAssetType assetType, FEchoMemoryAsset &result);

	/**
	 * turns on/off hiding for a bunch of warnings that we probably dont care about like unhandled duplicate properties etc.
	 * default is on.
	 * 
	 * This is mostly to keep things intelligible by default and make actual errors more obvious. call with true if your not seeing an import property and want to see if errors about it are being hidden etc
	 * meant to be invoked via Echo3DService's same-name method
	**/
	UFUNCTION(BlueprintCallable, Category = "EchoMesh")
	static void SetHideDefaultUnwantedWarnings(bool setHideDefaultUnwantedWarnings);
	
	/** gets the current state of hiding unwanted warnings. **/
	UFUNCTION(BlueprintCallable, Category = "EchoMesh")
	static bool GetHideDefaultUnwantedWarnings()
	{
		return hideDefaultUnwantedWarnings;
	}

	//deprecated
	/**
	 * gets the default material master. deprecated since we want to move towards templates and it was not really that easy to resuse material masters without some custom glue code to configure them
	**/
	static UMaterial *GetDefaultMaterial()
	{
		return defaultMaterial.Get();
	}

	/**
	 * meant to be called by Echo3DService to chain resetting in init sequence
	 * NOT exposed to blueprint presently - this should be being called internally for now
	**/
	static void ResetState();
		
//END PUBLIC
protected:
	
	/**
	 * Temporary workaround for not yet supporting texture loading. Might add something like this later on for side channel injection of textures though.
	**/
	UFUNCTION(BlueprintCallable, Category = "EchoMesh", DisplayName="BindDebugTexture")
	static void BindDebugTexture_BP(FName texName, UTexture2D *setTexture)
	{
		BindDebugTexture(texName, setTexture);
	}
//END PROTECTED

private:
	//State
	static TWeakObjectPtr<UMaterial> defaultMaterial;
	static FEchoCustomMeshImportSettings defaultMeshSettings;
	static TMap<FName, TWeakObjectPtr<UTexture2D>> debugTextureBindings;
	static bool debugPrintMaterialInfo;
	static bool hideDefaultUnwantedWarnings;
};
