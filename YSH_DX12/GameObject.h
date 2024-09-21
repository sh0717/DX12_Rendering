#pragma once
class CGameObject
{
public:/*function*/
	CGameObject();
	~CGameObject();

	bool initialize(class CGame* pGame);

	void SetPosition(float x, float y, float z);
	void SetScale(float  x, float y, float z);
	void SetRotationY(float RotY);

	void Update();
	void Render();

private:/*function*/
	void UpdateTransform();

	void Clenaup();

	void* CreateBoxMeshObject();
private:

	class CGame* m_pGame = nullptr;
	class CD3D12Renderer* m_pRenderer = nullptr;
	void* m_pMeshObject = nullptr;     //CBasicObject

	XMVECTOR m_Scale = { 1.f , 1.f , 1.f, 1.f };
	XMVECTOR m_Position = {};
	float m_RotY = 0.f;

	XMMATRIX m_matScale = {};
	XMMATRIX m_matRot = {};
	XMMATRIX m_matTrans = {};
	XMMATRIX m_matWorld = {};
	bool	m_bUpdateTransform = false;

};

