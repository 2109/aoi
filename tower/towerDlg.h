
// towerDlg.h : ͷ�ļ�
//

#pragma once

extern "C" {
#include "aoi.h"
}

#include <vector>
#include <map>

#include <map>
#include <vector>

struct TriggerCtx {
	CPoint pos;
	CPoint dest;
	int range;
	int id;
};

struct EntityCtx {
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
	std::vector<TriggerCtx*> m_trigger_list;
	std::vector<EntityCtx*> m_entity_list;
	std::map<int, struct aoi_object*> m_map;
	std::map<int, bool> m_entity_status;
	std::map<int, bool> m_trigger_status;
	int m_timer_countor;


	int CreateEntity(CPoint& point);
	int CreateTrigger(CPoint& point, int range);
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
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
};
