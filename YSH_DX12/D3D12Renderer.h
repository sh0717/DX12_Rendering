#pragma once

/*
	coding standards 
l
	Type is Pascal
*/

constexpr UINT32 SWAP_CHAIN_FRAME_COUNT =4;
constexpr UINT32 MAX_PENDING_FRAME_COUNT = SWAP_CHAIN_FRAME_COUNT - 1;
class CD3D12Renderer
{
public:/*function*/
	CD3D12Renderer();
	~CD3D12Renderer();

	bool	Initialize(HWND hWnd, BOOL bEnableDebugLayer, BOOL bEnableGBV);
	void	BeginRender();
	void	EndRender();
	void	Present();
	BOOL	UpdateWindowSize(DWORD dwBackBufferWidth, DWORD dwBackBufferHeight);

	/*Basic Mesh Object*/

	void* CreateBasicMeshObject();
	void  DeleteBasicMeshObject(void* pMeshObjHandle);

	bool InsertVertexDataToMeshObject(void* pMeshObjHandle, const BasicVertex* pVertexList, UINT VertexCount, UINT TriGroupCount);
	bool InsertTriGroupDataToMeshObject(void* pMeshObjHandle, const WORD* pIndexList, UINT TriangleCount, const WCHAR* TextureFileName);
	void EndCreateMesh(void* pMeshObjHandle);

	void  RenderMeshObject(void* pMeshObjHandle , XMMATRIX* pWorldMatrix );


	/*Sprite Object*/

	void* CreateSpriteObject();
	void* CreateSpriteObject(const WCHAR* TextureFileName , int PosX , int PosY , int Width , int Height);
	void  DeleteSpriteObject(void* pSpriteObjHandle);

	void RenderSpriteObject(void* pSpriteObjectHandle, INT PosX, INT PosY, float ScaleX, float ScaleY, float Z);
	void RenderSpriteObject(void* pSprObjHandle, int iPosX, int iPosY, float fScaleX, float fScaleY, const RECT* pRect, float Z, void* pTexHandle);

	/*Font Object Handle*/
	void* CreateFontObject(const WCHAR* FontFamilyName, float FontSize);
	void DeleteFontObject(void* pFontHandle);

	bool WriteTextToBitmap(BYTE* pDestImage, UINT DestWidth, UINT DestHeight, UINT DestPitch, int* piOutWidth, int* piOutHeight, void* pFontObjHandle, const WCHAR* wchString, DWORD dwLen);


	UINT Get_CBV_SRV_UAV_DescriptorSize() const { return m_CBV_SRV_UBV_DescriptorSize; };

	/*Texture Related Function*/
	/*Texture is handled by class TextureHandle*/
	void* CreateTiledTexture(UINT TexWidth, UINT TexHeight, UINT32 Red, UINT32 Green, UINT32 Blue);
	void* CreateTextureFromFile(const WCHAR* wchFileName) const;
	void* CreateDynamicTexture(UINT TextureWidth , UINT TextureHeight);
	void DeleteTexture(void* pTexHandle);

	void UpdateTextureWithImage(void* pTextureHandle, const BYTE* pSourceData, UINT SourceWidth, UINT SourceHeight);
	struct TextureHandle* GetDefaultTexture() const { return m_DefaultTextureHandle; }

	
	ID3D12Device5* GetD3DDevice() const { return m_pD3DDevice; }
	class CD3D12ResourceManager* GetResourceManager() const { return m_pResourceManager.get(); };
	class CSingleDescriptorAllocator* GetSingleDescriptorAllocator() const{ return m_pSingleDescriptorAllocator.get(); }
	class CDescriptorPool* GetDescriptorPool() const { return m_ppDescriptorPools[m_CurContextIndex]; }
	class CConstantBufferPool* GetConstantBufferPool(EConstantBufferType Type) const;


	void GetViewProjMatrix(XMMATRIX* pOutViewMatrix, XMMATRIX* pOutProjMatrix);

	UINT	GetScreenWidth() const { return m_Width; }
	UINT	GetScreenHeigt() const { return m_Height; }
	float	GetDPI() const { return m_DPI; }

	/*Camera*/
	void MoveCamera(float x, float y, float z);
	void SetCamera(const XMVECTOR* pCameraPos, const XMVECTOR* pCameraDir, const XMVECTOR* pCameraUp);


private:/*function*/
	void PostInitialize();
	/*this function Create Descriptor used for D3D12  RTV and DSV*/
	bool	CreateDescriptorHeap();
	void	CreateCommandListAndQueue();
	void	CreateFence();

	bool CreateRenderTarget(UINT Width, UINT Height, IDXGIFactory4* pFactory , HWND hwnd);
	bool CreateDepthStencil(UINT Width, UINT Height);
	UINT64	Fence();
	void	WaitForFenceValue(UINT64 ExpectedFenceValue);

	void	Cleanup();
	void	CleanupFence();
	void	CleanupCommandListAndQueue();
	void	CleanupResource();
	void	CleanupDescriptorHeap();	

	void InitCamera();


private: /*variable*/

	/*Static Variable*/
	static constexpr  UINT MAX_DRAW_COUNT_PER_FRAME = 1024;
	static constexpr UINT MAX_DESCRIPTOR_COUNT = 4096;



	std::unique_ptr<class CD3D12ResourceManager> m_pResourceManager;
	std::unique_ptr<class CSingleDescriptorAllocator> m_pSingleDescriptorAllocator;
	class CDescriptorPool* m_ppDescriptorPools[MAX_PENDING_FRAME_COUNT] = {};
	class CConstantBufferManager* m_ppConstantBufferManager[MAX_PENDING_FRAME_COUNT] = {};
	class CRenderQueue* m_pRenderQueue = nullptr;
	//class CConstantBufferPool* m_ppConstantBufferPools[MAX_PENDING_FRAME_COUNT] = {};

	//std::unique_ptr<class CDescriptorPool[]> TestDescriptorPools;
	//std::unique_ptr<class CConstantBufferPool[]> m_pConstantBufferPools;

	std::unique_ptr<class CFontManager> m_pFontManager;
	class CTextureManager* m_pTextureManager;
	

	HWND	m_HWND = nullptr;
	ID3D12Device5*	m_pD3DDevice = nullptr;
	IDXGISwapChain3* m_pSwapChain = nullptr;

	ID3D12CommandQueue*	m_pCommandQueue = nullptr;
	ID3D12CommandAllocator* m_ppCommandAllocators[MAX_PENDING_FRAME_COUNT] = {};
	ID3D12GraphicsCommandList* m_ppCommandLists[MAX_PENDING_FRAME_COUNT] = {};

	UINT64 m_FenceValue = 0;
	UINT64 m_LastFenceValues[MAX_DESCRIPTOR_COUNT] = {};

	D3D_FEATURE_LEVEL	m_FeatureLevel = D3D_FEATURE_LEVEL_11_0;
	DXGI_ADAPTER_DESC1	m_AdapterDesc = {};
	
	ID3D12Resource*	  m_pRenderTargets[SWAP_CHAIN_FRAME_COUNT] = {};
	ID3D12Resource*   m_pDepthStencil= nullptr;

	/*Descriptor Having Heap*/
	ID3D12DescriptorHeap*	m_pRTVHeap = nullptr;
	ID3D12DescriptorHeap*	m_pDSVHeap = nullptr;
	ID3D12DescriptorHeap*	m_pSRVHeap = nullptr;

	/*Descriptor size depends on GPU*/
	UINT	m_RTVDescriptorSize = 0;
	UINT	m_DSVDescriptorSize = 0;
	UINT	m_SwapChainFlags = 0;
	UINT	m_CurrentRenderTargetIndex = 0;
	HANDLE	m_hFenceEvent = nullptr;
	ID3D12Fence* m_pFence = nullptr;

	//Currently Resource  using Index
	DWORD	m_CurContextIndex = 0;

	D3D12_VIEWPORT	m_Viewport = {};
	D3D12_RECT	m_ScissorRect = {};
	DWORD	m_Width = 0;
	DWORD	m_Height = 0;

	struct TextureHandle* m_DefaultTextureHandle = nullptr;

	UINT m_CBV_SRV_UBV_DescriptorSize = 0;

	/*Camera Variable*/
	XMMATRIX m_matView = {};
	XMMATRIX m_matProj = {};

	XMVECTOR m_CameraPos = {};
	XMVECTOR m_CameraDir = {};


	float m_DPI = 96.f;
};

