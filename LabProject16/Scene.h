#pragma once
#include "Timer.h"
#include "stdafx.h"
#include "Shader.h"
#include "Camera.h"

class CScene
{
public:
	//���� ��� ���� ��ü�鿡 ���� ���콺 ��ŷ�� �����Ѵ�.
	CGameObject* PickObjectPointedByCursor(int xClient, int yClient, CCamera* pCamera);
public:
	CScene();
	~CScene();
	//������ ���콺�� Ű���� �޽����� ó���Ѵ�.
	bool OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM
		lParam);
	bool OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM
		lParam);
	virtual void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList
		* pd3dCommandList);
	void ReleaseObjects();
	bool ProcessInput(UCHAR* pKeysBuffer);
	void AnimateObjects(float fTimeElapsed);
	void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);
	void ReleaseUploadBuffers();
	//�׷��� ��Ʈ �ñ׳��ĸ� �����Ѵ�.
	ID3D12RootSignature* CreateGraphicsRootSignature(ID3D12Device* pd3dDevice);
	ID3D12RootSignature* GetGraphicsRootSignature();
protected:
	//���� ���� ��ü���� �����̴�. ���� ��ü�� ���̴��� �����Ѵ�.
	ID3D12RootSignature* m_pd3dGraphicsRootSignature = NULL;
	CObjectsShader* m_pShaders = NULL;
	int m_nShaders = 0;
};

class start_Scene :CScene
{
public:
	start_Scene();
	~start_Scene();

	virtual void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);

};

class stage_Scene :CScene
{
public:
	stage_Scene();
	~stage_Scene();

	virtual void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
};