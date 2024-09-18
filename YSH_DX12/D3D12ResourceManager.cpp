#include "pch.h"
#include "D3D12ResourceManager.h"

#include <iostream>

#include "../DirectXTex/DDSTextureLoader/DDSTextureLoader12.h"
#include "../DirectXTex/DirectXTex/DirectXTex.h"

CD3D12ResourceManager::CD3D12ResourceManager()
{
}

CD3D12ResourceManager::~CD3D12ResourceManager()
{
	CleanUp();
}

bool CD3D12ResourceManager::Initialize(ID3D12Device5* pDevice)
{
	bool bResult = false;
	if (m_pD3DDevice = pDevice)
	{
		m_pD3DDevice->AddRef();
		if (CreateCommandListAndQueue() && CreateFence()) 
		{
			bResult = true;
		}
	}
	return bResult;
}

HRESULT CD3D12ResourceManager::CreateVertexBuffer(UINT SizePerVertex, DWORD VertexNum, OUT D3D12_VERTEX_BUFFER_VIEW* pVertexBufferView, OUT ID3D12Resource** ppVertexBuffer, IN void* pInitData) 
{
	HRESULT hr = S_OK;
	if (!m_pD3DDevice) 
	{
		/*You Should Initialize ResourceManager First*/
		__debugbreak();
		return hr = E_FAIL;
	}

	D3D12_VERTEX_BUFFER_VIEW VertexBufferView{};
	ID3D12Resource* pVertexBuffer = nullptr;
	ID3D12Resource* _pUploadBuffer = nullptr;
	UINT VertexBufferSize = SizePerVertex * VertexNum;


	hr = m_pD3DDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), 
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(VertexBufferSize),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&pVertexBuffer)
	);
	if (FAILED(hr)) 
	{
		__debugbreak();
		return hr;
	}

	/*If Initial Data make UploadBuffer and Send To GPU*/
	if (pInitData) 
	{
		if (FAILED(m_pCommandAllocator->Reset())) 
		{
			__debugbreak();
			return hr = E_FAIL;
		}
		
		if (FAILED(m_pCommandList->Reset(m_pCommandAllocator, nullptr))) 
		{
			__debugbreak();
			return hr = E_FAIL;
		}

		hr = m_pD3DDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(VertexBufferSize),
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&_pUploadBuffer));

		if (FAILED(hr))
		{
			__debugbreak();
			return hr;
		}

		// Copy the triangle data to the vertex buffer.
		UINT8* pVertexDataBegin = nullptr;
		CD3DX12_RANGE writeRange(0, 0);        // We do not intend to read from this resource on the CPU.

		hr = _pUploadBuffer->Map(0, &writeRange, reinterpret_cast<void**>(&pVertexDataBegin));
		if (FAILED(hr))
		{
			__debugbreak();
			return hr;
		}
		memcpy(pVertexDataBegin, pInitData, VertexBufferSize);
		_pUploadBuffer->Unmap(0, nullptr);

		m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pVertexBuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
		m_pCommandList->CopyBufferRegion(pVertexBuffer, 0, _pUploadBuffer, 0, VertexBufferSize);
		m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pVertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

		m_pCommandList->Close();

		ID3D12CommandList* ppCommandLists[] = { m_pCommandList };
		m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		Fence();
		WaitForFenceValue();

	}

	VertexBufferView.BufferLocation = pVertexBuffer->GetGPUVirtualAddress();
	VertexBufferView.StrideInBytes = SizePerVertex;
	VertexBufferView.SizeInBytes = VertexBufferSize;
	*pVertexBufferView = VertexBufferView;
	*ppVertexBuffer = pVertexBuffer;
	
	
	if (_pUploadBuffer) 
	{
		_pUploadBuffer->Release();
		_pUploadBuffer = nullptr;
	}


	return hr;
}

HRESULT CD3D12ResourceManager::CreateIndexBuffer(DWORD IndexNum, OUT D3D12_INDEX_BUFFER_VIEW* pIndexBufferView, OUT ID3D12Resource** ppBuffer,IN void* pInitData)
{
	HRESULT hr = S_OK;

	D3D12_INDEX_BUFFER_VIEW IndexBufferView{};
	ID3D12Resource* pIndexBuffer = nullptr;
	ID3D12Resource* _pUploadBuffer = nullptr;
	UINT IndexBufferSize = sizeof(WORD) * IndexNum;


	hr = m_pD3DDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(IndexBufferSize),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&pIndexBuffer)
	);

	if (FAILED(hr)) 
	{
		__debugbreak();
		return hr = E_FAIL;
	}

	if (pInitData) 
	{
		if (FAILED(m_pCommandAllocator->Reset())) 
		{
			__debugbreak();
			return hr = E_FAIL;
		}

		if (FAILED(m_pCommandList->Reset(m_pCommandAllocator, nullptr))) 
		{
			__debugbreak();
			return hr = E_FAIL;
		}
		

		hr = m_pD3DDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(IndexBufferSize),
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&_pUploadBuffer)
		);

		if (FAILED(hr)) 
		{
			__debugbreak();
			return hr = E_FAIL;
		}
		

		UINT8* pIndexDataBegin = nullptr;
		CD3DX12_RANGE WriteRange(0, 0);
		hr = _pUploadBuffer->Map(0, &WriteRange, reinterpret_cast<void**>(&pIndexDataBegin));
		if (FAILED(hr))
		{
			__debugbreak();
			return hr = E_FAIL;
		}
		memcpy(pIndexDataBegin, pInitData, IndexBufferSize);
		_pUploadBuffer->Unmap(0, nullptr);

		m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pIndexBuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
		m_pCommandList->CopyBufferRegion(pIndexBuffer, 0, _pUploadBuffer, 0, IndexBufferSize);
		m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pIndexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER));

		m_pCommandList->Close();

		ID3D12CommandList* ppCommandLists[] = { m_pCommandList };
		m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		Fence();
		WaitForFenceValue();
	}
	IndexBufferView.BufferLocation = pIndexBuffer->GetGPUVirtualAddress();
	IndexBufferView.Format = DXGI_FORMAT_R16_UINT;
	IndexBufferView.SizeInBytes = IndexBufferSize;

	*pIndexBufferView = IndexBufferView;
	*ppBuffer = pIndexBuffer;

	if (_pUploadBuffer) 
	{
		_pUploadBuffer->Release();
		_pUploadBuffer = nullptr;
	}
	return hr;
}

void CD3D12ResourceManager::UpdateTextureForWrite(ID3D12Resource* pDestTexResource, ID3D12Resource* pSrcTexResource)
{
	const DWORD MAX_SUB_RESOURCE_NUM = 32;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT Footprint[MAX_SUB_RESOURCE_NUM] = {};
	UINT	Rows[MAX_SUB_RESOURCE_NUM] = {};
	UINT64	RowSize[MAX_SUB_RESOURCE_NUM] = {};
	UINT64	TotalBytes = 0;

	D3D12_RESOURCE_DESC Desc = pDestTexResource->GetDesc();
	if (Desc.MipLevels > (UINT)_countof(Footprint))
		__debugbreak();

	m_pD3DDevice->GetCopyableFootprints(&Desc, 0, Desc.MipLevels, 0, Footprint, Rows, RowSize, &TotalBytes);

	if (FAILED(m_pCommandAllocator->Reset()))
		__debugbreak();

	if (FAILED(m_pCommandList->Reset(m_pCommandAllocator, nullptr)))
		__debugbreak();

	m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pDestTexResource, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST));
	for (DWORD i = 0; i < Desc.MipLevels; i++)
	{

		D3D12_TEXTURE_COPY_LOCATION	destLocation = {};
		destLocation.PlacedFootprint = Footprint[i];
		destLocation.pResource = pDestTexResource;
		destLocation.SubresourceIndex = i;
		destLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

		D3D12_TEXTURE_COPY_LOCATION	srcLocation = {};
		srcLocation.PlacedFootprint = Footprint[i];
		srcLocation.pResource = pSrcTexResource;
		srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

		m_pCommandList->CopyTextureRegion(&destLocation, 0, 0, 0, &srcLocation, nullptr);
	}
	m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pDestTexResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE));
	m_pCommandList->Close();

	ID3D12CommandList* ppCommandLists[] = { m_pCommandList };
	m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	Fence();
	WaitForFenceValue();
}

bool CD3D12ResourceManager::CreateTexture(ID3D12Resource** ppResource, UINT Width, UINT Height, DXGI_FORMAT format, const BYTE* pInitImage)
{
	ID3D12Resource* pTexResource = nullptr;
	ID3D12Resource* pUploadBuffer = nullptr;

	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.MipLevels = 1;
	textureDesc.Format = format;	// ex) DXGI_FORMAT_R8G8B8A8_UNORM, etc...
	textureDesc.Width = Width;
	textureDesc.Height = Height;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	if (FAILED(m_pD3DDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&pTexResource))))
	{
		__debugbreak();
	}

	if (pInitImage)
	{
		D3D12_RESOURCE_DESC Desc = pTexResource->GetDesc();
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT Footprint;
		UINT	Rows = 0;
		UINT64	RowSize = 0;
		UINT64	TotalBytes = 0;

		m_pD3DDevice->GetCopyableFootprints(&Desc, 0, 1, 0, &Footprint, &Rows, &RowSize, &TotalBytes);

		BYTE* pMappedPtr = nullptr;
		CD3DX12_RANGE writeRange(0, 0);

		UINT64 uploadBufferSize = GetRequiredIntermediateSize(pTexResource, 0, 1);

		if (FAILED(m_pD3DDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&pUploadBuffer))))
		{
			__debugbreak();
		}

		HRESULT hr = pUploadBuffer->Map(0, &writeRange, reinterpret_cast<void**>(&pMappedPtr));
		if (FAILED(hr))
			__debugbreak();

		const BYTE* pSrc = pInitImage;
		BYTE* pDest = pMappedPtr;
		for (UINT y = 0; y < Height; y++)
		{
			memcpy(pDest, pSrc, Width * 4);
			pSrc += (Width * 4);
			pDest += Footprint.Footprint.RowPitch;
		}
		// Unmap
		pUploadBuffer->Unmap(0, nullptr);

		UpdateTextureForWrite(pTexResource, pUploadBuffer);

		pUploadBuffer->Release();
		pUploadBuffer = nullptr;

	}
	*ppResource = pTexResource;

	return true;
}

bool CD3D12ResourceManager::CreateTexturePair(ID3D12Resource** ppOutTextureResource, ID3D12Resource** ppOutUploadBuffer, UINT Width, UINT Height, DXGI_FORMAT Format)
{
	ID3D12Resource* pTextureResource = nullptr;
	ID3D12Resource* pUploadBuffer = nullptr;

	D3D12_RESOURCE_DESC TextureDesc = {};
	TextureDesc.MipLevels = 1;
	TextureDesc.Format = Format;	
	TextureDesc.Width = Width;
	TextureDesc.Height = Height;
	TextureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	TextureDesc.DepthOrArraySize = 1;
	TextureDesc.SampleDesc.Count = 1;
	TextureDesc.SampleDesc.Quality = 0;
	TextureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	HRESULT hr = m_pD3DDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&TextureDesc,
		D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&pTextureResource)
	);

	if(FAILED(hr))
	{
		__debugbreak();
	}

	UINT64 uploadBufferSize = GetRequiredIntermediateSize(pTextureResource, 0, 1);

	if (FAILED(m_pD3DDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&pUploadBuffer))))
	{
		__debugbreak();
	}

	*ppOutTextureResource = pTextureResource;
	*ppOutUploadBuffer = pUploadBuffer;

	return true;
}

bool CD3D12ResourceManager::CreateTextureFromFile(ID3D12Resource** ppOutResource, D3D12_RESOURCE_DESC* pOutResourceDesc, const WCHAR* FileName)
{

	bool bResult = false;

	ID3D12Resource* pTexResource = nullptr;
	ID3D12Resource* pUploadBuffer = nullptr;

	D3D12_RESOURCE_DESC TextureDesc = {};

	std::unique_ptr<uint8_t[]> ddsData;
	std::vector<D3D12_SUBRESOURCE_DATA> SubResourceData;

	HRESULT hr = DirectX::LoadDDSTextureFromFile(m_pD3DDevice, FileName, &pTexResource, ddsData, SubResourceData);
	if(FAILED(hr))
	{
		//__debugbreak();
		std::cerr << "Texture File Load Failure :" << ConvertWCtoC(FileName) << "\n";
		
		return bResult;
	}
	TextureDesc = pTexResource->GetDesc();

	UINT SubResourceSize = (UINT)SubResourceData.size();
	UINT64 UploadBufferSize = GetRequiredIntermediateSize(pTexResource, 0, SubResourceSize);

	hr = m_pD3DDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(UploadBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&pUploadBuffer));

	if(FAILED(hr))
	{
		__debugbreak();
		return bResult;
	}

	hr = m_pCommandAllocator->Reset();
	if(FAILED(hr))
	{
		__debugbreak();
	}

	hr = m_pCommandList->Reset(m_pCommandAllocator, nullptr);
	if (FAILED(hr))
	{
		__debugbreak();
	}


	m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pTexResource, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST));
	UpdateSubresources(m_pCommandList, pTexResource, pUploadBuffer, 0, 0, SubResourceSize, &SubResourceData[0]);
	m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pTexResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE));

	m_pCommandList->Close();

	ID3D12CommandList* ppCommandLists[] = { m_pCommandList };
	m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	Fence();
	WaitForFenceValue();

	if (pUploadBuffer)
	{
		pUploadBuffer->Release();
		pUploadBuffer = nullptr;
	}
	*ppOutResource = pTexResource;
	*pOutResourceDesc = TextureDesc;
	bResult = TRUE;

	bResult = true;
	return bResult;
}

bool CD3D12ResourceManager::CreateCommandListAndQueue()
{
	bool bResult = false;

	if (!m_pD3DDevice)
	{
		__debugbreak();
		return bResult;
	}

	D3D12_COMMAND_QUEUE_DESC CommandQueueDesc {};
	CommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	CommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	
	if (FAILED(m_pD3DDevice->CreateCommandQueue(&CommandQueueDesc, IID_PPV_ARGS(&m_pCommandQueue)))) 
	{
		__debugbreak();
		return bResult;
	}

	if (FAILED(m_pD3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_pCommandAllocator)))) 
	{
		__debugbreak();
		return bResult;
	}
	
	if (FAILED(m_pD3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCommandAllocator, nullptr, IID_PPV_ARGS(&m_pCommandList))))
	{
		__debugbreak();
		return bResult;
	}

	//The main Loop expects it to be close, So close it for now.
	m_pCommandList->Close();
	
	return bResult = true;
}

bool CD3D12ResourceManager::CreateFence()
{
	bool bResult = false;
	if (!m_pD3DDevice) 
	{
		__debugbreak();
		return bResult;
	}

	if (FAILED(m_pD3DDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence))))
	{
		__debugbreak();
		return bResult;
	}

	m_FenceValue = 0;
	m_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	return bResult = true;
}

/*Clean Start*/
void CD3D12ResourceManager::CleanUp()
{
	if(m_pD3DDevice)
	{
		m_pD3DDevice->Release();
		m_pD3DDevice = nullptr;
	}
	CleanUpCommandList();
	CleanUpFence();
}

void CD3D12ResourceManager::CleanUpCommandList()
{
	if (m_pCommandList)
	{
		m_pCommandList->Release();
		m_pCommandList = nullptr;
	}
	if (m_pCommandAllocator)
	{
		m_pCommandAllocator->Release();
		m_pCommandAllocator = nullptr;
	}

	if (m_pCommandQueue)
	{
		m_pCommandQueue->Release();
		m_pCommandQueue = nullptr;
	}
}

void CD3D12ResourceManager::CleanUpFence()
{
	if (m_FenceEvent)
	{
		CloseHandle(m_FenceEvent);
		m_FenceEvent = nullptr;
	}
	if (m_pFence)
	{
		m_pFence->Release();
		m_pFence = nullptr;
	}
}
/*Clean End*/


UINT64 CD3D12ResourceManager::Fence()
{
	m_FenceValue++;
	m_pCommandQueue->Signal(m_pFence, m_FenceValue);
	return m_FenceValue;
}

void CD3D12ResourceManager::WaitForFenceValue()
{
	const UINT64 ExpectedFenceValue = m_FenceValue;

	if (m_pFence->GetCompletedValue() < ExpectedFenceValue) 
	{
		m_pFence->SetEventOnCompletion(ExpectedFenceValue, m_FenceEvent);
		WaitForSingleObject(m_FenceEvent, INFINITE);
	}
}
