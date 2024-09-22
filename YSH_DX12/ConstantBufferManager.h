#pragma once

#include "typedef.h"

class CConstantBufferManager
{
public:
	CConstantBufferManager();
	~CConstantBufferManager();

	bool Initialize(ID3D12Device* pD3DDevice, UINT MaxCBVCount);
	void Reset();

	class CConstantBufferPool* GetConstantBufferPool(EConstantBufferType Type);
private:
	class CConstantBufferPool* m_ppConstantBufferPool[static_cast<UINT>(EConstantBufferType::TypeCount)] = {};
};

