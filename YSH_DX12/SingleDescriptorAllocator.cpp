#include "pch.h"
#include "SingleDescriptorAllocator.h"

CSingleDescriptorAllocator::CSingleDescriptorAllocator()
	:m_IndexCreator()
{
}

CSingleDescriptorAllocator::~CSingleDescriptorAllocator()
{
	Cleanup();
}

bool CSingleDescriptorAllocator::Initialize(ID3D12Device* pDevice, UINT32 MaxCount, D3D12_DESCRIPTOR_HEAP_FLAGS Flag)
{
	bool bResult = false; 
	if(!pDevice)
	{
		__debugbreak();
		return bResult;
	}
	m_pD3DDevice = pDevice;
	m_pD3DDevice->AddRef();

	assert(MaxCount > 0);


	D3D12_DESCRIPTOR_HEAP_DESC DescriptorHeapDesc = {};
	DescriptorHeapDesc.NumDescriptors = MaxCount;
	DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	DescriptorHeapDesc.Flags = Flag;

	if(FAILED(m_pD3DDevice->CreateDescriptorHeap(&DescriptorHeapDesc, IID_PPV_ARGS(&m_pDescriptorHeap))))
	{
		__debugbreak();
		return bResult;
	}

	m_DescriptorSize = m_pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_IndexCreator.Initialize(MaxCount);

	bInitialized = true;

	bResult = true;
	return bResult;
}

bool CSingleDescriptorAllocator::AllocateDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE* pOutCPUDescriptorHandle)
{
	bool bResult = false;
	
	if(!bInitialized)
	{
		__debugbreak();
		return bResult;
	}

	INT32 Index = m_IndexCreator.Allocate();
	if(Index != CIndexCreator::INDEX_NONE)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE DescriptorHandle(m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),Index, m_DescriptorSize);
		*pOutCPUDescriptorHandle = DescriptorHandle;
		bResult = true;
	}

	return bResult;
}

void CSingleDescriptorAllocator::FreeDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE CPUDescriptorHandle)
{
	if (!bInitialized)
	{
		__debugbreak();
		return;
	}
	D3D12_CPU_DESCRIPTOR_HANDLE base = m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	if(base.ptr > CPUDescriptorHandle.ptr)
	{
		__debugbreak();
	}

	UINT32 Index = (UINT32)(CPUDescriptorHandle.ptr - base.ptr) / m_DescriptorSize;
	m_IndexCreator.Free(Index);
}

bool CSingleDescriptorAllocator::Check(D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHandle)
{
	if(bInitialized == false)
	{
		__debugbreak();
		return false;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE base = m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	if(base.ptr > DescriptorHandle.ptr)
	{
		__debugbreak();
		return false;
	}

	return true;
}

D3D12_GPU_DESCRIPTOR_HANDLE CSingleDescriptorAllocator::GetGPUHandleFromCPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle)
{
	if (bInitialized == false)
	{
		__debugbreak();
	}

	D3D12_CPU_DESCRIPTOR_HANDLE Base = m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
#ifdef _DEBUG
	if(Base.ptr > CPUHandle.ptr)
	{
		__debugbreak();
	}
#endif

	UINT32 Index = (UINT32)(CPUHandle.ptr - Base.ptr) / m_DescriptorSize;
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(m_pDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), Index, m_DescriptorSize);

	return gpuHandle;
}

void CSingleDescriptorAllocator::Cleanup()
{

#ifdef _DEBUG
	m_IndexCreator.Check();
#endif
	if(m_pDescriptorHeap)
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
