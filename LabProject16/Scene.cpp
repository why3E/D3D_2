#include "Scene.h"
#include "stdafx.h"
#include "Shader.h"

CScene::CScene()
{
}
CScene::~CScene()
{

}

void CScene::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList
	* pd3dCommandList)
{

}

void CScene::ReleaseObjects()
{
	if (m_pd3dGraphicsRootSignature) m_pd3dGraphicsRootSignature->Release();
	for (int i = 0; i < m_nShaders; i++)
	{
		m_pShaders[i]->ReleaseShaderVariables();
		m_pShaders[i]->ReleaseObjects();
	}
	if (m_pShaders) delete[] m_pShaders;
}

bool CScene::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM
	lParam)
{
	return(false);
}
bool CScene::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam,
	LPARAM lParam)
{
	return(false);
}



bool CScene::ProcessInput(UCHAR* pKeysBuffer)
{
	return(false);
}

void CScene::AnimateObjects(float fTimeElapsed)
{
	for (int i = 0; i < m_pShaders[0]->m_nObjects; i++)
	{
		m_pShaders[0]->m_ppObjects[i]->m_pLockedObject = m_pPlayer->GetPosition();
	}


	for (int i = 0; i < m_nShaders; i++)
	{

		m_pShaders[i]->AnimateObjects(fTimeElapsed);
	}


	CheckObjectByBulletCollisions();
	CheckPlyaerByBulletCollisions();
	CheckObjectByWallCollisions();
}



void CScene::ReleaseUploadBuffers()
{
	for (int i = 0; i < m_nShaders; i++) m_pShaders[i]->ReleaseUploadBuffers();
}

void CScene::setPlayer(CPlayer* pPlayer)
{
	m_pPlayer = pPlayer;
}

void CScene::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	pCamera->SetViewportsAndScissorRects(pd3dCommandList);
	pd3dCommandList->SetGraphicsRootSignature(m_pd3dGraphicsRootSignature);
	pCamera->UpdateShaderVariables(pd3dCommandList);
	for (int i = 0; i < m_nShaders; i++)
	{
		m_pShaders[i]->Render(pd3dCommandList, pCamera);
	}
}

ID3D12RootSignature* CScene::GetGraphicsRootSignature()
{
	return(m_pd3dGraphicsRootSignature);
}

ID3D12RootSignature* CScene::CreateGraphicsRootSignature(ID3D12Device* pd3dDevice)
{
	ID3D12RootSignature* pd3dGraphicsRootSignature = NULL;
	D3D12_ROOT_PARAMETER pd3dRootParameters[2];
	pd3dRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	pd3dRootParameters[0].Constants.Num32BitValues = 16;
	pd3dRootParameters[0].Constants.ShaderRegister = 0;
	pd3dRootParameters[0].Constants.RegisterSpace = 0;
	pd3dRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	pd3dRootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	pd3dRootParameters[1].Constants.Num32BitValues = 32;
	pd3dRootParameters[1].Constants.ShaderRegister = 1;
	pd3dRootParameters[1].Constants.RegisterSpace = 0;
	pd3dRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	D3D12_ROOT_SIGNATURE_FLAGS d3dRootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
	D3D12_ROOT_SIGNATURE_DESC d3dRootSignatureDesc;
	::ZeroMemory(&d3dRootSignatureDesc, sizeof(D3D12_ROOT_SIGNATURE_DESC));
	d3dRootSignatureDesc.NumParameters = _countof(pd3dRootParameters);
	d3dRootSignatureDesc.pParameters = pd3dRootParameters;
	d3dRootSignatureDesc.NumStaticSamplers = 0;
	d3dRootSignatureDesc.pStaticSamplers = NULL;
	d3dRootSignatureDesc.Flags = d3dRootSignatureFlags;
	ID3DBlob* pd3dSignatureBlob = NULL;
	ID3DBlob* pd3dErrorBlob = NULL;
	::D3D12SerializeRootSignature(&d3dRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		&pd3dSignatureBlob, &pd3dErrorBlob);
	pd3dDevice->CreateRootSignature(0, pd3dSignatureBlob->GetBufferPointer(),
		pd3dSignatureBlob->GetBufferSize(), __uuidof(ID3D12RootSignature), (void
			**)&pd3dGraphicsRootSignature);
	if (pd3dSignatureBlob) pd3dSignatureBlob->Release();
	if (pd3dErrorBlob) pd3dErrorBlob->Release();
	return(pd3dGraphicsRootSignature);
}

CGameObject* CScene::PickObjectPointedByCursor(int xClient, int yClient, CCamera
	* pCamera)
{
	if (!pCamera) return(NULL);
	XMFLOAT4X4 xmf4x4View = pCamera->GetViewMatrix();
	XMFLOAT4X4 xmf4x4Projection = pCamera->GetProjectionMatrix();
	D3D12_VIEWPORT d3dViewport = pCamera->GetViewport();
	XMFLOAT3 xmf3PickPosition;
	/*화면 좌표계의 점 (xClient, yClient)를 화면 좌표 변환의 역변환과 투영 변환의 역변환을 한다. 그 결과는 카메라
   좌표계의 점이다. 투영 평면이 카메라에서 z-축으로 거리가 1이므로 z-좌표는 1로 설정한다.*/
	xmf3PickPosition.x = (((2.0f * xClient) / d3dViewport.Width) - 1) /
		xmf4x4Projection._11;
	xmf3PickPosition.y = -(((2.0f * yClient) / d3dViewport.Height) - 1) /
		xmf4x4Projection._22;
	xmf3PickPosition.z = 1.0f;
	int nIntersected = 0;
	float fHitDistance = FLT_MAX, fNearestHitDistance = FLT_MAX;
	CGameObject* pIntersectedObject = NULL, * pNearestObject = NULL;
	//셰이더의 모든 게임 객체들에 대한 마우스 픽킹을 수행하여 카메라와 가장 가까운 게임 객체를 구한다.
	for (int i = 0; i < m_nShaders; i++)
	{
		pIntersectedObject = m_pShaders[i]->PickObjectByRayIntersection(xmf3PickPosition,
			xmf4x4View, &fHitDistance);
		if (pIntersectedObject && (fHitDistance < fNearestHitDistance))
		{
			fNearestHitDistance = fHitDistance;
			pNearestObject = pIntersectedObject;
		}
	}
	return(pNearestObject);
}

void CScene::CheckObjectByBulletCollisions()
{
	CBulletObject** ppBullets = ((CAirplanePlayer*)m_pPlayer)->m_ppBullets;

	for (int i = 0; i < m_pShaders[0]->m_nObjects; i++)
	{
		for (int j = 0; j < BULLETS; j++)
		{
			if (ppBullets[j]->m_bActive && m_pShaders[0]->m_ppObjects[i]->m_xmOOBB.Intersects(ppBullets[j]->m_xmOOBB))
			{
				CExplosiveObject* pExplosiveObject = (CExplosiveObject*)m_pShaders[0]->m_ppObjects[i];
				pExplosiveObject->m_bBlowingUp = true;
				ppBullets[j]->Reset();
			}
		}
	}

	if (m_pPlayer->m_shield)
	{
		for (int i = 0; i < m_pShaders[0]->m_nObjects; i++)
		{
			CBulletObject** ppBullets = ((CExplosiveObject*)m_pShaders[0]->m_ppObjects[i])->m_ppBullets;
			for (int j = 0; j < BULLETS; j++)
			{
				if (ppBullets[j]->m_bActive && m_pPlayer->m_pshield->m_xmOOBB.Intersects(ppBullets[j]->m_xmOOBB))
				{
					ppBullets[j]->Reset();
				}
			}
		}
	}
}

void CScene::CheckPlyaerByBulletCollisions()
{
	for (int i = 0; i < m_pShaders[0]->m_nObjects; i++)
	{
		CBulletObject** ppBullets = ((CExplosiveObject*)m_pShaders[0]->m_ppObjects[i])->m_ppBullets;

		for (int j = 0; j < BULLETS; j++)
		{
			if (ppBullets[j]->m_bActive && m_pPlayer->m_xmOOBB.Intersects(ppBullets[j]->m_xmOOBB))
			{
				m_pPlayer->m_bBlowingUp = true;
				ppBullets[j]->Reset();
			}
		}
	}

}

void CScene::CheckObjectByWallCollisions()
{
}

void stage_Scene::CheckObjectByWallCollisions()
{
	if (m_nShaders > 1){
		for (int i = 0; i < m_pShaders[1]->m_nObjects; i++)
		{
			ContainmentType containType = m_pShaders[1]->m_ppObjects[i]->m_xmOOSS.Contains(m_pPlayer->m_xmOOBB);
			switch (containType)
			{
			case DISJOINT:
			{
				XMFLOAT3 wallPos = m_pShaders[1]->m_ppObjects[i]->GetPosition();
				XMFLOAT3 playerPos = m_pPlayer->GetPosition();

				XMFLOAT3 normalVec = Vector3::Normalize(Vector3::Subtract(wallPos, playerPos));


				XMFLOAT3 playerDirection = m_pPlayer->GetVelocity();

				float dotProduct = Vector3::DotProduct(playerDirection, normalVec);
				XMFLOAT3 reflectedDirection = Vector3::Subtract(playerDirection, Vector3::ScalarProduct(normalVec, 2.0f * dotProduct));
				m_pPlayer->SetVelocity(reflectedDirection);

				break;
			}
			case INTERSECTS:
			{
				XMFLOAT3 wallPos = m_pShaders[1]->m_ppObjects[i]->GetPosition();
				XMFLOAT3 playerPos = m_pPlayer->GetPosition();

				XMFLOAT3 normalVec = Vector3::Normalize(Vector3::Subtract(wallPos, playerPos));

				XMFLOAT3 playerDirection = m_pPlayer->GetVelocity();

				float dotProduct = Vector3::DotProduct(playerDirection, normalVec);
				XMFLOAT3 reflectedDirection = Vector3::Subtract(playerDirection, Vector3::ScalarProduct(normalVec, 2.0f * dotProduct));
				m_pPlayer->SetVelocity(reflectedDirection);

				break;
			}
			case CONTAINS:
				break;
			}
		}
	}
}

// 씬 빌드ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ
start_Scene::start_Scene()
{
	
}
start_Scene::~start_Scene()
{
}
void start_Scene::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	type = 0;
	CExplosiveObject::PrepareExplosion(pd3dDevice, pd3dCommandList);

	m_pd3dGraphicsRootSignature = CreateGraphicsRootSignature(pd3dDevice);
	m_nShaders = 1;
	m_pShaders = new CObjectsShader * [m_nShaders];

	CstartShader* startShader = new CstartShader();
	m_pShaders[0] = startShader;
	m_pShaders[0]->CreateShader(pd3dDevice, m_pd3dGraphicsRootSignature);
	m_pShaders[0]->BuildObjects(pd3dDevice, pd3dCommandList);

}


stage_Scene::stage_Scene()
{
}
stage_Scene::~stage_Scene()
{
}

bool stage_Scene::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam,
	LPARAM lParam)
{
	switch (nMessageID)
	{
	case WM_KEYDOWN:
		switch (wParam)
		{
		case '1': {
			CExplosiveObject* pExplosiveObject = (CExplosiveObject*)m_pShaders[0]->m_ppObjects[int(wParam - '1')];
			pExplosiveObject->m_bBlowingUp = true;
		}
				break;
		case '2': {
			CExplosiveObject* pExplosiveObject = (CExplosiveObject*)m_pShaders[0]->m_ppObjects[int(wParam - '1')];
			pExplosiveObject->m_bBlowingUp = true;
		}
				break;
		case '3': {
			CExplosiveObject* pExplosiveObject = (CExplosiveObject*)m_pShaders[0]->m_ppObjects[int(wParam - '1')];
			pExplosiveObject->m_bBlowingUp = true;
		}
				break;
		case '4': {
			CExplosiveObject* pExplosiveObject = (CExplosiveObject*)m_pShaders[0]->m_ppObjects[int(wParam - '1')];
			pExplosiveObject->m_bBlowingUp = true;
		}
				break;
		case '5': {
			CExplosiveObject* pExplosiveObject = (CExplosiveObject*)m_pShaders[0]->m_ppObjects[int(wParam - '1')];
			pExplosiveObject->m_bBlowingUp = true;
		}
				break;
		case '6': {
			CExplosiveObject* pExplosiveObject = (CExplosiveObject*)m_pShaders[0]->m_ppObjects[int(wParam - '1')];
			pExplosiveObject->m_bBlowingUp = true;
		}
				break;
		case '7': {
			CExplosiveObject* pExplosiveObject = (CExplosiveObject*)m_pShaders[0]->m_ppObjects[int(wParam - '1')];
			pExplosiveObject->m_bBlowingUp = true;
		}
				break;

		case 'A':
			for (int i = 0; i < m_pShaders[0]->m_nObjects; i++)
			{
				CExplosiveObject* pExplosiveObject = (CExplosiveObject*)m_pShaders[0]->m_ppObjects[i];
				pExplosiveObject->m_bBlowingUp = true;
			}
			break;
		case 'X':
			m_pPlayer->m_shield = false;
			break;
		case 'Z':
			m_pPlayer->m_shield = true;
			break;
		default:
			break;
		}
		break;
	default:
		break;

	}
	return(false);
}


void stage_Scene::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	type = 1;
	CExplosiveObject::PrepareExplosion(pd3dDevice, pd3dCommandList);

	m_pd3dGraphicsRootSignature = CreateGraphicsRootSignature(pd3dDevice);
	m_nShaders = 2;
	m_pShaders = new CObjectsShader * [m_nShaders];

	CstageShader* stageShader = new CstageShader;
	m_pShaders[0] = (CObjectsShader*)stageShader;
	m_pShaders[0]->CreateShader(pd3dDevice, m_pd3dGraphicsRootSignature);
	m_pShaders[0]->BuildObjects(pd3dDevice, pd3dCommandList);


	CwallShader* wallShader = new CwallShader;
	m_pShaders[1] = (CObjectsShader*)wallShader;
	m_pShaders[1]->CreateShader(pd3dDevice, m_pd3dGraphicsRootSignature);
	m_pShaders[1]->BuildObjects(pd3dDevice, pd3dCommandList);

}
//ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ