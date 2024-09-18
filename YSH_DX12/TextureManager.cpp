#include "pch.h"
#include "TextureManager.h"

#include "D3D12Renderer.h"
#include "../Util/HashTable.h"
#include "SingleDescriptorAllocator.h"
#include "D3D12ResourceManager.h"
CTextureManager::CTextureManager()
{
}

CTextureManager::~CTextureManager()
{
	Cleanup();
}

bool CTextureManager::Initialize(class CD3D12Renderer* pRenderer, class CD3D12ResourceManager* pResourceManager, UINT MaxBucketCount, UINT MaxFileCount)
{
	m_pRenderer = pRenderer;
	m_pResourceManager = pResourceManager;
	m_pHashTable = new CHashTable{};

	if (!m_pRenderer)
	{
		__debugbreak();
	}
	if (!m_pResourceManager)
	{
		__debugbreak();
	}
	if (!m_pHashTable)
	{
		__debugbreak();
	}

	m_pHashTable->Initialize(MaxBucketCount, _MAX_PATH * sizeof(WCHAR), MaxFileCount);

	return true;
}

TextureHandle* CTextureManager::CreateTextureFromFile(const WCHAR* FileName)
{
	ID3D12Device* pD3DDevice = m_pRenderer->GetD3DDevice();
	CSingleDescriptorAllocator* pSingleDescriptorAllocator = m_pRenderer->GetSingleDescriptorAllocator();

	TextureHandle* pTextureHandle = nullptr;
	ID3D12Resource* pTextureResource = nullptr;
	D3D12_RESOURCE_DESC	desc = {};
	D3D12_CPU_DESCRIPTOR_HANDLE SRVHandle = {};

	/*for Cashing*/
	DWORD dwFileNameLen = (DWORD)wcslen(FileName);
	DWORD dwKeySize = dwFileNameLen * sizeof(WCHAR);
	if (m_pHashTable->Select((void**)&pTextureHandle, 1, FileName, dwKeySize))
	{
		pTextureHandle->RefCount++;
	}
	else
	{
		if (m_pResourceManager->CreateTextureFromFile(&pTextureResource, &desc, FileName))
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
			SRVDesc.Format = desc.Format;
			SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			SRVDesc.Texture2D.MipLevels = desc.MipLevels;

			if (pSingleDescriptorAllocator->AllocateDescriptorHandle(&SRVHandle))
			{
				pD3DDevice->CreateShaderResourceView(pTextureResource, &SRVDesc, SRVHandle);

				pTextureHandle = AllocateTextureHandle();
				pTextureHandle->pTexResource = pTextureResource;
				pTextureHandle->SRVHandle = SRVHandle;
				pTextureHandle->bFromFile = true;

				pTextureHandle->pHashSearchHandle = m_pHashTable->Insert((void*)pTextureHandle, FileName, dwKeySize);
				if (!pTextureHandle->pHashSearchHandle)
				{
					__debugbreak();
				}
			}
			else
			{
				pTextureResource->Release();
				pTextureResource = nullptr;
			}
		}
	}
	

	return pTextureHandle;
}

TextureHandle* CTextureManager::CreateDynamicTexture(UINT TextureWidth, UINT TextureHeight)
{
	ID3D12Device* pD3DDevice = m_pRenderer->GetD3DDevice();
	CSingleDescriptorAllocator* pSingleDescriptorAllocator = m_pRenderer->GetSingleDescriptorAllocator();

	TextureHandle* pTextureHandle = nullptr;

	ID3D12Resource* pTexResource = nullptr;
	ID3D12Resource* pUploadBuffer = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE srv = {};
	if (m_pResourceManager->CreateTexturePair(&pTexResource, &pUploadBuffer, TextureWidth, TextureHeight, DXGI_FORMAT_R8G8B8A8_UNORM))
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
		SRVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MipLevels = 1;

		if (pSingleDescriptorAllocator->AllocateDescriptorHandle(&srv))
		{
			pD3DDevice->CreateShaderResourceView(pTexResource, &SRVDesc, srv);

			pTextureHandle = AllocateTextureHandle();
			pTextureHandle->pTexResource = pTexResource;
			pTextureHandle->pUploadBuffer = pUploadBuffer;
			pTextureHandle->SRVHandle = srv;
		}
		else
		{
			pTexResource->Release();
			pTexResource = nullptr;

			pUploadBuffer->Release();
			pUploadBuffer = nullptr;
		}
	}

	return pTextureHandle;
}

TextureHandle* CTextureManager::CreateImmutableTexture(UINT TexWidth, UINT TexHeight, DXGI_FORMAT TextureFormat, const BYTE* pInitImage)
{
	TextureHandle* pTextureHandle = nullptr;
	ID3D12Resource* pTextureResource = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE SRVHandle = {};
	ID3D12Device* pD3DDevice = m_pRenderer->GetD3DDevice();
	CSingleDescriptorAllocator* pSingleDescriptorAllocator = m_pRenderer->GetSingleDescriptorAllocator();


	if (m_pResourceManager && m_pResourceManager->CreateTexture(&pTextureResource, TexWidth, TexHeight, TextureFormat, pInitImage))
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
		SRVDesc.Format = TextureFormat;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MipLevels = 1;

		if (pSingleDescriptorAllocator->AllocateDescriptorHandle(&SRVHandle))
		{
			pD3DDevice->CreateShaderResourceView(pTextureResource, &SRVDesc, SRVHandle);
			pTextureHandle =AllocateTextureHandle();
			pTextureHandle->pTexResource = pTextureResource;
			pTextureHandle->SRVHandle = SRVHandle;
		}
		else
		{
			pTextureResource->Release();
			pTextureResource = nullptr;
			__debugbreak();
		}
	}
	else
	{
		pTextureResource->Release();
		pTextureResource = nullptr;
		__debugbreak();
	}
	return pTextureHandle;
}

void CTextureManager::DeleteTexture(TextureHandle* pTextureHandle)
{
	DeallocateTextureHandle(pTextureHandle);
}

TextureHandle* CTextureManager::AllocateTextureHandle()
{
	TextureHandle* pTexHandle =  new TextureHandle{};
	if(pTexHandle)
	{
		m_TextureHandles.insert(pTexHandle);
		pTexHandle->RefCount = 1;
	}
	return pTexHandle;
	//TODO 
}

UINT CTextureManager::DeallocateTextureHandle(TextureHandle* pTextureHandle)
{
	if(pTextureHandle)
	{
		if(pTextureHandle->RefCount == 0 )
		{
			/*중복 해제!!*/
			__debugbreak();
		}

		UINT ref_count = --pTextureHandle->RefCount;
		if(ref_count == 0 )
		{
			if (pTextureHandle->pUploadBuffer)
			{
				pTextureHandle->pUploadBuffer->Release();
				pTextureHandle->pUploadBuffer = nullptr;
			}

			if (pTextureHandle->pTexResource)
			{
				pTextureHandle->pTexResource->Release();
				pTextureHandle->pTexResource = nullptr;
			}

			CSingleDescriptorAllocator* pSingleDescirptorAllocator = m_pRenderer->GetSingleDescriptorAllocator();
			pSingleDescirptorAllocator->FreeDescriptorHandle(pTextureHandle->SRVHandle);

			
			if(pTextureHandle->pHashSearchHandle)
			{
				assert(pTextureHandle->bFromFile);

				m_pHashTable->Delete(pTextureHandle->pHashSearchHandle);
				pTextureHandle->pHashSearchHandle = nullptr;
			}

			m_TextureHandles.erase(pTextureHandle);

		
			delete pTextureHandle;
		}

		return ref_count; /*현재 몇군데 남아있는지*/
	}
	return 0;
	//TODO
}

void CTextureManager::Cleanup()
{
	if(m_TextureHandles.empty() == false)
	{
		__debugbreak();
	}


	if (m_pHashTable)
	{
		delete m_pHashTable;
		m_pHashTable = nullptr;
	}
}
