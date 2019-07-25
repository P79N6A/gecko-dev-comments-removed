









































#ifndef nsAbsoluteContainingBlock_h___
#define nsAbsoluteContainingBlock_h___

#include "nsFrameList.h"
#include "nsHTMLReflowState.h"
#include "nsGkAtoms.h"
#include "nsContainerFrame.h"

class nsIAtom;
class nsIFrame;
class nsPresContext;















class nsAbsoluteContainingBlock
{
public:
  typedef nsIFrame::ChildListID ChildListID;

  nsAbsoluteContainingBlock(ChildListID aChildListID)
#ifdef DEBUG
    : mChildListID(aChildListID)
#endif
  {
    NS_ASSERTION(mChildListID == nsIFrame::kAbsoluteList ||
                 mChildListID == nsIFrame::kFixedList,
                 "should either represent position:fixed or absolute content");
  }

#ifdef DEBUG
  ChildListID GetChildListID() const { return mChildListID; }
#endif

  const nsFrameList& GetChildList() const { return mAbsoluteFrames; }
  void AppendChildList(nsTArray<nsIFrame::ChildList>* aLists,
                       ChildListID aListID) const
  {
    NS_ASSERTION(aListID == GetChildListID(), "wrong list ID");
    GetChildList().AppendIfNonempty(aLists, aListID);
  }

  nsresult SetInitialChildList(nsIFrame*       aDelegatingFrame,
                               ChildListID     aListID,
                               nsFrameList&    aChildList);
  nsresult AppendFrames(nsIFrame*      aDelegatingFrame,
                        ChildListID    aListID,
                        nsFrameList&   aFrameList);
  nsresult InsertFrames(nsIFrame*      aDelegatingFrame,
                        ChildListID    aListID,
                        nsIFrame*      aPrevFrame,
                        nsFrameList&   aFrameList);
  void RemoveFrame(nsIFrame*      aDelegatingFrame,
                   ChildListID    aListID,
                   nsIFrame*      aOldFrame);

  
  
  
  
  
  
  
  
  
  
  
  nsresult Reflow(nsContainerFrame*        aDelegatingFrame,
                  nsPresContext*           aPresContext,
                  const nsHTMLReflowState& aReflowState,
                  nsReflowStatus&          aReflowStatus,
                  nscoord                  aContainingBlockWidth,
                  nscoord                  aContainingBlockHeight,
                  PRBool                   aConstrainHeight,
                  PRBool                   aCBWidthChanged,
                  PRBool                   aCBHeightChanged,
                  nsOverflowAreas*         aOverflowAreas);


  void DestroyFrames(nsIFrame* aDelegatingFrame,
                     nsIFrame* aDestructRoot);

  PRBool  HasAbsoluteFrames() {return mAbsoluteFrames.NotEmpty();}

  
  
  void MarkSizeDependentFramesDirty();

  
  void MarkAllFramesDirty();

protected:
  
  
  
  PRBool FrameDependsOnContainer(nsIFrame* f, PRBool aCBWidthChanged,
                                 PRBool aCBHeightChanged);

  nsresult ReflowAbsoluteFrame(nsIFrame*                aDelegatingFrame,
                               nsPresContext*          aPresContext,
                               const nsHTMLReflowState& aReflowState,
                               nscoord                  aContainingBlockWidth,
                               nscoord                  aContainingBlockHeight,
                               PRBool                   aConstrainHeight,
                               nsIFrame*                aKidFrame,
                               nsReflowStatus&          aStatus,
                               nsOverflowAreas*         aOverflowAreas);

  
  
  
  void DoMarkFramesDirty(PRBool aMarkAllDirty);

protected:
  nsFrameList mAbsoluteFrames;  

#ifdef DEBUG
  ChildListID const mChildListID; 

  
  void PrettyUC(nscoord aSize,
                char*   aBuf);
#endif
};

#endif 

