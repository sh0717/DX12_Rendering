#include "pch.h"
#include "ConstantBufferPool.h"

/*Upload Resource 는  CPU 로 매핑 가능 */

CConstantBufferPool::CConstantBufferPool()
{
}

CConstantBufferPool::~CConstantBufferPool()
{
	Cleanup();
}

bool CConstantBufferPool::Initialize(ID3D12Device* pD3DDevice, EConstantBufferType type, UINT SizePerCBV, UINT MaxCBVCount)
{
	bool bResult = false;

	if(!pD3DDevice)
	{
		__debugbreak();
		return bResult;
	}
	mConstantBufferType = type;
	m_MaxCBVCount = MaxCBVCount;
	m_SizePerCBV = SizePerCBV;
	UINT ByteWidth = m_MaxCBVCount * m_SizePerCBV;

	HRESULT hrCreateResource = pD3DDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(ByteWidth),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_pResource)
	);

	if(FAILED(hrCreateResource))
	{
		__debugbreak();
		return bResult;
	}

	D3D12_DESCRIPTOR_HEAP_DESC CBVHeapDesc = {};
	CBVHeapDesc.NumDescriptors = m_MaxCBVCount;
	CBVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	CBVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	HRESULT hrCreateDescriptorHeap = pD3DDevice->CreateDescriptorHeap(&CBVHeapDesc, IID_PPV_ARGS(&m_pCBVHeap));
	if(FAILED(hrCreateDescriptorHeap))
	{
		__debugbreak();
		return bResult;
	}

	m_pCBContainerList = new CB_Container[m_MaxCBVCount];

	CD3DX12_RANGE WriteRange(0, 0);
	m_pResource->Map(0, &WriteRange, reinterpret_cast<void**>(&m_pSystemMemberAddr));

	D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc = {};
	CBVDesc.BufferLocation = m_pResource->GetGPUVirtualAddress();
	CBVDesc.SizeInBytes = m_SizePerCBV;

	UINT8* pSystemMember = m_pSystemMemberAddr;

	CD3DX12_CPU_DESCRIPTOR_HANDLE DescriptorHandle(m_pCBVHeap->GetCPUDescriptorHandleForHeapStart());
	UINT	DescriptorSize = pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	for(UINT i = 0 ; i <m_MaxCBVCount ; ++i)
	{
		pD3DDevice->CreateConstantBufferView(&CBVDesc, DescriptorHandle);

		m_pCBContainerList[i].CBVHandle = DescriptorHandle;
		m_pCBContainerList[i].pGPUMemberAddr = CBVDesc.BufferLocation;
		m_pCBContainerList[i].pSystemMemberAddr = pSystemMember;

		DescriptorHandle.Offset(1, DescriptorSize);
		CBVDesc.BufferLocation += m_SizePerCBV;
		pSystemMember += m_SizePerCBV;
	}

	bResult = true;
	return bResult;
}

CB_Container* CConstantBufferPool::AllocateConstantBuffer()
{
	CB_Container* pCB_Container = nullptr;

	if(m_AllocatedCBVCount >= m_MaxCBVCount)
	{
		return pCB_Container/*=nullptr*/;
	}

	pCB_Container = m_pCBContainerList + m_AllocatedCBVCount;
	m_AllocatedCBVCount++;

	return pCB_Container;
}


void CConstantBufferPool::Reset()
{
	m_AllocatedCBVCount = 0;
}

void CConstantBufferPool::Cleanup()
{
	if(m_pCBContainerList)
	{
		delete[] m_pCBContainerList;
		m_pCBContainerList = nullptr;
	}
	
	if(m_pCBVHeap)
	{
		m_pCBVHeap->Release();
		m_pCBVHeap = nullptr;
	}

	if(m_pResource)
	{
		m_pResource->Release();
		m_pResource = nullptr;
	}
}
