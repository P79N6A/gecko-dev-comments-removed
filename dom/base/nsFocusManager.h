



































#ifndef nsFocusManager_h___
#define nsFocusManager_h___

#include "nsIFocusManager.h"
#include "nsWeakReference.h"
#include "nsIObserver.h"
#include "nsIContent.h"

#define FOCUSMETHOD_MASK 0xF000

#define FOCUSMANAGER_CONTRACTID "@mozilla.org/focus-manager;1"

class nsIDocShellTreeItem;
class nsPIDOMWindow;
struct nsDelayedBlurOrFocusEvent;






class nsFocusManager : public nsIFocusManager,
                       public nsIObserver,
                       public nsSupportsWeakReference
{
public:

  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsFocusManager, nsIFocusManager)
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIFOCUSMANAGER

  
  static nsresult Init();
  static void Shutdown();

  


  static nsFocusManager* GetFocusManager() { return sInstance; }

  




  nsIContent* GetFocusedContent() { return mFocusedContent; }

  









  static nsIContent* GetFocusedDescendant(nsPIDOMWindow* aWindow, PRBool aDeep,
                                          nsPIDOMWindow** aFocusedWindow);

  









  static nsIContent* GetRedirectedFocus(nsIContent* aContent);

protected:

  nsFocusManager();
  ~nsFocusManager();

  



  void EnsureCurrentWidgetFocused();

  









  void SetFocusInner(nsIContent* aNewContent, PRInt32 aFlags, PRBool aFocusChanged);

  



  PRBool IsSameOrAncestor(nsPIDOMWindow* aPossibleAncestor,
                          nsPIDOMWindow* aWindow);

  



  already_AddRefed<nsPIDOMWindow> GetCommonAncestor(nsPIDOMWindow* aWindow1,
                                                    nsPIDOMWindow* aWindow2);

  





  void AdjustWindowFocus(nsPIDOMWindow* aNewWindow);

  


  PRBool IsWindowVisible(nsPIDOMWindow* aWindow);

  











  nsIContent* CheckIfFocusable(nsIContent* aContent, PRUint32 aFlags);

  


















  PRBool Blur(nsPIDOMWindow* aWindowToClear,
              nsPIDOMWindow* aAncestorWindowToFocus,
              PRBool aIsLeavingDocument);

  























  void Focus(nsPIDOMWindow* aWindow,
             nsIContent* aContent,
             PRUint32 aFlags,
             PRBool aIsNewDocument,
             PRBool aFocusChanged,
             PRBool aWindowRaised);

  







  void SendFocusOrBlurEvent(PRUint32 aType,
                            nsIPresShell* aPresShell,
                            nsIDocument* aDocument,
                            nsISupports* aTarget,
                            PRUint32 aFocusMethod,
                            PRBool aWindowRaised);

  


  void ScrollIntoView(nsIPresShell* aPresShell,
                      nsIContent* aContent,
                      PRUint32 aFlags);

  


  void RaiseWindow(nsPIDOMWindow* aWindow);

  







  void UpdateCaret(PRBool aMoveCaretToFocus,
                   PRBool aUpdateVisibility,
                   nsIContent* aContent);

  


  void MoveCaretToFocus(nsIPresShell* aPresShell, nsIContent* aContent);

  


  nsresult SetCaretVisible(nsIPresShell* aPresShell,
                           PRBool aVisible,
                           nsIContent* aContent);


  

  



  nsresult GetSelectionLocation(nsIDocument* aDocument,
                                nsIPresShell* aPresShell,
                                nsIContent **aStartContent,
                                nsIContent **aEndContent);

  









  nsresult DetermineElementToMoveFocus(nsPIDOMWindow* aWindow,
                                       nsIContent* aStart,
                                       PRInt32 aType,
                                       nsIContent** aNextContent);

  



























  nsresult GetNextTabbableContent(nsIPresShell* aPresShell,
                                  nsIContent* aRootContent,
                                  nsIContent* aOriginalStartContent,
                                  nsIContent* aStartContent,
                                  PRBool aForward,
                                  PRInt32 aCurrentTabIndex,
                                  PRBool aIgnoreTabIndex,
                                  nsIContent** aResultContent);

  











  nsIContent* GetNextTabbableMapArea(PRBool aForward,
                                     PRInt32 aCurrentTabIndex,
                                     nsIContent* aImageContent,
                                     nsIContent* aStartContent);

  




  PRInt32 GetNextTabIndex(nsIContent* aParent,
                          PRInt32 aCurrentTabIndex,
                          PRBool aForward);

  









  nsIContent* GetRootForFocus(nsPIDOMWindow* aWindow,
                              nsIDocument* aDocument,
                              PRBool aIsForDocNavigation,
                              PRBool aCheckVisibility);

  


  void GetLastDocShell(nsIDocShellTreeItem* aItem,
                       nsIDocShellTreeItem** aResult);

  


  void GetNextDocShell(nsIDocShellTreeItem* aItem,
                       nsIDocShellTreeItem** aResult);

  


  void GetPreviousDocShell(nsIDocShellTreeItem* aItem,
                           nsIDocShellTreeItem** aResult);

  









  nsIContent* GetNextTabbableDocument(PRBool aForward);

  







  void GetFocusInSelection(nsPIDOMWindow* aWindow,
                           nsIContent* aStartSelection,
                           nsIContent* aEndSelection,
                           nsIContent** aFocusedContent);

  
  nsCOMPtr<nsPIDOMWindow> mActiveWindow;

  
  
  nsCOMPtr<nsPIDOMWindow> mFocusedWindow;

  
  
  
  nsCOMPtr<nsIContent> mFocusedContent;

  
  
  
  nsCOMPtr<nsIContent> mFirstBlurEvent;
  nsCOMPtr<nsIContent> mFirstFocusEvent;

  
  nsCOMPtr<nsPIDOMWindow> mWindowBeingLowered;

  
  
  nsTArray<nsDelayedBlurOrFocusEvent> mDelayedBlurFocusEvents;

  
  static nsFocusManager* sInstance;
};

nsresult
NS_NewFocusManager(nsIFocusManager** aResult);

#endif
