










































#ifndef _STDAFX_H
#define _STDAFX_H

#if _MSC_VER > 1000
#pragma once
#endif 

#define VC_EXTRALEAN

#include <afxwin.h>         
#include <afxext.h>         
#include <afxdtctl.h>		
#include <afxpriv.h>		
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			
#endif 

#include "nsCOMPtr.h"
#include "nsNetUtil.h"
#include "nsString.h"
#include "nsVoidArray.h"
#include "nsCWebBrowser.h"
#include "nsXPIDLString.h"
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
#include "nsIDocShellTreeOwner.h"
#include "nsIDocShellTreeItem.h"
#include "nsIClipboardCommands.h"
#include "nsIWebBrowserPersist.h"
#include "nsIContextMenuListener.h"
#include "nsIDOMNode.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsReadableUtils.h"
#include "nsIPrompt.h"
#include "nsEmbedAPI.h"		 
#include "nsISHistory.h"
#include "nsISHEntry.h"
#include "nsIPref.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsIProfileChangeStatus.h"
#include "nsIObserverService.h"
#ifdef MOZ_OLD_CACHE
#include "nsINetDataCacheManager.h"
#endif
#include "nsError.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"
#include "nsIEmbeddingSiteWindow.h"
#include "nsIWebBrowserFind.h"
#include "nsIWebBrowserFocus.h"
#include "nsIServiceManager.h"


#include "nsIWebBrowserPrint.h"


#include "nsIGlobalHistory.h"
#include "nsIBrowserHistory.h"
#include "nsILocalFile.h"
#include "nsIProfile.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDocumentType.h"
#include "nsIURIContentListener.h"
#include "nsIHelperAppLauncherDialog.h"

#include "nsIDOMWindow.h"
#include "nsIDOMRange.h"
#include "nsIDOMBarProp.h"
#include "nsIDOMWindowCollection.h"
#include "nsISelection.h"
#include "nsITooltipListener.h"
#include "nsISimpleEnumerator.h"

#include "nsIXPIProgressDialog.h"
#include "nsIXPIDialogService.h"
#include "nsIWebBrowserSetup.h"
#include "nsCRT.h"
#include "nsIURILoader.h"
#include "nsCURILoader.h"
#include "nsIHttpHeaderVisitor.h"
#include "nsIEditingSession.h"
#include "nsICommandManager.h"
#include "nsICommandParams.h"




#endif 
