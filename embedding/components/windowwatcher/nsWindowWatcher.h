




#ifndef __nsWindowWatcher_h__
#define __nsWindowWatcher_h__


#define NS_WINDOWWATCHER_CID \
 {0xa21bfa01, 0xf349, 0x4394, {0xa8, 0x4c, 0x8d, 0xe5, 0xcf, 0x7, 0x37, 0xd0}}

#include "nsCOMPtr.h"
#include "mozilla/Mutex.h"
#include "nsIWindowCreator.h" 
#include "nsIWindowWatcher.h"
#include "nsIPromptFactory.h"
#include "nsITabParent.h"
#include "nsPIWindowWatcher.h"
#include "nsTArray.h"

class  nsIURI;
class  nsIDocShellTreeItem;
class  nsIDocShellTreeOwner;
class nsPIDOMWindow;
class  nsWatcherWindowEnumerator;
class  nsPromptService;
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

  nsresult Init();

  NS_DECL_ISUPPORTS

  NS_DECL_NSIWINDOWWATCHER
  NS_DECL_NSPIWINDOWWATCHER
  NS_DECL_NSIPROMPTFACTORY

  static int32_t    GetWindowOpenLocation(nsIDOMWindow *aParent,
                                          uint32_t aChromeFlags,
                                          bool aCalledFromJS,
                                          bool aPositionSpecified,
                                          bool aSizeSpecified);

protected:
  virtual ~nsWindowWatcher();

  friend class nsPromptService;
  bool AddEnumerator(nsWatcherWindowEnumerator* inEnumerator);
  bool RemoveEnumerator(nsWatcherWindowEnumerator* inEnumerator);

  nsWatcherWindowEntry *FindWindowEntry(nsIDOMWindow *aWindow);
  nsresult RemoveWindow(nsWatcherWindowEntry *inInfo);

  
  
  already_AddRefed<nsIDocShellTreeItem>
    GetCallerTreeItem(nsIDocShellTreeItem* aParentItem);
  
  
  
  nsPIDOMWindow*
    SafeGetWindowByName(const nsAString& aName, nsIDOMWindow* aCurrentWindow);

  
  
  nsresult OpenWindowInternal(nsIDOMWindow *aParent,
                              const char *aUrl,
                              const char *aName,
                              const char *aFeatures,
                              bool aCalledFromJS,
                              bool aDialog,
                              bool aNavigate,
                              nsITabParent *aOpeningTab,
                              nsIArray *argv,
                              nsIDOMWindow **_retval);

  static nsresult   URIfromURL(const char *aURL,
                               nsIDOMWindow *aParent,
                               nsIURI **aURI);
  
  static uint32_t   CalculateChromeFlags(nsIDOMWindow *aParent,
                                         const char *aFeatures,
                                         bool aFeaturesSpecified,
                                         bool aDialog,
                                         bool aChromeURL,
                                         bool aHasChromeParent,
                                         bool aOpenedFromRemoteTab);
  static int32_t    WinHasOption(const char *aOptions, const char *aName,
                                 int32_t aDefault, bool *aPresenceFlag);
  
  static void       CalcSizeSpec(const char* aFeatures, SizeSpec& aResult);
  static nsresult   ReadyOpenedDocShellItem(nsIDocShellTreeItem *aOpenedItem,
                                            nsIDOMWindow *aParent,
                                            bool aWindowIsNew,
                                            nsIDOMWindow **aOpenedWindow);
  static void       SizeOpenedDocShellItem(nsIDocShellTreeItem *aDocShellItem,
                                           nsIDOMWindow *aParent,
                                           bool aIsCallerChrome,
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

