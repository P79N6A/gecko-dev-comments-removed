




#ifndef nsFocusManager_h___
#define nsFocusManager_h___

#include "nsCycleCollectionParticipant.h"
#include "nsIDocument.h"
#include "nsIFocusManager.h"
#include "nsIObserver.h"
#include "nsIWidget.h"
#include "nsWeakReference.h"
#include "mozilla/Attributes.h"

#define FOCUSMETHOD_MASK 0xF000
#define FOCUSMETHODANDRING_MASK 0xF0F000

#define FOCUSMANAGER_CONTRACTID "@mozilla.org/focus-manager;1"

class nsIContent;
class nsIDocShellTreeItem;
class nsPIDOMWindow;

struct nsDelayedBlurOrFocusEvent;






class nsFocusManager MOZ_FINAL : public nsIFocusManager,
                                 public nsIObserver,
                                 public nsSupportsWeakReference
{
  typedef mozilla::widget::InputContextAction InputContextAction;

public:

  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsFocusManager, nsIFocusManager)
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIFOCUSMANAGER

  
  static nsresult Init();
  static void Shutdown();

  


  static nsFocusManager* GetFocusManager() { return sInstance; }

  




  nsIContent* GetFocusedContent() { return mFocusedContent; }

  


  nsPIDOMWindow* GetFocusedWindow() const { return mFocusedWindow; }

  


  nsPIDOMWindow* GetActiveWindow() const { return mActiveWindow; }

  


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

  


  static InputContextAction::Cause GetFocusMoveActionCause(uint32_t aFlags);

  static bool sMouseFocusesFormControl;

protected:

  nsFocusManager();
  ~nsFocusManager();

  



  void EnsureCurrentWidgetFocused();

  










  void SetFocusInner(nsIContent* aNewContent, int32_t aFlags,
                     bool aFocusChanged, bool aAdjustWidget);

  



  bool IsSameOrAncestor(nsPIDOMWindow* aPossibleAncestor,
                          nsPIDOMWindow* aWindow);

  



  already_AddRefed<nsPIDOMWindow> GetCommonAncestor(nsPIDOMWindow* aWindow1,
                                                    nsPIDOMWindow* aWindow2);

  





  void AdjustWindowFocus(nsPIDOMWindow* aNewWindow, bool aCheckPermission);

  


  bool IsWindowVisible(nsPIDOMWindow* aWindow);

  






  bool IsNonFocusableRoot(nsIContent* aContent);

  











  nsIContent* CheckIfFocusable(nsIContent* aContent, uint32_t aFlags);

  




















  bool Blur(nsPIDOMWindow* aWindowToClear,
              nsPIDOMWindow* aAncestorWindowToFocus,
              bool aIsLeavingDocument,
              bool aAdjustWidget);

  

























  void Focus(nsPIDOMWindow* aWindow,
             nsIContent* aContent,
             uint32_t aFlags,
             bool aIsNewDocument,
             bool aFocusChanged,
             bool aWindowRaised,
             bool aAdjustWidget);

  







  void SendFocusOrBlurEvent(uint32_t aType,
                            nsIPresShell* aPresShell,
                            nsIDocument* aDocument,
                            nsISupports* aTarget,
                            uint32_t aFocusMethod,
                            bool aWindowRaised,
                            bool aIsRefocus = false);

  


  void ScrollIntoView(nsIPresShell* aPresShell,
                      nsIContent* aContent,
                      uint32_t aFlags);

  


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
                                       int32_t aType, bool aNoParentTraversal,
                                       nsIContent** aNextContent);

  



























  nsresult GetNextTabbableContent(nsIPresShell* aPresShell,
                                  nsIContent* aRootContent,
                                  nsIContent* aOriginalStartContent,
                                  nsIContent* aStartContent,
                                  bool aForward,
                                  int32_t aCurrentTabIndex,
                                  bool aIgnoreTabIndex,
                                  nsIContent** aResultContent);

  











  nsIContent* GetNextTabbableMapArea(bool aForward,
                                     int32_t aCurrentTabIndex,
                                     nsIContent* aImageContent,
                                     nsIContent* aStartContent);

  




  int32_t GetNextTabIndex(nsIContent* aParent,
                          int32_t aCurrentTabIndex,
                          bool aForward);

  









  nsIContent* GetRootForFocus(nsPIDOMWindow* aWindow,
                              nsIDocument* aDocument,
                              bool aIsForDocNavigation,
                              bool aCheckVisibility);

  


  void GetLastDocShell(nsIDocShellTreeItem* aItem,
                       nsIDocShellTreeItem** aResult);

  


  void GetNextDocShell(nsIDocShellTreeItem* aItem,
                       nsIDocShellTreeItem** aResult);

  


  void GetPreviousDocShell(nsIDocShellTreeItem* aItem,
                           nsIDocShellTreeItem** aResult);

  








  nsIContent* GetNextTabbablePanel(nsIDocument* aDocument, nsIFrame* aCurrentPopup, bool aForward);

  













  nsIContent* GetNextTabbableDocument(nsIContent* aStartContent, bool aForward);

  







  void GetFocusInSelection(nsPIDOMWindow* aWindow,
                           nsIContent* aStartSelection,
                           nsIContent* aEndSelection,
                           nsIContent** aFocusedContent);

private:
  
  
  
  
  
  
  
  
  static void NotifyFocusStateChange(nsIContent* aContent,
                                     bool aWindowShouldShowFocusRing,
                                     bool aGettingFocus);

  void SetFocusedWindowInternal(nsPIDOMWindow* aWindow);

  
  nsCOMPtr<nsPIDOMWindow> mActiveWindow;

  
  
  
  nsCOMPtr<nsPIDOMWindow> mFocusedWindow;

  
  
  
  nsCOMPtr<nsIContent> mFocusedContent;

  
  
  
  nsCOMPtr<nsIContent> mFirstBlurEvent;
  nsCOMPtr<nsIContent> mFirstFocusEvent;

  
  nsCOMPtr<nsPIDOMWindow> mWindowBeingLowered;

  
  
  nsTArray<nsDelayedBlurOrFocusEvent> mDelayedBlurFocusEvents;

  
  
  
  
  
  
  nsCOMPtr<nsIDocument> mMouseDownEventHandlingDocument;

  static bool sTestMode;

  
  static nsFocusManager* sInstance;
};

nsresult
NS_NewFocusManager(nsIFocusManager** aResult);

#endif
