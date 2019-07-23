



#if !defined(AFX_STDAFX_H__45E5B413_2805_11D3_9425_000000000000__INCLUDED_)
#define AFX_STDAFX_H__45E5B413_2805_11D3_9425_000000000000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif 

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED

#include <atlbase.h>


extern CComModule _Module;
#include <atlcom.h>



extern const CLSID CLSID_MozXMLElement;
extern const CLSID CLSID_MozXMLDocument;
extern const CLSID CLSID_MozXMLElementCollection;
extern const IID LIBID_MozActiveXMLLib;

#include "XMLElement.h"
#include "XMLElementCollection.h"
#include "XMLDocument.h"

extern HRESULT ParseExpat(const char *pBuffer, unsigned long cbBufSize, IXMLDocument *pDocument, IXMLElement **ppElement);




#endif 
