#include "ObjMeshUtil.h"
//#include "EchoHelperLib.h"
#include "EchoStringUtil.h"
#include <string>
#include "Util/EchoStopWatch.h"
#include "EchoStringUtil.h"

//#include <wstring>
/*
#include <vector>
#include <stdio.h>
#include <string>
#include <stdlib.h>
*/


bool ReadFilePointer(FILE *file, TArray<uint8> &result)
{
	result.Reset();
	if (file == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("FAILED TO READ FILE!"));
		return false;
	}
	//size_t fileLength = 
	const int bufferSize = 1024;
	char buffer[bufferSize];
	
	while(true)
	{
		int numRead = fread_s(buffer, bufferSize * sizeof(char), sizeof(char), bufferSize, file);
		result.Reserve(result.Num() + numRead);
		for(int i=0; i<numRead; i++)
		{
			result.Push(buffer[i]);
		}
		if (numRead < 1)
		{
			break;
		}
	}
	result.Push('\0');
	return true;
}


//bool ReadTestFile(const std::wstring &filename, TArray<uint8> &result)
bool ReadWFile(const TCHAR *filename, TArray<uint8> &result)
{
	//this is horrible. just for a quick test!
	result.Reset();
	
	if (filename == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("filename was nullptr!"));
		return false;
	}
	bool ret;
	//FILE *file = fopen(filename, "r");
	FILE *file = nullptr;
	errno_t err;
	err = _wfopen_s(&file, filename, L"rb");
	if (err == 0)
	{
		ReadFilePointer(file, result);
		ret = true;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("FAILED TO READ LOCAL FILE: %s"), filename);
		ret = false;
	}
	if (file != nullptr)
	{
		fclose(file);
	}
	file = nullptr;
	//return true;
	return ret;
}


bool ReadLocalFile(const FString &filename, TArray<uint8> &result)
{
	/*
	std::string fname_nonwstring;
	const auto &uechars = localFilename.GetCharArray();
	std::wstring fname_wstring(uechars.GetData());
	fname_nonwstring.assign(fname_wstring.begin(), fname_wstring.end());
	*/

	/*
	std::string strFilename = FStringToSTDString(filename);
	//std::string strFilename = StringUtil::(filename);
	return ReadTestFile(strFilename.c_str(), result);
	*/
	return ReadWFile(filename.operator*(), result);
}

#ifndef DISABLE_TEST_OBJ_MESH
//TODO: look at TCHAR_TO_UTF8 etc
FString cStringToFString(const char *str)
{
	
	if (str == nullptr)
	{
		return EchoStringConstants::EmptyString;
	}
	std::string sstr(str);
	std::wstring wstr;
	wstr.assign(sstr.begin(), sstr.end());
	return FString(wstr.c_str());
}

//TArray<uint8> ReadTestFile(const FString &filename)
bool ReadTestFile(const char *filename, TArray<uint8> &result)
{
//this is horrible. just for a quick test!
	result.Reset();
	//bool ret = false;
	if (filename == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("filename arg was nullptr!"));
		return false;
	}
	FILE *file = fopen(filename, "r");
	ReadFilePointer(file, result);
	if (file != nullptr)
	{
		fclose(file);
	}
	file = nullptr;
}


std::string FStringToSTDString(const FString &fstr)
{
	/*
	std::string fname_nonwstring;
	const auto &uechars = localFilename.GetCharArray();
	std::wstring fname_wstring(uechars.GetData());
	fname_nonwstring.assign(fname_wstring.begin(), fname_wstring.end());
	*/
	//std::wstring wstr(fstr.GetData());
	const auto &uechars = fstr.GetCharArray();
	std::wstring wstr(uechars.GetData());
	std::string str;
	str.assign(wstr.begin(), wstr.end());
	return str;
}


struct ObjVert
{
	int vert;
	int uv;
	int normal;

	ObjVert &operator =(const ObjVert &rhs)
	{
		if (this != &rhs)
		{
			this->vert = rhs.vert;
			this->uv = rhs.uv;
			this->normal = rhs.normal;
		}
		return *this;
	}
};

struct ObjPoly
{
	std::vector<ObjVert> verts;
};

struct ObjMesh
{
	std::vector<FVector> verts;
	std::vector<FVector2D> uvs;
	std::vector<FVector> normals;
	//std::vector<int> triangles;
	std::vector<ObjPoly> polys;
};

std::wstring toWString(const std::string &input)
{
	std::wstring ret;
	ret.assign(input.begin(), input.end());
	return ret;
}

void DoLog(const char *msg)
{
	if (msg == nullptr)
	{
		msg = "<null>";
	}
	std::wstring wstr = toWString(std::string(msg));
	FString arg(wstr.c_str());
	UE_LOG(LogTemp, Error, TEXT("%s"), *arg);
}

void DoLog(const char *format, const char *arg2)
{
	if (arg2 != nullptr)
	{
		char buffer[1024];
		sprintf_s(buffer, format, arg2);
		DoLog(buffer);
	}
	else
	{
		DoLog(format);
	}
}

void DoLog(const char *format, int number)
{
	char buffer[1024];
	sprintf_s(buffer, format, number);
	DoLog(buffer);
}





void SplitString_Advanced(const char *_data, int numDelims, const char *delims, std::vector<std::string> &result, bool bOmitEmptyTokens)//, bool bSkipConsecutiveTokens)
{
	result.clear();
	if (_data == nullptr)
	{
		return;
	}
	bool bWasDelim = false;
	//bool bJustPushedToken = false;
	std::string tok;
	size_t len = strlen(_data);
	for(size_t i=0; i<len; i++)
	{
		
		bWasDelim = false;
		char ch = _data[i];
		//if (ch == delim)
		bool bMatchAny = false;
		for(int idelim=0; idelim<numDelims; idelim++)
		{
			if (ch == delims[idelim])
			{
				bMatchAny = true;
				break;
			}
		}
		
		if (bMatchAny)
		{
			bWasDelim = true;
			if ((!tok.empty()) || (!bOmitEmptyTokens))
			{
				result.push_back(tok);
			}
			tok = "";
		}
		else
		{
			tok += ch;
			//bJustPushedToken = false;
		}
	}
	//if ((!tok.empty()) || (!bJustPushedToken))
	if ((!tok.empty()) || (!bWasDelim))
	{
		//push remaining token if we didn't END on a delim
		if ((!tok.empty()) || (!bOmitEmptyTokens))
		{
			result.push_back(tok);
		}
		tok = "";
	}
	//char ch = 'a';
	//std::string str;
	//str += ch;
}




void SplitStringOmitEmptyTokens(const char *_data, const char *delims, std::vector<std::string> &result)
{
	return SplitString_Advanced(_data, strlen(delims), delims, result, true);
}

void SplitStringKeepEmptyTokens(const char *_data, int numDelims, const char *delims, std::vector<std::string> &result)
{
	return SplitString_Advanced(_data, numDelims, delims, result, false);
}


//void SplitString(char *data, const char *delims, std::vector<std::string> &result)
void SplitStringKeepEmptyTokens(const char *_data, const char *delims, std::vector<std::string> &result)
{
	int numDelims = strlen(delims);
	SplitStringKeepEmptyTokens(_data, numDelims, delims, result);
}

//void SplitStringKeepEmptyTokens(const char *_data, int numDelims, const char *delims, std::vector<std::string> &result)
void SplitStringKeepEmptyTokensOld(const char *_data, char delim, std::vector<std::string> &result)
{
	result.clear();
	if (_data == nullptr)
	{
		return;
	}
	bool bWasDelim = false;
	//bool bJustPushedToken = false;
	std::string tok;
	size_t len = strlen(_data);
	for(size_t i=0; i<len; i++)
	{
		
		bWasDelim = false;
		char ch = _data[i];
		//if (ch == delim)
		/*
		bool bMatchAny = false;
		for(int idelim=0; idelim<numDelims; idelim++)
		{
			if (ch == delims[idelim])
			{
				bMatchAny = true;
				break;
			}
		}
		*/
		if (ch == delim)
		{
			bWasDelim = true;
			result.push_back(tok);
			tok = "";
		}
		else
		{
			tok += ch;
			//bJustPushedToken = false;
		}
	}
	//if ((!tok.empty()) || (!bJustPushedToken))
	if ((!tok.empty()) || (!bWasDelim))
	{
		//push remaining token if we didn't END on a delim
		result.push_back(tok);
		tok = "";
	}
	//char ch = 'a';
	//std::string str;
	//str += ch;
}

void SplitString(const char *_data, const char *delims, std::vector<std::string> &result)
{
	result.clear();
	if (_data != nullptr)
	{
		int len = strlen(_data)+1;
		char * data = new char[len];
		strcpy(data, _data);
		//int paranoia = 4096;
		int paranoia = 4096 * 4096;
		char *ptr = data;
		while(paranoia-- >= 0)
		{
			//char * item = strtok(data, delims);
			char * item = strtok(ptr, delims);
			ptr = nullptr; //continue with old tokenization
			if (item != nullptr)
			{
				result.push_back(std::string(item));
			}
			else
			{
				break;
			}
		}
		delete[]data;
	}
}

float ParseFloat(const char *str)
{
	if (str == nullptr)
	{
		return 0.0f;
	}
	return atof(str);
}

int ParseInt(const char *str, int errorValue)
{
	if ((str == nullptr)||(str[0]=='\0'))
	{
		//return 0.0f;
		return errorValue;
	}
	return atoi(str);	
}



//moved to EchoMeshService.cpp:
FVector ConvertObjCoordToUnreal(const FVector &objSpace);
/*
{
	return FVector(
		+objSpace.Z,
		-objSpace.X,
		+objSpace.Y
	);
	/|*
	float vxPrime, vyPrime, vzPrime; //in unreal coordinate system
	vxPrime = +vz; //is this correct???
	vyPrime = -vx;
	vzPrime = +vy;
	*|/
	//ret->verts.push_back(FVector(vxPrime, vyPrime, vzPrime) * customConfig.uniformScale);
}
*/
//https://en.wikipedia.org/wiki/Wavefront_.obj_file
//ObjMesh *ParseObjMesh(const TArray<uint8> &blob, float uniformScale = 1.0f)
ObjMesh *ParseObjMesh(const TArray<uint8> &blob, const FEchoCustomMeshImportSettings &customConfig)
{
	FEchoStopwatch swTotal;
	swTotal.Start();

	FEchoStopwatch swSetup;
	FEchoStopwatch swEveryLine;
	FEchoStopwatch swFilters;
	FEchoStopwatch swFinishMesh;
	swSetup.Start();
	//////////////////////////////////////////////
	#define IF_NOT_TOO_MANY_WARNINGS \
		if (printedWarnings++ < MaxPrintWarnings)

	static_assert(sizeof(char)==sizeof(uint8), "Sanity check sizeof char == sizeof uint8");
	if (blob.Num() < 1)
	{
		return nullptr; //empty blob
	}

	const char *rawData = (const char *)blob.GetData();
	if (rawData == nullptr)
	{
		return nullptr;
	}
	int len = blob.Num();
	//char *dataCopy = new char[len+1];
	ObjMesh *ret = new ObjMesh();
	const char *lineEndings = "\r\n";
	//const char *lineTok = strtok(dataCopy,
	int printedWarnings = 0;
	const int MaxPrintWarnings = 100;

	std::vector<std::string> lines;
	//SplitString(rawData, lineEndings, lines);
	SplitStringKeepEmptyTokens(rawData, lineEndings, lines);
	int lineNumber = 0;//might be wrong
	std::vector<ObjPoly> rawPolys;

	swSetup.Stop();
	swEveryLine.Start();
	std::vector<std::string> tokens;
	std::vector<std::string> vertParts;
	tokens.reserve(10);
	vertParts.reserve(16);//probably dont need as many as these but meh cheap

	for(const std::string &line: lines)
	{
		/*if (line.empty())
		{
			lineNumber++;
			continue;
		}
		*/
		char firstChar = line.c_str()[0];
		if (firstChar == '\0')
		{
			//empty string
			lineNumber++;
			continue;
		}
		if (firstChar == '#')
		{
			continue;//comment
		}
		else if (firstChar == ' ')
		{
			continue;
		}

		//std::vector<std::string> tokens;
		tokens.clear();
		
		//SplitString(line.c_str(), " ", tokens);
		//SplitStringKeepEmptyTokens(line.c_str(), " ", tokens);
		SplitStringOmitEmptyTokens(line.c_str(), " ", tokens);
		if (tokens.size() < 1)
		{
			continue;
		}
		const std::string &lineType = tokens[0];
		//int baseTokens = 1;
		int numTokens = tokens.size();
		int numTokensLeft = numTokens - 1;
		//numTokens--;
		//char secondChar = line[1];
		//if (line
		if (lineType == "v")
		{
			if (numTokensLeft != 3)
			{
				//const auto *ptr = L"cat";
				//UE_LOG(LogTemp, Error, TEXT("line has too many
				IF_NOT_TOO_MANY_WARNINGS
				{
					DoLog("Line: %d", lineNumber);
					DoLog(line.c_str());
					DoLog("line v(vert): need 3 tokens, got: %d", numTokensLeft);
				}
				//continue;
				if (numTokensLeft<3)
				{
					//otherwise we have to reindex our counts?
					//TODO: setinvalid flag?
					//ret->invalid[
					ret->verts.push_back(FVector(0,0,0));
					continue;//abort line
				}
			}
			//in model package coordinate system
			float vx = ParseFloat(tokens[1].c_str());
			float vy = ParseFloat(tokens[2].c_str());
			float vz = ParseFloat(tokens[3].c_str());
			/*
			float vxPrime, vyPrime, vzPrime; //in unreal coordinate system
			vxPrime = +vz; //is this correct???
			vyPrime = -vx;
			vzPrime = +vy;

			//ret->verts.push_back(FVector(vx, vy, vz));
			//ret->verts.push_back(FVector(vx, vy, vz) * customConfig.uniformScale);
			ret->verts.push_back(FVector(vxPrime, vyPrime, vzPrime) * customConfig.uniformScale);
			*/
			FVector vertex(vx, vy, vz);
			vertex = ConvertObjCoordToUnreal(vertex);
			ret->verts.push_back(vertex * customConfig.uniformScale);
			
		}
		else if (lineType == "vt")
		{
			ret->uvs.reserve(ret->verts.size());
			if (numTokensLeft != 2)
			{
				//const auto *ptr = L"cat";
				//UE_LOG(LogTemp, Error, TEXT("line has too many
				IF_NOT_TOO_MANY_WARNINGS
				{
					DoLog("Line: %d", lineNumber);
					DoLog(line.c_str());
					DoLog("line vt(uv): need 3 tokens, got: %d", numTokensLeft);
				}
				//continue;
				if (numTokensLeft<2)
				{
					//otherwise we have to reindex our counts?
					//TODO: setinvalid flag?
					ret->uvs.push_back(FVector2D(0,0));
					continue;//abort line
				}
			}
			float tu = ParseFloat(tokens[1].c_str());
			float tv = ParseFloat(tokens[2].c_str());
			ret->uvs.push_back(FVector2D(tu, tv));
		}
		else if (lineType == "vn")
		{
			ret->normals.reserve(ret->verts.size());
			if (numTokensLeft != 3)
			{
				//const auto *ptr = L"cat";
				//UE_LOG(LogTemp, Error, TEXT("line has too many
				IF_NOT_TOO_MANY_WARNINGS
				{
					DoLog("Line: %d", lineNumber);
					DoLog(line.c_str());
					DoLog("line vn(normal): need 3 tokens, got: %d", numTokensLeft);
				}
				//continue;
				if (numTokensLeft<3)
				{
					//otherwise we have to reindex our counts?
					//TODO: setinvalid flag?
					ret->normals.push_back(FVector(1,0,0));
					continue;//abort line
				}
			}
			float nx = ParseFloat(tokens[1].c_str());
			float ny = ParseFloat(tokens[2].c_str());
			float nz = ParseFloat(tokens[3].c_str());
			//ret->normals.push_back(FVector(nx, ny, nz));
			FVector normal = FVector(nx, ny, nz);
			normal = ConvertObjCoordToUnreal(normal);
			ret->normals.push_back(normal);
		}
		else if (lineType == "f")
		{
			rawPolys.reserve(ret->verts.size()/3);
			//NB: these are off by one, so subtract 1 to each type here (one-based indices)
			ObjPoly poly;
			for(int i=1; i<numTokens; i++)
			{
				const char *vertTok = tokens[i].c_str();
				ObjVert vert;
				//std::vector<std::string> vertParts;
				vertParts.clear();

				//SplitString(vertTok, "/", vertParts);
				SplitStringKeepEmptyTokens(vertTok, "/", vertParts);
				//SplitStringKeepEmptyTokensOld(vertTok, '/', vertParts);
				int vChildTokens = vertParts.size();
				if (vChildTokens != 3)
				{
					//const auto *ptr = L"cat";
					//UE_LOG(LogTemp, Error, TEXT("line has too many
					IF_NOT_TOO_MANY_WARNINGS
					{
						DoLog("Line: %d", lineNumber);
						DoLog(line.c_str());
						DoLog("line f(face): need 3 vert tokens, got: %d", vChildTokens);
						DoLog("vertTok=%s", vertTok);
					}
					//continue;
					/*if (vChildTokens<3)
					{
						continue;//abort line
					}*/
				}
				//TODO: handle missing stuffs?
				//TODO: iterator here instead of [0,1,2]?
				int vi = 1; 
				float ti = 1; 
				float ni = 1; 
				if (vertParts.size() >= 1)
				{
					vi = ParseInt(vertParts[0].c_str(), vi);
				}
				if (vertParts.size() >= 2)
				{
					ti = ParseInt(vertParts[1].c_str(), ti);	
				}
				if (vertParts.size() >= 3)
				{
					ParseInt(vertParts[2].c_str(), ni);
				}
				if (vertParts.size() > 3)
				{
					IF_NOT_TOO_MANY_WARNINGS
					{
						DoLog("Line: %d", lineNumber);
						DoLog(line.c_str());
						//DoLog("line f(face): need 3 vert tokens, got: %d", vChildTokens);
						DoLog("WARN: vertParts >3: %d", vertParts.size());
						DoLog("vertTok=%s", vertTok);
						DoLog("line=%s", line.c_str());
					}
				}

				/*
				//also wrong
				int vi = ParseInt(vertParts[1].c_str());
				float ti = ParseInt(vertParts[2].c_str());
				float ni = ParseInt(vertParts[3].c_str());
				
				//very wrong
				int vi = ParseInt(tokens[1].c_str());
				float ti = ParseInt(tokens[2].c_str());
				float ni = ParseInt(tokens[3].c_str());
				*/
				vi--;
				ti--;
				ni--;
				vert.vert = vi;
				vert.uv = ti;
				vert.normal = ni;
				poly.verts.push_back(vert);
			}
			rawPolys.push_back(poly);
		}
	}
	//TODO: max reserved?
	swEveryLine.Stop();
	swFilters.Start();
	int invalidPolyCount = 0;
	std::vector<ObjPoly> actualPolys;
	actualPolys.reserve(rawPolys.size());
	for(const auto &poly: rawPolys)
	{
		//TODO: add option to invert winding?
		int numVerts = poly.verts.size();
		if (numVerts == 3)
		{
			//tri
			actualPolys.push_back(poly);
		}
		else if (numVerts == 4)
		{
			//break quad
			const ObjVert &v0 = poly.verts[0];
			const ObjVert &v1 = poly.verts[1];
			const ObjVert &v2 = poly.verts[2];
			const ObjVert &v3 = poly.verts[3];
			ObjPoly tri0, tri1;
			/*
			//assuming we have
			v0   v1

			v3   v2
			//we want tri0(v0,v1,v3) and tri1(v3,v1,v2);
			*/
			tri0.verts.reserve(3);
			tri0.verts.push_back(v0);
			tri0.verts.push_back(v1);
			tri0.verts.push_back(v3);

			tri1.verts.reserve(3);
			tri1.verts.push_back(v3);
			tri1.verts.push_back(v1);
			tri1.verts.push_back(v2);
			
			actualPolys.push_back(std::move(tri0));
			actualPolys.push_back(std::move(tri1));
		}
		else
		{
			IF_NOT_TOO_MANY_WARNINGS
			{
				DoLog("Ignore Poly N!=3 and N!=4: N=%d", poly.verts.size());
			}
			invalidPolyCount++;
			continue;
		}
	}

	const bool bInvertWinding = customConfig.bInvertWinding;//false;
	if (bInvertWinding)
	{
		//for(const auto &poly: actualPolys)
		for(auto &poly: actualPolys)
		{
			if (poly.verts.size() != 3)
			{
				continue;
			}
			// 0 1 2 => 0 2 1
			
			ObjVert swap = poly.verts[1];
			poly.verts[1] = poly.verts[2];
			poly.verts[2] = swap;
		}
	}
	swFilters.Stop();
	swFinishMesh.Start();
	for(const auto &poly: actualPolys)
	{
		if (poly.verts.size() != 3)
		{
			IF_NOT_TOO_MANY_WARNINGS
			{
				DoLog("Ignore Poly N>3: N=%d", poly.verts.size());
			}
			invalidPolyCount++;
			continue;
		}
		bool bVertValid = true;
		for(const auto &vert: poly.verts)
		{
			if ((vert.vert < 0)||(vert.vert>=ret->verts.size()))
			{
				IF_NOT_TOO_MANY_WARNINGS
				{
					DoLog("Vert out of bounds: %d", vert.vert);
				}
				bVertValid = false;
				break;
			}
			if ((vert.uv < 0)||(vert.uv >= ret->uvs.size()))
			{
				IF_NOT_TOO_MANY_WARNINGS
				{
					DoLog("UV out of bounds: %d", vert.uv);
				}
				bVertValid = false;
				break;
			}
			if ((vert.normal <0)||(vert.normal>= ret->normals.size()))
			{
				IF_NOT_TOO_MANY_WARNINGS
				{
					DoLog("Normal out of bounds: %d", vert.normal);
				}
				bVertValid = false;
				break;
			}
		}
		if (bVertValid)
		{
			ret->polys.push_back(poly); //triangle!
		}
		else
		{
			invalidPolyCount++;
		}
	}
	swFinishMesh.Stop();
	if (invalidPolyCount != 0)
	{
		DoLog("#Invalid discarded Polys: %d", invalidPolyCount);
	}
	//delete[] dataCopy;
	swTotal.Stop();

	{
		UE_LOG(LogTemp, Error, TEXT("ParseObjMesh: total=%lfms - breakdown: \n\tSetup: %lfms\n\tEveryLine: %lfms\n\tFilters: %lfsms\n\tFinish: %lfms\n"), 
			swTotal.GetElapsedMilliseconds(), swSetup.GetElapsedMilliseconds(), swEveryLine.GetElapsedMilliseconds(), swFilters.GetElapsedMilliseconds(), swFinishMesh.GetElapsedMilliseconds()
		);
	}

	return ret;
	#undef IF_NOT_TOO_MANY_WARNINGS
}


//ObjMesh *ReadTestMesh(const char *filename, FEchoCustomMeshImportSettings customConfig = FEchoCustomMeshImportSettings{1.0f, false})//float uniformScale = 1.0f)
ObjMesh *ReadTestMesh(const char *filename, const FEchoCustomMeshImportSettings &customConfig )
{
	TArray<uint8> buffer;
	bool bReadFile = ReadTestFile(filename, buffer);
	if (!bReadFile)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to read test file!"));
		return nullptr;
	}
	return ParseObjMesh(buffer, customConfig);//uniformScale); 
}

ObjMesh *ReadMeshFromEchoMemoryAsset(const FEchoMemoryAsset &asset, FEchoCustomMeshImportSettings customConfig = FEchoCustomMeshImportSettings{1.0f, false, true})
{
	if (asset.assetType != EEchoAssetType::EEchoAsset_Mesh)
	{
		UE_LOG(LogTemp, Error, TEXT("ReadMeshFromEchoMemoryAsset: must have mesh type!! fname=%s type=%d"), *asset.fileInfo.filename, asset.assetType);
		return nullptr;
	}
	if (!asset.bHaveContent)
	{
		UE_LOG(LogTemp, Error, TEXT("ReadMeshFromEchoMemoryAsset: must have content! fname=%s type=%d"), *asset.fileInfo.filename, asset.assetType);
		return nullptr;
	}
	return ParseObjMesh(asset.blob, customConfig);
}


///////////////////////////////////////////////
//Extern
void SimpleMesh::LoadObj(const ObjMesh &fromObj)
{
	for(const auto &tri: fromObj.polys)
	{
		if (tri.verts.size() != 3)
		{
			continue;//not a triangle!
		}
		for(const auto &vert: tri.verts)
		{
			AddVert(fromObj, vert);
		}
	}
	bLoaded = true;
}

void SimpleMesh::AddVert(const ObjMesh &fromObj, const ObjVert &vert)
{
	int vertId = verts.Num();
	verts.Push(fromObj.verts[vert.vert]);//position instead of vert?
	uvs.Push(fromObj.uvs[vert.uv]);
	normals.Push(fromObj.normals[vert.normal]);
	tris.Add(vertId);
}



bool ReadMeshFromEchoMemoryAsset(const FEchoMemoryAsset &asset, const FEchoCustomMeshImportSettings &config, SimpleMesh &result)
{
	result.Reset();

	if ((!asset.bHaveContent)||(asset.blob.Num()<1))
	{
		UE_LOG(LogTemp, Error, TEXT("ReadMeshFromEchoMemoryAsset: bad blob. 'filename'=%s"), *asset.fileInfo.filename);
		return false;
	}

	ObjMesh *mesh = nullptr;
	mesh = ParseObjMesh(asset.blob, config);
	if (mesh == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("ReadMeshFromEchoMemoryAsset: failed to parse Obj Mesh from blob. 'filename'=%s"), *asset.fileInfo.filename);
		return false;
	}

	result.LoadObjPtr(mesh);
	delete mesh;

	//TODO: also check count of polys is > 0???
	return result.bLoaded;//hack??? or just true?
}


//random code to calculate bounds:
/*
float hugeNumber = 1000000.0f;
FVector minBounds(hugeNumber, hugeNumber, hugeNumber);
FVector maxBounds(-hugeNumber, -hugeNumber, -hugeNumber);
{
	//print bounds
	for(const auto &tri: mesh->polys)
	{
		for(const auto &v: tri.verts)
		{
			FVector vp = mesh->verts[v.vert];
			minBounds = minBounds.ComponentMin(vp);
			maxBounds = maxBounds.ComponentMax(vp);
		}
	}
	FVector boundsExtent = maxBounds - minBounds;
	UE_LOG(LogTemp, Error, TEXT("Bounds: (%f, %f, %f) - (%f, %f, %f) sides=[%f, %f, %f]"), 
		minBounds.X, minBounds.Y, minBounds.Z,
		maxBounds.X, maxBounds.Y, maxBounds.Z,
		boundsExtent.X, boundsExtent.Y, boundsExtent.Z
	);
	//TODO: pass these to unreal?
}
*/
#endif
