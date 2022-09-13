#pragma once
/**
 * provides functionality to import meshes.
 * 
 * Under the hood we use AssImp to import the models and get mesh and material and texture data with a bunch of logic to make things play (nicer) with our system and Unreal
 * 
 * portions based on RML are under the MIT License. (Included below)
**/
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

//needed for FProcMeshTangent
#include "ProceduralMeshComponent.h"

#include "EchoImporter.generated.h"



//TODO: just put these as a dictionary perhaps?
/**
 * one imported scalar
**/
USTRUCT(BlueprintType)
struct ECHO3DASSIMP_API FEchoImportMaterialScalar
{
    GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "EchoMaterial")
	FName PropertyName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "EchoMaterial")
	float Value;
};

/**
 * one imported Vector4
**/
USTRUCT(BlueprintType)
struct ECHO3DASSIMP_API FEchoImportMaterialVector
{
    GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "EchoMaterial")
	FName PropertyName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "EchoMaterial")
	FVector4 Value;
};

/**
 * binding information for one texture
**/
USTRUCT(BlueprintType)
struct ECHO3DASSIMP_API FEchoImportMaterialTexture
{
    GENERATED_BODY()

	//poorly defined
	//UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "EchoMaterial")
	//FName PropertyName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "EchoMaterial")
	FName TextureKey;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "EchoMaterial")
	int32 TextureIndex;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "EchoMaterial")
	int32 SemanticIndex;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "EchoMaterial")
	UTexture2D *Value;

};

/**
 * properties for one imported material. 
 * 
 * presently not converted from AssImp keys
**/
USTRUCT(BlueprintType)
struct ECHO3DASSIMP_API FEchoImportMaterialData
{
    GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "EchoMaterial")
	FName MaterialName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "EchoMaterial")
	TArray<FEchoImportMaterialScalar> Scalars;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "EchoMaterial")
	TArray<FEchoImportMaterialVector> Vectors;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "EchoMaterial")
	TArray<FEchoImportMaterialTexture> Textures;

};

/**
 * one imported texture
**/
USTRUCT(BlueprintType)
struct ECHO3DASSIMP_API FEchoImportTexture
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "EchoTexture")
	UTexture2D *texture; //same as textureref

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "EchoTexture")
	bool IsValid;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "EchoTexture")
	int Width;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "EchoTexture")
	int Height;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "EchoTexture")
	FString filename;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "EchoTexture")
	FName importerTexId;
};

//////////////////// FROM RuntimeMeshLoader //////////////////////////////////////////
//Portions of this are from RML. 
/**
 MIT License

Copyright (c) 2017 Eric Liu

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
**/
/**
 * contains a vertex data stream
**/
USTRUCT(BlueprintType)
struct ECHO3DASSIMP_API FMeshData
{
    GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FinalReturnData")
	FString SubmeshName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FinalReturnData")
	TArray<FVector> Vertices;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FinalReturnData")
	TArray<FLinearColor> Colors;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FinalReturnData")
	TArray<int32> Triangles;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FinalReturnData")
	TArray<FVector> Normals;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FinalReturnData")
	TArray<FVector2D> UVs0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FinalReturnData")
	TArray<FVector2D> UVs1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FinalReturnData")
	TArray<FVector2D> UVs2;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FinalReturnData")
	TArray<FVector2D> UVs3;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FinalReturnData")
	TArray<FProcMeshTangent> Tangents;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FinalReturnData")
	int32 UseMaterialIndex;
	
};

/**
 * contains information about a node in the mesh "tree"
**/
USTRUCT(BlueprintType)
struct ECHO3DASSIMP_API FNodeData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FinalReturnData")
	FString NodeName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FinalReturnData")
	FTransform RelativeTransformTransform;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FinalReturnData")
	int NodeParentIndex;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FinalReturnData")
	TArray<FMeshData> Meshes;
};

/**
 * contains the parsed model data
**/
USTRUCT(BlueprintType)
struct ECHO3DASSIMP_API FFinalReturnData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FinalReturnData")
	bool Success;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FinalReturnData")
	TArray<FNodeData> Nodes;

	//TODO: make this a TMap later?
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FinalReturnData")
	TArray<FEchoImportMaterialData> Materials;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FinalReturnData")
	TArray<FEchoImportTexture> Textures;
	//TArray<FEchoImportTextureData> Textures;
};

/**
 * used with some deprecated RML functionality to control how to generate the path for local files
**/
UENUM(BlueprintType)
enum class EPathType : uint8
{
	Absolute,
	Relative
};

//End MIT License block for RuntimeMeshLoader
////////////////////////////////////////////////////////////////////////////////

/**
 * Class that provides access to mesh importing.
 * 
 * Portions based on RML.
**/
UCLASS()
class ECHO3DASSIMP_API UMeshLoader : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * load mesh data from a local file
	**/
	UFUNCTION(BlueprintCallable,Category="EchoImport")
	static FFinalReturnData LoadMeshFromFile(FString FilePath, EPathType type = EPathType::Absolute);
	
	/**
	 * load mesh data from blob
	**/
	UFUNCTION(BlueprintCallable,Category="EchoImport")
	static FFinalReturnData LoadMeshFromBlob(const FString &filenameInfo, const TArray<uint8> &blob);

	/**
	 * lots of import verbosity
	**/
	UFUNCTION(BlueprintCallable,Category="EchoImport|Debug")
	static void SetImporterVerbose(bool setVerbose)
	{
		importerVerbose = setVerbose;
	}
	
	/**
	 * print verbose texture assignment info
	**/
	UFUNCTION(BlueprintCallable,Category="EchoImport|Debug")
	static void SetEchoImporterVerboseTextures(bool setVerboseTextures)
	{
		importerVerboseTextures = setVerboseTextures;
	}

	/** 
	 * prints vertex attributes stream data loaded from model for first 64 verts for debugging 
	 */
	UFUNCTION(BlueprintCallable,Category="EchoImport|Debug")
	static void SetDebugVertexStreams(bool setDebugVertexData)
	{
		debugVertexStreams = setDebugVertexData;
	}

	/**
	 * prints heirarchy information from imported model
	**/
	UFUNCTION(BlueprintCallable,Category="EchoImport|Debug")
	static void SetDebugImportedHierarchy(bool setDebugImportedHierarchy)
	{
		debugImportedHierarchy = setDebugImportedHierarchy;
	}

	/**
	 * control how we handle importing normals
	 * 
	 * I'm not quite happy with how the imports seem to work in some cases so beware of weird normals
	**/
	UFUNCTION(BlueprintCallable,Category="EchoImport|Debug")
	static void SetDebugImporterNormalStripping(bool setStripNormals, bool setGenerateNormals, bool setGenerateSmoothNormalsInstead)
	{
		stripNormals = setStripNormals;
		generateNormals = setGenerateNormals;
		generateSmoothNormals = setGenerateSmoothNormalsInstead;//setUseSmoothGeneration;// bSmoothNormalsGenerated;
	}

	/**
	 * enables a magical and sorta inefficent hack that goes through and checks the vertex color stream, if present, to see if its all belows some threshold
	 * if so, we will then choose to ignore the vertex color stream for that mesh. this is because some models we tested seem to randomly include a vertex color
	 * attribute but set it to 0 or values close to zero, leading to all the models becoming black.
	 * 
	 * enabled by default.
	**/
	UFUNCTION(BlueprintCallable,Category="EchoImport|Debug")
	static void SetDebugImporterAutoOmitBadVertexColorStreams(bool setAllowBadVertexColorDetectionAndOmission)
	{
		allowBadVertexColorDetectionAndOmission = setAllowBadVertexColorDetectionAndOmission;
	}

	/**
	 * prints a bunch of material info
	**/
	UFUNCTION(BlueprintCallable,Category="EchoImport|Debug")
	static void SetDebugImporterPrintMaterialInfo(bool setShowDebugMaterialInfo)
	{
		debugMaterialInfo = setShowDebugMaterialInfo;
	}

	/**
	 * sets the smoothing angle parameter used for generated smooth normals. 
	 *
	 * I'm not convinced this part is working
	**/
	UFUNCTION(BlueprintCallable,Category="EchoImport|ImportSettings")
	static void SetImporterSmoothingAngleForNormals(float setSmoothingAngle);

	/**
	 * imports a texture2d from a local file
	**/
	UFUNCTION(BlueprintCallable,Category="EchoImport")
	static UTexture2D* LoadTexture2DFromFile(const FString& FullFilePath, bool& IsValid, int32& Width, int32& Height);

	/**
	 * imports a texture2d from a blob
	**/
	UFUNCTION(BlueprintCallable,Category="EchoImport")
	static UTexture2D* LoadTexture2DFromBlob(const FString &filenameInfo, const TArray<uint8> &blob);



////////////////////////////////////////////////
	//internal stuff don't use - not private since a bunch of random procedures need access, but that will probably eventually change when this is refactored
	static bool importerVerbose;
	static bool importerVerboseTextures;
	static bool debugVertexStreams;
	static bool debugImportedHierarchy;

	static bool debugMaterialInfo;

	//WorkArounds:
	static bool stripNormals;
	static bool generateNormals;
	static bool generateSmoothNormals;
	static bool allowBadVertexColorDetectionAndOmission;
	static float smoothNormalsAngle; //default angle for AssImp (apparently?)

	/**
	 * internal texture loading function wrapped by other uses
	**/
	static UTexture2D* LoadTexture2DFromBlob_Internal(const FString &filenameInfo, const TArray<uint8> &blob, bool& IsValid, int32& Width, int32& Height);
};
