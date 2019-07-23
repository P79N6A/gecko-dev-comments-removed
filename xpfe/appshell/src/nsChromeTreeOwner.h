






































#ifndef nsChromeTreeOwner_h__
#define nsChromeTreeOwner_h__


#include "nsCOMPtr.h"


#include "nsIBaseWindow.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIWebProgressListener.h"
#include "nsWeakReference.h"

class nsXULWindow;

class nsChromeTreeOwner : public nsIDocShellTreeOwner_MOZILLA_1_8_BRANCH,
                          public nsIBaseWindow, 
                          public nsIInterfaceRequestor,
                          public nsIWebProgressListener,
                          public nsSupportsWeakReference
{
friend class nsXULWindow;

public:
   NS_DECL_ISUPPORTS

   NS_DECL_NSIINTERFACEREQUESTOR
   NS_DECL_NSIBASEWINDOW
   NS_DECL_NSIDOCSHELLTREEOWNER
   NS_DECL_NSIWEBPROGRESSLISTENER
   NS_DECL_NSIDOCSHELLTREEOWNER_MOZILLA_1_8_BRANCH

   static nsresult InitGlobals();
   static void     FreeGlobals();

protected:
   nsChromeTreeOwner();
   virtual ~nsChromeTreeOwner();

   void XULWindow(nsXULWindow* aXULWindow);
   nsXULWindow* XULWindow();

protected:
   nsXULWindow*      mXULWindow;
};

#endif 
