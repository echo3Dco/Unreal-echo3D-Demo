#pragma once
#include "EchoImporter.h"

//TODO: maybe put common echo types into a helper module both can rely on? or something?
//TODO: fix all ascii <-> fstring conversion into one place and ensure its legit secure/correct
//tho fstring->ascii might be lossy... =(



/*
Some portions are from originally from RuntimeMeshLoader (RML) and are under the MIT License
most have been substantially modified

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



#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Modules/ModuleManager.h"
#include "Misc/FileHelper.h"
#include "IImageWrapper.h"
#include "Runtime/ImageWrapper/Public/IImageWrapperModule.h"
#include "HAL/FileManager.h"
#include "HAL/FileManagerGeneric.h"
//#include "ProceduralMeshComponent.h"

//break your limits, uint16_t vertex color data, GO!
#include <limits>

#pragma optimize( "", off )

//TODO: make it an importer flag??

namespace EchoImporterCpp
{
	//NB: I'm not sure which version of AssImp we have, I'm thinking its bounded by a06133ab52cc49d611f2616a55840dfdb7522d09 ~ 4e7e47bd436f4a97775be22dcdb8e99f5c158606
	//since the copyright dates change before/after those commits - https://github.com/assimp/assimp/commits/master/include/assimp/defs.h

	//This is needed since we apparently have an old version of AssImp, and Echo is outputting a non-supported vertex color type, so this magically works around.
	// AFAIK this seems to be the likely PR we need: https://github.com/assimp/assimp/pull/3582
	//const bool hackVertexColors = true;//hack
	const bool hackVertexColors = false;//no longer needed in AssImp 5.2.4

	//a hack to detect and ignore dummy vertex color streams
	//const bool hackVertexColorsV2 = false;
	//const bool hackVertexColorsV2 = true; //now use UMeshLoader::allowBadVertexColorDetectionAndOmission
	

	const FString DefaultNullString(TEXT("<null>"));

	const bool superVerbose = false;



	///IMPORTER FLAGS
	
	//TODO: try triangulate at the very end??

	//Original
	//const unsigned int _BaseImportFlagsOriginal = aiProcess_Triangulate | aiProcess_MakeLeftHanded | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_OptimizeMeshes;
	//const unsigned int DefaultImportFlags = aiProcess_Triangulate | aiProcess_MakeLeftHanded | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_OptimizeMeshes;
	//const unsigned int _BaseImportFlags = aiProcess_Triangulate | aiProcess_MakeLeftHanded | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_OptimizeMeshes;
	const unsigned int _BaseImportFlags = aiProcess_Triangulate;
	//aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals
	
	//Test
	//const unsigned int TestFlags = aiProcess_RemoveComponent | aiProcess_GenSmoothNormals;
	//const unsigned int TestFlags = aiProcess_RemoveComponent;
	const unsigned int StripNormalsFlags= aiProcess_RemoveComponent; //works with mImporter.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_NORMALS); 
	
	const unsigned int FixNormalsFlags = aiProcess_GenNormals;
	const unsigned int FixSmoothNormalsFlags = aiProcess_GenSmoothNormals;
	//const unsigned int FixNormalsFlags = aiProcess_GenNormals;

	const unsigned int GenerateTangentsFlags = aiProcess_CalcTangentSpace;

	//const unsigned int FinishImportFlags = aiProcess_MakeLeftHanded | aiProcess_CalcTangentSpace | aiProcess_OptimizeMeshes;
	const unsigned int FinishImportFlags = aiProcess_MakeLeftHanded | aiProcess_OptimizeMeshes;

	//const unsigned int DefaultImportFlags = _BaseImportFlags;
	//const unsigned int DefaultImportFlags = _BaseImportFlags | TestFlags;
	
	const unsigned int DefaultImportFlags = _BaseImportFlags; //good?
	//const unsigned int DefaultImportFlags = _BaseImportFlagsOriginal;

	//const bool bStripNormals = false;
	//const bool bGenerateNormalsAfterStripping = true;

	/*
	aiProcess_RemoveComponent
	aiProcess_GenNormals
	aiProcess_GenSmoothNormals
	*/
};
using namespace EchoImporterCpp;

FString cStringToFString(const char *cChars, const FString &defaultValue)
{
	if (cChars == nullptr)
	{
		return defaultValue;
		//return FString(TEXT("<null>"));
	}
	std::string str(cChars);
	std::wstring wstr;
	wstr.assign(str.begin(), str.end()); //HACK
	return FString(wstr.c_str());
}
FString cStringToFString(const char *cChars)
{
	return cStringToFString(cChars, DefaultNullString);
}

FString aiString2FString(const aiString *strPtr, const FString &defaultValue)
{
	if (strPtr == nullptr)
	{
		//return FString();
		return defaultValue;
		//return FString(TEXT("<null>"));
	}
	const char *cChars = strPtr->C_Str();
	return cStringToFString(cChars, defaultValue);
}
FString aiString2FString(const aiString *strPtr)
{
	return aiString2FString(strPtr, DefaultNullString);//FString(TEXT("<null>")));
}
FString aiString2FString(const aiString &str)
{
	return aiString2FString(&str); //HACK
}
/*
FString aiString2FString(const aiString &aiStr)
{
	//TODO: find a better way!
	//const aiString &aiPropName = prop->mKey;
	// FName propName(fstr);
	//const char * cName = aiPropName.C_Str();
	const char * cStr = aiStr.C_Str();
	std::string strStr(cStr);
	std::wstring wstrStr;
	wstrStr.assign(strStr.begin(), strStr.end());
	FString fstr(wstrStr.c_str());
	return fstr;
	//FName propName(fstr);
}
*/

#define LOG_ERROR_OR_MESSAGE( isError, Format, ... ) \
	if (isError) \
	{ \
		UE_LOG(LogTemp, Error, Format,  __VA_ARGS__ ); \
	} \
	else \
	{ \
		UE_LOG(LogTemp, Display, Format, __VA_ARGS__); \
	}

//TODO: specify which submeshes?

//const bool bDebugMeshes = true;
//UFUNCTION(BlueprintCallable,Category="Echo3dImporter")

bool UMeshLoader::importerVerbose = false;
bool UMeshLoader::importerVerboseTextures = false;
bool UMeshLoader::debugVertexStreams = false;
bool UMeshLoader::debugImportedHierarchy = false;

bool UMeshLoader::debugMaterialInfo = false;


bool UMeshLoader::stripNormals = false;
bool UMeshLoader::generateNormals = true;
bool UMeshLoader::generateSmoothNormals = true;
bool UMeshLoader::allowBadVertexColorDetectionAndOmission = true;
float UMeshLoader::smoothNormalsAngle = 175.0f;

struct MagicVertexColors
{
	unsigned short colors[4];
};

float uint16ToFloatColorComponent(uint16 v)
{
	constexpr double MaxValue = std::numeric_limits<uint16>::max();
	double vd = (double)v;
	double ret = vd / MaxValue;
	ret = (ret<0)?0:((ret>1)?1:ret); //clamp01
	return ret;
}
FLinearColor ConvertToLinearColor(const MagicVertexColors &vc)
{
	return FLinearColor(
		uint16ToFloatColorComponent(vc.colors[0]),
		uint16ToFloatColorComponent(vc.colors[1]),
		uint16ToFloatColorComponent(vc.colors[2]),
		uint16ToFloatColorComponent(vc.colors[3])
	);
}

//deprecated - switch to stringutil once have a common module
//	replace with StringUtil::StringToBool
const FString& bool2String(bool value)
{
	static const FString trueValue(TEXT("true"));
	static const FString falseValue(TEXT("false"));
	return value ? trueValue : falseValue;
}

//FName aiStringToFNAme(const aiString &aiStr)
FName aiStringToFName(const aiString &aiStr)
{
	return FName(aiString2FString(aiStr));
}
FName aiStringToFName(const aiString *aiStr)
{
	if (aiStr == nullptr)
	{
		return FName();
	}
	else
	{
		return aiStringToFName(*aiStr);
	}
}

FName aiPropToFName(const aiMaterialProperty *prop)
{
	//const aiString &aiPropName = prop->mKey;
	// FName propName(fstr);
	const aiString *str = (prop != nullptr) ? &prop->mKey : nullptr;
	return aiStringToFName(str);
	/*
	if (prop == nullptr)
	{
		return FName();
	}
	else
	{
		return FName(aiString2FString(prop->mKey));
	}
	*/
}


#define MAKE_CASE(caseLabel, caseString) \
		case caseLabel: return TEXT(caseString)
	#define MAKE_CASE_SIMPLE(caseLabel) \
		MAKE_CASE(caseLabel, #caseLabel)

FString LookupMaterialPropertyTypeName(aiPropertyTypeInfo type)
{
	//a horrible hack
	switch(type)
	{
		case aiPTI_Float: return TEXT("aiPTI_Float");
		case aiPTI_Double: return TEXT("aiPTI_Double");
		case aiPTI_String: return TEXT("aiPTI_String");
		case aiPTI_Integer: return TEXT("aiPTI_Integer");
		case aiPTI_Buffer: return TEXT("aiPTI_Buffer");
		default: return TEXT("(unknown)");
	}
}


//FString LookupTextureTypeSemanticName(aiTextureType type)
FString LookupTextureTypeSemanticName(unsigned int type)
{
	switch(type)
	{
		//Dummy
		MAKE_CASE_SIMPLE(aiTextureType_NONE);

		//LEGACY API MATERIALS 
		MAKE_CASE_SIMPLE(aiTextureType_DIFFUSE);//want
		MAKE_CASE_SIMPLE(aiTextureType_SPECULAR);
		MAKE_CASE_SIMPLE(aiTextureType_AMBIENT);
		MAKE_CASE_SIMPLE(aiTextureType_EMISSIVE);//want
		MAKE_CASE_SIMPLE(aiTextureType_HEIGHT);
		MAKE_CASE_SIMPLE(aiTextureType_NORMALS); //want
		MAKE_CASE_SIMPLE(aiTextureType_SHININESS);
		MAKE_CASE_SIMPLE(aiTextureType_OPACITY);
		MAKE_CASE_SIMPLE(aiTextureType_DISPLACEMENT);
		MAKE_CASE_SIMPLE(aiTextureType_LIGHTMAP);
		MAKE_CASE_SIMPLE(aiTextureType_REFLECTION);
	
		//PBR Materials
		MAKE_CASE_SIMPLE(aiTextureType_BASE_COLOR);
		MAKE_CASE_SIMPLE(aiTextureType_NORMAL_CAMERA);
		MAKE_CASE_SIMPLE(aiTextureType_EMISSION_COLOR);
		MAKE_CASE_SIMPLE(aiTextureType_METALNESS);
		MAKE_CASE_SIMPLE(aiTextureType_DIFFUSE_ROUGHNESS);
		MAKE_CASE_SIMPLE(aiTextureType_AMBIENT_OCCLUSION);
		
		//Unknown texture
		MAKE_CASE_SIMPLE(aiTextureType_UNKNOWN);
		default: return FString(TEXT("Unknown Case for TextureType Semantic"));
	};
}

#undef MAKE_CASE_SIMPLE
#undef MAKE_CASE


FString bufferToString(const void *ptrBuff, uint32 length)
{
	//static const FString NullString("<nullptr>");
	if (ptrBuff == nullptr)
	{
		return DefaultNullString;
	}
	FString ret;
	const uint8 *bytes = (const uint8*)ptrBuff;
	bool bSpace = true;
	for(uint32 i=0; i<length; i++)
	{
		if (bSpace)
		{
			ret += TEXT("0x");
			bSpace = false;
		}
		//ret += FString::Printf(TEXT("%02hhX "), bytes[i]);
		ret += FString::Printf(TEXT("%02hhX"), bytes[i]);
		if ((i % 4 == 0)&&(i>0))
		{
			ret += TEXT(" ");
			bSpace = true;
		}
	}
	if (!bSpace)
	{
		ret += TEXT(" ");
	}
	return ret;
}

FString prop_StringToString(const void *ptrString)
{
	return aiString2FString( (const aiString*)ptrString, DefaultNullString );
}

FString propValueToString(const aiMaterialProperty *prop)
{
	
	if (prop == nullptr)
	{
		return TEXT("<null prop>");
	}

	const FString typeName = LookupMaterialPropertyTypeName(prop->mType);
	switch (prop->mType)
	{
		case aiPTI_Buffer:
			return bufferToString(prop->mData, prop->mDataLength);
		case aiPTI_String:
			return prop_StringToString(prop->mData);
		default:
			return FString::Printf(TEXT("Unhandled Prop Type Case: %d : (%s)"), prop->mType, *typeName);
	}
}

FString propValueToStringWithType(const aiMaterialProperty *prop)
{
	if (prop == nullptr)
	{
		return TEXT("<null prop>");
	}
	const FString typeName = LookupMaterialPropertyTypeName(prop->mType);
	return typeName + TEXT(": ") + propValueToString(prop);
}

//Originally based on RML, but substantially modified
FMeshData ProcessMesh(const FString &meshLabel, uint32 subMeshIndex, aiMesh* Mesh, const aiScene* Scene)
{
	#define LOG_INDENT ""
	
	if (Mesh == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT( LOG_INDENT "MeshFile: %s[%i]: <null> mesh!"), *meshLabel, subMeshIndex);
	}
	else
	{
		if (UMeshLoader::debugVertexStreams)
		{
			UE_LOG(LogTemp, Error, TEXT( LOG_INDENT "MeshFile: %s[%i]: %s"), *meshLabel, subMeshIndex, *aiString2FString(Mesh->mName));
		}
	}
	#define LOG_INDENT "\t"
    FMeshData MeshData;
	MeshData.SubmeshName = aiString2FString(Mesh->mName);
	bool bHavePositions = Mesh->HasPositions();
	bool bHaveNormals = Mesh->HasNormals();
	if (!bHaveNormals)
	{
		UE_LOG(LogTemp, Warning, TEXT( LOG_INDENT "EchoImporter: ProcessMesh: %s[%i]: Submesh does not define Normals"), *meshLabel, subMeshIndex);
	}
	//TODO: warn if not normals?
	int numColorChannels = Mesh->GetNumColorChannels();
	bool bHaveVertexColors = (numColorChannels > 0);
	//bool bHaveVertColor0 = false;
	bool bHaveVertexColor0 = false;

	int numTexCoords = Mesh->GetNumUVChannels();
	bool bHaveTexCoords = numTexCoords > 0;
	bool bHaveTexCoords0 = false; 
	bool bHaveTexCoords1 = false; 
	bool bHaveTexCoords2 = false; 
	bool bHaveTexCoords3 = false; 
	
	bool bHaveTangentsAndBitangents = Mesh->HasTangentsAndBitangents();

	if (numColorChannels > 1)
	{
		UE_LOG(LogTemp, Error, TEXT( LOG_INDENT "EchoImporter: ProcessMesh: %s[%i]: We currently only support one VertexColor channel; found: %d"), *meshLabel, subMeshIndex, numColorChannels);
	}

	
	if (bHaveVertexColors)
	{
		bHaveVertexColor0 = Mesh->HasVertexColors(0);
		if (!bHaveVertexColor0)
		{
			UE_LOG(LogTemp, Error, TEXT( LOG_INDENT "EchoImporter: ProcessMesh: %s[%i]: Mesh has at least one vertex color channel but doesn't have channel 0! channels: "), *meshLabel, subMeshIndex, numColorChannels);
		}
		else
		{
			if (superVerbose)
			{
				UE_LOG(LogTemp, Log, TEXT( LOG_INDENT "EchoImporter: ProcessMesh: %s[%i]: Mesh has vertexColor0 channel! "), *meshLabel, subMeshIndex);
			}
		}
	}

	
	
	
	/*
	if (numTexCoords > 1)
	{
		UE_LOG(LogTemp, Error, TEXT( LOG_INDENT "EchoImporter: ProcessMesh: %s[%i]: We currently only support one texCoord channel; found: %d"), *meshLabel, subMeshIndex, numTexCoords);
	}
	else
	*/
	if (numTexCoords <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT( LOG_INDENT "EchoImporter: ProcessMesh: %s[%i]: mesh specifies ZERO texcoord channels"), *meshLabel, subMeshIndex);
	}
	
	if (bHaveTexCoords)
	{
		bHaveTexCoords0 = Mesh->HasTextureCoords(0);
		bHaveTexCoords1 = Mesh->HasTextureCoords(1);
		bHaveTexCoords2 = Mesh->HasTextureCoords(2);
		bHaveTexCoords3 = Mesh->HasTextureCoords(3);
		const uint32 howManyDeclared = Mesh->GetNumUVChannels();
		uint32 seenMany = 0
			+ (bHaveTexCoords0 ? 1 : 0)
			+ (bHaveTexCoords1 ? 1 : 0)
			+ (bHaveTexCoords2 ? 1 : 0)
			+ (bHaveTexCoords3 ? 1 : 0)
		;
		if (howManyDeclared > 4)
		{
			UE_LOG(LogTemp, Error, TEXT( LOG_INDENT "EchoImporter: ProcessMesh: %s[%i]: Max of 4 UV channels supported. Mesh seems to have %i texCoords declared"), *meshLabel, subMeshIndex, howManyDeclared);
			//UE_LOG(LogTemp, Error, TEXT("EchoImporter: WARNING: Max of 4 UV channels supported. Mesh seems to have %i texCoords declared"), 
		}
		if (seenMany != howManyDeclared)
		{
			UE_LOG(LogTemp, Error, TEXT( LOG_INDENT "EchoImporter: ProcessMesh: %s[%i]: Mesh seems to have %i texCoords declared, but we only saw %i. t0=%s, t1=%s, t2=%s, t3=%s"), *meshLabel, subMeshIndex, 
			howManyDeclared, seenMany, *bool2String(bHaveTexCoords0), *bool2String(bHaveTexCoords1), *bool2String(bHaveTexCoords2), *bool2String(bHaveTexCoords3)
			);
		}
		//TODO: warn if lack texcoords or lack coords in expected indicies?
	}

	if (!bHaveTexCoords0)
	{
		UE_LOG(LogTemp, Warning, TEXT( LOG_INDENT "EchoImporter: ProcessMesh: %s[%i]: Model does not specify texCoord0, but specifies at least one texCoord channels: "), *meshLabel, subMeshIndex, numTexCoords);
		int found = 0;
		const uint32 maxSearchLen= 32;
		for(uint32 i=0; i<maxSearchLen; i++)
		{
			if (Mesh->HasTextureCoords(i))
			{
				found++;
				UE_LOG(LogTemp, Warning, TEXT( LOG_INDENT "\tHas Channel[%i]"), i);
				if (found >= numTexCoords)
				{
					break;
				}
			}
		}
		if (found < numTexCoords)
		{
			UE_LOG(LogTemp, Error, TEXT( LOG_INDENT "\tUnsure which channesl are specified, as found: %i channels searching 0~(%i-1)"), found, maxSearchLen);
		}
	}

	//warn about future tasks missing since was missing vertex colors silently
	if (Mesh->HasBones())
	{
		UE_LOG(LogTemp, Error, TEXT( LOG_INDENT "EchoImporter: ProcessMesh: %s[%i]: Bones not yet supported!"), *meshLabel, subMeshIndex);
	}
	bool bHaveFaces = Mesh->HasFaces();
	if (!bHaveFaces)
	{
		//UE_LOG(LogTemp, Error, TEXT( LOG_INDENT "EchoImporter: ProcessMesh: %s[%i]: Faces not yet supported!"), *meshLabel, subMeshIndex);
		UE_LOG(LogTemp, Error, TEXT( LOG_INDENT "EchoImporter: ProcessMesh: %s[%i]: Unsupported faces are required!"), *meshLabel, subMeshIndex);
	}

	if (!bHavePositions)
	{
		UE_LOG(LogTemp, Error, TEXT( LOG_INDENT "EchoImporter: ProcessMesh: %s[%i]: Unsupported: Does not have Positions!"), *meshLabel, subMeshIndex);
	}
	//TODO: soft? fail if missing position/faces?

	//static int printMaxVerts = 64;//48 expected for test model
	int printMaxVerts = 64;//48 expected for test model
	if (UMeshLoader::debugVertexStreams)
	{
		UE_LOG(LogTemp, Error, TEXT( LOG_INDENT "VERTEX COUNT=%i"), Mesh->mNumVertices);
	}
	#define LOG_INDENT "\t\t"
	#define LOG_INDENT2 LOG_INDENT "\t"

	
	bool bNonBlackVertexColorFound = true;//default assumption
	//if (hackVertexColorsV2)
	if (UMeshLoader::allowBadVertexColorDetectionAndOmission)
	{
		if (bHaveVertexColor0)
		{
			bNonBlackVertexColorFound = false;//if this hack is enabled
			for (uint32 j = 0; j < Mesh->mNumVertices; ++j)
			{
				const aiColor4D &color = Mesh->mColors[0][j];
				//if ((color.r == 0)&&(color.g==0)&&(color.b==0)&&((color.a==0)||(color.a==1)))
				const float MinValue = 0.01f;
				if ((color.r <= MinValue)&&(color.g <= MinValue)&&(color.b <= MinValue)&&((color.a<=MinValue)||(color.a==1)))
				{
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("Mesh %s[%i]: Counterexample @ vertex[%i]: (%f, %f, %f, %f)"), *meshLabel, subMeshIndex, j, color.r, color.g, color.b, color.a); 
					bNonBlackVertexColorFound = true;
					break;
				}
			}
			if (!bNonBlackVertexColorFound)
			{
				UE_LOG(LogTemp, Error, TEXT("Mesh %s[%i]: WARNING: Mesh vertex color channel is filled with (0,0,0,0) or (0,0,0,1). Ignoring because hackVertexColorsV2 is true!"), *meshLabel, subMeshIndex); 
				bHaveVertexColor0 = false; //HACK
			}
		}
	}

	//TODO: split these into several loops instead of branching everywhere inside a potentially huge inner loop
	for (uint32 j = 0; j < Mesh->mNumVertices; ++j)
	{
		if (UMeshLoader::debugVertexStreams)
		{
			if (printMaxVerts>0)
			{
				printMaxVerts--;
			}
		}
		const bool bPrintVert = UMeshLoader::debugVertexStreams && (printMaxVerts > 0);
		if (bPrintVert)
		{
			UE_LOG(LogTemp, Log, TEXT( LOG_INDENT "Vertex: %i"), j);
		}
		FVector Vertex = FVector::ZeroVector;
		if (bHavePositions)
		{
			Vertex = FVector(Mesh->mVertices[j].x, Mesh->mVertices[j].y, Mesh->mVertices[j].z);
			if (bPrintVert) UE_LOG(LogTemp, Log, TEXT( LOG_INDENT2 "SV_Position: (%f, %f, %f)"), Vertex.X, Vertex.Y, Vertex.Z); 
		}
		MeshData.Vertices.Add(Vertex);

		FVector Normal = FVector::ZeroVector;
		if (bHaveNormals)
		{
		    Normal = FVector(Mesh->mNormals[j].x, Mesh->mNormals[j].y, Mesh->mNormals[j].z);
			if (bPrintVert) UE_LOG(LogTemp, Log, TEXT( LOG_INDENT2 "MS_Normal: (%f, %f, %f)"), Normal.X, Normal.Y, Normal.Z); 
		}

		MeshData.Normals.Add(Normal);
		//if (Mesh->mTextureCoords[0])

		//TODO: add support for uv2/uv3/uv4 etc?
		//note: AI_MAX_NUMBER_OF_TEXTURECOORDS is 0x8? do we need uv4~uv7?

		//TODO: what about UVW type coords? etc?
		if (bHaveTexCoords0)
		{
			FVector2D texCoord = FVector2D(static_cast<float>(Mesh->mTextureCoords[0][j].x), 1.f-static_cast<float>(Mesh->mTextureCoords[0][j].y));
			if (bPrintVert) UE_LOG(LogTemp, Log, TEXT( LOG_INDENT2 "TexCoord0: (%f, %f)"), texCoord.X, texCoord.Y); 
		    MeshData.UVs0.Add(texCoord);
		}
		if (bHaveTexCoords1)
		{
			FVector2D texCoord = FVector2D(static_cast<float>(Mesh->mTextureCoords[1][j].x), 1.f-static_cast<float>(Mesh->mTextureCoords[1][j].y));
			if (bPrintVert) UE_LOG(LogTemp, Log, TEXT( LOG_INDENT2 "TexCoord1: (%f, %f)"), texCoord.X, texCoord.Y); 
		    MeshData.UVs1.Add(texCoord);
		}
		if (bHaveTexCoords2)
		{
			FVector2D texCoord = FVector2D(static_cast<float>(Mesh->mTextureCoords[2][j].x), 1.f-static_cast<float>(Mesh->mTextureCoords[2][j].y));
			if (bPrintVert) UE_LOG(LogTemp, Log, TEXT( LOG_INDENT2 "TexCoord2: (%f, %f)"), texCoord.X, texCoord.Y); 
		    MeshData.UVs2.Add(texCoord);
		}
		if (bHaveTexCoords3)
		{
			FVector2D texCoord = FVector2D(static_cast<float>(Mesh->mTextureCoords[3][j].x), 1.f-static_cast<float>(Mesh->mTextureCoords[3][j].y));
			if (bPrintVert) UE_LOG(LogTemp, Log, TEXT( LOG_INDENT2 "TexCoord3: (%f, %f)"), texCoord.X, texCoord.Y); 
		    MeshData.UVs3.Add(texCoord);
		}

		if (bHaveVertexColor0)
		{
			const aiColor4D &sourceColor = Mesh->mColors[0][j];
			/*if (color.g > 0)
			{
				int breakHere=1;
			}
			*/
			//if (bPrintVert)
			//if (bPrintVert) UE_LOG(LogTemp, Error, TEXT( LOG_INDENT "\tVERTEX COLOR[%d]: (%f, %f, %f, %f)"), j, color.r, color.g, color.b, color.a);
			//if (bPrintVert) UE_LOG(LogTemp, Error, TEXT( LOG_INDENT "\tVERTEX COLOR[%d]: (%f, %f, %f, %f)"), j, color.r, color.g, color.b, color.a);
			FLinearColor outColor = FLinearColor(sourceColor.r, sourceColor.g, sourceColor.b, sourceColor.a);
			if (bPrintVert) UE_LOG(LogTemp, Log, TEXT( LOG_INDENT2 "Color: (%f, %f, %f, %f)"), sourceColor.r, sourceColor.g, sourceColor.b, sourceColor.a); 
			
			
			if (hackVertexColors)
			{
				const MagicVertexColors *hack = (const MagicVertexColors*)(&sourceColor);//Mesh->mColors[0][j]);
				if (bPrintVert)
				{
					UE_LOG(LogTemp, Log, TEXT( LOG_INDENT2 "Color[ushortHax]: (%hu, %hu, %hu, %hu)"), hack->colors[0], hack->colors[1], hack->colors[2], hack->colors[3]);
				}
				//const uint16_t MaxValue = std::numeric_limits<uint16>::max();
				
				outColor = ConvertToLinearColor(*hack);
			}
		    MeshData.Colors.Add(outColor);
		}

		
		if (bHaveTangentsAndBitangents)
		{
			FProcMeshTangent Tangent = FProcMeshTangent(Mesh->mTangents[j].x, Mesh->mTangents[j].y, Mesh->mTangents[j].z);
			if (bPrintVert) 
			{
				UE_LOG(LogTemp, Log, TEXT( LOG_INDENT2 "TangentBitangent: (%f, %f, %f : Flip? %s)"), Tangent.TangentX.X, Tangent.TangentX.Y, Tangent.TangentX.Z, Tangent.bFlipTangentY ? TEXT("True") : TEXT("False"));
			}
			MeshData.Tangents.Add(Tangent);
		}	
	}
	
	if (bHaveFaces)
	{
		//UE_LOG(LogTemp, Log, TEXT("mNumFaces: %d"), Mesh->mNumFaces);
		for (uint32 i = 0; i < Mesh->mNumFaces; i++)
		{
			aiFace Face = Mesh->mFaces[i];
			for (uint32 f = 0; f < Face.mNumIndices; f++)
			{
				MeshData.Triangles.Add(Face.mIndices[f]);
			}
		}
	}

	MeshData.UseMaterialIndex = Mesh->mMaterialIndex;
	return MeshData;
}

void ProcessNode(const FString &meshLabel, aiNode* Node, const aiScene* Scene, int ParentNodeIndex, int* CurrentIndex, FFinalReturnData* FinalReturnData)
{
    FNodeData NodeData;
	NodeData.NodeParentIndex = ParentNodeIndex;
	
	NodeData.NodeName = aiString2FString(Node->mName);

	aiMatrix4x4 TempTrans = Node->mTransformation;
	FMatrix tempMatrix;
	tempMatrix.M[0][0] = TempTrans.a1; tempMatrix.M[0][1] = TempTrans.b1; tempMatrix.M[0][2] = TempTrans.c1; tempMatrix.M[0][3] = TempTrans.d1;
	tempMatrix.M[1][0] = TempTrans.a2; tempMatrix.M[1][1] = TempTrans.b2; tempMatrix.M[1][2] = TempTrans.c2; tempMatrix.M[1][3] = TempTrans.d2;
	tempMatrix.M[2][0] = TempTrans.a3; tempMatrix.M[2][1] = TempTrans.b3; tempMatrix.M[2][2] = TempTrans.c3; tempMatrix.M[2][3] = TempTrans.d3;
	tempMatrix.M[3][0] = TempTrans.a4; tempMatrix.M[3][1] = TempTrans.b4; tempMatrix.M[3][2] = TempTrans.c4; tempMatrix.M[3][3] = TempTrans.d4;
	NodeData.RelativeTransformTransform = FTransform(tempMatrix);

	//TODO: should we restructure this to not duplicate/reparse the mesh stuff?
    for (uint32 n = 0; n < Node->mNumMeshes; n++)
    {
		uint32 MeshIndex = Node->mMeshes[n];
		/*
		if (superVerbose)
		{
			UE_LOG(LogTemp, Log, TEXT("Loading Mesh at index: %d"), MeshIndex);
		}
		*/
        aiMesh* Mesh = Scene->mMeshes[MeshIndex];
		NodeData.Meshes.Add(ProcessMesh(meshLabel, MeshIndex, Mesh, Scene));
    }

	FinalReturnData->Nodes.Add(NodeData);
	/*
	if (superVerbose)
	{
		UE_LOG(LogTemp, Log, TEXT("mNumMeshes: %d, mNumChildren of Node: %d"), Node->mNumMeshes, Node->mNumChildren);
	}
	*/
	int CurrentParentIndex = *CurrentIndex;
	for (uint32 n = 0; n < Node->mNumChildren; n++)
	{
		(*CurrentIndex)++;
	    ProcessNode(meshLabel, Node->mChildren[n], Scene, CurrentParentIndex, CurrentIndex, FinalReturnData);
	}
}


//type helper is for easy template usage
template<typename T>
//bool ResolveMatPropCount(aiMaterialProperty *prop, unsigned int &ret, const T &typeHelper, bool bSilent = false)
bool ResolveMatPropCount(aiMaterialProperty *prop, size_t &ret, const T &typeHelper, bool bSilent = false)
{
	ret = 0;
	if (prop == nullptr)
	{
		return false;
	}
	if (prop->mData == nullptr)
	{
		//paranoia!
		return false;
	}

	size_t sz = sizeof(T);
	size_t dataLen = prop->mDataLength;
	//static_assert(sizeof(unsigned int) == sizeof(size_t), "Unexpected issue size_t vs unsigned int"));

	//Gah, actually unsigned long long vs unsigned int, but should be fine?
	// https://stackoverflow.com/questions/4021981/use-static-assert-to-check-types-passed-to-macro
	//static_assert(std::is_same<decltype(prop->mDataLength), decltype(sizeof(size_t))>::value, "Expected size_t to be unsigned int. If not, suble logic bugs may exist");

	if (dataLen % sz != 0) //not a multiple, probably a bad type
	{
		if (!bSilent)
		{
			UE_LOG(LogTemp, Error, TEXT("\t\t\tBadType? dataLen(%i) %% sizeof(T)(%i) != 0: %i"), dataLen, sz, dataLen % sz);
		}
		return false;
	}
	if (dataLen < sz)
	{
		if (!bSilent)
		{
			UE_LOG(LogTemp, Error, TEXT("\t\t\tTooSmall? dataLen(%i) < sizeof(T)(%i) != 0: %i"), dataLen, sz, dataLen % sz);
		}
		return false;
	}
	ret = dataLen / sz;
	return true;
}

//template<typename t>
//template<typename T>
//T ResolveMatProp(aiMaterialProperty *prop, const T &defaultValue)
//bool ResolveMatProp(aiMaterialProperty *prop, const T &defaultValue, T &ret)
//bool ResolveMatProp(aiMaterialProperty *prop, const T &defaultValue, TArray<T> &ret)
template<typename T>
//bool ResolveMatProp(aiMaterialProperty *prop, const T &defaultValue, T &ret, unsigned int index)
bool ResolveMatProp(aiMaterialProperty *prop, const T &defaultValue, T &ret, size_t index)
{
	//ret = defaultValue;
	if (prop == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("\t\t\tPROP IS nullptr!"));
		//return defaultValue;
		return false;
	}
	//if (prop->mDataLength != sizeof(T))
	if (prop->mDataLength < sizeof(T))
	{
		UE_LOG(LogTemp, Error, TEXT("\t\t\tBuffer too small! need: %i, have %i"), sizeof(T), prop->mDataLength);
		//return defaultValue;
		return false;
	}
	else if (prop->mDataLength % sizeof(T) != 0)
	{
		UE_LOG(LogTemp, Error, TEXT("\t\t\tBuffer not divisible by %i! dataLength=%i"), sizeof(T), prop->mDataLength);
		//return defaultValue;
		return false;
	}
	else
	{
		
		unsigned int numElements = prop->mDataLength / sizeof(T);
		//T ret = defaultValue;
		T *tPtr = (T*)prop->mData;
		//if ((index < 0)||(index>=numElements))
		if (tPtr == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("\t\t\tProp mData is nullptr!"));
			return false;
		}
		//static_assert(std::is_unsigned<decltype(index)>, "Expected index to be unsigned");
		// static_assert(std::is_unsigned<decltype(index)>, "Expected index to be unsigned"); //GAH
		// 
		//written oddly because index is expected to be an unsigned type
		if ((index>=numElements)||(index<0))
		{
			UE_LOG(LogTemp, Error, TEXT("\t\t\tIndex out of bounds: %i, count=%i"), index, numElements);
			return false;
		}
		tPtr += index;
		{
			ret = *tPtr;
			return true;
		}
		//return ret;
		return false;
	}
}


//using PostProcessReturnType = std::pair<bool, const aiScene*>;

//std::pair<bool, const aiScene*> RunPostProcesStep(Assimp::Importer &importer, const FString &filename, unsigned int runFlags)
//PostProcessReturnType RunPostProcesStep(Assimp::Importer &importer, const FString &filename, unsigned int runFlags)
using ScenePtr = const aiScene *;
bool RunPostProcesStep(Assimp::Importer &importer, const FString &filename, unsigned int runFlags, ScenePtr &outScene)
{
	//outScene = nullptr;
	//unsigned int runPostProcessingSteps = runFlags;//StripNormalsFlags;
	//const aiScene *Scene = importer.ApplyPostProcessing(runFlags);
	outScene = importer.ApplyPostProcessing(runFlags);
	const char *strError = importer.GetErrorString();

	//if (Scene == nullptr)
	if (outScene == nullptr)
	{
		//FString msg = cStringToFString(importer.GetErrorString());
		FString msg = cStringToFString(strError);
		//UE_LOG(LogTemp, Error, TEXT("Importer Failed after PostProcessing step(s) (%#x)! File: %s"), runFlags, *filename);
		//UE_LOG(LogTemp, Error, TEXT("Importer Failed after PostProcessing step(s) (%#x)! File: %s; Message = %s"), runFlags, *filename, *msg);
		UE_LOG(LogTemp, Error, TEXT("Importer: File: %s: FAILED Importing while running postprocessing flags (%#x): %s"), *filename, runFlags, *msg);
		//return PostProcessReturnType(false, Scene);
		return false;
	}

	
	//NB: should never be null per spec, but going to be paranoid
	if ((strError != nullptr) && (strlen(strError) > 0))
	{
		//Check for error messages, if present and dutifully print them for inspection
		FString msg = cStringToFString(strError);
		UE_LOG(LogTemp, Error, TEXT("Importer: File: %s: Error Messages while running postprocessing flags (%#x): %s"), *filename, runFlags, *msg);
	}
	//return PostProcessReturnType(true, Scene);
	return true;
}

const FString kMapModeU(TEXT(_AI_MATKEY_MAPPINGMODE_U_BASE));
const FString kMapModeV(TEXT(_AI_MATKEY_MAPPINGMODE_V_BASE));
const FString kUVWSrc(TEXT(_AI_MATKEY_UVWSRC_BASE));

bool FinishLoadMeshFromImporterAndScene(const FString &filenameLabel, Assimp::Importer &importer, const aiScene *Scene, FFinalReturnData &result)
{
	{
		//extra importer steps
		
		//int runPostProcessingSteps;

		#define RUN_POSTSTEP( withFlags ) \
			if (!RunPostProcesStep(importer, filenameLabel, (withFlags), Scene)) { return false; }

		//if (bStripNormals)
		if (UMeshLoader::stripNormals)
		{
			//bool ok;
			RUN_POSTSTEP(StripNormalsFlags);

			/*
			runPostProcessingSteps = StripNormalsFlags;
			Scene = importer.ApplyPostProcessing(runPostProcessingSteps);
			if (Scene == nullptr)
			{
				UE_LOG(LogTemp, Error, TEXT("Importer Failed after extra postprocessing steps (%#x)! File: %s"), runPostProcessingSteps, *filenameLabel);
				return false;
			}
			*/
			//if (bGenerateNormalsAfterStripping)// ((runPostProcessingSteps & aiProcess_RemoveComponent) != 0)
		}
		if (UMeshLoader::generateNormals || UMeshLoader::generateSmoothNormals)
		{
			//prefer smooth normals if both set
			RUN_POSTSTEP(UMeshLoader::generateSmoothNormals ? FixSmoothNormalsFlags : FixNormalsFlags);

			/*
			ok = RunPostProcesStep(importer, filenameLabel, FixNormalsFlags, Scene);
			if (!ok)
			{
				return false;
			}
			*/
			/*
			runPostProcessingSteps = FixNormalsFlags;
			Scene = importer.ApplyPostProcessing(runPostProcessingSteps);
			if (Scene == nullptr)
			{
				UE_LOG(LogTemp, Error, TEXT("Importer Failed after extra postprocessing steps (%#x)! File: %s"), runPostProcessingSteps, *filenameLabel);
				return false;
			}
			*/
		}
		//}

		/*
		ok = RunPostProcesStep(importer, filenameLabel, GenerateTangentsFlags, Scene);
		if (!ok)
		{
			return false;
		}
		*/
		RUN_POSTSTEP(GenerateTangentsFlags);

		//this has to be done regardless. It was originally done in one single step
		/*
		runPostProcessingSteps = FinishImportFlags;
		Scene = importer.ApplyPostProcessing(runPostProcessingSteps);
		if (Scene == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("Importer Failed after extra postprocessing steps (%#x)! File: %s"), runPostProcessingSteps, *filenameLabel);
			return false;
		}
		*/

		//final step - weird things happen if this is combined with previous steps
		RUN_POSTSTEP(FinishImportFlags);
	}

	///////////////////////////////////////////////////////////////////////
	//const char *errorString = mImporter.GetErrorString();
	const char *errorString = importer.GetErrorString();
	int errorStrlen = 0;
	if (errorString != nullptr)
	{
		errorStrlen = strlen(errorString);
	}
	FString fstr = L"";
	if (errorString != nullptr)
	{
		fstr = FString(ANSI_TO_TCHAR(errorString));
	}

	if (Scene != nullptr)
	{
		if (fstr.Len() > 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("MeshImport: %s: %s"), *filenameLabel, *fstr);
		}
	}
	else
	{
		if (fstr.Len() > 0)
		{
			UE_LOG(LogTemp, Error, TEXT("MeshImport: ERROR in %s: %s"), *filenameLabel, *fstr);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("MeshImport: ERROR in %s: <no error string given>"), *filenameLabel);
		}
	}
	///////////////////////////////////////////////////////////////////////
	
	if (Scene == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Runtime Mesh Loader: Read mesh file failure.\n"));
		//return ReturnData;
		result.Success = false;
		return false;
	}

	if (!Scene->HasMeshes())
	{
		UE_LOG(LogTemp, Error, TEXT("ImportModel: '%s': Importer: Scene has NO MESHES!"), *filenameLabel);
	}
	else
	{
		#define MAT_PROP_CASE(propVar, caseLabel, t_valueType, l_defaultValue, s_formatSpec, s_caption) \
			case caseLabel: \
				{ \
					t_valueType myValue = l_defaultValue; \
					size_t numElements = 0; \
					bool arrayOk = ResolveMatPropCount(propVar, numElements, l_defaultValue); \
					if ((!arrayOk) || (numElements == 0)) \
					{ \
						UE_LOG(LogTemp, Error, TEXT("\t\t\tBad Prop Pointer!")); \
					} \
					else \
					{ \
						for(size_t iArrayHelper=0; iArrayHelper<numElements; iArrayHelper++) \
						{ \
							bool ok; \
							ok = ResolveMatProp(propVar, (l_defaultValue), myValue, iArrayHelper); \
							if (!ok) \
							{ \
								UE_LOG(LogTemp, Error, TEXT("\t\t\t[%d]: BAD PROP!"), iArrayHelper); \
							} \
							else \
							{ \
								UE_LOG(LogTemp, Log, L"\t\t\t[%d]: %s: "#s_formatSpec, iArrayHelper, s_caption, myValue); \
							} \
						} \
					} \
				} \
				break;
		////////////////////////////////////////
		{
			if (Scene->HasAnimations())
			{
				UE_LOG(LogTemp, Warning, TEXT("WARNING: Imported Model %s has Animation data which is not yet supported."), *filenameLabel);
			}
		}
		////////////////////////////////////////
		{
			//Scene->GetEmbeddedTexture
			if (Scene->HasMaterials())
			{
				for(uint32 iMat=0; iMat<Scene->mNumMaterials; iMat++)
				{
					aiMaterial *mat = Scene->mMaterials[iMat];
					if (mat != nullptr)
					{
						
						//if (UMeshLoader::importerVerbose)
						if (UMeshLoader::debugMaterialInfo)
						{
							//mat->GetName not defined!? - looks like was some weird holdover from the old dll?
							UE_LOG(LogTemp, Log, TEXT("\tMat[%d]: %s"), iMat, *aiString2FString(mat->GetName()));
							
							/*
							//Dump Raw
							if (false)
							{
								for(unsigned int iMatProp=0; iMatProp<mat->mNumProperties; iMatProp++)
								{
									aiMaterialProperty *prop = mat->mProperties[iMatProp];
									if (prop != nullptr)
									{
										#define LOG_INDENT "\t\t"
										
										if ((prop->mSemantic != aiTextureType_NONE) || (prop->mIndex != 0))
										{
											UE_LOG(LogTemp, Error, TEXT(LOG_INDENT "Prop[%d]: T_%d(T: %s) '%s' [%d] : S_%d(S: %s)"), 
												iMatProp, 
												prop->mType, *LookupMaterialPropertyTypeName(prop->mType), 
												*aiString2FString(prop->mKey), prop->mIndex, 
												prop->mSemantic, *LookupTextureTypeSemanticName(prop->mSemantic)
											);
										}
										else
										{
											//UE_LOG(LogTemp, Error, TEXT(LOG_INDENT "Prop[%d]: T_%d(T: %s) '%s' [%d] : S_%d(S: %s)"), 
											UE_LOG(LogTemp, Error, TEXT(LOG_INDENT "Prop[%d]: T_%d(T: %s) '%s' [%d]"), 
												iMatProp, 
												prop->mType, *LookupMaterialPropertyTypeName(prop->mType), 
												*aiString2FString(prop->mKey)
												//, prop->mIndex, 
												//prop->mSemantic, *LookupTextureTypeSemanticName(prop->mSemantic)
											);
										}
										#define LOG_INDENT "\t\t\t"
										
										switch(prop->mType)
										{
											MAT_PROP_CASE(prop, aiPTI_Float, float, 0.0f, "%f", L"float"); 
											MAT_PROP_CASE(prop, aiPTI_Integer, int, 0, "%d", L"int"); 
											MAT_PROP_CASE(prop, aiPTI_Double, double, 0.0, "%lf", L"double"); 
									
											case aiPTI_String:
												{
													FString str=L"<null>";
													aiString *stringPtr = (aiString*)prop->mData;
													if (stringPtr != nullptr)
													{
														str = aiString2FString(stringPtr);
													}
													else
													{
														//ok = 
														str = TEXT("null");
													}
													UE_LOG(LogTemp, Error, TEXT( LOG_INDENT "string: '%s'"), *str);
												}
												break;

											case aiPTI_Buffer:
												{
													//TODO: print hex string of byte dump
													UE_LOG(LogTemp, Error, TEXT( LOG_INDENT "[Buffer]: ptr=0x%x, len=%d"), prop->mData, prop->mDataLength);
												}
												break;

											default:
												{
													UE_LOG(LogTemp, Error, TEXT( LOG_INDENT "Unandled type: %d, ptr=0x%x, len=%d"), prop->mType, prop->mData, prop->mDataLength);
												}
												break;
										}//end switch for print
										
									}
								}
							}//end if raw
							*/

							if (true)
							{
								for(unsigned int iMatProp=0; iMatProp<mat->mNumProperties; iMatProp++)
								{
									aiMaterialProperty *prop = mat->mProperties[iMatProp];
									if (prop != nullptr)
									{
										#define LOG_INDENT "\t\t"
										FString prefix;
										if ((prop->mSemantic != aiTextureType_NONE) || (prop->mIndex != 0))
										{
											prefix = FString::Printf(TEXT("Prop[%d]: T_%d(T: %s) '%s' [%d] : S_%d(S: %s)"), iMatProp, 
												prop->mType, *LookupMaterialPropertyTypeName(prop->mType), 
												*aiString2FString(prop->mKey), prop->mIndex, 
												prop->mSemantic, *LookupTextureTypeSemanticName(prop->mSemantic)
											);
											/*
											UE_LOG(LogTemp, Error, TEXT(LOG_INDENT "Prop[%d]: T_%d(T: %s) '%s' [%d] : S_%d(S: %s)"), 
												iMatProp, 
												prop->mType, *LookupMaterialPropertyTypeName(prop->mType), 
												*aiString2FString(prop->mKey), prop->mIndex, 
												prop->mSemantic, *LookupTextureTypeSemanticName(prop->mSemantic)
											);
											*/
										}
										else
										{
											prefix = FString::Printf(TEXT("Prop[%d]: T_%d(T: %s) '%s'"), //" [%d] : S_%d(S: %s)"), 
												iMatProp, 
												prop->mType, *LookupMaterialPropertyTypeName(prop->mType), 
												*aiString2FString(prop->mKey)
												//, prop->mIndex, prop->mSemantic, *LookupTextureTypeSemanticName(prop->mSemantic)
											);
											/*
											//UE_LOG(LogTemp, Error, TEXT(LOG_INDENT "Prop[%d]: T_%d(T: %s) '%s' [%d] : S_%d(S: %s)"), 
											UE_LOG(LogTemp, Error, TEXT(LOG_INDENT "Prop[%d]: T_%d(T: %s) '%s' [%d]"), 
												iMatProp, 
												prop->mType, *LookupMaterialPropertyTypeName(prop->mType), 
												*aiString2FString(prop->mKey)
												//, prop->mIndex, 
												//prop->mSemantic, *LookupTextureTypeSemanticName(prop->mSemantic)
											);
											*/
										}
										
										bool bSkip = false;
										switch(prop->mType)
										{
											case aiPTI_Float:
												{
													size_t count = 0;
													const float *floatPtr = (const float*)prop->mData;
													if (floatPtr != nullptr)
													{
														const bool Silently = true;
														if (ResolveMatPropCount<float>(prop, count, 0.0f, Silently))
														{
															//TODO: custom format specifier for %f??
															bSkip = true;
															if (count == 1)
															{
																UE_LOG(LogTemp, Log, TEXT(LOG_INDENT "%s: %f"), *prefix, floatPtr[0]);
															}
															else if (count == 4)
															{
																UE_LOG(LogTemp, Log, TEXT(LOG_INDENT "%s: (%f, %f, %f, %f)"), *prefix, floatPtr[0], floatPtr[1], floatPtr[2], floatPtr[3]);
															}
															else
															{
																bSkip = false; //not handled
															}
														}
													}
												}
												break;
											case aiPTI_Integer:
												{
													//TODO: ensure matches AssImp's int type?
													size_t count = 0;
													const int *arrayPtr = (const int*)prop->mData;
													if (arrayPtr != nullptr)
													{
														const bool Silently = true;
														if (ResolveMatPropCount<int>(prop, count, 0, Silently))
														{
															//TODO: custom format specifier for %f??
															bSkip = true;
															if (count == 1)
															{
																UE_LOG(LogTemp, Log, TEXT(LOG_INDENT "%s: %d"), *prefix, arrayPtr[0]);
															}
															/*
															else if (count == 4)
															{
																UE_LOG(LogTemp, Log, TEXT(LOG_INDENT "%s: (%d, %d, %d, %d)"), *prefix, arrayPtr[0], arrayPtr[1], arrayPtr[2], arrayPtr[3]);
															}
															*/
															else
															{
																bSkip = false; //not handled
															}
														}
													}
												}
												break;
											case aiPTI_Buffer:
												{
													//TODO: ensure matches AssImp's int type?
													size_t count = 0;
													const uint8 *arrayPtr = (uint8*)prop->mData;
													if (arrayPtr != nullptr)
													{
														bSkip = true;
														count = prop->mDataLength;//assuming this should be num bytes?
														if (count == 1)
														{
															//UE_LOG(LogTemp, Log, TEXT(LOG_INDENT "%s: 0x%2x"), *prefix, arrayPtr[0]);
															UE_LOG(LogTemp, Log, TEXT(LOG_INDENT "%s: %#02x"), *prefix, arrayPtr[0]);
														}
														else if (count == 4)
														{
															static_assert(sizeof(uint32) == 4, "Uint32 not 4 bytes!");
															//UE_LOG(LogTemp, Error, TEXT(LOG_INDENT "%s: 0x%8x"), *prefix, ((const uint32*)arrayPtr)[0]);
															UE_LOG(LogTemp, Log, TEXT(LOG_INDENT "%s: 0x%#08x"), *prefix, ((const uint32*)arrayPtr)[0]);
														}
														else
														{
															UE_LOG(LogTemp, Log, TEXT(LOG_INDENT "%s: %s"), *prefix, *bufferToString(prop->mData, prop->mDataLength));
														}
														/*
														else if (count == 4)
														{
															UE_LOG(LogTemp, Error, TEXT(LOG_INDENT "%s: (%d, %d, %d, %d)"), *prefix, arrayPtr[0], arrayPtr[1], arrayPtr[2], arrayPtr[3]);
														}
														*/
														/*
														else
														{
															bSkip = false; //not handled
														}
														*/
													}
												}
												break;

											case aiPTI_String:
												{
													const aiString *stringPtr = (const aiString*)prop->mData;
													if (stringPtr != nullptr)
													{
														//TODO: if convert to string safely?
														FString stringAsUEString = aiString2FString(*stringPtr);
														UE_LOG(LogTemp, Log, TEXT(LOG_INDENT "%s: '%s'"), *prefix, *stringAsUEString);
														bSkip = true;
													}
												}
												break;
											default:
												bSkip = false;
												break; //use fallback logic below
										}

										
										
										
										if (!bSkip)
										{
											//#define LOG_INDENT_OUTER "\t\t"
											//UE_LOG(LogTemp, Error, TEXT(LOG_INDENT_OUTER "%s"), *prefix);
											UE_LOG(LogTemp, Error, TEXT(LOG_INDENT "%s"), *prefix);

											#define LOG_INDENT "\t\t\t"
											
											switch(prop->mType)
											{
												MAT_PROP_CASE(prop, aiPTI_Float, float, 0.0f, "%f", L"float"); 
												MAT_PROP_CASE(prop, aiPTI_Integer, int, 0, "%d", L"int"); 
												MAT_PROP_CASE(prop, aiPTI_Double, double, 0.0, "%lf", L"double"); 
									
												case aiPTI_String:
													{
														FString str=L"<null>";
														aiString *stringPtr = (aiString*)prop->mData;
														if (stringPtr != nullptr)
														{
															str = aiString2FString(stringPtr);
														}
														else
														{
															//ok = 
															str = TEXT("null");
														}
														UE_LOG(LogTemp, Error, TEXT( LOG_INDENT "string: '%s'"), *str);
													}
													break;
												case aiPTI_Buffer:
													{
														//TODO: print hex string of byte dump
														UE_LOG(LogTemp, Error, TEXT( LOG_INDENT "[Buffer]: ptr=0x%x, len=%d"), prop->mData, prop->mDataLength);
													}
													break;

												default:
													{
														UE_LOG(LogTemp, Error, TEXT( LOG_INDENT "Unandled type: %d, ptr=0x%x, len=%d"), prop->mType, prop->mData, prop->mDataLength);
													}
													break;
											}//end switch for print
										}//if !bSkip
									}
								}
							}//cleaner output
							//}//end if verbose
						} //if (UMeshLoader::debugMaterialInfo)


						FEchoImportMaterialData echoMat;
						//TODO: ignore scalars/vectors from texture stuffs!
						//TArray<FEchoImportMaterialTexture> importTextures;
						using EchoImpTextureIndex = uint32;
						TArray<std::tuple<EchoImpTextureIndex,EchoImpTextureIndex>> foundTextures;
						
						for(unsigned int iMatProp=0; iMatProp<mat->mNumProperties; iMatProp++)
						{
							aiMaterialProperty *prop = mat->mProperties[iMatProp];
							if (prop != nullptr)
							{
								bool bTextureProp = (prop->mSemantic != aiTextureType_NONE);
								if (bTextureProp)
								{
									if (prop->mSemantic != aiTextureType_NONE)
									{
										bool bFound = false;
										for(const auto &pair: foundTextures)
										{
											auto fIndex = std::get<0>(pair);
											auto fSemantic = std::get<1>(pair);
											if (fIndex == prop->mIndex)
											{
												if (prop->mSemantic != aiTextureType_NONE)
												{
													if (fSemantic == prop->mSemantic)
													{
														bFound = true;
													}
													else
													{
														//this indeed happens normally
														//UE_LOG(LogTemp, Warning, TEXT( LOG_INDENT "Found multiple semantics for texture index %d: existing: %d, new: %d"), fIndex, fSemantic, prop->mSemantic);
													}
												}
											}
										}
										if (!bFound)
										{
											foundTextures.Push({(int32)prop->mIndex, (int32)prop->mSemantic}); //nice
										}
									}
								}
								else
								{
									#define LOG_INDENT "\t\t"
									static const FName kMatNameProp(TEXT("?mat.name"));
									FName propName( aiPropToFName( prop ) );
									switch(prop->mType)
									{
										case aiPTI_Float:
											{
												float *rawValue = (float*)prop->mData;
												if (rawValue != nullptr)
												{
													const int numElements = prop->mDataLength / sizeof(float);
													if (numElements < 1)
													{
														//NB: we can reach here of numElements is zero
														//if ((numElements < 1)||(numElements >4))
														{	
															UE_LOG(LogTemp, Error, TEXT( LOG_INDENT "Float param: unexpected count: %d [expected >= 1]"), numElements);
														}
													}
													else if (numElements == 1)
													{
														//scalar
														float result = rawValue[0];
														echoMat.Scalars.Push(FEchoImportMaterialScalar{propName, result});
													}
													else
													{
														//if ((numElements < 1)||(numElements >4))
														if (numElements >4)
														{	
															UE_LOG(LogTemp, Error, TEXT( LOG_INDENT "Vector param: unexpected count: %d [expected 2-4]"), numElements);
														}
														//vector, pad with zero
														float vecData[4];
														int len = std::min(numElements, 4);
														int i=0;
														for (; i<len; i++)
														{
															vecData[i] = rawValue[i];
														}
														for(; i<4; i++)
														{
															//zero fill rest
															//TODO: maybe fill w with 1?
															vecData[i] = 0.0f;
														}
														FVector4 result(vecData[0], vecData[1], vecData[2], vecData[3]);
														echoMat.Vectors.Push(FEchoImportMaterialVector{propName, result});
													}
												}
											}
											break;
										case aiPTI_String:
											{
												if (propName == kMatNameProp)
												{
													echoMat.MaterialName = aiStringToFName((const aiString*)prop->mData);
													//UE_LOG(LogTemp, Error, TEXT(LOG_INDENT "Found Mat Name: %s"), *echoMat.MaterialName.ToString());
													break;	
												}
											}
											[[fallthrough]]

										default:
											//types like strings and buffers cant just be passed through (or can they???)
											UE_LOG(LogTemp, Error, TEXT(LOG_INDENT "Unhandled Property Type for %s : %d (%s)"), *propName.ToString(), prop->mType, *LookupMaterialPropertyTypeName(prop->mType));

											UE_LOG(LogTemp, Error, TEXT(LOG_INDENT "\t%s"), *propValueToStringWithType(prop));
											/*
											if (prop->mType == aiPTI_Buffer)
											{
												//just for sanity
												
												UE_LOG(LogTemp, Error, TEXT(LOG_INDENT "\tBuffer: %s"), *bufferToString(prop->mData, prop->mDataLength));
											}
											else if (prop->mType == aiPTI_String)
											{
												UE_LOG(LogTemp, Error, TEXT(LOG_INDENT "\t""String: %s"), );
											}
											*/
											break;
									}//end switch for assignment
								}//end if
							}
							else
							{
								//UE_LOG(LogTemp, Error, TEXT("MAT PROP[%d] is nullptr!"), iMatProp);
							}
						}//end 2nd for

						//auto len = foundTextures.Num();

						//sort texture identities so its consistent regardless of enumeration order, since we're now using them as a potential identifier (of sorts)
						for(size_t i=0; i<foundTextures.Num(); i++)
						{
							for(size_t j=i+1; j<foundTextures.Num(); j++)
							{
								const auto ix = std::get<0>(foundTextures[i]);
								const auto iy = std::get<1>(foundTextures[i]);

								const auto jx = std::get<0>(foundTextures[j]);
								const auto jy = std::get<1>(foundTextures[j]);
								bool bSwap = (jx < ix) || ((jx == ix) && (jy < iy)); //absolute ordering
								if (bSwap)
								{
									//TODO: is this actually sane?
									std::swap(foundTextures[i], foundTextures[j]);
								}
								//if (std::get<0>(foundTextures[j]) < std::get<0>(foundTextures[i])
							}
						}

						const bool verboseTextureInfo = UMeshLoader::importerVerbose;
						size_t texPairIndex = (size_t)-1;

						//TODO: gltf importer from assimp always aliases some textures. ignore them if aliases present
						/*
						aiTextureType_UNKNOWN
						aiTextureType_METALNESS
						aiTextureType_DIFFUSE_ROUGHNESS
						*/
						//prefer later stuffs

						//bool bUnknownTex = 
						//or just map one per semantic?
						/*
						TArray<EchoImpTextureIndex> foundIndexes;
						for(const auto &texPair: foundTextures)
						{
							const auto fIndex = std::get<0>(texPair);
							const auto fSemantic = std::get<1>(texPair);
							if (!foundIndexes.Contains(fIndex))
							{
								foundIndexes.Add(fIndex);
							}
						}
						
						for(const auto& matchFIndex = found
						for(auto &texPair: foundTextures)
						{
							const auto fIndex = std::get<0>(texPair);
							const auto fSemantic = std::get<1>(texPair);
							if (!foundIndexes.Contains(fIndex))
						*/

						for(const auto &texPair: foundTextures)
						{
							texPairIndex++;
							FEchoImportMaterialTexture echoTex;
							echoTex.Value = nullptr; //resolved later
							
							const auto fIndex = std::get<0>(texPair);
							const auto fSemantic = std::get<1>(texPair);
							
							if (fSemantic == aiTextureType_UNKNOWN)
							{
								UE_LOG(LogTemp, Error, TEXT(LOG_INDENT "Import WARNING: Unknown Texture Type!"));
							}
							///texture index within material
							//TODO: sort by tuple?
							echoTex.TextureIndex = texPairIndex; //since textureIndex can apparently be repeated for different semantics, lets differentiate them. This is an arbitrary ordering.
							echoTex.SemanticIndex = fSemantic;
							//TODO: #define INDENT?
							//TODO: maybe cache properties above not just foundTex?
							#define LOG_INDENT "\t"
							if (verboseTextureInfo)
							{
								//UE_LOG(LogTemp, Error, TEXT( LOG_INDENT "TEXTURE INFO: [%d] : %d"), fIndex, fSemantic);
								UE_LOG(LogTemp, Log, TEXT( LOG_INDENT "TEXTURE INFO: [%d] : %d (%s)"), fIndex, fSemantic, *LookupTextureTypeSemanticName(fSemantic));
							}
							for(unsigned int iMatProp=0; iMatProp < mat->mNumProperties; iMatProp++)
							{
								#define LOG_INDENT "\t\t"
								aiMaterialProperty *prop = mat->mProperties[iMatProp];
								
								if (prop != nullptr)
								{
									if ((prop->mIndex == fIndex)&&(prop->mSemantic == fSemantic))
									{
										if (verboseTextureInfo)
										{
											//UE_LOG(LogTemp, Error, TEXT( LOG_INDENT "Prop[%d]: T_%d(T: %s) '%s' [%d] : S_%d(S: %s)"), 
											UE_LOG(LogTemp, Log, TEXT( LOG_INDENT "Prop[%d]: T_%d(T: %s) '%s' [%d]"), 
												iMatProp, 
												prop->mType, *LookupMaterialPropertyTypeName(prop->mType), 
												*aiString2FString(prop->mKey), prop->mIndex
												//, prop->mSemantic, *LookupTextureTypeSemanticName(prop->mSemantic)
											);
										}
										#define LOG_INDENT "\t\t\t"
										const FString propKey = aiString2FString(prop->mKey);
										switch(prop->mType)
										{
											case aiPTI_String:
												{
													//??//AI_MATKEY_MAPPING_AMBIENT
													const FString EmptyString(L"");
													const FString NullString(L"<echo_null>");
													//const char *aiStringUTF8 = (prop->mData != nullptr) ? ((const aiString*)prop->mData).
													static const FString TexKey_File = cStringToFString(_AI_MATKEY_TEXTURE_BASE);
													//Or convert to fname?
													FString propValue = aiString2FString((const aiString*)prop->mData, EmptyString);

													
													
													bool bIgnore = false;
													bool bHandled = false;

													if (TexKey_File == propKey)
													{
														echoTex.TextureKey = FName(propValue);
														bHandled = true;
													}
													/*
													//not reachable
													if (propKey == NullString)
													{
														UE_LOG(LogTemp, Error, TEXT( LOG_INDENT "Bad string!"))
													}
													*/
													
													bool bUnhandled = (!bIgnore) && (!bHandled);
													{
														if (verboseTextureInfo)
														{
															//UE_LOG(LogTemp, Error, TEXT( LOG_INDENT ": String: %s"), *propKey);
															//UE_LOG(LogTemp, Log, TEXT( LOG_INDENT ": String: '%s'"), *propValue);
															LOG_ERROR_OR_MESSAGE(bUnhandled, TEXT( LOG_INDENT ": String: '%s'"), *propValue);

														}
													}
												}
												break;
											case aiPTI_Buffer:
												{
													bool bIgnore = false;
													bool bHandled = false;
													if (propKey == kMapModeU)
													{
														if (prop->mData == nullptr)
														{
															bIgnore = true; //no effect
														}
													}

													if (propKey == kMapModeV)
													{
														if (prop->mData == nullptr)
														{
															bIgnore = true; //no effect
														}
													}

													//if ((!bIgnore) && (!bHandled))
													//if ()
													bool bUnhandled = (!bIgnore) && (!bHandled);
													{	
														const FString UnhandledPrefix(TEXT("Unhandled Buffer value for"));
														const FString BufferPrefix(TEXT("Buffer"));
														const FString prefix = bUnhandled ? UnhandledPrefix : BufferPrefix;
														//UE_LOG(LogTemp, Error, TEXT( LOG_INDENT "%s prop %s: %s"), 
														LOG_ERROR_OR_MESSAGE(bUnhandled, 
															TEXT( LOG_INDENT "%s prop %s: %s"), 
															*prefix,
															*aiString2FString(prop->mKey), *bufferToString(prop->mData, prop->mDataLength)
														);
													}
												}
												break;
											case aiPTI_Integer:
												{
													bool bIgnore = false;
													bool bHandled = false;

													size_t count = 0;
													bool bArrayOkay = (ResolveMatPropCount<int>(prop, count, 0, true));
													bool bSingleValueValid = (bArrayOkay && (count == 1));
													int32 value0 = bSingleValueValid ? ((const int*)prop->mData)[0] : 0;

													if (kUVWSrc == propKey)
													{
														if (bSingleValueValid)
														{
															if (value0 == 0)
															{
																bIgnore = true;
															}
															else
															{
																//Client CTA
																UE_LOG(LogTemp, Error, TEXT(LOG_INDENT "Texture mapped to uvw: %d. We only support textures on texcoord0 presently."), value0);
															}
														}
														else
														{
															//unexpected
															UE_LOG(LogTemp, Error, TEXT(LOG_INDENT "uvw mapping not a single value! count=%i"), count);
														}
													}

													//if ((!bIgnore) && (!bHandled))
													bool bUnhandled = (!bIgnore) && (!bHandled);
													{
														const FString UnhandledPrefix(TEXT("Unhandled Integer value for"));
														const FString BufferPrefix(TEXT("in32"));
														const FString prefix = bUnhandled ? UnhandledPrefix : BufferPrefix;
														FString intString;
														if (prop->mData == nullptr)
														{
															intString = DefaultNullString;
														}
														else
														{
															if (bArrayOkay)
															{
																const int *propArray = (const int*)prop->mData;
																for(size_t i=0; i<count; i++)
																{
																	intString += FString::Printf(TEXT("%d "), propArray[i]);
																}
															}
															else
															{
																UE_LOG(LogTemp, Error, TEXT( LOG_INDENT "Unhandled Int value for prop ERROR Count bad!")); 
																intString = TEXT("<bad array>");
															}
														}

														LOG_ERROR_OR_MESSAGE(bUnhandled, TEXT( LOG_INDENT "%s prop %s: %s"), 
																*prefix,
																*aiString2FString(prop->mKey), *intString
														);
														/*
														if (bUnhandled)
														{
															UE_LOG(LogTemp, Error, TEXT( LOG_INDENT "%s prop %s: %s"), 
																*prefix,
																*aiString2FString(prop->mKey), *intString
															);
														}
														else
														{
															UE_LOG(LogTemp, Message, TEXT( LOG_INDENT "%s prop %s: %s"), 
																*prefix,
																*aiString2FString(prop->mKey), *intString
															);
														}
														*/
														/*
														UE_LOG(LogTemp, Error, TEXT( LOG_INDENT "Unhandled Int value for prop %s: %s"), 
															*aiString2FString(prop->mKey), *intString
														);
														*/
													}
												}
												break;

											default:
												{
													
													UE_LOG(LogTemp, Error, TEXT( LOG_INDENT "Parsing Texture Definition: unhandled prop type for prop %s: %d (%s)"), 
														*aiString2FString(prop->mKey), prop->mType, *LookupMaterialPropertyTypeName(prop->mType)
													);
												}
												break;
										}//end switch

										#define LOG_INDENT "\t\t"
									}//end inner if
								}//mat prop != null
								else
								{
									UE_LOG(LogTemp, Error, TEXT("MAT PROP[%d] is nullptr!"), iMatProp);
								}
								
							}//end for
							#define LOG_INDENT "\t"
							echoMat.Textures.Push(echoTex);

						}//for foundTextures
						result.Materials.Push(echoMat);
					} //if mat != nullptr
					else
					{
						UE_LOG(LogTemp, Error, TEXT("MAT[%d] is nullptr!"), iMat);
					}
				}//next material
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("ImportModel: '%s': Importer: Scene has NO MATERIALS!"), *filenameLabel);
			}

		}//end debug materials

		UE_LOG(LogTemp, Warning, TEXT("======================"));
		
		{
			if (Scene->HasTextures())
			{
				for(unsigned int iTexture=0; iTexture<Scene->mNumTextures; iTexture++)
				{
					//push all textures to avoid misalignment when parsing *0 / *1 / etc
					FEchoImportTexture texImport;
					texImport.IsValid = false;
					texImport.Width = 0;
					texImport.Height = 0;
					texImport.texture = nullptr;

					aiTexture *tex = Scene->mTextures[iTexture];
					if (tex != nullptr)
					{
						
						FString formatHintFString = cStringToFString(tex->achFormatHint);
						FString textureFilename = aiString2FString(tex->mFilename);
						texImport.filename = textureFilename;
						
						bool bPrintedTextureHeader = false;
						#define LOG_PRINT_TEXTURE_HEADER \
							UE_LOG(LogTemp, Error, TEXT("\tTex[%d]: '%s': Hint<%s> [%i x %i]: 0x%x"), iTexture, *textureFilename, *formatHintFString, tex->mWidth, tex->mHeight, tex->pcData)

						if (superVerbose || UMeshLoader::importerVerboseTextures)
						{
							bPrintedTextureHeader = true;
							LOG_PRINT_TEXTURE_HEADER;
						}
						if (tex->mHeight == 0)
						{
							
							//bool bValid = false;
							//int width = 0;
							//int height =0;
							TArray<uint8> blob((uint8*)tex->pcData, tex->mWidth);
							texImport.texture = UMeshLoader::LoadTexture2DFromBlob_Internal(textureFilename, blob, texImport.IsValid, texImport.Width, texImport.Height);
							texImport.IsValid = texImport.IsValid && (texImport.texture != nullptr) && (texImport.Width>0) && (texImport.Height > 0);//sanity check
							
							if (!texImport.IsValid)
							{
								if (!bPrintedTextureHeader)
								{
									LOG_PRINT_TEXTURE_HEADER;
								}
								UE_LOG(LogTemp, Error, TEXT("\t\tFAILED TO IMPORT COMPRESSED EMBEDDED TEXTURE! mesh=%s, Texture Filename = %s"), *filenameLabel, *textureFilename);
							}
							else
							{
								if (superVerbose)
								{
									if (!bPrintedTextureHeader)
									{
										LOG_PRINT_TEXTURE_HEADER;
									}
									//UE_LOG(LogTemp, Warning, TEXT("Mesh %s: Mat: %s: Imported Tex: %s - %d x %d"), *filenameLabel,  
									UE_LOG(LogTemp, Log, TEXT("Mesh %s: Imported Tex as *%d: '%s' - %d x %d"), *filenameLabel, result.Textures.Num(), *textureFilename, texImport.Width, texImport.Height)
								}
							}
						}
						else
						{
							//UE_LOG(LogTemp, Error, TEXT("\tTex[%d]: '%s': Hint<%s> [%i x %i]: 0x%x"), iTexture, *textureFilename, *formatHintFString, tex->mWidth, tex->mHeight, tex->pcData);
							if (!bPrintedTextureHeader)
							{
								//UE_LOG(LogTemp, Error, TEXT("\tTex[%d]: '%s': Hint<%s> [%i x %i]: 0x%x"), iTexture, *textureFilename, *formatHintFString, tex->mWidth, tex->mHeight, tex->pcData);
								LOG_PRINT_TEXTURE_HEADER;
							}
							UE_LOG(LogTemp, Error, TEXT("\t\tEcho Import Textures: non-compressed textures not currently supported: %s"), *textureFilename);
							//texImport.filename = FString::Printf(TEXT("uncompressed_texture_index_%d"), iTexture);
						}
						
					}
					else
					{
						UE_LOG(LogTemp, Error, TEXT("\tTEX[%d]: nullptr"), iTexture);
						texImport.filename = FString::Printf(TEXT("null_index_%d"), iTexture);
					}

					#undef LOG_PRINT_TEXTURE_HEADER
					//texImport.importerTexId = FName(FString::Printf(TEXT("*%zu"), result.Textures.Num());
					texImport.importerTexId = FName(FString::Printf(TEXT("*%d"), result.Textures.Num())); // *0 *1 *N etc
					result.Textures.Push(texImport);
				}//for textures
			}
			else
			{
				//UE_LOG(LogTemp, Warning, TEXT("NO TEXTURES"));
				UE_LOG(LogTemp, Warning, TEXT("ImportModel: '%s': Importer: Scene has NO TEXTURES!"), *filenameLabel);
			}
		}//end debug textures

		{//bind textures to mat stuffs
			//LOL & very important here!
			for(auto &mat: result.Materials)
			{
				for (auto &texRef: mat.Textures)
				{
					bool bFoundTexRef = false;
					//if (texRef.TextureKey
					for(auto &texImp: result.Textures)
					{
						if (texImp.importerTexId == texRef.TextureKey)
						{
							if (!texImp.IsValid)
							{
								//UE_LOG(LogTemp, Error, TEXT("Texture resolved but target not valid!
								UE_LOG(LogTemp, Error, TEXT("Model: %s, Mat: %s, Texture resolved but target not valid: '%s'"), *filenameLabel, *mat.MaterialName.ToString(), *texRef.TextureKey.ToString());
							}
							else
							{
								if (superVerbose)
								{
									//UE_LOG(LogTemp, Warning, TEXT("Model: %s, Mat: %s, Resolved Tex for mat! Tex=%s, slot=%s"), *filenameLabel, *mat.MaterialName.ToString(), *texRef.TextureKey.ToString());
									UE_LOG(LogTemp, Warning, TEXT("Model: %s, Mat: %s, Resolved Tex for mat! Tex='%s', slot=%s"), 
										*filenameLabel, *mat.MaterialName.ToString(), 
										*texImp.filename, *texRef.TextureKey.ToString()
									);
								}
							}
							//or keep the reference?
							bFoundTexRef = true;
							texRef.Value = texImp.texture;
							
							break;
						}
					}
					if (!bFoundTexRef)
					{
						UE_LOG(LogTemp, Error, TEXT("Model: %s, Mat: %s, Texture not resolved: '%s'"), *filenameLabel, *mat.MaterialName.ToString(), *texRef.TextureKey.ToString());
					}
				}
			}
		}

		//if (superVerbose)
		//UE_LOG(LogTemp, Warning, TEXT("======================"));

		int NodeIndex = 0;
		int* NodeIndexPtr = &NodeIndex;
		ProcessNode(filenameLabel, Scene->mRootNode, Scene, -1, NodeIndexPtr, &result);
		result.Success = true;

		if (UMeshLoader::debugImportedHierarchy)
		{
			UE_LOG(LogTemp, Error, TEXT("Parse Result: %s"), *filenameLabel);
			size_t nodeI=(size_t)-1; //increment will get us to zero
			for(const auto &node: result.Nodes)
			{
				nodeI++;
				UE_LOG(LogTemp, Error, TEXT("\tNode[%i]: %s"), nodeI, *node.NodeName);
				UE_LOG(LogTemp, Error, TEXT("\t\tParent: %d"), node.NodeParentIndex);
				UE_LOG(LogTemp, Error, TEXT("\t\tSubmeshes: %d"), node.Meshes.Num());
				for(size_t iSubMesh=0; iSubMesh<node.Meshes.Num(); iSubMesh++)
				{
					const auto &submesh = node.Meshes[iSubMesh];
					UE_LOG(LogTemp, Error, TEXT("\t\t\tSubmesh[%i]: %s"), iSubMesh, *submesh.SubmeshName);
					UE_LOG(LogTemp, Error, TEXT("\t\t\t\tUsingMaterial: %d"), submesh.UseMaterialIndex);
					UE_LOG(LogTemp, Error, TEXT("\t\t\t\t""Verts: %d"), submesh.Vertices.Num());
					UE_LOG(LogTemp, Error, TEXT("\t\t\t\t""Tris: %d"), submesh.Triangles.Num());
					UE_LOG(LogTemp, Error, TEXT("\t\t\t\t""Normals: %d"), submesh.Normals.Num());
					UE_LOG(LogTemp, Error, TEXT("\t\t\t\t""TexCoord0: %d"), submesh.UVs0.Num());
					if (submesh.UVs1.Num() > 0)
					{
						UE_LOG(LogTemp, Error, TEXT("\t\t\t\t""TexCoord1: %d"), submesh.UVs1.Num());
					}
					if (submesh.UVs2.Num() > 0)
					{
						UE_LOG(LogTemp, Error, TEXT("\t\t\t\t""TexCoord2: %d"), submesh.UVs2.Num());
					}
					if (submesh.UVs3.Num() > 0)
					{
						UE_LOG(LogTemp, Error, TEXT("\t\t\t\t""TexCoord3: %d"), submesh.UVs3.Num());
					}
					UE_LOG(LogTemp, Error, TEXT("\t\t\t\t""Colors: %d"), submesh.Colors.Num());
					UE_LOG(LogTemp, Error, TEXT("\t\t\t\t""Tangents: %d"), submesh.Tangents.Num());
				}
				//UE_LOG(LogTemp, Error, TEXT("\t\tParent: %d"), node.RelativeTransformTransform);
			}
		}//end if UMeshLoader::debugImportedHierarchy
		
	}
	return result.Success;
}

void ConfigureImporterCommon(Assimp::Importer &mImporter)
{
	//HACK - debug importer settings - nuke these or convert to actual logic later?
	{
		mImporter.SetExtraVerbose(true);
		//mImporter.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_NORMALS); //HACK - strip normals
		mImporter.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_NORMALS | aiComponent_TANGENTS_AND_BITANGENTS); //HACK - strip normals
		mImporter.SetPropertyFloat(AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE,  UMeshLoader::smoothNormalsAngle);//80.0f); //?? apparently 175 is the default
		//TODO: we might want to normally remove these??
		//importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_CAMERAS | aiComponent_LIGHTS);
		//aiComponent_NORMALS
	}

	
}

//This function contains some code originally from RML
FFinalReturnData UMeshLoader::LoadMeshFromFile(FString FilePath, EPathType type)
{
    FFinalReturnData ReturnData;
	ReturnData.Success = false;

	if (FilePath.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Runtime Mesh Loader: filepath is empty.\n"));
		return ReturnData;
	}

	std::string FinalFilePath;
	switch (type)
	{
	    case EPathType::Absolute:
		    FinalFilePath = TCHAR_TO_UTF8(*FilePath);
		    break;
	    case EPathType::Relative:
		    FinalFilePath = TCHAR_TO_UTF8(*FPaths::Combine(FPaths::ProjectContentDir(), FilePath));
		    break;
	}

	Assimp::Importer mImporter;
	
	//const aiScene* Scene = mImporter.ReadFile(FinalFilePath.c_str(), aiProcess_Triangulate | aiProcess_MakeLeftHanded | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_OptimizeMeshes);
	//unsigned int importFlags = aiProcess_Triangulate | aiProcess_MakeLeftHanded | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_OptimizeMeshes;
	unsigned int importFlags = DefaultImportFlags;
	ConfigureImporterCommon(mImporter);
	const aiScene* Scene = mImporter.ReadFile(FinalFilePath.c_str(), importFlags);
	bool ok;
	//ok = FinishLoadMeshFromImporterAndScene(FinalFilePath, mImporter, Scene, ReturnData);
	ok = FinishLoadMeshFromImporterAndScene(FilePath, mImporter, Scene, ReturnData);
	
	return ReturnData;
}

////////////////////////// END MASSIVE CHUNK FROM RuntimeMeshLoader via UnrealEchoPOC project //////////////////////////

//static FFinalReturnData LoadMeshFromBlob(const FString &filenameInfo, const TArray<uint8> &blob);
struct HintMappings
{
	FString endsWithMatch;
	const char *passHint;
};

#define MAKE_HINT( tok ) HintMappings{ FString(TEXT( tok )), tok }
static TArray<HintMappings> hintMappings = {
	MAKE_HINT(".glb"),
	MAKE_HINT(".obj"),
	MAKE_HINT(".fbx")
};
#undef MAKE_HINT


FFinalReturnData UMeshLoader::LoadMeshFromBlob(const FString &filenameInfo, const TArray<uint8> &blob)
{
	FFinalReturnData ReturnData;
	ReturnData.Success = false;
	Assimp::Importer mImporter;
	const aiScene* Scene;
	//unsigned int importFlags = aiProcess_Triangulate | aiProcess_MakeLeftHanded | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_OptimizeMeshes;
	unsigned int importFlags = DefaultImportFlags;//aiProcess_Triangulate | aiProcess_MakeLeftHanded | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_OptimizeMeshes;
	
	const char *formatExtHint = nullptr;// = "";//".glb";//hack, default ext tends to fail on something about blender magic missing
	int lastDotIndex = -1;
	//TODO: ensure working in invariant culture?
	FString filenameLCase = filenameInfo.ToLower();

	for(const auto &hint: hintMappings)
	{
		if (filenameLCase.EndsWith(hint.endsWithMatch))
		{
			formatExtHint = hint.passHint;
			break;
		}
	}
	if (formatExtHint == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("No hint found for: '%s'"), *filenameLCase);
		formatExtHint = "";
	}

	ConfigureImporterCommon(mImporter);
	Scene = mImporter.ReadFileFromMemory(blob.GetData(), blob.Num(), importFlags, formatExtHint);
	bool ok;
	ok = FinishLoadMeshFromImporterAndScene(filenameInfo, mImporter, Scene, ReturnData);
	return ReturnData;	
}

//This function is based on a similar function from RML but supports a wider range of texture formats
UTexture2D* UMeshLoader::LoadTexture2DFromBlob_Internal(const FString &filenameInfo, const TArray<uint8> &blob, bool& IsValid, int32& Width, int32& Height)
{
	//possibly unneeded sanity check:
	//TODO: lol maybe can parse files from just a filename? =)
	if (blob.Num() < 1)
	{
		IsValid = false;
		Width = 0;
		Height = 0;
		UE_LOG(LogTemp, Error, TEXT("LoadTexture2DFromBlob_Internal: blob length < 1: '%s'"), *filenameInfo);
		return nullptr;
	}
	IsValid = false;
	UTexture2D* LoadedT2D = nullptr;
	
	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	
	EImageFormat detectedFormat = ImageWrapperModule.DetectImageFormat(blob.GetData(), blob.Num());
	if (superVerbose)
	{
		UE_LOG(LogTemp, Error, TEXT("Detected format: %d"), (int)detectedFormat);
	}
	if (detectedFormat == EImageFormat::Invalid)
	{
		//FAIL
		UE_LOG(LogTemp, Error, TEXT("Invalid or Unrecognized Format!"));
		IsValid = false;
		Width = 0;
		Height = 0;
		return nullptr;
	}
	
	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(detectedFormat);
	if ((!ImageWrapper.IsValid())||(ImageWrapper.Get() == nullptr))
	{
		UE_LOG(LogTemp, Error, TEXT("ImageWrapper not valid or is null!"));
		IsValid = false;
		Width = 0;
		Height = 0;
		return nullptr;
	}
	  
	//Create T2D!
	if (ImageWrapper.IsValid() && ImageWrapper->SetCompressed(blob.GetData(), blob.Num()))
	{ 
		TArray<uint8> UncompressedBGRA;
		if (ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, UncompressedBGRA))
		{
			LoadedT2D = UTexture2D::CreateTransient(ImageWrapper->GetWidth(), ImageWrapper->GetHeight(), PF_B8G8R8A8);
			
			/*
			//HACK experiment for messy textures - didnt work
			{
				//LoadedT2D->CompressionSettings = COMPRESS_None;//HACK
				LoadedT2D->CompressionSettings = TC_HDR; // = COMPRESS_None;//HACK
				LoadedT2D->rLossyCompressionAmount = TLCA_None;
			}
			*/

			//Valid?
			if (!LoadedT2D) 
			{
				return NULL;
			}
			
			//Out!
			Width = ImageWrapper->GetWidth();
			Height = ImageWrapper->GetHeight();
			 
			//Copy!
			void* TextureData = LoadedT2D->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
			FMemory::Memcpy(TextureData, UncompressedBGRA.GetData(), UncompressedBGRA.Num());
			/*if you use the code"const TArray<uint8>* UncompressedBGRA = NULL;",Accordingly, since UncompressedBGRA becomes a pointer, you need to use a pointer reference method, like this
			FMemory::Memcpy(TextureData, UncompressedBGRA->GetData(), UncompressedBGRA->Num());*/
			LoadedT2D->PlatformData->Mips[0].BulkData.Unlock();

			//Update!
			LoadedT2D->UpdateResource();
		}
	}
	 
	// Success!
	IsValid = true;
	return LoadedT2D;
}

//This is based on code from RML
UTexture2D* UMeshLoader::LoadTexture2DFromFile(const FString& FullFilePath, bool& IsValid, int32& Width, int32& Height)
{
	//Load From File
	TArray<uint8> RawFileData;
	/*If you use lower unreal engine,for example the version is 4.20,you may get a error message in bulid,you should use The following code replace "TArray<uint8> RawFileData;"
	const TArray<uint8>* UncompressedBGRA = NULL;*/
	
	if (!FFileHelper::LoadFileToArray(RawFileData, * FullFilePath)) 
	{
		return nullptr;
	}

    return LoadTexture2DFromBlob_Internal(FullFilePath, RawFileData, IsValid, Width, Height);
}

UTexture2D* UMeshLoader::LoadTexture2DFromBlob(const FString &filenameInfo, const TArray<uint8> &blob)
{
	//bool& IsValid, int32& Width, int32& Height
	bool bValid = false;
	int width = 0;
	int height = 0;
	UTexture2D *ret = LoadTexture2DFromBlob_Internal(filenameInfo, blob, bValid, width, height);
	if ((!bValid)||(ret == nullptr))
	{
		UE_LOG(LogTemp, Error, TEXT("LoadTexture2DFromBlob: '%s': not valid result!(or returned nullptr)"), *filenameInfo);
	}
	return ret;
}


void UMeshLoader::SetImporterSmoothingAngleForNormals(float setSmoothingAngle)
{
	//static void SetImporterSettingSmoothingAngleForNormals(float setSmoothingAngle)
	const float MinSmoothingAngle = 0;
	const float MaxSmoothingAngle = 175;
	if ((setSmoothingAngle < MinSmoothingAngle)||(setSmoothingAngle>MaxSmoothingAngle))
	{
		UE_LOG(LogTemp, Error, TEXT("SetImporterSmoothingAngleForNormals: value must be between [0, 175): %f"), setSmoothingAngle);
		//clamp input:
		if (setSmoothingAngle < MinSmoothingAngle)
		{
			setSmoothingAngle = MinSmoothingAngle;
		}
		else
		{
			setSmoothingAngle = MaxSmoothingAngle;
		}
	}
	smoothNormalsAngle = setSmoothingAngle;
}


#pragma optimize( "", on )