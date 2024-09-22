#include "pch.h"
#include "ConstantBufferManager.h"
#include "ConstantBufferPool.h"

CONSTANT_BUFFER_PROPERTY g_pConstBufferPropList[] =
{
	EConstantBufferType::MeshObject, sizeof(CONSTANT_BUFFER_DEFAULT),
	EConstantBufferType::SpriteObject, sizeof(CONSTANT_BUFFER_SPRITE)
};

CConstantBufferManager::CConstantBufferManager()
{
}

CConstantBufferManager::~CConstantBufferManager()
{
	for (DWORD i = 0; i < static_cast<DWORD>(EConstantBufferType::TypeCount); ++i)
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
	for (DWORD i = 0; i < static_cast<DWORD>(EConstantBufferType::TypeCount); i++)
	{
		m_ppConstantBufferPool[i] = new CConstantBufferPool;
		m_ppConstantBufferPool[i]->Initialize(pD3DDevice, (EConstantBufferType)i, AlignConstantBufferSize(g_pConstBufferPropList[i].Size), MaxCBVCount);
	}
	return true;
}

void CConstantBufferManager::Reset()
{
	for (DWORD i = 0; i < static_cast<DWORD>(EConstantBufferType::TypeCount); i++)
	{
		m_ppConstantBufferPool[i]->Reset();
	}
}

class CConstantBufferPool* CConstantBufferManager::GetConstantBufferPool(EConstantBufferType Type)
{
	if (Type >= EConstantBufferType::TypeCount)
	{
		__debugbreak();
	}

	return m_ppConstantBufferPool[static_cast<UINT>(Type)];
}
