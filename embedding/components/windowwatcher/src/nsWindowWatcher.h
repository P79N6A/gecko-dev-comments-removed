




































#ifndef __nsWindowWatcher_h__
#define __nsWindowWatcher_h__


#define NS_WINDOWWATCHER_CID \
 {0xa21bfa01, 0xf349, 0x4394, {0xa8, 0x4c, 0x8d, 0xe5, 0xcf, 0x7, 0x37, 0xd0}}

#include "nsCOMPtr.h"
#include "jspubtd.h"
#include "mozilla/Mutex.h"
#include "nsIWindowCreator.h" 
#include "nsIWindowWatcher.h"
#include "nsIPromptFactory.h"
#include "nsPIWindowWatcher.h"
#include "nsTArray.h"

class  nsIURI;
class  nsIDocShellTreeItem;
class  nsIDocShellTreeOwner;
class  nsIWebBrowserChrome;
class  nsString;
class  nsWatcherWindowEnumerator;
class  nsIScriptContext;
class  nsPromptService;
struct JSContext;
struct JSObject;
struct nsWatcherWindowEntry;
struct SizeSpec;

class nsWindowWatcher :
      public nsIWindowWatcher,
      public nsPIWindowWatcher,
      public nsIPromptFactory
{
friend class nsWatcherWindowEnumerator;

public:
  nsWindowWatcher();
  virtual ~nsWindowWatcher();

  nsresult Init();

  NS_DECL_ISUPPORTS

  NS_DECL_NSIWINDOWWATCHER
  NS_DECL_NSPIWINDOWWATCHER
  NS_DECL_NSIPROMPTFACTORY

protected:
  friend class nsPromptService;
  PRBool AddEnumerator(nsWatcherWindowEnumerator* inEnumerator);
  PRBool RemoveEnumerator(nsWatcherWindowEnumerator* inEnumerator);

  nsWatcherWindowEntry *FindWindowEntry(nsIDOMWindow *aWindow);
  nsresult RemoveWindow(nsWatcherWindowEntry *inInfo);

  
  
  already_AddRefed<nsIDocShellTreeItem>
    GetCallerTreeItem(nsIDocShellTreeItem* aParentItem);
  
  
  
  nsresult SafeGetWindowByName(const nsAString& aName,
                               nsIDOMWindow* aCurrentWindow,
                               nsIDOMWindow** aResult);

  
  
  nsresult OpenWindowJSInternal(nsIDOMWindow *aParent,
                                const char *aUrl,
                                const char *aName,
                                const char *aFeatures,
                                PRBool aDialog,
                                nsIArray *argv,
                                PRBool aCalledFromJS,
                                nsIDOMWindow **_retval);

  static JSContext *GetJSContextFromWindow(nsIDOMWindow *aWindow);
  static JSContext *GetJSContextFromCallStack();
  static nsresult   URIfromURL(const char *aURL,
                               nsIDOMWindow *aParent,
                               nsIURI **aURI);
  
  static PRUint32   CalculateChromeFlags(const char *aFeatures,
                                         PRBool aFeaturesSpecified,
                                         PRBool aDialog,
                                         PRBool aChromeURL,
                                         PRBool aHasChromeParent);
  static PRInt32    WinHasOption(const char *aOptions, const char *aName,
                                 PRInt32 aDefault, PRBool *aPresenceFlag);
  
  static void       CalcSizeSpec(const char* aFeatures, SizeSpec& aResult);
  static nsresult   ReadyOpenedDocShellItem(nsIDocShellTreeItem *aOpenedItem,
                                            nsIDOMWindow *aParent,
                                            PRBool aWindowIsNew,
                                            nsIDOMWindow **aOpenedWindow);
  static void       SizeOpenedDocShellItem(nsIDocShellTreeItem *aDocShellItem,
                                           nsIDOMWindow *aParent,
                                           const SizeSpec & aSizeSpec);
  static void       GetWindowTreeItem(nsIDOMWindow *inWindow,
                                      nsIDocShellTreeItem **outTreeItem);
  static void       GetWindowTreeOwner(nsIDOMWindow *inWindow,
                                       nsIDocShellTreeOwner **outTreeOwner);

  nsTArray<nsWatcherWindowEnumerator*> mEnumeratorList;
  nsWatcherWindowEntry *mOldestWindow;
  mozilla::Mutex        mListLock;

  nsCOMPtr<nsIWindowCreator> mWindowCreator;
};

#endif

