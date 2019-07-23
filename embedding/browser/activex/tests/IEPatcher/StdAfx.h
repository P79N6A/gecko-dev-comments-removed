




#if !defined(AFX_STDAFX_H__A6031681_3B36_11D2_B44D_00600819607E__INCLUDED_)
#define AFX_STDAFX_H__A6031681_3B36_11D2_B44D_00600819607E__INCLUDED_

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
#include <afxtempl.h>
#include <afxmt.h>
#include <afxpriv.h>

#include <shlobj.h>





#ifndef IMPLEMENT_OLECREATE2
#define IMPLEMENT_OLECREATE2(class_name, external_name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
	AFX_DATADEF COleObjectFactory class_name::factory(class_name::guid, \
		RUNTIME_CLASS(class_name), TRUE, _T(external_name)); \
	const AFX_DATADEF GUID class_name::guid = \
		{ l, w1, w2, { b1, b2, b3, b4, b5, b6, b7, b8 } };
#endif 




#endif 
