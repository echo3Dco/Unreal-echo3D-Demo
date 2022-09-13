#include "EchoCallbackNode.h"

#include "Echo3DService.h"
#include "EchoHelperLib.h"

//This (and the related header) include some sample code from the following:
//based on https://nerivec.github.io/old-ue4-wiki/pages/creating-asynchronous-blueprint-nodes.html
//based on https://www.tomlooman.com/unreal-engine-async-blueprint-http-json/
//but have been modified

//TODO: some kind of echo connection class?
//TODO: do we need to release some kind of global gameobject thingy? that might only have been in the minitimer code though.
//TODO: keep checking that this is this reentrant

//TODO: either nuke these nodes OR make them work via EchoService Request helper thingy so only one piece of code does requests?

UEchoCallbackNode::UEchoCallbackNode(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
 , WorldContextObject(nullptr)
 , URL(), bExpectedCallback( false ), callback( )
{
}

UEchoCallbackNode* UEchoCallbackNode::EchoTestRequest(const UObject* WorldContextObject, const FString &url)
{
	UEchoCallbackNode* Node = NewObject<UEchoCallbackNode>();
	Node->WorldContextObject = WorldContextObject;
	Node->URL = url;
	return Node;
}

UEchoCallbackNode* UEchoCallbackNode::EchoRequestAll(const UObject* WorldContextObject, const FEchoConnection &fromConnection)
{
	UEchoCallbackNode* Node = NewObject<UEchoCallbackNode>();
	Node->WorldContextObject = WorldContextObject;
	Node->URL = EchoHelperLib::GenerateEchoDatabaseRequestURL(fromConnection);
	return Node;
}

UEchoCallbackNode* UEchoCallbackNode::EchoRequestStorage(const UObject* WorldContextObject, const FEchoConnection &fromConnection, const FString &storageId)
{
	UEchoCallbackNode* Node = NewObject<UEchoCallbackNode>();
	Node->WorldContextObject = WorldContextObject;
	//Node->URL = EchoHelperLib::GenerateEchoStorageRequestURL(fromConnection, storageId);
	Node->URL = EchoHelperLib::GenerateEchoStorageRequestURL(fromConnection, FEchoFile(storageId, storageId));
	return Node;
}

UEchoCallbackNode* UEchoCallbackNode::EchoRequestStorageAndCallback(const UObject* WorldContextObject, const FEchoConnection &fromConnection, const FString &storageId, const FEchoDownloadCallback &setCallback)
{
	UEchoCallbackNode* Node = NewObject<UEchoCallbackNode>();
	Node->WorldContextObject = WorldContextObject;
	//Node->URL = EchoHelperLib::GenerateEchoStorageRequestURL(fromConnection, storageId);
	Node->URL = EchoHelperLib::GenerateEchoStorageRequestURL(fromConnection, FEchoFile(storageId, storageId));
	Node->callback = setCallback;
	Node->bExpectedCallback = setCallback.IsBound();
	return Node;
}

void UEchoCallbackNode::Activate()
{
	if (nullptr == WorldContextObject)
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid WorldContextObject. Cannot execute EchoCallbackNode."), ELogVerbosity::Error);
		return;
	}
	
	TWeakObjectPtr<UEchoCallbackNode> weakThis = this;
	FEchoRequestHandler callbackWrapper;
	callbackWrapper.BindLambda([weakThis](const FEchoRawQueryResult &result)
	{
		if (weakThis.IsStale())
		{
			UE_LOG(LogTemp, Warning, TEXT("UEchoCallbackNode::Activate:[lambda]: this node was deleted!"));
			return;
		}
		UEchoCallbackNode *thisLater = nullptr;
		if (weakThis.IsValid())
		{
			thisLater = weakThis.Get();
		}
		if (thisLater == nullptr)
		{
			//maybe verbose?
			UE_LOG(LogTemp, Warning, TEXT("UEchoCallbackNode::Activate:[lambda]: callback was never bound!"));
			return;
		}

		if (thisLater->callback.IsBound())
		{
			thisLater->callback.Execute(result.bSuccess, result.contentBlob);	
		}

		//TODO: (future paranoia): double check if this is safe if unbound multicast?
		thisLater->OnResult.Broadcast(result);
	});
	AEcho3DService::DoEchoRequest(URL, callbackWrapper);
}
