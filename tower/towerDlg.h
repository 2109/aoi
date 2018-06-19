
// towerDlg.h : ͷ�ļ�
//

#pragma once

extern "C" {
#include "aoi.h"
}

#include <vector>
#include <map>

struct TriggerCtx {
	CPoint pos;
	CPoint dest;
	int id;
};
// CtowerDlg �Ի���
class CtowerDlg : public CDialogEx
{
// ����
public:
	CtowerDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_TOWER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��

public:
	struct aoi* m_aoi_ctx;
	int m_countor;
	CRect m_rt;
	std::vector<int> m_entity_list;
	std::vector<TriggerCtx*> m_trigger_list;
	std::map<int, bool> m_status;

	int CreateEntity(CPoint& point);
	int CreateTrigger(CPoint& point, int range);
	void UpdateTrigger();
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
};
