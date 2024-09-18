#pragma once

#include "typedef.h"

class CConstantBufferManager
{
public:
	CConstantBufferManager();
	~CConstantBufferManager();

	bool Initialize(ID3D12Device* pD3DDevice, UINT MaxCBVCount);
	void Reset();

	class CConstantBufferPool* GetConstantBufferPool(CONSTANT_BUFFER_TYPE Type);
private:
	class CConstantBufferPool* m_ppConstantBufferPool[CONSTANT_BUFFER_TYPE_COUNT] = {};
};

