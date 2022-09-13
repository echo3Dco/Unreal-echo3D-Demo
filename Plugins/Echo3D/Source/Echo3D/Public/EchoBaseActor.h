// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EchoBaseActor.generated.h"

UCLASS()
class ECHO3D_API AEchoBaseActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AEchoBaseActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	//underscore so it renders at the start of the frame:

	UPROPERTY(EditAnywhere, Category = "Echo3D|_Source")
	FString hologramId;

	UPROPERTY(EditAnywhere, Category = "Echo3D|_Source")
	FString filename;

	UPROPERTY(EditAnywhere, Category = "Echo3D|_Source")
	FString storageId;

	//FEchoImportConfig importConfig;//unsafe
	UPROPERTY(EditAnywhere, Category = "Echo3D|_Source|ImportConfig")
	FEchoHologramInfo hologram;
	
	UPROPERTY(EditAnywhere, Category = "Echo3D|_Source|ImportConfig")
	FString importTitle;

	//this crashes and burns us
	UPROPERTY(EditAnywhere, Category = "Echo3D|_Source|ImportConfig")
	const UEchoHologramBaseTemplate *hologramTemplate;
	//TWeakObjectPtr<const UEchoHologramBaseTemplate> hologramTemplate;

	UPROPERTY(EditAnywhere, Category = "Echo3D|_Source|MeshConfig")
	FEchoCustomMeshImportSettings importSettings;



	UPROPERTY(EditAnywhere, Category = "Echo3D|Inspector")
	TArray<USceneComponent*> InspectableComponents;
	
	UPROPERTY(EditAnywhere, Category = "Echo3D|Inspector|Verbose")
	TArray<UTexture2D*> InspectableTextures;
	

	//TODO: try making an inspectable tree setup for inspection??

	//UPROPERTY(VisibleAnywhere, Category = "Echo3D|Inspector")
	//HACK - editing transform causes crashes
	//TArray<UActorComponent*> InspectableComponents;
	

	//UPROPERTY(EditAnywhere, Category = "Echo3D|Inspector")
	//UActorComponent *HardReferenceHack;

	

};
