






#ifndef nsFrame_h___
#define nsFrame_h___

#include "nsBox.h"
#include "nsRect.h"
#include "nsString.h"
#include "prlog.h"

#include "nsIPresShell.h"
#include "nsFrameSelection.h"
#include "nsHTMLReflowState.h"
#include "nsHTMLReflowMetrics.h"
#include "nsHTMLParts.h"







#define NS_FRAME_TRACE_CALLS        0x1
#define NS_FRAME_TRACE_PUSH_PULL    0x2
#define NS_FRAME_TRACE_CHILD_REFLOW 0x4
#define NS_FRAME_TRACE_NEW_FRAMES   0x8

#define NS_FRAME_LOG_TEST(_lm,_bit) (PRIntn((_lm)->level) & (_bit))

#ifdef DEBUG
#define NS_FRAME_LOG(_bit,_args)                                \
  PR_BEGIN_MACRO                                                \
    if (NS_FRAME_LOG_TEST(nsFrame::GetLogModuleInfo(),_bit)) {  \
      PR_LogPrint _args;                                        \
    }                                                           \
  PR_END_MACRO
#else
#define NS_FRAME_LOG(_bit,_args)
#endif


#ifdef DEBUG
#define NS_FRAME_TRACE_IN(_method) Trace(_method, true)

#define NS_FRAME_TRACE_OUT(_method) Trace(_method, false)


#define NS_FRAME_TRACE_MSG(_bit,_args)                          \
  PR_BEGIN_MACRO                                                \
    if (NS_FRAME_LOG_TEST(nsFrame::GetLogModuleInfo(),_bit)) {  \
      TraceMsg _args;                                           \
    }                                                           \
  PR_END_MACRO

#define NS_FRAME_TRACE(_bit,_args)                              \
  PR_BEGIN_MACRO                                                \
    if (NS_FRAME_LOG_TEST(nsFrame::GetLogModuleInfo(),_bit)) {  \
      TraceMsg _args;                                           \
    }                                                           \
  PR_END_MACRO

#define NS_FRAME_TRACE_REFLOW_IN(_method) Trace(_method, true)

#define NS_FRAME_TRACE_REFLOW_OUT(_method, _status) \
  Trace(_method, false, _status)

#else
#define NS_FRAME_TRACE(_bits,_args)
#define NS_FRAME_TRACE_IN(_method)
#define NS_FRAME_TRACE_OUT(_method)
#define NS_FRAME_TRACE_MSG(_bits,_args)
#define NS_FRAME_TRACE_REFLOW_IN(_method)
#define NS_FRAME_TRACE_REFLOW_OUT(_method, _status)
#endif







#define NS_DECL_FRAMEARENA_HELPERS                                \
  NS_MUST_OVERRIDE void* operator new(size_t, nsIPresShell*);     \
  virtual NS_MUST_OVERRIDE nsQueryFrame::FrameIID GetFrameId();

#define NS_IMPL_FRAMEARENA_HELPERS(class)                         \
  void* class::operator new(size_t sz, nsIPresShell* aShell)      \
  { return aShell->AllocateFrame(nsQueryFrame::class##_id, sz); } \
  nsQueryFrame::FrameIID class::GetFrameId()                      \
  { return nsQueryFrame::class##_id; }



struct nsBoxLayoutMetrics;
class nsDisplayBackground;








class nsFrame : public nsBox
{
public:
  



  friend nsIFrame* NS_NewEmptyFrame(nsIPresShell* aShell,
                                    nsStyleContext* aContext);

private:
  
  void* operator new(size_t sz) CPP_THROW_NEW;

protected:
  
  
  
  
  
  
  
  
  
  
  void operator delete(void* aPtr, size_t sz);

public:

  
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  
  NS_IMETHOD  Init(nsIContent*      aContent,
                   nsIFrame*        aParent,
                   nsIFrame*        asPrevInFlow);
  NS_IMETHOD  SetInitialChildList(ChildListID        aListID,
                                  nsFrameList&       aChildList);
  NS_IMETHOD  AppendFrames(ChildListID     aListID,
                           nsFrameList&    aFrameList);
  NS_IMETHOD  InsertFrames(ChildListID     aListID,
                           nsIFrame*       aPrevFrame,
                           nsFrameList&    aFrameList);
  NS_IMETHOD  RemoveFrame(ChildListID     aListID,
                          nsIFrame*       aOldFrame);
  virtual void DestroyFrom(nsIFrame* aDestructRoot);
  virtual nsStyleContext* GetAdditionalStyleContext(PRInt32 aIndex) const;
  virtual void SetAdditionalStyleContext(PRInt32 aIndex,
                                         nsStyleContext* aStyleContext);
  virtual void SetParent(nsIFrame* aParent);
  virtual nscoord GetBaseline() const;
  virtual const nsFrameList& GetChildList(ChildListID aListID) const;
  virtual void GetChildLists(nsTArray<ChildList>* aLists) const;

  NS_IMETHOD  HandleEvent(nsPresContext* aPresContext, 
                          nsGUIEvent*     aEvent,
                          nsEventStatus*  aEventStatus);
  NS_IMETHOD  GetContentForEvent(nsEvent* aEvent,
                                 nsIContent** aContent);
  NS_IMETHOD  GetCursor(const nsPoint&    aPoint,
                        nsIFrame::Cursor& aCursor);

  NS_IMETHOD  GetPointFromOffset(PRInt32                inOffset,
                                 nsPoint*               outPoint);

  NS_IMETHOD  GetChildFrameContainingOffset(PRInt32     inContentOffset,
                                 bool                   inHint,
                                 PRInt32*               outFrameContentOffset,
                                 nsIFrame*              *outChildFrame);

  static nsresult  GetNextPrevLineFromeBlockFrame(nsPresContext* aPresContext,
                                        nsPeekOffsetStruct *aPos, 
                                        nsIFrame *aBlockFrame, 
                                        PRInt32 aLineStart, 
                                        PRInt8 aOutSideLimit
                                        );

  NS_IMETHOD  CharacterDataChanged(CharacterDataChangeInfo* aInfo);
  NS_IMETHOD  AttributeChanged(PRInt32         aNameSpaceID,
                               nsIAtom*        aAttribute,
                               PRInt32         aModType);
  virtual nsSplittableType GetSplittableType() const;
  virtual nsIFrame* GetPrevContinuation() const;
  NS_IMETHOD  SetPrevContinuation(nsIFrame*);
  virtual nsIFrame* GetNextContinuation() const;
  NS_IMETHOD  SetNextContinuation(nsIFrame*);
  virtual nsIFrame* GetPrevInFlowVirtual() const;
  NS_IMETHOD  SetPrevInFlow(nsIFrame*);
  virtual nsIFrame* GetNextInFlowVirtual() const;
  NS_IMETHOD  SetNextInFlow(nsIFrame*);
  NS_IMETHOD  GetOffsetFromView(nsPoint& aOffset, nsIView** aView) const;
  virtual nsIAtom* GetType() const;

  NS_IMETHOD  IsSelectable(bool* aIsSelectable, PRUint8* aSelectStyle) const;

  NS_IMETHOD  GetSelectionController(nsPresContext *aPresContext, nsISelectionController **aSelCon);

  virtual bool PeekOffsetNoAmount(bool aForward, PRInt32* aOffset);
  virtual bool PeekOffsetCharacter(bool aForward, PRInt32* aOffset,
                                     bool aRespectClusters = true);
  virtual bool PeekOffsetWord(bool aForward, bool aWordSelectEatSpace, bool aIsKeyboardSelect,
                                PRInt32* aOffset, PeekWordState *aState);
  







  bool BreakWordBetweenPunctuation(const PeekWordState* aState,
                                     bool aForward,
                                     bool aPunctAfter, bool aWhitespaceAfter,
                                     bool aIsKeyboardSelect);

  NS_IMETHOD  CheckVisibility(nsPresContext* aContext, PRInt32 aStartIndex, PRInt32 aEndIndex, bool aRecurse, bool *aFinished, bool *_retval);

  NS_IMETHOD  GetOffsets(PRInt32 &aStart, PRInt32 &aEnd) const;
  virtual void ChildIsDirty(nsIFrame* aChild);

#ifdef ACCESSIBILITY
  virtual already_AddRefed<Accessible> CreateAccessible();
#endif

  virtual nsIFrame* GetParentStyleContextFrame() const {
    return DoGetParentStyleContextFrame();
  }

  






  nsIFrame* DoGetParentStyleContextFrame() const;

  virtual bool IsEmpty();
  virtual bool IsSelfEmpty();

  virtual void MarkIntrinsicWidthsDirty();
  virtual nscoord GetMinWidth(nsRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsRenderingContext *aRenderingContext);
  virtual void AddInlineMinWidth(nsRenderingContext *aRenderingContext,
                                 InlineMinWidthData *aData);
  virtual void AddInlinePrefWidth(nsRenderingContext *aRenderingContext,
                                  InlinePrefWidthData *aData);
  virtual IntrinsicWidthOffsetData
    IntrinsicWidthOffsets(nsRenderingContext* aRenderingContext);
  virtual IntrinsicSize GetIntrinsicSize();
  virtual nsSize GetIntrinsicRatio();

  virtual nsSize ComputeSize(nsRenderingContext *aRenderingContext,
                             nsSize aCBSize, nscoord aAvailableWidth,
                             nsSize aMargin, nsSize aBorder, nsSize aPadding,
                             PRUint32 aFlags) MOZ_OVERRIDE;

  
  
  nsRect ComputeSimpleTightBounds(gfxContext* aContext) const;
  
  














  virtual nsSize ComputeAutoSize(nsRenderingContext *aRenderingContext,
                                 nsSize aCBSize, nscoord aAvailableWidth,
                                 nsSize aMargin, nsSize aBorder,
                                 nsSize aPadding, bool aShrinkWrap);

  



  nscoord ShrinkWidthToFit(nsRenderingContext *aRenderingContext,
                           nscoord aWidthInCB);

  NS_IMETHOD  WillReflow(nsPresContext* aPresContext);
  NS_IMETHOD  Reflow(nsPresContext*          aPresContext,
                     nsHTMLReflowMetrics&     aDesiredSize,
                     const nsHTMLReflowState& aReflowState,
                     nsReflowStatus&          aStatus);
  NS_IMETHOD  DidReflow(nsPresContext*           aPresContext,
                        const nsHTMLReflowState*  aReflowState,
                        nsDidReflowStatus         aStatus);
  void ReflowAbsoluteFrames(nsPresContext*           aPresContext,
                            nsHTMLReflowMetrics&     aDesiredSize,
                            const nsHTMLReflowState& aReflowState,
                            nsReflowStatus&          aStatus);
  void FinishReflowWithAbsoluteFrames(nsPresContext*           aPresContext,
                                      nsHTMLReflowMetrics&     aDesiredSize,
                                      const nsHTMLReflowState& aReflowState,
                                      nsReflowStatus&          aStatus);
  void DestroyAbsoluteFrames(nsIFrame* aDestructRoot);
  virtual bool CanContinueTextRun() const;

  virtual bool UpdateOverflow();

  
  
  
  
  NS_IMETHOD HandlePress(nsPresContext* aPresContext,
                         nsGUIEvent *    aEvent,
                         nsEventStatus*  aEventStatus);

  NS_IMETHOD HandleMultiplePress(nsPresContext* aPresContext,
                         nsGUIEvent *    aEvent,
                         nsEventStatus*  aEventStatus,
                         bool            aControlHeld);

  NS_IMETHOD HandleDrag(nsPresContext* aPresContext,
                        nsGUIEvent *    aEvent,
                        nsEventStatus*  aEventStatus);

  NS_IMETHOD HandleRelease(nsPresContext* aPresContext,
                           nsGUIEvent *    aEvent,
                           nsEventStatus*  aEventStatus);

  NS_IMETHOD PeekBackwardAndForward(nsSelectionAmount aAmountBack,
                                    nsSelectionAmount aAmountForward,
                                    PRInt32 aStartPos,
                                    nsPresContext* aPresContext,
                                    bool aJumpLines,
                                    bool aMultipleSelection);


  
  
  virtual ContentOffsets CalcContentOffsetsFromFramePoint(nsPoint aPoint);

  
  virtual nsSize GetPrefSize(nsBoxLayoutState& aBoxLayoutState);
  virtual nsSize GetMinSize(nsBoxLayoutState& aBoxLayoutState);
  virtual nsSize GetMaxSize(nsBoxLayoutState& aBoxLayoutState);
  virtual nscoord GetFlex(nsBoxLayoutState& aBoxLayoutState);
  virtual nscoord GetBoxAscent(nsBoxLayoutState& aBoxLayoutState);

  
  
  virtual bool ComputesOwnOverflowArea() { return true; }

  
  

  







  void CheckInvalidateSizeChange(nsHTMLReflowMetrics&     aNewDesiredSize);

  
  
  
  
  
  bool IsFrameTreeTooDeep(const nsHTMLReflowState& aReflowState,
                            nsHTMLReflowMetrics& aMetrics,
                            nsReflowStatus& aStatus);

  
  
  void ConsiderChildOverflow(nsOverflowAreas& aOverflowAreas,
                             nsIFrame* aChildFrame);

  virtual const void* GetStyleDataExternal(nsStyleStructID aSID) const;


#ifdef DEBUG
  





  void Trace(const char* aMethod, bool aEnter);
  void Trace(const char* aMethod, bool aEnter, nsReflowStatus aStatus);
  void TraceMsg(const char* fmt, ...);

  
  
  static void VerifyDirtyBitSet(const nsFrameList& aFrameList);

  
  
  static PRInt32 ContentIndexInContainer(const nsIFrame* aFrame);

  static void IndentBy(FILE* out, PRInt32 aIndent) {
    while (--aIndent >= 0) fputs("  ", out);
  }
  
  void ListTag(FILE* out) const {
    ListTag(out, this);
  }

  static void ListTag(FILE* out, const nsIFrame* aFrame) {
    nsAutoString tmp;
    aFrame->GetFrameName(tmp);
    fputs(NS_LossyConvertUTF16toASCII(tmp).get(), out);
    fprintf(out, "@%p", static_cast<const void*>(aFrame));
  }

  static void XMLQuote(nsString& aString);

  








  virtual void DumpBaseRegressionData(nsPresContext* aPresContext, FILE* out, PRInt32 aIndent);
  
  nsresult MakeFrameName(const nsAString& aKind, nsAString& aResult) const;

  
  static void* DisplayReflowEnter(nsPresContext*          aPresContext,
                                  nsIFrame*                aFrame,
                                  const nsHTMLReflowState& aReflowState);
  static void* DisplayLayoutEnter(nsIFrame* aFrame);
  static void* DisplayIntrinsicWidthEnter(nsIFrame* aFrame,
                                          const char* aType);
  static void* DisplayIntrinsicSizeEnter(nsIFrame* aFrame,
                                         const char* aType);
  static void  DisplayReflowExit(nsPresContext*      aPresContext,
                                 nsIFrame*            aFrame,
                                 nsHTMLReflowMetrics& aMetrics,
                                 PRUint32             aStatus,
                                 void*                aFrameTreeNode);
  static void  DisplayLayoutExit(nsIFrame* aFrame,
                                 void* aFrameTreeNode);
  static void  DisplayIntrinsicWidthExit(nsIFrame* aFrame,
                                         const char* aType,
                                         nscoord aResult,
                                         void* aFrameTreeNode);
  static void  DisplayIntrinsicSizeExit(nsIFrame* aFrame,
                                        const char* aType,
                                        nsSize aResult,
                                        void* aFrameTreeNode);

  static void DisplayReflowStartup();
  static void DisplayReflowShutdown();
#endif

  static void ShutdownLayerActivityTimer();

  









  nsresult DisplayBackgroundUnconditional(nsDisplayListBuilder*   aBuilder,
                                          const nsDisplayListSet& aLists,
                                          bool aForceBackground,
                                          nsDisplayBackground** aBackground);
  








  nsresult DisplayBorderBackgroundOutline(nsDisplayListBuilder*   aBuilder,
                                          const nsDisplayListSet& aLists,
                                          bool aForceBackground = false);
  


  nsresult DisplayOutlineUnconditional(nsDisplayListBuilder*   aBuilder,
                                       const nsDisplayListSet& aLists);
  



  nsresult DisplayOutline(nsDisplayListBuilder*   aBuilder,
                          const nsDisplayListSet& aLists);

  








  static nsIFrame*
  CorrectStyleParentFrame(nsIFrame* aProspectiveParent, nsIAtom* aChildPseudo);

protected:
  
  nsFrame(nsStyleContext* aContext);
  virtual ~nsFrame();

  





  nsresult DisplaySelectionOverlay(nsDisplayListBuilder* aBuilder,
      nsDisplayList* aList, PRUint16 aContentType = nsISelectionDisplay::DISPLAY_FRAMES);

  PRInt16 DisplaySelection(nsPresContext* aPresContext, bool isOkToTurnOn = false);
  
  
  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext);

public:
  
  
  static void GetLastLeaf(nsPresContext* aPresContext, nsIFrame **aFrame);
  static void GetFirstLeaf(nsPresContext* aPresContext, nsIFrame **aFrame);

  
  
  
  
  static PRInt32 GetLineNumber(nsIFrame *aFrame,
                               bool aLockScroll,
                               nsIFrame** aContainingBlock = nsnull);

  


  static bool ApplyOverflowClipping(const nsIFrame* aFrame,
                                    const nsStyleDisplay* aDisp)
  {
    
    if (NS_UNLIKELY(aDisp->mOverflowX == NS_STYLE_OVERFLOW_CLIP)) {
      return true;
    }

    
    if (aDisp->mOverflowX == NS_STYLE_OVERFLOW_HIDDEN &&
        aDisp->mOverflowY == NS_STYLE_OVERFLOW_HIDDEN) {
      
      nsIAtom* type = aFrame->GetType();
      if (type == nsGkAtoms::tableFrame ||
          type == nsGkAtoms::tableCellFrame ||
          type == nsGkAtoms::bcTableCellFrame) {
        return true;
      }
    }
    
    
    
    return
      (aFrame->GetStateBits() & NS_BLOCK_CLIP_PAGINATED_OVERFLOW) != 0 &&
      aFrame->PresContext()->IsPaginated() &&
      aFrame->GetType() == nsGkAtoms::blockFrame;
  }

protected:

  
  
  
  
  
  
  
  
  NS_IMETHOD GetDataForTableSelection(const nsFrameSelection *aFrameSelection,
                                      nsIPresShell *aPresShell, nsMouseEvent *aMouseEvent, 
                                      nsIContent **aParentContent, PRInt32 *aContentOffset, 
                                      PRInt32 *aTarget);

  
  static void FillCursorInformationFromStyle(const nsStyleUserInterface* ui,
                                             nsIFrame::Cursor& aCursor);
  NS_IMETHOD DoLayout(nsBoxLayoutState& aBoxLayoutState);

#ifdef DEBUG_LAYOUT
  virtual void GetBoxName(nsAutoString& aName);
#endif

  void InitBoxMetrics(bool aClear);
  nsBoxLayoutMetrics* BoxMetrics() const;

  
  void FireDOMEvent(const nsAString& aDOMEventName, nsIContent *aContent = nsnull);

private:
  nsresult BoxReflow(nsBoxLayoutState& aState,
                     nsPresContext*    aPresContext,
                     nsHTMLReflowMetrics&     aDesiredSize,
                     nsRenderingContext* aRenderingContext,
                     nscoord aX,
                     nscoord aY,
                     nscoord aWidth,
                     nscoord aHeight,
                     bool aMoveFrame = true);

  NS_IMETHODIMP RefreshSizeCache(nsBoxLayoutState& aState);

  virtual nsILineIterator* GetLineIterator();

#ifdef DEBUG
public:
  

  NS_IMETHOD  List(FILE* out, PRInt32 aIndent) const;
  



  static void RootFrameList(nsPresContext* aPresContext,
                            FILE* out, PRInt32 aIndent);

  static void DumpFrameTree(nsIFrame* aFrame);

  



  NS_IMETHOD  GetFrameName(nsAString& aResult) const;
  



  NS_IMETHOD_(nsFrameState)  GetDebugStateBits() const;
  







  NS_IMETHOD  DumpRegressionData(nsPresContext* aPresContext,
                                 FILE* out, PRInt32 aIndent);

  





  static bool GetVerifyStyleTreeEnable();

  


  static void SetVerifyStyleTreeEnable(bool aEnabled);

  






  static PRLogModuleInfo* GetLogModuleInfo();

  
  static void ShowFrameBorders(bool aEnable);
  static bool GetShowFrameBorders();

  
  static void ShowEventTargetFrameBorder(bool aEnable);
  static bool GetShowEventTargetFrameBorder();

#endif
#ifdef MOZ_DUMP_PAINTING
public:

  static void PrintDisplayList(nsDisplayListBuilder* aBuilder,
                               const nsDisplayList& aList,
                               FILE* aFile = stdout);

#endif
};


#ifdef DEBUG

  struct DR_cookie {
    DR_cookie(nsPresContext*          aPresContext,
              nsIFrame*                aFrame, 
              const nsHTMLReflowState& aReflowState,
              nsHTMLReflowMetrics&     aMetrics,
              nsReflowStatus&          aStatus);     
    ~DR_cookie();
    void Change() const;

    nsPresContext*          mPresContext;
    nsIFrame*                mFrame;
    const nsHTMLReflowState& mReflowState;
    nsHTMLReflowMetrics&     mMetrics;
    nsReflowStatus&          mStatus;    
    void*                    mValue;
  };

  struct DR_layout_cookie {
    DR_layout_cookie(nsIFrame* aFrame);
    ~DR_layout_cookie();

    nsIFrame* mFrame;
    void* mValue;
  };
  
  struct DR_intrinsic_width_cookie {
    DR_intrinsic_width_cookie(nsIFrame* aFrame, const char* aType,
                              nscoord& aResult);
    ~DR_intrinsic_width_cookie();

    nsIFrame* mFrame;
    const char* mType;
    nscoord& mResult;
    void* mValue;
  };
  
  struct DR_intrinsic_size_cookie {
    DR_intrinsic_size_cookie(nsIFrame* aFrame, const char* aType,
                             nsSize& aResult);
    ~DR_intrinsic_size_cookie();

    nsIFrame* mFrame;
    const char* mType;
    nsSize& mResult;
    void* mValue;
  };

  struct DR_init_constraints_cookie {
    DR_init_constraints_cookie(nsIFrame* aFrame, nsHTMLReflowState* aState,
                               nscoord aCBWidth, nscoord aCBHeight,
                               const nsMargin* aBorder,
                               const nsMargin* aPadding);
    ~DR_init_constraints_cookie();

    nsIFrame* mFrame;
    nsHTMLReflowState* mState;
    void* mValue;
  };

  struct DR_init_offsets_cookie {
    DR_init_offsets_cookie(nsIFrame* aFrame, nsCSSOffsetState* aState,
                           nscoord aCBWidth, const nsMargin* aBorder,
                           const nsMargin* aPadding);
    ~DR_init_offsets_cookie();

    nsIFrame* mFrame;
    nsCSSOffsetState* mState;
    void* mValue;
  };

  struct DR_init_type_cookie {
    DR_init_type_cookie(nsIFrame* aFrame, nsHTMLReflowState* aState);
    ~DR_init_type_cookie();

    nsIFrame* mFrame;
    nsHTMLReflowState* mState;
    void* mValue;
  };

#define DISPLAY_REFLOW(dr_pres_context, dr_frame, dr_rf_state, dr_rf_metrics, dr_rf_status) \
  DR_cookie dr_cookie(dr_pres_context, dr_frame, dr_rf_state, dr_rf_metrics, dr_rf_status); 
#define DISPLAY_REFLOW_CHANGE() \
  dr_cookie.Change();
#define DISPLAY_LAYOUT(dr_frame) \
  DR_layout_cookie dr_cookie(dr_frame); 
#define DISPLAY_MIN_WIDTH(dr_frame, dr_result) \
  DR_intrinsic_width_cookie dr_cookie(dr_frame, "Min", dr_result)
#define DISPLAY_PREF_WIDTH(dr_frame, dr_result) \
  DR_intrinsic_width_cookie dr_cookie(dr_frame, "Pref", dr_result)
#define DISPLAY_PREF_SIZE(dr_frame, dr_result) \
  DR_intrinsic_size_cookie dr_cookie(dr_frame, "Pref", dr_result)
#define DISPLAY_MIN_SIZE(dr_frame, dr_result) \
  DR_intrinsic_size_cookie dr_cookie(dr_frame, "Min", dr_result)
#define DISPLAY_MAX_SIZE(dr_frame, dr_result) \
  DR_intrinsic_size_cookie dr_cookie(dr_frame, "Max", dr_result)
#define DISPLAY_INIT_CONSTRAINTS(dr_frame, dr_state, dr_cbw, dr_cbh,       \
                                 dr_bdr, dr_pad)                           \
  DR_init_constraints_cookie dr_cookie(dr_frame, dr_state, dr_cbw, dr_cbh, \
                                       dr_bdr, dr_pad)
#define DISPLAY_INIT_OFFSETS(dr_frame, dr_state, dr_cbw, dr_bdr, dr_pad)  \
  DR_init_offsets_cookie dr_cookie(dr_frame, dr_state, dr_cbw, dr_bdr, dr_pad)
#define DISPLAY_INIT_TYPE(dr_frame, dr_result) \
  DR_init_type_cookie dr_cookie(dr_frame, dr_result)

#else

#define DISPLAY_REFLOW(dr_pres_context, dr_frame, dr_rf_state, dr_rf_metrics, dr_rf_status) 
#define DISPLAY_REFLOW_CHANGE() 
#define DISPLAY_LAYOUT(dr_frame) PR_BEGIN_MACRO PR_END_MACRO
#define DISPLAY_MIN_WIDTH(dr_frame, dr_result) PR_BEGIN_MACRO PR_END_MACRO
#define DISPLAY_PREF_WIDTH(dr_frame, dr_result) PR_BEGIN_MACRO PR_END_MACRO
#define DISPLAY_PREF_SIZE(dr_frame, dr_result) PR_BEGIN_MACRO PR_END_MACRO
#define DISPLAY_MIN_SIZE(dr_frame, dr_result) PR_BEGIN_MACRO PR_END_MACRO
#define DISPLAY_MAX_SIZE(dr_frame, dr_result) PR_BEGIN_MACRO PR_END_MACRO
#define DISPLAY_INIT_CONSTRAINTS(dr_frame, dr_state, dr_cbw, dr_cbh,       \
                                 dr_bdr, dr_pad)                           \
  PR_BEGIN_MACRO PR_END_MACRO
#define DISPLAY_INIT_OFFSETS(dr_frame, dr_state, dr_cbw, dr_bdr, dr_pad)  \
  PR_BEGIN_MACRO PR_END_MACRO
#define DISPLAY_INIT_TYPE(dr_frame, dr_result) PR_BEGIN_MACRO PR_END_MACRO

#endif



#define ENSURE_TRUE(x)                                        \
  PR_BEGIN_MACRO                                              \
    if (!(x)) {                                               \
       NS_WARNING("ENSURE_TRUE(" #x ") failed");              \
       return;                                                \
    }                                                         \
  PR_END_MACRO
#endif 
