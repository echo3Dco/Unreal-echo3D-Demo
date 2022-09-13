// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

#include "EchoStructsFwd.h"
#include "EchoConnection.h"

//for matfunc
#include "EchoImporter.h"


#include "EchoImportTemplate.generated.h"
//TODO: maybe rename UEchoImportTemplate to UEchoTemplate ??
//TODO: remove template from the name to avoid C++ confusion???
//UEchoPattern? something?
//lol: UEchoConstructionFactoryRegistrant //#facepalm


//TODO: restructure these into a base template class and subclass it with specializations!
//TODO: should all properties be editable only during construction somehow????
//TODO: figure out how to never modify CDO via blueprint properties? - can blueprint understand const-ness?

//TODO: can i update a template from a hologram???
//TODO: rename this?
 ////class ECHO3D_API UEchoImportTemplate : public UDataAsset
 //TODO: maybe make actorimporttemplate specialization??? or provide a uobject maker?
 //I don't know enough about slate to handle this in a sane way for now so just gonna go full force to actors


 //TODO: TFactory helpers to convert? or UClass based instantiation?

/*
class UEchoImportTemplate;//hologram

class UEchoImportActorTemplate;
class UEchoImportMeshTemplate;
class UEchoImportMaterialTemplate;
class UEchoImportTextureTemplate;
*/
//I'm realizing these templates basically feel like functional programming since everything has to appear const unless it dynamically generates an new result (which is then const)


//TODO: undef RECURSIVE_PROPERTY_GETTER at the end
//RECURSIVE_PROPERTY_GETTER_REFERENCE

//TODO: how to support slate or other uobject subclasses?
	//-probably via new helper templates other than actor template
	//TODO: execute which template or subclass templates such as actorhologramtemplate extends hologramtemplate?

//TODO: does this need to become a UObject so it can be properly reference counted along with its uproperties?
//contains resolved templates for an actor case
/*
USTRUCT(BlueprintType)
struct ECHO3D_API FEchoActorTemplateResolution
{
	GENERATED_BODY()

	//these should all be const at this point
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EchoTemplate")
	TWeakObjectPtr<const UEchoImportTemplate> hologramTemplate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EchoTemplate")
	TWeakObjectPtr<const UEchoImportActorTemplate> actorTemplate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EchoTemplate")
	TWeakObjectPtr<const UEchoImportMeshTemplate> meshTemplate;
	
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EchoTemplate")
	//UEchoImportMaterialTemplate *materialTemplate;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EchoTemplate")
	//UEchoImportTextureTemplate *textureTemplate;

	void Reset()
	{
		hologramTemplate = nullptr;
		actorTemplate = nullptr;
		meshTemplate = nullptr;
		//materialTemplate = nullptr;
		//textureTemplate = nullptr;
	}

	//does anything use this right now??
	bool IsAnyStale()
	{
		return (hologramTemplate.IsStale() || actorTemplate.IsStale() || meshTemplate.IsStale());
		// or material or texture
	}
};
*/

//TODO: could tsubclass of UEchoHologramTemplate instead of UEchoHologramBaseTemplate??
//TODO: should these use reference counting or "garbage collection counting"??
/**
 * base class for template instantiation state
**/
UCLASS(BlueprintType, Blueprintable)
class ECHO3D_API UEchoTemplateState : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TemplateState")
	const UEchoHologramBaseTemplate* fromTemplate;
};

//TODO: revist this later on with some improvements to be able to recieve other memory assets and have a mutable state object like hologramtemplates -- if that turns out useful!
UCLASS(BlueprintType, Blueprintable)
//class ECHO3D_API UEchoMaterialTemplate : public UObject
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
	//UFUNCTION(BlueprintCallable, BlueprintNativeEvent, meta=(WorldContext="WorldContextObject", CallableWithoutWorldContext), Category = "EchoTemplate")
	//TODO: material state? / prepare material template instance data/state?
	UFUNCTION(BlueprintPure=false, BlueprintNativeEvent, meta=(WorldContext="WorldContextObject", CallableWithoutWorldContext), Category = "EchoTemplate")
	UMaterialInterface *EvalMaterial(
		const FEchoImportConfig &importConfig, 
		const FEchoImportMeshConfig &meshConfig,
		const FFinalReturnData &importedData, 
		const FEchoImportMaterialData &forMaterial,
		const FEchoMeshVariants &forMeshVariant
		/*
		UObject *WorldContextObject,
		const FEchoImportConfig &thisConfig, 
		//const FEchoActorTemplateResolution &actorConfig,
		const FFinalReturnData &importedData, 
		const FEchoImportMaterialData &forMaterial,
		const FEchoMeshVariants &forMeshVariant
		*/
	) const;

	//placeholder
	//UFUNCTION(BlueprintPure, BlueprintNativeEvent, meta=(WorldContext="WorldContextObject", CallableWithoutWorldContext), Category = "EchoTemplate")
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
	
	//unused for now
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|Base", Meta=(BlueprintProtected))
	//TMap<FString, UMaterial*> namedMaterials;

	/**
	 * returns the default material master to be used. This is what the default implementation will use
	**/
	//UFUNCTION(BlueprintPure, Category = "EchoTemplate|Accessor")
	//UMaterial* GetDefaultMaterialMaster() const;

	//Default C++ Implementation
	UMaterialInterface *EvalMaterial_Implementation(
		const FEchoImportConfig &importConfig, 
		const FEchoImportMeshConfig &meshConfig,
		const FFinalReturnData &importedData, 
		const FEchoImportMaterialData &forMaterial,
		const FEchoMeshVariants &forMeshVariant
		
		//UObject *WorldContextObject,
		//const FEchoImportConfig &thisConfig, 
		//const FEchoActorTemplateResolution &actorConfig,
		
	) const;
	
	//TODO: maybe expand this into a proper thing again?
	//UFUNCTION(BlueprintPure, BlueprintNativeEvent, meta=(WorldContext="WorldContextObject", CallableWithoutWorldContext), Category = "EchoTemplate")
	UMaterial *GetDefaultMaterialMaster() const
	{
		return defaultMaterial;
	}

	
};
//TODO: should we have GetTemplateKey_BP and _BP_Implementation and wrap around gettemplatekey? - probably not since c++ wants to call the bp versions ugh
/**
 * almost more of an interface
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
	//UFUNCTION(BlueprintPure=false, BlueprintNativeEvent, meta=(WorldContext="WorldContextObject", CallableWithoutWorldContext), Category = "EchoTemplate")
	UFUNCTION(BlueprintPure=false, BlueprintNativeEvent, meta=(WorldContext="WorldContextObject", CallableWithoutWorldContext), Category = "EchoTemplate")
	const UEchoHologramBaseTemplate *ResolveTemplate(
		UPARAM(ref) FEchoImportConfig &thisConfig
	) const;

	/**
	 * more of a debug name. this needs to be changed to be less confusing with getTemplateKey
	**/
	UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category = "EchoTemplate")
	//FString GetTemplateName() const;
	FString GetTemplateDebugString() const;

	//TODO: make this take external arguments or something that can be provided to processholograms???
	/**
	 * create mutable state for the template to work with.
	**/
	UFUNCTION(BlueprintPure=false, BlueprintNativeEvent, Category = "EchoTemplate")
	UEchoTemplateState *CreateInstanceState(
		//UObject *WorldContextObject 
		UPARAM(ref) FEchoImportConfig &importConfig
	) const;

	//UFUNCTION(BlueprintPure=false, BlueprintNativeEvent, meta=(WorldContext="WorldContextObject", CallableWithoutWorldContext), Category = "EchoTemplate")
	//meta=(WorldContext="WorldContextObject", CallableWithoutWorldContext)
	UFUNCTION(BlueprintPure=false, BlueprintNativeEvent, Category = "EchoTemplate", meta=(CallableWithoutWorldContext))
	void ExecTemplate(
		//UObject *WorldContextObject,
		UPARAM(ref) FEchoImportConfig &thisConfig
		//const FEchoConnection &connection,
		//UObject *useExisting, UClass *constructClass //TODO should these two be deprecated??
	) const;

	/**
	 * test function should not be callable in a sane context since everywhere should pretty much be const.
	 * if you can succesfully call this something has subverted the const-ness of the type system
	**/
	//UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "EchoTemplate")
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

	/*
	virtual FString GetTemplateName_Implementation() const
	{
		return GetClass()->GetName() + TEXT("_") + templateKey;
	}
	*/
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
		//UE_LOG(LogTemp, Error, TEXT("INSTANCE STRING = %s"), *instanceString);
		//UE_LOG(LogTemp, Error, TEXT("\tClazz="), *GetClass()->GetName());
		//UE_LOG(LogTemp, Error, TEXT("\tTemplateKey = %s"), *templateKey);
		return FString::Printf(TEXT("Template{class='%s', templateKey='%s', %s}"), 
			*GetClass()->GetName(), *templateKey, *instanceString
		);
	}

	/**
	 * C++ overrideable version of CreateInstanceState. the default is to just create a UEchoTemplateState object but you can return any thing you want to be passed back to yourself later
	**/
	//virtual UEchoTemplateState *CreateInstanceState_Implementation(UObject *WorldContextObject ) const
	virtual UEchoTemplateState *CreateInstanceState_Implementation(
		UPARAM(ref) FEchoImportConfig &thisConfig
	) const
	{
		UEchoTemplateState *ret = NewObject<UEchoTemplateState>(thisConfig.WorldContextObject);
		ret->fromTemplate = this;
		return ret;
	}

	virtual void ExecTemplate_Implementation(
		UPARAM(ref) FEchoImportConfig &thisConfig
		//UObject *WorldContextObject,
		//UPARAM(ref) FEchoImportConfig &thisConfig,
		//const FEchoImportConfig &thisConfig,
		//const FEchoConnection &connection, 
		//UObject *useExisting, UClass *constructClass
	) const
	{
		//does nothing, by design
	}
};





/**
 * a slightly more focused template class to derive from. this does a lot of our base template defaults
 * this is usually the recommended base class for new templates
**/
UCLASS(BlueprintType, Blueprintable)
//class ECHO3D_API UEchoHologramTemplate : public UObject
class ECHO3D_API UEchoHologramTemplate : public UEchoHologramBaseTemplate
{
	GENERATED_BODY()

/////////////// RANDOM TODOS
	//TODO: some kind of material-resesolver logics?? etc...
	//TODO: ability to handle custom asset lists on the target object? like SFX/lights etc?

	//TODO: outer function that things should use that calls this then parent function?

	//TODO: pass in a FEchoActorTemplateResolution to be filled out??
	
public:
	//TODO: maybe make this a native event later if needed?
	/*
	UFUNCTION(BlueprintPure, Category = "EchoTemplates")
	bool IsAbstractTemplate() const
	{
		return abstractTemplate;
	}
	*/


protected:

///////// TEMPLATE STUFF
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|Base")
	//FString templateKey;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|Base")
	FString userTemplateKey;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|Base")
	//bool abstractTemplate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|Base")
	TSubclassOf<UEchoHologramBaseTemplate> parentTemplate;
	
//public:
	
protected:
	//helper utility
	//TODO: does this still belong in the base template class?
	//TODO: maybe define a templateBase that doesn't provide them but have the normal implementation do so?
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


	/*
	virtual override void ExecTemplate_Implementation(
		UObject *WorldContextObject,
		UPARAM(ref) FEchoImportConfig &thisConfig,
		const FEchoConnection &connection, 
		UObject *useExisting, UClass *constructClass
	) const;
	*/
/////////////////// HOLOGRAM TEMPLATE
};










/**
 * a hologram template that knows about assets
 * this is meant to be derrived from.
 * GenerateAssetRequests is called during ExecTemplate and generates a list of assets to request, which are then retrieved and ExecWithAssets is called.
**/
UCLASS(BlueprintType, Blueprintable)
//class ECHO3D_API UEchoHologramTemplate : public UObject
class ECHO3D_API UEchoHologramAssetTemplate : public UEchoHologramTemplate
{
	GENERATED_BODY()

public:
	//TODO: should these have callable w/o worldcontext object?
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
		//const TArray<const FEchoMemoryAsset> &assets
		const TArray<FEchoMemoryAsset> &assets
	) const
	{
		//base does nothing by default.
	}
};










//TODO: should this be two classes???
//TODO: subclass actor further with mesh stuff and echo default stuffs?
//TODO: UEchoHologramActorBaseTemplate ???
/**
 * a hologram template that knows about actors, meshes and materials?
**/
UCLASS(BlueprintType, Blueprintable)
//class ECHO3D_API UEchoHologramTemplate : public UObject
//class ECHO3D_API UEchoHologramTemplate : public UEchoHologramAssetTemplate
class ECHO3D_API UEchoHologramActorTemplate : public UEchoHologramAssetTemplate
{
	GENERATED_BODY()

public:
	//TODO: echoconstructable?
	//UFUNCTION(BlueprintCallable, BlueprintNativeEvent, meta=(WorldContext="WorldContextObject", CallableWithoutWorldContext), Category = "EchoTemplate")
	//TODO: we probably want some version of this that can run "pre" asset downloads in case you just want to inspect the assets not download them
	//TODO: maybe make a function to flag assets as to be downloaded or not? and then can have more behavior in the actor/mesh/etc specific parts?

	//TODO: maybe i just want to pass around/store a "my template data" class type and then a lot of this (and the endless infinite arguments) becomes just a matter of making a subclass for custom details and casting back to it?
	

	//TODO: which template does this belong in?
	UFUNCTION(BlueprintPure, Category = "EchoTemplate|Construct")
	//TSubclassOf<UObject> GetBaseObjectClass() const;
	//TSubclassOf<UObject> GetBaseObjectClass() const;
	TSubclassOf<AActor> GetBaseObjectClass() const;
	//TSubclassOf<UObject> GetBaseActorClass() const;

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
	//TSubclassOf<UObject> baseObjectClass;
	TSubclassOf<AActor> baseObjectClass;
	//UClass *baseObjectClass;
	
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
public:
	/*
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, meta=(WorldContext="WorldContextObject", CallableWithoutWorldContext), Category = "EchoTemplate")
	const UEchoImportActorTemplate *ResolveActorTemplate(
		UObject *WorldContextObject,
		UPARAM(ref) FEchoImportConfig &thisConfig
	) const; //implemented by blueprint codegen
	*/
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

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|Base")
	//TSubclassOf<UEchoImportActorTemplate> defaultActorTemplateClass;
	
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|Base", Meta=(BlueprintProtected))
	//UEchoImportActorTemplate *defaultActorTemplate;

	//TODO: rename to getbaseusercompoentclass?
	//UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category = "EchoTemplate|Accessor")
	UFUNCTION(BlueprintPure, Category = "EchoTemplate|Accessor")
	TSubclassOf<UActorComponent> GetBaseUserComponent() const;

	UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category = "EchoTemplate|Accessor")
	AActor *GetUseExistingActor() const;

	//placeholder
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
	
	/*
	const UEchoImportActorTemplate *ResolveActorTemplate_Implementation(
		UObject *WorldContextObject,
		UPARAM(ref) FEchoImportConfig &thisConfig
	) const
	{
		//This might be wrong...should find the instance? or are we okay since we're doing the recursion here?
		// we probably still want a resolver for defaultActorTemplate class so subclasses can use this behavior too?
		//#Placeholder
		UE_LOG(LogTemp, Error, TEXT("EchoTemplate::ResolveActorTemplate_Implementation"));
		//return nullptr;
		if (defaultActorTemplate == nullptr)
		{
			if (defaultActorTemplateClass == nullptr)
			{
				//We've exhausted the possible resolutions within this template, try our parent
				//NOTE that this is different from a sublclass returning null which ends the search
				//try the data "super" version
				//dont cache for now, might change dynamically in parent
				return ResolveActorTemplateSuper(WorldContextObject, thisConfig);
			}
			//defaultActorTemplate = NewObject<UEchoImportActorTemplate>(WorldContextObject, defaultActorTemplateClass.Get());
			//return NewObject<UEchoImportActorTemplate>(WorldContextObject, defaultActorTemplateClass.Get());
			//if (defaultActorTemplateClass
			return GetDefault<UEchoImportActorTemplate>(defaultActorTemplateClass);
		}
		return defaultActorTemplate;
	}

	/|**
	 * helper to call data inheritance "super" method for ResolveActorTemplate
	**|/
	UFUNCTION(BlueprintPure=false, meta=(WorldContext="WorldContextObject", CallableWithoutWorldContext, BlueprintProtected), Category = "EchoTemplate")
	const UEchoImportActorTemplate *ResolveActorTemplateSuper(
		UObject *WorldContextObject,
		UPARAM(ref) FEchoImportConfig &thisConfig
	) const
	{
		auto parent = GetParentTemplate();
		if (parent != nullptr)
		{
			return parent->ResolveActorTemplate(WorldContextObject, thisConfig);
		}
		return nullptr;
	}
	*/
};











////////////////////////////
//TODO: switch me to a mesh resolver or something with a parsed mesh that can be invoked later and have a callback
//TODO: add a material "master" template for different types of materials that could be handled and have actor templates retrieve one?
//TODO: IEchoActorTemplate vs UEchoActorTemplateSimple ???
/*
UCLASS(BlueprintType, Blueprintable)
class ECHO3D_API UEchoImportMeshTemplate : public UObject
{
	GENERATED_BODY()

////////////// Template Stuff
public:
	//default bind to template key
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|Base")
	FString templateKey;
	
	/|**
	 * resolves a template into its final form. this can just return ourselves if desired.
	 * Or it could return a completely different template, possibly based on the passed in data.
	 * The default does the latter if userTemplateKey is set in the blueprint template
	 * 
	 * note that thisConfig is non-const in case you want to pass along some additional data or otherwise modify/transform the input data
	 *|/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, meta=(WorldContext="WorldContextObject", CallableWithoutWorldContext), Category = "EchoTemplate")
	const UEchoImportMeshTemplate *ResolveTemplate(
		UObject *WorldContextObject,
		UPARAM(ref) const FEchoActorTemplateResolution &actorTemplateResolution,
		UPARAM(ref) FEchoImportConfig &thisConfig
	) const;

protected:
	const UEchoImportMeshTemplate *ResolveTemplate_Implementation(
		UObject *WorldContextObject,
		UPARAM(ref) const FEchoActorTemplateResolution &actorTemplateResolution,
		UPARAM(ref) FEchoImportConfig &thisConfig
	) const
	{
		return this;
	}

///////////// Mesh Stuff
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, meta=(WorldContext="WorldContextObject", CallableWithoutWorldContext), Category = "EchoTemplate")
	void ApplyMesh(
		UObject *WorldContextObject,
		UEchoHologramBaseTemplate *holoTemplate,
		UEchoImportActorTemplate *actorTemplate,
		UPARAM(ref) FEchoImportConfig &thisConfig
	) const;

	//TODO: should this populate the rest of the templateResolution?
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, meta=(WorldContext="WorldContextObject", CallableWithoutWorldContext), Category = "EchoTemplate")
	UEchoImportMeshTemplate *ResolveMeshTemplate(
		UObject *WorldContextObject,
		UPARAM(ref) const FEchoActorTemplateResolution &actorTemplateResolution,
		UPARAM(ref) FEchoImportConfig &thisConfig
	) const;

protected:
	void ApplyMesh_Implementation(
		UObject *WorldContextObject,
		UEchoHologramBaseTemplate *holoTemplate,
		UEchoImportActorTemplate *actorTemplate,
		UPARAM(ref) FEchoImportConfig &thisConfig
	) const
	{
		//#Placeholder
		UE_LOG(LogTemp, Error, TEXT("MeshTemplate::ApplyMesh_Implementation"));
	}

	
	UEchoImportMeshTemplate *ResolveMeshTemplate_Implementation(
		UObject *WorldContextObject,
		UPARAM(ref) const FEchoActorTemplateResolution &actorTemplateResolution,
		UPARAM(ref) FEchoImportConfig &thisConfig
	) const
	{
		//#Placeholder
		UE_LOG(LogTemp, Error, TEXT("MeshTemplate::ResolveMeshTemplate_Implementation"));
		return nullptr;
	}
};
*/






/*
//moved to UEchoHologramActorTemplate
UCLASS(BlueprintType, Blueprintable)
class ECHO3D_API UEchoImportActorTemplate : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo3D|Base")
	FString templateKey;
	
	//can mutate config
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, meta=(WorldContext="WorldContextObject", CallableWithoutWorldContext), Category = "EchoTemplate")
	const UEchoImportActorTemplate *ResolveTemplate(
		UObject *WorldContextObject,
		UPARAM(ref) FEchoImportConfig &thisConfig
	) const;

	const UEchoImportActorTemplate *ResolveTemplate_Implementation(
		UObject *WorldContextObject,
		UPARAM(ref) FEchoImportConfig &thisConfig
	) const
	{
		return this;
	}
	
	//TODO: template group for import? - a lightweight copyable object with a bunch of template pointers
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, meta=(WorldContext="WorldContextObject", CallableWithoutWorldContext), Category = "EchoTemplate")
	AActor *EvalActor(
		UObject *WorldContextObject,
		//UEchoHologramBaseTemplate *viaHologramTemplate,
		UPARAM(ref) FEchoImportConfig &thisConfig,
		const FTransform &applyTransform
	) const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, meta=(WorldContext="WorldContextObject", CallableWithoutWorldContext), Category = "EchoTemplate")
	const UEchoImportMeshTemplate *ResolveMeshTemplate(
		UObject *WorldContextObject,
		//UEchoHologramBaseTemplate *viaHologramTemplate,
		UPARAM(ref) const FEchoActorTemplateResolution &actorTemplateResolution,
		UPARAM(ref) FEchoImportConfig &thisConfig
	) const;

protected:
	AActor *EvalActor_Implementation(
		UObject *WorldContextObject,
		//UEchoHologramBaseTemplate *viaHologramTemplate,
		UPARAM(ref) FEchoImportConfig &thisConfig,
		const FTransform &applyTransform
	) const
	{
		//TODO: just spawnactor by default?
		//#Placeholder
		UE_LOG(LogTemp, Error, TEXT("ActorTemplate::EvalActor_Implementation"));
		return nullptr;
	}


	const UEchoImportMeshTemplate *ResolveMeshTemplate_Implementation(
		UObject *WorldContextObject,
		//UEchoHologramBaseTemplate *viaHologramTemplate,
		UPARAM(ref) const FEchoActorTemplateResolution &actorTemplateResolution,
		UPARAM(ref) FEchoImportConfig &thisConfig
	) const
	{
		//#Placeholder
		UE_LOG(LogTemp, Error, TEXT("ActorTemplate::ResolveMeshTemplate_Implementation"));
		return nullptr;
	}

///////////////////// Assets stuff	
//NB: now handled via extending echohologramssetstemplate
//public:
	
	//UFUNCTION(BlueprintCallable, BlueprintNativeEvent, meta=(WorldContext="WorldContextObject", CallableWithoutWorldContext), Category = "EchoTemplate")
	//update the requested assets list as desired
	UFUNCTION(BlueprintPure=false, BlueprintNativeEvent, Category = "EchoTemplate")
	void ResolveAssets(
		UObject *WorldContextObject,
		//UEchoHologramBaseTemplate *viaHologramTemplate,
		UPARAM(ref) FEchoImportConfig &thisConfig,
		//UPARAM(ref) FEchoAssetRequestArray &requestAssets
		UPARAM(ref) TArray<FEchoAssetRequest> &requestAssets
	) const;
	
protected:
	void ResolveAssets_Implementation(
		UObject *WorldContextObject,
		//UEchoHologramBaseTemplate *viaHologramTemplate,
		UPARAM(ref) FEchoImportConfig &thisConfig,
		//UPARAM(ref) FEchoAssetRequestArray &requestAssets
		UPARAM(ref) TArray<FEchoAssetRequest> &requestAssets
	) const
	{
		//Default: no change.
		//probably want to reset list if not loading anything
	}
};
*/

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

/**
 * an epic hack since i couldn't figure out core redirects. this should be nuked once anything was reparented
**/
//deprecated
/*
UCLASS(BlueprintType, Blueprintable)
class ECHO3D_API UEchoImportTemplate : public UEchoHologramDefaultActorTemplate
{
GENERATED_BODY()
};
*/