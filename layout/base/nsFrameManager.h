

















#ifndef _nsFrameManager_h_
#define _nsFrameManager_h_

#include "nsFrameManagerBase.h"

#include "nsAutoPtr.h"
#include "nsFrameList.h"
#include "nsIContent.h"
#include "nsStyleContext.h"

class nsContainerFrame;
class nsPlaceholderFrame;

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
  typedef mozilla::layout::FrameChildListID ChildListID;

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

  
  nsStyleContext* GetUndisplayedContent(nsIContent* aContent)
  {
    if (!mUndisplayedMap) {
      return nullptr;
    }
    return GetStyleContextInMap(mUndisplayedMap, aContent);
  }
  mozilla::UndisplayedNode*
    GetAllUndisplayedContentIn(nsIContent* aParentContent);
  void SetUndisplayedContent(nsIContent* aContent,
                             nsStyleContext* aStyleContext);
  void ChangeUndisplayedContent(nsIContent* aContent,
                                nsStyleContext* aStyleContext)
  {
    ChangeStyleContextInMap(mUndisplayedMap, aContent, aStyleContext);
  }

  void ClearUndisplayedContentIn(nsIContent* aContent,
                                 nsIContent* aParentContent);
  void ClearAllUndisplayedContentIn(nsIContent* aParentContent);

  
  


  nsStyleContext* GetDisplayContentsStyleFor(nsIContent* aContent)
  {
    if (!mDisplayContentsMap) {
      return nullptr;
    }
    return GetStyleContextInMap(mDisplayContentsMap, aContent);
  }

  



  mozilla::UndisplayedNode* GetAllDisplayContentsIn(nsIContent* aParentContent);
  


  void SetDisplayContents(nsIContent* aContent,
                          nsStyleContext* aStyleContext);
  


  void ChangeDisplayContents(nsIContent* aContent,
                             nsStyleContext* aStyleContext)
  {
    ChangeStyleContextInMap(mDisplayContentsMap, aContent, aStyleContext);
  }

  




  void ClearDisplayContentsIn(nsIContent* aContent,
                              nsIContent* aParentContent);
  void ClearAllDisplayContentsIn(nsIContent* aParentContent);

  
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
protected:
  static nsStyleContext* GetStyleContextInMap(UndisplayedMap* aMap,
                                              nsIContent* aContent);
  static mozilla::UndisplayedNode*
    GetAllUndisplayedNodesInMapFor(UndisplayedMap* aMap,
                                   nsIContent* aParentContent);
  static void SetStyleContextInMap(UndisplayedMap* aMap,
                                   nsIContent* aContent,
                                   nsStyleContext* aStyleContext);
  static void ChangeStyleContextInMap(UndisplayedMap* aMap,
                                      nsIContent* aContent,
                                      nsStyleContext* aStyleContext);
};

#endif
