









































#if !defined(AFX_STDAFX_H__1339B542_3453_11D2_93B9_000000000000__INCLUDED_)
#define AFX_STDAFX_H__1339B542_3453_11D2_93B9_000000000000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif 

#define STRICT

#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#define _ATL_APARTMENT_THREADED
#define _ATL_STATIC_REGISTRY

#define USE_PLUGIN






#if _MSC_VER >= 1400
#pragma conform(forScope, push, atlhack, off)
#endif

#include <atlbase.h>


extern CComModule _Module;
#include <atlcom.h>
#include <atlctl.h>

#if _MSC_VER >= 1400
#pragma conform(forScope, pop, atlhack)
#endif

#include <mshtml.h>
#include <mshtmhst.h>
#include <docobj.h>
#include <winsock2.h>
#include <comdef.h>

#ifdef USE_PLUGIN
#include <activscp.h>
#endif


#include <vector>
#include <list>
#include <string>


typedef long int32;

#include "PropertyList.h"
#include "PropertyBag.h"
#include "ItemContainer.h"
#include "ControlSite.h"
#include "ControlSiteIPFrame.h"
#include "ControlEventSink.h"

#include "npapi.h"




#endif 
