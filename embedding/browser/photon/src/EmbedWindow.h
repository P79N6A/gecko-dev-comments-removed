





































#ifndef __EmbedWindow_h
#define __EmbedWindow_h

#include <nsString.h>
#include <nsIWebBrowserChrome.h>
#include <nsIWebBrowserChromeFocus.h>
#include <nsIEmbeddingSiteWindow.h>
#include <nsITooltipListener.h>
#include <nsIContextMenuListener.h>
#include <nsIDNSListener.h>
#include <nsISupports.h>
#include <nsIWebBrowser.h>
#include <nsIDocShell.h>
#include <nsIBaseWindow.h>
#include <nsIInterfaceRequestor.h>
#include <nsCOMPtr.h>
#include "nsString.h"

#include "nsIDOMMouseEvent.h"
#include "nsIDOMHTMLAnchorElement.h"

#include <Pt.h>

class EmbedPrivate;

class EmbedWindow : public nsIWebBrowserChrome,
			    public nsIWebBrowserChromeFocus,
                    public nsIEmbeddingSiteWindow,
                    public nsITooltipListener,
                    public nsIDNSListener,
                    public nsIContextMenuListener,
			    public nsIInterfaceRequestor
{

 public:

  EmbedWindow();
  virtual ~EmbedWindow();

  nsresult Init            (EmbedPrivate *aOwner);
  nsresult CreateWindow    (void);
  void     ReleaseChildren (void);
  int 	   SaveAs(char *fname,char *dirname);

  NS_DECL_ISUPPORTS

  NS_DECL_NSIWEBBROWSERCHROME

  NS_DECL_NSIWEBBROWSERCHROMEFOCUS

  NS_DECL_NSIEMBEDDINGSITEWINDOW

  NS_DECL_NSITOOLTIPLISTENER

  NS_DECL_NSICONTEXTMENULISTENER

  NS_DECL_NSIDNSLISTENER

  NS_DECL_NSIINTERFACEREQUESTOR

  nsString                 mTitle;
  nsString                 mJSStatus;
  nsString                 mLinkMessage;

  nsCOMPtr<nsIBaseWindow>  mBaseWindow; 
  nsCOMPtr<nsIWebBrowser>  mWebBrowser; 

private:

  EmbedPrivate            *mOwner;
  static PtWidget_t        *sTipWindow;
  PRBool                   mVisibility;
  PRBool                   mIsModal;

};
  

#endif 
