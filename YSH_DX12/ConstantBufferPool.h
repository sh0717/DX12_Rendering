#pragma once

/*
 *
 *CPU Write - GPU Read
pGPUMemberAddr is resource GPU-���� address (���� �޸�)
pSystemMemberAddr is mapped address

System Side �� Constantbuffer memory �� �����Ѵ� 
*/

struct CB_Container 
{
	D3D12_CPU_DESCRIPTOR_HANDLE CBVHandle;
	D3D12_GPU_VIRTUAL_ADDRESS pGPUMemberAddr;
	UINT8* pSystemMemberAddr;
}; 


/*

	Constant Buffer Pool

	Heap 
	Descriptor Array

	Memory 
	Buffer Array


	After Allocate Constant Buffer 

	You will get CB_Container 
	and Write through pSystemMemberAddr 
	and copy CBVHandle to Shader Visible Descriptor


	And AllocateConstantBuffer only allocate one buffer at one call
*/


class CConstantBufferPool
{
public: /*function*/
	CConstantBufferPool();

	~CConstantBufferPool();

	bool Initialize(ID3D12Device* pD3DDevice, CONSTANT_BUFFER_TYPE type , UINT SizePerCBV, UINT MaxCBVCount);

	CB_Container* AllocateConstantBuffer();

	void Reset();
private:
	void Cleanup();
private:
	CB_Container* m_pCBContainerList = nullptr;
	
	ID3D12DescriptorHeap* m_pCBVHeap = nullptr;

	ID3D12Resource* m_pResource = nullptr;

	UINT8* m_pSystemMemberAddr = nullptr;

	UINT m_SizePerCBV = 0;
	UINT m_MaxCBVCount = 0;
	UINT m_AllocatedCBVCount = 0;


	CONSTANT_BUFFER_TYPE mConstantBufferType;
};

