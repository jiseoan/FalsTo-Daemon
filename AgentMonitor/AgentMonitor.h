
// AgentMonitor.h : PROJECT_NAME ���� ���α׷��� ���� �� ��� �����Դϴ�.
//

#pragma once

#ifndef __AFXWIN_H__
	#error "PCH�� ���� �� ������ �����ϱ� ���� 'stdafx.h'�� �����մϴ�."
#endif

#include "resource.h"		// �� ��ȣ�Դϴ�.


// CAgentMonitorApp:
// �� Ŭ������ ������ ���ؼ��� AgentMonitor.cpp�� �����Ͻʽÿ�.
//

class CAgentMonitorApp : public CWinApp
{
public:
	CAgentMonitorApp();

// �������Դϴ�.
public:
	virtual BOOL InitInstance();

// �����Դϴ�.

	DECLARE_MESSAGE_MAP()
};

extern CAgentMonitorApp theApp;