
// AgentMonitorDlg.cpp : 구현 파일
//

#include "stdafx.h"
#include "AgentMonitor.h"
#include "AgentMonitorDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define _UDP_PORT				12345
#define _DFT_HOST				"192.168.0.50"
//#define _DFT_HOST				"192.168.25.5"

#ifdef _DEBUG
#define _INTERVAL_BROADCAST		3	// 초
#else
#define _INTERVAL_BROADCAST		10	// 초
#endif


// CAgentMonitorDlg 대화 상자




CAgentMonitorDlg::CAgentMonitorDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CAgentMonitorDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_pThread = NULL;
	m_pEvent = NULL;
	m_nTimer = 0;
}

CAgentMonitorDlg::~CAgentMonitorDlg()
{
	if (m_pThread != NULL)
	{
		WaitForSingleObject(m_pThread->m_hThread, INFINITE);
		delete m_pEvent;
		delete m_pThread;
	}
}

void CAgentMonitorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAgentMonitorDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON1, &CAgentMonitorDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CAgentMonitorDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, &CAgentMonitorDlg::OnBnClickedButton3)
END_MESSAGE_MAP()


// CAgentMonitorDlg 메시지 처리기

BOOL CAgentMonitorDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 이 대화 상자의 아이콘을 설정합니다. 응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	ShowWindow(SW_MAXIMIZE);

	CRect r, r2;
	GetClientRect(r);

	int w = r.Width() / 2;

	GetDlgItem(IDC_BUTTON1)->GetWindowRect(r2);
	GetDlgItem(IDC_BUTTON1)->SetWindowPos(NULL, r.left, r.top, r2.Width(), r2.Height(), SWP_NOZORDER);
	GetDlgItem(IDC_BUTTON2)->SetWindowPos(NULL, r.left + w, r.top, r2.Width(), r2.Height(), SWP_NOZORDER);

	int top = r2.Height() + 10;
	GetDlgItem(IDC_EDIT1)->SetWindowPos(NULL, r.left, top, w, r.Height() - top, SWP_NOZORDER);
	GetDlgItem(IDC_EDIT2)->SetWindowPos(NULL, r.left + w, top, w, r.Height() - top, SWP_NOZORDER);
	
	GetDlgItem(IDC_EDIT3)->SetWindowText(_DFT_HOST);
	
	BroadcastSend();

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다. 문서/뷰 모델을 사용하는 MFC 응용 프로그램의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CAgentMonitorDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CAgentMonitorDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CAgentMonitorDlg::OnDestroy()
{
	m_bRun = FALSE;
	if (m_pThread != NULL)
	{
		m_pEvent->SetEvent();
	}

	CDialogEx::OnDestroy();
}

void CAgentMonitorDlg::OnTimer(UINT_PTR nIDEvent)
{
	KillTimer(1);
	BroadcastSend();

	CDialogEx::OnTimer(nIDEvent);
}

UINT __cdecl WorkProc(LPVOID pParam)
{
	((CAgentMonitorDlg*)pParam)->OnThreadWorkProc();

	AfxEndThread(0, FALSE);
	return 0;
}

void CAgentMonitorDlg::BroadcastSend()
{
	static int nSeq = 1;
	CString s;

	if (m_pThread == NULL)
	{
		m_bRun = TRUE;
		m_pEvent = new CEvent(FALSE, FALSE);
		m_pThread = AfxBeginThread(WorkProc, this);

		if (m_pThread == NULL)
		{
			m_bRun = FALSE;
			delete m_pEvent;
			m_pEvent = NULL;

			ReportMessageWrite(_T("쓰레드 생성 실패"));
			return;
		}
		
		m_pThread->m_bAutoDelete = FALSE;

		ReportMessageWrite(_T("쓰레드 생성 성공"));
	}

	SOCKET sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == INVALID_SOCKET)
	{
		s.Format(_T("\r\nSEND %d -> 실패: 소켓생성(%d)"), nSeq, WSAGetLastError());
		BroadcastMessageWrite(s);
	}

	char opt = 1; 
	setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char*)&opt, sizeof(char));

	SOCKADDR_IN sockaddr;
	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(_UDP_PORT);
	sockaddr.sin_addr.s_addr = inet_addr("192.168.0.255");

	char msg[256];
	int nMsg = sprintf_s(msg, _T("SHi, there! (%d)"), nSeq);
	int n = sendto(sock, msg, nMsg, 0, (struct sockaddr*)&sockaddr, (int)sizeof(sockaddr));

	if (n == SOCKET_ERROR)
	{
		s.Format(_T("\r\nSEND %d -> 실패: 발송(%d)"), nSeq, WSAGetLastError());
		BroadcastMessageWrite(s);
	}
	else
	{
		s.Format(_T("\r\nSEND %d -> 성공(%d/%d)"), nSeq, n, nMsg);
		BroadcastMessageWrite(s);
	}

	::closesocket(sock);

	nSeq++;
	m_nTimer = SetTimer(1, _INTERVAL_BROADCAST * 1000, NULL);
}

void CAgentMonitorDlg::BroadcastMessageWrite(LPCTSTR pMsg)
{
	CString s, sLog;
	CEdit* pEdit = (CEdit*)GetDlgItem(IDC_EDIT1);

	pEdit->GetWindowText(sLog);

	if (sLog.IsEmpty())
	{
		s = pMsg;
	}
	else
	{
		s.Format(_T("\r\n%s"), pMsg);
	}

	sLog += s;

	pEdit->SetWindowText(sLog);
	pEdit->LineScroll(pEdit->GetLineCount());
}

void CAgentMonitorDlg::ReportMessageWrite(LPCTSTR pMsg)
{
	CString s, sLog;
	CEdit* pEdit = (CEdit*)GetDlgItem(IDC_EDIT2);

	pEdit->GetWindowText(sLog);

	if (sLog.IsEmpty())
	{
		s = pMsg;
	}
	else
	{
		s.Format(_T("\r\n%s"), pMsg);
	}

	sLog += s;

	pEdit->SetWindowText(sLog);
	pEdit->LineScroll(pEdit->GetLineCount());
}

void CAgentMonitorDlg::OnThreadWorkProc()
{
	CString s;
	SOCKET sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == INVALID_SOCKET)
	{
		s.Format(_T("RECV -> 실패: 소켓생성(%d)"), WSAGetLastError());
		ReportMessageWrite(s);
		return;
	}

	SOCKADDR_IN sockaddr;
	memset(&sockaddr,0, sizeof(SOCKADDR_IN));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(_UDP_PORT);
	sockaddr.sin_addr.s_addr = INADDR_ANY;
    
	if (bind(sock, (SOCKADDR*)&sockaddr, sizeof(SOCKADDR_IN)) == SOCKET_ERROR) 
	{
		s.Format(_T("RECV -> 실패: binding(%d)"), WSAGetLastError());
		ReportMessageWrite(s);
		return;
	}

	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 200;

	fd_set fds;
	int i, n, nRcv;
	char buf[256];

	while (m_bRun)
	{
		FD_ZERO(&fds);
		FD_SET(sock, &fds);

		n = select(sizeof(fd_set) * 8, &fds, NULL, NULL, &timeout);
		if (n > 0)
		{
			n = sizeof(sockaddr);
			nRcv = recvfrom(sock, buf, 256, 0, (struct sockaddr*)&sockaddr, &n);
			if (nRcv > 0)
			{
				buf[nRcv] = 0x00;

				s.Format(_T("RECV -> (%s) %s"), inet_ntoa(sockaddr.sin_addr), buf + 1);

				switch (*buf)
				{
					case 'S':
						ReportMessageWrite(s);
						sockaddr.sin_port = htons(_UDP_PORT);
						i = sprintf_s(buf, _T("RHi!"));
						sendto(sock, buf, i, 0, (struct sockaddr*)&sockaddr, n);
						break;
					case 'R':
						BroadcastMessageWrite(s);
						break;
					default:
						ReportMessageWrite(s);
				}
			}	
		}
	}
}


void CAgentMonitorDlg::OnBnClickedButton1()
{
	CEdit* pEdit = (CEdit*)GetDlgItem(IDC_EDIT1);
	pEdit->SetSel(0, -1, TRUE);
	pEdit->Copy();
	pEdit->SetSel(-1, -1, TRUE);
}


void CAgentMonitorDlg::OnBnClickedButton2()
{
	CEdit* pEdit = (CEdit*)GetDlgItem(IDC_EDIT2);
	pEdit->SetSel(0, -1, TRUE);
	pEdit->Copy();
	pEdit->SetSel(-1, -1, TRUE);
}


void CAgentMonitorDlg::OnBnClickedButton3()
{
	CString s;
	GetDlgItem(IDC_EDIT3)->GetWindowText(s);

	SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == INVALID_SOCKET)
	{
		s.Format(_T("실패: 소켓생성(%d)"), WSAGetLastError());
		AfxMessageBox(s);
		return;
	}

	SOCKADDR_IN sockaddr;
	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(_UDP_PORT);
	sockaddr.sin_addr.s_addr = inet_addr(s);

	s.Format(_T("SHi! there~ (2)"));

	int n = sendto(sock, s.GetBuffer(0), s.GetLength(), 0, (struct sockaddr*)&sockaddr, sizeof(sockaddr));

	if (n == SOCKET_ERROR)
	{
		s.Format(_T("실패: 발송(%d)"), WSAGetLastError());
		AfxMessageBox(s);
	}

	::closesocket(sock);
}
