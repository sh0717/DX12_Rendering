#include "pch.h"
#include "SpriteObject.h"

#include <d3dcompiler.h>
#include <iostream>
#include "D3D12Renderer.h"
#include "D3D12ResourceManager.h"
#include "ConstantBufferPool.h"
#include "DescriptorPool.h"


ID3D12RootSignature* CSpriteObject::m_pRootSignature = nullptr;
ID3D12PipelineState* CSpriteObject::m_pPipelineState = nullptr;
UINT CSpriteObject::m_InitRefCount = 0;

ID3D12Resource* CSpriteObject::m_pVertexBuffer = nullptr;
D3D12_VERTEX_BUFFER_VIEW CSpriteObject::m_VertexBufferView = {};

ID3D12Resource* CSpriteObject::m_pIndexBuffer = nullptr;
D3D12_INDEX_BUFFER_VIEW CSpriteObject::m_IndexBufferView = {};


CSpriteObject::CSpriteObject()
{
}

CSpriteObject::~CSpriteObject()
{
	if(bInitialized  == false)
	{
		std::cerr << "UnInitialized Sprite Object\n";
	}
	Cleanup();
}

bool CSpriteObject::Initialize(class CD3D12Renderer* p_renderer)
{
	bool bResult = false;
	m_pRenderer = p_renderer;
	bResult = InitializeCommonResources();

	bInitialized = bResult;
	return bResult;
}

bool CSpriteObject::Initialize(class CD3D12Renderer* p_renderer, const WCHAR* TextureFileName, const RECT* pRect)
{
	bool bResult = false;
	m_pRenderer = p_renderer;

	if(bResult= InitializeCommonResources())
	{
		UINT TexWidth = 1;
		UINT TexHeight = 1;
		if(m_pTextureHandle = (TextureHandle*)m_pRenderer->CreateTextureFromFile(TextureFileName))
		{
			D3D12_RESOURCE_DESC TextureResouceDesc = m_pTextureHandle->pTexResource->GetDesc();
			TexWidth = TextureResouceDesc.Width;
			TexHeight = TextureResouceDesc.Height;
		}
		if(pRect)
		{
			m_TextureRect = *pRect;
			if(m_pTextureHandle)
			{
				m_Scale.x = (float)(m_TextureRect.right - m_TextureRect.left) / (float)TexWidth;
				m_Scale.y = (float)(m_TextureRect.bottom - m_TextureRect.top) / (float)TexHeight;
			}
		}
		else
		{
			if(m_pTextureHandle)
			{
				D3D12_RESOURCE_DESC TextureResouceDesc = m_pTextureHandle->pTexResource->GetDesc();
				m_TextureRect.left = 0;
				m_TextureRect.top = 0;
				m_TextureRect.right = TextureResouceDesc.Width;
				m_TextureRect.bottom = TextureResouceDesc.Height;
			}
		}
	}
	bInitialized = bResult;
	return bResult;
}

void CSpriteObject::Draw(ID3D12GraphicsCommandList* pCommandList, const XMFLOAT2* pPos, const XMFLOAT2* pScale, const RECT* pRect, float Z, TextureHandle* pTexHandle)
{
	if(!pCommandList)
	{
		__debugbreak();
	}

	ID3D12Device5* pD3DDevice = m_pRenderer->GetD3DDevice();
	UINT SRVDescriptorSize = m_pRenderer->Get_CBV_SRV_UAV_DescriptorSize();
	CDescriptorPool* pDescriptorPool = m_pRenderer->GetDescriptorPool();
	ID3D12DescriptorHeap* pDescriptorHeap = pDescriptorPool->GetDescriptorHeap();
	CConstantBufferPool* pConstantBufferPool = m_pRenderer->GetConstantBufferPool(EConstantBufferType::SpriteObject);


	UINT TexWidth = 0;
	UINT TexHeight = 0;

	D3D12_CPU_DESCRIPTOR_HANDLE SRV = {};
	if(pTexHandle)
	{
		D3D12_RESOURCE_DESC desc = pTexHandle->pTexResource->GetDesc();
		TexWidth = desc.Width;
		TexHeight = desc.Height;
		SRV = pTexHandle->SRVHandle;
	}
	else
	{
		TextureHandle* DefaultTexture =  m_pRenderer->GetDefaultTexture();
		if(DefaultTexture)
		{
			D3D12_RESOURCE_DESC desc = DefaultTexture->pTexResource->GetDesc();
			TexWidth = desc.Width;
			TexHeight = desc.Height;
			SRV = DefaultTexture->SRVHandle;
		}
		else
		{
			__debugbreak();
		}
	}

	RECT rect;
	if(!pRect)
	{
		rect.left = 0;
		rect.top = 0;
		rect.right = TexWidth;
		rect.bottom = TexHeight;
		pRect = &rect;
	}

	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptorTable = {};
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDescriptorTable = {};

	if (!pDescriptorPool->AllocateDescriptorTable(&cpuDescriptorTable, &gpuDescriptorTable, DESCRIPTOR_COUNT_FOR_DRAW))
	{
		__debugbreak();
	}
	CB_Container* pCB = pConstantBufferPool->AllocateConstantBuffer();
	if (!pCB)
	{
		__debugbreak();
	}

	CONSTANT_BUFFER_SPRITE* pConstantBufferSprite = (CONSTANT_BUFFER_SPRITE*)pCB->pSystemMemberAddr;

	// constant buffer의 내용을 설정
	pConstantBufferSprite->ScreenRes.x = (float)m_pRenderer->GetScreenWidth();
	pConstantBufferSprite->ScreenRes.y = (float)m_pRenderer->GetScreenHeigt();
	pConstantBufferSprite->Pos = *pPos;
	pConstantBufferSprite->Scale = *pScale;
	pConstantBufferSprite->TexSize.x = (float)TexWidth;
	pConstantBufferSprite->TexSize.y = (float)TexHeight;
	pConstantBufferSprite->TexSamplePos.x = (float)pRect->left;
	pConstantBufferSprite->TexSamplePos.y = (float)pRect->top;
	pConstantBufferSprite->TexSampleSize.x = (float)(pRect->right - pRect->left);
	pConstantBufferSprite->TexSampleSize.y = (float)(pRect->bottom - pRect->top);
	pConstantBufferSprite->Z = Z;
	pConstantBufferSprite->Alpha = 1.0f;

	pCommandList->SetGraphicsRootSignature(m_pRootSignature);
	pCommandList->SetDescriptorHeaps(1, &pDescriptorHeap);



	CD3DX12_CPU_DESCRIPTOR_HANDLE cbvDest(cpuDescriptorTable, SPRITE_DESCRIPTOR_INDEX_CBV_0, SRVDescriptorSize);
	pD3DDevice->CopyDescriptorsSimple(1, cbvDest, pCB->CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	if (SRV.ptr)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE srvDest(cpuDescriptorTable, SPRITE_DESCRIPTOR_INDEX_SRV_0, SRVDescriptorSize);
		pD3DDevice->CopyDescriptorsSimple(1, srvDest, SRV, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
	pCommandList->SetPipelineState(m_pPipelineState);
	pCommandList->SetGraphicsRootDescriptorTable(0, gpuDescriptorTable);

	
	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCommandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
	pCommandList->IASetIndexBuffer(&m_IndexBufferView);
	pCommandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

}

void CSpriteObject::Draw(ID3D12GraphicsCommandList* pCommandList, const XMFLOAT2* pPos, const XMFLOAT2* pScale, float Z)
{
	XMFLOAT2 Scale = { m_Scale.x * pScale->x, m_Scale.y * pScale->y };
	Draw(pCommandList, pPos, &Scale, &m_TextureRect, Z, m_pTextureHandle);
}

bool CSpriteObject::InitializeCommonResources()
{
	if(m_InitRefCount >0)
	{
		m_InitRefCount++;
		return true;
	}

	if(!InitRootSignature())
	{
	#ifdef _DEBUG
		std::cerr << "SpriteObject init RootSignature failed";
	#endif

		return false;
	}

	if(!InitPipelineState())
	{
#ifdef _DEBUG
		std::cerr << "SpriteObject init pipeline";
#endif

		return false;
	}

	if(!InitMesh())
	{
#ifdef _DEBUG
		std::cerr << "SpriteObject init mesh";
#endif

		return false;
	}

	m_InitRefCount++;
	return true;
}

bool CSpriteObject::InitRootSignature()
{
	bool bResult = false;

	ID3D12Device5* pD3DDevice = m_pRenderer->GetD3DDevice();

	ID3DBlob* t_pSignature = nullptr;
	ID3DBlob* t_pError = nullptr;


	CD3DX12_DESCRIPTOR_RANGE Ranges[2] = {};
	Ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	Ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	CD3DX12_ROOT_PARAMETER RootParameters[1] = {};
	RootParameters[0].InitAsDescriptorTable(_countof(Ranges), Ranges, D3D12_SHADER_VISIBILITY_ALL);

	// default sampler
	D3D12_STATIC_SAMPLER_DESC sampler = {};
	SetDefaultSamplerDesc(&sampler, 0);
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;

	// Allow input layout and deny uneccessary access to certain pipeline stages.
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;


	CD3DX12_ROOT_SIGNATURE_DESC RootSignatureDesc = {};
	RootSignatureDesc.Init(_countof(RootParameters), RootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	if (FAILED(D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &t_pSignature, &t_pError)))
	{
		__debugbreak();
	}

	if (FAILED(pD3DDevice->CreateRootSignature(0, t_pSignature->GetBufferPointer(), t_pSignature->GetBufferSize(), IID_PPV_ARGS(&m_pRootSignature))))
	{
		__debugbreak();
	}

	if (t_pSignature)
	{
		t_pSignature->Release();
		t_pSignature = nullptr;
	}
	if (t_pError)
	{
		t_pError->Release();
		t_pError = nullptr;
	}

	bResult = true;
	return bResult;
}

bool CSpriteObject::InitPipelineState()
{
	ID3D12Device5* pD3DDevice = m_pRenderer->GetD3DDevice();
	ID3DBlob* t_pVertexShader = nullptr;
	ID3DBlob* t_pPixelShader = nullptr;

#if defined(_DEBUG)
	// Enable better shader debugging with the graphics debugging tools.
	UINT ShaderCompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT ShaderCompileFlags = 0;
#endif

	ID3DBlob* pErrorBlob = nullptr;
	if (FAILED(D3DCompileFromFile(L"./Shaders/Sprite.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", ShaderCompileFlags, 0, &t_pVertexShader, &pErrorBlob)))
	{
		if (pErrorBlob != nullptr)
		{
			OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
			pErrorBlob->Release();
		}
		__debugbreak();
	}
	if (FAILED(D3DCompileFromFile(L"./Shaders/Sprite.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", ShaderCompileFlags, 0, &t_pPixelShader, &pErrorBlob)))
	{
		if (pErrorBlob != nullptr)
		{
			OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
			pErrorBlob->Release();
		}
		__debugbreak();
	}

	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,	0, 28,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = m_pRootSignature;
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(t_pVertexShader->GetBufferPointer(), t_pVertexShader->GetBufferSize());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(t_pPixelShader->GetBufferPointer(), t_pPixelShader->GetBufferSize());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	//psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleDesc.Count = 1;
	if (FAILED(pD3DDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPipelineState))))
	{
		__debugbreak();
	}


	if(t_pVertexShader)
	{
		t_pVertexShader->Release();
		t_pVertexShader = nullptr;
	}

	if(t_pPixelShader)
	{
		t_pPixelShader->Release();
		t_pPixelShader = nullptr;
	}


	return true;
}

bool CSpriteObject::InitMesh()
{
	bool bResult = false;
	ID3D12Device5* pD3DDevice = m_pRenderer->GetD3DDevice();
	CD3D12ResourceManager* pResourceManager = m_pRenderer->GetResourceManager();


	BasicVertex Vertices[] =
	{
		{ { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
		{ { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } },
		{ { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
		{ { 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
	};

	WORD Indices[] =
	{
		0, 1, 2,
		0, 2, 3
	};

	const UINT VertexBufferSize = sizeof(Vertices);

	if (FAILED(pResourceManager->CreateVertexBuffer(sizeof(BasicVertex), (DWORD)_countof(Vertices), &m_VertexBufferView, &m_pVertexBuffer, Vertices)))
	{
		__debugbreak();
		return bResult;
	}

	if (FAILED(pResourceManager->CreateIndexBuffer((DWORD)_countof(Indices), &m_IndexBufferView, &m_pIndexBuffer, Indices)))
	{
		__debugbreak();
		m_pVertexBuffer->Release();
		return bResult;
	}

	bResult = true;
	return bResult;
}




void CSpriteObject::Cleanup()
{
	if(m_pTextureHandle)
	{
		m_pRenderer->DeleteTexture(m_pTextureHandle);
		m_pTextureHandle = nullptr;
	}
	ClenaupSharedResource();
}

void CSpriteObject::ClenaupSharedResource()
{
	if(!bInitialized)
	{
		return; 
	}

	if(m_InitRefCount == 0 )
	{
		return;
	}

	UINT ref_count = --m_InitRefCount;
	if(ref_count == 0)
	{
		if(m_pRootSignature)
		{
			m_pRootSignature->Release();
			m_pRootSignature = nullptr;
		}

		if(m_pPipelineState)
		{
			m_pPipelineState->Release();
			m_pPipelineState = nullptr;
		}

		if(m_pVertexBuffer)
		{
			m_pVertexBuffer->Release();
			m_pVertexBuffer = nullptr;
		}
		if(m_pIndexBuffer)
		{
			m_pIndexBuffer->Release();
			m_pIndexBuffer = nullptr;
		}
	}
}
