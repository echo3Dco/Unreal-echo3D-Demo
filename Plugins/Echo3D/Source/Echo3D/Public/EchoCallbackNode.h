#pragma once
#include "Kismet/BlueprintAsyncActionBase.h"
#include "EchoConnection.h"
#include "EchoCallbackNode.generated.h"

/**
 * probably deprecated node class to represent delayed actions.
**/
UCLASS()
class ECHO3D_API UEchoCallbackNode : public UBlueprintAsyncActionBase
{
	GENERATED_UCLASS_BODY()

public:
	//DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEchoCallbackNodeOutputPin, bool, success, const FString &, responseString);
	
	DECLARE_DYNAMIC_DELEGATE_TwoParams(FEchoDownloadCallback, bool, success, const TArray<uint8> &, responseBlob);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEchoCallbackNodeOutputPinV2, const FEchoRawQueryResult &, result);
	
	UPROPERTY(BlueprintAssignable)
	FEchoCallbackNodeOutputPinV2 OnResult;

	//Query any url, mostly for testing, esp to get bad responses/etc
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "Deprecated_Echo3D")
	static UEchoCallbackNode* EchoTestRequest(const UObject* WorldContextObject, const FString &url);

	//Query "all" from connection.
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "Deprecated_Echo3D")
	static UEchoCallbackNode* EchoRequestAll(const UObject* WorldContextObject, const FEchoConnection &connection);

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "Deprecated_Echo3D")
	static UEchoCallbackNode* EchoRequestStorage(const UObject* WorldContextObject, const FEchoConnection &connection, const FString &storageId);

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "Deprecated_Echo3D")
	static UEchoCallbackNode* EchoRequestStorageAndCallback(const UObject* WorldContextObject, const FEchoConnection &connection, const FString &storageId, const FEchoDownloadCallback &callback);

	// UBlueprintAsyncActionBase interface
	virtual void Activate() override;
	//~UBlueprintAsyncActionBase interface

	
private:
	const UObject* WorldContextObject;
	
	FString URL;
	bool bExpectedCallback;
	FEchoDownloadCallback callback;
};