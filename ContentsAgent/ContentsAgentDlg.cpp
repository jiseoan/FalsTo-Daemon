
// ContentsAgentDlg.cpp : 구현 파일
//

#include "stdafx.h"
#include "ContentsAgent.h"
#include "ContentsAgentDlg.h"
#include "afxdialogex.h"
#include "Registry.h"
#include "../ziparchive/ZipArchive/ZipArchive.h"
#include <Winhttp.h>

//#define _THOLO
#define _KIOSK

#ifdef _DEBUG

#define new DEBUG_NEW
//#define _HTTP_TEST
#define _NO_APP_EXEC
#define _NO_PC_SHUTDOWN
#define _QUERY_INTERVAL_DATA		15	// 초
#define _QUERY_INTERVAL_ALIVE		5	// 초
#define _QUERY_INTERVAL_COMM		5	// 초

#else

#define _QUERY_INTERVAL_DATA		30	// 초
#define _QUERY_INTERVAL_ALIVE		15	// 초
#define _QUERY_INTERVAL_COMM		10	// 초

#endif

#define _UDP_PORT				12345

#define WM_UPDATE_DATA_2			(WM_USER + 1)

#ifdef _THOLO
#define _VIEWER_APP_NAME			_T("hologram")
#define _VIEWER_APP_TITLE			_T("Holo")
#define _VIEWER_APP_HOME			_T("C:\\Program Files (x86)\\T1view\\Tholo")
#define _VIEWER_APP_EXEC			_T("Holo-1680.bat")
#endif

#ifdef _KIOSK
#define _VIEWER_APP_NAME			_T("kiosk")
#define _VIEWER_APP_TITLE			_T("T1view-galleria Kiosk")
#define _VIEWER_APP_HOME			_T("C:\\Program Files (x86)\\T1view\\Kiosk\\T1view-galleria Kiosk")
#define _VIEWER_APP_EXEC			_T("T1view-galleria Kiosk.exe")
#endif

#define REGISTRY_PATH				_T("software\\jiseoan")
#define RK_CLIENTID					_T("cid")				// m_sAppID
#define RK_APPINSTDIR				_T("appinst")			// m_sAppHome
#define RK_SERVERIP					_T("sip")				// m_dwSrvIP, m_sSrvIP
#define RK_LOCALIP					_T("lcip")				// m_dwLocalIp, m_sLocalIP
#define RK_LOCALMAC					_T("lcmac")				// m_sLocalMac
#define RK_APPRELNO					_T("apprlsno")			// m_sAppReleaseNo
#define RK_CONTSRELNO				_T("cntsrlsno")			// m_sContentsReleaseNo
#define RK_APPCMDLINE				_T("appcmdln")			// m_sCmdLine

#define _CONF_FOLDER				_T("json")
#define _CONF_PATH_CONFIGURE_UPDATE	_T("configure.json")
#define _CONF_PATH_DATA_UPDATE		_T("dataupdate.json")
#define _ALIVE_FILEPATH				_T("conf\\alive.txt")


typedef struct _RCV_DATAITEM
{
	LPSTR pData;
	DWORD dwLen;
} RCV_DATAITEM, *PRCV_DATAITEM;



// 응용 프로그램 정보에 사용되는 CAboutDlg 대화 상자입니다.

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 대화 상자 데이터입니다.
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

// 구현입니다.
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CContentsAgentDlg 대화 상자




CContentsAgentDlg::CContentsAgentDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CContentsAgentDlg::IDD, pParent)
	, m_dwSrvIP(0)
	, m_sAppHome(_VIEWER_APP_HOME)
	, m_sLocalMac(_T(""))
	, m_dwLocalIp(0)
	, m_sAppID(_T("0"))
{
	ZeroMemory(m_Thrd, sizeof(XTHREAD_CONTEXT) * XTHREADS);
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_sSrvIP = _T("");
	m_sAppReleaseNo = _T("0");
	m_sContentsReleaseNo = _T("0");
	m_sCmdLine = _VIEWER_APP_EXEC;
	m_sLocalIP = _T("");
	m_sRanking = _T("");
	m_sAliveText = _T("");

	// 디버깅용 로그파일
	CString sDir;
	sDir.Format(_T("%s\\aglog"), _VIEWER_APP_HOME);
	::CreateDirectory(sDir, NULL);

	SYSTEMTIME st;
	GetLocalTime(&st);

	CString s;
	s.Format(_T("%s\\aglog_%02d%02d_%02d%02d%02d.txt"), sDir, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	m_bWriteLogFile = m_LogFile.Open(s, CFile::modeCreate | CFile::modeWrite, NULL);
}

CContentsAgentDlg::~CContentsAgentDlg()
{
	for (int i = 0 ; i < XTHREADS ; i++)
	{
		if (m_Thrd[i].pThread != NULL)
		{
			WaitForSingleObject(m_Thrd[i].pThread->m_hThread, INFINITE);
			delete m_Thrd[i].pEvent;
			delete m_Thrd[i].pThread;
		}
	}

	if (m_bWriteLogFile)
	{
		m_LogFile.Close();
	}
}

void CContentsAgentDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_IPAddress(pDX, IDC_SERVER_IP, m_dwSrvIP);
	DDX_Text(pDX, IDC_APP_HOME, m_sAppHome);
	DDX_Text(pDX, IDC_LOCAL_MAC, m_sLocalMac);
	DDX_IPAddress(pDX, IDC_LOCAL_IP, m_dwLocalIp);
	DDX_Text(pDX, IDC_APP_ID, m_sAppID);
	DDX_Text(pDX, IDC_APP_RELEASE_NO, m_sAppReleaseNo);
	DDX_Text(pDX, IDC_CONTENTS_RELEASE_NO, m_sContentsReleaseNo);
	DDX_Text(pDX, IDC_COMMAND_LINE, m_sCmdLine);
}

BEGIN_MESSAGE_MAP(CContentsAgentDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_APPLY, &CContentsAgentDlg::OnBnClickedApply)
	ON_BN_CLICKED(IDC_TEST, &CContentsAgentDlg::OnBnClickedTest)
	ON_WM_DESTROY()
	ON_MESSAGE(WM_UPDATE_DATA_2, OnUpdateData2)
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CContentsAgentDlg 메시지 처리기

BOOL CContentsAgentDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.

	// IDM_ABOUTBOX는 시스템 명령 범위에 있어야 합니다.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 이 대화 상자의 아이콘을 설정합니다. 응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	UpdateAgentTitle();

#ifndef DEBUG
	ShowWindow(SW_MINIMIZE);
#endif

	CString s;
	s.Format(_T("%s\\%s"), _VIEWER_APP_HOME, _CONF_FOLDER);
	::CreateDirectory(s, NULL);

	InitData();

#ifndef _HTTP_TEST
	ServiceBegin();
#endif

	AliveSend();

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다. 문서/뷰 모델을 사용하는 MFC 응용 프로그램의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CContentsAgentDlg::OnPaint()
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
HCURSOR CContentsAgentDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CContentsAgentDlg::InitData()
{
	ReadRegistry();

	if (m_sAppHome.IsEmpty())
	{
		TCHAR buf[MAX_PATH];
		GetCurrentDirectory(sizeof(TCHAR) * MAX_PATH, buf);
		m_sAppHome = buf;
	}

	CStringArray arrIp;
	if (GetIP(arrIp))
	{
		char buf[16];
		CStringArray arrMac;
		CString s;

		for (int i = 0, n = arrIp.GetCount() ; i < n ; i++)
		{
			s = arrIp.GetAt(i);
			if (!s.IsEmpty() && s.Compare(_T("0.0.0.0")) != 0 && GetMacAddress(arrMac, s))
			{
				m_sLocalMac = arrMac.GetAt(0);
				m_sLocalIP = s;
				WideCharToMultiByte(CP_ACP, 0, m_sLocalIP, -1, buf, 15, NULL, NULL);
				m_dwLocalIp = ntohl(inet_addr(buf));
				break;
			}
		}
	}

	UpdateData(FALSE);
}

void CContentsAgentDlg::ReadRegistry()
{
	CRegistry reg;
	CString s;

	s.Format(_T("%s\\%s"), REGISTRY_PATH, _VIEWER_APP_NAME);
	if (reg.Open(s, HKEY_CURRENT_USER, FALSE))
	{
		CString s;
		char buf[16];

		s = reg.ReadStr(RK_CLIENTID);
		if (!s.IsEmpty())
		{
			m_sAppID = s;
		}

		s = reg.ReadStr(RK_APPINSTDIR);
		if (!s.IsEmpty())
		{
			m_sAppHome = s;
		}

		s = reg.ReadStr(RK_SERVERIP);
		if (!s.IsEmpty())
		{
			m_sSrvIP = s;
			WideCharToMultiByte(CP_ACP, 0, s, -1, buf, 15, NULL, NULL);
			m_dwSrvIP = ntohl(inet_addr(buf));
		}

		s = reg.ReadStr(RK_LOCALMAC);
		if (!s.IsEmpty())
		{
			m_sLocalMac = s;
		}

		s = reg.ReadStr(RK_APPRELNO);
		if (!s.IsEmpty())
		{
			m_sAppReleaseNo = s;
		}

		s = reg.ReadStr(RK_CONTSRELNO);
		if (!s.IsEmpty())
		{
			m_sContentsReleaseNo = s;
		}

		s = reg.ReadStr(RK_APPCMDLINE);
		if (!s.IsEmpty())
		{
			m_sCmdLine = s;
		}

		reg.Close();
	}
}

void CContentsAgentDlg::WriteRegistry()
{
	CRegistry reg;
	CString s;

	s.Format(_T("%s\\%s"), REGISTRY_PATH, _VIEWER_APP_NAME);
	if (reg.Open(s))
	{
		reg.Write(RK_CLIENTID, m_sAppID);
		reg.Write(RK_APPINSTDIR, m_sAppHome);
		reg.Write(RK_SERVERIP, m_sSrvIP);
		reg.Write(RK_LOCALMAC, m_sLocalMac);
		reg.Write(RK_APPRELNO, m_sAppReleaseNo);
		reg.Write(RK_CONTSRELNO, m_sContentsReleaseNo);
		reg.Write(RK_APPCMDLINE, m_sCmdLine);

		reg.Close();
	}
}

void CContentsAgentDlg::OnBnClickedApply()
{
	UpdateData(TRUE);

	BYTE byIP[4];
	byIP[0] = (BYTE)FIRST_IPADDRESS(m_dwSrvIP);
	byIP[1] = (BYTE)SECOND_IPADDRESS(m_dwSrvIP);
	byIP[2] = (BYTE)THIRD_IPADDRESS(m_dwSrvIP);
	byIP[3] = (BYTE)FOURTH_IPADDRESS(m_dwSrvIP);

	CString s = m_sSrvIP, sLog;
	m_sSrvIP.Format(_T("%d.%d.%d.%d"), byIP[0], byIP[1], byIP[2], byIP[3]);

	WriteRegistry();

	if (m_sSrvIP.Compare(s) != 0)
	{
		sLog.Format(_T("Server-Ip:(%s)->(%s), Register"), s, m_sSrvIP);

#ifndef _HTTP_TEST
		LogRegister(1, 3, sLog);
#endif
	}

	sLog.Format(_T("Server-Ip:%s, exec-dir:%s, exec-cmd:%s"), m_sSrvIP, m_sAppHome, m_sCmdLine);

#ifndef _HTTP_TEST
	LogRegister(1, 2, sLog);
#endif
}

BOOL CContentsAgentDlg::GetIP(CStringArray& strIPArray)
{
	CStringA sIp;

	PIP_ADAPTER_INFO pAdapterInfo;
	PIP_ADAPTER_INFO pAdapter = NULL;
	DWORD dwRetVal = 0;

	ULONG ulOutBufLen = sizeof (IP_ADAPTER_INFO);
	pAdapterInfo = new IP_ADAPTER_INFO[ulOutBufLen];

	if (pAdapterInfo == NULL)
		return FALSE;

	if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
	{
		delete pAdapterInfo;
		pAdapterInfo = new IP_ADAPTER_INFO[ulOutBufLen];
		if (pAdapterInfo == NULL)
			return FALSE;
	}

	if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR)
	{
		pAdapter = pAdapterInfo;
		while (pAdapter)
		{
			sIp.Format("%s", pAdapter->IpAddressList.IpAddress.String);
			strIPArray.Add(CA2T(sIp));
			pAdapter = pAdapter->Next;
		}
	}

	delete pAdapterInfo;
	pAdapterInfo = NULL;
	return TRUE;
}

BOOL CContentsAgentDlg::GetMacAddress(CStringArray& arrMac, CString arrLcIp)
{
	PIP_ADAPTER_INFO pAdapterInfo;
	PIP_ADAPTER_INFO pAdapter = NULL;
	DWORD dwRetVal = 0;

	ULONG ulOutBufLen = sizeof (IP_ADAPTER_INFO);
	pAdapterInfo = new IP_ADAPTER_INFO[ulOutBufLen];

	if (pAdapterInfo == NULL)
		return FALSE;

	if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
	{
		delete pAdapterInfo;
		pAdapterInfo = new IP_ADAPTER_INFO[ulOutBufLen];
		if (pAdapterInfo == NULL)
			return FALSE;
	}

	if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR)
	{
		pAdapter = pAdapterInfo;
		while (pAdapter)
		{
			CStringA strMacAddress;
			strMacAddress.Format("%02X:%02X:%02X:%02X:%02X:%02X",
			pAdapter->Address[0],
			pAdapter->Address[1],
			pAdapter->Address[2],
			pAdapter->Address[3],
			pAdapter->Address[4],
			pAdapter->Address[5]);

			CStringA sIp;
			sIp.Format("%s", pAdapter->IpAddressList.IpAddress.String);

			if(arrLcIp.GetLength() > 0)
			{
				if(sIp.Find(CT2A(arrLcIp)) >= 0)
				{
					arrMac.Add(CA2T(strMacAddress));
					break;
				}
			}
			else
				arrMac.Add(CA2T(strMacAddress));

			pAdapter = pAdapter->Next;
		}
	}

	delete pAdapterInfo;
	pAdapterInfo = NULL;
	return TRUE;
}

void CContentsAgentDlg::OnBnClickedTest()
{
#ifndef _HTTP_TEST
	CString s, sPath;
	BYTE byIP[4];
	CIPAddressCtrl* pIPAC = (CIPAddressCtrl*)GetDlgItem(IDC_SERVER_IP);

	pIPAC->GetAddress(byIP[0], byIP[1], byIP[2], byIP[3]);
	s.Format(_T("%d.%d.%d.%d"), byIP[0], byIP[1], byIP[2], byIP[3]);
	sPath.Format(_T("/%s/cli/commtest.php"), _VIEWER_APP_NAME);

	int nRcv;
	LPTSTR pRcv = HttpPost(s, sPath, NULL, nRcv);
	
	if (pRcv)
	{
		BOOL bOK = (_tcscmp(L"OK", pRcv) == 0);
		delete [] pRcv;
		AfxMessageBox(_T("연결 성공하였습니다"));
	}
	else
	{
		AfxMessageBox(_T("연결 실패하였습니다"));
	}
#else
	
	BOOL b = FALSE;
//	ConfigureUpdate();
//	AppUpdate(b);
//	DataUpdate();
//	RankingUpdate();
//	ShutdownQuery(b);
//	AliveTest();
	if (RebootingQuery(b) && b)
	{
		AfxMessageBox(_T("rebooting"));
	}

#endif
}

BOOL CContentsAgentDlg::AppClose()
{
	CWnd *pWnd = FindWindow(NULL, _VIEWER_APP_TITLE);
	if (pWnd != NULL)
	{
		ULONG pid;
		GetWindowThreadProcessId(pWnd->GetSafeHwnd(), &pid);

		// 리스트윈도우의 프로세서핸들을 구한다.
		HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
		if (hProcess != NULL)
		{
			DWORD ExitCode = 0;
			GetExitCodeProcess(hProcess, &ExitCode);
			if (TerminateProcess(hProcess, ExitCode))
			{
				WaitForSingleObject(hProcess, INFINITE);
			}
			CloseHandle(hProcess);
			return TRUE;
		}
	}
	return FALSE;
}

BOOL CContentsAgentDlg::DataUpdateQuery(BOOL& bUpdate, CString& sReleaseNo, CString& sZip)
{
	CString s, sPath;
	s.Format(_T("&releaseno=%s"), m_sContentsReleaseNo);
	sPath.Format(_T("/%s/cli/dataupdatequery.php"), _VIEWER_APP_NAME);
	
	int nRcv;
	LPTSTR pRcv = HttpPost(m_sSrvIP, sPath, s, nRcv);

	BOOL bOK = FALSE;
	if (pRcv)
	{
		CUnlimitedList<LPTSTR> lines;

		LineSplit(pRcv, &lines);
		lines.SetAutoFree(FALSE);

		LPTSTR pResCode = lines.GetAt(0);
		if (lines.GetCount() >= 3)
		{
			sReleaseNo = lines.GetAt(1);
			sReleaseNo.Trim();
			sZip = lines.GetAt(2);
			sZip.Trim();
		}

		if (_tcscmp(L"OK", pResCode) == 0)
		{
			bUpdate = TRUE;
			bOK = TRUE;
		}
		else
		{
			bUpdate = FALSE;
			bOK = (_tcscmp(L"NOT", pResCode) == 0);
		}

		delete [] pRcv;
	}

	return bOK;
}

BOOL CContentsAgentDlg::AppUpdateQuery(BOOL& bUpdate, CString& sReleaseNo, CString& sZip)
{
	CString s, sPath;
	s.Format(_T("&releaseno=%s"), m_sAppReleaseNo);
	sPath.Format(_T("/%s/cli/appupdatequery.php"), _VIEWER_APP_NAME);

	int nRcv;
	LPTSTR pRcv = HttpPost(m_sSrvIP, sPath, s, nRcv);

	BOOL bOK = FALSE;
	if (pRcv)
	{
		CUnlimitedList<LPTSTR> lines;

		LineSplit(pRcv, &lines);
		lines.SetAutoFree(FALSE);

		LPTSTR pResCode = lines.GetAt(0);
		if (lines.GetCount() >= 3)
		{
			sReleaseNo = lines.GetAt(1);
			sReleaseNo.Trim();
			sZip = lines.GetAt(2);
			sZip.Trim();
		}

		if (_tcscmp(L"OK", pResCode) == 0)
		{
			bUpdate = TRUE;
			bOK = TRUE;
		}
		else
		{
			bUpdate = FALSE;
			bOK = (_tcscmp(L"NOT", pResCode) == 0);
		}

		delete [] pRcv;
	}

	return bOK;
}

BOOL CContentsAgentDlg::ConfigureQuery(CString& sID, CString& sConf)
{
	int nRcv;
	CString sPath;

	sPath.Format(_T("/%s/cli/configurequery.php"), _VIEWER_APP_NAME);

	LPTSTR pRcv = HttpPost(m_sSrvIP, sPath, NULL, nRcv);

	BOOL bOK = FALSE;
	if (pRcv)
	{
		CUnlimitedList<LPTSTR> lines;

		LineSplit(pRcv, &lines);
		lines.SetAutoFree(FALSE);

		LPTSTR pResCode = lines.GetAt(0);

		if (lines.GetCount() >= 3)
		{
			sID = lines.GetAt(1);
			sID.Trim();
			sConf = lines.GetAt(2);
			sConf.Trim();
		}
		bOK = (_tcscmp(L"OK", pResCode) == 0);

		delete [] pRcv;
	}

	return bOK;
}

BOOL CContentsAgentDlg::ShutdownQuery(BOOL& bShutdown)
{
	int nRcv;
	CString sPath;

	sPath.Format(_T("/%s/cli/pcshutdowntimequery.php"), _VIEWER_APP_NAME);

	LPTSTR pRcv = HttpPost(m_sSrvIP, sPath, NULL, nRcv);

	BOOL bOK = FALSE;
	if (pRcv)
	{
		if (_tcscmp(L"OK", pRcv) == 0)
		{
			bShutdown = TRUE;
			bOK = TRUE;
		}
		else
		{
			bShutdown = FALSE;
			bOK = (_tcscmp(L"NOT", pRcv) == 0);
		}

		delete [] pRcv;
	}

	return bOK;
}

void CContentsAgentDlg::LineSplit(LPTSTR pRcv, CUnlimitedList<LPTSTR>* pLines)
{
	TCHAR seps[] = _T("\r\n");
	LPTSTR pNext = NULL;

	for (LPTSTR pTok = _tcstok_s(pRcv, seps, &pNext) ; pTok != NULL ; pTok = _tcstok_s(NULL, seps, &pNext))
	{
		pLines->Add(pTok);
	}
}

BOOL CContentsAgentDlg::EnablePrivilege(HANDLE hToken, LPCTSTR szPrivName, BOOL fEnable)
{ 
	TOKEN_PRIVILEGES tp;
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Attributes = fEnable ? SE_PRIVILEGE_ENABLED : 0;

	LookupPrivilegeValue( 0, szPrivName, &tp.Privileges[0].Luid);
	AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);

	return ((GetLastError() == ERROR_SUCCESS));
}

BOOL CContentsAgentDlg::Shutdown()
{
	HANDLE hToken;
	BOOL bOK = OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken);

	if (bOK)
	{
		if (bOK = EnablePrivilege(hToken, SE_SHUTDOWN_NAME, TRUE))
		{
			ExitWindowsEx(EWX_SHUTDOWN, 0);
		}

		CloseHandle(hToken);
	}

	return bOK;
}

BOOL CContentsAgentDlg::Upzip(LPCTSTR pSrc, LPCTSTR pDst)
{
	CZipArchive zip;

	if (!zip.Open(pSrc))
	{
		return FALSE;
	}

	BOOL bOK = TRUE;
	TCHAR buf[1024];

	for (int i = 0, n = zip.GetCount(); bOK && i < n; i++)
	{
		TRY
		{
			bOK = zip.ExtractFile(i, pDst);
		}
		CATCH(CException, pEx)
		{
			pEx->GetErrorMessage(buf, 1023);
			::OutputDebugString(buf);
			Sleep(100);
			--i;
		}
		END_CATCH
	}

	zip.Close();

	return bOK;
}

BOOL CContentsAgentDlg::Download(LPCTSTR pSrc, LPCTSTR pDst)
{
	HINTERNET hSession = WinHttpOpen(L"Data Agent/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!hSession)
	{
		return FALSE;
	}

	HINTERNET hConnect = WinHttpConnect(hSession, m_sSrvIP, INTERNET_DEFAULT_HTTP_PORT, 0);
	if (!hConnect)
	{
		WinHttpCloseHandle(hSession);
		return FALSE;
	}

	HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", pSrc, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, NULL);
	if (!hRequest)
	{
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		return FALSE;
	}

	BOOL bOK = TRUE;
	BOOL bRet = FALSE;

	if (bOK)
	{
		bOK = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
		if (bOK && WinHttpReceiveResponse(hRequest, NULL))
		{
			DWORD RES_CODE = 0;
			DWORD dwSize = sizeof(DWORD);

			bOK = WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE| WINHTTP_QUERY_FLAG_NUMBER, NULL, &RES_CODE, &dwSize, NULL);
			if (bOK && RES_CODE == HTTP_STATUS_OK)
			{
				CFile f;
				bRet = f.Open(pDst, CFile::modeCreate|CFile::modeWrite, NULL, NULL);
				if (bRet)
				{
					CHAR* pT = NULL;
					DWORD dwT = 0;
					DWORD dwLen;

					do
					{
						dwSize = 0;
						WinHttpQueryDataAvailable(hRequest, &dwSize);

						if (dwSize > 0)
						{
							if (dwSize > dwT)
							{
								if (pT)
								{
									delete [] pT;
								}

								pT = new CHAR[dwSize];
								dwT = dwSize;
							}

							bOK = WinHttpReadData(hRequest, (LPVOID)pT, dwSize, &dwLen);
							if (bOK)
							{
								f.Write(pT, dwLen);
							}
						}
					} while (dwSize > 0);

					f.Flush();
					f.Close();

					if (pT)
					{
						delete [] pT;
					}
				}
			}
		}
	}

	WinHttpCloseHandle(hRequest);
	WinHttpCloseHandle(hConnect);
	WinHttpCloseHandle(hSession);

	return bRet;
}

LPTSTR CContentsAgentDlg::HttpPost(LPCWSTR pSrvIP, LPCWSTR pPath, LPCWSTR pParam2, int& nRecv)
{
	WriteFileLog(_T("HttpPost...0\r\n"));

	HINTERNET hSession = WinHttpOpen(L"Data Agent/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!hSession)
	{
		return NULL;
	}

	HINTERNET hConnect = WinHttpConnect(hSession, pSrvIP, INTERNET_DEFAULT_HTTP_PORT, 0);
	if (!hConnect)
	{
		WinHttpCloseHandle(hSession);
		return NULL;
	}

	HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", pPath, NULL, WINHTTP_NO_REFERER, NULL, NULL);
	if (!hRequest)
	{
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		return NULL;
	}

	LPCWSTR pPostHeader = L"Content-Type: application/x-www-form-urlencoded; charset=utf-8";
	BOOL bOK = WinHttpAddRequestHeaders(hRequest, pPostHeader, -1, WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
	LPSTR pRcv = NULL;

	nRecv = 0;

	if (bOK)
	{
		TCHAR buf[128];
		int n = _stprintf_s(buf, _T("appid=%s&macaddr=%s&cliip=%s"), m_sAppID, m_sLocalMac, m_sLocalIP);

		n = WideCharToMultiByte(CP_ACP, 0, buf, -1, NULL, 0, NULL, NULL);
		if (pParam2)
		{
			n += WideCharToMultiByte(CP_ACP, 0, pParam2, -1, NULL, 0, NULL, NULL);
		}

		CHAR* pT = new CHAR[n + 1];
		int m = WideCharToMultiByte(CP_ACP, 0, buf, -1, pT, n, NULL, NULL);

		if (pParam2)
		{
			WideCharToMultiByte(CP_ACP, 0, pParam2, -1, &pT[m], n - m, NULL, NULL);
			pT[n] = NULL;
		}
		
		bOK = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, -1, pT, n, n, 0);
		delete [] pT;

		if (bOK && WinHttpReceiveResponse(hRequest, NULL))
		{
			DWORD RES_CODE = 0;
			DWORD dwSize = sizeof(DWORD);

			bOK = WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE| WINHTTP_QUERY_FLAG_NUMBER, NULL, &RES_CODE, &dwSize, NULL);
			if (bOK && RES_CODE == HTTP_STATUS_OK)
			{
				CUnlimitedList<PRCV_DATAITEM> list;
				PRCV_DATAITEM pT;
				do
				{
					dwSize = 0;
					WinHttpQueryDataAvailable(hRequest, &dwSize);
					if (dwSize <= 0)
					{
						break;
					}

					pT = new RCV_DATAITEM;
					pT->pData = new CHAR[dwSize + 1];
					pT->dwLen = 0;

					bOK = WinHttpReadData(hRequest, (LPVOID)pT->pData, dwSize, &pT->dwLen);
					if (bOK)
					{
						nRecv += (int)pT->dwLen;
						list.Add(pT);
					}
					else
					{
						delete pT;
					}
				} while (dwSize > 0);

				int i = 0, k = 0, n;
				PRCV_DATAITEM* ppT = list.GetList(&n);

				for (pRcv = new CHAR[nRecv + 1] ; i < n ; i++)
				{
					memcpy_s(&pRcv[k], nRecv - k, ppT[i]->pData, ppT[i]->dwLen);
					k += ppT[i]->dwLen;
					delete [] ppT[i]->pData;
				}

				ASSERT(k == nRecv);
				pRcv[k] = NULL;
			}
		}
	}

	WinHttpCloseHandle(hRequest);
	WinHttpCloseHandle(hConnect);
	WinHttpCloseHandle(hSession);

	WriteFileLog(_T("HttpPost...1\r\n"));

	LPTSTR pT = NULL;
	if (pRcv)
	{
		int n = MultiByteToWideChar(CP_UTF8, 0, pRcv, nRecv, NULL, 0);
		pT = new WCHAR[n + 1];

		MultiByteToWideChar(CP_UTF8, 0, pRcv, nRecv, pT, n);
		pT[n] = L'\0';
		delete [] pRcv;

		nRecv = n;

		if (pT[0] == 65279)
		{
			memmove(&pT[0], &pT[1], sizeof(TCHAR) * nRecv--);
		}
	}

	WriteFileLog(_T("HttpPost...2(%d)\r\n"), nRecv);

	return pT;
}

BOOL CContentsAgentDlg::LogRegister(int nMajor, int nMinor, LPCTSTR pMore)
{
	CString s, sPath, s2;
	s.Format(_T("&major=%d&minor=%d&more=%s"), nMajor, nMinor, pMore ? pMore : _T(""));
	sPath.Format(_T("/%s/cli/logregister.php"), _VIEWER_APP_NAME);

	int nRcv;
	LPTSTR pRcv = HttpPost(m_sSrvIP, sPath, s, nRcv);

	BOOL bOK = FALSE;
	if (pRcv)
	{
		bOK = (_tcscmp(L"OK", pRcv) == 0);
		delete [] pRcv;
	}

	const int nMajors = 4, nMinors = 5;
	const TCHAR* pppName[nMajors][nMinors] = {{_T("PC 종료실행"), _T("설정등록"), _T("서버변경"), _T(""), _T("")},
											{_T("Agent 시작"), _T("Agent 종료"), _T(""), _T(""), _T("")},
											{_T("App 시작"), _T("App 종료"), _T("App ID 변경"), _T("App 프로세스 미존재"), _T("App 다운로드")},
											{_T("데이터 다운로드"), _T("데이터 업데이트"), _T("랭킹데이터 업데이트"), _T(""), _T("")}};
	ASSERT(nMajor > 0 && nMajor <= nMajors);
	ASSERT(nMinor > 0 && nMinor <= nMinors);

	SYSTEMTIME st;
	GetLocalTime(&st);

	if (pMore)
	{
		s.Format(_T("%02d:%02d:%02d > %s, %s\r\n"), st.wHour, st.wMinute, st.wSecond, pppName[nMajor - 1][nMinor - 1], pMore);
	}
	else
	{
		s.Format(_T("%02d:%02d:%02d > %s\r\n"), st.wHour, st.wMinute, st.wSecond, pppName[nMajor - 1][nMinor - 1]);
	}

	LogWrite(s);

	return bOK;
}

BOOL CContentsAgentDlg::WriteConfigureString(LPCTSTR pPath, LPCTSTR sConf)
{
	BOOL bOK = FALSE;
	CString s;
	int n = WideCharToMultiByte(CP_ACP, 0, sConf, -1, NULL, 0, NULL, NULL);
	CHAR* pT = new CHAR[n];

	n = WideCharToMultiByte(CP_ACP, 0, sConf, -1, pT, n, NULL, NULL);
	if (pT[n - 1] == NULL)
	{
		n--;
	}

	s.Format(_T("%s\\%s\\%s"), m_sAppHome, _CONF_FOLDER, pPath);
	
	CFile f;
	if (f.Open(s, CFile::modeCreate|CFile::modeWrite, NULL, NULL))
	{
		f.Write(pT, sizeof(CHAR) * n);
		f.Flush();
		f.Close();
	}

	delete [] pT;

	return bOK;
}


void CContentsAgentDlg::OnDestroy()
{
	CDialogEx::OnDestroy();
#ifndef _HTTP_TEST
	ServiceEnd();
#endif
}

void CContentsAgentDlg::ServiceBegin()
{
	BOOL b;
	// PC 종료 조회
	if (ShutdownQuery(b) && b)
	{
		LogRegister(1, 1, NULL);

#ifndef _NO_PC_SHUTDOWN
		Shutdown();
#endif
		OnCancel();
		return;
	}

	// Agent 시작 로그
	LogRegister(2, 1, NULL);

	CString s, sLog;

	// 환경설정 조회
	BOOL bRegWrite = ConfigureUpdate();

	// App 업데이트 조회
	CString sReleaseNo, sDate, sZip;
	if (AppUpdate(FALSE))
	{
		bRegWrite = TRUE;
	}

	// 데이터 업데이트 조회
	if (DataUpdate())
	{
		bRegWrite = TRUE;
	}
		
#ifdef _KIOSK
	// 랭킹데이터 업데이트 조회
	RankingUpdate();
#endif

	// 레지스트리 기록
	if (bRegWrite)
	{
		WriteRegistry();
	}

	SendMessage(WM_UPDATE_DATA_2, FALSE, 0);

	// App 실행
	CWnd* pWnd = FindWindow(NULL, _VIEWER_APP_TITLE);
	if (pWnd == NULL)
	{
		s.Format(_T("%s\\%s"), m_sAppHome, m_sCmdLine);
#ifndef _NO_APP_EXEC
		ShellExecute(NULL, _T("open"), s, NULL, m_sAppHome, SW_SHOW);
#endif
	}

	if (!ThreadBegin())
	{
		AfxMessageBox(_T("쓰레드를 생성하지 못하였습니다. 프로그램을 종료합니다."));
		CDialog::OnCancel();
	}
}

void CContentsAgentDlg::ServiceEnd()
{
	// Agent 종료 로그
	LogRegister(2, 2, NULL);

	m_bRun = FALSE;
	for (int i = 0 ; i < XTHREADS ; i++)
	{
		if (m_Thrd[i].pThread != NULL)
		{
			m_Thrd[i].pEvent->SetEvent();
		}
	}
}

UINT __cdecl WorkProc(LPVOID pParam)
{
	CoInitialize(NULL);

	((CContentsAgentDlg*)pParam)->OnThreadWorkProc();

	CoUninitialize();

	AfxEndThread(0, FALSE);
	return 0;
}

UINT __cdecl AliveProc(LPVOID pParam)
{
	CoInitialize(NULL);

	((CContentsAgentDlg*)pParam)->OnThreadAliveProc();

	CoUninitialize();

	AfxEndThread(0, FALSE);
	return 0;
}

UINT __cdecl CommProc(LPVOID pParam)
{
	CoInitialize(NULL);

	((CContentsAgentDlg*)pParam)->OnThreadCommProc();

	CoUninitialize();

	AfxEndThread(0, FALSE);
	return 0;
}

BOOL CContentsAgentDlg::ThreadBegin()
{
	static const AFX_THREADPROC ppProc[XTHREADS] = {AliveProc, CommProc, WorkProc};
	PXTHREAD_CONTEXT pCtx;

	m_bRun = TRUE;
	for (int i = 0 ; i < XTHREADS ; i++)
	{
		pCtx = &m_Thrd[i];
		pCtx->pEvent = new CEvent(FALSE, FALSE);
		pCtx->pThread = AfxBeginThread(ppProc[i], this);

		if (pCtx->pThread == NULL)
		{
			m_bRun = FALSE;
			delete pCtx->pEvent;
			pCtx->pEvent = NULL;
			break;
		}
		
		pCtx->pThread->m_bAutoDelete = FALSE;
	}

	return m_bRun;
}

void CContentsAgentDlg::OnThreadWorkProc()
{
	CWnd* pWnd;
	BOOL bRegWrite, bUpdate;
	CString s, sName, sReleaseNo, sDate, sZip, sLog;

	while (m_bRun)
	{
		WaitForSingleObject(m_Thrd[THRD_WORK].pEvent->m_hObject, _QUERY_INTERVAL_DATA * 1000);

		if (!m_bRun)
		{
			break;
		}

		// PC 종료 조회
		if (ShutdownQuery(bUpdate) && bUpdate)
		{
			LogRegister(1, 1, NULL);
#ifndef _NO_PC_SHUTDOWN
			Shutdown();
#endif
			break;
		}

		// PC 리부팅 조회
		if (m_bRun && RebootingQuery(bUpdate) && bUpdate)
		{
			LogRegister(1, 1, NULL);
#ifndef _NO_PC_SHUTDOWN
			Rebooting();
#endif
			break;
		}

		// App 프로세스 조회
		if (m_bRun && (pWnd = FindWindow(NULL, _VIEWER_APP_TITLE)) == NULL)
		{
			LogRegister(3, 4, NULL);
			if (m_bRun)
			{
				// App 실행
				s.Format(_T("%s\\%s"), m_sAppHome, m_sCmdLine);
#ifndef _NO_APP_EXEC
				ShellExecute(NULL, _T("open"), s, NULL, m_sAppHome, SW_SHOW);
#endif
			}
		}

		bRegWrite = FALSE;

		// 환경설정 조회
		if (m_bRun && ConfigureUpdate())
		{
			bRegWrite = TRUE;
		}

		// App 업데이트 조회
		if (m_bRun && AppUpdate(TRUE))
		{
			bRegWrite = TRUE;
		}

		// 데이터 업데이트 조회
		if (m_bRun && DataUpdate())
		{
			bRegWrite = TRUE;
		}
		
#ifdef _KIOSK
		// 랭킹데이터 업데이트 조회
		if (m_bRun && RankingUpdate())
		{
			bRegWrite = TRUE;
		}
#endif

		// 레지스트리 기록
		if (bRegWrite)
		{
			WriteRegistry();
			if (m_bRun && GetSafeHwnd() != NULL)
			{
				SendMessage(WM_UPDATE_DATA_2, FALSE, 0);
			}
		}
	}
}

void CContentsAgentDlg::OnThreadAliveProc()
{
	while (m_bRun)
	{
		WaitForSingleObject(m_Thrd[THRD_ALIVE].pEvent->m_hObject, _QUERY_INTERVAL_ALIVE * 1000);

		if (!m_bRun)
		{
			break;
		}

		// App alive 로그 확인
		if (m_bRun)
		{
			AliveTest();
		}
	}
}

void CContentsAgentDlg::LogWrite(LPCTSTR pMsg)
{
	if (m_Mutex.Lock())
	{
		CString s;

		if (m_sLog.IsEmpty())
		{
			m_sLog = pMsg;
		}
		else
		{
			s.Format(_T("\r\n%s"), pMsg);
			m_sLog += s;
		}

		s = m_sLog;
		m_Mutex.Unlock();

		CEdit* pEdit = (CEdit*)GetDlgItem(IDC_LOG);
		if (pEdit != NULL && pEdit->GetSafeHwnd() != NULL)
		{
			pEdit->SetWindowText(s);
			pEdit->LineScroll(pEdit->GetLineCount());
		}

		WriteFileLog(_T("\r\n%s"), pMsg);
	}
}

void CContentsAgentDlg::WriteFileLog(LPCTSTR pFormat, ...)
{
	if (m_bWriteLogFile)
	{
		TCHAR buf[2049];
		va_list args;

		va_start(args, pFormat);
		_vsntprintf_s(buf, 2048, pFormat, args);
		va_end(args);

		char buf2[2049];
		int n = WideCharToMultiByte(CP_ACP, 0, buf, -1, buf2, 2049, NULL, NULL);

		m_LogFile.Write(buf2, n);
	}
}

void CContentsAgentDlg::OnThreadCommProc()
{
	CString s, s2;
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == INVALID_SOCKET)
	{
		s.Format(_T("RECV -> 실패: 소켓생성(%d)"), WSAGetLastError());
		LogWrite(s);
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
		LogWrite(s);
		return;
	}

	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 200;

	fd_set fds;
	int i, n, nRcv;
	char buf[512];
	WCHAR wbuf[512];

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

				if (*buf == 'S')
				{
					i = MultiByteToWideChar(CP_UTF8, 0, buf + 1, nRcv - 1, NULL, 0);
					MultiByteToWideChar(CP_UTF8, 0, buf + 1, nRcv - 1, wbuf, i);
					wbuf[i] = L'\0';

					s.Format(_T("RECV -> %s"), wbuf);
					LogWrite(s);

					sockaddr.sin_port = htons(_UDP_PORT);
					s.Format(_T("RHi! %s(%s), app:%s, data:%s, alivetext:%s"), m_sLocalIP, m_sLocalMac, m_sAppReleaseNo, m_sContentsReleaseNo, m_sAliveText);
					i = WideCharToMultiByte(CP_ACP, 0, s, -1, buf, 256, NULL, NULL);
					sendto(sock, buf, i, 0, (struct sockaddr*)&sockaddr, n);
					s2.Format(_T("SEND -> %s"), s.GetBuffer(0) + 1);
					LogWrite(s2);
				}
			}	
		}
	}
}

void CContentsAgentDlg::UpdateAgentTitle()
{
	CString s;
	s.Format(_T("%s Agent (%s)"), _VIEWER_APP_TITLE, m_sAppID);
	SetWindowText(s);
}

LRESULT CContentsAgentDlg::OnUpdateData2(WPARAM wParam, LPARAM lParam)
{
	UpdateData(wParam);
	return 0L;
}


BOOL CContentsAgentDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_ESCAPE)
		{
			return TRUE;
		}
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}


BOOL CContentsAgentDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	// Enter Key 부작용 핸들링
	if (wParam == 1)
	{
		return TRUE;
	}

	return CDialogEx::OnCommand(wParam, lParam);
}


void CContentsAgentDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		switch (nID)
		{
			case SC_CLOSE:
			{
#ifndef DEBUG
				if (AfxMessageBox(_T("프로그램을 종료하시면 서비스에 장애가 일어날 수 있습니다. 정말로 종료하시겠습니까?"), MB_YESNO) == IDNO)
				{
					return;
				}
#endif
				break;
			}
		}
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

BOOL CContentsAgentDlg::ConfigureUpdate()
{
	CString sID, sConf;
	if (ConfigureQuery(sID, sConf) && !sConf.IsEmpty())
	{
		WriteConfigureString(_CONF_PATH_CONFIGURE_UPDATE, sConf);

		if (m_sAppID.CompareNoCase(sID) != 0)
		{
			CString sLog;
			sLog.Format(_T("App-Id:(%s)->(%s), Register"), m_sAppID, sID);

			m_sAppID = sID;
			UpdateAgentTitle();
			LogRegister(3, 3, sLog);
			return TRUE;
		}
	}

	return FALSE;
}

BOOL CContentsAgentDlg::AppUpdate(BOOL bExec)
{
	BOOL bUpdate;
	CString s, sReleaseNo, sZip, sLog, sName, sPath;

	WriteFileLog(_T("AppUpdate...0\r\n"));

	if (AppUpdateQuery(bUpdate, sReleaseNo, sZip) && bUpdate && sReleaseNo.CompareNoCase(m_sAppReleaseNo) != 0)
	{
		WriteFileLog(_T("AppUpdate...1\r\n"));
		// 다운로드 unzip
		sName = sZip.Right(sZip.GetLength() - sZip.ReverseFind('/') - 1);
		s.Format(_T("%s\\%s"), m_sAppHome, sName);
		sPath.Format(_T("/%s/%s"), _VIEWER_APP_NAME, sZip);
		if (Download(sPath, s))
		{
			sLog.Format(_T("App-release-no.:%s, OK"), sReleaseNo);
			LogRegister(3, 5, sLog);

			// App 종료
			AppClose();
			LogRegister(3, 2, NULL);
			Sleep(150);
			Upzip(s, m_sAppHome);
			_flushall();
			Sleep(150);
			WriteFileLog(_T("AppUpdate...2\r\n"));
			::DeleteFile(s);

			m_sAppReleaseNo = sReleaseNo;

			if (bExec)
			{
				// App 실행
				LogRegister(3, 1, NULL);
				s.Format(_T("%s\\%s"), m_sAppHome, m_sCmdLine);
#ifndef _NO_APP_EXEC
				ShellExecute(NULL, _T("open"), s, NULL, m_sAppHome, SW_SHOW);
#endif
			}
			WriteFileLog(_T("AppUpdate...3\r\n"));

			return TRUE;
		}
		
		sLog.Format(_T("App-release-no.:%s, Err"), sReleaseNo);
		LogRegister(3, 5, sLog);
	}

	return FALSE;
}

BOOL CContentsAgentDlg::DataUpdate()
{
	BOOL bUpdate;
	CString s, sReleaseNo, sZip, sLog, sName, sPath;

	WriteFileLog(_T("DataUpdate...0\r\n"));
	if (DataUpdateQuery(bUpdate, sReleaseNo, sZip) && bUpdate && sReleaseNo.CompareNoCase(m_sContentsReleaseNo) != 0)
	{
		WriteFileLog(_T("DataUpdate...1\r\n"));
		// 다운로드 unzip
		sName = sZip.Right(sZip.GetLength() - sZip.ReverseFind('/') - 1);
		s.Format(_T("%s\\%s"), m_sAppHome, sName);
		sPath.Format(_T("/%s/%s"), _VIEWER_APP_NAME, sZip);
		if (Download(sPath, s))
		{
			sLog.Format(_T("data-release-no.:%s, OK"), sReleaseNo);
			LogRegister(4, 1, sLog);

			Sleep(150);
			Upzip(s, m_sAppHome);
			_flushall();
			Sleep(150);
			WriteFileLog(_T("DataUpdate...2\r\n"));
			::DeleteFile(s);

			m_sContentsReleaseNo = sReleaseNo;

			// 업데이트 설정
#ifdef _KIOSK
			s.Format(_T("{\"releaseno\":\"%s\", \"ranking\":\"%s\"}"), sReleaseNo, m_sRanking);
			WriteConfigureString(_CONF_PATH_DATA_UPDATE, s);
#else
			s.Format(_T("{\"releaseno\":\"%s\"}"), sReleaseNo);
			WriteConfigureString(_CONF_PATH_DATA_UPDATE, s);
#endif

			sLog.Format(_T("data-release-no.:%s, Register"), sReleaseNo);
			LogRegister(4, 2, sLog);

			return TRUE;
		}
		
		sLog.Format(_T("data-release-no.:%s, Err"), sReleaseNo);
		LogRegister(4, 1, sLog);
	}

	return FALSE;
}

BOOL CContentsAgentDlg::RankingUpdate()
{
	BOOL bUpdate;
	CString s, sReleaseNo, sJson, sLog, sName, sPath;

	WriteFileLog(_T("RankingUpdate...0\r\n"));
	if (RankingUpdateQuery(bUpdate, sReleaseNo, sJson) && bUpdate && sReleaseNo.CompareNoCase(m_sRanking) != 0)
	{
		WriteFileLog(_T("RankingUpdate...1\r\n"));
		// 다운로드 json
		sName = sJson.Right(sJson.GetLength() - sJson.ReverseFind('/') - 1);
		s.Format(_T("%s\\%s\\%s"), m_sAppHome, _CONF_FOLDER, sName);
		sPath.Format(_T("/%s/%s"), _VIEWER_APP_NAME, sJson);
		if (Download(sPath, s))
		{
			m_sRanking = sReleaseNo;

			// 업데이트 설정
#ifdef _KIOSK
			s.Format(_T("{\"releaseno\":\"%s\", \"ranking\":\"%s\"}"), m_sContentsReleaseNo, m_sRanking);
			WriteConfigureString(_CONF_PATH_DATA_UPDATE, s);
#else
			ASSERT(0);
#endif
			sLog.Format(_T("ranking-release-no.:%s, Register"), sReleaseNo);
			LogRegister(4, 3, sLog);

			return TRUE;
		}
		
		sLog.Format(_T("ranking-release-no.:%s, Err"), sReleaseNo);
		LogRegister(4, 3, sLog);
	}

	return FALSE;
}

BOOL CContentsAgentDlg::RankingUpdateQuery(BOOL& bUpdate, CString& sReleaseNo, CString& sJson)
{
	CString s, sPath;
	s.Format(_T("&releaseno=%s"), m_sRanking);
	sPath.Format(_T("/%s/cli/rankingdataupdatequery.php"), _VIEWER_APP_NAME);
	
	int nRcv;
	LPTSTR pRcv = HttpPost(m_sSrvIP, sPath, s, nRcv);

	BOOL bOK = FALSE;
	if (pRcv)
	{
		CUnlimitedList<LPTSTR> lines;

		LineSplit(pRcv, &lines);
		lines.SetAutoFree(FALSE);

		LPTSTR pResCode = lines.GetAt(0);
		if (lines.GetCount() >= 3)
		{
			sReleaseNo = lines.GetAt(1);
			sReleaseNo.Trim();
			sJson = lines.GetAt(2);
			sJson.Trim();
		}

		if (_tcscmp(L"OK", pResCode) == 0)
		{
			bUpdate = TRUE;
			bOK = TRUE;
		}
		else
		{
			bUpdate = FALSE;
			bOK = (_tcscmp(L"NOT", pResCode) == 0);
		}

		delete [] pRcv;
	}

	return bOK;
}

BOOL CContentsAgentDlg::Rebooting()
{
	HANDLE hToken;
	BOOL bOK = OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken);

	if (bOK)
	{
		if (bOK = EnablePrivilege(hToken, SE_SHUTDOWN_NAME, TRUE))
		{
			ExitWindowsEx(EWX_REBOOT, 0);
		}

		CloseHandle(hToken);
	}

	return bOK;
}

BOOL CContentsAgentDlg::RebootingQuery(BOOL& bReboot)
{
	int nRcv;
	CString sPath;

	sPath.Format(_T("/%s/cli/pcrebootingquery.php"), _VIEWER_APP_NAME);

	LPTSTR pRcv = HttpPost(m_sSrvIP, sPath, NULL, nRcv);

	BOOL bOK = FALSE;
	if (pRcv)
	{
		if (_tcscmp(L"OK", pRcv) == 0)
		{
			bReboot = TRUE;
			bOK = TRUE;
		}
		else
		{
			bReboot = FALSE;
			bOK = (_tcscmp(L"NOT", pRcv) == 0);
		}

		delete [] pRcv;
	}

	return bOK;
}

BOOL CContentsAgentDlg::AliveTest()
{
	CString s;
	s.Format(_T("%s\\%s"), m_sAppHome, _ALIVE_FILEPATH);

	CStdioFile f;
	if (f.Open(s, CFile::shareDenyNone|CFile::modeRead, NULL, NULL))
	{
		f.ReadString(s);
		f.Close();

		s.Trim();
		if (!s.IsEmpty())
		{
			if (m_sAliveText.IsEmpty())
			{
				m_sAliveText = s;
			}
			else
			{
				BOOL bSend = m_sAliveText.CompareNoCase(s);
				m_sAliveText = s;

				if (bSend)
				{
					CString sPath;
					s.Format(_T("&alive=%s"), m_sAliveText);
					sPath.Format(_T("/%s/cli/appaliveregister.php"), _VIEWER_APP_NAME);
	
					int nRcv;
					LPTSTR pRcv = HttpPost(m_sSrvIP, sPath, s, nRcv);
					if (pRcv)
					{
						delete [] pRcv;
					}
					m_tAlive = time(NULL);
				}
				else if (time(NULL) - m_tAlive > 30)
				{
					// App 종료
					AppClose();
					LogRegister(3, 2, NULL);
					Sleep(2000);
					// App 실행
					LogRegister(3, 1, NULL);
					s.Format(_T("%s\\%s"), m_sAppHome, m_sCmdLine);
#ifndef _NO_APP_EXEC
					ShellExecute(NULL, _T("open"), s, NULL, m_sAppHome, SW_SHOW);
#endif
					m_tAlive = time(NULL);

				}
				
				return bSend;
			}
		}
	}

	return FALSE;
}


void CContentsAgentDlg::OnTimer(UINT_PTR nIDEvent)
{
	KillTimer(1);
	AliveSend();

	CDialogEx::OnTimer(nIDEvent);
}

void CContentsAgentDlg::AliveSend()
{
	static int nSeq = 1;
	CString s;

	SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == INVALID_SOCKET)
	{
		s.Format(_T("\r\nSEND %d -> 실패: 소켓생성(%d)"), nSeq, WSAGetLastError());
		LogWrite(s);
	}

	SOCKADDR_IN sockaddr;
	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(_UDP_PORT);
	sockaddr.sin_addr.s_addr = htonl(m_dwSrvIP);

	s.Format(_T("CAlive! %s(%s), app:%s, data:%s, alivetext:%s"), m_sLocalIP, m_sLocalMac, m_sAppReleaseNo, m_sContentsReleaseNo, m_sAliveText);

	char buf[256];
	int nMsg = WideCharToMultiByte(CP_ACP, 0, s, -1, buf, 256, NULL, NULL);
	int n = sendto(sock, buf, nMsg, 0, (struct sockaddr*)&sockaddr, sizeof(sockaddr));

	if (n == SOCKET_ERROR)
	{
		s.Format(_T("SEND %d -> 실패: 발송(%d)"), nSeq, WSAGetLastError());
		LogWrite(s);
	}
	else
	{
		s.Format(_T("SEND %d -> 성공(%d/%d)"), nSeq, n, nMsg);
		LogWrite(s);
	}

	::closesocket(sock);

	nSeq++;
	SetTimer(1, _QUERY_INTERVAL_COMM * 1000, NULL);
}
