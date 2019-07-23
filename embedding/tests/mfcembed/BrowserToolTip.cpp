






























#include "stdafx.h"

#include "BrowserToolTip.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif




CBrowserToolTip::CBrowserToolTip()
{
}

CBrowserToolTip::~CBrowserToolTip()
{
}


BEGIN_MESSAGE_MAP(CBrowserToolTip, CWnd)
	
	ON_WM_PAINT()
	
END_MESSAGE_MAP()


BOOL CBrowserToolTip::Create(CWnd *pParentWnd)
{
    return CWnd::CreateEx(WS_EX_TOOLWINDOW,
        AfxRegisterWndClass(CS_SAVEBITS, NULL, GetSysColorBrush(COLOR_INFOBK), NULL),
        _T("ToolTip"), WS_POPUP | WS_BORDER, 0, 0, 1, 1, pParentWnd->GetSafeHwnd(), NULL);
}




void CBrowserToolTip::OnPaint() 
{
	CPaintDC dc(this); 

    CRect rcClient;
    GetClientRect(&rcClient);

    
    int oldBkMode = dc.SetBkMode(TRANSPARENT);
    COLORREF oldTextColor = dc.SetTextColor(GetSysColor(COLOR_INFOTEXT));
    HGDIOBJ oldFont = dc.SelectObject(GetStockObject(DEFAULT_GUI_FONT));

    dc.DrawText(m_szTipText, -1, rcClient, DT_SINGLELINE | DT_VCENTER | DT_CENTER);

    dc.SetBkMode(oldBkMode);
    dc.SetTextColor(oldTextColor);
    dc.SelectObject(oldFont);
}

BOOL CBrowserToolTip::PreCreateWindow(CREATESTRUCT& cs) 
{
	return CWnd::PreCreateWindow(cs);
}

void CBrowserToolTip::SetTipText(const CString &szTipText)
{
    m_szTipText = szTipText;
}

void CBrowserToolTip::Show(CWnd *pOverWnd, long left, long top)
{
    
    CRect rcNewClient(0,0,0,0);
    CDC *pdc = GetDC();
    HGDIOBJ oldFont = pdc->SelectObject(GetStockObject(DEFAULT_GUI_FONT));
    rcNewClient.bottom = pdc->DrawText(m_szTipText, -1, rcNewClient,
        DT_SINGLELINE | DT_VCENTER | DT_CENTER | DT_CALCRECT);
    pdc->SelectObject(oldFont);
    ReleaseDC(pdc);
    rcNewClient.right += 8;
    rcNewClient.bottom += 8;
    
    
    AdjustWindowRectEx(rcNewClient, GetWindowLong(m_hWnd, GWL_STYLE), FALSE, GetWindowLong(m_hWnd, GWL_EXSTYLE));

    
    CPoint ptTip(left, top);
    pOverWnd->ClientToScreen(&ptTip);

    
    POINT ptCursor;
    GetCursorPos(&ptCursor);
    long cyCursor = GetSystemMetrics(SM_CYCURSOR);
    if (ptTip.y < ptCursor.y + cyCursor)
        ptTip.y = ptCursor.y + cyCursor;

    
    RECT rcScreen;
    GetDesktopWindow()->GetClientRect(&rcScreen);
    if (ptTip.x < 0)
        ptTip.x = 0;
    else if (ptTip.x + rcNewClient.Width() > rcScreen.right)
        ptTip.x = rcScreen.right - rcNewClient.Width();
    if (ptTip.y < 0)
        ptTip.y = 0;
    else if (ptTip.y + rcNewClient.Height() > rcScreen.bottom)
        ptTip.y = rcScreen.bottom - rcNewClient.Height();

    
    SetWindowPos(&CWnd::wndTop, ptTip.x, ptTip.y, rcNewClient.Width(), rcNewClient.Height(),
        SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

void CBrowserToolTip::Hide()
{
    ShowWindow(SW_HIDE);
}
