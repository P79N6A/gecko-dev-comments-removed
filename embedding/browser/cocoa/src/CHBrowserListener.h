




































#ifndef __nsCocoaBrowserListener_h__
#define __nsCocoaBrowserListener_h__

#include "nsWeakReference.h"
#include "nsIInterfaceRequestor.h"
#include "nsIWebBrowser.h"
#include "nsIWebBrowserChrome.h"
#include "nsIWebProgressListener.h"
#include "nsIEmbeddingSiteWindow2.h"
#include "nsIWindowCreator.h"

#include "nsIContextMenuListener.h"
#include "nsITooltipListener.h"

@class CHBrowserView;

class CHBrowserListener : public nsSupportsWeakReference,
                               public nsIInterfaceRequestor,
                               public nsIWebBrowserChrome,
                               public nsIWindowCreator,
                               public nsIEmbeddingSiteWindow2,
                               public nsIWebProgressListener,
                               public nsIContextMenuListener,
                               public nsITooltipListener
{
public:
  CHBrowserListener(CHBrowserView* aView);
  virtual ~CHBrowserListener();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSIWEBBROWSERCHROME
  NS_DECL_NSIWINDOWCREATOR
  NS_DECL_NSIEMBEDDINGSITEWINDOW
  NS_DECL_NSIEMBEDDINGSITEWINDOW2
  NS_DECL_NSIWEBPROGRESSLISTENER
  NS_DECL_NSICONTEXTMENULISTENER
  NS_DECL_NSITOOLTIPLISTENER
    
  void AddListener(id <CHBrowserListener> aListener);
  void RemoveListener(id <CHBrowserListener> aListener);
  void SetContainer(id <CHBrowserContainer> aContainer);

private:
  CHBrowserView*          mView;     
  NSMutableArray*         mListeners;
  id <CHBrowserContainer> mContainer;
  PRBool                  mIsModal;
  PRUint32                mChromeFlags;
};


#endif 
