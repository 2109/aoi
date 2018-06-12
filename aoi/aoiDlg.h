
// aoi_testDlg.h : ͷ�ļ�
//

#pragma once

extern "C" {
#include "aoi.h"
}

#include <map>

// CAoiDlg �Ի���
class CAoiDlg : public CDialogEx
{
// ����
public:
	CAoiDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_AOI_TEST_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��

public:
	struct aoi_context* m_aoi_ctx;
	int m_countor;
	struct aoi_object* m_trigger;
	std::map<int, struct aoi_object*> m_map;
// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
};
