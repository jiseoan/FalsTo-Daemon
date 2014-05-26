#include "UnlimitedList.h"

// ContentsAgentDlg.h : 헤더 파일
//

#pragma once

//#define _DEBUG_FILELOG

typedef struct _XTHREAD_CONTEXT
{
	CWinThread* pThread;
	CEvent* pEvent;
} XTHREAD_CONTEXT, *PXTHREAD_CONTEXT;

typedef enum _XTHREAD_TYPE
{
	THRD_ALIVE = 0,
	THRD_COMM,
	THRD_WORK
} XTHREAD_TYPE, *PXTHREAD_TYPE;

#define XTHREADS			(THRD_WORK + 1)



// CContentsAgentDlg 대화 상자
class CContentsAgentDlg : public CDialogEx
{
// 생성입니다.
public:
	CContentsAgentDlg(CWnd* pParent = NULL);	// 표준 생성자입니다.
	virtual ~CContentsAgentDlg();

// 대화 상자 데이터입니다.
	enum { IDD = IDD_CONTENTSAGENT_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.

protected:
	BOOL m_bRun;
	XTHREAD_CONTEXT m_Thrd[XTHREADS];
	XTHREAD_CONTEXT m_TrdAlive;
	XTHREAD_CONTEXT m_TrdComm;
	CMutex m_Mutex;

protected:
	DWORD m_dwSrvIP;
	CString m_sAppHome;
	DWORD m_dwLocalIp;
	CString m_sLocalIP;
	CString m_sLocalMac;
	CString m_sSrvIP;
	CString m_sAppID;
	CString m_sAppReleaseNo;
	CString m_sContentsReleaseNo;
	CString m_sCmdLine;
	CString m_sLog;
	CString m_sRanking;
	CString m_sAliveText;
	time_t m_tAlive;

// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBnClickedApply();
	afx_msg void OnBnClickedTest();
	afx_msg void OnDestroy();
	afx_msg LRESULT OnUpdateData2(WPARAM wParam, LPARAM lParam);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

protected:
	void LogWrite(LPCTSTR pMsg);
	void InitData();
	void ReadRegistry();
	void WriteRegistry();
	BOOL GetIP(CStringArray& strIPArray);
	BOOL GetMacAddress(CStringArray& arrMac, CString arrLcIp);
	BOOL EnablePrivilege(HANDLE hToken, LPCTSTR szPrivName, BOOL fEnable);
	BOOL Shutdown();
	BOOL Download(LPCTSTR pSrc, LPCTSTR pDst);
	BOOL Upzip(LPCTSTR pSrc, LPCTSTR pDst);
	LPTSTR HttpPost(LPCWSTR pSrvIP, LPCWSTR pPath, LPCWSTR pParam2, int& nRecv);
	BOOL WriteConfigureString(LPCTSTR pPath, LPCTSTR sConf);
	BOOL LogRegister(int nMajor, int nMinor, LPCTSTR pMore);
	BOOL ShutdownQuery(BOOL& bShutdown);
	void LineSplit(LPTSTR pRcv, CUnlimitedList<LPTSTR>* pLines);
	BOOL ConfigureQuery(CString& sID, CString& sConf);
	BOOL AppUpdateQuery(BOOL& bUpdate, CString& sReleaseNo, CString& sZip);
	BOOL DataUpdateQuery(BOOL& bUpdate, CString& sReleaseNo, CString& sZip);
	BOOL AppClose();
	void ServiceBegin();
	void ServiceEnd();
	BOOL ThreadBegin();
	void UpdateAgentTitle();
	BOOL ConfigureUpdate();
	BOOL AppUpdate(BOOL bExec);
	BOOL DataUpdate();
	BOOL RankingUpdate();
	BOOL RankingUpdateQuery(BOOL& bUpdate, CString& sReleaseNo, CString& sJson);
	BOOL Rebooting();
	BOOL RebootingQuery(BOOL& bReboot);
	BOOL AliveTest();
	void AliveSend();
#ifdef _DEBUG_FILELOG
	void WriteFileLog(LPCTSTR pLog);
#endif

public:
	void OnThreadWorkProc();
	void OnThreadAliveProc();
	void OnThreadCommProc();
};
