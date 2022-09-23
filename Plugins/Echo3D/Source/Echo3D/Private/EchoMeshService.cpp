// Fill out your copyright notice in the Description page of Project Settings.

#include "EchoMeshService.h"
#include "Echo3DService.h"

//Custom Components
//#include "CustomMeshComponent.h"
#include "ProceduralMeshComponent.h"

#include "Util/EchoStopwatch.h"
//#include "Components/StaticMeshComponent.h"
//#include "Engine/StaticMesh.h"


#include "Materials/MaterialInstanceDynamic.h"

#include <vector>
#include <stdio.h>
#include <string>
#include <stdlib.h>

#include "TechTests/ObjMeshUtil.h"
#include "EchoImporter.h"

//#include "EchoActorBase.h"
#include "EchoBaseActor.h"
//#include "../../Echo3DAssImp/Public/EchoImporter.h"

#pragma optimize( "", off )
//probably nuke this - duplicate "GameObject" names seems to be a fatal error in unreal!? - or append some kind of number?
const FName Name_EchoMeshComponent(TEXT("EchoMesh"));

//TODO: make better and put somewhere better?
//#define ECHO_FALLTHROUGH [[fallThough]]
#define ECHO_FALLTHROUGH [[fallThough]]

const bool bDebugForceLocalFile = false;//NORMAL
//const bool bDebugForceLocalFile = true; //HACK

//turning this on will break some stuff ugh
//const bool DoRenames = false;//true; //turning this on will break some stuff ugh
const bool DoRenames = true; 

const bool hackShowUnknownMapsAsEmissive = false;
//const bool hackShowUnknownMapsAsEmissive = true; //debug land - looks like roughness/metallic in some order?'

//sequence of meshvariants to be handled. right now its just vertex colors
//this should be incremented if we update meshvariants with new properties that need to be handled
const int32 MeshFormatVersion = UEchoMeshService::MeshVariantFormatVersion_01; //should always match one of these constants - todo: how to keep these in sync for blueprint?
const FEchoMeshVariants varDefaultVariant = {false, MeshFormatVersion};
const FEchoMeshVariants varVertexColors = {true, MeshFormatVersion};

//TODO: provide a way to resolve these
struct EchoMeshhResultNode
{
	//NB: these will probably be the same
	EchoMeshhResultNode *parentNode;
	USceneComponent *transformComponent;
	UProceduralMeshComponent *meshComponent;
	FString nodeName;
	int numberedParentIndex;

	EchoMeshhResultNode()
	: parentNode( nullptr ), transformComponent( nullptr ), meshComponent( nullptr)
	{
	}

	EchoMeshhResultNode(UProceduralMeshComponent *setMesh)
	: parentNode( nullptr ), transformComponent( setMesh ), meshComponent( setMesh )
	{
	}

	//TODO: try casting to procedural mesh or some mesh for fun?
	EchoMeshhResultNode(USceneComponent *setTransform)
	: parentNode( nullptr ), transformComponent( setTransform), meshComponent( nullptr)
	{
	}

	void SetRelativeTransform(const FTransform &transform)
	{
		if (transformComponent != nullptr)
		{
			transformComponent->SetRelativeTransform(transform);
		}
	}
};



struct FEchoOneMaterial
{
	//materials choices that vary based on input model NOT on input material
	//UMaterialInstanceDynamic *baseMat;
	//UMaterialInstanceDynamic *matVertexColors;
	
	UMaterialInterface *baseMat;
	UMaterialInterface *matVertexColors;
};

struct FEchoParsedMaterials
{
	TArray<FEchoOneMaterial> materials;
};



namespace EchoMeshServiceInternal
{
	//TODO: bind shading models to different default material templates??
	//TODO: probably ignore for defaults and certain cases
	//const TMap<FString, bool> ignorePropsDictionary = {
	const TMap<FName, bool> ignorePropsDictionary = {
		{ TEXT("$mat.shininess"), true },
		{ TEXT("$mat.opacity"), true }, 
		{ TEXT("$mat.gltf.alphaCutoff"), true }, 
		{ TEXT("$clr.base"), true }, 
	};
	/*
	
	LogTemp: Error: UNKNOWN (To Importer) Texture Type: 18
	LogTemp: Error: Unhandled Scalar Name: $mat.shininess
	LogTemp: Error: Unhandled Scalar Name: $mat.opacity
	LogTemp: Error: Unhandled Scalar Name: $mat.gltf.alphaCutoff
	LogTemp: Error: Unhandled Vector Name: $clr.base
	*/

	//bool shouldIgnoreProperty(const FString &propName)
	bool shouldIgnoreProperty(const FName &propName)
	{
		if (!UEchoMeshService::GetHideDefaultUnwantedWarnings())
		{
			return false;
		}
		/*if (prop == nullptr)
		{
			return false; //something went wrong if asking about nullptr
		}*/
		//FString propName = aiString2FString(prop->mKey);
		//if (ignorePropsDictionary.find(propName) != ignorePropsDictionary.end())
		if (ignorePropsDictionary.Contains(propName))// != ignorePropsDictionary.end())
		{
			return true;
		}
		return false;
	}
};
using namespace EchoMeshServiceInternal;


USceneComponent *CreateTransformComponent(USceneComponent *outer)
{
	return NewObject<USceneComponent>(outer);
}

UProceduralMeshComponent *CreateEmptyMeshComponent(USceneComponent *outer)
{
	return NewObject<UProceduralMeshComponent>(outer);
}

//TODO: figure out how to deal with tangent space conversion stuff?
FVector ConvertObjCoordToUnreal(const FVector &objSpace)
{
	const float MX = objSpace.X;
	const float MY = objSpace.Y;
	const float MZ = objSpace.Z;
	return FVector(
		
		//wrong again //+MY, +MX, +MZ//v2
		//+MY, +MZ, +MX//v3
		//+MZ, +MY, +MX //v4 - swapped v3.x, v3.y
		+MZ, +MX, +MY //v5 - swapped v4.y, v3.z
	);
	/*
	//v1
	return FVector(
		+objSpace.Z,
		-objSpace.X,
		+objSpace.Y
	);
	*/
	/*
	float vxPrime, vyPrime, vzPrime; //in unreal coordinate system
	vxPrime = +vz; //is this correct???
	vyPrime = -vx;
	vzPrime = +vy;
	*/
	//ret->verts.push_back(FVector(vxPrime, vyPrime, vzPrime) * customConfig.uniformScale);
}


//void CreateMeshNodeHelper2(const FString &meshLabel, EchoMeshhResultNode &result, int setParentIndex, const FNodeData &node, int meshIndex, const FEchoCustomMeshImportSettings &meshImportSettings, TArray<UMaterialInterface*> matInstances)
void CreateMeshNodeHelper2(const FString &meshLabel, EchoMeshhResultNode &result, int setParentIndex, const FNodeData &node, int meshIndex, const FEchoCustomMeshImportSettings &meshImportSettings, const FEchoParsedMaterials &materials)
{
	//TODO: should we transform stuff here or in the root?
	
	//TODO: can GC happen inside a frame?
	//setParentIndex from node but replaceable?
	//TODO: set error flag true if fail somewhere?
	result.numberedParentIndex = setParentIndex;
	//result.numMeshes?//or destroy?
	if ((meshIndex<0)||(meshIndex >= node.Meshes.Num()))
	{
		UE_LOG(LogTemp, Error, TEXT("MeshFile: %s: CreateMeshNodeHelper2: invalid meshIndex: %d"), *meshLabel, meshIndex);
		return;
	}
	//TODO: should we create all mesh sections in one proc mesh?
	const FMeshData &meshData = node.Meshes[meshIndex];
	
	UProceduralMeshComponent *meshComp = result.meshComponent;
	if (meshComp != nullptr)
	{
		//meshData.
		bool bCreateCollision = false; //maybe kater?
		const TArray<FLinearColor> EmptyMeshColors; //unused atm

		//const bool bFixAxes = true;
		//if (bFixAxes)
		const bool bCustomDirs = (meshImportSettings.bFixAxes);
		const bool bCustomVerts = bCustomDirs || (meshImportSettings.uniformScale != 1);
		const bool bCustomTris = (meshImportSettings.bInvertWinding);
		//if (bCustomDirs || bCustomVerts || bCustomTris)
		{
			/*
			TArray<FVector> verts(meshData.Vertices);
			TArray<FVector> normals(meshData.Normals);
			TArray<FProcMeshTangent> tangents(meshData.Tangents);
			*/
			//we might need to transform these things to import into unreal:
			TArray<FVector> verts;//(meshData.Vertices);
			TArray<FVector> normals;//(meshData.Normals);
			TArray<FProcMeshTangent> tangents;//(meshData.Tangents);
			TArray<int32> triangles;
			
			const TArray<FVector> &useVerts = bCustomVerts ? verts : meshData.Vertices;
			const TArray<FVector> &useNormals = bCustomDirs ? normals : meshData.Normals;
			const TArray<FProcMeshTangent> &useTangents = bCustomDirs? tangents : meshData.Tangents;
			const TArray<int32> &useTriangles = bCustomTris ? triangles : meshData.Triangles;
			//bool bHaveCustomColors = meshData.Colors.Num() > 0);
			//const TArray<FLinearColor> &useColors = bCustomColors? triangles : meshData.Triangles;
			const TArray<FLinearColor> &useColors = meshData.Colors;//hack////bHaveCustomColors? meshData.Colors : EmptyMeshColors;
			//EmptyMeshColors
			//TArray<FVector> normsl(meshData.Normals);
			/*
			TArray<FVector> verts(meshData.Vertices);
			TArray<FVector> normals(meshData.Normals);
			TArray<FProcMeshTangent> tangents(meshData.Tangents);
			*/
			if (bCustomVerts)
			{
				verts.Append(meshData.Vertices);
				int32 i;
				int32 len = verts.Num();
				if (meshImportSettings.bFixAxes)
				{
					for(i=0; i<len; i++)
					{
						verts[i] = ConvertObjCoordToUnreal(verts[i]) * meshImportSettings.uniformScale;
					}
				}
				else
				{
					//skips fix axes
					for(i=0; i<len; i++)
					{
						verts[i] = verts[i] * meshImportSettings.uniformScale;
					}
				}
			}
				//FVector v = verts[i];
				//v = ConvertObjCoordToUnreal(v) * meshImportSettings.uniformScale;
				//verts[i] = v;
			//}
			if (bCustomDirs)
			{
				normals.Append(meshData.Normals);
				tangents.Append(meshData.Tangents);

				int32 i;
				int32 len = verts.Num();
				
				len=normals.Num();
				for(i=0; i<len; i++)
				{
					normals[i] = ConvertObjCoordToUnreal(normals[i]);
				}
				len=tangents.Num();
				for(i=0; i<len; i++)
				{
					//Q: what about y part? TODO: check tangents make sense?
					auto &tangent = tangents[i];
					tangent.TangentX = ConvertObjCoordToUnreal(tangent.TangentX);
				}
				//tangents.Reset(); //HACK
			}
			//TArray<int32>
			//const TArray<int32> &useTriangles =
			//if 
			//const TArray<int32> &useTriangles = meshImportSettings.bInvertWinding ? triangles : meshData.Triangles;
			//if (meshImportSettings.bInvertWinding)
			if (bCustomTris)
			{
				//lazy work
				triangles.Append(meshData.Triangles);
				int32 triCount = triangles.Num();
				for(int32 iTriangle=0; iTriangle+2<triCount; iTriangle+=3)
				{
					//swap [+1] and [+2] to invert winding
					//0 1 2 => 0 2 1
					int32 temp;
					int32 &vi1 = triangles[iTriangle+1];
					int32 &vi2 = triangles[iTriangle+2];
					temp = vi1;
					vi1 = vi2;
					vi2 = temp;
				}
			}
			//meshComp->CreateMeshSection_LinearColor(meshIndex, verts, meshData.Triangles, normals, meshData.UVs, EmptyMeshColors, tangents, bCreateCollision);
			//meshComp->CreateMeshSection_LinearColor(meshIndex, verts, useTriangles, normals, meshData.UVs, EmptyMeshColors, tangents, bCreateCollision);
			//meshComp->CreateMeshSection_LinearColor(meshIndex, useVerts, useTriangles, useNormals, meshData.UVs, EmptyMeshColors, useTangents, bCreateCollision);

			//TODO: possibly branch on if hasvertexcolors?
			if ((!node.NodeName.IsEmpty())||(!meshData.SubmeshName.IsEmpty()))
			{
				if (DoRenames)
				{
					//MakeUniqueObjectName
					FName uniqueName = FName((node.NodeName + TEXT("_")+meshData.SubmeshName));
					//uniqueName = MakeUniqueObjectName(meshComp->GetAttachmentRootActor(), meshComp->GetClass(), uniqueName);
					uniqueName = MakeUniqueObjectName(meshComp->GetWorld(), meshComp->GetClass(), uniqueName);
					//meshComp->Rename(*(node.NodeName + TEXT("_")+meshData.SubmeshName));
					meshComp->Rename(*(uniqueName.ToString()));
				}
			}
			//meshComp->CreateMeshSection_LinearColor(meshIndex, useVerts, useTriangles, useNormals, meshData.UVs, useColors, useTangents, bCreateCollision);
			meshComp->CreateMeshSection_LinearColor(meshIndex, useVerts, useTriangles, useNormals, meshData.UVs0, meshData.UVs1, meshData.UVs2, meshData.UVs3, useColors, useTangents, bCreateCollision);
			//UMaterialInstanceDynamic *matInstance = nullptr;
			//UMaterialInterface *matInstance = nullptr;

			//TODO: provide a custom material resolution callback?
			const FEchoOneMaterial *usingMatGroup = nullptr;
			int whichMaterial = meshData.UseMaterialIndex;
			if ((whichMaterial < 0)||(whichMaterial >=materials.materials.Num()))
			{
				//TODO:
				UE_LOG(LogTemp, Error, TEXT("MeshFile: %s: material index out of bounds: %d, max=%d"), *meshLabel, whichMaterial, materials.materials.Num());
			}
			else
			{
				usingMatGroup = &materials.materials[whichMaterial];
				//matInstance = matInstances[whichMaterial];
				/*
				if (matInstance == nullptr)
				{
					UE_LOG(LogTemp, Error, TEXT("MeshFile: %s, matInstances[%d] was null!"), *meshLabel, whichMaterial);
				}
				*/
			}

			//f (matInstance != nullptr)
			if (usingMatGroup != nullptr)
			{
				UMaterialInterface *matInterface = nullptr;
				bool bHaveVertColors = (useColors.Num() > 0);
				matInterface = bHaveVertColors ? usingMatGroup->matVertexColors : usingMatGroup->baseMat;
				//TODO: should we fall back to non-vertex color case and retry?
				if (UEchoMeshService::GetMeshServiceDebugPrintMaterialInfo())
				{
					if (matInterface == nullptr)
					{
						UE_LOG(LogTemp, Error, TEXT("mat interface null! which: [%d][vertexColors? %s]"), whichMaterial, *StringUtil::BoolToString(bHaveVertColors));
					}
				}
				meshComp->SetMaterial(0, matInterface);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("MeshFile %s: no material group found for [%d]"), *meshLabel, whichMaterial);
			}
		}
		/*
		else
		{
			//meshComp->CreateMeshSection_LinearColor(meshIndex, meshData.Vertices, meshData.Triangles, meshData.Normals, meshData.UVs, EmptyMeshColors, meshData.Tangents, bCreateCollision);
			meshComp->CreateMeshSection_LinearColor(meshIndex, meshData.Vertices, meshData.Triangles, meshData.Normals, meshData.UVs, EmptyMeshColors, meshData.Tangents, bCreateCollision);
		}
		*/
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("NULL MESHCOMP!"));
	}
}

//void CreateMeshNodeHelper(const FString &meshLabel, TArray<EchoMeshhResultNode> &baseNodes, const FNodeData &node, USceneComponent *rootOuter, const FEchoCustomMeshImportSettings &meshImportSettings, TArray<UMaterialInterface*> matInstances)
void CreateMeshNodeHelper(const FString &meshLabel, TArray<EchoMeshhResultNode> &baseNodes, const FNodeData &node, USceneComponent *rootOuter, const FEchoCustomMeshImportSettings &meshImportSettings, const FEchoParsedMaterials &materials)
{
	//TArray<EchoMeshhResultNode> &extraNodes, //not used atm
	/*
	EchoMeshhResultNode newResult;
	newResult.meshComponent = nullptr;
	newResult.transformComponent = nullptr;
	*/
	EchoMeshhResultNode baseNode;
	int numMeshes = node.Meshes.Num();
	if (numMeshes < 0)
	{
		//FAIL//UE_LOG(LogError, Temp, TEXT("Error: numMeshes < 0: %d"), numMeshes);
		UE_LOG(LogTemp, Error, TEXT("Error: numMeshes < 0: %d"), numMeshes);
		numMeshes = 0; //force transform only node - prevent potential bad parenting bugs later on
		//continues in empty node case
	}
	int parentIndex = node.NodeParentIndex;
	//NOT ELSE IF
	if (numMeshes == 0)
	{
		//transform only node
		baseNode = EchoMeshhResultNode(CreateTransformComponent(rootOuter));
		if (!node.NodeName.IsEmpty())
		{
			if (DoRenames)
			{
				FName uniqueName(node.NodeName);
				//uniqueName = MakeUniqueObjectName(baseNode.transformComponent->GetAttachmentRootActor(), baseNode.transformComponent->GetClass(), uniqueName);
				uniqueName = MakeUniqueObjectName(baseNode.transformComponent->GetWorld(), baseNode.transformComponent->GetClass(), uniqueName);
				baseNode.transformComponent->Rename(*uniqueName.ToString());
				//baseNode.transformComponent->Rename(*node.NodeName);
			}
		}
		//newResult.transformComponent = CreateTransformComponent();
		//output.push_back(EchoMeshhResultNode(
	}
	else if (numMeshes > 0)
	{
		baseNode = EchoMeshhResultNode(CreateEmptyMeshComponent(rootOuter));//use mesh 0 as base
		for(int i=0; i<numMeshes; i++)
		{
			//CreateMeshNodeHelper2(meshLabel, baseNode, parentIndex, node, i, meshImportSettings, matInstances);
			CreateMeshNodeHelper2(meshLabel, baseNode, parentIndex, node, i, meshImportSettings, materials);
		}
		
	}

	//baseNode.SetRelativeTransform(node.RelativeTransformTransform);
	FTransform transform = node.RelativeTransformTransform;
	//transform.SetLocation( transform.GetLocation() * uniformScale );
	transform.SetLocation( transform.GetLocation() * meshImportSettings.uniformScale );
	baseNode.SetRelativeTransform(transform);
	//baseNode.numberedParentIndex = numberedIndex;
	baseNode.numberedParentIndex = parentIndex;//node.NodeParentIndex;
	//output.Add(baseNode);
	baseNodes.Add(baseNode);
}

//NOTE: connection param because it might be still needed if have to download unexpected dependent assets like a texture only specified in an FBX or OBJ/MTL or something
//or some other random reason


//TODO: move this to some common util lib thingy
void CreateRootComponentHelper(AActor *actor, TSubclassOf<USceneComponent> *customClass)
{
	if (actor != nullptr)
	{
		USceneComponent *sceneBase = actor->GetRootComponent();
		if (sceneBase == nullptr)
		{
			UClass *usingCustomClass = nullptr;
			if (customClass != nullptr)
			{
				usingCustomClass = customClass->Get();
			}

			if (usingCustomClass == nullptr)
			{
				usingCustomClass = USceneComponent::StaticClass();
			}
			sceneBase = NewObject<USceneComponent>(actor, usingCustomClass);
			sceneBase->SetupAttachment(actor->GetRootComponent());
			sceneBase->RegisterComponent();
			actor->SetRootComponent(sceneBase);
		}
	}
}

//bool ParseAllMaterials(FEchoParsedMaterials &matResults, AActor *owner, const FEchoImportConfig &importConfig, const FEchoActorTemplateResolution &actorConfig, const FFinalReturnData &importedData)
bool ParseAllMaterials(
	const FEchoImportConfig &importConfig, 
	const FEchoImportMeshConfig &meshConfig,
	//FEchoParsedMaterials &matResults, AActor *owner, const FEchoImportConfig &importConfig, const FEchoActorTemplateResolution &actorConfig, const FFinalReturnData &importedData
	const FFinalReturnData &importedData,
	FEchoParsedMaterials &results
	//AActor *owner, const FEchoImportConfig &importConfig, const FEchoActorTemplateResolution &actorConfig
)
{
	bool allOk = true;
	//TODO: capture list of used materials + variants from nodes first - so we can generate only what is needed, not a full graph

	
	if (importedData.Materials.Num() < 1)
	{
		UE_LOG(LogTemp, Error, TEXT("No materials parsed in mesh asset!"));
		allOk = false;
	}
	else
	{
		if (UEchoMeshService::GetMeshServiceDebugPrintMaterialInfo())
		{
			UE_LOG(LogTemp, Warning, TEXT("MeshFile: %s: Found: %d materials from importer"), *importConfig.importTitle, importedData.Materials.Num());
		}
		
		
		bool bIgnoreMissingMatWarnings = false;
		{
			//temporary check while this is not per instance for less spam:
			const UEchoMaterialBaseTemplate *templateForMaterials = meshConfig.materialTemplate;
			if (templateForMaterials == nullptr)
			{
				if (UEchoMeshService::GetDefaultMaterial() == nullptr)
				{
				
				
					if (importConfig.hologramTemplate == nullptr)
					{
						bIgnoreMissingMatWarnings = true;
						UE_LOG(LogTemp, Error, TEXT("No hologram template and no default material found importing %s"), *importConfig.importTitle);
					}
					else
					{
						//TODO: this subcase will eventually be nuked and handled in the for loop
						bIgnoreMissingMatWarnings = true;
						FString hologramTemplateName = (importConfig.hologramTemplate != nullptr) ? importConfig.hologramTemplate->GetTemplateDebugString() : EchoStringConstants::NullString;
						//UE_LOG(LogTemp, Error, TEXT("No material template and no default material found importing %s with template %s, meshMaterial: %s"), *importConfig.importTitle, *hologramTemplateName, *matDef.MaterialName.ToString());
						UE_LOG(LogTemp, Error, TEXT("No material template and no default material found importing %s with template %s"), *importConfig.importTitle, *hologramTemplateName);
					}
				}
			}
		}

		const bool bIgnoreMissingMatWarningsOuter = bIgnoreMissingMatWarnings; //capture before start of for loop
		for(auto &matDef: importedData.Materials)
		{
			//bool bIgnoreMissingMatWarnings = false;
			
			FEchoOneMaterial oneMat;
			oneMat.baseMat = nullptr;
			oneMat.matVertexColors = nullptr;

			UMaterialInterface *myMat = nullptr;
			//most of the objects should already be held by templates/strongobjects of templates?
				
			//if (importConfig.hologramTemplate != nullptr)
			
			//const UEchoImportTemplate *templateForMaterials = actorConfig.hologramTemplate.Get(); //HORRIBLE HACK but eval mat lives in hologram template ... for now
			//const UEchoMaterialBaseTemplate *templateForMaterials = actorConfig.hologramTemplate.Get(); //HORRIBLE HACK but eval mat lives in hologram template ... for now
			const UEchoMaterialBaseTemplate *templateForMaterials = meshConfig.materialTemplate;
			/*
			if (meshConfig.materialTemplate.IsStale())
			{
				
			}*/
			//if (actorConfig.hologramTemplate != nullptr)
			bool bIgnoreMissingMatWarningsThisTime = bIgnoreMissingMatWarningsOuter;
			UObject *usingWCO = importConfig.WorldContextObject.Get();
			if (importConfig.IsStale() || (usingWCO == nullptr))
			{
				UE_LOG(LogTemp, Error, TEXT("WCO is null or stale"));
			}
			if (templateForMaterials != nullptr)
			{
				oneMat.baseMat = templateForMaterials->EvalMaterial(importConfig, meshConfig, importedData, matDef, varDefaultVariant);
				oneMat.matVertexColors = templateForMaterials->EvalMaterial(importConfig, meshConfig, importedData, matDef, varVertexColors);
				//oneMat.baseMat = templateForMaterials->EvalMaterial(owner, importConfig, actorConfig, importedData, matDef, varDefaultVariant);
				//oneMat.matVertexColors = templateForMaterials->EvalMaterial(owner, importConfig, actorConfig, importedData, matDef, varVertexColors);
			}
			else
			{
				/*
				//if (importConfig.hologramTemplate.IsStale())
				if (actorConfig.hologramTemplate.IsStale())
				{
					UE_LOG(LogTemp, Error, TEXT("hologram template was stale: importing '%s' : mat '%s'"), *importConfig.importTitle, *matDef.MaterialName.ToString());
				}
				*/

				//note: materialtemplate might not be a constant per iteration later on
				if (UEchoMeshService::GetDefaultMaterial() == nullptr)
				{
					if (!bIgnoreMissingMatWarnings)
					{
						FString hologramTemplateName = (importConfig.hologramTemplate != nullptr) ? importConfig.hologramTemplate->GetTemplateDebugString() : EchoStringConstants::NullString;
						UE_LOG(LogTemp, Error, TEXT("No material template and no default material found importing %s with template %s, meshMaterial: %s"), *importConfig.importTitle, *hologramTemplateName, *matDef.MaterialName.ToString());
					}
					oneMat.baseMat = nullptr;
					oneMat.matVertexColors = nullptr;
					bIgnoreMissingMatWarnings = true;//consolidating errors
					bIgnoreMissingMatWarningsThisTime = true;
				}
				else
				{
					oneMat.baseMat = UEchoMeshService::ParseOneMaterialDefault(usingWCO, importConfig.importTitle, importedData, matDef, varDefaultVariant);
					oneMat.matVertexColors = UEchoMeshService::ParseOneMaterialDefault(usingWCO, importConfig.importTitle, importedData, matDef, varVertexColors);
				}
				//oneMat.baseMat = UEchoMeshService::ParseOneMaterialDefault(owner, importConfig, actorConfig, importedData, matDef, varDefaultVariant);
				//oneMat.matVertexColors = UEchoMeshService::ParseOneMaterialDefault(owner, importConfig, actorConfig, importedData, matDef, varVertexColors);
			}
			//TODO: warn if any variation is nullptr???

			//TODO: make these only ignored for global ignore or per this specific mat
			if (oneMat.baseMat == nullptr)
			{
				if (!bIgnoreMissingMatWarningsThisTime)
				{
					//UE_LOG(LogTemp, Error, TEXT("baseMat result was nullptr: importing '%s' : mat '%s' : hasTemplate? %s, matTemplate: %s"), *importConfig.importTitle, *matDef.MaterialName.ToString(), *StringUtil::BoolToString(importConfig.hologramTemplate != nullptr), *((meshConfig.materialTemplate != nullptr) ? meshConfig.materialTemplate->GetTemplateName() : EchoStringConstants::NullString));
					UE_LOG(LogTemp, Error, TEXT("baseMat result was nullptr: importing '%s' : mat '%s' : hasTemplate? %s, matTemplate: %s"), 
						*importConfig.importTitle, 
						*StringUtil::BoolToString(importConfig.hologramTemplate != nullptr), 
						*((meshConfig.materialTemplate != nullptr) ? meshConfig.materialTemplate->GetTemplateName() : EchoStringConstants::NullString),
						*matDef.MaterialName.ToString()	
					);
				}
				allOk = false;
			}
			if (oneMat.matVertexColors == nullptr)
			{
				if (!bIgnoreMissingMatWarningsThisTime)
				{
					//UE_LOG(LogTemp, Error, TEXT("matVertexColors result was nullptr: importing '%s' : mat '%s' : hasTemplate? %s"), *importConfig.importTitle, *matDef.MaterialName.ToString(), *StringUtil::BoolToString(actorConfig.hologramTemplate != nullptr));
					UE_LOG(LogTemp, Error, TEXT("matVertexColors result was nullptr: importing '%s' : mat '%s' : hasTemplate? %s, matTemplate: %s"), 
						*importConfig.importTitle, 
						*StringUtil::BoolToString(importConfig.hologramTemplate != nullptr), 
						*((meshConfig.materialTemplate != nullptr) ? meshConfig.materialTemplate->GetTemplateName() : EchoStringConstants::NullString),
						*matDef.MaterialName.ToString()	
					);
				}
				allOk = false;
			}

			results.materials.Push(oneMat);
		}
	}
	
	return allOk;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//void UEchoMeshService::AttachMeshFromAsset(AActor *actor, const FEchoConnection &connection, const FEchoMemoryAsset &asset, UClass *spawnClass, const FEchoCustomMeshImportSettings *applyMeshImportSettings, const FEchoImportConfig &importConfig, const FEchoActorTemplateResolution &actorConfig)
//TODO: accept echomaterialtemplate and stuffs???
//TODO: does this actually want to resolve the material template later on???
void UEchoMeshService::AttachMeshFromAsset(
	const FEchoImportConfig &importConfig, 
	const FEchoImportMeshConfig &meshConfig,
	const FEchoMemoryAsset &asset
	/*
	const FEchoImportConfig &importConfig, 
	const FEchoMemoryAsset &asset,
	const FEchoCustomMeshImportSettings *applyMeshImportSettings, 
	const UEchoMaterialTemplate *materialTemplate,
	AActor *actor, 
	UClass *spawnClass,
	//const FEchoActorTemplateResolution &actorConfig
	*/
)
{
	//TODO: attach to USceneComponent to allow complex setup instead of forcing root component!

	//TODO_???: omit actor if we just want to spawn stuff later on?
	
	///////////////////////////
	// Preconditions
	
	const FString &meshTitle = importConfig.importTitle;//should be clearer  //asset.fileInfo.filename;
	const FString &meshStorage = asset.fileInfo.storageId;
	
	AActor *actor = meshConfig.actor.Get();
	if (meshConfig.actor.IsStale())
	{
		//UE_LOG(LogTemp, Error, TEXT("UEchoMeshService::AttachMeshFromAsset: Stale ACTOR GIVEN: %s"), *meshTitle);
		UE_LOG(LogTemp, Error, TEXT("UEchoMeshService::AttachMeshFromAsset: Stale ACTOR GIVEN: %s"), *meshTitle);
		return;
	}
	if (actor == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("NULL ACTOR GIVEN: %s"), *meshTitle);
		return;
	}

	UWorld *world = actor->GetWorld();
	if (world == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("NO WORLD!"));
		return;
	}

	AEchoBaseActor *echoActor = Cast<AEchoBaseActor>(actor);
	bool bAcceptedAssetType = false;
	bAcceptedAssetType = bAcceptedAssetType || (asset.assetType == EEchoAssetType::EEchoAsset_Mesh);
	bAcceptedAssetType = bAcceptedAssetType || (asset.assetType == EEchoAssetType::EEchoAsset_Unknown);
	if ((!asset.bHaveContent)||(asset.blob.Num()<1)||(!bAcceptedAssetType))
	{
		UE_LOG(LogTemp, Error, TEXT("BAD MESH ASSET: %s - storage=%s"), *meshTitle, *meshStorage);
		{
			switch(asset.assetDebugInfo.fromSource)
			{
				case EEchoAssetSource::EchoAssetSource_FromRequest:
					{
						UE_LOG(LogTemp, Error, TEXT("\tSource: Echo Request\n\t\tRequestId: " ECHO_FORMAT_ARG_UINT64 "\n\t\tRequest: %s"), asset.assetDebugInfo.sourceQuery.queryNumber, *asset.assetDebugInfo.sourceQuery.query);
					}
					break;

				case EEchoAssetSource::EchoAssetSource_FromAsset:
					//Not handled atm.
					ECHO_FALLTHROUGH
				default:
					{
						UE_LOG(LogTemp, Error, TEXT("\tSource: Unknown or unhandled: %d"), asset.assetDebugInfo.fromSource);
					}
					break;
			}
		}
		if (!asset.bHaveContent)
		{
			UE_LOG(LogTemp, Error, TEXT("\t*asset.bHaveContent is false"));
		}
		if (asset.blob.Num() < 1)
		{
			UE_LOG(LogTemp, Error, TEXT("\t*asset.blob.Num() < 1: %d"), asset.blob.Num());
		}
		if (!bAcceptedAssetType)
		{
			UE_LOG(LogTemp, Error, TEXT("\t*invalid mesh asset type: %d"), (int)asset.assetType);
		}
		
		return;
	}

	/////////////////////////////////////////////////////////////////////
	// Deal with root component issues
	USceneComponent *actorRoot = actor->GetRootComponent();
	if (actorRoot == nullptr)
	{
		//HACK
		CreateRootComponentHelper(actor, nullptr);
		actorRoot = actor->GetRootComponent();
		if (actorRoot == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("ACTOR ROOTCOMP IS NULL!"));
			return;
		}
	}
	
	UWorld *pWorld = actorRoot->GetWorld();
	if ((pWorld == nullptr)||(pWorld != world))
	{
		//Can this even happen?
		UE_LOG(LogTemp, Error, TEXT("BAD WORLD IN ROOT COMP! %s"), *meshTitle);
		return;
	}

	///////////////////////////////////////////////////////////////////////
	// Evauluate with AssImp

	FFinalReturnData data = UEchoImporter::LoadMeshFromBlob(asset.fileInfo.filename, asset.blob);

	TArray<EchoMeshhResultNode> baseNodes;
	if (data.Nodes.Num() < 1)
	{
		UE_LOG(LogTemp, Error, TEXT("Zero Nodes in MESH ASSET: %s"), *meshTitle);
		return;
	}


	///////////////////////////////////////////////////////////////////////
	// Prepare Materials and Textures
	
	if (echoActor != nullptr)
	{
		//show textures for debugging if importer verbose
		if (UEchoImporter::importerVerboseTextures)
		{
			for(const auto &texImport: data.Textures)
			{
				if (texImport.IsValid)
				{
					if (echoActor != nullptr)
					{
						echoActor->InspectableTextures.Push(texImport.texture);
					}
				}
			}
		}
	}
	
	
	FEchoParsedMaterials matResults;
	//or todo: only parse used materials?
	bool matOK;
	//matOK = ParseAllMaterials(matResults, data, meshTitle, actor, mat);
	//matOK = ParseAllMaterials(matResults, actor, importConfig, actorConfig, data);//, meshTitle, actor, mat);
	matOK = ParseAllMaterials(importConfig, meshConfig, data, matResults);//, meshTitle, actor, mat);
	if (!matOK)
	{
		//TODO: should we early out or not?
		UE_LOG(LogTemp, Error, TEXT("ParseAllMaterials: FAILED: %s"), *meshTitle);
	}
	
	///////////////////////////////////////////////////////////////////////
	// Deal with Meshes and Helper Transform nodes

	//const FEchoCustomMeshImportSettings &meshImportSettings = (applyMeshImportSettings != nullptr) ? *applyMeshImportSettings : defaultMeshSettings;
	//TODO: make a sane accessor for these cases?
	/*
	const UEchoImportTemplate *forMeshTemplate = importConfig.hologramTemplate;//actorConfig.hologramTemplate.Get();
	//if ((forMeshTemplate == nullptr) && actorConfig.hologramTemplate.IsStale())
	if (actorConfig.hologramTemplate.IsStale())
	{
		//or hologramConfig.meshTitle??
		UE_LOG(LogTemp, Error, TEXT("actorConfig's hologram template was stale (GC'd!): %s"), *meshTitle);
	}
	*/
	//const FEchoCustomMeshImportSettings &templateSettings = meshConfig.meshImportSettings;
	/*
	const FEchoCustomMeshImportSettings *meshImportSettingsPtr = nullptr;
	FEchoCustomMeshImportSettings templateSettings;
	{
		meshImportSettingsPtr = ((applyMeshImportSettings != nullptr) && (meshImportSettingsPtr == nullptr)) ? applyMeshImportSettings : meshImportSettingsPtr;
		if (forMeshTemplate != nullptr)
		{
			//const FEchoCustomMeshImportSettings *templateSettings = forMeshTemplate->GetImportSettings();
			if (forMeshTemplate->GetImportSettings(templateSettings))
			{
				meshImportSettingsPtr = ((true) && (meshImportSettingsPtr == nullptr)) ? &templateSettings : meshImportSettingsPtr;
			}
		}
		meshImportSettingsPtr = ((true) && (meshImportSettingsPtr == nullptr)) ? &defaultMeshSettings : meshImportSettingsPtr;
		//static_assert(meshImportSettingsPtr != nullptr, "somehow null");
	}

	if (meshImportSettingsPtr == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("AssembleMeshFromAsset: Assertion Failed: somehow didn't get a meshImportSettingsPtr!: %s"), *meshTitle);
		return; //avoid crashing
	}

	const FEchoCustomMeshImportSettings &meshImportSettings = *meshImportSettingsPtr;
	*/
	const FEchoCustomMeshImportSettings &meshImportSettings = meshConfig.meshImportSettings;

	for(int32 i=0; i<data.Nodes.Num(); i++)
	{
		//CreateMeshNodeHelper(meshTitle, baseNodes, data.Nodes[i], actorRoot, meshImportSettings, matInstances);
		CreateMeshNodeHelper(meshTitle, baseNodes, data.Nodes[i], actorRoot, meshImportSettings, matResults);
	}
	const int BaseParentIndex = -1;
	int debug_index=-1;
	for(EchoMeshhResultNode &nd: baseNodes)
	{
		debug_index++; //-1++: starts at 0

		if (nd.transformComponent == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("null transform!: %s::%d"), *meshTitle, debug_index);
			continue;
		}

		USceneComponent *parentComp = nullptr;
		if (nd.numberedParentIndex == BaseParentIndex)
		{
			//attach to actor
			parentComp = actorRoot;
		}
		else
		{
			int pi = nd.numberedParentIndex;
			if ((pi < 0)||(pi >= baseNodes.Num()))
			{
				//TODO: destroy node?
				UE_LOG(LogTemp, Error, TEXT("parentIndex out of bounds: : %s::%d: parentIndex=%d"), *meshTitle, debug_index, pi);
				continue;
			}
			
			//Q: what socket names actually for?
			
			parentComp = baseNodes[pi].transformComponent;
			if (parentComp == nullptr)
			{
				UE_LOG(LogTemp, Error, TEXT("Null parent transform!: %s::%d, parentIndex=%d"), *meshTitle, debug_index, pi);
				continue;
			}
		}
		
		//TODO: figure out attachtocomponent?
		bool bWeld = true;//TODO: figure out later
		FAttachmentTransformRules attachRules(EAttachmentRule::KeepRelative, bWeld);
		nd.transformComponent->AttachToComponent(parentComp, attachRules);
		
		//EPIC HACK For now, to allow inspecting stuff. TODO: maybe define some kind of interface?
		if (echoActor != nullptr)
		{
			echoActor->InspectableComponents.Push(nd.transformComponent); //HACK
		}
	}

	if (echoActor != nullptr)
	{
		bool bAllEmpty = (echoActor->filename.IsEmpty() && echoActor->storageId.IsEmpty() && echoActor->hologramId.IsEmpty());
		echoActor->filename = asset.fileInfo.filename;
		echoActor->storageId = asset.fileInfo.filename;
		if (bAllEmpty)
		{
			//not best place for this but...
			echoActor->hologramId = asset.assetDebugInfo.sourceQuery.query;//hack
		}
		echoActor->importSettings = meshConfig.meshImportSettings;
	}

	//TODO: name mesh nodes?
	//TODO: move this ALL to the helper functions?
	debug_index=-1;
	for(EchoMeshhResultNode &nd: baseNodes)
	{
		debug_index++; //-1++: starts at 0
		nd.transformComponent->RegisterComponent();
		
		if (nd.meshComponent != nullptr)
		{
			//TODO: allow multiple materials to be bound somehow? will it be useful in a unity-like manner??

			UProceduralMeshComponent *meshComp = nd.meshComponent;
			//now normaly done in CreateMeshNodeHelper2
			if (UEchoMeshService::GetMeshServiceDebugPrintMaterialInfo())
			{
				if (meshComp->GetMaterial(0) == nullptr)
				{
					UE_LOG(LogTemp, Error, TEXT("No material to bind for meshcomponent child!: %s"), *nd.nodeName);
				}
			}
			meshComp->MarkRenderStateDirty();
		}
	}

	//TODO: extra!	
}


void UEchoMeshService::AttachMeshFromStorage(
	const FEchoImportConfig &importConfig, 
	const FEchoImportMeshConfig &meshConfig,
	const FString &storageId
	//AActor *actor, const FEchoConnection &connection, const FString &storageId, UClass *spawnClass, const FEchoImportConfig &config, const FEchoActorTemplateResolution &actorConfig
)
{
	/*
	TWeakObjectPtr<AActor> weakActor = actor;
	//TStrongObjectPtr<UClass> x;
	//TSubclassOf<UClass> desu; //not useful for maintaing refeferences
	//const bool bHadSpawnClass
	//FEchoConnection connectionCopy = connection;

	TStrongObjectPtr<UClass> strongSpawnClass(spawnClass);
	*/

	FEchoFile requestFile;
	requestFile.filename = storageId; //HACK
	requestFile.storageId = storageId;

	
	/*
	FEchoImportConfig configCopy = config;
	const FEchoActorTemplateResolution actorConfigCopy = actorConfig;
	*/
	//TStrongObjectPtr<const FEchoImportConfig> importConfigCopy = importConfig;
	//TStrongObjectPtr<const FEchoImportMeshConfig> meshConfigCopy = meshConfig;

	auto pinImport = importConfig.Pin();
	auto pinMesh = meshConfig.Pin();
	//callback.BindLambda([weakActor, strongSpawnClass, connectionCopy, configCopy, actorConfigCopy](const FEchoMemoryAsset &meshAsset)
	
	FEchoMemoryAssetCallback callback;
	//callback.BindLambda([importConfigCopy, meshConfigCopy, pinImport, pinMesh](const FEchoMemoryAsset &meshAsset)
	callback.BindLambda([pinImport, pinMesh](const FEchoMemoryAsset &meshAsset)
	{
		FEchoImportConfig importConfigCopy;
		FEchoImportMeshConfig meshConfigCopy;
		bool ok = true;
		ok = pinImport.Get(importConfigCopy) && ok;
		ok = pinMesh.Get(meshConfigCopy) && ok;
		if (!ok)
		{
			//Fail here - might be able to continue but is probably incorrect if something was stale
			//importTitle should still be valid as a non-uobject so it can't be garbage collected
			UE_LOG(LogTemp, Error, TEXT("something was GC'd during requesting assets! importing: %s"), *importConfigCopy.importTitle);
			return;
		}

		//UEchoMeshService::AttachMeshFromAsset(weakActor.Get(), connectionCopy, meshAsset, strongSpawnClass.Get(), nullptr, configCopy, actorConfigCopy);
		UEchoMeshService::AttachMeshFromAsset(importConfigCopy, meshConfigCopy, meshAsset);
		//weakActor.Get(), connectionCopy, meshAsset, strongSpawnClass.Get(), nullptr, configCopy, actorConfigCopy);
	});

	//TODO: nuke this param for now?
	bool bAllowCache = true;
	//AEcho3DService::RequestAsset(connection, requestFile, EEchoAssetType::EEchoAsset_Mesh, bAllowCache, callback);
	AEcho3DService::RequestAsset(importConfig.connection, requestFile, EEchoAssetType::EEchoAsset_Mesh, bAllowCache, callback);
}

//formerly Assemble:
void UEchoMeshService::AttachMeshFromAssetArray(
	const FEchoImportConfig &importConfig, 
	const FEchoImportMeshConfig &meshConfig,
	const TArray<FEchoMemoryAsset> &assetList
)
{
	if (importConfig.importTitle.IsEmpty())
	{
		//sanity check
		UE_LOG(LogTemp, Error, TEXT("importConfig appears to have been trashed!"));
	}

	//TODO: list assets?
	//const FEchoMemoryAsset *meshAsset = nullptr;
	int found = 0;
	for(const auto &asset: assetList)
	{
		//UE_LOG(LogTemp, Log, TEXT("Asset: %s: ==%d, length=%d"), *asset.fileInfo.filename, (int32)asset.assetType, asset.blob.Num());
		if (asset.assetType == EEchoAssetType::EEchoAsset_Mesh)
		{
			//if (meshAsset != nullptr)
			if (found > 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("More than one EEchoAsset_Mesh found in results!"));
			}
			found++;
			//TODO: pass along other assets as well or some kind of asset resolver??
				
			AttachMeshFromAsset(importConfig, meshConfig, asset);
		}
	}
	if (found < 1)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to identify any mesh assets in %s"), *importConfig.importTitle);
	}
}

//static props
TWeakObjectPtr<UMaterial> UEchoMeshService::defaultMaterial = nullptr;
FEchoCustomMeshImportSettings UEchoMeshService::defaultMeshSettings;
bool UEchoMeshService::debugPrintMaterialInfo = false;
bool UEchoMeshService::hideDefaultUnwantedWarnings = true;

void UEchoMeshService::SetHideDefaultUnwantedWarnings(bool setHideDefaultUnwantedWarnings)
{
	hideDefaultUnwantedWarnings = setHideDefaultUnwantedWarnings;
	//UEchoImporter::SetHideDEfaultUnwantedWarnings(setHideDefaultUnwantedWarnings);
	UEchoImporter::SetHideDefaultUnwantedWarnings(setHideDefaultUnwantedWarnings);
}

//void UEchoMeshService::BindDefaultMaterial(const FString &matName, UMaterial *setDefaultMat)
void UEchoMeshService::BindDefaultMaterial(UMaterial *setDefaultMat)
{
	defaultMaterial = setDefaultMat;
}
//TODO: provide custom import settings?
void UEchoMeshService::SetDefaultMeshImportSettings(const FEchoCustomMeshImportSettings &useSettings)
{
	defaultMeshSettings = useSettings;
}
/*
void UEchoMeshService::ConfigureDebugProps(bool bSetInvertWinding, float setUniformScale)
{
	bInvertWinding = bSetInvertWinding;
	uniformScale = setUniformScale;
}
*/

bool UEchoMeshService::DebugLoadAssetFromFile(const FString &localFilename, EEchoAssetType assetType, FEchoMemoryAsset &result)//, const FEchoImportConfig &config)
{
	/*
	//horrible hack:
	std::string fname_nonwstring;
	const auto &uechars = localFilename.GetCharArray();
	std::wstring fname_wstring(uechars.GetData());
	fname_nonwstring.assign(fname_wstring.begin(), fname_wstring.end());
	*/
	result.assetType = EEchoAssetType::EEchoAsset_Unknown;//before loading complete
	result.bHaveContent = false;
	result.blob.Reset();
	result.cachedAt = EchoHelperLib::GetCurrentTime_BP();
	result.fileInfo.filename = localFilename; //hack, might just want file title?
	result.fileInfo.storageId = "file://" + localFilename; //HACK
	//bool ok = ReadTestFile(fname_nonwstring.c_str(), result.blob);
	//bool ok = ReadLocalFile(fname_nonwstring.c_str(), result.blob);
	//bool ok = ReadLocalFile(fname_nonwstring, result.blob);
	bool ok = ReadLocalFile(localFilename, result.blob);
	if (!ok)
	{
		UE_LOG(LogTemp, Error, TEXT("FAILED TO READ FILE BLOB AS ASSET: %s"), *localFilename);
		return false;
	}
	result.bHaveContent = result.blob.Num() > 0;
	if (!result.bHaveContent)
	{
		UE_LOG(LogTemp, Error, TEXT("READ ZERO BYTES FROM FILE BLOB AS ASSET: %s"), *localFilename);
		return false;
	}
	result.assetType = EEchoAssetType::EEchoAsset_Mesh; //HACK assume given correct type - TODO: maybe check file extension if present and/or file MAGIC
	return true;
}

//TMap<FName, TSharedRef<UTexture2D>> UEchoMeshService::debugTextureBindings;
TMap<FName, TWeakObjectPtr<UTexture2D>> UEchoMeshService::debugTextureBindings;
//void UEchoMeshService::BindDebugTexture(const FName &texName, TSharedPtr<UTexture2D> setTexture)
void UEchoMeshService::BindDebugTexture(const FName &texName, UTexture2D *setTexture)
{
	if (setTexture != nullptr)
	{
		//TSharedPtr<UTexture2D> texPtr(setTexture);
		//
		//debugTextureBindings
		if (debugTextureBindings.Contains(texName))
		{
			//TStrongRef
			//TODO: convert to TStrongObjectPtr ???
			//got fatal crash here with deleted render resource!
			//TSharedRef<UTexture2D> &texRef = debugTextureBindings[texName];
			TWeakObjectPtr<UTexture2D> &texRef = debugTextureBindings[texName];
			//texRef = texPtr.ToSharedRef();
			texRef = setTexture;
		}
		else
		{
			TWeakObjectPtr<UTexture2D> texPtr(setTexture);
			//debugTextureBindings.Add(texName, texPtr.ToSharedRef());
			debugTextureBindings.Add(texName, texPtr);
		}
	}
	else
	{
		debugTextureBindings.Remove(texName);
	}
	
}


///////////////////////////////////////////////////////////

bool UEchoMeshService::CheckSupportedMeshVersion(const FString &callContext, const FEchoMeshVariants &forMeshVariant, int32 maxSupportedMeshVersion)
{
	//bool ret(forMeshVariant.meshFormatVersion <= maxSupportedMeshVersion);
	bool ret = (forMeshVariant.meshFormatVersion <= maxSupportedMeshVersion);
	if (!ret)
	{
		UE_LOG(LogTemp, Error, TEXT("%s: EchoMeshVariant handling needs to be updated. max supported version: %d, caller provided version: %d. new cases need to be handled!"), *callContext, maxSupportedMeshVersion, forMeshVariant.meshFormatVersion);
	}
	return ret;
}

UMaterialInstanceDynamic *UEchoMeshService::ParseOneMaterialDefault(
	UObject *WorldContextObject,
	const FString &importTitle, 
	const FFinalReturnData & materialResults, 
	const FEchoImportMaterialData &forMaterial,
	const FEchoMeshVariants &forMeshVariant,
	UMaterial *overrideMaterialMaster
)
{
	bool supported;
	const int32 MaxSupportedMeshFormatVersion = 1; //this is per-handler function generally
	static const FString ThisFunction(TEXT("ParseOneMaterialDefault")); //or __FUNC__ ??
	supported = CheckSupportedMeshVersion(ThisFunction, forMeshVariant, MaxSupportedMeshFormatVersion);
	if (!supported)
	{
		//lets continue even if unsupported since can probably not crash and burn?
	}

	//NB: WorldContextObject is the "owner" - something with a valid GetWorld()
	
	//pick override, else if have template then defaultMaterial else nullptr
	
	//carrying around too many things
	//thisconfig template vs actorConfig.hologramTemplate ???
	UMaterial *materialMaster = (overrideMaterialMaster != nullptr) ? overrideMaterialMaster : nullptr;
		//((actorConfig.hologramTemplate != nullptr) ? actorConfig.hologramTemplate->GetDefaultMaterialMaster() : nullptr); //caller should handle
		//((thisConfig.hologramTemplate != nullptr) ? thisConfig.hologramTemplate->defaultMaterial : nullptr);

	//try to pull in "default" material if we don't have one
	materialMaster = (materialMaster != nullptr) ? materialMaster : defaultMaterial.Get();

	if (materialMaster == nullptr)
	{
		//UE_LOG(LogTemp, Error, TEXT("UEchoMeshService::ParseOneMaterialDefault: unable to find a material master: importing '%s' : mat '%s'"), *thisConfig.importTitle, *forMaterial.MaterialName.ToString());
		UE_LOG(LogTemp, Error, TEXT("UEchoMeshService::ParseOneMaterialDefault: unable to find a material master: importing '%s' : mat '%s'"), *importTitle, *forMaterial.MaterialName.ToString());
		return nullptr;
	}

	UMaterialInstanceDynamic *matInstance = nullptr;
	FName usingMatName; //None by default
	if (DoRenames)
	{
		FString strUsingMatName;
		strUsingMatName += materialMaster->GetName();
		strUsingMatName += TEXT("_");
		//strUsingMatName += meshTitleLetters; //or meshSource?
		//TODO: importId vs importTitle??
		//strUsingMatName += StringUtil::IdentifiersOnly(thisConfig.importTitle);//meshTitle); //or meshSource?
		strUsingMatName += StringUtil::IdentifiersOnly(importTitle);//meshTitle); //or meshSource?
		strUsingMatName += TEXT("_");
		strUsingMatName += forMaterial.MaterialName.ToString(); //should maybe be a string after all?
		
		if (forMeshVariant.needVertexColors)
		{
			strUsingMatName += TEXT("_UsingVertexColors");
		}

		usingMatName= FName(strUsingMatName);
		//usingMatName = MakeUniqueObjectName(actor, materialMaster->GetClass(), usingMatName);
		//usingMatName = MakeUniqueObjectName(actor->GetWorld(), materialMaster->GetClass(), usingMatName);
		usingMatName = MakeUniqueObjectName(WorldContextObject->GetWorld(), materialMaster->GetClass(), usingMatName);
					
	}
	//const FEchoImportMaterialData &importMat = matDef;//data.Materials[0];
	const FEchoImportMaterialData &importMat = forMaterial;

	//UE_LOG(LogTemp, Error, TEXT("usingMatName = %s (%s, %s, %s)"), *usingMatName.ToString(), *materialMaster->GetName(), *thisConfig.importTitle, *importMat.MaterialName.ToString());
	//matInstance = UMaterialInstanceDynamic::Create(mat, actor, usingMatName);
	matInstance = UMaterialInstanceDynamic::Create(materialMaster, WorldContextObject, usingMatName);
	//myMat = matInstance;

	//TODO: for-each material?//foreach meshnode??
	//FAIL//const FEchoImportMaterialData &importMat = data.Materials[0];

	if (UEchoMeshService::debugPrintMaterialInfo)
	{
		UE_LOG(LogTemp, Error, TEXT("Material: %s"), *importMat.MaterialName.ToString());
		UE_LOG(LogTemp, Error, TEXT("\tScalars: %d"), importMat.Scalars.Num());
		for(const auto &prop: importMat.Scalars)
		{
			UE_LOG(LogTemp, Error, TEXT("\t\t%s: %f"), *prop.PropertyName.ToString(), prop.Value);
		}
		UE_LOG(LogTemp, Error, TEXT("\tVectors: %d"), importMat.Scalars.Num());
		for(const auto &prop: importMat.Vectors)
		{
			UE_LOG(LogTemp, Error, TEXT("\t\t%s: (%f, %f, %f, %f)"), *prop.PropertyName.ToString(), prop.Value.X, prop.Value.Y, prop.Value.Z, prop.Value.W );
		}
		UE_LOG(LogTemp, Error, TEXT("\tTextures: %d"), importMat.Textures.Num());
		for(const auto &prop: importMat.Textures)
		{
			//TODO: semantic?
			//UE_LOG(LogTemp, Error, TEXT("\t%s: %s"), *prop.PropertyName.ToString(), *prop.TextureKey.ToString());
			//UE_LOG(LogTemp, Error, TEXT("\t%s: [%d]: %d"), *prop.PropertyName.ToString(), prop.TextureIndex, prop.SemanticIndex);
			UE_LOG(LogTemp, Error, TEXT("\t\t%s: [%d]: %d"), *prop.TextureKey.ToString(), prop.TextureIndex, prop.SemanticIndex);
		}
	}
	//TODO: material parser function by dictionary key?
				
	//TODO: these constants are aiTextureType from material.h in assimp there needs to be a better way?
	//TODO: should probably translate these to something more sane at a level thats not likely to be replaced with a factory method?
	//	unless we want to formalize our dependence on AssImp???

	//Semantics are based on AssImp's material.h
	const int Semantic_Albedo = 1;
	const int Semantic_BaseColor = 12;
	const FName kAlbedoMap = FName(TEXT("AlbedoMap"));
	const FName kUseAlbedoMap = FName(TEXT("useAlbedoMap"));

	const int Semantic_Normal = 6;
	const FName kNormalMap = FName(TEXT("NormalMap"));
	const FName kUseNormalMap = FName(TEXT("useNormalMap"));

	const int Semantic_Emissive = 4;
	const FName kEmissiveMap = FName(TEXT("EmissiveMap"));
	const FName kUseEmissiveMap = FName(TEXT("useEmissiveMap"));

	const int Semantic_Metalness = 15;
	const FName kMetalnessMap = FName(TEXT("MetalnessMap"));
	const FName kUseMetalnessMap = FName(TEXT("useMetalnessMap"));
				
	const int Semantic_Roughness = 16;
	const FName kRoughnessMap = FName(TEXT("RoughnessMap"));
	const FName kUseRoughnessMap = FName(TEXT("useRoughnessMap"));
				
	const FName kMetalRoughnessMap = FName(TEXT("MetallicRoughnessMap"));

	const int Semantic_Unknown = 18;

	//TODO: per texture grouping settings?
	bool bEnableAlbedoMap = false;
	bool bEnableEmissiveMap = false;
	bool bEnableNormalMap = false;
	bool bEnableMetalnessMap = false;
	bool bEnableRoughnessMap = false;

	const int BaseScore = 10;
	const int ScoreStep = 10;
	const TMap<int, int> TextureScorePerSemantic = {
		{Semantic_Albedo, BaseScore},
		{Semantic_BaseColor, BaseScore + 1 * ScoreStep},

		{Semantic_Normal, BaseScore},

		{Semantic_Emissive, BaseScore},
					
		{Semantic_Metalness, BaseScore},
		{Semantic_Roughness, BaseScore},
	};

	TMap<FName, int> seenTextureScores;
	//TArray<FEchoImportMaterialTexture> unknownTextures;
	int unknownCounter = 0;
	int textureSlotIndex = -1;
	//TODO: deal with repeated textures
	//UE_LOG(LogTemp, Error, TEXT("Texturecount: %s [vc=%s] : %d"), *forMaterial.MaterialName.ToString(), *StringUtil::BoolToString(forMeshVariant.needVertexColors), importMat.Textures.Num());
	for(const auto &prop: importMat.Textures)
	{
		textureSlotIndex++;
		bool bUnknownTexture = false;
		FName semanticMapping;
		switch(prop.SemanticIndex)
		{
			case Semantic_BaseColor:
				ECHO_FALLTHROUGH
			case Semantic_Albedo://1: 
				semanticMapping = kAlbedoMap;//= FName(TEXT("albedoMap"));
				bEnableAlbedoMap = true;
				break;

			case Semantic_Emissive://4
				semanticMapping = kEmissiveMap;
				bEnableEmissiveMap = true;
				break;

			case Semantic_Normal://6:
				semanticMapping = kNormalMap;
				bEnableNormalMap = true;
				break;


			case Semantic_Metalness:
				semanticMapping = kMetalnessMap;
				bEnableMetalnessMap = true;
				break;

			case Semantic_Roughness:
				semanticMapping = kRoughnessMap;
				bEnableRoughnessMap = true;
				break;

			//These are hacks to print the status and visualize the missing texture as emissive.
			case Semantic_Unknown://18
				//TODO: ignore if pbr already known and is same?
				UE_LOG(LogTemp, Error, TEXT("UNKNOWN (To Importer) Texture Type: %d"), prop.SemanticIndex);
				bUnknownTexture = true;
				//unknownTextures.Push(prop);
				if (hackShowUnknownMapsAsEmissive)
				{
					semanticMapping = kEmissiveMap;
					bEnableEmissiveMap = true;
				}
				break;
			default:
				UE_LOG(LogTemp, Error, TEXT("Unhandled Texture Type: %d"), prop.SemanticIndex);
				//unknownTextures.Push(prop);
				bUnknownTexture = true;
				if (hackShowUnknownMapsAsEmissive)
				{
					semanticMapping = kEmissiveMap;
					bEnableEmissiveMap = true;
				}
				break;
		}

		const int *seenScoreForThisOutSemanticPtr = seenTextureScores.Find(semanticMapping);
		const int seenScoreForThisOutSemanticValue = (seenScoreForThisOutSemanticPtr != nullptr) ? *seenScoreForThisOutSemanticPtr : 0;

		const int *newScorePtr = TextureScorePerSemantic.Find(prop.SemanticIndex);
		const int newScoreValue = (newScorePtr != nullptr) ? *newScorePtr : 0;
					
		//TODO: maybe not skip if unknown semantic?
		if (newScoreValue < seenScoreForThisOutSemanticValue)
		{
			UE_LOG(LogTemp, Warning, TEXT("Mat: '%s': Texture assignment: Ignoring lower scored source for semantic: %s: found score: %d, already seen score: %d"), *importMat.MaterialName.ToString(), *semanticMapping.ToString(), newScoreValue, seenScoreForThisOutSemanticValue);
			continue;
		}
		seenTextureScores.Add(semanticMapping, newScoreValue);
					
		//const TSharedRef<UTexture> * foundMap = 
		//auto tmp = debugTextureBindings.Find(semanticMapping);

		//const TSharedRef<UTexture2D> * foundMap = debugTextureBindings.Find(semanticMapping);
		UTexture *tex = prop.Value;
		bool bTexValid = (tex != nullptr);
		//#if CHECK_BU
		if (bTexValid)
		{
			//TODO: put this in a checked define
			bTexValid = bTexValid && tex->IsValidLowLevel();
			if (!bTexValid)
			{
				//UE_LOG(LogTemp, Error, TEXT("!bTexValid!"));
				UE_LOG(LogTemp, Error, TEXT("Mat: %s: semantic: %s: !IsValidLowLevel"), *importMat.MaterialName.ToString(), *semanticMapping.ToString());
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Mat: %s: tex is nullptr! was it resolved? gonna try debug textures!: %s - semantic %s"), *importMat.MaterialName.ToString(), *prop.TextureKey.ToString(), *semanticMapping.ToString());
		}


		//lol defaults? or fallback?
		if (!bTexValid)
		{
			//const TWeakObjectPtr<UTexture2D> *foundMap = debugTextureBindings.Find(semanticMapping);
			const TWeakObjectPtr<UTexture2D> *foundMap = debugTextureBindings.Find(semanticMapping);
			if ((foundMap !=nullptr)&&(foundMap->IsValid()))
			{
				tex = foundMap->Get();
								
			}
			else if (foundMap == nullptr)
			{
				UE_LOG(LogTemp, Error, TEXT("No resolution found in mat %s for texture for semantic: %s"), *importMat.MaterialName.ToString(), *semanticMapping.ToString());
			}
			else if (!foundMap->IsExplicitlyNull())
			{
				//TODO: perhaps we want a strong reference after all?
				UE_LOG(LogTemp, Error, TEXT("Texture in debug bindings was garbage collected: %s"), *semanticMapping.ToString());
			}
			else
			{
				//unexpected
				UE_LOG(LogTemp, Error, TEXT("Somehow explicitly stored null in debug texture bindings!"));
			}

			//ensure sanity on this end
			bTexValid = (tex != nullptr);
			if (bTexValid)
			{
				bTexValid = bTexValid && tex->IsValidLowLevel();
				if (!bTexValid)
				{
					//UE_LOG(LogTemp, Error, TEXT("Fallback TExture for Mat: %s: in semantic %s: !bTexValid!"), *importMat.MaterialName.ToString(), *semanticMapping.ToString()); 
					UE_LOG(LogTemp, Error, TEXT("Fallback TExture for Mat: %s: in semantic %s: !IsValidLowLevel"), *importMat.MaterialName.ToString(), *semanticMapping.ToString()); 
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Mat: %s: No Fallback Texture for semantic: %s"), *importMat.MaterialName.ToString(), *semanticMapping.ToString()); 
			}

		}

		/*if (tex == nullptr)
		{
			//UE_LOG(LogTemp, Error, TEXT("Failed To resolve texture for %s"), *semanticMapping.ToString());
		}
		else
		*/
		if (semanticMapping == NAME_None)
		{
			if (bUnknownTexture)
			{
				semanticMapping = FName(FString::Printf(TEXT("Unknown_TextureSlot_%d"), textureSlotIndex)); 
			}
		}
		if (bTexValid)
		{
			//if (tex != nullptr)
			{
				//matInstance->SetTextureParameterValue(semanticMapping, &foundMap->Get());
				matInstance->SetTextureParameterValue(semanticMapping, tex);
			}
		}
	}
	//toggle map switches
	const float EnableParameter = 1, DisableParameter = 0;
	//matInstance->SetScalarParameterValue(kUseAlbedoMap, bEnableAlbedoMap ? 1 : 0);
	matInstance->SetScalarParameterValue(kUseAlbedoMap, bEnableAlbedoMap ? EnableParameter : DisableParameter);
	matInstance->SetScalarParameterValue(kUseEmissiveMap, bEnableEmissiveMap ? EnableParameter : DisableParameter);
	matInstance->SetScalarParameterValue(kUseNormalMap, bEnableNormalMap ? EnableParameter : DisableParameter);
	matInstance->SetScalarParameterValue(kUseMetalnessMap, bEnableMetalnessMap ? EnableParameter : DisableParameter);
	matInstance->SetScalarParameterValue(kUseRoughnessMap, bEnableRoughnessMap ? EnableParameter : DisableParameter);

	//TODO: don't set both separately???
	if (bEnableMetalnessMap || bEnableRoughnessMap)
	{
		if (bEnableMetalnessMap && bEnableRoughnessMap)
		{
			//TODO: what if somehow not a tex2d? or null?
			UTexture *metallicRoughness = nullptr;
			if (matInstance->GetTextureParameterValue(kRoughnessMap, metallicRoughness))
			{
				matInstance->SetTextureParameterValue(kMetalRoughnessMap, metallicRoughness);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("(bEnableMetalnessMap && bEnableRoughnessMap), but unable to read roughnessmap! Mat=%s"), *usingMatName.ToString());
			}
		}
		else
		{
			//TODO: implement me later - have to figure out how to remap channels
			UE_LOG(LogTemp, Error, TEXT("Mat: %s : unsupported mat case: only roughness map or only metallicness map! metalness? %s roughness? %s"), *usingMatName.ToString(), *StringUtil::BoolToString(bEnableMetalnessMap), *StringUtil::BoolToString(bEnableRoughnessMap));
		}
	}
				
	//static const FName kOutSpecular(TEXT("Specular"));//should this be a color or an intensity????
	//TODO: do something similar with textures?
	
	static const FName kUseVertexColors(TEXT("useVertexColor"));
	

	//scalars
		//in
			static const FName kInGLTFMetalic(TEXT("$mat.metallicFactor"));
			static const FName kInGLTFMetalic_PBR(TEXT("$mat.gltf.pbrMetallicRoughness.metallicFactor"));
						
			static const FName kInGLTFRoughness(TEXT("$mat.roughnessFactor"));
			static const FName kInGLTFRoughness_PBR(TEXT("$mat.gltf.pbrMetallicRoughness.roughnessFactor"));
						

			//LogTemp: Error: Unhandled Scalar Name: $mat.shininess
		//out
			static const FName kOutMetalic(TEXT("Metallic"));
			static const FName kOutRoughness(TEXT("Roughness"));
				
	//vectors
		//in
			static const FName kInGLTFBaseColor(TEXT("$mat.gltf.pbrMetallicRoughness.baseColorFactor"));
			static const FName kInDiffuseColor(TEXT("$clr.diffuse"));
			static const FName kInEmissiveColor(TEXT("$clr.emissive"));

		//out
			static const FName kOutBaseColor(TEXT("AlbedoColor"));
			static const FName kOutEmissiveColor(TEXT("EmissiveColor"));
						
	//TODO: allow aliases but add a priority for each and pick the highest priority?//most specifificity? or are they just aliases??
	//static const TMap<FName, FName> scalarRenames = {
	static const TMap<FName, std::tuple<FName,int>> scalarRenames = {
		{kInGLTFMetalic, {kOutMetalic,0}},
		{kInGLTFMetalic_PBR, {kOutMetalic,1}},
		{kInGLTFRoughness, {kOutRoughness,0}},
		{kInGLTFRoughness_PBR, {kOutRoughness,1}},
	};
	static const TMap<FName, std::tuple<FName, int>> vectorRenames = {
		{kInGLTFBaseColor, {kOutBaseColor, 1}},
		{kInDiffuseColor, {kOutBaseColor, 0}},
		{kInEmissiveColor, {kOutEmissiveColor,0}},
	};

	TMap<FName, int> seenScalarScores;
	for(const auto &prop: importMat.Scalars)
	{
		//UE_LOG(LogTemp, Error, TEXT("\t%s: %f"), *prop.PropertyName.ToString(), prop.Value);
		FName remappedName = prop.PropertyName; //hack
		const std::tuple<FName, int> *match = scalarRenames.Find(prop.PropertyName);
		//const FName *foundName = nullptr;
		if (match != nullptr)
		{
			//remappedName = *foundName;
			remappedName = std::get<0>(*match);
			//foundName = *std::get<0>(*match);
		}
		else
		{
			//No match.
			if (!shouldIgnoreProperty(prop.PropertyName))
			{
				UE_LOG(LogTemp, Error, TEXT("Unhandled Scalar Name: %s"), *prop.PropertyName.ToString());
			}
		}

		const int *seenScorePtr = seenScalarScores.Find(remappedName);
		const int seenScoreValue = (seenScorePtr != nullptr) ? *seenScorePtr : -1;

		const int foundScoreValue = (match != nullptr) ? std::get<1>(*match) : 0;

		if (seenScoreValue > foundScoreValue)
		{
			UE_LOG(LogTemp, Warning, TEXT("scalar: seen %i > found %i - skipping scalar"), seenScoreValue, foundScoreValue);
			continue;
		}
		matInstance->SetScalarParameterValue(remappedName, prop.Value);

	}
	//UE_LOG(LogTemp, Error, TEXT("Vectors: %d"), importMat.Scalars.Num());
	TMap<FName, int> bestSeenVectors;
	for(const auto &prop: importMat.Vectors)
	{
		FName remappedName = prop.PropertyName; //hack
		//UE_LOG(LogTemp, Error, TEXT("Vector VISITED Set: %s"), *remappedName.ToString());
		const std::tuple<FName,int> *foundName = vectorRenames.Find(prop.PropertyName);
		int *score = bestSeenVectors.Find(prop.PropertyName);
		int existingScore = -1;
		if (score != nullptr)
		{
			existingScore = *score;
		}
		if (foundName != nullptr)
		{
			int newScore = std::get<1>(*foundName);
			if (newScore > existingScore)
			{
				remappedName = std::get<0>(*foundName);
				if (score != nullptr)
				{
					bestSeenVectors[prop.PropertyName] = newScore;//update score
				}
				else
				{
					bestSeenVectors.Add(prop.PropertyName, newScore);
				}
							
			}
			else
			{
				//failed test (we're lower priority)
				//UE_LOG(LogTemp, Warning, TEXT("SKIPPING ALIASED PROPERTY: %s"), *prop.PropertyName.ToString());
				continue;
			}
		}
		else
		{
			if (!shouldIgnoreProperty(prop.PropertyName))
			{
				UE_LOG(LogTemp, Error, TEXT("Unhandled Vector Name: %s"), *prop.PropertyName.ToString());
			}
		}
		//UE_LOG(LogTemp, Error, TEXT("Vector Set: %s"), *remappedName.ToString());
		matInstance->SetVectorParameterValue(remappedName, FLinearColor(prop.Value)); //gah -- maybe store as flinearcolor?
	}

	const float TrueFloat = 1.0f;
	const float FalseFloat = 0.0f;
	//matInstanceVC->SetScalarParameterValue(kUseVertexColors FName(TEXT("useVertexColor")), (forMeshVariant.needVertexColors) ? TrueFloat : FalseFloat);
	//matInstanceVC->SetScalarParameterValue(kUseVertexColors, (forMeshVariant.needVertexColors) ? TrueFloat : FalseFloat);
	matInstance->SetScalarParameterValue(kUseVertexColors, (forMeshVariant.needVertexColors) ? TrueFloat : FalseFloat);

	return matInstance;
	//}
}//END EVALMATDEFAULT

void UEchoMeshService::ResetState()
{
	defaultMaterial = nullptr;
	defaultMeshSettings = FEchoCustomMeshImportSettings();
	debugTextureBindings.Reset();
	debugPrintMaterialInfo = false;
	hideDefaultUnwantedWarnings = true;

	UEchoImporter::ResetState();


	SetHideDefaultUnwantedWarnings( true );
}

#pragma optimize( "", on )
