






#ifndef nsFrame_h___
#define nsFrame_h___

#include "mozilla/Attributes.h"
#include "mozilla/EventForwards.h"
#include "mozilla/Likely.h"
#include "nsBox.h"
#include "prlog.h"

#include "nsIPresShell.h"
#include "nsHTMLReflowState.h"
#include "nsHTMLParts.h"
#include "nsISelectionDisplay.h"







#define NS_FRAME_TRACE_CALLS        0x1
#define NS_FRAME_TRACE_PUSH_PULL    0x2
#define NS_FRAME_TRACE_CHILD_REFLOW 0x4
#define NS_FRAME_TRACE_NEW_FRAMES   0x8

#define NS_FRAME_LOG_TEST(_lm,_bit) (int((_lm)->level) & (_bit))

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
  void* operator new(size_t, nsIPresShell*) MOZ_MUST_OVERRIDE;    \
  virtual nsQueryFrame::FrameIID GetFrameId() MOZ_MUST_OVERRIDE;

#define NS_IMPL_FRAMEARENA_HELPERS(class)                         \
  void* class::operator new(size_t sz, nsIPresShell* aShell)      \
  { return aShell->AllocateFrame(nsQueryFrame::class##_id, sz); } \
  nsQueryFrame::FrameIID class::GetFrameId()                      \
  { return nsQueryFrame::class##_id; }



struct nsBoxLayoutMetrics;
class nsDisplayBackgroundImage;
struct nsRect;








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

  
  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) MOZ_OVERRIDE;
  virtual void DestroyFrom(nsIFrame* aDestructRoot) MOZ_OVERRIDE;
  virtual nsStyleContext* GetAdditionalStyleContext(int32_t aIndex) const MOZ_OVERRIDE;
  virtual void SetAdditionalStyleContext(int32_t aIndex,
                                         nsStyleContext* aStyleContext) MOZ_OVERRIDE;
  virtual nscoord GetLogicalBaseline(mozilla::WritingMode aWritingMode) const MOZ_OVERRIDE;
  virtual const nsFrameList& GetChildList(ChildListID aListID) const MOZ_OVERRIDE;
  virtual void GetChildLists(nsTArray<ChildList>* aLists) const MOZ_OVERRIDE;

  virtual nsresult  HandleEvent(nsPresContext* aPresContext, 
                                mozilla::WidgetGUIEvent* aEvent,
                                nsEventStatus* aEventStatus) MOZ_OVERRIDE;
  virtual nsresult  GetContentForEvent(mozilla::WidgetEvent* aEvent,
                                       nsIContent** aContent) MOZ_OVERRIDE;
  virtual nsresult  GetCursor(const nsPoint&    aPoint,
                              nsIFrame::Cursor& aCursor) MOZ_OVERRIDE;

  virtual nsresult  GetPointFromOffset(int32_t  inOffset,
                                       nsPoint* outPoint) MOZ_OVERRIDE;

  virtual nsresult  GetChildFrameContainingOffset(int32_t    inContentOffset,
                                                  bool       inHint,
                                                  int32_t*   outFrameContentOffset,
                                                  nsIFrame** outChildFrame) MOZ_OVERRIDE;

  static nsresult  GetNextPrevLineFromeBlockFrame(nsPresContext* aPresContext,
                                        nsPeekOffsetStruct *aPos, 
                                        nsIFrame *aBlockFrame, 
                                        int32_t aLineStart, 
                                        int8_t aOutSideLimit
                                        );

  virtual nsresult  CharacterDataChanged(CharacterDataChangeInfo* aInfo) MOZ_OVERRIDE;
  virtual nsresult  AttributeChanged(int32_t         aNameSpaceID,
                                     nsIAtom*        aAttribute,
                                     int32_t         aModType) MOZ_OVERRIDE;
  virtual nsSplittableType GetSplittableType() const MOZ_OVERRIDE;
  virtual nsIFrame* GetPrevContinuation() const MOZ_OVERRIDE;
  virtual void SetPrevContinuation(nsIFrame*) MOZ_OVERRIDE;
  virtual nsIFrame* GetNextContinuation() const MOZ_OVERRIDE;
  virtual void SetNextContinuation(nsIFrame*) MOZ_OVERRIDE;
  virtual nsIFrame* GetPrevInFlowVirtual() const MOZ_OVERRIDE;
  virtual void SetPrevInFlow(nsIFrame*) MOZ_OVERRIDE;
  virtual nsIFrame* GetNextInFlowVirtual() const MOZ_OVERRIDE;
  virtual void SetNextInFlow(nsIFrame*) MOZ_OVERRIDE;
  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

  virtual nsresult  IsSelectable(bool* aIsSelectable, uint8_t* aSelectStyle) const MOZ_OVERRIDE;

  virtual nsresult  GetSelectionController(nsPresContext *aPresContext, nsISelectionController **aSelCon) MOZ_OVERRIDE;

  virtual FrameSearchResult PeekOffsetNoAmount(bool aForward, int32_t* aOffset) MOZ_OVERRIDE;
  virtual FrameSearchResult PeekOffsetCharacter(bool aForward, int32_t* aOffset,
                                     bool aRespectClusters = true) MOZ_OVERRIDE;
  virtual FrameSearchResult PeekOffsetWord(bool aForward, bool aWordSelectEatSpace, bool aIsKeyboardSelect,
                                int32_t* aOffset, PeekWordState *aState) MOZ_OVERRIDE;
  







  bool BreakWordBetweenPunctuation(const PeekWordState* aState,
                                     bool aForward,
                                     bool aPunctAfter, bool aWhitespaceAfter,
                                     bool aIsKeyboardSelect);

  virtual nsresult  CheckVisibility(nsPresContext* aContext, int32_t aStartIndex, int32_t aEndIndex, bool aRecurse, bool *aFinished, bool *_retval) MOZ_OVERRIDE;

  virtual nsresult  GetOffsets(int32_t &aStart, int32_t &aEnd) const MOZ_OVERRIDE;
  virtual void ChildIsDirty(nsIFrame* aChild) MOZ_OVERRIDE;

#ifdef ACCESSIBILITY
  virtual mozilla::a11y::AccType AccessibleType() MOZ_OVERRIDE;
#endif

  virtual nsIFrame* GetParentStyleContextFrame() const MOZ_OVERRIDE {
    return DoGetParentStyleContextFrame();
  }

  






  nsIFrame* DoGetParentStyleContextFrame() const;

  virtual bool IsEmpty() MOZ_OVERRIDE;
  virtual bool IsSelfEmpty() MOZ_OVERRIDE;

  virtual void MarkIntrinsicISizesDirty() MOZ_OVERRIDE;
  virtual nscoord GetMinISize(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;
  virtual nscoord GetPrefISize(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;
  virtual void AddInlineMinISize(nsRenderingContext *aRenderingContext,
                                 InlineMinISizeData *aData) MOZ_OVERRIDE;
  virtual void AddInlinePrefISize(nsRenderingContext *aRenderingContext,
                                  InlinePrefISizeData *aData) MOZ_OVERRIDE;
  virtual IntrinsicISizeOffsetData
    IntrinsicISizeOffsets(nsRenderingContext* aRenderingContext) MOZ_OVERRIDE;
  virtual mozilla::IntrinsicSize GetIntrinsicSize() MOZ_OVERRIDE;
  virtual nsSize GetIntrinsicRatio() MOZ_OVERRIDE;

  virtual mozilla::LogicalSize
  ComputeSize(nsRenderingContext *aRenderingContext,
              mozilla::WritingMode aWritingMode,
              const mozilla::LogicalSize& aCBSize,
              nscoord aAvailableISize,
              const mozilla::LogicalSize& aMargin,
              const mozilla::LogicalSize& aBorder,
              const mozilla::LogicalSize& aPadding,
              uint32_t aFlags) MOZ_OVERRIDE;

  
  
  nsRect ComputeSimpleTightBounds(gfxContext* aContext) const;
  
  














  virtual nsSize ComputeAutoSize(nsRenderingContext *aRenderingContext,
                                 nsSize aCBSize, nscoord aAvailableWidth,
                                 nsSize aMargin, nsSize aBorder,
                                 nsSize aPadding, bool aShrinkWrap);

  



  nscoord ShrinkWidthToFit(nsRenderingContext *aRenderingContext,
                           nscoord aWidthInCB);

  virtual void WillReflow(nsPresContext* aPresContext) MOZ_OVERRIDE;
  





















  virtual void Reflow(nsPresContext*           aPresContext,
                      nsHTMLReflowMetrics&     aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus) MOZ_OVERRIDE;
  virtual void DidReflow(nsPresContext*           aPresContext,
                         const nsHTMLReflowState* aReflowState,
                         nsDidReflowStatus        aStatus) MOZ_OVERRIDE;

  




  void ReflowAbsoluteFrames(nsPresContext*           aPresContext,
                            nsHTMLReflowMetrics&     aDesiredSize,
                            const nsHTMLReflowState& aReflowState,
                            nsReflowStatus&          aStatus,
                            bool                     aConstrainHeight = true);
  void FinishReflowWithAbsoluteFrames(nsPresContext*           aPresContext,
                                      nsHTMLReflowMetrics&     aDesiredSize,
                                      const nsHTMLReflowState& aReflowState,
                                      nsReflowStatus&          aStatus,
                                      bool                     aConstrainHeight = true);

  










  void PushDirtyBitToAbsoluteFrames();

  virtual bool CanContinueTextRun() const MOZ_OVERRIDE;

  virtual bool UpdateOverflow() MOZ_OVERRIDE;

  

  NS_IMETHOD HandlePress(nsPresContext* aPresContext,
                         mozilla::WidgetGUIEvent* aEvent,
                         nsEventStatus* aEventStatus);

  NS_IMETHOD HandleMultiplePress(nsPresContext* aPresContext,
                                 mozilla::WidgetGUIEvent* aEvent,
                                 nsEventStatus* aEventStatus,
                                 bool aControlHeld);

  NS_IMETHOD HandleDrag(nsPresContext* aPresContext,
                        mozilla::WidgetGUIEvent* aEvent,
                        nsEventStatus* aEventStatus);

  NS_IMETHOD HandleRelease(nsPresContext* aPresContext,
                           mozilla::WidgetGUIEvent* aEvent,
                           nsEventStatus* aEventStatus);

  enum { SELECT_ACCUMULATE = 0x01 };

  nsresult PeekBackwardAndForward(nsSelectionAmount aAmountBack,
                                  nsSelectionAmount aAmountForward,
                                  int32_t aStartPos,
                                  nsPresContext* aPresContext,
                                  bool aJumpLines,
                                  uint32_t aSelectFlags);

  nsresult SelectByTypeAtPoint(nsPresContext* aPresContext,
                               const nsPoint& aPoint,
                               nsSelectionAmount aBeginAmountType,
                               nsSelectionAmount aEndAmountType,
                               uint32_t aSelectFlags);

  
  
  virtual ContentOffsets CalcContentOffsetsFromFramePoint(nsPoint aPoint);

  
  virtual nsSize GetPrefSize(nsBoxLayoutState& aBoxLayoutState) MOZ_OVERRIDE;
  virtual nsSize GetMinSize(nsBoxLayoutState& aBoxLayoutState) MOZ_OVERRIDE;
  virtual nsSize GetMaxSize(nsBoxLayoutState& aBoxLayoutState) MOZ_OVERRIDE;
  virtual nscoord GetFlex(nsBoxLayoutState& aBoxLayoutState) MOZ_OVERRIDE;
  virtual nscoord GetBoxAscent(nsBoxLayoutState& aBoxLayoutState) MOZ_OVERRIDE;

  
  
  virtual bool ComputesOwnOverflowArea() MOZ_OVERRIDE { return true; }

  
  

  
  
  
  
  
  bool IsFrameTreeTooDeep(const nsHTMLReflowState& aReflowState,
                            nsHTMLReflowMetrics& aMetrics,
                            nsReflowStatus& aStatus);

  
  
  void ConsiderChildOverflow(nsOverflowAreas& aOverflowAreas,
                             nsIFrame* aChildFrame);

  


  bool ShouldAvoidBreakInside(const nsHTMLReflowState& aReflowState) const {
    return !aReflowState.mFlags.mIsTopOfPage &&
           NS_STYLE_PAGE_BREAK_AVOID == StyleDisplay()->mBreakInside &&
           !GetPrevInFlow();
  }

#ifdef DEBUG
  





  void Trace(const char* aMethod, bool aEnter);
  void Trace(const char* aMethod, bool aEnter, nsReflowStatus aStatus);
  void TraceMsg(const char* fmt, ...);

  
  
  static void VerifyDirtyBitSet(const nsFrameList& aFrameList);

  static void XMLQuote(nsString& aString);

  








  virtual void DumpBaseRegressionData(nsPresContext* aPresContext, FILE* out, int32_t aIndent);
  
  
  static void* DisplayReflowEnter(nsPresContext*          aPresContext,
                                  nsIFrame*                aFrame,
                                  const nsHTMLReflowState& aReflowState);
  static void* DisplayLayoutEnter(nsIFrame* aFrame);
  static void* DisplayIntrinsicISizeEnter(nsIFrame* aFrame,
                                          const char* aType);
  static void* DisplayIntrinsicSizeEnter(nsIFrame* aFrame,
                                         const char* aType);
  static void  DisplayReflowExit(nsPresContext*      aPresContext,
                                 nsIFrame*            aFrame,
                                 nsHTMLReflowMetrics& aMetrics,
                                 uint32_t             aStatus,
                                 void*                aFrameTreeNode);
  static void  DisplayLayoutExit(nsIFrame* aFrame,
                                 void* aFrameTreeNode);
  static void  DisplayIntrinsicISizeExit(nsIFrame* aFrame,
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

  








  bool DisplayBackgroundUnconditional(nsDisplayListBuilder* aBuilder,
                                      const nsDisplayListSet& aLists,
                                      bool aForceBackground);
  








  void DisplayBorderBackgroundOutline(nsDisplayListBuilder*   aBuilder,
                                      const nsDisplayListSet& aLists,
                                      bool aForceBackground = false);
  


  void DisplayOutlineUnconditional(nsDisplayListBuilder*   aBuilder,
                                   const nsDisplayListSet& aLists);
  



  void DisplayOutline(nsDisplayListBuilder*   aBuilder,
                      const nsDisplayListSet& aLists);

  








  static nsIFrame*
  CorrectStyleParentFrame(nsIFrame* aProspectiveParent, nsIAtom* aChildPseudo);

protected:
  
  explicit nsFrame(nsStyleContext* aContext);
  virtual ~nsFrame();

  





  void DisplaySelectionOverlay(nsDisplayListBuilder* aBuilder,
      nsDisplayList* aList, uint16_t aContentType = nsISelectionDisplay::DISPLAY_FRAMES);

  int16_t DisplaySelection(nsPresContext* aPresContext, bool isOkToTurnOn = false);
  
  
  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext) MOZ_OVERRIDE;

public:
  
  
  static void GetLastLeaf(nsPresContext* aPresContext, nsIFrame **aFrame);
  static void GetFirstLeaf(nsPresContext* aPresContext, nsIFrame **aFrame);

  
  
  
  
  static int32_t GetLineNumber(nsIFrame *aFrame,
                               bool aLockScroll,
                               nsIFrame** aContainingBlock = nullptr);

  


  static bool ShouldApplyOverflowClipping(const nsIFrame* aFrame,
                                          const nsStyleDisplay* aDisp)
  {
    
    if (MOZ_UNLIKELY(aDisp->mOverflowX == NS_STYLE_OVERFLOW_CLIP)) {
      return true;
    }

    
    if (aDisp->mOverflowX == NS_STYLE_OVERFLOW_HIDDEN &&
        aDisp->mOverflowY == NS_STYLE_OVERFLOW_HIDDEN) {
      
      nsIAtom* type = aFrame->GetType();
      if (type == nsGkAtoms::tableFrame ||
          type == nsGkAtoms::tableCellFrame ||
          type == nsGkAtoms::bcTableCellFrame ||
          type == nsGkAtoms::svgOuterSVGFrame ||
          type == nsGkAtoms::svgInnerSVGFrame ||
          type == nsGkAtoms::svgForeignObjectFrame) {
        return true;
      }
      if (aFrame->IsFrameOfType(nsIFrame::eReplacedContainsBlock)) {
        if (type == nsGkAtoms::textInputFrame) {
          
          return false;
        }
        return true;
      }
    }

    if ((aFrame->GetStateBits() & NS_FRAME_SVG_LAYOUT)) {
      return false;
    }

    
    
    return
      (aFrame->GetStateBits() & NS_BLOCK_CLIP_PAGINATED_OVERFLOW) != 0 &&
      aFrame->PresContext()->IsPaginated() &&
      aFrame->GetType() == nsGkAtoms::blockFrame;
  }

  virtual nsILineIterator* GetLineIterator() MOZ_OVERRIDE;

protected:

  
  
  
  
  
  
  
  
  NS_IMETHOD GetDataForTableSelection(const nsFrameSelection* aFrameSelection,
                                      nsIPresShell* aPresShell,
                                      mozilla::WidgetMouseEvent* aMouseEvent,
                                      nsIContent** aParentContent,
                                      int32_t* aContentOffset,
                                      int32_t* aTarget);

  
  static void FillCursorInformationFromStyle(const nsStyleUserInterface* ui,
                                             nsIFrame::Cursor& aCursor);
  NS_IMETHOD DoLayout(nsBoxLayoutState& aBoxLayoutState) MOZ_OVERRIDE;

#ifdef DEBUG_LAYOUT
  virtual void GetBoxName(nsAutoString& aName) MOZ_OVERRIDE;
#endif

  nsBoxLayoutMetrics* BoxMetrics() const;

  
  void FireDOMEvent(const nsAString& aDOMEventName, nsIContent *aContent = nullptr);

private:
  void BoxReflow(nsBoxLayoutState& aState,
                 nsPresContext*    aPresContext,
                 nsHTMLReflowMetrics&     aDesiredSize,
                 nsRenderingContext* aRenderingContext,
                 nscoord aX,
                 nscoord aY,
                 nscoord aWidth,
                 nscoord aHeight,
                 bool aMoveFrame = true);

  NS_IMETHODIMP RefreshSizeCache(nsBoxLayoutState& aState);

#ifdef DEBUG_FRAME_DUMP
public:
  



  virtual nsresult  GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
  nsresult MakeFrameName(const nsAString& aKind, nsAString& aResult) const;
  
  
  static int32_t ContentIndexInContainer(const nsIFrame* aFrame);
#endif

#ifdef DEBUG
public:
  



  virtual nsFrameState  GetDebugStateBits() const MOZ_OVERRIDE;
  







  virtual nsresult  DumpRegressionData(nsPresContext* aPresContext,
                                       FILE* out, int32_t aIndent) MOZ_OVERRIDE;

  





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

  static void PrintDisplayItem(nsDisplayListBuilder* aBuilder,
                               nsDisplayItem* aItem,
                               std::stringstream& aStream,
                               bool aDumpSublist = false,
                               bool aDumpHtml = false);

  static void PrintDisplayList(nsDisplayListBuilder* aBuilder,
                               const nsDisplayList& aList,
                               bool aDumpHtml = false)
  {
    std::stringstream ss;
    PrintDisplayList(aBuilder, aList, ss, aDumpHtml);
    fprintf_stderr(stderr, "%s", ss.str().c_str());
  }
  static void PrintDisplayList(nsDisplayListBuilder* aBuilder,
                               const nsDisplayList& aList,
                               std::stringstream& aStream,
                               bool aDumpHtml = false);
  static void PrintDisplayListSet(nsDisplayListBuilder* aBuilder,
                                  const nsDisplayListSet& aList,
                                  std::stringstream& aStream,
                                  bool aDumpHtml = false);

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
    explicit DR_layout_cookie(nsIFrame* aFrame);
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
                           nscoord aHorizontalPercentBasis,
                           nscoord aVerticalPercentBasis,
                           const nsMargin* aBorder,
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
#define DISPLAY_INIT_OFFSETS(dr_frame, dr_state, dr_hpb, dr_vpb, dr_bdr, dr_pad)  \
  DR_init_offsets_cookie dr_cookie(dr_frame, dr_state, dr_hpb, dr_vpb, dr_bdr, dr_pad)
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
#define DISPLAY_INIT_OFFSETS(dr_frame, dr_state, dr_hpb, dr_vpb, dr_bdr, dr_pad)  \
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
