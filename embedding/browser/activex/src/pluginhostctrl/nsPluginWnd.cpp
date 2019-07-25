



































#include "stdafx.h"
#include "pluginhostctrl.h"
#include "nsPluginWnd.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif





nsPluginWnd::nsPluginWnd()
{

}

nsPluginWnd::~nsPluginWnd()
{

}

LRESULT nsPluginWnd::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    PAINTSTRUCT ps;
    HDC hdc;
    RECT rc;

    hdc = BeginPaint(&ps);
    GetClientRect(&rc);
    FillRect(hdc, &rc, (HBRUSH) GetStockObject(LTGRAY_BRUSH));
    EndPaint(&ps);

    return 0;
}
