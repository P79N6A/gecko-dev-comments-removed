







































#ifndef nsContainerFrame_h___
#define nsContainerFrame_h___

#include "nsSplittableFrame.h"
#include "nsFrameList.h"
#include "nsLayoutUtils.h"
#include "nsAutoPtr.h"





#define NS_CONTAINER_LIST_COUNT_SANS_OC 1
  
#define NS_CONTAINER_LIST_COUNT_INCL_OC 3
  



#define NS_FRAME_NO_MOVE_VIEW         0x0001
#define NS_FRAME_NO_MOVE_FRAME        (0x0002 | NS_FRAME_NO_MOVE_VIEW)
#define NS_FRAME_NO_SIZE_VIEW         0x0004
#define NS_FRAME_NO_VISIBILITY        0x0008


#define NS_FRAME_INVALIDATE_ON_MOVE   0x0010 

class nsOverflowContinuationTracker;




class nsContainerFrame : public nsSplittableFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  
  NS_IMETHOD Init(nsIContent* aContent,
                  nsIFrame*   aParent,
                  nsIFrame*   aPrevInFlow);
  NS_IMETHOD SetInitialChildList(nsIAtom*     aListName,
                                 nsFrameList& aChildList);
  NS_IMETHOD AppendFrames(nsIAtom*  aListName,
                          nsFrameList& aFrameList);
  NS_IMETHOD InsertFrames(nsIAtom*  aListName,
                          nsIFrame* aPrevFrame,
                          nsFrameList& aFrameList);
  NS_IMETHOD RemoveFrame(nsIAtom*  aListName,
                         nsIFrame* aOldFrame);

  virtual nsFrameList GetChildList(nsIAtom* aListName) const;
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
                                     nsIFrame*      aNextInFlow,
                                     PRBool         aDeletingEmptyFrames);

  
  static void PositionFrameView(nsIFrame* aKidFrame);

  
  
  
  
  
  
  static void SyncFrameViewAfterReflow(nsPresContext* aPresContext,
                                       nsIFrame*       aFrame,
                                       nsIView*        aView,
                                       const nsRect*   aCombinedArea,
                                       PRUint32        aFlags = 0);

  
  
  static void SyncWindowProperties(nsPresContext*       aPresContext,
                                   nsIFrame*            aFrame,
                                   nsIView*             aView);

  
  
  
  
  
  
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

  










  nsresult ReflowChild(nsIFrame*                      aKidFrame,
                       nsPresContext*                 aPresContext,
                       nsHTMLReflowMetrics&           aDesiredSize,
                       const nsHTMLReflowState&       aReflowState,
                       nscoord                        aX,
                       nscoord                        aY,
                       PRUint32                       aFlags,
                       nsReflowStatus&                aStatus,
                       nsOverflowContinuationTracker* aTracker = nsnull);

  
















  static nsresult FinishReflowChild(nsIFrame*                  aKidFrame,
                                    nsPresContext*             aPresContext,
                                    const nsHTMLReflowState*   aReflowState,
                                    const nsHTMLReflowMetrics& aDesiredSize,
                                    nscoord                    aX,
                                    nscoord                    aY,
                                    PRUint32                   aFlags);

  
  static void PositionChildViews(nsIFrame* aFrame);

  
  



































  friend class nsOverflowContinuationTracker;

  























  nsresult ReflowOverflowContainerChildren(nsPresContext*           aPresContext,
                                           const nsHTMLReflowState& aReflowState,
                                           nsRect&                  aOverflowRect,
                                           PRUint32                 aFlags,
                                           nsReflowStatus&          aStatus);

  










  virtual nsresult StealFrame(nsPresContext* aPresContext,
                              nsIFrame*      aChild,
                              PRBool         aForceNormal = PR_FALSE);

  


  void DisplayOverflowContainers(nsDisplayListBuilder*   aBuilder,
                                 const nsRect&           aDirtyRect,
                                 const nsDisplayListSet& aLists);

  








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


  
  





  





  inline nsFrameList* GetOverflowFrames() const;

  







  inline nsFrameList* StealOverflowFrames();
  
  


  nsresult SetOverflowFrames(nsPresContext*  aPresContext,
                             const nsFrameList& aOverflowFrames);

  


  void DestroyOverflowList(nsPresContext* aPresContext);

  








  PRBool MoveOverflowToChildList(nsPresContext* aPresContext);

  













  void PushChildren(nsPresContext*  aPresContext,
                    nsIFrame*       aFromChild,
                    nsIFrame*       aPrevSibling);

  
  




  



  nsFrameList* GetPropTableFrames(nsPresContext*  aPresContext,
                                  nsIAtom*        aPropID) const;

  



  nsFrameList* RemovePropTableFrames(nsPresContext*  aPresContext,
                                     nsIAtom*        aPropID) const;

  






  PRBool RemovePropTableFrame(nsPresContext*  aPresContext,
                              nsIFrame*       aFrame,
                              nsIAtom*        aPropID) const;

  



  nsresult SetPropTableFrames(nsPresContext*  aPresContext,
                              nsFrameList*    aFrameList,
                              nsIAtom*        aPropID) const;
  

  nsFrameList mFrames;
};









#define IS_TRUE_OVERFLOW_CONTAINER(frame)                      \
  (  (frame->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER)  \
  && !( (frame->GetStateBits() & NS_FRAME_OUT_OF_FLOW) &&      \
        frame->GetStyleDisplay()->IsAbsolutelyPositioned()  )  )




























class nsOverflowContinuationTracker {
public:
  











  nsOverflowContinuationTracker(nsPresContext*    aPresContext,
                                nsContainerFrame* aFrame,
                                PRBool            aWalkOOFFrames,
                                PRBool            aSkipOverflowContainerChildren = PR_TRUE);
  


















  nsresult Insert(nsIFrame*       aOverflowCont,
                  nsReflowStatus& aReflowStatus);
  







  void Finish(nsIFrame* aChild);

  








  void Skip(nsIFrame* aChild, nsReflowStatus& aReflowStatus)
  {
    NS_PRECONDITION(aChild, "null ptr");
    if (aChild == mSentry) {
      StepForward();
      NS_MergeReflowStatusInto(&aReflowStatus, NS_FRAME_OVERFLOW_INCOMPLETE);
    }
  }

private:

  void SetUpListWalker();
  void StepForward();

  



  nsFrameList* mOverflowContList;
  



  nsIFrame* mPrevOverflowCont;
  


  nsIFrame* mSentry;
  




  nsContainerFrame* mParent;
  

  PRBool mSkipOverflowContainerChildren;
  
  PRBool mWalkOOFFrames;
};

inline
nsFrameList*
nsContainerFrame::GetOverflowFrames() const
{
  nsFrameList* list =
    static_cast<nsFrameList*>(GetProperty(nsGkAtoms::overflowProperty));
  NS_ASSERTION(!list || !list->IsEmpty(), "Unexpected empty overflow list");
  return list;
}

inline
nsFrameList*
nsContainerFrame::StealOverflowFrames()
{
  nsFrameList* list =
    static_cast<nsFrameList*>(UnsetProperty(nsGkAtoms::overflowProperty));
  NS_ASSERTION(!list || !list->IsEmpty(), "Unexpected empty overflow list");
  return list;
}

#endif 
