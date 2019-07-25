





































#ifndef TextOverflow_h_
#define TextOverflow_h_

#include "nsDisplayList.h"
#include "nsLineBox.h"
#include "nsStyleStruct.h"
#include "nsTHashtable.h"

namespace mozilla {
namespace css {








class TextOverflow {
 public:
  



  static TextOverflow* WillProcessLines(nsDisplayListBuilder*   aBuilder,
                                        const nsDisplayListSet& aLists,
                                        nsIFrame*               aBlockFrame);
  




  void ProcessLine(const nsDisplayListSet& aLists, nsLineBox* aLine);

  



  void DidProcessLines();

  


  static bool CanHaveTextOverflow(nsDisplayListBuilder* aBuilder,
                                  nsIFrame*             aBlockFrame);

  typedef nsTHashtable<nsPtrHashKey<nsIFrame> > FrameHashtable;

 protected:
  TextOverflow() {}

  struct AlignmentEdges {
    AlignmentEdges() : mAssigned(false) {}
    void Accumulate(const nsRect& aRect) {
      if (NS_LIKELY(mAssigned)) {
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

  








  void ExamineLineFrames(nsLineBox*      aLine,
                         FrameHashtable* aFramesToHide,
                         AlignmentEdges* aAlignmentEdges);

  









  void ExamineFrameSubtree(nsIFrame*       aFrame,
                           const nsRect&   aContentArea,
                           const nsRect&   aInsideMarkersArea,
                           FrameHashtable* aFramesToHide,
                           AlignmentEdges* aAlignmentEdges);

  














  void AnalyzeMarkerEdges(nsIFrame*       aFrame,
                          const nsIAtom*  aFrameType,
                          const nsRect&   aInsideMarkersArea,
                          FrameHashtable* aFramesToHide,
                          AlignmentEdges* aAlignmentEdges);

  






  void PruneDisplayListContents(nsDisplayList*        aList,
                                const FrameHashtable& aFramesToHide,
                                const nsRect&         aInsideMarkersArea);

  







  void CreateMarkers(const nsLineBox* aLine,
                     bool             aCreateLeft,
                     bool             aCreateRight,
                     const nsRect&    aInsideMarkersArea) const;

  nsRect                 mContentArea;
  nsDisplayListBuilder*  mBuilder;
  nsIFrame*              mBlock;
  nsDisplayList*         mMarkerList;
  bool                   mBlockIsRTL;
  bool                   mCanHaveHorizontalScrollbar;

  class Marker {
  public:
    void Init(const nsStyleTextOverflow& aStyle) {
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
    
    nsString                       mMarkerString;
    
    const nsStyleTextOverflow*     mStyle;
    
    bool                           mHasOverflow;
    
    bool                           mInitialized;
  };

  Marker mLeft;  
  Marker mRight; 
};

} 
} 

#endif 
