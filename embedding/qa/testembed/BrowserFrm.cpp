









































































#include "stdafx.h"
#include "TestEmbed.h"
#include "BrowserFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif




IMPLEMENT_DYNAMIC(CBrowserFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CBrowserFrame, CFrameWnd)
	
	ON_WM_CREATE()
	ON_WM_SETFOCUS()
	ON_WM_SIZE()
	ON_WM_CLOSE()
	ON_WM_ACTIVATE()
	
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           
	ID_SEPARATOR,           
};




CBrowserFrame::CBrowserFrame(PRUint32 chromeMask)
{
	
	
	

	m_chromeMask = chromeMask;
}

CBrowserFrame::~CBrowserFrame()
{
}

void CBrowserFrame::OnClose()
{
	CTestEmbedApp *pApp = (CTestEmbedApp *)AfxGetApp();
	pApp->RemoveFrameFromList(this);

	DestroyWindow();
}




int CBrowserFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	
	
	
	m_wndBrowserView.SetBrowserFrame(this);

	
	
	
	
	
	m_wndBrowserView.SetBrowserFrameGlue((PBROWSERFRAMEGLUE)&m_xBrowserFrameGlueObj);

	
	
	
	
	if (!m_wndBrowserView.Create(NULL, NULL, AFX_WS_DEFAULT_VIEW,
		CRect(0, 0, 0, 0), this, AFX_IDW_PANE_FIRST, NULL))
	{
		TRACE0("Failed to create view window\n");
		return -1;
	}

	
	if (!m_wndUrlBar.Create(CBS_DROPDOWN | WS_CHILD, CRect(0, 0, 200, 150), this, ID_URL_BAR))
	{
		TRACE0("Failed to create URL Bar\n");
		return -1;      
	}
    
    
    m_wndUrlBar.LoadMRUList();

	
	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
		| CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      
	}

	
	
	if (!m_wndReBar.Create(this))
	{
		TRACE0("Failed to create ReBar\n");
		return -1;      
	}
	
	
	m_wndReBar.AddBar(&m_wndToolBar);
	m_wndReBar.AddBar(&m_wndUrlBar, "Enter URL:");

	
	
	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      
	}

	
	
	
	
	
	
	
	RECT rc;
	m_wndStatusBar.GetItemRect (1, &rc);
	if (!m_wndProgressBar.Create(WS_CHILD|WS_VISIBLE|PBS_SMOOTH, rc, &m_wndStatusBar, ID_PROG_BAR))
	{
		TRACE0("Failed to create progress bar\n");
		return -1;      
	}

	
	
	


	
	
	
	

	SetupFrameChrome(); 

	return 0;
}

void CBrowserFrame::SetupFrameChrome()
{
	if(m_chromeMask == nsIWebBrowserChrome::CHROME_ALL)
		return;

	if(! (m_chromeMask & nsIWebBrowserChrome::CHROME_MENUBAR) )
		SetMenu(NULL); 

	if(! (m_chromeMask & nsIWebBrowserChrome::CHROME_TOOLBAR) )
		m_wndReBar.ShowWindow(SW_HIDE); 

	if(! (m_chromeMask & nsIWebBrowserChrome::CHROME_STATUSBAR) )
		m_wndStatusBar.ShowWindow(SW_HIDE); 
}

BOOL CBrowserFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;

	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;

	

	if(! (m_chromeMask & nsIWebBrowserChrome::CHROME_TITLEBAR) )
		cs.style &= ~WS_CAPTION; 

	if(! (m_chromeMask & nsIWebBrowserChrome::CHROME_WINDOW_RESIZE) )
	{
		
		cs.style &= ~WS_SIZEBOX;
		cs.style &= ~WS_THICKFRAME;
		cs.style &= ~WS_MINIMIZEBOX;
		cs.style &= ~WS_MAXIMIZEBOX;
	}

	cs.lpszClass = AfxRegisterWndClass(0);

	return TRUE;
}



void CBrowserFrame::OnSetFocus(CWnd* pOldWnd)
{
	
	m_wndBrowserView.SetFocus();
}

BOOL CBrowserFrame::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	
	if (m_wndBrowserView.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		return TRUE;

	
	return CFrameWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}



void CBrowserFrame::OnSize(UINT nType, int cx, int cy) 
{
       CFrameWnd::OnSize(nType, cx, cy);
	
       
       
       RECT rc;
       m_wndStatusBar.GetItemRect(1, &rc);

       
       
       m_wndProgressBar.MoveWindow(&rc);
}

#ifdef _DEBUG
void CBrowserFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CBrowserFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif 


void CBrowserFrame::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized) 
{
	CFrameWnd::OnActivate(nState, pWndOther, bMinimized);
	
    m_wndBrowserView.Activate(nState, pWndOther, bMinimized);
}
