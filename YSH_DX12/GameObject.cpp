#include "pch.h"
#include "GameObject.h"

#include "D3D12Renderer.h"
#include "Game.h"

CGameObject::CGameObject()
{
	m_matScale = XMMatrixIdentity();
	m_matRot = XMMatrixIdentity();
	m_matTrans = XMMatrixIdentity();
	m_matWorld = XMMatrixIdentity();
}

CGameObject::~CGameObject()
{
	Clenaup();
}

bool CGameObject::initialize(class CGame* pGame)
{
	bool bResult = false;

	m_pGame = pGame;
	m_pRenderer = pGame->GetRenderer();


	m_pMeshObject = CreateBoxMeshObject();

	bResult = true;
	return bResult;
}

void CGameObject::SetPosition(float x, float y, float z)
{
	m_Position.m128_f32[0] = x;
	m_Position.m128_f32[1] = y;
	m_Position.m128_f32[2] = z;

	m_matTrans = XMMatrixTranslation(x, y, z);
	m_bUpdateTransform = true;
}

void CGameObject::SetScale(float x, float y, float z)
{
	m_Scale.m128_f32[0] = x;
	m_Scale.m128_f32[1] = y;
	m_Scale.m128_f32[2] = z;

	m_matScale = XMMatrixScaling(x, y, z);
	m_bUpdateTransform = true;
}

void CGameObject::SetRotationY(float RotY)
{
	m_RotY = RotY;
	m_matRot = XMMatrixRotationY(RotY);
	m_bUpdateTransform = true;
}


void CGameObject::UpdateTransform()
{
	m_matWorld = XMMatrixMultiply(m_matScale, m_matRot);
	m_matWorld = XMMatrixMultiply(m_matWorld, m_matTrans);
}


void CGameObject::Update()
{
	if(m_bUpdateTransform)
	{
		UpdateTransform();
		m_bUpdateTransform = false;
	}
}

void CGameObject::Render()
{
	if(m_pMeshObject)
	{
		m_pRenderer->RenderMeshObject(m_pMeshObject, &m_matWorld);
	}
}


void CGameObject::Clenaup()
{
	if(m_pMeshObject)
	{
		m_pRenderer->DeleteBasicMeshObject(m_pMeshObject);
		m_pMeshObject = nullptr;
	}
}

void* CGameObject::CreateBoxMeshObject()
{
	void* pMeshObj = nullptr;

	// create box mesh
	// create vertices and indices
	WORD	pIndexList[36] = {};
	BasicVertex* pVertexList = nullptr;
	DWORD dwVertexCount = CreateBoxMesh(&pVertexList, pIndexList, (DWORD)_countof(pIndexList), 0.25f);

	// create CBasicMeshObject from Renderer
	pMeshObj = m_pRenderer->CreateBasicMeshObject();

	const WCHAR* wchTexFileNameList[6] =
	{
		L"./Assets/Textures/tex_01.dds",
		L"./Assets/Textures/tex_02.dds",
		L"./Assets/Textures/tex_03.dds",
		L"./Assets/Textures/tex_04.dds",
		L"./Assets/Textures/tex_05.dds",
		L"./Assets/Textures/tex_07.dds"
	};

	// Set meshes to the CBasicMeshObject
	m_pRenderer->InsertVertexDataToMeshObject(pMeshObj, pVertexList, dwVertexCount, 6);	// 박스의 6면-1면당 삼각형 2개-인덱스 6개
	for (DWORD i = 0; i < 6; i++)
	{
		m_pRenderer->InsertTriGroupDataToMeshObject(pMeshObj, pIndexList + i * 6, 2, wchTexFileNameList[i]);
	}
	m_pRenderer->EndCreateMesh(pMeshObj);

	// delete vertices and indices
	if (pVertexList)
	{
		DeleteBoxMesh(pVertexList);
		pVertexList = nullptr;
	}
	return pMeshObj;
}
