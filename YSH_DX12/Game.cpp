#include "pch.h"
#include "Game.h"
#include "D3D12Renderer.h"


class CBasicMeshObject* gMeshObj = nullptr;

float g_fOffsetX = 0.0f;
float g_fOffsetY = 0.0f;
float g_fSpeedX = 0.02f;
float g_fSpeedY = 0.02f;

float g_fRot0 = 0.0f;
float g_fRot1 = 0.0f;

XMMATRIX g_matWorld0 = {};
XMMATRIX g_matWorld1 = {};


ULONGLONG g_PrvFrameCheckTick = 0;
ULONGLONG g_PrvUpdateTick = 0;
DWORD g_FPS = 0;
DWORD	g_FrameCount = 0;

void* g_pSpriteObj0 = nullptr;
void* g_pSpriteObj1 = nullptr;


TextureHandle* gTextureHandle = nullptr;
UINT g_ImageWidth = 512;
UINT g_ImageHeight = 512;
TextureHandle* gDynamicTextureHandle = nullptr;
BYTE* g_pImage = nullptr;


/*for text rendering*/
void* g_pSpriteObjCommon = nullptr;

void* g_pFontObj = nullptr;
BYTE* g_pTextImage = nullptr;
UINT g_TextImageWidth = 0;
UINT g_TextImageHeight = 0;
void* g_pTextTextureHandle = nullptr;

WCHAR g_wchText[64] = {};




CGame::CGame()
{
}

CGame::~CGame()
{
	Cleanup();
}

BOOL CGame::Initialiize(HWND hWnd, BOOL bEnableDebugLayer, BOOL bEnableGBV)
{
	m_pRenderer = new CD3D12Renderer;
	m_pRenderer->Initialize(hWnd, bEnableDebugLayer, bEnableGBV);
	m_hWnd = hWnd;


	gMeshObj = (CBasicMeshObject*)CreateBoxMeshObject();

	g_pSpriteObj0 = m_pRenderer->CreateSpriteObject(L"./Assets/Textures/tex_03.dds", 0, 0, 256, 256);
	gTextureHandle = (TextureHandle*)m_pRenderer->CreateTextureFromFile(L"./Assets/Textures/sprite_1024x1024.dds");
	gDynamicTextureHandle = (TextureHandle*)m_pRenderer->CreateDynamicTexture(g_ImageWidth, g_ImageHeight);
	g_pImage = (BYTE*)malloc(g_ImageWidth * g_ImageHeight * 4);
	DWORD* pDest = (DWORD*)g_pImage;
	for (DWORD y = 0; y < g_ImageHeight; y++)
	{
		for (DWORD x = 0; x < g_ImageWidth; x++)
		{
			pDest[x + g_ImageWidth * y] = 0xff0000ff;
		}
	}


	/*for texture rendering*/
	g_pFontObj = m_pRenderer->CreateFontObject(L"Tahoma", 18.0f);
	g_TextImageWidth = 512;
	g_TextImageHeight = 256;
	g_pTextImage = (BYTE*)malloc(g_TextImageWidth * g_TextImageHeight * 4);
	memset(g_pTextImage, 0, g_TextImageWidth * g_TextImageHeight * 4);
	g_pTextTextureHandle = m_pRenderer->CreateDynamicTexture(g_TextImageWidth, g_TextImageHeight);
	g_pSpriteObjCommon = m_pRenderer->CreateSpriteObject();


	return TRUE;
}

void CGame::Run()
{
	m_FrameCount++;

	// begin
	ULONGLONG CurTick = GetTickCount64();

	// game business logic
	Update(CurTick);

	Render();

	if (CurTick - m_PrvFrameCheckTick > 1000)
	{
		m_PrvFrameCheckTick = CurTick;

		WCHAR wchTxt[64];
		m_FPS = m_FrameCount;
		swprintf_s(wchTxt, L"FPS:%u", m_FPS);
		SetWindowText(m_hWnd, wchTxt);

		m_FrameCount = 0;
	}
}

BOOL CGame::UpdateWindowSize(DWORD dwBackBufferWidth, DWORD dwBackBufferHeight)
{
	BOOL bResult = FALSE;
	if (m_pRenderer)
	{
		bResult = m_pRenderer->UpdateWindowSize(dwBackBufferWidth, dwBackBufferHeight);
	}
	return bResult;
}

bool CGame::Update(UINT64 CurTick)
{
	if (CurTick - m_PrvUpdateTick < 16)
	{
		return FALSE;
	}
	m_PrvUpdateTick = CurTick;

	if (m_CamOffsetX != 0.0f || m_CamOffsetY != 0.0f || m_CamOffsetZ != 0.0f)
	{
		m_pRenderer->MoveCamera(m_CamOffsetX, m_CamOffsetY, m_CamOffsetZ);
	}


	//
		// world matrix 0
		//
	g_matWorld0 = XMMatrixIdentity();

	// rotation 
	XMMATRIX matRot0 = XMMatrixRotationX(g_fRot0);

	// translation
	XMMATRIX matTrans0 = XMMatrixTranslation(-0.15f, 0.0f, 0.25f);

	// rot0 x trans0
	g_matWorld0 = XMMatrixMultiply(matRot0, matTrans0);

	//
	// world matrix 1
	//
	g_matWorld1 = XMMatrixIdentity();

	// world matrix 1
	// rotation 
	XMMATRIX matRot1 = XMMatrixRotationY(g_fRot1);

	// translation
	XMMATRIX matTrans1 = XMMatrixTranslation(0.15f, 0.0f, 0.25f);

	// rot1 x trans1
	g_matWorld1 = XMMatrixMultiply(matRot1, matTrans1);

	BOOL	bChangeTex = FALSE;
	g_fRot0 += 0.05f;
	if (g_fRot0 > 2.0f * 3.1415f)
	{
		g_fRot0 = 0.0f;
		bChangeTex = TRUE;
	}

	g_fRot1 += 0.1f;
	if (g_fRot1 > 2.0f * 3.1415f)
	{
		g_fRot1 = 0.0f;
	}





	// Update Texture
	static DWORD g_dwCount = 0;
	static DWORD g_dwTileColorR = 0;
	static DWORD g_dwTileColorG = 0;
	static DWORD g_dwTileColorB = 0;

	const DWORD TILE_WIDTH = 16;
	const DWORD TILE_HEIGHT = 16;

	DWORD TILE_WIDTH_COUNT = g_ImageWidth / TILE_WIDTH;
	DWORD TILE_HEIGHT_COUNT = g_ImageHeight / TILE_HEIGHT;

	if (g_dwCount >= TILE_WIDTH_COUNT * TILE_HEIGHT_COUNT)
	{
		g_dwCount = 0;
	}
	DWORD TileY = g_dwCount / TILE_WIDTH_COUNT;
	DWORD TileX = g_dwCount % TILE_WIDTH_COUNT;

	DWORD StartX = TileX * TILE_WIDTH;
	DWORD StartY = TileY * TILE_HEIGHT;


	//DWORD r = rand() % 256;
	//DWORD g = rand() % 256;
	//DWORD b = rand() % 256;

	DWORD r = g_dwTileColorR;
	DWORD g = g_dwTileColorG;
	DWORD b = g_dwTileColorB;


	DWORD* pDest = (DWORD*)g_pImage;
	for (DWORD y = 0; y < 16; y++)
	{
		for (DWORD x = 0; x < 16; x++)
		{
			if (StartX + x >= g_ImageWidth)
				__debugbreak();

			if (StartY + y >= g_ImageHeight)
				__debugbreak();

			pDest[(StartX + x) + (StartY + y) * g_ImageWidth] = 0xff000000 | (b << 16) | (g << 8) | r;
		}
	}
	g_dwCount++;
	g_dwTileColorR += 8;
	if (g_dwTileColorR > 255)
	{
		g_dwTileColorR = 0;
		g_dwTileColorG += 8;
	}
	if (g_dwTileColorG > 255)
	{
		g_dwTileColorG = 0;
		g_dwTileColorB += 8;
	}
	if (g_dwTileColorB > 255)
	{
		g_dwTileColorB = 0;
	}
	m_pRenderer->UpdateTextureWithImage(gDynamicTextureHandle, g_pImage, g_ImageWidth, g_ImageHeight);





	// draw text
	int iTextWidth = 0;
	int iTextHeight = 0;
	WCHAR	wchTxt[64] = {};
	DWORD	dwTxtLen = swprintf_s(wchTxt, L"xxx Current FrameRate: %u xxx", m_FPS);

	if (wcscmp(g_wchText, wchTxt))
	{
		// 텍스트가 변경된 경우
		

		memset(g_pTextImage, 0, g_TextImageWidth * g_TextImageHeight * 4);
		m_pRenderer->WriteTextToBitmap(g_pTextImage, g_TextImageWidth, g_TextImageHeight, g_TextImageWidth * 4, &iTextWidth, &iTextHeight, g_pFontObj, wchTxt, dwTxtLen);
		m_pRenderer->UpdateTextureWithImage(g_pTextTextureHandle, g_pTextImage, g_TextImageWidth, g_TextImageHeight);
		wcscpy_s(g_wchText, wchTxt);
	}
	else
	{
		// 텍스트가 변경되지 않은 경우 - 업데이트 할 필요 없다.
		int a = 0;
	}





	return TRUE;


}

void CGame::Render()
{
	m_pRenderer->BeginRender();





	m_pRenderer->RenderMeshObject(gMeshObj, &g_matWorld0);
	m_pRenderer->RenderMeshObject(gMeshObj, &g_matWorld1);


	/*sprite*/
	RECT rect;
	rect.left = 0;
	rect.top = 0;
	rect.right = g_ImageWidth * 2;
	rect.bottom = g_ImageHeight;
	m_pRenderer->RenderSpriteObject(g_pSpriteObj0, 0, 0, .4f, .2f, &rect, .1f, gDynamicTextureHandle);


	m_pRenderer->RenderSpriteObject(g_pSpriteObjCommon, 512 + 5, 256 + 5 + 256 + 5, 1.0f, 1.0f, nullptr, 0.0f, g_pTextTextureHandle);



	// end
	m_pRenderer->EndRender();

	// Present
	m_pRenderer->Present();
}

void CGame::Cleanup()
{
	
	if (g_pImage)
	{
		delete g_pImage;
		g_pImage = nullptr;
	}
	

	if (m_pRenderer)
	{
		m_pRenderer->DeleteBasicMeshObject(gMeshObj);
		gMeshObj = nullptr;

		m_pRenderer->DeleteSpriteObject(g_pSpriteObj0);
		g_pSpriteObj0 = nullptr;
		m_pRenderer->DeleteSpriteObject(g_pSpriteObj1);
		g_pSpriteObj1 = nullptr;

		m_pRenderer->DeleteTexture(gTextureHandle);
		m_pRenderer->DeleteTexture(gDynamicTextureHandle);

		if (g_pFontObj)
		{
			m_pRenderer->DeleteFontObject(g_pFontObj);
			g_pFontObj = nullptr;
		}

		if (g_pTextTextureHandle)
		{
			m_pRenderer->DeleteTexture(g_pTextTextureHandle);
			g_pTextTextureHandle = nullptr;
		}

		if (g_pTextImage)
		{
			free(g_pTextImage);
			g_pTextImage = nullptr;
		}

		if (g_pSpriteObjCommon)
		{
			m_pRenderer->DeleteSpriteObject(g_pSpriteObjCommon);
			g_pSpriteObjCommon = nullptr;
		}
		delete m_pRenderer;
		m_pRenderer = nullptr;
	}
}

void CGame::OnKeyDown(UINT nChar, UINT uiScanCode)
{
	switch (nChar)
	{
	case VK_SHIFT:
		m_bShiftKeyDown = TRUE;
		break;
	case 'W':
		if (m_bShiftKeyDown)
		{
			m_CamOffsetY = 0.05f;
		}
		else
		{
			m_CamOffsetZ = 0.05f;
		}
		break;
	case 'S':
		if (m_bShiftKeyDown)
		{
			m_CamOffsetY = -0.05f;
		}
		else
		{
			m_CamOffsetZ = -0.05f;
		}
		break;
	case 'A':
		m_CamOffsetX = -0.05f;
		break;
	case 'D':
		m_CamOffsetX = 0.05f;
		break;
	}
}

void CGame::OnKeyUp(UINT nChar, UINT uiScanCode)
{
	switch (nChar)
	{
	case VK_SHIFT:
		m_bShiftKeyDown = FALSE;
		break;
	case 'W':
		m_CamOffsetY = 0.0f;
		m_CamOffsetZ = 0.0f;
		break;
	case 'S':
		m_CamOffsetY = 0.0f;
		m_CamOffsetZ = 0.0f;
		break;
	case 'A':
		m_CamOffsetX = 0.0f;
		break;
	case 'D':
		m_CamOffsetX = 0.0f;
		break;
	}
}

void* CGame::CreateBoxMeshObject()
{
	void* pMeshObj = nullptr;

	// create box mesh
	// create vertices and indices
	WORD	pIndexList[36] = {};
	BasicVertex* pVertexList = nullptr;
	DWORD dwVertexCount = CreateBoxMesh(&pVertexList, pIndexList, (DWORD)_countof(pIndexList), 0.25f);

	// create CBasicMeshObject from Renderer
	pMeshObj = m_pRenderer->CreateBasicMeshObject();

	const WCHAR* wchTexFileNameList[6] =
	{
		L"./Assets/Textures/tex_01.dds",
		L"./Assets/Textures/tex_02.dds",
		L"./Assets/Textures/tex_03.dds",
		L"./Assets/Textures/tex_04.dds",
		L"./Assets/Textures/tex_05.dds",
		L"./Assets/Textures/tex_07.dds" // intentionally wrong texture
	};

	// Set meshes to the CBasicMeshObject
	m_pRenderer->InsertVertexDataToMeshObject(pMeshObj, pVertexList, dwVertexCount, 6);	// 박스의 6면-1면당 삼각형 2개-인덱스 6개
	for (DWORD i = 0; i < 6; i++)
	{
		m_pRenderer->InsertTriGroupDataToMeshObject(pMeshObj, pIndexList + i * 6, 2, wchTexFileNameList[i]);
	}
	m_pRenderer->EndCreateMesh(pMeshObj);

	// delete vertices and indices
	if (pVertexList)
	{
		DeleteBoxMesh(pVertexList);
		pVertexList = nullptr;
	}
	return pMeshObj;
}
