

















#ifndef _nsFrameManager_h_
#define _nsFrameManager_h_

#include "nsIFrame.h"
#include "nsFrameManagerBase.h"
#include "nsIContent.h"

class nsContainerFrame;

namespace mozilla {




struct UndisplayedNode {
  UndisplayedNode(nsIContent* aContent, nsStyleContext* aStyle)
    : mContent(aContent),
      mStyle(aStyle),
      mNext(nullptr)
  {
    MOZ_COUNT_CTOR(mozilla::UndisplayedNode);
  }

  ~UndisplayedNode()
  {
    MOZ_COUNT_DTOR(mozilla::UndisplayedNode);

    
    UndisplayedNode* cur = mNext;
    while (cur) {
      UndisplayedNode* next = cur->mNext;
      cur->mNext = nullptr;
      delete cur;
      cur = next;
    }
  }

  nsCOMPtr<nsIContent>      mContent;
  nsRefPtr<nsStyleContext>  mStyle;
  UndisplayedNode*          mNext;
};

} 












class nsFrameManager : public nsFrameManagerBase
{
  typedef nsIFrame::ChildListID ChildListID;

public:
  nsFrameManager(nsIPresShell *aPresShell, nsStyleSet* aStyleSet) {
    mPresShell = aPresShell;
    mStyleSet = aStyleSet;
    MOZ_ASSERT(mPresShell, "need a pres shell");
    MOZ_ASSERT(mStyleSet, "need a style set");
  }
  ~nsFrameManager();

  




  void Destroy();

  
  nsPlaceholderFrame* GetPlaceholderFrameFor(const nsIFrame* aFrame);
  nsresult
    RegisterPlaceholderFrame(nsPlaceholderFrame* aPlaceholderFrame);

  void
    UnregisterPlaceholderFrame(nsPlaceholderFrame* aPlaceholderFrame);

  void      ClearPlaceholderFrameMap();

  
  nsStyleContext* GetUndisplayedContent(nsIContent* aContent);
  mozilla::UndisplayedNode*
    GetAllUndisplayedContentIn(nsIContent* aParentContent);
  void SetUndisplayedContent(nsIContent* aContent,
                                         nsStyleContext* aStyleContext);
  void ChangeUndisplayedContent(nsIContent* aContent,
                                            nsStyleContext* aStyleContext);
  void ClearUndisplayedContentIn(nsIContent* aContent,
                                             nsIContent* aParentContent);
  void ClearAllUndisplayedContentIn(nsIContent* aParentContent);

  
  void AppendFrames(nsContainerFrame* aParentFrame,
                    ChildListID       aListID,
                    nsFrameList&      aFrameList);

  void InsertFrames(nsContainerFrame* aParentFrame,
                    ChildListID       aListID,
                    nsIFrame*         aPrevFrame,
                    nsFrameList&      aFrameList);

  void RemoveFrame(ChildListID     aListID,
                   nsIFrame*       aOldFrame);

  



  void     NotifyDestroyingFrame(nsIFrame* aFrame);

  








  void CaptureFrameState(nsIFrame*              aFrame,
                                     nsILayoutHistoryState* aState);

  void RestoreFrameState(nsIFrame*              aFrame,
                                     nsILayoutHistoryState* aState);

  


  void CaptureFrameStateFor(nsIFrame*              aFrame,
                                        nsILayoutHistoryState* aState);

  void RestoreFrameStateFor(nsIFrame*              aFrame,
                                        nsILayoutHistoryState* aState);

  nsIPresShell* GetPresShell() const { return mPresShell; }
  nsPresContext* GetPresContext() const {
    return mPresShell->GetPresContext();
  }
};

#endif
