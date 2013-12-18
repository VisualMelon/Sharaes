#include <windows.h>
#include <stdlib.h>
#include <cstdio>
#include <iostream>
#include <string>
#include <tchar.h>
#include <cmath>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>

// dx
#include <d3d9.h>
#include <d3dx9.h>

const float halfPi = D3DX_PI / 2.0f;
const float FOV = D3DX_PI / 2.5f;

// :?
// :?
// START OF UNCRZ STUFF
// :D
// :D

// debug/hr (performance) defines
#define DEBUG_HR_START(x) if (debugData) hr_start(x)
#define DEBUG_HR_ZERO(x) if (debugData) hr_zero(x)
#define DEBUG_HR_END(x, y, z) if (debugData) hr_end(x, y, z)
#define DEBUG_HR_ACCEND(x, y, z) if (debugData) hr_accend(x, y, z)
#define DEBUG_DX_FLUSH() if (debugData && debugFlushing) { flushQuery->Issue(D3DISSUE_END); while(S_FALSE == flushQuery->GetData( NULL, 0, D3DGETDATA_FLUSH )) ; }

const DWORD VX_PC = 0x00000001;
const DWORD VX_PCT = 0x00000002;
const DWORD VX_PTW4 = 0x00000004;
const DWORD VX_PAT4 = 0x00000008;
const DWORD VX_Over = 0x00000010;

const DWORD DF_light = 0x00000001;
const DWORD DF_default = 0x00000000;

const DWORD SF_notimed = 0x00000001;
const DWORD SF_noclouds = 0x00000002;
const DWORD SF_swapcull = 0x00000004;

const DWORD SF_light = SF_notimed | SF_noclouds | SF_swapcull;
const DWORD SF_default = 0x00000000;

LPDIRECT3DVERTEXDECLARATION9 vertexDecPC;
LPDIRECT3DVERTEXDECLARATION9 vertexDecPCT;
LPDIRECT3DVERTEXDECLARATION9 vertexDecPTW4;
LPDIRECT3DVERTEXDECLARATION9 vertexDecPAT4;
LPDIRECT3DVERTEXDECLARATION9 vertexDecOver;

float stof(std::string str)
{
	return atof(str.c_str());
}

int stoi(std::string str)
{
	return atoi(str.c_str());
}

std::string trim(std::string str)
{
	int ni;
	while ((ni = str.find("  ", 0)) != std::string::npos)
	{
		str = str.erase(ni, 1);
	}
	while ((ni = str.find("\r", 0)) != std::string::npos)
	{
		str = str.erase(ni, 1);
	}
	return str;
}

std::vector<std::string> split(std::string str, std::string s)
{
	std::vector<std::string> output;
	int ni;
	while ((ni = str.find(s)) != std::string::npos)
	{

		if (ni > 0)
			output.push_back(str.substr(0, ni));
		str = str.erase(0, ni + s.length());
	}
	output.push_back(str);
	return output;
}

std::string replace(std::string str, std::string gin, std::string rpl)
{
	int off = 0;
	int ni;
	while ((ni = str.find(gin, off)) != std::string::npos)
	{
		str = str.replace(ni, gin.length(), rpl);
		off = ni + rpl.length();
	}
	return str;
}

const float LT_ortho = 0;
const float LT_persp = 1;
const float LT_point = 2;

struct UNCRZ_obj;

struct UNCRZ_texture
{
public:
	std::string name;

	LPDIRECT3DTEXTURE9 tex;

	UNCRZ_texture();
	UNCRZ_texture(std::string, LPDIRECT3DTEXTURE9);
};

UNCRZ_texture::UNCRZ_texture()
{

}

UNCRZ_texture::UNCRZ_texture(std::string nameN, LPDIRECT3DTEXTURE9 texN)
{
	name = nameN;
	tex = texN;
}

void createTexture(LPDIRECT3DDEVICE9 dxDevice, char* fileName, LPDIRECT3DTEXTURE9* dest, std::vector<UNCRZ_texture*>* textureList)
{
	for (int i = textureList->size() - 1; i >= 0; i--)
	{
		if (textureList->at(i)->name == fileName)
		{
			*dest = textureList->at(i)->tex;
			return;
		}
	}

	D3DXCreateTextureFromFile(dxDevice, fileName, dest);
	UNCRZ_texture* tex = new UNCRZ_texture(fileName, *dest);
	textureList->push_back(tex);
}

struct lightData
{
public:
	std::string name;

	bool lightEnabled;

	float dimX;
	float dimY;
	float lightDepth;
	float coneness;

	float lightType;

	D3DXVECTOR4 lightAmbient;
	D3DXVECTOR4 lightColMod;

	D3DXVECTOR4 lightPos;
	D3DXVECTOR4 lightDir;
	D3DXVECTOR3 lightUp;

	LPDIRECT3DTEXTURE9 lightTex;
	LPDIRECT3DTEXTURE9 lightPatternTex;
	LPDIRECT3DSURFACE9 lightSurface;
	UINT texWidth;
	UINT texHeight;

	D3DXMATRIX lightViewProj; // (texAligned, x and y are 0..1)
	D3DXMATRIX lightViewProjVP; // (x and y are -1..1)

	std::vector<int> zsoLocalIndexes;

	bool useLightMap; // should be disabled for LT_point!

	lightData(std::string nameN)
	{
		name = nameN;
	}

	void init(LPDIRECT3DDEVICE9 dxDevice, UINT w, UINT h, char* lightPatternFile, std::vector<UNCRZ_texture*>* textureList)
	{	
		D3DXCreateTexture(dxDevice, w, h, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R32F, D3DPOOL_DEFAULT, &lightTex); // I don't know what implications there are of using this, but it seems to work
		//D3DXCreateTexture(dxDevice, w, h, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &lightTex);
		lightTex->GetSurfaceLevel(0, &lightSurface);
		
		createTexture(dxDevice, lightPatternFile, &lightPatternTex, textureList);
		//HRESULT res = D3DXCreateTextureFromFile(dxDevice, lightPatternFile, &lightPatternTex);

		float ldMod = sqrtf(lightDir.x * lightDir.x + lightDir.y * lightDir.y + lightDir.z * lightDir.z);
		lightDir.x /= ldMod;
		lightDir.y /= ldMod;
		lightDir.z /= ldMod;

		texWidth = w;
		texHeight = h;
	}

	void release()
	{
		// implement
	}
};

const float DDT_ortho = 0;
const float DDT_persp = 1;

// this looks /alot/ like light data, because it is a simplified light data
//  - this projects pure colour (source colour etc. ignored, like a decal) without a shadow map
//  - these have NO respect for lighting and should be used for highlighting effects and such
//    if you want lighting build proper decals
struct dynamicDecalData
{
public:
	bool decalEnabled;

	float dimX;
	float dimY;
	float lightDepth;
	float lightType;

	D3DXVECTOR4 lightColMod;

	D3DXVECTOR4 lightPos;
	D3DXVECTOR4 lightDir;
	D3DXVECTOR3 lightUp;

	LPDIRECT3DTEXTURE9 lightPatternTex;

	D3DXMATRIX lightViewProj;

	// this is for fun mostley
	UNCRZ_obj* attach;

	void init(LPDIRECT3DDEVICE9 dxDevice, char* lightPatternFile, std::vector<UNCRZ_texture*>* textureList)
	{
		createTexture(dxDevice, lightPatternFile, &lightPatternTex, textureList);
		//HRESULT res = D3DXCreateTextureFromFile(dxDevice, lightPatternFile, &lightPatternTex);

		float ldMod = sqrtf(lightDir.x * lightDir.x + lightDir.y * lightDir.y + lightDir.z * lightDir.z);
		lightDir.x /= ldMod;
		lightDir.y /= ldMod;
		lightDir.z /= ldMod;
	}

	void release()
	{
		// implement
	}
};

struct UNCRZ_bbox;

struct uiItem;

const DWORD GDOF_ms = 0;
const DWORD GDOF_dms = 1;
const DWORD GDOF_prop100 = 2;

struct drawData
{
public:
	LPD3DXMATRIX viewProj;
	LPD3DXMATRIX viewMat;
	LPD3DXMATRIX projMat;
	D3DXVECTOR4 eyePos;
	D3DXVECTOR4 eyeDir;
	std::vector<lightData*> lightDatas;
	std::vector<dynamicDecalData*> dynamicDecalDatas;
	bool performPlainPass;
	float lightCoof;
	float lightType;
	float lightDepth;
	float lightConeness;

	float ticker;

	int cullCount;
	int drawCount;

	D3DVIEWPORT9* targetVp;
	D3DVIEWPORT9* sideVp;
	LPDIRECT3DTEXTURE9 targetTex;
	LPDIRECT3DSURFACE9 targetSurface;
	LPDIRECT3DTEXTURE9 sideTex;
	LPDIRECT3DSURFACE9 sideSurface;
	
	LPDIRECT3DSURFACE9 zSideSurface;
	LPDIRECT3DSURFACE9 zTargetSurface;


	// these are for things passes draw datas without global access
	char buff[100];
	IDirect3DQuery9* flushQuery;
	bool debugData;
	bool debugFlushing;
	uiItem* genericDebugView;
	uiItem** genericLabel;
	uiItem** genericBar;
	LARGE_INTEGER hrsec;

	LARGE_INTEGER hridstart;
	LARGE_INTEGER hridend;

	drawData(LPD3DXMATRIX viewProjN, LPD3DXMATRIX viewMatN, LPD3DXMATRIX projMatN, std::vector<lightData*> lightDatasN, std::vector<dynamicDecalData*> dynamicDecalDatasN, float lightCoofN)
	{
		viewProj = viewProjN;
		viewMat = viewMatN;
		projMat = projMatN;
		lightDatas = lightDatasN;
		dynamicDecalDatas = dynamicDecalDatasN;
		lightCoof = lightCoofN;

		cullCount = 0;
		drawCount = 0;
	}

private:
	void hr_start(LARGE_INTEGER* start)
	{
		QueryPerformanceCounter(start);
	}

	void hr_end(LARGE_INTEGER* start, LARGE_INTEGER* end, float* outTime)
	{
		QueryPerformanceCounter(end);
		*outTime = ((double)(end->QuadPart - start->QuadPart) / (double)(hrsec.QuadPart));
	}

public:
	void startTimer(bool preFlushDx);
	void stopTimer(int genericIndex, bool flushDx, char* label, DWORD);
	void nontimed(int genericIndex, char* label, float value, DWORD);
};

struct vertexPC
{
public:
	float x;
	float y;
	float z;
	float w;

	float nx;
	float ny;
	float nz;
	float nw;

	float r;
	float g;
	float b;
	float a;

	float tti;

	vertexPC() { }

	vertexPC(D3DXVECTOR3 posN, D3DXVECTOR3 colN, float ttiN)
	{
		x = posN.x;
		y = posN.y;
		z = posN.z;
		w = 1.0;

		nx = 0.0;
		ny = 0.0;
		nz = 0.0;
		nw = 0.0;

		a = 1.0;
		r = colN.x;
		g = colN.y;
		b = colN.z;

		tti = ttiN;
	}

	vertexPC(D3DXVECTOR3 posN, float rN, float gN, float bN, float ttiN)
	{
		x = posN.x;
		y = posN.y;
		z = posN.z;
		w = 1.0;

		nx = 0.0;
		ny = 0.0;
		nz = 0.0;
		nw = 0.0;

		a = 1.0;
		r = rN;
		g = gN;
		b = bN;

		tti = ttiN;
	}

	vertexPC(float xN, float yN, float zN, D3DXVECTOR3 colN, float ttiN)
	{
		x = xN;
		y = yN;
		z = zN;
		w = 1.0;

		nx = 0.0;
		ny = 0.0;
		nz = 0.0;
		nw = 0.0;

		a = 1.0;
		r = colN.x;
		g = colN.y;
		b = colN.z;

		tti = ttiN;
	}

	vertexPC(float xN, float yN, float zN, float rN, float gN, float bN, float ttiN)
	{
		x = xN;
		y = yN;
		z = zN;
		w = 1.0;

		nx = 0.0;
		ny = 0.0;
		nz = 0.0;
		nw = 0.0;

		a = 1.0;
		r = rN;
		g = gN;
		b = bN;

		tti = ttiN;
	}

	vertexPC(float xN, float yN, float zN, float nxN, float nyN, float nzN, float rN, float gN, float bN, float ttiN)
	{
		x = xN;
		y = yN;
		z = zN;
		w = 1.0;

		nx = nxN;
		ny = nyN;
		nz = nzN;
		nw = 0.0;

		a = 1.0;
		r = rN;
		g = gN;
		b = bN;

		tti = ttiN;
	}

	D3DXVECTOR3 getPosVec3()
	{
		return D3DXVECTOR3(x, y, z);
	}
};

struct vertexPCT
{
public:
	float x;
	float y;
	float z;
	float w;

	float nx;
	float ny;
	float nz;
	float nw;

	float r;
	float g;
	float b;
	float a;

	float tu;
	float tv;

	float tti;

	vertexPCT() { }

	vertexPCT(vertexPC posCol, float tuN, float tvN)
	{
		x = posCol.x;
		y = posCol.y;
		z = posCol.z;
		w = posCol.w;

		nx = posCol.nx;
		ny = posCol.ny;
		nz = posCol.nz;
		nw = 0.0;

		a = posCol.a;
		r = posCol.r;
		g = posCol.g;
		b = posCol.b;
	
		tu = tuN;
		tv = tvN;

		tti = posCol.tti;
	}

	D3DXVECTOR3 getPosVec3()
	{
		return D3DXVECTOR3(x, y, z);
	}
};

struct vertexPTW4
{
public:
	float x;
	float y;
	float z;
	float w;

	float nx;
	float ny;
	float nz;
	float nw;

	float tu;
	float tv;

	float w0;
	float w1;
	float w2;
	float w3;

	vertexPTW4() { }

	vertexPTW4(float xN, float yN, float zN, float uN, float vN, float w0N, float w1N, float w2N, float w3N)
	{
		x = xN;
		y = yN;
		z = zN;
		w = 1.0f;

		nx = 0.0;
		ny = 1.0;
		nz = 0.0;
		nw = 0.0;
		
		tu = uN;
		tv = vN;
		
		w0 = w0N;
		w1 = w1N;
		w2 = w2N;
		w3 = w3N;
	}

	vertexPCT quickPCT()
	{
		vertexPCT res(vertexPC(x, y, z, 1.0f, 1.0f, 1.0f, 0), tu, tv);
		res.nx = nx;
		res.ny = ny;
		res.nz = nz;
		res.nw = nw;
		return res;
	}

	vertexPCT quickPCT(float a, float r, float g, float b)
	{
		vertexPCT res(vertexPC(x, y, z, r, g, b, 0), tu, tv);
		res.a = a;
		res.nx = nx;
		res.ny = ny;
		res.nz = nz;
		res.nw = nw;
		return res;
	}

	D3DXVECTOR3 quickPos()
	{
		D3DXVECTOR3 res(x, y, z);
		return res;
	}
};

struct vertexPSW4
{
public:
	float x;
	float y;
	float z;
	float w;

	float sx;
	float sy;

	float w0;
	float w1;
	float w2;
	float w3;
};

struct vertexPAT4
{
public:
	float x;
	float y;
	float z;
	float w;

	float nx;
	float ny;
	float nz;
	float nw;

	float a;

	float tx;
	float ty;
	float tz;
	float tw;

	float tti;

	vertexPAT4() { }

	vertexPAT4(vertexPC posCol, float txN, float tyN, float tzN, float twN)
	{
		x = posCol.x;
		y = posCol.y;
		z = posCol.z;
		w = posCol.w;

		nx = posCol.nx;
		ny = posCol.ny;
		nz = posCol.nz;
		nw = posCol.nw;

		a = posCol.a;
	
		tx = txN;
		ty = tyN;
		tz = tzN;
		tw = twN;

		tti = posCol.tti;
	}

	vertexPAT4(vertexPC pos, float aN, float txN, float tyN, float tzN, float twN)
	{
		x = pos.x;
		y = pos.y;
		z = pos.z;
		w = pos.w;

		nx = pos.nx;
		ny = pos.ny;
		nz = pos.nz;
		nw = pos.nw;

		a = aN;
	
		tx = txN;
		ty = tyN;
		tz = tzN;
		tw = twN;

		tti = pos.tti;
	}

	vertexPAT4(vertexPCT pos, float tzN, float twN)
	{
		x = pos.x;
		y = pos.y;
		z = pos.z;
		w = pos.w;

		nx = pos.nx;
		ny = pos.ny;
		nz = pos.nz;
		nw = pos.nw;

		a = pos.a;
	
		tx = pos.tu;
		ty = pos.tv;
		tz = tzN;
		tw = twN;

		tti = pos.tti;
	}

	vertexPAT4(vertexPTW4 pos, float aN, float tzN, float twN)
	{
		x = pos.x;
		y = pos.y;
		z = pos.z;
		w = pos.w;

		nx = pos.nx;
		ny = pos.ny;
		nz = pos.nz;
		nw = pos.nw;

		a = aN;
	
		tx = pos.tu;
		ty = pos.tv;
		tz = tzN;
		tw = twN;

		tti = 0.0;
	}

	vertexPAT4(vertexPTW4 pos, float aN, float txN, float tyN, float tzN, float twN)
	{
		x = pos.x;
		y = pos.y;
		z = pos.z;
		w = pos.w;

		nx = pos.nx;
		ny = pos.ny;
		nz = pos.nz;
		nw = pos.nw;

		a = aN;
	
		tx = txN;
		ty = tyN;
		tz = tzN;
		tw = twN;

		tti = 0.0;
	}
};

struct vertexOver
{
public:
	float x;
	float y;
	float z;
	float w;

	float tu;
	float tv;

	vertexOver() { }

	vertexOver(float xN, float yN, float zN, float tuN, float tvN)
	{
		x = xN;
		y = yN;
		z = zN;
		w = 1.0;
	
		tu = tuN;
		tv = tvN;
	}
};

struct UNCRZ_FBF_anim;
struct UNCRZ_FBF_anim_inst;
struct UNCRZ_FBF_anim_motion;
struct UNCRZ_FBF_anim_instr;
struct UNCRZ_section;
struct UNCRZ_segment;
struct UNCRZ_trans_arr;

struct UNCRZ_decal
{
public:
	int age;
	int remAge;
	int numFaces;
	vertexPAT4* vertexArray;
	int* indexArray; // for re-retrieving values (should they change) (NOT for rendering)
	LPDIRECT3DTEXTURE9 tex;
	void* vertexSource;
	DWORD vertexType;

	UNCRZ_decal(int, vertexPAT4*, int*, void*, DWORD, int);
	void updateVerts();
	void updatePTW4();
	void updatePCT();
	void destroy();
};

UNCRZ_decal::UNCRZ_decal(int remAgeN, vertexPAT4* vertexArraySrc, int* indexArraySrc, void* vertexSourceN, DWORD vertexTypeN, int numFacesN)
{
	remAge = remAgeN;
	numFaces = numFacesN;

	int stride = sizeof(vertexPAT4);
	int iStride = sizeof(int);
	vertexArray = (vertexPAT4*)malloc(numFaces * 3 * stride);
	memcpy(vertexArray, vertexArraySrc, numFaces * 3 * stride);
	indexArray = (int*)malloc(numFaces * 3 * iStride);
	memcpy(indexArray, indexArraySrc, numFaces * 3 * iStride);
	vertexSource = vertexSourceN;
	vertexType = vertexTypeN;
}

void UNCRZ_decal::updateVerts()
{
	if (vertexType == VX_PCT)
		updatePCT();
	else if (vertexType == VX_PTW4)
		updatePTW4();
}

void UNCRZ_decal::updatePTW4()
{
	vertexPTW4* vPTW4s = (vertexPTW4*)vertexSource;

	for (int i = 0; i < numFaces * 3; i++)
	{
		int si = indexArray[i];
		vertexArray[i].x = vPTW4s[si].x;
		vertexArray[i].y = vPTW4s[si].y;
		vertexArray[i].z = vPTW4s[si].z;
		vertexArray[i].w = vPTW4s[si].w;
		vertexArray[i].nx = vPTW4s[si].nx;
		vertexArray[i].ny = vPTW4s[si].ny;
		vertexArray[i].nz = vPTW4s[si].nz;
		vertexArray[i].nw = vPTW4s[si].nw;
	}
}

void UNCRZ_decal::updatePCT()
{
	vertexPCT* vPCTs = (vertexPCT*)vertexSource;

	for (int i = 0; i < numFaces * 3; i++)
	{
		int si = indexArray[i];
		vertexArray[i].x = vPCTs[si].x;
		vertexArray[i].y = vPCTs[si].y;
		vertexArray[i].z = vPCTs[si].z;
		vertexArray[i].w = vPCTs[si].w;
		vertexArray[i].nx = vPCTs[si].nx;
		vertexArray[i].ny = vPCTs[si].ny;
		vertexArray[i].nz = vPCTs[si].nz;
		vertexArray[i].nw = vPCTs[si].nw;
	}
}

void UNCRZ_decal::destroy()
{
	delete[numFaces * 3 * sizeof(vertexPCT)] vertexArray;
}

const int bboxIndices[36] = {
	0, 1, 2, // base
	0, 2, 3,

	4, 7, 6, // top
	4, 6, 5,

	0, 4, 5, // the others...
	0, 5, 1,

	1, 5, 6,
	1, 6, 2,

	2, 6, 7,
	2, 7, 3,

	3, 7, 4,
	3, 4, 0,
	};

struct UNCRZ_bbox
{
public:
	bool empty;
	float minX, minY, minZ;
	float maxX, maxY, maxZ;
	D3DXVECTOR3 vecArr[8];

	UNCRZ_bbox()
	{
		empty = true;
	}

	bool inside(D3DXVECTOR3 point)
	{
		if (point.x >= minX && point.x <= maxX && point.y >= minY && point.y <= maxY && point.z >= minZ && point.z <= maxZ)
			return true;
		return false;
	}
	
	bool overlap(UNCRZ_bbox* bbox)
	{
		if (bbox->minX < maxX && bbox->maxX > minX && bbox->minY < maxY && bbox->maxY > minY && bbox->minZ < maxZ && bbox->maxZ > minZ)
			return true;
		return false;
	}

	bool dothSurviveClipTransformed(D3DXMATRIX* mat)
	{
		D3DXVECTOR4 vecsA[8];

		// transform bbox
		transformVectors(mat, &(vecsA[0]));

		float w;
		D3DXVECTOR4* curVec;

		// check everything
		for (int i = 0; i < 8; i++)
		{
			curVec = &(vecsA[i]);

			w = curVec->w;
			curVec->x /= w;
			curVec->y /= w;
			curVec->z /= w;
			if (-1 <= curVec->x && curVec->x <= 1 && -1 <= curVec->y && curVec->y <= 1 && 0 <= curVec->z && curVec->z <= 1)
				return true;
		}

		// check everything
		for (int i = 0; i < 8; i++)
		{
			curVec = &(vecsA[i]);

			w = curVec->w;
		}

		// try some other thing
		float aminX, aminY, aminZ, amaxX, amaxY, amaxZ;

		// get 2D box
		aminX = vecsA[0].x;
		amaxX = aminX;
		aminY = vecsA[0].y;
		amaxY = aminY;
		aminZ = vecsA[0].z;
		amaxZ = aminZ;
		for (int i = 1; i < 8; i++)
		{
			if (vecsA[i].x < aminX)
				aminX = vecsA[i].x;
			if (vecsA[i].x > amaxX)
				amaxX = vecsA[i].x;
			if (vecsA[i].y < aminY)
				aminY = vecsA[i].y;
			if (vecsA[i].y > amaxY)
				amaxY = vecsA[i].y;
			if (vecsA[i].z < aminZ)
				aminZ = vecsA[i].z;
			if (vecsA[i].z > amaxZ)
				amaxZ = vecsA[i].z;
		}

		if (-1 <= amaxX && aminX <= 1 && -1 <= amaxY && aminY <= 1 && 0 <= amaxZ && aminZ <= 1)
			return true;
		return false;
	}

	bool overlapTransformed(UNCRZ_bbox* bbox, D3DXMATRIX* mat)
	{
		D3DXVECTOR4 vecsA[8];

		// transform bbox
		transformVectors(mat, &(vecsA[0]));

		float aminX, aminY, aminZ, amaxX, amaxY, amaxZ;

		// get 2D box
		aminX = vecsA[0].x;
		amaxX = aminX;
		aminY = vecsA[0].y;
		amaxY = aminY;
		aminZ = vecsA[0].z;
		amaxZ = aminZ;
		for (int i = 1; i < 8; i++)
		{
			if (vecsA[i].x < aminX)
				aminX = vecsA[i].x;
			if (vecsA[i].x > amaxX)
				amaxX = vecsA[i].x;
			if (vecsA[i].y < aminY)
				aminY = vecsA[i].y;
			if (vecsA[i].y > amaxY)
				amaxY = vecsA[i].y;
			if (vecsA[i].z < aminZ)
				aminZ = vecsA[i].z;
			if (vecsA[i].z > amaxZ)
				amaxZ = vecsA[i].z;
		}

		if (bbox->minX < amaxX && bbox->maxX > aminX && bbox->minY < amaxY && bbox->maxY > aminY && bbox->minZ < amaxZ && bbox->maxZ > aminZ)
			return true;
		return false;
	}

	// overlap in x.z plane - assumes projected result is a 2D bounding box, not for precision (no surprises there)
	bool projectedBoundsOverlap(UNCRZ_bbox* bbox, D3DXMATRIX* mat)
	{
		D3DXVECTOR4 vecsA[8];
		D3DXVECTOR4 vecsB[8];

		// transform bboxes
		transformVectors(mat, &(vecsA[0]));
		bbox->transformVectors(mat, &(vecsB[0]));

		float aminX, aminZ, amaxX, amaxZ;
		float bminX, bminZ, bmaxX, bmaxZ;

		// get 2D boxes
		aminX = vecsA[0].x;
		amaxX = aminX;
		aminZ = vecsA[0].z;
		amaxZ = aminZ;
		for (int i = 1; i < 8; i++)
		{
			if (vecsA[i].x < aminX)
				aminX = vecsA[i].x;
			if (vecsA[i].x > amaxX)
				amaxX = vecsA[i].x;
			if (vecsA[i].z < aminZ)
				aminZ = vecsA[i].z;
			if (vecsA[i].z > amaxZ)
				amaxZ = vecsA[i].z;
		}

		bminX = vecsB[0].x;
		bmaxX = bminX;
		bminZ = vecsB[0].z;
		bmaxZ = bminZ;
		for (int i = 1; i < 8; i++)
		{
			if (vecsB[i].x < bminX)
				bminX = vecsB[i].x;
			if (vecsB[i].x > bmaxX)
				bmaxX = vecsB[i].x;
			if (vecsB[i].z < bminZ)
				bminZ = vecsB[i].z;
			if (vecsB[i].z > bmaxZ)
				bmaxZ = vecsB[i].z;
		}

		// see if boxes overlap
		if (bminX < amaxX && bmaxX > aminX && bminX < amaxX && bmaxX > aminX)
			return true;
		return false;
	}

	bool collides(D3DXVECTOR3* rayPos, D3DXVECTOR3* rayDir)
	{
		float uRes, vRes, distRes; // not returned

		D3DXVECTOR3* a;
		D3DXVECTOR3* b;
		D3DXVECTOR3* c;

		for (int i = 0; i < 36; i += 3)
		{
			a = &(vecArr[bboxIndices[i]]);
			b = &(vecArr[bboxIndices[i + 1]]);
			c = &(vecArr[bboxIndices[i + 2]]);

			if (D3DXIntersectTri(a, b, c, rayPos, rayDir, &uRes, &vRes, &distRes))
			{
				return true;
			}
		}

		return false;
	}
	
	void transformVectors(D3DXMATRIX* mat, D3DXVECTOR4* vecs)
	{
		D3DXVec3TransformArray(vecs, sizeof(D3DXVECTOR4), &(vecArr[0]), sizeof(D3DXVECTOR3), mat, 8);
		//for (int i = 0; i < 8; i++)
		//{
		//	D3DXVec3Transform(&(vecs[i]), &(vecArr[i]), mat);
		//}
	}

	void fillVectors()
	{
		// boottom
		vecArr[0].x = minX;
		vecArr[0].y = minY;
		vecArr[0].z = minZ;

		vecArr[1].x = maxX;
		vecArr[1].y = minY;
		vecArr[1].z = minZ;
		
		vecArr[2].x = maxX;
		vecArr[2].y = maxY;
		vecArr[2].z = minZ;

		vecArr[3].x = minX;
		vecArr[3].y = maxY;
		vecArr[3].z = minZ;
		
		// top
		vecArr[4].x = minX;
		vecArr[4].y = minY;
		vecArr[4].z = maxZ;

		vecArr[5].x = maxX;
		vecArr[5].y = minY;
		vecArr[5].z = maxZ;
		
		vecArr[6].x = maxX;
		vecArr[6].y = maxY;
		vecArr[6].z = maxZ;

		vecArr[7].x = minX;
		vecArr[7].y = maxY;
		vecArr[7].z = maxZ;
	}

	void include(UNCRZ_bbox* bbox, D3DXMATRIX* mat)
	{
		D3DXVECTOR4 vecs[8];
		D3DXVECTOR4 curVec;
		
		if (bbox->empty)
		{
			return;
		}

		bbox->transformVectors(mat, &(vecs[0]));

		for (int i = 0; i < 8; i++)
		{
			curVec = vecs[i];
			include(D3DXVECTOR3(curVec.x, curVec.y, curVec.z));
		}
	}

	void include(UNCRZ_bbox* bbox)
	{
		if (empty)
		{
			minX = bbox->minX;
			minY = bbox->minY;
			minZ = bbox->minZ;

			maxX = bbox->maxX;
			maxY = bbox->maxY;
			maxZ = bbox->maxZ;

			empty = false;
		}
		else
		{
			if (bbox->minX < minX)
				minX = bbox->minX;
			if (bbox->minY < minY)
				minY = bbox->minY;
			if (bbox->minZ < minZ)
				minZ = bbox->minZ;

			if (bbox->maxX > maxX)
				maxX = bbox->maxX;
			if (bbox->maxY > maxY)
				maxY = bbox->maxY;
			if (bbox->maxZ > maxZ)
				maxZ = bbox->maxZ;
		}
	}

	void include(D3DXVECTOR3 vec)
	{
		if (empty)
		{
			minX = vec.x;
			minY = vec.y;
			minZ = vec.z;

			maxX = vec.x;
			maxY = vec.y;
			maxZ = vec.z;

			empty = false;
		}
		else
		{
			if (vec.x < minX)
				minX = vec.x;
			if (vec.y < minY)
				minY = vec.y;
			if (vec.z < minZ)
				minZ = vec.z;

			if (vec.x > maxX)
				maxX = vec.x;
			if (vec.y > maxY)
				maxY = vec.y;
			if (vec.z > maxZ)
				maxZ = vec.z;
		}
	}
};

const DWORD LF_lit = 0x00000001; // uses multi-pass lighting to be pretty
const DWORD LF_shadows = 0x00000002; // rendered onto light maps

const DWORD LF_default = 0x00000003; // lit | shadows

struct UNCRZ_lodData
{
public:
	DWORD lightingFlags;

	/* draw dists (concept)
	float decalDrawDist; //  dist at which decals stop being drawn (from center of model/obj (not sure which yet))
	float drawDist; //  dist at which the model stops being drawn (from center of model/obj (not sure which yet))
	*/

};

struct UNCRZ_model
{
public:
	std::string name;

	LPDIRECT3DVERTEXBUFFER9 vBuff;
	LPDIRECT3DVERTEXDECLARATION9 vertexDec;
	LPDIRECT3DINDEXBUFFER9 iBuff;
	DWORD vertexType;
	UINT stride;
	int numVertices;
	int numIndices;
	UNCRZ_trans_arr* transArr;

	std::vector<UNCRZ_segment*> segments;
	std::vector<UNCRZ_segment*> allSegs;
	std::vector<UNCRZ_section*> sections;

	UNCRZ_FBF_anim_inst* animInst;

	void* vertexArray; // array of vertices
	short* indexArray; // array of indicies

	UNCRZ_bbox modelBox;
	bool noCull;
	
	void allSegsRequireUpdate();
	void update(LPD3DXMATRIX trans, bool forceUpdate = false);
	void drawMany(LPDIRECT3DDEVICE9 dxDevice, drawData* ddat, UNCRZ_model** arr, int count, DWORD drawArgs);
	void draw(LPDIRECT3DDEVICE9 dxDevice, drawData* ddat, DWORD drawArgs);
	void drawBBoxDebug(LPDIRECT3DDEVICE9 dxDevice, drawData* ddat);
	UNCRZ_model();
	UNCRZ_model(UNCRZ_model* gin);
	UNCRZ_model(std::string nameN);
	void createSegmentBoxes();
	void changeAnim(UNCRZ_FBF_anim* newAnim);
	void release();
	void clearDecals();
	void fillVBuff();
	void createVBuff(LPDIRECT3DDEVICE9 dxDevice, void* vds);
	void fillIBuff();
	void createIBuff(LPDIRECT3DDEVICE9 dxDevice, short* ids);
	bool collides(D3DXVECTOR3* rayPos, D3DXVECTOR3* rayDir, float* distRes);
	bool collidesVX_PC(D3DXVECTOR3* rayPos, D3DXVECTOR3* rayDir, float* distRes);
	bool collidesVX_PCT(D3DXVECTOR3* rayPos, D3DXVECTOR3* rayDir, float* distRes);
	int collidesVertex(D3DXVECTOR3* rayPos, D3DXVECTOR3* rayDir, float* distRes);
	int collidesVertexVX_PC(D3DXVECTOR3* rayPos, D3DXVECTOR3* rayDir, float* distRes);
	int collidesVertexVX_PCT(D3DXVECTOR3* rayPos, D3DXVECTOR3* rayDir, float* distRes);
};

struct UNCRZ_trans_arr
{
	std::vector<D3DXMATRIX> arr;
	int len;
	
	void setValue(int index, D3DXMATRIX* m)
	{
		arr[index] = *m;
	}
	
	D3DXMATRIX* getValue(int index)
	{
		return &arr[index];
	}

	void create(int lenN)
	{
		len = lenN;
		for (int i = 0; i < len; i++)
			arr.push_back(D3DXMATRIX());
	}

	UNCRZ_trans_arr() { }
};

struct UNCRZ_effect
{
public:
	std::string name;

	LPD3DXEFFECT effect;
	D3DXHANDLE colMod;
	D3DXHANDLE texture;
	D3DXHANDLE textureData;
	D3DXHANDLE targetVPMat;
	D3DXHANDLE targetTexture;
	D3DXHANDLE targetTextureData;
	D3DXHANDLE lightType;
	D3DXHANDLE lightCoof;
	D3DXHANDLE lightDepth;
	D3DXHANDLE lightConeness;
	D3DXHANDLE lightPos;
	D3DXHANDLE lightDir;
	D3DXHANDLE lightAmbient;
	D3DXHANDLE lightColMod;
	D3DXHANDLE lightTexture;
	D3DXHANDLE lightPatternTexture;
	D3DXHANDLE texture0;
	D3DXHANDLE texture1;
	D3DXHANDLE texture2;
	D3DXHANDLE texture3;
	D3DXHANDLE transArr;
	D3DXHANDLE spriteLoc;
	D3DXHANDLE spriteDim;
	D3DXHANDLE viewMat;
	D3DXHANDLE projMat;
	D3DXHANDLE viewProj;
	D3DXHANDLE eyePos;
	D3DXHANDLE eyeDir;
	D3DXHANDLE lightViewProj;
	D3DXHANDLE ticker;

	void setLightData(lightData* ld)
	{
		setLightType(ld->lightType);
		setLightDepth(ld->lightDepth);
		//setLightConeness(ld->coneness);
		setLightPos(&ld->lightPos);
		setLightDir(&ld->lightDir);
		setLightAmbient((float*)&ld->lightAmbient);
		setLightColMod((float*)&ld->lightColMod);

		if (ld->useLightMap)
		{
			setLightTexture(ld->lightTex);
			setLightViewProj(&ld->lightViewProj);
			setLightPatternTexture(ld->lightPatternTex);
		}
	}

	void setDynamicDecalData(dynamicDecalData* ddd)
	{
		setLightType(ddd->lightType);
		setLightDepth(ddd->lightDepth);
		setLightPos(&ddd->lightPos);
		setLightDir(&ddd->lightDir);
		setLightColMod((float*)&ddd->lightColMod);

		setLightViewProj(&ddd->lightViewProj);
		setLightPatternTexture(ddd->lightPatternTex);
	}

	void setcolMod(float* ptr)
	{
		effect->SetFloatArray(colMod, ptr, 4);
	}

	void setLightAmbient(float* ptr)
	{
		effect->SetFloatArray(lightAmbient, ptr, 4);
	}

	void setLightColMod(float* ptr)
	{
		effect->SetFloatArray(lightColMod, ptr, 4);
	}

	void setTexture(LPDIRECT3DTEXTURE9 tex)
	{
		effect->SetTexture(texture, tex);
	}

	void setTextureData(float* dat)
	{
		effect->SetFloatArray(textureData, dat, 4);
	}

	void setTargetTexture(LPDIRECT3DTEXTURE9 tex)
	{
		effect->SetTexture(targetTexture, tex);
	}

	void setTargetTextureData(float* dat)
	{
		effect->SetFloatArray(targetTextureData, dat, 4);
	}

	void setTargetVPMat(float* mat)
	{
		effect->SetFloatArray(targetVPMat, mat, 16);
	}

	void setLightCoof(float coof)
	{
		effect->SetFloatArray(lightCoof, &coof, 1);
	}

	void setLightDepth(float coof)
	{
		effect->SetFloatArray(lightDepth, &coof, 1);
	}

	void setLightConeness(float coneness)
	{
		effect->SetFloatArray(lightConeness, &coneness, 1);
	}

	void setLightType(float type)
	{
		effect->SetFloatArray(lightType, &type, 1);
	}

	void setLightPos(D3DXVECTOR4* pos)
	{
		effect->SetFloatArray(lightPos, (float*)pos, 4);
	}

	void setLightDir(D3DXVECTOR4* dir)
	{
		effect->SetFloatArray(lightDir, (float*)dir, 4);
	}

	void setLightTexture(LPDIRECT3DTEXTURE9 tex)
	{
		effect->SetTexture(lightTexture, tex);
	}

	void setLightPatternTexture(LPDIRECT3DTEXTURE9 tex)
	{
		effect->SetTexture(lightPatternTexture, tex);
	}

	void setTexture0(LPDIRECT3DTEXTURE9 tex)
	{
		effect->SetTexture(texture0, tex);
	}

	void setTexture1(LPDIRECT3DTEXTURE9 tex)
	{
		effect->SetTexture(texture1, tex);
	}

	void setTexture2(LPDIRECT3DTEXTURE9 tex)
	{
		effect->SetTexture(texture2, tex);
	}

	void setTexture3(LPDIRECT3DTEXTURE9 tex)
	{
		effect->SetTexture(texture3, tex);
	}

	void setTransArr(UNCRZ_trans_arr* tarr)
	{
		HRESULT res = effect->SetFloatArray(transArr, (float*)tarr->arr.front(), tarr->len * 16);
	}

	void setSpriteLoc(float* larr, int offset, int num, int dataSizeInFloats)
	{
		HRESULT res = effect->SetFloatArray(spriteLoc, (float*)(larr + offset * dataSizeInFloats), num * dataSizeInFloats);
		if (res != D3D_OK)
			std::cout << "Help!";
	}

	void setSpriteDim(D3DXVECTOR4* dim)
	{
		HRESULT res = effect->SetFloatArray(spriteDim, (float*)dim, 4);
	}

	void setViewMat(LPD3DXMATRIX viewMatrix)
	{
		effect->SetMatrix(viewMat, viewMatrix);
	}

	void setProjMat(LPD3DXMATRIX projMatrix)
	{
		effect->SetMatrix(projMat, projMatrix);
	}

	void setViewProj(LPD3DXMATRIX viewProjMatrix)
	{
		effect->SetMatrix(viewProj, viewProjMatrix);
	}

	void setEyePos(D3DXVECTOR4* pos)
	{
		effect->SetFloatArray(eyePos, (float*)pos, 4);
	}

	void setEyeDir(D3DXVECTOR4* dir)
	{
		effect->SetFloatArray(eyeDir, (float*)dir, 4);
	}

	void setLightViewProj(LPD3DXMATRIX lightViewProjMatrix)
	{
		effect->SetMatrix(lightViewProj, lightViewProjMatrix);
	}

	void setTechnique(D3DXHANDLE tech)
	{
		effect->SetTechnique(tech);
	}

	void setTicker(float val)
	{
		effect->SetFloat(ticker, val);
	}

	UNCRZ_effect() { }
	
	static UNCRZ_effect UNCRZ_effectFromFile(char* fileName, LPDIRECT3DDEVICE9 dxDevice, DWORD vertexType, std::string nameN)
	{
		LPD3DXEFFECT tempEffect;
		LPD3DXBUFFER errs = NULL;
		HRESULT res = D3DXCreateEffectFromFile(dxDevice, fileName, NULL, NULL, 0, NULL, &tempEffect, &errs);

		return UNCRZ_effect(tempEffect, vertexType, nameN);
	}

	UNCRZ_effect(LPD3DXEFFECT effectN, DWORD vertexType, std::string nameN)
	{
		effect = effectN;

		// vPC, vPCT
		colMod = effect->GetParameterByName(NULL, "colMod");
		// vPCT
		texture = effect->GetParameterByName(NULL, "tex");
		textureData = effect->GetParameterByName(NULL, "texData");
		
		// vPTW4
		texture0 = effect->GetParameterByName(NULL, "tex0");
		texture1 = effect->GetParameterByName(NULL, "tex1");
		texture2 = effect->GetParameterByName(NULL, "tex2");
		texture3 = effect->GetParameterByName(NULL, "tex3");

		targetVPMat = effect->GetParameterByName(NULL, "vpMat");
		targetTexture = effect->GetParameterByName(NULL, "targTex");
		targetTextureData = effect->GetParameterByName(NULL, "targTexData");
		lightType = effect->GetParameterByName(NULL, "lightType");
		lightCoof = effect->GetParameterByName(NULL, "lightCoof");
		lightDepth = effect->GetParameterByName(NULL, "lightDepth");
		lightConeness = effect->GetParameterByName(NULL, "lightConeness");
		lightPos = effect->GetParameterByName(NULL, "lightPos");
		lightDir = effect->GetParameterByName(NULL, "lightDir");
		lightAmbient = effect->GetParameterByName(NULL, "lightAmbient");
		lightColMod = effect->GetParameterByName(NULL, "lightColMod");
		lightTexture = effect->GetParameterByName(NULL, "lightTex");
		lightPatternTexture = effect->GetParameterByName(NULL, "lightPatternTex");
		transArr = effect->GetParameterByName(NULL, "transarr");
		spriteLoc = effect->GetParameterByName(NULL, "spriteLoc");
		spriteDim = effect->GetParameterByName(NULL, "spriteDim");
		viewMat = effect->GetParameterByName(NULL, "viewMat");
		projMat = effect->GetParameterByName(NULL, "projMat");
		viewProj = effect->GetParameterByName(NULL, "viewProj");
		eyePos = effect->GetParameterByName(NULL, "eyePos");
		eyeDir = effect->GetParameterByName(NULL, "eyeDir");
		lightViewProj = effect->GetParameterByName(NULL, "lightViewProj");
		ticker = effect->GetParameterByName(NULL, "ticker");
		name = nameN;
	}

	void release()
	{
		effect->Release();
	}
};

struct UNCRZ_FBF_anim_flow_start
{
	int motion;
	int motionDuration;

	UNCRZ_FBF_anim_flow_start()
	{
		motion = 0;
		motionDuration = 0;
	}

	UNCRZ_FBF_anim_flow_start(int motionN, int motionDurationN)
	{
		motion = motionN;
		motionDuration = motionDurationN;
	}
};

struct UNCRZ_FBF_anim_flow_state
{
	UNCRZ_FBF_anim_flow_start* start;
	int curMotion;
	int curMotionDuration;

	void reset()
	{
		curMotion = start->motion;
		curMotionDuration = start->motionDuration;
	}

	UNCRZ_FBF_anim_flow_state(UNCRZ_FBF_anim_flow_start* startN)
	{
		start = startN;
		reset();
	}
};

struct UNCRZ_FBF_anim_flow
{
public:
	UNCRZ_FBF_anim_flow_start start;
	std::string name;
	std::vector<UNCRZ_FBF_anim_instr> instrs;
	std::vector<UNCRZ_FBF_anim_motion> motions;

	void run(UNCRZ_FBF_anim_inst* inst, UNCRZ_FBF_anim_flow_state* state);
	UNCRZ_FBF_anim_flow() { }
	UNCRZ_FBF_anim_flow(std::string nameN);
};

struct UNCRZ_FBF_anim
{
public:
	std::vector<UNCRZ_FBF_anim_flow> flows;
	UNCRZ_model* model; // used for create, and NOTHING ELSE
	UNCRZ_FBF_anim_flow* lastFlow;

	std::string name;
	std::string modelName; // used for checking if a model can use it

	UNCRZ_FBF_anim::UNCRZ_FBF_anim() { }

	UNCRZ_FBF_anim::UNCRZ_FBF_anim(std::string nameN)
	{
		name = nameN;
	}

	void run(UNCRZ_FBF_anim_inst* inst);
	void addFlow(std::string name);
	void setFlowStart(int sm, int smd);
	void setFlowStart(std::string sms, int smd);
	void endMotion();
	void addMotion(std::string name);
	void setMotionDuration(int duration);
	void setMotionCausesUpdate(bool mg);
	void addLine(std::string line);
	UNCRZ_FBF_anim(UNCRZ_model* modelN);
};

struct UNCRZ_FBF_anim_inst
{
	UNCRZ_model* model;
	std::vector<UNCRZ_FBF_anim_flow_state> states;
	UNCRZ_FBF_anim* anim;

	void reset(UNCRZ_FBF_anim* animN)
	{
		anim = animN;
		int olen = states.size() - 1;
		for (int i = anim->flows.size() - 1; i >= olen; i--)
		{
			states.push_back(UNCRZ_FBF_anim_flow_state(&anim->flows[i].start));
		}
		for (int i = states.size() - 1; i >= 0; i--)
		{
			states[i].reset();
		}
	}

	UNCRZ_FBF_anim_inst(UNCRZ_model* modelN, UNCRZ_FBF_anim* animN)
	{
		anim = animN;
		model = modelN;
		for (int i = anim->flows.size() - 1; i >= 0; i--)
		{
			states.push_back(UNCRZ_FBF_anim_flow_state(&anim->flows[i].start));
		}
	}
};

const DWORD AM_none = 0x00000000;
const DWORD AM_nice = 0x00000001;
const DWORD AM_add = 0x00000002;

const DWORD LM_none = 0x00000000;
const DWORD LM_full = 0x00000001;

const DWORD SD_none = 0x00000000; // don't use this
const DWORD SD_colour = 0x00000001;
const DWORD SD_alpha = 0x00000002;
const DWORD SD_default = 0x00000003;

// this is for internal use only
const DWORD SD_itrl_sidewise = 0x00000004; // see drawSideWise

struct UNCRZ_sprite_data
{
public:
	D3DXVECTOR4 pos;
	D3DXVECTOR4 other;

	UNCRZ_sprite_data(D3DXVECTOR4 posN, D3DXVECTOR4 otherN)
	{
		pos = posN;
		other = otherN;
	}

	UNCRZ_sprite_data(float x, float y, float z, float w, float ox, float oy, float oz, float ow)
	{
		pos = D3DXVECTOR4(x, y, z, w);
		other = D3DXVECTOR4(ox, oy, oz, ow);
	}
};

struct UNCRZ_sprite_buffer
{
public:
	int size;

	void* vertexArray; // array of vertices
	short* indexArray; // array of indicies

	LPDIRECT3DVERTEXBUFFER9 vBuff;
	LPDIRECT3DVERTEXDECLARATION9 vertexDec; // vertexDecPCT (always)
	LPDIRECT3DINDEXBUFFER9 iBuff;
	DWORD vertexType;
	UINT stride;
	int numVertices;
	int numIndices;

	UNCRZ_sprite_buffer()
	{
	}

	void create(LPDIRECT3DDEVICE9 dxDevice, LPDIRECT3DVERTEXDECLARATION9 vertexDecN, int sizeN)
	{
		size = sizeN;
		stride = sizeof(vertexPCT);
		vertexType = VX_PCT;
		vertexDec = vertexDecN;
	
		std::vector<vertexPCT> vPCTs;
		std::vector<short> indicies;

		int ci = 0;
		for (int i = 0; i < size; i++)
		{
			int tti = i * sizeof(UNCRZ_sprite_data) / (sizeof(float) * 4);
			vPCTs.push_back(vertexPCT(vertexPC(-1, -1, 1, 1, 1, 1, tti), 0, 0));
			vPCTs.push_back(vertexPCT(vertexPC(1, -1, 1, 1, 1, 1, tti), 1, 0));
			vPCTs.push_back(vertexPCT(vertexPC(1, 1, 1, 1, 1, 1, tti), 1, 1));
			vPCTs.push_back(vertexPCT(vertexPC(-1, 1, 1, 1, 1, 1, tti), 0, 1));

			indicies.push_back((short)ci + 0);
			indicies.push_back((short)ci + 1);
			indicies.push_back((short)ci + 2);

			indicies.push_back((short)ci + 0);
			indicies.push_back((short)ci + 2);
			indicies.push_back((short)ci + 3);

			ci += 4;
		}

		numVertices = (int)vPCTs.size();
		createVBuff(dxDevice, (void*)&vPCTs.front());
		numIndices = (int)indicies.size();
		createIBuff(dxDevice, (short*)&indicies.front());
	}

	void fillVBuff()
	{
		VOID* buffPtr;
		vBuff->Lock(0, numVertices * stride, (VOID**)&buffPtr, D3DLOCK_DISCARD);
		memcpy(buffPtr, vertexArray, numVertices * stride);
		vBuff->Unlock();
	}

	void createVBuff(LPDIRECT3DDEVICE9 dxDevice, void* vds)
	{
		dxDevice->CreateVertexBuffer(numVertices * stride, D3DUSAGE_WRITEONLY, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, D3DPOOL_DEFAULT, &vBuff, NULL);

		vertexArray = malloc(stride * numVertices);
		memcpy(vertexArray, vds, numVertices * stride);

		fillVBuff();
	}

	void fillIBuff()
	{
		VOID* buffPtr;
		iBuff->Lock(0, numIndices * sizeof (short), (VOID**)&buffPtr, D3DLOCK_DISCARD);
		memcpy(buffPtr, indexArray, numIndices * sizeof (short));
		iBuff->Unlock();
	}

	void createIBuff(LPDIRECT3DDEVICE9 dxDevice, short* ids)
	{
		dxDevice->CreateIndexBuffer(numIndices * sizeof (short), D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &iBuff, NULL);

		indexArray = (short*)malloc(sizeof (short) * numIndices);
		memcpy(indexArray, ids, numIndices * sizeof (short));

		fillIBuff();
	}
};

// sprite re-implements most of UNCRZ_section
struct UNCRZ_sprite
{
public:
	std::string name;

	UNCRZ_effect effect;
	D3DXHANDLE tech;
	D3DXHANDLE lightTech;
	D3DXHANDLE overTech;
	D3DXVECTOR4 colMod;

	bool useTex;
	LPDIRECT3DTEXTURE9 tex;
	bool useTex0;
	LPDIRECT3DTEXTURE9 tex0;
	bool useTex1;
	LPDIRECT3DTEXTURE9 tex1;
	bool useTex2;
	LPDIRECT3DTEXTURE9 tex2;
	bool useTex3;
	LPDIRECT3DTEXTURE9 tex3;

	float dimX, dimY, dimZ;
	DWORD alphaMode;
	DWORD sideWiseAlphaMode;
	DWORD lightingMode;

	void zeroIsh();
	void setAlpha(LPDIRECT3DDEVICE9);
	void setTextures();
	void setSideWiseAlpha(LPDIRECT3DDEVICE9);
	void draw(LPDIRECT3DDEVICE9, drawData*, UNCRZ_sprite_buffer*, UNCRZ_sprite_data*, int, int, DWORD, DWORD);
	void drawSideWise(LPDIRECT3DDEVICE9, drawData*, UNCRZ_sprite_buffer*, UNCRZ_sprite_data*, int, int, DWORD, DWORD);
	void drawSideOver(LPDIRECT3DDEVICE9, drawData*);
	void drawToSide(LPDIRECT3DDEVICE9, drawData*, UNCRZ_sprite_buffer*, UNCRZ_sprite_data*, int, int, DWORD, DWORD);

	UNCRZ_sprite();
	UNCRZ_sprite(std::string);
};

struct UNCRZ_section
{
public:
	std::string name;

	UNCRZ_effect effect;
	bool useTex;
	LPDIRECT3DTEXTURE9 tex;
	bool useTex0;
	LPDIRECT3DTEXTURE9 tex0;
	bool useTex1;
	LPDIRECT3DTEXTURE9 tex1;
	bool useTex2;
	LPDIRECT3DTEXTURE9 tex2;
	bool useTex3;
	LPDIRECT3DTEXTURE9 tex3;
	D3DXVECTOR4 colMod;
	D3DXHANDLE tech;
	D3DXHANDLE lightTech;
	D3DXHANDLE decalTech;
	D3DXHANDLE overTech;
	DWORD vertexType;
	DWORD alphaMode;
	DWORD lightingMode;

	int vOffset; // in Indices
	int vLen; // in Triangles

	int numVertices; // needed for DrawIndexedPrim call

	// decals?
	std::vector<UNCRZ_decal*> decals;
	bool drawDecals;
	bool acceptDecals;
	
	bool sectionEnabled; // whether it should draw or not

	// indicates whether the section should not be rendered for the rest of this draw sequence (for multi-draw sequences)
	bool curDrawCull;

	void setAlpha(LPDIRECT3DDEVICE9 dxDevice)
	{
		switch (alphaMode)
		{
		case AM_none:
			dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
			break;
		case AM_nice:
			dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			dxDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
			dxDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			dxDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
			break;
		case AM_add:
			dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			dxDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
			dxDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			dxDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
			break;
		}
	}

	void setTextures()
	{
		if (useTex)
			effect.setTexture(tex);
		if (useTex0)
			effect.setTexture0(tex0);
		if (useTex1)
			effect.setTexture1(tex1);
		if (useTex2)
			effect.setTexture2(tex2);
		if (useTex3)
			effect.setTexture3(tex3);
	}

	// doesn't support any alphaMode except AM_none
	void drawMany(LPDIRECT3DDEVICE9 dxDevice, drawData* ddat, UNCRZ_model** arr, int count, int secIndex, DWORD drawArgs)
	{
		HRESULT res;

		// model
		if (drawArgs & DF_light)
		{
			effect.setTechnique(lightTech);
			effect.setLightType(ddat->lightType);
			effect.setLightDepth(ddat->lightDepth);
			effect.setLightConeness(ddat->lightConeness);
		}
		else
		{
			effect.setTechnique(tech);
			effect.setLightCoof(ddat->lightCoof);
		}

		setTextures();

		effect.setEyePos(&ddat->eyePos);
		effect.setEyeDir(&ddat->eyeDir);
		effect.setTicker(ddat->ticker);
		effect.setViewProj(ddat->viewProj);

		effect.effect->CommitChanges();

		setAlpha(dxDevice);

		UINT numPasses, pass;
			effect.effect->Begin(&numPasses, 0);

		// pass 0
		if (drawArgs & DF_light)
			effect.effect->BeginPass((int)ddat->lightType);
		else if (ddat->performPlainPass)
			effect.effect->BeginPass(0);
		else
			goto skipPlainPass;

		for (int i = count - 1; i >= 0; i--)
		{
			if (arr[i]->sections[secIndex]->curDrawCull)
				continue;
			if (vertexType == VX_PC || vertexType == VX_PCT)
				effect.setcolMod(&arr[i]->sections[secIndex]->colMod.x);
			effect.setTransArr(arr[i]->transArr);
			effect.effect->CommitChanges();
			res = dxDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, numVertices, vOffset, vLen);
			if (res != D3D_OK)
				res = res;
		}
		effect.effect->EndPass();
skipPlainPass:

		if (lightingMode == LM_full && numPasses > 1 && (drawArgs & DF_light) == false)
		{
			// enable blend
			dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			dxDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
			dxDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
			dxDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

			// light pass
			for (int ldi = ddat->lightDatas.size() - 1; ldi >= 0; ldi--)
			{
				lightData* ld = ddat->lightDatas[ldi];
				
				if (!ld->lightEnabled)
					continue;
				
				effect.setLightData(ld);
				effect.effect->CommitChanges();

				effect.effect->BeginPass((int)ld->lightType + 1);
				for (int i = count - 1; i >= 0; i--)
				{
					if (arr[i]->sections[secIndex]->curDrawCull)
						continue;
					if (vertexType == VX_PC || vertexType == VX_PCT)
						effect.setcolMod(&arr[i]->sections[secIndex]->colMod.x);
					effect.setTransArr(arr[i]->transArr);
					effect.effect->CommitChanges();
					res = dxDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, numVertices, vOffset, vLen);
					if (res != D3D_OK)
						res = res;
				}
				effect.effect->EndPass();
			}

		}
		// disable blend
		dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

		effect.effect->End();

		/*UINT numPasses, pass;
		effect.effect->Begin(&numPasses, 0);

		for (pass = 0; pass < numPasses; pass++)
		{
			effect.effect->BeginPass(pass);

			for (int i = count - 1; i >= 0; i--)
			{
				if (vertexType == VX_PC || vertexType == VX_PCT)
					effect.setcolMod(&arr[i]->sections[secIndex]->colMod.x);
				effect.setTransArr(arr[i]->transArr);
				effect.effect->CommitChanges();
				res = dxDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, numVertices, vOffset, vLen);
				if (res != D3D_OK)
					res = res;
			}

			effect.effect->EndPass();
		}

		effect.effect->End();*/

		if (drawDecals == false || drawArgs & DF_light)
			return; // no decals for light

		// decals
		dxDevice->SetVertexDeclaration(vertexDecPAT4);
		effect.setTechnique(decalTech);

		effect.effect->CommitChanges();

		dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		dxDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
		dxDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		dxDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

		effect.effect->Begin(&numPasses, 0);

		// pass 0
		if (drawArgs & DF_light)
			effect.effect->BeginPass((int)ddat->lightType);
		else if (ddat->performPlainPass)
			effect.effect->BeginPass(0);
		else
			goto skipPlainDecalPass;

		for (int i = count - 1; i >= 0; i--)
		{
			UNCRZ_section* curSec = arr[i]->sections[secIndex];
			if (curSec->curDrawCull)
				continue;
			if (curSec->decals.size() != 0)
			{
				effect.setcolMod(&arr[i]->sections[secIndex]->colMod.x);
				effect.setTransArr(arr[i]->transArr);

				//for (int i = curSec->decals.size() - 1; i >= 0; i--)
				for (int i = 0; i < curSec->decals.size(); i++)
				{
					effect.setTexture(curSec->decals[i]->tex);
					effect.effect->CommitChanges();
					res = dxDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST, curSec->decals[i]->numFaces, curSec->decals[i]->vertexArray, sizeof(vertexPAT4));
					if (res != D3D_OK)
						res = res;
				}
			}
		}
		effect.effect->EndPass();
skipPlainDecalPass:

		if (lightingMode == LM_full && numPasses > 1 && (drawArgs & DF_light) == false)
		{
			// enable blend
			dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			dxDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
			dxDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			dxDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

			// light pass
			for (int ldi = ddat->lightDatas.size() - 1; ldi >= 0; ldi--)
			{
				lightData* ld = ddat->lightDatas[ldi];
				
				if (!ld->lightEnabled)
					continue;

				effect.setLightData(ld);
				effect.effect->CommitChanges();

				effect.effect->BeginPass((int)ld->lightType + 1);
				for (int i = count - 1; i >= 0; i--)
				{
					UNCRZ_section* curSec = arr[i]->sections[secIndex];
					if (curSec->decals.size() != 0)
					{
						effect.setcolMod(&arr[i]->sections[secIndex]->colMod.x);
						effect.setTransArr(arr[i]->transArr);

						//for (int i = curSec->decals.size() - 1; i >= 0; i--)
						for (int i = 0; i < curSec->decals.size(); i++)
						{
							effect.setTexture(curSec->decals[i]->tex);
							effect.effect->CommitChanges();
							res = dxDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST, curSec->decals[i]->numFaces, curSec->decals[i]->vertexArray, sizeof(vertexPAT4));
							if (res != D3D_OK)
								res = res;
						}
					}
				}
				effect.effect->EndPass();
			}

			// disable blend
			dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		}

		effect.effect->End();

		/*effect.effect->Begin(&numPasses, 0);

		for (pass = 0; pass < numPasses; pass++)
		{
			effect.effect->BeginPass(pass);

			for (int i = count - 1; i >= 0; i--)
			{
				UNCRZ_section* curSec = arr[i]->sections[secIndex];
				if (curSec->decals.size() != 0)
				{
					effect.setcolMod(&arr[i]->sections[secIndex]->colMod.x);
					effect.setTransArr(arr[i]->transArr);

					//for (int i = curSec->decals.size() - 1; i >= 0; i--)
					for (int i = 0; i < curSec->decals.size(); i++)
					{
						effect.setTexture(curSec->decals[i]->tex);
						effect.effect->CommitChanges();
						res = dxDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST, curSec->decals[i]->numFaces, curSec->decals[i]->vertexArray, sizeof(vertexPAT4));
						if (res != D3D_OK)
							res = res;
					}
				}
			}

			effect.effect->EndPass();
		}

		effect.effect->End();*/
		
		dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	}

	void draw(LPDIRECT3DDEVICE9 dxDevice, UNCRZ_trans_arr* transArr, drawData* ddat, DWORD drawArgs)
	{
		effect.setEyePos(&ddat->eyePos);
		effect.setEyeDir(&ddat->eyeDir);
		effect.setTicker(ddat->ticker);

		if (sectionEnabled == false)
			return;

		if (drawArgs & DF_light || alphaMode == AM_none)
		{
			drawDraw(dxDevice, transArr, ddat, drawArgs);
		}
		else
		{
			drawToSide(dxDevice, transArr, ddat, drawArgs);
			drawSideOver(dxDevice, ddat);
		}
	}

	void drawSideOver(LPDIRECT3DDEVICE9 dxDevice, drawData* ddat)
	{
		D3DXMATRIX idMat;
		D3DXMatrixIdentity(&idMat);
		dxDevice->SetRenderTarget(0, ddat->targetSurface);
		//dxDevice->SetViewport(ddat->targetVp);

		dxDevice->SetRenderState(D3DRS_ZENABLE, false);
		dxDevice->SetRenderState(D3DRS_ZWRITEENABLE, false);

		vertexOver overVerts[4]; // make this const / VBuff
		overVerts[0] = vertexOver(-1, -1, 0, 0, 1);
		overVerts[1] = vertexOver(-1, 1, 0, 0, 0);
		overVerts[2] = vertexOver(1, -1, 0, 1, 1);
		overVerts[3] = vertexOver(1, 1, 0, 1, 0);

		D3DXVECTOR4 texData = D3DXVECTOR4(0.5 / (float)ddat->targetVp->Width, 0.5 / (float)ddat->targetVp->Height, 1.0 / (float)ddat->targetVp->Width, 1.0 / (float)ddat->targetVp->Height);
		effect.setTextureData((float*)&texData.x);
		
		for (int i = 0; i < 4; i++) // do ahead of shader
		{
			overVerts[i].tu += texData.x;
			overVerts[i].tv += texData.y;
		}

		effect.setTexture(ddat->sideTex);
		effect.setTechnique(overTech);
		effect.setViewProj(&idMat);
		effect.effect->CommitChanges();

		setAlpha(dxDevice);

		UINT numPasses, pass;
		effect.effect->Begin(&numPasses, 0);
		effect.effect->BeginPass(0);

		dxDevice->SetVertexDeclaration(vertexDecOver);
		dxDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, overVerts, sizeof(vertexOver));

		effect.effect->EndPass();
		effect.effect->End();
		
		dxDevice->SetRenderState(D3DRS_ZENABLE, true);
		dxDevice->SetRenderState(D3DRS_ZWRITEENABLE, true);
	}

	void drawToSide(LPDIRECT3DDEVICE9 dxDevice, UNCRZ_trans_arr* transArr, drawData* ddat, DWORD drawArgs)
	{
		dxDevice->SetRenderTarget(0, ddat->sideSurface);
		dxDevice->SetViewport(ddat->sideVp);
		//dxDevice->SetDepthStencilSurface(ddat->zSideSurface);

		dxDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 0, 0);
		dxDevice->SetRenderState(D3DRS_ZENABLE, true);
		dxDevice->SetRenderState(D3DRS_ZWRITEENABLE, true);

		effect.setTargetTexture(ddat->targetTex);
		D3DXVECTOR4 targTexData = D3DXVECTOR4(0.5 / (float)ddat->targetVp->Width, 0.5 / (float)ddat->targetVp->Height, 1.0 / (float)ddat->targetVp->Width, 1.0 / (float)ddat->targetVp->Height);
		effect.setTargetTextureData((float*)&targTexData.x);
		
		// targetVP allows the shader to sample the target texture
		float matDat[16] = {
			(float)ddat->targetVp->Width / 2.0, 0.0, 0.0, 0.0,
			0.0, -(float)ddat->targetVp->Height / 2.0, 0.0, 0.0,
			0.0, 0.0, ddat->targetVp->MaxZ - ddat->targetVp->MinZ, 0.0,
			ddat->targetVp->X + (float)ddat->targetVp->Width / 2.0, ddat->targetVp->Y + (float)ddat->targetVp->Height / 2.0, ddat->targetVp->MinZ, 1.0
		};
		//D3DXMATRIX vpMat(&matDat);

		effect.setTargetVPMat((float*)&matDat);

		drawDraw(dxDevice, transArr, ddat, drawArgs);
	}

	void drawDraw(LPDIRECT3DDEVICE9 dxDevice, UNCRZ_trans_arr* transArr, drawData* ddat, DWORD drawArgs)
	{
		HRESULT res;

		if (drawArgs & DF_light)
		{
			effect.setTechnique(lightTech);
			effect.setLightType(ddat->lightType);
			effect.setLightDepth(ddat->lightDepth);
			effect.setLightConeness(ddat->lightConeness);
		}
		else
		{
			effect.setTechnique(tech);
			effect.setLightCoof(ddat->lightCoof);
		}

		effect.setTransArr(transArr);
		if (vertexType == VX_PC || vertexType == VX_PCT)
			effect.setcolMod(&colMod.x);
		
		setTextures();

		// disable blend (either side render  or  alphaMode is set to AM_none  or  DF_light)
		dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

		effect.setViewProj(ddat->viewProj);

		effect.effect->CommitChanges();

		UINT numPasses, pass;
		effect.effect->Begin(&numPasses, 0);

		// pass 0
		if (drawArgs & DF_light)
			effect.effect->BeginPass((int)ddat->lightType);
		else if (ddat->performPlainPass)
			effect.effect->BeginPass(0);
		else
			goto skipPlainPass;

		res = dxDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, numVertices, vOffset, vLen);
		if (res != D3D_OK)
			res = res;
		effect.effect->EndPass();
skipPlainPass:

		if (lightingMode == LM_full && numPasses > 1 && (drawArgs & DF_light) == false)
		{
			// enable blend
			dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			dxDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
			dxDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
			dxDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

			// light pass
			for (int ldi = ddat->lightDatas.size() - 1; ldi >= 0; ldi--)
			{
				lightData* ld = ddat->lightDatas[ldi];
				
				if (!ld->lightEnabled)
					continue;

				effect.setLightData(ld);
				effect.effect->CommitChanges();

				effect.effect->BeginPass((int)ld->lightType + 1);
				res = dxDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, numVertices, vOffset, vLen);
				if (res != D3D_OK)
					res = res;
				effect.effect->EndPass();
			}

		}
		// disable blend
		dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

		effect.effect->End();

		/*UINT numPasses, pass;
		effect.effect->Begin(&numPasses, 0);

		for (pass = 0; pass < numPasses; pass++)
		{
			effect.effect->BeginPass(pass);
			// draw model
			res = dxDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, numVertices, vOffset, vLen);
			if (res != D3D_OK)
				res = res;
			effect.effect->EndPass();
		}

		effect.effect->End();*/

		if (drawDecals == false || drawArgs & DF_light || decals.size() == 0)
			return; // no decals for light

		// decals
		LPDIRECT3DVERTEXDECLARATION9 oldDec;

		dxDevice->GetVertexDeclaration(&oldDec);
		dxDevice->SetVertexDeclaration(vertexDecPAT4);

		effect.setTechnique(decalTech);

		effect.effect->CommitChanges();

		dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		dxDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
		dxDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		dxDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

		effect.effect->Begin(&numPasses, 0);

		// pass 0
		if (drawArgs & DF_light)
			effect.effect->BeginPass((int)ddat->lightType);
		else if (ddat->performPlainPass)
			effect.effect->BeginPass(0);
		else
			goto skipPlainDecalPass;

		for (int i = 0; i < decals.size(); i++)
		{
			effect.setTexture(decals[i]->tex);
			effect.effect->CommitChanges();
			res = dxDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST, decals[i]->numFaces, decals[i]->vertexArray, sizeof(vertexPAT4));
			if (res != D3D_OK)
				res = res;
		}
		effect.effect->EndPass();
skipPlainDecalPass:

		if (lightingMode == LM_full && numPasses > 1 && (drawArgs & DF_light) == false)
		{
			// enable blend
			dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			dxDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
			dxDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			dxDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

			// light pass
			for (int ldi = ddat->lightDatas.size() - 1; ldi >= 0; ldi--)
			{
				lightData* ld = ddat->lightDatas[ldi];
				
				if (!ld->lightEnabled)
					continue;

				effect.setLightData(ld);
				effect.effect->CommitChanges();

				effect.effect->BeginPass((int)ld->lightType + 1);
				for (int i = 0; i < decals.size(); i++)
				{
					effect.setTexture(decals[i]->tex);
					effect.effect->CommitChanges();
					res = dxDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST, decals[i]->numFaces, decals[i]->vertexArray, sizeof(vertexPAT4));
					if (res != D3D_OK)
						res = res;
				}
				effect.effect->EndPass();
			}

		}
		// disable blend
		dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

		effect.effect->End();

		/*effect.effect->Begin(&numPasses, 0);

		for (pass = 0; pass < numPasses; pass++)
		{
			effect.effect->BeginPass(pass);

			for (int i = 0; i < decals.size(); i++)
			{
				effect.setTexture(decals[i]->tex);
				effect.effect->CommitChanges();
				res = dxDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST, decals[i]->numFaces, decals[i]->vertexArray, sizeof(vertexPAT4));
				if (res != D3D_OK)
					res = res;
			}

			effect.effect->EndPass();
		}

		effect.effect->End();*/

		dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		
		dxDevice->SetVertexDeclaration(oldDec);
	}

	void zeroIsh()
	{
		useTex = false;
		useTex0 = false;
		useTex1 = false;
		useTex2 = false;
		useTex3 = false;
	}

	UNCRZ_section()
	{
		zeroIsh();
	}
	
	UNCRZ_section(UNCRZ_section* gin)
	{
		zeroIsh();
		name = gin->name;
		tech = gin->tech;
		lightTech = gin->lightTech;
		decalTech = gin->decalTech;
		overTech = gin->overTech;
		lightingMode = gin->lightingMode;
		alphaMode = gin->alphaMode;
		colMod = gin->colMod;
		effect = gin->effect;
		vOffset = gin->vOffset;
		vLen = gin->vLen;
		numVertices = gin->numVertices;
		vertexType = gin->vertexType;
		drawDecals = gin->drawDecals;
		acceptDecals = gin->acceptDecals;
		sectionEnabled = gin->sectionEnabled;
		useTex = gin->useTex;
		tex = gin->tex;
		useTex0 = gin->useTex0;
		tex0 = gin->tex0;
		useTex1 = gin->useTex1;
		tex1 = gin->tex1;
		useTex2 = gin->useTex2;
		tex2 = gin->tex2;
		useTex3 = gin->useTex3;
		tex3 = gin->tex3;
	}

	UNCRZ_section(std::string nameN)
	{
		zeroIsh();
		name = nameN;
	}

	void release()
	{
		effect.release();
		tex->Release();
	}
};

struct UNCRZ_blend
{
public:
	std::string name;

	UNCRZ_segment* parent;

	D3DXVECTOR3 offset;
	D3DXVECTOR3 rotation;
	D3DXMATRIX offsetMatrix; // call updateMatrices whenever the offset are modified
	D3DXMATRIX rotationMatrix; // call updateMatrices whenever the offset/rotation are modified
	D3DXMATRIX offsetMatrixinv; // call updateMatrices whenever the offse are modified
	D3DXMATRIX rotationMatrixinv; // call updateMatrices whenever the offset/rotation are modified
	D3DXMATRIX localTrans; // call updateMatrices whenever the offset/rotation are modified
	D3DXMATRIX localTransinv; // call updateMatrices whenever the offset/rotation are modified

	int transIndex;
	float prop;

	UNCRZ_blend(std::string nameN, int tti, float propN, UNCRZ_segment* parentN)
	{
		name = nameN;
		transIndex = tti;
		prop = propN;
		parent = parentN;
	}

	void update(D3DXMATRIX trans, UNCRZ_trans_arr* transArr)
	{
		updateMatrices();

		D3DXMatrixMultiply(&trans, &offsetMatrix, &trans);
		D3DXMatrixMultiply(&trans, &rotationMatrix, &trans);

		transArr->setValue(transIndex, &trans);
	}

	void updateMatrices()
	{
		D3DXMatrixTranslation(&offsetMatrix, offset.x, offset.y, offset.z);
		D3DXMatrixRotationYawPitchRoll(&rotationMatrix, rotation.y, rotation.x, rotation.z);
		D3DXMatrixMultiply(&localTrans, &offsetMatrix, &rotationMatrix);
		
		D3DXMatrixInverse(&offsetMatrixinv, NULL, &offsetMatrix);
		D3DXMatrixInverse(&rotationMatrixinv, NULL, &rotationMatrix);
		D3DXMatrixInverse(&localTransinv, NULL, &localTrans);
	}
};

struct UNCRZ_segment
{
public:
	std::string name;

	D3DXVECTOR3 offset;
	D3DXVECTOR3 rotation;
	D3DXMATRIX offsetMatrix; // call updateMatrices whenever the offset are modified
	D3DXMATRIX rotationMatrix; // call updateMatrices whenever the offset/rotation are modified
	D3DXMATRIX offsetMatrixinv; // call updateMatrices whenever the offse are modified
	D3DXMATRIX rotationMatrixinv; // call updateMatrices whenever the offset/rotation are modified
	D3DXMATRIX localTrans; // call updateMatrices whenever the offset/rotation are modified
	D3DXMATRIX localTransinv; // call updateMatrices whenever the offset/rotation are modified
	
	UNCRZ_segment* parent;
	UNCRZ_model* model;
	std::vector<UNCRZ_segment*> segments;
	std::vector<UNCRZ_blend*> blends;

	UNCRZ_bbox segBox; // bounding box for vertices of segment

	int transIndex;

	bool requiresUpdate;

	void addBlend(std::string name, int tti, float prop)
	{
		blends.push_back(new UNCRZ_blend(name, tti, prop, this));
	}

	// need to transform
	void update(LPD3DXMATRIX trans, UNCRZ_trans_arr* transArr)
	{
		if (requiresUpdate)
		{
			for each (UNCRZ_blend* b in blends)
			{
				b->offset = offset * b->prop;
				b->rotation = rotation * b->prop;
				b->update(*trans, transArr);
			}

			updateMatrices();

			D3DXMatrixMultiply(trans, &offsetMatrix, trans);
			D3DXMatrixMultiply(trans, &rotationMatrix, trans);

			transArr->setValue(transIndex, trans);

			int len = segments.size();
			for (int i = len - 1; i >= 0; i--)
			{
				segments[i]->requiresUpdate = true;
				segments[i]->update(trans, transArr);
			}

			requiresUpdate = false;
		}
		else
		{
			D3DXMatrixMultiply(trans, &offsetMatrix, trans);
			D3DXMatrixMultiply(trans, &rotationMatrix, trans);

			int len = segments.size();
			for (int i = len - 1; i >= 0; i--)
			{
				segments[i]->update(trans, transArr);
			}
		}

		D3DXMatrixMultiply(trans, &rotationMatrixinv, trans);
		D3DXMatrixMultiply(trans, &offsetMatrixinv, trans);
	}

	UNCRZ_segment()
	{
		requiresUpdate = true;
	}

	UNCRZ_segment(UNCRZ_segment* gin, UNCRZ_segment* parentN, UNCRZ_model* modelN, std::vector<UNCRZ_segment*>* allSegs)
	{
		requiresUpdate = true;

		name = gin->name;
		for each(UNCRZ_segment* s in gin->segments)
		{
			segments.push_back(new UNCRZ_segment(s, this, modelN, allSegs));
		}
		parent = parentN;
		model = modelN;

		offset = gin->offset;
		rotation = gin->rotation;
		segBox = gin->segBox;

		transIndex = gin->transIndex;

		for (int i = allSegs->size() - 1; i >= 0; i--)
		{
			if (allSegs->at(i) == gin)
				allSegs->at(i) = this;
		}

		for each (UNCRZ_blend* b in gin->blends)
		{
			addBlend(b->name, b->transIndex, b->prop);
		}
	}

	UNCRZ_segment(std::string nameN)
	{
		requiresUpdate = true;
		name = nameN;
	}

	void updateMatrices()
	{
		D3DXMatrixTranslation(&offsetMatrix, offset.x, offset.y, offset.z);
		D3DXMatrixRotationYawPitchRoll(&rotationMatrix, rotation.y, rotation.x, rotation.z);
		D3DXMatrixMultiply(&localTrans, &offsetMatrix, &rotationMatrix);
		
		D3DXMatrixInverse(&offsetMatrixinv, NULL, &offsetMatrix);
		D3DXMatrixInverse(&rotationMatrixinv, NULL, &rotationMatrix);
		D3DXMatrixInverse(&localTransinv, NULL, &localTrans);
	}

	void createSegBox()
	{
		if (model->vertexType == VX_PC)
		{
			createSegBoxVX_PC();
		}
		else if (model->vertexType == VX_PCT)
		{
			createSegBoxVX_PCT();
		}
	}

	void createSegBoxVX_PC()
	{
		segBox = UNCRZ_bbox();
		vertexPC curVX_PC;
		D3DXVECTOR3 curVec;
		for (int i = model->numVertices - 1; i >= 0; i--)
		{
			curVX_PC = ((vertexPC*)model->vertexArray)[i];

			if (curVX_PC.tti == transIndex)
			{
				curVec.x = curVX_PC.x;
				curVec.y = curVX_PC.y;
				curVec.z = curVX_PC.z;
				segBox.include(curVec);
			}
		}
		segBox.fillVectors();
	}

	void createSegBoxVX_PCT()
	{
		segBox = UNCRZ_bbox();
		vertexPCT curVX_PCT;
		D3DXVECTOR3 curVec;
		for (int i = model->numVertices - 1; i >= 0; i--)
		{
			curVX_PCT = ((vertexPCT*)model->vertexArray)[i];

			if (curVX_PCT.tti == transIndex)
			{
				curVec.x = curVX_PCT.x;
				curVec.y = curVX_PCT.y;
				curVec.z = curVX_PCT.z;
				segBox.include(curVec);
			}
		}
		segBox.fillVectors();
	}

	void release()
	{
		int len = segments.size();
		for (int i = len - 1; i >= 0; i--)
		{
			segments[i]->release();
		}
	}
};

struct UNCRZ_FBF_anim_inst;

// uncrz_sprite (don't ask why it's here... it just is)
UNCRZ_sprite::UNCRZ_sprite()
	{
		zeroIsh();
	}

UNCRZ_sprite::UNCRZ_sprite(std::string nameN)
	{
		zeroIsh();
		name = nameN;
	}

void UNCRZ_sprite::zeroIsh()
	{
		useTex = false;
		useTex0 = false;
		useTex1 = false;
		useTex2 = false;
		useTex3 = false;
	}

void UNCRZ_sprite::setAlpha(LPDIRECT3DDEVICE9 dxDevice)
	{
		switch (alphaMode)
		{
		case AM_none:
			dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
			break;
		case AM_nice:
			dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			dxDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
			dxDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			dxDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
			break;
		case AM_add:
			dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			dxDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
			dxDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			dxDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
			break;
		}
	}

void UNCRZ_sprite::setTextures()
	{
		if (useTex)
			effect.setTexture(tex);
		if (useTex0)
			effect.setTexture0(tex0);
		if (useTex1)
			effect.setTexture1(tex1);
		if (useTex2)
			effect.setTexture2(tex2);
		if (useTex3)
			effect.setTexture3(tex3);
	}

void UNCRZ_sprite::setSideWiseAlpha(LPDIRECT3DDEVICE9 dxDevice)
	{
		switch (sideWiseAlphaMode)
		{
		case AM_none:
			dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
			break;
		case AM_nice:
			dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			dxDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
			dxDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			dxDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
			break;
		case AM_add:
			dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			dxDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
			dxDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			dxDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
			break;
		}
	}

void UNCRZ_sprite::draw(LPDIRECT3DDEVICE9 dxDevice, drawData* ddat, UNCRZ_sprite_buffer* sbuff, UNCRZ_sprite_data* larr, int offset, int count, DWORD drawArgs, DWORD spriteDrawArgs)
	{
		HRESULT res;

		if (spriteDrawArgs & SD_alpha)
			dxDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
		else
			dxDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

		if (spriteDrawArgs & SD_colour)
		{
			if (spriteDrawArgs & SD_itrl_sidewise)
				setSideWiseAlpha(dxDevice);
			else
				setAlpha(dxDevice);
		}
		else
		{
			// colour OFF (ish)
			dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			dxDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
			dxDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
			dxDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
		}

		dxDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

		dxDevice->SetVertexDeclaration(sbuff->vertexDec);
		dxDevice->SetStreamSource(0, sbuff->vBuff, 0, sbuff->stride);
		dxDevice->SetIndices(sbuff->iBuff);

		if (drawArgs & DF_light)
		{
			effect.setTechnique(lightTech);
			effect.setLightType(ddat->lightType);
			effect.setLightDepth(ddat->lightDepth);
			effect.setLightConeness(ddat->lightConeness);
		}
		else
		{
			effect.setTechnique(tech);
			effect.setLightCoof(ddat->lightCoof);
		}

		D3DXVECTOR4 dim(dimX, dimY, dimZ, 0);
		effect.setSpriteDim(&dim);

		effect.setEyePos(&ddat->eyePos);
		effect.setEyeDir(&ddat->eyeDir);
		effect.setTicker(ddat->ticker);

		setTextures();

		effect.setViewMat(ddat->viewMat);
		effect.setProjMat(ddat->projMat);
		effect.setViewProj(ddat->viewProj);
		effect.setcolMod(colMod);
		
		effect.effect->CommitChanges();

		UINT numPasses, pass;
		effect.effect->Begin(&numPasses, 0);

		// pass 0
		if (drawArgs & DF_light)
			effect.effect->BeginPass((int)ddat->lightType);
		else if (ddat->performPlainPass)
			effect.effect->BeginPass(0);
		else
			goto skipPlainPass;

		for (int i = 0; i < count; i += sbuff->size)
		{
			int ccount = count - i; // ccount is min(sbuff->size, count - i)
			if (ccount > sbuff->size)
				ccount = sbuff->size;

			effect.setSpriteLoc((float*)larr, i, ccount, sizeof(UNCRZ_sprite_data) / sizeof(float));
			effect.effect->CommitChanges();
			
			res = dxDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, ccount * 4, 0, ccount * 2);
		}
		effect.effect->EndPass();
skipPlainPass:

		if (lightingMode == LM_full && numPasses > 1 && (drawArgs & DF_light) == false)
		{
			// enable blend
			dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			dxDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
			dxDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
			dxDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

			// light pass
			for (int ldi = ddat->lightDatas.size() - 1; ldi >= 0; ldi--)
			{
				lightData* ld = ddat->lightDatas[ldi];
				
				if (!ld->lightEnabled)
					continue;
				
				effect.setLightData(ld);

				effect.effect->BeginPass((int)ld->lightType + 1);

				for (int i = offset; i < count; i += sbuff->size)
				{
					int ccount = count - i; // ccount is min(sbuff->size, count - i)
					if (ccount > sbuff->size)
						ccount = sbuff->size;

					effect.setSpriteLoc((float*)larr, i, ccount, sizeof(UNCRZ_sprite_data) / sizeof(float));
					effect.effect->CommitChanges();
					
					res = dxDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, ccount * 4, 0, ccount * 2);
				}

				effect.effect->EndPass();
			}

		}
		// disable blend
		dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

		effect.effect->End();

		/*UINT numPasses, pass;
		effect.effect->Begin(&numPasses, 0);

		for (pass = 0; pass < numPasses; pass++)
		{
			effect.effect->BeginPass(pass);

			for (int i = count - 1; i >= 0; i--)
			{
				if (vertexType == VX_PC || vertexType == VX_PCT)
					effect.setcolMod(&arr[i]->sections[secIndex]->colMod.x);
				effect.setTransArr(arr[i]->transArr);
				effect.effect->CommitChanges();
				res = dxDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, numVertices, vOffset, vLen);
				if (res != D3D_OK)
					res = res;
			}

			effect.effect->EndPass();
		}

		effect.effect->End();*/
	}



void UNCRZ_sprite::drawSideWise(LPDIRECT3DDEVICE9 dxDevice, drawData* ddat, UNCRZ_sprite_buffer* sbuff, UNCRZ_sprite_data* larr, int offset, int count, DWORD drawArgs, DWORD spriteDrawArgs)
	{
		effect.setEyePos(&ddat->eyePos);
		effect.setEyeDir(&ddat->eyeDir);
		effect.setTicker(ddat->ticker);

		if (drawArgs & DF_light || alphaMode == AM_none)
		{
			draw(dxDevice, ddat, sbuff, larr, offset, count, drawArgs, spriteDrawArgs);
		}
		else
		{
			drawToSide(dxDevice, ddat, sbuff, larr, offset, count, drawArgs, spriteDrawArgs | SD_itrl_sidewise);
			drawSideOver(dxDevice, ddat);
		}
	}

void UNCRZ_sprite::drawSideOver(LPDIRECT3DDEVICE9 dxDevice, drawData* ddat)
	{
		D3DXMATRIX idMat;
		D3DXMatrixIdentity(&idMat);
		dxDevice->SetRenderTarget(0, ddat->targetSurface);
		//dxDevice->SetViewport(ddat->targetVp);

		dxDevice->SetRenderState(D3DRS_ZENABLE, false);
		dxDevice->SetRenderState(D3DRS_ZWRITEENABLE, false);

		vertexOver overVerts[4]; // make this const / VBuff
		overVerts[0] = vertexOver(-1, -1, 0, 0, 1);
		overVerts[1] = vertexOver(-1, 1, 0, 0, 0);
		overVerts[2] = vertexOver(1, -1, 0, 1, 1);
		overVerts[3] = vertexOver(1, 1, 0, 1, 0);

		D3DXVECTOR4 texData = D3DXVECTOR4(0.5 / (float)ddat->targetVp->Width, 0.5 / (float)ddat->targetVp->Height, 1.0 / (float)ddat->targetVp->Width, 1.0 / (float)ddat->targetVp->Height);
		effect.setTextureData((float*)&texData.x);
		
		for (int i = 0; i < 4; i++) // do ahead of shader
		{
			overVerts[i].tu += texData.x;
			overVerts[i].tv += texData.y;
		}

		effect.setTexture(ddat->sideTex);
		effect.setTechnique(overTech);
		effect.setViewProj(&idMat);
		effect.effect->CommitChanges();

		setAlpha(dxDevice);

		UINT numPasses, pass;
		effect.effect->Begin(&numPasses, 0);
		effect.effect->BeginPass(0);

		dxDevice->SetVertexDeclaration(vertexDecOver);
		dxDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, overVerts, sizeof(vertexOver));

		effect.effect->EndPass();
		effect.effect->End();
		
		dxDevice->SetRenderState(D3DRS_ZENABLE, true);
		dxDevice->SetRenderState(D3DRS_ZWRITEENABLE, true);
	}

void UNCRZ_sprite::drawToSide(LPDIRECT3DDEVICE9 dxDevice, drawData* ddat, UNCRZ_sprite_buffer* sbuff, UNCRZ_sprite_data* larr, int offset, int count, DWORD drawArgs, DWORD spriteDrawArgs)
	{
		dxDevice->SetRenderTarget(0, ddat->sideSurface);
		dxDevice->SetViewport(ddat->sideVp);
		//dxDevice->SetDepthStencilSurface(ddat->zSideSurface);

		dxDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 0, 0);
		dxDevice->SetRenderState(D3DRS_ZENABLE, true);
		dxDevice->SetRenderState(D3DRS_ZWRITEENABLE, true);

		effect.setTargetTexture(ddat->targetTex);
		D3DXVECTOR4 targTexData = D3DXVECTOR4(0.5 / (float)ddat->targetVp->Width, 0.5 / (float)ddat->targetVp->Height, 1.0 / (float)ddat->targetVp->Width, 1.0 / (float)ddat->targetVp->Height);
		effect.setTargetTextureData((float*)&targTexData.x);
		
		float matDat[16] = {
			(float)ddat->targetVp->Width / 2.0, 0.0, 0.0, 0.0,
			0.0, -(float)ddat->targetVp->Height / 2.0, 0.0, 0.0,
			0.0, 0.0, ddat->targetVp->MaxZ - ddat->targetVp->MinZ, 0.0,
			ddat->targetVp->X + (float)ddat->targetVp->Width / 2.0, ddat->targetVp->Y + (float)ddat->targetVp->Height / 2.0, ddat->targetVp->MinZ, 1.0
		};
		//D3DXMATRIX vpMat(&matDat);

		effect.setTargetVPMat((float*)&matDat);

		draw(dxDevice, ddat, sbuff, larr, offset, count, drawArgs, spriteDrawArgs);
	}

// uncrz_model
void UNCRZ_model::allSegsRequireUpdate()
	{
		int len = allSegs.size();
		for (int i = len - 1; i >= 0; i--)
		{
			allSegs[i]->requiresUpdate = true;
		}
	}

void UNCRZ_model::update(LPD3DXMATRIX trans, bool forceUpdate)
{
	int len = segments.size();
	for (int i = len - 1; i >= 0; i--)
	{
		if (forceUpdate)
			segments[i]->requiresUpdate = true;
		segments[i]->update(trans, transArr);
	}

	// assemble bbox
	modelBox = UNCRZ_bbox();

	len = allSegs.size();
	for (int i = len - 1; i >= 0; i--)
	{
		modelBox.include(&(allSegs[i]->segBox), transArr->getValue(allSegs[i]->transIndex));
	}

	modelBox.fillVectors();
}

void UNCRZ_model::drawBBoxDebug(LPDIRECT3DDEVICE9 dxDevice, drawData* ddat)
{
	vertexPC vPCs[36];
	D3DXVECTOR3 curVec;

	for (int i = 0; i < 36; i += 6)
	{
		curVec = modelBox.vecArr[bboxIndices[i]];
		vPCs[i] = vertexPC(curVec.x, curVec.y, curVec.z, 1.0, 0.0, 0.0, -1);

		curVec = modelBox.vecArr[bboxIndices[i + 1]];
		vPCs[i + 1] = vertexPC(curVec.x, curVec.y, curVec.z, 1.0, 1.0, 0.0, -1);

		curVec = modelBox.vecArr[bboxIndices[i + 2]];
		vPCs[i + 2] = vertexPC(curVec.x, curVec.y, curVec.z, 0.0, 1.0, 0.0, -1);

		curVec = modelBox.vecArr[bboxIndices[i + 3]];
		vPCs[i + 3] = vertexPC(curVec.x, curVec.y, curVec.z, 1.0, 0.0, 0.0, -1);

		curVec = modelBox.vecArr[bboxIndices[i + 4]];
		vPCs[i + 4] = vertexPC(curVec.x, curVec.y, curVec.z, 0.0, 1.0, 0.0, -1);

		curVec = modelBox.vecArr[bboxIndices[i + 5]];
		vPCs[i + 5] = vertexPC(curVec.x, curVec.y, curVec.z, 0.0, 0.0, 1.0, -1);
	}

	UNCRZ_effect effect = sections[0]->effect;

	effect.setcolMod(sections[0]->colMod);
	effect.effect->SetTechnique(effect.effect->GetTechniqueByName("simple"));
	effect.setViewProj(ddat->viewProj);
	effect.effect->CommitChanges();
	dxDevice->SetVertexDeclaration(vertexDecPC);

	UINT numPasses, pass;
	effect.effect->Begin(&numPasses, 0);

	HRESULT res;

	for (pass = 0; pass < numPasses; pass++)
	{
		effect.effect->BeginPass(pass);
		res = dxDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 12, &vPCs, sizeof(vertexPC));
		if (res != D3D_OK)
			res = res;
		effect.effect->EndPass();
	}

	effect.effect->End();
}

void UNCRZ_model::drawMany(LPDIRECT3DDEVICE9 dxDevice, drawData* ddat, UNCRZ_model** arr, int count, DWORD drawArgs)
{
	bool res; // true means it will be culled
	for (int i = count - 1; i >= 0; i--)
	{
		res = true;
		if (noCull || arr[i]->modelBox.dothSurviveClipTransformed(ddat->viewProj))
		{
			res = false;
			goto set;
		}
set:
		if (res == true)
			ddat->cullCount++;
		else
			ddat->drawCount++;
		for (int j = sections.size() - 1; j >= 0; j--)
		{
			arr[i]->sections[j]->curDrawCull = res;
		}
	}

	dxDevice->SetVertexDeclaration(vertexDec);
	dxDevice->SetStreamSource(0, vBuff, 0, stride);
	dxDevice->SetIndices(iBuff);

	int len = sections.size();
	for (int i = len - 1; i >= 0; i--)
	{
		sections[i]->drawMany(dxDevice, ddat, arr, count, i, drawArgs);
	}
}

void UNCRZ_model::draw(LPDIRECT3DDEVICE9 dxDevice, drawData* ddat, DWORD drawArgs)
{
	if (noCull || modelBox.dothSurviveClipTransformed(ddat->viewProj))
		goto notOcced;
	
	ddat->cullCount++;
	return;
notOcced:
	ddat->drawCount++;

	//dxDevice->SetVertexDeclaration(vertexDec);
	//dxDevice->SetStreamSource(0, vBuff, 0, stride);
	//dxDevice->SetIndices(iBuff);

	int len = sections.size();
	for (int i = len - 1; i >= 0; i--)
	{
		dxDevice->SetVertexDeclaration(vertexDec);
		dxDevice->SetStreamSource(0, vBuff, 0, stride);
		dxDevice->SetIndices(iBuff);

		sections[i]->draw(dxDevice, transArr, ddat, drawArgs);
	}
}

UNCRZ_model::UNCRZ_model()
{
	animInst = NULL;
}

UNCRZ_model::UNCRZ_model(UNCRZ_model* gin)
{
	name = gin->name;

	for each(UNCRZ_segment* s in gin->allSegs)
	{
		allSegs.push_back(s);
	}
	for each(UNCRZ_segment* s in gin->segments)
	{
		segments.push_back(new UNCRZ_segment(s, s->parent, this, &allSegs));
	}
	for each(UNCRZ_section* ss in gin->sections)
	{
		sections.push_back(new UNCRZ_section(ss));
	}

	vBuff = gin->vBuff;
	iBuff = gin->iBuff;
	vertexArray = gin->vertexArray;
	indexArray = gin->indexArray;
	stride = gin->stride;
	vertexDec = gin->vertexDec;
	vertexType = gin->vertexType;
	numVertices = gin->numVertices;
	numIndices = gin->numIndices;
	noCull = gin->noCull;

	transArr = new UNCRZ_trans_arr();
	transArr->create((int)allSegs.size() * 2);
	
	animInst = new UNCRZ_FBF_anim_inst(this, gin->animInst->anim);
}

UNCRZ_model::UNCRZ_model(std::string nameN)
{
	name = nameN;
	animInst = NULL;
	noCull = false;
}

void UNCRZ_model::createSegmentBoxes()
{
	int len = allSegs.size();
	for (int i = len - 1; i >= 0; i--)
	{
		allSegs[i]->createSegBox();
	}
}

void UNCRZ_model::changeAnim(UNCRZ_FBF_anim* newAnim)
{
	if (animInst == NULL)
		animInst = new UNCRZ_FBF_anim_inst(this, newAnim);
	else
		animInst->reset(newAnim);
}

void UNCRZ_model::release()
{
	vBuff->Release();
	iBuff->Release();
	delete[stride * numVertices] vertexArray;
	delete[sizeof(short) * numIndices] indexArray;
	int len = segments.size();
	for (int i = len - 1; i >= 0; i--)
	{
		segments[i]->release();
	}
}

void UNCRZ_model::clearDecals()
{
	for (int i = sections.size() - 1; i >= 0; i--)
	{
		sections[i]->decals.clear();
	}
}

void UNCRZ_model::fillVBuff()
{
	VOID* buffPtr;
	vBuff->Lock(0, numVertices * stride, (VOID**)&buffPtr, D3DLOCK_DISCARD);
	memcpy(buffPtr, vertexArray, numVertices * stride);
	vBuff->Unlock();
}

void UNCRZ_model::createVBuff(LPDIRECT3DDEVICE9 dxDevice, void* vds)
{
	dxDevice->CreateVertexBuffer(numVertices * stride, D3DUSAGE_WRITEONLY, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, D3DPOOL_DEFAULT, &vBuff, NULL);

	vertexArray = malloc(stride * numVertices);
	memcpy(vertexArray, vds, numVertices * stride);

	fillVBuff();
}

void UNCRZ_model::fillIBuff()
{
	VOID* buffPtr;
	iBuff->Lock(0, numIndices * sizeof (short), (VOID**)&buffPtr, D3DLOCK_DISCARD);
	memcpy(buffPtr, indexArray, numIndices * sizeof (short));
	iBuff->Unlock();
}

void UNCRZ_model::createIBuff(LPDIRECT3DDEVICE9 dxDevice, short* ids)
{
	dxDevice->CreateIndexBuffer(numIndices * sizeof (short), D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &iBuff, NULL);

	indexArray = (short*)malloc(sizeof (short) * numIndices);
	memcpy(indexArray, ids, numIndices * sizeof (short));

	fillIBuff();
}

bool UNCRZ_model::collides(D3DXVECTOR3* rayPos, D3DXVECTOR3* rayDir, float* distRes)
{
	// bbox collision
	if (modelBox.collides(rayPos, rayDir) == false)
		return false;

	// vertex collision
	if (vertexType == VX_PC)
	{
		return collidesVX_PC(rayPos, rayDir, distRes);
	}
	else if (vertexType == VX_PCT)
	{
		return collidesVX_PCT(rayPos, rayDir, distRes);
	}
	// ERR... :S
	return false;
}

D3DXVECTOR3 baryToVec(D3DXVECTOR3 va, D3DXVECTOR3 vb, D3DXVECTOR3 vc, float u, float v)
{
	D3DXVECTOR3 res = (vb * u) + (vc * v) + (va * (1 - (u + v)));
	return res;
}

bool UNCRZ_model::collidesVX_PC(D3DXVECTOR3* rayPos, D3DXVECTOR3* rayDir, float* distRes)
{
	float uRes, vRes;
	vertexPC a;
	vertexPC b;
	vertexPC c;
	D3DXVECTOR4 va;
	D3DXVECTOR4 vb;
	D3DXVECTOR4 vc;

	bool victory = false;
	float bestRes;
	float tempRes;

	for (int i = 0; i < numIndices; i += 3)
	{
		a = (((vertexPC*)vertexArray)[(int)indexArray[i]]);
		b = (((vertexPC*)vertexArray)[(int)indexArray[i + 1]]);
		c = (((vertexPC*)vertexArray)[(int)indexArray[i + 2]]);

		va = D3DXVECTOR4(a.x, a.y, a.z, 1.0f);
		D3DXVec4Transform(&va, &va, transArr->getValue((int)a.tti));
		vb = D3DXVECTOR4(b.x, b.y, b.z, 1.0f);
		D3DXVec4Transform(&vb, &vb, transArr->getValue((int)b.tti));
		vc = D3DXVECTOR4(c.x, c.y, c.z, 1.0f);
		D3DXVec4Transform(&vc, &vc, transArr->getValue((int)c.tti));

		if (D3DXIntersectTri((D3DXVECTOR3*)&va, (D3DXVECTOR3*)&vb, (D3DXVECTOR3*)&vc, rayPos, rayDir, &uRes, &vRes, &tempRes))
		{
			if (victory == false)
			{
				victory = true;
				bestRes = tempRes;
			}
			else if (tempRes < bestRes)
			{
				bestRes = tempRes;
			}
		}
		//if (D3DXIntersectTri((D3DXVECTOR3*)&va, (D3DXVECTOR3*)&vb, (D3DXVECTOR3*)&vc, rayPos, rayDir, &uRes, &vRes, distRes))
		//	return true;
	}

	if (victory)
	{
		*distRes = bestRes;
		return true;
	}
	return false;
}

bool UNCRZ_model::collidesVX_PCT(D3DXVECTOR3* rayPos, D3DXVECTOR3* rayDir, float* distRes)
{
	float uRes, vRes;
	vertexPCT a;
	vertexPCT b;
	vertexPCT c;
	D3DXVECTOR4 va;
	D3DXVECTOR4 vb;
	D3DXVECTOR4 vc;

	bool victory = false;
	float bestRes;
	float tempRes;

	for (int i = 0; i < numIndices; i += 3)
	{
		a = (((vertexPCT*)vertexArray)[(int)indexArray[i]]);
		b = (((vertexPCT*)vertexArray)[(int)indexArray[i + 1]]);
		c = (((vertexPCT*)vertexArray)[(int)indexArray[i + 2]]);

		va = D3DXVECTOR4(a.x, a.y, a.z, 1.0f);
		D3DXVec4Transform(&va, &va, transArr->getValue((int)a.tti));
		vb = D3DXVECTOR4(b.x, b.y, b.z, 1.0f);
		D3DXVec4Transform(&vb, &vb, transArr->getValue((int)b.tti));
		vc = D3DXVECTOR4(c.x, c.y, c.z, 1.0f);
		D3DXVec4Transform(&vc, &vc, transArr->getValue((int)c.tti));

		if (D3DXIntersectTri((D3DXVECTOR3*)&va, (D3DXVECTOR3*)&vb, (D3DXVECTOR3*)&vc, rayPos, rayDir, &uRes, &vRes, &tempRes))
		{
			if (victory == false)
			{
				victory = true;
				bestRes = tempRes;
			}
			else if (tempRes < bestRes)
			{
				bestRes = tempRes;
			}
		}
		//if (D3DXIntersectTri((D3DXVECTOR3*)&va, (D3DXVECTOR3*)&vb, (D3DXVECTOR3*)&vc, rayPos, rayDir, &uRes, &vRes, distRes))
		//	return true;
	}

	if (victory)
	{
		*distRes = bestRes;
		return true;
	}
	return false;
}



int UNCRZ_model::collidesVertex(D3DXVECTOR3* rayPos, D3DXVECTOR3* rayDir, float* distRes)
{
	// bbox collision
	if (modelBox.collides(rayPos, rayDir) == false)
		return false;

	// vertex collision
	if (vertexType == VX_PC)
	{
		return collidesVertexVX_PC(rayPos, rayDir, distRes);
	}
	else if (vertexType == VX_PCT)
	{
		return collidesVertexVX_PCT(rayPos, rayDir, distRes);
	}
	// ERR... :S
	return -1;
}

int UNCRZ_model::collidesVertexVX_PC(D3DXVECTOR3* rayPos, D3DXVECTOR3* rayDir, float* distRes)
{
	float uRes, vRes;
	vertexPC a;
	vertexPC b;
	vertexPC c;
	D3DXVECTOR4 va;
	D3DXVECTOR4 vb;
	D3DXVECTOR4 vc;

	bool victory = false;
	float bestRes;
	float tempRes;
	int indexRes;

	for (int i = 0; i < numIndices; i += 3)
	{
		a = (((vertexPC*)vertexArray)[(int)indexArray[i]]);
		b = (((vertexPC*)vertexArray)[(int)indexArray[i + 1]]);
		c = (((vertexPC*)vertexArray)[(int)indexArray[i + 2]]);

		va = D3DXVECTOR4(a.x, a.y, a.z, 1.0f);
		D3DXVec4Transform(&va, &va, transArr->getValue((int)a.tti));
		vb = D3DXVECTOR4(b.x, b.y, b.z, 1.0f);
		D3DXVec4Transform(&vb, &vb, transArr->getValue((int)b.tti));
		vc = D3DXVECTOR4(c.x, c.y, c.z, 1.0f);
		D3DXVec4Transform(&vc, &vc, transArr->getValue((int)c.tti));

		if (D3DXIntersectTri((D3DXVECTOR3*)&va, (D3DXVECTOR3*)&vb, (D3DXVECTOR3*)&vc, rayPos, rayDir, &uRes, &vRes, &tempRes))
		{
			//if (victory == false)
			//{
			//	victory = true;
			//	bestRes = tempRes;
			//}
			//else if (tempRes < bestRes)
			//{
			//	bestRes = tempRes;
			//}
			
			if (victory == false || (victory && tempRes < bestRes))
			{
				victory = true;
				bestRes = tempRes;
				
				if (uRes < 0.5f && vRes < 0.5f)
					indexRes = (int)indexArray[i];
				else if (uRes > vRes)
					indexRes = indexArray[i + 1];
				else
					indexRes = indexArray[i + 2];
			}
		}
		//if (D3DXIntersectTri((D3DXVECTOR3*)&va, (D3DXVECTOR3*)&vb, (D3DXVECTOR3*)&vc, rayPos, rayDir, &uRes, &vRes, distRes))
		//	return true;
	}

	if (victory)
	{
		*distRes = bestRes;
		return indexRes;
	}
	return -1;
}

int UNCRZ_model::collidesVertexVX_PCT(D3DXVECTOR3* rayPos, D3DXVECTOR3* rayDir, float* distRes)
{
	float uRes, vRes;
	vertexPCT a;
	vertexPCT b;
	vertexPCT c;
	D3DXVECTOR4 va;
	D3DXVECTOR4 vb;
	D3DXVECTOR4 vc;

	bool victory = false;
	float bestRes;
	float tempRes;
	int indexRes;

	for (int i = 0; i < numIndices; i += 3)
	{
		a = (((vertexPCT*)vertexArray)[(int)indexArray[i]]);
		b = (((vertexPCT*)vertexArray)[(int)indexArray[i + 1]]);
		c = (((vertexPCT*)vertexArray)[(int)indexArray[i + 2]]);

		va = D3DXVECTOR4(a.x, a.y, a.z, 1.0f);
		D3DXVec4Transform(&va, &va, transArr->getValue((int)a.tti));
		vb = D3DXVECTOR4(b.x, b.y, b.z, 1.0f);
		D3DXVec4Transform(&vb, &vb, transArr->getValue((int)b.tti));
		vc = D3DXVECTOR4(c.x, c.y, c.z, 1.0f);
		D3DXVec4Transform(&vc, &vc, transArr->getValue((int)c.tti));

		if (D3DXIntersectTri((D3DXVECTOR3*)&va, (D3DXVECTOR3*)&vb, (D3DXVECTOR3*)&vc, rayPos, rayDir, &uRes, &vRes, &tempRes))
		{
			//if (victory == false)
			//{
			//	victory = true;
			//	bestRes = tempRes;
			//}
			//else if (tempRes < bestRes)
			//{
			//	bestRes = tempRes;
			//}

			if (victory == false || (victory && tempRes < bestRes))
			{
				victory = true;
				bestRes = tempRes;
				
				if (uRes < 0.5f && vRes < 0.5f)
					indexRes = (int)indexArray[i];
				else if (uRes > vRes)
					indexRes = indexArray[i + 1];
				else
					indexRes = indexArray[i + 2];
			}
		}
		//if (D3DXIntersectTri((D3DXVECTOR3*)&va, (D3DXVECTOR3*)&vb, (D3DXVECTOR3*)&vc, rayPos, rayDir, &uRes, &vRes, distRes))
		//	return true;
	}

	if (victory)
	{
		*distRes = bestRes;
		return indexRes;
	}
	return -1;
}
// end uncrz_model

const DWORD UNCRZ_fai_shift = 0x00000000;
const DWORD UNCRZ_fai_rotate = 0x00000001;
const DWORD UNCRZ_fai_modcol = 0x00000002;
const DWORD UNCRZ_fai_rotate_around = 0x00000003;

const DWORD UNCRZ_fai_offset = 0x00000010;
const DWORD UNCRZ_fai_rotation = 0x00000011;
const DWORD UNCRZ_fai_colmod = 0x00000012;

const DWORD UNCRZ_fai_offset_smth0 = 0x00000020;
const DWORD UNCRZ_fai_rotation_smth0 = 0x00000021;
const DWORD UNCRZ_fai_colmod_smth0 = 0x00000021;

const DWORD UNCRZ_fai_offset_smth1 = 0x00000030;
const DWORD UNCRZ_fai_rotation_smth1 = 0x00000031;
const DWORD UNCRZ_fai_colmod_smth1 = 0x00000031;

const DWORD AI_proc_0 = 0x00000001;
const DWORD AI_proc_1 = 0x00000002;
const DWORD AI_proc_2 = 0x00000004;
const DWORD AI_proc_3 = 0x00000008;

const DWORD AI_proc_all4 = 0x000000000f;

struct UNCRZ_FBF_anim_instr
{
	DWORD instrType;
	int segIndex;
	int secIndex;
	D3DXVECTOR4 val;
	DWORD aiFlags; 

	std::string dbgLine;

	// proportions (out of 1.0) of duration consumed
	static float smth0_func(float num)
	{
		num = D3DX_PI * num;
		return num * 0.5f - 0.25f * sin(num * 2.0f);
	}

	// proportions (out of 1.0) of duration consumed
	static float smth1_func(float num)
	{
		num = D3DX_PI * num;
		return -cos(num);
	}

	void run(UNCRZ_model* model, int curDuration, int totalDuration)
	{
		UNCRZ_segment* seg;
		if (segIndex != -1)
		{
			seg = model->allSegs[segIndex];
			seg->requiresUpdate = true;
		}

		UNCRZ_section* sec;
		if (secIndex != -1)
			sec = model->sections[secIndex];

		float gone; // what is gone
		float left; // what is left
		float yum; // what will be used
		float ratio; // what will be used, over what is left to be used

		D3DXMATRIX tempMatrix; // general use tempory array of 16 values (feel free to use it NOT as a matrix ;) )

		if (instrType == UNCRZ_fai_shift)
		{
			if (aiFlags & AI_proc_0)
				seg->offset.x += val.x;
			if (aiFlags & AI_proc_1)
				seg->offset.y += val.y;
			if (aiFlags & AI_proc_2)
				seg->offset.z += val.z;
		}
		else if (instrType == UNCRZ_fai_rotate)
		{
			if (aiFlags & AI_proc_0)
				seg->rotation.x += val.x;
			if (aiFlags & AI_proc_1)
				seg->rotation.y += val.y;
			if (aiFlags & AI_proc_2)
				seg->rotation.z += val.z;
		}
		else if (instrType == UNCRZ_fai_rotate_around)
		{
			D3DXMatrixRotationAxis(&tempMatrix, (D3DXVECTOR3*)(&val.x), val.w);

			D3DXVec3TransformNormal(&seg->rotation, &seg->rotation, &tempMatrix);

			//if (aiFlags & AI_proc_0)
			//	seg->rotation.x += val.x;
			//if (aiFlags & AI_proc_1)
			//	seg->rotation.y += val.y;
			//if (aiFlags & AI_proc_2)
			//	seg->rotation.z += val.z;
		}
		else if (instrType == UNCRZ_fai_modcol)
		{
			if (aiFlags & AI_proc_0)
			{
				sec->colMod.x += val.x;
				while (sec->colMod.x > 1.0)
					sec->colMod.x -= 1.0;
				while (sec->colMod.x < 0.0)
					sec->colMod.x += 1.0;
			}
			
			if (aiFlags & AI_proc_1)
			{
				sec->colMod.y += val.y;
				while (sec->colMod.y > 1.0)
					sec->colMod.y -= 1.0;
				while (sec->colMod.y < 0.0)
					sec->colMod.y += 1.0;
			}
			if (aiFlags & AI_proc_2)
			{
				sec->colMod.z += val.z;
				while (sec->colMod.z > 1.0)
					sec->colMod.z -= 1.0;
				while (sec->colMod.z < 0.0)
					sec->colMod.z += 1.0;
			}
			if (aiFlags & AI_proc_3)
			{
				sec->colMod.w += val.w;
				while (sec->colMod.w > 1.0)
					sec->colMod.w -= 1.0;
				while (sec->colMod.w < 0.0)
					sec->colMod.w += 1.0;
			}
		}
		else if (instrType == UNCRZ_fai_offset)
		{
			left = totalDuration - curDuration;
			ratio = 1.0 / left;
			if (aiFlags & AI_proc_0)
				seg->offset.x += (val.x - seg->offset.x) * ratio;
			if (aiFlags & AI_proc_1)
				seg->offset.y += (val.y - seg->offset.y) * ratio;
			if (aiFlags & AI_proc_2)
				seg->offset.z += (val.z - seg->offset.z) * ratio;
		}
		else if (instrType == UNCRZ_fai_rotation) // dodgy!!!
		{
			left = totalDuration - curDuration;
			ratio = 1.0 / left;
			if (aiFlags & AI_proc_0)
				seg->rotation.x += (val.x - seg->rotation.x) * ratio;
			if (aiFlags & AI_proc_1)
				seg->rotation.y += (val.y - seg->rotation.y) * ratio;
			if (aiFlags & AI_proc_2)
				seg->rotation.z += (val.z - seg->rotation.z) * ratio;
		}
		else if (instrType == UNCRZ_fai_colmod) // dodgy!!!
		{
			left = totalDuration - curDuration;
			ratio = 1.0 / left;
			if (aiFlags & AI_proc_0)
				sec->colMod.x += (val.x - sec->colMod.x) * ratio;
			if (aiFlags & AI_proc_1)
				sec->colMod.y += (val.y - sec->colMod.y) * ratio;
			if (aiFlags & AI_proc_2)
				sec->colMod.z += (val.z - sec->colMod.z) * ratio;
			if (aiFlags & AI_proc_3)
				sec->colMod.w += (val.z - sec->colMod.w) * ratio;
		}
		else if (instrType == UNCRZ_fai_offset_smth0) // dodgy
		{
			yum = 1.0f / (float)totalDuration;
			gone = (float)curDuration * yum;
			yum = smth0_func(yum + gone);
			gone = -smth0_func(gone);
			left = D3DX_PI * 0.5 + gone;
			yum += gone;
			ratio = yum / left;

			if (aiFlags & AI_proc_0)
				seg->offset.x += (val.x - seg->offset.x) * ratio;
			if (aiFlags & AI_proc_1)
				seg->offset.y += (val.y - seg->offset.y) * ratio;
			if (aiFlags & AI_proc_2)
				seg->offset.z += (val.z - seg->offset.z) * ratio;
		}
		else if (instrType == UNCRZ_fai_rotation_smth0) // dodgy!!!
		{
			//yum = 1.0f / (float)totalDuration;
			//gone = (float)curDuration / (float)totalDuration;
			//left = D3DX_PI * 0.5 - smth0_func(gone);
			//yum = smth0_func(yum + gone) - smth0_func(gone);
			//ratio = yum / left; // original, unabridged, ish

			yum = 1.0f / (float)totalDuration;
			gone = (float)curDuration * yum;
			yum = smth0_func(yum + gone);
			gone = -smth0_func(gone);
			left = D3DX_PI * 0.5 + gone;
			yum += gone;
			ratio = yum / left;

			if (aiFlags & AI_proc_0)
				seg->rotation.x += (val.x - seg->rotation.x) * ratio;
			if (aiFlags & AI_proc_1)
				seg->rotation.y += (val.y - seg->rotation.y) * ratio;
			if (aiFlags & AI_proc_2)
				seg->rotation.z += (val.z - seg->rotation.z) * ratio;
		}
		else if (instrType == UNCRZ_fai_colmod_smth0) // dodgy!!!
		{
			yum = 1.0f / (float)totalDuration;
			gone = (float)curDuration * yum;
			yum = smth0_func(yum + gone);
			gone = -smth0_func(gone);
			left = D3DX_PI * 0.5 + gone;
			yum += gone;
			ratio = yum / left;

			if (aiFlags & AI_proc_0)
				sec->colMod.x += (val.x - sec->colMod.x) * ratio;
			if (aiFlags & AI_proc_1)
				sec->colMod.y += (val.y - sec->colMod.y) * ratio;
			if (aiFlags & AI_proc_2)
				sec->colMod.z += (val.z - sec->colMod.z) * ratio;
			if (aiFlags & AI_proc_3)
				sec->colMod.w += (val.z - sec->colMod.w) * ratio;
		}
		else if (instrType == UNCRZ_fai_offset_smth1) // dodgy
		{
			yum = 1.0f / (float)totalDuration;
			gone = (float)curDuration * yum;
			yum = smth1_func(yum + gone);
			gone = -smth1_func(gone);
			left = 1 + gone;
			yum += gone;
			ratio = yum / left;

			if (aiFlags & AI_proc_0)
				seg->offset.x += (val.x - seg->offset.x) * ratio;
			if (aiFlags & AI_proc_1)
				seg->offset.y += (val.y - seg->offset.y) * ratio;
			if (aiFlags & AI_proc_2)
				seg->offset.z += (val.z - seg->offset.z) * ratio;
		}
		else if (instrType == UNCRZ_fai_rotation_smth1) // dodgy!!!
		{
			//yum = 1.0f / (float)totalDuration;
			//gone = (float)curDuration / (float)totalDuration;
			//left = D3DX_PI * 0.5 - smth1_func(gone);
			//yum = smth1_func(yum + gone) - smth1_func(gone);
			//ratio = yum / left; // original, unabridged, ish

			yum = 1.0f / (float)totalDuration;
			gone = (float)curDuration * yum;
			yum = smth1_func(yum + gone);
			gone = -smth1_func(gone);
			left = 1 + gone;
			yum += gone;
			ratio = yum / left;

			if (aiFlags & AI_proc_0)
				seg->rotation.x += (val.x - seg->rotation.x) * ratio;
			if (aiFlags & AI_proc_1)
				seg->rotation.y += (val.y - seg->rotation.y) * ratio;
			if (aiFlags & AI_proc_2)
				seg->rotation.z += (val.z - seg->rotation.z) * ratio;
		}
		else if (instrType == UNCRZ_fai_colmod_smth1) // dodgy!!!
		{
			yum = 1.0f / (float)totalDuration;
			gone = (float)curDuration * yum;
			yum = smth1_func(yum + gone);
			gone = -smth1_func(gone);
			left = 1 + gone;
			yum += gone;
			ratio = yum / left;

			if (aiFlags & AI_proc_0)
				sec->colMod.x += (val.x - sec->colMod.x) * ratio;
			if (aiFlags & AI_proc_1)
				sec->colMod.y += (val.y - sec->colMod.y) * ratio;
			if (aiFlags & AI_proc_2)
				sec->colMod.z += (val.z - sec->colMod.z) * ratio;
			if (aiFlags & AI_proc_3)
				sec->colMod.w += (val.z - sec->colMod.w) * ratio;
		}
	}

	void getSeg(std::string segName, UNCRZ_model* model)
	{
		for (int i = model->allSegs.size() - 1; i >= 0; i--)
		{
			if (model->allSegs[i]->name == segName)
			{
				segIndex = i;
			}
		}
	}

	void getSec(std::string secName, UNCRZ_model* model)
	{
		for (int i = model->sections.size() - 1; i >= 0; i--)
		{
			if (model->sections[i]->name == secName)
			{
				secIndex = i;
			}
		}
	}

	UNCRZ_FBF_anim_instr(std::string line, UNCRZ_model* model)
	{
		secIndex = -1;
		segIndex = -1;

		dbgLine = line;
		std::vector<std::string> data = split(line, " ");

		if (data[0] == "shift")
		{
			instrType = UNCRZ_fai_shift;
			getSeg(data[1], model);
		}
		else if (data[0] == "rotate")
		{
			instrType = UNCRZ_fai_rotate;
			getSeg(data[1], model);
		}
		else if (data[0] == "modcol")
		{
			instrType = UNCRZ_fai_modcol;
			getSec(data[1], model);
		}
		else if (data[0] == "offset")
		{
			instrType = UNCRZ_fai_offset;
			getSeg(data[1], model);
		}
		else if (data[0] == "rotation")
		{
			instrType = UNCRZ_fai_rotation;
			getSeg(data[1], model);
		}
		else if (data[0] == "colmod")
		{
			instrType = UNCRZ_fai_colmod;
			getSec(data[1], model);
		}
		else if (data[0] == "offset_smth0")
		{
			instrType = UNCRZ_fai_offset_smth0;
			getSeg(data[1], model);
		}
		else if (data[0] == "rotation_smth0")
		{
			instrType = UNCRZ_fai_rotation_smth0;
			getSeg(data[1], model);
		}
		else if (data[0] == "offset_smth1")
		{
			instrType = UNCRZ_fai_offset_smth1;
			getSeg(data[1], model);
		}
		else if (data[0] == "rotation_smth1")
		{
			instrType = UNCRZ_fai_rotation_smth1;
			getSeg(data[1], model);
		}

		if (data.size() == 7)
		{
			val = D3DXVECTOR4(stof(data[2]), stof(data[3]), stof(data[4]), stof(data[5]));
			aiFlags = (DWORD)stoi(data[6]);
		}
		else
		{
			aiFlags = AI_proc_all4;
			if (data.size() == 6)
				val = D3DXVECTOR4(stof(data[2]), stof(data[3]), stof(data[4]), stof(data[5]));
			else if (data.size() == 5)
				val = D3DXVECTOR4(stof(data[2]), stof(data[3]), stof(data[4]), 0.0f);
			else if (data.size() == 4)
				val = D3DXVECTOR4(stof(data[2]), stof(data[3]), 0.0f, 0.0f);
			else if (data.size() == 3)
				val = D3DXVECTOR4(stof(data[2]), 0.0, 0.0f, 0.0f);
		}
	}
};

struct UNCRZ_FBF_anim_motion
{
public:
	std::string name;
	int startInstr;
	int endInstr;
	int duration; // frames
	bool causesUpdate;

	UNCRZ_FBF_anim_motion()
	{
		causesUpdate = true;
	}

	UNCRZ_FBF_anim_motion(std::string nameN)
	{
		name = nameN;
		causesUpdate = true;
	}
};

//struct UNCRZ_FBF_anim_flow
//{
void UNCRZ_FBF_anim_flow::run(UNCRZ_FBF_anim_inst* inst, UNCRZ_FBF_anim_flow_state* state)
	{
		int d = motions[state->curMotion].duration;
		int end = motions[state->curMotion].endInstr;
		for (int i = motions[state->curMotion].startInstr; i <= end; i++)
		{
			instrs[i].run(inst->model, state->curMotionDuration, d);
		}

		state->curMotionDuration = state->curMotionDuration + 1;
		if (state->curMotionDuration >= d)
		{
			state->curMotionDuration = 0;
			state->curMotion = state->curMotion + 1;
			if (state->curMotion >= motions.size())
			{
				state->curMotion = 0;
			}
		}

		if (motions[state->curMotion].causesUpdate)
		{
			inst->model->allSegsRequireUpdate();
		}
	}

UNCRZ_FBF_anim_flow::UNCRZ_FBF_anim_flow(std::string nameN)
	{
		name = nameN;
	}

//};

//struct UNCRZ_FBF_anim
//{

void UNCRZ_FBF_anim::run(UNCRZ_FBF_anim_inst* inst)
	{
		for (int i = flows.size() - 1; i >= 0; i--)
		{
			flows[i].run(inst, &inst->states[i]);
		}
	}

void UNCRZ_FBF_anim::addFlow(std::string name)
	{
		flows.push_back(UNCRZ_FBF_anim_flow(name));
		lastFlow = &flows[flows.size() - 1];
	}

void UNCRZ_FBF_anim::setFlowStart(int sm, int smd)
	{
		lastFlow->start.motion = sm;
		lastFlow->start.motionDuration = smd;
	}

void UNCRZ_FBF_anim::setFlowStart(std::string sms, int smd)
	{
		for (int i = lastFlow->motions.size() - 1; i >= 0; i--)
		{
			if (lastFlow->motions[i].name == sms)
			{
				lastFlow->start.motion = i;
				lastFlow->start.motionDuration = smd;
				return;
			}
		}
	}
	
void UNCRZ_FBF_anim::endMotion()
	{
		lastFlow->motions[lastFlow->motions.size() - 1].endInstr = (int)lastFlow->instrs.size() - 1;
	}
	
void UNCRZ_FBF_anim::addMotion(std::string name)
	{
		lastFlow->motions.push_back(UNCRZ_FBF_anim_motion(name));
		lastFlow->motions[lastFlow->motions.size() - 1].startInstr = (int)lastFlow->instrs.size();
	}
	
void UNCRZ_FBF_anim::setMotionDuration(int duration)
	{
		lastFlow->motions[lastFlow->motions.size() - 1].duration = duration;
	}

void UNCRZ_FBF_anim::setMotionCausesUpdate(bool mg)
	{
		lastFlow->motions[lastFlow->motions.size() - 1].causesUpdate = mg;
	}

void UNCRZ_FBF_anim::addLine(std::string line)
	{
		lastFlow->instrs.push_back(UNCRZ_FBF_anim_instr(line, model));
	}

UNCRZ_FBF_anim::UNCRZ_FBF_anim(UNCRZ_model* modelN)
	{
		model = modelN;
		modelName = model->name;
	}
//};

/*
OMG OMG Concept Code, right?
// node flags
DWORD NF_none = 0x00000000;

// node game flags
DWORD NGF_none = 0x00000000;

struct UNCRZ_node
{
public:
	bool tight; // whether or not it's tight here
	DWORD nodeFlags;
	DWORD nodeGameFlags;
	D3DXVECTOR3 location;

};

struct UNCRZ_pathing
{
public:
	std::vector<UNCRZ_node> nodes;


};
*/

// this bit isn't strictly UNCRZ
// separators
const DWORD ST_none = 0;
const DWORD ST_circle = 1;
const DWORD ST_square = 2;

// 2D Spacing Separator
struct separator
{
public:
	DWORD separatorType;
	float dimension;

	separator() { /* chill */ };

	separator(DWORD separatorTypeN, float dimensionN)
	{
		separatorType = separatorTypeN;
		dimension = dimensionN;
	}

	// linear dist ^ 2
	static float getLinearDistSqr(float ax, float ay, float bx, float by)
	{
		float dx = ax - bx;
		float dy = ay - by;
		return dx * dx + dy * dy;
	}

	// linear dist
	static float getLinearDist(float ax, float ay, float bx, float by)
	{
		float dx = ax - bx;
		float dy = ay - by;
		return sqrtf(dx * dx + dy * dy);
	}

	// axis dist
	static float getAxisDist(float ax, float ay, float bx, float by)
	{
		float dx = abs(ax - bx);
		float dy = abs(ay - by);
		if (dx > dy)
			return dx;
		else
			return dy;
	}

	static bool fits(separator as, float ax, float ay, separator bs, float bx, float by, float margin)
	{
		if (as.separatorType == ST_circle)
		{
			if (bs.separatorType == ST_circle)
			{ // circle circle
				float distSqr = getLinearDistSqr(ax, ay, bx, by);
				float cond = as.dimension + bs.dimension + margin;
				if (distSqr >= cond * cond)
					return true;
				else
					return false;
			}
			else if (bs.separatorType == ST_square)
			{
				// impement square/cirlce
				goto squareSquare; // just for now, right?
			}
		}
		else if (as.separatorType == ST_square)
		{
			if (bs.separatorType == ST_circle)
			{
				// impement square/cirlce
				goto squareSquare; // just for now, right?
			}
			else if (bs.separatorType == ST_square)
			{ // square square
squareSquare:
				float dist = getAxisDist(ax, ay, bx, by);
				float cond = as.dimension + bs.dimension + margin;
				return dist >= cond;
			}
		}

		return true;
	}

	static float sign(float num)
	{
		if (num > 0)
			return 1;
		else if (num < 0)
			return -1;
		return 0;
	}

	static bool getIntersect(float a1X, float a1Y, float a2X, float a2Y, float b1X, float b1Y, float b2X, float b2Y, float* resX, float* resY) 
	{
		// round /everything/ - I'm sure this is a good idea, right?
		a1X = (float)(int)a1X;
		a1Y = (float)(int)a1Y;
		a2X = (float)(int)a2X;
		a2Y = (float)(int)a2Y;
		b1X = (float)(int)b1X;
		b1Y = (float)(int)b1Y;
		b2X = (float)(int)b2X;
		b2Y = (float)(int)b2Y;

		float aM, bM, aC, bC; 
		float x = 0; 
		float y = 0; 
		
		if (a1X == a2X && b1X == b2X)
		{
			if (a1X == b1X)
			{
				if ((sign(a1Y - b1Y) != sign(a2Y - b1Y)) || (sign(a1Y - b2Y) != sign(a2Y - b2Y)))
				{
					*resX = b1X;
					*resY = b1Y;
					return true;
				}
				*resX = -1;
				*resY = -1;
				return false;
			}
		}
		else if (a1X == a2X)
		{
			bM = (b1Y - b2Y) / (b1X - b2X); 
			bC = b1Y - bM * b1X;
			
			x = a1X;
			y = bM * x + bC;
		}
		else if (b1X == b2X)
		{
			aM = (a1Y - a2Y) / (a1X - a2X); 
			aC = a1Y - aM * a1X;
			
			x = b1X;
			y = aM * x + aC;
		}
		else
		{
			aM = (a1Y - a2Y) / (a1X - a2X); 
			aC = a1Y - aM * a1X; 
			bM = (b1Y - b2Y) / (b1X - b2X); 
			bC = b1Y - bM * b1X; 
			
			x = (bC - aC) / (aM - bM); 
			y = (bM * aC - aM * bC) / (bM - aM); 
		}
		
		if (((a1X >= x && a2X <= x) || (a1X <= x && a2X >= x)) && ((a1Y <= y && a2Y >= y) || (a1Y >= y && a2Y <= y)) && ((b1X >= x && b2X <= x) || (b1X <= x && b2X >= x)) && ((b1Y <= y && b2Y >= y) || (b1Y >= y && b2Y <= y))) 
		{
			*resX = x;
			*resY = y;
			return true;
		}
		
		*resX = -1;
		*resY = -1;
		return false;
	}

	// a and b describe the line (a must be source), c described the centre of the sepor
	// px, py is the prefered point to more towards to avoid face-palming this
	bool collides(float ax, float ay, float bx, float by, float cx, float cy, float margin, float pMargin, float* px, float* py)
	{
		if (separatorType == ST_circle)
		{
			float reqDist = (margin + dimension);

			float dx = cx - ax; // this is infact the vector from a to c (source to centre)
			float dy = cy - ay;

			float ang = atan2f(dx, -dy);
			float pDist = (dimension + margin + pMargin);
			float ox = cos(ang);
			float oy = sin(ang);

			float e1x = cx - ox * reqDist;
			float e1y = cy - oy * reqDist;
			float e2x = cx + ox * reqDist;
			float e2y = cy + oy * reqDist;
			float tempX, tempY;
			
			if (getIntersect(ax, ay, bx, by, e1x, e1y, cx, cy, &tempX, &tempY))
			{
				*px = cx - ox * pDist;
				*py = cy - oy * pDist;
				return true;
			}
			else if (getIntersect(ax, ay, bx, by, e2x, e2y, cx, cy, &tempX, &tempY))
			{
				*px = cx + ox * pDist;
				*py = cy + oy * pDist;
				return true;
			}
		}
		else if (separatorType == ST_square)
		{
			float reqDist = (margin + dimension);
			float pDist = (dimension + margin + pMargin);

			// see if we are off a corner or off an edge
			float fx0 = cx - reqDist;
			float fx1 = cx + reqDist;
			float fy0 = cy - reqDist;
			float fy1 = cy + reqDist;

			// cheap checks
			
			if (ax > fx1 && bx >= ax)
				return false;
			if (ax < fx0 && bx <= ax)
				return false;
			if (ay > fy1 && by >= ay)
				return false;
			if (ay < fy0 && by <= ay)
				return false;

			if (ax > fx0 && ax < fx1)
				goto squareEdge;
			if (ay > fy0 && ay < fy1)
				goto squareEdge;
			goto squareCorner;

			// off an edge
squareEdge:
			{
				float dx = cx - ax; // this is infact the vector from a to c (source to centre)
				float dy = cy - ay;

				float ex = cx;
				float ey = cy;
				float pcx = cx;
				float pcy = cy;
				float ox;
				float oy;
				// find near edge
				if (ax > fx1)
				{
					if (bx > ax)
						return false;
					cx += reqDist;
					pcx += pDist;
					ox = 0;
					oy = 1;
				}
				else if (ax < fx0)
				{
					cx -= reqDist;
					pcx -= pDist;
					ox = 0;
					oy = 1;
				}
				else if (ay > fy1)
				{
					cy += reqDist;
					pcy += pDist;
					oy = 0;
					ox = 1;
				}
				else if (ay < fy0)
				{
					cy -= reqDist;
					pcy -= pDist;
					oy = 0;
					ox = 1;
				}
				else
				{
					// errr what?
					goto squareCorner; // an iffy result is better than no result (or crashing)
				}

				// offset centre to the near edge

				float e1x = cx - ox * reqDist;
				float e1y = cy - oy * reqDist;
				float e2x = cx + ox * reqDist;
				float e2y = cy + oy * reqDist;
				float tempX, tempY;
				

				// from centre
				if (getIntersect(ax, ay, bx, by, e1x, e1y, ex, ey, &tempX, &tempY))
				{
					*px = pcx - ox * pDist;
					*py = pcy - oy * pDist;
					return true;
				}
				else if (getIntersect(ax, ay, bx, by, e2x, e2y, ex, ey, &tempX, &tempY))
				{
					*px = pcx + ox * pDist;
					*py = pcy + oy * pDist;
					return true;
				}
				// from near edge
				else if (getIntersect(ax, ay, bx, by, e1x, e1y, cx, cy, &tempX, &tempY))
				{
					*px = pcx - ox * pDist;
					*py = pcy - oy * pDist;
					return true;
				}
				else if (getIntersect(ax, ay, bx, by, e2x, e2y, cx, cy, &tempX, &tempY))
				{
					*px = pcx + ox * pDist;
					*py = pcy + oy * pDist;
					return true;
				}

				return false;
			}

			// off a corner
squareCorner:
			{
				float ox;
				float oy;
				// find near corner
				if (ax > fx1)
					ox = 1;
				else
					ox = -1;
				if (ay > fy1)
					oy = 1;
				else
					oy = -1;

				/*else
				{ // THIS CODE IS CURRENTLY OUT OF USE, IT MAY BE BROUGHT BACK IN, DO NOT DELETE IT!!!
					// what the?
					// do some dodgy maths, hope it works
					float dx = cx - ax; // this is infact the vector from a to c (source to centre)
					float dy = cy - ay;
					float ang = atan2f(dx, -dy);
					ox = cos(ang);
					oy = sin(ang);
					ox = sign(ox); // snap to corners
					oy = sign(oy);
				}*/

				// ox/oy currently points to near corner
				float ex = cx + ox * reqDist; // coords of the near corner
				float ey = cy + oy * reqDist;

				// rotate ox/oy pi/2
				float toy = oy;
				oy = ox;
				ox = -toy;

				float e1x = cx - ox * reqDist;
				float e1y = cy - oy * reqDist;
				float e2x = cx + ox * reqDist;
				float e2y = cy + oy * reqDist;
				float tempX, tempY;
				
				// from near edge
				if (getIntersect(ax, ay, bx, by, e1x, e1y, ex, ey, &tempX, &tempY))
				{
					*px = cx - ox * pDist;
					*py = cy - oy * pDist;
					return true;
				}
				else if (getIntersect(ax, ay, bx, by, e2x, e2y, ex, ey, &tempX, &tempY))
				{
					*px = cx + ox * pDist;
					*py = cy + oy * pDist;
					return true;
				}
				// from centre
				else if (getIntersect(ax, ay, bx, by, e1x, e1y, cx, cy, &tempX, &tempY))
				{
					*px = cx - ox * pDist;
					*py = cy - oy * pDist;
					return true;
				}
				else if (getIntersect(ax, ay, bx, by, e2x, e2y, cx, cy, &tempX, &tempY))
				{
					*px = cx + ox * pDist;
					*py = cy + oy * pDist;
					return true;
				}

				return false;
			}

						// off a corner
//squareCorner:
//			{
//				float dx = cx - ax; // this is infact the vector from a to c (source to centre)
//				float dy = cy - ay;
//
//				float ang = atan2f(dx, -dy);
//				float ox = cos(ang);
//				float oy = sin(ang);
//
//				ox = sign(ox); // snap to corners
//				oy = sign(oy);
//
//				float ex = cx + oy * reqDist; // coords of the ear corner
//				float ey = cy - ox * reqDist;
//				float e1x = cx - ox * reqDist;
//				float e1y = cy - oy * reqDist;
//				float e2x = cx + ox * reqDist;
//				float e2y = cy + oy * reqDist;
//				float tempX, tempY;
//				
//				if (getIntersect(ax, ay, bx, by, e1x, e1y, ex, ey, &tempX, &tempY))
//				{
//					if (getLinearDist(tempX, tempY, cx, cy) < reqDist)
//					{
//						*px = cx - ox * pDist;
//						*py = cy - oy * pDist;
//						return true;
//					}
//					return false;
//				}
//				else if (getIntersect(ax, ay, bx, by, e2x, e2y, ex, ey, &tempX, &tempY))
//				{
//					if (getLinearDist(tempX, tempY, cx, cy) < reqDist)
//					{
//						*px = cx + ox * pDist;
//						*py = cy + oy * pDist;
//						return true;
//					}
//					return false;
//				}
//
//				return false;
//			}
		}

		return false;
	}

	// BROKED BROKED
	// a and b describe the line, c described the centre of the sepor
	// px, py is the prefered point to more towards to avoid face-palming this
	//bool collides(float ax, float ay, float bx, float by, float cx, float cy, float margin, float pMargin, float* px, float* py)
	//{
	//	if (separatorType == ST_circle)
	//	{
	//		float reqDist = (margin + dimension) * (margin + dimension); 

	//		float dx = bx - ax;
	//		float dy = by - ay;

	//		float m = -(dx / dy);

	//		float e1x = cx - reqDist;
	//		float e1y = cy - m * reqDist;
	//		float e2x = cx + reqDist;
	//		float e2y = cy + m * reqDist;
	//		float tempX, tempY;
	//		
	//		if (getIntersect(ax, ay, bx, by, e1x, e1y, cx, cy, &tempX, &tempY))
	//		{
	//			if (getLinearDistSqr(tempX, tempY, cx, cy) < reqDist)
	//			{
	//				float ang = atan2f(dx, -dy);
	//				float pDist = (dimension + margin + pMargin);
	//				*px = cx + cos(ang) * pDist;
	//				*py = cy + sin(ang) * pDist;
	//				return true;
	//			}
	//			return false;
	//		}
	//		else if (getIntersect(ax, ay, bx, by, e2x, e2y, cx, cy, &tempX, &tempY))
	//		{
	//			if (getLinearDistSqr(tempX, tempY, cx, cy) < reqDist)
	//			{
	//				float ang = atan2f(-dx, dy);
	//				float pDist = (dimension + margin + pMargin);
	//				*px = cx + cos(ang) * pDist;
	//				*py = cy + sin(ang) * pDist;
	//				return true;
	//			}
	//			return false;
	//		}
	//	}
	//	else if (separatorType == ST_square)
	//	{
	//		return false;
	//	}

	//	return false;
	//}
};
// back to (mostly) uncrz	stuff

// generic object states
const DWORD OS_undefined = 0x000;
// worker states
const DWORD WS_idle = 0x100; // static
const DWORD WS_move = 0x101; // motion
const DWORD WS_moveToTree = 0x102; // motion
const DWORD WS_collectFromTree = 0x103; // static
const DWORD WS_unload = 0x104; // motion/static (moveToBase)
const DWORD WS_construct = 0x105; // motion/static
const DWORD WS_rndMove = 0x106; // motion
// tree states
const DWORD TS_idle = 0x200; // static
/*
const DWORD TS_falling = 0x201; // motion
const DWORD TS_felled = 0x202; // static
*/

// obj types
const DWORD OT_worker = 1;
const DWORD OT_tree = 2;
const DWORD OT_base = 3;
const DWORD OT_refinary = 4;
const DWORD OT_plant = 5;
const DWORD OT_lamp = 6;

struct UNCRZ_obj
{
public:
	D3DXVECTOR3 offset;
	D3DXVECTOR3 rotation;
	D3DXMATRIX offsetMatrix; // call updateMatrices whenever the offset are modified
	D3DXMATRIX rotationMatrix; // call updateMatrices whenever the offset/rotation are modified
	D3DXMATRIX offsetMatrixinv; // call updateMatrices whenever the offset are modified
	D3DXMATRIX rotationMatrixinv; // call updateMatrices whenever the offset/rotation are modified
	D3DXMATRIX localTrans; // call updateMatrices whenever the offset are modified
	D3DXMATRIX localTransinv; // call updateMatrices whenever the offset/rotation are modified

	UNCRZ_model* model; // must be unique if you want unique transforms

	float zSortDepth; // used for sorting

	// game mechs (these should TOTALLY be done by inheritance)
	int latex;
	UNCRZ_obj* task; // object working at
	UNCRZ_obj* destObj; // used purly for ignoring things when doing local avoidance, do not use for anything else
	DWORD objectType;
	DWORD objectState;
	D3DXVECTOR3 dest; // destination of journey (ignore y axis)
	D3DXVECTOR3 targ; // destination of current leg (ignore y axis)
	bool targNeedsUpdate;
	bool onFire;
	separator sepor;
	int boredCounter;
	bool noDraw;

	float construction; // when this hits 1 it becomes active

	// when it has a new dest, it calls 'walk', and then it gets a new targ - when it gets there, it calls 'walk' again, and so and so forth until it hits dest

	// Obj/Sprites (this is a very inconplete concept which will be used to /try/ and solve the transparency problem for sprites
	UNCRZ_sprite* sprite;
	int spriteOffset;
	int spriteCount;
	UNCRZ_sprite_buffer* spriteBuffer;

	UNCRZ_obj(UNCRZ_model* modelN)
	{
		model = modelN;
		offset = D3DXVECTOR3(0.0, 0.0, 0.0);
		rotation = D3DXVECTOR3(0.0, 0.0, 0.0);

		noDraw = false;
	}

	void changeAnim(UNCRZ_FBF_anim* newAnim)
	{
		if (model->animInst == NULL)
			model->animInst = new UNCRZ_FBF_anim_inst(model, newAnim);
		else
			model->animInst->reset(newAnim);
	}

	void run()
	{
		if (model->animInst != NULL)
			model->animInst->anim->run(model->animInst);
	}

	void update(bool forceUpdate = false)
	{
		updateMatrices();

		D3DXMATRIX trans;
		D3DXMatrixIdentity(&trans);

		D3DXMatrixMultiply(&trans, &offsetMatrix, &trans);
		D3DXMatrixMultiply(&trans, &rotationMatrix, &trans);

		model->update(&trans, forceUpdate);
	}

	void drawMany(LPDIRECT3DDEVICE9 dxDevice, drawData* ddat, UNCRZ_model** arr, int count, DWORD drawArgs)
	{
		// no noDraw check here
		model->drawMany(dxDevice, ddat, arr, count, drawArgs);
	}

	void draw(LPDIRECT3DDEVICE9 dxDevice, drawData* ddat, DWORD drawArgs)
	{
		if (noDraw)
			return;
		model->draw(dxDevice, ddat, drawArgs);
	}

	void updateMatrices()
	{
		D3DXMatrixTranslation(&offsetMatrix, offset.x, offset.y, offset.z);
		D3DXMatrixRotationYawPitchRoll(&rotationMatrix, rotation.y, rotation.x, rotation.z);
		D3DXMatrixMultiply(&localTrans, &offsetMatrix, &rotationMatrix);
		
		D3DXMatrixInverse(&offsetMatrixinv, NULL, &offsetMatrix);
		D3DXMatrixInverse(&rotationMatrixinv, NULL, &rotationMatrix);
		D3DXMatrixInverse(&localTransinv, NULL, &localTrans);
	}
};

// enum MD
const DWORD MD_access = 0x00000001;

// PTW4 etc. terrain
struct UNCRZ_terrain
{
	std::string name;

	UNCRZ_effect effect;

	bool useTex;
	LPDIRECT3DTEXTURE9 tex;
	bool useTex0;
	LPDIRECT3DTEXTURE9 tex0;
	bool useTex1;
	LPDIRECT3DTEXTURE9 tex1;
	bool useTex2;
	LPDIRECT3DTEXTURE9 tex2;
	bool useTex3;
	LPDIRECT3DTEXTURE9 tex3;

	D3DXHANDLE tech;
	D3DXHANDLE lightTech;
	D3DXHANDLE decalTech;
	D3DXHANDLE dynamicDecalTech;

	LPDIRECT3DVERTEXBUFFER9 vBuff;
	LPDIRECT3DVERTEXDECLARATION9 vertexDec;
	LPDIRECT3DINDEXBUFFER9 iBuff;
	DWORD vertexType;
	UINT stride;
	int numVertices;
	int numIndices;

	void* vertexArray; // array of vertices
	short* indexArray; // array of indicies

	std::vector<UNCRZ_decal*> decals;

	float xCorner;
	float zCorner;
	int width;
	int height;
	float dimension;

	UNCRZ_terrain()
	{
	}

	UNCRZ_terrain(std::string nameN)
	{
		name = nameN;
	}

	void destroy()
	{
		int len = width * height;
		vBuff->Release();
		iBuff->Release();
	}

	vertexPTW4* getPTW4(int x, int z)
	{
		return (((vertexPTW4*)vertexArray) + x + z * width);
	}

	vertexPTW4* getPTW4(int index)
	{
		int x = index % width;
		int z = (index - x) / width;
		if (x < 0 || z < 0 || x >= width || z >= height)
			return NULL;
		return (((vertexPTW4*)vertexArray) + x + z * width);
	}

	vertexPTW4* getPTW4(int index, int* x, int* z)
	{
		*x = index % width;
		*z = (index - *x) / width;
		return (((vertexPTW4*)vertexArray) + *x + *z * width);
	}

	int getX(int index)
	{
		return index % width;
	}

	int getZ(int index)
	{
		int x = index % width;
		return (index - x) / width;
	}

	void setTextures()
	{
		if (useTex)
			effect.setTexture(tex);
		if (useTex0)
			effect.setTexture0(tex0);
		if (useTex1)
			effect.setTexture1(tex1);
		if (useTex2)
			effect.setTexture2(tex2);
		if (useTex3)
			effect.setTexture3(tex3);
	}

	void draw(LPDIRECT3DDEVICE9 dxDevice, drawData* ddat, DWORD drawArgs)
	{
		HRESULT res;

		dxDevice->SetVertexDeclaration(vertexDec);
		dxDevice->SetStreamSource(0, vBuff, 0, stride);
		dxDevice->SetIndices(iBuff);

		if (drawArgs & DF_light)
		{
			effect.setTechnique(lightTech);
			effect.setLightType(ddat->lightType);
			effect.setLightDepth(ddat->lightDepth);
			effect.setLightConeness(ddat->lightConeness);
		}
		else
		{
			effect.setTechnique(tech);
			effect.setLightCoof(ddat->lightCoof);
		}

		setTextures();

		effect.setEyePos(&ddat->eyePos);
		effect.setEyeDir(&ddat->eyeDir);
		effect.setTicker(ddat->ticker);
		effect.setViewProj(ddat->viewProj);

		effect.effect->CommitChanges();

		UINT numPasses, pass;
		effect.effect->Begin(&numPasses, 0);

		// disable blend
		dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

		// pass 0
		if (drawArgs & DF_light)
			effect.effect->BeginPass((int)ddat->lightType);
		else if (ddat->performPlainPass)
			effect.effect->BeginPass(0);
		else
			goto skipPlainPass;

		res = dxDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, numVertices, 0, numIndices / 3);
		if (res != D3D_OK)
			res = res;
		effect.effect->EndPass();
skipPlainPass:

		if (numPasses > 1 && (drawArgs & DF_light) == false)
		{
			// enable blend
			dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			dxDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
			dxDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
			dxDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

			// light pass
			for (int ldi = ddat->lightDatas.size() - 1; ldi >= 0; ldi--)
			{
				lightData* ld = ddat->lightDatas[ldi];
				
				if (!ld->lightEnabled)
					continue;

				effect.setLightData(ld);
				effect.effect->CommitChanges();

				effect.effect->BeginPass((int)ld->lightType + 1);
				res = dxDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, numVertices, 0, numIndices / 3);
				if (res != D3D_OK)
					res = res;
				effect.effect->EndPass();
			}

		}
		// disable blend
		dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

		effect.effect->End();

		if (drawArgs & DF_light)
			return; // no decals for light

		if (decals.size() == 0)
			goto skipToDynamicDecals;

		ddat->startTimer(true);

		// decals
		dxDevice->SetVertexDeclaration(vertexDecPAT4);
		effect.setTechnique(decalTech);
		
		float white[4];
		white[0] = 1.0;
		white[1] = 1.0;
		white[2] = 1.0;
		white[3] = 1.0;
		effect.setcolMod(&(white[0]));

		effect.effect->CommitChanges();

		dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		dxDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
		dxDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		dxDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

		effect.effect->Begin(&numPasses, 0);

		// pass 0
		if (drawArgs & DF_light)
			effect.effect->BeginPass((int)ddat->lightType);
		else if (ddat->performPlainPass)
			effect.effect->BeginPass(0);
		else
			goto skipPlainDecalPass;

		for (int i = 0; i < decals.size(); i++)
		{
			effect.setTexture(decals[i]->tex);
			effect.effect->CommitChanges();

			res = dxDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST, decals[i]->numFaces, decals[i]->vertexArray, sizeof(vertexPAT4));
			if (res != D3D_OK)
				res = res;
		}
		effect.effect->EndPass();
skipPlainDecalPass:

		if (numPasses > 1 && (drawArgs & DF_light) == false)
		{
			// enable blend
			dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			dxDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
			dxDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			dxDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

			// light pass
			for (int ldi = ddat->lightDatas.size() - 1; ldi >= 0; ldi--)
			{
				lightData* ld = ddat->lightDatas[ldi];
				
				if (!ld->lightEnabled)
					continue;

				effect.setLightData(ld);
				effect.effect->CommitChanges();

				effect.effect->BeginPass((int)ld->lightType + 1);
				for (int i = 0; i < decals.size(); i++)
				{
					effect.setTexture(decals[i]->tex);
					effect.effect->CommitChanges();

					res = dxDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST, decals[i]->numFaces, decals[i]->vertexArray, sizeof(vertexPAT4));
					if (res != D3D_OK)
						res = res;
				}
				effect.effect->EndPass();
			}

		}
		// disable blend
		dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		
		ddat->stopTimer(0, true, "Decals: ", GDOF_dms);

		effect.effect->End();

		/*effect.effect->Begin(&numPasses, 0);

		for (pass = 0; pass < numPasses; pass++)
		{
			effect.effect->BeginPass(pass);

			//for (int i = decals.size() - 1; i >= 0; i--)
			for (int i = 0; i < decals.size(); i++)
			{
				effect.setTexture(decals[i]->tex);
				effect.effect->CommitChanges();

				res = dxDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST, decals[i]->numFaces, decals[i]->vertexArray, sizeof(vertexPAT4));
				if (res != D3D_OK)
					res = res;
			}

			effect.effect->EndPass();
		}

		effect.effect->End();*/

		// dynamic decals (DO NOT GET CONFUSED WITH LIGHTS)
skipToDynamicDecals:
		if (ddat->dynamicDecalDatas.size() == 0)
			return;

		ddat->startTimer(true);

		// disable z write
		dxDevice->SetRenderState(D3DRS_ZWRITEENABLE, false);

		dxDevice->SetVertexDeclaration(vertexDec);
		dxDevice->SetStreamSource(0, vBuff, 0, stride);
		dxDevice->SetIndices(iBuff);
		effect.setTechnique(dynamicDecalTech);
		
		effect.effect->Begin(&numPasses, 0);

		if (numPasses > 0 && (drawArgs & DF_light) == false)
		{
			// enable blend
			dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			dxDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
			dxDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			dxDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

			// dynamic decal pass
			for (int dddi = ddat->dynamicDecalDatas.size() - 1; dddi >= 0; dddi--)
			{
				dynamicDecalData* ddd = ddat->dynamicDecalDatas[dddi];
				
				if (!ddd->decalEnabled)
					continue;

				effect.setDynamicDecalData(ddd);
				effect.effect->CommitChanges();

				effect.effect->BeginPass((int)ddd->lightType);
				res = dxDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, numVertices, 0, numIndices / 3);
				if (res != D3D_OK)
					res = res;
				effect.effect->EndPass();
			}

		}
		// disable blend
		dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

		// re-enable z write
		dxDevice->SetRenderState(D3DRS_ZWRITEENABLE, true);
		
		ddat->stopTimer(1, true, "DynDecals: ", GDOF_dms);

		effect.effect->End();
	}

	void release()
	{
		vBuff->Release();
		iBuff->Release();
		delete[stride * numVertices] vertexArray;
		delete[sizeof(short) * numIndices] indexArray;
	}

	void fillVBuff()
	{
		VOID* buffPtr;
		vBuff->Lock(0, numVertices * stride, (VOID**)&buffPtr, D3DLOCK_DISCARD);
		memcpy(buffPtr, vertexArray, numVertices * stride);
		vBuff->Unlock();
	}

	void createVBuff(LPDIRECT3DDEVICE9 dxDevice, void* vds)
	{
		dxDevice->CreateVertexBuffer(numVertices * stride, D3DUSAGE_WRITEONLY, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, D3DPOOL_DEFAULT, &vBuff, NULL);

		vertexArray = malloc(stride * numVertices);
		memcpy(vertexArray, vds, numVertices * stride);

		fillVBuff();
	}

	void fillIBuff()
	{
		VOID* buffPtr;
		iBuff->Lock(0, numIndices * sizeof (short), (VOID**)&buffPtr, D3DLOCK_DISCARD);
		memcpy(buffPtr, indexArray, numIndices * sizeof (short));
		iBuff->Unlock();
	}

	void createIBuff(LPDIRECT3DDEVICE9 dxDevice, short* ids)
	{
		dxDevice->CreateIndexBuffer(numIndices * sizeof (short), D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &iBuff, NULL);

		indexArray = (short*)malloc(sizeof (short) * numIndices);
		memcpy(indexArray, ids, numIndices * sizeof (short));

		fillIBuff();
	}

	// just modifies the y prop, autoGenNormals and fillVBuff should be done externally
	void level(int sx, int sz, int ex, int ez, float height, float coof)
	{
		for (int i = sx; i <= ex; i++)
		{
			for (int j = sz; j <= ez; j++)
			{
				vertexPTW4* vPTW4 = getPTW4(i, j);
				vPTW4->y = height * coof + (1.0 - coof) * vPTW4->y;
				//(((vertexPTW4*)vertexArray)[i + j * width]).y = height + (1.0 - coof) * ((((vertexPTW4*)vertexArray)[i + j * width]).y - height);
			}
		}
	}

	bool collidesVert(D3DXVECTOR3* rayPos, D3DXVECTOR3* rayDir, float* distRes)
	{
		if (vertexType == VX_PTW4)
		{
			return collidesVertVX_PTW4(rayPos, rayDir, distRes);
		}
		return false;
	}

	bool collidesVertVX_PTW4(D3DXVECTOR3* rayPos, D3DXVECTOR3* rayDir, float* distRes)
	{
		float widthmo = width - 1; // this makes sense, don't doubt it

		// 2 indicies per sqr
		int rx = (rayPos->x - xCorner) / dimension; // roughX
		int rz = (rayPos->z - zCorner) / dimension; // roughZ
		int offset = (rx + widthmo * rz) * 6;

		float uRes, vRes;
		vertexPTW4 a;
		vertexPTW4 b;
		vertexPTW4 c;
		D3DXVECTOR4 va;
		D3DXVECTOR4 vb;
		D3DXVECTOR4 vc;

		bool victory = false;
		float bestRes;
		float tempRes;

		for (int j = offset - widthmo * 6 * 2; j < offset + widthmo * 6 * 2; j += widthmo * 6)
		{
			for (int i = j - 12; i < j + 12; i += 3)
			{
				if (i < 0)
					continue;
				if (i >= widthmo * height * 6)
					continue;
				a = (((vertexPTW4*)vertexArray)[(int)indexArray[i]]);
				b = (((vertexPTW4*)vertexArray)[(int)indexArray[i + 1]]);
				c = (((vertexPTW4*)vertexArray)[(int)indexArray[i + 2]]);

				va = D3DXVECTOR4(a.x, a.y, a.z, 1.0f);
				vb = D3DXVECTOR4(b.x, b.y, b.z, 1.0f);
				vc = D3DXVECTOR4(c.x, c.y, c.z, 1.0f);

				if (D3DXIntersectTri((D3DXVECTOR3*)&va, (D3DXVECTOR3*)&vb, (D3DXVECTOR3*)&vc, rayPos, rayDir, &uRes, &vRes, &tempRes))
				{
					if (victory == false)
					{
						victory = true;
						bestRes = tempRes;
					}
					else if (tempRes < bestRes)
					{
						bestRes = tempRes;
					}
				}
				//if (D3DXIntersectTri((D3DXVECTOR3*)&va, (D3DXVECTOR3*)&vb, (D3DXVECTOR3*)&vc, rayPos, rayDir, &uRes, &vRes, distRes))
				//	return true;
			}
		}

		if (victory)
		{
			*distRes = bestRes;
			return true;
		}
		return false;
	}

	int collidesVertex(D3DXVECTOR3* rayPos, D3DXVECTOR3* rayDir, float* distRes)
	{
		if (vertexType == VX_PTW4)
		{
			return collidesVertexVX_PTW4(rayPos, rayDir, distRes);
		}

		return false;
	}

	bool collides(D3DXVECTOR3* rayPos, D3DXVECTOR3* rayDir, float* distRes)
	{
		if (vertexType == VX_PTW4)
		{
			return collidesVX_PTW4(rayPos, rayDir, distRes);
		}

		return false;
	}

	int collidesVertexVX_PTW4(D3DXVECTOR3* rayPos, D3DXVECTOR3* rayDir, float* distRes)
	{
		float uRes, vRes;
		vertexPTW4 a;
		vertexPTW4 b;
		vertexPTW4 c;
		D3DXVECTOR4 va;
		D3DXVECTOR4 vb;
		D3DXVECTOR4 vc;

		bool victory = false;
		int indexRes;
		float bestRes;
		float tempRes;

		for (int i = 0; i < numIndices; i += 3)
		{
			a = (((vertexPTW4*)vertexArray)[(int)indexArray[i]]);
			b = (((vertexPTW4*)vertexArray)[(int)indexArray[i + 1]]);
			c = (((vertexPTW4*)vertexArray)[(int)indexArray[i + 2]]);

			va = D3DXVECTOR4(a.x, a.y, a.z, 1.0f);
			vb = D3DXVECTOR4(b.x, b.y, b.z, 1.0f);
			vc = D3DXVECTOR4(c.x, c.y, c.z, 1.0f);

			if (D3DXIntersectTri((D3DXVECTOR3*)&va, (D3DXVECTOR3*)&vb, (D3DXVECTOR3*)&vc, rayPos, rayDir, &uRes, &vRes, &tempRes))
			{
				if (victory == false || (victory && tempRes < bestRes))
				{
					victory = true;
					bestRes = tempRes;
					
					if (uRes < 0.5f && vRes < 0.5f)
						indexRes = (int)indexArray[i];
					else if (uRes > vRes)
						indexRes = indexArray[i + 1];
					else
						indexRes = indexArray[i + 2];
				}
			}
			//if (D3DXIntersectTri((D3DXVECTOR3*)&va, (D3DXVECTOR3*)&vb, (D3DXVECTOR3*)&vc, rayPos, rayDir, &uRes, &vRes, distRes))
			//{
			//	if (uRes < 0.5f && vRes < 0.5f)
			//		return (int)indexArray[i];
			//	if (uRes > vRes)
			//		return (int)indexArray[i + 1];
			//	return (int)indexArray[i + 2];
			//}
		}

		if (victory)
		{
			*distRes = bestRes;
			return indexRes;
		}
		return -1;
	}

	bool collidesVX_PTW4(D3DXVECTOR3* rayPos, D3DXVECTOR3* rayDir, float* distRes)
	{
		float uRes, vRes;
		vertexPTW4 a;
		vertexPTW4 b;
		vertexPTW4 c;
		D3DXVECTOR4 va;
		D3DXVECTOR4 vb;
		D3DXVECTOR4 vc;

		bool victory = false;
		float bestRes;
		float tempRes;

		for (int i = 0; i < numIndices; i += 3)
		{
			a = (((vertexPTW4*)vertexArray)[(int)indexArray[i]]);
			b = (((vertexPTW4*)vertexArray)[(int)indexArray[i + 1]]);
			c = (((vertexPTW4*)vertexArray)[(int)indexArray[i + 2]]);

			va = D3DXVECTOR4(a.x, a.y, a.z, 1.0f);
			vb = D3DXVECTOR4(b.x, b.y, b.z, 1.0f);
			vc = D3DXVECTOR4(c.x, c.y, c.z, 1.0f);

			if (D3DXIntersectTri((D3DXVECTOR3*)&va, (D3DXVECTOR3*)&vb, (D3DXVECTOR3*)&vc, rayPos, rayDir, &uRes, &vRes, &tempRes))
			{
				if (victory == false)
				{
					victory = true;
					bestRes = tempRes;
				}
				else if (tempRes < bestRes)
				{
					bestRes = tempRes;
				}
			}
			//if (D3DXIntersectTri((D3DXVECTOR3*)&va, (D3DXVECTOR3*)&vb, (D3DXVECTOR3*)&vc, rayPos, rayDir, &uRes, &vRes, distRes))
			//	return true;
		}

		if (victory)
		{
			*distRes = bestRes;
			return true;
		}
		return false;
	}
};

// flat rendered terrain

// like a mip level...
struct UNCRZ_frion
{
	UNCRZ_effect effect;

	LPDIRECT3DTEXTURE9 tex;

	D3DXHANDLE flatTech;
	D3DXHANDLE tech;
	D3DXHANDLE lightTech;
	D3DXHANDLE dynamicDecalTech;

	LPDIRECT3DVERTEXBUFFER9 vBuff;
	LPDIRECT3DVERTEXDECLARATION9 vertexDec;
	LPDIRECT3DINDEXBUFFER9 iBuff;
	DWORD vertexType;
	UINT stride;
	int numVertices;
	int numIndices;

	void* vertexArray; // array of vertices
	short* indexArray; // array of indicies

	float sx, sz, ex, ez; // start/end coords of flat rect
	int resX, resY;
	int startIndex;
	int drawLevel;

	void setTextures()
	{
		effect.setTexture(tex);
	}

	void draw(LPDIRECT3DDEVICE9 dxDevice, drawData* ddat, DWORD drawArgs)
	{
		HRESULT res;

		dxDevice->SetVertexDeclaration(vertexDec);
		dxDevice->SetStreamSource(0, vBuff, 0, stride);
		dxDevice->SetIndices(iBuff);

		if (drawArgs & DF_light)
		{
			effect.setTechnique(lightTech);
			effect.setLightType(ddat->lightType);
			effect.setLightDepth(ddat->lightDepth);
			effect.setLightConeness(ddat->lightConeness);
		}
		else
		{
			effect.setTechnique(tech);
			effect.setLightCoof(ddat->lightCoof);
		}

		setTextures();

		effect.setEyePos(&ddat->eyePos);
		effect.setEyeDir(&ddat->eyeDir);
		effect.setTicker(ddat->ticker);
		effect.setViewProj(ddat->viewProj);

		effect.effect->CommitChanges();

		UINT numPasses, pass;
		effect.effect->Begin(&numPasses, 0);

		// disable blend
		dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

		// pass 0
		if (drawArgs & DF_light)
			effect.effect->BeginPass((int)ddat->lightType);
		else if (ddat->performPlainPass)
			effect.effect->BeginPass(0);
		else
			goto skipPlainPass;

		res = dxDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, numVertices, 0, numIndices / 3);
		if (res != D3D_OK)
			res = res;
		effect.effect->EndPass();
skipPlainPass:

		if (numPasses > 1 && (drawArgs & DF_light) == false)
		{
			// enable blend
			dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			dxDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
			dxDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
			dxDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

			// light pass
			for (int ldi = ddat->lightDatas.size() - 1; ldi >= 0; ldi--)
			{
				lightData* ld = ddat->lightDatas[ldi];
				
				if (!ld->lightEnabled)
					continue;

				effect.setLightData(ld);
				effect.effect->CommitChanges();

				effect.effect->BeginPass((int)ld->lightType + 1);
				res = dxDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, numVertices, 0, numIndices / 3);
				if (res != D3D_OK)
					res = res;
				effect.effect->EndPass();
			}

		}
		// disable blend
		dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

		effect.effect->End();

		if (drawArgs & DF_light)
			return; // no dyndecals for light

		// dynamic decals (DO NOT GET CONFUSED WITH LIGHTS)
skipToDynamicDecals:
		if (ddat->dynamicDecalDatas.size() == 0)
			return;

		ddat->startTimer(true);

		// disable z write
		dxDevice->SetRenderState(D3DRS_ZWRITEENABLE, false);

		dxDevice->SetVertexDeclaration(vertexDec);
		dxDevice->SetStreamSource(0, vBuff, 0, stride);
		dxDevice->SetIndices(iBuff);
		effect.setTechnique(dynamicDecalTech);
		
		effect.effect->Begin(&numPasses, 0);

		if (numPasses > 0 && (drawArgs & DF_light) == false)
		{
			// enable blend
			dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			dxDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
			dxDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			dxDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

			// dynamic decal pass
			for (int dddi = ddat->dynamicDecalDatas.size() - 1; dddi >= 0; dddi--)
			{
				dynamicDecalData* ddd = ddat->dynamicDecalDatas[dddi];
				
				if (!ddd->decalEnabled)
					continue;

				effect.setDynamicDecalData(ddd);
				effect.effect->CommitChanges();

				effect.effect->BeginPass((int)ddd->lightType);
				res = dxDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, numVertices, 0, numIndices / 3);
				if (res != D3D_OK)
					res = res;
				effect.effect->EndPass();
			}

		}
		// disable blend
		dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

		// re-enable z write
		dxDevice->SetRenderState(D3DRS_ZWRITEENABLE, true);
		
		ddat->stopTimer(1, true, "DynDecals: ", GDOF_dms);

		effect.effect->End();
	}

	void release()
	{
		vBuff->Release();
		iBuff->Release();
		delete[stride * numVertices] vertexArray;
		delete[sizeof(short) * numIndices] indexArray;
	}

	void fillVBuff()
	{
		VOID* buffPtr;
		vBuff->Lock(0, numVertices * stride, (VOID**)&buffPtr, D3DLOCK_DISCARD);
		memcpy(buffPtr, vertexArray, numVertices * stride);
		vBuff->Unlock();
	}

	void createVBuff(LPDIRECT3DDEVICE9 dxDevice, void* vds)
	{
		dxDevice->CreateVertexBuffer(numVertices * stride, D3DUSAGE_WRITEONLY, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, D3DPOOL_DEFAULT, &vBuff, NULL);

		vertexArray = malloc(stride * numVertices);
		memcpy(vertexArray, vds, numVertices * stride);

		fillVBuff();
	}

	void fillIBuff()
	{
		VOID* buffPtr;
		iBuff->Lock(0, numIndices * sizeof (short), (VOID**)&buffPtr, D3DLOCK_DISCARD);
		memcpy(buffPtr, indexArray, numIndices * sizeof (short));
		iBuff->Unlock();
	}

	void createIBuff(LPDIRECT3DDEVICE9 dxDevice, short* ids)
	{
		dxDevice->CreateIndexBuffer(numIndices * sizeof (short), D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &iBuff, NULL);

		indexArray = (short*)malloc(sizeof (short) * numIndices);
		memcpy(indexArray, ids, numIndices * sizeof (short));

		fillIBuff();
	}
};

// flat rendered terrain
struct UNCRZ_frain
{
	std::string name;

	std::vector<UNCRZ_frion*> frions;

	UNCRZ_effect effect;

	D3DXHANDLE flatTech;
	D3DXHANDLE tech;
	D3DXHANDLE lightTech;
	D3DXHANDLE dynamicDecalTech;

	LPDIRECT3DVERTEXBUFFER9 vBuff;
	LPDIRECT3DVERTEXDECLARATION9 vertexDec;
	LPDIRECT3DINDEXBUFFER9 iBuff;
	DWORD vertexType;
	UINT stride;
	int numVertices;
	int numIndices;

	void* vertexArray; // array of vertices
	short* indexArray; // array of indicies

	//std::vector<UNCRZ_decal*> decals; TODO: write new decal management for frions

	float xCorner;
	float zCorner;
	int width;
	int height;
	float dimension;

	UNCRZ_frain()
	{
	}

	UNCRZ_frain(std::string nameN)
	{
		name = nameN;
	}

	void draw(LPDIRECT3DDEVICE9 dxDevice, drawData* ddat, DWORD drawArgs)
	{
		int drawLevel = 0;

		for (int i = frions.size() - 1; i >= 0; i--)
		{
			if (frions[i]->drawLevel == drawLevel)
			{
				frions[i]->draw(dxDevice, ddat, drawArgs);
			}
		}
	}

	void destroy()
	{
		int len = width * height;
		vBuff->Release();
		iBuff->Release();
	}

	vertexPTW4* getPTW4(int x, int z)
	{
		return (((vertexPTW4*)vertexArray) + x + z * width);
	}

	vertexPTW4* getPTW4(int index)
	{
		int x = index % width;
		int z = (index - x) / width;
		if (x < 0 || z < 0 || x >= width || z >= height)
			return NULL;
		return (((vertexPTW4*)vertexArray) + x + z * width);
	}

	vertexPTW4* getPTW4(int index, int* x, int* z)
	{
		*x = index % width;
		*z = (index - *x) / width;
		return (((vertexPTW4*)vertexArray) + *x + *z * width);
	}

	int getX(int index)
	{
		return index % width;
	}

	int getZ(int index)
	{
		int x = index % width;
		return (index - x) / width;
	}

	// just modifies the y prop, autoGenNormals and fillVBuff should be done externally
	void level(int sx, int sz, int ex, int ez, float height, float coof)
	{
		for (int i = sx; i <= ex; i++)
		{
			for (int j = sz; j <= ez; j++)
			{
				vertexPTW4* vPTW4 = getPTW4(i, j);
				vPTW4->y = height * coof + (1.0 - coof) * vPTW4->y;
				//(((vertexPTW4*)vertexArray)[i + j * width]).y = height + (1.0 - coof) * ((((vertexPTW4*)vertexArray)[i + j * width]).y - height);
			}
		}
	}

	bool collidesVert(D3DXVECTOR3* rayPos, D3DXVECTOR3* rayDir, float* distRes)
	{
		if (vertexType == VX_PTW4)
		{
			return collidesVertVX_PTW4(rayPos, rayDir, distRes);
		}
		return false;
	}

	bool collidesVertVX_PTW4(D3DXVECTOR3* rayPos, D3DXVECTOR3* rayDir, float* distRes)
	{
		float widthmo = width - 1; // this makes sense, don't doubt it

		// 2 indicies per sqr
		int rx = (rayPos->x - xCorner) / dimension; // roughX
		int rz = (rayPos->z - zCorner) / dimension; // roughZ
		int offset = (rx + widthmo * rz) * 6;

		float uRes, vRes;
		vertexPTW4 a;
		vertexPTW4 b;
		vertexPTW4 c;
		D3DXVECTOR4 va;
		D3DXVECTOR4 vb;
		D3DXVECTOR4 vc;

		bool victory = false;
		float bestRes;
		float tempRes;

		for (int j = offset - widthmo * 6 * 2; j < offset + widthmo * 6 * 2; j += widthmo * 6)
		{
			for (int i = j - 12; i < j + 12; i += 3)
			{
				if (i < 0)
					continue;
				if (i >= widthmo * height * 6)
					continue;
				a = (((vertexPTW4*)vertexArray)[(int)indexArray[i]]);
				b = (((vertexPTW4*)vertexArray)[(int)indexArray[i + 1]]);
				c = (((vertexPTW4*)vertexArray)[(int)indexArray[i + 2]]);

				va = D3DXVECTOR4(a.x, a.y, a.z, 1.0f);
				vb = D3DXVECTOR4(b.x, b.y, b.z, 1.0f);
				vc = D3DXVECTOR4(c.x, c.y, c.z, 1.0f);

				if (D3DXIntersectTri((D3DXVECTOR3*)&va, (D3DXVECTOR3*)&vb, (D3DXVECTOR3*)&vc, rayPos, rayDir, &uRes, &vRes, &tempRes))
				{
					if (victory == false)
					{
						victory = true;
						bestRes = tempRes;
					}
					else if (tempRes < bestRes)
					{
						bestRes = tempRes;
					}
				}
				//if (D3DXIntersectTri((D3DXVECTOR3*)&va, (D3DXVECTOR3*)&vb, (D3DXVECTOR3*)&vc, rayPos, rayDir, &uRes, &vRes, distRes))
				//	return true;
			}
		}

		if (victory)
		{
			*distRes = bestRes;
			return true;
		}
		return false;
	}

	int collidesVertex(D3DXVECTOR3* rayPos, D3DXVECTOR3* rayDir, float* distRes)
	{
		if (vertexType == VX_PTW4)
		{
			return collidesVertexVX_PTW4(rayPos, rayDir, distRes);
		}

		return false;
	}

	bool collides(D3DXVECTOR3* rayPos, D3DXVECTOR3* rayDir, float* distRes)
	{
		if (vertexType == VX_PTW4)
		{
			return collidesVX_PTW4(rayPos, rayDir, distRes);
		}

		return false;
	}

	int collidesVertexVX_PTW4(D3DXVECTOR3* rayPos, D3DXVECTOR3* rayDir, float* distRes)
	{
		float uRes, vRes;
		vertexPTW4 a;
		vertexPTW4 b;
		vertexPTW4 c;
		D3DXVECTOR4 va;
		D3DXVECTOR4 vb;
		D3DXVECTOR4 vc;

		bool victory = false;
		int indexRes;
		float bestRes;
		float tempRes;

		for (int i = 0; i < numIndices; i += 3)
		{
			a = (((vertexPTW4*)vertexArray)[(int)indexArray[i]]);
			b = (((vertexPTW4*)vertexArray)[(int)indexArray[i + 1]]);
			c = (((vertexPTW4*)vertexArray)[(int)indexArray[i + 2]]);

			va = D3DXVECTOR4(a.x, a.y, a.z, 1.0f);
			vb = D3DXVECTOR4(b.x, b.y, b.z, 1.0f);
			vc = D3DXVECTOR4(c.x, c.y, c.z, 1.0f);

			if (D3DXIntersectTri((D3DXVECTOR3*)&va, (D3DXVECTOR3*)&vb, (D3DXVECTOR3*)&vc, rayPos, rayDir, &uRes, &vRes, &tempRes))
			{
				if (victory == false || (victory && tempRes < bestRes))
				{
					victory = true;
					bestRes = tempRes;
					
					if (uRes < 0.5f && vRes < 0.5f)
						indexRes = (int)indexArray[i];
					else if (uRes > vRes)
						indexRes = indexArray[i + 1];
					else
						indexRes = indexArray[i + 2];
				}
			}
			//if (D3DXIntersectTri((D3DXVECTOR3*)&va, (D3DXVECTOR3*)&vb, (D3DXVECTOR3*)&vc, rayPos, rayDir, &uRes, &vRes, distRes))
			//{
			//	if (uRes < 0.5f && vRes < 0.5f)
			//		return (int)indexArray[i];
			//	if (uRes > vRes)
			//		return (int)indexArray[i + 1];
			//	return (int)indexArray[i + 2];
			//}
		}

		if (victory)
		{
			*distRes = bestRes;
			return indexRes;
		}
		return -1;
	}

	bool collidesVX_PTW4(D3DXVECTOR3* rayPos, D3DXVECTOR3* rayDir, float* distRes)
	{
		float uRes, vRes;
		vertexPTW4 a;
		vertexPTW4 b;
		vertexPTW4 c;
		D3DXVECTOR4 va;
		D3DXVECTOR4 vb;
		D3DXVECTOR4 vc;

		bool victory = false;
		float bestRes;
		float tempRes;

		for (int i = 0; i < numIndices; i += 3)
		{
			a = (((vertexPTW4*)vertexArray)[(int)indexArray[i]]);
			b = (((vertexPTW4*)vertexArray)[(int)indexArray[i + 1]]);
			c = (((vertexPTW4*)vertexArray)[(int)indexArray[i + 2]]);

			va = D3DXVECTOR4(a.x, a.y, a.z, 1.0f);
			vb = D3DXVECTOR4(b.x, b.y, b.z, 1.0f);
			vc = D3DXVECTOR4(c.x, c.y, c.z, 1.0f);

			if (D3DXIntersectTri((D3DXVECTOR3*)&va, (D3DXVECTOR3*)&vb, (D3DXVECTOR3*)&vc, rayPos, rayDir, &uRes, &vRes, &tempRes))
			{
				if (victory == false)
				{
					victory = true;
					bestRes = tempRes;
				}
				else if (tempRes < bestRes)
				{
					bestRes = tempRes;
				}
			}
			//if (D3DXIntersectTri((D3DXVECTOR3*)&va, (D3DXVECTOR3*)&vb, (D3DXVECTOR3*)&vc, rayPos, rayDir, &uRes, &vRes, distRes))
			//	return true;
		}

		if (victory)
		{
			*distRes = bestRes;
			return true;
		}
		return false;
	}
};

struct strPair
{
public:
	std::string gin;
	std::string rpl;

	strPair() { }

	strPair(std::string ginN, std::string rplN)
	{
		gin = ginN;
		rpl = rplN;
	}
};

struct iOff
{
public:
	std::string name;
	int i;

	iOff() { }

	iOff(std::string nameN, int iN)
	{
		name = nameN;
		i = iN;
	}
};

short stoIndex(std::string str, std::vector<iOff>* iOffs)
{
	std::vector<std::string> data = split(str, "+");
	if (data.size() > 1)
	{
		short f;
		for (int i = iOffs->size() - 1; i >= 0; i--)
		{
			if (data[0] == iOffs->at(i).name)
			{
				f = iOffs->at(i).i;
				break;
			}
		}
		f += atoi(data[1].c_str());
		return f;
	}
	return atoi(str.c_str());
}

void autoGenNormals(void*, short*, int, int, DWORD, std::vector<vertexPC>*);

struct UNCRZ_font
{
public:
	std::string name;

	LPD3DXFONT font;

	UNCRZ_font();
	UNCRZ_font(std::string, LPD3DXFONT);
};

UNCRZ_font::UNCRZ_font()
{

}

UNCRZ_font::UNCRZ_font(std::string nameN, LPD3DXFONT fontN)
{
	name = nameN;
	font = fontN;
}

void createFont(LPDIRECT3DDEVICE9 dxDevice, int size, std::string name, LPD3DXFONT* dest, std::vector<UNCRZ_font*>* fontList)
{
	for (int i = fontList->size() - 1; i >= 0; i--)
	{
		if (fontList->at(i)->name == name)
		{
			*dest = fontList->at(i)->font;
			return;
		}
	}

	D3DXCreateFont(dxDevice, size, 0, FW_NORMAL, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Tahoma", dest);
	UNCRZ_font* font = new UNCRZ_font(name, *dest);
	fontList->push_back(font);
}

UNCRZ_effect createEffect(LPDIRECT3DDEVICE9 dxDevice, char* fileName, DWORD vertexType, std::string name, std::vector<UNCRZ_effect>* effectList)
{
	for (int i = effectList->size() - 1; i >= 0; i--)
	{
		if (effectList->at(i).name == name)
		{
			return effectList->at(i);
		}
	}

	UNCRZ_effect res = UNCRZ_effect::UNCRZ_effectFromFile(fileName, dxDevice, vertexType, name);
	effectList->push_back(res);
	return res;
}

void loadSpritesFromFile(char* fileName, LPDIRECT3DDEVICE9 dxDevice, std::vector<UNCRZ_sprite*>* spriteList, std::vector<UNCRZ_effect>* effectList, std::vector<UNCRZ_texture*>* textureList)
{
	UNCRZ_sprite* curSprite;

	std::ifstream file(fileName);
	if (file.is_open())
	{
		while (file.good())
		{
			std::string line;
			std::getline(file, line);
			int ci;
			if ((ci = line.find("//")) != -1)
			{
				line.erase(ci, line.length() - ci);
			}
			if (line == "")
				continue;

			line = trim(line);

			std::vector<std::string> data = split(line, " ");

			if (data.size() > 0)
			{
				if (data[0] == "end")
				{
					if (data[1] == "sprite")
					{
						spriteList->push_back(curSprite);
					}
				}
				else if (data[0] == "sprite")
				{
					curSprite = new UNCRZ_sprite(data[1]);
				}
				else if (data[0] == "texture")
				{
					createTexture(dxDevice, (char*)line.substr(8).c_str(), &curSprite->tex, textureList);
					curSprite->useTex = true;
				}
				else if (data[0] == "texture0")
				{
					createTexture(dxDevice, (char*)line.substr(8).c_str(), &curSprite->tex0, textureList);
					curSprite->useTex0 = true;
				}
				else if (data[0] == "texture1")
				{
					createTexture(dxDevice, (char*)line.substr(8).c_str(), &curSprite->tex1, textureList);
					curSprite->useTex1 = true;
				}
				else if (data[0] == "texture2")
				{
					createTexture(dxDevice, (char*)line.substr(8).c_str(), &curSprite->tex2, textureList);
					curSprite->useTex2 = true;
				}
				else if (data[0] == "texture3")
				{
					createTexture(dxDevice, (char*)line.substr(8).c_str(), &curSprite->tex3, textureList);
					curSprite->useTex3 = true;
				}
				else if (data[0] == "dim")
				{
					curSprite->dimX = stof(data[1]);
					curSprite->dimY = stof(data[2]);
					curSprite->dimZ = stof(data[3]);
				}
				else if (data[0] == "colmod")
				{
					curSprite->colMod = D3DXVECTOR4(stof(data[1]), stof(data[2]), stof(data[3]), stof(data[4])); 
				}
				else if (data[0] == "alpha")
				{
					if (data[1] == "none")
						curSprite->alphaMode = AM_none;
					else if (data[1] == "nice")
						curSprite->alphaMode = AM_nice;
					else if (data[1] == "add")
						curSprite->alphaMode = AM_add;
				}
				else if (data[0] == "sidewisealpha")
				{
					if (data[1] == "none")
						curSprite->sideWiseAlphaMode = AM_none;
					else if (data[1] == "nice")
						curSprite->sideWiseAlphaMode = AM_nice;
					else if (data[1] == "add")
						curSprite->sideWiseAlphaMode = AM_add;
				}
				else if (data[0] == "lighting")
				{
					if (data[1] == "none")
					{
						curSprite->lightingMode = LM_none;
					}
					else if (data[1] == "full")
					{
						curSprite->lightingMode = LM_full;
					}
				}
				else if (data[0] == "shader_dx9")
				{
					std::string effectName = line.substr(11);
					for (int i = effectList->size() - 1; i >= 0; i--)
					{
						if ((effectList->at(i).name == effectName))
						{
							curSprite->effect = effectList->at(i);
							continue;
						}
					}	
					//curSection->effect = UNCRZ_effect::UNCRZ_effectFromFile((char*)line.substr(11).c_str(), dxDevice, vertexType, line.substr(11)); 
					curSprite->effect = createEffect(dxDevice, (char*)line.substr(11).c_str(), VX_PCT, line.substr(11), effectList); 
				}
				else if (data[0] == "technique")
				{
					curSprite->tech = curSprite->effect.effect->GetTechniqueByName(data[1].c_str());
				}
				else if (data[0] == "technique_light")
				{
					curSprite->lightTech = curSprite->effect.effect->GetTechniqueByName(data[1].c_str());
				}
				else if (data[0] == "technique_over")
				{
					curSprite->overTech = curSprite->effect.effect->GetTechniqueByName(data[1].c_str());
				}
			}
		}
	}
}

struct lpTti
{
	std::string segName;
	int index;

	lpTti(std::string segNameN, int indexN)
	{
		segName = segNameN;
		index = indexN;
	}
};

int getSegTti(UNCRZ_model* model, std::string segOrBlendName)
{
	for (int i = model->allSegs.size() - 1; i >= 0; i--)
	{
		UNCRZ_segment* seg = model->allSegs[i];
		if (seg->name == segOrBlendName)
		{
			return seg->transIndex;
		}
		
		for (int i = seg->blends.size() - 1; i >= 0; i--)
		{
			if (seg->blends[i]->name == segOrBlendName)
			{
				return seg->blends[i]->transIndex;
			}
		}
	}
	return -1;
}

UNCRZ_segment* getSeg(UNCRZ_model* model, std::string segName)
{						
	for (int i = model->allSegs.size() - 1; i >= 0; i--)
	{
		if (model->allSegs[i]->name == segName)
		{
			return model->allSegs[i];
		}
	}
	return NULL;
}

UNCRZ_model* getModel(std::vector<UNCRZ_model*>* modelList, std::string modelName)
{
	for (int i = modelList->size() - 1; i >= 0; i--)
	{
		if (modelList->at(i)->name == modelName)
		{
			return modelList->at(i);
		}
	}
	return NULL;
}

UNCRZ_FBF_anim* getFBF_anim(std::vector<UNCRZ_FBF_anim*>* animList, std::string animName)
{
	for (int i = animList->size() - 1; i >= 0; i--)
	{
		if (animList->at(i)->name == animName)
		{
			return animList->at(i);
		}
	}
	return NULL;
}

void loadModelsFromFile(char* fileName, LPDIRECT3DDEVICE9 dxDevice, std::vector<UNCRZ_model*>* modelList, std::vector<UNCRZ_effect>* effectList, std::vector<UNCRZ_texture*>* textureList, std::vector<vertexPC>* nrmVis)
{
	std::vector<strPair> reps;
	std::vector<iOff> iOffs;

	UNCRZ_model* curModel;
	UNCRZ_segment* lastSegment = NULL; // if null, means curSeg should be added to the model
	UNCRZ_segment* curSegment;
	UNCRZ_section* curSection;

	std::vector<vertexPC> vPCs;
	std::vector<vertexPCT> vPCTs;
	std::vector<short> indices;

	std::vector<lpTti> latePlaceTtis;

	int curTti = -1;
	int nextTti = 0;
	bool manualNormals = false;
	bool subOffset = false;

	DWORD vertexType = VX_PC;

	std::ifstream file(fileName);
	if (file.is_open())
	{
		while (file.good())
		{
			std::string line;
			std::getline(file, line);
			int ci;
			if ((ci = line.find("//")) != -1)
			{
				line.erase(ci, line.length() - ci);
			}
			if (line == "")
				continue;

			line = trim(line);

			if (line.length() > 4 && line.substr(0, 4) == "rep ")
			{
			}
			else
			{
				for each(strPair sp in reps)
				{
					line = replace(line, sp.gin, sp.rpl);
				}
			}

			std::vector<std::string> data = split(line, " ");

			if (data.size() > 0)
			{
				if (data[0] == "end")
				{
					if (data[1] == "mdl")
					{
						if (vertexType == VX_PC)
						{
							for (int i = latePlaceTtis.size() - 1; i >= 0; i--)
							{
								lpTti *lpt = &latePlaceTtis[i];
								vPCs[lpt->index].tti = getSegTti(curModel, lpt->segName);
							}

							if (!manualNormals)
								autoGenNormals((void*)&vPCs.front(), (short*)&indices.front(), vPCs.size(), indices.size(), VX_PC, nrmVis);

							curModel->numVertices = (int)vPCs.size();
							curModel->createVBuff(dxDevice, (void*)&vPCs.front());
						}
						else if (vertexType == VX_PCT)
						{
							for (int i = latePlaceTtis.size() - 1; i >= 0; i--)
							{
								lpTti *lpt = &latePlaceTtis[i];
								vPCTs[lpt->index].tti = getSegTti(curModel, lpt->segName);
							}

							if (!manualNormals)
								autoGenNormals((void*)&vPCTs.front(), (short*)&indices.front(), vPCTs.size(), indices.size(), VX_PCT, nrmVis);

							curModel->numVertices = (int)vPCTs.size();
							curModel->createVBuff(dxDevice, (void*)&vPCTs.front());
						}
						curModel->numIndices = (int)indices.size();
						curModel->createIBuff(dxDevice, (short*)&indices.front());
						vPCs.clear();
						vPCTs.clear();
						indices.clear();
						latePlaceTtis.clear();
						curModel->transArr = new UNCRZ_trans_arr();
						curModel->transArr->create((int)curModel->allSegs.size() * 2);
						curModel->createSegmentBoxes();
						modelList->push_back(curModel);

						for each(UNCRZ_section* ss in curModel->sections)
						{
							ss->numVertices = curModel->numVertices;
						}

						iOffs.clear();
						lastSegment = NULL;
						curSegment = NULL;
						manualNormals = false;
						subOffset = false;
					}
					else if (data[1] == "blend")
					{
						curTti = curSegment->transIndex;
					}
					else if (data[1] == "seg")
					{
						lastSegment = curSegment->parent;
						curSegment = lastSegment;
						if (curSegment != NULL)
							curTti = curSegment->transIndex;
						else
							curTti = -1; // useful to check for vertices which arn't actually latePlaced
					}
					else if (data[1] == "sec")
					{
						curSection->vLen = ((int)indices.size() - curSection->vOffset) / 3;
					}
				}
				else if (data[0] == "mdl")
				{
					curModel = new UNCRZ_model(data[1]);
					nextTti = 0;
				}
				else if (data[0] == "blend")
				{
					curTti = nextTti;
					nextTti++;
					curSegment->addBlend(data[1], curTti, stof(data[2]));
				}
				else if (data[0] == "seg")
				{
					for each (UNCRZ_segment* ts in curModel->allSegs)
					{
						if (ts->name == data[1])
						{
							curSegment = ts;
							lastSegment = curSegment;
							curTti = curSegment->transIndex;
							goto oldSeg;
						}
					}

					curSegment = new UNCRZ_segment(data[1]);
					curTti = nextTti;
					nextTti++;
					curSegment->transIndex = curTti;
					if (lastSegment == NULL)
					{
						curModel->segments.push_back(curSegment);
						curSegment->parent = NULL;
						curSegment->model = curModel;
					}
					else
					{
						lastSegment->segments.push_back(curSegment);
						curSegment->parent = lastSegment;
						curSegment->model = curModel;
					}
					lastSegment = curSegment;
					curModel->allSegs.push_back(curSegment);
oldSeg:
					continue;
				}
				else if (data[0] == "sec")
				{
					curSection = new UNCRZ_section(data[1]);

					curSection->sectionEnabled = true;
					curSection->lightingMode = LM_full;

					curSection->drawDecals = true;
					curSection->acceptDecals = true;

					curSection->alphaMode = AM_none;

					curSection->vertexType = vertexType;
					curSection->vOffset = (int)indices.size();

					curModel->sections.push_back(curSection);
				}
				else if (data[0] == "shader_dx9")
				{
					std::string effectName = line.substr(11);
					for (int i = effectList->size() - 1; i >= 0; i--)
					{
						if ((effectList->at(i).name == effectName))
						{
							curSection->effect = effectList->at(i);
							continue;
						}
					}
					//curSection->effect = UNCRZ_effect::UNCRZ_effectFromFile((char*)line.substr(11).c_str(), dxDevice, vertexType, line.substr(11)); 
					curSection->effect = createEffect(dxDevice, (char*)line.substr(11).c_str(), vertexType, line.substr(11), effectList);
					//effectList->push_back(curSection->effect); // pretty sure we don't want this (done by createEffect these days)
				}
				else if (data[0] == "texture")
				{
					createTexture(dxDevice, (char*)line.substr(8).c_str(), &curSection->tex, textureList);
					curSection->useTex = true;
				}
				else if (data[0] == "texture0")
				{
					createTexture(dxDevice, (char*)line.substr(8).c_str(), &curSection->tex0, textureList);
					curSection->useTex0 = true;
				}
				else if (data[0] == "texture1")
				{
					createTexture(dxDevice, (char*)line.substr(8).c_str(), &curSection->tex1, textureList);
					curSection->useTex1 = true;
				}
				else if (data[0] == "texture2")
				{
					createTexture(dxDevice, (char*)line.substr(8).c_str(), &curSection->tex2, textureList);
					curSection->useTex2 = true;
				}
				else if (data[0] == "texture3")
				{
					createTexture(dxDevice, (char*)line.substr(8).c_str(), &curSection->tex3, textureList);
					curSection->useTex3 = true;
				}
				else if (data[0] == "colmod")
				{
					curSection->colMod = D3DXVECTOR4(stof(data[1]), stof(data[2]), stof(data[3]), stof(data[4])); 
				}
				else if (data[0] == "alpha")
				{
					if (data[1] == "none")
						curSection->alphaMode = AM_none;
					else if (data[1] == "nice")
						curSection->alphaMode = AM_nice;
					else if (data[1] == "add")
						curSection->alphaMode = AM_add;
				}
				else if (data[0] == "decals")
				{
					if (data[1] == "accept")
					{
						curSection->acceptDecals = true;
					}
					else if (data[1] == "noaccept")
					{
						curSection->acceptDecals = false;
					}
					else if (data[1] == "draw")
					{
						curSection->drawDecals = true;
					}
					else if (data[1] == "nodraw")
					{
						curSection->drawDecals = false;
					}
				}
				else if (data[0] == "lighting")
				{
					if (data[1] == "none")
					{
						curSection->lightingMode = LM_none;
					}
					else if (data[1] == "full")
					{
						curSection->lightingMode = LM_full;
					}
				}
				else if (data[0] == "manualnormals")
				{
					manualNormals = true;
				}
				else if (data[0] == "suboffset")
				{
					subOffset = true;
				}
				else if (data[0] == "v")
				{
					if (manualNormals)
					{
						if (vertexType == VX_PC)
							vPCs.push_back(vertexPC(stof(data[1]), stof(data[2]), stof(data[3]), stof(data[4]), stof(data[5]), stof(data[6]), stof(data[7]), stof(data[8]), stof(data[9]), curTti));
						else if (vertexType == VX_PCT)
							vPCTs.push_back(vertexPCT(vertexPC(stof(data[1]), stof(data[2]), stof(data[3]), stof(data[4]), stof(data[5]), stof(data[6]), stof(data[7]), stof(data[8]), stof(data[9]), curTti), stof(data[10]), stof(data[11])));
					}
					else
					{
						if (vertexType == VX_PC)
							vPCs.push_back(vertexPC(stof(data[1]), stof(data[2]), stof(data[3]), stof(data[4]), stof(data[5]), stof(data[6]), curTti));
						else if (vertexType == VX_PCT)
							vPCTs.push_back(vertexPCT(vertexPC(stof(data[1]), atof(data[2].c_str()), stof(data[3]), stof(data[4]), stof(data[5]), stof(data[6]), curTti), stof(data[7]), stof(data[8])));
					}
					if (subOffset)
					{
						if (vertexType == VX_PC)
						{
							vPCs[vPCs.size() - 1].x -= curSegment->offset.x;
							vPCs[vPCs.size() - 1].y -= curSegment->offset.y;
							vPCs[vPCs.size() - 1].z -= curSegment->offset.z;
						}
						else if (vertexType == VX_PCT)
						{
							vPCTs[vPCTs.size() - 1].x -= curSegment->offset.x;
							vPCTs[vPCTs.size() - 1].y -= curSegment->offset.y;
							vPCTs[vPCTs.size() - 1].z -= curSegment->offset.z;
						}
					}
				}
				else if (data[0] == "lpt")
				{
					int ttti = -1;
					if (vertexType == VX_PC)
					{
						if ((ttti = getSegTti(curModel, data[1])) != -1) // data[1] is segName
						{
							vPCs[vPCs.size() - 1].tti = ttti;
						}
						else
						{
							latePlaceTtis.push_back(lpTti(data[1], vPCs.size() - 1));
						}
					}
					else if (vertexType == VX_PCT)
					{
						if ((ttti = getSegTti(curModel, data[1])) != -1)
						{
							vPCTs[vPCTs.size() - 1].tti = ttti;
						}
						else
						{
							latePlaceTtis.push_back(lpTti(data[1], vPCTs.size() - 1));
						}
					}
				}
				else if (data[0] == "f")
				{
					indices.push_back(stoIndex(data[1], &iOffs));
					indices.push_back(stoIndex(data[2], &iOffs));
					indices.push_back(stoIndex(data[3], &iOffs));
				}
				else if (data[0] == "s")
				{
					int surfaceStart = stoi(data[1]);
					int colDim = stoi(data[2]);
					int numCols = stoi(data[3]);
					for (int i = 0; i < numCols - 1; i++)
					{
						for (int j = 0; j < colDim - 1; j++)
						{
							indices.push_back(surfaceStart + i * colDim + j);
							indices.push_back(surfaceStart + (i + 1) * colDim + j + 1);
							indices.push_back(surfaceStart + i * colDim + j + 1);
							
							indices.push_back(surfaceStart + i * colDim + j);
							indices.push_back(surfaceStart + (i + 1) * colDim + j);
							indices.push_back(surfaceStart + (i + 1) * colDim + j + 1);
						}
					}
				}
				else if (data[0] == "cs")
				{
					int surfaceStart = stoi(data[1]);
					int colDim = stoi(data[2]);
					int numCols = stoi(data[3]);
					for (int i = 0; i < numCols - 1; i++)
					{
						for (int j = 0; j < colDim - 1; j++)
						{
							indices.push_back(surfaceStart + i * colDim + j);
							indices.push_back(surfaceStart + (i + 1) * colDim + j + 1);
							indices.push_back(surfaceStart + i * colDim + j + 1);
							
							indices.push_back(surfaceStart + i * colDim + j);
							indices.push_back(surfaceStart + (i + 1) * colDim + j);
							indices.push_back(surfaceStart + (i + 1) * colDim + j + 1);
						}
						indices.push_back(surfaceStart + i * colDim + colDim - 1);
						indices.push_back(surfaceStart + (i + 1) * colDim);
						indices.push_back(surfaceStart + i * colDim);
					
						indices.push_back(surfaceStart + i * colDim + colDim - 1);
						indices.push_back(surfaceStart + (i + 1) * colDim + colDim - 1);
						indices.push_back(surfaceStart + (i + 1) * colDim);
					}
				}
				else if (data[0] == "offset")
				{
					curSegment->offset = D3DXVECTOR3(stof(data[1]), stof(data[2]), stof(data[3]));
				}
				else if (data[0] == "rotation")
				{
					curSegment->rotation = D3DXVECTOR3(stof(data[1]), stof(data[2]), stof(data[3]));
				}
				else if (data[0] == "technique")
				{
					curSection->tech = curSection->effect.effect->GetTechniqueByName(data[1].c_str());
				}
				else if (data[0] == "technique_light")
				{
					curSection->lightTech = curSection->effect.effect->GetTechniqueByName(data[1].c_str());
				}
				else if (data[0] == "technique_decal")
				{
					curSection->decalTech = curSection->effect.effect->GetTechniqueByName(data[1].c_str());
				}
				else if (data[0] == "technique_over")
				{
					curSection->overTech = curSection->effect.effect->GetTechniqueByName(data[1].c_str());
				}
				else if (data[0] == "vertex")
				{
					if (data[1] == "PC")
					{
						vertexType = VX_PC;
						curModel->vertexDec = vertexDecPC;
						curModel->stride = sizeof(vertexPC);
					}
					else if (data[1] == "PCT")
					{
						vertexType = VX_PCT;
						curModel->vertexDec = vertexDecPCT;
						curModel->stride = sizeof(vertexPCT);
					}
					curModel->vertexType = vertexType;
				}
				else if (data[0] == "rep")
				{
					// rem old
					for (int i = reps.size() - 1; i >= 0; i--)
					{
						if (reps[i].gin == data[1])
						{
							reps[i] = reps[reps.size() - 1];
							reps.pop_back();
						}
					}
					// add new
					reps.push_back(strPair(data[1], line.substr(5 + data[1].length())));
				}
				else if (data[0] == "ioff")
				{
					// rem old
					for (int i = iOffs.size() - 1; i >= 0; i--)
					{
						if (iOffs[i].name == data[1])
						{
							iOffs[i] = iOffs[iOffs.size() - 1];
							iOffs.pop_back();
						}
					}
					// add new
					if (vertexType == VX_PC)
						iOffs.push_back(iOff(data[1], (int)vPCs.size()));
					else if (vertexType == VX_PCT)
						iOffs.push_back(iOff(data[1], (int)vPCTs.size()));
				}
			}
		}
		file.close();
	}
}

void loadAnimsFromFile(char* fileName, std::vector<UNCRZ_FBF_anim*>* animList, std::vector<UNCRZ_model*>* modelList)
{
	std::vector<strPair> reps;

	UNCRZ_FBF_anim* curAnim;
	UNCRZ_model* curModel;

	std::string startMotion;
	int startDur;
	bool startSet = false;

	std::ifstream file(fileName);
	if (file.is_open())
	{
		while (file.good())
		{
			std::string line;
			std::getline(file, line);
			int ci;
			if ((ci = line.find("//")) != -1)
			{
				line.erase(ci, line.length() - ci);
			}
			if (line == "")
				continue;

			line = trim(line);

			for each(strPair sp in reps)
			{
				line = replace(line, sp.gin, sp.rpl);
			}

			std::vector<std::string> data = split(line, " ");

			if (data.size() > 0)
			{
				if (data[0] == "end")
				{
					if (data[1] == "anim")
					{
					}
					else if (data[1] == "flow")
					{
						if (startSet)
							curAnim->setFlowStart(startMotion, startDur);
						startSet = false;
					}
					else if (data[1] == "motion")
					{
						curAnim->endMotion();
					}
				}
				else if (data[0] == "anim")
				{
					curAnim = new UNCRZ_FBF_anim(data[1]);
					animList->push_back(curAnim);
				}
				else if (data[0] == "a")
				{
					curAnim->addLine(line.substr(2));
				}
				else if (data[0] == "noupdate")
				{
					curAnim->setMotionCausesUpdate(false);
				}
				else if (data[0] == "motion")
				{
					curAnim->addMotion(data[1]);
				}
				else if (data[0] == "flow")
				{
					curAnim->addFlow(data[1]);
				}
				else if (data[0] == "start")
				{
					startSet = true;
					startMotion = data[1];
					startDur = stoi(data[2]);
				}
				else if (data[0] == "duration")
				{
					curAnim->setMotionDuration(stoi(data[1]));
				}
				else if (data[0] == "model")
				{
					for (int i = modelList->size() - 1; i >= 0; i--)
					{
						if ((curModel = modelList->at(i))->name == data[1])
						{
							curAnim->model = curModel;
							curAnim->modelName = data[1];
							break;
						}
					}
				}
				else if (data[0] == "rep")
				{
					reps.push_back(strPair(data[1], line.substr(5 + data[1].length())));
				}
			}
		}
		file.close();
	}
}
void initVertexDec(LPDIRECT3DDEVICE9 dxDevice)
{
	D3DVERTEXELEMENT9 vecA[] = {
		{ 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 4 * sizeof(float), D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
		{ 0, 8 * sizeof(float), D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
		{ 0, 12 * sizeof(float), D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 }, // transArrIndex
		D3DDECL_END()
	};

	if (FAILED(dxDevice->CreateVertexDeclaration(vecA, &vertexDecPC)))
	{
		int i = 7;
		i++;
	}

	D3DVERTEXELEMENT9 vecB[] = {
		{ 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 4 * sizeof(float), D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
		{ 0, 8 * sizeof(float), D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
		{ 0, 12 * sizeof(float), D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		{ 0, 14 * sizeof(float), D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 }, // transArrIndex
		D3DDECL_END()
	};

	if (FAILED(dxDevice->CreateVertexDeclaration(vecB, &vertexDecPCT)))
	{
		int i = 7;
		i++;
	}

	D3DVERTEXELEMENT9 vecC[] = {
		{ 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 4 * sizeof(float), D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
		{ 0, 8 * sizeof(float), D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
		{ 0, 9 * sizeof(float), D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		{ 0, 13 * sizeof(float), D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 }, // transArrIndex
		D3DDECL_END()
	};

	if (FAILED(dxDevice->CreateVertexDeclaration(vecC, &vertexDecPAT4)))
	{
		int i = 7;
		i++;
	}

	D3DVERTEXELEMENT9 vecD[] = {
		{ 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 4 * sizeof(float), D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
		{ 0, 8 * sizeof(float), D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		{ 0, 10 * sizeof(float), D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 }, // weights
		D3DDECL_END()
	};

	if (FAILED(dxDevice->CreateVertexDeclaration(vecD, &vertexDecPTW4)))
	{
		int i = 7;
		i++;
	}

	D3DVERTEXELEMENT9 vecE[] = {
		{ 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 4 * sizeof(float), D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		D3DDECL_END()
	};

	if (FAILED(dxDevice->CreateVertexDeclaration(vecE, &vertexDecOver)))
	{
		int i = 7;
		i++;
	}
}

UNCRZ_decal* splatSquareDecalPCT(int remAge, vertexPCT* vertexArray, short* indexArray, int vOffset, int vLen, D3DXMATRIX* splatProj, D3DXVECTOR3* rayDir, float nrmCoof, UNCRZ_trans_arr* transArr)
{
	std::vector<vertexPAT4> resVertices;
	std::vector<int> resIndicies;

	vertexPCT a;
	vertexPCT b;
	vertexPCT c;
	D3DXVECTOR4 va;
	D3DXVECTOR4 vb;
	D3DXVECTOR4 vc;
	//D3DXVECTOR4 na;
	//D3DXVECTOR4 nb;
	//D3DXVECTOR4 nc;
	D3DXVECTOR3 nrm;
	D3DXVECTOR3 fnt;
	D3DXVECTOR3 bck;

	for (int i = vOffset; i < vOffset + vLen * 3; i += 3)
	{
		a = (((vertexPCT*)vertexArray)[(int)indexArray[i]]);
		b = (((vertexPCT*)vertexArray)[(int)indexArray[i + 1]]);
		c = (((vertexPCT*)vertexArray)[(int)indexArray[i + 2]]);

		//D3DXMATRIX trans;

		//va = D3DXVECTOR4(a.x, a.y, a.z, 1.0f);
		//D3DXMatrixMultiply(&trans, transArr->getValue((int)a.tti), splatProj);
		//D3DXVec4Transform(&va, &va, &trans);
		//vb = D3DXVECTOR4(b.x, b.y, b.z, 1.0f);
		//D3DXMatrixMultiply(&trans, transArr->getValue((int)b.tti), splatProj);
		//D3DXVec4Transform(&vb, &vb, &trans);
		//vc = D3DXVECTOR4(c.x, c.y, c.z, 1.0f);
		//D3DXMatrixMultiply(&trans, transArr->getValue((int)c.tti), splatProj);
		//D3DXVec4Transform(&vc, &vc, &trans);

		/*va = D3DXVECTOR4(a.x, a.y, a.z, 1.0f);
		na = D3DXVECTOR4(a.nx, a.ny, a.nz, 1.0f);
		//va += na;
		D3DXVec4Transform(&va, &va, transArr->getValue((int)a.tti));
		D3DXVec4Transform(&va, &va, splatProj);

		vb = D3DXVECTOR4(b.x, b.y, b.z, 1.0f);
		nb = D3DXVECTOR4(b.nx, b.ny, b.nz, 1.0f);
		//vb += nb;
		D3DXVec4Transform(&vb, &vb, transArr->getValue((int)b.tti));
		D3DXVec4Transform(&vb, &vb, splatProj);

		vc = D3DXVECTOR4(c.x, c.y, c.z, 1.0f);
		nc = D3DXVECTOR4(c.nx, c.ny, c.nz, 1.0f);
		//vc += nc;
		D3DXVec4Transform(&vc, &vc, transArr->getValue((int)c.tti));
		D3DXVec4Transform(&vc, &vc, splatProj);*/

		va = D3DXVECTOR4(a.x, a.y, a.z, 1.0f);
		vb = D3DXVECTOR4(b.x, b.y, b.z, 1.0f);
		vc = D3DXVECTOR4(c.x, c.y, c.z, 1.0f);

		D3DXVec4Transform(&vb, &vb, transArr->getValue((int)b.tti));
		D3DXVec4Transform(&va, &va, transArr->getValue((int)a.tti));
		D3DXVec4Transform(&vc, &vc, transArr->getValue((int)c.tti));

		// find normal
		fnt = D3DXVECTOR3(vb.x - va.x, vb.y - va.y, vb.z - va.z);
		bck = D3DXVECTOR3(vc.x - va.x, vc.y - va.y, vc.z - va.z);

		nrm.x = fnt.y * bck.z - bck.y * fnt.z;
		nrm.y = fnt.z * bck.x - bck.z * fnt.x;
		nrm.z = fnt.x * bck.y - bck.x * fnt.y;

		float mod = sqrtf(nrm.x * nrm.x + nrm.y * nrm.y + nrm.z * nrm.z);

		if (mod == 0)
			nrm = D3DXVECTOR3(0, 1, 0);
		else
		{
			nrm.x /= mod;
			nrm.y /= mod;
			nrm.z /= mod;
		}

		// dot/filter
		float aa = -D3DXVec3Dot(&nrm, rayDir);
		if (aa < 0.0)
			continue;
		aa = (1.0 - nrmCoof) + nrmCoof * aa;

		// splat proj
		D3DXVec4Transform(&va, &va, splatProj);
		D3DXVec4Transform(&vb, &vb, splatProj);
		D3DXVec4Transform(&vc, &vc, splatProj);

		// filter
		if (va.x > 1.0 && vb.x > 1.0 && vc.x > 1.0)
			continue;
		if (va.y > 1.0 && vb.y > 1.0 && vc.y > 1.0)
			continue;
		if (va.z > 1.0 && vb.z > 1.0 && vc.z > 1.0)
			continue;

		if (va.x < -1.0 && vb.x < -1.0 && vc.x < -1.0)
			continue;
		if (va.y < -1.0 && vb.y < -1.0 && vc.y < -1.0)
			continue;
		if (va.z < 0.0 && vb.z < 0.0 && vc.z < 0.0)
			continue;

		// put them all in! (x : -1, y : -1) => (u : 0, v : 0), (x : 1, y : 1) => (u : 1, v : 1)

		a.tu = va.x * 0.5 + 0.5;
		a.tv = va.y * 0.5 + 0.5;
		a.a = aa;

		b.tu = vb.x * 0.5 + 0.5;
		b.tv = vb.y * 0.5 + 0.5;
		b.a = aa;

		c.tu = vc.x * 0.5 + 0.5;
		c.tv = vc.y * 0.5 + 0.5;
		c.a = aa;

		resIndicies.push_back(indexArray[i]);
		resVertices.push_back(vertexPAT4(a, 0.0, 0.0));
		resIndicies.push_back(indexArray[i + 1]);
		resVertices.push_back(vertexPAT4(b, 0.0, 0.0));
		resIndicies.push_back(indexArray[i + 2]);
		resVertices.push_back(vertexPAT4(c, 0.0, 0.0));

		/*va = D3DXVECTOR4(a.x, a.y, a.z, 1.0f);
		na = D3DXVECTOR4(a.nx, a.ny, a.nz, 0.0f);
		na += va;
		D3DXVec4Transform(&va, &va, transArr->getValue((int)a.tti));
		D3DXVec4Transform(&na, &na, transArr->getValue((int)a.tti));
		na -= va;
		D3DXVec4Transform(&va, &va, splatProj);

		vb = D3DXVECTOR4(b.x, b.y, b.z, 1.0f);
		nb = D3DXVECTOR4(b.nx, b.ny, b.nz, 0.0f);
		nb += vb;
		D3DXVec4Transform(&vb, &vb, transArr->getValue((int)b.tti));
		D3DXVec4Transform(&nb, &nb, transArr->getValue((int)b.tti));
		nb -= vb;
		D3DXVec4Transform(&vb, &vb, splatProj);

		vc = D3DXVECTOR4(c.x, c.y, c.z, 1.0f);
		nc = D3DXVECTOR4(c.nx, c.ny, c.nz, 0.0f);
		nc += vc;
		D3DXVec4Transform(&vc, &vc, transArr->getValue((int)c.tti));
		D3DXVec4Transform(&nc, &nc, transArr->getValue((int)c.tti));
		nc -= vc;
		D3DXVec4Transform(&vc, &vc, splatProj);

		if (va.x > 1.0 && vb.x > 1.0 && vc.x > 1.0)
			continue;
		if (va.y > 1.0 && vb.y > 1.0 && vc.y > 1.0)
			continue;
		if (va.z > 1.0 && vb.z > 1.0 && vc.z > 1.0)
			continue;

		if (va.x < -1.0 && vb.x < -1.0 && vc.x < -1.0)
			continue;
		if (va.y < -1.0 && vb.y < -1.0 && vc.y < -1.0)
			continue;
		if (va.z < 0.0 && vb.z < 0.0 && vc.z < 0.0)
			continue;

		// put them all in! (x : -1, y : -1) => (u : 0, v : 0), (x : 1, y : 1) => (u : 1, v : 1)
		D3DXVECTOR3 nrm;

		nrm = D3DXVECTOR3(na.x + nb.x + nc.x, na.y + nb.y + nc.y, na.z + nb.z + nc.z);
		float nrmMod = sqrt(nrm.x * nrm.x + nrm.y * nrm.y + nrm.z * nrm.z);
		nrm.x /= nrmMod;
		nrm.y /= nrmMod;
		nrm.z /= nrmMod;
		float aa = -D3DXVec3Dot(&nrm, rayDir);
		if (aa < 0.0)
			continue;
		aa = (1.0 - nrmCoof) + nrmCoof * aa;

		a.tu = va.x * 0.5 + 0.5;
		a.tv = va.y * 0.5 + 0.5;
		a.a = aa;

		b.tu = vb.x * 0.5 + 0.5;
		b.tv = vb.y * 0.5 + 0.5;
		b.a = aa;

		c.tu = vc.x * 0.5 + 0.5;
		c.tv = vc.y * 0.5 + 0.5;
		c.a = aa;

		resVertices.push_back(vertexPAT4(a, 0.0, 0.0));
		resVertices.push_back(vertexPAT4(b, 0.0, 0.0));
		resVertices.push_back(vertexPAT4(c, 0.0, 0.0));*/


		/*D3DXVECTOR3 nrm;
		int backCount = 0;
		
		a.tu = va.x * 0.5 + 0.5;
		a.tv = va.y * 0.5 + 0.5;
		nrm = D3DXVECTOR3(a.nx, a.ny, a.nz);
		float aa = -D3DXVec3Dot(&nrm, rayDir);
		if (aa < 0.0)
		{
			aa = 0.0;
			backCount++;
		}
		a.a = aa;

		b.tu = vb.x * 0.5 + 0.5;
		b.tv = vb.y * 0.5 + 0.5;
		nrm = D3DXVECTOR3(b.nx, b.ny, b.nz);
		float ba = -D3DXVec3Dot(&nrm, rayDir);
		if (ba < 0.0)
		{
			ba = 0.0;
			backCount++;
		}
		b.a = ba;

		c.tu = vc.x * 0.5 + 0.5;
		c.tv = vc.y * 0.5 + 0.5;
		nrm = D3DXVECTOR3(c.nx, c.ny, c.nz);
		float ca = -D3DXVec3Dot(&nrm, rayDir);
		if (ca < 0.0)
		{
			ca = 0.0;
			backCount++;
		}
		c.a = ca;

		if (backCount < 3)
		{
			resVertices.push_back(a);
			resVertices.push_back(b);
			resVertices.push_back(c);
		}*/
	}

	if (resVertices.size() != 0)
		return new UNCRZ_decal(remAge, (vertexPAT4*)&resVertices.front(), (int*)&resIndicies.front(), (void*)vertexArray, VX_PCT, (int)resVertices.size() / 3);
	else
		return NULL;
}

void splatSquareDecal_Model(UNCRZ_model* mdl, LPDIRECT3DDEVICE9 dxDevice, int remAge, D3DXMATRIX* splatProj, D3DXVECTOR3* rayDir, float nrmCoof, char* textureFileName, std::vector<UNCRZ_texture*>* textures)
{
	LPDIRECT3DTEXTURE9 tex;

	createTexture(dxDevice, textureFileName, &tex, textures);

	for (int i = 0; i < mdl->sections.size(); i++)
	{
		UNCRZ_section* curSec = mdl->sections[i];

		if (curSec->acceptDecals == false)
			continue;

		if (mdl->vertexType == VX_PCT)
		{
			UNCRZ_decal* dec = splatSquareDecalPCT(remAge, (vertexPCT*)mdl->vertexArray, mdl->indexArray, curSec->vOffset, curSec->vLen, splatProj, rayDir, nrmCoof, mdl->transArr);
			if (dec != NULL)
			{
				dec->tex = tex;
				curSec->decals.push_back(dec);
			}
		}
	}
}

void simpleSplatSquareDecal_Model(UNCRZ_model* mdl, LPDIRECT3DDEVICE9 dxDevice, int remAge, D3DXVECTOR3* rayPos, D3DXVECTOR3* rayDir, float nrmCoof, float width, float height, float nearZ, float farZ, char* textureFileName, std::vector<UNCRZ_texture*>* textures, float rot, D3DXMATRIX* outSplatProj)
{
	D3DXVECTOR3 eyeVec = *rayPos;
	D3DXVECTOR3 targVec = eyeVec + *rayDir;
	D3DXVECTOR3 upVec(0, 1, 0);

	if (rayDir->x == 0 && rayDir->z == 0)
		upVec = D3DXVECTOR3(1, 0, 0);

	D3DXMATRIX rotM;
	D3DXMatrixRotationAxis(&rotM, rayDir, rot);
	D3DXVec3TransformNormal(&upVec, &upVec, &rotM);

	D3DXMATRIX viewM;
	D3DXMATRIX projM;
	D3DXMATRIX splatProj;

	D3DXMatrixLookAtLH(&viewM, &eyeVec, &targVec, &upVec);
	D3DXMatrixOrthoLH(&projM, width, height, nearZ, farZ);

	D3DXMatrixMultiply(&splatProj, &viewM, &projM);

	splatSquareDecal_Model(mdl, dxDevice, remAge, &splatProj, rayDir, nrmCoof, textureFileName, textures);

	*outSplatProj = splatProj;
}

UNCRZ_decal* splatSquareDecalPTW4(int remAge, vertexPTW4* vertexArray, short* indexArray, int vOffset, int vLen, D3DXMATRIX* splatProj, D3DXVECTOR3* rayDir, float nrmCoof)
{
	std::vector<vertexPAT4> resVertices;
	std::vector<int> resIndicies;

	vertexPTW4 a;
	vertexPTW4 b;
	vertexPTW4 c;
	D3DXVECTOR4 va;
	D3DXVECTOR4 vb;
	D3DXVECTOR4 vc;

	for (int i = vOffset; i < vOffset + vLen * 3; i += 3)
	{
		a = (((vertexPTW4*)vertexArray)[(int)indexArray[i]]);
		b = (((vertexPTW4*)vertexArray)[(int)indexArray[i + 1]]);
		c = (((vertexPTW4*)vertexArray)[(int)indexArray[i + 2]]);

		//D3DXMATRIX trans;

		//va = D3DXVECTOR4(a.x, a.y, a.z, 1.0f);
		//D3DXMatrixMultiply(&trans, transArr->getValue((int)a.tti), splatProj);
		//D3DXVec4Transform(&va, &va, &trans);
		//vb = D3DXVECTOR4(b.x, b.y, b.z, 1.0f);
		//D3DXMatrixMultiply(&trans, transArr->getValue((int)b.tti), splatProj);
		//D3DXVec4Transform(&vb, &vb, &trans);
		//vc = D3DXVECTOR4(c.x, c.y, c.z, 1.0f);
		//D3DXMatrixMultiply(&trans, transArr->getValue((int)c.tti), splatProj);
		//D3DXVec4Transform(&vc, &vc, &trans);

		va = D3DXVECTOR4(a.x, a.y, a.z, 1.0f);
		D3DXVec4Transform(&va, &va, splatProj);
		vb = D3DXVECTOR4(b.x, b.y, b.z, 1.0f);
		D3DXVec4Transform(&vb, &vb, splatProj);
		vc = D3DXVECTOR4(c.x, c.y, c.z, 1.0f);
		D3DXVec4Transform(&vc, &vc, splatProj);

		if (va.x > 1.0 && vb.x > 1.0 && vc.x > 1.0)
			continue;
		if (va.y > 1.0 && vb.y > 1.0 && vc.y > 1.0)
			continue;
		if (va.z > 1.0 && vb.z > 1.0 && vc.z > 1.0)
			continue;

		if (va.x < -1.0 && vb.x < -1.0 && vc.x < -1.0)
			continue;
		if (va.y < -1.0 && vb.y < -1.0 && vc.y < -1.0)
			continue;
		if (va.z < 0.0 && vb.z < 0.0 && vc.z < 0.0)
			continue;

		// put them all in! (x : -1, y : -1) => (u : 0, v : 0), (x : 1, y : 1) => (u : 1, v : 1)
				D3DXVECTOR3 nrm;

		nrm = D3DXVECTOR3((a.nx + b.nx + c.nx) / 3.0, (a.ny + b.ny + c.ny) / 3.0, (a.nz + b.nz + c.nz) / 3.0);
		float aa = -D3DXVec3Dot(&nrm, rayDir);
		if (aa < 0.0)
			continue;
		aa = (1.0 - nrmCoof) + nrmCoof * aa;

		a.tu = va.x * 0.5 + 0.5;
		a.tv = va.y * 0.5 + 0.5;

		b.tu = vb.x * 0.5 + 0.5;
		b.tv = vb.y * 0.5 + 0.5;

		c.tu = vc.x * 0.5 + 0.5;
		c.tv = vc.y * 0.5 + 0.5;

		resIndicies.push_back(indexArray[i]);
		resVertices.push_back(vertexPAT4(a, aa, a.tu, a.tv, va.z, 0.0));
		resIndicies.push_back(indexArray[i + 1]);
		resVertices.push_back(vertexPAT4(b, aa, b.tu, b.tv, vb.z, 0.0));
		resIndicies.push_back(indexArray[i + 2]);
		resVertices.push_back(vertexPAT4(c, aa, c.tu, c.tv, vc.z, 0.0));
		
		/*D3DXVECTOR3 nrm;
		int backCount = 0;

		a.tu = va.x * 0.5 + 0.5;
		a.tv = va.y * 0.5 + 0.5;
		nrm = D3DXVECTOR3(a.nx, a.ny, a.nz);
		float aa = -D3DXVec3Dot(&nrm, rayDir);
		if (aa < 0.0)
		{
			aa = 0.0;
			backCount++;
		}

		b.tu = vb.x * 0.5 + 0.5;
		b.tv = vb.y * 0.5 + 0.5;
		nrm = D3DXVECTOR3(b.nx, b.ny, b.nz);
		float ba = -D3DXVec3Dot(&nrm, rayDir);
		if (ba < 0.0)
		{
			ba = 0.0;
			backCount++;
		}

		c.tu = vc.x * 0.5 + 0.5;
		c.tv = vc.y * 0.5 + 0.5;
		nrm = D3DXVECTOR3(c.nx, c.ny, c.nz);
		float ca = -D3DXVec3Dot(&nrm, rayDir);
		if (ca < 0.0)
		{
			ca = 0.0;
			backCount++;
		}

		if (backCount < 3)
		{
			resVertices.push_back(a.quickPCT(aa, 1.0, 1.0, 1.0));
			resVertices.push_back(b.quickPCT(ba, 1.0, 1.0, 1.0));
			resVertices.push_back(c.quickPCT(ca, 1.0, 1.0, 1.0));
		}*/
	}

	if (resVertices.size() != 0)
		return new UNCRZ_decal(remAge, (vertexPAT4*)&resVertices.front(), (int*)&resIndicies.front(), (void*)vertexArray, VX_PTW4, (int)resVertices.size() / 3);
	else
		return NULL;
}

void splatSquareDecal_Terrain(UNCRZ_terrain* trn, LPDIRECT3DDEVICE9 dxDevice, int remAge, D3DXMATRIX* splatProj, D3DXVECTOR3* rayDir, float nrmCoof, char* textureFileName, std::vector<UNCRZ_texture*>* textures)
{
	if (trn->vertexType == VX_PTW4)
	{
		UNCRZ_decal* dec = splatSquareDecalPTW4(remAge, (vertexPTW4*)trn->vertexArray, trn->indexArray, 0, trn->numIndices / 3, splatProj, rayDir, nrmCoof);
		if (dec != NULL)
		{
			createTexture(dxDevice, textureFileName, &dec->tex, textures);
			trn->decals.push_back(dec);
		}
	}
}

void simpleSplatSquareDecal_Terrain(UNCRZ_terrain* trn, LPDIRECT3DDEVICE9 dxDevice, int remAge, D3DXVECTOR3* rayPos, D3DXVECTOR3* rayDir, float nrmCoof, float width, float height, float nearZ, float farZ, char* textureFileName, std::vector<UNCRZ_texture*>* textures, float rot, D3DXMATRIX* outSplatProj)
{
	D3DXVECTOR3 eyeVec = *rayPos;
	D3DXVECTOR3 targVec = eyeVec + *rayDir;
	D3DXVECTOR3 upVec(0, 1, 0);

	if (rayDir->x == 0 && rayDir->z == 0)
		upVec = D3DXVECTOR3(1, 0, 0);

	D3DXMATRIX rotM;
	D3DXMatrixRotationAxis(&rotM, rayDir, rot);
	D3DXVec3TransformNormal(&upVec, &upVec, &rotM);

	D3DXMATRIX viewM;
	D3DXMATRIX projM;
	D3DXMATRIX splatProj;

	D3DXMatrixLookAtLH(&viewM, &eyeVec, &targVec, &upVec);
	D3DXMatrixOrthoLH(&projM, width, height, nearZ, farZ);

	D3DXMatrixMultiply(&splatProj, &viewM, &projM);

	splatSquareDecal_Terrain(trn, dxDevice, remAge, &splatProj, rayDir, nrmCoof, textureFileName, textures);

	*outSplatProj = splatProj;
}

UNCRZ_decal* splatTriangleDecalPCT(int remAge, vertexPCT* vertexArray, short* indexArray, int vOffset, int vLen, D3DXMATRIX* splatProj, D3DXVECTOR3* rayDir, float nrmCoof, UNCRZ_trans_arr* transArr)
{
	std::vector<vertexPAT4> resVertices;
	std::vector<int> resIndicies;

	vertexPCT a;
	vertexPCT b;
	vertexPCT c;
	D3DXVECTOR4 va;
	D3DXVECTOR4 vb;
	D3DXVECTOR4 vc;
	//D3DXVECTOR4 na;
	//D3DXVECTOR4 nb;
	//D3DXVECTOR4 nc;
	D3DXVECTOR3 nrm;
	D3DXVECTOR3 fnt;
	D3DXVECTOR3 bck;

	for (int i = vOffset; i < vOffset + vLen * 3; i += 3)
	{
		a = (((vertexPCT*)vertexArray)[(int)indexArray[i]]);
		b = (((vertexPCT*)vertexArray)[(int)indexArray[i + 1]]);
		c = (((vertexPCT*)vertexArray)[(int)indexArray[i + 2]]);

		//D3DXMATRIX trans;

		//va = D3DXVECTOR4(a.x, a.y, a.z, 1.0f);
		//D3DXMatrixMultiply(&trans, transArr->getValue((int)a.tti), splatProj);
		//D3DXVec4Transform(&va, &va, &trans);
		//vb = D3DXVECTOR4(b.x, b.y, b.z, 1.0f);
		//D3DXMatrixMultiply(&trans, transArr->getValue((int)b.tti), splatProj);
		//D3DXVec4Transform(&vb, &vb, &trans);
		//vc = D3DXVECTOR4(c.x, c.y, c.z, 1.0f);
		//D3DXMatrixMultiply(&trans, transArr->getValue((int)c.tti), splatProj);
		//D3DXVec4Transform(&vc, &vc, &trans);

		/*va = D3DXVECTOR4(a.x, a.y, a.z, 1.0f);
		na = D3DXVECTOR4(a.nx, a.ny, a.nz, 1.0f);
		//va += na;
		D3DXVec4Transform(&va, &va, transArr->getValue((int)a.tti));
		D3DXVec4Transform(&va, &va, splatProj);

		vb = D3DXVECTOR4(b.x, b.y, b.z, 1.0f);
		nb = D3DXVECTOR4(b.nx, b.ny, b.nz, 1.0f);
		//vb += nb;
		D3DXVec4Transform(&vb, &vb, transArr->getValue((int)b.tti));
		D3DXVec4Transform(&vb, &vb, splatProj);

		vc = D3DXVECTOR4(c.x, c.y, c.z, 1.0f);
		nc = D3DXVECTOR4(c.nx, c.ny, c.nz, 1.0f);
		//vc += nc;
		D3DXVec4Transform(&vc, &vc, transArr->getValue((int)c.tti));
		D3DXVec4Transform(&vc, &vc, splatProj);*/

		va = D3DXVECTOR4(a.x, a.y, a.z, 1.0f);
		vb = D3DXVECTOR4(b.x, b.y, b.z, 1.0f);
		vc = D3DXVECTOR4(c.x, c.y, c.z, 1.0f);

		D3DXVec4Transform(&vb, &vb, transArr->getValue((int)b.tti));
		D3DXVec4Transform(&va, &va, transArr->getValue((int)a.tti));
		D3DXVec4Transform(&vc, &vc, transArr->getValue((int)c.tti));

		// find normal
		fnt = D3DXVECTOR3(vb.x - va.x, vb.y - va.y, vb.z - va.z);
		bck = D3DXVECTOR3(vc.x - va.x, vc.y - va.y, vc.z - va.z);

		nrm.x = fnt.y * bck.z - bck.y * fnt.z;
		nrm.y = fnt.z * bck.x - bck.z * fnt.x;
		nrm.z = fnt.x * bck.y - bck.x * fnt.y;

		float mod = sqrtf(nrm.x * nrm.x + nrm.y * nrm.y + nrm.z * nrm.z);

		if (mod == 0)
			nrm = D3DXVECTOR3(0, 1, 0);
		else
		{
			nrm.x /= mod;
			nrm.y /= mod;
			nrm.z /= mod;
		}

		// dot/filter
		float aa = -D3DXVec3Dot(&nrm, rayDir);
		if (aa < 0.0)
			continue;
		aa = (1.0 - nrmCoof) + nrmCoof * aa;

		// splat proj
		D3DXVec4Transform(&va, &va, splatProj);
		D3DXVec4Transform(&vb, &vb, splatProj);
		D3DXVec4Transform(&vc, &vc, splatProj);

		// filter
		if (va.x > 1.0 && vb.x > 1.0 && vc.x > 1.0)
			continue;
		if (va.y > 1.0 && vb.y > 1.0 && vc.y > 1.0)
			continue;
		if (va.z > 1.0 && vb.z > 1.0 && vc.z > 1.0)
			continue;

		if (va.x < -1.0 && vb.x < -1.0 && vc.x < -1.0)
			continue;
		if (va.y < -1.0 && vb.y < -1.0 && vc.y < -1.0)
			continue;
		if (va.z < 0.0 && vb.z < 0.0 && vc.z < 0.0)
			continue;

		// put them all in! (x : -1, y : -1) => (u : 0, v : 0), (x : 1, y : 1) => (u : 1, v : 1)

		a.tu = va.x * 0.5 + 0.5;
		a.tv = va.y * 0.5 + 0.5;
		a.a = aa;

		b.tu = vb.x * 0.5 + 0.5;
		b.tv = vb.y * 0.5 + 0.5;
		b.a = aa;

		c.tu = vc.x * 0.5 + 0.5;
		c.tv = vc.y * 0.5 + 0.5;
		c.a = aa;

		resIndicies.push_back(indexArray[i]);
		resVertices.push_back(vertexPAT4(a, 0.0, 0.0));
		resIndicies.push_back(indexArray[i + 1]);
		resVertices.push_back(vertexPAT4(b, 0.0, 0.0));
		resIndicies.push_back(indexArray[i + 2]);
		resVertices.push_back(vertexPAT4(c, 0.0, 0.0));

		/*va = D3DXVECTOR4(a.x, a.y, a.z, 1.0f);
		na = D3DXVECTOR4(a.nx, a.ny, a.nz, 0.0f);
		na += va;
		D3DXVec4Transform(&va, &va, transArr->getValue((int)a.tti));
		D3DXVec4Transform(&na, &na, transArr->getValue((int)a.tti));
		na -= va;
		D3DXVec4Transform(&va, &va, splatProj);

		vb = D3DXVECTOR4(b.x, b.y, b.z, 1.0f);
		nb = D3DXVECTOR4(b.nx, b.ny, b.nz, 0.0f);
		nb += vb;
		D3DXVec4Transform(&vb, &vb, transArr->getValue((int)b.tti));
		D3DXVec4Transform(&nb, &nb, transArr->getValue((int)b.tti));
		nb -= vb;
		D3DXVec4Transform(&vb, &vb, splatProj);

		vc = D3DXVECTOR4(c.x, c.y, c.z, 1.0f);
		nc = D3DXVECTOR4(c.nx, c.ny, c.nz, 0.0f);
		nc += vc;
		D3DXVec4Transform(&vc, &vc, transArr->getValue((int)c.tti));
		D3DXVec4Transform(&nc, &nc, transArr->getValue((int)c.tti));
		nc -= vc;
		D3DXVec4Transform(&vc, &vc, splatProj);

		if (va.x > 1.0 && vb.x > 1.0 && vc.x > 1.0)
			continue;
		if (va.y > 1.0 && vb.y > 1.0 && vc.y > 1.0)
			continue;
		if (va.z > 1.0 && vb.z > 1.0 && vc.z > 1.0)
			continue;

		if (va.x < -1.0 && vb.x < -1.0 && vc.x < -1.0)
			continue;
		if (va.y < -1.0 && vb.y < -1.0 && vc.y < -1.0)
			continue;
		if (va.z < 0.0 && vb.z < 0.0 && vc.z < 0.0)
			continue;

		// put them all in! (x : -1, y : -1) => (u : 0, v : 0), (x : 1, y : 1) => (u : 1, v : 1)
		D3DXVECTOR3 nrm;

		nrm = D3DXVECTOR3(na.x + nb.x + nc.x, na.y + nb.y + nc.y, na.z + nb.z + nc.z);
		float nrmMod = sqrt(nrm.x * nrm.x + nrm.y * nrm.y + nrm.z * nrm.z);
		nrm.x /= nrmMod;
		nrm.y /= nrmMod;
		nrm.z /= nrmMod;
		float aa = -D3DXVec3Dot(&nrm, rayDir);
		if (aa < 0.0)
			continue;
		aa = (1.0 - nrmCoof) + nrmCoof * aa;

		a.tu = va.x * 0.5 + 0.5;
		a.tv = va.y * 0.5 + 0.5;
		a.a = aa;

		b.tu = vb.x * 0.5 + 0.5;
		b.tv = vb.y * 0.5 + 0.5;
		b.a = aa;

		c.tu = vc.x * 0.5 + 0.5;
		c.tv = vc.y * 0.5 + 0.5;
		c.a = aa;

		resVertices.push_back(vertexPAT4(a, 0.0, 0.0));
		resVertices.push_back(vertexPAT4(b, 0.0, 0.0));
		resVertices.push_back(vertexPAT4(c, 0.0, 0.0));*/


		/*D3DXVECTOR3 nrm;
		int backCount = 0;
		
		a.tu = va.x * 0.5 + 0.5;
		a.tv = va.y * 0.5 + 0.5;
		nrm = D3DXVECTOR3(a.nx, a.ny, a.nz);
		float aa = -D3DXVec3Dot(&nrm, rayDir);
		if (aa < 0.0)
		{
			aa = 0.0;
			backCount++;
		}
		a.a = aa;

		b.tu = vb.x * 0.5 + 0.5;
		b.tv = vb.y * 0.5 + 0.5;
		nrm = D3DXVECTOR3(b.nx, b.ny, b.nz);
		float ba = -D3DXVec3Dot(&nrm, rayDir);
		if (ba < 0.0)
		{
			ba = 0.0;
			backCount++;
		}
		b.a = ba;

		c.tu = vc.x * 0.5 + 0.5;
		c.tv = vc.y * 0.5 + 0.5;
		nrm = D3DXVECTOR3(c.nx, c.ny, c.nz);
		float ca = -D3DXVec3Dot(&nrm, rayDir);
		if (ca < 0.0)
		{
			ca = 0.0;
			backCount++;
		}
		c.a = ca;

		if (backCount < 3)
		{
			resVertices.push_back(a);
			resVertices.push_back(b);
			resVertices.push_back(c);
		}*/
	}

	if (resVertices.size() != 0)
		return new UNCRZ_decal(remAge, (vertexPAT4*)&resVertices.front(), (int*)&resIndicies.front(), (void*)vertexArray, VX_PCT, (int)resVertices.size() / 3);
	else
		return NULL;
}

void splatTriangleDecal_Model(UNCRZ_model* mdl, LPDIRECT3DDEVICE9 dxDevice, int remAge, D3DXMATRIX* splatProj, D3DXVECTOR3* rayDir, float nrmCoof, char* textureFileName, std::vector<UNCRZ_texture*>* textures)
{
	LPDIRECT3DTEXTURE9 tex;

	createTexture(dxDevice, textureFileName, &tex, textures);

	for (int i = 0; i < mdl->sections.size(); i++)
	{
		UNCRZ_section* curSec = mdl->sections[i];

		if (curSec->acceptDecals == false)
			continue;

		if (mdl->vertexType == VX_PCT)
		{
			UNCRZ_decal* dec = splatTriangleDecalPCT(remAge, (vertexPCT*)mdl->vertexArray, mdl->indexArray, curSec->vOffset, curSec->vLen, splatProj, rayDir, nrmCoof, mdl->transArr);
			if (dec != NULL)
			{
				dec->tex = tex;
				curSec->decals.push_back(dec);
			}
		}
	}
}

void simpleSplatTriangleDecal_Model(UNCRZ_model* mdl, LPDIRECT3DDEVICE9 dxDevice, int remAge, D3DXVECTOR3* rayPos, D3DXVECTOR3* rayDir, float nrmCoof, float width, float height, float nearZ, float farZ, char* textureFileName, std::vector<UNCRZ_texture*>* textures, float rot, D3DXMATRIX* outSplatProj)
{
	D3DXVECTOR3 eyeVec = *rayPos;
	D3DXVECTOR3 targVec = eyeVec + *rayDir;
	D3DXVECTOR3 upVec(0, 1, 0);

	if (rayDir->x == 0 && rayDir->z == 0)
		upVec = D3DXVECTOR3(1, 0, 0);

	D3DXMATRIX rotM;
	D3DXMatrixRotationAxis(&rotM, rayDir, rot);
	D3DXVec3TransformNormal(&upVec, &upVec, &rotM);

	D3DXMATRIX viewM;
	D3DXMATRIX projM;
	D3DXMATRIX splatProj;

	D3DXMatrixLookAtLH(&viewM, &eyeVec, &targVec, &upVec);
	D3DXMatrixOrthoLH(&projM, width, height, nearZ, farZ);

	D3DXMatrixMultiply(&splatProj, &viewM, &projM);

	splatTriangleDecal_Model(mdl, dxDevice, remAge, &splatProj, rayDir, nrmCoof, textureFileName, textures);

	*outSplatProj = splatProj;
}

UNCRZ_decal* splatTriangleDecalPTW4(int remAge, vertexPTW4* vertexArray, short* indexArray, int vOffset, int vLen, D3DXMATRIX* splatProj, D3DXVECTOR3* rayDir, float nrmCoof)
{
	std::vector<vertexPAT4> resVertices;
	std::vector<int> resIndicies;

	vertexPTW4 a;
	vertexPTW4 b;
	vertexPTW4 c;
	D3DXVECTOR4 va;
	D3DXVECTOR4 vb;
	D3DXVECTOR4 vc;

	for (int i = vOffset; i < vOffset + vLen * 3; i += 3)
	{
		a = (((vertexPTW4*)vertexArray)[(int)indexArray[i]]);
		b = (((vertexPTW4*)vertexArray)[(int)indexArray[i + 1]]);
		c = (((vertexPTW4*)vertexArray)[(int)indexArray[i + 2]]);

		//D3DXMATRIX trans;

		//va = D3DXVECTOR4(a.x, a.y, a.z, 1.0f);
		//D3DXMatrixMultiply(&trans, transArr->getValue((int)a.tti), splatProj);
		//D3DXVec4Transform(&va, &va, &trans);
		//vb = D3DXVECTOR4(b.x, b.y, b.z, 1.0f);
		//D3DXMatrixMultiply(&trans, transArr->getValue((int)b.tti), splatProj);
		//D3DXVec4Transform(&vb, &vb, &trans);
		//vc = D3DXVECTOR4(c.x, c.y, c.z, 1.0f);
		//D3DXMatrixMultiply(&trans, transArr->getValue((int)c.tti), splatProj);
		//D3DXVec4Transform(&vc, &vc, &trans);

		va = D3DXVECTOR4(a.x, a.y, a.z, 1.0f);
		D3DXVec4Transform(&va, &va, splatProj);
		vb = D3DXVECTOR4(b.x, b.y, b.z, 1.0f);
		D3DXVec4Transform(&vb, &vb, splatProj);
		vc = D3DXVECTOR4(c.x, c.y, c.z, 1.0f);
		D3DXVec4Transform(&vc, &vc, splatProj);

		if (va.x > 1.0 && vb.x > 1.0 && vc.x > 1.0)
			continue;
		if (va.y > 1.0 && vb.y > 1.0 && vc.y > 1.0)
			continue;
		if (va.z > 1.0 && vb.z > 1.0 && vc.z > 1.0)
			continue;

		if (va.x < -1.0 && vb.x < -1.0 && vc.x < -1.0)
			continue;
		if (va.y < -1.0 && vb.y < -1.0 && vc.y < -1.0)
			continue;
		if (va.z < 0.0 && vb.z < 0.0 && vc.z < 0.0)
			continue;

		// put them all in! (x : -1, y : -1) => (u : 0, v : 0), (x : 1, y : 1) => (u : 1, v : 1)
				D3DXVECTOR3 nrm;

		nrm = D3DXVECTOR3((a.nx + b.nx + c.nx) / 3.0, (a.ny + b.ny + c.ny) / 3.0, (a.nz + b.nz + c.nz) / 3.0);
		float aa = -D3DXVec3Dot(&nrm, rayDir);
		if (aa < 0.0)
			continue;
		aa = (1.0 - nrmCoof) + nrmCoof * aa;

		a.tu = va.x * 0.5 + 0.5;
		a.tv = va.y * 0.5 + 0.5;

		b.tu = vb.x * 0.5 + 0.5;
		b.tv = vb.y * 0.5 + 0.5;

		c.tu = vc.x * 0.5 + 0.5;
		c.tv = vc.y * 0.5 + 0.5;

		resIndicies.push_back(indexArray[i]);
		resVertices.push_back(vertexPAT4(a, aa, a.tu, a.tv, va.z, 0.0));
		resIndicies.push_back(indexArray[i + 1]);
		resVertices.push_back(vertexPAT4(b, aa, b.tu, b.tv, vb.z, 0.0));
		resIndicies.push_back(indexArray[i + 2]);
		resVertices.push_back(vertexPAT4(c, aa, c.tu, c.tv, vc.z, 0.0));
		
		/*D3DXVECTOR3 nrm;
		int backCount = 0;

		a.tu = va.x * 0.5 + 0.5;
		a.tv = va.y * 0.5 + 0.5;
		nrm = D3DXVECTOR3(a.nx, a.ny, a.nz);
		float aa = -D3DXVec3Dot(&nrm, rayDir);
		if (aa < 0.0)
		{
			aa = 0.0;
			backCount++;
		}

		b.tu = vb.x * 0.5 + 0.5;
		b.tv = vb.y * 0.5 + 0.5;
		nrm = D3DXVECTOR3(b.nx, b.ny, b.nz);
		float ba = -D3DXVec3Dot(&nrm, rayDir);
		if (ba < 0.0)
		{
			ba = 0.0;
			backCount++;
		}

		c.tu = vc.x * 0.5 + 0.5;
		c.tv = vc.y * 0.5 + 0.5;
		nrm = D3DXVECTOR3(c.nx, c.ny, c.nz);
		float ca = -D3DXVec3Dot(&nrm, rayDir);
		if (ca < 0.0)
		{
			ca = 0.0;
			backCount++;
		}

		if (backCount < 3)
		{
			resVertices.push_back(a.quickPCT(aa, 1.0, 1.0, 1.0));
			resVertices.push_back(b.quickPCT(ba, 1.0, 1.0, 1.0));
			resVertices.push_back(c.quickPCT(ca, 1.0, 1.0, 1.0));
		}*/
	}

	if (resVertices.size() != 0)
		return new UNCRZ_decal(remAge, (vertexPAT4*)&resVertices.front(), (int*)&resIndicies.front(), (void*)vertexArray, VX_PTW4, (int)resVertices.size() / 3);
	else
		return NULL;
}

void splatTriangleDecal_Terrain(UNCRZ_terrain* trn, LPDIRECT3DDEVICE9 dxDevice, int remAge, D3DXMATRIX* splatProj, D3DXVECTOR3* rayDir, float nrmCoof, char* textureFileName, std::vector<UNCRZ_texture*>* textures)
{
	if (trn->vertexType == VX_PTW4)
	{
		UNCRZ_decal* dec = splatTriangleDecalPTW4(remAge, (vertexPTW4*)trn->vertexArray, trn->indexArray, 0, trn->numIndices / 3, splatProj, rayDir, nrmCoof);
		if (dec != NULL)
		{
			createTexture(dxDevice, textureFileName, &dec->tex, textures);
			trn->decals.push_back(dec);
		}
	}
}

void simpleSplatTriangleDecal_Terrain(UNCRZ_terrain* trn, LPDIRECT3DDEVICE9 dxDevice, int remAge, D3DXVECTOR3* rayPos, D3DXVECTOR3* rayDir, float nrmCoof, float width, float height, float nearZ, float farZ, char* textureFileName, std::vector<UNCRZ_texture*>* textures, float rot, D3DXMATRIX* outSplatProj)
{
	D3DXVECTOR3 eyeVec = *rayPos;
	D3DXVECTOR3 targVec = eyeVec + *rayDir;
	D3DXVECTOR3 upVec(0, 1, 0);

	if (rayDir->x == 0 && rayDir->z == 0)
		upVec = D3DXVECTOR3(1, 0, 0);

	D3DXMATRIX rotM;
	D3DXMatrixRotationAxis(&rotM, rayDir, rot);
	D3DXVec3TransformNormal(&upVec, &upVec, &rotM);

	D3DXMATRIX viewM;
	D3DXMATRIX projM;
	D3DXMATRIX splatProj;

	D3DXMatrixLookAtLH(&viewM, &eyeVec, &targVec, &upVec);
	D3DXMatrixOrthoLH(&projM, width, height, nearZ, farZ);

	D3DXMatrixMultiply(&splatProj, &viewM, &projM);

	splatTriangleDecal_Terrain(trn, dxDevice, remAge, &splatProj, rayDir, nrmCoof, textureFileName, textures);

	*outSplatProj = splatProj;
}


void genPCNormal(short index, vertexPC* vertexArray, short* indexArray, int numIndices, std::vector<vertexPC>* nrmVis)
{
	vertexPC* vPC = &vertexArray[index];
	D3DXVECTOR4 nrm;
	vertexPC b;
	vertexPC f;
	nrm.w = 0;

	for (int i = 0; i < numIndices; i += 3)
	{
		if (indexArray[i] == index)
		{
			f = vertexArray[indexArray[i + 1]];
			b = vertexArray[indexArray[i + 2]];
		}
		else if (indexArray[i + 1] == index)
		{
			f = vertexArray[indexArray[i + 2]];
			b = vertexArray[indexArray[i]];
		}
		else if (indexArray[i + 2] == index)
		{
			f = vertexArray[indexArray[i]];
			b = vertexArray[indexArray[i + 1]];
		}
		else
			continue;

		if (f.tti != vPC->tti || b.tti != vPC->tti)
			continue;

		f.x -= vPC->x;
		f.y -= vPC->y;
		f.z -= vPC->z;

		b.x -= vPC->x;
		b.y -= vPC->y;
		b.z -= vPC->z;

		nrm.x = f.y * b.z - b.y * f.z;
		nrm.y = f.z * b.x - b.z * f.x;
		nrm.z = f.x * b.y - b.x * f.y;

		float mod = sqrtf(nrm.x * nrm.x + nrm.y * nrm.y + nrm.z * nrm.z);

		nrm.x /= mod;
		nrm.y /= mod;
		nrm.z /= mod;
		break;
	}

	vPC->nx = nrm.x;
	vPC->ny = nrm.y;
	vPC->nz = nrm.z;
	vPC->nw = 0.0f;
}

void genPCTNormal(short index, vertexPCT* vertexArray, short* indexArray, int numIndices, std::vector<vertexPC>* nrmVis)
{
	vertexPCT* vPCT = &vertexArray[index];
	D3DXVECTOR4 nrm;
	vertexPCT b;
	vertexPCT f;
	nrm.w = 0;

	for (int i = 0; i < numIndices; i += 3)
	{
		if (indexArray[i] == index)
		{
			f = vertexArray[indexArray[i + 1]];
			b = vertexArray[indexArray[i + 2]];
		}
		else if (indexArray[i + 1] == index)
		{
			f = vertexArray[indexArray[i + 2]];
			b = vertexArray[indexArray[i]];
		}
		else if (indexArray[i + 2] == index)
		{
			f = vertexArray[indexArray[i]];
			b = vertexArray[indexArray[i + 1]];
		}
		else
			continue;

		if (f.tti != vPCT->tti || b.tti != vPCT->tti)
			continue;

		f.x -= vPCT->x;
		f.y -= vPCT->y;
		f.z -= vPCT->z;

		b.x -= vPCT->x;
		b.y -= vPCT->y;
		b.z -= vPCT->z;

		nrm.x = f.y * b.z - b.y * f.z;
		nrm.y = f.z * b.x - b.z * f.x;
		nrm.z = f.x * b.y - b.x * f.y;

		float mod = sqrtf(nrm.x * nrm.x + nrm.y * nrm.y + nrm.z * nrm.z);

		nrm.x /= mod;
		nrm.y /= mod;
		nrm.z /= mod;
		break;
	}

	vPCT->nx = nrm.x;
	vPCT->ny = nrm.y;
	vPCT->nz = nrm.z;
	vPCT->nw = 0.0f;

	nrmVis->push_back(vertexPC(vPCT->x, vPCT->y, vPCT->z, 255, 255, 0, 0));
	nrmVis->push_back(vertexPC(vPCT->x + vPCT->nx, vPCT->y + vPCT->ny, vPCT->z + vPCT->nz, 255, 255, 0, 0));
}

void genPTW4Normal(short index, vertexPTW4* vertexArray, short* indexArray, int numIndices, std::vector<vertexPC>* nrmVis)
{
	vertexPTW4* vPTW4 = &vertexArray[index];
	D3DXVECTOR4 nrm;
	vertexPTW4 b;
	vertexPTW4 f;
	nrm.w = 0;
	float mod;

	for (int i = 0; i < numIndices; i += 3)
	{
		if (indexArray[i] == index)
		{
			f = vertexArray[indexArray[i + 1]];
			b = vertexArray[indexArray[i + 2]];
		}
		else if (indexArray[i + 1] == index)
		{
			f = vertexArray[indexArray[i + 2]];
			b = vertexArray[indexArray[i]];
		}
		else if (indexArray[i + 2] == index)
		{
			f = vertexArray[indexArray[i]];
			b = vertexArray[indexArray[i + 1]];
		}
		else
			continue;

		f.x -= vPTW4->x;
		f.y -= vPTW4->y;
		f.z -= vPTW4->z;

		b.x -= vPTW4->x;
		b.y -= vPTW4->y;
		b.z -= vPTW4->z;

		nrm.x = f.y * b.z - b.y * f.z;
		nrm.y = f.z * b.x - b.z * f.x;
		nrm.z = f.x * b.y - b.x * f.y;

		mod = sqrtf(nrm.x * nrm.x + nrm.y * nrm.y + nrm.z * nrm.z);

		nrm.x /= mod;
		nrm.y /= mod;
		nrm.z /= mod;
		break;
	}

	vPTW4->nx = nrm.x;
	vPTW4->ny = nrm.y;
	vPTW4->nz = nrm.z;
	vPTW4->nw = 0.0f;

	nrmVis->push_back(vertexPC(vPTW4->x, vPTW4->y, vPTW4->z, 255, 255, 0, 0));
	nrmVis->push_back(vertexPC(vPTW4->x + vPTW4->nx, vPTW4->y + vPTW4->ny, vPTW4->z + vPTW4->nz, 255, 255, 0, 0));

	// ultra Debug only
	
	/*nrmVis.push_back(vertexPC(vPTW4->x, vPTW4->y, vPTW4->z, 255, 255, 0, 0));
	nrmVis.push_back(vertexPC(vPTW4->x + f.x, vPTW4->y + f.y, vPTW4->z + f.z, 255, 255, 0, 0));
	
	nrmVis.push_back(vertexPC(vPTW4->x, vPTW4->y, vPTW4->z, 255, 255, 0, 0));
	nrmVis.push_back(vertexPC(vPTW4->x + b.x, vPTW4->y + b.y, vPTW4->z + b.z, 255, 255, 0, 0));*/
}

void fillOutNrmsPCT(void* vertexArray, int numVertices, std::vector<vertexPC>* nrmVis)
{
	vertexPCT* vPCTs = (vertexPCT*)vertexArray;

	for (int i = 0; i < numVertices; i++)
	{
		vertexPCT* vPCT = &vPCTs[i];
		vPCT->nz = vPCT->nz;
		nrmVis->push_back(vertexPC(vPCT->x, vPCT->y, vPCT->z, 255, 255, 0, 0));
		nrmVis->push_back(vertexPC(vPCT->x + vPCT->nx, vPCT->y + vPCT->ny, vPCT->z + vPCT->nz, 255, 255, 0, 0));
	}
}

void autoGenNormals(void* vertexArray, short* indexArray, int numVertices, int numIndices, DWORD vertexType, std::vector<vertexPC>* nrmVis)
{
	vertexPC* vPCs;
	vertexPCT* vPCTs;
	vertexPTW4* vPTW4s;

start:

	if (vertexType == VX_PC)
	{
		vPCs = (vertexPC*)vertexArray;

		for (int i = 0; i < numVertices; i++)
		{
			genPCNormal(i, vPCs, indexArray, numIndices, nrmVis);
		}
	}
	else if (vertexType == VX_PCT)
	{
		vPCTs = (vertexPCT*)vertexArray;

		for (int i = 0; i < numVertices; i++)
		{
			genPCTNormal(i, vPCTs, indexArray, numIndices, nrmVis);
		}
	}
	else if (vertexType == VX_PTW4)
	{
		vPTW4s = (vertexPTW4*)vertexArray;

		for (int i = 0; i < numVertices; i++)
		{
			genPTW4Normal(i, vPTW4s, indexArray, numIndices, nrmVis);
		}
	}
	else
	{
		vertexType = VX_PTW4;
		goto start;
	}
}

struct viewTrans
{
public:
	float bbuffWidth, bbuffHeight;
	float textScaleX, textScaleY;
	float invTextScaleX, invTextScaleY;
	float winWidth, winHeight;
	float centreX, centreY;
	float scaleX, scaleY;
	float invScaleX, invScaleY;

	viewTrans() { };

	viewTrans(float bbuffSizeX, float bbuffSizeY, float winSizeX, float winSizeY)
	{
		bbuffWidth = bbuffSizeX;
		bbuffHeight = bbuffSizeY;

		winWidth = winSizeX;
		winHeight = winSizeY;

		textScaleX = bbuffWidth / winWidth;
		textScaleY = bbuffHeight / winHeight;
		invTextScaleX = 1.0 / textScaleX;
		invTextScaleY = 1.0 / textScaleY;

		centreX = winSizeX / 2.0;
		centreY = winSizeY / 2.0;
		invScaleX = centreX;
		invScaleY = -centreY;
		scaleX = 1.0 / invScaleX;
		scaleY = 1.0 / invScaleY;
	}

	float xToTextX(float x)
	{
		return x * textScaleX;
	}

	float yToTextY(float y)
	{
		return y * textScaleY;
	}

	float xToScreen(float x)
	{
		return (x - centreX) * scaleX;
	}

	float xToWindow(float x)
	{
		return x * invScaleX + centreX;
	}

	float yToScreen(float y)
	{
		return (y - centreY) * scaleY;
	}

	float yToWindow(float y)
	{
		return y * invScaleY + centreY;
	}
};

// ui item types
const DWORD UIT_blank = 0;
const DWORD UIT_text = 1;
const DWORD UIT_button = 2;

// ui item action
const DWORD UIA_leftclick = 1;
const DWORD UIA_rightclick = 2;
const DWORD UIA_mousemove = 3;

struct uiItem
{
public:
	char* name;
	bool enabled; // false initially
	bool clickable; // false initially
	DWORD itemType;
	RECT rect;
	UNCRZ_effect effect;
	D3DXHANDLE tech;
	LPDIRECT3DVERTEXDECLARATION9 vertexDec;

	bool useTex;
	LPDIRECT3DTEXTURE9 tex;
	bool useTex0;
	LPDIRECT3DTEXTURE9 tex0;
	bool useTex1;
	LPDIRECT3DTEXTURE9 tex1;
	bool useTex2;
	LPDIRECT3DTEXTURE9 tex2;
	bool useTex3;
	LPDIRECT3DTEXTURE9 tex3;

	LPD3DXFONT font;
	D3DXVECTOR4 colMod;
	D3DCOLOR textCol;
	int textBufferSize;
	std::string text;

	std::vector<uiItem*> uiItems;
	uiItem* parent;

	void zeroIsh()
	{
		useTex = false;
		useTex0 = false;
		useTex1 = false;
		useTex2 = false;
		useTex3 = false;
	}

	// tex construtor
	// vertexDecN must be vertexPecPCT
	uiItem(LPDIRECT3DDEVICE9 dxDevice, char* nameN, uiItem* parentN, DWORD itemTypeN, LPDIRECT3DVERTEXDECLARATION9 vertexDecN, char* effectFileName, char* techName, char* texFileName, RECT rectN, std::vector<UNCRZ_effect>* effectList, std::vector<UNCRZ_texture*>* textureList)
	{
		zeroIsh();
		useTex = true;

		name = nameN;
		parent = parentN;
		if (parent != NULL)
			parent->uiItems.push_back(this);
		enabled = false;
		effect = createEffect(dxDevice, effectFileName, VX_PCT, effectFileName, effectList);
		tech = effect.effect->GetTechniqueByName(techName);
		createTexture(dxDevice, texFileName, &tex, textureList);
		useTex = true;
		itemType = itemTypeN;
		rect = rectN;
		vertexDec = vertexDecN;
		colMod = D3DXVECTOR4(0, 1, 1, 1);
	}

	// text constructor
	uiItem(LPDIRECT3DDEVICE9 dxDevice, char* nameN, uiItem* parentN, DWORD itemTypeN, RECT rectN, char* textN, D3DCOLOR textColN, LPD3DXFONT fontN)
	{
		zeroIsh();

		name = nameN;
		parent = parentN;
		if (parent != NULL)
			parent->uiItems.push_back(this);
		enabled = false;
		itemType = itemTypeN;
		rect = rectN;
		font = fontN;
		textCol = textColN;
		text = std::string(textN);
	}

	void setTextures()
	{
		if (useTex)
			effect.setTexture(tex);
		if (useTex0)
			effect.setTexture0(tex0);
		if (useTex1)
			effect.setTexture1(tex1);
		if (useTex2)
			effect.setTexture2(tex2);
		if (useTex3)
			effect.setTexture3(tex3);
	}

	bool getTaped(float x, float y, float offsetX, float offsetY, uiItem** tapedOut, float* xOut, float* yOut)
	{
		if (x >= rect.left + offsetX && x <= rect.right + offsetX && y >= rect.top + offsetY && y <= rect.bottom + offsetY)
		{
			// check children
			for each (uiItem* uii in uiItems)
			{
				if (uii->enabled && uii->getTaped(x, y, offsetX + rect.left, offsetY + rect.top, tapedOut, xOut, yOut))
				{
					return true; // stop on the first child to be taped
				}
			}

			if (clickable)
			{
				// no children taped, return me
				*xOut = x - (rect.left + offsetX);
				*yOut = y - (rect.top + offsetY);
				*tapedOut = this;
				return true;
			}
		}

		return false;
	}

	void draw(LPDIRECT3DDEVICE9 dxDevice, float offsetX, float offsetY, viewTrans* vt)
	{
		if (itemType == UIT_button)
		{
			drawTex(dxDevice, offsetX, offsetY, vt);
		}
		
		if (itemType == UIT_text)
		{
			drawText(offsetX, offsetY, vt);
		}

		for each (uiItem* uii in uiItems)
		{
			if (uii->enabled)
				uii->draw(dxDevice, offsetX + rect.left, offsetY + rect.top, vt);
		}
	}

	void drawText(float offsetX, float offsetY, viewTrans* vt)
	{
		RECT nRect;
		nRect.left = vt->xToTextX(rect.left + offsetX);
		nRect.right = vt->xToTextX(rect.right + offsetX);
		nRect.top = vt->yToTextY(rect.top + offsetY);
		nRect.bottom = vt->yToTextY(rect.bottom + offsetY);

		font->DrawTextA(NULL, text.c_str(), -1, &nRect, 0, textCol);
	}

	void drawTex(LPDIRECT3DDEVICE9 dxDevice, float offsetX, float offsetY, viewTrans* vt)
	{
		D3DXMATRIX idMat;
		D3DXMatrixIdentity(&idMat);

		vertexPCT verts[4];
		float left = vt->xToScreen(rect.left + offsetX);
		float right = vt->xToScreen(rect.right + offsetX);
		float top = vt->yToScreen(rect.top + offsetY);
		float bottom = vt->yToScreen(rect.bottom + offsetY);

		verts[0] = vertexPCT(vertexPC(left, top, 0, 1, 1, 1, -1), 0, 0); // negative tti means ignore tti
		verts[1] = vertexPCT(vertexPC(right, top, 0, 1, 1, 1, -1), 1, 0);
		verts[2] = vertexPCT(vertexPC(left, bottom, 0, 1, 1, 1, -1), 0, 1);
		verts[3] = vertexPCT(vertexPC(right, bottom, 0, 1, 1, 1, -1), 1, 1);

		dxDevice->SetVertexDeclaration(vertexDec);
		effect.setTechnique(tech);
		setTextures();
		effect.setcolMod((float*)&colMod);
		effect.setViewProj(&idMat);
		// effect.setTicker(ticker); would be nice to have this information

		effect.effect->CommitChanges();

		UINT numPasses;
		effect.effect->Begin(&numPasses, 0);
		effect.effect->BeginPass(0);

		dxDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, &verts, sizeof(vertexPCT));

		effect.effect->EndPass();
		effect.effect->End();
	}
};

void drawData::startTimer(bool preFlushDx)
	{
		if (preFlushDx)
		{
			DEBUG_DX_FLUSH();
		}
		DEBUG_HR_START(&hridstart);
	}

void drawData::stopTimer(int genericIndex, bool flushDx, char* label, DWORD outFormat)
	{
		if (flushDx)
		{
			if (!debugFlushing)
			{
				genericLabel[genericIndex]->enabled = false;
				genericBar[genericIndex]->enabled = false;
				return;
			}
			DEBUG_DX_FLUSH();
		}
		float timedSecs;
		DEBUG_HR_END(&hridstart, &hridend, &timedSecs);
		
		float textScale;
		float barScale;
		if (outFormat == GDOF_ms)
		{
			textScale = 1000.0;
			barScale = 1.0;
			genericLabel[genericIndex]->text = std::string(label) + std::string(itoa((int)(timedSecs * textScale), (char*)&buff, 10)) + "ms";
			genericBar[genericIndex]->rect.right = 10 + (int)((float)(genericDebugView->rect.right - genericDebugView->rect.left - 20) * timedSecs * barScale);
		}
		else if (outFormat == GDOF_dms)
		{
			textScale = 10000.0;
			barScale = 10.0;
			genericLabel[genericIndex]->text = std::string(label) + std::string(itoa((int)(timedSecs * textScale), (char*)&buff, 10)) + "dms";
			genericBar[genericIndex]->rect.right = 10 + (int)((float)(genericDebugView->rect.right - genericDebugView->rect.left - 20) * timedSecs * barScale);
		}
		genericLabel[genericIndex]->enabled = true;
		genericBar[genericIndex]->enabled = true;
	}

void drawData::nontimed(int genericIndex, char* label, float value, DWORD outFormat)
	{
		float textScale;
		float barScale;
		if (outFormat == GDOF_ms)
		{
			textScale = 1000.0;
			barScale = 1.0;
			genericLabel[genericIndex]->text = std::string(label) + std::string(itoa((int)(value * textScale), (char*)&buff, 10)) + "ms";
			genericBar[genericIndex]->rect.right = 10 + (int)((float)(genericDebugView->rect.right - genericDebugView->rect.left - 20) * value * barScale);
		}
		else if (outFormat == GDOF_dms)
		{
			textScale = 10000.0;
			barScale = 10.0;
			genericLabel[genericIndex]->text = std::string(label) + std::string(itoa((int)(value * textScale), (char*)&buff, 10)) + "dms";
			genericBar[genericIndex]->rect.right = 10 + (int)((float)(genericDebugView->rect.right - genericDebugView->rect.left - 20) * value * barScale);
		}
		else if (outFormat == GDOF_prop100)
		{
			textScale = 100.0;
			barScale = 1.0;
			genericLabel[genericIndex]->text = std::string(label) + std::string(itoa((int)(value * textScale), (char*)&buff, 10)) + "%";
			genericBar[genericIndex]->rect.right = 10 + (int)((float)(genericDebugView->rect.right - genericDebugView->rect.left - 20) * value * barScale);
		}
		genericLabel[genericIndex]->enabled = true;
		genericBar[genericIndex]->enabled = true;
	}

const DWORD VM_persp = 0;
const DWORD VM_ortho = 1;

struct UNCRZ_view
{
public:
	std::string name;
	int texWidth;
	int texHeight;
	LPDIRECT3DSURFACE9 targetSurface;
	LPDIRECT3DTEXTURE9 targetTex;
	LPDIRECT3DSURFACE9 zSurface;
	
	DWORD viewMode;
	DWORD alphaMode;

	float projectionNear;
	float projectionFar;
	float dimX;
	float dimY;

	D3DXVECTOR3 camPos;
	D3DXVECTOR3 camDir;
	D3DXVECTOR3 camUp;

	std::vector<UNCRZ_obj*> zSortedObjs; // for anything that needs to be drawn back-to-front
	std::vector<int> zsoLocalIndexes; // for anything that needs to be drawn back-to-front

	D3DVIEWPORT9 viewViewPort;
	D3DXMATRIX viewViewMat;
	D3DXMATRIX viewProjMat;
	D3DXMATRIX viewViewProj; // (texAligned, x and y are 0..1)
	D3DXMATRIX viewViewProjVP; // (x and y are -1..1)
	
	bool clearView;
	D3DCOLOR clearColor;

	bool viewEnabled;

	UNCRZ_view(std::string nameN)
	{
		name = nameN;

		// loads of defaults
		camPos = D3DXVECTOR3(0, 0, 0);
		camDir = D3DXVECTOR3(1, 0, 0);
		camUp = D3DXVECTOR3(0, 1, 0);

		viewMode = VM_persp;

		viewEnabled = true;
	}

	void init(LPDIRECT3DDEVICE9 dxDevice, int texWidthN, int texHeightN, D3DFORMAT targetFormat)
	{
		texWidth = texWidthN;
		texHeight = texHeightN;
		D3DXCreateTexture(dxDevice, texWidth, texHeight, 0, D3DUSAGE_RENDERTARGET, targetFormat, D3DPOOL_DEFAULT, &targetTex);
		targetTex->GetSurfaceLevel(0, &targetSurface);
	}

	// for if you allready have a targetSurface (i.e. THE target surface)
	void init(LPDIRECT3DSURFACE9 targetSurfaceN, int texWidthN, int texHeightN)
	{
		texWidth = texWidthN;
		texHeight = texHeightN;
		targetSurface = targetSurfaceN;
	}

	void initStencil(LPDIRECT3DDEVICE9 dxDevice, LPDIRECT3DSURFACE9 zSurfaceN)
	{
		zSurface = zSurfaceN;
	}

	void initStencil(LPDIRECT3DDEVICE9 dxDevice)
	{
		dxDevice->CreateDepthStencilSurface(texWidth, texHeight, D3DFMT_D16, D3DMULTISAMPLE_NONE, 0, TRUE, &zSurface, NULL);
	}

	void dirNormalAt(D3DXVECTOR3 camTarg)
	{
		camDir = camTarg - camPos;
		float mod = sqrtf(camDir.x * camDir.x + camDir.y * camDir.y + camDir.z * camDir.z);
		camDir.x /= mod;
		camDir.y /= mod;
		camDir.z /= mod;
	}
};

struct UNCRZ_over
{
public:
	std::string name;
	int texWidth;
	int texHeight;
	LPDIRECT3DSURFACE9 targetSurface;
	LPDIRECT3DTEXTURE9 targetTex;
	LPDIRECT3DSURFACE9 zSurface;

	DWORD alphaMode;

	UNCRZ_effect effect;
	D3DXHANDLE overTech;
	D3DXVECTOR4 colMod;

	bool useTex;
	LPDIRECT3DTEXTURE9 tex;
	bool useTex0;
	LPDIRECT3DTEXTURE9 tex0;
	bool useTex1;
	LPDIRECT3DTEXTURE9 tex1;
	bool useTex2;
	LPDIRECT3DTEXTURE9 tex2;
	bool useTex3;
	LPDIRECT3DTEXTURE9 tex3;
	
	bool clearView;
	D3DCOLOR clearColor;
	
	bool overEnabled;

	void zeroIsh()
	{
		useTex = false;
		useTex0 = false;
		useTex1 = false;
		useTex2 = false;
		useTex3 = false;
	}

	void setTextures()
	{
		if (useTex)
			effect.setTexture(tex);
		if (useTex0)
			effect.setTexture0(tex0);
		if (useTex1)
			effect.setTexture1(tex1);
		if (useTex2)
			effect.setTexture2(tex2);
		if (useTex3)
			effect.setTexture3(tex3);
	}

	UNCRZ_over(std::string nameN)
	{
		name = nameN;
		zeroIsh();
		overEnabled = true;
	}

	void init(LPDIRECT3DDEVICE9 dxDevice, int texWidthN, int texHeightN, char* effectFileName, char* techName, std::vector<UNCRZ_effect>* effectList, D3DFORMAT targetFormat)
	{
		texWidth = texWidthN;
		texHeight = texHeightN;
		D3DXCreateTexture(dxDevice, texWidth, texHeight, 0, D3DUSAGE_RENDERTARGET, targetFormat, D3DPOOL_DEFAULT, &targetTex);
		targetTex->GetSurfaceLevel(0, &targetSurface);
		
		effect = createEffect(dxDevice, effectFileName, VX_PCT, effectFileName, effectList);
		overTech = effect.effect->GetTechniqueByName(techName);
	}

	void init(LPDIRECT3DDEVICE9 dxDevice, LPDIRECT3DSURFACE9 targetSurfaceN, int texWidthN, int texHeightN, char* effectFileName, char* techName, std::vector<UNCRZ_effect>* effectList)
	{
		texWidth = texWidthN;
		texHeight = texHeightN;
		targetSurface = targetSurfaceN;
		
		effect = createEffect(dxDevice, effectFileName, VX_PCT, effectFileName, effectList);
		overTech = effect.effect->GetTechniqueByName(techName);
	}

	void initStencil(LPDIRECT3DDEVICE9 dxDevice, LPDIRECT3DSURFACE9 zSurfaceN)
	{
		zSurface = zSurfaceN;
	}

	void initStencil(LPDIRECT3DDEVICE9 dxDevice)
	{
		dxDevice->CreateDepthStencilSurface(texWidth, texHeight, D3DFMT_D16, D3DMULTISAMPLE_NONE, 0, TRUE, &zSurface, NULL);
	}
};

// :O
// :O
// END OF UNCRZ STUFF
// :S
// :S

// global lists
std::vector<UNCRZ_effect> effects;
std::vector<UNCRZ_sprite*> sprites;
std::vector<UNCRZ_model*> models;
std::vector<UNCRZ_texture*> textures;
std::vector<UNCRZ_font*> fonts;
std::vector<UNCRZ_FBF_anim*> anims;

std::vector<UNCRZ_model*> plantArr;
std::vector<UNCRZ_sprite_data> fireSprites;
std::vector<UNCRZ_sprite_data> smokeSprites;

UNCRZ_obj* mapObj; // map, for detailed collision
UNCRZ_obj* meObj; // camera gets attached to this is
UNCRZ_obj* cloudObj; // clouds
std::vector<UNCRZ_obj*> objs; // game objects (not rendered)
std::vector<UNCRZ_obj*> zSortedObjs; // for anything that needs to be drawn back-to-front
std::vector<int> zsoLocalIndexes;

UNCRZ_sprite_buffer sbuff;

UNCRZ_sprite* fireSprite;
UNCRZ_sprite* smokeSprite;

// ui
std::vector<uiItem*> uiItems;
viewTrans mainVt;
uiItem* mainView;

// debuging (ui items)
bool debugData = false;
bool debugFlushing = false; // only done if debugData is true
uiItem* debugView;
uiItem* fpsLabel;

uiItem* frameTimeLabel;
uiItem* frameTimeBar;

uiItem* evalTimeLabel;
uiItem* evalTimeBar;
uiItem* evalPlacingLabel;
uiItem* evalPlacingBar;
uiItem* evalObjsLabel;
uiItem* evalObjsBar;
uiItem* evalSpritesLabel;
uiItem* evalSpritesBar;

uiItem* drawTimeLabel;
uiItem* drawTimeBar;
uiItem* drawAnimsLabel;
uiItem* drawAnimsBar;
uiItem* drawUpdateLabel;
uiItem* drawUpdateBar;
uiItem* drawLightsLabel;
uiItem* drawLightsBar;
uiItem* drawTerrainLabel;
uiItem* drawTerrainBar;
uiItem* drawCloudsLabel;
uiItem* drawCloudsBar;
uiItem* drawObjsLabel;
uiItem* drawObjsBar;
uiItem* drawSpritesLabel;
uiItem* drawSpritesBar;
uiItem* drawUiLabel;
uiItem* drawUiBar;
uiItem* drawPresentLabel;
uiItem* drawPresentBar;
uiItem* drawOverLabel;
uiItem* drawOverBar;

const int genericUiCount = 10;
uiItem* genericDebugView;
uiItem* genericLabel[genericUiCount];
uiItem* genericBar[genericUiCount];

// separators
separator zeroSepor;
separator contactSepor;
separator humanSepor;
separator treeSepor;
separator lampSepor;
separator baseSepor;
separator refinarySepor;
separator plantSepor;
float placeMargin = 5;

// forward defs
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LPDIRECT3DDEVICE9 initDevice(HWND);
void initEffect(LPDIRECT3DDEVICE9);
void drawFrame(LPDIRECT3DDEVICE9);
void drawScene(LPDIRECT3DDEVICE9, drawData*, UNCRZ_view*, DWORD, DWORD);
void drawOver(LPDIRECT3DDEVICE9, drawData*, UNCRZ_over*);
void drawUi(LPDIRECT3DDEVICE9);
void drawLight(LPDIRECT3DDEVICE9, lightData*);
void prepDynamicDecal(LPDIRECT3DDEVICE9, dynamicDecalData*);
void attachDynamicDecal(LPDIRECT3DDEVICE9, UNCRZ_obj* o);
void initObjs(LPDIRECT3DDEVICE9);
void initSprites(LPDIRECT3DDEVICE9);
void initFont(LPDIRECT3DDEVICE9);
void initUi(LPDIRECT3DDEVICE9);
void initTextures(LPDIRECT3DDEVICE9);
void initViews(LPDIRECT3DDEVICE9);
void initOvers(LPDIRECT3DDEVICE9);
void initLights(LPDIRECT3DDEVICE9);
void initOther();
void settleObj(UNCRZ_obj*);
void updateDecals();
bool canPlace(separator, float, float);
void term();
void deSelect();
void eval();
void handleUi(uiItem*, DWORD);
void handleUi(uiItem*, DWORD, DWORD*, int);
void handleKeys();
void reload(LPDIRECT3DDEVICE9);
void moveCamera(LPDIRECT3DDEVICE9);
void moveCameraView(LPDIRECT3DDEVICE9, UNCRZ_view*);
void moveCamerawaterReflect(LPDIRECT3DDEVICE9);
void moveCamerawaterRefract(LPDIRECT3DDEVICE9);
void moveCameraLight(LPDIRECT3DDEVICE9, lightData*);
void moveCameraDynamicDecal(LPDIRECT3DDEVICE9, dynamicDecalData*);
int rnd(int);
drawData createDrawData(float, D3DVIEWPORT9* vp);
drawData createDrawDataView(float, D3DVIEWPORT9* vp, UNCRZ_view*);
drawData createDrawDataOver(float, D3DVIEWPORT9* vp, UNCRZ_over*);
D3DVIEWPORT9 createViewPort(float);
D3DVIEWPORT9 createViewPort(UINT, UINT);		

float ticker = 0;

char* appName = "Sharaes";
char* windowText = "Sharaes";
int windowSizeX = 800;
int windowSizeY = 600;

D3DXVECTOR3 camPos(0.0f, 40.0f, 0.0f);
float rotY = 0;
float rotPar = 0.0f;
float moveVel = 0.4f;
float focalDist = 10.0f;

D3DXVECTOR4 eyePos; // for shaders
D3DXVECTOR4 eyeDir;

// OMG OMG
bool disableOpenTarget = true; // disables the historical target->postprocess->screen system (use UI Items to show stuff)
// OMG OMG

D3DXMATRIXA16 viewMatrix;
D3DXMATRIX projMatrix;
D3DXMATRIX viewProj;
D3DXMATRIX vpMat; // view port matrix

D3DCOLOR textCol;
LPD3DXFONT textFont;

D3DXVECTOR4 white(1.0, 1.0, 1.0, 1.0);

DWORD viewMode = VM_persp;

bool wireFrame;
bool vertigo = false;
bool running = true;
HINSTANCE hInst;
HWND mainHWnd;
LPDIRECT3DDEVICE9 mainDxDevice;

//float rippleCycle = 0.0f;
//float vertigoCycle = 1.0f;
float farDepth = 1000.0f;
std::vector<vertexPC> normalVis;

LARGE_INTEGER hrticks; // ticks since some time in the past
LARGE_INTEGER hrsec; // ticks per second
LARGE_INTEGER hrlsec; // tick of the last second
LARGE_INTEGER hrbstart; // start of a timed block
LARGE_INTEGER hrbend; // end of a timed block
LARGE_INTEGER hrsbstart; // start of a timed sub block
LARGE_INTEGER hrsbend; // end of a timed sub block
LARGE_INTEGER hrfstart; // start of frame
LARGE_INTEGER hrfend; // end of frame
int frames = 0;
int fps = 0;

// measurements
float hrframeTime; // seconds

float hrevalTime; // seconds
float hrevalSpritesTime; // seconds
float hrevalObjsTime; // seconds

float hrdrawTime; // seconds
float hrdrawAnimsTime; // seconds
float hrdrawUpdateTime; // seconds
float hrdrawLightsTime; // seconds
float hrdrawTerrainTime; // seconds
float hrdrawCloudsTime; // seconds
float hrdrawObjsTime; // seconds
float hrdrawSpritesTime; // seconds
float hrdrawOverTime; // seconds
float hrdrawUiTime; // seconds
float hrdrawPresentTime; // seconds

float hrgenericTime[genericUiCount];

// scary queries for flushing
IDirect3DQuery9* flushQuery;

void createFlushQuery(LPDIRECT3DDEVICE9 dxDevice)
{
	dxDevice->CreateQuery(D3DQUERYTYPE_EVENT, &flushQuery);
}

// measureFunctions
void hr_start(LARGE_INTEGER* start)
{
	QueryPerformanceCounter(start);
}

void hr_zero(float* outTime)
{
	*outTime = 0.0;
}

void hr_end(LARGE_INTEGER* start, LARGE_INTEGER* end, float* outTime)
{
	QueryPerformanceCounter(end);
	*outTime = ((double)(end->QuadPart - start->QuadPart) / (double)(hrsec.QuadPart));
}

void hr_accend(LARGE_INTEGER* start, LARGE_INTEGER* end, float* outTime)
{ // cumulative
	QueryPerformanceCounter(end);
	*outTime += ((double)(end->QuadPart - start->QuadPart) / (double)(hrsec.QuadPart));
}

float tInc = 0;
bool keyDown[255];

// vpData
UINT vpWidth;
UINT vpHeight;

float targetTexScale = 1.0f; // SSAA.... ish
float lightTexScale = 10.0f;

// textures
LPDIRECT3DTEXTURE9 targetTex;
LPDIRECT3DTEXTURE9 waterReflectTex;
LPDIRECT3DTEXTURE9 waterRefractTex;
LPDIRECT3DTEXTURE9 underTex;
LPDIRECT3DTEXTURE9 sideTex;
LPDIRECT3DTEXTURE9 ripplesTex;

// surfaces in use
LPDIRECT3DSURFACE9 finalTargetSurface;
LPDIRECT3DSURFACE9 targetSurface;
LPDIRECT3DSURFACE9 sideSurface; // for side rendering (stuff that will ultimatly be on the target)

// depth surfaces
LPDIRECT3DSURFACE9 zSurface;
LPDIRECT3DSURFACE9 zSideSurface;
LPDIRECT3DSURFACE9 zLightSurface;

// shady stuff
LPD3DXEFFECT effectA = NULL;
UNCRZ_effect aeffect;
LPDIRECT3DTEXTURE9 tex;

// views
std::vector<UNCRZ_view*> views;

// overs
std::vector<UNCRZ_over*> overs;

// lighting
std::vector<lightData*> lights;

// dynamic decals
std::vector<dynamicDecalData*> dynamicDecals;

//
// game stuff
//

bool g_pause = false;
bool g_ticks = 0;
bool initialised = false;

//
// end game stuff
//

int WINAPI WinMain(HINSTANCE hInstance,
				   HINSTANCE hPrevInstance,
				   LPSTR lpCmdLine,
				   int nCmdShow)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style          = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc    = WndProc;
	wcex.cbClsExtra     = 0;
	wcex.cbWndExtra     = 0;
	wcex.hInstance      = hInstance;
	wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPLICATION));
	wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground  = GetSysColorBrush(COLOR_BTNFACE);
	wcex.lpszMenuName   = NULL;
	wcex.lpszClassName  = appName;
	wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_APPLICATION));

	if (!RegisterClassEx(&wcex))
	{
		MessageBox(NULL,
			_T("Call to RegisterClassEx failed!"),
			windowText,
			NULL);

		return 1;
	}

	hInst = hInstance; // Store instance handle in our global variable

	// The parameters to CreateWindow explained:
	// szWindowClass: the name of the application
	// szTitle: the text that appears in the title bar
	// WS_OVERLAPPEDWINDOW: the type of window to create
	// CW_USEDEFAULT, CW_USEDEFAULT: initial position (x, y)
	// 500, 100: initial size (width, length)
	// NULL: the parent of this window
	// NULL: this application does not have a menu bar
	// hInstance: the first parameter from WinMain
	// NULL: not used in this application
	HWND hWnd = CreateWindowEx(0,
		appName,
		windowText,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		windowSizeX, windowSizeY,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	if (!hWnd)
	{
		MessageBox(NULL,
			_T("Call to CreateWindow failed!"),
			windowText,
			NULL);

		return 1;
	}

	mainHWnd = hWnd;

	SetTimer(hWnd, 77, 100, NULL);

	// The parameters to ShowWindow explained:
	// hWnd: the value returned from CreateWindow
	// nCmdShow: the fourth parameter from WinMain
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	mainDxDevice = initDevice(hWnd);
	initVertexDec(mainDxDevice);
	initEffect(mainDxDevice); // Enable to test for shader errors
	initTextures(mainDxDevice);
	initViews(mainDxDevice);
	initOvers(mainDxDevice);
	initLights(mainDxDevice);
	initFont(mainDxDevice);
	initUi(mainDxDevice);
	initOther();

	reload(mainDxDevice);

	initialised = true; // this is rather important

	// Main message loop:
	MSG msg;
	while (running && GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int) msg.wParam;
}

int getTapedObj(D3DXVECTOR3* rayPos, D3DXVECTOR3* rayDir, float* distRes, bool checkSepor = false)
{
	int best = -1;
	float bestDist;
	float localDistRes = 0.0;

	for (int i = objs.size() - 1; i >= 0; i--)
	{
		if (objs[i]->model->collides(rayPos, rayDir, &localDistRes))
		{
			if (best == -1 || localDistRes < bestDist)
			{
				best = i;
				bestDist = localDistRes;
			}
		}
	}

	*distRes = localDistRes;
	if (best != -1 || checkSepor == false)
		return best;

	return -1;
}

uiItem* getTapedUiItem(float x, float y, float* xOut, float* yOut)
{
	uiItem* taped;
	for each (uiItem* uii in uiItems)
	{
		if (uii->enabled && uii->getTaped(x, y, 0, 0, &taped, xOut, yOut))
			return taped;
	}
	return NULL;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	float sx, sy;
	float lx, ly;
	uiItem* tapedUii;

	switch (message)
	{
	case WM_TIMER:
		/*if (wParam == 77)
		{
			drawFrame(mainDxDevice);
			fps = frames;
			frames = 0;
		}*/
		break;
	case WM_MOUSEMOVE:
		sx = (float)LOWORD(lParam);
		sy = (float)HIWORD(lParam);

		tapedUii = getTapedUiItem(sx, sy, &lx, &ly);
		if (tapedUii != NULL)
		{
			DWORD hudat[2] = { lx, ly };
			handleUi(tapedUii, UIA_mousemove, hudat, 2);
			break;
		}
		break;
	case WM_LBUTTONDOWN:
		sx = (float)LOWORD(lParam);
		sy = (float)HIWORD(lParam);

		tapedUii = getTapedUiItem(sx, sy, &lx, &ly);
		if (tapedUii != NULL)
		{
			DWORD hudat[2] = { lx, ly};
			handleUi(tapedUii, UIA_leftclick, hudat, 2);
			break;
		}
		break;
	case WM_RBUTTONDOWN:
		sx = (float)LOWORD(lParam);
		sy = (float)HIWORD(lParam);

		tapedUii = getTapedUiItem(sx, sy, &lx, &ly);
		if (tapedUii != NULL)
		{
			DWORD hudat[2] = { lx, ly };
			handleUi(tapedUii, UIA_rightclick, hudat, 2);
			break;
		}
		break;
	case WM_KEYDOWN:
		keyDown[wParam] = true;
		if (wParam == 64 + 6) // f
		{
			wireFrame = !wireFrame;
		}
		else if (wParam == 64 + 7) // g
		{
		}
		else if (wParam == VK_PAUSE || wParam == 64 + 16) // p
		{
			g_pause = !g_pause;
		}
		else if (wParam == VK_HOME)
		{
			if (debugFlushing)
			{
				debugData = false;
				debugFlushing = false;
			}
			else if (debugData)
			{
				debugFlushing = true;
			}
			else
			{
				debugData = true;
			}
		}
		else if (wParam == VK_ESCAPE)
		{
		}
		break;
	case WM_KEYUP:
		keyDown[wParam] = false;
		break;
	case WM_DESTROY:
		mainDxDevice->Release();
		running = false;
		for each(UNCRZ_model* m in models)
		{
			m->release();
		}
		break;
	case WM_PAINT:
		if (!initialised || g_pause)
			break;
		
		DEBUG_HR_START(&hrfstart);

		frames++;
		// hr perf stuff
		QueryPerformanceFrequency(&hrsec);
		QueryPerformanceCounter(&hrticks);
		if (hrticks.QuadPart > hrlsec.QuadPart + hrsec.QuadPart)
		{
			// 1sec elapsed
			fps = frames;
			hrlsec = hrticks;
			frames = 0;
		}

		// everything interesting
		DEBUG_HR_START(&hrbstart);
		eval();
		DEBUG_HR_END(&hrbstart, &hrbend, &hrevalTime);

		DEBUG_HR_START(&hrbstart);
		drawFrame(mainDxDevice);
		DEBUG_HR_END(&hrbstart, &hrbend, &hrdrawTime);

		DEBUG_HR_END(&hrfstart, &hrfend, &hrframeTime);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

float getDist(float x0, float y0, float x1, float y1)
{
	float x = x0 - x1;
	float y = y0 - y1;
	return sqrt(x * x + y * y);
}

float getDistNoSqrt(float x0, float y0, float x1, float y1)
{
	float x = x0 - x1;
	float y = y0 - y1;
	return x * x + y * y;
}

float getDist(float x0, float y0, float z0, float x1, float y1, float z1)
{
	float x = x0 - x1;
	float y = y0 - y1;
	float z = z0 - z1;
	return sqrt(x * x + y * y + z * z);
}

float getDistNoSqrt(float x0, float y0, float z0, float x1, float y1, float z1)
{
	float x = x0 - x1;
	float y = y0 - y1;
	float z = z0 - z1;
	return x * x + y * y + z * z;
}

void prepBMap(std::ifstream* file, int* width, int* height)
{
	for (int i = 0; i < 10; i++)
	{
		file->get();
	}

	// 11
	int offset;
	offset = file->get();
	offset += file->get() * 256;
	offset += file->get() * 256 * 256;
	offset += file->get() * 256 * 256 * 256;
	// 15

	for (int i = 0; i < 4; i++)
		file->get();
	
	// 19
	*width = file->get();
	*width += file->get() * 256;
	*width += file->get() * 256 * 256;
	*width += file->get() * 256 * 256 * 256;
	
	*height = file->get();
	*height += file->get() * 256;
	*height += file->get() * 256 * 256;
	*height += file->get() * 256 * 256 * 256;

	// 27
	for (int i = 27; i <= offset; i++)
		file->get();
}

void handleUi(uiItem* uii, DWORD action)
{
	handleUi(uii, action, NULL, 0);
}

void handleUi(uiItem* uii, DWORD action, DWORD* data, int datalen)
{
	RECT crect;
	D3DVIEWPORT9 vp;
	D3DXMATRIX mehMatrix;
	D3DXMATRIX viewProjInv;
	D3DXVECTOR3 nearVec, farVec, screenVec, rayPos, rayDir;
	float distRes = 0.0f;
	float rayDirMod;
	D3DXVECTOR3 simpleDir;
	D3DXVECTOR3 simpleLoc;

	switch (action)
	{
	case UIA_leftclick:
		if (uii->name == "mainover" && datalen == 2)
		{
			//mainDxDevice->GetViewport(&vp);
			vp = createViewPort(1);
			GetClientRect(mainHWnd, &crect);
			vp = createViewPort(crect.right - crect.left, crect.bottom - crect.top);

			screenVec.x = data[0];
			screenVec.y = data[1];

			screenVec.z = 0.1f;

			D3DXMatrixIdentity(&mehMatrix);

			D3DXVec3Unproject(&nearVec, &screenVec, &vp, &views[0]->viewProjMat, &views[0]->viewViewMat, &mehMatrix);

			screenVec.z = 1.0f;
			D3DXVec3Unproject(&farVec, &screenVec, &vp, &views[0]->viewProjMat, &views[0]->viewViewMat, &mehMatrix);

			rayPos = nearVec;
			rayDir = farVec - nearVec;
			rayDirMod = sqrt(rayDir.x * rayDir.x + rayDir.y * rayDir.y + rayDir.z * rayDir.z);
			rayDir.x /= rayDirMod;
			rayDir.y /= rayDirMod;
			rayDir.z /= rayDirMod;

			simpleSplatSquareDecal_Model(meObj->model, mainDxDevice, 100, &rayPos, &rayDir, 1.0, 1.0, 1.0, 0, 100, "tap.tga", &textures, rnd(3142), &mehMatrix);
		}
		break;
	case UIA_rightclick:
		break;
	}
}

void handleKeys()
{
	RECT crect;
	float sx, sy;
	D3DVIEWPORT9 vp;
	D3DXMATRIX mehMatrix;
	D3DXMATRIX viewProjInv;
	D3DXVECTOR3 nearVec, farVec, screenVec, rayPos, rayDir;
	D3DXVECTOR3 dirVec;
	float distRes = 0.0f;

	if (keyDown[VK_UP])
	{
		rotPar += 0.1f;
	}
	if (keyDown[VK_DOWN])
	{
		rotPar -= 0.1f;
	}
	if (keyDown[VK_RIGHT])
	{
		rotY += 0.1f;
	}
	if (keyDown[VK_LEFT])
	{
		rotY -= 0.1f;
	}
	if (keyDown[VK_SPACE])
	{
		//D3DXSaveTextureToFile("mehReflect.bmp", D3DXIFF_BMP, waterReflectTex, NULL);
		//D3DXSaveTextureToFile("mehRefract.bmp", D3DXIFF_BMP, waterRefractTex, NULL);
		//D3DXSaveTextureToFile("mehUnder.bmp", D3DXIFF_BMP, underTex, NULL);
		//D3DXSaveTextureToFile("mehLight.bmp", D3DXIFF_BMP, lights[2]->lightTex, NULL);
		D3DXSaveTextureToFile("mehSun.bmp", D3DXIFF_BMP, lights[0]->lightTex, NULL);
		D3DXSaveTextureToFile("mehSide.bmp", D3DXIFF_BMP, sideTex, NULL);
		D3DXSaveTextureToFile("mehTarget.bmp", D3DXIFF_BMP, targetTex, NULL);
		D3DXSaveTextureToFile("mehMainV.bmp", D3DXIFF_BMP, views[0]->targetTex, NULL);
		D3DXSaveTextureToFile("mehMainO.bmp", D3DXIFF_BMP, overs[0]->targetTex, NULL);
	}
	if (keyDown[64 + 23]) // w
	{ // FORWARD
		dirVec = D3DXVECTOR3(moveVel, 0.0f, 0.0f);
		D3DXMatrixRotationY(&mehMatrix, rotY);
		D3DXVec3TransformNormal(&dirVec, &dirVec, &mehMatrix);
		meObj->offset += dirVec;
		settleObj(meObj);
		//camPos += dirVec;
	}
	if (keyDown[64 + 1]) // a
	{ // LEFT
		dirVec = D3DXVECTOR3(moveVel, 0.0f, 0.0f);
		D3DXMatrixRotationY(&mehMatrix, rotY - D3DX_PI * 0.5f);
		D3DXVec3TransformNormal(&dirVec, &dirVec, &mehMatrix);
		meObj->offset += dirVec;
		settleObj(meObj);
		//camPos += dirVec;
	}
	if (keyDown[64 + 19]) // s
	{ // BACKWARD
		dirVec = D3DXVECTOR3(moveVel, 0.0f, 0.0f);
		D3DXMatrixRotationY(&mehMatrix, rotY - D3DX_PI);
		D3DXVec3TransformNormal(&dirVec, &dirVec, &mehMatrix);
		meObj->offset += dirVec;
		settleObj(meObj);
		//camPos += dirVec;
	}
	if (keyDown[64 + 4]) // d
	{ // RIGHT
		dirVec = D3DXVECTOR3(moveVel, 0.0f, 0.0f);
		D3DXMatrixRotationY(&mehMatrix, rotY + D3DX_PI * 0.5f);
		D3DXVec3TransformNormal(&dirVec, &dirVec, &mehMatrix);
		meObj->offset += dirVec;
		settleObj(meObj);
		//camPos += dirVec;
	}
}

void evalTime()
{
	g_ticks++;
}

void eval()
{
	evalTime();
	handleKeys();

	float distRes;
	D3DXVECTOR3 nearVec, farVec, rayDir;

	// sprites
	DEBUG_HR_START(&hrsbstart);

	if (rnd(10) == 4)
	{
		fireSprites.push_back(UNCRZ_sprite_data(D3DXVECTOR4(
		0,
		0,
		0,
		1
		), D3DXVECTOR4(0, 100, 30, 80)));
	}
	else if (rnd(9) == 3)
	{
		smokeSprites.push_back(UNCRZ_sprite_data(D3DXVECTOR4(
		0,
		0,
		0,
		1
		), D3DXVECTOR4(0.0, 0.001, 5, 0.1)));
	}

	// fire
	for (int i = (int)fireSprites.size() - 1; i >= 0; i--)
	{
		if (++fireSprites[i].other.x >= fireSprites[i].other.w)
			fireSprites.erase(fireSprites.begin() + i, fireSprites.begin() + i + 1);
		else
		{
			fireSprites[i].pos.x += 0.001 * (float)rnd(100);
			fireSprites[i].pos.y += 0.1;
			fireSprites[i].pos.z += 0.001 * (float)rnd(100);
		}
	}

	// smoke
	for (int i = (int)smokeSprites.size() - 1; i >= 0; i--)
	{
		smokeSprites[i].pos.x += 0.001 * (float)rnd(100);
		smokeSprites[i].pos.y += 0.1;
		smokeSprites[i].pos.z += 0.001 * (float)rnd(100);
		smokeSprites[i].other.x += smokeSprites[i].other.y * (float)rnd(10);
		if (smokeSprites[i].other.x >= 1)
			smokeSprites.erase(smokeSprites.begin() + i, smokeSprites.begin() + i + 1);
	}

	DEBUG_HR_END(&hrsbstart, &hrsbend, &hrevalSpritesTime);





	// units
	DEBUG_HR_START(&hrsbstart);

	DEBUG_HR_END(&hrsbstart, &hrsbend, &hrevalObjsTime);



	// move me to map
	meObj->rotation.y = rotY;
	settleObj(meObj);

	// views
	
	RECT crect;
	GetClientRect(mainHWnd, &crect);
	int winWidth = crect.right - crect.left;
	int winHeight = crect.bottom - crect.top;

	// get normaled vecs
	camPos = meObj->offset + D3DXVECTOR3(0.0f, 10.0f, 0.0f);

	D3DXMATRIX mehMatrix;
	D3DXVECTOR3 dirVec = D3DXVECTOR3(1.0f, 0.0f, 0.0f);
	D3DXMatrixRotationZ(&mehMatrix, rotPar);
	D3DXVec3TransformNormal(&dirVec, &dirVec, &mehMatrix);
	D3DXMatrixRotationY(&mehMatrix, rotY);
	D3DXVec3TransformNormal(&dirVec, &dirVec, &mehMatrix);

	camPos -= dirVec * focalDist;
	D3DXVECTOR3 targVec = camPos + dirVec * focalDist;

	views[0]->dimY = (float)winWidth / (float)winHeight;
	views[0]->camPos = camPos;
	views[0]->dirNormalAt(targVec);
}

void settleObj(UNCRZ_obj* obj)
{
	float distRes;
	D3DXVECTOR3 nearVec, farVec, rayDir;

	// set Y
	nearVec = obj->offset;
	nearVec.y += 5.0f;

	rayDir = D3DXVECTOR3(0.0, -1.0, 0.0);

	bool victory = mapObj->model->collidesVertex(&nearVec, &rayDir, &distRes);
	if (victory)
	{
		obj->offset.y = nearVec.y - distRes;
	}

	obj->update(true);
}

void reload(LPDIRECT3DDEVICE9 dxDevice)
{
	objs.clear();

	for (int i = models.size() - 1; i >= 0; i--)
	{
		models[i]->release();
	}
	models.clear();
	anims.clear();

	loadModelsFromFile("text.uncrz", mainDxDevice, &models, &effects, &textures, &normalVis);
	loadSpritesFromFile("textS.uncrz", mainDxDevice, &sprites, &effects, &textures);
	loadAnimsFromFile("textA.uncrz", &anims, &models);

	initObjs(dxDevice);
	initSprites(dxDevice);
}

LPDIRECT3DDEVICE9 initDevice(HWND hWnd)
{	
	LPDIRECT3D9 dxObj;
	dxObj = Direct3DCreate9(D3D_SDK_VERSION);

	if (dxObj == NULL)
	{
		MessageBox(hWnd, "ARGH!!!!", "ARGH!!!", MB_OK);
	}

	D3DPRESENT_PARAMETERS dxPresParams;
	ZeroMemory(&dxPresParams, sizeof(dxPresParams));
	dxPresParams.Windowed = true;
	dxPresParams.SwapEffect = D3DSWAPEFFECT_DISCARD;
	dxPresParams.EnableAutoDepthStencil = TRUE;
	dxPresParams.AutoDepthStencilFormat = D3DFMT_D16;
    dxPresParams.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
	dxPresParams.MultiSampleType = D3DMULTISAMPLE_4_SAMPLES;
	//dxPresParams.MultiSampleType = D3DMULTISAMPLE_NONE;
	
	dxPresParams.BackBufferWidth = 1600;
	dxPresParams.BackBufferHeight = 1200;
	//dxPresParams.BackBufferWidth = 1280;
	//dxPresParams.BackBufferHeight = 960;
	//dxPresParams.BackBufferWidth = 800;
	//dxPresParams.BackBufferHeight = 600;

	// fit buffers to size of screen (for testing only)
	RECT crect;
	GetClientRect(mainHWnd, &crect);
	//dxPresParams.BackBufferWidth = crect.right - crect.left + 1;
	//dxPresParams.BackBufferHeight = crect.bottom - crect.top + 1;

	if (dxPresParams.Windowed)
		dxPresParams.FullScreen_RefreshRateInHz = 0;
	else
	{
		dxPresParams.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
		dxPresParams.BackBufferCount = 1;
		dxPresParams.BackBufferFormat = D3DFMT_R5G6B5;
	}

	LPDIRECT3DDEVICE9 dxDevice;
	LRESULT createRes = dxObj->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &dxPresParams, &dxDevice);

	dxDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	dxDevice->SetRenderState(D3DRS_LIGHTING, false);
	dxDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);

	//dxDevice->GetTexture(0, (IDirect3DBaseTexture9**)&targetTex);
	dxDevice->GetRenderTarget(0, &finalTargetSurface);

	createFlushQuery(dxDevice);

	return dxDevice;
}

void initFont(LPDIRECT3DDEVICE9 dxDevice)
{
	D3DXCreateFont(dxDevice, 20, 0, FW_NORMAL, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Tahoma", &textFont);

	textCol = D3DCOLOR_ARGB(255, 0, 0, 128);
}

void initUi(LPDIRECT3DDEVICE9 dxDevice)
{
	D3DVIEWPORT9 vp = createViewPort(1);

	RECT crect;
	GetClientRect(mainHWnd, &crect);
	int winWidth = crect.right - crect.left;
	int winHeight = crect.bottom - crect.top;
	mainVt = viewTrans(vp.Width, vp.Height, winWidth, winHeight);

	// font
	LPD3DXFONT uiFont;
	createFont(dxDevice, (int)mainVt.yToTextY((float)20), "uifont", &uiFont, &fonts);

	// uiItems
	RECT rect;
	uiItem* temp;

	// (view one view)
	rect.left = 0;
	rect.right = winWidth + 1;
	rect.top = 0;
	rect.bottom = winHeight + 1;
	temp = new uiItem(dxDevice, "mainover", NULL, UIT_button, vertexDecPCT, "un_shade.fx", "over_final", "over_main", rect, &effects, &textures);
	temp->enabled = true; // NEED to work out why these shaders are so whiney (simpleUi uses linear sampler, can't do linear sample on render target? - seems to be happy now?)
	temp->clickable = true;
	uiItems.push_back(temp);
	temp->colMod = D3DXVECTOR4(1, 1, 1, 1);
	mainView = temp;

	// debug view
	rect.left = 175;
	rect.right = 395;
	rect.top = 45;
	rect.bottom = 500;
	temp = new uiItem(dxDevice, "debugItem", NULL, UIT_button, vertexDecPCT, "un_shade.fx", "simpleUi", "ui/bland.tga", rect, &effects, &textures);
	temp->enabled = false;
	temp->clickable = false;
	uiItems.push_back(temp);
	temp->colMod = D3DXVECTOR4(0, 0.5, 1, 0.5);
	debugView = temp;

	// vlines
	rect.left = 9;
	rect.right = 10;
	rect.top = 10;
	rect.bottom = debugView->rect.bottom - debugView->rect.top - 10;
	temp = new uiItem(dxDevice, "vlineLeft", debugView, UIT_button, vertexDecPCT, "un_shade.fx", "simpleUi", "ui/pure.tga", rect, &effects, &textures);
	temp->colMod = D3DXVECTOR4(0, 0, 0, 1);
	temp->enabled = true;
	temp->clickable = false;

	rect.left = debugView->rect.right - debugView->rect.left - 10;
	rect.right = debugView->rect.right - debugView->rect.left - 9;
	rect.top = 10;
	rect.bottom = debugView->rect.bottom - debugView->rect.top - 10;
	temp = new uiItem(dxDevice, "vlineLeft", debugView, UIT_button, vertexDecPCT, "un_shade.fx", "simpleUi", "ui/pure.tga", rect, &effects, &textures);
	temp->colMod = D3DXVECTOR4(0, 0, 0, 1);
	temp->enabled = true;
	temp->clickable = false;

	int dg = 10;
	// fps
	rect.left = 15;
	rect.right = 210;
	rect.top = dg;
	rect.bottom = dg + 20;
	temp = new uiItem(dxDevice, "fpsLabel", debugView, UIT_text, rect, "~~ fps ~~", D3DCOLOR_ARGB(255, 0, 0, 128), uiFont);
	temp->enabled = true;
	temp->clickable = false;
	fpsLabel = temp;

	dg += 30;
	// frame time
	rect.left = 10;
	rect.right = 10; // this will change
	rect.top = dg;
	rect.bottom = dg + 20;
	temp = new uiItem(dxDevice, "frameTimeBar", debugView, UIT_button, vertexDecPCT, "un_shade.fx", "simpleUi", "ui/pure.tga", rect, &effects, &textures);
	temp->colMod = D3DXVECTOR4(1, 1, 1, 1);
	temp->enabled = true;
	temp->clickable = false;
	frameTimeBar = temp;

	rect.left = 15;
	rect.right = 190;
	rect.top = dg;
	rect.bottom = dg + 20;
	temp = new uiItem(dxDevice, "frameTimeLabel", debugView, UIT_text, rect, "~~ frame time ~~", D3DCOLOR_ARGB(255, 0, 0, 128), uiFont);
	temp->enabled = true;
	temp->clickable = false;
	frameTimeLabel = temp;

	dg += 30;
	// eval time
	rect.left = 10;
	rect.right = 10; // this will change
	rect.top = dg;
	rect.bottom = dg + 20;
	temp = new uiItem(dxDevice, "evalTimeBar", debugView, UIT_button, vertexDecPCT, "un_shade.fx", "simpleUi", "ui/pure.tga", rect, &effects, &textures);
	temp->colMod = D3DXVECTOR4(1, 0, 0, 1);
	temp->enabled = true;
	temp->clickable = false;
	evalTimeBar = temp;

	rect.left = 15;
	rect.right = 190;
	rect.top = dg;
	rect.bottom = dg + 20;
	temp = new uiItem(dxDevice, "evalTimeLabel", debugView, UIT_text, rect, "~~ eval time ~~", D3DCOLOR_ARGB(255, 0, 0, 128), uiFont);
	temp->enabled = true;
	temp->clickable = false;
	evalTimeLabel = temp;

	dg += 25;
	// eval sprites time
	rect.left = 10;
	rect.right = 10; // this will change
	rect.top = dg;
	rect.bottom = dg + 20;
	temp = new uiItem(dxDevice, "evalPlacingBar", debugView, UIT_button, vertexDecPCT, "un_shade.fx", "simpleUi", "ui/pure.tga", rect, &effects, &textures);
	temp->colMod = D3DXVECTOR4(1, 0.5, 0.5, 1);
	temp->enabled = true;
	temp->clickable = false;
	evalPlacingBar = temp;

	rect.left = 15;
	rect.right = 190;
	rect.top = dg;
	rect.bottom = dg + 20;
	temp = new uiItem(dxDevice, "evalPlacingLabel", debugView, UIT_text, rect, "~~ eval placing time ~~", D3DCOLOR_ARGB(255, 0, 0, 128), uiFont);
	temp->enabled = true;
	temp->clickable = false;
	evalPlacingLabel = temp;

	dg += 25;
	// eval sprites time
	rect.left = 10;
	rect.right = 10; // this will change
	rect.top = dg;
	rect.bottom = dg + 20;
	temp = new uiItem(dxDevice, "evalSpritesBar", debugView, UIT_button, vertexDecPCT, "un_shade.fx", "simpleUi", "ui/pure.tga", rect, &effects, &textures);
	temp->colMod = D3DXVECTOR4(1, 0.5, 0.5, 1);
	temp->enabled = true;
	temp->clickable = false;
	evalSpritesBar = temp;

	rect.left = 15;
	rect.right = 190;
	rect.top = dg;
	rect.bottom = dg + 20;
	temp = new uiItem(dxDevice, "evalSpritesLabel", debugView, UIT_text, rect, "~~ eval sprites time ~~", D3DCOLOR_ARGB(255, 0, 0, 128), uiFont);
	temp->enabled = true;
	temp->clickable = false;
	evalSpritesLabel = temp;

	dg += 25;
	// eval objs time
	rect.left = 10;
	rect.right = 10; // this will change
	rect.top = dg;
	rect.bottom = dg + 20;
	temp = new uiItem(dxDevice, "evalObjsBar", debugView, UIT_button, vertexDecPCT, "un_shade.fx", "simpleUi", "ui/pure.tga", rect, &effects, &textures);
	temp->colMod = D3DXVECTOR4(1, 0.5, 0.5, 1);
	temp->enabled = true;
	temp->clickable = false;
	evalObjsBar = temp;

	rect.left = 15;
	rect.right = 190;
	rect.top = dg;
	rect.bottom = dg + 20;
	temp = new uiItem(dxDevice, "evalObjsLabel", debugView, UIT_text, rect, "~~ draw sprites time ~~", D3DCOLOR_ARGB(255, 0, 0, 128), uiFont);
	temp->enabled = true;
	temp->clickable = false;
	evalObjsLabel = temp;

	dg += 30;
	// draw time
	rect.left = 10;
	rect.right = 10; // this will change
	rect.top = dg;
	rect.bottom = dg + 20;
	temp = new uiItem(dxDevice, "drawTimeBar", debugView, UIT_button, vertexDecPCT, "un_shade.fx", "simpleUi", "ui/pure.tga", rect, &effects, &textures);
	temp->colMod = D3DXVECTOR4(0, 1, 0, 1);
	temp->enabled = true;
	temp->clickable = false;
	drawTimeBar = temp;

	rect.left = 15;
	rect.right = 190;
	rect.top = dg;
	rect.bottom = dg + 20;
	temp = new uiItem(dxDevice, "drawTimeLabel", debugView, UIT_text, rect, "~~ draw time ~~", D3DCOLOR_ARGB(255, 0, 0, 128), uiFont);
	temp->enabled = true;
	temp->clickable = false;
	drawTimeLabel = temp;

	dg += 25;
	// draw anims time
	rect.left = 10;
	rect.right = 10; // this will change
	rect.top = dg;
	rect.bottom = dg + 20;
	temp = new uiItem(dxDevice, "drawAnimsBar", debugView, UIT_button, vertexDecPCT, "un_shade.fx", "simpleUi", "ui/pure.tga", rect, &effects, &textures);
	temp->colMod = D3DXVECTOR4(0.5, 1, 0.5, 1);
	temp->enabled = true;
	temp->clickable = false;
	drawAnimsBar = temp;

	rect.left = 15;
	rect.right = 190;
	rect.top = dg;
	rect.bottom = dg + 20;
	temp = new uiItem(dxDevice, "drawAnimsLabel", debugView, UIT_text, rect, "~~ draw anims time ~~", D3DCOLOR_ARGB(255, 0, 0, 128), uiFont);
	temp->enabled = true;
	temp->clickable = false;
	drawAnimsLabel = temp;

	dg += 25;
	// draw update time
	rect.left = 10;
	rect.right = 10; // this will change
	rect.top = dg;
	rect.bottom = dg + 20;
	temp = new uiItem(dxDevice, "drawUpdateBar", debugView, UIT_button, vertexDecPCT, "un_shade.fx", "simpleUi", "ui/pure.tga", rect, &effects, &textures);
	temp->colMod = D3DXVECTOR4(0.5, 1, 0.5, 1);
	temp->enabled = true;
	temp->clickable = false;
	drawUpdateBar = temp;

	rect.left = 15;
	rect.right = 190;
	rect.top = dg;
	rect.bottom = dg + 20;
	temp = new uiItem(dxDevice, "drawUpdateLabel", debugView, UIT_text, rect, "~~ draw update time ~~", D3DCOLOR_ARGB(255, 0, 0, 128), uiFont);
	temp->enabled = true;
	temp->clickable = false;
	drawUpdateLabel = temp;

	dg += 25;
	// draw lights time
	rect.left = 10;
	rect.right = 10; // this will change
	rect.top = dg;
	rect.bottom = dg + 20;
	temp = new uiItem(dxDevice, "drawLightsBar", debugView, UIT_button, vertexDecPCT, "un_shade.fx", "simpleUi", "ui/pure.tga", rect, &effects, &textures);
	temp->colMod = D3DXVECTOR4(0.5, 1, 0.5, 1);
	temp->enabled = true;
	temp->clickable = false;
	drawLightsBar = temp;

	rect.left = 15;
	rect.right = 190;
	rect.top = dg;
	rect.bottom = dg + 20;
	temp = new uiItem(dxDevice, "drawLightsLabel", debugView, UIT_text, rect, "~~ draw lights time ~~", D3DCOLOR_ARGB(255, 0, 0, 128), uiFont);
	temp->enabled = true;
	temp->clickable = false;
	drawLightsLabel = temp;

	dg += 25;
	// draw terrain time
	rect.left = 10;
	rect.right = 10; // this will change
	rect.top = dg;
	rect.bottom = dg + 20;
	temp = new uiItem(dxDevice, "drawTerrainBar", debugView, UIT_button, vertexDecPCT, "un_shade.fx", "simpleUi", "ui/pure.tga", rect, &effects, &textures);
	temp->colMod = D3DXVECTOR4(0.5, 1, 0.5, 1);
	temp->enabled = true;
	temp->clickable = false;
	drawTerrainBar = temp;

	rect.left = 15;
	rect.right = 190;
	rect.top = dg;
	rect.bottom = dg + 20;
	temp = new uiItem(dxDevice, "drawTerrainLabel", debugView, UIT_text, rect, "~~ draw terrain time ~~", D3DCOLOR_ARGB(255, 0, 0, 128), uiFont);
	temp->enabled = true;
	temp->clickable = false;
	drawTerrainLabel = temp;

	dg += 25;
	// draw clouds time
	rect.left = 10;
	rect.right = 10; // this will change
	rect.top = dg;
	rect.bottom = dg + 20;
	temp = new uiItem(dxDevice, "drawCloudsBar", debugView, UIT_button, vertexDecPCT, "un_shade.fx", "simpleUi", "ui/pure.tga", rect, &effects, &textures);
	temp->colMod = D3DXVECTOR4(0.5, 1, 0.5, 1);
	temp->enabled = true;
	temp->clickable = false;
	drawCloudsBar = temp;

	rect.left = 15;
	rect.right = 190;
	rect.top = dg;
	rect.bottom = dg + 20;
	temp = new uiItem(dxDevice, "drawCloudsLabel", debugView, UIT_text, rect, "~~ draw cloud time ~~", D3DCOLOR_ARGB(255, 0, 0, 128), uiFont);
	temp->enabled = true;
	temp->clickable = false;
	drawCloudsLabel = temp;

	dg += 25;
	// draw objs time
	rect.left = 10;
	rect.right = 10; // this will change
	rect.top = dg;
	rect.bottom = dg + 20;
	temp = new uiItem(dxDevice, "drawObjsBar", debugView, UIT_button, vertexDecPCT, "un_shade.fx", "simpleUi", "ui/pure.tga", rect, &effects, &textures);
	temp->colMod = D3DXVECTOR4(0.5, 1, 0.5, 1);
	temp->enabled = true;
	temp->clickable = false;
	drawObjsBar = temp;

	rect.left = 15;
	rect.right = 190;
	rect.top = dg;
	rect.bottom = dg + 20;
	temp = new uiItem(dxDevice, "drawObjsLabel", debugView, UIT_text, rect, "~~ draw objs time ~~", D3DCOLOR_ARGB(255, 0, 0, 128), uiFont);
	temp->enabled = true;
	temp->clickable = false;
	drawObjsLabel = temp;

	dg += 25;
	// draw sprites time
	rect.left = 10;
	rect.right = 10; // this will change
	rect.top = dg;
	rect.bottom = dg + 20;
	temp = new uiItem(dxDevice, "drawSpritesBar", debugView, UIT_button, vertexDecPCT, "un_shade.fx", "simpleUi", "ui/pure.tga", rect, &effects, &textures);
	temp->colMod = D3DXVECTOR4(0.5, 1, 0.5, 1);
	temp->enabled = true;
	temp->clickable = false;
	drawSpritesBar = temp;

	rect.left = 15;
	rect.right = 190;
	rect.top = dg;
	rect.bottom = dg + 20;
	temp = new uiItem(dxDevice, "drawSpritesLabel", debugView, UIT_text, rect, "~~ draw sprites time ~~", D3DCOLOR_ARGB(255, 0, 0, 128), uiFont);
	temp->enabled = true;
	temp->clickable = false;
	drawSpritesLabel = temp;

	dg += 25;
	// draw over time
	rect.left = 10;
	rect.right = 10; // this will change
	rect.top = dg;
	rect.bottom = dg + 20;
	temp = new uiItem(dxDevice, "drawOverBar", debugView, UIT_button, vertexDecPCT, "un_shade.fx", "simpleUi", "ui/pure.tga", rect, &effects, &textures);
	temp->colMod = D3DXVECTOR4(0.5, 1, 0.5, 1);
	temp->enabled = true;
	temp->clickable = false;
	drawOverBar = temp;

	rect.left = 15;
	rect.right = 190;
	rect.top = dg;
	rect.bottom = dg + 20;
	temp = new uiItem(dxDevice, "drawOverLabel", debugView, UIT_text, rect, "~~ draw over time ~~", D3DCOLOR_ARGB(255, 0, 0, 128), uiFont);
	temp->enabled = true;
	temp->clickable = false;
	drawOverLabel = temp;

	dg += 25;
	// draw ui time
	rect.left = 10;
	rect.right = 10; // this will change
	rect.top = dg;
	rect.bottom = dg + 20;
	temp = new uiItem(dxDevice, "drawUiBar", debugView, UIT_button, vertexDecPCT, "un_shade.fx", "simpleUi", "ui/pure.tga", rect, &effects, &textures);
	temp->colMod = D3DXVECTOR4(0.5, 1, 0.5, 1);
	temp->enabled = true;
	temp->clickable = false;
	drawUiBar = temp;

	rect.left = 15;
	rect.right = 190;
	rect.top = dg;
	rect.bottom = dg + 20;
	temp = new uiItem(dxDevice, "drawUiLabel", debugView, UIT_text, rect, "~~ draw ui time ~~", D3DCOLOR_ARGB(255, 0, 0, 128), uiFont);
	temp->enabled = true;
	temp->clickable = false;
	drawUiLabel = temp;

	dg += 25;
	// draw present time
	rect.left = 10;
	rect.right = 10; // this will change
	rect.top = dg;
	rect.bottom = dg + 20;
	temp = new uiItem(dxDevice, "drawPresentBar", debugView, UIT_button, vertexDecPCT, "un_shade.fx", "simpleUi", "ui/pure.tga", rect, &effects, &textures);
	temp->colMod = D3DXVECTOR4(0.5, 1, 0.5, 1);
	temp->enabled = true;
	temp->clickable = false;
	drawPresentBar = temp;

	rect.left = 15;
	rect.right = 190;
	rect.top = dg;
	rect.bottom = dg + 20;
	temp = new uiItem(dxDevice, "drawPresentLabel", debugView, UIT_text, rect, "~~ draw present time ~~", D3DCOLOR_ARGB(255, 0, 0, 128), uiFont);
	temp->enabled = true;
	temp->clickable = false;
	drawPresentLabel = temp;



	// generic debug view
		// debug view
	rect.left = 400;
	rect.right = 620;
	rect.top = 45;
	rect.bottom = 500;
	temp = new uiItem(dxDevice, "debugView", NULL, UIT_button, vertexDecPCT, "un_shade.fx", "simpleUi", "ui/bland.tga", rect, &effects, &textures);
	temp->enabled = false;
	temp->clickable = false;
	uiItems.push_back(temp);
	temp->colMod = D3DXVECTOR4(0, 0.5, 1, 0.5);
	genericDebugView = temp;

	// vlines
	rect.left = 9;
	rect.right = 10;
	rect.top = 10;
	rect.bottom = debugView->rect.bottom - debugView->rect.top - 10;
	temp = new uiItem(dxDevice, "vlineLeft", genericDebugView, UIT_button, vertexDecPCT, "un_shade.fx", "simpleUi", "ui/pure.tga", rect, &effects, &textures);
	temp->colMod = D3DXVECTOR4(0, 0, 0, 1);
	temp->enabled = true;
	temp->clickable = false;

	rect.left = debugView->rect.right - debugView->rect.left - 10;
	rect.right = debugView->rect.right - debugView->rect.left - 9;
	rect.top = 10;
	rect.bottom = debugView->rect.bottom - debugView->rect.top - 10;
	temp = new uiItem(dxDevice, "vlineLeft", genericDebugView, UIT_button, vertexDecPCT, "un_shade.fx", "simpleUi", "ui/pure.tga", rect, &effects, &textures);
	temp->colMod = D3DXVECTOR4(0, 0, 0, 1);
	temp->enabled = true;
	temp->clickable = false;

	dg = 10 - 30;
	for (int i = 0; i < genericUiCount; i++)
	{
		dg += 30;
		// frame time
		rect.left = 10;
		rect.right = 10; // this will change
		rect.top = dg;
		rect.bottom = dg + 20;
		temp = new uiItem(dxDevice, "", genericDebugView, UIT_button, vertexDecPCT, "un_shade.fx", "simpleUi", "ui/pure.tga", rect, &effects, &textures);
		temp->colMod = D3DXVECTOR4(1, 1, 1, 1);
		temp->enabled = true;
		temp->clickable = false;
		genericBar[i] = temp;

		rect.left = 15;
		rect.right = 190;
		rect.top = dg;
		rect.bottom = dg + 20;
		temp = new uiItem(dxDevice, "", genericDebugView, UIT_text, rect, "~~ clint ~~", D3DCOLOR_ARGB(255, 0, 0, 128), uiFont);
		temp->enabled = true;
		temp->clickable = false;
		genericLabel[i] = temp;
	}
}

void initEffect(LPDIRECT3DDEVICE9 dxDevice)
{
	LPD3DXBUFFER errs = NULL;
	HRESULT res = D3DXCreateEffectFromFile(dxDevice, "un_shade.fx", NULL, NULL, 0, NULL, &effectA, &errs);

	if (errs != NULL)
	{
		LPVOID meh = errs->GetBufferPointer();
		int mehhem = (int)errs->GetBufferSize();

		if (res == D3DERR_INVALIDCALL)
		{
			mehhem++;
		}
		if (res == D3DXERR_INVALIDDATA)
		{
			mehhem++;
		}
		if (res == E_OUTOFMEMORY)
		{
			mehhem++;
		}
		if (res == D3D_OK)
		{
			mehhem++;
		}
		char* mehb = new char[mehhem];
		memcpy(mehb, meh, mehhem);

		std::cout << mehb;
		mehhem++;
	}

	effectA->SetTechnique("simple");


	// now do aeffect

	aeffect = createEffect(dxDevice, "un_shade.fx", VX_PCT, "un_shade.fx", &effects);
}

int rnd(int top)
{
	return rand() % top;
}

void runAnims()
{
	for (int i = objs.size() - 1; i >= 0; i--)
	{
		objs[i]->run();
	}
}

void initObjs(LPDIRECT3DDEVICE9 dxDevice)
{
	D3DXVECTOR3 simpleDir;
	D3DXVECTOR3 simpleLoc;

	UNCRZ_FBF_anim* mapAnim = getFBF_anim(&anims, "mapIdle");
	UNCRZ_FBF_anim* meAnim = getFBF_anim(&anims, "walk");
	UNCRZ_FBF_anim* cloudAnim = getFBF_anim(&anims, "cloudsIdle");

	mapObj = new UNCRZ_obj(getModel(&models, "map"));
	mapObj->changeAnim(mapAnim);
	meObj = new UNCRZ_obj(getModel(&models, "human"));
	meObj->changeAnim(meAnim);
	cloudObj = new UNCRZ_obj(getModel(&models, "clouds"));
	cloudObj->changeAnim(cloudAnim);

	zSortedObjs.push_back(meObj);
	objs.push_back(meObj);
	meObj->offset = D3DXVECTOR3(0, 100, 0);
}

void initSprites(LPDIRECT3DDEVICE9 dxDevice)
{
	sbuff = UNCRZ_sprite_buffer();
	sbuff.create(dxDevice, vertexDecPCT, 50);

	fireSprite = sprites[0];
	smokeSprite = sprites[1];
}

void disableClip(LPDIRECT3DDEVICE9);
void setClip(LPDIRECT3DDEVICE9, float, bool);

void drawZBackToFront(std::vector<UNCRZ_obj*>& objList, std::vector<int>& mol, LPDIRECT3DDEVICE9 dxDevice, drawData* ddat, DWORD drawArgs) // mutable objs list
{
	int osize = (int)objList.size();
	int msize = (int)mol.size();
	if (msize > osize)
	{
		// lost items somewhere (current implmentation is poor)
		mol.resize(osize);
		
		for (int i = 0; i < osize; i++)
		{
			mol[i] = i;
		}
	}
	else if (msize < osize)
	{
		// rarely going to be adding more than 1 item when speed matters
		for (int i = msize; i < osize; i++)
		{
			mol.insert(mol.begin(), i);
		}
	}

	int top = osize - 1;

	if (top < 0)
		return;

	for (int i = top; i >= 0; i--)
	{
		objList[mol[i]]->zSortDepth = getDistNoSqrt(objList[mol[i]]->offset.x, objList[mol[i]]->offset.y, objList[mol[i]]->offset.z, camPos.x, camPos.y, camPos.z);
	}
	
	bool noSwaps = false;
	int curTop = top + 1;
	int curIdx;

next:
	curTop--;

	curIdx = mol[0];

	noSwaps = true;
	for (int i = 0; i < curTop; i++)
	{
		if (objList[mol[i + 1]]->zSortDepth < objList[curIdx]->zSortDepth)
		{ // swap
			noSwaps = false;
			mol[i] = mol[i + 1];
			mol[i + 1] = curIdx;
		}
		else
		{
			curIdx = mol[i + 1];
		}
	}

	objList[curIdx]->draw(dxDevice, ddat, drawArgs);

	if (noSwaps || curTop <= 0)
		goto sorted;
	goto next;

sorted:
	for (int i = curTop; i >= 0; i--)
	{
		objList[mol[i]]->draw(dxDevice, ddat, drawArgs);
	}
}

//void drawZBackToFront(std::vector<UNCRZ_obj*>& mol, LPDIRECT3DDEVICE9 dxDevice, drawData* ddat, DWORD drawArgs) // mutable objs list
//{
//	int top = (int)mol.size() - 1;
//
//	if (top < 0)
//		return;
//
//	for (int i = top; i >= 0; i--)
//	{
//		mol[i]->zSortDepth = getDistNoSqrt(mol[i]->offset.x, mol[i]->offset.y, mol[i]->offset.z, camPos.x, camPos.y, camPos.z);
//	}
//	
//	bool noSwaps = false;
//	int curTop = top + 1;
//	UNCRZ_obj* curObj;
//
//next:
//	curTop--;
//
//	curObj = mol[0];
//
//	noSwaps = true;
//	for (int i = 0; i < curTop; i++)
//	{
//		if (mol[i + 1]->zSortDepth < curObj->zSortDepth)
//		{ // swap
//			noSwaps = false;
//			mol[i] = mol[i + 1];
//			mol[i + 1] = curObj;
//		}
//		else
//		{
//			curObj = mol[i + 1];
//		}
//	}
//
//	curObj->draw(dxDevice, ddat, drawArgs);
//
//	if (noSwaps || curTop <= 0)
//		goto sorted;
//	goto next;
//
//sorted:
//	for (int i = curTop; i >= 0; i--)
//	{
//		mol[i]->draw(dxDevice, ddat, drawArgs);
//	}
//}

// takes from side target
void drawTargetOverFinalTarget(LPDIRECT3DDEVICE9 dxDevice, drawData* ddat, UNCRZ_effect* effect)
{
	D3DXMATRIX idMat;
	D3DXMatrixIdentity(&idMat);
	dxDevice->SetRenderTarget(0, ddat->targetSurface);
	dxDevice->SetViewport(ddat->targetVp);

	dxDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
	dxDevice->SetRenderState(D3DRS_ZENABLE, false);
	dxDevice->SetRenderState(D3DRS_ZWRITEENABLE, false);

	vertexOver overVerts[4]; // make this const / VBuff
	overVerts[0] = vertexOver(-1, -1, 0, 0, 1);
	overVerts[1] = vertexOver(-1, 1, 0, 0, 0);
	overVerts[2] = vertexOver(1, -1, 0, 1, 1);
	overVerts[3] = vertexOver(1, 1, 0, 1, 0);

	D3DXVECTOR4 texData = D3DXVECTOR4(0.5 / (float)ddat->targetVp->Width, 0.5 / (float)ddat->targetVp->Height, 1.0 / (float)ddat->targetVp->Width, 1.0 / (float)ddat->targetVp->Height);
	effect->setTextureData((float*)&texData.x);
	
	for (int i = 0; i < 4; i++) // do ahead of shader
	{
		overVerts[i].tu += texData.x;
		overVerts[i].tv += texData.y;
	}

	effect->setEyePos(&ddat->eyePos);
	effect->setEyeDir(&ddat->eyeDir);
	effect->setTicker(ddat->ticker);
	effect->setTexture(ddat->sideTex);
	effect->effect->SetTechnique("over_final");
	effect->setViewProj(&idMat);
	effect->effect->CommitChanges();

	dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

	UINT numPasses, pass;
	effect->effect->Begin(&numPasses, 0);
	effect->effect->BeginPass(0);

	dxDevice->SetVertexDeclaration(vertexDecOver);
	dxDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, overVerts, sizeof(vertexOver));

	effect->effect->EndPass();
	effect->effect->End();
	
	dxDevice->SetRenderState(D3DRS_ZENABLE, true);
	dxDevice->SetRenderState(D3DRS_ZWRITEENABLE, true);
}

void drawUi(LPDIRECT3DDEVICE9 dxDevice)
{
	// need to update a few labels
	char buff[500];

	// update debug stuff
	if (debugData)
	{
		debugView->enabled = true;
		fpsLabel->text = "  FPS: " + std::string(itoa(fps, (char*)&buff, 10));

		frameTimeLabel->text = " Frame:    " + std::string(itoa((int)(hrframeTime * 1000.0), (char*)&buff, 10)) + "ms";
		frameTimeBar->rect.right = 10 + (int)((float)(debugView->rect.right - debugView->rect.left - 20) * hrframeTime);

		evalTimeLabel->text = " Eval:     " + std::string(itoa((int)(hrevalTime * 1000.0), (char*)&buff, 10)) + "ms";
		evalTimeBar->rect.right = 10 + (int)((float)(debugView->rect.right - debugView->rect.left - 20) * hrevalTime);
		//evalPlacingLabel->text = "Placing:  " + std::string(itoa((int)(hrevalPlacingTime / hrevalTime * 100.0), (char*)&buff, 10)) + "%";
		//evalPlacingBar->rect.right = 10 + (int)((float)(debugView->rect.right - debugView->rect.left - 20) * hrevalPlacingTime / hrevalTime);
		evalSpritesLabel->text = "Sprites:  " + std::string(itoa((int)(hrevalSpritesTime / hrevalTime * 100.0), (char*)&buff, 10)) + "%";
		evalSpritesBar->rect.right = 10 + (int)((float)(debugView->rect.right - debugView->rect.left - 20) * hrevalSpritesTime / hrevalTime);
		evalObjsLabel->text = "Objs:     " + std::string(itoa((int)(hrevalObjsTime / hrevalTime * 100.0), (char*)&buff, 10)) + "%";
		evalObjsBar->rect.right = 10 + (int)((float)(debugView->rect.right - debugView->rect.left - 20) * hrevalObjsTime / hrevalTime);

		drawTimeLabel->text = " Draw:     " + std::string(itoa((int)(hrdrawTime * 1000.0), (char*)&buff, 10)) + "ms";
		drawTimeBar->rect.right = 10 + (int)((float)(debugView->rect.right - debugView->rect.left - 20) * hrdrawTime);
		drawAnimsLabel->text = "Anims:    " + std::string(itoa((int)(hrdrawAnimsTime / hrdrawTime * 100.0), (char*)&buff, 10)) + "%";
		drawAnimsBar->rect.right = 10 + (int)((float)(debugView->rect.right - debugView->rect.left - 20) * hrdrawAnimsTime / hrdrawTime);
		drawUpdateLabel->text = "Update:   " + std::string(itoa((int)(hrdrawUpdateTime / hrdrawTime * 100.0), (char*)&buff, 10)) + "%";
		drawUpdateBar->rect.right = 10 + (int)((float)(debugView->rect.right - debugView->rect.left - 20) * hrdrawUpdateTime / hrdrawTime);
		drawLightsLabel->text = "Lights:   " + std::string(itoa((int)(hrdrawLightsTime / hrdrawTime * 100.0), (char*)&buff, 10)) + "%";
		drawLightsBar->rect.right = 10 + (int)((float)(debugView->rect.right - debugView->rect.left - 20) * hrdrawLightsTime / hrdrawTime);
		drawLightsLabel->enabled = debugFlushing;
		drawLightsBar->enabled = debugFlushing;
		drawTerrainLabel->text = "Terrain:  " + std::string(itoa((int)(hrdrawTerrainTime / hrdrawTime * 100.0), (char*)&buff, 10)) + "%";
		drawTerrainBar->rect.right = 10 + (int)((float)(debugView->rect.right - debugView->rect.left - 20) * hrdrawTerrainTime / hrdrawTime);
		drawTerrainLabel->enabled = debugFlushing;
		drawTerrainBar->enabled = debugFlushing;
		drawCloudsLabel->text = "Clouds:  " + std::string(itoa((int)(hrdrawCloudsTime / hrdrawTime * 100.0), (char*)&buff, 10)) + "%";
		drawCloudsBar->rect.right = 10 + (int)((float)(debugView->rect.right - debugView->rect.left - 20) * hrdrawCloudsTime / hrdrawTime);
		drawCloudsLabel->enabled = debugFlushing;
		drawCloudsBar->enabled = debugFlushing;
		drawObjsLabel->text = "Objs:     " + std::string(itoa((int)(hrdrawObjsTime / hrdrawTime * 100.0), (char*)&buff, 10)) + "%";
		drawObjsBar->rect.right = 10 + (int)((float)(debugView->rect.right - debugView->rect.left - 20) * hrdrawObjsTime / hrdrawTime);
		drawObjsLabel->enabled = debugFlushing;
		drawObjsBar->enabled = debugFlushing;
		drawSpritesLabel->text = "Sprites:  " + std::string(itoa((int)(hrdrawSpritesTime / hrdrawTime * 100.0), (char*)&buff, 10)) + "%";
		drawSpritesBar->rect.right = 10 + (int)((float)(debugView->rect.right - debugView->rect.left - 20) * hrdrawSpritesTime / hrdrawTime);
		drawSpritesLabel->enabled = debugFlushing;
		drawSpritesBar->enabled = debugFlushing;
		drawOverLabel->text = "Over:     " + std::string(itoa((int)(hrdrawOverTime / hrdrawTime * 100.0), (char*)&buff, 10)) + "%";
		drawOverBar->rect.right = 10 + (int)((float)(debugView->rect.right - debugView->rect.left - 20) * hrdrawOverTime / hrdrawTime);
		drawOverLabel->enabled = debugFlushing;
		drawOverBar->enabled = debugFlushing;
		drawUiLabel->text = "Ui:       " + std::string(itoa((int)(hrdrawUiTime / hrdrawTime * 100.0), (char*)&buff, 10)) + "%";
		drawUiBar->rect.right = 10 + (int)((float)(debugView->rect.right - debugView->rect.left - 20) * hrdrawUiTime / hrdrawTime);
		drawUiLabel->enabled = debugFlushing;
		drawUiBar->enabled = debugFlushing;
		drawPresentLabel->text = "Present:  " + std::string(itoa((int)(hrdrawPresentTime / hrdrawTime * 100.0), (char*)&buff, 10)) + "%";
		drawPresentBar->rect.right = 10 + (int)((float)(debugView->rect.right - debugView->rect.left - 20) * hrdrawPresentTime / hrdrawTime);
		
		genericDebugView->enabled = true;
	}
	else
	{
		debugView->enabled = false;
		
		genericDebugView->enabled = false;
	}

	// down to drawing
	dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	dxDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
	dxDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	dxDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

	dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

	dxDevice->SetRenderState(D3DRS_ZENABLE, false);
	dxDevice->SetRenderState(D3DRS_ZWRITEENABLE, false);

	dxDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	for each (uiItem* uii in uiItems)
	{
		if (uii->enabled)
			uii->draw(dxDevice, 0, 0, &mainVt);
	}

	// disable all the generics
	for (int i = 0; i < genericUiCount; i++)
	{
		genericLabel[i]->enabled = false;
		genericBar[i]->enabled = false;
	}
}

void drawFrame(LPDIRECT3DDEVICE9 dxDevice)
{
	// reset some stuff (these all do cumulative timing (because they can be done more than once per frame);
	DEBUG_HR_ZERO(&hrdrawCloudsTime);
	DEBUG_HR_ZERO(&hrdrawObjsTime);
	DEBUG_HR_ZERO(&hrdrawSpritesTime);
	DEBUG_HR_ZERO(&hrdrawTerrainTime);

	// this hasn't been organised yet
	dxDevice->SetRenderState(D3DRS_ZENABLE, true);
	dxDevice->SetRenderState(D3DRS_ZWRITEENABLE, true);

	float lightCoof = 0.7f;

	DEBUG_HR_START(&hrsbstart);
	// anims
	runAnims();
	DEBUG_HR_END(&hrsbstart, &hrsbend, &hrdrawAnimsTime);

	getSeg(meObj->model, "head")->rotation.z = rotPar;
	//getSeg(meObj->model, "head")->rotation.y = rotY;

	// update models
	DEBUG_HR_START(&hrsbstart);
	//cloudObj->offset = camPos;
	//cloudObj->offset.y -= 400;
	cloudObj->update(true);
	cloudObj->model->noCull = true;
	mapObj->update(true);
	mapObj->model->noCull = true;
	for (int i = objs.size() - 1; i >= 0; i--)
	{
		objs[i]->update();
	}
	DEBUG_HR_END(&hrsbstart, &hrsbend, &hrdrawUpdateTime);

	DEBUG_HR_START(&hrsbstart);
	// drawLight
	for (int i = lights.size() - 1; i >= 0; i--)
	{
		if (lights[i]->lightEnabled)
			drawLight(dxDevice, lights[i]);
	}
	DEBUG_DX_FLUSH();
	DEBUG_HR_END(&hrsbstart, &hrsbend, &hrdrawLightsTime);

	// prep DynamicDecals
	for (int i = dynamicDecals.size() - 1; i >= 0; i--)
	{
		dynamicDecalData* ddd = dynamicDecals[i];
		if (ddd->decalEnabled)
		{
			prepDynamicDecal(dxDevice, ddd);
		}
	}

	D3DVIEWPORT9 vp = createViewPort(targetTexScale);
	dxDevice->SetViewport(&vp);

	if (wireFrame)
		dxDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
	else
		dxDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);

	// drawView
	for (int i = views.size() - 1; i >= 0; i--)
	{
		if (views[i]->viewEnabled)
		{
			moveCameraView(dxDevice, views[i]);
			drawData viewDdat = createDrawDataView(lightCoof, &vp, views[i]);
			drawScene(dxDevice, &viewDdat, views[i], DF_default, SF_default); // timed
		}
	}

	dxDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);

	// drawOver
	for (int i = overs.size() - 1; i >= 0; i--)
	{
		if (overs[i]->overEnabled)
		{
			drawData overDdat = createDrawDataOver(lightCoof, &vp, overs[i]);
			drawOver(dxDevice, &overDdat, overs[i]); // timed
		}
	}

	if (!disableOpenTarget)
	{
		// draw scene
		moveCamera(dxDevice);
		dxDevice->SetRenderTarget(0, targetSurface);
		dxDevice->SetDepthStencilSurface(zSurface);

		dxDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 100, 200), 1.0f, 0); // don't really need this
		dxDevice->Clear(0, NULL, D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
		dxDevice->SetRenderState(D3DRS_ZENABLE, true);
		dxDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
		dxDevice->BeginScene();

		if (wireFrame)
			dxDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
		else
			dxDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);

		D3DXMATRIX meh;
		D3DXMatrixIdentity(&meh);
		UINT numPasses, pass;

		drawData ddat = createDrawData(lightCoof, &vp);
		
		// special version of drawScene (because the main camera doesn't take a UNCRZ_view* yet)
		DEBUG_HR_START(&hrsbstart);
		cloudObj->draw(dxDevice, &ddat, DF_default);
		DEBUG_DX_FLUSH();
		DEBUG_HR_ACCEND(&hrsbstart, &hrsbend, &hrdrawCloudsTime);

		DEBUG_HR_START(&hrsbstart);
		mapObj->draw(dxDevice, &ddat, DF_default);
		DEBUG_DX_FLUSH();
		DEBUG_HR_ACCEND(&hrsbstart, &hrsbend, &hrdrawTerrainTime);

		DEBUG_HR_START(&hrsbstart);
		drawZBackToFront(zSortedObjs, zsoLocalIndexes, dxDevice, &ddat, DF_default);
		DEBUG_DX_FLUSH();
		DEBUG_HR_ACCEND(&hrsbstart, &hrsbend, &hrdrawObjsTime);

		DEBUG_HR_START(&hrsbstart);
		// colour
		if (fireSprites.size() > 0)
			fireSprite->draw(dxDevice, &ddat, &sbuff, &fireSprites.front(), 0, fireSprites.size(), DF_default, SD_colour);
		if (smokeSprites.size() > 0)
			smokeSprite->draw(dxDevice, &ddat, &sbuff, &smokeSprites.front(), 0, smokeSprites.size(), DF_default, SD_colour);
		// alpha
		if (fireSprites.size() > 0)
			fireSprite->draw(dxDevice, &ddat, &sbuff, &fireSprites.front(), 0, fireSprites.size(), DF_default, SD_alpha);
		if (smokeSprites.size() > 0)
			smokeSprite->draw(dxDevice, &ddat, &sbuff, &smokeSprites.front(), 0, smokeSprites.size(), DF_default, SD_alpha);
		DEBUG_DX_FLUSH();
		DEBUG_HR_ACCEND(&hrsbstart, &hrsbend, &hrdrawSpritesTime);

		// reset D3DRS_ZWRITEENABLE
		dxDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);

		// while we still have acces to the ddat
		ddat.nontimed(2, "CulledObjs: ", (float)ddat.cullCount / (float)(ddat.cullCount + ddat.drawCount), GDOF_prop100);


		// draw waste of space light indicators
		/*terrain->effect.effect->Begin(&numPasses, 0);
		terrain->effect.effect->BeginPass(0);

		terrain->effect.setViewProj(&viewProj);
		terrain->effect.effect->CommitChanges();

		dxDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
		dxDevice->SetVertexDeclaration(vertexDecPC);
		vertexPC lVs[3];
		for (int i = lights.size() - 1; i >= 0; i--)
		{
			D3DXVECTOR3 lPos = D3DXVECTOR3(lights[i]->lightPos.x, lights[i]->lightPos.y, lights[i]->lightPos.z);
			lVs[0] = vertexPC(lPos, D3DXVECTOR3(1.0, 0.0, 0.0), -1);
			lVs[1] = vertexPC(lPos + D3DXVECTOR3(lights[i]->lightDir.x, lights[i]->lightDir.y, lights[i]->lightDir.z) , D3DXVECTOR3(0.0, 1.0, 0.0), -1);
			lVs[2] = vertexPC(lPos + lights[i]->lightUp, D3DXVECTOR3(0.0, 0.0, 1.0), -1);
			
			dxDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 1, &(lVs[0]), sizeof(vertexPC));
		}

		terrain->effect.effect->EndPass();	
		terrain->effect.effect->End();*/




		// from the old days of testing
		//for (int i = objs.size() - 1; i >= 1; i--)
		//{
		//	objs[i]->model->drawBBoxDebug(dxDevice, &ddat);
		//}



		// normals
		/*dxDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

		terrain->effect.effect->SetTechnique("dull");
		terrain->effect.setViewProj(&viewProj);
		terrain->effect.effect->CommitChanges();

		terrain->effect.effect->Begin(&numPasses, 0);

		dxDevice->SetVertexDeclaration(vertexDecPC);

		for (pass = 0; pass < numPasses; pass++)
		{
			terrain->effect.effect->BeginPass(pass);

			dxDevice->DrawPrimitiveUP(D3DPT_LINELIST, (UINT)normalVis.size() / 2, (vertexPC*)&normalVis.front(), sizeof(vertexPC));

			terrain->effect.effect->EndPass();
		}
		terrain->effect.effect->End();*/

		dxDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);

		dxDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

		DEBUG_HR_START(&hrsbstart);
		ddat.sideSurface = targetSurface;
		ddat.sideTex = targetTex;
		ddat.targetSurface = finalTargetSurface;
		*ddat.targetVp = createViewPort(1.0); // this ensures the final draw and UI draws are done with a suitable VP

		aeffect.effect->SetTechnique(aeffect.effect->GetTechniqueByName("simple"));
		drawTargetOverFinalTarget(dxDevice, &ddat, &aeffect);

		DEBUG_DX_FLUSH();
		DEBUG_HR_END(&hrsbstart, &hrsbend, &hrdrawOverTime);

	}
	else // if (disableOpenTarget)
	{
		// this bit replicates init that drawTargetOverFinalTarget() would do otherwise
		/*D3DVIEWPORT9 finalVp = createViewPort(1.0);

		//D3DXMATRIX idMat;
		//D3DXMatrixIdentity(&idMat);
		dxDevice->SetRenderTarget(0, finalTargetSurface);
		dxDevice->SetDepthStencilSurface(zSurface);

		dxDevice->SetViewport(&finalVp);

		dxDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 100, 200), 1.0f, 0);
		dxDevice->Clear(0, NULL, D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

		dxDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
		dxDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
		dxDevice->SetRenderState(D3DRS_ZENABLE, false);
		dxDevice->SetRenderState(D3DRS_ZWRITEENABLE, false);

		dxDevice->BeginScene();*/

		moveCamera(dxDevice);

		dxDevice->SetRenderTarget(0, finalTargetSurface);
		dxDevice->SetDepthStencilSurface(zSurface);

		dxDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 100, 200), 1.0f, 0);
		dxDevice->Clear(0, NULL, D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
		dxDevice->SetRenderState(D3DRS_ZENABLE, false);
		dxDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
		dxDevice->BeginScene();

	}

	DEBUG_HR_START(&hrsbstart);
	drawUi(dxDevice);
	DEBUG_DX_FLUSH();
	DEBUG_HR_END(&hrsbstart, &hrsbend, &hrdrawUiTime);


	dxDevice->GetViewport(&vp);
	RECT crect;
	GetClientRect(mainHWnd, &crect);
	int winWidth = crect.right - crect.left;
	int winHeight = crect.bottom - crect.top;
	mainVt = viewTrans(vpWidth, vpHeight, winWidth, winHeight);


	dxDevice->EndScene();

	DEBUG_HR_START(&hrsbstart);
	dxDevice->Present(NULL, NULL, NULL, NULL);
	DEBUG_HR_END(&hrsbstart, &hrsbend, &hrdrawPresentTime);
}

void createVPMatrix(D3DXMATRIX* out, int x, int y, int w, int h, float minZ, float maxZ)
{
	*out = D3DXMATRIX(
		(float)w * 0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, (float)h * -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, maxZ - minZ, 0.0f,
		(float)w * 0.5f + x, (float)h * 0.5f + y, minZ, 1.0f);
}

drawData createDrawData(float lightCoof, D3DVIEWPORT9* vp)
{
	drawData ddat = drawData(&viewProj, &viewMatrix, &projMatrix, lights, dynamicDecals, lightCoof);

	ddat.performPlainPass = true;
	
	ddat.eyePos = eyePos;
	ddat.eyeDir = eyeDir;
	ddat.ticker = ticker;

	ddat.targetVp = vp;
	ddat.sideVp = new D3DVIEWPORT9();
	*ddat.sideVp = createViewPort(targetTexScale);
	
	ddat.targetTex = targetTex;
	ddat.targetSurface = targetSurface;
	ddat.sideTex = sideTex;
	ddat.sideSurface = sideSurface;

	ddat.zSideSurface = zSideSurface;
	ddat.zTargetSurface = zSurface;

	// debugging/hr
	ddat.hrsec = hrsec;
	ddat.debugData = debugData;
	ddat.debugFlushing = debugFlushing;
	ddat.flushQuery = flushQuery;
	ddat.genericDebugView = genericDebugView;
	ddat.genericLabel = genericLabel;
	ddat.genericBar = genericBar;

	return ddat;
}

drawData createDrawDataView(float lightCoof, D3DVIEWPORT9* vp, UNCRZ_view* view)
{
	drawData ddat = drawData(&viewProj, &viewMatrix, &projMatrix, lights, dynamicDecals, lightCoof);

	ddat.performPlainPass = true;
	
	ddat.eyePos = eyePos; // err
	ddat.eyeDir = eyeDir; // err
	ddat.ticker = ticker;

	ddat.targetVp = vp;
	ddat.sideVp = new D3DVIEWPORT9();
	*ddat.sideVp = createViewPort(targetTexScale);
	
	ddat.targetTex = view->targetTex;
	ddat.targetSurface = view->targetSurface;
	ddat.sideTex = sideTex;
	ddat.sideSurface = sideSurface;

	ddat.zSideSurface = zSideSurface;
	ddat.zTargetSurface = zSurface;

	// debugging/hr
	ddat.hrsec = hrsec;
	ddat.debugData = debugData;
	ddat.debugFlushing = debugFlushing;
	ddat.flushQuery = flushQuery;
	ddat.genericDebugView = genericDebugView;
	ddat.genericLabel = genericLabel;
	ddat.genericBar = genericBar;

	return ddat;
}

// this can be stripped considerably... (and createDrawDataView too
drawData createDrawDataOver(float lightCoof, D3DVIEWPORT9* vp, UNCRZ_over* over)
{
	drawData ddat = drawData(&viewProj, &viewMatrix, &projMatrix, lights, dynamicDecals, lightCoof);

	ddat.performPlainPass = true;
	
	ddat.eyePos = eyePos; // err
	ddat.eyeDir = eyeDir; // err
	ddat.ticker = ticker;

	ddat.targetVp = vp;
	ddat.sideVp = new D3DVIEWPORT9();
	*ddat.sideVp = createViewPort(targetTexScale);
	
	ddat.targetTex = over->targetTex;
	ddat.targetSurface = over->targetSurface;
	ddat.sideTex = sideTex;
	ddat.sideSurface = sideSurface;

	ddat.zSideSurface = zSideSurface;
	ddat.zTargetSurface = zSurface;

	// debugging/hr
	ddat.hrsec = hrsec;
	ddat.debugData = debugData;
	ddat.debugFlushing = debugFlushing;
	ddat.flushQuery = flushQuery;
	ddat.genericDebugView = genericDebugView;
	ddat.genericLabel = genericLabel;
	ddat.genericBar = genericBar;

	return ddat;
}

D3DVIEWPORT9 createViewPort(float scale)
{
	D3DVIEWPORT9 vp;
	vp.X = 0;
	vp.Y = 0;
	vp.Width = (float)vpWidth * scale;
	vp.Height = (float)vpHeight * scale;
	vp.MaxZ = 1.0f;
	vp.MinZ = 0.0f;

	return vp;
}

D3DVIEWPORT9 createViewPort(UINT w, UINT h)
{
	D3DVIEWPORT9 vp;
	vp.X = 0;
	vp.Y = 0;
	vp.Width = w;
	vp.Height = h;
	vp.MaxZ = 1.0f;
	vp.MinZ = 0.0f;

	return vp;
}

void disableClip(LPDIRECT3DDEVICE9 dxDevice)
{
	dxDevice->SetRenderState(D3DRS_CLIPPING, false);
}

void setClip(LPDIRECT3DDEVICE9 dxDevice, float height, bool above)
{
	D3DXVECTOR4 planeCoofs;
	if (above)
		planeCoofs = D3DXVECTOR4(0.0f, -1.0f, 0.0f, height);
	else
		planeCoofs = D3DXVECTOR4(0.0f, 1.0f, 0.0f, -height);
	D3DXMATRIX viewProjInv;
	D3DXMatrixInverse(&viewProjInv, NULL, &viewProj);
	D3DXMatrixTranspose(&viewProjInv, &viewProjInv);
	D3DXVec4Transform(&planeCoofs, &planeCoofs, &viewProjInv);

	D3DXPLANE plane((const float*)&planeCoofs);

	dxDevice->SetClipPlane(0, (const float*)&plane);
	dxDevice->SetRenderState(D3DRS_CLIPPLANEENABLE, D3DCLIPPLANE0);
	dxDevice->SetRenderState(D3DRS_CLIPPING, true);
}

void setAlpha(LPDIRECT3DDEVICE9 dxDevice, DWORD alphaMode)
{
	switch (alphaMode)
	{
	case AM_none:
		dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		break;
	case AM_nice:
		dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		dxDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
		dxDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		dxDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		break;
	case AM_add:
		dxDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		dxDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
		dxDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		dxDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
		break;
	}
}

// does some actual drawing
void drawScene(LPDIRECT3DDEVICE9 dxDevice, drawData* ddat, UNCRZ_view* view, DWORD drawArgs, DWORD sceneArgs)
{
	dxDevice->SetRenderTarget(0, view->targetSurface);
	dxDevice->SetDepthStencilSurface(view->zSurface);

	D3DVIEWPORT9 vp = createViewPort(view->texWidth, view->texHeight);
	view->viewViewPort = vp;
	dxDevice->SetViewport(&vp);

	if (view->clearView)
		dxDevice->Clear(0, NULL, D3DCLEAR_TARGET, view->clearColor, 1.0f, 0);
	dxDevice->Clear(0, NULL, D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
	dxDevice->SetRenderState(D3DRS_ZENABLE, true);
	dxDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);

	setAlpha(dxDevice, view->alphaMode);

	dxDevice->BeginScene();

	if (sceneArgs & SF_notimed)
	{
		if ((sceneArgs & SF_noclouds) == false)
			cloudObj->draw(dxDevice, ddat, drawArgs);
		mapObj->draw(dxDevice, ddat, drawArgs);
		drawZBackToFront(zSortedObjs, view->zsoLocalIndexes, dxDevice, ddat, drawArgs);

		if (fireSprites.size() > 0)
		{
			fireSprite->draw(dxDevice, ddat, &sbuff, &fireSprites.front(), 0, fireSprites.size(), drawArgs, SD_default);
		}
		if (smokeSprites.size() > 0)
		{
			smokeSprite->draw(dxDevice, ddat, &sbuff, &smokeSprites.front(), 0, smokeSprites.size(), drawArgs, SD_default);
		}

		// not asif we need these or anything
		if (smokeSprites.size() > 0)
		{
			fireSprite->draw(dxDevice, ddat, &sbuff, &fireSprites.front(), 0, fireSprites.size(), drawArgs, SD_alpha);
		}
		if (smokeSprites.size() > 0)
		{
			smokeSprite->draw(dxDevice, ddat, &sbuff, &smokeSprites.front(), 0, smokeSprites.size(), drawArgs, SD_alpha);
		}
	}
	else
	{
		if ((sceneArgs & SF_noclouds) == false)
		{
			DEBUG_HR_START(&hrsbstart);
			cloudObj->draw(dxDevice, ddat, drawArgs);
			DEBUG_DX_FLUSH();
			DEBUG_HR_ACCEND(&hrsbstart, &hrsbend, &hrdrawCloudsTime);
		}

		DEBUG_HR_START(&hrsbstart);
		mapObj->draw(dxDevice, ddat, drawArgs);
		DEBUG_DX_FLUSH();
		DEBUG_HR_ACCEND(&hrsbstart, &hrsbend, &hrdrawTerrainTime);

		DEBUG_HR_START(&hrsbstart);
		drawZBackToFront(zSortedObjs, view->zsoLocalIndexes, dxDevice, ddat, drawArgs);
		DEBUG_DX_FLUSH();
		DEBUG_HR_ACCEND(&hrsbstart, &hrsbend, &hrdrawObjsTime);

		DEBUG_HR_START(&hrsbstart);
		// colour
		if (fireSprites.size() > 0)
			fireSprite->draw(dxDevice, ddat, &sbuff, &fireSprites.front(), 0, fireSprites.size(), drawArgs, SD_colour);
		if (smokeSprites.size() > 0)
			smokeSprite->draw(dxDevice, ddat, &sbuff, &smokeSprites.front(), 0, smokeSprites.size(), drawArgs, SD_colour);
		// alpha
		if (fireSprites.size() > 0)
			fireSprite->draw(dxDevice, ddat, &sbuff, &fireSprites.front(), 0, fireSprites.size(), drawArgs, SD_alpha);
		if (smokeSprites.size() > 0)
			smokeSprite->draw(dxDevice, ddat, &sbuff, &smokeSprites.front(), 0, smokeSprites.size(), drawArgs, SD_alpha);
		DEBUG_DX_FLUSH();
		DEBUG_HR_ACCEND(&hrsbstart, &hrsbend, &hrdrawSpritesTime);
	}

	dxDevice->EndScene();
}

void drawOver(LPDIRECT3DDEVICE9 dxDevice, drawData* ddat, UNCRZ_over* over)
{
	D3DXMATRIX idMat;
	D3DXMatrixIdentity(&idMat);
	dxDevice->SetRenderTarget(0, ddat->targetSurface);
	dxDevice->SetDepthStencilSurface(over->zSurface);

	D3DVIEWPORT9 vp = createViewPort(over->texWidth, over->texHeight);
	dxDevice->SetViewport(&vp);

	dxDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
	dxDevice->SetRenderState(D3DRS_ZENABLE, false);
	dxDevice->SetRenderState(D3DRS_ZWRITEENABLE, false);

	dxDevice->BeginScene();

	vertexOver overVerts[4]; // make this const / VBuff
	overVerts[0] = vertexOver(-1, -1, 0, 0, 1);
	overVerts[1] = vertexOver(-1, 1, 0, 0, 0);
	overVerts[2] = vertexOver(1, -1, 0, 1, 1);
	overVerts[3] = vertexOver(1, 1, 0, 1, 0);

	D3DXVECTOR4 texData = D3DXVECTOR4(0.5 / (float)ddat->targetVp->Width, 0.5 / (float)ddat->targetVp->Height, 1.0 / (float)ddat->targetVp->Width, 1.0 / (float)ddat->targetVp->Height);
	over->effect.setTextureData((float*)&texData.x);
	
	for (int i = 0; i < 4; i++) // do ahead of shader
	{
		overVerts[i].tu += texData.x;
		overVerts[i].tv += texData.y;
	}

	over->setTextures();
	over->effect.setEyePos(&ddat->eyePos);
	over->effect.setEyeDir(&ddat->eyeDir);
	over->effect.setTicker(ddat->ticker);
	over->effect.effect->SetTechnique(over->overTech);
	over->effect.setViewProj(&idMat);
	over->effect.effect->CommitChanges();
	over->effect.setcolMod(over->colMod);

	setAlpha(dxDevice, over->alphaMode);

	UINT numPasses, pass;
	over->effect.effect->Begin(&numPasses, 0);
	over->effect.effect->BeginPass(0);

	dxDevice->SetVertexDeclaration(vertexDecOver);
	dxDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, overVerts, sizeof(vertexOver));

	over->effect.effect->EndPass();
	over->effect.effect->End();
	
	dxDevice->SetRenderState(D3DRS_ZENABLE, true);
	dxDevice->SetRenderState(D3DRS_ZWRITEENABLE, true);
	
	dxDevice->EndScene();
}

void drawLight(LPDIRECT3DDEVICE9 dxDevice, lightData* ld)
{
	if (ld->useLightMap == false)
		return;

	dxDevice->SetRenderTarget(0, ld->lightSurface);
	dxDevice->SetDepthStencilSurface(zLightSurface);

	D3DVIEWPORT9 vp = createViewPort(ld->texWidth, ld->texHeight);
	dxDevice->SetViewport(&vp);

	moveCameraLight(dxDevice, ld);
	disableClip(dxDevice);

	dxDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(255, 0, 0), 1.0f, 0); // (all of texture used for depth (lies))
	dxDevice->Clear(0, NULL, D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
	dxDevice->SetRenderState(D3DRS_ZENABLE, true);
	dxDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
	dxDevice->BeginScene();

	drawData ddat = createDrawData(0.0f, &vp);
	ddat.lightType = ld->lightType;
	ddat.lightDepth = ld->lightDepth;
	ddat.lightConeness = ld->coneness;

	//cloudObj->draw(dxDevice, &ddat, DF_light); // just gets in the way
	mapObj->draw(dxDevice, &ddat, DF_light);	
	drawZBackToFront(zSortedObjs, ld->zsoLocalIndexes, dxDevice, &ddat, DF_light);
	if (fireSprites.size() > 0)
	{
		fireSprite->draw(dxDevice, &ddat, &sbuff, &fireSprites.front(), 0, fireSprites.size(), DF_light, SD_default);
	}
	if (smokeSprites.size() > 0)
	{
		smokeSprite->draw(dxDevice, &ddat, &sbuff, &smokeSprites.front(), 0, smokeSprites.size(), DF_light, SD_default);
	}

	dxDevice->EndScene();
}

void prepDynamicDecal(LPDIRECT3DDEVICE9 dxDevice, dynamicDecalData* ddd)
{
	moveCameraDynamicDecal(dxDevice, ddd);
}

void initTextures(LPDIRECT3DDEVICE9 dxDevice)
{
	D3DVIEWPORT9 vp;
	dxDevice->GetViewport(&vp);

	vp.Width;
	vp.Height;

	vpWidth = vp.Width;
	vpHeight = vp.Height;

	//D3DXCreateTexture(dxDevice, vp.Width, vp.Height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &waterReflectTex);
	//waterReflectTex->GetSurfaceLevel(0, &waterReflectSurface);
	//
	//D3DXCreateTexture(dxDevice, vp.Width, vp.Height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &waterRefractTex);
	//waterRefractTex->GetSurfaceLevel(0, &waterRefractSurface);
	//
	//D3DXCreateTexture(dxDevice, vp.Width, vp.Height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &underTex);
	//underTex->GetSurfaceLevel(0, &underSurface);

	// side
	D3DXCreateTexture(dxDevice, vp.Width * targetTexScale, vp.Height * targetTexScale, 0, D3DUSAGE_RENDERTARGET, D3DFMT_A8B8G8R8, D3DPOOL_DEFAULT, &sideTex);
	sideTex->GetSurfaceLevel(0, &sideSurface);
	dxDevice->CreateDepthStencilSurface(vp.Width * targetTexScale, vp.Height * targetTexScale, D3DFMT_D16, D3DMULTISAMPLE_NONE, 0, TRUE, &zSideSurface, NULL);
	
	// target
	D3DXCreateTexture(dxDevice, vp.Width * targetTexScale, vp.Height * targetTexScale, 0, D3DUSAGE_RENDERTARGET, D3DFMT_A8B8G8R8, D3DPOOL_DEFAULT, &targetTex);
	targetTex->GetSurfaceLevel(0, &targetSurface);
	dxDevice->CreateDepthStencilSurface(vp.Width * targetTexScale, vp.Height * targetTexScale, D3DFMT_D16, D3DMULTISAMPLE_NONE, 0, TRUE, &zSurface, NULL);
	
	// light
	dxDevice->CreateDepthStencilSurface(vp.Width * lightTexScale, vp.Height * lightTexScale, D3DFMT_D16, D3DMULTISAMPLE_NONE, 0, TRUE, &zLightSurface, NULL);

	HRESULT res;
	//res = D3DXCreateTextureFromFile(dxDevice, "ripples.tga", &ripplesTex);
	//res = res;


	// targettexture linking

	UNCRZ_texture* tex = new UNCRZ_texture("target", targetTex);
	textures.push_back(tex);
}

void initViews(LPDIRECT3DDEVICE9 dxDevice)
{
	UNCRZ_view* tempView;

	tempView = new UNCRZ_view("main");
	tempView->camPos = D3DXVECTOR3(-30, 20, -30);
	tempView->camPos += D3DXVECTOR3(0.5, 0, -0.5);
	tempView->dirNormalAt(D3DXVECTOR3(0, 1, 0));
	tempView->init(dxDevice, vpWidth * targetTexScale, vpHeight * targetTexScale, D3DFMT_A8B8G8R8);
	tempView->initStencil(dxDevice, zSurface);
	tempView->dimX = FOV;
	tempView->dimY = vpWidth / (float)vpHeight / 2.0;
	tempView->viewMode = VM_persp;
	tempView->alphaMode = AM_none;
	tempView->clearView = true;
	tempView->clearColor = D3DCOLOR_XRGB(0, 0, 0);
	tempView->projectionNear = 1;
	tempView->projectionFar = 1000; // broken ATM, must be 1000
	views.push_back(tempView);

	// this bit is IMPERATIVE (targettexture linking)
	for (int i = views.size() - 1; i >= 0; i--)
	{
		UNCRZ_texture* tex = new UNCRZ_texture(std::string("view_") + views[i]->name, views[i]->targetTex);
		textures.push_back(tex);
	}
}

void initOvers(LPDIRECT3DDEVICE9 dxDevice)
{
	UNCRZ_over* tempOver;

	tempOver = new UNCRZ_over("main");
	tempOver->init(dxDevice, vpWidth * targetTexScale, vpHeight * targetTexScale, "un_shade.fx", "over", &effects, D3DFMT_A8B8G8R8);
	tempOver->initStencil(dxDevice, zSurface);
	tempOver->useTex = true; // write a "loadtex" func or something?
	createTexture(dxDevice, "view_main", &tempOver->tex, &textures);
	tempOver->alphaMode = AM_none;
	tempOver->clearView = true;
	tempOver->clearColor = D3DCOLOR_XRGB(0, 0, 0);
	tempOver->colMod = D3DXVECTOR4(1, 1, 1, 1);
	overs.push_back(tempOver);

	// this bit is IMPERATIVE (targettexture linking)
	for (int i = overs.size() - 1; i >= 0; i--)
	{
		UNCRZ_texture* tex = new UNCRZ_texture(std::string("over_") + overs[i]->name, overs[i]->targetTex);
		textures.push_back(tex);
	}
}

void initLights(LPDIRECT3DDEVICE9 dxDevice)
{
	lightData* ld;


	ld = new lightData("sun");

	ld->lightEnabled = true;
	ld->lightType = LT_ortho;
	ld->dimX = 500;
	ld->dimY = 500;
	ld->lightDepth = 100;
	ld->lightDir = D3DXVECTOR4(0.1, -10, 0.5, 0.0);
	ld->lightPos = D3DXVECTOR4(0, 50.0, 0, 0.0);
	ld->lightUp = D3DXVECTOR3(1, 0, 0);
	ld->lightAmbient = D3DXVECTOR4(0, 0, 0, 0);
	ld->lightColMod = D3DXVECTOR4(1, 1, 1, 1);
	ld->init(dxDevice, vpWidth * lightTexScale, vpHeight * lightTexScale, "lightPattern.tga", &textures); // MAXIMUM SIZE
	ld->useLightMap = true;

	lights.push_back(ld);



	// disable these lights while testing
	/*ld = new lightData();

	ld->lightEnabled = true;
	ld->lightType = LT_point;
	ld->lightDepth = 100;
	ld->lightDir = D3DXVECTOR4(0, 0, 1, 0.0); // not used
	ld->lightPos = D3DXVECTOR4(0, 20, 0, 0.0);
	ld->lightUp = D3DXVECTOR3(1, 0, 0); // not used
	ld->useLightMap = false;

	lights.push_back(ld);


	ld = new lightData();

	ld->lightType = LT_persp;
	ld->dimX = FOV;
	ld->dimY = 1;
	ld->lightDepth = 50;
	ld->lightDir = D3DXVECTOR4(0.0, -10, 0.0, 0.0);
	ld->lightPos = D3DXVECTOR4(0, 20.0, 0, 0.0);
	ld->lightUp = D3DXVECTOR3(0, 1, 0);
	ld->init(dxDevice, vpWidth, vpHeight, "lightPattern1.tga");
	ld->useLightMap = true;

	lights.push_back(ld);


	ld = new lightData();

	ld->lightType = LT_persp;
	ld->dimX = FOV * 0.6f;
	ld->dimY = 1;
	ld->lightDepth = 100;
	ld->lightDir = D3DXVECTOR4(0.1, -10, 0.0, 0.0);
	ld->lightPos = D3DXVECTOR4(100, 20.0, 0.0, 0.0);
	ld->lightUp = D3DXVECTOR3(0, 1, 0);
	ld->init(dxDevice, vpWidth, vpHeight, "lightPattern2.tga");
	ld->useLightMap = true;

	lights.push_back(ld);


	ld = new lightData();

	ld->lightType = LT_point;
	ld->lightDepth = 100;
	ld->lightDir = D3DXVECTOR4(0, 0, 1, 0.0); // not used
	ld->lightPos = D3DXVECTOR4(0, 20, 0, 0.0);
	ld->lightUp = D3DXVECTOR3(1, 0, 0); // not useds
	ld->useLightMap = false;

	lights.push_back(ld);*/

	// this bit is IMPERATIVE (targettexture linking)
	for (int i = lights.size() - 1; i >= 0; i--)
	{
		UNCRZ_texture* tex = new UNCRZ_texture(std::string("light_") + lights[i]->name, lights[i]->lightTex);
		textures.push_back(tex);
	}
}

void initOther()
{
}

void texAlignViewProj(LPD3DXMATRIX vp)
{
	D3DXMATRIX translate, scale;

	D3DXMatrixScaling(&scale, 0.5, -0.5, 1.0);
	D3DXMatrixMultiply(vp, vp, &scale);
	D3DXMatrixTranslation(&translate, 0.5, 0.5, 0);
	D3DXMatrixMultiply(vp, vp, &translate);
}

void moveCamera_persp()
{
	D3DXMATRIXA16 rotMatrix;

	D3DXVECTOR3 eyeVec = camPos;
	D3DXVECTOR3 targVec(1, 0, 0);  // ??
	D3DXVECTOR3 upVec(0, 1, 0);

	D3DXMatrixRotationZ(&rotMatrix, rotPar);
	D3DXVec3TransformNormal(&targVec, &targVec, &rotMatrix);

	D3DXMatrixRotationY(&rotMatrix, rotY);
	D3DXVec3TransformNormal(&targVec, &targVec, &rotMatrix);

	targVec += eyeVec;

	D3DXMatrixLookAtLH(&viewMatrix, &eyeVec, &targVec, &upVec);
 
	D3DXMatrixPerspectiveFovLH(&projMatrix, FOV, mainVt.winWidth / mainVt.winHeight, 1, farDepth);
	D3DXMatrixMultiply(&viewProj, &viewMatrix, &projMatrix);

	eyePos = D3DXVECTOR4(eyeVec, 0);
	eyeDir = D3DXVECTOR4(targVec, 0) - eyePos;
}

void moveCamera(LPDIRECT3DDEVICE9 dxDevice)
{
	switch (viewMode)
	{
	case VM_persp:
		moveCamera_persp();
		break;
	}
}

void moveCameraView_ortho(UNCRZ_view* view)
{
	D3DXVECTOR3 eyeVec = view->camPos;
	D3DXVECTOR3 targVec = eyeVec + view->camDir;
	D3DXVECTOR3 upVec = view->camUp;

	D3DXMatrixLookAtLH(&viewMatrix, &eyeVec, &targVec, &upVec);
 
	D3DXMatrixOrthoLH(&projMatrix, view->dimX, view->dimY, view->projectionNear, view->projectionFar);
	D3DXMatrixMultiply(&viewProj, &viewMatrix, &projMatrix);

	texAlignViewProj(&viewProj);
}

void moveCameraView_persp(UNCRZ_view* view)
{
	D3DXVECTOR3 eyeVec = view->camPos;
	D3DXVECTOR3 targVec = eyeVec + view->camDir;
	D3DXVECTOR3 upVec = view->camUp;

	D3DXMatrixLookAtLH(&viewMatrix, &eyeVec, &targVec, &upVec);
 
	D3DXMatrixPerspectiveFovLH(&projMatrix, view->dimX, view->dimY, view->projectionNear, view->projectionFar);
	D3DXMatrixMultiply(&viewProj, &viewMatrix, &projMatrix);
}

void moveCameraView(LPDIRECT3DDEVICE9 dxDevice, UNCRZ_view* view)
{
	if (view->viewMode == VM_ortho)
		moveCameraView_ortho(view);
	else if (view->viewMode == VM_persp)
		moveCameraView_persp(view);

	view->viewViewMat = viewMatrix;
	view->viewProjMat = projMatrix;
	view->viewViewProjVP = viewProj;
	view->viewViewProj = viewProj;

	texAlignViewProj(&view->viewViewProj);
}

/*void moveCamerawaterReflect(LPDIRECT3DDEVICE9 dxDevice)
{
	D3DXMATRIXA16 rotMatrix;

	D3DXVECTOR3 eyeVec = camPos;
	eyeVec.y = 1.0f - eyeVec.y;
	D3DXVECTOR3 targVec(1, 0, 0);  // ??
	D3DXVECTOR3 upVec(0, 1, 0);

	D3DXMatrixRotationZ(&rotMatrix, rotPar + xVPM);
	D3DXVec3TransformNormal(&targVec, &targVec, &rotMatrix);

	targVec.y = 0.0f - targVec.y;

	D3DXMatrixRotationY(&rotMatrix, rotY + yVPM);
	D3DXVec3TransformNormal(&targVec, &targVec, &rotMatrix);

	targVec += eyeVec;

	D3DXMatrixLookAtLH(&viewMatrix, &eyeVec, &targVec, &upVec);
 
	D3DXMatrixPerspectiveFovLH(&projMatrix, FOV, 500/500, 1, farDepth);
	D3DXMatrixMultiply(&viewProj, &viewMatrix, &projMatrix);
}

void moveCamerawaterRefract(LPDIRECT3DDEVICE9 dxDevice)
{
	D3DXMATRIXA16 rotMatrix;

	D3DXVECTOR3 eyeVec = camPos;
	//eyeVec.y = 1.0f - eyeVec.y;
	D3DXVECTOR3 targVec(1, 0, 0);  // ??
	D3DXVECTOR3 upVec(0, 1, 0);

	D3DXMatrixRotationZ(&rotMatrix, rotPar + xVPM);
	D3DXVec3TransformNormal(&targVec, &targVec, &rotMatrix);

	//targVec.y = - targVec.y;

	D3DXMatrixRotationY(&rotMatrix, rotY + yVPM);
	D3DXVec3TransformNormal(&targVec, &targVec, &rotMatrix);

	targVec += eyeVec;

	D3DXMatrixLookAtLH(&viewMatrix, &eyeVec, &targVec, &upVec);
 
	D3DXMatrixPerspectiveFovLH(&projMatrix, FOV, 500/500, 1, farDepth);
	D3DXMatrixMultiply(&viewProj, &viewMatrix, &projMatrix);
}*/

void moveCameraLight_ortho(LPDIRECT3DDEVICE9 dxDevice, lightData* ld)
{
	D3DXVECTOR3 eyeVec(ld->lightPos.x, ld->lightPos.y, ld->lightPos.z);
	D3DXVECTOR3 targVec(ld->lightDir.x, ld->lightDir.y, ld->lightDir.z);
	D3DXVECTOR3 upVec = ld->lightUp;

	targVec += eyeVec;

	D3DXMatrixLookAtLH(&viewMatrix, &eyeVec, &targVec, &upVec);
 
	D3DXMatrixOrthoLH(&projMatrix, ld->dimX, ld->dimY, 0, ld->lightDepth);
	D3DXMatrixMultiply(&viewProj, &viewMatrix, &projMatrix);
}

void moveCameraLight_persp(LPDIRECT3DDEVICE9 dxDevice, lightData* ld)
{
	D3DXVECTOR3 eyeVec = ld->lightPos;
	D3DXVECTOR3 targVec(ld->lightDir.x, ld->lightDir.y, ld->lightDir.z);
	D3DXVECTOR3 upVec = ld->lightUp;

	targVec += eyeVec;

	D3DXMatrixLookAtLH(&viewMatrix, &eyeVec, &targVec, &upVec);
 
	D3DXMatrixPerspectiveFovLH(&projMatrix, ld->dimX, ld->dimY, 0, ld->lightDepth);
	D3DXMatrixMultiply(&viewProj, &viewMatrix, &projMatrix);
}

void moveCameraLight(LPDIRECT3DDEVICE9 dxDevice, lightData* ld)
{
	if (ld->lightType == LT_ortho)
		moveCameraLight_ortho(dxDevice, ld);
	else if (ld->lightType == LT_persp)
		moveCameraLight_persp(dxDevice, ld);

	ld->lightViewProjVP = viewProj;
	ld->lightViewProj = viewProj;

	texAlignViewProj(&ld->lightViewProj);
}

void moveCameraDynamicDecal_ortho(LPDIRECT3DDEVICE9 dxDevice, dynamicDecalData* ddd)
{
	D3DXVECTOR3 eyeVec(ddd->lightPos.x, ddd->lightPos.y, ddd->lightPos.z);
	D3DXVECTOR3 targVec(ddd->lightDir.x, ddd->lightDir.y, ddd->lightDir.z);
	D3DXVECTOR3 upVec = ddd->lightUp;

	targVec += eyeVec;

	D3DXMatrixLookAtLH(&viewMatrix, &eyeVec, &targVec, &upVec);
 
	D3DXMatrixOrthoLH(&projMatrix, ddd->dimX, ddd->dimY, 0, ddd->lightDepth);
	D3DXMatrixMultiply(&viewProj, &viewMatrix, &projMatrix);

	texAlignViewProj(&viewProj);

	ddd->lightViewProj = viewProj;
}

void moveCameraDynamicDecal_persp(LPDIRECT3DDEVICE9 dxDevice, dynamicDecalData* ddd)
{
	D3DXVECTOR3 eyeVec = ddd->lightPos;
	D3DXVECTOR3 targVec(ddd->lightDir.x, ddd->lightDir.y, ddd->lightDir.z);
	D3DXVECTOR3 upVec = ddd->lightUp;

	targVec += eyeVec;

	D3DXMatrixLookAtLH(&viewMatrix, &eyeVec, &targVec, &upVec);

	D3DXMatrixPerspectiveFovLH(&projMatrix, ddd->dimX, ddd->dimY, 0, ddd->lightDepth);
	D3DXMatrixMultiply(&viewProj, &viewMatrix, &projMatrix);

	texAlignViewProj(&viewProj);

	ddd->lightViewProj = viewProj;
}

void moveCameraDynamicDecal(LPDIRECT3DDEVICE9 dxDevice, dynamicDecalData* ddd)
{
	if (ddd->lightType == DDT_ortho)
		moveCameraDynamicDecal_ortho(dxDevice, ddd);
	else if (ddd->lightType == DDT_persp)
		moveCameraDynamicDecal_persp(dxDevice, ddd);
}

void term()
{
	DestroyWindow(mainHWnd);
}
