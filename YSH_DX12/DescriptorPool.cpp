#include "pch.h"
#include "DescriptorPool.h"

CDescriptorPool::CDescriptorPool()
{
}

CDescriptorPool::~CDescriptorPool()
{
	CleanUp();
}

bool CDescriptorPool::Initialize(ID3D12Device5* pD3DDevice, UINT MaxDescriptorCount)
{
	bool bResult = false;
	if (!(m_pD3DDevice = pD3DDevice)) 
	{
		//INVALID DEVICE!
		__debugbreak();
		return bResult;
	}

	m_pD3DDevice->AddRef();

	m_MaxDescriptorCount = MaxDescriptorCount;
	m_CBV_SRV_UAV_DescriptorSize = m_pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_DESCRIPTOR_HEAP_DESC DescriptorHeapDesc = {};
	DescriptorHeapDesc.NumDescriptors = m_MaxDescriptorCount;
	DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	DescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	HRESULT hrCreateDescriptorHeap = m_pD3DDevice->CreateDescriptorHeap(&DescriptorHeapDesc, IID_PPV_ARGS(&m_pDescriptorHeap));
	if (FAILED(hrCreateDescriptorHeap)) 
	{
		__debugbreak();
		return bResult;
	}

	m_CPUDescriptorHandle = m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_GPUDescriptorHandle = m_pDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

	//SUCCESS
	bResult = true;
	return bResult;
}

bool CDescriptorPool::AllocateDescriptorTable( D3D12_CPU_DESCRIPTOR_HANDLE* pOutCPUDescriptor,  D3D12_GPU_DESCRIPTOR_HANDLE* pOutGPUDescriptor, UINT DescriptorCount)
{
	bool bResult = false;

	if (m_AllocatedDescriptorCount + DescriptorCount > m_MaxDescriptorCount) 
	{
		return false;
	}
	
	*pOutCPUDescriptor = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_CPUDescriptorHandle, m_AllocatedDescriptorCount, m_CBV_SRV_UAV_DescriptorSize);
	*pOutGPUDescriptor = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_GPUDescriptorHandle, m_AllocatedDescriptorCount, m_CBV_SRV_UAV_DescriptorSize);
	m_AllocatedDescriptorCount += DescriptorCount;

	bResult = true;
	return bResult;
}



void CDescriptorPool::Reset()
{
	m_AllocatedDescriptorCount = 0;
}

void CDescriptorPool::CleanUp()
{
	if (m_pDescriptorHeap) 
	{
		m_pDescriptorHeap->Release();
		m_pDescriptorHeap = nullptr;
	}

	if(m_pD3DDevice)
	{
		m_pD3DDevice->Release();
		m_pD3DDevice = nullptr;
	}
}

