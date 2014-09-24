
// AgentMonitorDlg.h : ��� ����
//

#pragma once


// CAgentMonitorDlg ��ȭ ����
class CAgentMonitorDlg : public CDialogEx
{
// �����Դϴ�.
public:
	CAgentMonitorDlg(CWnd* pParent = NULL);	// ǥ�� �������Դϴ�.
	virtual ~CAgentMonitorDlg();

// ��ȭ ���� �������Դϴ�.
	enum { IDD = IDD_AGENTMONITOR_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV �����Դϴ�.


// �����Դϴ�.
protected:
	HICON m_hIcon;

protected:
	BOOL m_bRun;
	CWinThread* m_pThread;
	CEvent* m_pEvent;
	UINT m_nTimer;

	// ������ �޽��� �� �Լ�
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
