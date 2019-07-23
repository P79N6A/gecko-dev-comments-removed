






































#ifndef __EmbedWindow_h
#define __EmbedWindow_h

#ifdef MOZILLA_INTERNAL_API
#include "nsString.h"
#else
#include "nsStringAPI.h"
#endif
#include "nsIWebBrowserChrome.h"
#include "nsIWebBrowserChromeFocus.h"
#include "nsIEmbeddingSiteWindow.h"

#include "nsISupports.h"
#include "nsIWebBrowser.h"
#include "nsIBaseWindow.h"
#include "nsIInterfaceRequestor.h"
#include "nsCOMPtr.h"

#include <gtk/gtk.h>

class EmbedPrivate;

class EmbedWindow : public nsIWebBrowserChrome,
        public nsIWebBrowserChromeFocus,
                    public nsIEmbeddingSiteWindow,

        public nsIInterfaceRequestor
{

 public:

  EmbedWindow();
  virtual ~EmbedWindow();

  nsresult Init            (EmbedPrivate *aOwner);
  nsresult CreateWindow    (void);
  void     ReleaseChildren (void);

  NS_DECL_ISUPPORTS

  NS_DECL_NSIWEBBROWSERCHROME

  NS_DECL_NSIWEBBROWSERCHROMEFOCUS

  NS_DECL_NSIEMBEDDINGSITEWINDOW



  NS_DECL_NSIINTERFACEREQUESTOR

  nsString                 mTitle;
  nsString                 mJSStatus;
  nsString                 mLinkMessage;

  nsCOMPtr<nsIBaseWindow>  mBaseWindow; 

private:

  EmbedPrivate            *mOwner;
  nsCOMPtr<nsIWebBrowser>  mWebBrowser; 
  static GtkWidget        *sTipWindow;
  PRBool                   mVisibility;
  PRBool                   mIsModal;

};


#endif 
