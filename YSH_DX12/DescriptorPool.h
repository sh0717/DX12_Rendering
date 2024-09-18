#pragma once

/*
	Descriptor Pool has Descriptor heap for Renderer 

	Allocate Descriptor using AllocDescriptorTable function and 
	copy your Descriptor to Allocated Descriptor 

	After Present in Renderer you need to reset Descriptor pool 
*/


class CDescriptorPool
{
public:/*function*/
	CDescriptorPool();

	~CDescriptorPool();

	bool Initialize( ID3D12Device5* pD3DDevice, UINT MaxDescriptorCount);

	bool AllocateDescriptorTable( D3D12_CPU_DESCRIPTOR_HANDLE* pOutCPUDescriptor , D3D12_GPU_DESCRIPTOR_HANDLE* pOutGPUDescriptor,UINT DescriptorCount );

	/*Reset all allocated descriptor 
	  Allocate Descriptor from ZERO 
	*/
	void Reset();


	inline ID3D12DescriptorHeap* GetDescriptorHeap() const { return m_pDescriptorHeap; }
	UINT Get_CBV_SRV_UAV_DescriptorSize() const{ return m_CBV_SRV_UAV_DescriptorSize; }
private:		/*function*/
	void CleanUp();
//
private:		/*variable*/

	ID3D12Device5* m_pD3DDevice = nullptr;
	UINT m_AllocatedDescriptorCount = 0;
	UINT m_MaxDescriptorCount = 0;
	UINT m_CBV_SRV_UAV_DescriptorSize = 0;
	ID3D12DescriptorHeap* m_pDescriptorHeap = nullptr;
	
	D3D12_CPU_DESCRIPTOR_HANDLE m_CPUDescriptorHandle = {};
	D3D12_GPU_DESCRIPTOR_HANDLE m_GPUDescriptorHandle = {};

};

