
// simpleDlg.cpp : ʵ���ļ�
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


// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
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


// CsimpleDlg �Ի���



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


// CsimpleDlg ��Ϣ�������

void OnAOIEnter(int self, int other, void* ud) {
	CsimpleDlg* pDlg = (CsimpleDlg*)ud;
	std::map<int, AoiObject*>::iterator iter = pDlg->m_trigger_list.find(self);
	if ( iter != pDlg->m_trigger_list.end())
	{
		pDlg->RefEntity(other);
	}
	else {
		std::map<int, AoiObject*>::iterator iter = pDlg->m_trigger_list.find(other);
		if ( iter != pDlg->m_trigger_list.end() ) {
			pDlg->RefEntity(self);
		}
	}
}

void OnAOILeave(int self, int other, void* ud) {
	CsimpleDlg* pDlg = (CsimpleDlg*)ud;
	std::map<int, AoiObject*>::iterator iter = pDlg->m_trigger_list.find(self);
	if ( iter != pDlg->m_trigger_list.end() )
	{
		pDlg->DeRefEntity(other);
	}
	else {
		std::map<int, AoiObject*>::iterator iter = pDlg->m_trigger_list.find(other);
		if ( iter != pDlg->m_trigger_list.end() ) {
			pDlg->DeRefEntity(self);
		}
	}
}

BOOL CsimpleDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
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

	// ���ô˶Ի����ͼ�ꡣ  ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO:  �ڴ���Ӷ���ĳ�ʼ������

	AllocConsole();
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	int hCrt = _open_osfhandle((long)handle, _O_TEXT);
	FILE * hf = _fdopen(hCrt, "w");
	*stdout = *hf;

	GetWindowRect(&m_rt);
	m_profiler = new AoiProfiler();
	m_entity_radius = 3;
	m_cell = 10;
	m_range = 5;
	m_aoi_ctx = aoi_create(m_rt.right + 100, m_rt.bottom + 100, m_cell, m_range, 64, OnAOIEnter, OnAOILeave);
	m_countor = 1;

	for ( int i = 0; i < 1000; i++ )
		CreateEntity();

	for ( int i = 0; i < 100; i++ )
		CreateTrigger();

	m_entity_static = new CStatic();
	m_entity_static->Create(_T(""), WS_CHILD | WS_VISIBLE | SS_LEFT, CRect(0, 20, 300, 100), this);

	m_trigger_static = new CStatic();
	m_trigger_static->Create(_T(""), WS_CHILD | WS_VISIBLE | SS_LEFT, CRect(0, 50, 300, 100), this);

	SetTimer(1, 50, NULL);

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
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

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ  ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�


void ForeachObject(int uid, float x,float z, void* ud) {
	CsimpleDlg* pDlg = (CsimpleDlg*)ud;
	CClientDC dc(pDlg);

	CPen pen1(PS_DOT, 1, RGB(255, 0, 0));
	CPen pen2(PS_SOLID, 1, RGB(255, 0, 0));

	std::map<int, AoiObject*>::iterator iter = pDlg->m_entity_list.find(uid);
	if ( iter != pDlg->m_entity_list.end())
	{
		AoiObject* object = iter->second;
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
	else {
		iter = pDlg->m_trigger_list.find(uid);
		AoiObject* object = iter->second;

		CPen pen(PS_DOT, 1, RGB(255, 0, 0));
		dc.SelectObject(&pen);

		dc.Ellipse(x - pDlg->m_entity_radius, z - pDlg->m_entity_radius, x + pDlg->m_entity_radius, z + pDlg->m_entity_radius);
		
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
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}

	forearch_object(m_aoi_ctx, ForeachObject, ( void* )this);

	CString str;
	str.Format(_T("ʵ���ʱ:%fms"), m_profiler->GetEntityCost());
	m_entity_static->SetWindowText(str);

	str;
	str.Format(_T("��������ʱ:%fms"), m_profiler->GetTriggerCost());
	m_trigger_static->SetWindowText(str);
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CsimpleDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CsimpleDlg::UpdateTrigger()
{
	std::map<int, AoiObject*>::iterator iter = m_trigger_list.begin();
	for ( ; iter != m_trigger_list.end(); iter++ )
	{
		EntityProfiler helper(m_profiler);
		AoiObject* ctx = iter->second;
		if ( ctx->Update() ) {
			aoi_update(m_aoi_ctx, ctx->m_id, ctx->m_pos.x, ctx->m_pos.y, ( void* )this);
		}
	}
}

void CsimpleDlg::UpdateEntity()
{
	std::map<int, AoiObject*>::iterator iter = m_entity_list.begin();
	for ( ; iter != m_entity_list.end(); iter++ )
	{
		TriggerProfiler helper(m_profiler);
		AoiObject* ctx = iter->second;
		if ( ctx->Update() ) {
			aoi_update(m_aoi_ctx, ctx->m_id, ctx->m_pos.x, ctx->m_pos.y, ( void* )this);
		}
	}
}

void CsimpleDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO:  �ڴ������Ϣ�����������/�����Ĭ��ֵ

	CDialogEx::OnTimer(nIDEvent);

	UpdateTrigger();
	UpdateEntity();
	Invalidate();
}


void CsimpleDlg::OnClose()
{
	// TODO:  �ڴ������Ϣ�����������/�����Ĭ��ֵ

	CDialogEx::OnClose();
	aoi_release(m_aoi_ctx);
}

void CsimpleDlg::RefEntity(int uid)
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

void CsimpleDlg::DeRefEntity(int uid)
{
	std::map<int, int>::iterator iter = m_entity_status.find(uid);
	assert(iter != m_entity_status.end());

	int countor = iter->second;
	countor--;
	m_entity_status[uid] = countor;
}

void CsimpleDlg::CreateEntity()
{
	int id = m_countor++;
	AoiObject* ctx = new AoiObject(m_rt);
	m_entity_list[id] = ctx;
	ctx->m_id = aoi_enter(m_aoi_ctx, id, ctx->m_pos.x, ctx->m_pos.y, LAYER_ITEM, ( void* )this);
}

void CsimpleDlg::CreateTrigger()
{
	int id = m_countor++;
	AoiObject* ctx = new AoiObject(m_rt);
	m_trigger_list[id] = ctx;
	ctx->m_id = aoi_enter(m_aoi_ctx, id, ctx->m_pos.x, ctx->m_pos.y, LAYER_USER, ( void* )this);
}