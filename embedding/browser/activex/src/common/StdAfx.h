









































#if !defined(AFX_STDAFX_H__1339B542_3453_11D2_93B9_000000000000__INCLUDED_)
#define AFX_STDAFX_H__1339B542_3453_11D2_93B9_000000000000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif 

  
#ifdef _MSC_VER
  #pragma warning( disable: 4786 )
#endif

#include "jstypes.h"
#include "prtypes.h"



#include "jscompat.h"

#include "prthread.h"
#include "prprf.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsStringAPI.h"
#include "nsCOMPtr.h"
#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"

#include "nsIDocument.h"
#include "nsIDocumentObserver.h"
#include "nsVoidArray.h"

#include "nsIDOMNode.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDocumentType.h"
#include "nsIDOMElement.h"

#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#define _ATL_APARTMENT_THREADED
#define _ATL_STATIC_REGISTRY




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


typedef long int32;


#pragma warning(disable : 4786)

#define TRACE_METHOD(fn) \
     { \
         ATLTRACE(_T("0x%04x %s()\n"), (int) GetCurrentThreadId(), _T(#fn)); \
     }
#define TRACE_METHOD_ARGS(fn, pattern, args) \
    { \
        ATLTRACE(_T("0x%04x %s(") _T(pattern) _T(")\n"), (int) GetCurrentThreadId(), _T(#fn), args); \
    }




#endif 
