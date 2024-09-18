#include "pch.h"
#include "ConstantBufferManager.h"
#include "ConstantBufferPool.h"

CONSTANT_BUFFER_PROPERTY g_pConstBufferPropList[] =
{
	CONSTANT_BUFFER_TYPE_DEFAULT, sizeof(CONSTANT_BUFFER_DEFAULT),
	CONSTANT_BUFFER_TYPE_SPRITE, sizeof(CONSTANT_BUFFER_SPRITE)
};

CConstantBufferManager::CConstantBufferManager()
{
}

CConstantBufferManager::~CConstantBufferManager()
{
	for (DWORD i = 0; i < CONSTANT_BUFFER_TYPE_COUNT; ++i)
	{
		if (m_ppConstantBufferPool[i])
		{
			delete m_ppConstantBufferPool[i];
			m_ppConstantBufferPool[i] = nullptr;
		}
	}
}

bool CConstantBufferManager::Initialize(ID3D12Device* pD3DDevice, UINT MaxCBVCount)
{
	for (DWORD i = 0; i < CONSTANT_BUFFER_TYPE_COUNT; i++)
	{
		m_ppConstantBufferPool[i] = new CConstantBufferPool;
		m_ppConstantBufferPool[i]->Initialize(pD3DDevice, (CONSTANT_BUFFER_TYPE)i, AlignConstantBufferSize(g_pConstBufferPropList[i].Size), MaxCBVCount);
	}
	return true;
}

void CConstantBufferManager::Reset()
{
	for (DWORD i = 0; i < CONSTANT_BUFFER_TYPE_COUNT; i++)
	{
		m_ppConstantBufferPool[i]->Reset();
	}
}

class CConstantBufferPool* CConstantBufferManager::GetConstantBufferPool(CONSTANT_BUFFER_TYPE Type)
{
	if (Type >= CONSTANT_BUFFER_TYPE_COUNT)
	{
		__debugbreak();
	}

	return m_ppConstantBufferPool[Type];
}
