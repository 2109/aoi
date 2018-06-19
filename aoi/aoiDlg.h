
// aoi_testDlg.h : 头文件
//

#pragma once

extern "C" {
#include "aoi.h"
}

#include <map>
#include <vector>

struct TriggerCtx {
	CPoint pos;
	CPoint dest;
	int range;
	struct aoi_object* trigger;
};

// CAoiDlg 对话框
class CAoiDlg : public CDialogEx
{
// 构造
public:
	CAoiDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_AOI_TEST_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

	struct aoi_object* CreateEntity(CPoint& point);
	struct aoi_object* CreateTrigger(CPoint& point,int range);
	void UpdateTrigger();
public:
	struct aoi_context* m_aoi_ctx;
	int m_countor;
	CRect m_rt;
	std::vector<TriggerCtx*> m_trigger_list;
	std::map<int, struct aoi_object*> m_map;
	std::map<int, bool> m_status;
// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};
