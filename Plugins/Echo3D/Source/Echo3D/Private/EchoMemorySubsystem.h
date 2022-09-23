// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Array.h"
#include "Containers/Map.h"
#include "UObject/StrongObjectPtr.h"

#include "EchoMemorySubsystem.generated.h"

class UEchoMemorySubsystem;

/**
 * a copyable scoped guard
**/
struct FEchoMemoryGuard
{
	FEchoMemoryGuard(UObject *WorldContextObject, const UObject *pin);

	//rule of 5
	~FEchoMemoryGuard() noexcept;
	FEchoMemoryGuard(const FEchoMemoryGuard &copyFrom) noexcept;
	FEchoMemoryGuard &operator=(const FEchoMemoryGuard &src);
	
    FEchoMemoryGuard(FEchoMemoryGuard &&) noexcept;
    FEchoMemoryGuard& operator=(FEchoMemoryGuard &&) noexcept;
   
	bool IsStale() const
	{
		return subsystem.IsStale();
	}
private:
	FEchoMemoryGuard(UEchoMemorySubsystem *setSubsystem, const UObject *pin);
	FEchoMemoryGuard();

	//helpers
	UEchoMemorySubsystem *CheckSubsystem() const;
	bool CheckMatch(const UObject *pin, TWeakObjectPtr<UEchoMemorySubsystem> setSubsystem, bool bPrimary) const;
	void Store(const UObject *pin, TWeakObjectPtr<UEchoMemorySubsystem> setSubsystem, bool bPrimary);
	void AfterMoved();
	void Release();
	
	const UObject *obj;
	TWeakObjectPtr<UEchoMemorySubsystem> subsystem;
};

/**
 * meant to help us keep objects alive while lambdas are executing. please note that we'll release everything if the game instance ends so you still need to check the references
 * to avoid crashing if PIE ends etc
 * 
 * Very prototype-ish last minute solve
 * I'm wondering if i should have just kept using TStrongObjectPtr on the stack/passed to a lambda
 */
UCLASS()
class UEchoMemorySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/**
	 * destroy subsystem. this will release all of our strong references as the game session has ended
	**/
    virtual void Deinitialize() override;

	/**
	 * adds a strong reference to the object if we don't have one and increments the counter associated with that strong reference
	**/
	void AddReference(const UObject *pinObject);

	/**
	 * deincrements a counter associated with the reference and if its now zero (ie, we've called RemoveReference exactly as many times as AddReference, we remove out strong reference to it)
	**/
	void RemoveReference(const UObject *relObject);

	static UEchoMemorySubsystem *ResolveFromWCO(const UObject *WorldContextObject);
	
	//TODO: expand upon this in a later version!
private:

	//TODO: provide some kind of our memory system strong/weak wrapper?
	struct TrackedObject
	{
		int32 counter;
		const UObject *rawPointer; //in case we want this later for some reason - NOT SAFE TO DEREFERENCE - use the strong reference
		TStrongObjectPtr<const UObject> reference;

		TrackedObject(const UObject *forObject)
		: counter( 0 )
		, rawPointer( forObject )
		, reference( forObject )
		{
			if (forObject == nullptr)
			{
				counter = 0;
				reference.Reset();
				UE_LOG(LogTemp, Error, TEXT("TrackedObject: tried to track nullptr!"));
				return;
			}
			counter = 1;
		}
	};

	TMap<const UObject*, TrackedObject> referenceMap;
	TArray<const UObject*> deterministicKeys;
};
