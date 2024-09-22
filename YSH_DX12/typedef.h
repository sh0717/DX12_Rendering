#pragma once

#include <DirectXMath.h>

using namespace DirectX;

struct BasicVertex
{
	XMFLOAT3 Position;
	XMFLOAT4 Color;
	XMFLOAT2 TexCoord;
};

enum class EConstantBufferType
{
	MeshObject = 0,
	SpriteObject = 1,
	TypeCount = 2
};

struct CONSTANT_BUFFER_PROPERTY
{
	EConstantBufferType type;
	UINT Size;
};


union RGBA
{
	struct
	{
		BYTE	r;
		BYTE	g;
		BYTE	b;
		BYTE	a;
	};
	BYTE		bColorFactor[4];
};

struct CONSTANT_BUFFER_DEFAULT 
{
	XMMATRIX matWorld;
	XMMATRIX matView;
	XMMATRIX matProj;
};

struct CONSTANT_BUFFER_SPRITE
{
	XMFLOAT2 ScreenRes;
	XMFLOAT2 Pos;
	XMFLOAT2 Scale;
	XMFLOAT2 TexSize;
	XMFLOAT2 TexSamplePos;
	XMFLOAT2 TexSampleSize;
	float	Z;
	float	Alpha;
	float	Reserved0;
	float	Reserved1;
};



struct TextureHandle
{
public:
	ID3D12Resource* pTexResource = nullptr;      
	ID3D12Resource* pUploadBuffer = nullptr;    
	D3D12_CPU_DESCRIPTOR_HANDLE SRVHandle;    
	void* pHashSearchHandle = nullptr; 
	UINT RefCount = 0;
	bool bUpdated = false;
	bool bFromFile = false;
};


struct FontHandle
{
public:
	IDWriteTextFormat* pTextFormat;
	float FontSize;
	WCHAR	FontFamilyName[512];
};


struct TVERTEX
{
	float u;
	float v;
};
struct FLOAT3
{
	float x;
	float y;
	float z;
};


enum class ERenderItemType
{
	MeshObject,
	SpriteObject
};

struct RenderMeshObjParam
{
	XMMATRIX WorldMatrix;
};

struct RenderSpriteObjParam
{
	int iPosX;
	int iPosY;
	float fScaleX;
	float fScaleY;
	RECT Rect;
	BOOL bUseRect;
	float Z;
	void* pTexHandle;
};

struct RenderItem
{
	ERenderItemType Type;
	void* pObjectHandle;
	union
	{
		RenderMeshObjParam MeshObjectParam;
		RenderSpriteObjParam SpriteObjectParam;
	};
};