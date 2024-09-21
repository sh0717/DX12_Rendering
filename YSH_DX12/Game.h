#pragma once
#include <vector>

//test
class CGame
{
public:
	CGame();
	~CGame();
	BOOL	Initialiize(HWND hWnd, BOOL bEnableDebugLayer, BOOL bEnableGBV);
	void Run();

	BOOL	UpdateWindowSize(DWORD dwBackBufferWidth, DWORD dwBackBufferHeight);
	void	OnKeyDown(UINT nChar, UINT uiScanCode);
	void	OnKeyUp(UINT nChar, UINT uiScanCode);

	class CD3D12Renderer* GetRenderer()const;

private:
	bool Update(UINT64 CurTick);
	void Render();

	void Cleanup();


	class CGameObject* CreateGameObject();
	void DeleteGameObject(class CGameObject* pGameObject);
	void DeleteAllGameObject();

private:
	class CD3D12Renderer* m_pRenderer = nullptr;
	HWND m_hWnd = nullptr;

	UINT64 m_PrvFrameCheckTick = 0;
	UINT64 m_PrvUpdateTick = 0;
	UINT m_FrameCount = 0;
	UINT m_FPS = 0;
	WCHAR m_wchText[64] = {};

	float m_CamOffsetX = 0.0f;
	float m_CamOffsetY = 0.0f;
	float m_CamOffsetZ = 0.0f;


	BOOL	m_bShiftKeyDown = FALSE;

	std::vector<void*> GameObjectArray;

	


};

