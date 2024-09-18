#pragma once

class CIndexCreator
{

public:
	CIndexCreator();

	~CIndexCreator();

	bool Initialize(INT32 Count);

	
	INT32 Allocate();
	void Free(INT32 Index);
	void Cleanup();
	void Check();

	enum{INDEX_NONE = -1};
private:
	INT32* m_pIndexTable = nullptr;
	INT32 m_MaxCount = 0;
	INT32 m_AllocatedCount = 0;
};

