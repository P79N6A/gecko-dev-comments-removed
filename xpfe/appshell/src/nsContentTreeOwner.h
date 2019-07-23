






































#ifndef nsContentTreeOwner_h__
#define nsContentTreeOwner_h__


#include "nsCOMPtr.h"
#include "nsString.h"


#include "nsIBaseWindow.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIWebBrowserChrome2.h"
#include "nsIWindowProvider.h"

class nsXULWindow;
class nsSiteWindow2;

class nsContentTreeOwner : public nsIDocShellTreeOwner_MOZILLA_1_8_BRANCH,
                           public nsIBaseWindow,
                           public nsIInterfaceRequestor,
                           public nsIWebBrowserChrome2,
                           public nsIWindowProvider
{
friend class nsXULWindow;
friend class nsSiteWindow2;

public:
   NS_DECL_ISUPPORTS

   NS_DECL_NSIBASEWINDOW
   NS_DECL_NSIDOCSHELLTREEOWNER
   NS_DECL_NSIINTERFACEREQUESTOR
   NS_DECL_NSIWEBBROWSERCHROME
   NS_DECL_NSIWEBBROWSERCHROME2
   NS_DECL_NSIWINDOWPROVIDER
   NS_DECL_NSIDOCSHELLTREEOWNER_MOZILLA_1_8_BRANCH

protected:
   nsContentTreeOwner(PRBool fPrimary);
   virtual ~nsContentTreeOwner();

   void XULWindow(nsXULWindow* aXULWindow);
   nsXULWindow* XULWindow();

protected:
   nsXULWindow      *mXULWindow;
   nsSiteWindow2    *mSiteWindow2;
   PRBool            mPrimary;
   PRBool            mContentTitleSetting;
   nsString          mWindowTitleModifier;
   nsString          mTitleSeparator;
   nsString          mTitlePreface;
   nsString          mTitleDefault;
};

#endif 
