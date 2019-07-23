





































#ifndef __EmbedPrivate_h
#define __EmbedPrivate_h

#include <nsCOMPtr.h>
#include <nsString.h>
#include <nsIWebNavigation.h>
#include <nsIURIFixup.h>
#include <nsISHistory.h>
#include "nsIExternalHelperAppService.h"


#include <nsIWebBrowserChrome.h>
#include <nsIPrintSettings.h>
#include <nsIAppShell.h>
#include <nsIDOMEventReceiver.h>
#include <nsVoidArray.h>
#include <nsClipboard.h>

#include <nsIPref.h>

#include <Pt.h>

class EmbedProgress;
class EmbedWindow;
class EmbedContentListener;
class EmbedEventListener;
class EmbedStream;
class EmbedPrintListener;

class nsPIDOMWindow;

class EmbedPrivate {

 public:

  EmbedPrivate();
  ~EmbedPrivate();

  nsresult    Init            (PtWidget_t *aOwningWidget);
  nsresult    Setup           ();
  void        Unrealize       (void);
  void        Show            (void);
  void        Hide            (void);
  void        Position        (PRUint32 aX, PRUint32 aY);
  void        Size            (PRUint32 aWidth, PRUint32 aHeight);
  void        Destroy         (void);
  void        SetURI          (const char *aURI);
  void        LoadCurrentURI  (void);
  void        Stop  (void);
  void        Reload(int32_t flags);
  void        Back  (void);
  void        Forward  (void);
  void 		  ScrollUp(int amount);
  void 		  ScrollDown(int amount);
  void 		  ScrollLeft(int amount);
  void 		  ScrollRight(int amount);
  void        Cut  (int ig);
  void        Copy  (int ig);
  void        Paste  (int ig);
  void        SelectAll  (void);
  void        Clear  (void);
  int    	  SaveAs(char *fname,char *dirname);
  void 		  Print(PpPrintContext_t *pc);
  PRBool 	  CanGoBack();
  PRBool 	  CanGoForward();
  nsIPref 	  *GetPrefs();

  nsresult OpenStream         (const char *aBaseURI, const char *aContentType);
  nsresult AppendToStream     (const char *aData, PRInt32 aLen);
  nsresult CloseStream        (void);

  
  
  static EmbedPrivate *FindPrivateForBrowser(nsIWebBrowserChrome *aBrowser);

  
  
  
  void        ContentStateChange    (void);

  
  
  
  
  void        ContentFinishedLoading(void);


#if 0






  
  
  void        TopLevelFocusIn (void);
  void        TopLevelFocusOut(void);

  
  
  void        ChildFocusIn (void);
  void        ChildFocusOut(void);
#endif


  PtWidget_t                   *mOwningWidget;

  
  EmbedWindow                   *mWindow;
  nsCOMPtr<nsISupports>          mWindowGuard;
  EmbedProgress                 *mProgress;
  nsCOMPtr<nsISupports>          mProgressGuard;
  EmbedContentListener          *mContentListener;
  nsCOMPtr<nsISupports>          mContentListenerGuard;
  EmbedEventListener            *mEventListener;
  nsCOMPtr<nsISupports>          mEventListenerGuard;
  EmbedStream                   *mStream;
  nsCOMPtr<nsISupports>          mStreamGuard;
  EmbedPrintListener             *mPrint;
  nsCOMPtr<nsISupports>          mPrintGuard;

  nsCOMPtr<nsIWebNavigation>     mNavigation;
  nsCOMPtr<nsISHistory>          mSessionHistory;

  
  nsCOMPtr<nsIDOMEventReceiver>  mEventReceiver;

  
  nsString                       mURI;

  nsCOMPtr<nsIPrintSettings> m_PrintSettings;

  
  static PRUint32                sWidgetCount;
  
  static char                   *sCompPath;
  
  static nsIAppShell            *sAppShell;
  
  static nsIPref                *sPrefs;
	static nsVoidArray            *sWindowList;
  
  static nsClipboard *sClipboard;

  
  PRUint32                       mChromeMask;
  
  PRBool                         mIsChrome;
  
  PRBool                         mChromeLoaded;
  
  PtWidget_t                     *mMozWindowWidget;

  
  nsresult        GetPIDOMWindow   (nsPIDOMWindow **aPIWin);
 private:

  
  PRBool                         mListenersAttached;

  void GetListener    (void);
  void AttachListeners(void);
  void DetachListeners(void);
	void PrintHeaderFooter_FormatSpecialCodes(const char *original, nsString& aNewStr);

};

#endif 
