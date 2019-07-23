







































#ifndef __EmbedEventListener_h
#define __EmbedEventListener_h

#include "nsIDOMKeyListener.h"
#include "nsIDOMMouseListener.h"
#include "nsIDOMUIListener.h"

#include "nsIDOMMouseMotionListener.h"
#include "nsIDOMEventListener.h"
#include "nsIDOMFocusListener.h"
#include "EmbedContextMenuInfo.h"

#include "nsIDOMNode.h"
#include "nsIDOMDocument.h"
#include "nsIDOMElement.h"
#include "nsIURI.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMEvent.h"
#include "nsIDOM3Node.h"

#include "nsIURI.h"
#include "nsIIOService.h"
#include "nsNetCID.h"
#include "nsCOMPtr.h"
#include "nsIFileURL.h"
#include "nsILocalFile.h"
#include "nsIFile.h"
#include "nsIWebBrowserPersist.h"
#include "nsCWebBrowserPersist.h"
#include "nsIWebProgressListener.h"
#include "nsISelectionController.h"
#include "nsIDOMMouseEvent.h"
#include "nsXPCOMStrings.h"
#include "nsCRTGlue.h"

class EmbedPrivate;

class EmbedEventListener : public nsIDOMKeyListener,
                           public nsIDOMMouseListener,
                           public nsIDOMUIListener,
                           public nsIDOMMouseMotionListener,
                           public nsIWebProgressListener,
                           public nsIDOMFocusListener
{
 public:

  EmbedEventListener();
  virtual ~EmbedEventListener();

  nsresult Init(EmbedPrivate *aOwner);

  NS_DECL_ISUPPORTS


  
  NS_DECL_NSIWEBPROGRESSLISTENER
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent);
  NS_IMETHOD HandleLink(nsIDOMNode* node);
  

  NS_IMETHOD KeyDown(nsIDOMEvent* aDOMEvent);
  NS_IMETHOD KeyUp(nsIDOMEvent* aDOMEvent);
  NS_IMETHOD KeyPress(nsIDOMEvent* aDOMEvent);

  

  NS_IMETHOD MouseDown(nsIDOMEvent* aDOMEvent);
  NS_IMETHOD MouseUp(nsIDOMEvent* aDOMEvent);
  NS_IMETHOD MouseClick(nsIDOMEvent* aDOMEvent);
  NS_IMETHOD MouseDblClick(nsIDOMEvent* aDOMEvent);
  NS_IMETHOD MouseOver(nsIDOMEvent* aDOMEvent);
  NS_IMETHOD MouseOut(nsIDOMEvent* aDOMEvent);

  

  NS_IMETHOD Activate(nsIDOMEvent* aDOMEvent);
  NS_IMETHOD FocusIn(nsIDOMEvent* aDOMEvent);
  NS_IMETHOD FocusOut(nsIDOMEvent* aDOMEvent);

  
  NS_IMETHOD MouseMove(nsIDOMEvent* aDOMEvent);
  NS_IMETHOD DragMove(nsIDOMEvent* aMouseEvent);
  EmbedContextMenuInfo* GetContextInfo() { return mCtxInfo; }

  
  NS_IMETHOD Focus(nsIDOMEvent* aEvent);
  NS_IMETHOD Blur(nsIDOMEvent* aEvent);
  NS_IMETHOD HandleSelection(nsIDOMMouseEvent* aDOMMouseEvent);

  nsresult   NewURI            (nsIURI **result,
                                const char *spec);
  nsresult   GetIOService      (nsIIOService **ioService);

  void       GeneratePixBuf    ();

  void       GetFaviconFromURI (const char*  aURI);
 private:

  EmbedPrivate *mOwner;
  EmbedContextMenuInfo *mCtxInfo;

  
  nsCOMPtr<nsISelectionController> mCurSelCon;
  nsCOMPtr<nsISelectionController> mLastSelCon;
  PRBool mFocusInternalFrame;
  PRInt32 mClickCount;
};

#endif 
