







































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
  NS_DECL_QUERYFRAME_TARGET(nsContainerFrame)
  NS_DECL_QUERYFRAME

  
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
  virtual void DestroyFrom(nsIFrame* aDestructRoot);
  virtual void ChildIsDirty(nsIFrame* aChild);

  virtual PRBool IsLeaf() const;
  virtual PRBool PeekOffsetNoAmount(PRBool aForward, PRInt32* aOffset);
  virtual PRBool PeekOffsetCharacter(PRBool aForward, PRInt32* aOffset,
                                     PRBool aRespectClusters = PR_TRUE);
  
#ifdef DEBUG
  NS_IMETHOD List(FILE* out, PRInt32 aIndent) const;
#endif  

  

  





  virtual void DeleteNextInFlowChild(nsPresContext* aPresContext,
                                     nsIFrame*      aNextInFlow,
                                     PRBool         aDeletingEmptyFrames);

  



  static nsresult CreateViewForFrame(nsIFrame* aFrame,
                                     PRBool aForce);

  
  static void PositionFrameView(nsIFrame* aKidFrame);

  static nsresult ReparentFrameView(nsPresContext* aPresContext,
                                    nsIFrame*       aChildFrame,
                                    nsIFrame*       aOldParentFrame,
                                    nsIFrame*       aNewParentFrame);

  static nsresult ReparentFrameViewList(nsPresContext*     aPresContext,
                                        const nsFrameList& aChildFrameList,
                                        nsIFrame*          aOldParentFrame,
                                        nsIFrame*          aNewParentFrame);

  
  
  
  
  
  
  static void SyncFrameViewAfterReflow(nsPresContext* aPresContext,
                                       nsIFrame*       aFrame,
                                       nsIView*        aView,
                                       const nsRect&   aVisualOverflowArea,
                                       PRUint32        aFlags = 0);

  
  
  static void SyncWindowProperties(nsPresContext*       aPresContext,
                                   nsIFrame*            aFrame,
                                   nsIView*             aView);

  
  
  
  
  
  
  static void SyncFrameViewProperties(nsPresContext*  aPresContext,
                                      nsIFrame*        aFrame,
                                      nsStyleContext*  aStyleContext,
                                      nsIView*         aView,
                                      PRUint32         aFlags = 0);

  
  void DoInlineIntrinsicWidth(nsRenderingContext *aRenderingContext,
                              InlineIntrinsicWidthData *aData,
                              nsLayoutUtils::IntrinsicWidthType aType);

  



  virtual nsSize ComputeAutoSize(nsRenderingContext *aRenderingContext,
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
                                           nsOverflowAreas&         aOverflowRects,
                                           PRUint32                 aFlags,
                                           nsReflowStatus&          aStatus);

  










  virtual nsresult StealFrame(nsPresContext* aPresContext,
                              nsIFrame*      aChild,
                              PRBool         aForceNormal = PR_FALSE);

  








  nsFrameList StealFramesAfter(nsIFrame* aChild);

  


  void DisplayOverflowContainers(nsDisplayListBuilder*   aBuilder,
                                 const nsRect&           aDirtyRect,
                                 const nsDisplayListSet& aLists);

  








  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  
  static void DestroyFrameList(void* aPropertyValue)
  {
    if (aPropertyValue) {
      static_cast<nsFrameList*>(aPropertyValue)->Destroy();
    }
  }

  NS_DECLARE_FRAME_PROPERTY(OverflowProperty, DestroyFrameList)
  NS_DECLARE_FRAME_PROPERTY(OverflowContainersProperty, DestroyFrameList)
  NS_DECLARE_FRAME_PROPERTY(ExcessOverflowContainersProperty, DestroyFrameList)

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

  




  void DestroyOverflowList(nsPresContext* aPresContext,
                           nsIFrame*      aDestructRoot);

  








  PRBool MoveOverflowToChildList(nsPresContext* aPresContext);

  













  void PushChildren(nsPresContext*  aPresContext,
                    nsIFrame*       aFromChild,
                    nsIFrame*       aPrevSibling);

  
  




  



  nsFrameList* GetPropTableFrames(nsPresContext*                 aPresContext,
                                  const FramePropertyDescriptor* aProperty) const;

  



  nsFrameList* RemovePropTableFrames(nsPresContext*                 aPresContext,
                                     const FramePropertyDescriptor* aProperty);

  





  PRBool RemovePropTableFrame(nsPresContext*                 aPresContext,
                              nsIFrame*                      aFrame,
                              const FramePropertyDescriptor* aProperty);

  



  nsresult SetPropTableFrames(nsPresContext*                 aPresContext,
                              nsFrameList*                   aFrameList,
                              const FramePropertyDescriptor* aProperty);
  

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
    static_cast<nsFrameList*>(Properties().Get(OverflowProperty()));
  NS_ASSERTION(!list || !list->IsEmpty(), "Unexpected empty overflow list");
  return list;
}

inline
nsFrameList*
nsContainerFrame::StealOverflowFrames()
{
  nsFrameList* list =
    static_cast<nsFrameList*>(Properties().Remove(OverflowProperty()));
  NS_ASSERTION(!list || !list->IsEmpty(), "Unexpected empty overflow list");
  return list;
}

#endif 
