// Fill out your copyright notice in the Description page of Project Settings.


#include "EchoMemorySubsystem.h"
#pragma optimize( "", off )

namespace EchoMemSysCpp
{
	constexpr const bool debugMemory = true;
};
using namespace EchoMemSysCpp;

//FEchoMemoryGuard::FEchoMemoryGuard(UEchoMemorySubsystem *setSubsystem)
FEchoMemoryGuard::FEchoMemoryGuard()
 : obj( nullptr )
 , subsystem( nullptr )
{
	
}

FEchoMemoryGuard::FEchoMemoryGuard(UEchoMemorySubsystem *setSubsystem, const UObject *pin)
 : FEchoMemoryGuard() //FEchoMemoryGuard(setSubsystem)
{
	/*
	subsystem = setSubsystem;
	if (setSubsystem == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("FEchoMemoryGuard::FEchoMemoryGuard: null setSubsystem!"));
	}
	*/
	const bool bPrimary = true;
	//Store(pin, subsystem, bPrimary);
	Store(pin, setSubsystem, bPrimary);
}

FEchoMemoryGuard::FEchoMemoryGuard(UObject *WorldContextObject, const UObject *pin)
 : FEchoMemoryGuard( UEchoMemorySubsystem::ResolveFromWCO(WorldContextObject), pin)
{
}

//rule of 5
FEchoMemoryGuard::FEchoMemoryGuard(FEchoMemoryGuard &&moveFrom) noexcept
 : FEchoMemoryGuard()//moveFrom.subsystem.Get())//moveFrom.CheckSubsystem())
{
	const bool bPrimary = false;
	//bool bCheck = CheckMatch(moveFrom.obj, moveFrom.subsystem, bPrimary);
	//bCheck = bCheck && (&moveFrom != this); //paranoia
	//if (bCheck)
	{
		Store(moveFrom.obj, moveFrom.subsystem, bPrimary);
		bool bNotSelf = (&moveFrom != this);
		if (bNotSelf)
		{
			moveFrom.AfterMoved();
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("FEchoMemoryGuard: Tried to move onto self!"));
		}
	}
}

FEchoMemoryGuard::FEchoMemoryGuard(const FEchoMemoryGuard &copyFrom) noexcept
 : FEchoMemoryGuard()//copyFrom.CheckSubsystem())
{
	const bool bPrimary = false;
	Store(copyFrom.obj, copyFrom.subsystem, bPrimary);
}

FEchoMemoryGuard::~FEchoMemoryGuard() noexcept
{
	
	if (obj != nullptr) //if we have a live reference
	{
		UEchoMemorySubsystem *subsys = CheckSubsystem();
		if (subsys != nullptr)
		{
			subsys->RemoveReference(obj);
		}
		else
		{
			if (debugMemory)
			{
				UE_LOG(LogTemp, Error, TEXT("~FEchoMemoryGuard: null subsystem with non-null guarded object - did our subsystem go stale?"));
			}
		}
	}

	//this should ALWAYS be nulled out
	obj = nullptr;
}

FEchoMemoryGuard &FEchoMemoryGuard::operator=(const FEchoMemoryGuard &copyFrom)
{
	const bool bPrimary = false;
	if (&copyFrom != this)
	{
		Release();
		/*
		if (copyFrom.IsStale())
		{
			Store(nullptr);
			subsystem = nullptr;
			return *this;
		}
		*/
		Store(copyFrom.obj, copyFrom.subsystem, bPrimary);
	}
	return *this;
}
	
FEchoMemoryGuard& FEchoMemoryGuard::operator=(FEchoMemoryGuard &&moveFrom) noexcept
{
	const bool bPrimary = false;
	if (&moveFrom != this)
	{
		//Release();
		/*
		if (moveFrom.IsStale())
		{
			//dont store, but do free moved:
			if (debugMemory)
			{
				UE_LOG(LogTemp, Error, TEXT("FEchoMemoryGuard::operator=(FEchoMemoryGuard &&): stale source"));
			}
			//Store(nullptr);
			//subsystem = nullptr;
			moveFrom.AfterMoved();
			return *this;
		}
		*/
		//subsystem = moveFrom.subsystem.Get();
		Store(moveFrom.obj, moveFrom.subsystem, bPrimary);
		moveFrom.AfterMoved();
	}
	return *this;
}

UEchoMemorySubsystem *FEchoMemoryGuard::CheckSubsystem() const
{
	if (subsystem.IsStale())
	{
		UE_LOG(LogTemp, Error, TEXT("FEchoMemoryGuard: subsystem is stale!"));
		return nullptr;
	}
	
	UEchoMemorySubsystem *subsys = subsystem.Get();
	if (subsys == nullptr)
	{
		if (debugMemory)
		{
			UE_LOG(LogTemp, Error, TEXT("FEchoMemoryGuard: subsys is nullptr!"));
		}
		return nullptr;
	}
	//should be okay to use it!
	return subsys;
}

void FEchoMemoryGuard::AfterMoved()
{
	//explicitly NOT released - we no longer own the guard
	obj = nullptr;
	subsystem = nullptr; //moved
}

//void FEchoMemoryGuard::Store(const UObject *pin)
//void FEchoMemoryGuard::Store(const FEchoMemoryGuard &src)
//void FEchoMemoryGuard::Store(const UObject *pin, UEchoMemorySubsystem *setSubsystem)
void FEchoMemoryGuard::Release()
{
	if (obj != nullptr)
	{
		UEchoMemorySubsystem *subsys = CheckSubsystem();
		if (subsys != nullptr)
		{				
			subsys->RemoveReference(obj);
		}
		else
		{
			if (debugMemory)
			{
				UE_LOG(LogTemp, Error, TEXT("FEchoMemoryGuard::Release: null subsystem"));
			}
		}
	}

	obj = nullptr;
	subsystem = nullptr;
}

bool FEchoMemoryGuard::CheckMatch(const UObject *pin, TWeakObjectPtr<UEchoMemorySubsystem> setSubsystem, bool bPrimary) const
{
	if ((setSubsystem == this->subsystem) && (pin == obj))
	{
		if (bPrimary)
		{
			if (bPrimary && ((setSubsystem != nullptr) || (pin != nullptr)))
			{	
				//sanity check should not occur - primary should only match if non-null
				UE_LOG(LogTemp, Error, TEXT("FEchoMemoryGuard::Store: Assertion Error!"));
			}
			if (setSubsystem.IsStale())
			{
				UE_LOG(LogTemp, Error, TEXT("FEchoMemoryGuard::Store: Primary Store but stale setSubsystem!"));
			}
		}
		return true;
	}
	return false;
}
//bPrimary - if true means that this is a original store from calling code not some copy or move (we want to flag certain thing during the primary assignment but not later on
void FEchoMemoryGuard::Store(const UObject *pin, TWeakObjectPtr<UEchoMemorySubsystem> setSubsystem, bool bPrimary)
{	
	if (CheckMatch(pin, setSubsystem, bPrimary))
	{
		//warnings handled in checkmatch
		return;
	}
	
	Release();
	
	bool bStale = (setSubsystem.IsStale());
	bool bNull = (setSubsystem == nullptr);

	if (bNull || bStale)
	{
		if (debugMemory || bPrimary)
		{
			if (bStale)
			{
				UE_LOG(LogTemp, Error, TEXT("FEchoMemoryGuard::Store: subsystem is stale!"));
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("FEchoMemoryGuard::Store: subsystem is nullptr!"));
			}
		}
		subsystem = nullptr;
		obj = nullptr;
		return;
	}

	subsystem = setSubsystem;
	obj = pin;

	if (obj != nullptr)
	{
		UEchoMemorySubsystem *subsys = CheckSubsystem();
		if (subsys != nullptr)
		{
			subsys->AddReference(obj);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("FEchoMemoryGuard::Store: null subsystem!"));
		}
	}
}

///////////////////////////////////////////////////////////////////////////

void UEchoMemorySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UEchoMemorySubsystem::Deinitialize()
{
	for(const UObject *key: deterministicKeys)
	{
		TrackedObject *trObj = referenceMap.Find(key);
		if (trObj->rawPointer != nullptr)
		{
			referenceMap.Remove(trObj->rawPointer);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Assertion Failed: rawPointer was null for TrackedObject?"));
		}
		trObj->rawPointer = nullptr;
		trObj->counter = 0;
		trObj->reference.Reset();
	}
	deterministicKeys.Reset();
	if (referenceMap.Num() > 0)
	{
		UE_LOG(LogTemp, Error, TEXT("ASSERTION FAILED: referenceMap not empty after clearing keys list!"));
	}
	referenceMap.Reset();
}

void UEchoMemorySubsystem::AddReference(const UObject *pinObject)
{
	if (pinObject == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("UEchoMemorySubsystem::AddReference: can't track nullptr!"));
		return;
	}
	//TODO: ensure tmap hashing is safe on GC'd objects? maybe map a map of void* to stuffs?
	//TODO: do we really want to store this in a trivially relocatable thing??
	TrackedObject *trObj = referenceMap.Find(pinObject);
	if (trObj == nullptr) //not found
	{
		referenceMap.Add(pinObject, TrackedObject(pinObject));
		deterministicKeys.Add(pinObject);
	}
	else
	{
		//TODO: maybe do some sanity checking here?
		trObj->counter++; //addreference
		//if (trObje
		//trObj->reference = pinObject;
	}
}

void UEchoMemorySubsystem::RemoveReference(const UObject *relObject)
{
	if (relObject == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("RemoveReference: nullptr passed in"));
		return;
	}
	TrackedObject *trObj = referenceMap.Find(relObject);
	if (trObj == nullptr)
	{
		//this might actually be our negative increment case
		UE_LOG(LogTemp, Error, TEXT("RemoveReference: trackedobject not found for pointer: %p"), (void*)relObject);
	}
	else
	{
		trObj->counter--;
		if (trObj->counter <= 0)
		{
			if (trObj->counter<0)
			{
				UE_LOG(LogTemp, Error, TEXT("Too many RemoveReference calls on object! trObj->counter < 0: %d pointer: %p"), trObj->counter, (void*)relObject);
			}
			trObj->reference.Reset();//release reference - probably would happen with removal from map but I'm paranoid
			deterministicKeys.Remove(relObject);
			referenceMap.Remove(relObject);
			//relObject is now no longer strongly referenced here. if nothing is using it, it might get GC'd
			// but that should not happen until after the game thread returns to engine control so stuff is probably safe for this part of this frame
		}
	}
}

//static
UEchoMemorySubsystem *UEchoMemorySubsystem::ResolveFromWCO(const UObject *WCO)
{
	UEchoMemorySubsystem *subsys = nullptr;
	if (WCO != nullptr)
	{
		UWorld *forWorld = WCO->GetWorld();
		if (forWorld != nullptr)
		{
			UGameInstance *gameInst = forWorld->GetGameInstance();
			if (gameInst != nullptr)
			{
				subsys = gameInst->GetSubsystem<UEchoMemorySubsystem>();
				if (subsys == nullptr)
				{
					UE_LOG(LogTemp, Error, TEXT("UEchoMemorySubsystem::ResolveFromWCO: null subsystem from gameInst!"));
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("UEchoMemorySubsystem::ResolveFromWCO: null gameInst from WCO's world"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("UEchoMemorySubsystem::ResolveFromWCO: null world from WCO"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("UEchoMemorySubsystem::ResolveFromWCO: null WCO"));
	}
	return subsys;
}
/*
FEchoMemoryGuard UEchoMemorySubsystem::CreateGuard(UObject *WCO, const UObject *pinObject)
{
	UEchoMemorySubsystem *subsys = ResolveFromWCO(WCO);
	if (subsys == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("CreateGuard: null subsys found"));
	}
	return FEchoMemoryGuard(subsys, pinObject);
}
*/

#pragma optimize( "", on )