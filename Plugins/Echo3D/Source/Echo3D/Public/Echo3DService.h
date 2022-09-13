// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

//hack - dumping ground for structs for now
#include "EchoConnection.h"

#include "EchoStringConstants.h"
#include "EchoImportTemplate.h"

#include "UObject/StrongObjectPtr.h"

#include <functional>
#include "Echo3DService.generated.h"


//TODO: convert this to a UOBject, probably?
//TODO: use AutoCreateRefTerm - see https://benui.ca/unreal/ufunction/#autocreaterefterm

//TODO: reorder functions into logical parts?

//WorldContextObjectArg - WCO but need to capture and give a different name in the lambda scope, might want to rename lambda named version though
//TODO: what if WCO has become null?

//NB: UEchoImportTemplate => UEchoHologramBaseTemplate;

UCLASS()
class ECHO3D_API AEcho3DService : public AActor
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Echo3D")
	static FEchoHologramQueryResult ParseEchoHologramQuery(const FEchoRawQueryResult &rawQuery)
	{
		//return ParseEchoHologramQuery_Impl(rawQuery.sourceQuery,  rawQuery.bSuccess, rawQuery.GetContentAsString());
		return ParseEchoHologramQuery_Impl(rawQuery.debugInfo,  rawQuery.bSuccess, rawQuery.GetContentAsString());
	}
	
	/*
	UFUNCTION(BlueprintCallable, Category = "Echo3D")
	static FEchoHologramQueryResult ParseEchoHologramQuery2(const FString &querySource, bool isQueryOk, const FString &queryResponse)
	{
		return ParseEchoHologramQuery_Impl(querySource, isQueryOk, queryResponse);
	}
	*/

private:
	//static FEchoHologramQueryResult ParseEchoHologramQuery_Impl(const FString &querySource, bool isQueryOk, const FString &queryResponse);
	static FEchoHologramQueryResult ParseEchoHologramQuery_Impl(const FEchoQueryDebugInfo &querySource, bool isQueryOk, const FString &queryResponse);

public:	
	UFUNCTION(BlueprintCallable, Category = "Echo3D|Debug")
	static void SetVerboseMode(bool bSetVerboseMode)
	{
		if (bVerboseMode != bSetVerboseMode)
		{
			UE_LOG(LogTemp, Warning, TEXT("Setting Echo Verbose Mode to %s"), (bSetVerboseMode ? TEXT("true"):TEXT("false")));
		}
		bVerboseMode = bSetVerboseMode;
		
	}
	
	/**
	 * request holograms and handle via a specific template name
	**/
	static void RequestHolograms(UObject *WorldContextObject, const FEchoConnection &connection, const FString &useTemplateKey, const TArray<FString> &includeEntries = EchoStringConstants::EmptyStringArray, const TArray<FString> &includeTags = EchoStringConstants::EmptyStringArray, const FEchoHologramQueryHandler &callback = FEchoHologramQueryHandler());

	/**
	 * request holograms and handle via a specific template instance
	**/
	static void RequestHologramsTemplate(UObject *WorldContextObject, const FEchoConnection &connection, const UEchoHologramBaseTemplate *useTemplateInstance, const TArray<FString> &includeEntries = EchoStringConstants::EmptyStringArray, const TArray<FString> &includeTags = EchoStringConstants::EmptyStringArray, const FEchoHologramQueryHandler &callback = FEchoHologramQueryHandler());
	
	/**
	 * request holograms and handle via a specific template class
	**/
	//static void RequestHologramsTemplateClass(UObject *WorldContextObject, const FEchoConnection &connection, TSoftClassPtr<UEchoHologramBaseTemplate> useTemplateClass, const TArray<FString> &includeEntries = EchoStringConstants::EmptyStringArray, const TArray<FString> &includeTags = EchoStringConstants::EmptyStringArray, const FEchoHologramQueryHandler &callback = FEchoHologramQueryHandler());
	static void RequestHologramsTemplateClass(UObject *WorldContextObject, const FEchoConnection &connection, TSubclassOf<UEchoHologramBaseTemplate> useTemplateClass, const TArray<FString> &includeEntries = EchoStringConstants::EmptyStringArray, const TArray<FString> &includeTags = EchoStringConstants::EmptyStringArray, const FEchoHologramQueryHandler &callback = FEchoHologramQueryHandler());



	//probably deprecated?
	UFUNCTION(BlueprintCallable, Category = "Echo3D", meta=(WorldContext="WorldContextObject"))
	//static void DefaultHologramHandler(UObject *WorldContextObject, const FEchoConnection &connection, const FString &blueprintKey, const FEchoHologramQueryResult &result);
	static void DefaultHologramHandler(UObject *WorldContextObject, const FEchoConnection &connection, const UEchoHologramBaseTemplate *hologramTemplate, const FEchoHologramQueryResult &result);

	
	//TODO get/set echofactory?
	//TODO: set prefab templates?
	/*
	UFUNCTION(BlueprintCallable, Category = "Echo|DefaultBehavior", meta=(WorldContext="WorldContextObject"))
	static void ProcessActorHologramDefault(
		UObject *WorldContextObject, 
		FEchoImportConfig &userConfig, const UEchoHologramBaseTemplate *hologramTemplate, 
		const FEchoConnection &connection, 
		UObject *useExisting, 
		UClass *constructClass,
		TSubclassOf<UActorComponent> userSubclass
	);
	*/

	//Q: maybe pass an actor template?
	UFUNCTION(BlueprintCallable, Category = "Echo3D", meta=(WorldContext="WorldContextObject"))
	static void ProcessAllHolograms(UObject *WorldContextObject, const FEchoConnection &connection, const UEchoHologramBaseTemplate *hologramTemplate, const TArray<FEchoHologramInfo> &holograms, UClass *constructClass);
	
	UFUNCTION(BlueprintCallable, Category = "Echo3D", meta=(WorldContext="WorldContextObject"))
	static void ProcessOneHologram(UObject *WorldContextObject, const FEchoConnection &connection, const UEchoHologramBaseTemplate *hologramTemplate, const FEchoHologramInfo &hologram, UObject *useExisting, UClass *constructClass);




	UFUNCTION(BlueprintCallable, Category = "Echo3D")
	static void RequestAssetBP(const FEchoConnection &connection, const FEchoFile &storage, EEchoAssetType expectedType, bool bAllowCache, const FEchoMemoryAssetCallbackBP &callback);

	//TODO: add some kind of cache?
	//Better asset starting ponit than a raw DoEchoRequest.
	static void RequestAsset(const FEchoConnection &connection, const FEchoFile &storage, EEchoAssetType expectedType, bool bAllowCache, const FEchoMemoryAssetCallback &callback);

	//FEchoAssetRequestArray or pass connection into each?
	static void RequestAssets(const FEchoConnection &connection, const FEchoAssetRequestArray  &requests, const FEchoMemoryAssetArrayCallback &afterCallback);
	
	///////////////////////////////////////////////////////////////////////////////

	//TODO: rename these to RegisterTemplateXYZ ???
	UFUNCTION(BlueprintCallable, Category = "Echo3D|Templates")
	static void RegisterTemplateClassWithCustomBinding(const FString &useBinding, TSubclassOf<UEchoHologramBaseTemplate> fromTemplateClass)
	{
		RegisterTemplateClassHelper(fromTemplateClass, useBinding, true); 
	}

	UFUNCTION(BlueprintCallable, Category = "Echo3D|Templates")
	static void RegisterTemplateClass(TSubclassOf<UEchoHologramBaseTemplate> fromTemplateClass)
	{
		RegisterTemplateClassHelper(fromTemplateClass, EchoStringConstants::BadStringValue, false); 
	}

	/**
	 * binds a template to the given name. often you'll just want AssignTemplateClass since its easier if you just want to use a singleton with preconfigured editor data
	**/
	UFUNCTION(BlueprintCallable, Category = "Echo3D|Templates")
	static void RegisterTemplate(const FString &bindTemplateName, const UEchoHologramBaseTemplate *setTemplate)
	{
		if (setTemplate == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("AssignTemplate: null template given"));
			return;
		}
		//TODO: delete template?/replacetemplate?
		if (templates.Contains(bindTemplateName))
		{
			const UEchoHologramBaseTemplate *foundTemplate = templates[bindTemplateName].Get();
			if (foundTemplate != setTemplate)
			{
				FString foundTemplateName = (foundTemplate!=nullptr) ? foundTemplate->GetTemplateDebugString() : EchoStringConstants::NullString;
				FString withTemplateName = (setTemplate!=nullptr) ? setTemplate->GetTemplateDebugString() : EchoStringConstants::NullString;
				UE_LOG(LogTemp, Warning, TEXT("Template key already bound '%s' to template %s, replacing it with template '%s'"), *bindTemplateName, *foundTemplateName, *withTemplateName);
			}
			else
			{
				FString withTemplateName = (setTemplate!=nullptr) ? setTemplate->GetTemplateDebugString() : EchoStringConstants::NullString;
				UE_LOG(LogTemp, Warning, TEXT("Template key already bound '%s' to template %s, replacing with itself"), *bindTemplateName, *withTemplateName);
			}
			templates[bindTemplateName] = TStrongObjectPtr<const UEchoHologramBaseTemplate>(setTemplate);
		}
		else
		{
			//TODO: if debug templates?
			FString withTemplateName = (setTemplate!=nullptr) ? setTemplate->GetTemplateDebugString() : EchoStringConstants::NullString;
			UE_LOG(LogTemp, Log, TEXT("Template bound '%s' to template %s"), *bindTemplateName, *withTemplateName);
			templates.Add(bindTemplateName, TStrongObjectPtr<const UEchoHologramBaseTemplate>(setTemplate));
		}
		//template is now live
	}
	
	UFUNCTION(BlueprintCallable, Category = "Echo3D|Templates")
	//static UEchoHologramBaseTemplate* InstantiateTemplateFromClass(TSubclassOf<UEchoHologramBaseTemplate> templateClass)
	static const UEchoHologramBaseTemplate *ResolveTemplateFromClass(TSubclassOf<UEchoHologramBaseTemplate> templateClass)
	{
		if (templateClass == nullptr)
		{
			//this would crash in newobject
			UE_LOG(LogTemp, Error, TEXT("ResolveTemplateFromClass: templateClass was nullptr!"));
			return nullptr;
		}
		//UEchoHologramBaseTemplate* ret =NewObject<UEchoHologramBaseTemplate>(GetTransientPackage(), templateClass);
		const UEchoHologramBaseTemplate* ret =GetDefault<UEchoHologramBaseTemplate>(templateClass);
		if (ret == nullptr)
		{
			if (templateClass.Get() == nullptr)
			{
				UE_LOG(LogTemp, Error, TEXT("ResolveTemplateFromClass: returning nullptr from template: <null>"));
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("ResolveTemplateFromClass: returning nullptr from template: %s"), *templateClass->GetName());
			}
		}
		return ret;
	}

	UFUNCTION(BlueprintCallable, Category = "Echo3D|Templates")
	static void DeleteTemplate(const FString &templateName)
	{
		//if (templates.Get
		templates.Remove(templateName);
	}
	
	UFUNCTION(BlueprintPure, Category = "Echo3D|Templates")
	static const UEchoHologramBaseTemplate *GetTemplate(const FString &templateName)//, UEchoHologramBaseTemplate *defaultValue)
	{
		//tryget?
		//UEchoHologramBaseTemplate *ret = nullptr;
		if (templates.Contains(templateName))
		{
			const auto &result = templates.Find(templateName);
			if (result != nullptr)
			{
				return result->Get();
			}
			//else warn?
		}
		return nullptr;
	}

	UFUNCTION(BlueprintCallable, Category = "Echo3D|Templates")
	static void SetDefaultTemplateByClass(TSubclassOf<UEchoHologramBaseTemplate> setDefaultClass)
	{
		//what if setDefaultClass is null? lets just do this and warn if we get a class holding nullptr for now - they can cal setDefaultTemplate null if they want
		SetDefaultTemplate(ResolveTemplateFromClass(setDefaultClass));
	}

	UFUNCTION(BlueprintCallable, Category = "Echo3D|Templates")
	static void SetDefaultTemplate(const UEchoHologramBaseTemplate *setDefaultTemplate)
	{
		//TODO: should we also store this in the templates as (emptystring)?
		defaultTemplate = TStrongObjectPtr<const UEchoHologramBaseTemplate>(setDefaultTemplate);
	}

	UFUNCTION(BlueprintPure, Category = "Echo3D|Templates")
	static const UEchoHologramBaseTemplate *GetDefaultTemplate()
	{
		//TODO: warn if have not completed finishinit?
		return defaultTemplate.Get();
	}


	///////////////////////////////////////////////////////////////////////////////

	/**
	 * pass true to enable more detailed logging messages for queries. useful if something is going wrong with the http requests 
	**/
	UFUNCTION(BlueprintCallable, Category = "Echo3D|Debug")
	static void SetLogFailedQueries(bool bSetLogFailedQueries)
	{
		logFailedQueries = true;
	}
	
	UFUNCTION(BlueprintPure, Category = "Echo3D|Debug")
	static bool GetLogFailedQueries()
	{
		return logFailedQueries;
	}

	/**
	 * pass true to use the development endpoint. mostly of interest for internal echo3d testing/development.
	 * default is false (use production endpoint)
	**/
	UFUNCTION(BlueprintCallable, Category = "Echo3D|Debug")
	static void SetUseDevelopmentEndpoint(bool bSetUseDevEndpoint);

	/**
	 * gets the last set endpoint mode. true = using development endpoint, false = using production
	**/
	UFUNCTION(BlueprintPure, Category = "Echo3D|Debug")
	static bool GetUseDevelopmentEndpoint();

	//provided to ensure we checkfornewrun even if we never queue any tasks.
	//safe to have called queueTaskAfterinit first as those are reset by a world change 
	UFUNCTION(BlueprintCallable, Category = "Echo3D|Lifecycle", meta=(WorldContext="WorldContextObject"))
	static void BeginInit(const UObject *WorldContextObject);

	//calls all queued after init tasks the first time
	UFUNCTION(BlueprintCallable, Category = "Echo3D|Lifecycle", meta=(WorldContext="WorldContextObject"))
	static void FinishInit(const UObject *WorldContextObject);
	
	//#TODO_MAKE_INSTANCE_METHOD
	//TODO: perhaps force a non-linear delay even if ready immediately so control flow shenanigans are caught and the same in all use cases
	/**
	 * executes callback during finishInit or immediately if we've already called finishInit
	 * used to ensure timing of multiple scripts entering beginplay are syncronized to any setup on their echoservice instance
	**/
	UFUNCTION(BlueprintCallable, Category = "Echo3D|Lifecycle",  meta=(WorldContext="WorldContextObject"))
	static void QueueTaskAfterInit(const UObject *WorldContextObject, const FEchoCallback &afterInit);
	
	//static void QueueTaskAfterInitEcho(const UObject *WorldContextObject, const FEchoCallback &afterInit);//Q: rename functions later?

	//#TODO_MAKE_INSTANCE_METHOD
	/**
	 * used internally to generate the value of FEchoImportConfig used to process holograms.
	 * public since might be needed elsewhere, but mostly of interest if doing things outside the normal workflow, such as doing some local file loading etc manually
	**/
	UFUNCTION(BlueprintCallable, Category = "Echo3D|Resolver", meta=(WorldContext="WorldContextObject"))
	//static const UEchoHologramBaseTemplate *ResolveTemplateAndCreateConfig(UObject *WorldContextObject, FEchoImportConfig &result, const FString &blueprintKey, const FEchoHologramInfo &hologramInfo);
	//static const UEchoHologramBaseTemplate *ResolveTemplateAndCreateConfig(UObject *WorldContextObject, FEchoImportConfig &outputConfig, const UEchoHologramBaseTemplate *hologramTemplate, const FEchoHologramInfo &hologramInfo);
	static const UEchoHologramBaseTemplate *ResolveTemplateAndCreateConfig(
		UObject *WorldContextObject, const FEchoConnection &usingConnection, const FEchoHologramInfo &hologramInfo,
		const UEchoHologramBaseTemplate *hologramTemplate,
		FEchoImportConfig &outputConfig
	);

	//TODO: should be part of the template logic base class
	//TODO: actor config probably shouldn't have hologramtemplate?? (or at least should not be duplicated in userConfig!)
	//#TODO_MAKE_INSTANCE_METHOD
	/**
	 * used internally to generate the value of FEchoImportConfig used to process holograms.
	 * public since might be needed elsewhere, but mostly of interest if doing things outside the normal workflow, such as doing some local file loading etc manually
	**/
	//UFUNCTION(BlueprintCallable, Category = "Echo3D|Lifecycle", meta=(WorldContext="WorldContextObject"))
	//static void ResolveActorTemplate(UObject *WorldContextObject, FEchoActorTemplateResolution &result, FEchoImportConfig userConfig, const UEchoHologramBaseTemplate *hologramTemplate);

	

///Echo provided default behaviors:
	/**
	 * applies the default echo metadata behaviors
	**/
	UFUNCTION(BlueprintCallable, Category = "Echo3D")
	static void HandleDefaultMetaData(AActor *actor, const FEchoHologramInfo &hologram);

	UFUNCTION(BlueprintCallable, Category = "Echo3D")
	//static AActor *ConstructBaseActor(
	static FEchoConstructActorResult ConstructBaseActor(
		const FEchoImportConfig &userConfig,
		const FString &setLabel, 
		TSubclassOf<AActor> spawnClassOverride, 
		TSubclassOf<UActorComponent> addUserClass, //this can apparently also hold a null value so dont need a pointer to it
		AActor *useExistingActor
		//const UEchoHologramActorTemplate *thisTemplate,
		
		/*
		UObject *WorldContextObject, 
		const FString &setLabel, 
		UPARAM(ref) FEchoImportConfig &userConfig, FEchoActorTemplateResolution importTemplate,
		*/

		//TSubclassOf<UActorComponent> *addUserClass
	);
	
protected:
	//TODO: add a blueprintable wrapper with dynamic delegates? lol ISignatureable?
	
	friend class UEchoCallbackNode; //HACK
	static FEchoRequestResultInfo DoEchoRequest(const FString &requestURL, const FEchoRequestHandler &callback);
	
	//Or set static WCO?
	//, meta=(WorldContext="WorldContextObject")
	//static UObject *ConstructBaseObject(const UObject *WorldContextObject, const FString &factoryKey, UClass *spawnClass, const FString &setLabel);
	//Note could concievably want to mutate userConfig to pass along information
	//static UObject *AEcho3DService::ConstructBaseObject(UObject *WorldContextObject, const FString &setLabel, UPARAM(ref) FEchoImportConfig &userConfig, FEchoActorTemplateResolution importTemplate, UClass *spawnClassOverride);
	//static UObject *AEcho3DService::ConstructBaseActor(UObject *WorldContextObject, const FString &setLabel, UPARAM(ref) FEchoImportConfig &userConfig, FEchoActorTemplateResolution importTemplate, UClass *spawnClassOverride);
	//static UObject *AEcho3DService::ConstructBaseActor(UObject *WorldContextObject, const FString &setLabel, UPARAM(ref) FEchoImportConfig &userConfig, FEchoActorTemplateResolution importTemplate, UClass *spawnClassOverride, TSubclassOf<UActorComponent> *addUserClass);
	//static UObject *AEcho3DService::ConstructBaseActor(UObject *WorldContextObject, const FString &setLabel, UPARAM(ref) FEchoImportConfig &userConfig, FEchoActorTemplateResolution importTemplate, TSubclassOf<AActor> spawnClassOverride, TSubclassOf<UActorComponent> *addUserClass);
	//static UObject *ConstructBaseActor(UObject *WorldContextObject, const FString &setLabel, UPARAM(ref) FEchoImportConfig &userConfig, FEchoActorTemplateResolution importTemplate, TSubclassOf<AActor> spawnClassOverride, TSubclassOf<UActorComponent> *addUserClass);
	//TODO: how to tell if the uclass is no longer valid via tsubclassof?

	/**
	 * internal helper for assigntemplateclassX variants
	**/
	static void RegisterTemplateClassHelper(TSubclassOf<UEchoHologramBaseTemplate> fromTemplateClass, const FString &customBind, bool bUseCustomBind)
	{
		//TODO: template renamed callback as XYZ? - could safely allow self-mutation right after instantiation of a new instance?
		// or else should we reuse the instance as a 'singleton' of sorts?
		const UEchoHologramBaseTemplate *myTemplate = ResolveTemplateFromClass(fromTemplateClass);
		if (myTemplate != nullptr)
		{
			//const FString &bindAs = bUseCustomBind ? customBind : myTemplate->templateKey;
			const FString &bindAs = bUseCustomBind ? customBind : myTemplate->GetTemplateKey();
			//AssignTemplate(myTemplate->templateKey, myTemplate);
			RegisterTemplate(bindAs, myTemplate);
		}
		else
		{
			if (fromTemplateClass.Get() == nullptr)
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to assign a template from template class: <null>"));
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to assign a template from template class: %s"), *fromTemplateClass->GetName());
			}
		}
	}

	//Wrapper around RequestHolograms so that we can play nice with Blueprint AND C++ users.
	//TODO: can I just "hide" this from C++ by making it protected but still visible to blueprint?
	//TODO: how to "hide" this in C++ autocomplete?
	//TODO check names
	
	/**
	 * Requests holograms using the specified templateKey
	**/
	UFUNCTION(BlueprintCallable, Category = "Echo3D", meta=(AutoCreateRefTerm="callback, includeEntries, includeTags", WorldContext="WorldContextObject"), DisplayName="RequestHolograms")
	static void RequestHolograms_BP(UObject *WorldContextObject, const FEchoConnection &connection, const FString &blueprintKey, const TArray<FString> &includeEntries, const TArray<FString> &includeTags,  const FEchoHologramQueryHandler &callback)
	{
		RequestHolograms(WorldContextObject, connection, blueprintKey, includeEntries, includeTags, callback);
	}

	UFUNCTION(BlueprintCallable, Category = "Echo3D", meta=(AutoCreateRefTerm="callback, includeEntries, includeTags", WorldContext="WorldContextObject"), DisplayName="RequestHologramsTemplate")
	static void RequestHologramsTemplate_BP(UObject *WorldContextObject, const FEchoConnection &connection, const UEchoHologramBaseTemplate *usingTemplate, const TArray<FString> &includeEntries, const TArray<FString> &includeTags,  const FEchoHologramQueryHandler &callback)
	{
		RequestHologramsTemplate(WorldContextObject, connection, usingTemplate, includeEntries, includeTags, callback);
	}
	
	UFUNCTION(BlueprintCallable, Category = "Echo3D", meta=(AutoCreateRefTerm="callback, includeEntries, includeTags", WorldContext="WorldContextObject"), DisplayName="RequestHologramsTemplateClass")
	static void RequestHologramsTemplateClass_BP(UObject *WorldContextObject, const FEchoConnection &connection, TSubclassOf<UEchoHologramBaseTemplate> usingTemplateClass, const TArray<FString> &includeEntries, const TArray<FString> &includeTags,  const FEchoHologramQueryHandler &callback)
	{
		RequestHologramsTemplateClass(WorldContextObject, connection, usingTemplateClass, includeEntries, includeTags, callback);
	}

	
	/*
	AActor *AEcho3DService::ConstructBaseActor(
		UObject *WorldContextObject, 
		const FString &setLabel, 
		FEchoImportConfig &userConfig, FEchoActorTemplateResolution importTemplate, TSubclassOf<AActor> *spawnClassOverride, 
		TSubclassOf<UActorComponent> addUserClass
	)
	*/


private:
	static void CheckForNewRun(const UObject *WorldContextObject);
	static void OnStaticReset();

	//UFUNCTION(BlueprintCallable, Category = "Echo3D|Lifecycle", meta=(WorldContext="WorldContextObject"))
	//static void ResetState(const UObject *WorldContextObject);

	//do we ever need to call this explicitly?
	UFUNCTION(BlueprintCallable, Category = "Echo3D|Lifecycle")
	static void ResetState();


	//static FEchoImportConfig ResolveTemplateAndCreateConfig(const FString &blueprintKey, const FEchoHologramInfo &hologramInfo);

	/** helpers for init **/
	enum class EWhichRequestHologramsCase : uint8
	{
		UseKey = 0,
		UseInstance = 1,
		UseClass = 2,
	};
	struct RequestHologramsArgHelper
	{
		EWhichRequestHologramsCase whichCase;

		FString viaKey; //NOTE: "late bound" - resolved after we have the holograms from HTTP

		//TWeakObjectPtr<UEchoHologramBaseTemplateClas> viaInstance;
		//TWeakObjectPtr<UEchoTemplateClas> viaInstance;
		TWeakObjectPtr<const UEchoHologramBaseTemplate> viaInstance;
		
		//TSoftClassPtr<UEchoHologramBaseTemplate> viaClass;
		TSubclassOf<UEchoHologramBaseTemplate> viaClass;

		RequestHologramsArgHelper(const FString &usingKey)
		 : whichCase( EWhichRequestHologramsCase::UseKey )
		 , viaKey( usingKey )
		 , viaInstance( nullptr )
		 , viaClass( nullptr )
		{
		}

		RequestHologramsArgHelper(const UEchoHologramBaseTemplate *usingInstance)
		 : whichCase( EWhichRequestHologramsCase::UseInstance )
		 , viaKey( EchoStringConstants::BadStringValue )
		 , viaInstance( usingInstance )
		 , viaClass( nullptr )
		{
		}

		//RequestHologramsArgHelper(TSoftClassPtr<UEchoHologramBaseTemplate> usingClass)
		RequestHologramsArgHelper(TSubclassOf<UEchoHologramBaseTemplate> usingClass)
		 : whichCase( EWhichRequestHologramsCase::UseInstance )
		 , viaKey( EchoStringConstants::BadStringValue )
		 , viaInstance( nullptr )
		 , viaClass( usingClass )
		{
		}
	};

	/**
	 * common requestHologramsXYZ implementation
	**/
	static void RequestHologramsHelper(UObject *WorldContextObject, const FEchoConnection &connection, RequestHologramsArgHelper requestedTemplateArg, const TArray<FString> &includeEntries = EchoStringConstants::EmptyStringArray, const TArray<FString> &includeTags = EchoStringConstants::EmptyStringArray, const FEchoHologramQueryHandler &callback = FEchoHologramQueryHandler());

	
////////////////////////////// OLD /////////////////////////////////////////
/*
public:	
	// Sets default values for this actor's properties
	AEcho3DService();
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	*/
public:
	
	// https://docs.unrealengine.com/5.0/en-US/event-programming-in-unreal-engine/
	//see https://stackoverflow.com/questions/62165120/c-equivalent-blueprint-event-dispatcher-vs-blueprint-events
	//apparently our choices are DECLARE_DYNAMIC_DELEGATE, DECLARE_MULTICAST_DELEGATE, and DECLARE_DYNAMIC_MULTICAST_DELEGATE 

	//UFUNCTION(BlueprintCallable, Category = "Echo3D")
	//static void SetServerDomain(const FString &setDomain){}

	//DECLARE_DYNAMIC_DELEGATE(FVoidDelegate);
	//DECLARE_DYNAMIC_MULTICAST_DELEGATE(FInfoEvent);

	//UFUNCTION(BlueprintCallable, Category = "Echo3D")
	//void Connect(const FString &apiKey, const FString &secKey){}

	//UFUNCTION(BlueprintCallable, Category = "Echo3D")
	//void RequestAllMulticast();

private:
	
	//UPROPERTY(EditAnywhere, BlueprintAssignable, Category = "Echo3D|EchoField")
	//FInfoEvent requestHandlersTest;

	//FEchoHologramQueryHandler defaultHologramHandler;

	static bool bVerboseMode;
	static bool logFailedQueries;

	static TArray<FEchoCallback> queuedPostInitTasks;
	static TWeakObjectPtr<UWorld> lastPlayWorld;
	static bool bInitCalled;


	//TODO: do i need a strongobject reference? lol if non uproperty??
	//--maybe upropertyify things?
	//static TDictionary<FString, UEchoHologramBaseTemplate> templates;
	//static TMap<FString, UEchoHologramBaseTemplate> templates;
	//static TMap<FString, TStrongObjectPtr<TSubclassOf<UEchoHologramBaseTemplate>>> templates;
	static TMap<FString, TStrongObjectPtr<const UEchoHologramBaseTemplate>> templates;
	static TStrongObjectPtr<const UEchoHologramBaseTemplate> defaultTemplate;
};
