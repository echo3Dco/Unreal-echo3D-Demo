#include "EchoMeshService.h"
//#include "CustomMeshComponent.h"
#include "Echo3DService.h"

//hack
#include "Components/BrushComponent.h"




//TSharedRef<FDestructorInvocationTester> myDesu
//It was recommended to me to use MakeShared?
//but...
//output was:
//LogTemp: Error: In Lambda
//LogTemp: Error: AFTER TestFunc()
//LogTemp: Error: ~DESU
//LogTemp: Error: After Test()
//TSharedRef<FDestructorInvocationTester> desuRef = MakeShared<FDestructorInvocationTester>( new FDestructorInvocationTester() );
	
int FDestructorInvocationTesterCtorCount = 0;
int FDestructorInvocationTesterMoveCount = 0;
int FDestructorInvocationTesterDtorCount = 0;

struct FDestructorInvocationTester
{
	FDestructorInvocationTester()
	{
		FDestructorInvocationTesterCtorCount++;
	}
	
	FDestructorInvocationTester(const FDestructorInvocationTester &copyFrom)
	{
		FDestructorInvocationTesterCtorCount++;
	}
	//used by TArray
	//FDestructorInvocationTester(FDestructorInvocationTester &&move) = delete;

	
	FDestructorInvocationTester(FDestructorInvocationTester &&move)
	{
		FDestructorInvocationTesterMoveCount++;	
	}
	
	~FDestructorInvocationTester()
	{
		FDestructorInvocationTesterDtorCount++;	
		UE_LOG(LogTemp, Error, TEXT("~FDestructorInvocationTester"));
	}
};

DECLARE_DELEGATE(FDestructorInvocationTesterFunc);
//using FMySharedValue = FDestructorInvocationTester;
//using FMySharedPtr = TSharedPtr<FMySharedValue>;

using FMySharedValue = TArray<FDestructorInvocationTester>;
using FMySharedPtr = TSharedPtr<FMySharedValue>;
using FMySharedRef = TSharedRef<FMySharedValue>;

//TSharedPtr<FDestructorInvocationTester> myDesu;
FMySharedPtr myDesu;
FDestructorInvocationTesterFunc TestHelper()
{
	//FMySharedPtr desuRef( new FMySharedValue() );
	FMySharedRef desuRef( new FMySharedValue() );
	desuRef.Get().Push(FDestructorInvocationTester());
	UE_LOG(LogTemp, Error, TEXT("==after push=="));

	//desuRef.ToSharedRef
	//FMySharedPtr desuRef( MakeShareable( new FMySharedValue() ) );
	int refCount;
	refCount = desuRef.GetSharedReferenceCount();
	UE_LOG(LogTemp, Error, TEXT("after new: %d"), refCount);
	FDestructorInvocationTesterFunc func;
	myDesu = desuRef;
	refCount = desuRef.GetSharedReferenceCount();
	UE_LOG(LogTemp, Error, TEXT("after set myDesu %d"), refCount);
	func.BindLambda([desuRef]()
	{
		int refCount = desuRef.GetSharedReferenceCount();
		UE_LOG(LogTemp, Error, TEXT("In Lambda: %d"), refCount);
	});
	refCount = desuRef.GetSharedReferenceCount();
	UE_LOG(LogTemp, Error, TEXT("after bind lambda: %d"), refCount);
	
	func.ExecuteIfBound();
	return func;
}

//void Test()
void Echo_TestDestructorsOnLambdas()
{
	FDestructorInvocationTesterFunc func = TestHelper();
	int refCount;
	refCount = myDesu.GetSharedReferenceCount();
	UE_LOG(LogTemp, Error, TEXT("AFTER Testunc(): %d"), refCount);

	FMySharedRef callbackRef = myDesu.ToSharedRef();
	refCount = myDesu.GetSharedReferenceCount();
	UE_LOG(LogTemp, Error, TEXT("AFTER toSharedRef: %d"), refCount);

	FEchoMemoryAssetCallback callbackHelper;
	callbackHelper.BindLambda([callbackRef](const FEchoMemoryAsset &){
		//func = nullptr;
		int refCount = callbackRef.GetSharedReferenceCount();
		UE_LOG(LogTemp, Error, TEXT("in callbackFunc: %d"), refCount);
	});

	refCount = myDesu.GetSharedReferenceCount();
	UE_LOG(LogTemp, Error, TEXT("AFTER bindLambda: %d"), refCount);

	AEcho3DService::RequestAsset(FEchoConnection(), FEchoFile(), EEchoAssetType::EEchoAsset_Unknown, false, callbackHelper);
	
	func = nullptr;
	refCount = myDesu.GetSharedReferenceCount();
	UE_LOG(LogTemp, Error, TEXT("AFTER clear func: %d"), refCount);

	refCount = myDesu.GetSharedReferenceCount();
	UE_LOG(LogTemp, Error, TEXT("AFTER Testunc(): %d"), refCount);
	//func = nullptr;
	//refCount = myDesu.GetSharedReferenceCount();
	//UE_LOG(LogTemp, Error, TEXT("AFTER clear func: %d"), refCount);
	myDesu = nullptr;//TSharedRef<FDestructorInvocationTester>();
	UE_LOG(LogTemp, Error, TEXT("AFTER null myDesu"));
	UE_LOG(LogTemp, Error, TEXT("EXIT Test(): FDestructorInvocationTester: ctor: %d, ~: %d, mov: %d"), FDestructorInvocationTesterCtorCount, FDestructorInvocationTesterDtorCount, FDestructorInvocationTesterMoveCount);
}

/*
//Actual Output: (note the last print is from right after invoking Test())
LogTemp: Error: after new: 1
LogTemp: Error: after set myDesu 2
LogTemp: Error: after bind lambda: 3
LogTemp: Error: In Lambda: 3
LogTemp: Error: AFTER Testunc(): 2
LogTemp: Error: AFTER clear func: 1
LogTemp: Error: ~DESU
LogTemp: Error: AFTER null myDesu
LogTemp: Error: EXIT Test()
LogTemp: Error: After Test()
*/

//void UEchoMeshService::AttachMeshFromAsset(AActor *actor, const FEchoConnection &connection, const FEchoMemoryAsset &asset, UClass *spawnClass)

//Runs some sanity tests on reference counting. I wanted to make sure passing an FSharedRef holding a heap-allocated object around including capturing into a lambda didn't slowly leak memory
// this appears to invoke the destructor properly for FDestructorInvocationTester.
void RunSomeRandomTestsOnlyOnce()
{

	static bool bTestOnce = true;
	if (bTestOnce)
	{
		bTestOnce = false;
		//Test();//hack
		Echo_TestDestructorsOnLambdas();
		UE_LOG(LogTemp, Error, TEXT("After Test()"));
	}

}