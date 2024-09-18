#pragma once

/*
 *	Text  ->   m_pD2DTargetBitmap  : Render
 *	m_pD2DTargetBitmap -> m_pD2DTargetBitmapRead   : Copy
 *	m_pD2DTargetBitmapRead -> BYTE   map 복사 
 *
 *	BYTE 가져가서 그걸 Upload buffer 에 쓰고
 *
 */

class CFontManager
{
public:/*function*/

	CFontManager();
	~CFontManager();

	bool Initialize(class CD3D12Renderer* pRenderer, ID3D12CommandQueue* pCommandQueue, UINT Width, UINT Height, bool bEnableDebugLayer);

	struct FontHandle* CreateFontObject(const WCHAR* FontFamilyName, float FontSize);
	void DeleteFontObject(struct FontHandle* pFontHandle);

	bool WriteTextToBitmap(BYTE* pDestImage, UINT DestWidth, UINT DestHeight, UINT DestPitch, INT* pOutWidth, INT* pOutHeight, FontHandle* pFontHandle, const WCHAR* String, UINT32 Length);

private:

	bool InitializeD2D(ID3D12Device* pD3DDevice , ID3D12CommandQueue* pCommandQueue , bool bEnableDebugLayer);
	bool InitializeDWrite(ID3D12Device* pD3DDevice , UINT TextureWidth , UINT TextureHeight , float DPI );


	bool RenderTextToTargetBitmap(INT* pOutWidth, INT* pOutHeight, IDWriteTextFormat* pTextFormat, const WCHAR* String, UINT Len);


	void Cleanup();
	void CleanupDWrite();
	void CleanupD2D();




private:/*variable*/

	ID2D1Device* m_pD2DDevice = nullptr;
	ID2D1DeviceContext* m_pD2DDeviceContext = nullptr;

	ID2D1Bitmap1* m_pD2DTargetBitmap = nullptr;
	ID2D1Bitmap1* m_pD2DTargetBitmapRead = nullptr;
	IDWriteFontCollection* m_pFontCollection = nullptr;
	ID2D1SolidColorBrush* m_pWhiteColorBrush = nullptr;

	IDWriteFactory5* m_pDWFactory = nullptr;

	UINT m_D2DBitmapWidth = 0;
	UINT m_D2DBitmapHeight = 0;


};

