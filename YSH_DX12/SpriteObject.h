#pragma once

/*git hub test*/

class CSpriteObject
{
public:/*function*/
	CSpriteObject();
	~CSpriteObject();

	bool Initialize(class CD3D12Renderer* p_renderer);
	bool Initialize(class CD3D12Renderer* p_renderer , const WCHAR* TextureFileName , const RECT* pRect);

	bool IsInitialized() const { return bInitialized; }

	void Draw(ID3D12GraphicsCommandList* pCommandList, const XMFLOAT2* pPos, const XMFLOAT2* pScale, const RECT* pRect, float Z, TextureHandle* pTexHandle);
	void Draw(ID3D12GraphicsCommandList* pCommandList, const XMFLOAT2* pPos, const XMFLOAT2* pScale, float Z);

private:/*function*/

	bool InitializeCommonResources();

	bool InitRootSignature();
	bool InitPipelineState();
	bool InitMesh();

	void Cleanup();
	void ClenaupSharedResource();
public:/*variable*/

	enum SPRITE_DESCRIPTOR_INDEX
	{
		SPRITE_DESCRIPTOR_INDEX_CBV_0 = 0,
		SPRITE_DESCRIPTOR_INDEX_SRV_0 = 1,
		SPRITE_DESCRIPTOR_COUNT=2
	};

	enum{ DESCRIPTOR_COUNT_FOR_DRAW  = 2};
private: /*variable*/

	/*STATIC*/
	static ID3D12RootSignature* m_pRootSignature;
	static ID3D12PipelineState* m_pPipelineState;
	static UINT m_InitRefCount;

	static ID3D12Resource* m_pVertexBuffer;
	static D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;

	static ID3D12Resource* m_pIndexBuffer;
	static D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;
	/*STATIC END*/

	class TextureHandle* m_pTextureHandle = nullptr;
	class CD3D12Renderer* m_pRenderer = nullptr;

	bool bInitialized = false;

	RECT	m_TextureRect = {};
	XMFLOAT2	m_Scale = { 1.0f, 1.0f };

};

