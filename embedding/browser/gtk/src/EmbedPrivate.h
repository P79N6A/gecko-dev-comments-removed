




































#ifndef __EmbedPrivate_h
#define __EmbedPrivate_h

#include "nsCOMPtr.h"
#include "nsStringGlue.h"
#include "nsIWebNavigation.h"
#include "nsISHistory.h"


#include "nsIWebBrowserChrome.h"
#include "nsIAppShell.h"
#include "nsPIDOMEventTarget.h"
#include "nsTArray.h"

#include "gtkmozembedprivate.h"

class EmbedProgress;
class EmbedWindow;
class EmbedContentListener;
class EmbedEventListener;

class nsPIDOMWindow;
class nsIDirectoryServiceProvider;

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

  
  nsCOMPtr<nsPIDOMEventTarget>   mEventTarget;

  
  nsCString                      mURI;

  
  static PRUint32                sWidgetCount;
  
  static char                   *sPath;
  
  static char                   *sCompPath;
  
  static nsIAppShell            *sAppShell;
  
  static nsTArray<EmbedPrivate*> *sWindowList;
  
  static nsILocalFile           *sProfileDir;
  static nsISupports            *sProfileLock;

  static nsIDirectoryServiceProvider * sAppFileLocProvider;

  
  PRUint32                       mChromeMask;
  
  PRBool                         mIsChrome;
  
  PRBool                         mChromeLoaded;
  
  GtkWidget                     *mMozWindowWidget;
  
  PRBool                         mIsDestroyed;

 private:

  
  PRBool                         mListenersAttached;

  void GetListener    (void);
  void AttachListeners(void);
  void DetachListeners(void);

  
  nsresult        GetPIDOMWindow   (nsPIDOMWindow **aPIWin);
  
  static void RegisterAppComponents();

  
  static void       EnsureOffscreenWindow(void);
  static void       DestroyOffscreenWindow(void);
  static GtkWidget *sOffscreenWindow;
  static GtkWidget *sOffscreenFixed;
 
};

#endif 
