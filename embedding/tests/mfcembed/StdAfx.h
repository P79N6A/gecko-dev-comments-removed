



































#ifndef _STDAFX_H
#define _STDAFX_H

#if _MSC_VER > 1000
#pragma once
#endif 

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

#include "nsCOMPtr.h"
#include "nsEmbedString.h"
#include "nsCWebBrowser.h"
#include "nsWidgetsCID.h"
#include "nsIDocShell.h"
#include "nsIWebBrowser.h"
#include "nsIBaseWindow.h"
#include "nsIWebNavigation.h"
#include "nsIWebBrowserChrome.h"
#include "nsIWebProgressListener.h"
#include "nsIWebProgress.h"
#include "nsIWindowCreator.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIDocShellTreeItem.h"
#include "nsIClipboardCommands.h"
#include "nsIWebBrowserPersist.h"
#include "nsIContextMenuListener2.h"
#include "nsITooltipListener.h"
#include "nsIDOMNode.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsIDOMDocument.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMHTMLFrameSetElement.h"
#include "nsIPrompt.h"
#include "nsEmbedAPI.h"         
#include "nsISHistory.h"
#include "nsISHEntry.h"
#include "nsIPref.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsIProfileChangeStatus.h"
#include "nsIObserverService.h"
#include "imgIContainer.h"
#ifdef MOZ_OLD_CACHE
#include "nsINetDataCacheManager.h"
#endif
#include "nsError.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"
#include "nsIEmbeddingSiteWindow2.h"
#include "nsIWebBrowserFind.h"
#include "nsIWebBrowserFocus.h"
#include "nsIURI.h"


#include "nsIWebBrowserPrint.h"
#include "nsIDOMWindow.h"








#define USE_PROFILES 1

#endif 
