
// ContentsAgent.h : PROJECT_NAME ���� ���α׷��� ���� �� ��� �����Դϴ�.
//

#pragma once

#ifndef __AFXWIN_H__
	#error "PCH�� ���� �� ������ �����ϱ� ���� 'stdafx.h'�� �����մϴ�."
#endif

#include "resource.h"		// �� ��ȣ�Դϴ�.


// CContentsAgentApp:
// �� Ŭ������ ������ ���ؼ��� ContentsAgent.cpp�� �����Ͻʽÿ�.
//

class CContentsAgentApp : public CWinApp
{
public:
	CContentsAgentApp();

// �������Դϴ�.
public:
	virtual BOOL InitInstance();

// �����Դϴ�.

	DECLARE_MESSAGE_MAP()
};

extern CContentsAgentApp theApp;