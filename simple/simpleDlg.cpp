
// simpleDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "simple.h"
#include "simpleDlg.h"
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


// CsimpleDlg 对话框



CsimpleDlg::CsimpleDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CsimpleDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CsimpleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CsimpleDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	ON_WM_CLOSE()
END_MESSAGE_MAP()


// CsimpleDlg 消息处理程序

void OnAOIEnter(int self, int other, void* ud) {
	CsimpleDlg* pDlg = (CsimpleDlg*)ud;
	std::map<int, AoiObject*>::iterator iter = pDlg->m_trigger_list.find(self);
	if ( iter != pDlg->m_trigger_list.end())
	{
		pDlg->m_entity_status[other] = true;
	}
	else {
		std::map<int, AoiObject*>::iterator iter = pDlg->m_trigger_list.find(other);
		if ( iter != pDlg->m_trigger_list.end() ) {
			pDlg->m_entity_status[self] = true;
		}
	}
	
}

void OnAOILeave(int self, int other, void* ud) {
	CsimpleDlg* pDlg = (CsimpleDlg*)ud;
	std::map<int, AoiObject*>::iterator iter = pDlg->m_trigger_list.find(self);
	if ( iter != pDlg->m_trigger_list.end() )
	{
		pDlg->m_entity_status[other] = false;
	}
	else {
		std::map<int, AoiObject*>::iterator iter = pDlg->m_trigger_list.find(other);
		if ( iter != pDlg->m_trigger_list.end() ) {
			pDlg->m_entity_status[self] = false;
		}
	}
}

BOOL CsimpleDlg::OnInitDialog()
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

	GetWindowRect(&m_rt);

	m_cell = 5;
	m_range = 20;
	m_aoi_ctx = aoi_create(m_rt.right + 100, m_rt.bottom + 100, m_cell, m_range, 10240, OnAOIEnter, OnAOILeave);
	m_countor = 1;

	for ( int i = 0; i < 500; i++ )
	{
		int id = m_countor++;
		AoiObject* ctx = new AoiObject();
		ctx->vt = rand() % 100 + 50;
		ctx->pos = CPoint(rand() % (m_rt.right - 1), rand() % (m_rt.bottom-1));
		ctx->dest = CPoint(rand() % ( m_rt.right - 1 ), rand() % ( m_rt.bottom - 1 ));
		ctx->id = aoi_enter(m_aoi_ctx, id, ctx->pos.x, ctx->pos.y, 1, ( void* )this);
		m_entity_list[id] = ctx;
	}

	for ( int i = 0; i < 3; i++ )
	{
		int id = m_countor++;
		AoiObject* ctx = new AoiObject();
		ctx->vt = rand() % 50 + 50;
		ctx->pos = CPoint(rand() % ( m_rt.right - 1 ), rand() % ( m_rt.bottom - 1 ));
		ctx->dest = CPoint(rand() % ( m_rt.right - 1 ), rand() % ( m_rt.bottom - 1 ));
		ctx->id = aoi_enter(m_aoi_ctx, id, ctx->pos.x, ctx->pos.y, 1, ( void* )this);
		m_trigger_list[id] = ctx;
	}

	//m_entity_status.clear();

	SetTimer(1, 50, NULL);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CsimpleDlg::OnSysCommand(UINT nID, LPARAM lParam)
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


void ForeachObject(int uid, float x,float z, void* ud) {
	CsimpleDlg* pDlg = (CsimpleDlg*)ud;
	CClientDC dc(pDlg);

	CPen pen1(PS_DOT, 1, RGB(255, 0, 0));
	CPen pen2(PS_SOLID, 1, RGB(255, 0, 0));

	std::map<int, AoiObject*>::iterator iter = pDlg->m_entity_list.find(uid);
	if ( iter != pDlg->m_entity_list.end())
	{
		AoiObject* object = iter->second;
		std::map<int, bool>::iterator it = pDlg->m_entity_status.find(uid);
		if ( it != pDlg->m_entity_status.end() )
		{
			bool val = it->second;

			CBrush brush0(RGB(0, 255, 0));
			CBrush brush1(RGB(255, 0, 0));

			if ( val )
				dc.SelectObject(&brush0);
			else
				dc.SelectObject(&brush1);
			
			dc.Ellipse(x - 5, z - 5, x + 5, z + 5);
		}
		else{
			CBrush brush0(RGB(255, 0, 0));
			dc.SelectObject(&brush0);
			dc.Ellipse(x - 5, z - 5, x + 5, z + 5);
		}
	}
	else {
		iter = pDlg->m_trigger_list.find(uid);
		AoiObject* object = iter->second;

		CPen pen(PS_DOT, 1, RGB(255, 0, 0));
		dc.SelectObject(&pen);

		dc.Ellipse(x - 5, z - 5, x + 5, z + 5);
		
		dc.MoveTo(x - pDlg->m_range * pDlg->m_cell, z - pDlg->m_range * pDlg->m_cell);
		dc.LineTo(x + pDlg->m_range * pDlg->m_cell, z - pDlg->m_range * pDlg->m_cell);

		dc.MoveTo(x - pDlg->m_range * pDlg->m_cell, z + pDlg->m_range * pDlg->m_cell);
		dc.LineTo(x + pDlg->m_range * pDlg->m_cell, z + pDlg->m_range * pDlg->m_cell);

		dc.MoveTo(x - pDlg->m_range * pDlg->m_cell, z - pDlg->m_range * pDlg->m_cell);
		dc.LineTo(x - pDlg->m_range * pDlg->m_cell, z + pDlg->m_range * pDlg->m_cell);

		dc.MoveTo(x + pDlg->m_range * pDlg->m_cell, z - pDlg->m_range * pDlg->m_cell);
		dc.LineTo(x + pDlg->m_range * pDlg->m_cell, z + pDlg->m_range * pDlg->m_cell);

	}
}

void CsimpleDlg::OnPaint()
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

	forearch_object(m_aoi_ctx, ForeachObject, ( void* )this);
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CsimpleDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CsimpleDlg::UpdateTrigger()
{
	std::map<int, AoiObject*>::iterator iter = m_trigger_list.begin();
	for ( ; iter != m_trigger_list.end(); iter++ )
	{
		AoiObject* ctx = iter->second;
		float dt = sqrt(( ctx->dest.x - ctx->pos.x ) * ( ctx->dest.x - ctx->pos.x ) + ( ctx->dest.y - ctx->pos.y ) * ( ctx->dest.y - ctx->pos.y ));
		if ( dt <= 5 )
		{
			ctx->dest = CPoint(rand() % m_rt.right, rand() % m_rt.bottom);
		}
		else {
			float vt = ctx->vt;
			float ratio = ( vt * 0.1f ) / dt;
			ctx->pos.x = ctx->pos.x + ( ctx->dest.x - ctx->pos.x ) * ratio;
			ctx->pos.y = ctx->pos.y + ( ctx->dest.y - ctx->pos.y ) * ratio;
			ctx->pos.x = ctx->pos.x <= 0 ? 1 : ctx->pos.x;
			ctx->pos.y = ctx->pos.y <= 0 ? 1 : ctx->pos.y;

			RECT rt;
			rt.left = ctx->pos.x - 300;
			rt.top = ctx->pos.y - 300;
			rt.right = ctx->pos.x + 300;
			rt.bottom = ctx->pos.y + 300;
			InvalidateRect(&rt);

			aoi_update(m_aoi_ctx, ctx->id, ctx->pos.x, ctx->pos.y, ( void* )this);
		}
	}
}

void CsimpleDlg::UpdateEntity()
{
	std::map<int, AoiObject*>::iterator iter = m_entity_list.begin();
	for ( ; iter != m_entity_list.end(); iter++ )
	{
		AoiObject* ctx = iter->second;
		float dt = sqrt(( ctx->dest.x - ctx->pos.x ) * ( ctx->dest.x - ctx->pos.x ) + ( ctx->dest.y - ctx->pos.y ) * ( ctx->dest.y - ctx->pos.y ));
		if ( dt <= 10 )
		{
			ctx->dest = CPoint(rand() % m_rt.right, rand() % m_rt.bottom);
		}
		else {
			float vt = ctx->vt;
			float ratio = ( vt * 0.1f ) / dt;
			ctx->pos.x = ctx->pos.x + ( ctx->dest.x - ctx->pos.x ) * ratio;
			ctx->pos.y = ctx->pos.y + ( ctx->dest.y - ctx->pos.y ) * ratio;
			ctx->pos.x = ctx->pos.x <= 0 ? 1 : ctx->pos.x;
			ctx->pos.y = ctx->pos.y <= 0 ? 1 : ctx->pos.y;

			RECT rt;
			rt.left = ctx->pos.x - 300;
			rt.top = ctx->pos.y - 300;
			rt.right = ctx->pos.x + 300;
			rt.bottom = ctx->pos.y + 300;
			InvalidateRect(&rt);

			aoi_update(m_aoi_ctx, ctx->id, ctx->pos.x, ctx->pos.y, ( void* )this);
		}
	}
}

void CsimpleDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnTimer(nIDEvent);

	UpdateTrigger();
	UpdateEntity();
}


void CsimpleDlg::OnClose()
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnClose();
	aoi_release(m_aoi_ctx);
}
