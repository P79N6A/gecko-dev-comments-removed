





#ifndef TextOverflow_h_
#define TextOverflow_h_

#include "nsDisplayList.h"
#include "nsLineBox.h"
#include "nsStyleStruct.h"
#include "nsTHashtable.h"
#include "mozilla/Likely.h"
class nsIScrollableFrame;

namespace mozilla {
namespace css {







class TextOverflow {
 public:
  



  static TextOverflow* WillProcessLines(nsDisplayListBuilder*   aBuilder,
                                        nsIFrame*               aBlockFrame);
  




  void ProcessLine(const nsDisplayListSet& aLists, nsLineBox* aLine);

  



  nsDisplayList& GetMarkers() { return mMarkerList; }

  


  static bool CanHaveTextOverflow(nsDisplayListBuilder* aBuilder,
                                  nsIFrame*             aBlockFrame);

  typedef nsTHashtable<nsPtrHashKey<nsIFrame> > FrameHashtable;

 protected:
  TextOverflow() {}
  void Init(nsDisplayListBuilder*   aBuilder,
            nsIFrame*               aBlockFrame);

  struct AlignmentEdges {
    AlignmentEdges() : mAssigned(false) {}
    void Accumulate(const nsRect& aRect) {
      if (MOZ_LIKELY(mAssigned)) {
        x = NS_MIN(x, aRect.X());
        xmost = NS_MAX(xmost, aRect.XMost());
      } else {
        x = aRect.X();
        xmost = aRect.XMost();
        mAssigned = true;
      }
    }
    nscoord Width() { return xmost - x; }
    nscoord x;
    nscoord xmost;
    bool mAssigned;
  };

  struct InnerClipEdges {
    InnerClipEdges() : mAssignedLeft(false), mAssignedRight(false) {}
    void AccumulateLeft(const nsRect& aRect) {
      if (MOZ_LIKELY(mAssignedLeft)) {
        mLeft = NS_MAX(mLeft, aRect.X());
      } else {
        mLeft = aRect.X();
        mAssignedLeft = true;
      }
    }
    void AccumulateRight(const nsRect& aRect) {
      if (MOZ_LIKELY(mAssignedRight)) {
        mRight = NS_MIN(mRight, aRect.XMost());
      } else {
        mRight = aRect.XMost();
        mAssignedRight = true;
      }
    }
    nscoord mLeft;
    nscoord mRight;
    bool mAssignedLeft;
    bool mAssignedRight;
  };

  








  void ExamineLineFrames(nsLineBox*      aLine,
                         FrameHashtable* aFramesToHide,
                         AlignmentEdges* aAlignmentEdges);

  













  void ExamineFrameSubtree(nsIFrame*       aFrame,
                           const nsRect&   aContentArea,
                           const nsRect&   aInsideMarkersArea,
                           FrameHashtable* aFramesToHide,
                           AlignmentEdges* aAlignmentEdges,
                           bool*           aFoundVisibleTextOrAtomic,
                           InnerClipEdges* aClippedMarkerEdges);

  


















  void AnalyzeMarkerEdges(nsIFrame*       aFrame,
                          const nsIAtom*  aFrameType,
                          const nsRect&   aInsideMarkersArea,
                          FrameHashtable* aFramesToHide,
                          AlignmentEdges* aAlignmentEdges,
                          bool*           aFoundVisibleTextOrAtomic,
                          InnerClipEdges* aClippedMarkerEdges);

  






  void PruneDisplayListContents(nsDisplayList*        aList,
                                const FrameHashtable& aFramesToHide,
                                const nsRect&         aInsideMarkersArea);

  







  void CreateMarkers(const nsLineBox* aLine,
                     bool             aCreateLeft,
                     bool             aCreateRight,
                     const nsRect&    aInsideMarkersArea);

  nsRect                 mContentArea;
  nsDisplayListBuilder*  mBuilder;
  nsIFrame*              mBlock;
  nsIScrollableFrame*    mScrollableFrame;
  nsDisplayList          mMarkerList;
  bool                   mBlockIsRTL;
  bool                   mCanHaveHorizontalScrollbar;
  bool                   mAdjustForPixelSnapping;

  class Marker {
  public:
    void Init(const nsStyleTextOverflowSide& aStyle) {
      mInitialized = false;
      mWidth = 0;
      mStyle = &aStyle;
    }

    


    void SetupString(nsIFrame* aFrame);

    bool IsNeeded() const {
      return mHasOverflow;
    }
    void Reset() {
      mHasOverflow = false;
    }

    
    nscoord                        mWidth;
    
    nscoord                        mIntrinsicWidth;
    
    nsString                       mMarkerString;
    
    const nsStyleTextOverflowSide* mStyle;
    
    bool                           mHasOverflow;
    
    bool                           mInitialized;
    
    
    bool                           mActive;
  };

  Marker mLeft;  
  Marker mRight; 
};

} 
} 

#endif 
