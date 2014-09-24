
// AgentMonitorDlg.h : 헤더 파일
//

#pragma once


// CAgentMonitorDlg 대화 상자
class CAgentMonitorDlg : public CDialogEx
{
// 생성입니다.
public:
	CAgentMonitorDlg(CWnd* pParent = NULL);	// 표준 생성자입니다.
	virtual ~CAgentMonitorDlg();

// 대화 상자 데이터입니다.
	enum { IDD = IDD_AGENTMONITOR_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.


// 구현입니다.
protected:
	HICON m_hIcon;

protected:
	BOOL m_bRun;
	CWinThread* m_pThread;
	CEvent* m_pEvent;
	UINT m_nTimer;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnDestroy();
	DECLARE_MESSAGE_MAP()

protected:
	void BroadcastSend();
	void BroadcastMessageWrite(LPCTSTR pMsg);
	void ReportMessageWrite(LPCTSTR pMsg);

public:
	void OnThreadWorkProc();
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedButton3();
};
