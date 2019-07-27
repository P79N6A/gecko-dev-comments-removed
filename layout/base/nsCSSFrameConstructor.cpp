










#include "nsCSSFrameConstructor.h"

#include "mozilla/AutoRestore.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/dom/HTMLSelectElement.h"
#include "mozilla/EventStates.h"
#include "mozilla/Likely.h"
#include "mozilla/LinkedList.h"
#include "nsAbsoluteContainingBlock.h"
#include "nsIAtom.h"
#include "nsIFrameInlines.h"
#include "nsGkAtoms.h"
#include "nsPresContext.h"
#include "nsIDocument.h"
#include "nsTableFrame.h"
#include "nsTableColFrame.h"
#include "nsTableRowFrame.h"
#include "nsTableCellFrame.h"
#include "nsIDOMHTMLDocument.h"
#include "nsHTMLParts.h"
#include "nsPresShell.h"
#include "nsIPresShell.h"
#include "nsUnicharUtils.h"
#include "nsStyleSet.h"
#include "nsViewManager.h"
#include "nsStyleConsts.h"
#include "nsIDOMXULElement.h"
#include "nsContainerFrame.h"
#include "nsNameSpaceManager.h"
#include "nsIComboboxControlFrame.h"
#include "nsIListControlFrame.h"
#include "nsIDOMCharacterData.h"
#include "nsPlaceholderFrame.h"
#include "nsTableRowGroupFrame.h"
#include "nsIFormControl.h"
#include "nsCSSAnonBoxes.h"
#include "nsTextFragment.h"
#include "nsIAnonymousContentCreator.h"
#include "nsBindingManager.h"
#include "nsXBLBinding.h"
#include "nsContentUtils.h"
#include "nsIScriptError.h"
#ifdef XP_MACOSX
#include "nsIDocShell.h"
#endif
#include "ChildIterator.h"
#include "nsError.h"
#include "nsLayoutUtils.h"
#include "nsAutoPtr.h"
#include "nsBoxFrame.h"
#include "nsBoxLayout.h"
#include "nsFlexContainerFrame.h"
#include "nsGridContainerFrame.h"
#include "RubyUtils.h"
#include "nsRubyFrame.h"
#include "nsRubyBaseFrame.h"
#include "nsRubyBaseContainerFrame.h"
#include "nsRubyTextFrame.h"
#include "nsRubyTextContainerFrame.h"
#include "nsImageFrame.h"
#include "nsIObjectLoadingContent.h"
#include "nsTArray.h"
#include "nsGenericDOMDataNode.h"
#include "mozilla/dom/Element.h"
#include "nsAutoLayoutPhase.h"
#include "nsStyleStructInlines.h"
#include "nsPageContentFrame.h"
#include "RestyleManager.h"
#include "StickyScrollContainer.h"
#include "nsFieldSetFrame.h"
#include "nsInlineFrame.h"
#include "nsBlockFrame.h"
#include "nsCanvasFrame.h"
#include "nsFirstLetterFrame.h"
#include "nsGfxScrollFrame.h"
#include "nsPageFrame.h"
#include "nsSimplePageSequenceFrame.h"
#include "nsTableOuterFrame.h"
#include "nsIScrollableFrame.h"

#ifdef MOZ_XUL
#include "nsIRootBox.h"
#endif
#ifdef ACCESSIBILITY
#include "nsAccessibilityService.h"
#endif

#include "nsXBLService.h"

#undef NOISY_FIRST_LETTER

#include "nsMathMLParts.h"
#include "mozilla/dom/SVGTests.h"
#include "nsSVGUtils.h"

#include "nsRefreshDriver.h"
#include "nsRuleProcessorData.h"
#include "nsTextNode.h"
#include "ActiveLayerTracker.h"

using namespace mozilla;
using namespace mozilla::dom;


static const nsIFrame::ChildListID kPrincipalList = nsIFrame::kPrincipalList;

nsIFrame*
NS_NewHTMLCanvasFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);

nsIFrame*
NS_NewHTMLVideoFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);

nsContainerFrame*
NS_NewSVGOuterSVGFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsContainerFrame*
NS_NewSVGOuterSVGAnonChildFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGInnerSVGFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGPathGeometryFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGGFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGGenericContainerFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsContainerFrame*
NS_NewSVGForeignObjectFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGAFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGSwitchFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGTextFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGContainerFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGUseFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGViewFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
extern nsIFrame*
NS_NewSVGLinearGradientFrame(nsIPresShell *aPresShell, nsStyleContext* aContext);
extern nsIFrame*
NS_NewSVGRadialGradientFrame(nsIPresShell *aPresShell, nsStyleContext* aContext);
extern nsIFrame*
NS_NewSVGStopFrame(nsIPresShell *aPresShell, nsStyleContext* aContext);
nsContainerFrame*
NS_NewSVGMarkerFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsContainerFrame*
NS_NewSVGMarkerAnonChildFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
extern nsIFrame*
NS_NewSVGImageFrame(nsIPresShell *aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGClipPathFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGFilterFrame(nsIPresShell *aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGPatternFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGMaskFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGFEContainerFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGFELeafFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGFEImageFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGFEUnstyledLeafFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

#include "mozilla/dom/NodeInfo.h"
#include "prenv.h"
#include "nsNodeInfoManager.h"
#include "nsContentCreatorFunctions.h"

#ifdef DEBUG



static bool gNoisyContentUpdates = false;
static bool gReallyNoisyContentUpdates = false;
static bool gNoisyInlineConstruction = false;

struct FrameCtorDebugFlags {
  const char* name;
  bool* on;
};

static FrameCtorDebugFlags gFlags[] = {
  { "content-updates",              &gNoisyContentUpdates },
  { "really-noisy-content-updates", &gReallyNoisyContentUpdates },
  { "noisy-inline",                 &gNoisyInlineConstruction }
};

#define NUM_DEBUG_FLAGS (sizeof(gFlags) / sizeof(gFlags[0]))
#endif


#ifdef MOZ_XUL
#include "nsMenuFrame.h"
#include "nsPopupSetFrame.h"
#include "nsTreeColFrame.h"
#include "nsIBoxObject.h"
#include "nsPIListBoxObject.h"
#include "nsListBoxBodyFrame.h"
#include "nsListItemFrame.h"
#include "nsXULLabelFrame.h"



nsIFrame*
NS_NewAutoRepeatBoxFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

nsContainerFrame*
NS_NewRootBoxFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);

nsContainerFrame*
NS_NewDocElementBoxFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

nsIFrame*
NS_NewThumbFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);

nsIFrame*
NS_NewDeckFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);

nsIFrame*
NS_NewLeafBoxFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);

nsIFrame*
NS_NewStackFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);

nsIFrame*
NS_NewProgressMeterFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);

nsIFrame*
NS_NewRangeFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);

nsIFrame*
NS_NewImageBoxFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);

nsIFrame*
NS_NewTextBoxFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);

nsIFrame*
NS_NewGroupBoxFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);

nsIFrame*
NS_NewButtonBoxFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);

nsIFrame*
NS_NewSplitterFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);

nsIFrame*
NS_NewMenuPopupFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);

nsIFrame*
NS_NewPopupSetFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

nsIFrame*
NS_NewMenuFrame (nsIPresShell* aPresShell, nsStyleContext* aContext, uint32_t aFlags);

nsIFrame*
NS_NewMenuBarFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);

nsIFrame*
NS_NewTreeBodyFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);


nsresult
NS_NewGridLayout2 ( nsIPresShell* aPresShell, nsBoxLayout** aNewLayout );
nsIFrame*
NS_NewGridRowLeafFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewGridRowGroupFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);



nsIFrame*
NS_NewTitleBarFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);

nsIFrame*
NS_NewResizerFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);


#endif

nsHTMLScrollFrame*
NS_NewHTMLScrollFrame(nsIPresShell* aPresShell, nsStyleContext* aContext, bool aIsRoot);

nsXULScrollFrame*
NS_NewXULScrollFrame(nsIPresShell* aPresShell, nsStyleContext* aContext,
                     bool aIsRoot, bool aClipAllDescendants);

nsIFrame*
NS_NewSliderFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);

nsIFrame*
NS_NewScrollbarFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);

nsIFrame*
NS_NewScrollbarButtonFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);


#ifdef NOISY_FINDFRAME
static int32_t FFWC_totalCount=0;
static int32_t FFWC_doLoop=0;
static int32_t FFWC_doSibling=0;
static int32_t FFWC_recursions=0;
static int32_t FFWC_nextInFlows=0;
#endif


static inline bool
IsAnonymousFlexOrGridItem(const nsIFrame* aFrame)
{
  const nsIAtom* pseudoType = aFrame->StyleContext()->GetPseudo();
  return pseudoType == nsCSSAnonBoxes::anonymousFlexItem ||
         pseudoType == nsCSSAnonBoxes::anonymousGridItem;
}


static inline bool
IsFlexOrGridContainer(const nsIFrame* aFrame)
{
  const nsIAtom* t = aFrame->GetType();
  return t == nsGkAtoms::flexContainerFrame ||
         t == nsGkAtoms::gridContainerFrame;
}

#if DEBUG
static void
AssertAnonymousFlexOrGridItemParent(const nsIFrame* aChild,
                                    const nsIFrame* aParent)
{
  MOZ_ASSERT(IsAnonymousFlexOrGridItem(aChild),
             "expected an anonymous flex or grid item child frame");
  MOZ_ASSERT(aParent, "expected a parent frame");
  const nsIAtom* pseudoType = aChild->StyleContext()->GetPseudo();
  if (pseudoType == nsCSSAnonBoxes::anonymousFlexItem) {
    MOZ_ASSERT(aParent->GetType() == nsGkAtoms::flexContainerFrame,
               "anonymous flex items should only exist as children "
               "of flex container frames");
  } else {
    MOZ_ASSERT(aParent->GetType() == nsGkAtoms::gridContainerFrame,
               "anonymous grid items should only exist as children "
               "of grid container frames");
  }
}
#else
#define AssertAnonymousFlexOrGridItemParent(x, y) do { /* nothing */ } while(0)
#endif

static inline nsContainerFrame*
GetFieldSetBlockFrame(nsIFrame* aFieldsetFrame)
{
  
  nsIFrame* firstChild = aFieldsetFrame->GetFirstPrincipalChild();
  nsIFrame* inner = firstChild && firstChild->GetNextSibling() ? firstChild->GetNextSibling() : firstChild;
  return inner ? inner->GetContentInsertionFrame() : nullptr;
}

#define FCDATA_DECL(_flags, _func)                          \
  { _flags, { (FrameCreationFunc)_func }, nullptr, nullptr }
#define FCDATA_WITH_WRAPPING_BLOCK(_flags, _func, _anon_box)  \
  { _flags | FCDATA_CREATE_BLOCK_WRAPPER_FOR_ALL_KIDS,        \
      { (FrameCreationFunc)_func }, nullptr, &_anon_box }











static bool
IsInlineFrame(const nsIFrame* aFrame)
{
  return aFrame->IsFrameOfType(nsIFrame::eLineParticipant);
}





static bool
IsFrameForSVG(const nsIFrame* aFrame)
{
  return aFrame->IsFrameOfType(nsIFrame::eSVG) ||
         aFrame->IsSVGText();
}







static bool
ShouldSuppressFloatingOfDescendants(nsIFrame* aFrame)
{
  return aFrame->IsFrameOfType(nsIFrame::eMathML) ||
    aFrame->IsBoxFrame() ||
    ::IsFlexOrGridContainer(aFrame);
}





static nsIContent*
AnyKidsNeedBlockParent(nsIFrame *aFrameList)
{
  for (nsIFrame *k = aFrameList; k; k = k->GetNextSibling()) {
    
    
    
    if (k->IsFrameOfType(nsIFrame::eLineParticipant)) {
      return k->GetContent();
    }
  }
  return nullptr;
}


static void
ReparentFrame(RestyleManager* aRestyleManager,
              nsContainerFrame* aNewParentFrame,
              nsIFrame* aFrame)
{
  aFrame->SetParent(aNewParentFrame);
  aRestyleManager->ReparentStyleContext(aFrame);
}

static void
ReparentFrames(nsCSSFrameConstructor* aFrameConstructor,
               nsContainerFrame* aNewParentFrame,
               const nsFrameList& aFrameList)
{
  RestyleManager* restyleManager = aFrameConstructor->RestyleManager();
  for (nsFrameList::Enumerator e(aFrameList); !e.AtEnd(); e.Next()) {
    ReparentFrame(restyleManager, aNewParentFrame, e.get());
  }
}







static inline bool
IsFramePartOfIBSplit(nsIFrame* aFrame)
{
  return (aFrame->GetStateBits() & NS_FRAME_PART_OF_IBSPLIT) != 0;
}

static nsContainerFrame* GetIBSplitSibling(nsIFrame* aFrame)
{
  NS_PRECONDITION(IsFramePartOfIBSplit(aFrame), "Shouldn't call this");

  
  
  return static_cast<nsContainerFrame*>
    (aFrame->FirstContinuation()->
       Properties().Get(nsIFrame::IBSplitSibling()));
}

static nsContainerFrame* GetIBSplitPrevSibling(nsIFrame* aFrame)
{
  NS_PRECONDITION(IsFramePartOfIBSplit(aFrame), "Shouldn't call this");

  
  
  return static_cast<nsContainerFrame*>
    (aFrame->FirstContinuation()->
       Properties().Get(nsIFrame::IBSplitPrevSibling()));
}

static nsContainerFrame*
GetLastIBSplitSibling(nsIFrame* aFrame, bool aReturnEmptyTrailingInline)
{
  for (nsIFrame *frame = aFrame, *next; ; frame = next) {
    next = GetIBSplitSibling(frame);
    if (!next ||
        (!aReturnEmptyTrailingInline && !next->GetFirstPrincipalChild() &&
         !GetIBSplitSibling(next))) {
      NS_ASSERTION(!next || !frame->IsInlineOutside(),
                   "Should have a block here!");
      return static_cast<nsContainerFrame*>(frame);
    }
  }
  NS_NOTREACHED("unreachable code");
  return nullptr;
}

static void
SetFrameIsIBSplit(nsContainerFrame* aFrame, nsIFrame* aIBSplitSibling)
{
  NS_PRECONDITION(aFrame, "bad args!");

  
  NS_ASSERTION(!aFrame->GetPrevContinuation(),
               "assigning ib-split sibling to other than first continuation!");
  NS_ASSERTION(!aFrame->GetNextContinuation() ||
               IsFramePartOfIBSplit(aFrame->GetNextContinuation()),
               "should have no non-ib-split continuations here");

  
  aFrame->AddStateBits(NS_FRAME_PART_OF_IBSPLIT);

  if (aIBSplitSibling) {
    NS_ASSERTION(!aIBSplitSibling->GetPrevContinuation(),
                 "assigning something other than the first continuation as the "
                 "ib-split sibling");

    
    
    FramePropertyTable* props = aFrame->PresContext()->PropertyTable();
    props->Set(aFrame, nsIFrame::IBSplitSibling(), aIBSplitSibling);
    props->Set(aIBSplitSibling, nsIFrame::IBSplitPrevSibling(), aFrame);
  }
}

static nsIFrame*
GetIBContainingBlockFor(nsIFrame* aFrame)
{
  NS_PRECONDITION(IsFramePartOfIBSplit(aFrame),
                  "GetIBContainingBlockFor() should only be called on known IB frames");

  
  nsIFrame* parentFrame;
  do {
    parentFrame = aFrame->GetParent();

    if (! parentFrame) {
      NS_ERROR("no unsplit block frame in IB hierarchy");
      return aFrame;
    }

    
    
    
    if (!IsFramePartOfIBSplit(parentFrame) &&
        !parentFrame->StyleContext()->GetPseudo())
      break;

    aFrame = parentFrame;
  } while (1);

  
  NS_ASSERTION(parentFrame, "no normal ancestor found for ib-split frame "
                            "in GetIBContainingBlockFor");
  NS_ASSERTION(parentFrame != aFrame, "parentFrame is actually the child frame - bogus reslt");

  return parentFrame;
}














static void
FindFirstBlock(nsFrameList::FrameLinkEnumerator& aLink)
{
  for ( ; !aLink.AtEnd(); aLink.Next()) {
    if (!aLink.NextFrame()->IsInlineOutside()) {
      return;
    }
  }
}




static nsFrameList::FrameLinkEnumerator
FindFirstNonBlock(const nsFrameList& aList)
{
  nsFrameList::FrameLinkEnumerator link(aList);
  for (; !link.AtEnd(); link.Next()) {
    if (link.NextFrame()->IsInlineOutside()) {
      break;
    }
  }
  return link;
}

inline void
SetInitialSingleChild(nsContainerFrame* aParent, nsIFrame* aFrame)
{
  NS_PRECONDITION(!aFrame->GetNextSibling(), "Should be using a frame list");
  nsFrameList temp(aFrame, aFrame);
  aParent->SetInitialChildList(kPrincipalList, temp);
}




struct nsFrameItems : public nsFrameList
{
  
  void AddChild(nsIFrame* aChild);
};

void
nsFrameItems::AddChild(nsIFrame* aChild)
{
  NS_PRECONDITION(aChild, "nsFrameItems::AddChild");

  
  
  
  
  if (IsEmpty()) {
    SetFrames(aChild);
  }
  else {
    NS_ASSERTION(aChild != mLastChild,
                 "Same frame being added to frame list twice?");
    mLastChild->SetNextSibling(aChild);
    mLastChild = nsLayoutUtils::GetLastSibling(aChild);
  }
}





struct nsAbsoluteItems : nsFrameItems {
  
  nsContainerFrame* containingBlock;

  explicit nsAbsoluteItems(nsContainerFrame* aContainingBlock);
#ifdef DEBUG
  
  
  
  ~nsAbsoluteItems() {
    NS_ASSERTION(!FirstChild(),
                 "Dangling child list.  Someone forgot to insert it?");
  }
#endif

  
  void AddChild(nsIFrame* aChild);
};

nsAbsoluteItems::nsAbsoluteItems(nsContainerFrame* aContainingBlock)
  : containingBlock(aContainingBlock)
{
}


void
nsAbsoluteItems::AddChild(nsIFrame* aChild)
{
  NS_ASSERTION(aChild->PresContext()->FrameManager()->
               GetPlaceholderFrameFor(aChild),
               "Child without placeholder being added to nsAbsoluteItems?");
  aChild->AddStateBits(NS_FRAME_OUT_OF_FLOW);
  nsFrameItems::AddChild(aChild);
}





class MOZ_STACK_CLASS nsFrameConstructorSaveState {
public:
  typedef nsIFrame::ChildListID ChildListID;
  nsFrameConstructorSaveState();
  ~nsFrameConstructorSaveState();

private:
  nsAbsoluteItems* mItems;      
  nsAbsoluteItems  mSavedItems; 

  
  ChildListID mChildListID;
  nsFrameConstructorState* mState;

  
  
  nsAbsoluteItems mSavedFixedItems;

  bool mSavedFixedPosIsAbsPos;

  friend class nsFrameConstructorState;
};




struct PendingBinding : public LinkedListElement<PendingBinding>
{
#ifdef NS_BUILD_REFCNT_LOGGING
  PendingBinding() {
    MOZ_COUNT_CTOR(PendingBinding);
  }
  ~PendingBinding() {
    MOZ_COUNT_DTOR(PendingBinding);
  }
#endif

  nsRefPtr<nsXBLBinding> mBinding;
};



class MOZ_STACK_CLASS nsFrameConstructorState {
public:
  typedef nsIFrame::ChildListID ChildListID;

  nsPresContext            *mPresContext;
  nsIPresShell             *mPresShell;
  nsFrameManager           *mFrameManager;

#ifdef MOZ_XUL
  
  nsAbsoluteItems           mPopupItems;
#endif

  
  nsAbsoluteItems           mFixedItems;
  nsAbsoluteItems           mAbsoluteItems;
  nsAbsoluteItems           mFloatedItems;

  nsCOMPtr<nsILayoutHistoryState> mFrameState;
  
  
  nsFrameState              mAdditionalStateBits;

  
  
  
  
  
  bool                      mFixedPosIsAbsPos;

  
  
  
  bool                      mHavePendingPopupgroup;

  
  
  
  
  
  
  
  bool                      mCreatingExtraFrames;

  nsCOMArray<nsIContent>    mGeneratedTextNodesWithInitializer;

  TreeMatchContext          mTreeMatchContext;

  
  
  nsFrameConstructorState(nsIPresShell*          aPresShell,
                          nsContainerFrame*      aFixedContainingBlock,
                          nsContainerFrame*      aAbsoluteContainingBlock,
                          nsContainerFrame*      aFloatContainingBlock,
                          nsILayoutHistoryState* aHistoryState);
  
  nsFrameConstructorState(nsIPresShell*          aPresShell,
                          nsContainerFrame*      aFixedContainingBlock,
                          nsContainerFrame*      aAbsoluteContainingBlock,
                          nsContainerFrame*      aFloatContainingBlock);

  ~nsFrameConstructorState();

  
  
  
  
  
  
  
  
  
  void PushAbsoluteContainingBlock(nsContainerFrame* aNewAbsoluteContainingBlock,
                                   nsIFrame* aPositionedFrame,
                                   nsFrameConstructorSaveState& aSaveState);

  
  
  
  
  
  
  
  void PushFloatContainingBlock(nsContainerFrame* aNewFloatContainingBlock,
                                nsFrameConstructorSaveState& aSaveState);

  
  
  
  nsContainerFrame* GetGeometricParent(const nsStyleDisplay* aStyleDisplay,
                                       nsContainerFrame* aContentParentFrame) const;

  

















  void AddChild(nsIFrame* aNewFrame,
                nsFrameItems& aFrameItems,
                nsIContent* aContent,
                nsStyleContext* aStyleContext,
                nsContainerFrame* aParentFrame,
                bool aCanBePositioned = true,
                bool aCanBeFloated = true,
                bool aIsOutOfFlowPopup = false,
                bool aInsertAfter = false,
                nsIFrame* aInsertAfterFrame = nullptr);

  





  nsAbsoluteItems& GetFixedItems()
  {
    return mFixedPosIsAbsPos ? mAbsoluteItems : mFixedItems;
  }
  const nsAbsoluteItems& GetFixedItems() const
  {
    return mFixedPosIsAbsPos ? mAbsoluteItems : mFixedItems;
  }


  




  class PendingBindingAutoPusher;
  friend class PendingBindingAutoPusher;
  class MOZ_STACK_CLASS PendingBindingAutoPusher {
  public:
    PendingBindingAutoPusher(nsFrameConstructorState& aState,
                             PendingBinding* aPendingBinding) :
      mState(aState),
      mPendingBinding(aState.mCurrentPendingBindingInsertionPoint)
        {
          if (aPendingBinding) {
            aState.mCurrentPendingBindingInsertionPoint = aPendingBinding;
          }
        }

    ~PendingBindingAutoPusher()
      {
        mState.mCurrentPendingBindingInsertionPoint = mPendingBinding;
      }

  private:
    nsFrameConstructorState& mState;
    PendingBinding* mPendingBinding;
  };

  


  void AddPendingBinding(PendingBinding* aPendingBinding) {
    if (mCurrentPendingBindingInsertionPoint) {
      mCurrentPendingBindingInsertionPoint->setPrevious(aPendingBinding);
    } else {
      mPendingBindings.insertBack(aPendingBinding);
    }
  }

protected:
  friend class nsFrameConstructorSaveState;

  



  void ProcessFrameInsertions(nsAbsoluteItems& aFrameItems,
                              ChildListID aChildListID);

  
  
  LinkedList<PendingBinding> mPendingBindings;

  PendingBinding* mCurrentPendingBindingInsertionPoint;
};

nsFrameConstructorState::nsFrameConstructorState(nsIPresShell*          aPresShell,
                                                 nsContainerFrame*      aFixedContainingBlock,
                                                 nsContainerFrame*      aAbsoluteContainingBlock,
                                                 nsContainerFrame*      aFloatContainingBlock,
                                                 nsILayoutHistoryState* aHistoryState)
  : mPresContext(aPresShell->GetPresContext()),
    mPresShell(aPresShell),
    mFrameManager(aPresShell->FrameManager()),
#ifdef MOZ_XUL
    mPopupItems(nullptr),
#endif
    mFixedItems(aFixedContainingBlock),
    mAbsoluteItems(aAbsoluteContainingBlock),
    mFloatedItems(aFloatContainingBlock),
    
    mFrameState(aHistoryState),
    mAdditionalStateBits(nsFrameState(0)),
    
    
    
    mFixedPosIsAbsPos(aFixedContainingBlock == aAbsoluteContainingBlock),
    mHavePendingPopupgroup(false),
    mCreatingExtraFrames(false),
    mTreeMatchContext(true, nsRuleWalker::eRelevantLinkUnvisited,
                      aPresShell->GetDocument()),
    mCurrentPendingBindingInsertionPoint(nullptr)
{
#ifdef MOZ_XUL
  nsIRootBox* rootBox = nsIRootBox::GetRootBox(aPresShell);
  if (rootBox) {
    mPopupItems.containingBlock = rootBox->GetPopupSetFrame();
  }
#endif
  MOZ_COUNT_CTOR(nsFrameConstructorState);
}

nsFrameConstructorState::nsFrameConstructorState(nsIPresShell* aPresShell,
                                                 nsContainerFrame* aFixedContainingBlock,
                                                 nsContainerFrame* aAbsoluteContainingBlock,
                                                 nsContainerFrame* aFloatContainingBlock)
  : mPresContext(aPresShell->GetPresContext()),
    mPresShell(aPresShell),
    mFrameManager(aPresShell->FrameManager()),
#ifdef MOZ_XUL
    mPopupItems(nullptr),
#endif
    mFixedItems(aFixedContainingBlock),
    mAbsoluteItems(aAbsoluteContainingBlock),
    mFloatedItems(aFloatContainingBlock),
    
    mAdditionalStateBits(nsFrameState(0)),
    
    
    
    mFixedPosIsAbsPos(aFixedContainingBlock == aAbsoluteContainingBlock),
    mHavePendingPopupgroup(false),
    mCreatingExtraFrames(false),
    mTreeMatchContext(true, nsRuleWalker::eRelevantLinkUnvisited,
                      aPresShell->GetDocument()),
    mCurrentPendingBindingInsertionPoint(nullptr)
{
#ifdef MOZ_XUL
  nsIRootBox* rootBox = nsIRootBox::GetRootBox(aPresShell);
  if (rootBox) {
    mPopupItems.containingBlock = rootBox->GetPopupSetFrame();
  }
#endif
  MOZ_COUNT_CTOR(nsFrameConstructorState);
  mFrameState = aPresShell->GetDocument()->GetLayoutHistoryState();
}

nsFrameConstructorState::~nsFrameConstructorState()
{
  
  
  
  
  
  
  
  
  MOZ_COUNT_DTOR(nsFrameConstructorState);
  ProcessFrameInsertions(mFloatedItems, nsIFrame::kFloatList);
  ProcessFrameInsertions(mAbsoluteItems, nsIFrame::kAbsoluteList);
  ProcessFrameInsertions(mFixedItems, nsIFrame::kFixedList);
#ifdef MOZ_XUL
  ProcessFrameInsertions(mPopupItems, nsIFrame::kPopupList);
#endif
  for (int32_t i = mGeneratedTextNodesWithInitializer.Count() - 1; i >= 0; --i) {
    mGeneratedTextNodesWithInitializer[i]->
      DeleteProperty(nsGkAtoms::genConInitializerProperty);
  }
  if (!mPendingBindings.isEmpty()) {
    nsBindingManager* bindingManager = mPresShell->GetDocument()->BindingManager();
    do {
      nsAutoPtr<PendingBinding> pendingBinding;
      pendingBinding = mPendingBindings.popFirst();
      bindingManager->AddToAttachedQueue(pendingBinding->mBinding);
    } while (!mPendingBindings.isEmpty());
    mCurrentPendingBindingInsertionPoint = nullptr;
  }
}

static nsContainerFrame*
AdjustAbsoluteContainingBlock(nsContainerFrame* aContainingBlockIn)
{
  if (!aContainingBlockIn) {
    return nullptr;
  }

  
  
  return static_cast<nsContainerFrame*>(aContainingBlockIn->FirstContinuation());
}

void
nsFrameConstructorState::PushAbsoluteContainingBlock(nsContainerFrame* aNewAbsoluteContainingBlock,
                                                     nsIFrame* aPositionedFrame,
                                                     nsFrameConstructorSaveState& aSaveState)
{
  aSaveState.mItems = &mAbsoluteItems;
  aSaveState.mSavedItems = mAbsoluteItems;
  aSaveState.mChildListID = nsIFrame::kAbsoluteList;
  aSaveState.mState = this;
  aSaveState.mSavedFixedPosIsAbsPos = mFixedPosIsAbsPos;

  if (mFixedPosIsAbsPos) {
    
    
    aSaveState.mSavedFixedItems = mFixedItems;
    mFixedItems = mAbsoluteItems;
  }

  mAbsoluteItems =
    nsAbsoluteItems(AdjustAbsoluteContainingBlock(aNewAbsoluteContainingBlock));

  


  mFixedPosIsAbsPos = aPositionedFrame &&
      aPositionedFrame->StyleDisplay()->IsFixedPosContainingBlock(aPositionedFrame);

  if (aNewAbsoluteContainingBlock) {
    aNewAbsoluteContainingBlock->MarkAsAbsoluteContainingBlock();
  }
}

void
nsFrameConstructorState::PushFloatContainingBlock(nsContainerFrame* aNewFloatContainingBlock,
                                                  nsFrameConstructorSaveState& aSaveState)
{
  NS_PRECONDITION(!aNewFloatContainingBlock ||
                  aNewFloatContainingBlock->IsFloatContainingBlock(),
                  "Please push a real float containing block!");
  NS_ASSERTION(!aNewFloatContainingBlock ||
               !ShouldSuppressFloatingOfDescendants(aNewFloatContainingBlock),
               "We should not push a frame that is supposed to _suppress_ "
               "floats as a float containing block!");
  aSaveState.mItems = &mFloatedItems;
  aSaveState.mSavedItems = mFloatedItems;
  aSaveState.mChildListID = nsIFrame::kFloatList;
  aSaveState.mState = this;
  mFloatedItems = nsAbsoluteItems(aNewFloatContainingBlock);
}

nsContainerFrame*
nsFrameConstructorState::GetGeometricParent(const nsStyleDisplay* aStyleDisplay,
                                            nsContainerFrame* aContentParentFrame) const
{
  NS_PRECONDITION(aStyleDisplay, "Must have display struct!");

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  if (aContentParentFrame && aContentParentFrame->IsSVGText()) {
    return aContentParentFrame;
  }

  if (aStyleDisplay->IsFloatingStyle() && mFloatedItems.containingBlock) {
    NS_ASSERTION(!aStyleDisplay->IsAbsolutelyPositionedStyle(),
                 "Absolutely positioned _and_ floating?");
    return mFloatedItems.containingBlock;
  }

  if (aStyleDisplay->mPosition == NS_STYLE_POSITION_ABSOLUTE &&
      mAbsoluteItems.containingBlock) {
    return mAbsoluteItems.containingBlock;
  }

  if (aStyleDisplay->mPosition == NS_STYLE_POSITION_FIXED &&
      GetFixedItems().containingBlock) {
    return GetFixedItems().containingBlock;
  }

  return aContentParentFrame;
}

void
nsFrameConstructorState::AddChild(nsIFrame* aNewFrame,
                                  nsFrameItems& aFrameItems,
                                  nsIContent* aContent,
                                  nsStyleContext* aStyleContext,
                                  nsContainerFrame* aParentFrame,
                                  bool aCanBePositioned,
                                  bool aCanBeFloated,
                                  bool aIsOutOfFlowPopup,
                                  bool aInsertAfter,
                                  nsIFrame* aInsertAfterFrame)
{
  NS_PRECONDITION(!aNewFrame->GetNextSibling(), "Shouldn't happen");

  const nsStyleDisplay* disp = aNewFrame->StyleDisplay();

  
  

  bool needPlaceholder = false;
  nsFrameState placeholderType;
  nsFrameItems* frameItems = &aFrameItems;
#ifdef MOZ_XUL
  if (MOZ_UNLIKELY(aIsOutOfFlowPopup)) {
      NS_ASSERTION(aNewFrame->GetParent() == mPopupItems.containingBlock,
                   "Popup whose parent is not the popup containing block?");
      NS_ASSERTION(mPopupItems.containingBlock, "Must have a popup set frame!");
      needPlaceholder = true;
      frameItems = &mPopupItems;
      placeholderType = PLACEHOLDER_FOR_POPUP;
  }
  else
#endif 
  if (aCanBeFloated && aNewFrame->IsFloating() &&
      mFloatedItems.containingBlock) {
    NS_ASSERTION(aNewFrame->GetParent() == mFloatedItems.containingBlock,
                 "Float whose parent is not the float containing block?");
    needPlaceholder = true;
    frameItems = &mFloatedItems;
    placeholderType = PLACEHOLDER_FOR_FLOAT;
  }
  else if (aCanBePositioned) {
    if (disp->mPosition == NS_STYLE_POSITION_ABSOLUTE &&
        mAbsoluteItems.containingBlock) {
      NS_ASSERTION(aNewFrame->GetParent() == mAbsoluteItems.containingBlock,
                   "Abs pos whose parent is not the abs pos containing block?");
      needPlaceholder = true;
      frameItems = &mAbsoluteItems;
      placeholderType = PLACEHOLDER_FOR_ABSPOS;
    }
    if (disp->mPosition == NS_STYLE_POSITION_FIXED &&
        GetFixedItems().containingBlock) {
      NS_ASSERTION(aNewFrame->GetParent() == GetFixedItems().containingBlock,
                   "Fixed pos whose parent is not the fixed pos containing block?");
      needPlaceholder = true;
      frameItems = &GetFixedItems();
      placeholderType = PLACEHOLDER_FOR_FIXEDPOS;
    }
  }

  if (needPlaceholder) {
    NS_ASSERTION(frameItems != &aFrameItems,
                 "Putting frame in-flow _and_ want a placeholder?");
    nsIFrame* placeholderFrame =
      nsCSSFrameConstructor::CreatePlaceholderFrameFor(mPresShell,
                                                       aContent,
                                                       aNewFrame,
                                                       aStyleContext,
                                                       aParentFrame,
                                                       nullptr,
                                                       placeholderType);

    placeholderFrame->AddStateBits(mAdditionalStateBits);
    
    aFrameItems.AddChild(placeholderFrame);
  }
#ifdef DEBUG
  else {
    NS_ASSERTION(aNewFrame->GetParent() == aParentFrame,
                 "In-flow frame has wrong parent");
  }
#endif

  if (aInsertAfter) {
    frameItems->InsertFrame(nullptr, aInsertAfterFrame, aNewFrame);
  } else {
    frameItems->AddChild(aNewFrame);
  }
}

void
nsFrameConstructorState::ProcessFrameInsertions(nsAbsoluteItems& aFrameItems,
                                                ChildListID aChildListID)
{
#define NS_NONXUL_LIST_TEST (&aFrameItems == &mFloatedItems &&            \
                             aChildListID == nsIFrame::kFloatList)    ||  \
                            (&aFrameItems == &mAbsoluteItems &&           \
                             aChildListID == nsIFrame::kAbsoluteList) ||  \
                            (&aFrameItems == &mFixedItems &&              \
                             aChildListID == nsIFrame::kFixedList)
#ifdef MOZ_XUL
  NS_PRECONDITION(NS_NONXUL_LIST_TEST ||
                  (&aFrameItems == &mPopupItems &&
                   aChildListID == nsIFrame::kPopupList),
                  "Unexpected aFrameItems/aChildListID combination");
#else
  NS_PRECONDITION(NS_NONXUL_LIST_TEST,
                  "Unexpected aFrameItems/aChildListID combination");
#endif

  if (aFrameItems.IsEmpty()) {
    return;
  }

  nsContainerFrame* containingBlock = aFrameItems.containingBlock;

  NS_ASSERTION(containingBlock,
               "Child list without containing block?");

  if (aChildListID == nsIFrame::kFixedList) {
    
    
    aChildListID = containingBlock->GetAbsoluteListID();
  }

  
  
  
  const nsFrameList& childList = containingBlock->GetChildList(aChildListID);
  if (childList.IsEmpty() &&
      (containingBlock->GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
    
    
    if (aChildListID == containingBlock->GetAbsoluteListID()) {
      containingBlock->GetAbsoluteContainingBlock()->
        SetInitialChildList(containingBlock, aChildListID, aFrameItems);
    } else {
      containingBlock->SetInitialChildList(aChildListID, aFrameItems);
    }
  } else {
    
    
    
    
    
    
    
    nsIFrame* lastChild = childList.LastChild();

    
    
    
    nsIFrame* firstNewFrame = aFrameItems.FirstChild();

    
    nsAutoTArray<nsIFrame*, 20> firstNewFrameAncestors;
    nsIFrame* notCommonAncestor = nullptr;
    if (lastChild) {
      notCommonAncestor = nsLayoutUtils::FillAncestors(firstNewFrame,
                                                       containingBlock,
                                                       &firstNewFrameAncestors);
    }

    if (!lastChild ||
        nsLayoutUtils::CompareTreePosition(lastChild, firstNewFrame,
                                           firstNewFrameAncestors,
                                           notCommonAncestor ?
                                             containingBlock : nullptr) < 0) {
      
      mFrameManager->AppendFrames(containingBlock, aChildListID, aFrameItems);
    } else {
      
      
      nsAutoTArray<nsIFrame*, 128> children;
      for (nsIFrame* f = childList.FirstChild(); f != lastChild;
           f = f->GetNextSibling()) {
        children.AppendElement(f);
      }

      nsIFrame* insertionPoint = nullptr;
      int32_t imin = 0;
      int32_t max = children.Length();
      while (max > imin) {
        int32_t imid = imin + ((max - imin) / 2);
        nsIFrame* f = children[imid];
        int32_t compare =
          nsLayoutUtils::CompareTreePosition(f, firstNewFrame, firstNewFrameAncestors,
                                             notCommonAncestor ? containingBlock : nullptr);
        if (compare > 0) {
          
          max = imid;
          insertionPoint = imid > 0 ? children[imid - 1] : nullptr;
        } else if (compare < 0) {
          
          imin = imid + 1;
          insertionPoint = f;
        } else {
          
          
          
          NS_WARNING("Something odd happening???");
          insertionPoint = nullptr;
          for (uint32_t i = 0; i < children.Length(); ++i) {
            nsIFrame* f = children[i];
            if (nsLayoutUtils::CompareTreePosition(f, firstNewFrame,
                                                   firstNewFrameAncestors,
                                                   notCommonAncestor ?
                                                     containingBlock : nullptr) > 0) {
              break;
            }
            insertionPoint = f;
          }
          break;
        }
      }
      mFrameManager->InsertFrames(containingBlock, aChildListID,
                                  insertionPoint, aFrameItems);
    }
  }

  NS_POSTCONDITION(aFrameItems.IsEmpty(), "How did that happen?");
}


nsFrameConstructorSaveState::nsFrameConstructorSaveState()
  : mItems(nullptr),
    mSavedItems(nullptr),
    mChildListID(kPrincipalList),
    mState(nullptr),
    mSavedFixedItems(nullptr),
    mSavedFixedPosIsAbsPos(false)
{
}

nsFrameConstructorSaveState::~nsFrameConstructorSaveState()
{
  
  if (mItems) {
    NS_ASSERTION(mState, "Can't have mItems set without having a state!");
    mState->ProcessFrameInsertions(*mItems, mChildListID);
    *mItems = mSavedItems;
#ifdef DEBUG
    
    
    mSavedItems.Clear();
#endif
    if (mItems == &mState->mAbsoluteItems) {
      mState->mFixedPosIsAbsPos = mSavedFixedPosIsAbsPos;
      if (mSavedFixedPosIsAbsPos) {
        
        
        mState->mAbsoluteItems = mState->mFixedItems;
        mState->mFixedItems = mSavedFixedItems;
#ifdef DEBUG
        mSavedFixedItems.Clear();
#endif
      }
    }
    NS_ASSERTION(!mItems->LastChild() || !mItems->LastChild()->GetNextSibling(),
                 "Something corrupted our list");
  }
}

static
bool IsBorderCollapse(nsIFrame* aFrame)
{
  for (nsIFrame* frame = aFrame; frame; frame = frame->GetParent()) {
    if (nsGkAtoms::tableFrame == frame->GetType()) {
      return ((nsTableFrame*)frame)->IsBorderCollapse();
    }
  }
  NS_ASSERTION(false, "program error");
  return false;
}

















static void
MoveChildrenTo(nsPresContext* aPresContext,
               nsIFrame* aOldParent,
               nsContainerFrame* aNewParent,
               nsFrameList& aFrameList)
{
  bool sameGrandParent = aOldParent->GetParent() == aNewParent->GetParent();

  if (aNewParent->HasView() || aOldParent->HasView() || !sameGrandParent) {
    
    nsContainerFrame::ReparentFrameViewList(aFrameList, aOldParent, aNewParent);
  }

  for (nsFrameList::Enumerator e(aFrameList); !e.AtEnd(); e.Next()) {
    e.get()->SetParent(aNewParent);
  }

  if (aNewParent->PrincipalChildList().IsEmpty() &&
      (aNewParent->GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
    aNewParent->SetInitialChildList(kPrincipalList, aFrameList);
  } else {
    aNewParent->AppendFrames(kPrincipalList, aFrameList);
  }
}



nsCSSFrameConstructor::nsCSSFrameConstructor(nsIDocument *aDocument,
                                             nsIPresShell *aPresShell,
                                             nsStyleSet* aStyleSet)
  : nsFrameManager(aPresShell, aStyleSet)
  , mDocument(aDocument)
  , mRootElementFrame(nullptr)
  , mRootElementStyleFrame(nullptr)
  , mDocElementContainingBlock(nullptr)
  , mGfxScrollFrame(nullptr)
  , mPageSequenceFrame(nullptr)
  , mCurrentDepth(0)
  , mUpdateCount(0)
  , mQuotesDirty(false)
  , mCountersDirty(false)
  , mIsDestroyingFrameTree(false)
  , mHasRootAbsPosContainingBlock(false)
  , mAlwaysCreateFramesForIgnorableWhitespace(false)
{
#ifdef DEBUG
  static bool gFirstTime = true;
  if (gFirstTime) {
    gFirstTime = false;
    char* flags = PR_GetEnv("GECKO_FRAMECTOR_DEBUG_FLAGS");
    if (flags) {
      bool error = false;
      for (;;) {
        char* comma = PL_strchr(flags, ',');
        if (comma)
          *comma = '\0';

        bool found = false;
        FrameCtorDebugFlags* flag = gFlags;
        FrameCtorDebugFlags* limit = gFlags + NUM_DEBUG_FLAGS;
        while (flag < limit) {
          if (PL_strcasecmp(flag->name, flags) == 0) {
            *(flag->on) = true;
            printf("nsCSSFrameConstructor: setting %s debug flag on\n", flag->name);
            found = true;
            break;
          }
          ++flag;
        }

        if (! found)
          error = true;

        if (! comma)
          break;

        *comma = ',';
        flags = comma + 1;
      }

      if (error) {
        printf("Here are the available GECKO_FRAMECTOR_DEBUG_FLAGS:\n");
        FrameCtorDebugFlags* flag = gFlags;
        FrameCtorDebugFlags* limit = gFlags + NUM_DEBUG_FLAGS;
        while (flag < limit) {
          printf("  %s\n", flag->name);
          ++flag;
        }
        printf("Note: GECKO_FRAMECTOR_DEBUG_FLAGS is a comma separated list of flag\n");
        printf("names (no whitespace)\n");
      }
    }
  }
#endif
}

void
nsCSSFrameConstructor::NotifyDestroyingFrame(nsIFrame* aFrame)
{
  NS_PRECONDITION(mUpdateCount != 0,
                  "Should be in an update while destroying frames");

  if (aFrame->GetStateBits() & NS_FRAME_GENERATED_CONTENT) {
    if (mQuoteList.DestroyNodesFor(aFrame))
      QuotesDirty();
  }

  if (mCounterManager.DestroyNodesFor(aFrame)) {
    
    
    
    CountersDirty();
  }

  RestyleManager()->NotifyDestroyingFrame(aFrame);

  nsFrameManager::NotifyDestroyingFrame(aFrame);
}

struct nsGenConInitializer {
  nsAutoPtr<nsGenConNode> mNode;
  nsGenConList*           mList;
  void (nsCSSFrameConstructor::*mDirtyAll)();

  nsGenConInitializer(nsGenConNode* aNode, nsGenConList* aList,
                      void (nsCSSFrameConstructor::*aDirtyAll)())
    : mNode(aNode), mList(aList), mDirtyAll(aDirtyAll) {}
};

already_AddRefed<nsIContent>
nsCSSFrameConstructor::CreateGenConTextNode(nsFrameConstructorState& aState,
                                            const nsString& aString,
                                            nsRefPtr<nsTextNode>* aText,
                                            nsGenConInitializer* aInitializer)
{
  nsRefPtr<nsTextNode> content = new nsTextNode(mDocument->NodeInfoManager());
  content->SetText(aString, false);
  if (aText) {
    *aText = content;
  }
  if (aInitializer) {
    content->SetProperty(nsGkAtoms::genConInitializerProperty, aInitializer,
                         nsINode::DeleteProperty<nsGenConInitializer>);
    aState.mGeneratedTextNodesWithInitializer.AppendObject(content);
  }
  return content.forget();
}

already_AddRefed<nsIContent>
nsCSSFrameConstructor::CreateGeneratedContent(nsFrameConstructorState& aState,
                                              nsIContent*     aParentContent,
                                              nsStyleContext* aStyleContext,
                                              uint32_t        aContentIndex)
{
  
  const nsStyleContentData &data =
    aStyleContext->StyleContent()->ContentAt(aContentIndex);
  nsStyleContentType type = data.mType;

  if (eStyleContentType_Image == type) {
    if (!data.mContent.mImage) {
      
      
      return nullptr;
    }

    
    

    nsRefPtr<NodeInfo> nodeInfo;
    nodeInfo = mDocument->NodeInfoManager()->
      GetNodeInfo(nsGkAtoms::mozgeneratedcontentimage, nullptr,
                  kNameSpaceID_XHTML, nsIDOMNode::ELEMENT_NODE);

    nsCOMPtr<nsIContent> content;
    NS_NewGenConImageContent(getter_AddRefs(content), nodeInfo.forget(),
                             data.mContent.mImage);
    return content.forget();
  }

  switch (type) {
  case eStyleContentType_String:
    return CreateGenConTextNode(aState,
                                nsDependentString(data.mContent.mString),
                                nullptr, nullptr);

  case eStyleContentType_Attr:
    {
      nsCOMPtr<nsIAtom> attrName;
      int32_t attrNameSpace = kNameSpaceID_None;
      nsAutoString contentString(data.mContent.mString);

      int32_t barIndex = contentString.FindChar('|'); 
      if (-1 != barIndex) {
        nsAutoString  nameSpaceVal;
        contentString.Left(nameSpaceVal, barIndex);
        nsresult error;
        attrNameSpace = nameSpaceVal.ToInteger(&error);
        contentString.Cut(0, barIndex + 1);
        if (contentString.Length()) {
          if (mDocument->IsHTMLDocument() && aParentContent->IsHTMLElement()) {
            ToLowerCase(contentString);
          }
          attrName = do_GetAtom(contentString);
        }
      }
      else {
        if (mDocument->IsHTMLDocument() && aParentContent->IsHTMLElement()) {
          ToLowerCase(contentString);
        }
        attrName = do_GetAtom(contentString);
      }

      if (!attrName) {
        return nullptr;
      }

      nsCOMPtr<nsIContent> content;
      NS_NewAttributeContent(mDocument->NodeInfoManager(),
                             attrNameSpace, attrName, getter_AddRefs(content));
      return content.forget();
    }

  case eStyleContentType_Counter:
  case eStyleContentType_Counters:
    {
      nsCSSValue::Array* counters = data.mContent.mCounters;
      nsCounterList* counterList = mCounterManager.CounterListFor(
          nsDependentString(counters->Item(0).GetStringBufferValue()));

      nsCounterUseNode* node =
        new nsCounterUseNode(mPresShell->GetPresContext(),
                             counters, aContentIndex,
                             type == eStyleContentType_Counters);

      nsGenConInitializer* initializer =
        new nsGenConInitializer(node, counterList,
                                &nsCSSFrameConstructor::CountersDirty);
      return CreateGenConTextNode(aState, EmptyString(), &node->mText,
                                  initializer);
    }

  case eStyleContentType_Image:
    NS_NOTREACHED("handled by if above");
    return nullptr;

  case eStyleContentType_OpenQuote:
  case eStyleContentType_CloseQuote:
  case eStyleContentType_NoOpenQuote:
  case eStyleContentType_NoCloseQuote:
    {
      nsQuoteNode* node =
        new nsQuoteNode(type, aContentIndex);

      nsGenConInitializer* initializer =
        new nsGenConInitializer(node, &mQuoteList,
                                &nsCSSFrameConstructor::QuotesDirty);
      return CreateGenConTextNode(aState, EmptyString(), &node->mText,
                                  initializer);
    }

  case eStyleContentType_AltContent:
    {
      
      
      
      
      
      if (aParentContent->HasAttr(kNameSpaceID_None, nsGkAtoms::alt)) {
        nsCOMPtr<nsIContent> content;
        NS_NewAttributeContent(mDocument->NodeInfoManager(),
                               kNameSpaceID_None, nsGkAtoms::alt, getter_AddRefs(content));
        return content.forget();
      }

      if (aParentContent->IsHTMLElement() &&
          aParentContent->NodeInfo()->Equals(nsGkAtoms::input)) {
        if (aParentContent->HasAttr(kNameSpaceID_None, nsGkAtoms::value)) {
          nsCOMPtr<nsIContent> content;
          NS_NewAttributeContent(mDocument->NodeInfoManager(),
                                 kNameSpaceID_None, nsGkAtoms::value, getter_AddRefs(content));
          return content.forget();
        }

        nsXPIDLString temp;
        nsContentUtils::GetLocalizedString(nsContentUtils::eFORMS_PROPERTIES,
                                           "Submit", temp);
        return CreateGenConTextNode(aState, temp, nullptr, nullptr);
      }

      break;
    }

  case eStyleContentType_Uninitialized:
    NS_NOTREACHED("uninitialized content type");
    return nullptr;
  } 

  return nullptr;
}
















void
nsCSSFrameConstructor::CreateGeneratedContentItem(nsFrameConstructorState& aState,
                                                  nsContainerFrame* aParentFrame,
                                                  nsIContent*      aParentContent,
                                                  nsStyleContext*  aStyleContext,
                                                  nsCSSPseudoElements::Type aPseudoElement,
                                                  FrameConstructionItemList& aItems)
{
  MOZ_ASSERT(aPseudoElement == nsCSSPseudoElements::ePseudo_before ||
             aPseudoElement == nsCSSPseudoElements::ePseudo_after,
             "unexpected aPseudoElement");

  
  if (!aParentContent->IsElement()) {
    NS_ERROR("Bogus generated content parent");
    return;
  }

  nsStyleSet *styleSet = mPresShell->StyleSet();

  
  nsRefPtr<nsStyleContext> pseudoStyleContext;
  pseudoStyleContext =
    styleSet->ProbePseudoElementStyle(aParentContent->AsElement(),
                                      aPseudoElement,
                                      aStyleContext,
                                      aState.mTreeMatchContext);
  if (!pseudoStyleContext)
    return;

  bool isBefore = aPseudoElement == nsCSSPseudoElements::ePseudo_before;

  
  
  nsRefPtr<NodeInfo> nodeInfo;
  nsIAtom* elemName = isBefore ?
    nsGkAtoms::mozgeneratedcontentbefore : nsGkAtoms::mozgeneratedcontentafter;
  nodeInfo = mDocument->NodeInfoManager()->GetNodeInfo(elemName, nullptr,
                                                       kNameSpaceID_None,
                                                       nsIDOMNode::ELEMENT_NODE);
  nsCOMPtr<Element> container;
  nsresult rv = NS_NewXMLElement(getter_AddRefs(container), nodeInfo.forget());
  if (NS_FAILED(rv))
    return;
  container->SetIsNativeAnonymousRoot();

  
  
  
  nsIDocument* bindDocument =
    aParentContent->HasFlag(NODE_IS_IN_SHADOW_TREE) ? nullptr : mDocument;
  rv = container->BindToTree(bindDocument, aParentContent, aParentContent, true);
  if (NS_FAILED(rv)) {
    container->UnbindFromTree();
    return;
  }

  RestyleManager::ReframingStyleContexts* rsc =
    RestyleManager()->GetReframingStyleContexts();
  if (rsc) {
    nsStyleContext* oldStyleContext = rsc->Get(container, aPseudoElement);
    if (oldStyleContext) {
      RestyleManager::TryStartingTransition(aState.mPresContext,
                                            container,
                                            oldStyleContext,
                                            &pseudoStyleContext);
    }
  }

  uint32_t contentCount = pseudoStyleContext->StyleContent()->ContentCount();
  for (uint32_t contentIndex = 0; contentIndex < contentCount; contentIndex++) {
    nsCOMPtr<nsIContent> content =
      CreateGeneratedContent(aState, aParentContent, pseudoStyleContext,
                             contentIndex);
    if (content) {
      container->AppendChildTo(content, false);
    }
  }

  AddFrameConstructionItemsInternal(aState, container, aParentFrame, elemName,
                                    kNameSpaceID_None, true,
                                    pseudoStyleContext,
                                    ITEM_IS_GENERATED_CONTENT, nullptr,
                                    aItems);
}















static bool
IsTablePseudo(nsIFrame* aFrame)
{
  nsIAtom* pseudoType = aFrame->StyleContext()->GetPseudo();
  return pseudoType &&
    (pseudoType == nsCSSAnonBoxes::table ||
     pseudoType == nsCSSAnonBoxes::inlineTable ||
     pseudoType == nsCSSAnonBoxes::tableColGroup ||
     pseudoType == nsCSSAnonBoxes::tableRowGroup ||
     pseudoType == nsCSSAnonBoxes::tableRow ||
     pseudoType == nsCSSAnonBoxes::tableCell ||
     (pseudoType == nsCSSAnonBoxes::cellContent &&
      aFrame->GetParent()->StyleContext()->GetPseudo() ==
        nsCSSAnonBoxes::tableCell) ||
     (pseudoType == nsCSSAnonBoxes::tableOuter &&
      (aFrame->GetFirstPrincipalChild()->StyleContext()->GetPseudo() ==
         nsCSSAnonBoxes::table ||
       aFrame->GetFirstPrincipalChild()->StyleContext()->GetPseudo() ==
         nsCSSAnonBoxes::inlineTable)));
}

static bool
IsRubyPseudo(nsIFrame* aFrame)
{
  nsIAtom* pseudoType = aFrame->StyleContext()->GetPseudo();
  return pseudoType &&
    (pseudoType == nsCSSAnonBoxes::ruby ||
     pseudoType == nsCSSAnonBoxes::rubyBase ||
     pseudoType == nsCSSAnonBoxes::rubyText ||
     pseudoType == nsCSSAnonBoxes::rubyBaseContainer ||
     pseudoType == nsCSSAnonBoxes::rubyTextContainer);
}

static bool
IsTableOrRubyPseudo(nsIFrame* aFrame)
{
  return IsTablePseudo(aFrame) || IsRubyPseudo(aFrame);
}


nsCSSFrameConstructor::ParentType
nsCSSFrameConstructor::GetParentType(nsIAtom* aFrameType)
{
  if (aFrameType == nsGkAtoms::tableFrame) {
    return eTypeTable;
  }
  if (aFrameType == nsGkAtoms::tableRowGroupFrame) {
    return eTypeRowGroup;
  }
  if (aFrameType == nsGkAtoms::tableRowFrame) {
    return eTypeRow;
  }
  if (aFrameType == nsGkAtoms::tableColGroupFrame) {
    return eTypeColGroup;
  }
  if (aFrameType == nsGkAtoms::rubyBaseContainerFrame) {
    return eTypeRubyBaseContainer;
  }
  if (aFrameType == nsGkAtoms::rubyTextContainerFrame) {
    return eTypeRubyTextContainer;
  } 
  if (aFrameType == nsGkAtoms::rubyFrame) {
    return eTypeRuby;
  }

  return eTypeBlock;
}

static nsContainerFrame*
AdjustCaptionParentFrame(nsContainerFrame* aParentFrame)
{
  if (nsGkAtoms::tableFrame == aParentFrame->GetType()) {
    return aParentFrame->GetParent();
  }
  return aParentFrame;
}







static bool
GetCaptionAdjustedParent(nsContainerFrame*  aParentFrame,
                         const nsIFrame*    aChildFrame,
                         nsContainerFrame** aAdjParentFrame)
{
  *aAdjParentFrame = aParentFrame;
  bool haveCaption = false;

  if (aChildFrame->IsTableCaption()) {
    haveCaption = true;
    *aAdjParentFrame = ::AdjustCaptionParentFrame(aParentFrame);
  }
  return haveCaption;
}

void
nsCSSFrameConstructor::AdjustParentFrame(nsContainerFrame**           aParentFrame,
                                         const FrameConstructionData* aFCData,
                                         nsStyleContext*              aStyleContext)
{
  NS_PRECONDITION(aStyleContext, "Must have child's style context");
  NS_PRECONDITION(aFCData, "Must have frame construction data");

  bool tablePart = ((aFCData->mBits & FCDATA_IS_TABLE_PART) != 0);

  if (tablePart && aStyleContext->StyleDisplay()->mDisplay ==
      NS_STYLE_DISPLAY_TABLE_CAPTION) {
    *aParentFrame = ::AdjustCaptionParentFrame(*aParentFrame);
  }
}


static void
PullOutCaptionFrames(nsFrameItems& aItems, nsFrameItems& aCaptions)
{
  nsIFrame* child = aItems.FirstChild();
  while (child) {
    nsIFrame* nextSibling = child->GetNextSibling();
    if (child->IsTableCaption()) {
      aItems.RemoveFrame(child);
      aCaptions.AddChild(child);
    }
    child = nextSibling;
  }
}






nsIFrame*
nsCSSFrameConstructor::ConstructTable(nsFrameConstructorState& aState,
                                      FrameConstructionItem&   aItem,
                                      nsContainerFrame*        aParentFrame,
                                      const nsStyleDisplay*    aDisplay,
                                      nsFrameItems&            aFrameItems)
{
  NS_PRECONDITION(aDisplay->mDisplay == NS_STYLE_DISPLAY_TABLE ||
                  aDisplay->mDisplay == NS_STYLE_DISPLAY_INLINE_TABLE,
                  "Unexpected call");

  nsIContent* const content = aItem.mContent;
  nsStyleContext* const styleContext = aItem.mStyleContext;
  const uint32_t nameSpaceID = aItem.mNameSpaceID;

  
  nsRefPtr<nsStyleContext> outerStyleContext;
  outerStyleContext = mPresShell->StyleSet()->
    ResolveAnonymousBoxStyle(nsCSSAnonBoxes::tableOuter, styleContext);

  
  nsContainerFrame* newFrame;
  if (kNameSpaceID_MathML == nameSpaceID)
    newFrame = NS_NewMathMLmtableOuterFrame(mPresShell, outerStyleContext);
  else
    newFrame = NS_NewTableOuterFrame(mPresShell, outerStyleContext);

  nsContainerFrame* geometricParent =
    aState.GetGeometricParent(outerStyleContext->StyleDisplay(),
                              aParentFrame);

  
  InitAndRestoreFrame(aState, content, geometricParent, newFrame);

  
  nsContainerFrame* innerFrame;
  if (kNameSpaceID_MathML == nameSpaceID)
    innerFrame = NS_NewMathMLmtableFrame(mPresShell, styleContext);
  else
    innerFrame = NS_NewTableFrame(mPresShell, styleContext);

  InitAndRestoreFrame(aState, content, newFrame, innerFrame);

  
  SetInitialSingleChild(newFrame, innerFrame);

  aState.AddChild(newFrame, aFrameItems, content, styleContext, aParentFrame);

  if (!mRootElementFrame) {
    
    
    mRootElementFrame = newFrame;
  }

  nsFrameItems childItems;

  
  nsFrameConstructorSaveState absoluteSaveState;
  const nsStyleDisplay* display = outerStyleContext->StyleDisplay();

  
  newFrame->AddStateBits(NS_FRAME_CAN_HAVE_ABSPOS_CHILDREN);
  if (display->IsAbsPosContainingBlock(newFrame)) {
    aState.PushAbsoluteContainingBlock(newFrame, newFrame, absoluteSaveState);
  }
  NS_ASSERTION(aItem.mAnonChildren.IsEmpty(),
               "nsIAnonymousContentCreator::CreateAnonymousContent "
               "implementations for table frames are not currently expected "
               "to output a list where the items have their own children");
  if (aItem.mFCData->mBits & FCDATA_USE_CHILD_ITEMS) {
    ConstructFramesFromItemList(aState, aItem.mChildItems,
                                innerFrame, childItems);
  } else {
    ProcessChildren(aState, content, styleContext, innerFrame,
                    true, childItems, false, aItem.mPendingBinding);
  }

  nsFrameItems captionItems;
  PullOutCaptionFrames(childItems, captionItems);

  
  innerFrame->SetInitialChildList(kPrincipalList, childItems);

  
  if (captionItems.NotEmpty()) {
    newFrame->SetInitialChildList(nsIFrame::kCaptionList, captionItems);
  }

  return newFrame;
}

static void
MakeTablePartAbsoluteContainingBlockIfNeeded(nsFrameConstructorState&     aState,
                                             const nsStyleDisplay*        aDisplay,
                                             nsFrameConstructorSaveState& aAbsSaveState,
                                             nsContainerFrame*            aFrame)
{
  
  
  
  
  
  
  
  
  if (aDisplay->IsAbsPosContainingBlock(aFrame)) {
    aFrame->AddStateBits(NS_FRAME_CAN_HAVE_ABSPOS_CHILDREN);
    aState.PushAbsoluteContainingBlock(aFrame, aFrame, aAbsSaveState);
    nsTableFrame::RegisterPositionedTablePart(aFrame);
  }
}

nsIFrame*
nsCSSFrameConstructor::ConstructTableRowOrRowGroup(nsFrameConstructorState& aState,
                                                   FrameConstructionItem&   aItem,
                                                   nsContainerFrame*        aParentFrame,
                                                   const nsStyleDisplay*    aDisplay,
                                                   nsFrameItems&            aFrameItems)
{
  NS_PRECONDITION(aDisplay->mDisplay == NS_STYLE_DISPLAY_TABLE_ROW ||
                  aDisplay->mDisplay == NS_STYLE_DISPLAY_TABLE_ROW_GROUP ||
                  aDisplay->mDisplay == NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP ||
                  aDisplay->mDisplay == NS_STYLE_DISPLAY_TABLE_HEADER_GROUP,
                  "Unexpected call");
  MOZ_ASSERT(aItem.mStyleContext->StyleDisplay() == aDisplay,
             "Display style doesn't match style context");
  nsIContent* const content = aItem.mContent;
  nsStyleContext* const styleContext = aItem.mStyleContext;
  const uint32_t nameSpaceID = aItem.mNameSpaceID;

  nsContainerFrame* newFrame;
  if (aDisplay->mDisplay == NS_STYLE_DISPLAY_TABLE_ROW) {
    if (kNameSpaceID_MathML == nameSpaceID)
      newFrame = NS_NewMathMLmtrFrame(mPresShell, styleContext);
    else
      newFrame = NS_NewTableRowFrame(mPresShell, styleContext);
  } else {
    newFrame = NS_NewTableRowGroupFrame(mPresShell, styleContext);
  }

  InitAndRestoreFrame(aState, content, aParentFrame, newFrame);

  nsFrameConstructorSaveState absoluteSaveState;
  MakeTablePartAbsoluteContainingBlockIfNeeded(aState, aDisplay,
                                               absoluteSaveState,
                                               newFrame);

  nsFrameItems childItems;
  NS_ASSERTION(aItem.mAnonChildren.IsEmpty(),
               "nsIAnonymousContentCreator::CreateAnonymousContent "
               "implementations for table frames are not currently expected "
               "to output a list where the items have their own children");
  if (aItem.mFCData->mBits & FCDATA_USE_CHILD_ITEMS) {
    ConstructFramesFromItemList(aState, aItem.mChildItems, newFrame,
                                childItems);
  } else {
    ProcessChildren(aState, content, styleContext, newFrame,
                    true, childItems, false, aItem.mPendingBinding);
  }

  newFrame->SetInitialChildList(kPrincipalList, childItems);
  aFrameItems.AddChild(newFrame);
  return newFrame;
}

nsIFrame*
nsCSSFrameConstructor::ConstructTableCol(nsFrameConstructorState& aState,
                                         FrameConstructionItem&   aItem,
                                         nsContainerFrame*        aParentFrame,
                                         const nsStyleDisplay*    aStyleDisplay,
                                         nsFrameItems&            aFrameItems)
{
  nsIContent* const content = aItem.mContent;
  nsStyleContext* const styleContext = aItem.mStyleContext;

  nsTableColFrame* colFrame = NS_NewTableColFrame(mPresShell, styleContext);
  InitAndRestoreFrame(aState, content, aParentFrame, colFrame);

  NS_ASSERTION(colFrame->StyleContext() == styleContext,
               "Unexpected style context");

  aFrameItems.AddChild(colFrame);

  
  int32_t span = colFrame->GetSpan();
  for (int32_t spanX = 1; spanX < span; spanX++) {
    nsTableColFrame* newCol = NS_NewTableColFrame(mPresShell, styleContext);
    InitAndRestoreFrame(aState, content, aParentFrame, newCol, false);
    aFrameItems.LastChild()->SetNextContinuation(newCol);
    newCol->SetPrevContinuation(aFrameItems.LastChild());
    aFrameItems.AddChild(newCol);
    newCol->SetColType(eColAnonymousCol);
  }

  return colFrame;
}

nsIFrame*
nsCSSFrameConstructor::ConstructTableCell(nsFrameConstructorState& aState,
                                          FrameConstructionItem&   aItem,
                                          nsContainerFrame*        aParentFrame,
                                          const nsStyleDisplay*    aDisplay,
                                          nsFrameItems&            aFrameItems)
{
  NS_PRECONDITION(aDisplay->mDisplay == NS_STYLE_DISPLAY_TABLE_CELL,
                  "Unexpected call");

  nsIContent* const content = aItem.mContent;
  nsStyleContext* const styleContext = aItem.mStyleContext;
  const uint32_t nameSpaceID = aItem.mNameSpaceID;

  bool borderCollapse = IsBorderCollapse(aParentFrame);
  nsContainerFrame* newFrame;
  
  
  
  
  
  
  
  if (kNameSpaceID_MathML == nameSpaceID && !borderCollapse)
    newFrame = NS_NewMathMLmtdFrame(mPresShell, styleContext);
  else
    
    
    
    newFrame = NS_NewTableCellFrame(mPresShell, styleContext, borderCollapse);

  
  InitAndRestoreFrame(aState, content, aParentFrame, newFrame);

  
  nsRefPtr<nsStyleContext> innerPseudoStyle;
  innerPseudoStyle = mPresShell->StyleSet()->
    ResolveAnonymousBoxStyle(nsCSSAnonBoxes::cellContent, styleContext);

  
  bool isBlock;
  nsContainerFrame* cellInnerFrame;
  if (kNameSpaceID_MathML == nameSpaceID) {
    cellInnerFrame = NS_NewMathMLmtdInnerFrame(mPresShell, innerPseudoStyle);
    isBlock = false;
  } else {
    cellInnerFrame = NS_NewBlockFormattingContext(mPresShell, innerPseudoStyle);
    isBlock = true;
  }

  InitAndRestoreFrame(aState, content, newFrame, cellInnerFrame);

  nsFrameConstructorSaveState absoluteSaveState;
  MakeTablePartAbsoluteContainingBlockIfNeeded(aState, aDisplay,
                                               absoluteSaveState,
                                               newFrame);

  nsFrameItems childItems;
  NS_ASSERTION(aItem.mAnonChildren.IsEmpty(),
               "nsIAnonymousContentCreator::CreateAnonymousContent "
               "implementations for table frames are not currently expected "
               "to output a list where the items have their own children");
  if (aItem.mFCData->mBits & FCDATA_USE_CHILD_ITEMS) {
    
    
    
    
    nsFrameConstructorSaveState floatSaveState;
    if (!isBlock) { 
      aState.PushFloatContainingBlock(nullptr, floatSaveState);
    } else {
      aState.PushFloatContainingBlock(cellInnerFrame, floatSaveState);
    }

    ConstructFramesFromItemList(aState, aItem.mChildItems, cellInnerFrame,
                                childItems);
  } else {
    
    ProcessChildren(aState, content, styleContext, cellInnerFrame,
                    true, childItems, isBlock, aItem.mPendingBinding);
  }

  cellInnerFrame->SetInitialChildList(kPrincipalList, childItems);
  SetInitialSingleChild(newFrame, cellInnerFrame);
  aFrameItems.AddChild(newFrame);
  return newFrame;
}

static inline bool
NeedFrameFor(const nsFrameConstructorState& aState,
             nsIFrame*   aParentFrame,
             nsIContent* aChildContent)
{
  
  
  NS_PRECONDITION(!aChildContent->GetPrimaryFrame() ||
                  aState.mCreatingExtraFrames ||
                  aChildContent->GetPrimaryFrame()->GetContent() != aChildContent,
                  "Why did we get called?");

  
  
  
  
  
  

  
  
  
  
  
  
  if ((aParentFrame &&
       (!aParentFrame->IsFrameOfType(nsIFrame::eExcludesIgnorableWhitespace) ||
        aParentFrame->IsGeneratedContentFrame())) ||
      !aChildContent->IsNodeOfType(nsINode::eTEXT)) {
    return true;
  }

  aChildContent->SetFlags(NS_CREATE_FRAME_IF_NON_WHITESPACE |
                          NS_REFRAME_IF_WHITESPACE);
  return !aChildContent->TextIsOnlyWhitespace();
}





static bool CheckOverflow(nsPresContext* aPresContext,
                            const nsStyleDisplay* aDisplay)
{
  if (aDisplay->mOverflowX == NS_STYLE_OVERFLOW_VISIBLE &&
      aDisplay->mScrollBehavior == NS_STYLE_SCROLL_BEHAVIOR_AUTO &&
      aDisplay->mScrollSnapTypeX == NS_STYLE_SCROLL_SNAP_TYPE_NONE &&
      aDisplay->mScrollSnapTypeY == NS_STYLE_SCROLL_SNAP_TYPE_NONE &&
      aDisplay->mScrollSnapPointsX == nsStyleCoord(eStyleUnit_None) &&
      aDisplay->mScrollSnapPointsY == nsStyleCoord(eStyleUnit_None) &&
      !aDisplay->mScrollSnapDestination.mXPosition.mHasPercent &&
      !aDisplay->mScrollSnapDestination.mYPosition.mHasPercent &&
      aDisplay->mScrollSnapDestination.mXPosition.mLength == 0 &&
      aDisplay->mScrollSnapDestination.mYPosition.mLength == 0) {
    return false;
  }

  if (aDisplay->mOverflowX == NS_STYLE_OVERFLOW_CLIP) {
    aPresContext->SetViewportScrollbarStylesOverride(
                                    ScrollbarStyles(NS_STYLE_OVERFLOW_HIDDEN,
                                                    NS_STYLE_OVERFLOW_HIDDEN,
                                                    aDisplay));
  } else {
    aPresContext->SetViewportScrollbarStylesOverride(
                                    ScrollbarStyles(aDisplay));
  }
  return true;
}









nsIContent*
nsCSSFrameConstructor::PropagateScrollToViewport()
{
  
  nsPresContext* presContext = mPresShell->GetPresContext();
  presContext->SetViewportScrollbarStylesOverride(
                             ScrollbarStyles(NS_STYLE_OVERFLOW_AUTO,
                                             NS_STYLE_OVERFLOW_AUTO));

  
  
  if (presContext->IsPaginated()) {
    return nullptr;
  }

  Element* docElement = mDocument->GetRootElement();

  
  nsStyleSet *styleSet = mPresShell->StyleSet();
  nsRefPtr<nsStyleContext> rootStyle;
  rootStyle = styleSet->ResolveStyleFor(docElement, nullptr);
  if (CheckOverflow(presContext, rootStyle->StyleDisplay())) {
    
    return docElement;
  }

  
  
  
  
  
  
  nsCOMPtr<nsIDOMHTMLDocument> htmlDoc(do_QueryInterface(mDocument));
  if (!htmlDoc || !docElement->IsHTMLElement()) {
    return nullptr;
  }

  nsCOMPtr<nsIDOMHTMLElement> body;
  htmlDoc->GetBody(getter_AddRefs(body));
  nsCOMPtr<nsIContent> bodyElement = do_QueryInterface(body);

  if (!bodyElement ||
      !bodyElement->NodeInfo()->Equals(nsGkAtoms::body)) {
    
    return nullptr;
  }

  nsRefPtr<nsStyleContext> bodyStyle;
  bodyStyle = styleSet->ResolveStyleFor(bodyElement->AsElement(), rootStyle);

  if (CheckOverflow(presContext, bodyStyle->StyleDisplay())) {
    
    return bodyElement;
  }

  return nullptr;
}

nsIFrame*
nsCSSFrameConstructor::ConstructDocElementFrame(Element*                 aDocElement,
                                                nsILayoutHistoryState*   aFrameState)
{
  MOZ_ASSERT(GetRootFrame(),
             "No viewport?  Someone forgot to call ConstructRootFrame!");
  MOZ_ASSERT(!mDocElementContainingBlock,
             "Shouldn't have a doc element containing block here");

  
  
  
#ifdef DEBUG
  nsIContent* propagatedScrollFrom =
#endif
    PropagateScrollToViewport();

  SetUpDocElementContainingBlock(aDocElement);

  NS_ASSERTION(mDocElementContainingBlock, "Should have parent by now");

  nsFrameConstructorState state(mPresShell,
                                GetAbsoluteContainingBlock(mDocElementContainingBlock, FIXED_POS),
                                nullptr,
                                nullptr, aFrameState);
  
  
  state.mTreeMatchContext.InitAncestors(nullptr);

  
  if (!mTempFrameTreeState)
    state.mPresShell->CaptureHistoryState(getter_AddRefs(mTempFrameTreeState));

  
  
  
  
  
  
  aDocElement->UnsetFlags(ELEMENT_ALL_RESTYLE_FLAGS);

  
  
  
  
  nsRefPtr<nsStyleContext> styleContext;
  styleContext = mPresShell->StyleSet()->ResolveStyleFor(aDocElement,
                                                         nullptr);

  const nsStyleDisplay* display = styleContext->StyleDisplay();

  
  if (display->mBinding) {
    
    nsresult rv;
    bool resolveStyle;

    nsXBLService* xblService = nsXBLService::GetInstance();
    if (!xblService) {
      return nullptr;
    }

    nsRefPtr<nsXBLBinding> binding;
    rv = xblService->LoadBindings(aDocElement, display->mBinding->GetURI(),
                                  display->mBinding->mOriginPrincipal,
                                  getter_AddRefs(binding), &resolveStyle);
    if (NS_FAILED(rv) && rv != NS_ERROR_XBL_BLOCKED)
      return nullptr; 

    if (binding) {
      
      
      
      mDocument->BindingManager()->AddToAttachedQueue(binding);
    }

    if (resolveStyle) {
      
      
      
      
      styleContext = mPresShell->StyleSet()->ResolveStyleFor(aDocElement,
                                                             nullptr);
      display = styleContext->StyleDisplay();
    }
  }

  

#ifdef DEBUG
  NS_ASSERTION(!display->IsScrollableOverflow() ||
               state.mPresContext->IsPaginated() ||
               propagatedScrollFrom == aDocElement,
               "Scrollbars should have been propagated to the viewport");
#endif

  if (MOZ_UNLIKELY(display->mDisplay == NS_STYLE_DISPLAY_NONE)) {
    SetUndisplayedContent(aDocElement, styleContext);
    return nullptr;
  }

  TreeMatchContext::AutoAncestorPusher ancestorPusher(state.mTreeMatchContext);
  ancestorPusher.PushAncestorAndStyleScope(aDocElement);

  
  styleContext->StartBackgroundImageLoads();

  nsFrameConstructorSaveState absoluteSaveState;
  if (mHasRootAbsPosContainingBlock) {
    
    
    mDocElementContainingBlock->AddStateBits(NS_FRAME_CAN_HAVE_ABSPOS_CHILDREN);
    state.PushAbsoluteContainingBlock(mDocElementContainingBlock,
                                      mDocElementContainingBlock,
                                      absoluteSaveState);
  }

  
  
  

  
  
  
  
  
  
  nsContainerFrame* contentFrame;
  nsIFrame* newFrame;
  bool processChildren = false;

  
#ifdef MOZ_XUL
  if (aDocElement->IsXULElement()) {
    contentFrame = NS_NewDocElementBoxFrame(mPresShell, styleContext);
    InitAndRestoreFrame(state, aDocElement, mDocElementContainingBlock,
                        contentFrame);
    newFrame = contentFrame;
    processChildren = true;
  }
  else
#endif
  if (aDocElement->IsSVGElement()) {
    if (!aDocElement->IsSVGElement(nsGkAtoms::svg)) {
      return nullptr;
    }
    
    

    
    
    
    
    
    static const FrameConstructionData rootSVGData = FCDATA_DECL(0, nullptr);
    already_AddRefed<nsStyleContext> extraRef =
      nsRefPtr<nsStyleContext>(styleContext).forget();
    FrameConstructionItem item(&rootSVGData, aDocElement,
                               aDocElement->NodeInfo()->NameAtom(),
                               kNameSpaceID_SVG, nullptr, extraRef, true,
                               nullptr);

    nsFrameItems frameItems;
    contentFrame = static_cast<nsContainerFrame*>(
      ConstructOuterSVG(state, item, mDocElementContainingBlock,
                        styleContext->StyleDisplay(),
                        frameItems));
    newFrame = frameItems.FirstChild();
    NS_ASSERTION(frameItems.OnlyChild(), "multiple root element frames");
  } else if (display->mDisplay == NS_STYLE_DISPLAY_FLEX) {
    contentFrame = NS_NewFlexContainerFrame(mPresShell, styleContext);
    InitAndRestoreFrame(state, aDocElement, mDocElementContainingBlock,
                        contentFrame);
    newFrame = contentFrame;
    processChildren = true;
  } else if (display->mDisplay == NS_STYLE_DISPLAY_GRID) {
    contentFrame = NS_NewGridContainerFrame(mPresShell, styleContext);
    InitAndRestoreFrame(state, aDocElement, mDocElementContainingBlock,
                        contentFrame);
    newFrame = contentFrame;
    processChildren = true;
  } else if (display->mDisplay == NS_STYLE_DISPLAY_TABLE) {
    
    

    
    
    
    
    
    static const FrameConstructionData rootTableData = FCDATA_DECL(0, nullptr);
    already_AddRefed<nsStyleContext> extraRef =
      nsRefPtr<nsStyleContext>(styleContext).forget();
    FrameConstructionItem item(&rootTableData, aDocElement,
                               aDocElement->NodeInfo()->NameAtom(),
                               kNameSpaceID_None, nullptr, extraRef, true,
                               nullptr);

    nsFrameItems frameItems;
    
    contentFrame = static_cast<nsContainerFrame*>(
      ConstructTable(state, item, mDocElementContainingBlock,
                     styleContext->StyleDisplay(),
                     frameItems));
    newFrame = frameItems.FirstChild();
    NS_ASSERTION(frameItems.OnlyChild(), "multiple root element frames");
  } else {
    MOZ_ASSERT(display->mDisplay == NS_STYLE_DISPLAY_BLOCK,
               "Unhandled display type for root element");
    contentFrame = NS_NewBlockFormattingContext(mPresShell, styleContext);
    nsFrameItems frameItems;
    
    ConstructBlock(state, display, aDocElement,
                   state.GetGeometricParent(display,
                                            mDocElementContainingBlock),
                   mDocElementContainingBlock, styleContext,
                   &contentFrame, frameItems,
                   display->IsAbsPosContainingBlock(contentFrame) ? contentFrame : nullptr,
                   nullptr);
    newFrame = frameItems.FirstChild();
    NS_ASSERTION(frameItems.OnlyChild(), "multiple root element frames");
  }

  MOZ_ASSERT(newFrame);
  MOZ_ASSERT(contentFrame);

  NS_ASSERTION(processChildren ? !mRootElementFrame :
                 mRootElementFrame == contentFrame,
               "unexpected mRootElementFrame");
  mRootElementFrame = contentFrame;

  
  
  
  contentFrame->GetParentStyleContext(&mRootElementStyleFrame);
  bool isChild = mRootElementStyleFrame &&
                 mRootElementStyleFrame->GetParent() == contentFrame;
  if (!isChild) {
    mRootElementStyleFrame = mRootElementFrame;
  }

  if (processChildren) {
    
    nsFrameItems childItems;

    NS_ASSERTION(!nsLayoutUtils::GetAsBlock(contentFrame) &&
                 !contentFrame->IsFrameOfType(nsIFrame::eSVG),
                 "Only XUL frames should reach here");
    
    ProcessChildren(state, aDocElement, styleContext, contentFrame, true,
                    childItems, false, nullptr);

    
    contentFrame->SetInitialChildList(kPrincipalList, childItems);
  }

  
  aDocElement->SetPrimaryFrame(contentFrame);

  SetInitialSingleChild(mDocElementContainingBlock, newFrame);

  
  if (mDocElementContainingBlock->GetType() == nsGkAtoms::canvasFrame) {
    ConstructAnonymousContentForCanvas(state, mDocElementContainingBlock,
                                       aDocElement);
  }

  return newFrame;
}


nsIFrame*
nsCSSFrameConstructor::ConstructRootFrame()
{
  AUTO_LAYOUT_PHASE_ENTRY_POINT(mPresShell->GetPresContext(), FrameC);

  nsStyleSet *styleSet = mPresShell->StyleSet();

  
  
  {
    styleSet->SetBindingManager(mDocument->BindingManager());
  }

  
  nsRefPtr<nsStyleContext> viewportPseudoStyle =
    styleSet->ResolveAnonymousBoxStyle(nsCSSAnonBoxes::viewport, nullptr);
  ViewportFrame* viewportFrame =
    NS_NewViewportFrame(mPresShell, viewportPseudoStyle);

  
  
  
  viewportFrame->Init(nullptr, nullptr, nullptr);

  
  nsView* rootView = mPresShell->GetViewManager()->GetRootView();
  viewportFrame->SetView(rootView);

  nsContainerFrame::SyncFrameViewProperties(mPresShell->GetPresContext(), viewportFrame,
                                            viewportPseudoStyle, rootView);
  nsContainerFrame::SyncWindowProperties(mPresShell->GetPresContext(), viewportFrame,
                                         rootView);

  
  viewportFrame->AddStateBits(NS_FRAME_CAN_HAVE_ABSPOS_CHILDREN);
  viewportFrame->MarkAsAbsoluteContainingBlock();

  return viewportFrame;
}

void
nsCSSFrameConstructor::SetUpDocElementContainingBlock(nsIContent* aDocElement)
{
  NS_PRECONDITION(aDocElement, "No element?");
  NS_PRECONDITION(!aDocElement->GetParent(), "Not root content?");
  NS_PRECONDITION(aDocElement->GetCurrentDoc(), "Not in a document?");
  NS_PRECONDITION(aDocElement->GetCurrentDoc()->GetRootElement() ==
                  aDocElement, "Not the root of the document?");

  


















































  


  
  
  
  
  
  
  

  nsPresContext* presContext = mPresShell->GetPresContext();
  bool isPaginated = presContext->IsRootPaginatedDocument();
  nsContainerFrame* viewportFrame = static_cast<nsContainerFrame*>(GetRootFrame());
  nsStyleContext* viewportPseudoStyle = viewportFrame->StyleContext();

  nsContainerFrame* rootFrame = nullptr;
  nsIAtom* rootPseudo;

  if (!isPaginated) {
#ifdef MOZ_XUL
    if (aDocElement->IsXULElement())
    {
      
      rootFrame = NS_NewRootBoxFrame(mPresShell, viewportPseudoStyle);
    } else
#endif
    {
      
      rootFrame = NS_NewCanvasFrame(mPresShell, viewportPseudoStyle);
      mHasRootAbsPosContainingBlock = true;
    }

    rootPseudo = nsCSSAnonBoxes::canvas;
    mDocElementContainingBlock = rootFrame;
  } else {
    
    rootFrame = NS_NewSimplePageSequenceFrame(mPresShell, viewportPseudoStyle);
    mPageSequenceFrame = rootFrame;
    rootPseudo = nsCSSAnonBoxes::pageSequence;
  }


  

  
  
  
  

  bool isHTML = aDocElement->IsHTMLElement();
  bool isXUL = false;

  if (!isHTML) {
    isXUL = aDocElement->IsXULElement();
  }

  
  bool isScrollable = isPaginated ? presContext->HasPaginatedScrolling() : !isXUL;

  
  
  
  NS_ASSERTION(!isScrollable || !isXUL,
               "XUL documents should never be scrollable - see above");

  nsContainerFrame* newFrame = rootFrame;
  nsRefPtr<nsStyleContext> rootPseudoStyle;
  
  
  nsFrameConstructorState state(mPresShell, nullptr, nullptr, nullptr);

  
  nsContainerFrame* parentFrame = viewportFrame;

  nsStyleSet* styleSet = mPresShell->StyleSet();
  
  if (!isScrollable) {
    rootPseudoStyle = styleSet->ResolveAnonymousBoxStyle(rootPseudo,
                                                         viewportPseudoStyle);
  } else {
      if (rootPseudo == nsCSSAnonBoxes::canvas) {
        rootPseudo = nsCSSAnonBoxes::scrolledCanvas;
      } else {
        NS_ASSERTION(rootPseudo == nsCSSAnonBoxes::pageSequence,
                     "Unknown root pseudo");
        rootPseudo = nsCSSAnonBoxes::scrolledPageSequence;
      }

      
      
      
      

      
      nsRefPtr<nsStyleContext>  styleContext;
      styleContext = styleSet->ResolveAnonymousBoxStyle(nsCSSAnonBoxes::viewportScroll,
                                                        viewportPseudoStyle);

      
      
      
      
      
      
      
      newFrame = nullptr;
      rootPseudoStyle = BeginBuildingScrollFrame( state,
                                                  aDocElement,
                                                  styleContext,
                                                  viewportFrame,
                                                  rootPseudo,
                                                  true,
                                                  newFrame);
      parentFrame = newFrame;
      mGfxScrollFrame = newFrame;
  }

  rootFrame->SetStyleContextWithoutNotification(rootPseudoStyle);
  rootFrame->Init(aDocElement, parentFrame, nullptr);

  if (isScrollable) {
    FinishBuildingScrollFrame(parentFrame, rootFrame);
  }

  if (isPaginated) {
    
    
    nsContainerFrame* canvasFrame;
    nsContainerFrame* pageFrame =
      ConstructPageFrame(mPresShell, presContext, rootFrame, nullptr,
                         canvasFrame);
    SetInitialSingleChild(rootFrame, pageFrame);

    
    
    mDocElementContainingBlock = canvasFrame;
    mHasRootAbsPosContainingBlock = true;
  }

  if (viewportFrame->GetStateBits() & NS_FRAME_FIRST_REFLOW) {
    SetInitialSingleChild(viewportFrame, newFrame);
  } else {
    nsFrameList newFrameList(newFrame, newFrame);
    viewportFrame->AppendFrames(kPrincipalList, newFrameList);
  }
}

void
nsCSSFrameConstructor::ConstructAnonymousContentForCanvas(nsFrameConstructorState& aState,
                                                          nsIFrame* aFrame,
                                                          nsIContent* aDocElement)
{
  NS_ASSERTION(aFrame->GetType() == nsGkAtoms::canvasFrame, "aFrame should be canvas frame!");

  nsAutoTArray<nsIAnonymousContentCreator::ContentInfo, 4> anonymousItems;
  GetAnonymousContent(aDocElement, aFrame, anonymousItems);
  if (anonymousItems.IsEmpty()) {
    
    return;
  }

  FrameConstructionItemList itemsToConstruct;
  nsContainerFrame* frameAsContainer = do_QueryFrame(aFrame);
  AddFCItemsForAnonymousContent(aState, frameAsContainer, anonymousItems, itemsToConstruct);

  nsFrameItems frameItems;
  ConstructFramesFromItemList(aState, itemsToConstruct, frameAsContainer, frameItems);
  frameAsContainer->AppendFrames(kPrincipalList, frameItems);
}

nsContainerFrame*
nsCSSFrameConstructor::ConstructPageFrame(nsIPresShell*  aPresShell,
                                          nsPresContext* aPresContext,
                                          nsContainerFrame* aParentFrame,
                                          nsIFrame*      aPrevPageFrame,
                                          nsContainerFrame*& aCanvasFrame)
{
  nsStyleContext* parentStyleContext = aParentFrame->StyleContext();
  nsStyleSet *styleSet = aPresShell->StyleSet();

  nsRefPtr<nsStyleContext> pagePseudoStyle;
  pagePseudoStyle = styleSet->ResolveAnonymousBoxStyle(nsCSSAnonBoxes::page,
                                                       parentStyleContext);

  nsContainerFrame* pageFrame = NS_NewPageFrame(aPresShell, pagePseudoStyle);

  
  
  pageFrame->Init(nullptr, aParentFrame, aPrevPageFrame);

  nsRefPtr<nsStyleContext> pageContentPseudoStyle;
  pageContentPseudoStyle =
    styleSet->ResolveAnonymousBoxStyle(nsCSSAnonBoxes::pageContent,
                                       pagePseudoStyle);

  nsContainerFrame* pageContentFrame =
    NS_NewPageContentFrame(aPresShell, pageContentPseudoStyle);

  
  
  nsIFrame* prevPageContentFrame = nullptr;
  if (aPrevPageFrame) {
    prevPageContentFrame = aPrevPageFrame->GetFirstPrincipalChild();
    NS_ASSERTION(prevPageContentFrame, "missing page content frame");
  }
  pageContentFrame->Init(nullptr, pageFrame, prevPageContentFrame);
  SetInitialSingleChild(pageFrame, pageContentFrame);
  
  pageContentFrame->AddStateBits(NS_FRAME_CAN_HAVE_ABSPOS_CHILDREN);
  pageContentFrame->MarkAsAbsoluteContainingBlock();

  nsRefPtr<nsStyleContext> canvasPseudoStyle;
  canvasPseudoStyle = styleSet->ResolveAnonymousBoxStyle(nsCSSAnonBoxes::canvas,
                                                         pageContentPseudoStyle);

  aCanvasFrame = NS_NewCanvasFrame(aPresShell, canvasPseudoStyle);

  nsIFrame* prevCanvasFrame = nullptr;
  if (prevPageContentFrame) {
    prevCanvasFrame = prevPageContentFrame->GetFirstPrincipalChild();
    NS_ASSERTION(prevCanvasFrame, "missing canvas frame");
  }
  aCanvasFrame->Init(nullptr, pageContentFrame, prevCanvasFrame);
  SetInitialSingleChild(pageContentFrame, aCanvasFrame);
  return pageFrame;
}


nsIFrame*
nsCSSFrameConstructor::CreatePlaceholderFrameFor(nsIPresShell*     aPresShell,
                                                 nsIContent*       aContent,
                                                 nsIFrame*         aFrame,
                                                 nsStyleContext*   aStyleContext,
                                                 nsContainerFrame* aParentFrame,
                                                 nsIFrame*         aPrevInFlow,
                                                 nsFrameState      aTypeBit)
{
  nsRefPtr<nsStyleContext> placeholderStyle = aPresShell->StyleSet()->
    ResolveStyleForNonElement(aStyleContext->GetParent());

  
  nsPlaceholderFrame* placeholderFrame =
    (nsPlaceholderFrame*)NS_NewPlaceholderFrame(aPresShell, placeholderStyle,
                                                aTypeBit);

  placeholderFrame->Init(aContent, aParentFrame, aPrevInFlow);

  
  placeholderFrame->SetOutOfFlowFrame(aFrame);

  aFrame->AddStateBits(NS_FRAME_OUT_OF_FLOW);

  
  aPresShell->FrameManager()->RegisterPlaceholderFrame(placeholderFrame);

  return placeholderFrame;
}







static inline void
ClearLazyBits(nsIContent* aStartContent, nsIContent* aEndContent)
{
  NS_PRECONDITION(aStartContent || !aEndContent,
                  "Must have start child if we have an end child");
  for (nsIContent* cur = aStartContent; cur != aEndContent;
       cur = cur->GetNextSibling()) {
    cur->UnsetFlags(NODE_DESCENDANTS_NEED_FRAMES | NODE_NEEDS_FRAME);
  }
}

nsIFrame*
nsCSSFrameConstructor::ConstructSelectFrame(nsFrameConstructorState& aState,
                                            FrameConstructionItem&   aItem,
                                            nsContainerFrame*        aParentFrame,
                                            const nsStyleDisplay*    aStyleDisplay,
                                            nsFrameItems&            aFrameItems)
{
  nsIContent* const content = aItem.mContent;
  nsStyleContext* const styleContext = aItem.mStyleContext;

  
  dom::HTMLSelectElement* sel = dom::HTMLSelectElement::FromContent(content);
  MOZ_ASSERT(sel);
  if (sel->IsCombobox()) {
    
    
    
    
    
    nsFrameState flags = NS_BLOCK_FLOAT_MGR;
    nsContainerFrame* comboboxFrame =
      NS_NewComboboxControlFrame(mPresShell, styleContext, flags);

    
    
    nsILayoutHistoryState *historyState = aState.mFrameState;
    aState.mFrameState = nullptr;
    
    InitAndRestoreFrame(aState, content,
                        aState.GetGeometricParent(aStyleDisplay, aParentFrame),
                        comboboxFrame);

    aState.AddChild(comboboxFrame, aFrameItems, content, styleContext,
                    aParentFrame);

    nsIComboboxControlFrame* comboBox = do_QueryFrame(comboboxFrame);
    NS_ASSERTION(comboBox, "NS_NewComboboxControlFrame returned frame that "
                 "doesn't implement nsIComboboxControlFrame");

    
    nsRefPtr<nsStyleContext> listStyle;
    listStyle = mPresShell->StyleSet()->
      ResolveAnonymousBoxStyle(nsCSSAnonBoxes::dropDownList, styleContext);

    
    nsContainerFrame* listFrame = NS_NewListControlFrame(mPresShell, listStyle);

    
    nsIListControlFrame * listControlFrame = do_QueryFrame(listFrame);
    if (listControlFrame) {
      listControlFrame->SetComboboxFrame(comboboxFrame);
    }
    
    comboBox->SetDropDown(listFrame);

    NS_ASSERTION(!listFrame->IsAbsPosContaininingBlock(),
                 "Ended up with positioned dropdown list somehow.");
    NS_ASSERTION(!listFrame->IsFloating(),
                 "Ended up with floating dropdown list somehow.");

    
    
    nsContainerFrame* scrolledFrame =
      NS_NewSelectsAreaFrame(mPresShell, styleContext, flags);

    InitializeSelectFrame(aState, listFrame, scrolledFrame, content,
                          comboboxFrame, listStyle, true,
                          aItem.mPendingBinding, aFrameItems);

    NS_ASSERTION(listFrame->GetView(), "ListFrame's view is nullptr");

    
    
    

    nsFrameItems childItems;
    CreateAnonymousFrames(aState, content, comboboxFrame,
                          aItem.mPendingBinding, childItems);

    comboboxFrame->SetInitialChildList(kPrincipalList, childItems);

    
    
    nsFrameItems popupItems;
    popupItems.AddChild(listFrame);
    comboboxFrame->SetInitialChildList(nsIFrame::kSelectPopupList,
                                       popupItems);

    aState.mFrameState = historyState;
    if (aState.mFrameState) {
      
      RestoreFrameState(comboboxFrame, aState.mFrameState);
    }
    return comboboxFrame;
  }

  
  nsContainerFrame* listFrame = NS_NewListControlFrame(mPresShell, styleContext);

  nsContainerFrame* scrolledFrame = NS_NewSelectsAreaFrame(
      mPresShell, styleContext, NS_BLOCK_FLOAT_MGR);

  
  

  InitializeSelectFrame(aState, listFrame, scrolledFrame, content,
                        aParentFrame, styleContext, false,
                        aItem.mPendingBinding, aFrameItems);

  return listFrame;
}






nsresult
nsCSSFrameConstructor::InitializeSelectFrame(nsFrameConstructorState& aState,
                                             nsContainerFrame*        scrollFrame,
                                             nsContainerFrame*        scrolledFrame,
                                             nsIContent*              aContent,
                                             nsContainerFrame*        aParentFrame,
                                             nsStyleContext*          aStyleContext,
                                             bool                     aBuildCombobox,
                                             PendingBinding*          aPendingBinding,
                                             nsFrameItems&            aFrameItems)
{
  
  nsContainerFrame* geometricParent =
    aState.GetGeometricParent(aStyleContext->StyleDisplay(), aParentFrame);

  
  
  

  scrollFrame->Init(aContent, geometricParent, nullptr);

  if (!aBuildCombobox) {
    aState.AddChild(scrollFrame, aFrameItems, aContent,
                    aStyleContext, aParentFrame);
  }

  if (aBuildCombobox) {
    nsContainerFrame::CreateViewForFrame(scrollFrame, true);
  }

  BuildScrollFrame(aState, aContent, aStyleContext, scrolledFrame,
                   geometricParent, scrollFrame);

  if (aState.mFrameState) {
    
    RestoreFrameStateFor(scrollFrame, aState.mFrameState);
  }

  
  nsFrameItems                childItems;

  ProcessChildren(aState, aContent, aStyleContext, scrolledFrame, false,
                  childItems, false, aPendingBinding);

  
  scrolledFrame->SetInitialChildList(kPrincipalList, childItems);
  return NS_OK;
}

nsIFrame*
nsCSSFrameConstructor::ConstructFieldSetFrame(nsFrameConstructorState& aState,
                                              FrameConstructionItem&   aItem,
                                              nsContainerFrame*        aParentFrame,
                                              const nsStyleDisplay*    aStyleDisplay,
                                              nsFrameItems&            aFrameItems)
{
  nsIContent* const content = aItem.mContent;
  nsStyleContext* const styleContext = aItem.mStyleContext;

  nsContainerFrame* fieldsetFrame = NS_NewFieldSetFrame(mPresShell, styleContext);

  
  InitAndRestoreFrame(aState, content,
                      aState.GetGeometricParent(aStyleDisplay, aParentFrame),
                      fieldsetFrame);

  
  nsRefPtr<nsStyleContext> fieldsetContentStyle;
  fieldsetContentStyle = mPresShell->StyleSet()->
    ResolveAnonymousBoxStyle(nsCSSAnonBoxes::fieldsetContent, styleContext);

  const nsStyleDisplay* fieldsetContentDisplay = fieldsetContentStyle->StyleDisplay();
  bool isScrollable = fieldsetContentDisplay->IsScrollableOverflow();
  nsContainerFrame* scrollFrame = nullptr;
  if (isScrollable) {
    fieldsetContentStyle =
      BeginBuildingScrollFrame(aState, content, fieldsetContentStyle,
                               fieldsetFrame, nsCSSAnonBoxes::scrolledContent,
                               false, scrollFrame);
  }

  nsContainerFrame* blockFrame =
    NS_NewBlockFrame(mPresShell, fieldsetContentStyle,
                     NS_BLOCK_FLOAT_MGR | NS_BLOCK_MARGIN_ROOT);
  InitAndRestoreFrame(aState, content,
    scrollFrame ? scrollFrame : fieldsetFrame, blockFrame);

  aState.AddChild(fieldsetFrame, aFrameItems, content, styleContext, aParentFrame);

  
  nsFrameConstructorSaveState absoluteSaveState;
  nsFrameItems                childItems;

  blockFrame->AddStateBits(NS_FRAME_CAN_HAVE_ABSPOS_CHILDREN);
  if (fieldsetFrame->IsAbsPosContaininingBlock()) {
    aState.PushAbsoluteContainingBlock(blockFrame, fieldsetFrame, absoluteSaveState);
  }

  ProcessChildren(aState, content, styleContext, blockFrame, true,
                  childItems, true, aItem.mPendingBinding);

  nsFrameItems fieldsetKids;
  fieldsetKids.AddChild(scrollFrame ? scrollFrame : blockFrame);

  for (nsFrameList::Enumerator e(childItems); !e.AtEnd(); e.Next()) {
    nsIFrame* child = e.get();
    nsContainerFrame* cif = child->GetContentInsertionFrame();
    if (cif && cif->GetType() == nsGkAtoms::legendFrame) {
      
      
      
      
      
      childItems.RemoveFrame(child);
      
      fieldsetKids.InsertFrame(fieldsetFrame, nullptr, child);
      if (scrollFrame) {
        StickyScrollContainer::NotifyReparentedFrameAcrossScrollFrameBoundary(
            child, blockFrame);
      }
      break;
    }
  }

  if (isScrollable) {
    FinishBuildingScrollFrame(scrollFrame, blockFrame);
  }

  
  blockFrame->SetInitialChildList(kPrincipalList, childItems);

  
  fieldsetFrame->SetInitialChildList(kPrincipalList, fieldsetKids);

  fieldsetFrame->AddStateBits(NS_FRAME_MAY_HAVE_GENERATED_CONTENT);

  
  return fieldsetFrame;
}

static nsIFrame*
FindAncestorWithGeneratedContentPseudo(nsIFrame* aFrame)
{
  for (nsIFrame* f = aFrame->GetParent(); f; f = f->GetParent()) {
    NS_ASSERTION(f->IsGeneratedContentFrame(),
                 "should not have exited generated content");
    nsIAtom* pseudo = f->StyleContext()->GetPseudo();
    if (pseudo == nsCSSPseudoElements::before ||
        pseudo == nsCSSPseudoElements::after)
      return f;
  }
  return nullptr;
}

#define SIMPLE_FCDATA(_func) FCDATA_DECL(0, _func)
#define FULL_CTOR_FCDATA(_flags, _func)                             \
  { _flags | FCDATA_FUNC_IS_FULL_CTOR, { nullptr }, _func, nullptr }


const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindTextData(nsIFrame* aParentFrame)
{
  if (aParentFrame && IsFrameForSVG(aParentFrame)) {
    nsIFrame *ancestorFrame =
      nsSVGUtils::GetFirstNonAAncestorFrame(aParentFrame);
    if (ancestorFrame) {
      static const FrameConstructionData sSVGTextData =
        FCDATA_DECL(FCDATA_IS_LINE_PARTICIPANT | FCDATA_IS_SVG_TEXT,
                    NS_NewTextFrame);
      if (ancestorFrame->IsSVGText()) {
        return &sSVGTextData;
      }
    }
    return nullptr;
  }

  static const FrameConstructionData sTextData =
    FCDATA_DECL(FCDATA_IS_LINE_PARTICIPANT, NS_NewTextFrame);
  return &sTextData;
}

void
nsCSSFrameConstructor::ConstructTextFrame(const FrameConstructionData* aData,
                                          nsFrameConstructorState& aState,
                                          nsIContent*              aContent,
                                          nsContainerFrame*        aParentFrame,
                                          nsStyleContext*          aStyleContext,
                                          nsFrameItems&            aFrameItems)
{
  NS_PRECONDITION(aData, "Must have frame construction data");

  nsIFrame* newFrame = (*aData->mFunc.mCreationFunc)(mPresShell, aStyleContext);

  InitAndRestoreFrame(aState, aContent, aParentFrame, newFrame);

  

  if (newFrame->IsGeneratedContentFrame()) {
    nsAutoPtr<nsGenConInitializer> initializer;
    initializer =
      static_cast<nsGenConInitializer*>(
        aContent->UnsetProperty(nsGkAtoms::genConInitializerProperty));
    if (initializer) {
      if (initializer->mNode->InitTextFrame(initializer->mList,
              FindAncestorWithGeneratedContentPseudo(newFrame), newFrame)) {
        (this->*(initializer->mDirtyAll))();
      }
      initializer->mNode.forget();
    }
  }

  
  aFrameItems.AddChild(newFrame);

  if (!aState.mCreatingExtraFrames)
    aContent->SetPrimaryFrame(newFrame);
}


const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindDataByInt(int32_t aInt,
                                     Element* aElement,
                                     nsStyleContext* aStyleContext,
                                     const FrameConstructionDataByInt* aDataPtr,
                                     uint32_t aDataLength)
{
  for (const FrameConstructionDataByInt *curData = aDataPtr,
         *endData = aDataPtr + aDataLength;
       curData != endData;
       ++curData) {
    if (curData->mInt == aInt) {
      const FrameConstructionData* data = &curData->mData;
      if (data->mBits & FCDATA_FUNC_IS_DATA_GETTER) {
        return data->mFunc.mDataGetter(aElement, aStyleContext);
      }

      return data;
    }
  }

  return nullptr;
}


const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindDataByTag(nsIAtom* aTag,
                                     Element* aElement,
                                     nsStyleContext* aStyleContext,
                                     const FrameConstructionDataByTag* aDataPtr,
                                     uint32_t aDataLength)
{
  for (const FrameConstructionDataByTag *curData = aDataPtr,
         *endData = aDataPtr + aDataLength;
       curData != endData;
       ++curData) {
    if (*curData->mTag == aTag) {
      const FrameConstructionData* data = &curData->mData;
      if (data->mBits & FCDATA_FUNC_IS_DATA_GETTER) {
        return data->mFunc.mDataGetter(aElement, aStyleContext);
      }

      return data;
    }
  }

  return nullptr;
}

#define SUPPRESS_FCDATA() FCDATA_DECL(FCDATA_SUPPRESS_FRAME, nullptr)
#define SIMPLE_INT_CREATE(_int, _func) { _int, SIMPLE_FCDATA(_func) }
#define SIMPLE_INT_CHAIN(_int, _func)                       \
  { _int, FCDATA_DECL(FCDATA_FUNC_IS_DATA_GETTER, _func) }
#define COMPLEX_INT_CREATE(_int, _func)         \
  { _int, FULL_CTOR_FCDATA(0, _func) }

#define SIMPLE_TAG_CREATE(_tag, _func)          \
  { &nsGkAtoms::_tag, SIMPLE_FCDATA(_func) }
#define SIMPLE_TAG_CHAIN(_tag, _func)                                   \
  { &nsGkAtoms::_tag, FCDATA_DECL(FCDATA_FUNC_IS_DATA_GETTER,  _func) }
#define COMPLEX_TAG_CREATE(_tag, _func)             \
  { &nsGkAtoms::_tag, FULL_CTOR_FCDATA(0, _func) }

static bool
IsFrameForFieldSet(nsIFrame* aFrame, nsIAtom* aFrameType)
{
  nsIAtom* pseudo = aFrame->StyleContext()->GetPseudo();
  if (pseudo == nsCSSAnonBoxes::fieldsetContent ||
      pseudo == nsCSSAnonBoxes::scrolledContent) {
    return IsFrameForFieldSet(aFrame->GetParent(), aFrame->GetParent()->GetType());
  }
  return aFrameType == nsGkAtoms::fieldSetFrame;
}


const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindHTMLData(Element* aElement,
                                    nsIAtom* aTag,
                                    int32_t aNameSpaceID,
                                    nsIFrame* aParentFrame,
                                    nsStyleContext* aStyleContext)
{
  
  
  
  if (aNameSpaceID != kNameSpaceID_XHTML) {
    return nullptr;
  }

  NS_ASSERTION(!aParentFrame ||
               aParentFrame->StyleContext()->GetPseudo() !=
                 nsCSSAnonBoxes::fieldsetContent ||
               aParentFrame->GetParent()->GetType() == nsGkAtoms::fieldSetFrame,
               "Unexpected parent for fieldset content anon box");
  if (aTag == nsGkAtoms::legend &&
      (!aParentFrame ||
       !IsFrameForFieldSet(aParentFrame, aParentFrame->GetType()) ||
       aStyleContext->StyleDisplay()->IsFloatingStyle() ||
       aStyleContext->StyleDisplay()->IsAbsolutelyPositionedStyle())) {
    
    
    
    
    
    return nullptr;
  }

  static const FrameConstructionDataByTag sHTMLData[] = {
    SIMPLE_TAG_CHAIN(img, nsCSSFrameConstructor::FindImgData),
    SIMPLE_TAG_CHAIN(mozgeneratedcontentimage,
                     nsCSSFrameConstructor::FindImgData),
    { &nsGkAtoms::br,
      FCDATA_DECL(FCDATA_IS_LINE_PARTICIPANT | FCDATA_IS_LINE_BREAK,
                  NS_NewBRFrame) },
    SIMPLE_TAG_CREATE(wbr, NS_NewWBRFrame),
    SIMPLE_TAG_CHAIN(input, nsCSSFrameConstructor::FindInputData),
    SIMPLE_TAG_CREATE(textarea, NS_NewTextControlFrame),
    COMPLEX_TAG_CREATE(select, &nsCSSFrameConstructor::ConstructSelectFrame),
    SIMPLE_TAG_CHAIN(object, nsCSSFrameConstructor::FindObjectData),
    SIMPLE_TAG_CHAIN(applet, nsCSSFrameConstructor::FindObjectData),
    SIMPLE_TAG_CHAIN(embed, nsCSSFrameConstructor::FindObjectData),
    COMPLEX_TAG_CREATE(fieldset,
                       &nsCSSFrameConstructor::ConstructFieldSetFrame),
    { &nsGkAtoms::legend,
      FCDATA_DECL(FCDATA_ALLOW_BLOCK_STYLES | FCDATA_MAY_NEED_SCROLLFRAME,
                  NS_NewLegendFrame) },
    SIMPLE_TAG_CREATE(frameset, NS_NewHTMLFramesetFrame),
    SIMPLE_TAG_CREATE(iframe, NS_NewSubDocumentFrame),
    { &nsGkAtoms::button,
      FCDATA_WITH_WRAPPING_BLOCK(FCDATA_ALLOW_BLOCK_STYLES,
                                 NS_NewHTMLButtonControlFrame,
                                 nsCSSAnonBoxes::buttonContent) },
    SIMPLE_TAG_CHAIN(canvas, nsCSSFrameConstructor::FindCanvasData),
    SIMPLE_TAG_CREATE(video, NS_NewHTMLVideoFrame),
    SIMPLE_TAG_CREATE(audio, NS_NewHTMLVideoFrame),
    SIMPLE_TAG_CREATE(progress, NS_NewProgressFrame),
    SIMPLE_TAG_CREATE(meter, NS_NewMeterFrame)
  };

  return FindDataByTag(aTag, aElement, aStyleContext, sHTMLData,
                       ArrayLength(sHTMLData));
}


const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindImgData(Element* aElement,
                                   nsStyleContext* aStyleContext)
{
  if (!nsImageFrame::ShouldCreateImageFrameFor(aElement, aStyleContext)) {
    return nullptr;
  }

  static const FrameConstructionData sImgData = SIMPLE_FCDATA(NS_NewImageFrame);
  return &sImgData;
}


const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindImgControlData(Element* aElement,
                                          nsStyleContext* aStyleContext)
{
  if (!nsImageFrame::ShouldCreateImageFrameFor(aElement, aStyleContext)) {
    return nullptr;
  }

  static const FrameConstructionData sImgControlData =
    SIMPLE_FCDATA(NS_NewImageControlFrame);
  return &sImgControlData;
}


const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindInputData(Element* aElement,
                                     nsStyleContext* aStyleContext)
{
  static const FrameConstructionDataByInt sInputData[] = {
    SIMPLE_INT_CREATE(NS_FORM_INPUT_CHECKBOX, NS_NewGfxCheckboxControlFrame),
    SIMPLE_INT_CREATE(NS_FORM_INPUT_RADIO, NS_NewGfxRadioControlFrame),
    SIMPLE_INT_CREATE(NS_FORM_INPUT_FILE, NS_NewFileControlFrame),
    SIMPLE_INT_CHAIN(NS_FORM_INPUT_IMAGE,
                     nsCSSFrameConstructor::FindImgControlData),
    SIMPLE_INT_CREATE(NS_FORM_INPUT_EMAIL, NS_NewTextControlFrame),
    SIMPLE_INT_CREATE(NS_FORM_INPUT_SEARCH, NS_NewTextControlFrame),
    SIMPLE_INT_CREATE(NS_FORM_INPUT_TEXT, NS_NewTextControlFrame),
    SIMPLE_INT_CREATE(NS_FORM_INPUT_TEL, NS_NewTextControlFrame),
    SIMPLE_INT_CREATE(NS_FORM_INPUT_URL, NS_NewTextControlFrame),
    SIMPLE_INT_CREATE(NS_FORM_INPUT_RANGE, NS_NewRangeFrame),
    SIMPLE_INT_CREATE(NS_FORM_INPUT_PASSWORD, NS_NewTextControlFrame),
    { NS_FORM_INPUT_COLOR,
      FCDATA_WITH_WRAPPING_BLOCK(0, NS_NewColorControlFrame,
                                 nsCSSAnonBoxes::buttonContent) },
    
    SIMPLE_INT_CREATE(NS_FORM_INPUT_NUMBER, NS_NewNumberControlFrame),
    
    SIMPLE_INT_CREATE(NS_FORM_INPUT_DATE, NS_NewTextControlFrame),
    
    SIMPLE_INT_CREATE(NS_FORM_INPUT_TIME, NS_NewTextControlFrame),
    { NS_FORM_INPUT_SUBMIT,
      FCDATA_WITH_WRAPPING_BLOCK(0, NS_NewGfxButtonControlFrame,
                                 nsCSSAnonBoxes::buttonContent) },
    { NS_FORM_INPUT_RESET,
      FCDATA_WITH_WRAPPING_BLOCK(0, NS_NewGfxButtonControlFrame,
                                 nsCSSAnonBoxes::buttonContent) },
    { NS_FORM_INPUT_BUTTON,
      FCDATA_WITH_WRAPPING_BLOCK(0, NS_NewGfxButtonControlFrame,
                                 nsCSSAnonBoxes::buttonContent) }
    
    
  };

  nsCOMPtr<nsIFormControl> control = do_QueryInterface(aElement);
  NS_ASSERTION(control, "input doesn't implement nsIFormControl?");

  return FindDataByInt(control->GetType(), aElement, aStyleContext,
                       sInputData, ArrayLength(sInputData));
}


const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindObjectData(Element* aElement,
                                      nsStyleContext* aStyleContext)
{
  
  
  
  uint32_t type;
  if (aElement->State().HasAtLeastOneOfStates(NS_EVENT_STATE_BROKEN |
                                              NS_EVENT_STATE_USERDISABLED |
                                              NS_EVENT_STATE_SUPPRESSED)) {
    type = nsIObjectLoadingContent::TYPE_NULL;
  } else {
    nsCOMPtr<nsIObjectLoadingContent> objContent(do_QueryInterface(aElement));
    NS_ASSERTION(objContent,
                 "applet, embed and object must implement "
                 "nsIObjectLoadingContent!");

    objContent->GetDisplayedType(&type);
  }

  static const FrameConstructionDataByInt sObjectData[] = {
    SIMPLE_INT_CREATE(nsIObjectLoadingContent::TYPE_LOADING,
                      NS_NewEmptyFrame),
    SIMPLE_INT_CREATE(nsIObjectLoadingContent::TYPE_PLUGIN,
                      NS_NewObjectFrame),
    SIMPLE_INT_CREATE(nsIObjectLoadingContent::TYPE_IMAGE,
                      NS_NewImageFrame),
    SIMPLE_INT_CREATE(nsIObjectLoadingContent::TYPE_DOCUMENT,
                      NS_NewSubDocumentFrame)
    
  };

  return FindDataByInt((int32_t)type, aElement, aStyleContext,
                       sObjectData, ArrayLength(sObjectData));
}


const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindCanvasData(Element* aElement,
                                      nsStyleContext* aStyleContext)
{
  
  
  
  
  nsIDocument* doc = aElement->OwnerDoc();
  if (doc->IsStaticDocument()) {
    doc = doc->GetOriginalDocument();
  }
  if (!doc->IsScriptEnabled()) {
    return nullptr;
  }

  static const FrameConstructionData sCanvasData =
    FCDATA_WITH_WRAPPING_BLOCK(0, NS_NewHTMLCanvasFrame,
                               nsCSSAnonBoxes::htmlCanvasContent);
  return &sCanvasData;
}

void
nsCSSFrameConstructor::ConstructFrameFromItemInternal(FrameConstructionItem& aItem,
                                                      nsFrameConstructorState& aState,
                                                      nsContainerFrame* aParentFrame,
                                                      nsFrameItems& aFrameItems)
{
  const FrameConstructionData* data = aItem.mFCData;
  NS_ASSERTION(data, "Must have frame construction data");

  uint32_t bits = data->mBits;

  NS_ASSERTION(!(bits & FCDATA_FUNC_IS_DATA_GETTER),
               "Should have dealt with this inside the data finder");

  
#define CHECK_ONLY_ONE_BIT(_bit1, _bit2)               \
  NS_ASSERTION(!(bits & _bit1) || !(bits & _bit2),     \
               "Only one of these bits should be set")
  CHECK_ONLY_ONE_BIT(FCDATA_FUNC_IS_FULL_CTOR, FCDATA_FORCE_NULL_ABSPOS_CONTAINER);
  CHECK_ONLY_ONE_BIT(FCDATA_FUNC_IS_FULL_CTOR, FCDATA_WRAP_KIDS_IN_BLOCKS);
  CHECK_ONLY_ONE_BIT(FCDATA_FUNC_IS_FULL_CTOR, FCDATA_MAY_NEED_SCROLLFRAME);
  CHECK_ONLY_ONE_BIT(FCDATA_FUNC_IS_FULL_CTOR, FCDATA_IS_POPUP);
  CHECK_ONLY_ONE_BIT(FCDATA_FUNC_IS_FULL_CTOR, FCDATA_SKIP_ABSPOS_PUSH);
  CHECK_ONLY_ONE_BIT(FCDATA_FUNC_IS_FULL_CTOR,
                     FCDATA_DISALLOW_GENERATED_CONTENT);
  CHECK_ONLY_ONE_BIT(FCDATA_FUNC_IS_FULL_CTOR, FCDATA_ALLOW_BLOCK_STYLES);
  CHECK_ONLY_ONE_BIT(FCDATA_FUNC_IS_FULL_CTOR,
                     FCDATA_CREATE_BLOCK_WRAPPER_FOR_ALL_KIDS);
  CHECK_ONLY_ONE_BIT(FCDATA_WRAP_KIDS_IN_BLOCKS,
                     FCDATA_CREATE_BLOCK_WRAPPER_FOR_ALL_KIDS);
#undef CHECK_ONLY_ONE_BIT
  NS_ASSERTION(!(bits & FCDATA_FORCED_NON_SCROLLABLE_BLOCK) ||
               ((bits & FCDATA_FUNC_IS_FULL_CTOR) &&
                data->mFullConstructor ==
                  &nsCSSFrameConstructor::ConstructNonScrollableBlock),
               "Unexpected FCDATA_FORCED_NON_SCROLLABLE_BLOCK flag");

  
  if (aState.mCreatingExtraFrames &&
      aItem.mContent->IsHTMLElement(nsGkAtoms::iframe))
  {
    return;
  }

  nsIContent* const content = aItem.mContent;
  nsIContent* parent = content->GetParent();

  
  AutoDisplayContentsAncestorPusher adcp(aState.mTreeMatchContext,
                                         aState.mPresContext, parent);

  
  
  
  
  
  
  TreeMatchContext::AutoAncestorPusher
    insertionPointPusher(aState.mTreeMatchContext);
  if (adcp.IsEmpty() && parent && nsContentUtils::IsContentInsertionPoint(parent)) {
    if (aState.mTreeMatchContext.mAncestorFilter.HasFilter()) {
      insertionPointPusher.PushAncestorAndStyleScope(parent);
    } else {
      insertionPointPusher.PushStyleScope(parent);
    }
  }

  
  
  
  
  
  
  
  
  TreeMatchContext::AutoAncestorPusher
    ancestorPusher(aState.mTreeMatchContext);
  if (aState.mTreeMatchContext.mAncestorFilter.HasFilter()) {
    ancestorPusher.PushAncestorAndStyleScope(content);
  } else {
    ancestorPusher.PushStyleScope(content);
  }

  nsIFrame* newFrame;
  nsIFrame* primaryFrame;
  nsStyleContext* const styleContext = aItem.mStyleContext;
  const nsStyleDisplay* display = styleContext->StyleDisplay();
  if (bits & FCDATA_FUNC_IS_FULL_CTOR) {
    newFrame =
      (this->*(data->mFullConstructor))(aState, aItem, aParentFrame,
                                        display, aFrameItems);
    MOZ_ASSERT(newFrame, "Full constructor failed");
    primaryFrame = newFrame;
  } else {
    newFrame =
      (*data->mFunc.mCreationFunc)(mPresShell, styleContext);

    bool allowOutOfFlow = !(bits & FCDATA_DISALLOW_OUT_OF_FLOW);
    bool isPopup = aItem.mIsPopup;
    NS_ASSERTION(!isPopup ||
                 (aState.mPopupItems.containingBlock &&
                  aState.mPopupItems.containingBlock->GetType() ==
                    nsGkAtoms::popupSetFrame),
                 "Should have a containing block here!");

    nsContainerFrame* geometricParent =
      isPopup ? aState.mPopupItems.containingBlock :
      (allowOutOfFlow ? aState.GetGeometricParent(display, aParentFrame)
                      : aParentFrame);

    
    nsIFrame* frameToAddToList = nullptr;
    if ((bits & FCDATA_MAY_NEED_SCROLLFRAME) &&
        display->IsScrollableOverflow()) {
      nsContainerFrame* scrollframe = nullptr;
      BuildScrollFrame(aState, content, styleContext, newFrame,
                       geometricParent, scrollframe);
      frameToAddToList = scrollframe;
    } else {
      InitAndRestoreFrame(aState, content, geometricParent, newFrame);
      
      nsContainerFrame::CreateViewForFrame(newFrame, false);
      frameToAddToList = newFrame;
    }

    
    
    
    
    primaryFrame = frameToAddToList;

    
    
    const nsStyleDisplay* maybeAbsoluteContainingBlockDisplay = display;
    nsIFrame* maybeAbsoluteContainingBlockStyleFrame = primaryFrame;
    nsIFrame* maybeAbsoluteContainingBlock = newFrame;
    nsIFrame* possiblyLeafFrame = newFrame;
    if (bits & FCDATA_CREATE_BLOCK_WRAPPER_FOR_ALL_KIDS) {
      nsRefPtr<nsStyleContext> blockContext;
      blockContext =
        mPresShell->StyleSet()->ResolveAnonymousBoxStyle(*data->mAnonBoxPseudo,
                                                         styleContext);
      nsIFrame* blockFrame =
        NS_NewBlockFormattingContext(mPresShell, blockContext);

#ifdef DEBUG
      nsContainerFrame* containerFrame = do_QueryFrame(newFrame);
      MOZ_ASSERT(containerFrame);
#endif
      nsContainerFrame* container = static_cast<nsContainerFrame*>(newFrame);
      InitAndRestoreFrame(aState, content, container, blockFrame);

      SetInitialSingleChild(container, blockFrame);

      
      
      
      const nsStyleDisplay* blockDisplay = blockContext->StyleDisplay();
      if (blockDisplay->IsAbsPosContainingBlock(blockFrame)) {
        maybeAbsoluteContainingBlockDisplay = blockDisplay;
        maybeAbsoluteContainingBlock = blockFrame;
        maybeAbsoluteContainingBlockStyleFrame = blockFrame;
      }

      
      newFrame = blockFrame;
    }

    aState.AddChild(frameToAddToList, aFrameItems, content, styleContext,
                    aParentFrame, allowOutOfFlow, allowOutOfFlow, isPopup);

    nsContainerFrame* newFrameAsContainer = do_QueryFrame(newFrame);
    if (newFrameAsContainer) {
#ifdef MOZ_XUL
      

      if (aItem.mIsRootPopupgroup) {
        NS_ASSERTION(nsIRootBox::GetRootBox(mPresShell) &&
                     nsIRootBox::GetRootBox(mPresShell)->GetPopupSetFrame() ==
                     newFrame,
                     "Unexpected PopupSetFrame");
        aState.mPopupItems.containingBlock = newFrameAsContainer;
        aState.mHavePendingPopupgroup = false;
      }
#endif 

      
      nsFrameItems childItems;
      nsFrameConstructorSaveState absoluteSaveState;

      if (bits & FCDATA_FORCE_NULL_ABSPOS_CONTAINER) {
        aState.PushAbsoluteContainingBlock(nullptr, nullptr, absoluteSaveState);
      } else if (!(bits & FCDATA_SKIP_ABSPOS_PUSH)) {
        maybeAbsoluteContainingBlock->AddStateBits(NS_FRAME_CAN_HAVE_ABSPOS_CHILDREN);
        
        
        
        
        if ((maybeAbsoluteContainingBlockDisplay->IsAbsolutelyPositionedStyle() ||
             maybeAbsoluteContainingBlockDisplay->IsRelativelyPositionedStyle() ||
             maybeAbsoluteContainingBlockDisplay->IsFixedPosContainingBlock(
                 maybeAbsoluteContainingBlockStyleFrame)) &&
            !maybeAbsoluteContainingBlockStyleFrame->IsSVGText()) {
          nsContainerFrame* cf = static_cast<nsContainerFrame*>(
              maybeAbsoluteContainingBlock);
          aState.PushAbsoluteContainingBlock(cf, cf, absoluteSaveState);
        }
      }

      if (!aItem.mAnonChildren.IsEmpty()) {
        NS_ASSERTION(!(bits & FCDATA_USE_CHILD_ITEMS),
                     "We should not have both anonymous and non-anonymous "
                     "children in a given FrameConstructorItem");
        AddFCItemsForAnonymousContent(aState, newFrameAsContainer, aItem.mAnonChildren,
                                      aItem.mChildItems);
        bits |= FCDATA_USE_CHILD_ITEMS;
      }

      if (bits & FCDATA_USE_CHILD_ITEMS) {
        nsFrameConstructorSaveState floatSaveState;

        if (ShouldSuppressFloatingOfDescendants(newFrame)) {
          aState.PushFloatContainingBlock(nullptr, floatSaveState);
        } else if (newFrame->IsFloatContainingBlock()) {
          aState.PushFloatContainingBlock(newFrameAsContainer, floatSaveState);
        }
        ConstructFramesFromItemList(aState, aItem.mChildItems, newFrameAsContainer,
                                    childItems);
      } else {
        
        ProcessChildren(aState, content, styleContext, newFrameAsContainer,
                        !(bits & FCDATA_DISALLOW_GENERATED_CONTENT),
                        childItems,
                        (bits & FCDATA_ALLOW_BLOCK_STYLES) != 0,
                        aItem.mPendingBinding, possiblyLeafFrame);
      }

      if (bits & FCDATA_WRAP_KIDS_IN_BLOCKS) {
        nsFrameItems newItems;
        nsFrameItems currentBlockItems;
        nsIFrame* f;
        while ((f = childItems.FirstChild()) != nullptr) {
          bool wrapFrame = IsInlineFrame(f) || IsFramePartOfIBSplit(f);
          if (!wrapFrame) {
            FlushAccumulatedBlock(aState, content, newFrameAsContainer,
                                  currentBlockItems, newItems);
          }

          childItems.RemoveFrame(f);
          if (wrapFrame) {
            currentBlockItems.AddChild(f);
          } else {
            newItems.AddChild(f);
          }
        }
        FlushAccumulatedBlock(aState, content, newFrameAsContainer,
                              currentBlockItems, newItems);

        if (childItems.NotEmpty()) {
          
          childItems.DestroyFrames();
        }

        childItems = newItems;
      }

      
      
      
      newFrameAsContainer->SetInitialChildList(kPrincipalList, childItems);
    }
  }

#ifdef MOZ_XUL
  
  if (aItem.mNameSpaceID == kNameSpaceID_XUL &&
      (aItem.mTag == nsGkAtoms::treechildren || 
       content->HasAttr(kNameSpaceID_None, nsGkAtoms::tooltiptext) ||
       content->HasAttr(kNameSpaceID_None, nsGkAtoms::tooltip))) {
    nsIRootBox* rootBox = nsIRootBox::GetRootBox(mPresShell);
    if (rootBox) {
      rootBox->AddTooltipSupport(content);
    }
  }
#endif

  NS_ASSERTION(newFrame->IsFrameOfType(nsIFrame::eLineParticipant) ==
               ((bits & FCDATA_IS_LINE_PARTICIPANT) != 0),
               "Incorrectly set FCDATA_IS_LINE_PARTICIPANT bits");

  if (aItem.mIsAnonymousContentCreatorContent) {
    primaryFrame->AddStateBits(NS_FRAME_ANONYMOUSCONTENTCREATOR_CONTENT);
  }

  
  
  
  
  if ((!aState.mCreatingExtraFrames ||
       ((primaryFrame->GetStateBits() & NS_FRAME_GENERATED_CONTENT) &&
        !aItem.mContent->GetPrimaryFrame())) &&
       !(bits & FCDATA_SKIP_FRAMESET)) {
    aItem.mContent->SetPrimaryFrame(primaryFrame);
    ActiveLayerTracker::TransferActivityToFrame(aItem.mContent, primaryFrame);
  }
}



nsresult
nsCSSFrameConstructor::CreateAnonymousFrames(nsFrameConstructorState& aState,
                                             nsIContent*              aParent,
                                             nsContainerFrame*        aParentFrame,
                                             PendingBinding*          aPendingBinding,
                                             nsFrameItems&            aChildItems)
{
  nsAutoTArray<nsIAnonymousContentCreator::ContentInfo, 4> newAnonymousItems;
  nsresult rv = GetAnonymousContent(aParent, aParentFrame, newAnonymousItems);
  NS_ENSURE_SUCCESS(rv, rv);

  uint32_t count = newAnonymousItems.Length();
  if (count == 0) {
    return NS_OK;
  }

  nsFrameConstructorState::PendingBindingAutoPusher pusher(aState,
                                                           aPendingBinding);
  TreeMatchContext::AutoAncestorPusher ancestorPusher(aState.mTreeMatchContext);
  if (aState.mTreeMatchContext.mAncestorFilter.HasFilter()) {
    ancestorPusher.PushAncestorAndStyleScope(aParent->AsElement());
  } else {
    ancestorPusher.PushStyleScope(aParent->AsElement());
  }

  nsIAnonymousContentCreator* creator = do_QueryFrame(aParentFrame);
  NS_ASSERTION(creator,
               "How can that happen if we have nodes to construct frames for?");

  InsertionPoint insertion(aParentFrame, aParent);
  for (uint32_t i=0; i < count; i++) {
    nsIContent* content = newAnonymousItems[i].mContent;
    NS_ASSERTION(content, "null anonymous content?");
    NS_ASSERTION(!newAnonymousItems[i].mStyleContext, "Unexpected style context");
    NS_ASSERTION(newAnonymousItems[i].mChildren.IsEmpty(),
                 "This method is not currently used with frames that implement "
                 "nsIAnonymousContentCreator::CreateAnonymousContent to "
                 "output a list where the items have their own children");

    nsIFrame* newFrame = creator->CreateFrameFor(content);
    if (newFrame) {
      NS_ASSERTION(content->GetPrimaryFrame(),
                   "Content must have a primary frame now");
      newFrame->AddStateBits(NS_FRAME_ANONYMOUSCONTENTCREATOR_CONTENT);
      aChildItems.AddChild(newFrame);
    } else {
      FrameConstructionItemList items;
      {
        
        
        TreeMatchContext::AutoParentDisplayBasedStyleFixupSkipper
          parentDisplayBasedStyleFixupSkipper(aState.mTreeMatchContext);

        AddFrameConstructionItems(aState, content, true, insertion, items);
      }
      ConstructFramesFromItemList(aState, items, aParentFrame, aChildItems);
    }
  }

  return NS_OK;
}

static void
SetFlagsOnSubtree(nsIContent *aNode, uintptr_t aFlagsToSet)
{
#ifdef DEBUG
  
  {
    FlattenedChildIterator iter(aNode);
    NS_ASSERTION(!iter.XBLInvolved() || !iter.GetNextChild(),
                 "The node should not have any XBL children");
  }
#endif

  
  aNode->SetFlags(aFlagsToSet);

  
  uint32_t count;
  nsIContent * const *children = aNode->GetChildArray(&count);

  for (uint32_t index = 0; index < count; ++index) {
    SetFlagsOnSubtree(children[index], aFlagsToSet);
  }
}













static void
ConnectAnonymousTreeDescendants(nsIContent* aParent,
                                nsTArray<nsIAnonymousContentCreator::ContentInfo>& aContent)
{
  uint32_t count = aContent.Length();
  for (uint32_t i=0; i < count; i++) {
    nsIContent* content = aContent[i].mContent;
    NS_ASSERTION(content, "null anonymous content?");

    ConnectAnonymousTreeDescendants(content, aContent[i].mChildren);

    aParent->AppendChildTo(content, false);
  }
}

nsresult
nsCSSFrameConstructor::GetAnonymousContent(nsIContent* aParent,
                                           nsIFrame* aParentFrame,
                                           nsTArray<nsIAnonymousContentCreator::ContentInfo>& aContent)
{
  nsIAnonymousContentCreator* creator = do_QueryFrame(aParentFrame);
  if (!creator)
    return NS_OK;

  nsresult rv = creator->CreateAnonymousContent(aContent);
  NS_ENSURE_SUCCESS(rv, rv);

  uint32_t count = aContent.Length();
  for (uint32_t i=0; i < count; i++) {
    
    nsIContent* content = aContent[i].mContent;
    NS_ASSERTION(content, "null anonymous content?");

    
    
    if (aParentFrame->GetType() == nsGkAtoms::svgUseFrame) {
      content->SetFlags(NODE_IS_ANONYMOUS_ROOT);
    } else {
      content->SetIsNativeAnonymousRoot();
    }

    ConnectAnonymousTreeDescendants(content, aContent[i].mChildren);

    bool anonContentIsEditable = content->HasFlag(NODE_IS_EDITABLE);

    
    
    
    nsIDocument* bindDocument =
      aParent->HasFlag(NODE_IS_IN_SHADOW_TREE) ? nullptr : mDocument;
    rv = content->BindToTree(bindDocument, aParent, aParent, true);
    
    
    
    
    if (anonContentIsEditable) {
      NS_ASSERTION(aParentFrame->GetType() == nsGkAtoms::textInputFrame,
                   "We only expect this for anonymous content under a text control frame");
      SetFlagsOnSubtree(content, NODE_IS_EDITABLE);
    }
    if (NS_FAILED(rv)) {
      content->UnbindFromTree();
      return rv;
    }
  }

  return NS_OK;
}

static
bool IsXULDisplayType(const nsStyleDisplay* aDisplay)
{
  return (aDisplay->mDisplay == NS_STYLE_DISPLAY_INLINE_BOX ||
#ifdef MOZ_XUL
          aDisplay->mDisplay == NS_STYLE_DISPLAY_INLINE_XUL_GRID ||
          aDisplay->mDisplay == NS_STYLE_DISPLAY_INLINE_STACK ||
#endif
          aDisplay->mDisplay == NS_STYLE_DISPLAY_BOX
#ifdef MOZ_XUL
          || aDisplay->mDisplay == NS_STYLE_DISPLAY_XUL_GRID ||
          aDisplay->mDisplay == NS_STYLE_DISPLAY_STACK ||
          aDisplay->mDisplay == NS_STYLE_DISPLAY_XUL_GRID_GROUP ||
          aDisplay->mDisplay == NS_STYLE_DISPLAY_XUL_GRID_LINE ||
          aDisplay->mDisplay == NS_STYLE_DISPLAY_DECK ||
          aDisplay->mDisplay == NS_STYLE_DISPLAY_POPUP ||
          aDisplay->mDisplay == NS_STYLE_DISPLAY_GROUPBOX
#endif
          );
}



#define SIMPLE_XUL_FCDATA(_func)                                        \
  FCDATA_DECL(FCDATA_DISALLOW_OUT_OF_FLOW | FCDATA_SKIP_ABSPOS_PUSH,    \
              _func)
#define SCROLLABLE_XUL_FCDATA(_func)                                    \
  FCDATA_DECL(FCDATA_DISALLOW_OUT_OF_FLOW | FCDATA_SKIP_ABSPOS_PUSH |   \
              FCDATA_MAY_NEED_SCROLLFRAME, _func)


#define SCROLLABLE_ABSPOS_CONTAINER_XUL_FCDATA(_func)                   \
  FCDATA_DECL(FCDATA_DISALLOW_OUT_OF_FLOW |                             \
              FCDATA_MAY_NEED_SCROLLFRAME, _func)

#define SIMPLE_XUL_CREATE(_tag, _func)            \
  { &nsGkAtoms::_tag, SIMPLE_XUL_FCDATA(_func) }
#define SCROLLABLE_XUL_CREATE(_tag, _func)            \
  { &nsGkAtoms::_tag, SCROLLABLE_XUL_FCDATA(_func) }
#define SIMPLE_XUL_INT_CREATE(_int, _func)      \
  { _int, SIMPLE_XUL_FCDATA(_func) }
#define SCROLLABLE_XUL_INT_CREATE(_int, _func)                          \
  { _int, SCROLLABLE_XUL_FCDATA(_func) }
#define SCROLLABLE_ABSPOS_CONTAINER_XUL_INT_CREATE(_int, _func)         \
  { _int, SCROLLABLE_ABSPOS_CONTAINER_XUL_FCDATA(_func) }

static
nsIFrame* NS_NewGridBoxFrame(nsIPresShell* aPresShell,
                             nsStyleContext* aStyleContext)
{
  nsCOMPtr<nsBoxLayout> layout;
  NS_NewGridLayout2(aPresShell, getter_AddRefs(layout));
  return NS_NewBoxFrame(aPresShell, aStyleContext, false, layout);
}


const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindXULTagData(Element* aElement,
                                      nsIAtom* aTag,
                                      int32_t aNameSpaceID,
                                      nsStyleContext* aStyleContext)
{
  if (aNameSpaceID != kNameSpaceID_XUL) {
    return nullptr;
  }

  static const FrameConstructionDataByTag sXULTagData[] = {
#ifdef MOZ_XUL
    SCROLLABLE_XUL_CREATE(button, NS_NewButtonBoxFrame),
    SCROLLABLE_XUL_CREATE(checkbox, NS_NewButtonBoxFrame),
    SCROLLABLE_XUL_CREATE(radio, NS_NewButtonBoxFrame),
    SCROLLABLE_XUL_CREATE(autorepeatbutton, NS_NewAutoRepeatBoxFrame),
    SCROLLABLE_XUL_CREATE(titlebar, NS_NewTitleBarFrame),
    SCROLLABLE_XUL_CREATE(resizer, NS_NewResizerFrame),
    SIMPLE_XUL_CREATE(image, NS_NewImageBoxFrame),
    SIMPLE_XUL_CREATE(spring, NS_NewLeafBoxFrame),
    SIMPLE_XUL_CREATE(spacer, NS_NewLeafBoxFrame),
    SIMPLE_XUL_CREATE(treechildren, NS_NewTreeBodyFrame),
    SIMPLE_XUL_CREATE(treecol, NS_NewTreeColFrame),
    SIMPLE_XUL_CREATE(text, NS_NewTextBoxFrame),
    SIMPLE_TAG_CHAIN(label, nsCSSFrameConstructor::FindXULLabelData),
    SIMPLE_TAG_CHAIN(description, nsCSSFrameConstructor::FindXULDescriptionData),
    SIMPLE_XUL_CREATE(menu, NS_NewMenuFrame),
    SIMPLE_XUL_CREATE(menubutton, NS_NewMenuFrame),
    SIMPLE_XUL_CREATE(menuitem, NS_NewMenuItemFrame),
#ifdef XP_MACOSX
    SIMPLE_TAG_CHAIN(menubar, nsCSSFrameConstructor::FindXULMenubarData),
#else
    SIMPLE_XUL_CREATE(menubar, NS_NewMenuBarFrame),
#endif 
    SIMPLE_TAG_CHAIN(popupgroup, nsCSSFrameConstructor::FindPopupGroupData),
    SIMPLE_XUL_CREATE(iframe, NS_NewSubDocumentFrame),
    SIMPLE_XUL_CREATE(editor, NS_NewSubDocumentFrame),
    SIMPLE_XUL_CREATE(browser, NS_NewSubDocumentFrame),
    SIMPLE_XUL_CREATE(progressmeter, NS_NewProgressMeterFrame),
    SIMPLE_XUL_CREATE(splitter, NS_NewSplitterFrame),
    SIMPLE_TAG_CHAIN(listboxbody,
                     nsCSSFrameConstructor::FindXULListBoxBodyData),
    SIMPLE_TAG_CHAIN(listitem, nsCSSFrameConstructor::FindXULListItemData),
#endif 
    SIMPLE_XUL_CREATE(slider, NS_NewSliderFrame),
    SIMPLE_XUL_CREATE(scrollbar, NS_NewScrollbarFrame),
    SIMPLE_XUL_CREATE(scrollbarbutton, NS_NewScrollbarButtonFrame)
};

  return FindDataByTag(aTag, aElement, aStyleContext, sXULTagData,
                       ArrayLength(sXULTagData));
}

#ifdef MOZ_XUL

const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindPopupGroupData(Element* aElement,
                                          nsStyleContext* )
{
  if (!aElement->IsRootOfNativeAnonymousSubtree()) {
    return nullptr;
  }

  static const FrameConstructionData sPopupSetData =
    SIMPLE_XUL_FCDATA(NS_NewPopupSetFrame);
  return &sPopupSetData;
}


const nsCSSFrameConstructor::FrameConstructionData
nsCSSFrameConstructor::sXULTextBoxData = SIMPLE_XUL_FCDATA(NS_NewTextBoxFrame);


const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindXULLabelData(Element* aElement,
                                        nsStyleContext* )
{
  if (aElement->HasAttr(kNameSpaceID_None, nsGkAtoms::value)) {
    return &sXULTextBoxData;
  }

  static const FrameConstructionData sLabelData =
    SIMPLE_XUL_FCDATA(NS_NewXULLabelFrame);
  return &sLabelData;
}

static nsIFrame*
NS_NewXULDescriptionFrame(nsIPresShell* aPresShell, nsStyleContext *aContext)
{
  
  
  return NS_NewBlockFrame(aPresShell, aContext,
                          NS_BLOCK_FLOAT_MGR | NS_BLOCK_MARGIN_ROOT);
}


const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindXULDescriptionData(Element* aElement,
                                              nsStyleContext* )
{
  if (aElement->HasAttr(kNameSpaceID_None, nsGkAtoms::value)) {
    return &sXULTextBoxData;
  }

  static const FrameConstructionData sDescriptionData =
    SIMPLE_XUL_FCDATA(NS_NewXULDescriptionFrame);
  return &sDescriptionData;
}

#ifdef XP_MACOSX

const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindXULMenubarData(Element* aElement,
                                          nsStyleContext* aStyleContext)
{
  nsCOMPtr<nsIDocShell> treeItem =
    aStyleContext->PresContext()->GetDocShell();
  if (treeItem && nsIDocShellTreeItem::typeChrome == treeItem->ItemType()) {
    nsCOMPtr<nsIDocShellTreeItem> parent;
    treeItem->GetParent(getter_AddRefs(parent));
    if (!parent) {
      
      
      static const FrameConstructionData sSuppressData = SUPPRESS_FCDATA();
      return &sSuppressData;
    }
  }

  static const FrameConstructionData sMenubarData =
    SIMPLE_XUL_FCDATA(NS_NewMenuBarFrame);
  return &sMenubarData;
}
#endif 


const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindXULListBoxBodyData(Element* aElement,
                                              nsStyleContext* aStyleContext)
{
  if (aStyleContext->StyleDisplay()->mDisplay !=
        NS_STYLE_DISPLAY_XUL_GRID_GROUP) {
    return nullptr;
  }

  static const FrameConstructionData sListBoxBodyData =
    SCROLLABLE_XUL_FCDATA(NS_NewListBoxBodyFrame);
  return &sListBoxBodyData;
}


const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindXULListItemData(Element* aElement,
                                           nsStyleContext* aStyleContext)
{
  if (aStyleContext->StyleDisplay()->mDisplay !=
        NS_STYLE_DISPLAY_XUL_GRID_LINE) {
    return nullptr;
  }

  static const FrameConstructionData sListItemData =
    SCROLLABLE_XUL_FCDATA(NS_NewListItemFrame);
  return &sListItemData;
}

#endif 


const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindXULDisplayData(const nsStyleDisplay* aDisplay,
                                          Element* aElement,
                                          nsStyleContext* aStyleContext)
{
  static const FrameConstructionDataByInt sXULDisplayData[] = {
    SCROLLABLE_ABSPOS_CONTAINER_XUL_INT_CREATE(NS_STYLE_DISPLAY_INLINE_BOX,
                                               NS_NewBoxFrame),
    SCROLLABLE_ABSPOS_CONTAINER_XUL_INT_CREATE(NS_STYLE_DISPLAY_BOX,
                                               NS_NewBoxFrame),
#ifdef MOZ_XUL
    SCROLLABLE_XUL_INT_CREATE(NS_STYLE_DISPLAY_INLINE_XUL_GRID, NS_NewGridBoxFrame),
    SCROLLABLE_XUL_INT_CREATE(NS_STYLE_DISPLAY_XUL_GRID, NS_NewGridBoxFrame),
    SCROLLABLE_XUL_INT_CREATE(NS_STYLE_DISPLAY_XUL_GRID_GROUP,
                              NS_NewGridRowGroupFrame),
    SCROLLABLE_XUL_INT_CREATE(NS_STYLE_DISPLAY_XUL_GRID_LINE,
                              NS_NewGridRowLeafFrame),
    SIMPLE_XUL_INT_CREATE(NS_STYLE_DISPLAY_DECK, NS_NewDeckFrame),
    SCROLLABLE_XUL_INT_CREATE(NS_STYLE_DISPLAY_GROUPBOX, NS_NewGroupBoxFrame),
    SCROLLABLE_XUL_INT_CREATE(NS_STYLE_DISPLAY_INLINE_STACK, NS_NewStackFrame),
    SCROLLABLE_XUL_INT_CREATE(NS_STYLE_DISPLAY_STACK, NS_NewStackFrame),
    { NS_STYLE_DISPLAY_POPUP,
      FCDATA_DECL(FCDATA_DISALLOW_OUT_OF_FLOW | FCDATA_IS_POPUP |
                  FCDATA_SKIP_ABSPOS_PUSH, NS_NewMenuPopupFrame) }
#endif 
  };

  
  return FindDataByInt(aDisplay->mDisplay, aElement, aStyleContext,
                       sXULDisplayData, ArrayLength(sXULDisplayData));
}

already_AddRefed<nsStyleContext>
nsCSSFrameConstructor::BeginBuildingScrollFrame(nsFrameConstructorState& aState,
                                                nsIContent*              aContent,
                                                nsStyleContext*          aContentStyle,
                                                nsContainerFrame*        aParentFrame,
                                                nsIAtom*                 aScrolledPseudo,
                                                bool                     aIsRoot,
                                                nsContainerFrame*&       aNewFrame)
{
  nsContainerFrame* gfxScrollFrame = aNewFrame;

  nsFrameItems anonymousItems;

  nsRefPtr<nsStyleContext> contentStyle = aContentStyle;

  if (!gfxScrollFrame) {
    
    
    
    
    const nsStyleDisplay* displayStyle = aContentStyle->StyleDisplay();
    if (IsXULDisplayType(displayStyle)) {
      gfxScrollFrame = NS_NewXULScrollFrame(mPresShell, contentStyle, aIsRoot,
          displayStyle->mDisplay == NS_STYLE_DISPLAY_STACK ||
          displayStyle->mDisplay == NS_STYLE_DISPLAY_INLINE_STACK);
    } else {
      gfxScrollFrame = NS_NewHTMLScrollFrame(mPresShell, contentStyle, aIsRoot);
    }

    InitAndRestoreFrame(aState, aContent, aParentFrame, gfxScrollFrame);
  }

  
  
  
  
  
  CreateAnonymousFrames(aState, aContent, gfxScrollFrame, nullptr,
                        anonymousItems);

  aNewFrame = gfxScrollFrame;

  
  nsStyleSet *styleSet = mPresShell->StyleSet();
  nsRefPtr<nsStyleContext> scrolledChildStyle =
    styleSet->ResolveAnonymousBoxStyle(aScrolledPseudo, contentStyle);

  if (gfxScrollFrame) {
     gfxScrollFrame->SetInitialChildList(kPrincipalList, anonymousItems);
  }

  return scrolledChildStyle.forget();
}

void
nsCSSFrameConstructor::FinishBuildingScrollFrame(nsContainerFrame* aScrollFrame,
                                                 nsIFrame* aScrolledFrame)
{
  nsFrameList scrolled(aScrolledFrame, aScrolledFrame);
  aScrollFrame->AppendFrames(kPrincipalList, scrolled);
}
































void
nsCSSFrameConstructor::BuildScrollFrame(nsFrameConstructorState& aState,
                                        nsIContent*              aContent,
                                        nsStyleContext*          aContentStyle,
                                        nsIFrame*                aScrolledFrame,
                                        nsContainerFrame*        aParentFrame,
                                        nsContainerFrame*&       aNewFrame)
{
  nsRefPtr<nsStyleContext> scrolledContentStyle =
    BeginBuildingScrollFrame(aState, aContent, aContentStyle, aParentFrame,
                             nsCSSAnonBoxes::scrolledContent,
                             false, aNewFrame);

  aScrolledFrame->SetStyleContextWithoutNotification(scrolledContentStyle);
  InitAndRestoreFrame(aState, aContent, aNewFrame, aScrolledFrame);

  FinishBuildingScrollFrame(aNewFrame, aScrolledFrame);
}

const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindDisplayData(const nsStyleDisplay* aDisplay,
                                       Element* aElement,
                                       nsIFrame* aParentFrame,
                                       nsStyleContext* aStyleContext)
{
  PR_STATIC_ASSERT(eParentTypeCount < (1 << (32 - FCDATA_PARENT_TYPE_OFFSET)));

  
  
  NS_ASSERTION(!(aDisplay->IsFloatingStyle() ||
                 aDisplay->IsAbsolutelyPositionedStyle()) ||
               aDisplay->IsBlockOutsideStyle() ||
               aDisplay->mDisplay == NS_STYLE_DISPLAY_CONTENTS,
               "Style system did not apply CSS2.1 section 9.7 fixups");

  
  
  
  
  
  
  bool propagatedScrollToViewport = false;
  if (aElement->IsHTMLElement(nsGkAtoms::body)) {
    propagatedScrollToViewport =
      PropagateScrollToViewport() == aElement;
  }

  NS_ASSERTION(!propagatedScrollToViewport ||
               !mPresShell->GetPresContext()->IsPaginated(),
               "Shouldn't propagate scroll in paginated contexts");

  if (aDisplay->IsBlockInsideStyle()) {
    
    
    
    
    const uint32_t kCaptionCtorFlags =
      FCDATA_IS_TABLE_PART | FCDATA_DESIRED_PARENT_TYPE_TO_BITS(eTypeTable);
    bool caption = aDisplay->mDisplay == NS_STYLE_DISPLAY_TABLE_CAPTION;
    bool suppressScrollFrame = false;
    bool needScrollFrame = aDisplay->IsScrollableOverflow() &&
                           !propagatedScrollToViewport;
    if (needScrollFrame) {
      suppressScrollFrame = mPresShell->GetPresContext()->IsPaginated() &&
                            aDisplay->IsBlockOutsideStyle() &&
                            !aElement->IsInNativeAnonymousSubtree();
      if (!suppressScrollFrame) {
        static const FrameConstructionData sScrollableBlockData[2] =
          { FULL_CTOR_FCDATA(0, &nsCSSFrameConstructor::ConstructScrollableBlock),
            FULL_CTOR_FCDATA(kCaptionCtorFlags,
                             &nsCSSFrameConstructor::ConstructScrollableBlock) };
        return &sScrollableBlockData[caption];
      }
    }

    
    static const FrameConstructionData sNonScrollableBlockData[2][2] = {
      { FULL_CTOR_FCDATA(0,
                         &nsCSSFrameConstructor::ConstructNonScrollableBlock),
        FULL_CTOR_FCDATA(kCaptionCtorFlags,
                         &nsCSSFrameConstructor::ConstructNonScrollableBlock) },
      { FULL_CTOR_FCDATA(FCDATA_FORCED_NON_SCROLLABLE_BLOCK,
                         &nsCSSFrameConstructor::ConstructNonScrollableBlock),
        FULL_CTOR_FCDATA(FCDATA_FORCED_NON_SCROLLABLE_BLOCK | kCaptionCtorFlags,
                         &nsCSSFrameConstructor::ConstructNonScrollableBlock) }
    };
    return &sNonScrollableBlockData[suppressScrollFrame][caption];
  }

  
  
  
  if (propagatedScrollToViewport && aDisplay->IsScrollableOverflow()) {
    if (aDisplay->mDisplay == NS_STYLE_DISPLAY_FLEX) {
      static const FrameConstructionData sNonScrollableFlexData =
        FCDATA_DECL(0, NS_NewFlexContainerFrame);
      return &sNonScrollableFlexData;
    }
    if (aDisplay->mDisplay == NS_STYLE_DISPLAY_GRID) {
      static const FrameConstructionData sNonScrollableGridData =
        FCDATA_DECL(0, NS_NewGridContainerFrame);
      return &sNonScrollableGridData;
    }
  }

  static const FrameConstructionDataByInt sDisplayData[] = {
    
    
    
    
    { NS_STYLE_DISPLAY_INLINE,
      FULL_CTOR_FCDATA(FCDATA_IS_INLINE | FCDATA_IS_LINE_PARTICIPANT,
                       &nsCSSFrameConstructor::ConstructInline) },
    { NS_STYLE_DISPLAY_FLEX,
      FCDATA_DECL(FCDATA_MAY_NEED_SCROLLFRAME, NS_NewFlexContainerFrame) },
    { NS_STYLE_DISPLAY_INLINE_FLEX,
      FCDATA_DECL(FCDATA_MAY_NEED_SCROLLFRAME, NS_NewFlexContainerFrame) },
    { NS_STYLE_DISPLAY_GRID,
      FCDATA_DECL(FCDATA_MAY_NEED_SCROLLFRAME, NS_NewGridContainerFrame) },
    { NS_STYLE_DISPLAY_INLINE_GRID,
      FCDATA_DECL(FCDATA_MAY_NEED_SCROLLFRAME, NS_NewGridContainerFrame) },
    { NS_STYLE_DISPLAY_RUBY,
      FCDATA_DECL(FCDATA_IS_LINE_PARTICIPANT,
                  NS_NewRubyFrame) },
    { NS_STYLE_DISPLAY_RUBY_BASE,
      FCDATA_DECL(FCDATA_IS_LINE_PARTICIPANT |
                  FCDATA_DESIRED_PARENT_TYPE_TO_BITS(eTypeRubyBaseContainer),
                  NS_NewRubyBaseFrame) },
    { NS_STYLE_DISPLAY_RUBY_BASE_CONTAINER,
      FCDATA_DECL(FCDATA_IS_LINE_PARTICIPANT |
                  FCDATA_DESIRED_PARENT_TYPE_TO_BITS(eTypeRuby),
                  NS_NewRubyBaseContainerFrame) },
    { NS_STYLE_DISPLAY_RUBY_TEXT,
      FCDATA_DECL(FCDATA_IS_LINE_PARTICIPANT |
                  FCDATA_DESIRED_PARENT_TYPE_TO_BITS(eTypeRubyTextContainer),
                  NS_NewRubyTextFrame) },
    { NS_STYLE_DISPLAY_RUBY_TEXT_CONTAINER,
      FCDATA_DECL(FCDATA_DESIRED_PARENT_TYPE_TO_BITS(eTypeRuby),
                  NS_NewRubyTextContainerFrame) },
    { NS_STYLE_DISPLAY_TABLE,
      FULL_CTOR_FCDATA(0, &nsCSSFrameConstructor::ConstructTable) },
    { NS_STYLE_DISPLAY_INLINE_TABLE,
      FULL_CTOR_FCDATA(0, &nsCSSFrameConstructor::ConstructTable) },
    
    
    
    { NS_STYLE_DISPLAY_TABLE_ROW_GROUP,
      FULL_CTOR_FCDATA(FCDATA_IS_TABLE_PART |
                       FCDATA_DESIRED_PARENT_TYPE_TO_BITS(eTypeTable),
                       &nsCSSFrameConstructor::ConstructTableRowOrRowGroup) },
    { NS_STYLE_DISPLAY_TABLE_HEADER_GROUP,
      FULL_CTOR_FCDATA(FCDATA_IS_TABLE_PART |
                       FCDATA_DESIRED_PARENT_TYPE_TO_BITS(eTypeTable),
                       &nsCSSFrameConstructor::ConstructTableRowOrRowGroup) },
    { NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP,
      FULL_CTOR_FCDATA(FCDATA_IS_TABLE_PART |
                       FCDATA_DESIRED_PARENT_TYPE_TO_BITS(eTypeTable),
                       &nsCSSFrameConstructor::ConstructTableRowOrRowGroup) },
    { NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP,
      FCDATA_DECL(FCDATA_IS_TABLE_PART | FCDATA_DISALLOW_OUT_OF_FLOW |
                  FCDATA_SKIP_ABSPOS_PUSH |
                  FCDATA_DESIRED_PARENT_TYPE_TO_BITS(eTypeTable),
                  NS_NewTableColGroupFrame) },
    { NS_STYLE_DISPLAY_TABLE_COLUMN,
      FULL_CTOR_FCDATA(FCDATA_IS_TABLE_PART |
                       FCDATA_DESIRED_PARENT_TYPE_TO_BITS(eTypeColGroup),
                       &nsCSSFrameConstructor::ConstructTableCol) },
    { NS_STYLE_DISPLAY_TABLE_ROW,
      FULL_CTOR_FCDATA(FCDATA_IS_TABLE_PART |
                       FCDATA_DESIRED_PARENT_TYPE_TO_BITS(eTypeRowGroup),
                       &nsCSSFrameConstructor::ConstructTableRowOrRowGroup) },
    { NS_STYLE_DISPLAY_TABLE_CELL,
      FULL_CTOR_FCDATA(FCDATA_IS_TABLE_PART |
                       FCDATA_DESIRED_PARENT_TYPE_TO_BITS(eTypeRow),
                       &nsCSSFrameConstructor::ConstructTableCell) },
    { NS_STYLE_DISPLAY_CONTENTS,
      FULL_CTOR_FCDATA(FCDATA_IS_CONTENTS, nullptr) }
  };

  
  MOZ_ASSERT(aDisplay->mDisplay != NS_STYLE_DISPLAY_CONTENTS ||
             !aElement->IsRootOfNativeAnonymousSubtree(),
             "display:contents on anonymous content is unsupported");

  return FindDataByInt(aDisplay->mDisplay,
                       aElement, aStyleContext, sDisplayData,
                       ArrayLength(sDisplayData));
}

nsIFrame*
nsCSSFrameConstructor::ConstructScrollableBlock(nsFrameConstructorState& aState,
                                                FrameConstructionItem&   aItem,
                                                nsContainerFrame*        aParentFrame,
                                                const nsStyleDisplay*    aDisplay,
                                                nsFrameItems&            aFrameItems)
{
  nsIContent* const content = aItem.mContent;
  nsStyleContext* const styleContext = aItem.mStyleContext;

  nsContainerFrame* newFrame = nullptr;
  nsRefPtr<nsStyleContext> scrolledContentStyle
    = BeginBuildingScrollFrame(aState, content, styleContext,
                               aState.GetGeometricParent(aDisplay, aParentFrame),
                               nsCSSAnonBoxes::scrolledContent,
                               false, newFrame);

  
  
  nsContainerFrame* scrolledFrame =
    NS_NewBlockFormattingContext(mPresShell, styleContext);

  
  
  aState.AddChild(newFrame, aFrameItems, content, styleContext, aParentFrame);

  nsFrameItems blockItem;
  ConstructBlock(aState, scrolledContentStyle->StyleDisplay(), content,
                 newFrame, newFrame, scrolledContentStyle,
                 &scrolledFrame, blockItem,
                 aDisplay->IsAbsPosContainingBlock(newFrame) ? newFrame : nullptr,
                 aItem.mPendingBinding);

  NS_ASSERTION(blockItem.FirstChild() == scrolledFrame,
               "Scrollframe's frameItems should be exactly the scrolled frame");
  FinishBuildingScrollFrame(newFrame, scrolledFrame);

  return newFrame;
}

nsIFrame*
nsCSSFrameConstructor::ConstructNonScrollableBlock(nsFrameConstructorState& aState,
                                                   FrameConstructionItem&   aItem,
                                                   nsContainerFrame*        aParentFrame,
                                                   const nsStyleDisplay*    aDisplay,
                                                   nsFrameItems&            aFrameItems)
{
  nsStyleContext* const styleContext = aItem.mStyleContext;

  
  
  
  
  bool clipPaginatedOverflow =
    (aItem.mFCData->mBits & FCDATA_FORCED_NON_SCROLLABLE_BLOCK) != 0;
  nsContainerFrame* newFrame;
  if ((aDisplay->IsAbsolutelyPositionedStyle() ||
       aDisplay->IsFloatingStyle() ||
       NS_STYLE_DISPLAY_INLINE_BLOCK == aDisplay->mDisplay ||
       clipPaginatedOverflow) &&
      !aParentFrame->IsSVGText()) {
    newFrame = NS_NewBlockFormattingContext(mPresShell, styleContext);
    if (clipPaginatedOverflow) {
      newFrame->AddStateBits(NS_BLOCK_CLIP_PAGINATED_OVERFLOW);
    }
  } else {
    newFrame = NS_NewBlockFrame(mPresShell, styleContext);
  }

  ConstructBlock(aState, aDisplay, aItem.mContent,
                 aState.GetGeometricParent(aDisplay, aParentFrame),
                 aParentFrame, styleContext, &newFrame,
                 aFrameItems,
                 aDisplay->IsAbsPosContainingBlock(newFrame) ? newFrame : nullptr,
                 aItem.mPendingBinding);
  return newFrame;
}


void
nsCSSFrameConstructor::InitAndRestoreFrame(const nsFrameConstructorState& aState,
                                           nsIContent*              aContent,
                                           nsContainerFrame*        aParentFrame,
                                           nsIFrame*                aNewFrame,
                                           bool                     aAllowCounters)
{
  NS_PRECONDITION(mUpdateCount != 0,
                  "Should be in an update while creating frames");

  MOZ_ASSERT(aNewFrame, "Null frame cannot be initialized");

  
  aNewFrame->Init(aContent, aParentFrame, nullptr);
  aNewFrame->AddStateBits(aState.mAdditionalStateBits);

  if (aState.mFrameState) {
    
    RestoreFrameStateFor(aNewFrame, aState.mFrameState);
  }

  if (aAllowCounters &&
      mCounterManager.AddCounterResetsAndIncrements(aNewFrame)) {
    CountersDirty();
  }
}

already_AddRefed<nsStyleContext>
nsCSSFrameConstructor::ResolveStyleContext(nsIFrame*         aParentFrame,
                                           nsIContent*       aContainer,
                                           nsIContent*       aChild,
                                           nsFrameConstructorState* aState)
{
  MOZ_ASSERT(aContainer, "Must have parent here");
  
  
  nsStyleContext* parentStyleContext = GetDisplayContentsStyleFor(aContainer);
  if (MOZ_LIKELY(!parentStyleContext)) {
    aParentFrame = nsFrame::CorrectStyleParentFrame(aParentFrame, nullptr);
    if (aParentFrame) {
      MOZ_ASSERT(aParentFrame->GetContent() == aContainer);
      
      
      parentStyleContext = aParentFrame->StyleContext();
    } else {
      
      
      
      
      
    }
  }

  return ResolveStyleContext(parentStyleContext, aChild, aState);
}

already_AddRefed<nsStyleContext>
nsCSSFrameConstructor::ResolveStyleContext(nsIFrame*          aParentFrame,
                                           nsIContent*              aChild,
                                           nsFrameConstructorState* aState)
{
  return ResolveStyleContext(aParentFrame, aChild->GetFlattenedTreeParent(), aChild, aState);
}

already_AddRefed<nsStyleContext>
nsCSSFrameConstructor::ResolveStyleContext(const InsertionPoint&    aInsertion,
                                           nsIContent*              aChild,
                                           nsFrameConstructorState* aState)
{
  return ResolveStyleContext(aInsertion.mParentFrame, aInsertion.mContainer,
                             aChild, aState);
}

already_AddRefed<nsStyleContext>
nsCSSFrameConstructor::ResolveStyleContext(nsStyleContext* aParentStyleContext,
                                           nsIContent* aContent,
                                           nsFrameConstructorState* aState)
{
  nsStyleSet *styleSet = mPresShell->StyleSet();
  aContent->OwnerDoc()->FlushPendingLinkUpdates();

  nsRefPtr<nsStyleContext> result;
  if (aContent->IsElement()) {
    if (aState) {
      result = styleSet->ResolveStyleFor(aContent->AsElement(),
                                         aParentStyleContext,
                                         aState->mTreeMatchContext);
    } else {
      result = styleSet->ResolveStyleFor(aContent->AsElement(),
                                         aParentStyleContext);
    }
  } else {
    NS_ASSERTION(aContent->IsNodeOfType(nsINode::eTEXT),
                 "shouldn't waste time creating style contexts for "
                 "comments and processing instructions");
    result = styleSet->ResolveStyleForNonElement(aParentStyleContext);
  }

  RestyleManager::ReframingStyleContexts* rsc =
    RestyleManager()->GetReframingStyleContexts();
  if (rsc) {
    nsStyleContext* oldStyleContext =
      rsc->Get(aContent, nsCSSPseudoElements::ePseudo_NotPseudoElement);
    if (oldStyleContext) {
      RestyleManager::TryStartingTransition(mPresShell->GetPresContext(),
                                            aContent,
                                            oldStyleContext, &result);
    }
  }

  return result.forget();
}


void
nsCSSFrameConstructor::FlushAccumulatedBlock(nsFrameConstructorState& aState,
                                             nsIContent* aContent,
                                             nsContainerFrame* aParentFrame,
                                             nsFrameItems& aBlockItems,
                                             nsFrameItems& aNewItems)
{
  if (aBlockItems.IsEmpty()) {
    
    return;
  }

  nsIAtom* anonPseudo = nsCSSAnonBoxes::mozMathMLAnonymousBlock;

  nsStyleContext* parentContext =
    nsFrame::CorrectStyleParentFrame(aParentFrame,
                                     anonPseudo)->StyleContext();
  nsStyleSet* styleSet = mPresShell->StyleSet();
  nsRefPtr<nsStyleContext> blockContext;
  blockContext = styleSet->
    ResolveAnonymousBoxStyle(anonPseudo, parentContext);


  
  
  
  nsContainerFrame* blockFrame =
      NS_NewMathMLmathBlockFrame(mPresShell, blockContext,
                                 NS_BLOCK_FLOAT_MGR | NS_BLOCK_MARGIN_ROOT);

  InitAndRestoreFrame(aState, aContent, aParentFrame, blockFrame);
  ReparentFrames(this, blockFrame, aBlockItems);
  
  
  blockFrame->SetInitialChildList(kPrincipalList, aBlockItems);
  NS_ASSERTION(aBlockItems.IsEmpty(), "What happened?");
  aBlockItems.Clear();
  aNewItems.AddChild(blockFrame);
}



#define SIMPLE_MATHML_CREATE(_tag, _func)                               \
  { &nsGkAtoms::_tag,                                                   \
      FCDATA_DECL(FCDATA_DISALLOW_OUT_OF_FLOW |                         \
                  FCDATA_FORCE_NULL_ABSPOS_CONTAINER |                  \
                  FCDATA_WRAP_KIDS_IN_BLOCKS, _func) }


const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindMathMLData(Element* aElement,
                                      nsIAtom* aTag,
                                      int32_t aNameSpaceID,
                                      nsStyleContext* aStyleContext)
{
  
  if (aNameSpaceID != kNameSpaceID_MathML)
    return nullptr;

  
  if (aTag == nsGkAtoms::math) {
    
    
    
    if (aStyleContext->StyleDisplay()->IsBlockOutsideStyle()) {
      static const FrameConstructionData sBlockMathData =
        FCDATA_DECL(FCDATA_FORCE_NULL_ABSPOS_CONTAINER |
                    FCDATA_WRAP_KIDS_IN_BLOCKS,
                    NS_CreateNewMathMLmathBlockFrame);
      return &sBlockMathData;
    }

    static const FrameConstructionData sInlineMathData =
      FCDATA_DECL(FCDATA_FORCE_NULL_ABSPOS_CONTAINER |
                  FCDATA_IS_LINE_PARTICIPANT |
                  FCDATA_WRAP_KIDS_IN_BLOCKS,
                  NS_NewMathMLmathInlineFrame);
    return &sInlineMathData;
  }


  static const FrameConstructionDataByTag sMathMLData[] = {
    SIMPLE_MATHML_CREATE(annotation_, NS_NewMathMLTokenFrame),
    SIMPLE_MATHML_CREATE(annotation_xml_, NS_NewMathMLmrowFrame),
    SIMPLE_MATHML_CREATE(mi_, NS_NewMathMLTokenFrame),
    SIMPLE_MATHML_CREATE(mn_, NS_NewMathMLTokenFrame),
    SIMPLE_MATHML_CREATE(ms_, NS_NewMathMLTokenFrame),
    SIMPLE_MATHML_CREATE(mtext_, NS_NewMathMLTokenFrame),
    SIMPLE_MATHML_CREATE(mo_, NS_NewMathMLmoFrame),
    SIMPLE_MATHML_CREATE(mfrac_, NS_NewMathMLmfracFrame),
    SIMPLE_MATHML_CREATE(msup_, NS_NewMathMLmmultiscriptsFrame),
    SIMPLE_MATHML_CREATE(msub_, NS_NewMathMLmmultiscriptsFrame),
    SIMPLE_MATHML_CREATE(msubsup_, NS_NewMathMLmmultiscriptsFrame),
    SIMPLE_MATHML_CREATE(munder_, NS_NewMathMLmunderoverFrame),
    SIMPLE_MATHML_CREATE(mover_, NS_NewMathMLmunderoverFrame),
    SIMPLE_MATHML_CREATE(munderover_, NS_NewMathMLmunderoverFrame),
    SIMPLE_MATHML_CREATE(mphantom_, NS_NewMathMLmrowFrame),
    SIMPLE_MATHML_CREATE(mpadded_, NS_NewMathMLmpaddedFrame),
    SIMPLE_MATHML_CREATE(mspace_, NS_NewMathMLmspaceFrame),
    SIMPLE_MATHML_CREATE(none, NS_NewMathMLmspaceFrame),
    SIMPLE_MATHML_CREATE(mprescripts_, NS_NewMathMLmspaceFrame),
    SIMPLE_MATHML_CREATE(mfenced_, NS_NewMathMLmfencedFrame),
    SIMPLE_MATHML_CREATE(mmultiscripts_, NS_NewMathMLmmultiscriptsFrame),
    SIMPLE_MATHML_CREATE(mstyle_, NS_NewMathMLmrowFrame),
    SIMPLE_MATHML_CREATE(msqrt_, NS_NewMathMLmsqrtFrame),
    SIMPLE_MATHML_CREATE(mroot_, NS_NewMathMLmrootFrame),
    SIMPLE_MATHML_CREATE(maction_, NS_NewMathMLmactionFrame),
    SIMPLE_MATHML_CREATE(mrow_, NS_NewMathMLmrowFrame),
    SIMPLE_MATHML_CREATE(merror_, NS_NewMathMLmrowFrame),
    SIMPLE_MATHML_CREATE(menclose_, NS_NewMathMLmencloseFrame),
    SIMPLE_MATHML_CREATE(semantics_, NS_NewMathMLsemanticsFrame)
  };

  return FindDataByTag(aTag, aElement, aStyleContext, sMathMLData,
                       ArrayLength(sMathMLData));
}


nsContainerFrame*
nsCSSFrameConstructor::ConstructFrameWithAnonymousChild(
                                   nsFrameConstructorState& aState,
                                   FrameConstructionItem&   aItem,
                                   nsContainerFrame*        aParentFrame,
                                   const nsStyleDisplay*    aDisplay,
                                   nsFrameItems&            aFrameItems,
                                   ContainerFrameCreationFunc aConstructor,
                                   ContainerFrameCreationFunc aInnerConstructor,
                                   nsICSSAnonBoxPseudo*     aInnerPseudo,
                                   bool                     aCandidateRootFrame)
{
  nsIContent* const content = aItem.mContent;
  nsStyleContext* const styleContext = aItem.mStyleContext;

  
  nsContainerFrame* newFrame = aConstructor(mPresShell, styleContext);

  InitAndRestoreFrame(aState, content,
                      aCandidateRootFrame ?
                        aState.GetGeometricParent(styleContext->StyleDisplay(),
                                                  aParentFrame) :
                        aParentFrame,
                      newFrame);

  
  nsRefPtr<nsStyleContext> scForAnon;
  scForAnon = mPresShell->StyleSet()->
    ResolveAnonymousBoxStyle(aInnerPseudo, styleContext);

  
  nsContainerFrame* innerFrame = aInnerConstructor(mPresShell, scForAnon);

  InitAndRestoreFrame(aState, content, newFrame, innerFrame);

  
  SetInitialSingleChild(newFrame, innerFrame);

  aState.AddChild(newFrame, aFrameItems, content, styleContext, aParentFrame,
                  aCandidateRootFrame, aCandidateRootFrame);

  if (!mRootElementFrame && aCandidateRootFrame) {
    
    
    mRootElementFrame = newFrame;
  }

  nsFrameItems childItems;

  
  NS_ASSERTION(aItem.mAnonChildren.IsEmpty(),
               "nsIAnonymousContentCreator::CreateAnonymousContent should not "
               "be implemented for frames for which we explicitly create an "
               "anonymous child to wrap its child frames");
  if (aItem.mFCData->mBits & FCDATA_USE_CHILD_ITEMS) {
    ConstructFramesFromItemList(aState, aItem.mChildItems,
                                innerFrame, childItems);
  } else {
    ProcessChildren(aState, content, styleContext, innerFrame,
                    true, childItems, false, aItem.mPendingBinding);
  }

  
  innerFrame->SetInitialChildList(kPrincipalList, childItems);

  return newFrame;
}

nsIFrame*
nsCSSFrameConstructor::ConstructOuterSVG(nsFrameConstructorState& aState,
                                         FrameConstructionItem&   aItem,
                                         nsContainerFrame*        aParentFrame,
                                         const nsStyleDisplay*    aDisplay,
                                         nsFrameItems&            aFrameItems)
{
  return ConstructFrameWithAnonymousChild(
      aState, aItem, aParentFrame, aDisplay, aFrameItems,
      NS_NewSVGOuterSVGFrame, NS_NewSVGOuterSVGAnonChildFrame,
      nsCSSAnonBoxes::mozSVGOuterSVGAnonChild, true);
}

nsIFrame*
nsCSSFrameConstructor::ConstructMarker(nsFrameConstructorState& aState,
                                       FrameConstructionItem&   aItem,
                                       nsContainerFrame*        aParentFrame,
                                       const nsStyleDisplay*    aDisplay,
                                       nsFrameItems&            aFrameItems)
{
  return ConstructFrameWithAnonymousChild(
      aState, aItem, aParentFrame, aDisplay, aFrameItems,
      NS_NewSVGMarkerFrame, NS_NewSVGMarkerAnonChildFrame,
      nsCSSAnonBoxes::mozSVGMarkerAnonChild, false);
}



#define SIMPLE_SVG_FCDATA(_func)                                        \
  FCDATA_DECL(FCDATA_DISALLOW_OUT_OF_FLOW |                             \
              FCDATA_SKIP_ABSPOS_PUSH |                                 \
              FCDATA_DISALLOW_GENERATED_CONTENT,  _func)
#define SIMPLE_SVG_CREATE(_tag, _func)            \
  { &nsGkAtoms::_tag, SIMPLE_SVG_FCDATA(_func) }

static bool
IsFilterPrimitiveChildTag(const nsIAtom* aTag)
{
  return aTag == nsGkAtoms::feDistantLight ||
         aTag == nsGkAtoms::fePointLight ||
         aTag == nsGkAtoms::feSpotLight ||
         aTag == nsGkAtoms::feFuncR ||
         aTag == nsGkAtoms::feFuncG ||
         aTag == nsGkAtoms::feFuncB ||
         aTag == nsGkAtoms::feFuncA ||
         aTag == nsGkAtoms::feMergeNode;
}


const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindSVGData(Element* aElement,
                                   nsIAtom* aTag,
                                   int32_t aNameSpaceID,
                                   nsIFrame* aParentFrame,
                                   bool aIsWithinSVGText,
                                   bool aAllowsTextPathChild,
                                   nsStyleContext* aStyleContext)
{
  if (aNameSpaceID != kNameSpaceID_SVG) {
    return nullptr;
  }

  static const FrameConstructionData sSuppressData = SUPPRESS_FCDATA();
  static const FrameConstructionData sContainerData =
    SIMPLE_SVG_FCDATA(NS_NewSVGContainerFrame);

  bool parentIsSVG = aIsWithinSVGText;
  nsIContent* parentContent =
    aParentFrame ? aParentFrame->GetContent() : nullptr;
  
  
  
  if (parentContent) {
    int32_t parentNSID;
    nsIAtom* parentTag =
      parentContent->OwnerDoc()->BindingManager()->
        ResolveTag(parentContent, &parentNSID);

    
    
    
    
    parentIsSVG = parentNSID == kNameSpaceID_SVG &&
                  parentTag != nsGkAtoms::foreignObject;
  }

  if ((aTag != nsGkAtoms::svg && !parentIsSVG) ||
      (aTag == nsGkAtoms::desc || aTag == nsGkAtoms::title)) {
    
    
    
    
    
    
    
    
    
    
    
    return &sSuppressData;
  }

  
  if (aElement->IsNodeOfType(nsINode::eANIMATION)) {
    return &sSuppressData;
  }

  if (aTag == nsGkAtoms::svg && !parentIsSVG) {
    
    
    
    
    
    
    static const FrameConstructionData sOuterSVGData =
      FULL_CTOR_FCDATA(0, &nsCSSFrameConstructor::ConstructOuterSVG);
    return &sOuterSVGData;
  }

  if (aTag == nsGkAtoms::marker) {
    static const FrameConstructionData sMarkerSVGData =
      FULL_CTOR_FCDATA(0, &nsCSSFrameConstructor::ConstructMarker);
    return &sMarkerSVGData;
  }

  nsCOMPtr<SVGTests> tests(do_QueryInterface(aElement));
  if (tests && !tests->PassesConditionalProcessingTests()) {
    
    
    
    return &sContainerData;
  }

  
  
  bool parentIsGradient = aParentFrame &&
    (aParentFrame->GetType() == nsGkAtoms::svgLinearGradientFrame ||
     aParentFrame->GetType() == nsGkAtoms::svgRadialGradientFrame);
  bool stop = (aTag == nsGkAtoms::stop);
  if ((parentIsGradient && !stop) ||
      (!parentIsGradient && stop)) {
    return &sSuppressData;
  }

  
  
  
  bool parentIsFilter = aParentFrame &&
    aParentFrame->GetType() == nsGkAtoms::svgFilterFrame;
  bool filterPrimitive = aElement->IsNodeOfType(nsINode::eFILTER);
  if ((parentIsFilter && !filterPrimitive) ||
      (!parentIsFilter && filterPrimitive)) {
    return &sSuppressData;
  }

  
  
  
  
  bool parentIsFEContainerFrame = aParentFrame &&
    aParentFrame->GetType() == nsGkAtoms::svgFEContainerFrame;
  if ((parentIsFEContainerFrame && !IsFilterPrimitiveChildTag(aTag)) ||
      (!parentIsFEContainerFrame && IsFilterPrimitiveChildTag(aTag))) {
    return &sSuppressData;
  }

  
  
  
  if (aIsWithinSVGText) {
    
    

    
    
    static const FrameConstructionData sTSpanData =
      FCDATA_DECL(FCDATA_DISALLOW_OUT_OF_FLOW |
                  FCDATA_SKIP_ABSPOS_PUSH |
                  FCDATA_DISALLOW_GENERATED_CONTENT |
                  FCDATA_IS_LINE_PARTICIPANT |
                  FCDATA_IS_INLINE |
                  FCDATA_USE_CHILD_ITEMS,
                  NS_NewInlineFrame);
    if (aTag == nsGkAtoms::textPath) {
      if (aAllowsTextPathChild) {
        return &sTSpanData;
      }
    } else if (aTag == nsGkAtoms::tspan ||
               aTag == nsGkAtoms::altGlyph ||
               aTag == nsGkAtoms::a) {
      return &sTSpanData;
    }
    return &sSuppressData;
  } else if (aTag == nsGkAtoms::text) {
    static const FrameConstructionData sTextData =
      FCDATA_WITH_WRAPPING_BLOCK(FCDATA_DISALLOW_OUT_OF_FLOW |
                                 FCDATA_ALLOW_BLOCK_STYLES,
                                 NS_NewSVGTextFrame,
                                 nsCSSAnonBoxes::mozSVGText);
    return &sTextData;
  } else if (aTag == nsGkAtoms::tspan ||
             aTag == nsGkAtoms::altGlyph ||
             aTag == nsGkAtoms::textPath) {
    return &sSuppressData;
  }

  static const FrameConstructionDataByTag sSVGData[] = {
    SIMPLE_SVG_CREATE(svg, NS_NewSVGInnerSVGFrame),
    SIMPLE_SVG_CREATE(g, NS_NewSVGGFrame),
    SIMPLE_SVG_CREATE(svgSwitch, NS_NewSVGSwitchFrame),
    SIMPLE_SVG_CREATE(polygon, NS_NewSVGPathGeometryFrame),
    SIMPLE_SVG_CREATE(polyline, NS_NewSVGPathGeometryFrame),
    SIMPLE_SVG_CREATE(circle, NS_NewSVGPathGeometryFrame),
    SIMPLE_SVG_CREATE(ellipse, NS_NewSVGPathGeometryFrame),
    SIMPLE_SVG_CREATE(line, NS_NewSVGPathGeometryFrame),
    SIMPLE_SVG_CREATE(rect, NS_NewSVGPathGeometryFrame),
    SIMPLE_SVG_CREATE(path, NS_NewSVGPathGeometryFrame),
    SIMPLE_SVG_CREATE(defs, NS_NewSVGContainerFrame),
    SIMPLE_SVG_CREATE(generic_, NS_NewSVGGenericContainerFrame),
    { &nsGkAtoms::foreignObject,
      FCDATA_WITH_WRAPPING_BLOCK(FCDATA_DISALLOW_OUT_OF_FLOW,
                                 NS_NewSVGForeignObjectFrame,
                                 nsCSSAnonBoxes::mozSVGForeignContent) },
    SIMPLE_SVG_CREATE(a, NS_NewSVGAFrame),
    SIMPLE_SVG_CREATE(linearGradient, NS_NewSVGLinearGradientFrame),
    SIMPLE_SVG_CREATE(radialGradient, NS_NewSVGRadialGradientFrame),
    SIMPLE_SVG_CREATE(stop, NS_NewSVGStopFrame),
    SIMPLE_SVG_CREATE(use, NS_NewSVGUseFrame),
    SIMPLE_SVG_CREATE(view, NS_NewSVGViewFrame),
    SIMPLE_SVG_CREATE(image, NS_NewSVGImageFrame),
    SIMPLE_SVG_CREATE(clipPath, NS_NewSVGClipPathFrame),
    SIMPLE_SVG_CREATE(filter, NS_NewSVGFilterFrame),
    SIMPLE_SVG_CREATE(pattern, NS_NewSVGPatternFrame),
    SIMPLE_SVG_CREATE(mask, NS_NewSVGMaskFrame),
    SIMPLE_SVG_CREATE(feDistantLight, NS_NewSVGFEUnstyledLeafFrame),
    SIMPLE_SVG_CREATE(fePointLight, NS_NewSVGFEUnstyledLeafFrame),
    SIMPLE_SVG_CREATE(feSpotLight, NS_NewSVGFEUnstyledLeafFrame),
    SIMPLE_SVG_CREATE(feBlend, NS_NewSVGFELeafFrame),
    SIMPLE_SVG_CREATE(feColorMatrix, NS_NewSVGFELeafFrame),
    SIMPLE_SVG_CREATE(feFuncR, NS_NewSVGFEUnstyledLeafFrame),
    SIMPLE_SVG_CREATE(feFuncG, NS_NewSVGFEUnstyledLeafFrame),
    SIMPLE_SVG_CREATE(feFuncB, NS_NewSVGFEUnstyledLeafFrame),
    SIMPLE_SVG_CREATE(feFuncA, NS_NewSVGFEUnstyledLeafFrame),
    SIMPLE_SVG_CREATE(feComposite, NS_NewSVGFELeafFrame),
    SIMPLE_SVG_CREATE(feComponentTransfer, NS_NewSVGFEContainerFrame),
    SIMPLE_SVG_CREATE(feConvolveMatrix, NS_NewSVGFELeafFrame),
    SIMPLE_SVG_CREATE(feDiffuseLighting, NS_NewSVGFEContainerFrame),
    SIMPLE_SVG_CREATE(feDisplacementMap, NS_NewSVGFELeafFrame),
    SIMPLE_SVG_CREATE(feDropShadow, NS_NewSVGFELeafFrame),
    SIMPLE_SVG_CREATE(feFlood, NS_NewSVGFELeafFrame),
    SIMPLE_SVG_CREATE(feGaussianBlur, NS_NewSVGFELeafFrame),
    SIMPLE_SVG_CREATE(feImage, NS_NewSVGFEImageFrame),
    SIMPLE_SVG_CREATE(feMerge, NS_NewSVGFEContainerFrame),
    SIMPLE_SVG_CREATE(feMergeNode, NS_NewSVGFEUnstyledLeafFrame),
    SIMPLE_SVG_CREATE(feMorphology, NS_NewSVGFELeafFrame),
    SIMPLE_SVG_CREATE(feOffset, NS_NewSVGFELeafFrame),
    SIMPLE_SVG_CREATE(feSpecularLighting, NS_NewSVGFEContainerFrame),
    SIMPLE_SVG_CREATE(feTile, NS_NewSVGFELeafFrame),
    SIMPLE_SVG_CREATE(feTurbulence, NS_NewSVGFELeafFrame)
  };

  const FrameConstructionData* data =
    FindDataByTag(aTag, aElement, aStyleContext, sSVGData,
                  ArrayLength(sSVGData));

  if (!data) {
    data = &sContainerData;
  }

  return data;
}

void
nsCSSFrameConstructor::AddPageBreakItem(nsIContent* aContent,
                                        nsStyleContext* aMainStyleContext,
                                        FrameConstructionItemList& aItems)
{
  
  
  
  
  nsRefPtr<nsStyleContext> pseudoStyle =
    mPresShell->StyleSet()->
      ResolveAnonymousBoxStyle(nsCSSAnonBoxes::pageBreak,
                               aMainStyleContext->GetParent());

  NS_ASSERTION(pseudoStyle->StyleDisplay()->mDisplay ==
                 NS_STYLE_DISPLAY_BLOCK, "Unexpected display");

  static const FrameConstructionData sPageBreakData =
    FCDATA_DECL(FCDATA_SKIP_FRAMESET, NS_NewPageBreakFrame);

  
  
  aItems.AppendItem(&sPageBreakData, aContent, nsCSSAnonBoxes::pageBreak,
                    kNameSpaceID_None, nullptr, pseudoStyle.forget(),
                    true, nullptr);
}

bool
nsCSSFrameConstructor::ShouldCreateItemsForChild(nsFrameConstructorState& aState,
                                                 nsIContent* aContent,
                                                 nsContainerFrame* aParentFrame)
{
  aContent->UnsetFlags(NODE_DESCENDANTS_NEED_FRAMES | NODE_NEEDS_FRAME);
  if (aContent->IsElement()) {
    
    
    
    
    
    
    aContent->UnsetFlags(ELEMENT_ALL_RESTYLE_FLAGS &
                         ~ELEMENT_PENDING_RESTYLE_FLAGS);
  }

  
  
  if (aContent->GetPrimaryFrame() &&
      aContent->GetPrimaryFrame()->GetContent() == aContent &&
      !aState.mCreatingExtraFrames) {
    NS_ERROR("asked to create frame construction item for a node that already "
             "has a frame");
    return false;
  }

  
  if (!NeedFrameFor(aState, aParentFrame, aContent)) {
    return false;
  }

  
  if (aContent->IsNodeOfType(nsINode::eCOMMENT) ||
      aContent->IsNodeOfType(nsINode::ePROCESSING_INSTRUCTION)) {
    return false;
  }

  return true;
}

void
nsCSSFrameConstructor::DoAddFrameConstructionItems(nsFrameConstructorState& aState,
                                                   nsIContent* aContent,
                                                   nsStyleContext* aStyleContext,
                                                   bool aSuppressWhiteSpaceOptimizations,
                                                   nsContainerFrame* aParentFrame,
                                                   nsTArray<nsIAnonymousContentCreator::ContentInfo>* aAnonChildren,
                                                   FrameConstructionItemList& aItems)
{
  uint32_t flags = ITEM_ALLOW_XBL_BASE | ITEM_ALLOW_PAGE_BREAK;
  if (aParentFrame) {
    if (aParentFrame->IsSVGText()) {
      flags |= ITEM_IS_WITHIN_SVG_TEXT;
    }
    if (aParentFrame->GetType() == nsGkAtoms::blockFrame &&
        aParentFrame->GetParent() &&
        aParentFrame->GetParent()->GetType() == nsGkAtoms::svgTextFrame) {
      flags |= ITEM_ALLOWS_TEXT_PATH_CHILD;
    }
  }
  AddFrameConstructionItemsInternal(aState, aContent, aParentFrame,
                                    aContent->NodeInfo()->NameAtom(),
                                    aContent->GetNameSpaceID(),
                                    aSuppressWhiteSpaceOptimizations,
                                    aStyleContext,
                                    flags, aAnonChildren,
                                    aItems);
}

void
nsCSSFrameConstructor::AddFrameConstructionItems(nsFrameConstructorState& aState,
                                                 nsIContent* aContent,
                                                 bool aSuppressWhiteSpaceOptimizations,
                                                 const InsertionPoint& aInsertion,
                                                 FrameConstructionItemList& aItems)
{
  nsContainerFrame* parentFrame = aInsertion.mParentFrame;
  if (!ShouldCreateItemsForChild(aState, aContent, parentFrame)) {
    return;
  }
  nsRefPtr<nsStyleContext> styleContext =
    ResolveStyleContext(aInsertion, aContent, &aState);
  DoAddFrameConstructionItems(aState, aContent, styleContext,
                              aSuppressWhiteSpaceOptimizations, parentFrame,
                              nullptr, aItems);
}

void
nsCSSFrameConstructor::SetAsUndisplayedContent(nsFrameConstructorState& aState,
                                               FrameConstructionItemList& aList,
                                               nsIContent* aContent,
                                               nsStyleContext* aStyleContext,
                                               bool aIsGeneratedContent)
{
  if (aStyleContext->GetPseudo()) {
    if (aIsGeneratedContent) {
      aContent->UnbindFromTree();
    }
    return;
  }
  NS_ASSERTION(!aIsGeneratedContent, "Should have had pseudo type");

  if (aState.mCreatingExtraFrames) {
    MOZ_ASSERT(GetUndisplayedContent(aContent),
               "should have called SetUndisplayedContent earlier");
    return;
  }
  aList.AppendUndisplayedItem(aContent, aStyleContext);
}

void
nsCSSFrameConstructor::AddFrameConstructionItemsInternal(nsFrameConstructorState& aState,
                                                         nsIContent* aContent,
                                                         nsContainerFrame* aParentFrame,
                                                         nsIAtom* aTag,
                                                         int32_t aNameSpaceID,
                                                         bool aSuppressWhiteSpaceOptimizations,
                                                         nsStyleContext* aStyleContext,
                                                         uint32_t aFlags,
                                                         nsTArray<nsIAnonymousContentCreator::ContentInfo>* aAnonChildren,
                                                         FrameConstructionItemList& aItems)
{
  NS_PRECONDITION(aContent->IsNodeOfType(nsINode::eTEXT) ||
                  aContent->IsElement(),
                  "Shouldn't get anything else here!");
  MOZ_ASSERT(!aContent->GetPrimaryFrame() || aState.mCreatingExtraFrames ||
             aContent->NodeInfo()->NameAtom() == nsGkAtoms::area);

  
  
  
  const nsStyleDisplay* display = aStyleContext->StyleDisplay();
  nsRefPtr<nsStyleContext> styleContext(aStyleContext);
  PendingBinding* pendingBinding = nullptr;
  if ((aFlags & ITEM_ALLOW_XBL_BASE) && display->mBinding)
  {
    

    nsXBLService* xblService = nsXBLService::GetInstance();
    if (!xblService)
      return;

    bool resolveStyle;

    nsAutoPtr<PendingBinding> newPendingBinding(new PendingBinding());

    nsresult rv = xblService->LoadBindings(aContent, display->mBinding->GetURI(),
                                           display->mBinding->mOriginPrincipal,
                                           getter_AddRefs(newPendingBinding->mBinding),
                                           &resolveStyle);
    if (NS_FAILED(rv) && rv != NS_ERROR_XBL_BLOCKED)
      return;

    if (newPendingBinding->mBinding) {
      pendingBinding = newPendingBinding;
      
      aState.AddPendingBinding(newPendingBinding.forget());
    }

    if (resolveStyle) {
      styleContext =
        ResolveStyleContext(styleContext->GetParent(), aContent, &aState);
      display = styleContext->StyleDisplay();
      aStyleContext = styleContext;
    }

    aTag = mDocument->BindingManager()->ResolveTag(aContent, &aNameSpaceID);
  }

  bool isGeneratedContent = ((aFlags & ITEM_IS_GENERATED_CONTENT) != 0);

  
  
  if (NS_STYLE_DISPLAY_NONE == display->mDisplay) {
    SetAsUndisplayedContent(aState, aItems, aContent, styleContext, isGeneratedContent);
    return;
  }

  bool isText = !aContent->IsElement();

  
  
  
  nsIContent *parent = aContent->GetParent();
  if (parent) {
    
    if (parent->IsAnyOfHTMLElements(nsGkAtoms::select, nsGkAtoms::optgroup) &&
        
        !aContent->IsHTMLElement(nsGkAtoms::option) &&
        
        (!aContent->IsHTMLElement(nsGkAtoms::optgroup) ||
         !parent->IsHTMLElement(nsGkAtoms::select)) &&
        
        !aContent->IsRootOfNativeAnonymousSubtree()) {
      
      if (!isText) {
        SetAsUndisplayedContent(aState, aItems, aContent, styleContext,
                                isGeneratedContent);
      }
      return;
    }
  }

  bool isPopup = false;
  
  const FrameConstructionData* data;
  if (isText) {
    data = FindTextData(aParentFrame);
    if (!data) {
      
      return;
    }
  } else {
    Element* element = aContent->AsElement();

    
    if (aNameSpaceID != kNameSpaceID_SVG &&
        ((aParentFrame &&
          IsFrameForSVG(aParentFrame) &&
          !aParentFrame->IsFrameOfType(nsIFrame::eSVGForeignObject)) ||
         (aFlags & ITEM_IS_WITHIN_SVG_TEXT))) {
      SetAsUndisplayedContent(aState, aItems, element, styleContext,
                              isGeneratedContent);
      return;
    }

    data = FindHTMLData(element, aTag, aNameSpaceID, aParentFrame,
                        styleContext);
    if (!data) {
      data = FindXULTagData(element, aTag, aNameSpaceID, styleContext);
    }
    if (!data) {
      data = FindMathMLData(element, aTag, aNameSpaceID, styleContext);
    }
    if (!data) {
      data = FindSVGData(element, aTag, aNameSpaceID, aParentFrame,
                         aFlags & ITEM_IS_WITHIN_SVG_TEXT,
                         aFlags & ITEM_ALLOWS_TEXT_PATH_CHILD,
                         styleContext);
    }

    
    if (!data) {
      data = FindXULDisplayData(display, element, styleContext);
    }

    
    if (!data) {
      data = FindDisplayData(display, element, aParentFrame, styleContext);
    }

    NS_ASSERTION(data, "Should have frame construction data now");

    if (data->mBits & FCDATA_SUPPRESS_FRAME) {
      SetAsUndisplayedContent(aState, aItems, element, styleContext, isGeneratedContent);
      return;
    }

#ifdef MOZ_XUL
    if ((data->mBits & FCDATA_IS_POPUP) &&
        (!aParentFrame || 
         aParentFrame->GetType() != nsGkAtoms::menuFrame)) {
      if (!aState.mPopupItems.containingBlock &&
          !aState.mHavePendingPopupgroup) {
        SetAsUndisplayedContent(aState, aItems, element, styleContext,
                                isGeneratedContent);
        return;
      }

      isPopup = true;
    }
#endif 
  }

  uint32_t bits = data->mBits;

  
  if (aParentFrame &&
      aParentFrame->GetType() == nsGkAtoms::tableColGroupFrame &&
      (!(bits & FCDATA_IS_TABLE_PART) ||
       display->mDisplay != NS_STYLE_DISPLAY_TABLE_COLUMN)) {
    SetAsUndisplayedContent(aState, aItems, aContent, styleContext, isGeneratedContent);
    return;
  }

  bool canHavePageBreak =
    (aFlags & ITEM_ALLOW_PAGE_BREAK) &&
    aState.mPresContext->IsPaginated() &&
    !display->IsAbsolutelyPositionedStyle() &&
    !(bits & FCDATA_IS_TABLE_PART) &&
    !(bits & FCDATA_IS_SVG_TEXT);

  if (canHavePageBreak && display->mBreakBefore) {
    AddPageBreakItem(aContent, aStyleContext, aItems);
  }

  if (MOZ_UNLIKELY(bits & FCDATA_IS_CONTENTS)) {
    if (!GetDisplayContentsStyleFor(aContent)) {
      MOZ_ASSERT(styleContext->GetPseudo() || !isGeneratedContent,
                 "Should have had pseudo type");
      aState.mFrameManager->SetDisplayContents(aContent, styleContext);
    } else {
      aState.mFrameManager->ChangeDisplayContents(aContent, styleContext);
    }

    TreeMatchContext::AutoAncestorPusher ancestorPusher(aState.mTreeMatchContext);
    if (aState.mTreeMatchContext.mAncestorFilter.HasFilter()) {
      ancestorPusher.PushAncestorAndStyleScope(aContent->AsElement());
    } else {
      ancestorPusher.PushStyleScope(aContent->AsElement());
    }

    if (aParentFrame) {
      aParentFrame->AddStateBits(NS_FRAME_MAY_HAVE_GENERATED_CONTENT);
    }
    CreateGeneratedContentItem(aState, aParentFrame, aContent, styleContext,
                               nsCSSPseudoElements::ePseudo_before, aItems);

    FlattenedChildIterator iter(aContent);
    for (nsIContent* child = iter.GetNextChild(); child; child = iter.GetNextChild()) {
      if (!ShouldCreateItemsForChild(aState, child, aParentFrame)) {
        continue;
      }

      
      
      
      
      
      nsIContent* parent = child->GetParent();
      MOZ_ASSERT(parent, "Parent must be non-null because we are iterating children.");
      TreeMatchContext::AutoAncestorPusher ancestorPusher(aState.mTreeMatchContext);
      if (parent != aContent && parent->IsElement()) {
        if (aState.mTreeMatchContext.mAncestorFilter.HasFilter()) {
          ancestorPusher.PushAncestorAndStyleScope(parent->AsElement());
        } else {
          ancestorPusher.PushStyleScope(parent->AsElement());
        }
      }

      nsRefPtr<nsStyleContext> childContext =
        ResolveStyleContext(styleContext, child, &aState);
      DoAddFrameConstructionItems(aState, child, childContext,
                                  aSuppressWhiteSpaceOptimizations,
                                  aParentFrame, aAnonChildren, aItems);
    }
    aItems.SetParentHasNoXBLChildren(!iter.XBLInvolved());

    CreateGeneratedContentItem(aState, aParentFrame, aContent, styleContext,
                               nsCSSPseudoElements::ePseudo_after, aItems);
    if (canHavePageBreak && display->mBreakAfter) {
      AddPageBreakItem(aContent, aStyleContext, aItems);
    }
    return;
  }

  FrameConstructionItem* item =
    aItems.AppendItem(data, aContent, aTag, aNameSpaceID,
                      pendingBinding, styleContext.forget(),
                      aSuppressWhiteSpaceOptimizations, aAnonChildren);
  item->mIsText = isText;
  item->mIsGeneratedContent = isGeneratedContent;
  item->mIsAnonymousContentCreatorContent =
    aFlags & ITEM_IS_ANONYMOUSCONTENTCREATOR_CONTENT;
  if (isGeneratedContent) {
    NS_ADDREF(item->mContent);
  }
  item->mIsRootPopupgroup =
    aNameSpaceID == kNameSpaceID_XUL && aTag == nsGkAtoms::popupgroup &&
    aContent->IsRootOfNativeAnonymousSubtree();
  if (item->mIsRootPopupgroup) {
    aState.mHavePendingPopupgroup = true;
  }
  item->mIsPopup = isPopup;
  item->mIsForSVGAElement = aNameSpaceID == kNameSpaceID_SVG &&
                            aTag == nsGkAtoms::a;

  if (canHavePageBreak && display->mBreakAfter) {
    AddPageBreakItem(aContent, aStyleContext, aItems);
  }

  if (bits & FCDATA_IS_INLINE) {
    
    
    BuildInlineChildItems(aState, *item,
                          aFlags & ITEM_IS_WITHIN_SVG_TEXT,
                          aFlags & ITEM_ALLOWS_TEXT_PATH_CHILD);
    item->mHasInlineEnds = true;
    item->mIsBlock = false;
  } else {
    
    
    bool isInline =
      
      
      
      ((bits & FCDATA_IS_TABLE_PART) &&
       (!aParentFrame || 
        aParentFrame->StyleDisplay()->mDisplay == NS_STYLE_DISPLAY_INLINE)) ||
      
      display->IsInlineOutsideStyle() ||
      
      isPopup;

    
    
    
    
    item->mIsAllInline = item->mHasInlineEnds = isInline ||
      
      
      
      
      
      
      
      
      
      
      
      
      
      (!(bits & FCDATA_DISALLOW_OUT_OF_FLOW) &&
       aState.GetGeometricParent(display, nullptr));

    
    
    
    
    
    item->mIsBlock = !isInline &&
                     !display->IsAbsolutelyPositionedStyle() &&
                     !display->IsFloatingStyle() &&
                     !(bits & FCDATA_IS_SVG_TEXT);
  }

  if (item->mIsAllInline) {
    aItems.InlineItemAdded();
  } else if (item->mIsBlock) {
    aItems.BlockItemAdded();
  }

  
  
  
  
  if ((bits & FCDATA_IS_LINE_PARTICIPANT) &&
      ((bits & FCDATA_DISALLOW_OUT_OF_FLOW) ||
       !aState.GetGeometricParent(display, nullptr))) {
    item->mIsLineParticipant = true;
    aItems.LineParticipantItemAdded();
  }
}

static void
AddGenConPseudoToFrame(nsIFrame* aOwnerFrame, nsIContent* aContent)
{
  NS_ASSERTION(nsLayoutUtils::IsFirstContinuationOrIBSplitSibling(aOwnerFrame),
               "property should only be set on first continuation/ib-sibling");

  typedef nsAutoTArray<nsIContent*, 2> T;
  const FramePropertyDescriptor* prop = nsIFrame::GenConProperty();
  FrameProperties props = aOwnerFrame->Properties();
  T* value = static_cast<T*>(props.Get(prop));
  if (!value) {
    value = new T;
    props.Set(prop, value);
  }
  value->AppendElement(aContent);
}









bool
nsCSSFrameConstructor::AtLineBoundary(FCItemIterator& aIter)
{
  if (aIter.item().mSuppressWhiteSpaceOptimizations) {
    return false;
  }

  if (aIter.AtStart()) {
    if (aIter.List()->HasLineBoundaryAtStart() &&
        !aIter.item().mContent->GetPreviousSibling())
      return true;
  } else {
    FCItemIterator prev = aIter;
    prev.Prev();
    if (prev.item().IsLineBoundary() &&
        !prev.item().mSuppressWhiteSpaceOptimizations &&
        aIter.item().mContent->GetPreviousSibling() == prev.item().mContent)
      return true;
  }

  FCItemIterator next = aIter;
  next.Next();
  if (next.IsDone()) {
    if (aIter.List()->HasLineBoundaryAtEnd() &&
        !aIter.item().mContent->GetNextSibling())
      return true;
  } else {
    if (next.item().IsLineBoundary() &&
        !next.item().mSuppressWhiteSpaceOptimizations &&
        aIter.item().mContent->GetNextSibling() == next.item().mContent)
      return true;
  }

  return false;
}

void
nsCSSFrameConstructor::ConstructFramesFromItem(nsFrameConstructorState& aState,
                                               FCItemIterator& aIter,
                                               nsContainerFrame* aParentFrame,
                                               nsFrameItems& aFrameItems)
{
  nsContainerFrame* adjParentFrame = aParentFrame;
  FrameConstructionItem& item = aIter.item();
  nsStyleContext* styleContext = item.mStyleContext;
  AdjustParentFrame(&adjParentFrame, item.mFCData, styleContext);

  if (item.mIsText) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (AtLineBoundary(aIter) &&
        !styleContext->StyleText()->WhiteSpaceOrNewlineIsSignificant() &&
        aIter.List()->ParentHasNoXBLChildren() &&
        !(aState.mAdditionalStateBits & NS_FRAME_GENERATED_CONTENT) &&
        (item.mFCData->mBits & FCDATA_IS_LINE_PARTICIPANT) &&
        !(item.mFCData->mBits & FCDATA_IS_SVG_TEXT) &&
        !mAlwaysCreateFramesForIgnorableWhitespace &&
        item.IsWhitespace(aState))
      return;

    ConstructTextFrame(item.mFCData, aState, item.mContent,
                       adjParentFrame, styleContext,
                       aFrameItems);
    return;
  }

  
  
  styleContext->StartBackgroundImageLoads();

  nsFrameState savedStateBits = aState.mAdditionalStateBits;
  if (item.mIsGeneratedContent) {
    
    
    aState.mAdditionalStateBits |= NS_FRAME_GENERATED_CONTENT;

    
    
    
    
    
    ::AddGenConPseudoToFrame(aParentFrame, item.mContent);

    
    
    item.mIsGeneratedContent = false;
  }

  
  ConstructFrameFromItemInternal(item, aState, adjParentFrame, aFrameItems);

  aState.mAdditionalStateBits = savedStateBits;
}


inline bool
IsRootBoxFrame(nsIFrame *aFrame)
{
  return (aFrame->GetType() == nsGkAtoms::rootFrame);
}

nsresult
nsCSSFrameConstructor::ReconstructDocElementHierarchy()
{
  Element* rootElement = mDocument->GetRootElement();
  if (!rootElement) {
    
    return NS_OK;
  }
  return RecreateFramesForContent(rootElement, false, REMOVE_FOR_RECONSTRUCTION,
                                  nullptr);
}

nsContainerFrame*
nsCSSFrameConstructor::GetAbsoluteContainingBlock(nsIFrame* aFrame,
                                                  ContainingBlockType aType)
{
  
  
  for (nsIFrame* frame = aFrame; frame; frame = frame->GetParent()) {
    if (frame->IsFrameOfType(nsIFrame::eMathML)) {
      
      
      
      return nullptr;
    }

    
    if (aType == FIXED_POS) {
      nsIAtom* t = frame->GetType();
      if (t == nsGkAtoms::viewportFrame ||
          t == nsGkAtoms::pageContentFrame) {
        return static_cast<nsContainerFrame*>(frame);
      }
    }

    
    
    
    
    
    
    
    
    if (!frame->IsAbsPosContaininingBlock() ||
        (aType == FIXED_POS &&
         !frame->StyleDisplay()->IsFixedPosContainingBlock(frame))) {
      continue;
    }
    nsIFrame* absPosCBCandidate = frame;
    nsIAtom* type = absPosCBCandidate->GetType();
    if (type == nsGkAtoms::fieldSetFrame) {
      absPosCBCandidate = static_cast<nsFieldSetFrame*>(absPosCBCandidate)->GetInner();
      if (!absPosCBCandidate) {
        continue;
      }
      type = absPosCBCandidate->GetType();
    }
    if (type == nsGkAtoms::scrollFrame) {
      nsIScrollableFrame* scrollFrame = do_QueryFrame(absPosCBCandidate);
      absPosCBCandidate = scrollFrame->GetScrolledFrame();
      if (!absPosCBCandidate) {
        continue;
      }
      type = absPosCBCandidate->GetType();
    }
    
    absPosCBCandidate = absPosCBCandidate->FirstContinuation();
    
    if (!absPosCBCandidate->IsAbsoluteContainer()) {
      continue;
    }

    
    if (type == nsGkAtoms::tableFrame) {
      continue;
    }
    
    MOZ_ASSERT((nsContainerFrame*)do_QueryFrame(absPosCBCandidate),
               "abs.pos. containing block must be nsContainerFrame sub-class");
    return static_cast<nsContainerFrame*>(absPosCBCandidate);
  }

  MOZ_ASSERT(aType != FIXED_POS, "no ICB in this frame tree?");

  
  
  
  return mHasRootAbsPosContainingBlock ? mDocElementContainingBlock : nullptr;
}

nsContainerFrame*
nsCSSFrameConstructor::GetFloatContainingBlock(nsIFrame* aFrame)
{
  
  
  
  
  for (nsIFrame* containingBlock = aFrame;
       containingBlock &&
         !ShouldSuppressFloatingOfDescendants(containingBlock);
       containingBlock = containingBlock->GetParent()) {
    if (containingBlock->IsFloatContainingBlock()) {
      MOZ_ASSERT((nsContainerFrame*)do_QueryFrame(containingBlock),
                 "float containing block must be nsContainerFrame sub-class");
      return static_cast<nsContainerFrame*>(containingBlock);
    }
  }

  
  
  return nullptr;
}







static nsContainerFrame*
AdjustAppendParentForAfterContent(nsFrameManager* aFrameManager,
                                  nsIContent* aContainer,
                                  nsContainerFrame* aParentFrame,
                                  nsIContent* aChild,
                                  nsIFrame** aAfterFrame)
{
  
  
  
  
  
  if (aParentFrame->GetGenConPseudos() ||
      nsLayoutUtils::HasPseudoStyle(aContainer, aParentFrame->StyleContext(),
                                    nsCSSPseudoElements::ePseudo_after,
                                    aParentFrame->PresContext()) ||
      aFrameManager->GetDisplayContentsStyleFor(aContainer)) {
    nsIFrame* afterFrame = nullptr;
    nsContainerFrame* parent =
      static_cast<nsContainerFrame*>(aParentFrame->LastContinuation());
    bool done = false;
    while (!done && parent) {
      
      parent->DrainSelfOverflowList();

      nsIFrame* child = parent->GetLastChild(nsIFrame::kPrincipalList);
      if (child && child->IsPseudoFrame(aContainer) &&
          !child->IsGeneratedContentFrame()) {
        
        nsContainerFrame* childAsContainer = do_QueryFrame(child);
        if (childAsContainer) {
          parent = nsLayoutUtils::LastContinuationWithChild(childAsContainer);
          continue;
        }
      }

      for (; child; child = child->GetPrevSibling()) {
        nsIContent* c = child->GetContent();
        if (child->IsGeneratedContentFrame()) {
          nsIContent* p = c->GetParent();
          if (c->NodeInfo()->NameAtom() == nsGkAtoms::mozgeneratedcontentafter) {
            if (!nsContentUtils::ContentIsDescendantOf(aChild, p) &&
                p != aContainer &&
                nsContentUtils::PositionIsBefore(p, aChild)) {
              
              
              
              
              
              
              done = true;
              break;
            }
          } else if (nsContentUtils::PositionIsBefore(p, aChild)) {
            
            done = true;
            break;
          }
        } else if (nsContentUtils::PositionIsBefore(c, aChild)) {
          
          done = true;
          break;
        }
        afterFrame = child;
      }

      parent = static_cast<nsContainerFrame*>(parent->GetPrevContinuation());
    }
    if (afterFrame) {
      *aAfterFrame = afterFrame;
      return afterFrame->GetParent();
    }
  }

  *aAfterFrame = nullptr;

  if (IsFramePartOfIBSplit(aParentFrame)) {
    
    
    
    
    
    nsContainerFrame* trailingInline = GetIBSplitSibling(aParentFrame);
    if (trailingInline) {
      aParentFrame = trailingInline;
    }

    
    
    
    
    
    
    aParentFrame =
      static_cast<nsContainerFrame*>(aParentFrame->LastContinuation());
  }

  return aParentFrame;
}






static nsIFrame*
FindAppendPrevSibling(nsIFrame* aParentFrame, nsIFrame* aAfterFrame)
{
  if (aAfterFrame) {
    NS_ASSERTION(aAfterFrame->GetParent() == aParentFrame, "Wrong parent");
    NS_ASSERTION(aAfterFrame->GetPrevSibling() ||
                 aParentFrame->GetFirstPrincipalChild() == aAfterFrame,
                 ":after frame must be on the principal child list here");
    return aAfterFrame->GetPrevSibling();
  }

  aParentFrame->DrainSelfOverflowList();

  return aParentFrame->GetLastChild(kPrincipalList);
}





static nsIFrame*
GetInsertNextSibling(nsIFrame* aParentFrame, nsIFrame* aPrevSibling)
{
  if (aPrevSibling) {
    return aPrevSibling->GetNextSibling();
  }

  return aParentFrame->GetFirstPrincipalChild();
}






nsresult
nsCSSFrameConstructor::AppendFramesToParent(nsFrameConstructorState&       aState,
                                            nsContainerFrame*              aParentFrame,
                                            nsFrameItems&                  aFrameList,
                                            nsIFrame*                      aPrevSibling,
                                            bool                           aIsRecursiveCall)
{
  NS_PRECONDITION(!IsFramePartOfIBSplit(aParentFrame) ||
                  !GetIBSplitSibling(aParentFrame) ||
                  !GetIBSplitSibling(aParentFrame)->GetFirstPrincipalChild(),
                  "aParentFrame has a ib-split sibling with kids?");
  NS_PRECONDITION(!aPrevSibling || aPrevSibling->GetParent() == aParentFrame,
                  "Parent and prevsibling don't match");

  nsIFrame* nextSibling = ::GetInsertNextSibling(aParentFrame, aPrevSibling);

  NS_ASSERTION(nextSibling ||
               !aParentFrame->GetNextContinuation() ||
               !aParentFrame->GetNextContinuation()->GetFirstPrincipalChild() ||
               aIsRecursiveCall,
               "aParentFrame has later continuations with kids?");
  NS_ASSERTION(nextSibling ||
               !IsFramePartOfIBSplit(aParentFrame) ||
               (IsInlineFrame(aParentFrame) &&
                !GetIBSplitSibling(aParentFrame) &&
                !aParentFrame->GetNextContinuation()) ||
               aIsRecursiveCall,
               "aParentFrame is not last?");

  
  
  
  if (!nextSibling && IsFramePartOfIBSplit(aParentFrame)) {
    
    
    
    
    
    
    
    if (aFrameList.NotEmpty() && !aFrameList.FirstChild()->IsInlineOutside()) {
      
      nsIFrame* firstContinuation = aParentFrame->FirstContinuation();
      if (firstContinuation->PrincipalChildList().IsEmpty()) {
        
        
        nsFrameList::FrameLinkEnumerator firstNonBlockEnumerator =
          FindFirstNonBlock(aFrameList);
        nsFrameList blockKids = aFrameList.ExtractHead(firstNonBlockEnumerator);
        NS_ASSERTION(blockKids.NotEmpty(), "No blocks?");

        nsContainerFrame* prevBlock = GetIBSplitPrevSibling(firstContinuation);
        prevBlock = static_cast<nsContainerFrame*>(prevBlock->LastContinuation());
        NS_ASSERTION(prevBlock, "Should have previous block here");

        MoveChildrenTo(aState.mPresContext, aParentFrame, prevBlock, blockKids);
      }
    }

    
    nsFrameList::FrameLinkEnumerator firstBlockEnumerator(aFrameList);
    FindFirstBlock(firstBlockEnumerator);

    nsFrameList inlineKids = aFrameList.ExtractHead(firstBlockEnumerator);
    if (!inlineKids.IsEmpty()) {
      AppendFrames(aParentFrame, kPrincipalList, inlineKids);
    }

    if (!aFrameList.IsEmpty()) {
      bool positioned = aParentFrame->IsRelativelyPositioned();
      nsFrameItems ibSiblings;
      CreateIBSiblings(aState, aParentFrame, positioned, aFrameList,
                       ibSiblings);

      
      
      
      mPresShell->FrameNeedsReflow(aParentFrame,
                                   nsIPresShell::eTreeChange,
                                   NS_FRAME_HAS_DIRTY_CHILDREN);

      
      return AppendFramesToParent(aState, aParentFrame->GetParent(), ibSiblings,
                                  aParentFrame, true);
    }

    return NS_OK;
  }

  
  InsertFrames(aParentFrame, kPrincipalList, aPrevSibling, aFrameList);
  return NS_OK;
}

#define UNSET_DISPLAY 255






bool
nsCSSFrameConstructor::IsValidSibling(nsIFrame*              aSibling,
                                      nsIContent*            aContent,
                                      uint8_t&               aDisplay)
{
  nsIFrame* parentFrame = aSibling->GetParent();
  nsIAtom* parentType = nullptr;
  if (parentFrame) {
    parentType = parentFrame->GetType();
  }

  uint8_t siblingDisplay = aSibling->GetDisplay();
  if ((NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP == siblingDisplay) ||
      (NS_STYLE_DISPLAY_TABLE_COLUMN       == siblingDisplay) ||
      (NS_STYLE_DISPLAY_TABLE_CAPTION      == siblingDisplay) ||
      (NS_STYLE_DISPLAY_TABLE_HEADER_GROUP == siblingDisplay) ||
      (NS_STYLE_DISPLAY_TABLE_ROW_GROUP    == siblingDisplay) ||
      (NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP == siblingDisplay) ||
      nsGkAtoms::menuFrame == parentType) {
    
    if (UNSET_DISPLAY == aDisplay) {
      nsIFrame* styleParent;
      aSibling->GetParentStyleContext(&styleParent);
      if (!styleParent) {
        styleParent = aSibling->GetParent();
      }
      if (!styleParent) {
        NS_NOTREACHED("Shouldn't happen");
        return false;
      }
      
      
      nsRefPtr<nsStyleContext> styleContext =
        ResolveStyleContext(styleParent, aContent, nullptr);
      const nsStyleDisplay* display = styleContext->StyleDisplay();
      aDisplay = display->mDisplay;
    }
    if (nsGkAtoms::menuFrame == parentType) {
      return
        (NS_STYLE_DISPLAY_POPUP == aDisplay) ==
        (NS_STYLE_DISPLAY_POPUP == siblingDisplay);
    }
    
    
    
    
    
    
    
    
    
    
    
    
    if ((siblingDisplay == NS_STYLE_DISPLAY_TABLE_CAPTION) !=
        (aDisplay == NS_STYLE_DISPLAY_TABLE_CAPTION)) {
      
      return false;
    }

    if ((siblingDisplay == NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP ||
         siblingDisplay == NS_STYLE_DISPLAY_TABLE_COLUMN) !=
        (aDisplay == NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP ||
         aDisplay == NS_STYLE_DISPLAY_TABLE_COLUMN)) {
      
      
      return false;
    }
    
    
    
  }

  if (IsFrameForFieldSet(parentFrame, parentType)) {
    
    if (nsContainerFrame* cif = aSibling->GetContentInsertionFrame()) {
      aSibling = cif;
    }
    nsIAtom* sibType = aSibling->GetType();
    bool legendContent = aContent->IsHTMLElement(nsGkAtoms::legend);

    if ((legendContent  && (nsGkAtoms::legendFrame != sibType)) ||
        (!legendContent && (nsGkAtoms::legendFrame == sibType)))
      return false;
  }

  return true;
}

nsIFrame*
nsCSSFrameConstructor::FindFrameForContentSibling(nsIContent* aContent,
                                                  nsIContent* aTargetContent,
                                                  uint8_t& aTargetContentDisplay,
                                                  nsContainerFrame* aParentFrame,
                                                  bool aPrevSibling)
{
  nsIFrame* sibling = aContent->GetPrimaryFrame();
  if (!sibling && GetDisplayContentsStyleFor(aContent)) {
    
    sibling = aPrevSibling ?
      nsLayoutUtils::GetAfterFrameForContent(aParentFrame, aContent) :
      nsLayoutUtils::GetBeforeFrameForContent(aParentFrame, aContent);
    if (!sibling) {
      
      const bool forward = !aPrevSibling;
      FlattenedChildIterator iter(aContent, forward);
      sibling = aPrevSibling ?
        FindPreviousSibling(iter, aTargetContent, aTargetContentDisplay, aParentFrame) :
        FindNextSibling(iter, aTargetContent, aTargetContentDisplay, aParentFrame);
    }
    if (!sibling) {
      
      sibling = aPrevSibling ?
        nsLayoutUtils::GetBeforeFrameForContent(aParentFrame, aContent) :
        nsLayoutUtils::GetAfterFrameForContent(aParentFrame, aContent);
    }
    if (!sibling) {
      return nullptr;
    }
  } else if (!sibling || sibling->GetContent() != aContent) {
    
    
    return nullptr;
  }

  
  
  if (sibling->GetStateBits() & NS_FRAME_OUT_OF_FLOW) {
    nsIFrame* placeholderFrame = GetPlaceholderFrameFor(sibling);
    NS_ASSERTION(placeholderFrame, "no placeholder for out-of-flow frame");
    sibling = placeholderFrame;
  }

  
  NS_ASSERTION(!sibling->GetPrevContinuation(), "How did that happen?");

  if (aPrevSibling) {
    
    
    if (IsFramePartOfIBSplit(sibling)) {
      sibling = GetLastIBSplitSibling(sibling, true);
    }

    
    
    sibling = sibling->GetTailContinuation();
  }

  if (aTargetContent &&
      !IsValidSibling(sibling, aTargetContent, aTargetContentDisplay)) {
    sibling = nullptr;
  }

  return sibling;
}

nsIFrame*
nsCSSFrameConstructor::FindPreviousSibling(FlattenedChildIterator aIter,
                                           nsIContent* aTargetContent,
                                           uint8_t& aTargetContentDisplay,
                                           nsContainerFrame* aParentFrame)
{
  
  
  while (nsIContent* sibling = aIter.GetPreviousChild()) {
    MOZ_ASSERT(sibling != aTargetContent);
    nsIFrame* prevSibling =
      FindFrameForContentSibling(sibling, aTargetContent, aTargetContentDisplay,
                                 aParentFrame, true);
    if (prevSibling) {
      
      return prevSibling;
    }
  }

  return nullptr;
}

nsIFrame*
nsCSSFrameConstructor::FindNextSibling(FlattenedChildIterator aIter,
                                       nsIContent* aTargetContent,
                                       uint8_t& aTargetContentDisplay,
                                       nsContainerFrame* aParentFrame)
{
  while (nsIContent* sibling = aIter.GetNextChild()) {
    MOZ_ASSERT(sibling != aTargetContent);
    nsIFrame* nextSibling =
      FindFrameForContentSibling(sibling, aTargetContent, aTargetContentDisplay,
                                 aParentFrame, false);

    if (nextSibling) {
      
      return nextSibling;
    }
  }

  return nullptr;
}


static nsContainerFrame*
GetAdjustedParentFrame(nsContainerFrame* aParentFrame,
                       nsIAtom*          aParentFrameType,
                       nsIContent*       aChildContent)
{
  NS_PRECONDITION(nsGkAtoms::tableOuterFrame != aParentFrameType,
                  "Shouldn't be happening!");

  nsContainerFrame* newParent = nullptr;

  if (nsGkAtoms::fieldSetFrame == aParentFrameType) {
    
    
    if (!aChildContent->IsHTMLElement(nsGkAtoms::legend)) {
      newParent = GetFieldSetBlockFrame(aParentFrame);
    }
  }
  return newParent ? newParent : aParentFrame;
}

nsIFrame*
nsCSSFrameConstructor::GetInsertionPrevSibling(InsertionPoint* aInsertion,
                                               nsIContent* aChild,
                                               bool*       aIsAppend,
                                               bool*       aIsRangeInsertSafe,
                                               nsIContent* aStartSkipChild,
                                               nsIContent* aEndSkipChild)
{
  NS_PRECONDITION(aInsertion->mParentFrame, "Must have parent frame to start with");

  *aIsAppend = false;

  
  
  
  
  FlattenedChildIterator iter(aInsertion->mContainer);
  bool xblCase = iter.XBLInvolved() ||
         aInsertion->mParentFrame->GetContent() != aInsertion->mContainer;
  if (xblCase || !aChild->IsRootOfAnonymousSubtree()) {
    
    
    
    if (aStartSkipChild) {
      iter.Seek(aStartSkipChild);
    } else {
      iter.Seek(aChild);
    }
  } else {
    
    iter.GetNextChild();
    NS_WARNING("Someone passed native anonymous content directly into frame "
               "construction.  Stop doing that!");
  }

  
  
  uint8_t childDisplay = UNSET_DISPLAY;
  nsIFrame* prevSibling =
    FindPreviousSibling(iter, iter.Get(), childDisplay, aInsertion->mParentFrame);

  
  
  
  if (prevSibling) {
    aInsertion->mParentFrame = prevSibling->GetParent()->GetContentInsertionFrame();
  } else {
    
    if (aEndSkipChild) {
      iter.Seek(aEndSkipChild);
      iter.GetPreviousChild();
    }
    nsIFrame* nextSibling =
      FindNextSibling(iter, iter.Get(), childDisplay, aInsertion->mParentFrame);
    if (GetDisplayContentsStyleFor(aInsertion->mContainer)) {
      if (!nextSibling) {
        
        
        
        
        nsIContent* child = aInsertion->mContainer;
        nsIContent* parent = child->GetParent();
        aInsertion->mParentFrame =
          ::GetAdjustedParentFrame(aInsertion->mParentFrame,
                                   aInsertion->mParentFrame->GetType(),
                                   parent);
        InsertionPoint fakeInsertion(aInsertion->mParentFrame, parent);
        nsIFrame* result = GetInsertionPrevSibling(&fakeInsertion, child, aIsAppend,
                                                   aIsRangeInsertSafe, nullptr, nullptr);
        MOZ_ASSERT(aInsertion->mParentFrame == fakeInsertion.mParentFrame);
        return result;
      }

      prevSibling = nextSibling->GetPrevSibling();
    }

    if (nextSibling) {
      aInsertion->mParentFrame = nextSibling->GetParent()->GetContentInsertionFrame();
    } else {
      
      *aIsAppend = true;
      if (IsFramePartOfIBSplit(aInsertion->mParentFrame)) {
        
        
        
        aInsertion->mParentFrame =
          GetLastIBSplitSibling(aInsertion->mParentFrame, false);
      }
      
      
      aInsertion->mParentFrame =
        nsLayoutUtils::LastContinuationWithChild(aInsertion->mParentFrame);
      
      aInsertion->mParentFrame =
        ::GetAdjustedParentFrame(aInsertion->mParentFrame,
                                 aInsertion->mParentFrame->GetType(),
                                 aChild);
      nsIFrame* appendAfterFrame;
      aInsertion->mParentFrame =
        ::AdjustAppendParentForAfterContent(this, aInsertion->mContainer,
                                            aInsertion->mParentFrame,
                                            aChild, &appendAfterFrame);
      prevSibling = ::FindAppendPrevSibling(aInsertion->mParentFrame, appendAfterFrame);
    }
  }

  *aIsRangeInsertSafe = (childDisplay == UNSET_DISPLAY);
  return prevSibling;
}

nsContainerFrame*
nsCSSFrameConstructor::GetContentInsertionFrameFor(nsIContent* aContent)
{
  
  nsIFrame* frame = aContent->GetPrimaryFrame();

  if (!frame) {
    if (GetDisplayContentsStyleFor(aContent)) {
      nsIContent* parent = aContent->GetParent();
      if (parent && parent == aContent->GetContainingShadow()) {
        parent = parent->GetBindingParent();
      }
      frame = parent ? GetContentInsertionFrameFor(parent) : nullptr;
    }
    if (!frame) {
      return nullptr;
    }
  } else {
    
    
    
    if (frame->GetContent() != aContent) {
      return nullptr;
    }
  }

  nsContainerFrame* insertionFrame = frame->GetContentInsertionFrame();

  NS_ASSERTION(!insertionFrame || insertionFrame == frame || !frame->IsLeaf(),
    "The insertion frame is the primary frame or the primary frame isn't a leaf");

  return insertionFrame;
}

static bool
IsSpecialFramesetChild(nsIContent* aContent)
{
  
  return aContent->IsAnyOfHTMLElements(nsGkAtoms::frameset, nsGkAtoms::frame);
}

static void
InvalidateCanvasIfNeeded(nsIPresShell* presShell, nsIContent* node);

#ifdef MOZ_XUL

static
bool
IsXULListBox(nsIContent* aContainer)
{
  return (aContainer->IsXULElement(nsGkAtoms::listbox));
}

static
nsListBoxBodyFrame*
MaybeGetListBoxBodyFrame(nsIContent* aContainer, nsIContent* aChild)
{
  if (!aContainer)
    return nullptr;

  if (IsXULListBox(aContainer) &&
      aChild->IsXULElement(nsGkAtoms::listitem)) {
    nsCOMPtr<nsIDOMXULElement> xulElement = do_QueryInterface(aContainer);
    nsCOMPtr<nsIBoxObject> boxObject;
    xulElement->GetBoxObject(getter_AddRefs(boxObject));
    nsCOMPtr<nsPIListBoxObject> listBoxObject = do_QueryInterface(boxObject);
    if (listBoxObject) {
      return listBoxObject->GetListBoxBody(false);
    }
  }

  return nullptr;
}
#endif

void
nsCSSFrameConstructor::AddTextItemIfNeeded(nsFrameConstructorState& aState,
                                           const InsertionPoint& aInsertion,
                                           nsIContent* aPossibleTextContent,
                                           FrameConstructionItemList& aItems)
{
  NS_PRECONDITION(aPossibleTextContent, "Must have node");
  if (!aPossibleTextContent->IsNodeOfType(nsINode::eTEXT) ||
      !aPossibleTextContent->HasFlag(NS_CREATE_FRAME_IF_NON_WHITESPACE)) {
    
    
    
    return;
  }
  NS_ASSERTION(!aPossibleTextContent->GetPrimaryFrame(),
               "Text node has a frame and NS_CREATE_FRAME_IF_NON_WHITESPACE");
  AddFrameConstructionItems(aState, aPossibleTextContent, false,
                            aInsertion, aItems);
}

void
nsCSSFrameConstructor::ReframeTextIfNeeded(nsIContent* aParentContent,
                                           nsIContent* aContent)
{
  if (!aContent->IsNodeOfType(nsINode::eTEXT) ||
      !aContent->HasFlag(NS_CREATE_FRAME_IF_NON_WHITESPACE)) {
    
    
    
    return;
  }
  NS_ASSERTION(!aContent->GetPrimaryFrame(),
               "Text node has a frame and NS_CREATE_FRAME_IF_NON_WHITESPACE");
  ContentInserted(aParentContent, aContent, nullptr, false);
}



bool
nsCSSFrameConstructor::MaybeConstructLazily(Operation aOperation,
                                            nsIContent* aContainer,
                                            nsIContent* aChild)
{
  if (mPresShell->GetPresContext()->IsChrome() || !aContainer ||
      aContainer->IsInNativeAnonymousSubtree() || aContainer->IsXULElement()) {
    return false;
  }

  if (aOperation == CONTENTINSERT) {
    if (aChild->IsRootOfAnonymousSubtree() ||
        (aChild->HasFlag(NODE_IS_IN_SHADOW_TREE) &&
         !aChild->IsInNativeAnonymousSubtree()) ||
        aChild->IsEditable() || aChild->IsXULElement()) {
      return false;
    }
  } else { 
    NS_ASSERTION(aOperation == CONTENTAPPEND,
                 "operation should be either insert or append");
    for (nsIContent* child = aChild; child; child = child->GetNextSibling()) {
      NS_ASSERTION(!child->IsRootOfAnonymousSubtree(),
                   "Should be coming through the CONTENTAPPEND case");
      if (child->IsXULElement() || child->IsEditable()) {
        return false;
      }
    }
  }

  
  

  
  nsIContent* content = aContainer;
#ifdef DEBUG
  
  
  
  
  
  bool noPrimaryFrame = false;
  bool needsFrameBitSet = false;
#endif
  while (content &&
         !content->HasFlag(NODE_DESCENDANTS_NEED_FRAMES)) {
#ifdef DEBUG
    if (content->GetPrimaryFrame() && content->GetPrimaryFrame()->IsLeaf()) {
      noPrimaryFrame = needsFrameBitSet = false;
    }
    if (!noPrimaryFrame && !content->GetPrimaryFrame()) {
      noPrimaryFrame = true;
    }
    if (!needsFrameBitSet && content->HasFlag(NODE_NEEDS_FRAME)) {
      needsFrameBitSet = true;
    }
#endif
    
    if (GetDisplayContentsStyleFor(content)) {
      return false;
    }
    content->SetFlags(NODE_DESCENDANTS_NEED_FRAMES);
    content = content->GetFlattenedTreeParent();
  }
#ifdef DEBUG
  if (content && content->GetPrimaryFrame() &&
      content->GetPrimaryFrame()->IsLeaf()) {
    noPrimaryFrame = needsFrameBitSet = false;
  }
  NS_ASSERTION(!noPrimaryFrame, "Ancestors of nodes with frames to be "
    "constructed lazily should have frames");
  NS_ASSERTION(!needsFrameBitSet, "Ancestors of nodes with frames to be "
    "constructed lazily should not have NEEDS_FRAME bit set");
#endif

  
  if (aOperation == CONTENTINSERT) {
    NS_ASSERTION(!aChild->GetPrimaryFrame() ||
                 aChild->GetPrimaryFrame()->GetContent() != aChild,
                 
                 
                 
                 "setting NEEDS_FRAME on a node that already has a frame?");
    aChild->SetFlags(NODE_NEEDS_FRAME);
  } else { 
    for (nsIContent* child = aChild; child; child = child->GetNextSibling()) {
      NS_ASSERTION(!child->GetPrimaryFrame() ||
                   child->GetPrimaryFrame()->GetContent() != child,
                   
                   
                   
                   "setting NEEDS_FRAME on a node that already has a frame?");
      child->SetFlags(NODE_NEEDS_FRAME);
    }
  }

  RestyleManager()->PostRestyleEventForLazyConstruction();
  return true;
}

void
nsCSSFrameConstructor::CreateNeededFrames(nsIContent* aContent)
{
  NS_ASSERTION(!aContent->HasFlag(NODE_NEEDS_FRAME),
    "shouldn't get here with a content node that has needs frame bit set");
  NS_ASSERTION(aContent->HasFlag(NODE_DESCENDANTS_NEED_FRAMES),
    "should only get here with a content node that has descendants needing frames");

  aContent->UnsetFlags(NODE_DESCENDANTS_NEED_FRAMES);

  
  
  

  
  
  
  

  
  
  uint32_t childCount = aContent->GetChildCount();
  bool inRun = false;
  nsIContent* firstChildInRun = nullptr;
  for (uint32_t i = 0; i < childCount; i++) {
    nsIContent* child = aContent->GetChildAt(i);
    if (child->HasFlag(NODE_NEEDS_FRAME)) {
      NS_ASSERTION(!child->GetPrimaryFrame() ||
                   child->GetPrimaryFrame()->GetContent() != child,
                   
                   
                   
                   "NEEDS_FRAME set on a node that already has a frame?");
      if (!inRun) {
        inRun = true;
        firstChildInRun = child;
      }
    } else {
      if (inRun) {
        inRun = false;
        
        ContentRangeInserted(aContent, firstChildInRun, child, nullptr,
                             false);
      }
    }
  }
  if (inRun) {
    ContentAppended(aContent, firstChildInRun, false);
  }

  
  FlattenedChildIterator iter(aContent);
  for (nsIContent* child = iter.GetNextChild(); child; child = iter.GetNextChild()) {
    if (child->HasFlag(NODE_DESCENDANTS_NEED_FRAMES)) {
      CreateNeededFrames(child);
    }
  }
}

void nsCSSFrameConstructor::CreateNeededFrames()
{
  NS_ASSERTION(!nsContentUtils::IsSafeToRunScript(),
               "Someone forgot a script blocker");

  Element* rootElement = mDocument->GetRootElement();
  NS_ASSERTION(!rootElement || !rootElement->HasFlag(NODE_NEEDS_FRAME),
    "root element should not have frame created lazily");
  if (rootElement && rootElement->HasFlag(NODE_DESCENDANTS_NEED_FRAMES)) {
    BeginUpdate();
    CreateNeededFrames(rootElement);
    EndUpdate();
  }
}

void
nsCSSFrameConstructor::IssueSingleInsertNofications(nsIContent* aContainer,
                                                    nsIContent* aStartChild,
                                                    nsIContent* aEndChild,
                                                    bool aAllowLazyConstruction)
{
  for (nsIContent* child = aStartChild;
       child != aEndChild;
       child = child->GetNextSibling()) {
    if ((child->GetPrimaryFrame() || GetUndisplayedContent(child) ||
         GetDisplayContentsStyleFor(child))
#ifdef MOZ_XUL
        
        
        && !MaybeGetListBoxBodyFrame(aContainer, child)
#endif
        ) {
      
      
      
      continue;
    }
    
    ContentInserted(aContainer, child, mTempFrameTreeState,
                    aAllowLazyConstruction);
  }
}

nsCSSFrameConstructor::InsertionPoint
nsCSSFrameConstructor::GetRangeInsertionPoint(nsIContent* aContainer,
                                              nsIContent* aStartChild,
                                              nsIContent* aEndChild,
                                              bool aAllowLazyConstruction)
{
  
  
  
  InsertionPoint insertionPoint = GetInsertionPoint(aContainer, nullptr);
  if (!insertionPoint.mParentFrame && !insertionPoint.mMultiple) {
    return insertionPoint; 
  }

  bool hasInsertion = false;
  if (!insertionPoint.mMultiple) {
    
    nsIDocument* document = aStartChild->GetComposedDoc();
    
    if (document && aStartChild->GetXBLInsertionParent()) {
      hasInsertion = true;
    }
  }

  if (insertionPoint.mMultiple || hasInsertion) {
    
    
    uint32_t childCount = 0;

    if (!insertionPoint.mMultiple) {
      
      
      
      
      
      
      
      
      
      
      
      
      childCount = insertionPoint.mParentFrame->GetContent()->GetChildCount();
    }

    
    
    
    if (insertionPoint.mMultiple || aEndChild != nullptr || childCount > 0) {
      
      
      
      IssueSingleInsertNofications(aContainer, aStartChild, aEndChild,
                                   aAllowLazyConstruction);
      insertionPoint.mParentFrame = nullptr;
    }
  }

  return insertionPoint;
}

bool
nsCSSFrameConstructor::MaybeRecreateForFrameset(nsIFrame* aParentFrame,
                                                nsIContent* aStartChild,
                                                nsIContent* aEndChild)
{
  if (aParentFrame->GetType() == nsGkAtoms::frameSetFrame) {
    
    for (nsIContent* cur = aStartChild;
         cur != aEndChild;
         cur = cur->GetNextSibling()) {
      if (IsSpecialFramesetChild(cur)) {
        
        RecreateFramesForContent(aParentFrame->GetContent(), false,
                                 REMOVE_FOR_RECONSTRUCTION, nullptr);
        return true;
      }
    }
  }
  return false;
}

nsresult
nsCSSFrameConstructor::ContentAppended(nsIContent*     aContainer,
                                       nsIContent*     aFirstNewContent,
                                       bool            aAllowLazyConstruction)
{
  AUTO_LAYOUT_PHASE_ENTRY_POINT(mPresShell->GetPresContext(), FrameC);
  NS_PRECONDITION(mUpdateCount != 0,
                  "Should be in an update while creating frames");

#ifdef DEBUG
  if (gNoisyContentUpdates) {
    printf("nsCSSFrameConstructor::ContentAppended container=%p "
           "first-child=%p lazy=%d\n",
           static_cast<void*>(aContainer), aFirstNewContent,
           aAllowLazyConstruction);
    if (gReallyNoisyContentUpdates && aContainer) {
      aContainer->List(stdout, 0);
    }
  }
#endif

#ifdef DEBUG
  for (nsIContent* child = aFirstNewContent;
       child;
       child = child->GetNextSibling()) {
    
    
    NS_ASSERTION(!child->GetPrimaryFrame() ||
                 child->GetPrimaryFrame()->GetContent() != child,
                 "asked to construct a frame for a node that already has a frame");
  }
#endif

#ifdef MOZ_XUL
  if (aContainer) {
    int32_t namespaceID;
    nsIAtom* tag =
      mDocument->BindingManager()->ResolveTag(aContainer, &namespaceID);

    
    if (tag == nsGkAtoms::treechildren ||
        tag == nsGkAtoms::treeitem ||
        tag == nsGkAtoms::treerow)
      return NS_OK;

  }
#endif 

  if (aContainer && aContainer->HasFlag(NODE_IS_IN_SHADOW_TREE) &&
      !aContainer->IsInNativeAnonymousSubtree() &&
      !aFirstNewContent->IsInNativeAnonymousSubtree()) {
    
    
    
    
    nsIContent* bindingParent = aContainer->GetBindingParent();
    LAYOUT_PHASE_TEMP_EXIT();
    nsresult rv = RecreateFramesForContent(bindingParent, false,
                                           REMOVE_FOR_RECONSTRUCTION, nullptr);
    LAYOUT_PHASE_TEMP_REENTER();
    return rv;
  }

  
  if (!GetContentInsertionFrameFor(aContainer) &&
      !aContainer->IsActiveChildrenElement()) {
    return NS_OK;
  }

  if (aAllowLazyConstruction &&
      MaybeConstructLazily(CONTENTAPPEND, aContainer, aFirstNewContent)) {
    return NS_OK;
  }

  LAYOUT_PHASE_TEMP_EXIT();
  InsertionPoint insertion =
    GetRangeInsertionPoint(aContainer, aFirstNewContent, nullptr,
                           aAllowLazyConstruction);
  nsContainerFrame*& parentFrame = insertion.mParentFrame;
  LAYOUT_PHASE_TEMP_REENTER();
  if (!parentFrame) {
    return NS_OK;
  }

  LAYOUT_PHASE_TEMP_EXIT();
  if (MaybeRecreateForFrameset(parentFrame, aFirstNewContent, nullptr)) {
    LAYOUT_PHASE_TEMP_REENTER();
    return NS_OK;
  }
  LAYOUT_PHASE_TEMP_REENTER();

  if (parentFrame->IsLeaf()) {
    
    
    ClearLazyBits(aFirstNewContent, nullptr);
    return NS_OK;
  }

  if (parentFrame->IsFrameOfType(nsIFrame::eMathML)) {
    LAYOUT_PHASE_TEMP_EXIT();
    nsresult rv = RecreateFramesForContent(parentFrame->GetContent(), false,
                                           REMOVE_FOR_RECONSTRUCTION, nullptr);
    LAYOUT_PHASE_TEMP_REENTER();
    return rv;
  }

  
  
  
  bool parentIBSplit = IsFramePartOfIBSplit(parentFrame);
  if (parentIBSplit) {
#ifdef DEBUG
    if (gNoisyContentUpdates) {
      printf("nsCSSFrameConstructor::ContentAppended: parentFrame=");
      nsFrame::ListTag(stdout, parentFrame);
      printf(" is ib-split\n");
    }
#endif

    
    
    
    parentFrame = GetLastIBSplitSibling(parentFrame, false);
  }

  
  
  parentFrame = nsLayoutUtils::LastContinuationWithChild(parentFrame);

  
  
  NS_ASSERTION(parentFrame->GetType() != nsGkAtoms::fieldSetFrame,
               "Unexpected parent");

  
  nsIFrame* parentAfterFrame;
  parentFrame =
    ::AdjustAppendParentForAfterContent(this, insertion.mContainer, parentFrame,
                                        aFirstNewContent, &parentAfterFrame);

  
  nsFrameConstructorState state(mPresShell,
                                GetAbsoluteContainingBlock(parentFrame, FIXED_POS),
                                GetAbsoluteContainingBlock(parentFrame, ABS_POS),
                                GetFloatContainingBlock(parentFrame));
  state.mTreeMatchContext.InitAncestors(aContainer->AsElement());

  
  bool haveFirstLetterStyle = false, haveFirstLineStyle = false;
  nsContainerFrame* containingBlock = state.mFloatedItems.containingBlock;
  if (containingBlock) {
    haveFirstLetterStyle = HasFirstLetterStyle(containingBlock);
    haveFirstLineStyle =
      ShouldHaveFirstLineStyle(containingBlock->GetContent(),
                               containingBlock->StyleContext());
  }

  if (haveFirstLetterStyle) {
    
    RemoveLetterFrames(state.mPresContext, state.mPresShell,
                       containingBlock);
  }

  nsIAtom* frameType = parentFrame->GetType();

  FlattenedChildIterator iter(aContainer);
  bool haveNoXBLChildren = (!iter.XBLInvolved() || !iter.GetNextChild());
  FrameConstructionItemList items;
  if (aFirstNewContent->GetPreviousSibling() &&
      GetParentType(frameType) == eTypeBlock &&
      haveNoXBLChildren) {
    
    
    
    
    
    
    
    
    
    
    
    AddTextItemIfNeeded(state, insertion,
                        aFirstNewContent->GetPreviousSibling(), items);
  }
  for (nsIContent* child = aFirstNewContent;
       child;
       child = child->GetNextSibling()) {
    AddFrameConstructionItems(state, child, false, insertion, items);
  }

  nsIFrame* prevSibling = ::FindAppendPrevSibling(parentFrame, parentAfterFrame);

  
  
  
  
  LAYOUT_PHASE_TEMP_EXIT();
  if (WipeContainingBlock(state, containingBlock, parentFrame, items,
                          true, prevSibling)) {
    LAYOUT_PHASE_TEMP_REENTER();
    return NS_OK;
  }
  LAYOUT_PHASE_TEMP_REENTER();

  
  
  
  if (nsLayoutUtils::GetAsBlock(parentFrame) && !haveFirstLetterStyle &&
      !haveFirstLineStyle && !parentIBSplit) {
    items.SetLineBoundaryAtStart(!prevSibling ||
        !prevSibling->IsInlineOutside() ||
        prevSibling->GetType() == nsGkAtoms::brFrame);
    
    items.SetLineBoundaryAtEnd(!parentAfterFrame ||
        !parentAfterFrame->IsInlineOutside());
  }
  
  
  items.SetParentHasNoXBLChildren(haveNoXBLChildren);

  nsFrameItems frameItems;
  ConstructFramesFromItemList(state, items, parentFrame, frameItems);

  for (nsIContent* child = aFirstNewContent;
       child;
       child = child->GetNextSibling()) {
    
    
    
    
    
    InvalidateCanvasIfNeeded(mPresShell, child);
  }

  
  
  nsFrameItems captionItems;
  if (nsGkAtoms::tableFrame == frameType) {
    
    
    
    
    
    
    PullOutCaptionFrames(frameItems, captionItems);
  }

  if (haveFirstLineStyle && parentFrame == containingBlock) {
    
    
    AppendFirstLineFrames(state, containingBlock->GetContent(),
                          containingBlock, frameItems);
  }

  
  
  
  if (captionItems.NotEmpty()) { 
    NS_ASSERTION(nsGkAtoms::tableFrame == frameType, "how did that happen?");
    nsContainerFrame* outerTable = parentFrame->GetParent();
    AppendFrames(outerTable, nsIFrame::kCaptionList, captionItems);
  }

  if (frameItems.NotEmpty()) { 
    AppendFramesToParent(state, parentFrame, frameItems, prevSibling);
  }

  
  if (haveFirstLetterStyle) {
    RecoverLetterFrames(containingBlock);
  }

#ifdef DEBUG
  if (gReallyNoisyContentUpdates) {
    printf("nsCSSFrameConstructor::ContentAppended: resulting frame model:\n");
    parentFrame->List(stdout, 0);
  }
#endif

#ifdef ACCESSIBILITY
  nsAccessibilityService* accService = nsIPresShell::AccService();
  if (accService) {
    accService->ContentRangeInserted(mPresShell, aContainer,
                                     aFirstNewContent, nullptr);
  }
#endif

  return NS_OK;
}

#ifdef MOZ_XUL

enum content_operation
{
    CONTENT_INSERTED,
    CONTENT_REMOVED
};



static
bool NotifyListBoxBody(nsPresContext*    aPresContext,
                         nsIContent*        aContainer,
                         nsIContent*        aChild,
                         
                         nsIContent*        aOldNextSibling,
                         nsIDocument*       aDocument,
                         nsIFrame*          aChildFrame,
                         content_operation  aOperation)
{
  nsListBoxBodyFrame* listBoxBodyFrame =
    MaybeGetListBoxBodyFrame(aContainer, aChild);
  if (listBoxBodyFrame) {
    if (aOperation == CONTENT_REMOVED) {
      
      
      if (!aChildFrame || aChildFrame->GetParent() == listBoxBodyFrame) {
        listBoxBodyFrame->OnContentRemoved(aPresContext, aContainer,
                                           aChildFrame, aOldNextSibling);
        return true;
      }
    } else {
      listBoxBodyFrame->OnContentInserted(aPresContext, aChild);
      return true;
    }
  }

  return false;
}
#endif 

nsresult
nsCSSFrameConstructor::ContentInserted(nsIContent*            aContainer,
                                       nsIContent*            aChild,
                                       nsILayoutHistoryState* aFrameState,
                                       bool                   aAllowLazyConstruction)
{
  return ContentRangeInserted(aContainer,
                              aChild,
                              aChild->GetNextSibling(),
                              aFrameState,
                              aAllowLazyConstruction);
}



















nsresult
nsCSSFrameConstructor::ContentRangeInserted(nsIContent*            aContainer,
                                            nsIContent*            aStartChild,
                                            nsIContent*            aEndChild,
                                            nsILayoutHistoryState* aFrameState,
                                            bool                   aAllowLazyConstruction)
{
  AUTO_LAYOUT_PHASE_ENTRY_POINT(mPresShell->GetPresContext(), FrameC);
  NS_PRECONDITION(mUpdateCount != 0,
                  "Should be in an update while creating frames");

  NS_PRECONDITION(aStartChild, "must always pass a child");

  
  
#ifdef DEBUG
  if (gNoisyContentUpdates) {
    printf("nsCSSFrameConstructor::ContentRangeInserted container=%p "
           "start-child=%p end-child=%p lazy=%d\n",
           static_cast<void*>(aContainer),
           static_cast<void*>(aStartChild), static_cast<void*>(aEndChild),
           aAllowLazyConstruction);
    if (gReallyNoisyContentUpdates) {
      if (aContainer) {
        aContainer->List(stdout,0);
      } else {
        aStartChild->List(stdout, 0);
      }
    }
  }
#endif

#ifdef DEBUG
  for (nsIContent* child = aStartChild;
       child != aEndChild;
       child = child->GetNextSibling()) {
    
    
    NS_ASSERTION(!child->GetPrimaryFrame() ||
                 child->GetPrimaryFrame()->GetContent() != child,
                 "asked to construct a frame for a node that already has a frame");
  }
#endif

  bool isSingleInsert = (aStartChild->GetNextSibling() == aEndChild);
  NS_ASSERTION(isSingleInsert || !aAllowLazyConstruction,
               "range insert shouldn't be lazy");
  NS_ASSERTION(isSingleInsert || aEndChild,
               "range should not include all nodes after aStartChild");

#ifdef MOZ_XUL
  if (aContainer && IsXULListBox(aContainer)) {
    if (isSingleInsert) {
      if (NotifyListBoxBody(mPresShell->GetPresContext(), aContainer,
                            
                            
                            aStartChild, nullptr,
                            mDocument, nullptr, CONTENT_INSERTED)) {
        return NS_OK;
      }
    } else {
      
      
      LAYOUT_PHASE_TEMP_EXIT();
      IssueSingleInsertNofications(aContainer, aStartChild, aEndChild,
                                   aAllowLazyConstruction);
      LAYOUT_PHASE_TEMP_REENTER();
      return NS_OK;
    }
  }
#endif 

  
  
  
  if (! aContainer) {
    NS_ASSERTION(isSingleInsert,
                 "root node insertion should be a single insertion");
    Element *docElement = mDocument->GetRootElement();

    if (aStartChild != docElement) {
      
      return NS_OK;
    }

    NS_PRECONDITION(nullptr == mRootElementFrame,
                    "root element frame already created");

    
    nsIFrame* docElementFrame =
      ConstructDocElementFrame(docElement, aFrameState);

    if (docElementFrame) {
      InvalidateCanvasIfNeeded(mPresShell, aStartChild);
#ifdef DEBUG
      if (gReallyNoisyContentUpdates) {
        printf("nsCSSFrameConstructor::ContentRangeInserted: resulting frame "
               "model:\n");
        docElementFrame->List(stdout, 0);
      }
#endif
    }

    if (aFrameState) {
      
      nsIFrame* rootScrollFrame = mPresShell->GetRootScrollFrame();
      if (rootScrollFrame) {
        RestoreFrameStateFor(rootScrollFrame, aFrameState);
      }
    }

#ifdef ACCESSIBILITY
    nsAccessibilityService* accService = nsIPresShell::AccService();
    if (accService) {
      accService->ContentRangeInserted(mPresShell, aContainer,
                                       aStartChild, aEndChild);
    }
#endif

    return NS_OK;
  }

  if (aContainer->HasFlag(NODE_IS_IN_SHADOW_TREE) &&
      !aContainer->IsInNativeAnonymousSubtree() &&
      (!aStartChild || !aStartChild->IsInNativeAnonymousSubtree()) &&
      (!aEndChild || !aEndChild->IsInNativeAnonymousSubtree())) {
    
    
    
    
    nsIContent* bindingParent = aContainer->GetBindingParent();
    LAYOUT_PHASE_TEMP_EXIT();
    nsresult rv = RecreateFramesForContent(bindingParent, false,
                                           REMOVE_FOR_RECONSTRUCTION, nullptr);
    LAYOUT_PHASE_TEMP_REENTER();
    return rv;
  }

  
  
  {
    nsContainerFrame* parentFrame = GetContentInsertionFrameFor(aContainer);
    
    
    
    if (!parentFrame && !aContainer->IsActiveChildrenElement()) {
      return NS_OK;
    }

    
    NS_ASSERTION(!parentFrame || parentFrame->GetContent() == aContainer ||
                 GetDisplayContentsStyleFor(aContainer), "New XBL code is possibly wrong!");

    if (aAllowLazyConstruction &&
        MaybeConstructLazily(CONTENTINSERT, aContainer, aStartChild)) {
      return NS_OK;
    }
  }

  InsertionPoint insertion;
  if (isSingleInsert) {
    
    
    
    insertion = GetInsertionPoint(aContainer, aStartChild);
  } else {
    
    
    LAYOUT_PHASE_TEMP_EXIT();
    insertion = GetRangeInsertionPoint(aContainer, aStartChild, aEndChild,
                                       aAllowLazyConstruction);
    LAYOUT_PHASE_TEMP_REENTER();
  }

  if (!insertion.mParentFrame) {
    return NS_OK;
  }

  bool isAppend, isRangeInsertSafe;
  nsIFrame* prevSibling = GetInsertionPrevSibling(&insertion, aStartChild,
                                                  &isAppend, &isRangeInsertSafe);

  
  if (!isSingleInsert && !isRangeInsertSafe) {
    
    LAYOUT_PHASE_TEMP_EXIT();
    IssueSingleInsertNofications(aContainer, aStartChild, aEndChild,
                                 aAllowLazyConstruction);
    LAYOUT_PHASE_TEMP_REENTER();
    return NS_OK;
  }

  nsIContent* container = insertion.mParentFrame->GetContent();

  nsIAtom* frameType = insertion.mParentFrame->GetType();
  LAYOUT_PHASE_TEMP_EXIT();
  if (MaybeRecreateForFrameset(insertion.mParentFrame, aStartChild, aEndChild)) {
    LAYOUT_PHASE_TEMP_REENTER();
    return NS_OK;
  }
  LAYOUT_PHASE_TEMP_REENTER();

  
  
  NS_ASSERTION(isSingleInsert || frameType != nsGkAtoms::fieldSetFrame,
               "Unexpected parent");
  if (IsFrameForFieldSet(insertion.mParentFrame, frameType) &&
      aStartChild->NodeInfo()->NameAtom() == nsGkAtoms::legend) {
    
    
    
    
    
    
    LAYOUT_PHASE_TEMP_EXIT();
    nsresult rv = RecreateFramesForContent(insertion.mParentFrame->GetContent(), false,
                                           REMOVE_FOR_RECONSTRUCTION, nullptr);
    LAYOUT_PHASE_TEMP_REENTER();
    return rv;
  }

  
  if (insertion.mParentFrame->IsLeaf()) {
    
    ClearLazyBits(aStartChild, aEndChild);
    return NS_OK;
  }

  if (insertion.mParentFrame->IsFrameOfType(nsIFrame::eMathML)) {
    LAYOUT_PHASE_TEMP_EXIT();
    nsresult rv = RecreateFramesForContent(insertion.mParentFrame->GetContent(), false,
                                           REMOVE_FOR_RECONSTRUCTION, nullptr);
    LAYOUT_PHASE_TEMP_REENTER();
    return rv;
  }

  nsFrameConstructorState state(mPresShell,
                                GetAbsoluteContainingBlock(insertion.mParentFrame, FIXED_POS),
                                GetAbsoluteContainingBlock(insertion.mParentFrame, ABS_POS),
                                GetFloatContainingBlock(insertion.mParentFrame),
                                aFrameState);
  state.mTreeMatchContext.InitAncestors(aContainer ?
                                          aContainer->AsElement() :
                                          nullptr);

  
  
  
  
  
  nsContainerFrame* containingBlock = state.mFloatedItems.containingBlock;
  bool haveFirstLetterStyle = false;
  bool haveFirstLineStyle = false;

  
  
  
  
  uint8_t parentDisplay = insertion.mParentFrame->GetDisplay();

  
  
  
  if ((NS_STYLE_DISPLAY_BLOCK == parentDisplay) ||
      (NS_STYLE_DISPLAY_LIST_ITEM == parentDisplay) ||
      (NS_STYLE_DISPLAY_INLINE == parentDisplay) ||
      (NS_STYLE_DISPLAY_INLINE_BLOCK == parentDisplay)) {
    
    if (containingBlock) {
      haveFirstLetterStyle = HasFirstLetterStyle(containingBlock);
      haveFirstLineStyle =
        ShouldHaveFirstLineStyle(containingBlock->GetContent(),
                                 containingBlock->StyleContext());
    }

    if (haveFirstLetterStyle) {
      
      
      if (insertion.mParentFrame->GetType() == nsGkAtoms::letterFrame) {
        
        
        if (insertion.mParentFrame->GetStateBits() & NS_FRAME_OUT_OF_FLOW) {
          nsPlaceholderFrame* placeholderFrame =
            GetPlaceholderFrameFor(insertion.mParentFrame);
          NS_ASSERTION(placeholderFrame, "No placeholder for out-of-flow?");
          insertion.mParentFrame = placeholderFrame->GetParent();
        } else {
          insertion.mParentFrame = insertion.mParentFrame->GetParent();
        }
      }

      
      RemoveLetterFrames(state.mPresContext, mPresShell,
                         state.mFloatedItems.containingBlock);

      
      
      
      prevSibling = GetInsertionPrevSibling(&insertion, aStartChild, &isAppend,
                                            &isRangeInsertSafe);

      
      if (!isSingleInsert && !isRangeInsertSafe) {
        
        RecoverLetterFrames(state.mFloatedItems.containingBlock);

        
        LAYOUT_PHASE_TEMP_EXIT();
        IssueSingleInsertNofications(aContainer, aStartChild, aEndChild,
                                     aAllowLazyConstruction);
        LAYOUT_PHASE_TEMP_REENTER();
        return NS_OK;
      }

      container = insertion.mParentFrame->GetContent();
      frameType = insertion.mParentFrame->GetType();
    }
  }

  if (!prevSibling) {
    
    
    nsIFrame* firstChild = insertion.mParentFrame->GetFirstPrincipalChild();

    if (firstChild &&
        nsLayoutUtils::IsGeneratedContentFor(container, firstChild,
                                             nsCSSPseudoElements::before)) {
      
      prevSibling = firstChild->GetTailContinuation();
      insertion.mParentFrame = prevSibling->GetParent()->GetContentInsertionFrame();
      
      
    }
  }

  FrameConstructionItemList items;
  ParentType parentType = GetParentType(frameType);
  FlattenedChildIterator iter(aContainer);
  bool haveNoXBLChildren = (!iter.XBLInvolved() || !iter.GetNextChild());
  if (aStartChild->GetPreviousSibling() &&
      parentType == eTypeBlock && haveNoXBLChildren) {
    
    
    
    
    
    AddTextItemIfNeeded(state, insertion, aStartChild->GetPreviousSibling(),
                        items);
  }

  if (isSingleInsert) {
    AddFrameConstructionItems(state, aStartChild,
                              aStartChild->IsRootOfAnonymousSubtree(),
                              insertion, items);
  } else {
    for (nsIContent* child = aStartChild;
         child != aEndChild;
         child = child->GetNextSibling()){
      AddFrameConstructionItems(state, child, false, insertion, items);
    }
  }

  if (aEndChild && parentType == eTypeBlock && haveNoXBLChildren) {
    
    
    
    
    
    AddTextItemIfNeeded(state, insertion, aEndChild, items);
  }

  
  
  
  
  LAYOUT_PHASE_TEMP_EXIT();
  if (WipeContainingBlock(state, containingBlock, insertion.mParentFrame, items,
                          isAppend, prevSibling)) {
    LAYOUT_PHASE_TEMP_REENTER();
    return NS_OK;
  }
  LAYOUT_PHASE_TEMP_REENTER();

  
  
  
  
  nsFrameItems frameItems, captionItems;
  ConstructFramesFromItemList(state, items, insertion.mParentFrame, frameItems);

  if (frameItems.NotEmpty()) {
    for (nsIContent* child = aStartChild;
         child != aEndChild;
         child = child->GetNextSibling()){
      InvalidateCanvasIfNeeded(mPresShell, child);
    }

    if (nsGkAtoms::tableFrame == frameType ||
        nsGkAtoms::tableOuterFrame == frameType) {
      PullOutCaptionFrames(frameItems, captionItems);
    }
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  if (prevSibling && frameItems.NotEmpty() &&
      frameItems.FirstChild()->GetParent() != prevSibling->GetParent()) {
#ifdef DEBUG
    nsIFrame* frame1 = frameItems.FirstChild()->GetParent();
    nsIFrame* frame2 = prevSibling->GetParent();
    NS_ASSERTION(!IsFramePartOfIBSplit(frame1) &&
                 !IsFramePartOfIBSplit(frame2),
                 "Neither should be ib-split");
    NS_ASSERTION((frame1->GetType() == nsGkAtoms::tableFrame &&
                  frame2->GetType() == nsGkAtoms::tableOuterFrame) ||
                 (frame1->GetType() == nsGkAtoms::tableOuterFrame &&
                  frame2->GetType() == nsGkAtoms::tableFrame) ||
                 frame1->GetType() == nsGkAtoms::fieldSetFrame ||
                 (frame1->GetParent() &&
                  frame1->GetParent()->GetType() == nsGkAtoms::fieldSetFrame),
                 "Unexpected frame types");
#endif
    isAppend = true;
    nsIFrame* appendAfterFrame;
    insertion.mParentFrame =
      ::AdjustAppendParentForAfterContent(this, container,
                                          frameItems.FirstChild()->GetParent(),
                                          aStartChild, &appendAfterFrame);
    prevSibling = ::FindAppendPrevSibling(insertion.mParentFrame, appendAfterFrame);
  }

  if (haveFirstLineStyle && insertion.mParentFrame == containingBlock) {
    
    
    if (isAppend) {
      
      AppendFirstLineFrames(state, containingBlock->GetContent(),
                            containingBlock, frameItems);
    }
    else {
      
      
      
      
      InsertFirstLineFrames(state, container, containingBlock, &insertion.mParentFrame,
                            prevSibling, frameItems);
    }
  }

  
  
  if (captionItems.NotEmpty()) {
    NS_ASSERTION(nsGkAtoms::tableFrame == frameType ||
                 nsGkAtoms::tableOuterFrame == frameType,
                 "parent for caption is not table?");
    
    
    
    bool captionIsAppend;
    nsIFrame* captionPrevSibling = nullptr;

    
    bool ignored;
    InsertionPoint captionInsertion(insertion.mParentFrame, insertion.mContainer);
    if (isSingleInsert) {
      captionPrevSibling =
        GetInsertionPrevSibling(&captionInsertion, aStartChild,
                                &captionIsAppend, &ignored);
    } else {
      nsIContent* firstCaption = captionItems.FirstChild()->GetContent();
      
      
      
      captionPrevSibling =
        GetInsertionPrevSibling(&captionInsertion, firstCaption,
                                &captionIsAppend, &ignored,
                                aStartChild, aEndChild);
    }

    nsContainerFrame* outerTable = nullptr;
    if (GetCaptionAdjustedParent(captionInsertion.mParentFrame,
                                 captionItems.FirstChild(),
                                 &outerTable)) {
      
      
      
      NS_ASSERTION(nsGkAtoms::tableOuterFrame == outerTable->GetType(),
                   "Pseudo frame construction failure; "
                   "a caption can be only a child of an outer table frame");

      
      
      
      if (captionPrevSibling &&
          captionPrevSibling->GetParent() != outerTable) {
          captionPrevSibling = nullptr;
      }
      if (captionIsAppend) {
        AppendFrames(outerTable, nsIFrame::kCaptionList, captionItems);
      } else {
        InsertFrames(outerTable, nsIFrame::kCaptionList,
                     captionPrevSibling, captionItems);
      }
    }
  }

  if (frameItems.NotEmpty()) {
    
    if (isAppend) {
      AppendFramesToParent(state, insertion.mParentFrame, frameItems, prevSibling);
    } else {
      InsertFrames(insertion.mParentFrame, kPrincipalList, prevSibling, frameItems);
    }
  }

  if (haveFirstLetterStyle) {
    
    
    RecoverLetterFrames(state.mFloatedItems.containingBlock);
  }

#ifdef DEBUG
  if (gReallyNoisyContentUpdates && insertion.mParentFrame) {
    printf("nsCSSFrameConstructor::ContentRangeInserted: resulting frame model:\n");
    insertion.mParentFrame->List(stdout, 0);
  }
#endif

#ifdef ACCESSIBILITY
  nsAccessibilityService* accService = nsIPresShell::AccService();
  if (accService) {
    accService->ContentRangeInserted(mPresShell, aContainer,
                                     aStartChild, aEndChild);
  }
#endif

  return NS_OK;
}

nsresult
nsCSSFrameConstructor::ContentRemoved(nsIContent*  aContainer,
                                      nsIContent*  aChild,
                                      nsIContent*  aOldNextSibling,
                                      RemoveFlags  aFlags,
                                      bool*        aDidReconstruct,
                                      nsIContent** aDestroyedFramesFor)
{
  AUTO_LAYOUT_PHASE_ENTRY_POINT(mPresShell->GetPresContext(), FrameC);
  NS_PRECONDITION(mUpdateCount != 0,
                  "Should be in an update while destroying frames");

  *aDidReconstruct = false;
  if (aDestroyedFramesFor) {
    *aDestroyedFramesFor = aChild;
  }

  
  

#ifdef DEBUG
  if (gNoisyContentUpdates) {
    printf("nsCSSFrameConstructor::ContentRemoved container=%p child=%p "
           "old-next-sibling=%p\n",
           static_cast<void*>(aContainer),
           static_cast<void*>(aChild),
           static_cast<void*>(aOldNextSibling));
    if (gReallyNoisyContentUpdates) {
      aContainer->List(stdout, 0);
    }
  }
#endif

  nsresult rv = NS_OK;
  nsIFrame* childFrame = aChild->GetPrimaryFrame();
  if (!childFrame || childFrame->GetContent() != aChild) {
    
    
    ClearUndisplayedContentIn(aChild, aContainer);
  }
  MOZ_ASSERT(!childFrame || !GetDisplayContentsStyleFor(aChild),
             "display:contents nodes shouldn't have a frame");
  if (!childFrame && GetDisplayContentsStyleFor(aChild)) {
    nsIFrame* ancestorFrame = nullptr;
    nsIContent* ancestor = aContainer;
    for (; ancestor; ancestor = ancestor->GetParent()) {
      ancestorFrame = ancestor->GetPrimaryFrame();
      if (ancestorFrame) {
        break;
      }
    }
    if (ancestorFrame) {
      nsTArray<nsIContent*>* generated = ancestorFrame->GetGenConPseudos();
      if (generated) {
        *aDidReconstruct = true;
        LAYOUT_PHASE_TEMP_EXIT();
        
        
        RecreateFramesForContent(ancestor, false, aFlags, aDestroyedFramesFor);
        LAYOUT_PHASE_TEMP_REENTER();
        return NS_OK;
      }
    }

    FlattenedChildIterator iter(aChild);
    for (nsIContent* c = iter.GetNextChild(); c; c = iter.GetNextChild()) {
      if (c->GetPrimaryFrame() || GetDisplayContentsStyleFor(c)) {
        LAYOUT_PHASE_TEMP_EXIT();
        rv = ContentRemoved(aChild, c, nullptr, aFlags, aDidReconstruct, aDestroyedFramesFor);
        LAYOUT_PHASE_TEMP_REENTER();
        NS_ENSURE_SUCCESS(rv, rv);
        if (aFlags != REMOVE_DESTROY_FRAMES && *aDidReconstruct) {
          return rv;
        }
      }
    }
    ClearDisplayContentsIn(aChild, aContainer);
  }

  nsPresContext* presContext = mPresShell->GetPresContext();
#ifdef MOZ_XUL
  if (NotifyListBoxBody(presContext, aContainer, aChild, aOldNextSibling,
                        mDocument, childFrame, CONTENT_REMOVED)) {
    if (aFlags == REMOVE_DESTROY_FRAMES) {
      CaptureStateForFramesOf(aChild, mTempFrameTreeState);
    }
    return NS_OK;
  }

#endif 

  
  
  
  
  
  
  
  
  
  
  bool isRoot = false;
  if (!aContainer) {
    nsIFrame* viewport = GetRootFrame();
    if (viewport) {
      nsIFrame* firstChild = viewport->GetFirstPrincipalChild();
      if (firstChild && firstChild->GetContent() == aChild) {
        isRoot = true;
        childFrame = firstChild;
        NS_ASSERTION(!childFrame->GetNextSibling(), "How did that happen?");
      }
    }
  }

  if (aContainer && aContainer->HasFlag(NODE_IS_IN_SHADOW_TREE) &&
      !aContainer->IsInNativeAnonymousSubtree() &&
      !aChild->IsInNativeAnonymousSubtree()) {
    
    
    
    
    nsIContent* bindingParent = aContainer->GetBindingParent();
    *aDidReconstruct = true;
    LAYOUT_PHASE_TEMP_EXIT();
    nsresult rv = RecreateFramesForContent(bindingParent, false,
                                           aFlags, aDestroyedFramesFor);
    LAYOUT_PHASE_TEMP_REENTER();
    return rv;
  }

  if (aFlags == REMOVE_DESTROY_FRAMES) {
    CaptureStateForFramesOf(aChild, mTempFrameTreeState);
  }

  if (childFrame) {
    InvalidateCanvasIfNeeded(mPresShell, aChild);

    
    LAYOUT_PHASE_TEMP_EXIT();
    nsIContent* container;
    if (MaybeRecreateContainerForFrameRemoval(childFrame, aFlags, &rv, &container)) {
      LAYOUT_PHASE_TEMP_REENTER();
      MOZ_ASSERT(container);
      *aDidReconstruct = true;
      if (aDestroyedFramesFor) {
        *aDestroyedFramesFor = container;
      }
      return rv;
    }
    LAYOUT_PHASE_TEMP_REENTER();

    
    nsIFrame* parentFrame = childFrame->GetParent();
    nsIAtom* parentType = parentFrame->GetType();

    if (parentType == nsGkAtoms::frameSetFrame &&
        IsSpecialFramesetChild(aChild)) {
      
      *aDidReconstruct = true;
      LAYOUT_PHASE_TEMP_EXIT();
      nsresult rv = RecreateFramesForContent(parentFrame->GetContent(), false,
                                             aFlags, aDestroyedFramesFor);
      LAYOUT_PHASE_TEMP_REENTER();
      return rv;
    }

    
    
    
    nsIFrame* possibleMathMLAncestor = parentType == nsGkAtoms::blockFrame ?
         parentFrame->GetParent() : parentFrame;
    if (possibleMathMLAncestor->IsFrameOfType(nsIFrame::eMathML)) {
      *aDidReconstruct = true;
      LAYOUT_PHASE_TEMP_EXIT();
      nsresult rv = RecreateFramesForContent(possibleMathMLAncestor->GetContent(),
                                             false, aFlags, aDestroyedFramesFor);
      LAYOUT_PHASE_TEMP_REENTER();
      return rv;
    }

    
    
    
    nsIFrame* grandparentFrame = parentFrame->GetParent();
    if (grandparentFrame && grandparentFrame->IsBoxFrame() &&
        (grandparentFrame->GetStateBits() & NS_STATE_BOX_WRAPS_KIDS_IN_BLOCK) &&
        
        aChild == AnyKidsNeedBlockParent(parentFrame->GetFirstPrincipalChild()) &&
        !AnyKidsNeedBlockParent(childFrame->GetNextSibling())) {
      *aDidReconstruct = true;
      LAYOUT_PHASE_TEMP_EXIT();
      nsresult rv = RecreateFramesForContent(grandparentFrame->GetContent(), true,
                                             aFlags, aDestroyedFramesFor);
      LAYOUT_PHASE_TEMP_REENTER();
      return rv;
    }

#ifdef ACCESSIBILITY
    nsAccessibilityService* accService = nsIPresShell::AccService();
    if (accService) {
      accService->ContentRemoved(mPresShell, aChild);
    }
#endif

    
    
    nsIFrame* inflowChild = childFrame;
    if (childFrame->GetStateBits() & NS_FRAME_OUT_OF_FLOW) {
      inflowChild = GetPlaceholderFrameFor(childFrame);
      NS_ASSERTION(inflowChild, "No placeholder for out-of-flow?");
    }
    nsContainerFrame* containingBlock =
      GetFloatContainingBlock(inflowChild->GetParent());
    bool haveFLS = containingBlock && HasFirstLetterStyle(containingBlock);
    if (haveFLS) {
      
      
#ifdef NOISY_FIRST_LETTER
      printf("ContentRemoved: containingBlock=");
      nsFrame::ListTag(stdout, containingBlock);
      printf(" parentFrame=");
      nsFrame::ListTag(stdout, parentFrame);
      printf(" childFrame=");
      nsFrame::ListTag(stdout, childFrame);
      printf("\n");
#endif

      
      
      
      RemoveLetterFrames(presContext, mPresShell, containingBlock);

      
      childFrame = aChild->GetPrimaryFrame();
      if (!childFrame || childFrame->GetContent() != aChild) {
        
        
        ClearUndisplayedContentIn(aChild, aContainer);
        return NS_OK;
      }
      parentFrame = childFrame->GetParent();
      parentType = parentFrame->GetType();

#ifdef NOISY_FIRST_LETTER
      printf("  ==> revised parentFrame=");
      nsFrame::ListTag(stdout, parentFrame);
      printf(" childFrame=");
      nsFrame::ListTag(stdout, childFrame);
      printf("\n");
#endif
    }

#ifdef DEBUG
    if (gReallyNoisyContentUpdates) {
      printf("nsCSSFrameConstructor::ContentRemoved: childFrame=");
      nsFrame::ListTag(stdout, childFrame);
      putchar('\n');
      parentFrame->List(stdout, 0);
    }
#endif


    
    if (childFrame->GetStateBits() & NS_FRAME_OUT_OF_FLOW) {
      childFrame = GetPlaceholderFrameFor(childFrame);
      NS_ASSERTION(childFrame, "Missing placeholder frame for out of flow.");
      parentFrame = childFrame->GetParent();
    }
    RemoveFrame(nsLayoutUtils::GetChildListNameFor(childFrame), childFrame);

    if (isRoot) {
      mRootElementFrame = nullptr;
      mRootElementStyleFrame = nullptr;
      mDocElementContainingBlock = nullptr;
      mPageSequenceFrame = nullptr;
      mGfxScrollFrame = nullptr;
      mHasRootAbsPosContainingBlock = false;
    }

    if (haveFLS && mRootElementFrame) {
      RecoverLetterFrames(containingBlock);
    }

    
    
    
    
    
    
    if (aContainer && !aChild->IsRootOfAnonymousSubtree() &&
        aFlags == REMOVE_CONTENT &&
        GetParentType(parentType) == eTypeBlock) {
      
      
      
      
      
      
      
      
      
      
      if (aOldNextSibling) {
        nsIContent* prevSibling = aOldNextSibling->GetPreviousSibling();
        if (prevSibling && prevSibling->GetPreviousSibling()) {
          LAYOUT_PHASE_TEMP_EXIT();
          ReframeTextIfNeeded(aContainer, prevSibling);
          LAYOUT_PHASE_TEMP_REENTER();
        }
      }
      
      
      if (aOldNextSibling && aOldNextSibling->GetNextSibling() &&
          aOldNextSibling->GetPreviousSibling()) {
        LAYOUT_PHASE_TEMP_EXIT();
        ReframeTextIfNeeded(aContainer, aOldNextSibling);
        LAYOUT_PHASE_TEMP_REENTER();
      }
    }

#ifdef DEBUG
    if (gReallyNoisyContentUpdates && parentFrame) {
      printf("nsCSSFrameConstructor::ContentRemoved: resulting frame model:\n");
      parentFrame->List(stdout, 0);
    }
#endif
  }

  return rv;
}









static void
InvalidateCanvasIfNeeded(nsIPresShell* presShell, nsIContent* node)
{
  NS_PRECONDITION(presShell->GetRootFrame(), "What happened here?");
  NS_PRECONDITION(presShell->GetPresContext(), "Say what?");

  
  

  nsIContent* parent = node->GetParent();
  if (parent) {
    
    nsIContent* grandParent = parent->GetParent();
    if (grandParent) {
      
      return;
    }

    
    if (!node->IsHTMLElement(nsGkAtoms::body)) {
      return;
    }
  }

  
  
  
  

  nsIFrame* rootFrame = presShell->GetRootFrame();
  rootFrame->InvalidateFrameSubtree();
}

nsIFrame*
nsCSSFrameConstructor::EnsureFrameForTextNode(nsGenericDOMDataNode* aContent)
{
  if (aContent->HasFlag(NS_CREATE_FRAME_IF_NON_WHITESPACE) &&
      !mAlwaysCreateFramesForIgnorableWhitespace) {
    
    
    
    
    mAlwaysCreateFramesForIgnorableWhitespace = true;
    nsAutoScriptBlocker blocker;
    BeginUpdate();
    ReconstructDocElementHierarchy();
    EndUpdate();
  }
  return aContent->GetPrimaryFrame();
}

nsresult
nsCSSFrameConstructor::CharacterDataChanged(nsIContent* aContent,
                                            CharacterDataChangeInfo* aInfo)
{
  AUTO_LAYOUT_PHASE_ENTRY_POINT(mPresShell->GetPresContext(), FrameC);
  nsresult      rv = NS_OK;

  if ((aContent->HasFlag(NS_CREATE_FRAME_IF_NON_WHITESPACE) &&
       !aContent->TextIsOnlyWhitespace()) ||
      (aContent->HasFlag(NS_REFRAME_IF_WHITESPACE) &&
       aContent->TextIsOnlyWhitespace())) {
#ifdef DEBUG
    nsIFrame* frame = aContent->GetPrimaryFrame();
    NS_ASSERTION(!frame || !frame->IsGeneratedContentFrame(),
                 "Bit should never be set on generated content");
#endif
    LAYOUT_PHASE_TEMP_EXIT();
    nsresult rv = RecreateFramesForContent(aContent, false,
                                           REMOVE_FOR_RECONSTRUCTION, nullptr);
    LAYOUT_PHASE_TEMP_REENTER();
    return rv;
  }

  
  nsIFrame* frame = aContent->GetPrimaryFrame();

  
  

  
  
  if (nullptr != frame) {
#if 0
    NS_FRAME_LOG(NS_FRAME_TRACE_CALLS,
       ("nsCSSFrameConstructor::CharacterDataChanged: content=%p[%s] subcontent=%p frame=%p",
        aContent, ContentTag(aContent, 0),
        aSubContent, frame));
#endif

    
    
    
    
    
    
    
    
    
    nsContainerFrame* block = GetFloatContainingBlock(frame);
    bool haveFirstLetterStyle = false;
    if (block) {
      
      haveFirstLetterStyle = HasFirstLetterStyle(block);
      if (haveFirstLetterStyle) {
        RemoveLetterFrames(mPresShell->GetPresContext(), mPresShell,
                           block);
        
        
        frame = aContent->GetPrimaryFrame();
        NS_ASSERTION(frame, "Should have frame here!");
      }
    }

    frame->CharacterDataChanged(aInfo);

    if (haveFirstLetterStyle) {
      RecoverLetterFrames(block);
    }
  }

  return rv;
}

void
nsCSSFrameConstructor::BeginUpdate() {
  NS_ASSERTION(!nsContentUtils::IsSafeToRunScript(),
               "Someone forgot a script blocker");

  nsRootPresContext* rootPresContext =
    mPresShell->GetPresContext()->GetRootPresContext();
  if (rootPresContext) {
    rootPresContext->IncrementDOMGeneration();
  }

  ++sGlobalGenerationNumber;
  ++mUpdateCount;
}

void
nsCSSFrameConstructor::EndUpdate()
{
  if (mUpdateCount == 1) {
    
    

    RecalcQuotesAndCounters();
    NS_ASSERTION(mUpdateCount == 1, "Odd update count");
  }
  NS_ASSERTION(mUpdateCount, "Negative mUpdateCount!");
  --mUpdateCount;
}

void
nsCSSFrameConstructor::RecalcQuotesAndCounters()
{
  if (mQuotesDirty) {
    mQuotesDirty = false;
    mQuoteList.RecalcAll();
  }

  if (mCountersDirty) {
    mCountersDirty = false;
    mCounterManager.RecalcAll();
  }

  NS_ASSERTION(!mQuotesDirty, "Quotes updates will be lost");
  NS_ASSERTION(!mCountersDirty, "Counter updates will be lost");
}

void
nsCSSFrameConstructor::NotifyCounterStylesAreDirty()
{
  NS_PRECONDITION(mUpdateCount != 0, "Should be in an update");
  mCounterManager.SetAllCounterStylesDirty();
  CountersDirty();
}

void
nsCSSFrameConstructor::WillDestroyFrameTree()
{
#if defined(DEBUG_dbaron_off)
  mCounterManager.Dump();
#endif

  mIsDestroyingFrameTree = true;

  
  mQuoteList.Clear();
  mCounterManager.Clear();

  
  
  
  
  mPresShell->GetPresContext()->RefreshDriver()->
    RemoveStyleFlushObserver(mPresShell);

  nsFrameManager::Destroy();
}





void nsCSSFrameConstructor::GetAlternateTextFor(nsIContent*    aContent,
                                                nsIAtom*       aTag,  
                                                nsXPIDLString& aAltText)
{
  
  

  
  
  if (!aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::alt, aAltText) &&
      nsGkAtoms::input == aTag) {
    
    
    if (!aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::value, aAltText)) {
      nsContentUtils::GetLocalizedString(nsContentUtils::eFORMS_PROPERTIES,
                                         "Submit", aAltText);
    }
  }
}

nsIFrame*
nsCSSFrameConstructor::CreateContinuingOuterTableFrame(nsIPresShell*     aPresShell,
                                                       nsPresContext*    aPresContext,
                                                       nsIFrame*         aFrame,
                                                       nsContainerFrame* aParentFrame,
                                                       nsIContent*       aContent,
                                                       nsStyleContext*   aStyleContext)
{
  nsTableOuterFrame* newFrame = NS_NewTableOuterFrame(aPresShell, aStyleContext);

  newFrame->Init(aContent, aParentFrame, aFrame);

  
  
  nsFrameItems  newChildFrames;

  nsIFrame* childFrame = aFrame->GetFirstPrincipalChild();
  if (childFrame) {
    nsIFrame* continuingTableFrame =
      CreateContinuingFrame(aPresContext, childFrame, newFrame);
    newChildFrames.AddChild(continuingTableFrame);

    NS_ASSERTION(!childFrame->GetNextSibling(),"there can be only one inner table frame");
  }

  
  newFrame->SetInitialChildList(kPrincipalList, newChildFrames);

  return newFrame;
}

nsIFrame*
nsCSSFrameConstructor::CreateContinuingTableFrame(nsIPresShell*     aPresShell,
                                                  nsPresContext*    aPresContext,
                                                  nsIFrame*         aFrame,
                                                  nsContainerFrame* aParentFrame,
                                                  nsIContent*       aContent,
                                                  nsStyleContext*   aStyleContext)
{
  nsTableFrame* newFrame = NS_NewTableFrame(aPresShell, aStyleContext);

  newFrame->Init(aContent, aParentFrame, aFrame);

  
  nsFrameItems  childFrames;
  nsIFrame* childFrame = aFrame->GetFirstPrincipalChild();
  for ( ; childFrame; childFrame = childFrame->GetNextSibling()) {
    
    nsTableRowGroupFrame* rowGroupFrame =
      static_cast<nsTableRowGroupFrame*>(childFrame);
    
    nsIFrame* rgNextInFlow = rowGroupFrame->GetNextInFlow();
    if (rgNextInFlow) {
      rowGroupFrame->SetRepeatable(false);
    }
    else if (rowGroupFrame->IsRepeatable()) {
      
      nsTableRowGroupFrame*   headerFooterFrame;
      nsFrameItems            childItems;
      nsFrameConstructorState state(mPresShell,
                                    GetAbsoluteContainingBlock(newFrame, FIXED_POS),
                                    GetAbsoluteContainingBlock(newFrame, ABS_POS),
                                    nullptr);
      state.mCreatingExtraFrames = true;

      nsStyleContext* const headerFooterStyleContext = rowGroupFrame->StyleContext();
      headerFooterFrame = static_cast<nsTableRowGroupFrame*>
                                     (NS_NewTableRowGroupFrame(aPresShell, headerFooterStyleContext));

      nsIContent* headerFooter = rowGroupFrame->GetContent();
      headerFooterFrame->Init(headerFooter, newFrame, nullptr);

      nsFrameConstructorSaveState absoluteSaveState;
      MakeTablePartAbsoluteContainingBlockIfNeeded(state,
                                                   headerFooterStyleContext->StyleDisplay(),
                                                   absoluteSaveState,
                                                   headerFooterFrame);

      ProcessChildren(state, headerFooter, rowGroupFrame->StyleContext(),
                      headerFooterFrame, true, childItems, false,
                      nullptr);
      NS_ASSERTION(state.mFloatedItems.IsEmpty(), "unexpected floated element");
      headerFooterFrame->SetInitialChildList(kPrincipalList, childItems);
      headerFooterFrame->SetRepeatable(true);

      
      headerFooterFrame->InitRepeatedFrame(aPresContext, rowGroupFrame);

      
      childFrames.AddChild(headerFooterFrame);
    }
  }

  
  newFrame->SetInitialChildList(kPrincipalList, childFrames);

  return newFrame;
}

nsIFrame*
nsCSSFrameConstructor::CreateContinuingFrame(nsPresContext*    aPresContext,
                                             nsIFrame*         aFrame,
                                             nsContainerFrame* aParentFrame,
                                             bool              aIsFluid)
{
  nsIPresShell*              shell = aPresContext->PresShell();
  nsStyleContext*            styleContext = aFrame->StyleContext();
  nsIFrame*                  newFrame = nullptr;
  nsIFrame*                  nextContinuation = aFrame->GetNextContinuation();
  nsIFrame*                  nextInFlow = aFrame->GetNextInFlow();

  
  nsIAtom* frameType = aFrame->GetType();
  nsIContent* content = aFrame->GetContent();

  NS_ASSERTION(aFrame->GetSplittableType() != NS_FRAME_NOT_SPLITTABLE,
               "why CreateContinuingFrame for a non-splittable frame?");

  if (nsGkAtoms::textFrame == frameType) {
    newFrame = NS_NewContinuingTextFrame(shell, styleContext);
    newFrame->Init(content, aParentFrame, aFrame);
  } else if (nsGkAtoms::inlineFrame == frameType) {
    newFrame = NS_NewInlineFrame(shell, styleContext);
    newFrame->Init(content, aParentFrame, aFrame);
  } else if (nsGkAtoms::blockFrame == frameType) {
    MOZ_ASSERT(!aFrame->IsTableCaption(),
               "no support for fragmenting table captions yet");
    newFrame = NS_NewBlockFrame(shell, styleContext);
    newFrame->Init(content, aParentFrame, aFrame);
#ifdef MOZ_XUL
  } else if (nsGkAtoms::XULLabelFrame == frameType) {
    newFrame = NS_NewXULLabelFrame(shell, styleContext);
    newFrame->Init(content, aParentFrame, aFrame);
#endif
  } else if (nsGkAtoms::columnSetFrame == frameType) {
    MOZ_ASSERT(!aFrame->IsTableCaption(),
               "no support for fragmenting table captions yet");
    newFrame = NS_NewColumnSetFrame(shell, styleContext, nsFrameState(0));
    newFrame->Init(content, aParentFrame, aFrame);
  } else if (nsGkAtoms::pageFrame == frameType) {
    nsContainerFrame* canvasFrame;
    newFrame = ConstructPageFrame(shell, aPresContext, aParentFrame, aFrame,
                                  canvasFrame);
  } else if (nsGkAtoms::tableOuterFrame == frameType) {
    newFrame =
      CreateContinuingOuterTableFrame(shell, aPresContext, aFrame, aParentFrame,
                                      content, styleContext);

  } else if (nsGkAtoms::tableFrame == frameType) {
    newFrame =
      CreateContinuingTableFrame(shell, aPresContext, aFrame, aParentFrame,
                                 content, styleContext);

  } else if (nsGkAtoms::tableRowGroupFrame == frameType) {
    newFrame = NS_NewTableRowGroupFrame(shell, styleContext);
    newFrame->Init(content, aParentFrame, aFrame);
    if (newFrame->GetStateBits() & NS_FRAME_CAN_HAVE_ABSPOS_CHILDREN) {
      nsTableFrame::RegisterPositionedTablePart(newFrame);
    }
  } else if (nsGkAtoms::tableRowFrame == frameType) {
    nsTableRowFrame* rowFrame = NS_NewTableRowFrame(shell, styleContext);

    rowFrame->Init(content, aParentFrame, aFrame);
    if (rowFrame->GetStateBits() & NS_FRAME_CAN_HAVE_ABSPOS_CHILDREN) {
      nsTableFrame::RegisterPositionedTablePart(rowFrame);
    }

    
    nsFrameItems  newChildList;
    nsIFrame* cellFrame = aFrame->GetFirstPrincipalChild();
    while (cellFrame) {
      
      if (IS_TABLE_CELL(cellFrame->GetType())) {
        nsIFrame* continuingCellFrame =
          CreateContinuingFrame(aPresContext, cellFrame, rowFrame);
        newChildList.AddChild(continuingCellFrame);
      }
      cellFrame = cellFrame->GetNextSibling();
    }

    rowFrame->SetInitialChildList(kPrincipalList, newChildList);
    newFrame = rowFrame;

  } else if (IS_TABLE_CELL(frameType)) {
    
    
    
    nsTableCellFrame* cellFrame =
      NS_NewTableCellFrame(shell, styleContext, IsBorderCollapse(aParentFrame));

    cellFrame->Init(content, aParentFrame, aFrame);
    if (cellFrame->GetStateBits() & NS_FRAME_CAN_HAVE_ABSPOS_CHILDREN) {
      nsTableFrame::RegisterPositionedTablePart(cellFrame);
    }

    
    nsIFrame* blockFrame = aFrame->GetFirstPrincipalChild();
    nsIFrame* continuingBlockFrame =
      CreateContinuingFrame(aPresContext, blockFrame,
                            static_cast<nsContainerFrame*>(cellFrame));

    SetInitialSingleChild(cellFrame, continuingBlockFrame);
    newFrame = cellFrame;
  } else if (nsGkAtoms::lineFrame == frameType) {
    newFrame = NS_NewFirstLineFrame(shell, styleContext);
    newFrame->Init(content, aParentFrame, aFrame);
  } else if (nsGkAtoms::letterFrame == frameType) {
    newFrame = NS_NewFirstLetterFrame(shell, styleContext);
    newFrame->Init(content, aParentFrame, aFrame);
  } else if (nsGkAtoms::imageFrame == frameType) {
    newFrame = NS_NewImageFrame(shell, styleContext);
    newFrame->Init(content, aParentFrame, aFrame);
  } else if (nsGkAtoms::imageControlFrame == frameType) {
    newFrame = NS_NewImageControlFrame(shell, styleContext);
    newFrame->Init(content, aParentFrame, aFrame);
  } else if (nsGkAtoms::placeholderFrame == frameType) {
    
    nsIFrame* oofFrame = nsPlaceholderFrame::GetRealFrameForPlaceholder(aFrame);
    nsIFrame* oofContFrame =
      CreateContinuingFrame(aPresContext, oofFrame, aParentFrame);
    newFrame =
      CreatePlaceholderFrameFor(shell, content, oofContFrame, styleContext,
                                aParentFrame, aFrame,
                                aFrame->GetStateBits() & PLACEHOLDER_TYPE_MASK);
  } else if (nsGkAtoms::fieldSetFrame == frameType) {
    nsContainerFrame* fieldset = NS_NewFieldSetFrame(shell, styleContext);

    fieldset->Init(content, aParentFrame, aFrame);

    
    
    nsContainerFrame* blockFrame = GetFieldSetBlockFrame(aFrame);
    if (blockFrame) {
      nsIFrame* continuingBlockFrame =
        CreateContinuingFrame(aPresContext, blockFrame, fieldset);
      
      SetInitialSingleChild(fieldset, continuingBlockFrame);
    } else {
      MOZ_ASSERT(aFrame->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER,
                 "FieldSet block may only be null for overflow containers");
    }
    newFrame = fieldset;
  } else if (nsGkAtoms::legendFrame == frameType) {
    newFrame = NS_NewLegendFrame(shell, styleContext);
    newFrame->Init(content, aParentFrame, aFrame);
  } else if (nsGkAtoms::flexContainerFrame == frameType) {
    newFrame = NS_NewFlexContainerFrame(shell, styleContext);
    newFrame->Init(content, aParentFrame, aFrame);
  } else if (nsGkAtoms::rubyFrame == frameType) {
    newFrame = NS_NewRubyFrame(shell, styleContext);
    newFrame->Init(content, aParentFrame, aFrame);
  } else if (nsGkAtoms::rubyBaseContainerFrame == frameType) {
    newFrame = NS_NewRubyBaseContainerFrame(shell, styleContext);
    newFrame->Init(content, aParentFrame, aFrame);
  } else if (nsGkAtoms::rubyTextContainerFrame == frameType) {
    newFrame = NS_NewRubyTextContainerFrame(shell, styleContext);
    newFrame->Init(content, aParentFrame, aFrame);
  } else {
    NS_RUNTIMEABORT("unexpected frame type");
  }

  
  
  
  if (!aIsFluid) {
    newFrame->SetPrevContinuation(aFrame);
  }

  
  if (aFrame->GetStateBits() & NS_FRAME_GENERATED_CONTENT) {
    newFrame->AddStateBits(NS_FRAME_GENERATED_CONTENT);
  }

  
  
  if (aFrame->GetStateBits() & NS_FRAME_ANONYMOUSCONTENTCREATOR_CONTENT) {
    newFrame->AddStateBits(NS_FRAME_ANONYMOUSCONTENTCREATOR_CONTENT);
  }

  
  if (aFrame->GetStateBits() & NS_FRAME_OUT_OF_FLOW) {
    newFrame->AddStateBits(NS_FRAME_OUT_OF_FLOW);
  }

  if (nextInFlow) {
    nextInFlow->SetPrevInFlow(newFrame);
    newFrame->SetNextInFlow(nextInFlow);
  } else if (nextContinuation) {
    nextContinuation->SetPrevContinuation(newFrame);
    newFrame->SetNextContinuation(nextContinuation);
  }

  NS_POSTCONDITION(!newFrame->GetNextSibling(), "unexpected sibling");
  return newFrame;
}

nsresult
nsCSSFrameConstructor::ReplicateFixedFrames(nsPageContentFrame* aParentFrame)
{
  
  
  

  nsIFrame* prevPageContentFrame = aParentFrame->GetPrevInFlow();
  if (!prevPageContentFrame) {
    return NS_OK;
  }
  nsContainerFrame* canvasFrame =
    do_QueryFrame(aParentFrame->GetFirstPrincipalChild());
  nsIFrame* prevCanvasFrame = prevPageContentFrame->GetFirstPrincipalChild();
  if (!canvasFrame || !prevCanvasFrame) {
    
    return NS_ERROR_UNEXPECTED;
  }

  nsFrameItems fixedPlaceholders;
  nsIFrame* firstFixed = prevPageContentFrame->GetFirstChild(nsIFrame::kFixedList);
  if (!firstFixed) {
    return NS_OK;
  }

  
  
  
  
  nsFrameConstructorState state(mPresShell, aParentFrame,
                                nullptr,
                                mRootElementFrame);
  state.mCreatingExtraFrames = true;

  
  
  
  

  
  
  
  
  for (nsIFrame* fixed = firstFixed; fixed; fixed = fixed->GetNextSibling()) {
    nsIFrame* prevPlaceholder = GetPlaceholderFrameFor(fixed);
    if (prevPlaceholder &&
        nsLayoutUtils::IsProperAncestorFrame(prevCanvasFrame, prevPlaceholder)) {
      
      
      nsIContent* content = fixed->GetContent();
      nsStyleContext* styleContext =
        nsLayoutUtils::GetStyleFrame(content)->StyleContext();
      FrameConstructionItemList items;
      AddFrameConstructionItemsInternal(state, content, canvasFrame,
                                        content->NodeInfo()->NameAtom(),
                                        content->GetNameSpaceID(),
                                        true,
                                        styleContext,
                                        ITEM_ALLOW_XBL_BASE |
                                          ITEM_ALLOW_PAGE_BREAK,
                                        nullptr, items);
      ConstructFramesFromItemList(state, items, canvasFrame, fixedPlaceholders);
    }
  }

  
  
  
  NS_ASSERTION(!canvasFrame->GetFirstPrincipalChild(),
               "leaking frames; doc root continuation must be empty");
  canvasFrame->SetInitialChildList(kPrincipalList, fixedPlaceholders);
  return NS_OK;
}

nsCSSFrameConstructor::InsertionPoint
nsCSSFrameConstructor::GetInsertionPoint(nsIContent* aContainer,
                                         nsIContent* aChild)
{
  nsBindingManager* bindingManager = mDocument->BindingManager();

  nsIContent* insertionElement;
  if (aChild) {
    
    
    if (aChild->GetBindingParent() == aContainer) {
      
      
      return InsertionPoint(GetContentInsertionFrameFor(aContainer), aContainer);
    }

    if (nsContentUtils::HasDistributedChildren(aContainer)) {
      
      
      
      nsIContent* flattenedParent = aChild->GetFlattenedTreeParent();
      if (flattenedParent) {
        return InsertionPoint(GetContentInsertionFrameFor(flattenedParent),
                              flattenedParent);
      }
      return InsertionPoint();
    }

    insertionElement = bindingManager->FindNestedInsertionPoint(aContainer, aChild);
  } else {
    if (nsContentUtils::HasDistributedChildren(aContainer)) {
      
      
      
      return InsertionPoint(nullptr, nullptr, true);
    }

    bool multiple;
    insertionElement = bindingManager->FindNestedSingleInsertionPoint(aContainer, &multiple);
    if (multiple) {
      return InsertionPoint(nullptr, nullptr, true);
    }
  }

  if (!insertionElement) {
    insertionElement = aContainer;
  }
  InsertionPoint insertion(GetContentInsertionFrameFor(insertionElement),
                           insertionElement);

  
  
  if (insertion.mParentFrame &&
      insertion.mParentFrame->GetType() == nsGkAtoms::fieldSetFrame) {
    insertion.mMultiple = true;
  }

  return insertion;
}



void
nsCSSFrameConstructor::CaptureStateForFramesOf(nsIContent* aContent,
                                               nsILayoutHistoryState* aHistoryState)
{
  if (!aHistoryState) {
    return;
  }
  nsIFrame* frame = aContent->GetPrimaryFrame();
  if (frame == mRootElementFrame) {
    frame = mRootElementFrame ?
      GetAbsoluteContainingBlock(mRootElementFrame, FIXED_POS) :
      GetRootFrame();
  }
  for ( ; frame;
        frame = nsLayoutUtils::GetNextContinuationOrIBSplitSibling(frame)) {
    CaptureFrameState(frame, aHistoryState);
  }
}

static bool EqualURIs(mozilla::css::URLValue *aURI1,
                      mozilla::css::URLValue *aURI2)
{
  return aURI1 == aURI2 ||    
         (aURI1 && aURI2 && aURI1->URIEquals(*aURI2));
}

nsStyleContext*
nsCSSFrameConstructor::MaybeRecreateFramesForElement(Element* aElement)
{
  nsRefPtr<nsStyleContext> oldContext = GetUndisplayedContent(aElement);
  uint8_t oldDisplay = NS_STYLE_DISPLAY_NONE;
  if (!oldContext) {
    oldContext = GetDisplayContentsStyleFor(aElement);
    if (!oldContext) {
      return nullptr;
    }
    oldDisplay = NS_STYLE_DISPLAY_CONTENTS;
  }

  
  nsRefPtr<nsStyleContext> newContext = mPresShell->StyleSet()->
    ResolveStyleFor(aElement, oldContext->GetParent());

  if (oldDisplay == NS_STYLE_DISPLAY_NONE) {
    ChangeUndisplayedContent(aElement, newContext);
  } else {
    ChangeDisplayContents(aElement, newContext);
  }

  const nsStyleDisplay* disp = newContext->StyleDisplay();
  if (oldDisplay == disp->mDisplay) {
    
    
    
    
    if (!disp->mBinding) {
      return newContext;
    }
    const nsStyleDisplay* oldDisp = oldContext->PeekStyleDisplay();
    if (oldDisp && EqualURIs(disp->mBinding, oldDisp->mBinding)) {
      return newContext;
    }
  }

  RecreateFramesForContent(aElement, false, REMOVE_FOR_RECONSTRUCTION, nullptr);
  return nullptr;
}

static bool
IsWhitespaceFrame(nsIFrame* aFrame)
{
  MOZ_ASSERT(aFrame, "invalid argument");
  return aFrame->GetType() == nsGkAtoms::textFrame &&
    aFrame->GetContent()->TextIsOnlyWhitespace();
}

static nsIFrame*
FindFirstNonWhitespaceChild(nsIFrame* aParentFrame)
{
  nsIFrame* f = aParentFrame->GetFirstPrincipalChild();
  while (f && IsWhitespaceFrame(f)) {
    f = f->GetNextSibling();
  }
  return f;
}

static nsIFrame*
FindNextNonWhitespaceSibling(nsIFrame* aFrame)
{
  nsIFrame* f = aFrame;
  do {
    f = f->GetNextSibling();
  } while (f && IsWhitespaceFrame(f));
  return f;
}

static nsIFrame*
FindPreviousNonWhitespaceSibling(nsIFrame* aFrame)
{
  nsIFrame* f = aFrame;
  do {
    f = f->GetPrevSibling();
  } while (f && IsWhitespaceFrame(f));
  return f;
}

bool
nsCSSFrameConstructor::MaybeRecreateContainerForFrameRemoval(nsIFrame* aFrame,
                                                             RemoveFlags aFlags,
                                                             nsresult* aResult,
                                                             nsIContent** aDestroyedFramesFor)
{
  NS_PRECONDITION(aFrame, "Must have a frame");
  NS_PRECONDITION(aFrame->GetParent(), "Frame shouldn't be root");
  NS_PRECONDITION(aResult, "Null out param?");
  NS_PRECONDITION(aFrame == aFrame->FirstContinuation(),
                  "aFrame not the result of GetPrimaryFrame()?");

  *aDestroyedFramesFor = nullptr;

  if (IsFramePartOfIBSplit(aFrame)) {
    
    
#ifdef DEBUG
    if (gNoisyContentUpdates) {
      printf("nsCSSFrameConstructor::MaybeRecreateContainerForFrameRemoval: "
             "frame=");
      nsFrame::ListTag(stdout, aFrame);
      printf(" is ib-split\n");
    }
#endif

    *aResult = ReframeContainingBlock(aFrame, aFlags, aDestroyedFramesFor);
    return true;
  }

  nsContainerFrame* insertionFrame = aFrame->GetContentInsertionFrame();
  if (insertionFrame && insertionFrame->GetType() == nsGkAtoms::legendFrame &&
      aFrame->GetParent()->GetType() == nsGkAtoms::fieldSetFrame) {
    
    
    *aResult = RecreateFramesForContent(aFrame->GetParent()->GetContent(), false,
                                        aFlags, aDestroyedFramesFor);
    return true;
  }

  
  nsIFrame* inFlowFrame =
    (aFrame->GetStateBits() & NS_FRAME_OUT_OF_FLOW) ?
      GetPlaceholderFrameFor(aFrame) : aFrame;
  MOZ_ASSERT(inFlowFrame, "How did that happen?");
  MOZ_ASSERT(inFlowFrame == inFlowFrame->FirstContinuation(),
             "placeholder for primary frame has previous continuations?");
  nsIFrame* parent = inFlowFrame->GetParent();
  
  
  
  if (IsTableOrRubyPseudo(parent)) {
    if (FindFirstNonWhitespaceChild(parent) == inFlowFrame ||
        !FindNextNonWhitespaceSibling(inFlowFrame->LastContinuation()) ||
        
        
        (IsWhitespaceFrame(aFrame) &&
         parent->PrincipalChildList().OnlyChild()) ||
        
        
        (inFlowFrame->GetType() == nsGkAtoms::tableColGroupFrame &&
         parent->GetFirstChild(nsIFrame::kColGroupList) == inFlowFrame) ||
        
        (inFlowFrame->IsTableCaption() &&
         parent->GetFirstChild(nsIFrame::kCaptionList) == inFlowFrame)) {
      
      
      *aResult = RecreateFramesForContent(parent->GetContent(), true, aFlags,
                                          aDestroyedFramesFor);
      return true;
    }
  }

  
  
  
  
  nsIFrame* nextSibling =
    FindNextNonWhitespaceSibling(inFlowFrame->LastContinuation());
  NS_ASSERTION(!IsTableOrRubyPseudo(inFlowFrame), "Shouldn't happen here");
  
  
  
  if (nextSibling && IsTableOrRubyPseudo(nextSibling)) {
    nsIFrame* prevSibling = FindPreviousNonWhitespaceSibling(inFlowFrame);
    if (prevSibling && IsTableOrRubyPseudo(prevSibling)) {
#ifdef DEBUG
      if (gNoisyContentUpdates) {
        printf("nsCSSFrameConstructor::MaybeRecreateContainerForFrameRemoval: "
               "frame=");
        nsFrame::ListTag(stdout, aFrame);
        printf(" has a table pseudo next sibling of different type and a "
               "table pseudo prevsibling\n");
      }
#endif
      
      
      *aResult = RecreateFramesForContent(parent->GetContent(), true, aFlags,
                                          aDestroyedFramesFor);
      return true;
    }
  }

  
  nsIAtom* parentType = parent->GetType();
  if (parentType == nsGkAtoms::rubyFrame ||
      RubyUtils::IsRubyContainerBox(parentType)) {
    
    
    
    
    
    
    
    *aResult = RecreateFramesForContent(parent->GetContent(), true, aFlags,
                                        aDestroyedFramesFor);
    return true;
  }

  
  
  
  
  
  
  
  
  if (nextSibling && IsAnonymousFlexOrGridItem(nextSibling)) {
    AssertAnonymousFlexOrGridItemParent(nextSibling, parent);
#ifdef DEBUG
    if (gNoisyContentUpdates) {
      printf("nsCSSFrameConstructor::MaybeRecreateContainerForFrameRemoval: "
             "frame=");
      nsFrame::ListTag(stdout, aFrame);
      printf(" has an anonymous flex item as its next sibling\n");
    }
#endif 
    
    *aResult = RecreateFramesForContent(parent->GetContent(), true, aFlags,
                                        aDestroyedFramesFor);
    return true;
  }

  
  
  
  if (!nextSibling && IsAnonymousFlexOrGridItem(parent)) {
    AssertAnonymousFlexOrGridItemParent(parent, parent->GetParent());
#ifdef DEBUG
    if (gNoisyContentUpdates) {
      printf("nsCSSFrameConstructor::MaybeRecreateContainerForFrameRemoval: "
             "frame=");
      nsFrame::ListTag(stdout, aFrame);
      printf(" has an anonymous flex item as its parent\n");
    }
#endif 
    
    *aResult = RecreateFramesForContent(parent->GetParent()->GetContent(), true,
                                        aFlags, aDestroyedFramesFor);
    return true;
  }

#ifdef MOZ_XUL
  if (aFrame->GetType() == nsGkAtoms::popupSetFrame) {
    nsIRootBox* rootBox = nsIRootBox::GetRootBox(mPresShell);
    if (rootBox && rootBox->GetPopupSetFrame() == aFrame) {
      *aResult = ReconstructDocElementHierarchy();
      return true;
    }
  }
#endif

  
  
  if (!inFlowFrame->GetPrevSibling() &&
      !inFlowFrame->GetNextSibling() &&
      ((parent->GetPrevContinuation() && !parent->GetPrevInFlow()) ||
       (parent->GetNextContinuation() && !parent->GetNextInFlow()))) {
    *aResult = RecreateFramesForContent(parent->GetContent(), true, aFlags,
                                        aDestroyedFramesFor);
    return true;
  }

  
  
  
  if (!IsFramePartOfIBSplit(parent)) {
    return false;
  }

  
  
  if (inFlowFrame != parent->GetFirstPrincipalChild() ||
      inFlowFrame->LastContinuation()->GetNextSibling()) {
    return false;
  }

  
  
  
  nsIFrame* parentFirstContinuation = parent->FirstContinuation();
  if (!GetIBSplitSibling(parentFirstContinuation) ||
      !GetIBSplitPrevSibling(parentFirstContinuation)) {
    return false;
  }

#ifdef DEBUG
  if (gNoisyContentUpdates) {
    printf("nsCSSFrameConstructor::MaybeRecreateContainerForFrameRemoval: "
           "frame=");
    nsFrame::ListTag(stdout, parent);
    printf(" is ib-split\n");
  }
#endif

  *aResult = ReframeContainingBlock(parent, aFlags, aDestroyedFramesFor);
  return true;
}

nsresult
nsCSSFrameConstructor::RecreateFramesForContent(nsIContent*  aContent,
                                                bool         aAsyncInsert,
                                                RemoveFlags  aFlags,
                                                nsIContent** aDestroyedFramesFor)
{
  NS_PRECONDITION(!aAsyncInsert || aContent->IsElement(),
                  "Can only insert elements async");
  
  
  
  
  
  NS_ENSURE_TRUE(aContent->GetCrossShadowCurrentDoc(), NS_ERROR_FAILURE);

  
  
  
  
  
  

  nsIFrame* frame = aContent->GetPrimaryFrame();
  if (frame && frame->IsFrameOfType(nsIFrame::eMathML)) {
    
    
    while (true) {
      nsIContent* parentContent = aContent->GetParent();
      nsIFrame* parentContentFrame = parentContent->GetPrimaryFrame();
      if (!parentContentFrame || !parentContentFrame->IsFrameOfType(nsIFrame::eMathML))
        break;
      aContent = parentContent;
      frame = parentContentFrame;
    }
  }

  if (frame) {
    nsIFrame* nonGeneratedAncestor = nsLayoutUtils::GetNonGeneratedAncestor(frame);
    if (nonGeneratedAncestor->GetContent() != aContent) {
      return RecreateFramesForContent(nonGeneratedAncestor->GetContent(), aAsyncInsert,
                                      aFlags, aDestroyedFramesFor);
    }

    if (frame->GetStateBits() & NS_FRAME_ANONYMOUSCONTENTCREATOR_CONTENT) {
      
      
      
      
      
      nsIAnonymousContentCreator* acc = nullptr;
      nsIFrame* ancestor = nsLayoutUtils::GetParentOrPlaceholderFor(frame);
      while (!(acc = do_QueryFrame(ancestor))) {
        ancestor = nsLayoutUtils::GetParentOrPlaceholderFor(ancestor);
      }
      NS_ASSERTION(acc, "Where is the nsIAnonymousContentCreator? We may fail "
                        "to recreate its content correctly");
      
      if (ancestor->GetType() != nsGkAtoms::svgUseFrame) {
        NS_ASSERTION(aContent->IsInNativeAnonymousSubtree(),
                     "Why is NS_FRAME_ANONYMOUSCONTENTCREATOR_CONTENT set?");
        return RecreateFramesForContent(ancestor->GetContent(), aAsyncInsert,
                                        aFlags, aDestroyedFramesFor);
      }
    }

    nsIFrame* parent = frame->GetParent();
    nsIContent* parentContent = parent ? parent->GetContent() : nullptr;
    
    
    
    if (parent && parent->IsLeaf() && parentContent &&
        parentContent != aContent) {
      return RecreateFramesForContent(parentContent, aAsyncInsert, aFlags,
                                      aDestroyedFramesFor);
    }
  }

  nsresult rv = NS_OK;
  nsIContent* container;
  if (frame && MaybeRecreateContainerForFrameRemoval(frame, aFlags, &rv,
                                                     &container)) {
    MOZ_ASSERT(container);
    if (aDestroyedFramesFor) {
      *aDestroyedFramesFor = container;
    }
    return rv;
  }

  nsINode* containerNode = aContent->GetParentNode();
  
  if (containerNode) {
    
    
    CaptureStateForFramesOf(aContent, mTempFrameTreeState);

    
    
    nsCOMPtr<nsIContent> container = aContent->GetParent();

    
    bool didReconstruct;
    nsIContent* nextSibling = aContent->IsRootOfAnonymousSubtree() ?
      nullptr : aContent->GetNextSibling();
    const bool reconstruct = aFlags != REMOVE_DESTROY_FRAMES;
    RemoveFlags flags = reconstruct ? REMOVE_FOR_RECONSTRUCTION : aFlags;
    rv = ContentRemoved(container, aContent, nextSibling, flags,
                        &didReconstruct, aDestroyedFramesFor);
    if (NS_FAILED(rv)) {
      return rv;
    }
    if (reconstruct && !didReconstruct) {
      
      
      
      if (aAsyncInsert) {
        
        RestyleManager()->PostRestyleEvent(
          aContent->AsElement(), nsRestyleHint(0), nsChangeHint_ReconstructFrame);
      } else {
        rv = ContentInserted(container, aContent, mTempFrameTreeState, false);
      }
    }
  }

  return rv;
}

void
nsCSSFrameConstructor::DestroyFramesFor(nsIContent*  aContent,
                                        nsIContent** aDestroyedFramesFor)
{
  MOZ_ASSERT(aContent && aContent->GetParentNode());

  bool didReconstruct;
  nsIContent* nextSibling =
    aContent->IsRootOfAnonymousSubtree() ? nullptr : aContent->GetNextSibling();
  ContentRemoved(aContent->GetParent(), aContent, nextSibling,
                 REMOVE_DESTROY_FRAMES, &didReconstruct, aDestroyedFramesFor);
}





already_AddRefed<nsStyleContext>
nsCSSFrameConstructor::GetFirstLetterStyle(nsIContent* aContent,
                                           nsStyleContext* aStyleContext)
{
  if (aContent) {
    return mPresShell->StyleSet()->
      ResolvePseudoElementStyle(aContent->AsElement(),
                                nsCSSPseudoElements::ePseudo_firstLetter,
                                aStyleContext,
                                nullptr);
  }
  return nullptr;
}

already_AddRefed<nsStyleContext>
nsCSSFrameConstructor::GetFirstLineStyle(nsIContent* aContent,
                                         nsStyleContext* aStyleContext)
{
  if (aContent) {
    return mPresShell->StyleSet()->
      ResolvePseudoElementStyle(aContent->AsElement(),
                                nsCSSPseudoElements::ePseudo_firstLine,
                                aStyleContext,
                                nullptr);
  }
  return nullptr;
}



bool
nsCSSFrameConstructor::ShouldHaveFirstLetterStyle(nsIContent* aContent,
                                                  nsStyleContext* aStyleContext)
{
  return nsLayoutUtils::HasPseudoStyle(aContent, aStyleContext,
                                       nsCSSPseudoElements::ePseudo_firstLetter,
                                       mPresShell->GetPresContext());
}

bool
nsCSSFrameConstructor::HasFirstLetterStyle(nsIFrame* aBlockFrame)
{
  NS_PRECONDITION(aBlockFrame, "Need a frame");
  NS_ASSERTION(nsLayoutUtils::GetAsBlock(aBlockFrame),
               "Not a block frame?");

  return (aBlockFrame->GetStateBits() & NS_BLOCK_HAS_FIRST_LETTER_STYLE) != 0;
}

bool
nsCSSFrameConstructor::ShouldHaveFirstLineStyle(nsIContent* aContent,
                                                nsStyleContext* aStyleContext)
{
  bool hasFirstLine =
    nsLayoutUtils::HasPseudoStyle(aContent, aStyleContext,
                                  nsCSSPseudoElements::ePseudo_firstLine,
                                  mPresShell->GetPresContext());
  if (hasFirstLine) {
    
    int32_t namespaceID;
    nsIAtom* tag = mDocument->BindingManager()->ResolveTag(aContent,
                                                           &namespaceID);
    
    hasFirstLine = tag != nsGkAtoms::fieldset ||
      namespaceID != kNameSpaceID_XHTML;
  }

  return hasFirstLine;
}

void
nsCSSFrameConstructor::ShouldHaveSpecialBlockStyle(nsIContent* aContent,
                                                   nsStyleContext* aStyleContext,
                                                   bool* aHaveFirstLetterStyle,
                                                   bool* aHaveFirstLineStyle)
{
  *aHaveFirstLetterStyle =
    ShouldHaveFirstLetterStyle(aContent, aStyleContext);
  *aHaveFirstLineStyle =
    ShouldHaveFirstLineStyle(aContent, aStyleContext);
}


const nsCSSFrameConstructor::PseudoParentData
nsCSSFrameConstructor::sPseudoParentData[eParentTypeCount] = {
  { 
    FULL_CTOR_FCDATA(FCDATA_IS_TABLE_PART | FCDATA_SKIP_FRAMESET |
                     FCDATA_USE_CHILD_ITEMS |
                     FCDATA_DESIRED_PARENT_TYPE_TO_BITS(eTypeRow),
                     &nsCSSFrameConstructor::ConstructTableCell),
    &nsCSSAnonBoxes::tableCell
  },
  { 
    FULL_CTOR_FCDATA(FCDATA_IS_TABLE_PART | FCDATA_SKIP_FRAMESET |
                     FCDATA_USE_CHILD_ITEMS |
                     FCDATA_DESIRED_PARENT_TYPE_TO_BITS(eTypeRowGroup),
                     &nsCSSFrameConstructor::ConstructTableRowOrRowGroup),
    &nsCSSAnonBoxes::tableRow
  },
  { 
    FULL_CTOR_FCDATA(FCDATA_IS_TABLE_PART | FCDATA_SKIP_FRAMESET |
                     FCDATA_USE_CHILD_ITEMS |
                     FCDATA_DESIRED_PARENT_TYPE_TO_BITS(eTypeTable),
                     &nsCSSFrameConstructor::ConstructTableRowOrRowGroup),
    &nsCSSAnonBoxes::tableRowGroup
  },
  { 
    FCDATA_DECL(FCDATA_IS_TABLE_PART | FCDATA_SKIP_FRAMESET |
                FCDATA_DISALLOW_OUT_OF_FLOW | FCDATA_USE_CHILD_ITEMS |
                FCDATA_SKIP_ABSPOS_PUSH |
                FCDATA_DESIRED_PARENT_TYPE_TO_BITS(eTypeTable),
                NS_NewTableColGroupFrame),
    &nsCSSAnonBoxes::tableColGroup
  },
  { 
    FULL_CTOR_FCDATA(FCDATA_SKIP_FRAMESET | FCDATA_USE_CHILD_ITEMS,
                     &nsCSSFrameConstructor::ConstructTable),
    &nsCSSAnonBoxes::table
  },
  { 
    FCDATA_DECL(FCDATA_IS_LINE_PARTICIPANT |
                FCDATA_USE_CHILD_ITEMS |
                FCDATA_SKIP_FRAMESET,
                NS_NewRubyFrame),
    &nsCSSAnonBoxes::ruby
  },
  { 
    FCDATA_DECL(FCDATA_USE_CHILD_ITEMS | 
                FCDATA_IS_LINE_PARTICIPANT |
                FCDATA_DESIRED_PARENT_TYPE_TO_BITS(eTypeRubyBaseContainer) |
                FCDATA_SKIP_FRAMESET,
                NS_NewRubyBaseFrame),
    &nsCSSAnonBoxes::rubyBase
  },
  { 
    FCDATA_DECL(FCDATA_USE_CHILD_ITEMS |
                FCDATA_IS_LINE_PARTICIPANT |
                FCDATA_DESIRED_PARENT_TYPE_TO_BITS(eTypeRuby) |
                FCDATA_SKIP_FRAMESET,
                NS_NewRubyBaseContainerFrame),
    &nsCSSAnonBoxes::rubyBaseContainer
  },
  { 
    FCDATA_DECL(FCDATA_USE_CHILD_ITEMS |
                FCDATA_IS_LINE_PARTICIPANT |
                FCDATA_DESIRED_PARENT_TYPE_TO_BITS(eTypeRubyTextContainer) |
                FCDATA_SKIP_FRAMESET,
                NS_NewRubyTextFrame),
    &nsCSSAnonBoxes::rubyText
  },
  { 
    FCDATA_DECL(FCDATA_USE_CHILD_ITEMS |
                FCDATA_DESIRED_PARENT_TYPE_TO_BITS(eTypeRuby) |
                FCDATA_SKIP_FRAMESET,
                NS_NewRubyTextContainerFrame),
    &nsCSSAnonBoxes::rubyTextContainer
  }
};

void
nsCSSFrameConstructor::CreateNeededAnonFlexOrGridItems(
  nsFrameConstructorState& aState,
  FrameConstructionItemList& aItems,
  nsIFrame* aParentFrame)
{
  if (aItems.IsEmpty() ||
      !::IsFlexOrGridContainer(aParentFrame)) {
    return;
  }

  nsIAtom* containerType = aParentFrame->GetType();
  FCItemIterator iter(aItems);
  do {
    
    if (iter.SkipItemsThatDontNeedAnonFlexOrGridItem(aState)) {
      
      
      return;
    }

    
    
    
    
    
    
    
    
    
    
    
    if (!aParentFrame->IsGeneratedContentFrame() &&
        iter.item().IsWhitespace(aState)) {
      FCItemIterator afterWhitespaceIter(iter);
      bool hitEnd = afterWhitespaceIter.SkipWhitespace(aState);
      bool nextChildNeedsAnonItem =
        !hitEnd && afterWhitespaceIter.item().NeedsAnonFlexOrGridItem(aState);

      if (!nextChildNeedsAnonItem) {
        
        
        iter.DeleteItemsTo(afterWhitespaceIter);
        if (hitEnd) {
          
          return;
        }
        
        
        
        MOZ_ASSERT(!iter.IsDone() &&
                   !iter.item().NeedsAnonFlexOrGridItem(aState),
                   "hitEnd and/or nextChildNeedsAnonItem lied");
        continue;
      }
    }

    
    
    
    FCItemIterator endIter(iter); 
    endIter.SkipItemsThatNeedAnonFlexOrGridItem(aState);

    NS_ASSERTION(iter != endIter,
                 "Should've had at least one wrappable child to seek past");

    
    
    nsIAtom* pseudoType = containerType == nsGkAtoms::flexContainerFrame ?
      nsCSSAnonBoxes::anonymousFlexItem : nsCSSAnonBoxes::anonymousGridItem;
    nsStyleContext* parentStyle = aParentFrame->StyleContext();
    nsIContent* parentContent = aParentFrame->GetContent();
    already_AddRefed<nsStyleContext> wrapperStyle =
      mPresShell->StyleSet()->ResolveAnonymousBoxStyle(pseudoType, parentStyle);

    static const FrameConstructionData sBlockFormattingContextFCData =
      FCDATA_DECL(FCDATA_SKIP_FRAMESET | FCDATA_USE_CHILD_ITEMS,
                  NS_NewBlockFormattingContext);

    FrameConstructionItem* newItem =
      new FrameConstructionItem(&sBlockFormattingContextFCData,
                                
                                parentContent,
                                
                                pseudoType,
                                iter.item().mNameSpaceID,
                                
                                nullptr,
                                wrapperStyle,
                                true, nullptr);

    newItem->mIsAllInline = newItem->mHasInlineEnds =
      newItem->mStyleContext->StyleDisplay()->IsInlineOutsideStyle();
    newItem->mIsBlock = !newItem->mIsAllInline;

    MOZ_ASSERT(!newItem->mIsAllInline && newItem->mIsBlock,
               "expecting anonymous flex/grid items to be block-level "
               "(this will make a difference when we encounter "
               "'align-items: baseline')");

    
    
    newItem->mChildItems.SetLineBoundaryAtStart(true);
    newItem->mChildItems.SetLineBoundaryAtEnd(true);
    
    
    newItem->mChildItems.SetParentHasNoXBLChildren(
      aItems.ParentHasNoXBLChildren());

    
    
    iter.AppendItemsToList(endIter, newItem->mChildItems);

    iter.InsertItem(newItem);
  } while (!iter.IsDone());
}

 nsCSSFrameConstructor::RubyWhitespaceType
nsCSSFrameConstructor::ComputeRubyWhitespaceType(uint_fast8_t aPrevDisplay,
                                                 uint_fast8_t aNextDisplay)
{
  MOZ_ASSERT(nsStyleDisplay::IsRubyDisplayType(aPrevDisplay) &&
             nsStyleDisplay::IsRubyDisplayType(aNextDisplay));
  if (aPrevDisplay == aNextDisplay &&
      (aPrevDisplay == NS_STYLE_DISPLAY_RUBY_BASE ||
       aPrevDisplay == NS_STYLE_DISPLAY_RUBY_TEXT)) {
    return eRubyInterLeafWhitespace;
  }
  if (aNextDisplay == NS_STYLE_DISPLAY_RUBY_TEXT ||
      aNextDisplay == NS_STYLE_DISPLAY_RUBY_TEXT_CONTAINER) {
    return eRubyInterLevelWhitespace;
  }
  return eRubyInterSegmentWhitespace;
}







 nsCSSFrameConstructor::RubyWhitespaceType
nsCSSFrameConstructor::InterpretRubyWhitespace(nsFrameConstructorState& aState,
                                               const FCItemIterator& aStartIter,
                                               const FCItemIterator& aEndIter)
{
  if (!aStartIter.item().IsWhitespace(aState)) {
    return eRubyNotWhitespace;
  }

  FCItemIterator spaceEndIter(aStartIter);
  spaceEndIter.SkipWhitespace(aState);
  if (spaceEndIter != aEndIter) {
    return eRubyNotWhitespace;
  }

  
  
  
  MOZ_ASSERT(!aStartIter.AtStart() && !aEndIter.IsDone());
  FCItemIterator prevIter(aStartIter);
  prevIter.Prev();
  return ComputeRubyWhitespaceType(
      prevIter.item().mStyleContext->StyleDisplay()->mDisplay,
      aEndIter.item().mStyleContext->StyleDisplay()->mDisplay);
}







void
nsCSSFrameConstructor::WrapItemsInPseudoRubyLeafBox(
  FCItemIterator& aIter,
  nsStyleContext* aParentStyle, nsIContent* aParentContent)
{
  uint_fast8_t parentDisplay = aParentStyle->StyleDisplay()->mDisplay;
  ParentType parentType, wrapperType;
  if (parentDisplay == NS_STYLE_DISPLAY_RUBY_TEXT_CONTAINER) {
    parentType = eTypeRubyTextContainer;
    wrapperType = eTypeRubyText;
  } else {
    MOZ_ASSERT(parentDisplay == NS_STYLE_DISPLAY_RUBY_BASE_CONTAINER);
    parentType = eTypeRubyBaseContainer;
    wrapperType = eTypeRubyBase;
  }

  MOZ_ASSERT(aIter.item().DesiredParentType() != parentType,
             "Should point to something needs to be wrapped.");

  FCItemIterator endIter(aIter);
  endIter.SkipItemsNotWantingParentType(parentType);

  WrapItemsInPseudoParent(aParentContent, aParentStyle,
                          wrapperType, aIter, endIter);
}






void
nsCSSFrameConstructor::WrapItemsInPseudoRubyLevelContainer(
  nsFrameConstructorState& aState, FCItemIterator& aIter,
  nsStyleContext* aParentStyle, nsIContent* aParentContent)
{
  MOZ_ASSERT(aIter.item().DesiredParentType() != eTypeRuby,
             "Pointing to a level container?");

  FrameConstructionItem& firstItem = aIter.item();
  ParentType wrapperType = firstItem.DesiredParentType();
  if (wrapperType != eTypeRubyTextContainer) {
    
    
    wrapperType = eTypeRubyBaseContainer;
  }

  FCItemIterator endIter(aIter);
  do {
    if (endIter.SkipItemsWantingParentType(wrapperType) ||
        
        
        IsRubyParentType(endIter.item().DesiredParentType())) {
      
      break;
    }

    FCItemIterator contentEndIter(endIter);
    contentEndIter.SkipItemsNotWantingRubyParent();
    
    MOZ_ASSERT(contentEndIter != endIter);

    
    
    
    
    RubyWhitespaceType whitespaceType =
      InterpretRubyWhitespace(aState, endIter, contentEndIter);
    if ((whitespaceType == eRubyInterLeafWhitespace ||
         whitespaceType == eRubyInterSegmentWhitespace) &&
        !endIter.item().mStyleContext->ShouldSuppressLineBreak()) {
      
      
      
      
      nsStyleContext* oldContext = endIter.item().mStyleContext;
      nsRefPtr<nsStyleContext> newContext = mPresShell->StyleSet()->
        ResolveStyleForNonElement(oldContext->GetParent(), true);
      MOZ_ASSERT(newContext->ShouldSuppressLineBreak());
      for (FCItemIterator iter(endIter); iter != contentEndIter; iter.Next()) {
        MOZ_ASSERT(iter.item().mStyleContext == oldContext,
                   "all whitespaces should share the same style context");
        iter.item().mStyleContext = newContext;
      }
    }
    if (whitespaceType == eRubyInterLevelWhitespace) {
      
      bool atStart = (aIter == endIter);
      endIter.DeleteItemsTo(contentEndIter);
      if (atStart) {
        aIter = endIter;
      }
    } else if (whitespaceType == eRubyInterSegmentWhitespace) {
      
      
      
      if (aIter == endIter) {
        MOZ_ASSERT(wrapperType == eTypeRubyBaseContainer,
                   "Inter-segment whitespace should be wrapped in rbc");
        endIter = contentEndIter;
      }
      break;
    } else if (wrapperType == eTypeRubyTextContainer &&
               whitespaceType != eRubyInterLeafWhitespace) {
      
      
      
      break;
    } else {
      endIter = contentEndIter;
    }
  } while (!endIter.IsDone());

  
  
  
  
  if (aIter != endIter) {
    WrapItemsInPseudoParent(aParentContent, aParentStyle,
                            wrapperType, aIter, endIter);
  }
}





void
nsCSSFrameConstructor::TrimLeadingAndTrailingWhitespaces(
    nsFrameConstructorState& aState,
    FrameConstructionItemList& aItems)
{
  FCItemIterator iter(aItems);
  if (!iter.IsDone() &&
      iter.item().IsWhitespace(aState)) {
    FCItemIterator spaceEndIter(iter);
    spaceEndIter.SkipWhitespace(aState);
    iter.DeleteItemsTo(spaceEndIter);
  }

  iter.SetToEnd();
  if (!iter.AtStart()) {
    FCItemIterator spaceEndIter(iter);
    do {
      iter.Prev();
      if (iter.AtStart()) {
        
        
        break;
      }
    } while (iter.item().IsWhitespace(aState));
    iter.Next();
    if (iter != spaceEndIter) {
      iter.DeleteItemsTo(spaceEndIter);
    }
  }
}





void
nsCSSFrameConstructor::CreateNeededPseudoInternalRubyBoxes(
  nsFrameConstructorState& aState,
  FrameConstructionItemList& aItems,
  nsIFrame* aParentFrame)
{
  const ParentType ourParentType = GetParentType(aParentFrame);
  if (!IsRubyParentType(ourParentType) ||
      aItems.AllWantParentType(ourParentType)) {
    return;
  }

  if (!IsRubyPseudo(aParentFrame)) {
    
    
    
    
    
    
    
    
    
    
    TrimLeadingAndTrailingWhitespaces(aState, aItems);
  }

  FCItemIterator iter(aItems);
  nsIContent* parentContent = aParentFrame->GetContent();
  nsStyleContext* parentStyle = aParentFrame->StyleContext();
  while (!iter.IsDone()) {
    if (!iter.SkipItemsWantingParentType(ourParentType)) {
      if (ourParentType == eTypeRuby) {
        WrapItemsInPseudoRubyLevelContainer(aState, iter, parentStyle,
                                            parentContent);
      } else {
        WrapItemsInPseudoRubyLeafBox(iter, parentStyle, parentContent);
      }
    }
  }
}












void
nsCSSFrameConstructor::CreateNeededPseudoContainers(
    nsFrameConstructorState& aState,
    FrameConstructionItemList& aItems,
    nsIFrame* aParentFrame)
{
  ParentType ourParentType = GetParentType(aParentFrame);
  if (IsRubyParentType(ourParentType) ||
      aItems.AllWantParentType(ourParentType)) {
    
    return;
  }

  FCItemIterator iter(aItems);
  do {
    if (iter.SkipItemsWantingParentType(ourParentType)) {
      
      return;
    }

    
    

    
    
    
    
    
    
    
    
    
    

    FCItemIterator endIter(iter); 
    ParentType groupingParentType = endIter.item().DesiredParentType();
    if (aItems.AllWantParentType(groupingParentType) &&
        groupingParentType != eTypeBlock) {
      
      
      
      endIter.SetToEnd();
    } else {
      

      
      
      
      ParentType prevParentType = ourParentType;
      do {
        
        
        FCItemIterator spaceEndIter(endIter);
        if (prevParentType != eTypeBlock &&
            !aParentFrame->IsGeneratedContentFrame() &&
            spaceEndIter.item().IsWhitespace(aState)) {
          bool trailingSpaces = spaceEndIter.SkipWhitespace(aState);

          
          
          
          
          
          
          
          
          if ((!trailingSpaces &&
               IsTableParentType(spaceEndIter.item().DesiredParentType())) ||
              (trailingSpaces && ourParentType != eTypeBlock)) {
            bool updateStart = (iter == endIter);
            endIter.DeleteItemsTo(spaceEndIter);
            NS_ASSERTION(trailingSpaces == endIter.IsDone(),
                         "These should match");

            if (updateStart) {
              iter = endIter;
            }

            if (trailingSpaces) {
              break; 
            }

            if (updateStart) {
              
              
              groupingParentType = iter.item().DesiredParentType();
            }
          }
        }

        
        
        
        
        
        
        
        
        
        prevParentType = endIter.item().DesiredParentType();
        if (prevParentType == ourParentType &&
            (endIter == spaceEndIter ||
             spaceEndIter.IsDone() ||
             !IsRubyParentType(groupingParentType) ||
             !IsRubyParentType(spaceEndIter.item().DesiredParentType()))) {
          
          break;
        }

        if (ourParentType == eTypeTable &&
            (prevParentType == eTypeColGroup) !=
            (groupingParentType == eTypeColGroup)) {
          
          
          break;
        }

        
        
        
        
        if (spaceEndIter != endIter &&
            !spaceEndIter.IsDone() &&
            ourParentType == spaceEndIter.item().DesiredParentType()) {
            endIter = spaceEndIter;
            break;
        }

        
        endIter = spaceEndIter;
        prevParentType = endIter.item().DesiredParentType();

        endIter.Next();
      } while (!endIter.IsDone());
    }

    if (iter == endIter) {
      
      continue;
    }

    
    
    ParentType wrapperType;
    switch (ourParentType) {
      case eTypeRow:
        
        
        wrapperType = eTypeBlock;
        break;
      case eTypeRowGroup:
        wrapperType = eTypeRow;
        break;
      case eTypeTable:
        
        wrapperType = groupingParentType == eTypeColGroup ?
          eTypeColGroup : eTypeRowGroup;
        break;
      case eTypeColGroup:
        MOZ_CRASH("Colgroups should be suppresing non-col child items");
      default: 
        NS_ASSERTION(ourParentType == eTypeBlock, "Unrecognized parent type");
        if (IsRubyParentType(groupingParentType)) {
          wrapperType = eTypeRuby;
        } else {
          NS_ASSERTION(IsTableParentType(groupingParentType),
                       "groupingParentType should be either Ruby or table");
          wrapperType = eTypeTable;
        }
    }

    nsStyleContext* parentStyle = aParentFrame->StyleContext();
    WrapItemsInPseudoParent(aParentFrame->GetContent(), parentStyle,
                            wrapperType, iter, endIter);

    
    
    
  } while (!iter.IsDone());
}






void
nsCSSFrameConstructor::WrapItemsInPseudoParent(nsIContent* aParentContent,
                                               nsStyleContext* aParentStyle,
                                               ParentType aWrapperType,
                                               FCItemIterator& aIter,
                                               const FCItemIterator& aEndIter)
{
  const PseudoParentData& pseudoData = sPseudoParentData[aWrapperType];
  nsIAtom* pseudoType = *pseudoData.mPseudoType;
  uint8_t parentDisplay = aParentStyle->StyleDisplay()->mDisplay;

  if (pseudoType == nsCSSAnonBoxes::table &&
      (parentDisplay == NS_STYLE_DISPLAY_INLINE ||
       parentDisplay == NS_STYLE_DISPLAY_RUBY_BASE ||
       parentDisplay == NS_STYLE_DISPLAY_RUBY_TEXT)) {
    pseudoType = nsCSSAnonBoxes::inlineTable;
  }

  already_AddRefed<nsStyleContext> wrapperStyle =
    mPresShell->StyleSet()->ResolveAnonymousBoxStyle(pseudoType, aParentStyle);
  FrameConstructionItem* newItem =
    new FrameConstructionItem(&pseudoData.mFCData,
                              
                              aParentContent,
                              
                              pseudoType,
                              
                              
                              
                              aIter.item().mNameSpaceID,
                              
                              nullptr,
                              wrapperStyle,
                              true, nullptr);

  const nsStyleDisplay* disp = newItem->mStyleContext->StyleDisplay();
  
  
  
  
  
  newItem->mIsAllInline = newItem->mHasInlineEnds =
    disp->IsInlineOutsideStyle();

  bool isRuby = disp->IsRubyDisplayType();
  
  
  newItem->mIsLineParticipant = isRuby;

  if (!isRuby) {
    
    
    newItem->mChildItems.SetLineBoundaryAtStart(true);
    newItem->mChildItems.SetLineBoundaryAtEnd(true);
  }
  
  
  newItem->mChildItems.SetParentHasNoXBLChildren(
      aIter.List()->ParentHasNoXBLChildren());

  
  
  aIter.AppendItemsToList(aEndIter, newItem->mChildItems);

  aIter.InsertItem(newItem);
}

void nsCSSFrameConstructor::CreateNeededPseudoSiblings(
    nsFrameConstructorState& aState,
    FrameConstructionItemList& aItems,
    nsIFrame* aParentFrame)
{
  if (aItems.IsEmpty() ||
      GetParentType(aParentFrame) != eTypeRuby) {
    return;
  }

  FCItemIterator iter(aItems);
  int firstDisplay = iter.item().mStyleContext->StyleDisplay()->mDisplay;
  if (firstDisplay == NS_STYLE_DISPLAY_RUBY_BASE_CONTAINER) {
    return;
  }
  NS_ASSERTION(firstDisplay == NS_STYLE_DISPLAY_RUBY_TEXT_CONTAINER,
               "Child of ruby frame should either a rbc or a rtc");

  const PseudoParentData& pseudoData =
    sPseudoParentData[eTypeRubyBaseContainer];
  already_AddRefed<nsStyleContext> pseudoStyle = mPresShell->StyleSet()->
    ResolveAnonymousBoxStyle(*pseudoData.mPseudoType,
                             aParentFrame->StyleContext());
  FrameConstructionItem* newItem =
    new FrameConstructionItem(&pseudoData.mFCData,
                              
                              aParentFrame->GetContent(),
                              
                              *pseudoData.mPseudoType,
                              
                              iter.item().mNameSpaceID,
                              
                              nullptr,
                              pseudoStyle,
                              true, nullptr);
  newItem->mIsAllInline = true;
  newItem->mChildItems.SetParentHasNoXBLChildren(true);
  iter.InsertItem(newItem);
}

inline void
nsCSSFrameConstructor::ConstructFramesFromItemList(nsFrameConstructorState& aState,
                                                   FrameConstructionItemList& aItems,
                                                   nsContainerFrame* aParentFrame,
                                                   nsFrameItems& aFrameItems)
{
  CreateNeededPseudoContainers(aState, aItems, aParentFrame);
  CreateNeededAnonFlexOrGridItems(aState, aItems, aParentFrame);
  CreateNeededPseudoInternalRubyBoxes(aState, aItems, aParentFrame);
  CreateNeededPseudoSiblings(aState, aItems, aParentFrame);

  aItems.SetTriedConstructingFrames();
  for (FCItemIterator iter(aItems); !iter.IsDone(); iter.Next()) {
    NS_ASSERTION(iter.item().DesiredParentType() == GetParentType(aParentFrame),
                 "Needed pseudos didn't get created; expect bad things");
    ConstructFramesFromItem(aState, iter, aParentFrame, aFrameItems);
  }

  NS_ASSERTION(!aState.mHavePendingPopupgroup,
               "Should have proccessed it by now");
}

void
nsCSSFrameConstructor::AddFCItemsForAnonymousContent(
            nsFrameConstructorState& aState,
            nsContainerFrame* aFrame,
            nsTArray<nsIAnonymousContentCreator::ContentInfo>& aAnonymousItems,
            FrameConstructionItemList& aItemsToConstruct,
            uint32_t aExtraFlags)
{
  for (uint32_t i = 0; i < aAnonymousItems.Length(); ++i) {
    nsIContent* content = aAnonymousItems[i].mContent;
#ifdef DEBUG
    nsIAnonymousContentCreator* creator = do_QueryFrame(aFrame);
    NS_ASSERTION(!creator || !creator->CreateFrameFor(content),
                 "If you need to use CreateFrameFor, you need to call "
                 "CreateAnonymousFrames manually and not follow the standard "
                 "ProcessChildren() codepath for this frame");
#endif
    
    MOZ_ASSERT(!(content->GetFlags() &
                 (NODE_DESCENDANTS_NEED_FRAMES | NODE_NEEDS_FRAME)),
               "Should not be marked as needing frames");
    MOZ_ASSERT(!content->IsElement() ||
               !(content->GetFlags() & ELEMENT_ALL_RESTYLE_FLAGS),
               "Should have no pending restyle flags");
    MOZ_ASSERT(!content->GetPrimaryFrame(),
               "Should have no existing frame");
    MOZ_ASSERT(!content->IsNodeOfType(nsINode::eCOMMENT) &&
               !content->IsNodeOfType(nsINode::ePROCESSING_INSTRUCTION),
               "Why is someone creating garbage anonymous content");

    nsRefPtr<nsStyleContext> styleContext;
    TreeMatchContext::AutoParentDisplayBasedStyleFixupSkipper
      parentDisplayBasedStyleFixupSkipper(aState.mTreeMatchContext);
    if (aAnonymousItems[i].mStyleContext) {
      styleContext = aAnonymousItems[i].mStyleContext.forget();
    } else {
      styleContext = ResolveStyleContext(aFrame, content, &aState);
    }

    nsTArray<nsIAnonymousContentCreator::ContentInfo>* anonChildren = nullptr;
    if (!aAnonymousItems[i].mChildren.IsEmpty()) {
      anonChildren = &aAnonymousItems[i].mChildren;
    }

    uint32_t flags = ITEM_ALLOW_XBL_BASE | ITEM_ALLOW_PAGE_BREAK |
                     ITEM_IS_ANONYMOUSCONTENTCREATOR_CONTENT | aExtraFlags;

    AddFrameConstructionItemsInternal(aState, content, aFrame,
                                      content->NodeInfo()->NameAtom(),
                                      content->GetNameSpaceID(),
                                      true, styleContext, flags,
                                      anonChildren, aItemsToConstruct);
  }
}

void
nsCSSFrameConstructor::ProcessChildren(nsFrameConstructorState& aState,
                                       nsIContent*              aContent,
                                       nsStyleContext*          aStyleContext,
                                       nsContainerFrame*        aFrame,
                                       const bool               aCanHaveGeneratedContent,
                                       nsFrameItems&            aFrameItems,
                                       const bool               aAllowBlockStyles,
                                       PendingBinding*          aPendingBinding,
                                       nsIFrame*                aPossiblyLeafFrame)
{
  NS_PRECONDITION(aFrame, "Must have parent frame here");
  NS_PRECONDITION(aFrame->GetContentInsertionFrame() == aFrame,
                  "Parent frame in ProcessChildren should be its own "
                  "content insertion frame");
  const uint32_t kMaxDepth = 2 * MAX_REFLOW_DEPTH;
  static_assert(kMaxDepth <= UINT16_MAX, "mCurrentDepth type is too narrow");
  AutoRestore<uint16_t> savedDepth(mCurrentDepth);
  if (mCurrentDepth != UINT16_MAX) {
    ++mCurrentDepth;
  }

  if (!aPossiblyLeafFrame) {
    aPossiblyLeafFrame = aFrame;
  }

  
  

  bool haveFirstLetterStyle = false, haveFirstLineStyle = false;
  if (aAllowBlockStyles) {
    ShouldHaveSpecialBlockStyle(aContent, aStyleContext, &haveFirstLetterStyle,
                                &haveFirstLineStyle);
  }

  
  nsFrameConstructorSaveState floatSaveState;
  if (ShouldSuppressFloatingOfDescendants(aFrame)) {
    aState.PushFloatContainingBlock(nullptr, floatSaveState);
  } else if (aFrame->IsFloatContainingBlock()) {
    aState.PushFloatContainingBlock(aFrame, floatSaveState);
  }

  nsFrameConstructorState::PendingBindingAutoPusher pusher(aState,
                                                           aPendingBinding);

  FrameConstructionItemList itemsToConstruct;

  
  
  if (aAllowBlockStyles && !haveFirstLetterStyle && !haveFirstLineStyle) {
    itemsToConstruct.SetLineBoundaryAtStart(true);
    itemsToConstruct.SetLineBoundaryAtEnd(true);
  }

  
  
  
  nsAutoTArray<nsIAnonymousContentCreator::ContentInfo, 4> anonymousItems;
  GetAnonymousContent(aContent, aPossiblyLeafFrame, anonymousItems);
#ifdef DEBUG
  for (uint32_t i = 0; i < anonymousItems.Length(); ++i) {
    MOZ_ASSERT(anonymousItems[i].mContent->IsRootOfAnonymousSubtree(),
               "Content should know it's an anonymous subtree");
  }
#endif
  AddFCItemsForAnonymousContent(aState, aFrame, anonymousItems,
                                itemsToConstruct);

  if (!aPossiblyLeafFrame->IsLeaf()) {
    
    
    
    
    
    nsStyleContext* styleContext;

    if (aCanHaveGeneratedContent) {
      aFrame->AddStateBits(NS_FRAME_MAY_HAVE_GENERATED_CONTENT);
      styleContext =
        nsFrame::CorrectStyleParentFrame(aFrame, nullptr)->StyleContext();
      
      CreateGeneratedContentItem(aState, aFrame, aContent, styleContext,
                                 nsCSSPseudoElements::ePseudo_before,
                                 itemsToConstruct);
    }

    const bool addChildItems = MOZ_LIKELY(mCurrentDepth < kMaxDepth);
    if (!addChildItems) {
      NS_WARNING("ProcessChildren max depth exceeded");
    }

    InsertionPoint insertion(aFrame, nullptr);
    FlattenedChildIterator iter(aContent);
    for (nsIContent* child = iter.GetNextChild(); child; child = iter.GetNextChild()) {
      
      
      
      
      
      insertion.mContainer = aContent;
      nsIContent* parent = child->GetParent();
      MOZ_ASSERT(parent, "Parent must be non-null because we are iterating children.");
      TreeMatchContext::AutoAncestorPusher ancestorPusher(aState.mTreeMatchContext);
      if (parent != aContent && parent->IsElement()) {
        insertion.mContainer = child->GetFlattenedTreeParent();
        MOZ_ASSERT(insertion.mContainer == GetInsertionPoint(parent, child).mContainer);
        if (aState.mTreeMatchContext.mAncestorFilter.HasFilter()) {
          ancestorPusher.PushAncestorAndStyleScope(parent->AsElement());
        } else {
          ancestorPusher.PushStyleScope(parent->AsElement());
        }
      }

      
      
      if (child->IsElement()) {
        child->UnsetFlags(ELEMENT_ALL_RESTYLE_FLAGS);
      }
      if (addChildItems) {
        AddFrameConstructionItems(aState, child, iter.XBLInvolved(), insertion,
                                  itemsToConstruct);
      } else {
        ClearLazyBits(child, child->GetNextSibling());
      }
    }
    itemsToConstruct.SetParentHasNoXBLChildren(!iter.XBLInvolved());

    if (aCanHaveGeneratedContent) {
      
      CreateGeneratedContentItem(aState, aFrame, aContent, styleContext,
                                 nsCSSPseudoElements::ePseudo_after,
                                 itemsToConstruct);
    }
  } else {
    ClearLazyBits(aContent->GetFirstChild(), nullptr);
  }

  ConstructFramesFromItemList(aState, itemsToConstruct, aFrame, aFrameItems);

  NS_ASSERTION(!aAllowBlockStyles || !aFrame->IsBoxFrame(),
               "can't be both block and box");

  if (haveFirstLetterStyle) {
    WrapFramesInFirstLetterFrame(aContent, aFrame, aFrameItems);
  }
  if (haveFirstLineStyle) {
    WrapFramesInFirstLineFrame(aState, aContent, aFrame, nullptr,
                               aFrameItems);
  }

  
  
  
  NS_ASSERTION(!haveFirstLineStyle || !aFrame->IsBoxFrame(),
               "Shouldn't have first-line style if we're a box");
  NS_ASSERTION(!aFrame->IsBoxFrame() ||
               itemsToConstruct.AnyItemsNeedBlockParent() ==
                 (AnyKidsNeedBlockParent(aFrameItems.FirstChild()) != nullptr),
               "Something went awry in our block parent calculations");

  if (aFrame->IsBoxFrame() && itemsToConstruct.AnyItemsNeedBlockParent()) {
    
    
    
    nsStyleContext *frameStyleContext = aFrame->StyleContext();
    
    if (!aFrame->IsGeneratedContentFrame() &&
        mPresShell->GetPresContext()->IsChrome()) {
      nsIContent *badKid = AnyKidsNeedBlockParent(aFrameItems.FirstChild());
      nsDependentAtomString parentTag(aContent->NodeInfo()->NameAtom()),
                            kidTag(badKid->NodeInfo()->NameAtom());
      const char16_t* params[] = { parentTag.get(), kidTag.get() };
      const nsStyleDisplay *display = frameStyleContext->StyleDisplay();
      const char *message =
        (display->mDisplay == NS_STYLE_DISPLAY_INLINE_BOX)
          ? "NeededToWrapXULInlineBox" : "NeededToWrapXUL";
      nsContentUtils::ReportToConsole(nsIScriptError::warningFlag,
                                      NS_LITERAL_CSTRING("Layout: FrameConstructor"),
                                      mDocument,
                                      nsContentUtils::eXUL_PROPERTIES,
                                      message,
                                      params, ArrayLength(params));
    }

    nsRefPtr<nsStyleContext> blockSC = mPresShell->StyleSet()->
      ResolveAnonymousBoxStyle(nsCSSAnonBoxes::mozXULAnonymousBlock,
                               frameStyleContext);
    nsBlockFrame* blockFrame = NS_NewBlockFrame(mPresShell, blockSC);
    
    
    

    InitAndRestoreFrame(aState, aContent, aFrame, blockFrame, false);

    NS_ASSERTION(!blockFrame->HasView(), "need to do view reparenting");
    ReparentFrames(this, blockFrame, aFrameItems);

    blockFrame->SetInitialChildList(kPrincipalList, aFrameItems);
    NS_ASSERTION(aFrameItems.IsEmpty(), "How did that happen?");
    aFrameItems.Clear();
    aFrameItems.AddChild(blockFrame);

    aFrame->AddStateBits(NS_STATE_BOX_WRAPS_KIDS_IN_BLOCK);
  }
}















void
nsCSSFrameConstructor::WrapFramesInFirstLineFrame(
  nsFrameConstructorState& aState,
  nsIContent*              aBlockContent,
  nsContainerFrame*        aBlockFrame,
  nsFirstLineFrame*        aLineFrame,
  nsFrameItems&            aFrameItems)
{
  
  nsFrameList::FrameLinkEnumerator link(aFrameItems);
  while (!link.AtEnd() && link.NextFrame()->IsInlineOutside()) {
    link.Next();
  }

  nsFrameList firstLineChildren = aFrameItems.ExtractHead(link);

  if (firstLineChildren.IsEmpty()) {
    
    return;
  }

  if (!aLineFrame) {
    
    nsStyleContext* parentStyle =
      nsFrame::CorrectStyleParentFrame(aBlockFrame,
                                       nsCSSPseudoElements::firstLine)->
        StyleContext();
    nsRefPtr<nsStyleContext> firstLineStyle = GetFirstLineStyle(aBlockContent,
                                                                parentStyle);

    aLineFrame = NS_NewFirstLineFrame(mPresShell, firstLineStyle);

    
    InitAndRestoreFrame(aState, aBlockContent, aBlockFrame, aLineFrame);

    
    
    
    aFrameItems.InsertFrame(nullptr, nullptr, aLineFrame);

    NS_ASSERTION(aLineFrame->StyleContext() == firstLineStyle,
                 "Bogus style context on line frame");
  }

  
  ReparentFrames(this, aLineFrame, firstLineChildren);
  if (aLineFrame->PrincipalChildList().IsEmpty() &&
      (aLineFrame->GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
    aLineFrame->SetInitialChildList(kPrincipalList, firstLineChildren);
  } else {
    AppendFrames(aLineFrame, kPrincipalList, firstLineChildren);
  }
}




void
nsCSSFrameConstructor::AppendFirstLineFrames(
  nsFrameConstructorState& aState,
  nsIContent*              aBlockContent,
  nsContainerFrame*        aBlockFrame,
  nsFrameItems&            aFrameItems)
{
  
  
  const nsFrameList& blockKids = aBlockFrame->PrincipalChildList();
  if (blockKids.IsEmpty()) {
    WrapFramesInFirstLineFrame(aState, aBlockContent,
                               aBlockFrame, nullptr, aFrameItems);
    return;
  }

  
  
  nsIFrame* lastBlockKid = blockKids.LastChild();
  if (lastBlockKid->GetType() != nsGkAtoms::lineFrame) {
    
    
    
    
    return;
  }

  nsFirstLineFrame* lineFrame = static_cast<nsFirstLineFrame*>(lastBlockKid);
  WrapFramesInFirstLineFrame(aState, aBlockContent, aBlockFrame,
                             lineFrame, aFrameItems);
}




nsresult
nsCSSFrameConstructor::InsertFirstLineFrames(
  nsFrameConstructorState& aState,
  nsIContent*              aContent,
  nsIFrame*                aBlockFrame,
  nsContainerFrame**       aParentFrame,
  nsIFrame*                aPrevSibling,
  nsFrameItems&            aFrameItems)
{
  nsresult rv = NS_OK;
  
  
  
  
#if 0
  nsIFrame* parentFrame = *aParentFrame;
  nsIFrame* newFrame = aFrameItems.childList;
  bool isInline = IsInlineOutside(newFrame);

  if (!aPrevSibling) {
    
    
    nsIFrame* firstBlockKid = aBlockFrame->GetFirstPrincipalChild();
    if (firstBlockKid->GetType() == nsGkAtoms::lineFrame) {
      
      nsIFrame* lineFrame = firstBlockKid;

      if (isInline) {
        
        ReparentFrame(this, lineFrame, newFrame);
        InsertFrames(lineFrame, kPrincipalList, nullptr, newFrame);

        
        
        aFrameItems.childList = nullptr;
        aFrameItems.lastChild = nullptr;
      }
      else {
        
        
        
      }
    }
    else {
      
      if (isInline) {
        
        nsIFrame* lineFrame = NS_NewFirstLineFrame(firstLineStyle);

        if (NS_SUCCEEDED(rv)) {
          
          nsStyleContext* parentStyle =
            nsFrame::CorrectStyleParentFrame(aBlockFrame,
                                             nsCSSPseudoElements::firstLine)->
              StyleContext();
          nsRefPtr<nsStyleContext> firstLineStyle =
            GetFirstLineStyle(aContent, parentStyle);

          
          InitAndRestoreFrame(aState, aContent, aBlockFrame, lineFrame);

          
          
          aFrameItems.childList = lineFrame;
          aFrameItems.lastChild = lineFrame;

          
          
          NS_ASSERTION(lineFrame->StyleContext() == firstLineStyle,
                       "Bogus style context on line frame");
          ReparentFrame(aPresContext, lineFrame, newFrame);
          lineFrame->SetInitialChildList(kPrincipalList, newFrame);
        }
      }
      else {
        
        
      }
    }
  }
  else {
    
    nsIFrame* prevSiblingParent = aPrevSibling->GetParent();
    if (prevSiblingParent == aBlockFrame) {
      
      
      
      
    }
    else {
      
      
      
      
      if (isInline) {
        
        
      }
      else {
        
        
        
        
        
        nsIFrame* nextSibling = aPrevSibling->GetNextSibling();
        nsIFrame* nextLineFrame = prevSiblingParent->GetNextInFlow();
        if (nextSibling || nextLineFrame) {
          
          
          
          if (nextSibling) {
            nsLineFrame* lineFrame = (nsLineFrame*) prevSiblingParent;
            nsFrameList tail = lineFrame->StealFramesAfter(aPrevSibling);
            
          }

          nsLineFrame* nextLineFrame = (nsLineFrame*) lineFrame;
          for (;;) {
            nextLineFrame = nextLineFrame->GetNextInFlow();
            if (!nextLineFrame) {
              break;
            }
            nsIFrame* kids = nextLineFrame->GetFirstPrincipalChild();
          }
        }
        else {
          
          
          ReparentFrame(this, aBlockFrame, newFrame);
          InsertFrames(aBlockFrame, kPrincipalList,
                       prevSiblingParent, newFrame);
          aFrameItems.childList = nullptr;
          aFrameItems.lastChild = nullptr;
        }
      }
    }
  }

#endif
  return rv;
}







static int32_t
FirstLetterCount(const nsTextFragment* aFragment)
{
  int32_t count = 0;
  int32_t firstLetterLength = 0;

  int32_t i, n = aFragment->GetLength();
  for (i = 0; i < n; i++) {
    char16_t ch = aFragment->CharAt(i);
    
    if (dom::IsSpaceCharacter(ch)) {
      if (firstLetterLength) {
        break;
      }
      count++;
      continue;
    }
    
    if ((ch == '\'') || (ch == '\"')) {
      if (firstLetterLength) {
        break;
      }
      
      firstLetterLength = 1;
    }
    else {
      count++;
      break;
    }
  }

  return count;
}

static bool
NeedFirstLetterContinuation(nsIContent* aContent)
{
  NS_PRECONDITION(aContent, "null ptr");

  bool result = false;
  if (aContent) {
    const nsTextFragment* frag = aContent->GetText();
    if (frag) {
      int32_t flc = FirstLetterCount(frag);
      int32_t tl = frag->GetLength();
      if (flc < tl) {
        result = true;
      }
    }
  }
  return result;
}

static bool IsFirstLetterContent(nsIContent* aContent)
{
  return aContent->TextLength() &&
         !aContent->TextIsOnlyWhitespace();
}




void
nsCSSFrameConstructor::CreateFloatingLetterFrame(
  nsFrameConstructorState& aState,
  nsContainerFrame* aBlockFrame,
  nsIContent* aTextContent,
  nsIFrame* aTextFrame,
  nsContainerFrame* aParentFrame,
  nsStyleContext* aStyleContext,
  nsFrameItems& aResult)
{
  nsFirstLetterFrame* letterFrame =
    NS_NewFirstLetterFrame(mPresShell, aStyleContext);
  
  
  
  nsIContent* letterContent = aTextContent->GetParent();
  nsContainerFrame* containingBlock = aState.GetGeometricParent(
    aStyleContext->StyleDisplay(), aParentFrame);
  InitAndRestoreFrame(aState, letterContent, containingBlock, letterFrame);

  
  
  
  
  nsRefPtr<nsStyleContext> textSC;
  nsStyleSet* styleSet = mPresShell->StyleSet();
  textSC = styleSet->ResolveStyleForNonElement(aStyleContext);
  aTextFrame->SetStyleContextWithoutNotification(textSC);
  InitAndRestoreFrame(aState, aTextContent, letterFrame, aTextFrame);

  
  SetInitialSingleChild(letterFrame, aTextFrame);

  
  
  
  nsIFrame* nextTextFrame = nullptr;
  if (NeedFirstLetterContinuation(aTextContent)) {
    
    nextTextFrame =
      CreateContinuingFrame(aState.mPresContext, aTextFrame, aParentFrame);
    
    nsStyleContext* parentStyleContext = aStyleContext->GetParent();
    if (parentStyleContext) {
      nsRefPtr<nsStyleContext> newSC;
      newSC = styleSet->ResolveStyleForNonElement(parentStyleContext);
      nextTextFrame->SetStyleContext(newSC);
    }
  }

  NS_ASSERTION(aResult.IsEmpty(), "aResult should be an empty nsFrameItems!");
  
  
  
  nsFrameList::FrameLinkEnumerator link(aState.mFloatedItems);
  while (!link.AtEnd() && link.NextFrame()->GetParent() != containingBlock) {
    link.Next();
  }

  aState.AddChild(letterFrame, aResult, letterContent, aStyleContext,
                  aParentFrame, false, true, false, true,
                  link.PrevFrame());

  if (nextTextFrame) {
    aResult.AddChild(nextTextFrame);
  }
}





void
nsCSSFrameConstructor::CreateLetterFrame(nsContainerFrame* aBlockFrame,
                                         nsContainerFrame* aBlockContinuation,
                                         nsIContent* aTextContent,
                                         nsContainerFrame* aParentFrame,
                                         nsFrameItems& aResult)
{
  NS_PRECONDITION(aTextContent->IsNodeOfType(nsINode::eTEXT),
                  "aTextContent isn't text");
  NS_ASSERTION(nsLayoutUtils::GetAsBlock(aBlockFrame),
                 "Not a block frame?");

  
  nsStyleContext* parentStyleContext =
    nsFrame::CorrectStyleParentFrame(aParentFrame,
                                     nsCSSPseudoElements::firstLetter)->
      StyleContext();

  
  
  nsIContent* blockContent = aBlockFrame->GetContent();

  
  nsRefPtr<nsStyleContext> sc = GetFirstLetterStyle(blockContent,
                                                    parentStyleContext);
  if (sc) {
    nsRefPtr<nsStyleContext> textSC;
    textSC = mPresShell->StyleSet()->ResolveStyleForNonElement(sc);

    
    
    
    
    
    
    
    aTextContent->SetPrimaryFrame(nullptr);
    nsIFrame* textFrame = NS_NewTextFrame(mPresShell, textSC);

    NS_ASSERTION(aBlockContinuation == GetFloatContainingBlock(aParentFrame),
                 "Containing block is confused");
    nsFrameConstructorState state(mPresShell,
                                  GetAbsoluteContainingBlock(aParentFrame, FIXED_POS),
                                  GetAbsoluteContainingBlock(aParentFrame, ABS_POS),
                                  aBlockContinuation);

    
    const nsStyleDisplay* display = sc->StyleDisplay();
    if (display->IsFloatingStyle() && !aParentFrame->IsSVGText()) {
      
      CreateFloatingLetterFrame(state, aBlockFrame, aTextContent, textFrame,
                                aParentFrame, sc, aResult);
    }
    else {
      
      nsFirstLetterFrame* letterFrame = NS_NewFirstLetterFrame(mPresShell, sc);

      
      
      
      nsIContent* letterContent = aTextContent->GetParent();
      letterFrame->Init(letterContent, aParentFrame, nullptr);

      InitAndRestoreFrame(state, aTextContent, letterFrame, textFrame);

      SetInitialSingleChild(letterFrame, textFrame);
      aResult.Clear();
      aResult.AddChild(letterFrame);
      NS_ASSERTION(!aBlockFrame->GetPrevContinuation(),
                   "should have the first continuation here");
      aBlockFrame->AddStateBits(NS_BLOCK_HAS_FIRST_LETTER_CHILD);
    }
    aTextContent->SetPrimaryFrame(textFrame);
  }
}

void
nsCSSFrameConstructor::WrapFramesInFirstLetterFrame(
  nsIContent*              aBlockContent,
  nsContainerFrame*        aBlockFrame,
  nsFrameItems&            aBlockFrames)
{
  aBlockFrame->AddStateBits(NS_BLOCK_HAS_FIRST_LETTER_STYLE);

  nsContainerFrame* parentFrame = nullptr;
  nsIFrame* textFrame = nullptr;
  nsIFrame* prevFrame = nullptr;
  nsFrameItems letterFrames;
  bool stopLooking = false;
  WrapFramesInFirstLetterFrame(aBlockFrame, aBlockFrame, aBlockFrame,
                               aBlockFrames.FirstChild(),
                               &parentFrame, &textFrame, &prevFrame,
                               letterFrames, &stopLooking);
  if (parentFrame) {
    if (parentFrame == aBlockFrame) {
      
      
      aBlockFrames.DestroyFrame(textFrame);
      aBlockFrames.InsertFrames(nullptr, prevFrame, letterFrames);
    }
    else {
      
      RemoveFrame(kPrincipalList, textFrame);

      
      parentFrame->InsertFrames(kPrincipalList, prevFrame, letterFrames);
    }
  }
}

void
nsCSSFrameConstructor::WrapFramesInFirstLetterFrame(
  nsContainerFrame*        aBlockFrame,
  nsContainerFrame*        aBlockContinuation,
  nsContainerFrame*        aParentFrame,
  nsIFrame*                aParentFrameList,
  nsContainerFrame**       aModifiedParent,
  nsIFrame**               aTextFrame,
  nsIFrame**               aPrevFrame,
  nsFrameItems&            aLetterFrames,
  bool*                    aStopLooking)
{
  nsIFrame* prevFrame = nullptr;
  nsIFrame* frame = aParentFrameList;

  while (frame) {
    nsIFrame* nextFrame = frame->GetNextSibling();

    nsIAtom* frameType = frame->GetType();
    if (nsGkAtoms::textFrame == frameType) {
      
      nsIContent* textContent = frame->GetContent();
      if (IsFirstLetterContent(textContent)) {
        
        CreateLetterFrame(aBlockFrame, aBlockContinuation, textContent,
                          aParentFrame, aLetterFrames);

        
        *aModifiedParent = aParentFrame;
        *aTextFrame = frame;
        *aPrevFrame = prevFrame;
        *aStopLooking = true;
        return;
      }
    }
    else if (IsInlineFrame(frame) && frameType != nsGkAtoms::brFrame) {
      nsIFrame* kids = frame->GetFirstPrincipalChild();
      WrapFramesInFirstLetterFrame(aBlockFrame, aBlockContinuation,
                                   static_cast<nsContainerFrame*>(frame),
                                   kids, aModifiedParent, aTextFrame,
                                   aPrevFrame, aLetterFrames, aStopLooking);
      if (*aStopLooking) {
        return;
      }
    }
    else {
      
      
      
      
      
      
      *aStopLooking = true;
      break;
    }

    prevFrame = frame;
    frame = nextFrame;
  }
}

static nsIFrame*
FindFirstLetterFrame(nsIFrame* aFrame, nsIFrame::ChildListID aListID)
{
  nsFrameList list = aFrame->GetChildList(aListID);
  for (nsFrameList::Enumerator e(list); !e.AtEnd(); e.Next()) {
    if (nsGkAtoms::letterFrame == e.get()->GetType()) {
      return e.get();
    }
  }
  return nullptr;
}

nsresult
nsCSSFrameConstructor::RemoveFloatingFirstLetterFrames(
  nsPresContext* aPresContext,
  nsIPresShell* aPresShell,
  nsIFrame* aBlockFrame,
  bool* aStopLooking)
{
  
  nsIFrame* floatFrame =
    ::FindFirstLetterFrame(aBlockFrame, nsIFrame::kFloatList);
  if (!floatFrame) {
    floatFrame =
      ::FindFirstLetterFrame(aBlockFrame, nsIFrame::kPushedFloatsList);
    if (!floatFrame) {
      return NS_OK;
    }
  }

  
  
  nsIFrame* textFrame = floatFrame->GetFirstPrincipalChild();
  if (!textFrame) {
    return NS_OK;
  }

  
  nsPlaceholderFrame* placeholderFrame = GetPlaceholderFrameFor(floatFrame);
  if (!placeholderFrame) {
    
    return NS_OK;
  }
  nsContainerFrame* parentFrame = placeholderFrame->GetParent();
  if (!parentFrame) {
    
    return NS_OK;
  }

  
  
  
  nsStyleContext* parentSC = parentFrame->StyleContext();
  nsIContent* textContent = textFrame->GetContent();
  if (!textContent) {
    return NS_OK;
  }
  nsRefPtr<nsStyleContext> newSC;
  newSC = aPresShell->StyleSet()->ResolveStyleForNonElement(parentSC);
  nsIFrame* newTextFrame = NS_NewTextFrame(aPresShell, newSC);
  newTextFrame->Init(textContent, parentFrame, nullptr);

  
  
  nsIFrame* frameToDelete = textFrame->LastContinuation();
  while (frameToDelete != textFrame) {
    nsIFrame* nextFrameToDelete = frameToDelete->GetPrevContinuation();
    RemoveFrame(kPrincipalList, frameToDelete);
    frameToDelete = nextFrameToDelete;
  }

  nsIFrame* prevSibling = placeholderFrame->GetPrevSibling();

  
#ifdef NOISY_FIRST_LETTER
  printf("RemoveFloatingFirstLetterFrames: textContent=%p oldTextFrame=%p newTextFrame=%p\n",
         textContent.get(), textFrame, newTextFrame);
#endif

  
  RemoveFrame(kPrincipalList, placeholderFrame);

  
  
  textContent->SetPrimaryFrame(newTextFrame);

  
  bool offsetsNeedFixing =
    prevSibling && prevSibling->GetType() == nsGkAtoms::textFrame;
  if (offsetsNeedFixing) {
    prevSibling->AddStateBits(TEXT_OFFSETS_NEED_FIXING);
  }

  
  nsFrameList textList(newTextFrame, newTextFrame);
  InsertFrames(parentFrame, kPrincipalList, prevSibling, textList);

  if (offsetsNeedFixing) {
    prevSibling->RemoveStateBits(TEXT_OFFSETS_NEED_FIXING);
  }

  return NS_OK;
}

nsresult
nsCSSFrameConstructor::RemoveFirstLetterFrames(nsPresContext* aPresContext,
                                               nsIPresShell* aPresShell,
                                               nsContainerFrame* aFrame,
                                               nsContainerFrame* aBlockFrame,
                                               bool* aStopLooking)
{
  nsIFrame* prevSibling = nullptr;
  nsIFrame* kid = aFrame->GetFirstPrincipalChild();

  while (kid) {
    if (nsGkAtoms::letterFrame == kid->GetType()) {
      
      nsIFrame* textFrame = kid->GetFirstPrincipalChild();
      if (!textFrame) {
        break;
      }

      
      nsStyleContext* parentSC = aFrame->StyleContext();
      if (!parentSC) {
        break;
      }
      nsIContent* textContent = textFrame->GetContent();
      if (!textContent) {
        break;
      }
      nsRefPtr<nsStyleContext> newSC;
      newSC = aPresShell->StyleSet()->ResolveStyleForNonElement(parentSC);
      textFrame = NS_NewTextFrame(aPresShell, newSC);
      textFrame->Init(textContent, aFrame, nullptr);

      
      RemoveFrame(kPrincipalList, kid);

      
      
      textContent->SetPrimaryFrame(textFrame);

      
      bool offsetsNeedFixing =
        prevSibling && prevSibling->GetType() == nsGkAtoms::textFrame;
      if (offsetsNeedFixing) {
        prevSibling->AddStateBits(TEXT_OFFSETS_NEED_FIXING);
      }

      
      nsFrameList textList(textFrame, textFrame);
      InsertFrames(aFrame, kPrincipalList, prevSibling, textList);

      if (offsetsNeedFixing) {
        prevSibling->RemoveStateBits(TEXT_OFFSETS_NEED_FIXING);
      }

      *aStopLooking = true;
      NS_ASSERTION(!aBlockFrame->GetPrevContinuation(),
                   "should have the first continuation here");
      aBlockFrame->RemoveStateBits(NS_BLOCK_HAS_FIRST_LETTER_CHILD);
      break;
    }
    else if (IsInlineFrame(kid)) {
      nsContainerFrame* kidAsContainerFrame = do_QueryFrame(kid);
      if (kidAsContainerFrame) {
        
        RemoveFirstLetterFrames(aPresContext, aPresShell,
                                kidAsContainerFrame,
                                aBlockFrame, aStopLooking);
        if (*aStopLooking) {
          break;
        }
      }
    }
    prevSibling = kid;
    kid = kid->GetNextSibling();
  }

  return NS_OK;
}

nsresult
nsCSSFrameConstructor::RemoveLetterFrames(nsPresContext* aPresContext,
                                          nsIPresShell* aPresShell,
                                          nsContainerFrame* aBlockFrame)
{
  aBlockFrame =
    static_cast<nsContainerFrame*>(aBlockFrame->FirstContinuation());
  nsContainerFrame* continuation = aBlockFrame;

  bool stopLooking = false;
  nsresult rv;
  do {
    rv = RemoveFloatingFirstLetterFrames(aPresContext, aPresShell,
                                         continuation, &stopLooking);
    if (NS_SUCCEEDED(rv) && !stopLooking) {
      rv = RemoveFirstLetterFrames(aPresContext, aPresShell,
                                   continuation, aBlockFrame, &stopLooking);
    }
    if (stopLooking) {
      break;
    }
    continuation =
      static_cast<nsContainerFrame*>(continuation->GetNextContinuation());
  }  while (continuation);
  return rv;
}


void
nsCSSFrameConstructor::RecoverLetterFrames(nsContainerFrame* aBlockFrame)
{
  aBlockFrame =
    static_cast<nsContainerFrame*>(aBlockFrame->FirstContinuation());
  nsContainerFrame* continuation = aBlockFrame;

  nsContainerFrame* parentFrame = nullptr;
  nsIFrame* textFrame = nullptr;
  nsIFrame* prevFrame = nullptr;
  nsFrameItems letterFrames;
  bool stopLooking = false;
  do {
    
    continuation->AddStateBits(NS_BLOCK_HAS_FIRST_LETTER_STYLE);
    WrapFramesInFirstLetterFrame(aBlockFrame, continuation, continuation,
                                 continuation->GetFirstPrincipalChild(),
                                 &parentFrame, &textFrame, &prevFrame,
                                 letterFrames, &stopLooking);
    if (stopLooking) {
      break;
    }
    continuation =
      static_cast<nsContainerFrame*>(continuation->GetNextContinuation());
  } while (continuation);

  if (parentFrame) {
    
    RemoveFrame(kPrincipalList, textFrame);

    
    parentFrame->InsertFrames(kPrincipalList, prevFrame, letterFrames);
  }
}





nsresult
nsCSSFrameConstructor::CreateListBoxContent(nsPresContext*         aPresContext,
                                            nsContainerFrame*      aParentFrame,
                                            nsIFrame*              aPrevFrame,
                                            nsIContent*            aChild,
                                            nsIFrame**             aNewFrame,
                                            bool                   aIsAppend,
                                            bool                   aIsScrollbar,
                                            nsILayoutHistoryState* aFrameState)
{
#ifdef MOZ_XUL
  nsresult rv = NS_OK;

  
  if (nullptr != aParentFrame) {
    nsFrameItems            frameItems;
    nsFrameConstructorState state(mPresShell, GetAbsoluteContainingBlock(aParentFrame, FIXED_POS),
                                  GetAbsoluteContainingBlock(aParentFrame, ABS_POS),
                                  GetFloatContainingBlock(aParentFrame),
                                  mTempFrameTreeState);

    
    

    nsRefPtr<nsStyleContext> styleContext;
    styleContext = ResolveStyleContext(aParentFrame, aChild, &state);

    
    
    const nsStyleDisplay* display = styleContext->StyleDisplay();

    if (NS_STYLE_DISPLAY_NONE == display->mDisplay) {
      *aNewFrame = nullptr;
      return NS_OK;
    }

    BeginUpdate();

    FrameConstructionItemList items;
    AddFrameConstructionItemsInternal(state, aChild, aParentFrame,
                                      aChild->NodeInfo()->NameAtom(),
                                      aChild->GetNameSpaceID(),
                                      true, styleContext,
                                      ITEM_ALLOW_XBL_BASE, nullptr, items);
    ConstructFramesFromItemList(state, items, aParentFrame, frameItems);

    nsIFrame* newFrame = frameItems.FirstChild();
    *aNewFrame = newFrame;

    if (newFrame) {
      
      if (aIsAppend)
        rv = ((nsListBoxBodyFrame*)aParentFrame)->ListBoxAppendFrames(frameItems);
      else
        rv = ((nsListBoxBodyFrame*)aParentFrame)->ListBoxInsertFrames(aPrevFrame, frameItems);
    }

    EndUpdate();

#ifdef ACCESSIBILITY
    if (newFrame) {
      nsAccessibilityService* accService = nsIPresShell::AccService();
      if (accService) {
        accService->ContentRangeInserted(mPresShell, aChild->GetParent(),
                                         aChild, aChild->GetNextSibling());
      }
    }
#endif
  }

  return rv;
#else
  return NS_ERROR_FAILURE;
#endif
}



void
nsCSSFrameConstructor::ConstructBlock(nsFrameConstructorState& aState,
                                      const nsStyleDisplay*    aDisplay,
                                      nsIContent*              aContent,
                                      nsContainerFrame*        aParentFrame,
                                      nsContainerFrame*        aContentParentFrame,
                                      nsStyleContext*          aStyleContext,
                                      nsContainerFrame**       aNewFrame,
                                      nsFrameItems&            aFrameItems,
                                      nsIFrame*                aPositionedFrameForAbsPosContainer,
                                      PendingBinding*          aPendingBinding)
{
  
  nsContainerFrame* blockFrame = *aNewFrame;
  NS_ASSERTION(blockFrame->GetType() == nsGkAtoms::blockFrame, "not a block frame?");
  nsContainerFrame* parent = aParentFrame;
  nsRefPtr<nsStyleContext> blockStyle = aStyleContext;
  const nsStyleColumn* columns = aStyleContext->StyleColumn();

  if (columns->mColumnCount != NS_STYLE_COLUMN_COUNT_AUTO
      || columns->mColumnWidth.GetUnit() != eStyleUnit_Auto) {
    nsContainerFrame* columnSetFrame =
      NS_NewColumnSetFrame(mPresShell, aStyleContext, nsFrameState(0));

    InitAndRestoreFrame(aState, aContent, aParentFrame, columnSetFrame);
    blockStyle = mPresShell->StyleSet()->
      ResolveAnonymousBoxStyle(nsCSSAnonBoxes::columnContent, aStyleContext);
    parent = columnSetFrame;
    *aNewFrame = columnSetFrame;
    if (aPositionedFrameForAbsPosContainer == blockFrame) {
      aPositionedFrameForAbsPosContainer = columnSetFrame;
    }

    SetInitialSingleChild(columnSetFrame, blockFrame);
  }

  blockFrame->SetStyleContextWithoutNotification(blockStyle);
  InitAndRestoreFrame(aState, aContent, parent, blockFrame);

  aState.AddChild(*aNewFrame, aFrameItems, aContent, aStyleContext,
                  aContentParentFrame ? aContentParentFrame :
                                        aParentFrame);
  if (!mRootElementFrame) {
    
    
    mRootElementFrame = *aNewFrame;
  }

  
  
  
  
  
  
  
  nsFrameConstructorSaveState absoluteSaveState;
  (*aNewFrame)->AddStateBits(NS_FRAME_CAN_HAVE_ABSPOS_CHILDREN);
  if (aPositionedFrameForAbsPosContainer) {
    
    aState.PushAbsoluteContainingBlock(*aNewFrame, aPositionedFrameForAbsPosContainer, absoluteSaveState);
  }

  
  nsFrameItems childItems;
  ProcessChildren(aState, aContent, aStyleContext, blockFrame, true,
                  childItems, true, aPendingBinding);

  
  blockFrame->SetInitialChildList(kPrincipalList, childItems);
}

nsIFrame*
nsCSSFrameConstructor::ConstructInline(nsFrameConstructorState& aState,
                                       FrameConstructionItem&   aItem,
                                       nsContainerFrame*        aParentFrame,
                                       const nsStyleDisplay*    aDisplay,
                                       nsFrameItems&            aFrameItems)
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  nsIContent* const content = aItem.mContent;
  nsStyleContext* const styleContext = aItem.mStyleContext;

  bool positioned =
    NS_STYLE_DISPLAY_INLINE == aDisplay->mDisplay &&
    aDisplay->IsRelativelyPositionedStyle() &&
    !aParentFrame->IsSVGText();

  nsInlineFrame* newFrame = NS_NewInlineFrame(mPresShell, styleContext);

  
  InitAndRestoreFrame(aState, content, aParentFrame, newFrame);

  
  newFrame->AddStateBits(NS_FRAME_MAY_HAVE_GENERATED_CONTENT);

  nsFrameConstructorSaveState absoluteSaveState;  
                                                  
                                                  

  newFrame->AddStateBits(NS_FRAME_CAN_HAVE_ABSPOS_CHILDREN);
  if (positioned) {
    
    
    aState.PushAbsoluteContainingBlock(newFrame, newFrame, absoluteSaveState);
  }

  
  nsFrameItems childItems;
  ConstructFramesFromItemList(aState, aItem.mChildItems, newFrame, childItems);

  nsFrameList::FrameLinkEnumerator firstBlockEnumerator(childItems);
  if (!aItem.mIsAllInline) {
    FindFirstBlock(firstBlockEnumerator);
  }

  if (aItem.mIsAllInline || firstBlockEnumerator.AtEnd()) {
    
    
    
    
    
    
    newFrame->SetInitialChildList(kPrincipalList, childItems);
    aState.AddChild(newFrame, aFrameItems, content, styleContext, aParentFrame);
    return newFrame;
  }

  
  

  
  nsFrameList firstInlineKids = childItems.ExtractHead(firstBlockEnumerator);
  newFrame->SetInitialChildList(kPrincipalList, firstInlineKids);

  aFrameItems.AddChild(newFrame);

  CreateIBSiblings(aState, newFrame, positioned, childItems, aFrameItems);

  return newFrame;
}

void
nsCSSFrameConstructor::CreateIBSiblings(nsFrameConstructorState& aState,
                                        nsContainerFrame* aInitialInline,
                                        bool aIsPositioned,
                                        nsFrameItems& aChildItems,
                                        nsFrameItems& aSiblings)
{
  nsIContent* content = aInitialInline->GetContent();
  nsStyleContext* styleContext = aInitialInline->StyleContext();
  nsContainerFrame* parentFrame = aInitialInline->GetParent();

  
  
  
  
  
  
  nsRefPtr<nsStyleContext> blockSC =
    mPresShell->StyleSet()->
      ResolveAnonymousBoxStyle(aIsPositioned ?
                                 nsCSSAnonBoxes::mozAnonymousPositionedBlock :
                                 nsCSSAnonBoxes::mozAnonymousBlock,
                               styleContext);

  nsContainerFrame* lastNewInline =
    static_cast<nsContainerFrame*>(aInitialInline->FirstContinuation());
  do {
    
    
    NS_PRECONDITION(aChildItems.NotEmpty(), "Should have child items");
    NS_PRECONDITION(!aChildItems.FirstChild()->IsInlineOutside(),
                    "Must have list starting with block");

    
    
    
    nsBlockFrame* blockFrame = NS_NewBlockFrame(mPresShell, blockSC);
    InitAndRestoreFrame(aState, content, parentFrame, blockFrame, false);

    
    
    nsFrameList::FrameLinkEnumerator firstNonBlock =
      FindFirstNonBlock(aChildItems);
    nsFrameList blockKids = aChildItems.ExtractHead(firstNonBlock);

    MoveChildrenTo(aState.mPresContext, aInitialInline, blockFrame, blockKids);

    SetFrameIsIBSplit(lastNewInline, blockFrame);
    aSiblings.AddChild(blockFrame);

    
    
    nsInlineFrame* inlineFrame = NS_NewInlineFrame(mPresShell, styleContext);
    InitAndRestoreFrame(aState, content, parentFrame, inlineFrame, false);
    inlineFrame->AddStateBits(NS_FRAME_MAY_HAVE_GENERATED_CONTENT |
                              NS_FRAME_CAN_HAVE_ABSPOS_CHILDREN);
    if (aIsPositioned) {
      inlineFrame->MarkAsAbsoluteContainingBlock();
    }

    if (aChildItems.NotEmpty()) {
      nsFrameList::FrameLinkEnumerator firstBlock(aChildItems);
      FindFirstBlock(firstBlock);
      nsFrameList inlineKids = aChildItems.ExtractHead(firstBlock);

      MoveChildrenTo(aState.mPresContext, aInitialInline, inlineFrame,
                     inlineKids);
    }

    SetFrameIsIBSplit(blockFrame, inlineFrame);
    aSiblings.AddChild(inlineFrame);
    lastNewInline = inlineFrame;
  } while (aChildItems.NotEmpty());

  SetFrameIsIBSplit(lastNewInline, nullptr);
}

void
nsCSSFrameConstructor::BuildInlineChildItems(nsFrameConstructorState& aState,
                                             FrameConstructionItem& aParentItem,
                                             bool aItemIsWithinSVGText,
                                             bool aItemAllowsTextPathChild)
{
  
  
  nsFrameConstructorState::PendingBindingAutoPusher
    pusher(aState, aParentItem.mPendingBinding);

  nsStyleContext* const parentStyleContext = aParentItem.mStyleContext;
  nsIContent* const parentContent = aParentItem.mContent;

  TreeMatchContext::AutoAncestorPusher ancestorPusher(aState.mTreeMatchContext);
  if (aState.mTreeMatchContext.mAncestorFilter.HasFilter()) {
    ancestorPusher.PushAncestorAndStyleScope(parentContent->AsElement());
  } else {
    ancestorPusher.PushStyleScope(parentContent->AsElement());
  }

  if (!aItemIsWithinSVGText) {
    
    CreateGeneratedContentItem(aState, nullptr, parentContent, parentStyleContext,
                               nsCSSPseudoElements::ePseudo_before,
                               aParentItem.mChildItems);
  }

  uint32_t flags = ITEM_ALLOW_XBL_BASE | ITEM_ALLOW_PAGE_BREAK;
  if (aItemIsWithinSVGText) {
    flags |= ITEM_IS_WITHIN_SVG_TEXT;
  }
  if (aItemAllowsTextPathChild && aParentItem.mIsForSVGAElement) {
    flags |= ITEM_ALLOWS_TEXT_PATH_CHILD;
  }

  if (!aParentItem.mAnonChildren.IsEmpty()) {
    
    
    
    
    AddFCItemsForAnonymousContent(aState, nullptr, aParentItem.mAnonChildren,
                                  aParentItem.mChildItems, flags);
  } else {
    
    FlattenedChildIterator iter(parentContent);
    for (nsIContent* content = iter.GetNextChild(); content; content = iter.GetNextChild()) {
      
      
      
      
      
      nsIContent* contentParent = content->GetParent();
      MOZ_ASSERT(contentParent, "Parent must be non-null because we are iterating children.");
      TreeMatchContext::AutoAncestorPusher insertionPointPusher(aState.mTreeMatchContext);
      if (contentParent != parentContent && contentParent->IsElement()) {
        if (aState.mTreeMatchContext.mAncestorFilter.HasFilter()) {
          insertionPointPusher.PushAncestorAndStyleScope(contentParent->AsElement());
        } else {
          insertionPointPusher.PushStyleScope(contentParent->AsElement());
        }
      }

      
      
      
      content->UnsetFlags(NODE_DESCENDANTS_NEED_FRAMES | NODE_NEEDS_FRAME);
      if (content->IsNodeOfType(nsINode::eCOMMENT) ||
          content->IsNodeOfType(nsINode::ePROCESSING_INSTRUCTION)) {
        continue;
      }
      if (content->IsElement()) {
        
        
        
        
        content->UnsetFlags(ELEMENT_ALL_RESTYLE_FLAGS);
      }

      nsRefPtr<nsStyleContext> childContext =
        ResolveStyleContext(parentStyleContext, content, &aState);

      AddFrameConstructionItemsInternal(aState, content, nullptr,
                                        content->NodeInfo()->NameAtom(),
                                        content->GetNameSpaceID(),
                                        iter.XBLInvolved(), childContext,
                                        flags, nullptr,
                                        aParentItem.mChildItems);
    }
  }

  if (!aItemIsWithinSVGText) {
    
    CreateGeneratedContentItem(aState, nullptr, parentContent, parentStyleContext,
                               nsCSSPseudoElements::ePseudo_after,
                               aParentItem.mChildItems);
  }

  aParentItem.mIsAllInline = aParentItem.mChildItems.AreAllItemsInline();
}




static bool
IsSafeToAppendToIBSplitInline(nsIFrame* aParentFrame, nsIFrame* aNextSibling)
{
  NS_PRECONDITION(IsInlineFrame(aParentFrame),
                  "Must have an inline parent here");
  do {
    NS_ASSERTION(IsFramePartOfIBSplit(aParentFrame),
                 "How is this not part of an ib-split?");
    if (aNextSibling || aParentFrame->GetNextContinuation() ||
        GetIBSplitSibling(aParentFrame)) {
      return false;
    }

    aNextSibling = aParentFrame->GetNextSibling();
    aParentFrame = aParentFrame->GetParent();
  } while (IsInlineFrame(aParentFrame));

  return true;
}

bool
nsCSSFrameConstructor::WipeContainingBlock(nsFrameConstructorState& aState,
                                           nsIFrame* aContainingBlock,
                                           nsIFrame* aFrame,
                                           FrameConstructionItemList& aItems,
                                           bool aIsAppend,
                                           nsIFrame* aPrevSibling)
{
  if (aItems.IsEmpty()) {
    return false;
  }

  
  

  
  
  if (aFrame->IsBoxFrame() &&
      !(aFrame->GetStateBits() & NS_STATE_BOX_WRAPS_KIDS_IN_BLOCK) &&
      aItems.AnyItemsNeedBlockParent()) {
    RecreateFramesForContent(aFrame->GetContent(), true,
                             REMOVE_FOR_RECONSTRUCTION, nullptr);
    return true;
  }

  nsIFrame* nextSibling = ::GetInsertNextSibling(aFrame, aPrevSibling);

  
  
  
  if (::IsFlexOrGridContainer(aFrame)) {
    FCItemIterator iter(aItems);

    
    
    if (aPrevSibling && IsAnonymousFlexOrGridItem(aPrevSibling) &&
        iter.item().NeedsAnonFlexOrGridItem(aState)) {
      RecreateFramesForContent(aFrame->GetContent(), true,
                               REMOVE_FOR_RECONSTRUCTION, nullptr);
      return true;
    }

    
    
    if (nextSibling && IsAnonymousFlexOrGridItem(nextSibling)) {
      
      iter.SetToEnd();
      iter.Prev();
      if (iter.item().NeedsAnonFlexOrGridItem(aState)) {
        RecreateFramesForContent(aFrame->GetContent(), true,
                                 REMOVE_FOR_RECONSTRUCTION, nullptr);
        return true;
      }
    }
  }

  
  
  if (IsAnonymousFlexOrGridItem(aFrame)) {
    AssertAnonymousFlexOrGridItemParent(aFrame, aFrame->GetParent());

    
    
    
    
    
    
    nsFrameConstructorSaveState floatSaveState;
    aState.PushFloatContainingBlock(nullptr, floatSaveState);

    FCItemIterator iter(aItems);
    
    
    if (!iter.SkipItemsThatNeedAnonFlexOrGridItem(aState)) {
      
      
      nsIFrame* containerFrame = aFrame->GetParent();
      RecreateFramesForContent(containerFrame->GetContent(), true,
                               REMOVE_FOR_RECONSTRUCTION, nullptr);
      return true;
    }

    
    
  }

  
  
  
  
  
  
  
  
  
  
  nsIAtom* frameType = aFrame->GetType();
  if (IsRubyPseudo(aFrame) ||
      frameType == nsGkAtoms::rubyFrame ||
      RubyUtils::IsRubyContainerBox(frameType)) {
    
    
    
    RecreateFramesForContent(aFrame->GetContent(), true,
                             REMOVE_FOR_RECONSTRUCTION, nullptr);
    return true;
  }

  
  ParentType parentType = GetParentType(aFrame);
  
  
  
  
  
  
  if (!aItems.AllWantParentType(parentType)) {
    
    
    
    if (parentType != eTypeBlock && !aFrame->IsGeneratedContentFrame()) {
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      FCItemIterator iter(aItems);
      FCItemIterator start(iter);
      do {
        if (iter.SkipItemsWantingParentType(parentType)) {
          break;
        }

        
        
        if (!iter.item().IsWhitespace(aState)) {
          break;
        }

        if (iter == start) {
          
          
          nsIFrame* prevSibling = aPrevSibling;
          if (!prevSibling) {
            
            nsIFrame* parentPrevCont = aFrame->GetPrevContinuation();
            while (parentPrevCont) {
              prevSibling = parentPrevCont->GetLastChild(kPrincipalList);
              if (prevSibling) {
                break;
              }
              parentPrevCont = parentPrevCont->GetPrevContinuation();
            }
          };
          if (prevSibling) {
            if (IsTablePseudo(prevSibling)) {
              
              break;
            }
          } else if (IsTablePseudo(aFrame)) {
            
            break;
          }
        }

        FCItemIterator spaceEndIter(iter);
        
        bool trailingSpaces = spaceEndIter.SkipWhitespace(aState);

        bool okToDrop;
        if (trailingSpaces) {
          
          
          okToDrop = aIsAppend && !nextSibling;
          if (!okToDrop) {
            if (!nextSibling) {
              
              nsIFrame* parentNextCont = aFrame->GetNextContinuation();
              while (parentNextCont) {
                nextSibling = parentNextCont->GetFirstPrincipalChild();
                if (nextSibling) {
                  break;
                }
                parentNextCont = parentNextCont->GetNextContinuation();
              }
            }

            okToDrop = (nextSibling && !IsTablePseudo(nextSibling)) ||
                       (!nextSibling && !IsTablePseudo(aFrame));
          }
#ifdef DEBUG
          else {
            NS_ASSERTION(!IsTablePseudo(aFrame), "How did that happen?");
          }
#endif
        } else {
          okToDrop = (spaceEndIter.item().DesiredParentType() == parentType);
        }

        if (okToDrop) {
          iter.DeleteItemsTo(spaceEndIter);
        } else {
          
          
          break;
        }

        
        
      } while (!iter.IsDone());
    }

    
    
    
    
    
    
    

    
    
    if (aItems.IsEmpty()) {
      return false;
    }

    if (!aItems.AllWantParentType(parentType)) {
      
      
      RecreateFramesForContent(aFrame->GetContent(), true,
                               REMOVE_FOR_RECONSTRUCTION, nullptr);
      return true;
    }
  }

  
  
  do {
    if (IsInlineFrame(aFrame)) {
      if (aItems.AreAllItemsInline()) {
        
        return false;
      }

      if (!IsFramePartOfIBSplit(aFrame)) {
        
        break;
      }

      
      
      
      
      
      
      
      
      if (aIsAppend && IsSafeToAppendToIBSplitInline(aFrame, nextSibling)) {
        return false;
      }

      
      break;
    }

    
    
    if (!IsFramePartOfIBSplit(aFrame)) {
      return false;
    }

    
    
    if (aItems.AreAllItemsBlock()) {
      return false;
    }

    
    break;
  } while (0);

  
  if (!aContainingBlock) {
    aContainingBlock = aFrame;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  while (IsFramePartOfIBSplit(aContainingBlock) ||
         aContainingBlock->IsInlineOutside() ||
         aContainingBlock->StyleContext()->GetPseudo()) {
    aContainingBlock = aContainingBlock->GetParent();
    NS_ASSERTION(aContainingBlock,
                 "Must have non-inline, non-ib-split, non-pseudo frame as "
                 "root (or child of root, for a table root)!");
  }

  
  
  

  nsIContent *blockContent = aContainingBlock->GetContent();
#ifdef DEBUG
  if (gNoisyContentUpdates) {
    printf("nsCSSFrameConstructor::WipeContainingBlock: blockContent=%p\n",
           static_cast<void*>(blockContent));
  }
#endif
  RecreateFramesForContent(blockContent, true, REMOVE_FOR_RECONSTRUCTION,
                           nullptr);
  return true;
}

nsresult
nsCSSFrameConstructor::ReframeContainingBlock(nsIFrame*    aFrame,
                                              RemoveFlags  aFlags,
                                              nsIContent** aDestroyedFramesFor)
{

#ifdef DEBUG
  
  
  
  
  if (gNoisyContentUpdates) {
    printf("nsCSSFrameConstructor::ReframeContainingBlock frame=%p\n",
           static_cast<void*>(aFrame));
  }
#endif

  
  
  if (mPresShell->IsReflowLocked()) {
    
    
    NS_ERROR("Atemptted to nsCSSFrameConstructor::ReframeContainingBlock during a Reflow!!!");
    return NS_OK;
  }

  
  nsIFrame* containingBlock = GetIBContainingBlockFor(aFrame);
  if (containingBlock) {
    
    
    
    

    
    
    

    
    nsCOMPtr<nsIContent> blockContent = containingBlock->GetContent();
    if (blockContent) {
#ifdef DEBUG
      if (gNoisyContentUpdates) {
        printf("  ==> blockContent=%p\n", static_cast<void*>(blockContent));
      }
#endif
      return RecreateFramesForContent(blockContent, true, aFlags, aDestroyedFramesFor);
    }
  }

  
  return RecreateFramesForContent(mPresShell->GetDocument()->GetRootElement(),
                                  true, aFlags, nullptr);
}

nsresult
nsCSSFrameConstructor::GenerateChildFrames(nsContainerFrame* aFrame)
{
  {
    nsAutoScriptBlocker scriptBlocker;
    BeginUpdate();

    nsFrameItems childItems;
    nsFrameConstructorState state(mPresShell, nullptr, nullptr, nullptr);
    
    
    
    ProcessChildren(state, aFrame->GetContent(), aFrame->StyleContext(),
                    aFrame, false, childItems, false,
                    nullptr);

    aFrame->SetInitialChildList(kPrincipalList, childItems);

    EndUpdate();
  }

#ifdef ACCESSIBILITY
  nsAccessibilityService* accService = nsIPresShell::AccService();
  if (accService) {
    nsIContent* container = aFrame->GetContent();
    nsIContent* child = container->GetFirstChild();
    if (child) {
      accService->ContentRangeInserted(mPresShell, container, child, nullptr);
    }
  }
#endif

  
  mPresShell->GetDocument()->BindingManager()->ProcessAttachedQueue();

  return NS_OK;
}




bool
nsCSSFrameConstructor::
FrameConstructionItem::IsWhitespace(nsFrameConstructorState& aState) const
{
  NS_PRECONDITION(aState.mCreatingExtraFrames ||
                  !mContent->GetPrimaryFrame(), "How did that happen?");
  if (!mIsText) {
    return false;
  }
  mContent->SetFlags(NS_CREATE_FRAME_IF_NON_WHITESPACE |
                     NS_REFRAME_IF_WHITESPACE);
  return mContent->TextIsOnlyWhitespace();
}




void
nsCSSFrameConstructor::FrameConstructionItemList::
AdjustCountsForItem(FrameConstructionItem* aItem, int32_t aDelta)
{
  NS_PRECONDITION(aDelta == 1 || aDelta == -1, "Unexpected delta");
  mItemCount += aDelta;
  if (aItem->mIsAllInline) {
    mInlineCount += aDelta;
  }
  if (aItem->mIsBlock) {
    mBlockCount += aDelta;
  }
  if (aItem->mIsLineParticipant) {
    mLineParticipantCount += aDelta;
  }
  mDesiredParentCounts[aItem->DesiredParentType()] += aDelta;
}




inline bool
nsCSSFrameConstructor::FrameConstructionItemList::
Iterator::SkipItemsWantingParentType(ParentType aParentType)
{
  NS_PRECONDITION(!IsDone(), "Shouldn't be done yet");
  while (item().DesiredParentType() == aParentType) {
    Next();
    if (IsDone()) {
      return true;
    }
  }
  return false;
}

inline bool
nsCSSFrameConstructor::FrameConstructionItemList::
Iterator::SkipItemsNotWantingParentType(ParentType aParentType)
{
  NS_PRECONDITION(!IsDone(), "Shouldn't be done yet");
  while (item().DesiredParentType() != aParentType) {
    Next();
    if (IsDone()) {
      return true;
    }
  }
  return false;
}

bool
nsCSSFrameConstructor::FrameConstructionItem::
  NeedsAnonFlexOrGridItem(const nsFrameConstructorState& aState)
{
  if (mFCData->mBits & FCDATA_IS_LINE_PARTICIPANT) {
    
    return true;
  }

  if (!(mFCData->mBits & FCDATA_DISALLOW_OUT_OF_FLOW) &&
      aState.GetGeometricParent(mStyleContext->StyleDisplay(), nullptr)) {
    
    
    
    
    return true;
  }

  return false;
}

inline bool
nsCSSFrameConstructor::FrameConstructionItemList::
Iterator::SkipItemsThatNeedAnonFlexOrGridItem(
  const nsFrameConstructorState& aState)
{
  NS_PRECONDITION(!IsDone(), "Shouldn't be done yet");
  while (item().NeedsAnonFlexOrGridItem(aState)) {
    Next();
    if (IsDone()) {
      return true;
    }
  }
  return false;
}

inline bool
nsCSSFrameConstructor::FrameConstructionItemList::
Iterator::SkipItemsThatDontNeedAnonFlexOrGridItem(
  const nsFrameConstructorState& aState)
{
  NS_PRECONDITION(!IsDone(), "Shouldn't be done yet");
  while (!(item().NeedsAnonFlexOrGridItem(aState))) {
    Next();
    if (IsDone()) {
      return true;
    }
  }
  return false;
}

inline bool
nsCSSFrameConstructor::FrameConstructionItemList::
Iterator::SkipItemsNotWantingRubyParent()
{
  NS_PRECONDITION(!IsDone(), "Shouldn't be done yet");
  while (!IsRubyParentType(item().DesiredParentType())) {
    Next();
    if (IsDone()) {
      return true;
    }
  }
  return false;
}

inline bool
nsCSSFrameConstructor::FrameConstructionItemList::
Iterator::SkipWhitespace(nsFrameConstructorState& aState)
{
  NS_PRECONDITION(!IsDone(), "Shouldn't be done yet");
  NS_PRECONDITION(item().IsWhitespace(aState), "Not pointing to whitespace?");
  do {
    Next();
    if (IsDone()) {
      return true;
    }
  } while (item().IsWhitespace(aState));

  return false;
}

void
nsCSSFrameConstructor::FrameConstructionItemList::
Iterator::AppendItemToList(FrameConstructionItemList& aTargetList)
{
  NS_ASSERTION(&aTargetList != &mList, "Unexpected call");
  NS_PRECONDITION(!IsDone(), "should not be done");

  FrameConstructionItem* item = ToItem(mCurrent);
  Next();
  PR_REMOVE_LINK(item);
  PR_APPEND_LINK(item, &aTargetList.mItems);

  mList.AdjustCountsForItem(item, -1);
  aTargetList.AdjustCountsForItem(item, 1);
}

void
nsCSSFrameConstructor::FrameConstructionItemList::
Iterator::AppendItemsToList(const Iterator& aEnd,
                            FrameConstructionItemList& aTargetList)
{
  NS_ASSERTION(&aTargetList != &mList, "Unexpected call");
  NS_PRECONDITION(mEnd == aEnd.mEnd, "end iterator for some other list?");

  
  
  if (!AtStart() || !aEnd.IsDone() || !aTargetList.IsEmpty() ||
      !aTargetList.mUndisplayedItems.IsEmpty()) {
    do {
      AppendItemToList(aTargetList);
    } while (*this != aEnd);
    return;
  }

  
  PR_INSERT_AFTER(&aTargetList.mItems, &mList.mItems);
  
  PR_REMOVE_AND_INIT_LINK(&mList.mItems);

  
  aTargetList.mInlineCount = mList.mInlineCount;
  aTargetList.mBlockCount = mList.mBlockCount;
  aTargetList.mLineParticipantCount = mList.mLineParticipantCount;
  aTargetList.mItemCount = mList.mItemCount;
  memcpy(aTargetList.mDesiredParentCounts, mList.mDesiredParentCounts,
         sizeof(aTargetList.mDesiredParentCounts));

  
  aTargetList.mUndisplayedItems.SwapElements(mList.mUndisplayedItems);

  
  mList.~FrameConstructionItemList();
  new (&mList) FrameConstructionItemList();

  
  mCurrent = mEnd = &mList.mItems;
  NS_POSTCONDITION(*this == aEnd, "How did that happen?");
}

void
nsCSSFrameConstructor::FrameConstructionItemList::
Iterator::InsertItem(FrameConstructionItem* aItem)
{
  
  PR_INSERT_BEFORE(aItem, mCurrent);
  mList.AdjustCountsForItem(aItem, 1);

  NS_POSTCONDITION(PR_NEXT_LINK(aItem) == mCurrent, "How did that happen?");
}

void
nsCSSFrameConstructor::FrameConstructionItemList::
Iterator::DeleteItemsTo(const Iterator& aEnd)
{
  NS_PRECONDITION(mEnd == aEnd.mEnd, "end iterator for some other list?");
  NS_PRECONDITION(*this != aEnd, "Shouldn't be at aEnd yet");

  do {
    NS_ASSERTION(!IsDone(), "Ran off end of list?");
    FrameConstructionItem* item = ToItem(mCurrent);
    Next();
    PR_REMOVE_LINK(item);
    mList.AdjustCountsForItem(item, -1);
    delete item;
  } while (*this != aEnd);
}
