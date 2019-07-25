



































#ifndef nsFocusManager_h___
#define nsFocusManager_h___

#include "nsIFocusManager.h"
#include "nsWeakReference.h"
#include "nsIObserver.h"
#include "nsIContent.h"

#define FOCUSMETHOD_MASK 0xF000
#define FOCUSMETHODANDRING_MASK 0xF0F000

#define FOCUSMANAGER_CONTRACTID "@mozilla.org/focus-manager;1"

class nsIDocShellTreeItem;
class nsPIDOMWindow;

namespace mozilla {
namespace dom {
  class TabParent;
}
}

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

  


  nsresult ContentRemoved(nsIDocument* aDocument, nsIContent* aContent);

  


  void SetMouseButtonDownHandlingDocument(nsIDocument* aDocument)
  {
    NS_ASSERTION(!aDocument || !mMouseDownEventHandlingDocument,
                 "Some mouse button down events are nested?");
    mMouseDownEventHandlingDocument = aDocument;
  }

  









  static nsIContent* GetFocusedDescendant(nsPIDOMWindow* aWindow, bool aDeep,
                                          nsPIDOMWindow** aFocusedWindow);

  









  static nsIContent* GetRedirectedFocus(nsIContent* aContent);

  




  static PRUint32 GetFocusMoveReason(PRUint32 aFlags);

  static bool sMouseFocusesFormControl;

protected:

  nsFocusManager();
  ~nsFocusManager();

  



  void EnsureCurrentWidgetFocused();

  










  void SetFocusInner(nsIContent* aNewContent, PRInt32 aFlags,
                     bool aFocusChanged, bool aAdjustWidget);

  



  bool IsSameOrAncestor(nsPIDOMWindow* aPossibleAncestor,
                          nsPIDOMWindow* aWindow);

  



  already_AddRefed<nsPIDOMWindow> GetCommonAncestor(nsPIDOMWindow* aWindow1,
                                                    nsPIDOMWindow* aWindow2);

  





  void AdjustWindowFocus(nsPIDOMWindow* aNewWindow, bool aCheckPermission);

  


  bool IsWindowVisible(nsPIDOMWindow* aWindow);

  






  bool IsNonFocusableRoot(nsIContent* aContent);

  











  nsIContent* CheckIfFocusable(nsIContent* aContent, PRUint32 aFlags);

  




















  bool Blur(nsPIDOMWindow* aWindowToClear,
              nsPIDOMWindow* aAncestorWindowToFocus,
              bool aIsLeavingDocument,
              bool aAdjustWidget);

  

























  void Focus(nsPIDOMWindow* aWindow,
             nsIContent* aContent,
             PRUint32 aFlags,
             bool aIsNewDocument,
             bool aFocusChanged,
             bool aWindowRaised,
             bool aAdjustWidget);

  







  void SendFocusOrBlurEvent(PRUint32 aType,
                            nsIPresShell* aPresShell,
                            nsIDocument* aDocument,
                            nsISupports* aTarget,
                            PRUint32 aFocusMethod,
                            bool aWindowRaised,
                            bool aIsRefocus = false);

  


  void ScrollIntoView(nsIPresShell* aPresShell,
                      nsIContent* aContent,
                      PRUint32 aFlags);

  


  void RaiseWindow(nsPIDOMWindow* aWindow);

  







  void UpdateCaret(bool aMoveCaretToFocus,
                   bool aUpdateVisibility,
                   nsIContent* aContent);

  


  void MoveCaretToFocus(nsIPresShell* aPresShell, nsIContent* aContent);

  


  nsresult SetCaretVisible(nsIPresShell* aPresShell,
                           bool aVisible,
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
                                  bool aForward,
                                  PRInt32 aCurrentTabIndex,
                                  bool aIgnoreTabIndex,
                                  nsIContent** aResultContent);

  











  nsIContent* GetNextTabbableMapArea(bool aForward,
                                     PRInt32 aCurrentTabIndex,
                                     nsIContent* aImageContent,
                                     nsIContent* aStartContent);

  




  PRInt32 GetNextTabIndex(nsIContent* aParent,
                          PRInt32 aCurrentTabIndex,
                          bool aForward);

  









  nsIContent* GetRootForFocus(nsPIDOMWindow* aWindow,
                              nsIDocument* aDocument,
                              bool aIsForDocNavigation,
                              bool aCheckVisibility);

  



  mozilla::dom::TabParent* GetRemoteForContent(nsIContent* aContent);

  


  void GetLastDocShell(nsIDocShellTreeItem* aItem,
                       nsIDocShellTreeItem** aResult);

  


  void GetNextDocShell(nsIDocShellTreeItem* aItem,
                       nsIDocShellTreeItem** aResult);

  


  void GetPreviousDocShell(nsIDocShellTreeItem* aItem,
                           nsIDocShellTreeItem** aResult);

  









  nsIContent* GetNextTabbableDocument(bool aForward);

  







  void GetFocusInSelection(nsPIDOMWindow* aWindow,
                           nsIContent* aStartSelection,
                           nsIContent* aEndSelection,
                           nsIContent** aFocusedContent);

private:
  
  
  
  
  
  
  
  
  static void NotifyFocusStateChange(nsIContent* aContent,
                                     bool aWindowShouldShowFocusRing,
                                     bool aGettingFocus);

  
  nsCOMPtr<nsPIDOMWindow> mActiveWindow;

  
  
  nsCOMPtr<nsPIDOMWindow> mFocusedWindow;

  
  
  
  nsCOMPtr<nsIContent> mFocusedContent;

  
  
  
  nsCOMPtr<nsIContent> mFirstBlurEvent;
  nsCOMPtr<nsIContent> mFirstFocusEvent;

  
  nsCOMPtr<nsPIDOMWindow> mWindowBeingLowered;

  
  
  nsTArray<nsDelayedBlurOrFocusEvent> mDelayedBlurFocusEvents;

  
  
  
  
  
  
  nsCOMPtr<nsIDocument> mMouseDownEventHandlingDocument;

  
  static nsFocusManager* sInstance;
};

nsresult
NS_NewFocusManager(nsIFocusManager** aResult);

#endif
