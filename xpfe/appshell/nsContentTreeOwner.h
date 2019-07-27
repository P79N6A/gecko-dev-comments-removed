





#ifndef nsContentTreeOwner_h__
#define nsContentTreeOwner_h__


#include "nsCOMPtr.h"
#include "nsString.h"


#include "nsIBaseWindow.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIWebBrowserChrome3.h"
#include "nsIWindowProvider.h"

class nsXULWindow;
class nsSiteWindow;

class nsContentTreeOwner MOZ_FINAL : public nsIDocShellTreeOwner,
                                     public nsIBaseWindow,
                                     public nsIInterfaceRequestor,
                                     public nsIWebBrowserChrome3,
                                     public nsIWindowProvider
{
friend class nsXULWindow;
friend class nsSiteWindow;

public:
   NS_DECL_ISUPPORTS

   NS_DECL_NSIBASEWINDOW
   NS_DECL_NSIDOCSHELLTREEOWNER
   NS_DECL_NSIINTERFACEREQUESTOR
   NS_DECL_NSIWEBBROWSERCHROME
   NS_DECL_NSIWEBBROWSERCHROME2
   NS_DECL_NSIWEBBROWSERCHROME3
   NS_DECL_NSIWINDOWPROVIDER

protected:
   explicit nsContentTreeOwner(bool fPrimary);
   virtual ~nsContentTreeOwner();

   void XULWindow(nsXULWindow* aXULWindow);
   nsXULWindow* XULWindow();

protected:
   nsXULWindow      *mXULWindow;
   nsSiteWindow    *mSiteWindow;
   bool              mPrimary;
   bool              mContentTitleSetting;
   nsString          mWindowTitleModifier;
   nsString          mTitleSeparator;
   nsString          mTitlePreface;
   nsString          mTitleDefault;
};

#endif 
