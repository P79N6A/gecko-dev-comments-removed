



































#ifndef __PLUGINWND_H_
#define __PLUGINWND_H_

class nsPluginWnd : public CWindowImpl<nsPluginWnd>
{
public:
	nsPluginWnd();
	virtual ~nsPluginWnd();

    DECLARE_WND_CLASS(_T("MozCtrlPluginWindow"))

BEGIN_MSG_MAP(nsPluginWnd)
	MESSAGE_HANDLER(WM_PAINT, OnPaint)
END_MSG_MAP()

    LRESULT OnPaint(UINT , WPARAM , LPARAM , BOOL& );

};

#endif