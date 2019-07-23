






































#ifndef nsContainerFrame_h___
#define nsContainerFrame_h___

#include "nsSplittableFrame.h"
#include "nsFrameList.h"
#include "nsLayoutUtils.h"



#define NS_FRAME_NO_MOVE_VIEW         0x0001
#define NS_FRAME_NO_MOVE_FRAME        (0x0002 | NS_FRAME_NO_MOVE_VIEW)
#define NS_FRAME_NO_SIZE_VIEW         0x0004
#define NS_FRAME_NO_VISIBILITY        0x0008




class nsContainerFrame : public nsSplittableFrame
{
public:
  
  NS_IMETHOD Init(nsIContent* aContent,
                  nsIFrame*   aParent,
                  nsIFrame*   aPrevInFlow);
  NS_IMETHOD SetInitialChildList(nsIAtom*  aListName,
                                 nsIFrame* aChildList);
  NS_IMETHOD AppendFrames(nsIAtom*  aListName,
                          nsIFrame* aFrameList);
  NS_IMETHOD InsertFrames(nsIAtom*  aListName,
                          nsIFrame* aPrevFrame,
                          nsIFrame* aFrameList);
  NS_IMETHOD RemoveFrame(nsIAtom*  aListName,
                         nsIFrame* aOldFrame);

  virtual nsIFrame* GetFirstChild(nsIAtom* aListName) const;
  virtual nsIAtom* GetAdditionalChildListName(PRInt32 aIndex) const;
  virtual void Destroy();
  virtual void ChildIsDirty(nsIFrame* aChild);

  virtual PRBool IsLeaf() const;
  virtual PRBool PeekOffsetNoAmount(PRBool aForward, PRInt32* aOffset);
  virtual PRBool PeekOffsetCharacter(PRBool aForward, PRInt32* aOffset);
  
#ifdef DEBUG
  NS_IMETHOD List(FILE* out, PRInt32 aIndent) const;
#endif  

  
  virtual void DeleteNextInFlowChild(nsPresContext* aPresContext,
                                     nsIFrame*       aNextInFlow);

  static PRInt32 LengthOf(nsIFrame* aFrameList) {
    nsFrameList tmp(aFrameList);
    return tmp.GetLength();
  }

  
  static void PositionFrameView(nsIFrame* aKidFrame);

  
  
  
  
  
  
  static void SyncFrameViewAfterReflow(nsPresContext* aPresContext,
                                       nsIFrame*       aFrame,
                                       nsIView*        aView,
                                       const nsRect*   aCombinedArea,
                                       PRUint32        aFlags = 0);
  
  
  
  
  
  
  
  
  
  static void SyncFrameViewProperties(nsPresContext*  aPresContext,
                                      nsIFrame*        aFrame,
                                      nsStyleContext*  aStyleContext,
                                      nsIView*         aView,
                                      PRUint32         aFlags = 0);

  
  static PRBool FrameNeedsView(nsIFrame* aFrame);
  
  
  void DoInlineIntrinsicWidth(nsIRenderingContext *aRenderingContext,
                              InlineIntrinsicWidthData *aData,
                              nsLayoutUtils::IntrinsicWidthType aType);

  



  virtual nsSize ComputeAutoSize(nsIRenderingContext *aRenderingContext,
                                 nsSize aCBSize, nscoord aAvailableWidth,
                                 nsSize aMargin, nsSize aBorder,
                                 nsSize aPadding, PRBool aShrinkWrap);

  










  nsresult ReflowChild(nsIFrame*                aKidFrame,
                       nsPresContext*           aPresContext,
                       nsHTMLReflowMetrics&     aDesiredSize,
                       const nsHTMLReflowState& aReflowState,
                       nscoord                  aX,
                       nscoord                  aY,
                       PRUint32                 aFlags,
                       nsReflowStatus&          aStatus);

  
















  static nsresult FinishReflowChild(nsIFrame*                 aKidFrame,
                                    nsPresContext*            aPresContext,
                                    const nsHTMLReflowState*  aReflowState,
                                    nsHTMLReflowMetrics&      aDesiredSize,
                                    nscoord                   aX,
                                    nscoord                   aY,
                                    PRUint32                  aFlags);

  
  static void PositionChildViews(nsIFrame* aFrame);
  
  








  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

protected:
  nsContainerFrame(nsStyleContext* aContext) : nsSplittableFrame(aContext) {}
  ~nsContainerFrame();

  








  nsresult BuildDisplayListForNonBlockChildren(nsDisplayListBuilder*   aBuilder,
                                               const nsRect&           aDirtyRect,
                                               const nsDisplayListSet& aLists,
                                               PRUint32                aFlags = 0);

  


  nsIFrame* GetOverflowFrames(nsPresContext*  aPresContext,
                              PRBool          aRemoveProperty) const;

  


  nsresult SetOverflowFrames(nsPresContext*  aPresContext,
                             nsIFrame*       aOverflowFrames);

  








  PRBool MoveOverflowToChildList(nsPresContext* aPresContext);

  













  void PushChildren(nsPresContext*  aPresContext,
                    nsIFrame*       aFromChild,
                    nsIFrame*       aPrevSibling);

  nsFrameList mFrames;
};

#endif 
