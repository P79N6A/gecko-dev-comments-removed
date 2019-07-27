





#ifndef TextOverflow_h_
#define TextOverflow_h_

#include "nsDisplayList.h"
#include "nsTHashtable.h"
#include "nsAutoPtr.h"
#include "mozilla/Likely.h"
#include "mozilla/WritingModes.h"
#include <algorithm>

class nsIScrollableFrame;
class nsLineBox;

namespace mozilla {
namespace css {







class TextOverflow {
 public:
  



  static TextOverflow* WillProcessLines(nsDisplayListBuilder*   aBuilder,
                                        nsIFrame*               aBlockFrame);
  




  void ProcessLine(const nsDisplayListSet& aLists, nsLineBox* aLine);

  



  nsDisplayList& GetMarkers() { return mMarkerList; }

  


  static bool HasClippedOverflow(nsIFrame* aBlockFrame);
  


  static bool CanHaveTextOverflow(nsDisplayListBuilder* aBuilder,
                                  nsIFrame*             aBlockFrame);

  typedef nsTHashtable<nsPtrHashKey<nsIFrame> > FrameHashtable;

 protected:
  TextOverflow(nsDisplayListBuilder* aBuilder,
               nsIFrame* aBlockFrame);

  typedef mozilla::WritingMode WritingMode;
  typedef mozilla::LogicalRect LogicalRect;

  struct AlignmentEdges {
    AlignmentEdges() : mAssigned(false) {}
    void Accumulate(WritingMode aWM, const LogicalRect& aRect)
    {
      if (MOZ_LIKELY(mAssigned)) {
        mIStart = std::min(mIStart, aRect.IStart(aWM));
        mIEnd = std::max(mIEnd, aRect.IEnd(aWM));
      } else {
        mIStart = aRect.IStart(aWM);
        mIEnd = aRect.IEnd(aWM);
        mAssigned = true;
      }
    }
    nscoord ISize() { return mIEnd - mIStart; }
    nscoord mIStart;
    nscoord mIEnd;
    bool mAssigned;
  };

  struct InnerClipEdges {
    InnerClipEdges() : mAssignedIStart(false), mAssignedIEnd(false) {}
    void AccumulateIStart(WritingMode aWM, const LogicalRect& aRect)
    {
      if (MOZ_LIKELY(mAssignedIStart)) {
        mIStart = std::max(mIStart, aRect.IStart(aWM));
      } else {
        mIStart = aRect.IStart(aWM);
        mAssignedIStart = true;
      }
    }
    void AccumulateIEnd(WritingMode aWM, const LogicalRect& aRect)
    {
      if (MOZ_LIKELY(mAssignedIEnd)) {
        mIEnd = std::min(mIEnd, aRect.IEnd(aWM));
      } else {
        mIEnd = aRect.IEnd(aWM);
        mAssignedIEnd = true;
      }
    }
    nscoord mIStart;
    nscoord mIEnd;
    bool mAssignedIStart;
    bool mAssignedIEnd;
  };

  LogicalRect
    GetLogicalScrollableOverflowRectRelativeToBlock(nsIFrame* aFrame) const
  {
    return LogicalRect(mBlockWM,
                       aFrame->GetScrollableOverflowRect() +
                         aFrame->GetOffsetTo(mBlock),
                       mBlockWidth);
  }

  








  void ExamineLineFrames(nsLineBox*      aLine,
                         FrameHashtable* aFramesToHide,
                         AlignmentEdges* aAlignmentEdges);

  













  void ExamineFrameSubtree(nsIFrame*       aFrame,
                           const LogicalRect& aContentArea,
                           const LogicalRect& aInsideMarkersArea,
                           FrameHashtable* aFramesToHide,
                           AlignmentEdges* aAlignmentEdges,
                           bool*           aFoundVisibleTextOrAtomic,
                           InnerClipEdges* aClippedMarkerEdges);

  


















  void AnalyzeMarkerEdges(nsIFrame*       aFrame,
                          const nsIAtom*  aFrameType,
                          const LogicalRect& aInsideMarkersArea,
                          FrameHashtable* aFramesToHide,
                          AlignmentEdges* aAlignmentEdges,
                          bool*           aFoundVisibleTextOrAtomic,
                          InnerClipEdges* aClippedMarkerEdges);

  






  void PruneDisplayListContents(nsDisplayList* aList,
                                const FrameHashtable& aFramesToHide,
                                const LogicalRect& aInsideMarkersArea);

  







  void CreateMarkers(const nsLineBox* aLine,
                     bool aCreateIStart, bool aCreateIEnd,
                     const LogicalRect& aInsideMarkersArea);

  LogicalRect            mContentArea;
  nsDisplayListBuilder*  mBuilder;
  nsIFrame*              mBlock;
  nsIScrollableFrame*    mScrollableFrame;
  nsDisplayList          mMarkerList;
  nscoord                mBlockWidth;
  WritingMode            mBlockWM;
  bool                   mCanHaveInlineAxisScrollbar;
  bool                   mAdjustForPixelSnapping;

  class Marker {
  public:
    void Init(const nsStyleTextOverflowSide& aStyle) {
      mInitialized = false;
      mISize = 0;
      mStyle = &aStyle;
    }

    


    void SetupString(nsIFrame* aFrame);

    bool IsNeeded() const {
      return mHasOverflow;
    }
    void Reset() {
      mHasOverflow = false;
    }

    
    nscoord                        mISize;
    
    nscoord                        mIntrinsicISize;
    
    const nsStyleTextOverflowSide* mStyle;
    
    bool                           mHasOverflow;
    
    bool                           mInitialized;
    
    
    bool                           mActive;
  };

  Marker mIStart; 
  Marker mIEnd; 
};

} 
} 

#endif 
