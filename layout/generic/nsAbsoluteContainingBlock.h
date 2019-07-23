









































#ifndef nsAbsoluteContainingBlock_h___
#define nsAbsoluteContainingBlock_h___

#include "nsFrameList.h"
#include "nsHTMLReflowState.h"
#include "nsGkAtoms.h"

class nsIAtom;
class nsIFrame;
class nsPresContext;
















class nsAbsoluteContainingBlock
{
public:
  nsAbsoluteContainingBlock(nsIAtom* aChildListName)
#ifdef DEBUG
    : mChildListName(aChildListName)
#endif
  {
    NS_ASSERTION(mChildListName == nsGkAtoms::absoluteList ||
                 mChildListName == nsGkAtoms::fixedList,
                 "should either represent position:fixed or absolute content");
  }

#ifdef DEBUG
  nsIAtom* GetChildListName() const { return mChildListName; }
#endif

  nsresult FirstChild(const nsIFrame* aDelegatingFrame,
                      nsIAtom*        aListName,
                      nsIFrame**      aFirstChild) const;
  nsIFrame* GetFirstChild() { return mAbsoluteFrames.FirstChild(); }
  
  nsresult SetInitialChildList(nsIFrame*       aDelegatingFrame,
                               nsIAtom*        aListName,
                               nsIFrame*       aChildList);
  nsresult AppendFrames(nsIFrame*      aDelegatingFrame,
                        nsIAtom*       aListName,
                        nsIFrame*      aFrameList);
  nsresult InsertFrames(nsIFrame*      aDelegatingFrame,
                        nsIAtom*       aListName,
                        nsIFrame*      aPrevFrame,
                        nsIFrame*      aFrameList);
  nsresult RemoveFrame(nsIFrame*      aDelegatingFrame,
                       nsIAtom*       aListName,
                       nsIFrame*      aOldFrame);

  
  
  
  
  
  
  
  
  
  
  
  nsresult Reflow(nsIFrame*                aDelegatingFrame,
                  nsPresContext*           aPresContext,
                  const nsHTMLReflowState& aReflowState,
                  nscoord                  aContainingBlockWidth,
                  nscoord                  aContainingBlockHeight,
                  PRBool                   aCBWidthChanged,
                  PRBool                   aCBHeightChanged,
                  nsRect*                  aChildBounds = nsnull);


  void DestroyFrames(nsIFrame* aDelegatingFrame);

  PRBool  HasAbsoluteFrames() {return mAbsoluteFrames.NotEmpty();}

protected:
  
  
  
  PRBool FrameDependsOnContainer(nsIFrame* f, PRBool aCBWidthChanged,
                                 PRBool aCBHeightChanged);

  nsresult ReflowAbsoluteFrame(nsIFrame*                aDelegatingFrame,
                               nsPresContext*          aPresContext,
                               const nsHTMLReflowState& aReflowState,
                               nscoord                  aContainingBlockWidth,
                               nscoord                  aContainingBlockHeight,
                               nsIFrame*                aKidFrame,
                               nsReflowStatus&          aStatus,
                               nsRect*                  aChildBounds);

protected:
  nsFrameList mAbsoluteFrames;  

#ifdef DEBUG
  nsIAtom* const mChildListName; 

  
  void PrettyUC(nscoord aSize,
                char*   aBuf);
#endif
};

#endif 

