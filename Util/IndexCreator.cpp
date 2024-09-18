#include "pch.h"
#include "IndexCreator.h"

CIndexCreator::CIndexCreator()
{

}
CIndexCreator::~CIndexCreator()
{
	Check();
	Cleanup();
}
bool CIndexCreator::Initialize(INT32 Count)
{
	m_MaxCount = Count;
	m_pIndexTable = new INT32[m_MaxCount];
	memset(m_pIndexTable, 0, sizeof(INT32) * m_MaxCount);
	
	for(INT i = 0 ; i < m_MaxCount ; ++ i )
	{
		m_pIndexTable[i] = i;
	}

	return true;
}
INT32 CIndexCreator::Allocate()
{
	INT32		Result = INDEX_NONE;

	if (m_AllocatedCount >= m_MaxCount)
	{
		return Result;
	}

	Result = m_pIndexTable[m_AllocatedCount];
	m_AllocatedCount++;

	return Result;
}
void CIndexCreator::Free(INT32 Index)
{
	if(m_AllocatedCount == 0)
	{
		__debugbreak();
	}

	m_AllocatedCount--;
	m_pIndexTable[m_AllocatedCount] = Index;
}


void CIndexCreator::Check()
{
	if (m_AllocatedCount)
	{
		__debugbreak();
	}
}


void CIndexCreator::Cleanup()
{
	if(m_pIndexTable)
	{
		delete[] m_pIndexTable;
		m_pIndexTable = nullptr;
	}	
}

