
// aoi_testDlg.h : 头文件
//

#pragma once

extern "C" {
#include "aoi.h"
}

#include <map>
#include <vector>

struct AoiCtx {
	CRect m_rt;
	CPoint m_pos;
	CPoint m_dest;
	int m_speed;
	struct aoi_object* m_aoi;

	AoiCtx(CRect rt) {
		m_rt = rt;
		m_speed = rand() % 50 + 50;
		RandomPos();
		RandomDest();
	}

	void RandomPos() {
		m_pos.x = rand() % m_rt.right;
		m_pos.y = rand() % m_rt.bottom;
	}

	void RandomDest() {
		m_dest.x = rand() % m_rt.right;
		m_dest.y = rand() % m_rt.bottom;
	}

	float Distance() {
		float dt = sqrt(( m_dest.x - m_pos.x ) * ( m_dest.x - m_pos.x ) + ( m_dest.y - m_pos.y ) * ( m_dest.y - m_pos.y ));
		return dt;
	}

	bool Update() {
		float dt = this->Distance();
		if ( dt <= 5 ) {
			RandomDest();
			return false;
		}
		else {
			float ratio = ( this->m_speed * 0.1f ) / dt;
			m_pos.x = m_pos.x + ( m_dest.x - m_pos.x ) * ratio;
			m_pos.y = m_pos.y + ( m_dest.y - m_pos.y ) * ratio;
			m_pos.x = m_pos.x < 0 ? 0 : m_pos.x;
			m_pos.y = m_pos.y < 0 ? 0 : m_pos.y;
			return true;
		}
	}
};

struct TriggerCtx : public AoiCtx {
	int m_range;
	TriggerCtx(CRect rt) :AoiCtx(rt) {
		m_range = rand() % 50 + 10;
	}
};

struct EntityCtx : public AoiCtx  {
	EntityCtx(CRect rt) :AoiCtx(rt) {
	}
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
	
public:
	void RefEntity(int uid);
	void DeRefEntity(int uid);
	void CreateEntity();
	void CreateTrigger();
	void UpdateTrigger();
	void UpdateEntity();
public:
	struct aoi_context* m_aoi_ctx;
	int m_countor;
	int m_entity_radius;
	CRect m_rt;
	std::vector<TriggerCtx*> m_trigger_list;
	std::vector<EntityCtx*> m_entity_list;
	std::map<int, int> m_entity_status;

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
