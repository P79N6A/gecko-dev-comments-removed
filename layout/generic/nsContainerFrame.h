






#ifndef nsContainerFrame_h___
#define nsContainerFrame_h___

#include "mozilla/Attributes.h"
#include "nsSplittableFrame.h"
#include "nsFrameList.h"
#include "nsLayoutUtils.h"



#define NS_FRAME_NO_MOVE_VIEW         0x0001
#define NS_FRAME_NO_MOVE_FRAME        (0x0002 | NS_FRAME_NO_MOVE_VIEW)
#define NS_FRAME_NO_SIZE_VIEW         0x0004
#define NS_FRAME_NO_VISIBILITY        0x0008


#define NS_FRAME_NO_DELETE_NEXT_IN_FLOW_CHILD 0x0010

class nsOverflowContinuationTracker;
namespace mozilla {
class FramePropertyTable;
}






#ifdef DEBUG
#define CRAZY_W (1000000*60)
#define CRAZY_H CRAZY_W

#define CRAZY_WIDTH(_x) (((_x) < -CRAZY_W) || ((_x) > CRAZY_W))
#define CRAZY_HEIGHT(_y) (((_y) < -CRAZY_H) || ((_y) > CRAZY_H))
#endif




class nsContainerFrame : public nsSplittableFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS
  NS_DECL_QUERYFRAME_TARGET(nsContainerFrame)
  NS_DECL_QUERYFRAME

  
  virtual void Init(nsIContent* aContent,
                    nsIFrame*   aParent,
                    nsIFrame*   aPrevInFlow) MOZ_OVERRIDE;
  NS_IMETHOD SetInitialChildList(ChildListID  aListID,
                                 nsFrameList& aChildList) MOZ_OVERRIDE;
  NS_IMETHOD AppendFrames(ChildListID  aListID,
                          nsFrameList& aFrameList);
  NS_IMETHOD InsertFrames(ChildListID aListID,
                          nsIFrame* aPrevFrame,
                          nsFrameList& aFrameList);
  NS_IMETHOD RemoveFrame(ChildListID aListID,
                         nsIFrame* aOldFrame) MOZ_OVERRIDE;

  virtual const nsFrameList& GetChildList(ChildListID aList) const MOZ_OVERRIDE;
  virtual void GetChildLists(nsTArray<ChildList>* aLists) const MOZ_OVERRIDE;
  virtual void DestroyFrom(nsIFrame* aDestructRoot) MOZ_OVERRIDE;
  virtual void ChildIsDirty(nsIFrame* aChild) MOZ_OVERRIDE;

  virtual bool IsLeaf() const;
  virtual bool PeekOffsetNoAmount(bool aForward, int32_t* aOffset) MOZ_OVERRIDE;
  virtual bool PeekOffsetCharacter(bool aForward, int32_t* aOffset,
                                     bool aRespectClusters = true) MOZ_OVERRIDE;
  
#ifdef DEBUG
  NS_IMETHOD List(FILE* out, int32_t aIndent, uint32_t aFlags = 0) const MOZ_OVERRIDE;
#endif  

  

  












  nsresult CreateNextInFlow(nsPresContext* aPresContext,
                            nsIFrame*       aFrame,
                            nsIFrame*&      aNextInFlowResult);

  





  virtual void DeleteNextInFlowChild(nsPresContext* aPresContext,
                                     nsIFrame*      aNextInFlow,
                                     bool           aDeletingEmptyFrames);

  



  static void CreateViewForFrame(nsIFrame* aFrame,
                                 bool aForce);

  
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
                                       nsView*        aView,
                                       const nsRect&   aVisualOverflowArea,
                                       uint32_t        aFlags = 0);

  
  
  static void SyncWindowProperties(nsPresContext*       aPresContext,
                                   nsIFrame*            aFrame,
                                   nsView*             aView,
                                   nsRenderingContext*  aRC = nullptr);

  
  
  
  
  
  
  static void SyncFrameViewProperties(nsPresContext*  aPresContext,
                                      nsIFrame*        aFrame,
                                      nsStyleContext*  aStyleContext,
                                      nsView*         aView,
                                      uint32_t         aFlags = 0);

  








  static void SetSizeConstraints(nsPresContext* aPresContext,
                                 nsIWidget* aWidget,
                                 const nsSize& aMinSize,
                                 const nsSize& aMaxSize);

  
  void DoInlineIntrinsicWidth(nsRenderingContext *aRenderingContext,
                              InlineIntrinsicWidthData *aData,
                              nsLayoutUtils::IntrinsicWidthType aType);

  



  virtual nsSize ComputeAutoSize(nsRenderingContext *aRenderingContext,
                                 nsSize aCBSize, nscoord aAvailableWidth,
                                 nsSize aMargin, nsSize aBorder,
                                 nsSize aPadding, bool aShrinkWrap) MOZ_OVERRIDE;

  










  nsresult ReflowChild(nsIFrame*                      aKidFrame,
                       nsPresContext*                 aPresContext,
                       nsHTMLReflowMetrics&           aDesiredSize,
                       const nsHTMLReflowState&       aReflowState,
                       nscoord                        aX,
                       nscoord                        aY,
                       uint32_t                       aFlags,
                       nsReflowStatus&                aStatus,
                       nsOverflowContinuationTracker* aTracker = nullptr);

  
















  static nsresult FinishReflowChild(nsIFrame*                  aKidFrame,
                                    nsPresContext*             aPresContext,
                                    const nsHTMLReflowState*   aReflowState,
                                    const nsHTMLReflowMetrics& aDesiredSize,
                                    nscoord                    aX,
                                    nscoord                    aY,
                                    uint32_t                   aFlags);

  
  static void PositionChildViews(nsIFrame* aFrame);

  
  



































  friend class nsOverflowContinuationTracker;

  























  nsresult ReflowOverflowContainerChildren(nsPresContext*           aPresContext,
                                           const nsHTMLReflowState& aReflowState,
                                           nsOverflowAreas&         aOverflowRects,
                                           uint32_t                 aFlags,
                                           nsReflowStatus&          aStatus);

  



  virtual bool DrainSelfOverflowList();

  










  virtual nsresult StealFrame(nsPresContext* aPresContext,
                              nsIFrame*      aChild,
                              bool           aForceNormal = false);

  








  nsFrameList StealFramesAfter(nsIFrame* aChild);

  


  void DisplayOverflowContainers(nsDisplayListBuilder*   aBuilder,
                                 const nsRect&           aDirtyRect,
                                 const nsDisplayListSet& aLists);

  








  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  



  static void DestroyFrameList(void* aPropertyValue)
  {
    MOZ_ASSERT(false, "The owning frame should destroy its nsFrameList props");
  }

#define NS_DECLARE_FRAME_PROPERTY_FRAMELIST(prop)                     \
  NS_DECLARE_FRAME_PROPERTY(prop, nsContainerFrame::DestroyFrameList)

  NS_DECLARE_FRAME_PROPERTY_FRAMELIST(OverflowProperty)
  NS_DECLARE_FRAME_PROPERTY_FRAMELIST(OverflowContainersProperty)
  NS_DECLARE_FRAME_PROPERTY_FRAMELIST(ExcessOverflowContainersProperty)

protected:
  nsContainerFrame(nsStyleContext* aContext) : nsSplittableFrame(aContext) {}
  ~nsContainerFrame();

  





  void DestroyAbsoluteFrames(nsIFrame* aDestructRoot);

  








  void BuildDisplayListForNonBlockChildren(nsDisplayListBuilder*   aBuilder,
                                           const nsRect&           aDirtyRect,
                                           const nsDisplayListSet& aLists,
                                           uint32_t                aFlags = 0);

  



  void BuildDisplayListForInline(nsDisplayListBuilder*   aBuilder,
                                 const nsRect&           aDirtyRect,
                                 const nsDisplayListSet& aLists) {
    DisplayBorderBackgroundOutline(aBuilder, aLists);
    BuildDisplayListForNonBlockChildren(aBuilder, aDirtyRect, aLists,
                                        DISPLAY_CHILD_INLINE);
  }


  
  





  





  inline nsFrameList* GetOverflowFrames() const;

  







  inline nsFrameList* StealOverflowFrames();
  
  


  void SetOverflowFrames(nsPresContext*  aPresContext,
                         const nsFrameList& aOverflowFrames);

  


  inline void DestroyOverflowList(nsPresContext* aPresContext);

  








  bool MoveOverflowToChildList(nsPresContext* aPresContext);

  













  void PushChildren(nsPresContext*  aPresContext,
                    nsIFrame*       aFromChild,
                    nsIFrame*       aPrevSibling);

  
  




  



  nsFrameList* GetPropTableFrames(nsPresContext*                 aPresContext,
                                  const FramePropertyDescriptor* aProperty) const;

  



  nsFrameList* RemovePropTableFrames(nsPresContext*                 aPresContext,
                                     const FramePropertyDescriptor* aProperty);

  



  void SetPropTableFrames(nsPresContext*                 aPresContext,
                          nsFrameList*                   aFrameList,
                          const FramePropertyDescriptor* aProperty);

  




  void SafelyDestroyFrameListProp(nsIFrame* aDestructRoot,
                                  nsIPresShell* aPresShell,
                                  mozilla::FramePropertyTable* aPropTable,
                                  const FramePropertyDescriptor* aProp);

  

  nsFrameList mFrames;
};









#define IS_TRUE_OVERFLOW_CONTAINER(frame)                      \
  (  (frame->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER)  \
  && !( (frame->GetStateBits() & NS_FRAME_OUT_OF_FLOW) &&      \
        frame->IsAbsolutelyPositioned()  )  )




























class nsOverflowContinuationTracker {
public:
  











  nsOverflowContinuationTracker(nsPresContext*    aPresContext,
                                nsContainerFrame* aFrame,
                                bool              aWalkOOFFrames,
                                bool              aSkipOverflowContainerChildren = true);
  


















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
  

  bool mSkipOverflowContainerChildren;
  
  bool mWalkOOFFrames;
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

inline void
nsContainerFrame::DestroyOverflowList(nsPresContext* aPresContext)
{
  nsFrameList* list = RemovePropTableFrames(aPresContext, OverflowProperty());
  MOZ_ASSERT(list && list->IsEmpty());
  list->Delete(aPresContext->PresShell());
}

#endif 
