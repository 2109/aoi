
// towerDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "tower.h"
#include "towerDlg.h"
#include "afxdialogex.h"

#include <io.h>    
#include <fcntl.h>  
#include <fstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CtowerDlg 对话框



CtowerDlg::CtowerDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CtowerDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CtowerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CtowerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	ON_WM_CLOSE()
	ON_WM_RBUTTONUP()
END_MESSAGE_MAP()


// CtowerDlg 消息处理程序

BOOL CtowerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO:  在此添加额外的初始化代码

	AllocConsole();
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	int hCrt = _open_osfhandle((long)handle, _O_TEXT);
	FILE * hf = _fdopen(hCrt, "w");
	*stdout = *hf;

	m_cell = 5;
	m_entity_radius = 3;
	m_aoi_ctx = create_aoi(1024 * 10, 1000, 1000, m_cell);
	m_countor = 1;

	GetWindowRect(&m_rt);

	for (int i = 0; i < 500; i++)
	{
		CreateEntity();
	}

	for ( int i = 0; i < 100; i++ )
	{
		CreateTrigger();
	}

	SetTimer(1, 50, NULL);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CtowerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void foreach_entity_callback(int uid, int x, int z, void* ud) {
	CtowerDlg* pDlg = (CtowerDlg*)ud;

	CClientDC dc(pDlg);

	std::map<int, int>::iterator it = pDlg->m_entity_status.find(uid);
	if ( it != pDlg->m_entity_status.end() )
	{
		int countor = it->second;

		CBrush brush0(RGB(0, 255, 0));
		CBrush brush1(RGB(255, 0, 0));

		if (countor > 0)
			dc.SelectObject(&brush0);
		else
			dc.SelectObject(&brush1);

		dc.Ellipse(x - pDlg->m_entity_radius, z - pDlg->m_entity_radius, x + pDlg->m_entity_radius, z + pDlg->m_entity_radius);
	}
	else{
		CBrush brush0(RGB(255, 0, 0));
		dc.SelectObject(&brush0);
		dc.Ellipse(x - pDlg->m_entity_radius, z - pDlg->m_entity_radius, x + pDlg->m_entity_radius, z + pDlg->m_entity_radius);
	}
}

void foreach_trigger_callback(int uid, int x, int z, int range, void* ud) {
	CtowerDlg* pDlg = (CtowerDlg*)ud;
	CClientDC dc(pDlg);

	CPen pen1(PS_DOT, 1, RGB(255, 0, 0));

	dc.SelectObject(&pen1);

	dc.Ellipse(x - pDlg->m_entity_radius, z - pDlg->m_entity_radius, x + pDlg->m_entity_radius, z + pDlg->m_entity_radius);

	dc.MoveTo(x - range * pDlg->m_cell, z - range * pDlg->m_cell);
	dc.LineTo(x + range * pDlg->m_cell, z - range * pDlg->m_cell);

	dc.MoveTo(x - range * pDlg->m_cell, z + range * pDlg->m_cell);
	dc.LineTo(x + range * pDlg->m_cell, z + range * pDlg->m_cell);

	dc.MoveTo(x - range * pDlg->m_cell, z - range * pDlg->m_cell);
	dc.LineTo(x - range * pDlg->m_cell, z + range * pDlg->m_cell);

	dc.MoveTo(x + range * pDlg->m_cell, z - range * pDlg->m_cell);
	dc.LineTo(x + range * pDlg->m_cell, z + range * pDlg->m_cell);
}

void CtowerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}

	foreach_entity(m_aoi_ctx, foreach_entity_callback, this);
	foreach_trigger(m_aoi_ctx, foreach_trigger_callback, this);
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CtowerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void OnEntityEnter(int self, int other, void* ud) {
	//printf("entity:%d enter:%d\n", self, other);
	CtowerDlg* pDlg = (CtowerDlg*)ud;
	pDlg->RefEntity(self);
}

void OnEntityLeave(int self, int other, void* ud) {
	//printf("entity:%d leave:%d\n", self, other);
	CtowerDlg* pDlg = (CtowerDlg*)ud;
	pDlg->DeRefEntity(self);
}

void OnTriggerEnter(int self, int other, void* ud) {
	CtowerDlg* pDlg = (CtowerDlg*)ud;
	//printf("trigger:%d enter:%d\n", self, other);
	pDlg->RefEntity(other);
}

void OnTriggerLeave(int self, int other, void* ud) {
	CtowerDlg* pDlg = (CtowerDlg*)ud;
	//printf("trigger:%d leave:%d\n", self, other);
	pDlg->DeRefEntity(other);
}

void CtowerDlg::CreateEntity()
{
	int uid = m_countor++;
	EntityCtx* ctx = new EntityCtx(m_rt);
	ctx->m_id = create_entity(m_aoi_ctx, uid, ctx->m_pos.x, ctx->m_pos.y, OnEntityEnter, ( void* )this);
	m_entity_list.push_back(ctx);
}

void CtowerDlg::CreateTrigger()
{
	int uid = m_countor++;
	TriggerCtx* ctx = new TriggerCtx(m_rt);
	ctx->m_id = create_trigger(m_aoi_ctx, uid, ctx->m_pos.x, ctx->m_pos.y, ctx->m_range, OnTriggerEnter, ( void* )this);
	m_trigger_list.push_back(ctx);
}


void CtowerDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnTimer(nIDEvent);
	UpdateTrigger();
	UpdateEntity();
}

void CtowerDlg::UpdateTrigger()
{
	for (int i = 0; i < m_trigger_list.size(); i++)
	{
		TriggerCtx* ctx = m_trigger_list[i];
		RECT rt;
		rt.left = ctx->m_pos.x - ctx->m_range * m_cell - 10;
		rt.top = ctx->m_pos.y - ctx->m_range * m_cell - 10;
		rt.right = ctx->m_pos.x + ctx->m_range * m_cell + 10;
		rt.bottom = ctx->m_pos.y + ctx->m_range * m_cell + 10;
		InvalidateRect(&rt);
		if (ctx->Update()) {
			
			move_trigger(m_aoi_ctx, ctx->m_id, ctx->m_pos.x, ctx->m_pos.y, OnTriggerEnter, (void*)this, OnTriggerLeave, (void*)this);
		}
	}
}

void CtowerDlg::UpdateEntity()
{
	for ( int i = 0; i < m_entity_list.size(); i++ )
	{
		EntityCtx* ctx = m_entity_list[i];
		RECT rt;
		rt.left = ctx->m_pos.x - m_entity_radius;
		rt.top = ctx->m_pos.y - m_entity_radius;
		rt.right = ctx->m_pos.x + m_entity_radius;
		rt.bottom = ctx->m_pos.y + m_entity_radius;
		InvalidateRect(&rt);
		if (ctx->Update()) {
			
			move_entity(m_aoi_ctx, ctx->m_id, ctx->m_pos.x, ctx->m_pos.y, OnEntityEnter, (void*)this, OnEntityLeave, (void*)this);
		}
	}
}

void CtowerDlg::OnClose()
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnClose();
	release_aoi(m_aoi_ctx);
}


void CtowerDlg::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnRButtonUp(nFlags, point);
}

void CtowerDlg::RefEntity(int uid)
{
	std::map<int, int>::iterator iter = m_entity_status.find(uid);
	if (iter == m_entity_status.end())
	{
		m_entity_status[uid] = 1;
	}
	else {
		int countor = iter->second;
		countor++;
		m_entity_status[uid] = countor;
	}
}

void CtowerDlg::DeRefEntity(int uid)
{
	std::map<int, int>::iterator iter = m_entity_status.find(uid);
	assert(iter != m_entity_status.end());

	int countor = iter->second;
	countor--;
	m_entity_status[uid] = countor;
}