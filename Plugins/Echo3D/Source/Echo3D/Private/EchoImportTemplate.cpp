// Fill out your copyright notice in the Description page of Project Settings.


#include "EchoImportTemplate.h"
#include "EchoMeshService.h"
#include "Echo3DService.h"

#include "EchoMemorySubsystem.h"

//hack to ensure cast is sane to compiler
#include "Materials/MaterialInstanceDynamic.h"

//TODO: should I just nuke these? since its getting kinda ugly and confusing?
#define RECURSIVE_PROPERTY_GETTER(ReturnType, ClassName, GetterName, PropAccess) \
	ReturnType ClassName :: GetterName () const \
	{ \
		auto &myCase = (PropAccess); \
		if (myCase != nullptr) \
		{ \
			return myCase; \
		} \
		const auto &parent = GetParentTemplate(); \
		if (parent != nullptr) \
		{ \
			const ClassName *parentAsThisType = Cast<ClassName>(parent); \
			if (parentAsThisType != nullptr) \
			{ \
				return parentAsThisType-> GetterName (); \
			} \
			/* else terminate recursion here, parent not convertable to this type */ \
		} \
		return nullptr; \
	}

//A variation needed for FSomethingTypes since blueprint doesn't allow pointers to them
#define RECURSIVE_PROPERTY_GETTER_REFERENCE(ReturnType, ClassName, GetterName, PropValidator, PropAccess) \
	bool ClassName :: GetterName (ReturnType &result) const \
	{ \
		if ( PropValidator ) \
		{ \
			/* store result in local case and return */ \
			result = (PropAccess); \
			return true; \
		} \
		const auto &parent = GetParentTemplate(); \
		if (parent != nullptr) \
		{ \
			const ClassName *parentAsThisType = Cast<ClassName>(parent); \
			if (parentAsThisType != nullptr) \
			{ \
				/* store result in data parent case and return */ \
				return parentAsThisType-> GetterName ( result ); \
			} \
			/* else terminate recursion, found a non-this-type template parent */ \
		} \
		/* failed - write default value for type and return */ \
		result = {}; \
		return false; \
	}


//hack
//#define MAT_TEMPLATE_CLASS UEchoHologramActorTemplate
#define MAT_TEMPLATE_CLASS UEchoMaterialBaseTemplate
//RECURSIVE_PROPERTY_GETTER( UMaterial*, MAT_TEMPLATE_CLASS, GetDefaultMaterialMaster, defaultMaterial ); //no longer recursive, possibly unneeded?
//RECURSIVE_PROPERTY_GETTER_REFERENCE( FEchoCustomMeshImportSettings, MAT_TEMPLATE_CLASS, GetImportSettings, bAreCustomModelSettingsValid, customModelImportSettings );


RECURSIVE_PROPERTY_GETTER_REFERENCE( FEchoCustomMeshImportSettings, UEchoHologramActorTemplate, GetImportSettings, bAreCustomModelSettingsValid, customModelImportSettings );
/*
bool UEchoHologramActorTemplate::GetImportSettings(FEchoCustomMeshImportSettings &results) const
{
	return false;
}
*/

//RECURSIVE_PROPERTY_GETTER( TSubclassOf<UObject>, UEchoHologramActorTemplate, GetBaseObjectClass, baseObjectClass );
RECURSIVE_PROPERTY_GETTER( TSubclassOf<AActor>, UEchoHologramActorTemplate, GetBaseObjectClass, baseObjectClass );
RECURSIVE_PROPERTY_GETTER( TSubclassOf<UActorComponent>, UEchoHologramActorTemplate, GetBaseUserComponent, baseUserComponent);




//RECURSIVE_PROPERTY_GETTER( UMaterial*, MAT_TEMPLATE_CLASS, GetDefaultMaterial, defaultMaterial );

/*bool UEchoImportTemplate::GetImportSettings(FEchoCustomMeshImportSettings &result) const
{
	return false; //HACK
}*/

//const UEchoImportTemplate *UEchoImportTemplate::ResolveTemplate_Implementation(
const UEchoHologramBaseTemplate *UEchoHologramTemplate::ResolveTemplate_Implementation(
		FEchoImportConfig &thisConfig
	) const
{
	//TODO: really need to have a reference to the echoservice instance or factory set for this
	//TODO: maybe support recursively? while(!userTemplate->userTemplateKey.IsEmpty())
	if (!userTemplateKey.IsEmpty())
	{
		//TODO: do we care about key value not found vs is empty string?
		FString useTemplateInstead = thisConfig.hologram.additionalData.ReadString(userTemplateKey);
		const UEchoHologramBaseTemplate *foundTemplateInstead = AEcho3DService::GetTemplate(useTemplateInstead);
		//TODO: possibly iterate recursively?
		if (foundTemplateInstead != nullptr)
		{
			return foundTemplateInstead;
		}
		else
		{
			//TODO: which echoservice object?
			FString echoServiceName = TEXT("static Echo3DService");
			UE_LOG(LogTemp, Error, TEXT("Hologram %s: Data specified templateKey via additionalData Field: '%s' did not find a valid template with key: '%s'"), *thisConfig.importTitle, *userTemplateKey, *useTemplateInstead);
			//TODO: should we return null or ourselves? its unclear which is the better behavior...
			// or maybe add a special not-found template and return that?
		}
	}
	return this;
}

//UMaterialInstance *UEchoImportTemplate::EvalMaterial_Implementation(
UMaterialInterface *UEchoMaterialBaseTemplate::EvalMaterial_Implementation(
		/*
		//const UObject *WorldContextObject,
		UObject *WorldContextObject,
		//UPARAM(ref)FEchoImportConfig & thisConfig, 
		//AActor *owner,
		const FEchoImportConfig &thisConfig, 
		const FEchoActorTemplateResolution &actorConfig,
		//const FFinalReturnData & materialResults, 
		const FFinalReturnData &importedData, 
		const FEchoImportMaterialData &forMaterial,
		const FEchoMeshVariants &forMeshVariant
		*/
		const FEchoImportConfig &importConfig, 
		const FEchoImportMeshConfig &meshConfig,
		const FFinalReturnData &importedData, 
		const FEchoImportMaterialData &forMaterial,
		const FEchoMeshVariants &forMeshVariant
	) const
{
	//TODO: do we want a super template version too?
	//return nullptr; //not implemented for now, just seeing how it jibes with blueprint
	//return UEchoMeshService::ParseOneMaterialDefault(WorldContextObject, owner, thisConfig, importedData, forMaterial, forMeshVariant, nullptr);
	//return UEchoMeshService::ParseOneMaterialDefault(WorldContextObject, thisConfig, actorConfig, importedData, forMaterial, forMeshVariant, nullptr);
	//return UEchoMeshService::ParseOneMaterialDefault(WorldContextObject, thisConfig, actorConfig, importedData, forMaterial, forMeshVariant, this->GetDefaultMaterialMaster());
	//return UEchoMeshService::ParseOneMaterialDefault(WorldContextObject, thisConfig, actorConfig, importedData, forMaterial, forMeshVariant, this->GetDefaultMaterialMaster());
	return UEchoMeshService::ParseOneMaterialDefault(
		importConfig.WorldContextObject.Get(), 
		importConfig.importTitle,
		//actorConfig, 
		importedData, forMaterial, forMeshVariant, this->GetDefaultMaterialMaster()
	);

}




void UEchoHologramAssetTemplate::ExecTemplate_Implementation(
		UPARAM(ref) FEchoImportConfig &importConfig
		//UObject *WorldContextObject,
		//const FEchoConnection &connection, 
		//UObject *useExisting, UClass *constructClass
	) const
{
	TArray<FEchoAssetRequest> requests;
	GenerateAssetRequests(importConfig, requests);
	if (requests.Num() < 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("UEchoHologramAssetTemplate::ExecTemplate: zero assets requested for %s"), *this->GetTemplateDebugString());
	}
	
	if (importConfig.IsStale())
	{
		UE_LOG(LogTemp, Error, TEXT("UEchoHologramAssetTemplate::ExecTemplate: WCO appears to be stale. abandoning construction"));
		return;
	}
	UObject *usingWCO = importConfig.WorldContextObject.Get();
	//if (!importConfig.IsValidWCO())
	if (usingWCO == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("UEchoHologramAssetTemplate::ExecTemplate: WCO appears to be null. abandoning construction"));
		return;
	}

	//local copy of state
	FString debugStringCopy = this->GetTemplateDebugString(); //capture here for possible later debug usage - TODO: maybe only do this in some debug mode(s)?
	TWeakObjectPtr<const UEchoHologramAssetTemplate> thisWeak = this;
	FEchoImportConfig importConfigCopy = importConfig;
	
	//TSharedRef<FEchoMemoryAssetCallback> delgateFunc= MakeShared<FEchoMemoryAssetCallback>();
	FEchoMemoryAssetArrayCallback delegateFunc;
	//delegateFunc.BindLambda([thisWeak, importConfigCopy, debugStringCopy](const TArray<const FEchoMemoryAsset> &assets)

	//FEchoMemoryGuard guardTemplate = UEchoMemorySubsystem::CreateGuard(importConfig.WorldContextObject, this);
	//TODO: provide a weakObjectPtr getter wrapper for ease of use?
	FEchoMemoryGuard guardTemplate(usingWCO, this);
	
	delegateFunc.BindLambda([thisWeak, importConfigCopy, debugStringCopy, guardTemplate](const TArray<FEchoMemoryAsset> &assets)
		{
			if (guardTemplate.IsStale())
			{
				UE_LOG(LogTemp, Error, TEXT("UEchoHologramAssetTemplate::ExecTemplate::(lambda): after assets retrieved, our guard is no longer valid"));
				return;
			}
			//we've resolved the assets.
			if (thisWeak.IsStale())
			{
				UE_LOG(LogTemp, Error, TEXT("UEchoHologramAssetTemplate::ExecTemplate::(lambda): after assets retrieved, 'this' has been destroyed in %s"), *debugStringCopy);
				return;
			}
			const UEchoHologramAssetTemplate *thisTemplate = thisWeak.Get();
			if (thisTemplate == nullptr)
			{
				//should not occur
				UE_LOG(LogTemp, Error, TEXT("UEchoHologramAssetTemplate::ExecTemplate: after assets retrieved, 'this' was somehow passed as a non-stale nullptr in %s"), *debugStringCopy);
				return;
			}
			//TODO: should we do anything to handle assets we failed to get?
			//TODO: do i need to capture another copy of importConfigCopy here?
			thisTemplate->ExecWithAssets(importConfigCopy, assets);
		}
	);

	AEcho3DService::RequestAssets(importConfigCopy.connection, requests, delegateFunc);
}

/*
//meh we should provide base class stuffs in init with blueprints
UEchoHologramActorTemplate::UEchoHologramActorTemplate()
{
	//or manage an instance??
	//this->materialTemplateClass = UEchoMaterialTemplate::StaticClass();
}

UEchoMaterialTemplate::UEchoMaterialTemplate()
{
	//this->defaultMaterial = 
	//static ConstructorHelpers::FObjectFinder<UMaterial> MaterialRef(TEXT("/Plugin"));
	//UStaticMesh* StaticMesh = MeshRef.Object;
	//check(StaticMesh != nullptr);
}
*/

void UEchoHologramActorTemplate::GenerateAssetRequests_Implementation(
		const FEchoImportConfig &importConfig,
		TArray<FEchoAssetRequest> &requests
	) const
{
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

	/*if (actorResolutionsBase.actorTemplate != nullptr)
	{
		actorResolutionsBase.actorTemplate->ResolveAssets(WorldContextObjectArg, userConfig, requests);
	}
	*/
}

void UEchoHologramActorTemplate::ExecWithAssets_Implementation(
		const FEchoImportConfig &importConfig, 
		//const TArray<const FEchoMemoryAsset> &assets
		const TArray<FEchoMemoryAsset> &assets
	) const
{

	AActor *useActor = nullptr;
	//useActor = weakUseExistingAsActor.Get();
	useActor = this->GetUseExistingActor();
	
	//FEchoImportConfig userConfigCopyInner = userConfigCopy;
	//FEchoImportConfig userConfigCopyInner = userConfigCopy;
	FEchoConstructActorResult constructionResult;
	if (useActor == nullptr)
	{
		//no existing actor, try to create one
		//const FString &hologramName = userConfigCopy.hologram.modelInfo.model.filename; //this is absolutely horrible
		const FString &hologramName = importConfig.hologram.modelInfo.model.filename; //this is absolutely horrible
		//GetBaseObjectClass
		//useActor = AEcho3DService::ConstructBaseActor(WorldContextObject, hologramName, userConfigCopyInner, actorResolutionsBase, softConstructActorClass.Get(), TSubclassOf<UActorComponent>(softUserComponent.Get()));
		//useActor = 
		constructionResult = AEcho3DService::ConstructBaseActor(
			importConfig,
			hologramName,
			this->GetBaseObjectClass(),
			this->GetBaseUserComponent(),
			nullptr//this->GetUseExistingActor() //must be null
		);
		useActor = constructionResult.actor;

		//	, softConstructActorClass.Get(), TSubclassOf<UActorComponent>(softUserComponent.Get()));
		if (useActor == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("Importing %s: ConstructBaseActor returned null!"), *importConfig.importTitle);
			return;
		}
	}
	else
	{
		//if (!softConstructActorClass.IsNull())
		//auto TSubclassOf<AActor> expectedClass = this->GetBaseUserComponent();
		TSubclassOf<AActor> expectedClass = this->GetBaseObjectClass();
		if (expectedClass != nullptr)
		{
			//sanity check
			//if (!softConstructActorClass.Get()->IsA( useActor->GetClass() ))
			//if (!softConstructActorClass.Get()->IsChildOf( useActor->GetClass() )) //check if useActor is NOT A SUBCLASS (note the !) of the provided class
			if (!expectedClass->IsChildOf( useActor->GetClass() )) //check if useActor is NOT A SUBCLASS (note the !) of the provided class
			{
				//TODO: can GetClass() ever return null??
				//UE_LOG(LogTemp, Error, TEXT("Importing %s: provided a construct actor class with an existing actor but the types do not match - existing is an %s, but specified asset %s"), *userConfigCopyInner.importTitle, *useActor->GetClass()->GetName(), *softConstructActorClass.GetAssetName());
				UE_LOG(LogTemp, Error, TEXT("Importing %s: provided a construct actor class with an existing actor but the types do not match - existing is an %s, but specified asset %s"), *importConfig.importTitle, *useActor->GetClass()->GetName(), *expectedClass->GetName());
			}
		}

		//TODO: possibly create the user component here?
		//TODO: maybe split construct into base actor and adding usercomponent??
	}

	//UEchoMeshService::Assemble(useActor, connectionCopy, assets, nullptr, userConfigCopyInner, actorResolutionsBase);
	FEchoImportMeshConfig meshConfig;
	meshConfig.actor = useActor;
	meshConfig.materialTemplate = this->GetMaterialTemplate(importConfig);
	bool ok;
	ok = this->GetImportSettings(meshConfig.meshImportSettings);
	if (!ok)
	{
		UE_LOG(LogTemp, Error, TEXT("GetImportSettings: failed to get valid import settings! importing: %s"), *importConfig.importTitle);
		meshConfig.meshImportSettings = FEchoCustomMeshImportSettings(); //UEchoMeshService::SetDefaultMeshImportSettings();
	}
	//meshConfig.spawnClass = nullptr; //????
	//UE_LOG(LogTemp, Error, TEXT("TODO: figure out how spawnClass was used in meshloader again"));

	UEchoMeshService::AttachMeshFromAssetArray(importConfig, meshConfig, assets);
	//useActor, connectionCopy, assets, nullptr, userConfigCopyInner, actorResolutionsBase);
	
	this->ExecAfter(importConfig, constructionResult.actor, constructionResult.userComponent);
	//TODO: move this to a subclass?
	//this->ExecTemplateAfter(
	//TODO: change this to an afterfunc in the template and make the default do this instead:
	//AEcho3DService::HandleDefaultMetaData(useActor, userConfigCopy.hologram);
	
	//AEcho3DService::HandleDefaultMetaData(useActor, importConfig.hologram);
};

/*
void UEchoHologramActorTemplate::ExecTemplateWithAssets_Implementation(
		UObject *WorldContextObject,
		FEchoImportConfig &thisConfig,
		const FEchoConnection &connection, 
		UObject *useExistingObject, UClass *constructBaseClassOverride
	) const
{
	//#Placeholder
	//UE_LOG(LogTemp, Error, TEXT("EchoTemplate::ExecTemplate_Implementation"));
	//AEcho3DService::ProcessActorHologramDefault(WorldContextObject, thisConfig, this, thisConfig.
	//AEcho3DService::ProcessActorHologramDefault(WorldContextObject, thisConfig, this, thisConfig.
	AEcho3DService::ProcessActorHologramDefault(WorldContextObject, thisConfig, this, connection, useExistingObject, constructBaseClassOverride, this->GetBaseUserComponent());
}
*/

void UEchoHologramDefaultActorTemplate::ExecAfter_Implementation(
		const FEchoImportConfig &importConfig,
		AActor *outputActor,
		UActorComponent *outputUserComponent
	) const
{
	AEcho3DService::HandleDefaultMetaData(outputActor, importConfig.hologram);
}