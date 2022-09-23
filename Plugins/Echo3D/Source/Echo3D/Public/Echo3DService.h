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

//TODO: Refactor to UEcho3DService and refactor into helpers + a shared subsystem portion
/**
 * Meant to be the main entry point for requesting things from echo
**/
UCLASS()
class ECHO3D_API AEcho3DService : public AActor
{
	GENERATED_BODY()

public:
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
	static void RequestHologramsTemplateClass(UObject *WorldContextObject, const FEchoConnection &connection, TSubclassOf<UEchoHologramBaseTemplate> useTemplateClass, const TArray<FString> &includeEntries = EchoStringConstants::EmptyStringArray, const TArray<FString> &includeTags = EchoStringConstants::EmptyStringArray, const FEchoHologramQueryHandler &callback = FEchoHologramQueryHandler());

	/**
	 * requests one asset then calls the callback
	**/
	static void RequestAsset(const FEchoConnection &connection, const FEchoFile &storage, EEchoAssetType expectedType, bool bAllowCache, const FEchoMemoryAssetCallback &callback);

	/**
	 * kicks of several asset requests then calls the callback handler once they are all complete
	**/
	static void RequestAssets(const FEchoConnection &connection, const FEchoAssetRequestArray  &requests, const FEchoMemoryAssetArrayCallback &afterCallback);
	
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * registers an instance of a template class via the provided name to bind to
	**/
	UFUNCTION(BlueprintCallable, Category = "Echo3D|Templates")
	static void RegisterTemplateClassWithCustomBinding(const FString &useBinding, TSubclassOf<UEchoHologramBaseTemplate> fromTemplateClass)
	{
		RegisterTemplateClassHelper(fromTemplateClass, useBinding, true); 
	}

	/**
	 * registers an instance of a template class by GetTemplateKey on said instance
	**/
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
			if (bDebugTemplateOperations)
			{
				UE_LOG(LogTemp, Log, TEXT("Template bound '%s' to template %s"), *bindTemplateName, *withTemplateName);
			}
			templates.Add(bindTemplateName, TStrongObjectPtr<const UEchoHologramBaseTemplate>(setTemplate));
		}
		//template is now live
	}
	
	/**
	 * resolves a template class to an instance we can actually do (const) stuff with
	**/
	UFUNCTION(BlueprintCallable, Category = "Echo3D|Templates")
	static const UEchoHologramBaseTemplate *ResolveTemplateFromClass(TSubclassOf<UEchoHologramBaseTemplate> templateClass)
	{
		if (templateClass == nullptr)
		{
			//this would crash in newobject
			UE_LOG(LogTemp, Error, TEXT("ResolveTemplateFromClass: templateClass was nullptr!"));
			return nullptr;
		}
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

	/**
	 * unregisters a template
	**/
	UFUNCTION(BlueprintCallable, Category = "Echo3D|Templates")
	static void UnregisterTemplate(const FString &templateName)
	{
		//note that this would also release our strong reference to said template object
		templates.Remove(templateName);
	}
	
	/**
	 * retrieves the template instance bound to a string
	**/
	UFUNCTION(BlueprintPure, Category = "Echo3D|Templates")
	static const UEchoHologramBaseTemplate *GetTemplate(const FString &templateName)//, UEchoHologramBaseTemplate *defaultValue)
	{
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

	/**
	 * set the default fallback template to an instance of the provided template class
	**/
	UFUNCTION(BlueprintCallable, Category = "Echo3D|Templates")
	static void SetDefaultTemplateByClass(TSubclassOf<UEchoHologramBaseTemplate> setDefaultClass);

	/**
	 * explicitly clears the default template so that we don't warn about it
	**/
	UFUNCTION(BlueprintCallable, Category = "Echo3D|Templates")
	static void ClearDefaultTemplate()
	{
		SetDefaultTemplateImpl(nullptr, true);
	}

	/**
	 * sets a template meant to be used as a fallback if we don't find a template with the key we meant to
	 * this possibly might be used in the future if ResolveTemplate returns null
	**/
	UFUNCTION(BlueprintCallable, Category = "Echo3D|Templates")
	static void SetDefaultTemplate(const UEchoHologramBaseTemplate *setDefaultTemplate)
	{
		//TODO: should we also store this in the templates as (emptystring)?
		SetDefaultTemplateImpl(setDefaultTemplate, false);
	}

	

	/**
	 * gets the instance assigned as the default (fallback) template if nothing else is found.
	**/
	UFUNCTION(BlueprintPure, Category = "Echo3D|Templates")
	static const UEchoHologramBaseTemplate *GetDefaultTemplate()
	{
		//TODO: warn if have not completed finishinit?
		return defaultTemplate.Get();
	}
	
	/////////////////////////////////////////////////////////////////////////////////////////////////
	
	//provided to ensure we checkfornewrun even if we never queue any tasks.
	//safe to have called queueTaskAfterinit first as those are reset by a world change 
	/**
	 * causes a bunch of static state to be reset. This will probably be nuked when we start using actual instances and a subsystem as we'll just recieve proper lifecycle events from unreal in that case 
	 * This should generally not be invoked more than once per service (ie more than once at present)
	**/
	UFUNCTION(BlueprintCallable, Category = "Echo3D|Lifecycle", meta=(WorldContext="WorldContextObject"))
	static void BeginInit(const UObject *WorldContextObject);

	//calls all queued after init tasks the first time
	/**
	 * indicates that this echo service has finished being set up. Generally we only want to call RequestHolograms after this is invoked, unless we're doing so as part of the configuration process
	 * as otherwise other scripts can't easily know that the main configuration has completed.
	**/
	UFUNCTION(BlueprintCallable, Category = "Echo3D|Lifecycle", meta=(WorldContext="WorldContextObject"))
	static void FinishInit(const UObject *WorldContextObject);
	
	//#TODO_MAKE_INSTANCE_METHOD
	//TODO: perhaps force a non-linear delay even if ready immediately so control flow shenanigans are caught and the same in all use cases
	/**
	 * executes callback during finishInit or immediately if we've already called finishInit
	 * used to ensure timing of multiple scripts entering beginplay are syncronized to any setup on their echoservice instance
	 * 
	 * This will probably persist in some form as an instance method on an instance of the echo service in order to allow things to defer behaviors until a given echo service is properly configured
	**/
	UFUNCTION(BlueprintCallable, Category = "Echo3D|Lifecycle",  meta=(WorldContext="WorldContextObject"))
	static void QueueTaskAfterInit(const UObject *WorldContextObject, const FEchoCallback &afterInit);
	
	//#TODO_MAKE_INSTANCE_METHOD
	//TODO: this should probably either return the template and not store it in the output config or just store it in the output config and return nothing
	/**
	 * used internally to generate the value of FEchoImportConfig used to process holograms.
	 * public since might be needed elsewhere, but mostly of interest if doing things outside the normal workflow, such as doing some local file loading etc manually
	**/
	UFUNCTION(BlueprintCallable, Category = "Echo3D|Resolver", meta=(WorldContext="WorldContextObject"))
	static const UEchoHologramBaseTemplate *ResolveTemplateAndCreateConfig(
		UObject *WorldContextObject, const FEchoConnection &usingConnection, const FEchoHologramInfo &hologramInfo,
		const UEchoHologramBaseTemplate *hologramTemplate,
		FEchoImportConfig &outputConfig
	);


///Echo provided default behaviors:
	/**
	 * processes a raw query result into a bunch of hologram data (basically a list of them)
	**/
	UFUNCTION(BlueprintCallable, Category = "Echo3D|Helpers")
	static FEchoHologramQueryResult ParseEchoHologramQuery(const FEchoRawQueryResult &rawQuery)
	{
		return ParseEchoHologramQuery_Impl(rawQuery.debugInfo,  rawQuery.bSuccess, rawQuery.GetContentAsString());
	}

	/**
	 * default handler for requestholograms
	**/
	UFUNCTION(BlueprintCallable, Category = "Echo3D|Helpers", meta=(WorldContext="WorldContextObject"))
	static void DefaultHologramHandler(UObject *WorldContextObject, const FEchoConnection &connection, const UEchoHologramBaseTemplate *hologramTemplate, const FEchoHologramQueryResult &result);

	/**
	 * processes an array of holograms
	**/
	UFUNCTION(BlueprintCallable, Category = "Echo3D|Helpers", meta=(WorldContext="WorldContextObject"))
	static void ProcessAllHolograms(UObject *WorldContextObject, const FEchoConnection &connection, const UEchoHologramBaseTemplate *hologramTemplate, const TArray<FEchoHologramInfo> &holograms, UClass *constructClass);
	
	/**
	 * processes one hologram with the default behavior. probably deprecated and should use templates or treat this a helper for the default behavior
	**/
	UFUNCTION(BlueprintCallable, Category = "Echo3D|Helpers", meta=(WorldContext="WorldContextObject"))
	static void ProcessOneHologram(UObject *WorldContextObject, const FEchoConnection &connection, const UEchoHologramBaseTemplate *hologramTemplate, const FEchoHologramInfo &hologram, UObject *useExisting, UClass *constructClass);


	/**
	 * applies the default echo metadata behaviors
	**/
	UFUNCTION(BlueprintCallable, Category = "Echo3D|TemplateHelpers")
	static void HandleDefaultMetaData(AActor *actor, const FEchoHologramInfo &hologram);

	/**
	 * helper to construct an actor for actor templates. might be moved out of this class soon
	**/
	UFUNCTION(BlueprintCallable, Category = "Echo3D|TemplateHelpers")
	static FEchoConstructActorResult ConstructBaseActor(
		const FEchoImportConfig &userConfig,
		const FString &setLabel, 
		TSubclassOf<AActor> spawnClassOverride, 
		TSubclassOf<UActorComponent> addUserClass, //this can apparently also hold a null value so dont need a pointer to it
		AActor *useExistingActor
	);


	/**
	 * sets the verbose mode. this mainly prints more query logs
	**/
	UFUNCTION(BlueprintCallable, Category = "Echo3D|Debug")
	static void SetVerboseMode(bool bSetVerboseMode)
	{
		if (bVerboseMode != bSetVerboseMode)
		{
			UE_LOG(LogTemp, Warning, TEXT("Setting Echo Verbose Mode to %s"), (bSetVerboseMode ? TEXT("true"):TEXT("false")));
		}
		bVerboseMode = bSetVerboseMode;
	}

	/////////////////////////////////////////////////////////////////////////

	/** query the current verbose mode **/
	UFUNCTION(BlueprintCallable, Category = "Echo3D|Debug")
	static bool GetVerboseMode()
	{
		return bVerboseMode;
	}

	/**
	 * pass true to enable more detailed logging messages for queries. useful if something is going wrong with the http requests 
	**/
	UFUNCTION(BlueprintCallable, Category = "Echo3D|Debug")
	static void SetLogFailedQueries(bool bSetLogFailedQueries)
	{
		logFailedQueries = true;
	}
	
	/**
	 * gets the last set log queries flag
	**/
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
	 * This is mostly for internal development. The default value shoudl be true until something calls SetUseDevelopmentEndpoint 
	**/
	UFUNCTION(BlueprintPure, Category = "Echo3D|Debug")
	static bool GetUseDevelopmentEndpoint();

	/**
	 * turns on/off hiding for a bunch of warnings that we probably dont care about like unhandled duplicate properties etc.
	 * default is on.
	 * 
	 * This is mostly to keep things intelligible by default and make actual errors more obvious
	 * 
	 * meant to be set via Echo3DService::SetHideDefaultUnwantedWarnings
	**/
	UFUNCTION(BlueprintCallable, Category = "Echo3D|Debug")
	static void SetHideDefaultUnwantedWarnings(bool setHideDefaultUnwantedWarnings);
	
	/**
	 * get last set hideunwantedwarnings value
	**/
	UFUNCTION(BlueprintCallable, Category = "Echo3D|Debug")
	static bool GetHideDefaultUnwantedWarnings()
	{
		return hideDefaultUnwantedWarnings;
	}

	/**
	 * prints more verbose template information
	**/
	UFUNCTION(BlueprintCallable, Category = "Echo3D|Debug")
	static void SetDebugTemplateOperations(bool bSetDebugTemplateOperations)
	{
		bDebugTemplateOperations = bSetDebugTemplateOperations;
	}
	
	/**
	 * get the current more verbose template setting
	**/
	UFUNCTION(BlueprintCallable, Category = "Echo3D|Debug")
	static bool GetDebugTemplateOperations()
	{
		return bDebugTemplateOperations;
	}

protected:
	friend class UEchoCallbackNode; //HACK

	/**
	 * the ONE function that directly does web requests to the echo service. meant fo internal use
	**/
	static FEchoRequestResultInfo DoEchoRequest(const FString &requestURL, const FEchoRequestHandler &callback);
	
	UFUNCTION(BlueprintCallable, Category = "Echo3D|Assets", meta=(DisplayName="RequestAsset"))
	static void RequestAssetBP(const FEchoConnection &connection, const FEchoFile &storage, EEchoAssetType expectedType, bool bAllowCache, const FEchoMemoryAssetCallbackBP &callback);

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
	
	/**
	 * Requests holograms using the specified templateKey. This key is resolved after the holograms have been retrieved from echo.
	 * this might change eventually to grab the initial key during this call.
	 * 
	 * Please note that nothing will be available immediately in the execution arrow leading off of this node as the http request will take some time.
	**/
	UFUNCTION(BlueprintCallable, Category = "Echo3D", meta=(AutoCreateRefTerm="callback, includeEntries, includeTags", WorldContext="WorldContextObject"), DisplayName="RequestHolograms")
	static void RequestHolograms_BP(UObject *WorldContextObject, const FEchoConnection &connection, const FString &blueprintKey, const TArray<FString> &includeEntries, const TArray<FString> &includeTags,  const FEchoHologramQueryHandler &callback)
	{
		RequestHolograms(WorldContextObject, connection, blueprintKey, includeEntries, includeTags, callback);
	}

	/**
	 * Requests holograms using the specified template instance. 
	 * We will still call ResolveTemplate later on so this can still be used to select a more specific template based on the actual hologram data
	 *
	 * This variation is probably most useful if you need to specifically configure the template for this request with some one-off data that you don't want to register for other requests
	 *
	 * Please note that nothing will be available immediately in the execution arrow leading off of this node as the http request will take some time.
	**/
	UFUNCTION(BlueprintCallable, Category = "Echo3D", meta=(AutoCreateRefTerm="callback, includeEntries, includeTags", WorldContext="WorldContextObject"), DisplayName="RequestHologramsTemplate")
	static void RequestHologramsTemplate_BP(UObject *WorldContextObject, const FEchoConnection &connection, const UEchoHologramBaseTemplate *usingTemplate, const TArray<FString> &includeEntries, const TArray<FString> &includeTags,  const FEchoHologramQueryHandler &callback)
	{
		RequestHologramsTemplate(WorldContextObject, connection, usingTemplate, includeEntries, includeTags, callback);
	}
	
	/**
	 * Requests holograms using the specified template class. (We'll acquire an instance of it and use that) 
	 * We will still call ResolveTemplate later on so this can still be used to select a more specific template based on the actual hologram data
	 *
	 * This variation is probably most useful if you want to use a specific template class without registering it but don't need to modify the template instance
	 *
	 * Please note that nothing will be available immediately in the execution arrow leading off of this node as the http request will take some time.
	**/
	UFUNCTION(BlueprintCallable, Category = "Echo3D", meta=(AutoCreateRefTerm="callback, includeEntries, includeTags", WorldContext="WorldContextObject"), DisplayName="RequestHologramsTemplateClass")
	static void RequestHologramsTemplateClass_BP(UObject *WorldContextObject, const FEchoConnection &connection, TSubclassOf<UEchoHologramBaseTemplate> usingTemplateClass, const TArray<FString> &includeEntries, const TArray<FString> &includeTags,  const FEchoHologramQueryHandler &callback)
	{
		RequestHologramsTemplateClass(WorldContextObject, connection, usingTemplateClass, includeEntries, includeTags, callback);
	}

private:
	//NOTE: a lot of this statefulness will probably be moved into some combination of echo3dService instances (with an easily accessible global object) alongside some common stuff being kept in a subsystem
	/** check if WCO is different/old WCO expired and trigger staticInit **/
	static void CheckForNewRun(const UObject *WorldContextObject);
	
	/** resets some static state **/
	static void OnStaticReset();

	/** resets a bunch of state for a new PIE session **/
	static void ResetState();

	/** helpers for init **/
	enum class EWhichRequestHologramsCase : uint8
	{
		UseKey = 0,
		UseInstance = 1,
		UseClass = 2,
	};

	/** basically a union holding different styles of template passing to requestHolograms**/
	struct RequestHologramsArgHelper
	{
		EWhichRequestHologramsCase whichCase;
		FString viaKey; //NOTE: "late bound" - resolved after we have the holograms from HTTP
		TWeakObjectPtr<const UEchoHologramBaseTemplate> viaInstance;
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

	/**
	 * internal implementation of setdefaulttemplate, handles error cases
	**/
	static void SetDefaultTemplateImpl(const UEchoHologramBaseTemplate *setDefaultTemplate, bool bNullExpected);

	/**
	 * internal parts echo hologram query helper
	**/
	static FEchoHologramQueryResult ParseEchoHologramQuery_Impl(const FEchoQueryDebugInfo &querySource, bool isQueryOk, const FString &queryResponse);

private:
	//TODO: most of this statefulness should eventually move to either an instance of our service OR a subsystem	
	static bool bVerboseMode;
	static bool logFailedQueries;
	static bool bDebugTemplateOperations;
	static bool hideDefaultUnwantedWarnings;
	//static bool verboseQueries; //maybe rename verbosemode to this?

	static TArray<FEchoCallback> queuedPostInitTasks;
	static TWeakObjectPtr<UWorld> lastPlayWorld;

	static bool bInitCalled;
	static TMap<FString, TStrongObjectPtr<const UEchoHologramBaseTemplate>> templates;
	static TStrongObjectPtr<const UEchoHologramBaseTemplate> defaultTemplate;

	
};
