// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EchoBaseActor.generated.h"

/**
 * mostly just an actor, but with some easily inspectable properties for in-editor debugging. 
**/
UCLASS()
class ECHO3D_API AEchoBaseActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AEchoBaseActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, Category = "Echo3D|_Source")
	FString hologramId;

	UPROPERTY(EditAnywhere, Category = "Echo3D|_Source")
	FString filename;

	UPROPERTY(EditAnywhere, Category = "Echo3D|_Source")
	FString storageId;

	//TODO: return to an importInfo now that it shouldn't crash use?
	UPROPERTY(EditAnywhere, Category = "Echo3D|_Source|ImportConfig")
	FEchoHologramInfo hologram;

	UPROPERTY(EditAnywhere, Category = "Echo3D|_Source|ImportConfig")
	FString importTitle;

	UPROPERTY(EditAnywhere, Category = "Echo3D|_Source|ImportConfig")
	const UEchoHologramBaseTemplate *hologramTemplate;
	
	UPROPERTY(EditAnywhere, Category = "Echo3D|_Source|MeshConfig")
	FEchoCustomMeshImportSettings importSettings;

	//NB: if these are not declared as USceneComponent then Unreal Editor crashes and burns if you try to edit the transform in the details window for one of these
	UPROPERTY(EditAnywhere, Category = "Echo3D|Inspector")
	TArray<USceneComponent*> InspectableComponents;
	
	UPROPERTY(EditAnywhere, Category = "Echo3D|Inspector|Verbose")
	TArray<UTexture2D*> InspectableTextures;

};
