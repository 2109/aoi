
// aoi_testDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "aoiApp.h"
#include "aoiDlg.h"
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


// CAoiDlg 对话框



CAoiDlg::CAoiDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CAoiDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CAoiDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAoiDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONUP()
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CAoiDlg 消息处理程序




BOOL CAoiDlg::OnInitDialog()
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

	m_aoi_ctx = create_aoi_ctx();
	m_countor = 1;
	m_trigger = NULL;
	m_entity = NULL;

	CRect rt;
	GetWindowRect(&rt);

	for (int i = 0; i < 500;i++)
	{
		CPoint pt(rand() % rt.right, rand() % rt.bottom);
		CreateEntity(pt);
	}
	
	for (int i = 0; i < 100;i++)
	{
		TriggerCtx* ctx = new TriggerCtx();
		ctx->pos = CPoint(rand() % rt.right, rand() % rt.bottom);
		ctx->dest = CPoint(rand() % rt.right, rand() % rt.bottom);
		ctx->trigger = CreateTrigger(ctx->pos, rand() % 30 + 20);
		m_trigger_list.push_back(ctx);
	}
	

	SetTimer(1, 50, NULL);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CAoiDlg::OnSysCommand(UINT nID, LPARAM lParam)
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
void foreach_entity_callback(int uid, int x, int z, void* ud) {
	CAoiDlg* pDlg = (CAoiDlg*)ud;

	CClientDC dc(pDlg);

	std::map<int, bool>::iterator it = pDlg->m_status.find(uid);
	if (it != pDlg->m_status.end())
	{
		bool val = it->second;

		CBrush brush0(RGB(0, 255, 0));
		CBrush brush1(RGB(255, 0, 0));

		if (val)
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

void foreach_trigger_callback(int uid, int x, int z, int range, void* ud) {
	CClientDC* dc = (CClientDC*)ud;
	dc->Ellipse(x - 5, z - 5, x + 5, z + 5);

	dc->MoveTo(x - range, z - range);
	dc->LineTo(x + range, z - range);

	dc->MoveTo(x - range, z + range);
	dc->LineTo(x + range, z + range);

	dc->MoveTo(x - range, z - range);
	dc->LineTo(x - range, z + range);

	dc->MoveTo(x + range, z - range);
	dc->LineTo(x + range, z + range);
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CAoiDlg::OnPaint()
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

	CClientDC dc(this);

	CBrush brush0(RGB(255, 0, 0));
	dc.SelectObject(&brush0);
	foreach_aoi_entity(m_aoi_ctx, foreach_entity_callback, this);

	CBrush brush1(RGB(0, 0, 0));
	dc.SelectObject(&brush1);
	foreach_aoi_trigger(m_aoi_ctx, foreach_trigger_callback, &dc);
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CAoiDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void OnEntityCallback(int self, int other, int op, void* ud) {
	if (op == 1)
	{
		printf("entity:%d enter:%d\n", self, other);
	}
	else {
		printf("entity:%d leave:%d\n", self, other);
	}

}

void OnTriggerCallback(int self, int other, int op, void* ud) {
	CAoiDlg* pDlg = (CAoiDlg*)ud;
	if (op == 1)
	{
		printf("trigger:%d enter:%d\n", self, other);
		pDlg->m_status[other] = true;
	}
	else {
		printf("trigger:%d leave:%d\n", self, other);
		pDlg->m_status[other] = false;
	}
}

struct aoi_object* CAoiDlg::CreateEntity(CPoint& point)
{
	int id = m_countor++;
	struct aoi_object* object = create_aoi_object(m_aoi_ctx, id);
	create_entity(m_aoi_ctx, object, point.x, point.y, OnEntityCallback, (void*)this);
	m_map[id] = object;
	return object;
}

struct aoi_object* CAoiDlg::CreateTrigger(CPoint& point,int range)
{
	int id = m_countor++;
	struct aoi_object* object = create_aoi_object(m_aoi_ctx, id);
	create_trigger(m_aoi_ctx, object, point.x, point.y, range, OnTriggerCallback, (void*)this);
	m_map[id] = object;
	return object;
}

void CAoiDlg::UpdateTrigger()
{
	for (int i = 0; i < m_trigger_list.size();i++)
	{
		TriggerCtx* ctx = m_trigger_list[i];
		float dt = sqrt((ctx->dest.x - ctx->pos.x) * (ctx->dest.x - ctx->pos.x) + (ctx->dest.y - ctx->pos.y) * (ctx->dest.y - ctx->pos.y));
		if (dt <= 5)
		{
			CRect rt;
			GetWindowRect(&rt);
			ctx->dest = CPoint(rand() % rt.right, rand() % rt.bottom);
		}
		else {
			float vt = 50;
			float ratio = (vt * 0.1f) / dt;
			ctx->pos.x = ctx->pos.x + (ctx->dest.x - ctx->pos.x) * ratio;
			ctx->pos.y = ctx->pos.y + (ctx->dest.y - ctx->pos.y) * ratio;

			move_trigger(m_aoi_ctx, ctx->trigger, ctx->pos.x, ctx->pos.y);

			Invalidate();
		}
	}
}

void CAoiDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnLButtonUp(nFlags, point);
	if (m_entity == NULL) {
		struct aoi_object* object = CreateEntity(point);
		//m_entity = object;
	}
	else {
		move_entity(m_aoi_ctx, m_entity, point.x, point.y);
	}
	
	Invalidate();
}


void CAoiDlg::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnRButtonUp(nFlags, point);
	if (m_trigger == NULL)
	{
		struct aoi_object* object = CreateTrigger(point,rand() % 30 + 20);
		//m_trigger = object;
	}
	else {
		move_trigger(m_aoi_ctx, m_trigger, point.x, point.y);
	}
	
	Invalidate();
}


void CAoiDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnTimer(nIDEvent);
	
	UpdateTrigger();
}
