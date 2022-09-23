// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

#include "EchoStructsFwd.h"
#include "EchoConnection.h"

//for matfunc
#include "EchoImporter.h"


#include "EchoImportTemplate.generated.h"

/**
 * defines a bunch of templates to extend to provide import functionality that the user can subclass to easily implement certain goals.
 * the most common uses presently will probably be to subclass UEchoHologramTemplate, UEchoHologramActorTemplate or UEchoMaterialTemplate
**/


//TODO: create some style of templates for various kinds of assets so we can handle SFX, lights etc and other stuff
//TODO: remove template from the name to avoid C++ confusion???
//TODO: split these into their own file

/**
 * base class for hologram template instantiation state
 * our templates are meant to be immutable so this allows us to hold mutable working state
**/
UCLASS(BlueprintType, Blueprintable)
class ECHO3D_API UEchoTemplateState : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TemplateState")
	const UEchoHologramBaseTemplate* fromTemplate;
};

class UEchoMaterialBaseTemplate;

/**
 * almost more of an interface. the minimal functionality for any kind of hologram template
**/
UCLASS(BlueprintType, Blueprintable)
class ECHO3D_API UEchoHologramBaseTemplate : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * get the template key to bind this to by default
	**/
	UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category = "EchoTemplate")
	FString GetTemplateKey() const;

	//TODO: does this really belong in the base class?
	/**
	 * meant to cause handling of a hologram via of certain templates to be an error. mainly meant for templates that are supposed to be subclassed or reference another template
	 *  fuctionally meant to work kind of like an abstract class.
	**/
	UFUNCTION(BlueprintPure, Category = "EchoTemplate")
	bool IsAbstractTemplate() const
	{
		return bAbstractTemplate;
	}

	/**
	 * Resolves the template. 
	 * note that this can return a new template instance and/or modify thisConfig. the returned template will be used thereafter. To continue using this template return, have it return this (which is the default sans some template renaming magic)
	 * however, THIS is const since it could be a CDO or other static data. but you COULD create a new template instance, mutate that and return it. 
	**/ 
	UFUNCTION(BlueprintPure=false, BlueprintNativeEvent, meta=(WorldContext="WorldContextObject", CallableWithoutWorldContext), Category = "EchoTemplate")
	const UEchoHologramBaseTemplate *ResolveTemplate(
		UPARAM(ref) FEchoImportConfig &thisConfig
	) const;

	/**
	 * get a debug representation of a template
	**/
	UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category = "EchoTemplate")
	FString GetTemplateDebugString() const;

	/**
	 * create mutable state for the template to work with.
	**/
	UFUNCTION(BlueprintPure=false, BlueprintNativeEvent, Category = "EchoTemplate")
	UEchoTemplateState *CreateInstanceState(
		UPARAM(ref) FEchoImportConfig &importConfig
	) const;

	UFUNCTION(BlueprintPure=false, BlueprintNativeEvent, Category = "EchoTemplate", meta=(CallableWithoutWorldContext))
	void ExecTemplate(
		UPARAM(ref) FEchoImportConfig &thisConfig
	) const;

	/**
	 * test function should not be callable in a sane context since everywhere should pretty much be const.
	 * if you can succesfully call this something has subverted the const-ness of the type system
	**/
	UFUNCTION(BlueprintCallable, Category = "EchoSanityCheck")
	void TestMutator()
	{
		//NB: non-const test method. should not be callable or else we'll have problems
		UE_LOG(LogTemp, Error, TEXT("We were able to call this mutator in blueprint"));
	}

protected:
	//I don't want to deal with abstract classes and blueprint atop everything so provide a default template class value
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|Base", Meta=(BlueprintProtected))
	FString templateKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|Base", Meta=(BlueprintProtected))
	bool bAbstractTemplate;
	
	/**
	 * C++ version of GetTemplateKey
	**/
	virtual FString GetTemplateKey_Implementation() const
	{
		return templateKey;
	}

	/**
	 * default implementation of ResolveTemplate. honors userTemplateKey, if non-empty, otherwise returns this
	 * behavior is not presently specified if the user template key was not found
	**/
	virtual const UEchoHologramBaseTemplate *ResolveTemplate_Implementation(
		UPARAM(ref) FEchoImportConfig &thisConfig
	) const
	{
		return this;
	}

	virtual FString GetTemplateDebugString_Implementation() const
	{
		//TODO: add a 'friendly name' variant?
		FString instanceString;
		{
			const UEchoHologramBaseTemplate *CDOSingleton = GetDefault<UEchoHologramBaseTemplate>(this->GetClass());
			//bool bIsCDO = IsTemplate(this);
			bool bIsCDO = (this == CDOSingleton);
			if (bIsCDO)
			{
				instanceString = TEXT("@ClassDefaultObject");
			}
			else
			{
				instanceString = FString::Printf(TEXT("@Instance: " ECHO_FORMAT_ARG_HEX64_WITH0x), (uint64)this);
			}
		}
		return FString::Printf(TEXT("Template{class='%s', templateKey='%s', %s}"), 
			*GetClass()->GetName(), *templateKey, *instanceString
		);
	}

	/**
	 * C++ overrideable version of CreateInstanceState. the default is to just create a UEchoTemplateState object but you can return any thing you want to be passed back to yourself later
	**/
	virtual UEchoTemplateState *CreateInstanceState_Implementation(
		UPARAM(ref) FEchoImportConfig &thisConfig
	) const
	{
		if (thisConfig.WorldContextObject.IsStale())
		{
			UE_LOG(LogTemp, Error, TEXT("CreateInstanceState: WCO is stale"));
			return nullptr;
		}
		UEchoTemplateState *ret = NewObject<UEchoTemplateState>(thisConfig.WorldContextObject.Get());
		ret->fromTemplate = this;
		return ret;
	}

	virtual void ExecTemplate_Implementation(
		UPARAM(ref) FEchoImportConfig &thisConfig
	) const
	{
		//does nothing, by design
	}
};


//TODO: break out the reference stuff into a new type of template base class instead of having it in everything?
/**
 * a slightly more focused template class to derive from. this does a lot of our base template defaults
 * this is usually the recommended base class for new templates
**/
UCLASS(BlueprintType, Blueprintable)
class ECHO3D_API UEchoHologramTemplate : public UEchoHologramBaseTemplate
{
	GENERATED_BODY()

protected:

///////// TEMPLATE STUFF
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|Base")
	FString userTemplateKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|Base")
	TSubclassOf<UEchoHologramBaseTemplate> parentTemplate;

	/** helper to get the parent template **/
	UFUNCTION(BlueprintPure, Category = "EchoTemplate")
	const UEchoHologramBaseTemplate *GetParentTemplate() const
	{
		if (parentTemplate != nullptr)
		{
			return GetDefault<UEchoHologramBaseTemplate>(parentTemplate);
		}
		return nullptr;
	}

//// implementations
	//TODO: should we have custom template subclass for this instead of providing it for any template subclass???
	/**
	 * provides more flexible support for templates. in particular this class allows forwarding of templates via a metadata property
	**/
	virtual const UEchoHologramBaseTemplate *ResolveTemplate_Implementation(
		FEchoImportConfig &thisConfig
	) const override;
};








/**
 * a hologram template that knows about assets
 * this is meant to be derrived from.
 * GenerateAssetRequests is called during ExecTemplate and generates a list of assets to request, which are then retrieved and ExecWithAssets is called.
**/
UCLASS(BlueprintType, Blueprintable)
class ECHO3D_API UEchoHologramAssetTemplate : public UEchoHologramTemplate
{
	GENERATED_BODY()

public:
	/**
	 * generates a list of assets to request from the echo website before continuing
	**/
	UFUNCTION(BlueprintPure=false, BlueprintNativeEvent, Category = "EchoTemplate|Assets")
	void GenerateAssetRequests(
		const FEchoImportConfig &importConfig, 
		UPARAM(ref) TArray<FEchoAssetRequest> &requests
	) const;

	/**
	 * resumes execution post-retrieval of assets
	**/
	UFUNCTION(BlueprintPure=false, BlueprintNativeEvent, Category = "EchoTemplate")
	void ExecWithAssets(
		const FEchoImportConfig &importConfig, 
		//const TArray<const FEchoMemoryAsset> &assets
		const TArray<FEchoMemoryAsset> &assets
	) const;

protected:
	
	/**
	 * provides basic functionality that requests assets and expects subclasses to handle the details in ExecWithAssets
	**/
	virtual void ExecTemplate_Implementation(
		UPARAM(ref) FEchoImportConfig &thisConfig
	) const override;

	/**
	 * generates a list of assets to request from the echo website before continuing
	 * overridable in c++
	**/
	virtual void GenerateAssetRequests_Implementation(
		const FEchoImportConfig &importConfig,
		TArray<FEchoAssetRequest> &requests
	) const
	{
		//default implementation does nothing.
	}

	/**
	 * This is called after assets are retrieved. This subclasses should probably override this rather than ExecTemplate unless they want to do something beforehand, in which case they should probably call ExecTemplate in their super
	**/
	virtual void ExecWithAssets_Implementation(
		const FEchoImportConfig &importConfig, 
		const TArray<FEchoMemoryAsset> &assets
	) const
	{
		//base does nothing by default.
	}
};








//TODO: should this be two classes???
//TODO: subclass actor further with mesh stuff and echo default stuffs?
//TODO: UEchoHologramActorBaseTemplate ???
//This should probably be split into an actor base template and a more echo-centric-way one
/**
 * a hologram template that knows about actors, meshes and materials?
**/
UCLASS(BlueprintType, Blueprintable)
class ECHO3D_API UEchoHologramActorTemplate : public UEchoHologramAssetTemplate
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintPure, Category = "EchoTemplate|Construct")
	TSubclassOf<AActor> GetBaseObjectClass() const;
	
	/**
	 * executed after we've otherwise finished constructing the actor. 
	 * mostly to easily perform some final tasks on a constructed actor
	**/
	UFUNCTION(BlueprintPure=false, BlueprintNativeEvent, Category = "EchoTemplate|Construct")
	void ExecAfter(
		const FEchoImportConfig &importConfig,
		AActor *outputActor,
		UActorComponent *outputUserComponent
	) const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|Base", Meta=(BlueprintProtected))
	TSubclassOf<AActor> baseObjectClass;
	
	//TODO: probably just deprecate this?
	//This is going to be a pain to handle recursively, probably?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|Base", Meta=(BlueprintProtected))
	FEchoAdditionalData configurationData;

	virtual void ExecAfter_Implementation(
		const FEchoImportConfig &importConfig,
		AActor *outputActor,
		UActorComponent *outputUserComponent
	) const
	{
		//TODO: maybe call handleDefaultMetaData here?
	}

////////////////// MESH STUFF
public:
	/**
	 * resolves the import settings if there is any valid settings in the data heirarchy
	 * returns false if the result is invalid and should be ignored (ie no results found)
	 * 
	 * TODO: should this be overrideable?
	**/
	UFUNCTION(BlueprintPure, Category = "EchoTemplate|Accessor")
	bool GetImportSettings(FEchoCustomMeshImportSettings& result) const; //forward declaration for blueprint, implemented below
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EchoTemplate|MeshDefaults", Meta=(BlueprintProtected))
	FEchoCustomMeshImportSettings customModelImportSettings;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EchoTemplate|MeshDefaults", Meta=(BlueprintProtected))
	bool bAreCustomModelSettingsValid;
	
	
///////////////////////////
///Actor template stuff
///////////////////////////

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|Base")
	TSubclassOf<UActorComponent> baseUserComponent;

	//TODO: support a Class method like for the hologram templates??
	//HACK
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|Base")
	const UEchoMaterialBaseTemplate *materialTemplate;

	/**
	 * if no instance is provided we'll use this
	**/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|Base")
	const TSubclassOf<UEchoMaterialBaseTemplate> materialTemplateClass;

	UFUNCTION(BlueprintPure, Category = "EchoTemplate|Accessor")
	TSubclassOf<UActorComponent> GetBaseUserComponent() const;

	UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category = "EchoTemplate|Accessor")
	AActor *GetUseExistingActor() const;

	//placeholder
	//TODO: probably should be public?
	UFUNCTION(BlueprintPure, Category = "EchoTemplate|Accessor")
	const UEchoMaterialBaseTemplate *GetMaterialTemplate(
		const FEchoImportConfig &importConfig
	) const
	{
		return GetMaterialTemplateImpl(importConfig, this);
	}

	/**
	 * generates a list of assets to request from the echo website before continuing
	 * overridable in c++
	**/
	virtual void GenerateAssetRequests_Implementation(
		const FEchoImportConfig &importConfig,
		TArray<FEchoAssetRequest> &requests
	) const override;

	/**
	 * This is called after assets are retrieved. This subclasses should probably override this rather than ExecTemplate unless they want to do something beforehand, in which case they should probably call ExecTemplate in their super
	**/
	virtual void ExecWithAssets_Implementation(
		const FEchoImportConfig &importConfig, 
		//const TArray<const FEchoMemoryAsset> &assets
		const TArray<FEchoMemoryAsset> &assets
	) const override;

	virtual AActor *GetUseExistingActor_Implementation() const
	{
		return nullptr; //construct a new actor by default
	}

private:
	//recursive form with better error messaging
	const UEchoMaterialBaseTemplate *GetMaterialTemplateImpl(
		const FEchoImportConfig &importConfig,
		const UEchoHologramActorTemplate *leafTemplate
	) const
	{
		if (materialTemplate != nullptr)
		{
			return materialTemplate;
		}
		else
		{
			if (materialTemplateClass.Get() != nullptr)
			{
				return GetDefault<UEchoMaterialBaseTemplate>(materialTemplateClass);
			}
			else
			{
				const UEchoHologramActorTemplate *parentActorTemplate = Cast<const UEchoHologramActorTemplate>( GetParentTemplate() );
				if (parentActorTemplate != nullptr)
				{
					return parentActorTemplate->GetMaterialTemplateImpl( importConfig, leafTemplate );
				}
				else
				{
					//out of fallbacks
					UE_LOG(LogTemp, Error, TEXT("No Material Template found for actor template %s while importing %s"), *leafTemplate->GetTemplateDebugString(), *importConfig.importTitle);
					return nullptr;
				}
			}
		}
		
	}
};






/**
 * A very simple specialization of an echo actor template that does any specific default echo behaviors such as handling metadata
**/
UCLASS(BlueprintType, Blueprintable)
class ECHO3D_API UEchoHologramDefaultActorTemplate : public UEchoHologramActorTemplate
{
	GENERATED_BODY()

protected:
	virtual void ExecAfter_Implementation(
		const FEchoImportConfig &importConfig,
		AActor *outputActor,
		UActorComponent *outputUserComponent
	) const override;
};

/*
UCLASS(BlueprintType, Blueprintable)
class ECHO3D_API UEchoImportTemplate : public UEchoHologramDefaultActorTemplate
{
GENERATED_BODY()
};
*/


















//TODO: revist this later on with some improvements to be able to recieve other memory assets and have a mutable state object like hologramtemplates -- if that turns out useful!
/**
 * base material template. generally want to use UEchoMaterialTemplate instead as this is meant to (eventually) provide only the minimal functionality
**/
UCLASS(BlueprintType, Blueprintable)
class ECHO3D_API UEchoMaterialBaseTemplate : public UObject
{
	GENERATED_BODY()
public:
////////MATERIAL STUFF
public:
	//TODO: move to some kind of UEchoMaterialTemplate class
	/**
	 * Resolve a material instance for this object. This is what is actually used to generate material instances
	**/
	UFUNCTION(BlueprintPure=false, BlueprintNativeEvent, meta=(WorldContext="WorldContextObject", CallableWithoutWorldContext), Category = "EchoTemplate")
	UMaterialInterface *EvalMaterial(
		const FEchoImportConfig &importConfig, 
		const FEchoImportMeshConfig &meshConfig,
		const FFinalReturnData &importedData, 
		const FEchoImportMaterialData &forMaterial,
		const FEchoMeshVariants &forMeshVariant
	) const;

	FString GetTemplateName() const
	{
		return templateName;
	}

protected:
	//TODO: check these are still visible and dont need private access?
	
	//HACK:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|Base", Meta=(BlueprintProtected))
	FString templateName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|Base", Meta=(BlueprintProtected))
	UMaterial *defaultMaterial;
	
	UMaterialInterface *EvalMaterial_Implementation(
		const FEchoImportConfig &importConfig, 
		const FEchoImportMeshConfig &meshConfig,
		const FFinalReturnData &importedData, 
		const FEchoImportMaterialData &forMaterial,
		const FEchoMeshVariants &forMeshVariant
	) const;
	
	//TODO: maybe expand this into a proper thing again?
	//UFUNCTION(BlueprintPure, BlueprintNativeEvent, meta=(WorldContext="WorldContextObject", CallableWithoutWorldContext), Category = "EchoTemplate")
	UMaterial *GetDefaultMaterialMaster() const
	{
		return defaultMaterial;
	}
};




/**
 * a placeholder for a more usable class of UEchoMaterialBaseTemplate 
**/
UCLASS(BlueprintType, Blueprintable)
class ECHO3D_API UEchoMaterialTemplate : public UEchoMaterialBaseTemplate 
{
	GENERATED_BODY()
public:
	//UEchoMaterialTemplate();
};

