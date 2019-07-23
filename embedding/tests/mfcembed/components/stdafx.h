



































#ifndef _STDAFX_H
#define _STDAFX_H

#define VC_EXTRALEAN










#if !defined(DEBUG)
#define THERECANBENODEBUG
#endif

#include <afxwin.h>         
#include <afxext.h>         
#include <afxdtctl.h>		
#include <afxpriv.h>		
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			
#endif 




#if defined(THERECANBENODEBUG) && defined(DEBUG)
#undef DEBUG
#endif

#endif 
