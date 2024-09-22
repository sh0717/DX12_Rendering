#include "pch.h"
#include "typedef.h"
#include <d3dcompiler.h>
#include <d3d12.h>
#include "../D3D_Util/D3DUtil.h"
#include "D3D12Renderer.h"


#include "D3D12ResourceManager.h"
#include "BasicMeshObject.h"
#include "DescriptorPool.h"
#include <iostream>
#include "ConstantBufferPool.h"

ID3D12RootSignature* CBasicMeshObject::m_pRootSignature = nullptr;
ID3D12PipelineState* CBasicMeshObject::m_pPipelineState = nullptr;
int CBasicMeshObject::m_InitRefCount = 0;


CBasicMeshObject::CBasicMeshObject()
{

}

CBasicMeshObject::~CBasicMeshObject()
{
	if(bInitialized == false)
	{
		std::cerr << "UnInitialized Basic Mesh Object\n";
	}

	Cleanup();
}

bool CBasicMeshObject::Initialize(class CD3D12Renderer* Renderer)
{
	m_pRenderer = Renderer;
	bool bResult = InitCommonResources();
	bInitialized = bResult;
	return bResult;
}

bool CBasicMeshObject::InitCommonResources()
{
	if (m_InitRefCount) 
	{
		m_InitRefCount++; 
		return true;
	}
	else 
	{
		if(!InitRootSinagture())
		{
#ifdef _DEBUG
			std::cerr << "Basic mesh object root signature init fail!\n";
#endif

			return false;
		}

		if(!InitPipelineState())
		{
#ifdef _DEBUG
			std::cerr << "Basic mesh object PSO init fail!\n";
#endif
			return false;
		}
		m_InitRefCount++;
		return true;

	}
}

bool CBasicMeshObject::InitRootSinagture()
{
	
	ID3D12Device5* pD3DDevice = m_pRenderer->GetD3DDevice();
	if (pD3DDevice) 
	{
		ID3DBlob* t_pSignature = nullptr;
		ID3DBlob* t_pError = nullptr;

		CD3DX12_DESCRIPTOR_RANGE rangesPerObj[1] = {};
		rangesPerObj[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);	// b0 : Constant Buffer View per Object

		CD3DX12_DESCRIPTOR_RANGE rangesPerTriGroup[1] = {};
		rangesPerTriGroup[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);	// t0 : Shader Resource View(Tex) per Tri-Group

		CD3DX12_ROOT_PARAMETER RootParameters[2] = {};
		RootParameters[0].InitAsDescriptorTable(_countof(rangesPerObj), rangesPerObj, D3D12_SHADER_VISIBILITY_ALL);
		RootParameters[1].InitAsDescriptorTable(_countof(rangesPerTriGroup), rangesPerTriGroup, D3D12_SHADER_VISIBILITY_ALL);

		D3D12_STATIC_SAMPLER_DESC Sampler = {};
		SetDefaultSamplerDesc(&Sampler, 0);
		Sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;

		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

		CD3DX12_ROOT_SIGNATURE_DESC RootSignatureDesc;
		RootSignatureDesc.Init(_countof(RootParameters), RootParameters, 1, &Sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		if (FAILED(D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &t_pSignature, &t_pError)))
		{
			__debugbreak();
		}
		
		if (FAILED(pD3DDevice->CreateRootSignature(0,t_pSignature->GetBufferPointer(), t_pSignature->GetBufferSize(), IID_PPV_ARGS(&m_pRootSignature)))) 
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
	}
	else 
	{
		__debugbreak();
		return false;
	}
	return true;
}

bool CBasicMeshObject::InitPipelineState()
{
	ID3D12Device5* pD3DDevice = m_pRenderer->GetD3DDevice();
	if (!pD3DDevice) 
	{
		__debugbreak();
		return false;
	}

	ID3DBlob* t_pVertexShader = nullptr;
	ID3DBlob* t_pPixelShader = nullptr;


	UINT CompileFlags = 0;
#if defined(_DEBUG)
	// Enable better shader debugging with the graphics debugging tools.
	 CompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	if (FAILED(D3DCompileFromFile(L"./Shaders/DefaultShader.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", CompileFlags, 0, &t_pVertexShader, nullptr)))
	{
		__debugbreak();
	}
	if (FAILED(D3DCompileFromFile(L"./Shaders/DefaultShader.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", CompileFlags, 0, &t_pPixelShader, nullptr)))
	{
		__debugbreak();
	}


		//VertexBuffer 랑 상호간에 맞춰야 됨
	D3D12_INPUT_ELEMENT_DESC InputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },  //last float is set by one 
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{"TEXCOORD" , 0 , DXGI_FORMAT_R32G32_FLOAT, 0 ,28 ,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA , 0  } 
	};
	D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc = {};
	PSODesc.InputLayout = { InputElementDescs, _countof(InputElementDescs) };
	PSODesc.pRootSignature = m_pRootSignature;
	PSODesc.VS = CD3DX12_SHADER_BYTECODE(t_pVertexShader->GetBufferPointer(), t_pVertexShader->GetBufferSize());
	PSODesc.PS = CD3DX12_SHADER_BYTECODE(t_pPixelShader->GetBufferPointer(), t_pPixelShader->GetBufferSize());
	PSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	PSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	PSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	PSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	PSODesc.DepthStencilState.StencilEnable = FALSE;

	PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	PSODesc.SampleMask = UINT_MAX;
	PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	PSODesc.NumRenderTargets = 1;
	PSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	PSODesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	PSODesc.SampleDesc.Count = 1;
	if (FAILED(pD3DDevice->CreateGraphicsPipelineState(&PSODesc, IID_PPV_ARGS(&m_pPipelineState))))
	{
		__debugbreak();
	}

	if (t_pVertexShader)
	{
		t_pVertexShader->Release();
		t_pVertexShader = nullptr;
	}
	if (t_pPixelShader)
	{
		t_pPixelShader->Release();
		t_pPixelShader = nullptr;
	}
	return true;
}




void CBasicMeshObject::Draw(ID3D12GraphicsCommandList* pCommandList, const XMMATRIX* pWorldMatrix )
{
	
	ID3D12Device5* pD3DDevice = m_pRenderer->GetD3DDevice();
	CDescriptorPool* pDescriptorPool = m_pRenderer->GetDescriptorPool();
	CConstantBufferPool* pConstantBufferPool = m_pRenderer->GetConstantBufferPool(EConstantBufferType::MeshObject);

	if(pDescriptorPool == nullptr)
	{
		__debugbreak();
	}
	if(pConstantBufferPool == nullptr)
	{
		__debugbreak();
	}

	UINT CBV_SRV_UAV_DescriptorSize = pDescriptorPool->Get_CBV_SRV_UAV_DescriptorSize();
	ID3D12DescriptorHeap* pDescriptorHeap = pDescriptorPool->GetDescriptorHeap();
	
	if( (!pDescriptorHeap) ||(!pD3DDevice))
	{
		DebugBreak();
	}


	CD3DX12_CPU_DESCRIPTOR_HANDLE CPUDescriptorTable = {};
	CD3DX12_GPU_DESCRIPTOR_HANDLE GPUDescriptorTable = {};
	DWORD RequiredDescriptorCount = BASIC_MESH_DESCRIPTOR_COUNT_PER_OBJ + (m_TriGroupCount * BASIC_MESH_DESCRIPTOR_COUNT_PER_TRI_GROUP);


	if(pDescriptorPool->AllocateDescriptorTable(&CPUDescriptorTable , &GPUDescriptorTable , RequiredDescriptorCount) == false)
	{
		__debugbreak();
	}

	CB_Container* pCB = pConstantBufferPool->AllocateConstantBuffer();
	if(!pCB)
	{
		__debugbreak();
	}
	CONSTANT_BUFFER_DEFAULT* pConstantBufferDefault = (CONSTANT_BUFFER_DEFAULT*)pCB->pSystemMemberAddr;
	// constant buffer의 내용을 설정
	pConstantBufferDefault->matWorld = XMMatrixTranspose(*pWorldMatrix);
	m_pRenderer->GetViewProjMatrix(&pConstantBufferDefault->matView, &pConstantBufferDefault->matProj);
	
	CD3DX12_CPU_DESCRIPTOR_HANDLE cbvDest(CPUDescriptorTable, BASIC_MESH_DESCRIPTOR_INDEX_PER_OBJ_CBV_0, CBV_SRV_UAV_DescriptorSize);
	pD3DDevice->CopyDescriptorsSimple(1, cbvDest, pCB->CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);	// cpu측 코드에서는 cpu descriptor handle에만 write가능
	

	

	CD3DX12_CPU_DESCRIPTOR_HANDLE PerTriDest(CPUDescriptorTable, BASIC_MESH_DESCRIPTOR_COUNT_PER_OBJ, CBV_SRV_UAV_DescriptorSize);
	for(UINT i = 0 ; i < m_TriGroupCount ; ++i)
	{
		IndexedTriGroup* pTriGroup = m_pTriGroupList + i;
		TextureHandle* pTexHandle = pTriGroup->pTextureHandle;
		if (pTexHandle)
		{
			pD3DDevice->CopyDescriptorsSimple(1, PerTriDest, pTexHandle->SRVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
		else
		{
			pD3DDevice->CopyDescriptorsSimple(1, PerTriDest, m_pRenderer->GetDefaultTexture()->SRVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

		PerTriDest.Offset(1, CBV_SRV_UAV_DescriptorSize);
	}

	
	
	pCommandList->SetGraphicsRootSignature(m_pRootSignature);
	pCommandList->SetDescriptorHeaps(1, &pDescriptorHeap);
	

	pCommandList->SetPipelineState(m_pPipelineState);
	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCommandList->IASetVertexBuffers(0, 1, &m_VerTexBufferView);


	pCommandList->SetGraphicsRootDescriptorTable(0, GPUDescriptorTable);

	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptorTableForTriGroup(GPUDescriptorTable, BASIC_MESH_DESCRIPTOR_COUNT_PER_OBJ, CBV_SRV_UAV_DescriptorSize);
	for (DWORD i = 0; i < m_TriGroupCount; i++)
	{
		// set descriptor table for root-param 1
		pCommandList->SetGraphicsRootDescriptorTable(1, gpuDescriptorTableForTriGroup);	// Entry of Tri-Groups
		gpuDescriptorTableForTriGroup.Offset(1, CBV_SRV_UAV_DescriptorSize);

		IndexedTriGroup* pTriGroup = m_pTriGroupList + i;
		pCommandList->IASetIndexBuffer(&pTriGroup->IndexBufferView);
		pCommandList->DrawIndexedInstanced(pTriGroup->TriangleCount * 3, 1, 0, 0, 0);
	}
}

bool CBasicMeshObject::InsertVertexData(const BasicVertex* pVertexList, UINT VertexCount, UINT TrigroupCount)
{
	bool bResult = false;

	assert(IsInitialized());

	ID3D12Device5* pD3DDevice = m_pRenderer->GetD3DDevice();
	CD3D12ResourceManager* pResourceManager = m_pRenderer->GetResourceManager();

	if(TrigroupCount > MAX_TRI_GROUP_COUNT_PER_OBJ)
	{
		__debugbreak();
		return false;
	}

	if(FAILED(pResourceManager->CreateVertexBuffer(sizeof(BasicVertex), VertexCount , &m_VerTexBufferView , & m_pVertexBuffer , (void*)pVertexList)))
	{
		__debugbreak();
		return false;
	}
	
	m_TriGroupMaxCount = TrigroupCount;
	m_pTriGroupList = new IndexedTriGroup[m_TriGroupMaxCount];
	memset(m_pTriGroupList, 0, sizeof(IndexedTriGroup) * m_TriGroupMaxCount);

	bResult = true;
	return bResult;
}

bool CBasicMeshObject::InsertIndexedTriangleData(const WORD* pIndexList, UINT TriangleCount, const WCHAR* TextureFileName)
{
	bool bResult = false;

	ID3D12Device5* pD3DDevice = m_pRenderer->GetD3DDevice();
	CD3D12ResourceManager* pResourceManager = m_pRenderer->GetResourceManager();

	ID3D12Resource* pIndexBuffer = nullptr;
	D3D12_INDEX_BUFFER_VIEW IndexBufferView = {};

	if(m_TriGroupCount >= m_TriGroupMaxCount)
	{
		__debugbreak();
		return bResult;
	}

	if(FAILED(pResourceManager->CreateIndexBuffer(TriangleCount*3 , & IndexBufferView , & pIndexBuffer , (void*)pIndexList)))
	{
		__debugbreak();
		return bResult;
	}

	IndexedTriGroup* pTriGroup = m_pTriGroupList + m_TriGroupCount;
	pTriGroup->pIndexBuffer = pIndexBuffer;
	pTriGroup->IndexBufferView = IndexBufferView;
	pTriGroup->TriangleCount = TriangleCount;
	pTriGroup->pTextureHandle = (TextureHandle*)m_pRenderer->CreateTextureFromFile(TextureFileName);
	m_TriGroupCount++;

	bResult = true;
	return bResult;
}

void CBasicMeshObject::EndCreateMesh()
{

}

//Clean Start 
void CBasicMeshObject::Cleanup()
{
	if (m_pVertexBuffer) 
	{
		m_pVertexBuffer->Release();
		m_pVertexBuffer = nullptr;
	}

	if(m_pTriGroupList)
	{
		for(UINT i = 0 ; i< m_TriGroupCount ; ++i)
		{
			if(m_pTriGroupList[i].pIndexBuffer)
			{
				m_pTriGroupList[i].pIndexBuffer->Release();
				m_pTriGroupList[i].pIndexBuffer = nullptr;
			}

			if(m_pTriGroupList[i].pTextureHandle)
			{
				m_pRenderer->DeleteTexture(m_pTriGroupList[i].pTextureHandle);
				m_pTriGroupList[i].pTextureHandle = nullptr;
			}
		}
		delete[] m_pTriGroupList;
		m_pTriGroupList = nullptr;
	}
	CleanupSharedResources();
}



void CBasicMeshObject::CleanupSharedResources()
{
	if(bInitialized == false)
	{
		return;
	}

	if (m_InitRefCount == 0) 
	{
		return;
	}

	DWORD ref_count = --m_InitRefCount;
	if (ref_count == 0)
	{
		if (m_pRootSignature) 
		{
			m_pRootSignature->Release();
			m_pRootSignature = nullptr;
		}

		if (m_pPipelineState) 
		{
			m_pPipelineState->Release();
			m_pPipelineState = nullptr;
		}
	}
}
//Clean END 