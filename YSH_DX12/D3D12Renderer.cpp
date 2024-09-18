#include "pch.h"
#include <dxgi.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <dxgidebug.h>
#include <d3dx12.h>
#include "../D3D_Util/D3DUtil.h"
#include "BasicMeshObject.h"
#include "D3D12ResourceManager.h"
#include "D3D12Renderer.h"

#include <iostream>

#include "ConstantBufferPool.h"
#include "DescriptorPool.h"
#include "SingleDescriptorAllocator.h"
#include "SpriteObject.h"
#include "ConstantBufferManager.h"
#include "FontManager.h"
#include "TextureManager.h"

CD3D12Renderer::CD3D12Renderer()
	:  m_pSingleDescriptorAllocator(new CSingleDescriptorAllocator) , m_pResourceManager(new CD3D12ResourceManager) 
{
	m_pFontManager = std::make_unique<CFontManager>();
	for(UINT i= 0 ; i < MAX_PENDING_FRAME_COUNT ; ++ i)
	{
		m_ppDescriptorPools[i] = new CDescriptorPool{};
		m_ppConstantBufferManager[i] = new CConstantBufferManager{};
	}
	
}
CD3D12Renderer::~CD3D12Renderer()
{
	Cleanup();
}

bool CD3D12Renderer::Initialize(HWND hWnd, BOOL bEnableDebugLayer, BOOL bEnableGBV)
{
	bool	bResult = false;
	HRESULT Hr = S_OK;
	ID3D12Debug*	t_pDebugController = nullptr;
	IDXGIFactory4*	t_pFactory = nullptr;
	IDXGIAdapter1*	t_pAdapter = nullptr;
	DXGI_ADAPTER_DESC1 AdapterDesc = {};

	DWORD dwCreateFlags = 0;
	UINT dwCreateFactoryFlags = 0;

	m_DPI = GetDpiForWindow(hWnd);


	// if use debug Layer
	if (bEnableDebugLayer)
	{
		// Enable the D3D12 debug layer.
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&t_pDebugController))))
		{
			t_pDebugController->EnableDebugLayer();
		}
		dwCreateFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
		if (bEnableGBV)
		{
			ID3D12Debug5*	t_pDebugController_5 = nullptr;
			if (S_OK == t_pDebugController->QueryInterface(IID_PPV_ARGS(&t_pDebugController_5)))
			{
				t_pDebugController_5->SetEnableGPUBasedValidation(TRUE);
				t_pDebugController_5->SetEnableAutoName(TRUE);
				t_pDebugController_5->Release();
			}
		}
	}

	// Create DXGIFactory
	CreateDXGIFactory2(dwCreateFactoryFlags, IID_PPV_ARGS(&t_pFactory));

	D3D_FEATURE_LEVEL	featureLevels[] =
	{
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0
	};

	UINT	FeatureLevelNum = _countof(featureLevels);

	for (UINT FeatureLevel_IDX = 0; FeatureLevel_IDX < FeatureLevelNum; FeatureLevel_IDX++)
	{
		UINT Adapter_IDX = 0;
		while (DXGI_ERROR_NOT_FOUND != t_pFactory->EnumAdapters1(Adapter_IDX, &t_pAdapter))
		{
			t_pAdapter->GetDesc1(&AdapterDesc);

			if (SUCCEEDED(D3D12CreateDevice(t_pAdapter, featureLevels[FeatureLevel_IDX], IID_PPV_ARGS(&m_pD3DDevice))))
			{
				goto lb_exit;

			}
			t_pAdapter = nullptr;
			Adapter_IDX++;
		}
	}
lb_exit:

	if (!m_pD3DDevice)
	{
		__debugbreak();
		goto SOMETING_WRONG;
	}

	m_AdapterDesc = AdapterDesc;
	m_HWND = hWnd;

	if (t_pDebugController)
	{
		SetDebugLayerInfo(m_pD3DDevice);
	}



	CreateCommandListAndQueue();
	CreateFence();

	if (CreateDescriptorHeap() == false) 
	{
		__debugbreak();
		goto SOMETING_WRONG;
	}

	RECT	rect;
	::GetClientRect(hWnd, &rect);
	DWORD	dwWndWidth = rect.right - rect.left;
	DWORD	dwWndHeight = rect.bottom - rect.top;
	UINT	dwBackBufferWidth = rect.right - rect.left;
	UINT	dwBackBufferHeight = rect.bottom - rect.top;

	//setting viewport 

	m_Viewport.Width = (float)dwWndWidth;
	m_Viewport.Height = (float)dwWndHeight;
	m_Viewport.MinDepth = 0.0f;
	m_Viewport.MaxDepth = 1.0f;

	m_ScissorRect.left = 0;
	m_ScissorRect.top = 0;
	m_ScissorRect.right = dwWndWidth;
	m_ScissorRect.bottom = dwWndHeight;

	m_Width = dwWndWidth;
	m_Height = dwWndHeight;

	m_CBV_SRV_UBV_DescriptorSize = m_pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);


	CreateRenderTarget(dwBackBufferWidth, dwBackBufferHeight, t_pFactory, hWnd);
	CreateDepthStencil(dwBackBufferWidth, dwBackBufferHeight);
	
	if (m_pResourceManager) 
	{
		if (m_pResourceManager->Initialize(m_pD3DDevice) == false)
		{
			goto SOMETING_WRONG;
		}
	}
	else 
	{
		goto SOMETING_WRONG;
	}

	if(m_pTextureManager = new CTextureManager{})
	{
		m_pTextureManager->Initialize(this, m_pResourceManager.get(), 1024 / 16, 1024);
	}

	
	for (UINT i = 0; i < MAX_PENDING_FRAME_COUNT; ++i)
	{
		
		if (m_ppDescriptorPools[i])
		{
			if (m_ppDescriptorPools[i]->Initialize(m_pD3DDevice, MAX_DRAW_COUNT_PER_FRAME * CBasicMeshObject::MAX_DESCRIPTOR_COUNT_FOR_DRAW) == false)
			{
				goto SOMETING_WRONG;
			}
		}
		else
		{
			__debugbreak();
			goto SOMETING_WRONG;
		}
	}
	
	
	for (UINT i = 0; i < MAX_PENDING_FRAME_COUNT; ++i)
	{
		if (m_ppConstantBufferManager[i])
		{
			if (m_ppConstantBufferManager[i]->Initialize(m_pD3DDevice, MAX_DRAW_COUNT_PER_FRAME) == false)
			{
				__debugbreak();
				goto SOMETING_WRONG;
			}
		}
		else
		{
			__debugbreak();
			goto SOMETING_WRONG;
		}
	}
	
	

	if(m_pSingleDescriptorAllocator)
	{
		m_pSingleDescriptorAllocator->Initialize(m_pD3DDevice, MAX_DESCRIPTOR_COUNT, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
	}
	else
	{
		goto SOMETING_WRONG;
	}

	if(m_pFontManager)
	{
		m_pFontManager->Initialize(this, m_pCommandQueue, 1024, 256, bEnableDebugLayer);
	}


	InitCamera();


	bResult = true;
	/*if there is something wrong*/
SOMETING_WRONG:
	if (t_pDebugController)
	{
		t_pDebugController->Release();
		t_pDebugController = nullptr;
	}
	if (t_pAdapter)
	{
		t_pAdapter->Release();
		t_pAdapter = nullptr;
	}
	if (t_pFactory)
	{
		t_pFactory->Release();
		t_pFactory = nullptr;
	}


	if(bResult)
	{
		PostInitialize();
	}
	return bResult;
}

void CD3D12Renderer::BeginRender()
{
	ID3D12CommandAllocator* pCommandAllocator = m_ppCommandAllocators[m_CurContextIndex];
	ID3D12GraphicsCommandList* pCommandList = m_ppCommandLists[m_CurContextIndex];

	if (FAILED(pCommandAllocator->Reset()))
		__debugbreak();

	if (FAILED(pCommandList->Reset(pCommandAllocator, nullptr)))
		__debugbreak();

	CD3DX12_CPU_DESCRIPTOR_HANDLE RTVHandle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart(), m_CurrentRenderTargetIndex, m_RTVDescriptorSize);

	
	pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pRenderTargets[m_CurrentRenderTargetIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	CD3DX12_CPU_DESCRIPTOR_HANDLE DSVHandle(m_pDSVHeap->GetCPUDescriptorHandleForHeapStart());

	// Record commands.
	constexpr float BackColor[] = { 0.0f, 1.0f, 1.0f, 1.0f };
	//객체를 넣어주는게 아니라 Descriptor 를 넣어준다 
	
	pCommandList->ClearRenderTargetView(RTVHandle, BackColor, 0, nullptr);
	pCommandList->ClearDepthStencilView(DSVHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	
	pCommandList->RSSetViewports(1, &m_Viewport);
	pCommandList->RSSetScissorRects(1, &m_ScissorRect);
	pCommandList->OMSetRenderTargets(1, &RTVHandle, FALSE, &DSVHandle);
}
void CD3D12Renderer::EndRender()
{
	ID3D12GraphicsCommandList* pCommandList = m_ppCommandLists[m_CurContextIndex];
	pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pRenderTargets[m_CurrentRenderTargetIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	pCommandList->Close();
	
	ID3D12CommandList* ppCommandLists[] = { pCommandList };
    m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
}

void CD3D12Renderer::Present()
{
	Fence();

	//
	// Back Buffer 화면을 Primary Buffer 로 전송
	//
	//UINT m_SyncInterval = 1;	// VSync On 수직동기화
	UINT m_SyncInterval = 0;	// VSync Off

	UINT uiSyncInterval = m_SyncInterval;
	UINT uiPresentFlags = 0;

	if (!uiSyncInterval)
	{
		uiPresentFlags = DXGI_PRESENT_ALLOW_TEARING;
	}
	
	HRESULT hr = m_pSwapChain->Present(uiSyncInterval, uiPresentFlags);
	
	if (DXGI_ERROR_DEVICE_REMOVED == hr)
	{
		__debugbreak();
	}
	
	// for next frame 다음 프레임의 RTVIndex
    m_CurrentRenderTargetIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	UINT64 NextContextIndex = (m_CurContextIndex + 1) % MAX_PENDING_FRAME_COUNT;
	WaitForFenceValue(m_LastFenceValues[NextContextIndex]);

	m_ppConstantBufferManager[NextContextIndex]->Reset();
	m_ppDescriptorPools[NextContextIndex]->Reset();
	m_CurContextIndex = NextContextIndex;
}


BOOL CD3D12Renderer::UpdateWindowSize(DWORD dwBackBufferWidth, DWORD dwBackBufferHeight)
{
	BOOL	bResult = FALSE;

	if (!(dwBackBufferWidth * dwBackBufferHeight))
		return FALSE;

	if (m_Width == dwBackBufferWidth && m_Height == dwBackBufferHeight)
		return FALSE;
	Fence();

	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++)
	{
		WaitForFenceValue(m_LastFenceValues[i]);
	}


	DXGI_SWAP_CHAIN_DESC1	desc;
	HRESULT	hr = m_pSwapChain->GetDesc1(&desc);
	if (FAILED(hr))
		__debugbreak();

	for (UINT n = 0; n < SWAP_CHAIN_FRAME_COUNT; n++)
	{
		m_pRenderTargets[n]->Release();
		m_pRenderTargets[n] = nullptr;
	}

	if (m_pDepthStencil)
	{
		m_pDepthStencil->Release();
		m_pDepthStencil = nullptr;
	}

	if (FAILED(m_pSwapChain->ResizeBuffers(SWAP_CHAIN_FRAME_COUNT, dwBackBufferWidth, dwBackBufferHeight, DXGI_FORMAT_R8G8B8A8_UNORM, m_SwapChainFlags)))
	{
		__debugbreak();
	}
	m_CurrentRenderTargetIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	// Create frame resources.
	CD3DX12_CPU_DESCRIPTOR_HANDLE RTVHandle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart());

	// Create a RTV for each frame.
	for (UINT n = 0; n < SWAP_CHAIN_FRAME_COUNT; n++)
	{
		m_pSwapChain->GetBuffer(n, IID_PPV_ARGS(&m_pRenderTargets[n]));
		m_pD3DDevice->CreateRenderTargetView(m_pRenderTargets[n], nullptr, RTVHandle);
		RTVHandle.Offset(1, m_RTVDescriptorSize);
	}


	CreateDepthStencil(dwBackBufferWidth, dwBackBufferHeight);


	m_Width = dwBackBufferWidth;
	m_Height = dwBackBufferHeight;
	m_Viewport.Width = (float)m_Width;
	m_Viewport.Height = (float)m_Height;
	m_ScissorRect.left = 0;
	m_ScissorRect.top = 0;
	m_ScissorRect.right = m_Width;
	m_ScissorRect.bottom = m_Height;
	InitCamera();
	return TRUE;	
}


void CD3D12Renderer::InitCamera()
{
	// 카메라 위치, 카메라 방향, 위쪽 방향을 설정
	m_CameraPos = XMVectorSet(0.0f, 0.0f, -1.0f, 1.0f); // 카메라 위치
	m_CameraDir = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f); // 카메라 방향 (정면을 향하도록 설정)
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); // 위쪽 방향 (일반적으로 y축을 따라 설정)

	
	SetCamera(&m_CameraPos, &m_CameraDir, &Up);
}

void* CD3D12Renderer::CreateBasicMeshObject()
{

	if (CBasicMeshObject* MeshObj = new CBasicMeshObject{})
	{
		MeshObj->Initialize(this);
		return MeshObj;
	}
	return nullptr;
}

void CD3D12Renderer::DeleteBasicMeshObject(void* pMeshObjHandle)
{
	if (CBasicMeshObject* MeshObj = static_cast<CBasicMeshObject*>(pMeshObjHandle))
	{
		for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++)
		{
			WaitForFenceValue(m_LastFenceValues[i]);
		}
		delete MeshObj;
	}
}

bool CD3D12Renderer::InsertVertexDataToMeshObject(void* pMeshObjHandle, const BasicVertex* pVertexList, UINT VertexCount,UINT TriGroupCount)
{
	bool bResult = false; 
	if(CBasicMeshObject* pMeshObject = (CBasicMeshObject*)pMeshObjHandle)
	{
		if(pMeshObject->IsInitialized())
		{
			bResult = pMeshObject->InsertVertexData(pVertexList, VertexCount, TriGroupCount);
		}
	}

	return bResult;
}

bool CD3D12Renderer::InsertTriGroupDataToMeshObject(void* pMeshObjHandle, const WORD* pIndexList, UINT TriangleCount,const WCHAR* TextureFileName)
{
	bool bResult = false;
	if (CBasicMeshObject* pMeshObject = (CBasicMeshObject*)pMeshObjHandle)
	{
		if (pMeshObject->IsInitialized())
		{
			bResult = pMeshObject->InsertIndexedTriangleData(pIndexList, TriangleCount, TextureFileName);
		}
	}
	return bResult;
}

void CD3D12Renderer::EndCreateMesh(void* pMeshObjHandle)
{
	if (CBasicMeshObject* pMeshObject = (CBasicMeshObject*)pMeshObjHandle)
	{
		if (pMeshObject->IsInitialized())
		{
			pMeshObject->EndCreateMesh();
		}
	}
}

void CD3D12Renderer::RenderMeshObject(void* MeshObjHandle , XMMATRIX* pWorldMatrix)
{
	ID3D12GraphicsCommandList* pCommandList = m_ppCommandLists[m_CurContextIndex];

	if (CBasicMeshObject* MeshObj = static_cast<CBasicMeshObject*>(MeshObjHandle)) 
	{
		if(MeshObj->IsInitialized())
		{
			MeshObj->Draw(pCommandList, pWorldMatrix);
		}
	}
#ifdef _DEBUG
	else
	{
		std::cerr << "CD3D12Renderer::RenderMeshObject(void* MeshObjHandle , XMMATRIX* pWorldMatrix)  With nullptr \n";
	}
#endif
}

void* CD3D12Renderer::CreateSpriteObject()
{
	CSpriteObject* pSprObj = new CSpriteObject{};
	pSprObj->Initialize(this);

	return (void*)pSprObj;
}

void* CD3D12Renderer::CreateSpriteObject(const WCHAR* TextureFileName, int PosX, int PosY, int Width, int Height)
{
	CSpriteObject* pSprObj = new CSpriteObject{};
	RECT rect;
	rect.left = PosX;
	rect.top = PosY;
	rect.right = Width;
	rect.bottom = Height;
	pSprObj->Initialize(this, TextureFileName, &rect);

	return (void*)pSprObj;
}

void CD3D12Renderer::DeleteSpriteObject(void* pSpriteObjHandle)
{
	if(CSpriteObject* pSprriteObj = (CSpriteObject*) pSpriteObjHandle)
	{
		for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++)
		{
			WaitForFenceValue(m_LastFenceValues[i]);
		}
		delete pSprriteObj;
	}
}

void CD3D12Renderer::RenderSpriteObject(void* pSpriteObjectHandle, INT PosX, INT PosY, float ScaleX, float ScaleY, float Z)
{

	ID3D12GraphicsCommandList* pCommandList = m_ppCommandLists[m_CurContextIndex];

	if(CSpriteObject* pSpriteObj = (CSpriteObject*)pSpriteObjectHandle)
	{
		XMFLOAT2 Pos = { (float)PosX, (float)PosY };
		XMFLOAT2 Scale = { ScaleX, ScaleY };
		pSpriteObj->Draw(pCommandList, &Pos, &Scale, Z);
	}
#ifdef _DEBUG
	else
	{
		std::cerr << "CD3D12Renderer::RenderSpriteObject(void* pSpriteObjectHandle, INT PosX, INT PosY, float ScaleX, float ScaleY, float Z) With nullptr \n";
	}
#endif

	
}

void CD3D12Renderer::RenderSpriteObject(void* pSprObjHandle, int iPosX, int iPosY, float fScaleX, float fScaleY, const RECT* pRect, float Z, void* pTexHandle)
{
	ID3D12GraphicsCommandList* pCommandList = m_ppCommandLists[m_CurContextIndex];

	if (CSpriteObject* pSpriteObj = (CSpriteObject*)pSprObjHandle)
	{
		XMFLOAT2 Pos = { (float)iPosX , (float)iPosY };
		XMFLOAT2 Scale = { fScaleX, fScaleY };
	
		TextureHandle* pTextureHandle = (TextureHandle*)pTexHandle;
		if(pTextureHandle)
		{
			if(pTextureHandle->pUploadBuffer)
			{
				if(pTextureHandle->bUpdated)
				{
					
					D3DUtil::UpdateTexture(m_pD3DDevice, pCommandList, pTextureHandle->pTexResource, pTextureHandle->pUploadBuffer);
					pTextureHandle->bUpdated = false;
				}
			}
		}

		pSpriteObj->Draw(pCommandList, &Pos, &Scale, pRect, Z, pTextureHandle);
	}
}

void* CD3D12Renderer::CreateFontObject(const WCHAR* FontFamilyName, float FontSize)
{
	FontHandle* pFontHandle = m_pFontManager->CreateFontObject(FontFamilyName, FontSize);
	return pFontHandle;
}

void CD3D12Renderer::DeleteFontObject(void* pFontHandle)
{
	m_pFontManager->DeleteFontObject((FontHandle*)pFontHandle);
}

bool CD3D12Renderer::WriteTextToBitmap(BYTE* pDestImage, UINT DestWidth, UINT DestHeight, UINT DestPitch,
	int* piOutWidth, int* piOutHeight, void* pFontObjHandle, const WCHAR* wchString, DWORD dwLen)
{
	return m_pFontManager->WriteTextToBitmap(pDestImage, DestWidth, DestHeight, DestPitch, piOutWidth, piOutHeight, (FontHandle*)pFontObjHandle, wchString, dwLen);
}


void* CD3D12Renderer::CreateTiledTexture(UINT TexWidth, UINT TexHeight, UINT32 Red, UINT32 Green, UINT32 Blue)
{
	
	TextureHandle* pTextureHandle = nullptr;
	DXGI_FORMAT TextureFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	BYTE* pImage = (BYTE*)malloc(TexWidth * TexHeight * 4);
	memset(pImage, 0, TexWidth * TexHeight * 4);

	BOOL bFirstColorIsWhite = TRUE;

	for (UINT y = 0; y < TexHeight; y++)
	{
		for (UINT x = 0; x < TexWidth; x++)
		{

			RGBA* pDest = (RGBA*)(pImage + (x + y * TexWidth) * 4);

			if ((bFirstColorIsWhite + x) % 2)
			{
				pDest->r = Red;
				pDest->g = Green;
				pDest->b = Blue;
			}
			else
			{
				pDest->r = 0;
				pDest->g = 0;
				pDest->b = 0;
			}
			pDest->a = 255;
		}
		bFirstColorIsWhite ++;
		bFirstColorIsWhite %= 2;
	}
	pTextureHandle = m_pTextureManager->CreateImmutableTexture(TexWidth, TexHeight, TextureFormat, pImage);
	free(pImage);
	pImage = nullptr;
	return pTextureHandle;
}

void* CD3D12Renderer::CreateTextureFromFile(const WCHAR* wchFileName) const 
{
	return m_pTextureManager->CreateTextureFromFile(wchFileName);
}

void* CD3D12Renderer::CreateDynamicTexture(UINT TextureWidth, UINT TextureHeight)
{
	return m_pTextureManager->CreateDynamicTexture(TextureWidth, TextureHeight);
}

void CD3D12Renderer::DeleteTexture(void* pTexHandle)
{
	assert(m_pTextureManager);

	if(TextureHandle* pTextureHandle = (TextureHandle*)pTexHandle)
	{
		for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++)
		{
			WaitForFenceValue(m_LastFenceValues[i]);
		}
		m_pTextureManager->DeleteTexture(pTextureHandle);
	}
	
}

void CD3D12Renderer::UpdateTextureWithImage(void* pTexHandle, const BYTE* pSourceData, UINT SourceWidth, UINT SourceHeight)
{
	if(!pTexHandle)
	{
		return;
	}

	TextureHandle* pTextureHandle = (TextureHandle*)pTexHandle;
	ID3D12Resource* pDestTexResource = pTextureHandle->pTexResource;
	ID3D12Resource* pUploadBuffer = pTextureHandle->pUploadBuffer;

	D3D12_RESOURCE_DESC Desc = pDestTexResource->GetDesc();
	if (SourceWidth > Desc.Width)
	{
		__debugbreak();
	}
	if (SourceHeight > Desc.Height)
	{
		__debugbreak();
	}
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT Footprint;
	UINT	Rows = 0;
	UINT64	RowSize = 0;
	UINT64	TotalBytes = 0;

	m_pD3DDevice->GetCopyableFootprints(&Desc, 0, 1, 0, &Footprint, &Rows, &RowSize, &TotalBytes);

	BYTE* pMappedPtr = nullptr;
	CD3DX12_RANGE writeRange(0, 0);

	HRESULT hr = pUploadBuffer->Map(0, &writeRange, reinterpret_cast<void**>(&pMappedPtr));
	if (FAILED(hr))
		__debugbreak();

	const BYTE* pSrc = pSourceData;
	BYTE* pDest = pMappedPtr;
	for (UINT y = 0; y < SourceHeight; y++)
	{
		memcpy(pDest, pSrc, SourceWidth * 4);
		pSrc += (SourceWidth * 4);
		pDest += Footprint.Footprint.RowPitch;
	}
	// Unmap
	pUploadBuffer->Unmap(0, nullptr);

	pTextureHandle->bUpdated = true;
}


UINT64 CD3D12Renderer::Fence()
{
	m_FenceValue++;
	m_pCommandQueue->Signal(m_pFence, m_FenceValue);
	m_LastFenceValues[m_CurContextIndex] = m_FenceValue;
	return m_FenceValue;
}
void CD3D12Renderer::WaitForFenceValue(UINT64 ExpectedFenceValue)
{

	if (m_pFence->GetCompletedValue() < ExpectedFenceValue)
	{
		m_pFence->SetEventOnCompletion(ExpectedFenceValue, m_hFenceEvent);
		WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}
void CD3D12Renderer::CreateCommandListAndQueue()
{
	D3D12_COMMAND_QUEUE_DESC QueueDesc = {};
	QueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	QueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	HRESULT hr = m_pD3DDevice->CreateCommandQueue(&QueueDesc, IID_PPV_ARGS(&m_pCommandQueue));
	if (FAILED(hr))
	{
		__debugbreak();
	}

	for(UINT i = 0 ; i< MAX_PENDING_FRAME_COUNT ; ++ i)
	{
		ID3D12CommandAllocator* pCommandAllocator = nullptr;
		ID3D12GraphicsCommandList* pCommandList = nullptr;

		if (FAILED(m_pD3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&pCommandAllocator))))
		{
			__debugbreak();
		}

		// Create the command list.
		if (FAILED(m_pD3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, pCommandAllocator, nullptr, IID_PPV_ARGS(&pCommandList))))
		{
			__debugbreak();
		}

		// Command lists are created in the recording state, but there is nothing
		// to record yet. The main loop expects it to be closed, so close it now.
		pCommandList->Close();

		m_ppCommandAllocators[i] = pCommandAllocator;
		m_ppCommandLists[i] = pCommandList;
	}
}


void CD3D12Renderer::CreateFence()
{
	// Create synchronization objects and wait until assets have been uploaded to the GPU.
	if (FAILED(m_pD3DDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence))))
	{
		__debugbreak();
	}

	m_FenceValue = 0;

	// Create an event handle to use for frame synchronization.
	m_hFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}


class CConstantBufferPool* CD3D12Renderer::GetConstantBufferPool(CONSTANT_BUFFER_TYPE Type) const
{
	CConstantBufferManager* pConstBufferManager = m_ppConstantBufferManager[m_CurContextIndex];
	CConstantBufferPool* pConstBufferPool = pConstBufferManager->GetConstantBufferPool(Type);
	return pConstBufferPool;
}

void CD3D12Renderer::GetViewProjMatrix(XMMATRIX* pOutViewMatrix, XMMATRIX* pOutProjMatrix)
{
	*pOutViewMatrix = XMMatrixTranspose(m_matView);
	*pOutProjMatrix = XMMatrixTranspose(m_matProj);
}

void CD3D12Renderer::MoveCamera(float x, float y, float z)
{
	m_CameraPos.m128_f32[0] += x;
	m_CameraPos.m128_f32[1] += y;
	m_CameraPos.m128_f32[2] += z;
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); // 위쪽 방향 (일반적으로 y축을 따라 설정)

	SetCamera(&m_CameraPos, &m_CameraDir, &Up);
}

void CD3D12Renderer::SetCamera(const XMVECTOR* pCameraPos, const XMVECTOR* pCameraDir, const XMVECTOR* pCameraUp)
{
	// view matrix
	m_matView = XMMatrixLookToLH(*pCameraPos, *pCameraDir, *pCameraUp);

	// 시야각 (FOV) 설정 (라디안 단위)
	float fovY = XM_PIDIV4; // 90도 (라디안으로 변환)

	// projection matrix
	float fAspectRatio = (float)m_Width / (float)m_Height;
	float fNear = 0.1f;
	float fFar = 1000.0f;
	m_matProj = XMMatrixPerspectiveFovLH(fovY, fAspectRatio, fNear, fFar);
}

void CD3D12Renderer::PostInitialize()
{

	m_DefaultTextureHandle = (TextureHandle*)CreateTiledTexture(32, 32, 40, 40, 40);
}

bool CD3D12Renderer::CreateDescriptorHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC RTVHeapDesc = {};
	RTVHeapDesc.NumDescriptors = SWAP_CHAIN_FRAME_COUNT;	// SwapChain Buffer 0	| SwapChain Buffer 1
	RTVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	RTVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	if (FAILED(m_pD3DDevice->CreateDescriptorHeap(&RTVHeapDesc, IID_PPV_ARGS(&m_pRTVHeap))))
	{
		__debugbreak();
		return false;
	}
	
	D3D12_DESCRIPTOR_HEAP_DESC DSVHeapDesc = {};
	DSVHeapDesc.NumDescriptors = 1;
	DSVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	DSVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	if (FAILED(m_pD3DDevice->CreateDescriptorHeap(&DSVHeapDesc, IID_PPV_ARGS(&m_pDSVHeap))))
	{
		__debugbreak();
		return false;
	}


	m_RTVDescriptorSize = m_pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_DSVDescriptorSize = m_pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	return true;
}




bool CD3D12Renderer::CreateRenderTarget(UINT Width, UINT Height, IDXGIFactory4* pFactory, HWND hwnd)
{
	bool bResult = false;

	if(pFactory)
	{
		pFactory->AddRef();
		// Describe and create the swap chain.
		{
			DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {};
			SwapChainDesc.Width = Width;
			SwapChainDesc.Height = Height;
			SwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			//swapChainDesc.BufferDesc.RefreshRate.Numerator = m_uiRefreshRate;
			//swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
			SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			SwapChainDesc.BufferCount = SWAP_CHAIN_FRAME_COUNT;
			SwapChainDesc.SampleDesc.Count = 1;
			SwapChainDesc.SampleDesc.Quality = 0;
			SwapChainDesc.Scaling = DXGI_SCALING_NONE;
			SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			SwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
			SwapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
			m_SwapChainFlags = SwapChainDesc.Flags;


			DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
			fsSwapChainDesc.Windowed = TRUE;

			IDXGISwapChain1* SwapChain_1 = nullptr;
			if (FAILED(pFactory->CreateSwapChainForHwnd(m_pCommandQueue, hwnd, &SwapChainDesc, &fsSwapChainDesc, nullptr, &SwapChain_1)))
			{
				__debugbreak();
			}
			//mSwapChain is SwapChain_3
			SwapChain_1->QueryInterface(IID_PPV_ARGS(&m_pSwapChain));
			SwapChain_1->Release();
			SwapChain_1 = nullptr;
			m_CurrentRenderTargetIndex = m_pSwapChain->GetCurrentBackBufferIndex();
		}

		// Create frame resources.
		CD3DX12_CPU_DESCRIPTOR_HANDLE RTVHandle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart());

		for (UINT n = 0; n < SWAP_CHAIN_FRAME_COUNT; n++)
		{
			m_pSwapChain->GetBuffer(n, IID_PPV_ARGS(&m_pRenderTargets[n]));
			m_pD3DDevice->CreateRenderTargetView(m_pRenderTargets[n], nullptr, RTVHandle);
			RTVHandle.Offset(1, m_RTVDescriptorSize);
		}

		pFactory->Release();
	}
	else
	{
		__debugbreak();
	}

	return bResult = true;
}

bool CD3D12Renderer::CreateDepthStencil(UINT Width, UINT Height)
{
	bool bResult = false;
	D3D12_DEPTH_STENCIL_VIEW_DESC DepthStencilViewDesc = {};
	DepthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
	DepthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	DepthStencilViewDesc.Flags = D3D12_DSV_FLAG_NONE;

	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;

	CD3DX12_RESOURCE_DESC depthDesc(
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		0,
		Width,
		Height,
		1,
		1,
		DXGI_FORMAT_R32_TYPELESS,
		1,
		0,
		D3D12_TEXTURE_LAYOUT_UNKNOWN,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	if (FAILED(m_pD3DDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depthDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthOptimizedClearValue,
		IID_PPV_ARGS(&m_pDepthStencil)
	)))
	{
		__debugbreak();
		return bResult;
	}

	m_pDepthStencil->SetName(L"CD3D12Renderer::m_pDepthStencil");

	CD3DX12_CPU_DESCRIPTOR_HANDLE	DSVHandle(m_pDSVHeap->GetCPUDescriptorHandleForHeapStart());
	m_pD3DDevice->CreateDepthStencilView(m_pDepthStencil, &DepthStencilViewDesc, DSVHandle);

	return bResult=true;
}



void CD3D12Renderer::Cleanup()
{
	Fence();
	for(UINT i = 0 ; i < MAX_PENDING_FRAME_COUNT ; ++i)
	{
		WaitForFenceValue(m_LastFenceValues[i]);
	}

	CleanupDescriptorHeap();
	CleanupCommandListAndQueue();
	CleanupResource();


	if(m_pTextureManager)
	{
		delete m_pTextureManager;
		m_pTextureManager = nullptr;
	}

	m_pResourceManager = nullptr;
	m_pSingleDescriptorAllocator = nullptr;
	m_pFontManager = nullptr;
	
	for (UINT i = 0; i < MAX_PENDING_FRAME_COUNT; ++i)
	{
		if(m_ppConstantBufferManager[i])
		{
			delete m_ppConstantBufferManager[i];
			m_ppConstantBufferManager[i] = nullptr;
		}

		if (m_ppDescriptorPools[i])
		{
			delete m_ppDescriptorPools[i];
			m_ppDescriptorPools[i] = nullptr;
		}
	}

	CleanupFence();
	if (m_pD3DDevice)
	{
		ULONG ref_count = m_pD3DDevice->Release();
		if (ref_count>0)
		{
			//resource leak!!!
			IDXGIDebug1* pDebug = nullptr;
			if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
			{
				pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
				pDebug->Release();
			}
			__debugbreak();
		}
		m_pD3DDevice = nullptr;
	}
}



void CD3D12Renderer::CleanupDescriptorHeap()
{
	if (m_pRTVHeap)
	{
		m_pRTVHeap->Release();
		m_pRTVHeap = nullptr;
	}
	if (m_pDSVHeap)
	{
		m_pDSVHeap->Release();
		m_pDSVHeap = nullptr;
	}
	if (m_pSRVHeap)
	{
		m_pSRVHeap->Release();
		m_pSRVHeap = nullptr;
	}
}


void CD3D12Renderer::CleanupCommandListAndQueue()
{
	for (UINT i = 0; i < MAX_PENDING_FRAME_COUNT; ++i)
	{
		ID3D12CommandAllocator* pCommandAllocator = m_ppCommandAllocators[i];
		ID3D12GraphicsCommandList* pCommandList = m_ppCommandLists[i];

		if (pCommandList)
		{
			pCommandList->Release();
			pCommandList = nullptr;
		}
		if (pCommandAllocator)
		{
			pCommandAllocator->Release();
			pCommandAllocator = nullptr;
		}
		m_ppCommandAllocators[i] = nullptr;
		m_ppCommandLists[i] = nullptr;
	}

	if (m_pCommandQueue)
	{
		m_pCommandQueue->Release();
		m_pCommandQueue = nullptr;
	}
}


void CD3D12Renderer::CleanupFence()
{
	if (m_hFenceEvent)
	{
		CloseHandle(m_hFenceEvent);
		m_hFenceEvent = nullptr;
	}
	if (m_pFence)
	{
		m_pFence->Release();
		m_pFence = nullptr;
	}

}



void CD3D12Renderer::CleanupResource()
{
	DeleteTexture(m_DefaultTextureHandle);

	for (DWORD i = 0; i < SWAP_CHAIN_FRAME_COUNT; i++)
	{
		if (m_pRenderTargets[i])
		{
			m_pRenderTargets[i]->Release();
			m_pRenderTargets[i] = nullptr;
		}
	}


	if (m_pSwapChain)
	{
		m_pSwapChain->Release();
		m_pSwapChain = nullptr;
	}

	if(m_pDepthStencil)
	{
		m_pDepthStencil->Release();
		m_pDepthStencil = nullptr;
	}
}