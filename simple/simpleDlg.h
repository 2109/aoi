
// simpleDlg.h : 头文件
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


// CsimpleDlg 对话框
class CsimpleDlg : public CDialogEx
{
// 构造
public:
	CsimpleDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_SIMPLE_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

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
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnClose();
};
