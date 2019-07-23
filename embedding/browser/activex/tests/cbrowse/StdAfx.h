




#if !defined(AFX_STDAFX_H__5121F5E8_5324_11D2_93E1_000000000000__INCLUDED_)
#define AFX_STDAFX_H__5121F5E8_5324_11D2_93E1_000000000000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif 

#define VC_EXTRALEAN

#include <afxwin.h>         
#include <afxext.h>         
#include <afxdisp.h>        
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			
#endif 

#define _ATL_APARTMENT_THREADED
#define _ATL_DEBUG_INTERFACES


#include <atlbase.h>

class CBrowseModule : public CComModule
{
public:
	DWORD dwThreadID;

	LPCTSTR FindOneOf(LPCTSTR p1, LPCTSTR p2);
	virtual LONG Unlock();
	virtual LONG Lock();
};
	
extern CBrowseModule _Module;

#include <atlcom.h>
#include <atlctl.h>

#include <exdisp.h>
#include <mshtml.h>
#include <mshtmhst.h>
#include <activscp.h>

#include <string>
#include <vector>
#include <map>
#include <list>

#include "..\..\src\control\ActiveXTypes.h"
#include "..\..\src\control\BrowserDiagnostics.h"
#include "..\..\src\control\ActiveScriptSite.h"

#include "..\..\src\common\PropertyList.h"
#include "..\..\src\common\PropertyBag.h"
#include "..\..\src\common\ControlSiteIPFrame.h"
#include "..\..\src\common\ControlSite.h"

#include "CBrowserCtlSite.h"
#include "Tests.h"

#define SECTION_TEST             _T("Test")
#define KEY_TESTURL              _T("TestURL")
#define KEY_TESTCGI              _T("TestCGI")
#define KEY_TESTURL_DEFAULTVALUE _T("http://www.mozilla.org")
#define KEY_TESTCGI_DEFAULTVALUE _T("http://www.mozilla.org")

#define NS_ASSERTION(x,y)
#define TRACE_METHOD(x)





#endif 
