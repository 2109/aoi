
// simpleDlg.h : ͷ�ļ�
//

#pragma once

#include <map>
#include <vector>
extern "C" {
#include "aoi.h"
}


struct AoiObject {
	CPoint pos;
	CPoint dest;
	int id;
};


// CsimpleDlg �Ի���
class CsimpleDlg : public CDialogEx
{
// ����
public:
	CsimpleDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_SIMPLE_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��

public:
	struct aoi_context* m_aoi_ctx;
	int m_countor;
	int m_cell;
	int m_range;
	CRect m_rt;
	std::map<int, AoiObject*> m_trigger_list;
	std::map<int, AoiObject*> m_entity_list;

	std::map<int, bool> m_entity_status;
	std::map<int, bool> m_trigger_status;

	void UpdateTrigger();
	void UpdateEntity();

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
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnClose();
};
