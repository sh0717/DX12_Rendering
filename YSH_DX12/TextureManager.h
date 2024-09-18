#pragma once
#include <set>

class CTextureManager
{
public:
	CTextureManager();
	~CTextureManager();

	bool Initialize(class CD3D12Renderer* pRenderer, class CD3D12ResourceManager* pResourceManager, UINT MaxBucketCount, UINT MaxFileCount);

	TextureHandle* CreateTextureFromFile(const WCHAR* FileName);
	TextureHandle* CreateDynamicTexture(UINT TextureWidth, UINT TextureHeight);
	TextureHandle* CreateImmutableTexture(UINT TexWidth, UINT TexHeight, DXGI_FORMAT TextureFormat, const BYTE* pInitImage);
	void DeleteTexture(TextureHandle* pTextureHandle);

private:
	TextureHandle* AllocateTextureHandle();
	UINT DeallocateTextureHandle(TextureHandle* pTextureHandle);

	void Cleanup();
private:/*variable*/

	CD3D12Renderer* m_pRenderer = nullptr;
	CD3D12ResourceManager* m_pResourceManager = nullptr;

	class CHashTable* m_pHashTable = nullptr;

	std::set<void*> m_TextureHandles;

};

