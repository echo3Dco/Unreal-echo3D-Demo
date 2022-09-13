#pragma once

#include "CoreMinimal.h"
#include "EchoStructsFwd.h"
#include "EchoMeshService.h"

#define DISABLE_TEST_OBJ_MESH 1

bool ReadLocalFile(const FString &filename, TArray<uint8> &result);

#ifndef DISABLE_TEST_OBJ_MESH
typedef struct ObjMesh;
typedef struct ObjVert;
/*
struct CustomMeshConfig
{
	float uniformScale;
	bool bInvertWinding;
	bool bFixAxes;
};
*/

struct SimpleMesh
{
	TArray<FVector> verts;
	TArray<FVector2D> uvs;
	TArray<FVector> normals;
	TArray<int32> tris;
	bool bLoaded = false;

	SimpleMesh()
	 : bLoaded( false )
	{
	}

	SimpleMesh(const ObjMesh &fromObj)
	 : bLoaded( false )
	{
		LoadObj(fromObj);
	}

	void LoadObjPtr(const ObjMesh *fromObj)
	{
		if (fromObj != nullptr)
		{
			LoadObj(*fromObj);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("LoadObjPtr: null fromObj"));
		}
	}
	void LoadObj(const ObjMesh &fromObj);
	void AddVert(const ObjMesh &fromObj, const ObjVert &vert);

	void Reset()
	{
		bLoaded = false;
		verts.Reset();
		uvs.Reset();
		normals.Reset();
		tris.Reset();
	}

};

//bool ReadMeshFromEchoMemoryAsset(asset, CustomMeshConfig{UEchoMeshService::uniformScale, UEchoMeshService::bInvertWinding});
//bool ReadMeshFromEchoMemoryAsset(const FEchoMemoryAsset &asset, const CustomMeshConfig &config, SimpleMesh &result);
bool ReadMeshFromEchoMemoryAsset(const FEchoMemoryAsset &asset, const FEchoCustomMeshImportSettings &config, SimpleMesh &result);
//{UEchoMeshService::uniformScale, UEchoMeshService::bInvertWinding});

#endif