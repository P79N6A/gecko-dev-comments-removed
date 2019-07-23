


















































#ifndef _nsFrameManager_h_
#define _nsFrameManager_h_

#include "nsIFrame.h"
#include "nsIStatefulFrame.h"
#include "nsChangeHint.h"
#include "nsFrameManagerBase.h"












class nsFrameManager : public nsFrameManagerBase
{
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

  




  NS_HIDDEN_(nsIFrame*) GetRootFrame() { return mRootFrame; }
  NS_HIDDEN_(void)      SetRootFrame(nsIFrame* aRootFrame)
  {
    NS_ASSERTION(!mRootFrame, "already have a root frame");
    mRootFrame = aRootFrame;
  }

  



  NS_HIDDEN_(nsIFrame*) GetCanvasFrame();

  
  
  
  NS_HIDDEN_(nsIFrame*) GetPrimaryFrameFor(nsIContent* aContent,
                                           PRInt32 aIndexHint);
  
  
  NS_HIDDEN_(nsresult)  SetPrimaryFrameFor(nsIContent* aContent,
                                           nsIFrame* aPrimaryFrame);
  
  
  
  
  NS_HIDDEN_(void)      RemoveAsPrimaryFrame(nsIContent* aContent,
                                             nsIFrame* aPrimaryFrame);
  NS_HIDDEN_(void)      ClearPrimaryFrameMap();

  
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
  NS_HIDDEN_(void) ClearUndisplayedContentMap();

  
  NS_HIDDEN_(nsresult) AppendFrames(nsIFrame*       aParentFrame,
                                    nsIAtom*        aListName,
                                    nsFrameList&    aFrameList)
  {
    return aParentFrame->AppendFrames(aListName, aFrameList);
  }

  NS_HIDDEN_(nsresult) InsertFrames(nsIFrame*       aParentFrame,
                                    nsIAtom*        aListName,
                                    nsIFrame*       aPrevFrame,
                                    nsFrameList&    aFrameList);

  NS_HIDDEN_(nsresult) RemoveFrame(nsIFrame*       aParentFrame,
                                   nsIAtom*        aListName,
                                   nsIFrame*       aOldFrame);

  



  NS_HIDDEN_(void)     NotifyDestroyingFrame(nsIFrame* aFrame);

  







  NS_HIDDEN_(nsresult) ReParentStyleContext(nsIFrame* aFrame);

  




  NS_HIDDEN_(void)
    ComputeStyleChangeFor(nsIFrame* aFrame,
                          nsStyleChangeList* aChangeList,
                          nsChangeHint aMinChange);

  
  
  
  NS_HIDDEN_(nsReStyleHint) HasAttributeDependentStyle(nsIContent *aContent,
                                                       nsIAtom *aAttribute,
                                                       PRInt32 aModType,
                                                       PRBool aAttrHasChanged);

  





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
  NS_HIDDEN_(nsChangeHint)
    ReResolveStyleContext(nsPresContext    *aPresContext,
                          nsIFrame          *aFrame,
                          nsIContent        *aParentContent,
                          nsStyleChangeList *aChangeList, 
                          nsChangeHint       aMinChange,
                          PRBool             aFireAccessibilityEvents);
};

#endif
