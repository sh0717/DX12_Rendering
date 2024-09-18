#include "pch.h"
#include "FontManager.h"
#include "D3D12Renderer.h"

using namespace  D2D1;
CFontManager::CFontManager()
{
}

CFontManager::~CFontManager()
{
	Cleanup();
}

bool CFontManager::Initialize( CD3D12Renderer* pRenderer, ID3D12CommandQueue* pCommandQueue, UINT Width, UINT Height, bool bEnableDebugLayer)
{
	ID3D12Device* pD3DDevice = pRenderer->GetD3DDevice();
	float DPI = pRenderer->GetDPI();

	InitializeD2D(pD3DDevice, pCommandQueue, bEnableDebugLayer);
	InitializeDWrite(pD3DDevice, Width, Height, DPI);
	return true;
}

struct FontHandle* CFontManager::CreateFontObject(const WCHAR* FontFamilyName, float FontSize)
{
	if(!m_pDWFactory)
	{
		__debugbreak();
	}

	IDWriteTextFormat* pTextFormat = nullptr;
	IDWriteFactory5* pDWFactory = m_pDWFactory;
	IDWriteFontCollection1* pFontCollection = nullptr;

	// The logical size of the font in DIP("device-independent pixel") units.A DIP equals 1 / 96 inch.

	if (pDWFactory)
	{
		if (FAILED(pDWFactory->CreateTextFormat(
			FontFamilyName,
			pFontCollection,                       // Font collection (nullptr sets it to use the system font collection).
			DWRITE_FONT_WEIGHT_REGULAR,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			FontSize,
			DEFULAT_LOCALE_NAME,
			&pTextFormat
		)))
		{
			__debugbreak();
		}
	}
	FontHandle* pFontHandle = new FontHandle{};
	memset(pFontHandle, 0, sizeof(FontHandle));
	wcscpy_s(pFontHandle->FontFamilyName, FontFamilyName);
	pFontHandle->FontSize = FontSize;

	if (pTextFormat)
	{
		if (FAILED(pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING)))
			__debugbreak();


		if (FAILED(pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR)))
			__debugbreak();
	}

	pFontHandle->pTextFormat = pTextFormat;

	return pFontHandle;
}


void CFontManager::DeleteFontObject(struct FontHandle* pFontHandle)
{
	if(pFontHandle)
	{
		if(pFontHandle->pTextFormat)
		{
			pFontHandle->pTextFormat->Release();
			pFontHandle->pTextFormat = nullptr;
		}

		delete pFontHandle;
	}
}

bool CFontManager::WriteTextToBitmap(BYTE* pDestImage, UINT DestWidth, UINT DestHeight, UINT DestPitch, INT* pOutWidth, INT* pOutHeight, FontHandle* pFontHandle, const WCHAR* String, UINT32 Length)
{
	INT TextWidth = 0;
	INT TextHeight = 0;
	if(!pFontHandle)
	{
		return false;
	}

	bool bResult = false;
	if( bResult = RenderTextToTargetBitmap(&TextWidth, &TextHeight, pFontHandle->pTextFormat, String, Length))
	{
		if (TextWidth > (int)DestWidth)
			TextWidth = (int)DestWidth;

		if (TextHeight > (int)DestHeight)
			TextHeight = (int)DestHeight;

		D2D1_MAPPED_RECT	mappedRect;
		if (FAILED(m_pD2DTargetBitmapRead->Map(D2D1_MAP_OPTIONS_READ, &mappedRect)))
			__debugbreak();

		BYTE* pDest = pDestImage;
		char* pSrc = (char*)mappedRect.bits;

		for (DWORD y = 0; y < (DWORD)TextHeight; y++)
		{
			memcpy(pDest, pSrc, TextWidth * 4);
			pDest += DestPitch;
			pSrc += mappedRect.pitch;
		}
		m_pD2DTargetBitmapRead->Unmap();
	}

	*pOutWidth = TextWidth;
	*pOutHeight = TextHeight;

	return bResult;
}

bool CFontManager::InitializeD2D(ID3D12Device* pD3DDevice, ID3D12CommandQueue* pCommandQueue, bool bEnableDebugLayer)
{
	UINT D3D11DeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
	D2D1_FACTORY_OPTIONS D2DFactoryOptions = {};

	ID2D1Factory3* pD2DFactory = nullptr;
	ID3D11Device* pD3D11Device = nullptr;
	ID3D11DeviceContext* pD3D11DeviceContext = nullptr;
	ID3D11On12Device* pD3D11On12Device = nullptr;

	if (bEnableDebugLayer)
	{
		D3D11DeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	}
	D2DFactoryOptions.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;

	if (FAILED(D3D11On12CreateDevice(pD3DDevice,
		D3D11DeviceFlags,
		nullptr,
		0,
		(IUnknown**)&pCommandQueue,
		1,
		0,
		&pD3D11Device,
		&pD3D11DeviceContext,
		nullptr
	)))
	{
		__debugbreak();
	}


	if(FAILED(pD3D11Device->QueryInterface(IID_PPV_ARGS(&pD3D11On12Device))))
	{
		__debugbreak();
	}

	// Create D2D/DWrite components.
	D2D1_DEVICE_CONTEXT_OPTIONS deviceOptions = D2D1_DEVICE_CONTEXT_OPTIONS_NONE;
	if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory3), &D2DFactoryOptions, (void**)&pD2DFactory)))
	{
		__debugbreak();
	}

	IDXGIDevice* pDXGIDevice = nullptr;
	if (FAILED(pD3D11On12Device->QueryInterface(IID_PPV_ARGS(&pDXGIDevice))))
	{
		__debugbreak();
	}
	if (FAILED(pD2DFactory->CreateDevice(pDXGIDevice, &m_pD2DDevice)))
	{
		__debugbreak();
	}


	if (FAILED(m_pD2DDevice->CreateDeviceContext(deviceOptions, &m_pD2DDeviceContext)))
	{
		__debugbreak();
	}

	if (pD3D11Device)
	{
		pD3D11Device->Release();
		pD3D11Device = nullptr;
	}
	if (pDXGIDevice)
	{
		pDXGIDevice->Release();
		pDXGIDevice = nullptr;
	}
	if (pD2DFactory)
	{
		pD2DFactory->Release();
		pD2DFactory = nullptr;
	}
	if (pD3D11On12Device)
	{
		pD3D11On12Device->Release();
		pD3D11On12Device = nullptr;
	}
	if (pD3D11DeviceContext)
	{
		pD3D11DeviceContext->Release();
		pD3D11DeviceContext = nullptr;
	}

	return true;
}

bool CFontManager::InitializeDWrite(ID3D12Device* pD3DDevice, UINT TextureWidth, UINT TextureHeight, float DPI)
{
	m_D2DBitmapHeight = TextureHeight;
	m_D2DBitmapWidth = TextureWidth;

	D2D1_SIZE_U	size;
	size.width = TextureWidth;
	size.height = TextureHeight;

	D2D1_BITMAP_PROPERTIES1 bitmapProperties =
		D2D1::BitmapProperties1(
			D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
			D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
			DPI,
			DPI
		);

	if (FAILED(m_pD2DDeviceContext->CreateBitmap(size, nullptr, 0, &bitmapProperties, &m_pD2DTargetBitmap)))
	{
		__debugbreak();
	}

	bitmapProperties.bitmapOptions = D2D1_BITMAP_OPTIONS_CANNOT_DRAW | D2D1_BITMAP_OPTIONS_CPU_READ;
	if (FAILED(m_pD2DDeviceContext->CreateBitmap(size, nullptr, 0, &bitmapProperties, &m_pD2DTargetBitmapRead)))
	{
		__debugbreak();
	}

	if (FAILED(m_pD2DDeviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &m_pWhiteColorBrush)))
	{
		__debugbreak();
	}
	
	if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory5), (IUnknown**)&m_pDWFactory)))
	{
		__debugbreak();
	}

	return true;
}

bool CFontManager::RenderTextToTargetBitmap(INT* pOutWidth, INT* pOutHeight, IDWriteTextFormat* pTextFormat, const WCHAR* String, UINT Len)
{
	ID2D1DeviceContext* pD2DDeviceContext = m_pD2DDeviceContext;
	IDWriteFactory5* pDWFactory = m_pDWFactory;
	D2D1_SIZE_F max_size = pD2DDeviceContext->GetSize();
	max_size.width = (float)m_D2DBitmapWidth;
	max_size.height = (float)m_D2DBitmapHeight;

	IDWriteTextLayout* pTextLayout = nullptr;

	if(pDWFactory && pTextFormat)
	{
		if (FAILED(pDWFactory->CreateTextLayout(String, Len, pTextFormat, max_size.width, max_size.height, &pTextLayout)))
		{
			__debugbreak();
		}
	}
	else
	{
		return false;
	}


	DWRITE_TEXT_METRICS metrics = {};
	if (pTextLayout)
	{
		pTextLayout->GetMetrics(&metrics);

		// Ÿ�ټ���
		pD2DDeviceContext->SetTarget(m_pD2DTargetBitmap);

		// ��Ƽ�ٸ���̸�� ����
		pD2DDeviceContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);

		// �ؽ�Ʈ ������
		pD2DDeviceContext->BeginDraw();

		pD2DDeviceContext->Clear(D2D1::ColorF(D2D1::ColorF::Black));
		pD2DDeviceContext->SetTransform(D2D1::Matrix3x2F::Identity());

		pD2DDeviceContext->DrawTextLayout(D2D1::Point2F(0.0f, 0.0f), pTextLayout, m_pWhiteColorBrush);

		// We ignore D2DERR_RECREATE_TARGET here. This error indicates that the device
		// is lost. It will be handled during the next call to Present.
		pD2DDeviceContext->EndDraw();

		// ��Ƽ�ٸ���� ��� ����    
		pD2DDeviceContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_DEFAULT);
		pD2DDeviceContext->SetTarget(nullptr);

		// ���̾ƿ� ������Ʈ �ʿ������ Release
		pTextLayout->Release();
		pTextLayout = nullptr;
	}


	UINT width = (UINT)ceil(metrics.width);
	UINT height = (UINT)ceil(metrics.height);

	D2D1_POINT_2U	destPos = {};
	D2D1_RECT_U		srcRect = { 0, 0, width, height };
	if (FAILED(m_pD2DTargetBitmapRead->CopyFromBitmap(&destPos, m_pD2DTargetBitmap, &srcRect)))
	{
		__debugbreak();
	}
		

	*pOutWidth = width;
	*pOutHeight = height;


	return true;
}


void CFontManager::Cleanup()
{
	CleanupDWrite();
	CleanupD2D();
}

void CFontManager::CleanupDWrite()
{
	if (m_pD2DTargetBitmap)
	{
		m_pD2DTargetBitmap->Release();
		m_pD2DTargetBitmap = nullptr;
	}
	if (m_pD2DTargetBitmapRead)
	{
		m_pD2DTargetBitmapRead->Release();
		m_pD2DTargetBitmapRead = nullptr;
	}

	if(m_pDWFactory)
	{
		m_pDWFactory->Release();
		m_pDWFactory = nullptr;
	}

	if(m_pWhiteColorBrush)
	{
		m_pWhiteColorBrush->Release();
		m_pWhiteColorBrush = nullptr;
	}
}

void CFontManager::CleanupD2D()
{
	if(m_pD2DDevice)
	{
		m_pD2DDevice->Release();
		m_pD2DDevice = nullptr;
	}
	if(m_pD2DDeviceContext)
	{
		m_pD2DDeviceContext->Release();
		m_pD2DDeviceContext = nullptr;
	}
}
