






































#ifndef __EmbedPrivate_h
#define __EmbedPrivate_h

#include "nsCOMPtr.h"
#ifdef MOZILLA_INTERNAL_API
#include "nsString.h"
#else
#include "nsStringAPI.h"
#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"
#endif
#include "nsIWebNavigation.h"
#include "nsISHistory.h"


#include "nsIWebBrowserChrome.h"
#include "nsIAppShell.h"
#include "nsIDOMEventReceiver.h"
#include "nsVoidArray.h"


#include "nsIGenericFactory.h"
#include "nsIComponentRegistrar.h"

#include "nsIDocCharset.h"
#include "nsIMarkupDocumentViewer.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIWebBrowserFind.h"

#include "nsIFocusController.h"

#include "nsIDOMWindowCollection.h"
#include "gtkmozembedprivate.h"

#include "nsICacheEntryDescriptor.h"

#include "EmbedGtkTools.h"
class EmbedProgress;
class EmbedWindow;
class EmbedContentListener;
class EmbedEventListener;

class nsPIDOMWindow;
class nsIDirectoryServiceProvider;

class EmbedCommon {
 public:
  EmbedCommon() {
  }
  ~EmbedCommon() { }
  static EmbedCommon* GetInstance();
  static void DeleteInstance();
  nsresult    Init(void);
  GtkObject   *mCommon;
  static GtkMozEmbed* GetAnyLiveWidget();
};
class EmbedPrivate {

 public:

  EmbedPrivate();
  ~EmbedPrivate();

  nsresult    Init            (GtkMozEmbed *aOwningWidget);
  nsresult    Realize         (PRBool *aAlreadRealized);
  void        Unrealize       (void);
  void        Show            (void);
  void        Hide            (void);
  void        Resize          (PRUint32 aWidth, PRUint32 aHeight);
  void        Destroy         (void);
  void        SetURI          (const char *aURI);
  void        LoadCurrentURI  (void);
  void        Reload          (PRUint32 reloadFlags);

  void        SetChromeMask   (PRUint32 chromeMask);
  void        ApplyChromeMask ();

  static void PushStartup     (void);
  static void PopStartup      (void);
  static void SetPath         (const char *aPath);
  static void SetCompPath     (const char *aPath);

  static void SetAppComponents (const nsModuleComponentInfo* aComps,
                                int aNumComponents);
  static void SetProfilePath  (const char *aDir, const char *aName);
  static void SetDirectoryServiceProvider (nsIDirectoryServiceProvider * appFileLocProvider);

  nsresult OpenStream         (const char *aBaseURI, const char *aContentType);
  nsresult AppendToStream     (const PRUint8 *aData, PRUint32 aLen);
  nsresult CloseStream        (void);

  
  
  static EmbedPrivate *FindPrivateForBrowser(nsIWebBrowserChrome *aBrowser);

  
  
  
  void        ContentStateChange    (void);

  
  
  
  
  void        ContentFinishedLoading(void);

  
  
  void        ChildFocusIn (void);
  void        ChildFocusOut(void);
  PRBool      ClipBoardAction(GtkMozEmbedClipboard type);
  char*       GetEncoding ();
  nsresult    SetEncoding (const char *encoding);
  PRBool      FindText(const char *exp, PRBool  reverse,
                       PRBool  whole_word, PRBool  case_sensitive,
                       PRBool  restart);
  void        SetScrollTop(PRUint32 aTop);
  nsresult    ScrollToSelectedNode(nsIDOMNode *aDOMNode);
  nsresult    InsertTextToNode(nsIDOMNode *aDOMNode, const char *string);
  nsresult    GetFocusController(nsIFocusController **controller);
  nsresult    GetDOMWindowByNode(nsIDOMNode *aNode, nsIDOMWindow * *aDOMWindow);
  nsresult    GetZoom(PRInt32 *aZoomLevel, nsISupports *aContext = nsnull);
  nsresult    SetZoom(PRInt32 aZoomLevel, nsISupports *aContext = nsnull);
  nsresult    HasFrames(PRUint32 *numberOfFrames);
  nsresult    GetMIMEInfo(const char **aMime, nsIDOMNode *aDOMNode = nsnull);
  nsresult    GetCacheEntry(const char *aStorage,
                            const char *aKeyName,
                            PRUint32 aAccess,
                            PRBool aIsBlocking,
                            nsICacheEntryDescriptor **aDescriptor);
  nsresult    GetSHistoryList(GtkMozHistoryItem **GtkHI,
                               GtkMozEmbedSessionHistory type, gint *count);


#ifdef MOZ_ACCESSIBILITY_ATK
  void *GetAtkObjectForCurrentDocument();
#endif

  GtkMozEmbed                   *mOwningWidget;

  
  EmbedWindow                   *mWindow;
  nsCOMPtr<nsISupports>          mWindowGuard;
  EmbedProgress                 *mProgress;
  nsCOMPtr<nsISupports>          mProgressGuard;
  EmbedContentListener          *mContentListener;
  nsCOMPtr<nsISupports>          mContentListenerGuard;
  EmbedEventListener            *mEventListener;
  nsCOMPtr<nsISupports>          mEventListenerGuard;

  nsCOMPtr<nsIWebNavigation>     mNavigation;
  nsCOMPtr<nsISHistory>          mSessionHistory;

  
  nsCOMPtr<nsIDOMEventReceiver>  mEventReceiver;

  
  nsString                       mURI;
  nsCString                      mPrePath;

  
  static PRUint32                sWidgetCount;
  
  static char                   *sPath;
  
  static char                   *sCompPath;
  
  static const nsModuleComponentInfo  *sAppComps;
  static int                     sNumAppComps;
  
  static nsIAppShell            *sAppShell;
  
  static nsVoidArray            *sWindowList;
  
  static nsILocalFile           *sProfileDir;
  static nsISupports            *sProfileLock;

  static nsIDirectoryServiceProvider * sAppFileLocProvider;

  
  PRUint32                       mChromeMask;
  
  PRBool                         mIsChrome;
  
  PRBool                         mChromeLoaded;

  
  PRBool                         mLoadFinished;

  
  GtkWidget                     *mMozWindowWidget;
  
  PRBool                         mIsDestroyed;

  
  
  PRBool                         mOpenBlock;
  PRBool                         mNeedFav;
 private:

  
  PRBool                         mListenersAttached;
  PRBool                         mDoResizeEmbed;

  void GetListener    (void);
  void AttachListeners(void);
  void DetachListeners(void);

  
  nsresult        GetPIDOMWindow   (nsPIDOMWindow **aPIWin);

  static nsresult RegisterAppComponents();

  
  static void       EnsureOffscreenWindow(void);
  static void       DestroyOffscreenWindow(void);
  static GtkWidget *sOffscreenWindow;
  static GtkWidget *sOffscreenFixed;

};

#endif 
