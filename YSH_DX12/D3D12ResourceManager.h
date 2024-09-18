#pragma once


/*You Can use CreateVertexBuffer,CreateIndexBuffer after it is Initialized using D3D12Device!!!!*/
/*그리고 ResourceManager 는 D3DRenderer 를 통해서 접근할 수 있습니다*/
class CD3D12ResourceManager
{
public:
	CD3D12ResourceManager();
	~CD3D12ResourceManager();

	bool Initialize(ID3D12Device5* pDevice);

	HRESULT CreateVertexBuffer(UINT SizePerVertex, DWORD VertexNum, OUT D3D12_VERTEX_BUFFER_VIEW* pVertexBufferView, OUT ID3D12Resource** ppVertexBuffer, IN void* pInitData);

	HRESULT CreateIndexBuffer(DWORD IndexNum, OUT D3D12_INDEX_BUFFER_VIEW* pIndexBufferView, OUT ID3D12Resource** ppBuffer, IN void* pInitData);

	void	UpdateTextureForWrite(ID3D12Resource* pDestTexResource, ID3D12Resource* pSrcTexResource);
	bool	CreateTexture(ID3D12Resource** ppResource, UINT Width, UINT Height, DXGI_FORMAT format, const BYTE* pInitImage);
	bool	CreateTexturePair(ID3D12Resource** ppOutTextureResource, ID3D12Resource** ppOutUploadBuffer, UINT Width, UINT Height, DXGI_FORMAT Format);
	bool CreateTextureFromFile(ID3D12Resource** ppOutResource, D3D12_RESOURCE_DESC* pOutResourceDesc, const WCHAR* FileName);

private:/*function*/
	bool CreateCommandListAndQueue();
	bool CreateFence();
	
	void CleanUp();
	void CleanUpCommandList();
	void CleanUpFence();

	UINT64	Fence();
	void	WaitForFenceValue();

private:/*variable*/
	ID3D12Device5* m_pD3DDevice = nullptr;

	ID3D12CommandQueue* m_pCommandQueue = nullptr;
	ID3D12CommandAllocator* m_pCommandAllocator = nullptr;
	// THE Name of command list class is "ID3D12GraphicsCommandList"
	ID3D12GraphicsCommandList* m_pCommandList = nullptr; 

	HANDLE	m_FenceEvent = nullptr;
	ID3D12Fence* m_pFence = nullptr;
	UINT64	m_FenceValue = 0;
};

