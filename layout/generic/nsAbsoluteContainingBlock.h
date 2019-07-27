









#ifndef nsAbsoluteContainingBlock_h___
#define nsAbsoluteContainingBlock_h___

#include "nsFrameList.h"
#include "nsIFrame.h"

class nsContainerFrame;
struct nsHTMLReflowState;
class nsPresContext;














class nsAbsoluteContainingBlock
{
public:
  typedef nsIFrame::ChildListID ChildListID;

  explicit nsAbsoluteContainingBlock(ChildListID aChildListID)
#ifdef DEBUG
    : mChildListID(aChildListID)
#endif
  {
    MOZ_ASSERT(mChildListID == nsIFrame::kAbsoluteList ||
               mChildListID == nsIFrame::kFixedList,
               "should either represent position:fixed or absolute content");
  }

  const nsFrameList& GetChildList() const { return mAbsoluteFrames; }
  void AppendChildList(nsTArray<nsIFrame::ChildList>* aLists,
                       ChildListID aListID) const
  {
    NS_ASSERTION(aListID == mChildListID, "wrong list ID");
    GetChildList().AppendIfNonempty(aLists, aListID);
  }

  void SetInitialChildList(nsIFrame*       aDelegatingFrame,
                           ChildListID     aListID,
                           nsFrameList&    aChildList);
  void AppendFrames(nsIFrame*      aDelegatingFrame,
                    ChildListID    aListID,
                    nsFrameList&   aFrameList);
  void InsertFrames(nsIFrame*      aDelegatingFrame,
                    ChildListID    aListID,
                    nsIFrame*      aPrevFrame,
                    nsFrameList&   aFrameList);
  void RemoveFrame(nsIFrame*      aDelegatingFrame,
                   ChildListID    aListID,
                   nsIFrame*      aOldFrame);

  













  void Reflow(nsContainerFrame*        aDelegatingFrame,
              nsPresContext*           aPresContext,
              const nsHTMLReflowState& aReflowState,
              nsReflowStatus&          aReflowStatus,
              const nsRect&            aContainingBlock,
              bool                     aConstrainHeight,
              bool                     aCBWidthChanged,
              bool                     aCBHeightChanged,
              nsOverflowAreas*         aOverflowAreas);

  void DestroyFrames(nsIFrame* aDelegatingFrame,
                     nsIFrame* aDestructRoot);

  bool HasAbsoluteFrames() const { return mAbsoluteFrames.NotEmpty(); }

  



  void MarkSizeDependentFramesDirty();

  


  void MarkAllFramesDirty();

protected:
  




  bool FrameDependsOnContainer(nsIFrame* aFrame, bool aCBWidthChanged,
                               bool aCBHeightChanged);

  void ReflowAbsoluteFrame(nsIFrame*                aDelegatingFrame,
                           nsPresContext*           aPresContext,
                           const nsHTMLReflowState& aReflowState,
                           const nsRect&            aContainingBlockRect,
                           bool                     aConstrainHeight,
                           nsIFrame*                aKidFrame,
                           nsReflowStatus&          aStatus,
                           nsOverflowAreas*         aOverflowAreas);

  





  void DoMarkFramesDirty(bool aMarkAllDirty);

protected:
  nsFrameList mAbsoluteFrames;  

#ifdef DEBUG
  ChildListID const mChildListID; 
#endif
};

#endif 
