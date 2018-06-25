
// simpleDlg.h : 头文件
//

#pragma once

#include <map>
#include <vector>
extern "C" {
#include "aoi.h"
}


struct AoiObject {
	CRect m_rt;
	CPoint m_pos;
	CPoint m_dest;
	int m_speed;
	int m_id;
	AoiObject(CRect rt) {
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


struct AoiProfiler {
	LARGE_INTEGER m_freq;
	double m_entity_cost;
	double m_trigger_cost;
	int m_invoke_entity_count;
	int m_invoke_trigger_count;

	AoiProfiler() {
		QueryPerformanceFrequency(&m_freq);
		m_entity_cost = 0;
		m_invoke_entity_count = 0;
		m_trigger_cost = 0;
		m_invoke_trigger_count = 0;
	}

	double GetEntityCost() {
		return m_entity_cost / m_invoke_entity_count;
	}

	double GetTriggerCost() {
		return m_trigger_cost / m_invoke_trigger_count;
	}
};

struct EntityProfiler {
	AoiProfiler* m_profiler;
	LARGE_INTEGER m_begin;
	EntityProfiler(AoiProfiler* profiler) {
		m_profiler = profiler;
		QueryPerformanceCounter(&m_begin);
	}

	~EntityProfiler() {
		LARGE_INTEGER over;
		QueryPerformanceCounter(&over);

		double cost = (double)( ( over.QuadPart - m_begin.QuadPart ) * 1000 ) / (double)m_profiler->m_freq.QuadPart;
		m_profiler->m_invoke_entity_count++;
		m_profiler->m_entity_cost += cost;
	}

};

struct TriggerProfiler {
	AoiProfiler* m_profiler;
	LARGE_INTEGER m_begin;
	TriggerProfiler(AoiProfiler* profiler) {
		m_profiler = profiler;
		QueryPerformanceCounter(&m_begin);
	}

	~TriggerProfiler() {
		LARGE_INTEGER over;
		QueryPerformanceCounter(&over);

		double cost = (double)( ( over.QuadPart - m_begin.QuadPart ) * 1000 ) / (double)m_profiler->m_freq.QuadPart;
		m_profiler->m_invoke_trigger_count++;
		m_profiler->m_trigger_cost += cost;
	}
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
	int m_entity_radius;
	CStatic* m_entity_static;
	CStatic* m_trigger_static;
	AoiProfiler* m_profiler;
	CRect m_rt;
	std::map<int, AoiObject*> m_trigger_list;
	std::map<int, AoiObject*> m_entity_list;

	std::map<int, int> m_entity_status;
	std::map<int, bool> m_trigger_status;

	void CreateEntity();
	void CreateTrigger();

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
};
