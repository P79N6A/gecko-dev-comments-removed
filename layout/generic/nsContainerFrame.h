






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
#define CRAZY_COORD (1000000*60)
#define CRAZY_SIZE(_x) (((_x) < -CRAZY_COORD) || ((_x) > CRAZY_COORD))
#endif




class nsContainerFrame : public nsSplittableFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS
  NS_DECL_QUERYFRAME_TARGET(nsContainerFrame)
  NS_DECL_QUERYFRAME

  
  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) override;
  virtual nsContainerFrame* GetContentInsertionFrame() override
  {
    return this;
  }

  virtual const nsFrameList& GetChildList(ChildListID aList) const override;
  virtual void GetChildLists(nsTArray<ChildList>* aLists) const override;
  virtual void DestroyFrom(nsIFrame* aDestructRoot) override;
  virtual void ChildIsDirty(nsIFrame* aChild) override;

  virtual bool IsLeaf() const override;
  virtual FrameSearchResult PeekOffsetNoAmount(bool aForward, int32_t* aOffset) override;
  virtual FrameSearchResult PeekOffsetCharacter(bool aForward, int32_t* aOffset,
                                     bool aRespectClusters = true) override;
  
#ifdef DEBUG_FRAME_DUMP
  void List(FILE* out = stderr, const char* aPrefix = "", uint32_t aFlags = 0) const override;
#endif  

  

  













  virtual void SetInitialChildList(ChildListID aListID,
                                   nsFrameList& aChildList);

  








  virtual void AppendFrames(ChildListID aListID, nsFrameList& aFrameList);

  









  virtual void InsertFrames(ChildListID  aListID,
                            nsIFrame*    aPrevFrame,
                            nsFrameList& aFrameList);

  








  virtual void RemoveFrame(ChildListID aListID, nsIFrame* aOldFrame);

  










  nsIFrame* CreateNextInFlow(nsIFrame* aFrame);

  





  virtual void DeleteNextInFlowChild(nsIFrame* aNextInFlow,
                                     bool      aDeletingEmptyFrames);

  



  static void CreateViewForFrame(nsIFrame* aFrame,
                                 bool aForce);

  
  static void PositionFrameView(nsIFrame* aKidFrame);

  static nsresult ReparentFrameView(nsIFrame* aChildFrame,
                                    nsIFrame* aOldParentFrame,
                                    nsIFrame* aNewParentFrame);

  static nsresult ReparentFrameViewList(const nsFrameList& aChildFrameList,
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

  
  void DoInlineIntrinsicISize(nsRenderingContext *aRenderingContext,
                              InlineIntrinsicISizeData *aData,
                              nsLayoutUtils::IntrinsicISizeType aType);

  



  virtual mozilla::LogicalSize
  ComputeAutoSize(nsRenderingContext *aRenderingContext,
                  mozilla::WritingMode aWritingMode,
                  const mozilla::LogicalSize& aCBSize,
                  nscoord aAvailableISize,
                  const mozilla::LogicalSize& aMargin,
                  const mozilla::LogicalSize& aBorder,
                  const mozilla::LogicalSize& aPadding,
                  bool aShrinkWrap) override;

  












  void ReflowChild(nsIFrame*                      aChildFrame,
                   nsPresContext*                 aPresContext,
                   nsHTMLReflowMetrics&           aDesiredSize,
                   const nsHTMLReflowState&       aReflowState,
                   const mozilla::WritingMode&    aWM,
                   const mozilla::LogicalPoint&   aPos,
                   nscoord                        aContainerWidth,
                   uint32_t                       aFlags,
                   nsReflowStatus&                aStatus,
                   nsOverflowContinuationTracker* aTracker = nullptr);

  


















  static void FinishReflowChild(nsIFrame*                    aKidFrame,
                                nsPresContext*               aPresContext,
                                const nsHTMLReflowMetrics&   aDesiredSize,
                                const nsHTMLReflowState*     aReflowState,
                                const mozilla::WritingMode&  aWM,
                                const mozilla::LogicalPoint& aPos,
                                nscoord                      aContainerWidth,
                                uint32_t                     aFlags);

  
  
  
  void ReflowChild(nsIFrame*                      aKidFrame,
                   nsPresContext*                 aPresContext,
                   nsHTMLReflowMetrics&           aDesiredSize,
                   const nsHTMLReflowState&       aReflowState,
                   nscoord                        aX,
                   nscoord                        aY,
                   uint32_t                       aFlags,
                   nsReflowStatus&                aStatus,
                   nsOverflowContinuationTracker* aTracker = nullptr);

  static void FinishReflowChild(nsIFrame*                  aKidFrame,
                                nsPresContext*             aPresContext,
                                const nsHTMLReflowMetrics& aDesiredSize,
                                const nsHTMLReflowState*   aReflowState,
                                nscoord                    aX,
                                nscoord                    aY,
                                uint32_t                   aFlags);

  static void PositionChildViews(nsIFrame* aFrame);

  
  



































  friend class nsOverflowContinuationTracker;

  























  void ReflowOverflowContainerChildren(nsPresContext*           aPresContext,
                                       const nsHTMLReflowState& aReflowState,
                                       nsOverflowAreas&         aOverflowRects,
                                       uint32_t                 aFlags,
                                       nsReflowStatus&          aStatus);

  



  virtual bool DrainSelfOverflowList() override;

  










  virtual nsresult StealFrame(nsIFrame* aChild,
                              bool      aForceNormal = false);

  








  nsFrameList StealFramesAfter(nsIFrame* aChild);

  


  void DisplayOverflowContainers(nsDisplayListBuilder*   aBuilder,
                                 const nsRect&           aDirtyRect,
                                 const nsDisplayListSet& aLists);

  








  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) override;

  



  static void DestroyFrameList(void* aPropertyValue)
  {
    MOZ_ASSERT(false, "The owning frame should destroy its nsFrameList props");
  }

  static void PlaceFrameView(nsIFrame* aFrame)
  {
    if (aFrame->HasView())
      nsContainerFrame::PositionFrameView(aFrame);
    else
      nsContainerFrame::PositionChildViews(aFrame);
  }

#define NS_DECLARE_FRAME_PROPERTY_FRAMELIST(prop)                     \
  NS_DECLARE_FRAME_PROPERTY(prop, nsContainerFrame::DestroyFrameList)

  NS_DECLARE_FRAME_PROPERTY_FRAMELIST(OverflowProperty)
  NS_DECLARE_FRAME_PROPERTY_FRAMELIST(OverflowContainersProperty)
  NS_DECLARE_FRAME_PROPERTY_FRAMELIST(ExcessOverflowContainersProperty)

protected:
  explicit nsContainerFrame(nsStyleContext* aContext) : nsSplittableFrame(aContext) {}
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
  
  


  void SetOverflowFrames(const nsFrameList& aOverflowFrames);

  


  inline void DestroyOverflowList();

  








  bool MoveOverflowToChildList();

  













  void PushChildren(nsIFrame* aFromChild, nsIFrame* aPrevSibling);

  
  



  struct ContinuationTraversingState
  {
    nsContainerFrame* mNextInFlow;
    explicit ContinuationTraversingState(nsContainerFrame* aFrame)
      : mNextInFlow(static_cast<nsContainerFrame*>(aFrame->GetNextInFlow()))
    { }
  };

  



  nsIFrame* GetNextInFlowChild(ContinuationTraversingState& aState,
                               bool* aIsInOverflow = nullptr);

  



  nsIFrame* PullNextInFlowChild(ContinuationTraversingState& aState);

  
  




  



  nsFrameList* GetPropTableFrames(const FramePropertyDescriptor* aProperty) const;

  



  nsFrameList* RemovePropTableFrames(const FramePropertyDescriptor* aProperty);

  



  void SetPropTableFrames(nsFrameList*                   aFrameList,
                          const FramePropertyDescriptor* aProperty);

  




  void SafelyDestroyFrameListProp(nsIFrame* aDestructRoot,
                                  nsIPresShell* aPresShell,
                                  mozilla::FramePropertyTable* aPropTable,
                                  const FramePropertyDescriptor* aProp);

  

  
  
  bool ResolvedOrientationIsVertical();

  

  nsFrameList mFrames;
};









#define IS_TRUE_OVERFLOW_CONTAINER(frame)                      \
  (  (frame->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER)  \
  && !( (frame->GetStateBits() & NS_FRAME_OUT_OF_FLOW) &&      \
        frame->IsAbsolutelyPositioned()  )  )




























class nsOverflowContinuationTracker {
public:
  











  nsOverflowContinuationTracker(nsContainerFrame* aFrame,
                                bool              aWalkOOFFrames,
                                bool              aSkipOverflowContainerChildren = true);
  


















  nsresult Insert(nsIFrame*       aOverflowCont,
                  nsReflowStatus& aReflowStatus);
  












  class MOZ_STACK_CLASS AutoFinish {
  public:
    AutoFinish(nsOverflowContinuationTracker* aTracker, nsIFrame* aChild)
      : mTracker(aTracker), mChild(aChild)
    {
      if (mTracker) mTracker->BeginFinish(mChild);
    }
    ~AutoFinish() 
    {
      if (mTracker) mTracker->EndFinish(mChild);
    }
  private:
    nsOverflowContinuationTracker* mTracker;
    nsIFrame* mChild;
  };

  








  void Skip(nsIFrame* aChild, nsReflowStatus& aReflowStatus)
  {
    NS_PRECONDITION(aChild, "null ptr");
    if (aChild == mSentry) {
      StepForward();
      NS_MergeReflowStatusInto(&aReflowStatus, NS_FRAME_OVERFLOW_INCOMPLETE);
    }
  }

private:

  


  void BeginFinish(nsIFrame* aChild);
  void EndFinish(nsIFrame* aChild);

  void SetupOverflowContList();
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
nsContainerFrame::DestroyOverflowList()
{
  nsFrameList* list = RemovePropTableFrames(OverflowProperty());
  MOZ_ASSERT(list && list->IsEmpty());
  list->Delete(PresContext()->PresShell());
}

#endif
