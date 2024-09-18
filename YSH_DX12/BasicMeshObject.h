#pragma once


/*			Note
	================================
	CVB, SRV ,UAV descriptor size는 동일
	
	RTV DSV?

*/


struct IndexedTriGroup
{
public:
	ID3D12Resource* pIndexBuffer = nullptr;
	D3D12_INDEX_BUFFER_VIEW IndexBufferView = {};
	UINT TriangleCount = 0;
	TextureHandle* pTextureHandle = nullptr;
};


/*
 *	Index Data is stored in Indexed TriGroup Data
 *
 *
 */


class CBasicMeshObject
{
public:/*function*/
	CBasicMeshObject();
	~CBasicMeshObject();
	
	bool Initialize(class CD3D12Renderer* pRenderer);

	
	void Draw(ID3D12GraphicsCommandList* pCommandList,  XMMATRIX* pWorldMatrix );

	//Input Vertex Data and Make Vertex buffer and Vertex Buffer View 
	bool InsertVertexData(const BasicVertex* pVertexList, UINT VertexCount, UINT TrigroupCount);
	bool InsertIndexedTriangleData(const WORD* pIndexList , UINT TriangleCount , const WCHAR* TextureFileName);
	void EndCreateMesh();

	inline bool IsInitialized() const { return bInitialized; }
private:/*function*/
	bool bInitialized = false;

	bool	InitCommonResources();
	bool	InitRootSinagture();
	bool	InitPipelineState();
	

	void	CleanupSharedResources();
	void	Cleanup();

public:/*variable*/
	enum BASIC_MESH_DESCRIPTOR_INDEX_PER_OBJ
	{
		BASIC_MESH_DESCRIPTOR_INDEX_PER_OBJ_CBV_0 = 0,
		BASIC_MESH_DESCRIPTOR_COUNT_PER_OBJ
	};

	enum BASIC_MESH_DESCRIPTOR_INDEX_PER_TRIGROUP
	{
		BASIC_MESH_DESRIPTOR_INDEX_PER_TRI_SRV_0 = 0,
		BASIC_MESH_DESCRIPTOR_COUNT_PER_TRI_GROUP
	};

	//static constexpr UINT BASIC_MESH_DESCRIPTOR_COUNT_PER_OBJ = 1;			// | Constant Buffer
	//static constexpr UINT BASIC_MESH_DESCRIPTOR_COUNT_PER_TRI_GROUP = 1;	// | SRV(tex)
	static constexpr UINT MAX_TRI_GROUP_COUNT_PER_OBJ = 8;
	static constexpr UINT MAX_DESCRIPTOR_COUNT_FOR_DRAW = BASIC_MESH_DESCRIPTOR_COUNT_PER_OBJ + (MAX_TRI_GROUP_COUNT_PER_OBJ * BASIC_MESH_DESCRIPTOR_COUNT_PER_TRI_GROUP);

private:/*variable*/

	/*STATIC*/
	static ID3D12RootSignature* m_pRootSignature;
	
	static ID3D12PipelineState* m_pPipelineState;
	
	static int m_InitRefCount;

	/*STATIC END*/

	class CD3D12Renderer* m_pRenderer;

	UINT	m_CBV_SRV_UAVDescriptorSize = 0; //일단 내 꺼에서는 32byte 하지만 GPU 마다 다를 수도 있어 
	

	ID3D12Resource* m_pVertexBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW m_VerTexBufferView{};

	IndexedTriGroup* m_pTriGroupList = nullptr;
	UINT m_TriGroupCount = 0;
	UINT m_TriGroupMaxCount = 0;


	
};

