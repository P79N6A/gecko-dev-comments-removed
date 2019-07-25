


















































#ifndef _nsFrameManager_h_
#define _nsFrameManager_h_

#include "nsIFrame.h"
#include "nsIStatefulFrame.h"
#include "nsChangeHint.h"
#include "nsFrameManagerBase.h"

namespace mozilla {
namespace css {
class RestyleTracker;
} 
} 












class nsFrameManager : public nsFrameManagerBase
{
  typedef mozilla::css::RestyleTracker RestyleTracker;

public:
  nsFrameManager() NS_HIDDEN;
  ~nsFrameManager() NS_HIDDEN;

  void* operator new(size_t aSize, nsIPresShell* aHost) {
    NS_ASSERTION(aSize == sizeof(nsFrameManager), "Unexpected subclass");
    NS_ASSERTION(aSize == sizeof(nsFrameManagerBase),
                 "Superclass/subclass mismatch");
    return aHost->FrameManager();
  }

  
  NS_HIDDEN_(nsresult) Init(nsIPresShell* aPresShell, nsStyleSet* aStyleSet);

  




  NS_HIDDEN_(void) Destroy();

  
  NS_HIDDEN_(nsPlaceholderFrame*) GetPlaceholderFrameFor(nsIFrame* aFrame);
  NS_HIDDEN_(nsresult)
    RegisterPlaceholderFrame(nsPlaceholderFrame* aPlaceholderFrame);

  NS_HIDDEN_(void)
    UnregisterPlaceholderFrame(nsPlaceholderFrame* aPlaceholderFrame);

  NS_HIDDEN_(void)      ClearPlaceholderFrameMap();

  
  NS_HIDDEN_(nsStyleContext*) GetUndisplayedContent(nsIContent* aContent);
  NS_HIDDEN_(void) SetUndisplayedContent(nsIContent* aContent,
                                         nsStyleContext* aStyleContext);
  NS_HIDDEN_(void) ChangeUndisplayedContent(nsIContent* aContent,
                                            nsStyleContext* aStyleContext);
  NS_HIDDEN_(void) ClearUndisplayedContentIn(nsIContent* aContent,
                                             nsIContent* aParentContent);
  NS_HIDDEN_(void) ClearAllUndisplayedContentIn(nsIContent* aParentContent);

  
  NS_HIDDEN_(nsresult) AppendFrames(nsIFrame*       aParentFrame,
                                    nsIAtom*        aListName,
                                    nsFrameList&    aFrameList);

  NS_HIDDEN_(nsresult) InsertFrames(nsIFrame*       aParentFrame,
                                    nsIAtom*        aListName,
                                    nsIFrame*       aPrevFrame,
                                    nsFrameList&    aFrameList);

  NS_HIDDEN_(nsresult) RemoveFrame(nsIAtom*        aListName,
                                   nsIFrame*       aOldFrame);

  



  NS_HIDDEN_(void)     NotifyDestroyingFrame(nsIFrame* aFrame);

  







  NS_HIDDEN_(nsresult) ReparentStyleContext(nsIFrame* aFrame);

  




  NS_HIDDEN_(void)
    ComputeStyleChangeFor(nsIFrame* aFrame,
                          nsStyleChangeList* aChangeList,
                          nsChangeHint aMinChange,
                          RestyleTracker& aRestyleTracker,
                          PRBool aRestyleDescendants);

  





  NS_HIDDEN_(void) CaptureFrameState(nsIFrame*              aFrame,
                                     nsILayoutHistoryState* aState);

  NS_HIDDEN_(void) RestoreFrameState(nsIFrame*              aFrame,
                                     nsILayoutHistoryState* aState);

  



  NS_HIDDEN_(void) CaptureFrameStateFor(nsIFrame*              aFrame,
                                        nsILayoutHistoryState* aState,
                                        nsIStatefulFrame::SpecialStateID aID =
                                                      nsIStatefulFrame::eNoID);

  NS_HIDDEN_(void) RestoreFrameStateFor(nsIFrame*              aFrame,
                                        nsILayoutHistoryState* aState,
                                        nsIStatefulFrame::SpecialStateID aID =
                                                      nsIStatefulFrame::eNoID);

#ifdef NS_DEBUG
  


  NS_HIDDEN_(void) DebugVerifyStyleTree(nsIFrame* aFrame);
#endif

  NS_HIDDEN_(nsIPresShell*) GetPresShell() const { return mPresShell; }
  NS_HIDDEN_(nsPresContext*) GetPresContext() const {
    return mPresShell->GetPresContext();
  }

private:
  enum DesiredA11yNotifications {
    eSkipNotifications,
    eSendAllNotifications,
    eNotifyIfShown
  };

  enum A11yNotificationType {
    eDontNotify,
    eNotifyShown,
    eNotifyHidden
  };

  
  
  
  
  
  NS_HIDDEN_(nsChangeHint)
    ReResolveStyleContext(nsPresContext    *aPresContext,
                          nsIFrame          *aFrame,
                          nsIContent        *aParentContent,
                          nsStyleChangeList *aChangeList, 
                          nsChangeHint       aMinChange,
                          nsRestyleHint      aRestyleHint,
                          RestyleTracker&    aRestyleTracker,
                          DesiredA11yNotifications aDesiredA11yNotifications,
                          nsTArray<nsIContent*>& aVisibleKidsOfHiddenElement);
};

#endif
