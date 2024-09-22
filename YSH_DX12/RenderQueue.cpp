#include "pch.h"
#include "RenderQueue.h"

#include "BasicMeshObject.h"
#include "D3D12Renderer.h"
#include "SpriteObject.h"

CRenderQueue::CRenderQueue()
{
}

CRenderQueue::~CRenderQueue()
{
	Cleanup();
}

bool CRenderQueue::Initialize(class CD3D12Renderer* pRenderer, UINT MaxItemCount)
{
	bool bResult = false;

	m_pRenderer = pRenderer;
	mMaxBufferSize = sizeof(RenderItem) * MaxItemCount;
	m_pBuffer = (char*)malloc(mMaxBufferSize);
	memset(m_pBuffer, 0, mMaxBufferSize);

	bResult = true;
	return bResult;
}

bool CRenderQueue::Add(const RenderItem* pItem)
{
	bool bResult = false;
	if(mAllocatedSize + sizeof(RenderItem) > mMaxBufferSize)
	{
		return bResult;
	}

	char* pDest = m_pBuffer + mAllocatedSize;
	memcpy(pDest, pItem, sizeof(RenderItem));
	mAllocatedSize += sizeof(RenderItem);

	bResult = true;
	return bResult;
}


const RenderItem* CRenderQueue::Dispatch()
{
	const RenderItem* pItem = nullptr;
	if(mReadBufferPos + sizeof(RenderItem) > mAllocatedSize)
	{
		return pItem;
	}

	pItem = (const RenderItem*)(m_pBuffer + mReadBufferPos);
	mReadBufferPos += sizeof(RenderItem);

	return pItem;
}


UINT CRenderQueue::Process(ID3D12GraphicsCommandList* pCommandList)
{
	ID3D12Device5* pD3DDevice = m_pRenderer->GetD3DDevice();
	UINT ItemCount = 0;
	const RenderItem* pItem = nullptr;

	while(pItem = Dispatch())
	{
		switch(pItem->Type)
		{
			case ERenderItemType::MeshObject:
				{
					if(CBasicMeshObject* pMeshObj = (CBasicMeshObject*)pItem->pObjectHandle)
					{
						pMeshObj->Draw(pCommandList, &pItem->MeshObjectParam.WorldMatrix);
					}
				}
				break;

			case ERenderItemType::SpriteObject:
			{
				CSpriteObject* pSpriteObj = (CSpriteObject*)pItem->pObjectHandle;
				TextureHandle* pTexureHandle = (TextureHandle*)pItem->SpriteObjectParam.pTexHandle;
				float Z = pItem->SpriteObjectParam.Z;
				if (pSpriteObj)
				{
					if (pTexureHandle)
					{
						XMFLOAT2 Pos = { (float)pItem->SpriteObjectParam.iPosX, (float)pItem->SpriteObjectParam.iPosY };
						XMFLOAT2 Scale = { pItem->SpriteObjectParam.fScaleX, pItem->SpriteObjectParam.fScaleY };

						const RECT* pRect = nullptr;
						if (pItem->SpriteObjectParam.bUseRect)
						{
							pRect = &pItem->SpriteObjectParam.Rect;
						}

						if (pTexureHandle->pUploadBuffer)
						{
							if (pTexureHandle->bUpdated)
							{
								D3DUtil::UpdateTexture(pD3DDevice, pCommandList, pTexureHandle->pTexResource, pTexureHandle->pUploadBuffer);
							}
							else
							{
								int a = 0;
							}
							pTexureHandle->bUpdated = FALSE;
						}
						pSpriteObj->Draw(pCommandList, &Pos, &Scale, pRect, Z, pTexureHandle);
					}
					else
					{
						XMFLOAT2 Pos = { (float)pItem->SpriteObjectParam.iPosX, (float)pItem->SpriteObjectParam.iPosY };
						XMFLOAT2 Scale = { pItem->SpriteObjectParam.fScaleX, pItem->SpriteObjectParam.fScaleY };
						pSpriteObj->Draw(pCommandList, &Pos, &Scale, Z);
					}
				}
			}
				break;

			default:
				{
					__debugbreak();
				}
		}
		ItemCount++;
	}

	return ItemCount;
}


void CRenderQueue::Reset()
{
	mAllocatedSize = 0;
	mReadBufferPos = 0;
}

void CRenderQueue::Cleanup()
{
	if(m_pBuffer)
	{
		free(m_pBuffer);
		m_pBuffer = nullptr;
	}
}
