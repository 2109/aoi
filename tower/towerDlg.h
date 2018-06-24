
// towerDlg.h : 头文件
//

#pragma once

extern "C" {
#include "aoi.h"
}

#include <vector>
#include <map>

#include <map>
#include <vector>

struct EntityCtx {
	CPoint pos;
	CPoint dest;
	CRect rt;
	int speed;
	int id;

	EntityCtx(CRect rt_) {
		rt = rt_;
		speed = rand() % 50 + 50;
		RandomPos();
		RandomDest();
	}

	void RandomPos() {
		pos.x = rand() % rt.right;
		pos.y = rand() % rt.bottom;
	}

	void RandomDest() {
		dest.x = rand() % rt.right;
		dest.y = rand() % rt.bottom;
	}

	float Distance() {
		float dt = sqrt((dest.x - pos.x) * (dest.x - pos.x) + (dest.y - pos.y) * (dest.y - pos.y));
		return dt;
	}

	bool Update() {
		float dt = this->Distance();
		if (dt <= 5) {
			RandomDest();
			return false;
		}
		else {
			float ratio = (this->speed * 0.1f) / dt;
			pos.x = pos.x + (dest.x - pos.x) * ratio;
			pos.y = pos.y + (dest.y - pos.y) * ratio;
			pos.x = pos.x < 0 ? 0 : pos.x;
			pos.y = pos.y < 0 ? 0 : pos.y;
			return true;
		}
	}
};

struct TriggerCtx:public EntityCtx {
	int range;
	TriggerCtx(CRect rt_) :EntityCtx(rt_){
	}
};


// CtowerDlg 对话框
class CtowerDlg : public CDialogEx
{
// 构造
public:
	CtowerDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_TOWER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

public:
	struct aoi* m_aoi_ctx;
	int m_countor;
	int m_cell;
	CRect m_rt;
	std::vector<TriggerCtx*> m_trigger_list;
	std::vector<EntityCtx*> m_entity_list;
	std::map<int, int> m_entity_status;

	int CreateEntity(CPoint& point);
	int CreateTrigger(CPoint& point, int range);
	void UpdateTrigger();
	void UpdateEntity();
	void RefEntity(int uid);
	void DeRefEntity(int uid);
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
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
};
