
// aoi_testDlg.cpp : ʵ���ļ�
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


// CAoiDlg �Ի���



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
END_MESSAGE_MAP()


// CAoiDlg ��Ϣ�������

BOOL CAoiDlg::OnInitDialog()
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

	m_aoi_ctx = create_aoi_ctx();
	m_countor = 1;

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
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
	CClientDC* dc = (CClientDC*)ud;
	dc->Ellipse(x - 5, z - 5, x + 5, z + 5);
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

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ  ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CAoiDlg::OnPaint()
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

	CClientDC dc(this);

	CBrush brush0(RGB(255, 0, 0));
	CBrush* obrush = dc.SelectObject(&brush0);
	foreach_aoi_entity(m_aoi_ctx, foreach_entity_callback, &dc);

	CBrush brush1(RGB(0, 0, 0));
	dc.SelectObject(&brush1);
	foreach_aoi_trigger(m_aoi_ctx, foreach_trigger_callback, &dc);
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
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
	if ( op == 1 )
	{
		printf("trigger:%d enter:%d\n", self, other);
	}
	else {
		printf("trigger:%d leave:%d\n", self, other);
	}
}

void CAoiDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO:  �ڴ������Ϣ�����������/�����Ĭ��ֵ

	CDialogEx::OnLButtonUp(nFlags, point);
	int id = m_countor++;
	struct aoi_object* object = create_aoi_object(m_aoi_ctx, id);
	create_entity(m_aoi_ctx, object, point.x, point.y, OnEntityCallback, (void*)this);
	m_map[id] = object;
	Invalidate();
}


void CAoiDlg::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO:  �ڴ������Ϣ�����������/�����Ĭ��ֵ

	CDialogEx::OnRButtonUp(nFlags, point);
	int id = m_countor++;
	struct aoi_object* object = create_aoi_object(m_aoi_ctx, id);
	create_trigger(m_aoi_ctx, object, point.x, point.y, 50, OnTriggerCallback, ( void* )this);
	m_map[id] = object;
	Invalidate();
}
