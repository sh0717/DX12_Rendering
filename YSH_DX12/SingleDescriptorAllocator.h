#pragma once
#include "../Util/IndexCreator.h"

class CSingleDescriptorAllocator
{
public: /*function*/

	CSingleDescriptorAllocator();

	~CSingleDescriptorAllocator();

	inline ID3D12DescriptorHeap* GetDescriptorHeap() const { return m_pDescriptorHeap; }

	bool Initialize(ID3D12Device* pDevice, UINT32 MaxCount, D3D12_DESCRIPTOR_HEAP_FLAGS Flag = D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	bool AllocateDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE* pOutCPUDescriptorHandle);
	void FreeDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE CPUDescriptorHandle);

	bool Check(D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHandle);
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandleFromCPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle);



private: /*function*/
	void Cleanup();

private:/*variable*/
	bool bInitialized = false;
	ID3D12Device* m_pD3DDevice = nullptr;
	ID3D12DescriptorHeap* m_pDescriptorHeap = nullptr;
	CIndexCreator m_IndexCreator;
	UINT32 m_DescriptorSize = 0;

};

