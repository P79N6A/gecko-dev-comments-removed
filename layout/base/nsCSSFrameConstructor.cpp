












































#include "nsCSSFrameConstructor.h"
#include "nsCRT.h"
#include "nsIAtom.h"
#include "nsIURL.h"
#include "nsHashtable.h"
#include "nsIHTMLDocument.h"
#include "nsIStyleRule.h"
#include "nsIFrame.h"
#include "nsGkAtoms.h"
#include "nsPresContext.h"
#include "nsILinkHandler.h"
#include "nsIDocument.h"
#include "nsTableFrame.h"
#include "nsTableColGroupFrame.h"
#include "nsTableColFrame.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMHTMLTableColElement.h"
#include "nsIDOMHTMLTableCaptionElem.h"
#include "nsHTMLParts.h"
#include "nsIPresShell.h"
#include "nsUnicharUtils.h"
#include "nsStyleSet.h"
#include "nsIViewManager.h"
#include "nsEventStates.h"
#include "nsStyleConsts.h"
#include "nsTableOuterFrame.h"
#include "nsIDOMXULElement.h"
#include "nsHTMLContainerFrame.h"
#include "nsINameSpaceManager.h"
#include "nsIDOMHTMLSelectElement.h"
#include "nsIDOMHTMLLegendElement.h"
#include "nsIComboboxControlFrame.h"
#include "nsIListControlFrame.h"
#include "nsISelectControlFrame.h"
#include "nsIDOMCharacterData.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsPlaceholderFrame.h"
#include "nsTableRowGroupFrame.h"
#include "nsStyleChangeList.h"
#include "nsIFormControl.h"
#include "nsCSSAnonBoxes.h"
#include "nsTextFragment.h"
#include "nsIAnonymousContentCreator.h"
#include "nsFrameManager.h"
#include "nsLegendFrame.h"
#include "nsIContentIterator.h"
#include "nsBoxLayoutState.h"
#include "nsBindingManager.h"
#include "nsXBLBinding.h"
#include "nsITheme.h"
#include "nsContentCID.h"
#include "nsContentUtils.h"
#include "nsIScriptError.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsObjectFrame.h"
#include "nsRuleNode.h"
#include "nsIDOMMutationEvent.h"
#include "nsChildIterator.h"
#include "nsCSSRendering.h"
#include "nsLayoutErrors.h"
#include "nsLayoutUtils.h"
#include "nsAutoPtr.h"
#include "nsBoxFrame.h"
#include "nsIBoxLayout.h"
#include "nsImageFrame.h"
#include "nsIObjectLoadingContent.h"
#include "nsContentErrors.h"
#include "nsIPrincipal.h"
#include "nsIDOMWindowInternal.h"
#include "nsStyleUtil.h"
#include "nsBox.h"
#include "nsTArray.h"
#include "nsGenericDOMDataNode.h"
#include "mozilla/dom/Element.h"
#include "FrameLayerBuilder.h"

#ifdef MOZ_XUL
#include "nsIRootBox.h"
#include "nsIDOMXULCommandDispatcher.h"
#include "nsIDOMXULDocument.h"
#include "nsIXULDocument.h"
#endif
#ifdef ACCESSIBILITY
#include "nsAccessibilityService.h"
#endif

#include "nsInlineFrame.h"
#include "nsBlockFrame.h"

#include "nsIScrollableFrame.h"

#include "nsIXBLService.h"

#undef NOISY_FIRST_LETTER

#include "nsMathMLParts.h"
#include "nsSVGFeatures.h"
#include "nsSVGEffects.h"
#include "nsSVGUtils.h"
#include "nsSVGOuterSVGFrame.h"

#include "nsRefreshDriver.h"
#include "nsRuleProcessorData.h"

using namespace mozilla;
using namespace mozilla::dom;

nsIFrame*
NS_NewHTMLCanvasFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);

#if defined(MOZ_MEDIA)
nsIFrame*
NS_NewHTMLVideoFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);
#endif

#include "nsSVGTextContainerFrame.h"

nsIFrame*
NS_NewSVGOuterSVGFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGInnerSVGFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGPathGeometryFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGGFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGGenericContainerFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGForeignObjectFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGAFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGGlyphFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGSwitchFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGTextFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGTSpanFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGContainerFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGUseFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
extern nsIFrame*
NS_NewSVGLinearGradientFrame(nsIPresShell *aPresShell, nsStyleContext* aContext);
extern nsIFrame*
NS_NewSVGRadialGradientFrame(nsIPresShell *aPresShell, nsStyleContext* aContext);
extern nsIFrame*
NS_NewSVGStopFrame(nsIPresShell *aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGMarkerFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
extern nsIFrame*
NS_NewSVGImageFrame(nsIPresShell *aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGClipPathFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGTextPathFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGFilterFrame(nsIPresShell *aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGPatternFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGMaskFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGLeafFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

#include "nsIDocument.h"
#include "nsIDOMElement.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDocumentXBL.h"
#include "nsIScrollable.h"
#include "nsINodeInfo.h"
#include "prenv.h"
#include "nsWidgetsCID.h"
#include "nsNodeInfoManager.h"
#include "nsContentCreatorFunctions.h"
#include "nsIServiceManager.h"


nsIXBLService * nsCSSFrameConstructor::gXBLService = nsnull;

#ifdef DEBUG



static PRBool gNoisyContentUpdates = PR_FALSE;
static PRBool gReallyNoisyContentUpdates = PR_FALSE;
static PRBool gNoisyInlineConstruction = PR_FALSE;

struct FrameCtorDebugFlags {
  const char* name;
  PRBool* on;
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
#include "nsMenuPopupFrame.h"
#include "nsPopupSetFrame.h"
#include "nsTreeColFrame.h"
#include "nsIBoxObject.h"
#include "nsPIListBoxObject.h"
#include "nsListBoxBodyFrame.h"
#include "nsListItemFrame.h"
#include "nsXULLabelFrame.h"



nsIFrame*
NS_NewAutoRepeatBoxFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

nsIFrame*
NS_NewRootBoxFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);

nsIFrame*
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
NS_NewMenuFrame (nsIPresShell* aPresShell, nsStyleContext* aContext, PRUint32 aFlags);

nsIFrame*
NS_NewMenuBarFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);

nsIFrame*
NS_NewTreeBodyFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);


nsresult
NS_NewGridLayout2 ( nsIPresShell* aPresShell, nsIBoxLayout** aNewLayout );
nsIFrame*
NS_NewGridRowLeafFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame*
NS_NewGridRowGroupFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);



nsIFrame*
NS_NewTitleBarFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);

nsIFrame*
NS_NewResizerFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);


#endif

nsIFrame*
NS_NewHTMLScrollFrame (nsIPresShell* aPresShell, nsStyleContext* aContext, PRBool aIsRoot);

nsIFrame*
NS_NewXULScrollFrame (nsIPresShell* aPresShell, nsStyleContext* aContext, PRBool aIsRoot);

nsIFrame*
NS_NewSliderFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);

nsIFrame*
NS_NewScrollbarFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);

nsIFrame*
NS_NewScrollbarButtonFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);


#ifdef NOISY_FINDFRAME
static PRInt32 FFWC_totalCount=0;
static PRInt32 FFWC_doLoop=0;
static PRInt32 FFWC_doSibling=0;
static PRInt32 FFWC_recursions=0;
static PRInt32 FFWC_nextInFlows=0;
#endif

static inline nsIFrame*
GetFieldSetBlockFrame(nsIFrame* aFieldsetFrame)
{
  
  nsIFrame* firstChild = aFieldsetFrame->GetFirstChild(nsnull);
  return firstChild && firstChild->GetNextSibling() ? firstChild->GetNextSibling() : firstChild;
}

#define FCDATA_DECL(_flags, _func) \
  { _flags, { (FrameCreationFunc)_func } }



static PRBool
IsInlineOutside(nsIFrame* aFrame)
{
  return aFrame->GetStyleDisplay()->IsInlineOutside();
}









static PRBool
IsInlineFrame(const nsIFrame* aFrame)
{
  return aFrame->IsFrameOfType(nsIFrame::eLineParticipant);
}





static nsIContent*
AnyKidsNeedBlockParent(nsIFrame *aFrameList)
{
  for (nsIFrame *k = aFrameList; k; k = k->GetNextSibling()) {
    
    
    
    if (k->IsFrameOfType(nsIFrame::eLineParticipant)) {
      return k->GetContent();
    }
  }
  return nsnull;
}


static void
ReparentFrame(nsFrameManager* aFrameManager,
              nsIFrame* aNewParentFrame,
              nsIFrame* aFrame)
{
  aFrame->SetParent(aNewParentFrame);
  aFrameManager->ReparentStyleContext(aFrame);
}

static void
ReparentFrames(nsFrameManager* aFrameManager,
               nsIFrame* aNewParentFrame,
               const nsFrameList& aFrameList)
{
  for (nsFrameList::Enumerator e(aFrameList); !e.AtEnd(); e.Next()) {
    ReparentFrame(aFrameManager, aNewParentFrame, e.get());
  }
}







static inline PRBool
IsFrameSpecial(nsIFrame* aFrame)
{
  return (aFrame->GetStateBits() & NS_FRAME_IS_SPECIAL) != 0;
}

static nsIFrame* GetSpecialSibling(nsIFrame* aFrame)
{
  NS_PRECONDITION(IsFrameSpecial(aFrame), "Shouldn't call this");

  
  
  return static_cast<nsIFrame*>
    (aFrame->GetFirstContinuation()->
       Properties().Get(nsIFrame::IBSplitSpecialSibling()));
}

static nsIFrame* GetSpecialPrevSibling(nsIFrame* aFrame)
{
  NS_PRECONDITION(IsFrameSpecial(aFrame), "Shouldn't call this");
  
  
  
  return static_cast<nsIFrame*>
    (aFrame->GetFirstContinuation()->
       Properties().Get(nsIFrame::IBSplitSpecialPrevSibling()));
}

static nsIFrame*
GetLastSpecialSibling(nsIFrame* aFrame, PRBool aReturnEmptyTrailingInline)
{
  for (nsIFrame *frame = aFrame, *next; ; frame = next) {
    next = GetSpecialSibling(frame);
    if (!next ||
        (!aReturnEmptyTrailingInline && !next->GetFirstChild(nsnull) &&
         !GetSpecialSibling(next))) {
      NS_ASSERTION(!next || !IsInlineOutside(frame),
                   "Should have a block here!");
      return frame;
    }
  }
  NS_NOTREACHED("unreachable code");
  return nsnull;
}

static void
SetFrameIsSpecial(nsIFrame* aFrame, nsIFrame* aSpecialSibling)
{
  NS_PRECONDITION(aFrame, "bad args!");

  
  NS_ASSERTION(!aFrame->GetPrevContinuation(),
               "assigning special sibling to other than first continuation!");
  NS_ASSERTION(!aFrame->GetNextContinuation() ||
               IsFrameSpecial(aFrame->GetNextContinuation()),
               "should have no non-special continuations here");

  
  aFrame->AddStateBits(NS_FRAME_IS_SPECIAL);

  if (aSpecialSibling) {
    NS_ASSERTION(!aSpecialSibling->GetPrevContinuation(),
                 "assigning something other than the first continuation as the "
                 "special sibling");

    
    
    FramePropertyTable* props = aFrame->PresContext()->PropertyTable();
    props->Set(aFrame, nsIFrame::IBSplitSpecialSibling(), aSpecialSibling);
    props->Set(aSpecialSibling, nsIFrame::IBSplitSpecialPrevSibling(), aFrame);
  }
}

static nsIFrame*
GetIBContainingBlockFor(nsIFrame* aFrame)
{
  NS_PRECONDITION(IsFrameSpecial(aFrame),
                  "GetIBContainingBlockFor() should only be called on known IB frames");

  
  nsIFrame* parentFrame;
  do {
    parentFrame = aFrame->GetParent();

    if (! parentFrame) {
      NS_ERROR("no unsplit block frame in IB hierarchy");
      return aFrame;
    }

    
    
    
    if (!IsFrameSpecial(parentFrame) &&
        !parentFrame->GetStyleContext()->GetPseudo())
      break;

    aFrame = parentFrame;
  } while (1);
 
  
  NS_ASSERTION(parentFrame, "no normal ancestor found for special frame in GetIBContainingBlockFor");
  NS_ASSERTION(parentFrame != aFrame, "parentFrame is actually the child frame - bogus reslt");

  return parentFrame;
}














static void
FindFirstBlock(nsFrameList::FrameLinkEnumerator& aLink)
{
  for ( ; !aLink.AtEnd(); aLink.Next()) {
    if (!IsInlineOutside(aLink.NextFrame())) {
      return;
    }
  }
}




static nsFrameList::FrameLinkEnumerator
FindFirstNonBlock(const nsFrameList& aList)
{
  nsFrameList::FrameLinkEnumerator link(aList);
  for (; !link.AtEnd(); link.Next()) {
    if (IsInlineOutside(link.NextFrame())) {
      break;
    }
  }
  return link;
}

inline void
SetInitialSingleChild(nsIFrame* aParent, nsIFrame* aFrame)
{
  NS_PRECONDITION(!aFrame->GetNextSibling(), "Should be using a frame list");
  nsFrameList temp(aFrame, aFrame);
  aParent->SetInitialChildList(nsnull, temp);
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
  
  nsIFrame* containingBlock;
  
  nsAbsoluteItems(nsIFrame* aContainingBlock);
#ifdef DEBUG
  
  
  
  ~nsAbsoluteItems() {
    NS_ASSERTION(!FirstChild(),
                 "Dangling child list.  Someone forgot to insert it?");
  }
#endif
  
  
  void AddChild(nsIFrame* aChild);
};

nsAbsoluteItems::nsAbsoluteItems(nsIFrame* aContainingBlock)
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





class NS_STACK_CLASS nsFrameConstructorSaveState {
public:
  nsFrameConstructorSaveState();
  ~nsFrameConstructorSaveState();

private:
  nsAbsoluteItems* mItems;                
  PRPackedBool*    mFixedPosIsAbsPos;

  nsAbsoluteItems  mSavedItems;           
  PRPackedBool     mSavedFixedPosIsAbsPos;

  
  nsIAtom* mChildListName;
  nsFrameConstructorState* mState;

  friend class nsFrameConstructorState;
};




struct PendingBinding : public PRCList
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



class NS_STACK_CLASS nsFrameConstructorState {
public:
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

  
  
  
  
  
  PRPackedBool              mFixedPosIsAbsPos;

  
  
  
  PRPackedBool              mHavePendingPopupgroup;

  
  
  
  
  
  
  PRPackedBool              mCreatingExtraFrames;

  nsCOMArray<nsIContent>    mGeneratedTextNodesWithInitializer;

  TreeMatchContext          mTreeMatchContext;

  
  
  nsFrameConstructorState(nsIPresShell*          aPresShell,
                          nsIFrame*              aFixedContainingBlock,
                          nsIFrame*              aAbsoluteContainingBlock,
                          nsIFrame*              aFloatContainingBlock,
                          nsILayoutHistoryState* aHistoryState);
  
  nsFrameConstructorState(nsIPresShell*          aPresShell,
                          nsIFrame*              aFixedContainingBlock,
                          nsIFrame*              aAbsoluteContainingBlock,
                          nsIFrame*              aFloatContainingBlock);

  ~nsFrameConstructorState();
  
  
  
  
  void PushAbsoluteContainingBlock(nsIFrame* aNewAbsoluteContainingBlock,
                                   nsFrameConstructorSaveState& aSaveState);

  
  
  
  
  
  
  
  void PushFloatContainingBlock(nsIFrame* aNewFloatContainingBlock,
                                nsFrameConstructorSaveState& aSaveState);

  
  
  
  nsIFrame* GetGeometricParent(const nsStyleDisplay* aStyleDisplay,
                               nsIFrame* aContentParentFrame);

  





















  nsresult AddChild(nsIFrame* aNewFrame,
                    nsFrameItems& aFrameItems,
                    nsIContent* aContent,
                    nsStyleContext* aStyleContext,
                    nsIFrame* aParentFrame,
                    PRBool aCanBePositioned = PR_TRUE,
                    PRBool aCanBeFloated = PR_TRUE,
                    PRBool aIsOutOfFlowPopup = PR_FALSE,
                    PRBool aInsertAfter = PR_FALSE,
                    nsIFrame* aInsertAfterFrame = nsnull);

  





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
  class NS_STACK_CLASS PendingBindingAutoPusher {
  public:
    PendingBindingAutoPusher(nsFrameConstructorState& aState,
                             PendingBinding* aPendingBinding) :
      mState(aState),
      mPendingBinding(aState.mCurrentPendingBindingInsertionPoint)
        {
          NS_PRECONDITION(mPendingBinding, "how did that happen?");
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
    PRCList* mPendingBinding;
  };

  


  void AddPendingBinding(PendingBinding* aPendingBinding) {
    PR_INSERT_BEFORE(aPendingBinding, mCurrentPendingBindingInsertionPoint);
  }

protected:
  friend class nsFrameConstructorSaveState;

  



  void ProcessFrameInsertions(nsAbsoluteItems& aFrameItems,
                              nsIAtom* aChildListName);

  
  
  PRCList mPendingBindings;

  PRCList* mCurrentPendingBindingInsertionPoint;
};

nsFrameConstructorState::nsFrameConstructorState(nsIPresShell*          aPresShell,
                                                 nsIFrame*              aFixedContainingBlock,
                                                 nsIFrame*              aAbsoluteContainingBlock,
                                                 nsIFrame*              aFloatContainingBlock,
                                                 nsILayoutHistoryState* aHistoryState)
  : mPresContext(aPresShell->GetPresContext()),
    mPresShell(aPresShell),
    mFrameManager(aPresShell->FrameManager()),
#ifdef MOZ_XUL    
    mPopupItems(nsnull),
#endif
    mFixedItems(aFixedContainingBlock),
    mAbsoluteItems(aAbsoluteContainingBlock),
    mFloatedItems(aFloatContainingBlock),
    
    mFrameState(aHistoryState),
    mAdditionalStateBits(0),
    mFixedPosIsAbsPos(aAbsoluteContainingBlock &&
                      aAbsoluteContainingBlock->GetStyleDisplay()->
                        HasTransform()),
    mHavePendingPopupgroup(PR_FALSE),
    mCreatingExtraFrames(PR_FALSE),
    mTreeMatchContext(PR_TRUE, nsRuleWalker::eRelevantLinkUnvisited,
                      aPresShell->GetDocument()),
    mCurrentPendingBindingInsertionPoint(&mPendingBindings)
{
#ifdef MOZ_XUL
  nsIRootBox* rootBox = nsIRootBox::GetRootBox(aPresShell);
  if (rootBox) {
    mPopupItems.containingBlock = rootBox->GetPopupSetFrame();
  }
#endif
  MOZ_COUNT_CTOR(nsFrameConstructorState);
  PR_INIT_CLIST(&mPendingBindings);
}

nsFrameConstructorState::nsFrameConstructorState(nsIPresShell* aPresShell,
                                                 nsIFrame*     aFixedContainingBlock,
                                                 nsIFrame*     aAbsoluteContainingBlock,
                                                 nsIFrame*     aFloatContainingBlock)
  : mPresContext(aPresShell->GetPresContext()),
    mPresShell(aPresShell),
    mFrameManager(aPresShell->FrameManager()),
#ifdef MOZ_XUL    
    mPopupItems(nsnull),
#endif
    mFixedItems(aFixedContainingBlock),
    mAbsoluteItems(aAbsoluteContainingBlock),
    mFloatedItems(aFloatContainingBlock),
    
    mAdditionalStateBits(0),
    mFixedPosIsAbsPos(aAbsoluteContainingBlock &&
                      aAbsoluteContainingBlock->GetStyleDisplay()->
                        HasTransform()),
    mHavePendingPopupgroup(PR_FALSE),
    mCreatingExtraFrames(PR_FALSE),
    mTreeMatchContext(PR_TRUE, nsRuleWalker::eRelevantLinkUnvisited,
                      aPresShell->GetDocument()),
    mCurrentPendingBindingInsertionPoint(&mPendingBindings)
{
#ifdef MOZ_XUL
  nsIRootBox* rootBox = nsIRootBox::GetRootBox(aPresShell);
  if (rootBox) {
    mPopupItems.containingBlock = rootBox->GetPopupSetFrame();
  }
#endif
  MOZ_COUNT_CTOR(nsFrameConstructorState);
  mFrameState = aPresShell->GetDocument()->GetLayoutHistoryState();
  PR_INIT_CLIST(&mPendingBindings);
}

nsFrameConstructorState::~nsFrameConstructorState()
{
  
  
  
  
  
  
  
  
  MOZ_COUNT_DTOR(nsFrameConstructorState);
  ProcessFrameInsertions(mFloatedItems, nsGkAtoms::floatList);
  ProcessFrameInsertions(mAbsoluteItems, nsGkAtoms::absoluteList);
  ProcessFrameInsertions(mFixedItems, nsGkAtoms::fixedList);
#ifdef MOZ_XUL
  ProcessFrameInsertions(mPopupItems, nsGkAtoms::popupList);
#endif
  for (PRInt32 i = mGeneratedTextNodesWithInitializer.Count() - 1; i >= 0; --i) {
    mGeneratedTextNodesWithInitializer[i]->
      DeleteProperty(nsGkAtoms::genConInitializerProperty);
  }
  if (!PR_CLIST_IS_EMPTY(&mPendingBindings)) {
    nsBindingManager* bindingManager = mPresShell->GetDocument()->BindingManager();
    do {
      PendingBinding* pendingBinding =
        static_cast<PendingBinding*>(PR_NEXT_LINK(&mPendingBindings));
      PR_REMOVE_LINK(pendingBinding);
      bindingManager->AddToAttachedQueue(pendingBinding->mBinding);
      delete pendingBinding;
    } while (!PR_CLIST_IS_EMPTY(&mPendingBindings));
  }
}

static nsIFrame*
AdjustAbsoluteContainingBlock(nsIFrame* aContainingBlockIn)
{
  if (!aContainingBlockIn) {
    return nsnull;
  }
  
  
  
  return aContainingBlockIn->GetFirstContinuation();
}

void
nsFrameConstructorState::PushAbsoluteContainingBlock(nsIFrame* aNewAbsoluteContainingBlock,
                                                     nsFrameConstructorSaveState& aSaveState)
{
  aSaveState.mItems = &mAbsoluteItems;
  aSaveState.mSavedItems = mAbsoluteItems;
  aSaveState.mChildListName = nsGkAtoms::absoluteList;
  aSaveState.mState = this;

  
  aSaveState.mFixedPosIsAbsPos = &mFixedPosIsAbsPos;
  aSaveState.mSavedFixedPosIsAbsPos = mFixedPosIsAbsPos;

  mAbsoluteItems = 
    nsAbsoluteItems(AdjustAbsoluteContainingBlock(aNewAbsoluteContainingBlock));

  


  mFixedPosIsAbsPos = (aNewAbsoluteContainingBlock &&
                       aNewAbsoluteContainingBlock->GetStyleDisplay()->HasTransform());

  if (aNewAbsoluteContainingBlock) {
    aNewAbsoluteContainingBlock->MarkAsAbsoluteContainingBlock();
  }
}

void
nsFrameConstructorState::PushFloatContainingBlock(nsIFrame* aNewFloatContainingBlock,
                                                  nsFrameConstructorSaveState& aSaveState)
{
  NS_PRECONDITION(!aNewFloatContainingBlock ||
                  aNewFloatContainingBlock->IsFloatContainingBlock(),
                  "Please push a real float containing block!");
  aSaveState.mItems = &mFloatedItems;
  aSaveState.mSavedItems = mFloatedItems;
  aSaveState.mChildListName = nsGkAtoms::floatList;
  aSaveState.mState = this;
  mFloatedItems = nsAbsoluteItems(aNewFloatContainingBlock);
}

nsIFrame*
nsFrameConstructorState::GetGeometricParent(const nsStyleDisplay* aStyleDisplay,
                                            nsIFrame* aContentParentFrame)
{
  NS_PRECONDITION(aStyleDisplay, "Must have display struct!");

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  if (aStyleDisplay->IsFloating() && mFloatedItems.containingBlock) {
    NS_ASSERTION(!aStyleDisplay->IsAbsolutelyPositioned(),
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

nsresult
nsFrameConstructorState::AddChild(nsIFrame* aNewFrame,
                                  nsFrameItems& aFrameItems,
                                  nsIContent* aContent,
                                  nsStyleContext* aStyleContext,
                                  nsIFrame* aParentFrame,
                                  PRBool aCanBePositioned,
                                  PRBool aCanBeFloated,
                                  PRBool aIsOutOfFlowPopup,
                                  PRBool aInsertAfter,
                                  nsIFrame* aInsertAfterFrame)
{
  NS_PRECONDITION(!aNewFrame->GetNextSibling(), "Shouldn't happen");
  
  const nsStyleDisplay* disp = aNewFrame->GetStyleDisplay();
  
  
  

  PRBool needPlaceholder = PR_FALSE;
  nsFrameState placeholderType;
  nsFrameItems* frameItems = &aFrameItems;
#ifdef MOZ_XUL
  if (NS_UNLIKELY(aIsOutOfFlowPopup)) {
      NS_ASSERTION(aNewFrame->GetParent() == mPopupItems.containingBlock,
                   "Popup whose parent is not the popup containing block?");
      NS_ASSERTION(mPopupItems.containingBlock, "Must have a popup set frame!");
      needPlaceholder = PR_TRUE;
      frameItems = &mPopupItems;
      placeholderType = PLACEHOLDER_FOR_POPUP;
  }
  else
#endif 
  if (aCanBeFloated && disp->IsFloating() &&
      mFloatedItems.containingBlock) {
    NS_ASSERTION(aNewFrame->GetParent() == mFloatedItems.containingBlock,
                 "Float whose parent is not the float containing block?");
    needPlaceholder = PR_TRUE;
    frameItems = &mFloatedItems;
    placeholderType = PLACEHOLDER_FOR_FLOAT;
  }
  else if (aCanBePositioned) {
    if (disp->mPosition == NS_STYLE_POSITION_ABSOLUTE &&
        mAbsoluteItems.containingBlock) {
      NS_ASSERTION(aNewFrame->GetParent() == mAbsoluteItems.containingBlock,
                   "Abs pos whose parent is not the abs pos containing block?");
      needPlaceholder = PR_TRUE;
      frameItems = &mAbsoluteItems;
      placeholderType = PLACEHOLDER_FOR_ABSPOS;
    }
    if (disp->mPosition == NS_STYLE_POSITION_FIXED &&
        GetFixedItems().containingBlock) {
      NS_ASSERTION(aNewFrame->GetParent() == GetFixedItems().containingBlock,
                   "Fixed pos whose parent is not the fixed pos containing block?");
      needPlaceholder = PR_TRUE;
      frameItems = &GetFixedItems();
      placeholderType = PLACEHOLDER_FOR_FIXEDPOS;
    }
  }

  if (needPlaceholder) {
    NS_ASSERTION(frameItems != &aFrameItems,
                 "Putting frame in-flow _and_ want a placeholder?");
    nsIFrame* placeholderFrame;
    nsresult rv =
      nsCSSFrameConstructor::CreatePlaceholderFrameFor(mPresShell,
                                                       aContent,
                                                       aNewFrame,
                                                       aStyleContext,
                                                       aParentFrame,
                                                       nsnull,
                                                       placeholderType,
                                                       &placeholderFrame);
    if (NS_FAILED(rv)) {
      
      
      
      
      aNewFrame->Destroy();
      return rv;
    }

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
    frameItems->InsertFrame(nsnull, aInsertAfterFrame, aNewFrame);
  } else {
    frameItems->AddChild(aNewFrame);
  }
  
  return NS_OK;
}

void
nsFrameConstructorState::ProcessFrameInsertions(nsAbsoluteItems& aFrameItems,
                                                nsIAtom* aChildListName)
{
#define NS_NONXUL_LIST_TEST (&aFrameItems == &mFloatedItems &&             \
                             aChildListName == nsGkAtoms::floatList)    || \
                            (&aFrameItems == &mAbsoluteItems &&            \
                             aChildListName == nsGkAtoms::absoluteList) || \
                            (&aFrameItems == &mFixedItems &&               \
                             aChildListName == nsGkAtoms::fixedList)
#ifdef MOZ_XUL
  NS_PRECONDITION(NS_NONXUL_LIST_TEST ||
                  (&aFrameItems == &mPopupItems &&
                   aChildListName == nsGkAtoms::popupList), 
                  "Unexpected aFrameItems/aChildListName combination");
#else
  NS_PRECONDITION(NS_NONXUL_LIST_TEST,
                  "Unexpected aFrameItems/aChildListName combination");
#endif

  if (aFrameItems.IsEmpty()) {
    return;
  }
  
  nsIFrame* containingBlock = aFrameItems.containingBlock;

  NS_ASSERTION(containingBlock,
               "Child list without containing block?");
  
  
  
  
  const nsFrameList& childList = containingBlock->GetChildList(aChildListName);
  nsresult rv = NS_OK;
  if (childList.IsEmpty() &&
      (containingBlock->GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
    
    
    if (aChildListName == containingBlock->GetAbsoluteListName()) {
      rv = containingBlock->GetAbsoluteContainingBlock()->
           SetInitialChildList(containingBlock, aChildListName, aFrameItems);
    } else {
      rv = containingBlock->SetInitialChildList(aChildListName, aFrameItems);
    }
  } else {
    
    
    
    
    
    
    
    nsIFrame* lastChild = childList.LastChild();

    
    
    
    nsIFrame* firstNewFrame = aFrameItems.FirstChild();  
    if (!lastChild ||
        nsLayoutUtils::CompareTreePosition(lastChild, firstNewFrame, containingBlock) < 0) {
      
      rv = mFrameManager->AppendFrames(containingBlock, aChildListName, aFrameItems);
    } else {
      
      nsIFrame* insertionPoint = nsnull;
      for (nsIFrame* f = childList.FirstChild(); f != lastChild;
           f = f->GetNextSibling()) {
        PRInt32 compare =
          nsLayoutUtils::CompareTreePosition(f, firstNewFrame, containingBlock);
        if (compare > 0) {
          
          
          break;
        }
        insertionPoint = f;
      }
      rv = mFrameManager->InsertFrames(containingBlock, aChildListName,
                                       insertionPoint, aFrameItems);
    }
  }

  NS_POSTCONDITION(aFrameItems.IsEmpty(), "How did that happen?");

  
  
  
  NS_ASSERTION(NS_SUCCEEDED(rv), "Frames getting lost!");
}


nsFrameConstructorSaveState::nsFrameConstructorSaveState()
  : mItems(nsnull),
    mFixedPosIsAbsPos(nsnull),
    mSavedItems(nsnull),
    mSavedFixedPosIsAbsPos(PR_FALSE),
    mChildListName(nsnull),
    mState(nsnull)
{
}

nsFrameConstructorSaveState::~nsFrameConstructorSaveState()
{
  
  if (mItems) {
    NS_ASSERTION(mState, "Can't have mItems set without having a state!");
    mState->ProcessFrameInsertions(*mItems, mChildListName);
    *mItems = mSavedItems;
#ifdef DEBUG
    
    
    mSavedItems.Clear();
#endif
  }
  if (mFixedPosIsAbsPos) {
    *mFixedPosIsAbsPos = mSavedFixedPosIsAbsPos;
  }
}

static 
PRBool IsBorderCollapse(nsIFrame* aFrame)
{
  for (nsIFrame* frame = aFrame; frame; frame = frame->GetParent()) {
    if (nsGkAtoms::tableFrame == frame->GetType()) {
      return ((nsTableFrame*)frame)->IsBorderCollapse();
    }
  }
  NS_ASSERTION(PR_FALSE, "program error");
  return PR_FALSE;
}

















static void
MoveChildrenTo(nsPresContext* aPresContext,
               nsIFrame* aOldParent,
               nsIFrame* aNewParent,
               nsFrameList& aFrameList)
{
  PRBool sameGrandParent = aOldParent->GetParent() == aNewParent->GetParent();

  if (aNewParent->HasView() || aOldParent->HasView() || !sameGrandParent) {
    
    nsContainerFrame::ReparentFrameViewList(aPresContext, aFrameList,
                                            aOldParent, aNewParent);
  }

  for (nsFrameList::Enumerator e(aFrameList); !e.AtEnd(); e.Next()) {
    e.get()->SetParent(aNewParent);
  }

  if (aNewParent->GetChildList(nsnull).IsEmpty() &&
      (aNewParent->GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
    aNewParent->SetInitialChildList(nsnull, aFrameList);
  } else {
    aNewParent->AppendFrames(nsnull, aFrameList);
  }
}



nsCSSFrameConstructor::nsCSSFrameConstructor(nsIDocument *aDocument,
                                             nsIPresShell *aPresShell)
  : mDocument(aDocument)
  , mPresShell(aPresShell)
  , mRootElementFrame(nsnull)
  , mRootElementStyleFrame(nsnull)
  , mFixedContainingBlock(nsnull)
  , mDocElementContainingBlock(nsnull)
  , mGfxScrollFrame(nsnull)
  , mPageSequenceFrame(nsnull)
  , mUpdateCount(0)
  , mQuotesDirty(PR_FALSE)
  , mCountersDirty(PR_FALSE)
  , mIsDestroyingFrameTree(PR_FALSE)
  , mRebuildAllStyleData(PR_FALSE)
  , mHasRootAbsPosContainingBlock(PR_FALSE)
  , mObservingRefreshDriver(PR_FALSE)
  , mInStyleRefresh(PR_FALSE)
  , mHoverGeneration(0)
  , mRebuildAllExtraHint(nsChangeHint(0))
  , mPendingRestyles(ELEMENT_HAS_PENDING_RESTYLE |
                     ELEMENT_IS_POTENTIAL_RESTYLE_ROOT, this)
  , mPendingAnimationRestyles(ELEMENT_HAS_PENDING_ANIMATION_RESTYLE |
                              ELEMENT_IS_POTENTIAL_ANIMATION_RESTYLE_ROOT, this)
{
  
  if (!mPendingRestyles.Init() || !mPendingAnimationRestyles.Init()) {
    
  }

#ifdef DEBUG
  static PRBool gFirstTime = PR_TRUE;
  if (gFirstTime) {
    gFirstTime = PR_FALSE;
    char* flags = PR_GetEnv("GECKO_FRAMECTOR_DEBUG_FLAGS");
    if (flags) {
      PRBool error = PR_FALSE;
      for (;;) {
        char* comma = PL_strchr(flags, ',');
        if (comma)
          *comma = '\0';

        PRBool found = PR_FALSE;
        FrameCtorDebugFlags* flag = gFlags;
        FrameCtorDebugFlags* limit = gFlags + NUM_DEBUG_FLAGS;
        while (flag < limit) {
          if (PL_strcasecmp(flag->name, flags) == 0) {
            *(flag->on) = PR_TRUE;
            printf("nsCSSFrameConstructor: setting %s debug flag on\n", flag->name);
            found = PR_TRUE;
            break;
          }
          ++flag;
        }

        if (! found)
          error = PR_TRUE;

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

nsIXBLService * nsCSSFrameConstructor::GetXBLService()
{
  if (!gXBLService) {
    nsresult rv = CallGetService("@mozilla.org/xbl;1", &gXBLService);
    if (NS_FAILED(rv))
      gXBLService = nsnull;
  }
  
  return gXBLService;
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
}

struct nsGenConInitializer {
  nsAutoPtr<nsGenConNode> mNode;
  nsGenConList*           mList;
  void (nsCSSFrameConstructor::*mDirtyAll)();
  
  nsGenConInitializer(nsGenConNode* aNode, nsGenConList* aList,
                      void (nsCSSFrameConstructor::*aDirtyAll)())
    : mNode(aNode), mList(aList), mDirtyAll(aDirtyAll) {}
};

static void
DestroyGenConInitializer(void*    aFrame,
                         nsIAtom* aPropertyName,
                         void*    aPropertyValue,
                         void*    aDtorData)
{
  delete static_cast<nsGenConInitializer*>(aPropertyValue);
}

already_AddRefed<nsIContent>
nsCSSFrameConstructor::CreateGenConTextNode(nsFrameConstructorState& aState,
                                            const nsString& aString,
                                            nsCOMPtr<nsIDOMCharacterData>* aText,
                                            nsGenConInitializer* aInitializer)
{
  nsCOMPtr<nsIContent> content;
  NS_NewTextNode(getter_AddRefs(content), mDocument->NodeInfoManager());
  if (!content) {
    
    
    NS_ASSERTION(!aText, "this OOM case isn't handled very well");
    return nsnull;
  }
  content->SetText(aString, PR_FALSE);
  if (aText) {
    *aText = do_QueryInterface(content);
  }
  if (aInitializer) {
    content->SetProperty(nsGkAtoms::genConInitializerProperty, aInitializer,
                         DestroyGenConInitializer);
    aState.mGeneratedTextNodesWithInitializer.AppendObject(content);
  }
  return content.forget();
}

already_AddRefed<nsIContent>
nsCSSFrameConstructor::CreateGeneratedContent(nsFrameConstructorState& aState,
                                              nsIContent*     aParentContent,
                                              nsStyleContext* aStyleContext,
                                              PRUint32        aContentIndex)
{
  
  const nsStyleContentData &data =
    aStyleContext->GetStyleContent()->ContentAt(aContentIndex);
  nsStyleContentType type = data.mType;

  if (eStyleContentType_Image == type) {
    if (!data.mContent.mImage) {
      
      
      return nsnull;
    }
    
    
    

    nsCOMPtr<nsINodeInfo> nodeInfo;
    nodeInfo = mDocument->NodeInfoManager()->GetNodeInfo(nsGkAtoms::mozgeneratedcontentimage, nsnull,
                                                         kNameSpaceID_XHTML);

    nsCOMPtr<nsIContent> content;
    NS_NewGenConImageContent(getter_AddRefs(content), nodeInfo.forget(),
                             data.mContent.mImage);
    return content.forget();
  }

  switch (type) {
  case eStyleContentType_String:
    return CreateGenConTextNode(aState,
                                nsDependentString(data.mContent.mString),
                                nsnull, nsnull);

  case eStyleContentType_Attr:
    {
      nsCOMPtr<nsIAtom> attrName;
      PRInt32 attrNameSpace = kNameSpaceID_None;
      nsAutoString contentString(data.mContent.mString);
      
      PRInt32 barIndex = contentString.FindChar('|'); 
      if (-1 != barIndex) {
        nsAutoString  nameSpaceVal;
        contentString.Left(nameSpaceVal, barIndex);
        PRInt32 error;
        attrNameSpace = nameSpaceVal.ToInteger(&error, 10);
        contentString.Cut(0, barIndex + 1);
        if (contentString.Length()) {
          if (mDocument->IsHTML() && aParentContent->IsHTML()) {
            ToLowerCase(contentString);
          }
          attrName = do_GetAtom(contentString);
        }
      }
      else {
        if (mDocument->IsHTML() && aParentContent->IsHTML()) {
          ToLowerCase(contentString);
        }
        attrName = do_GetAtom(contentString);
      }

      if (!attrName) {
        return nsnull;
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
      if (!counterList)
        return nsnull;

      nsCounterUseNode* node =
        new nsCounterUseNode(counters, aContentIndex,
                             type == eStyleContentType_Counters);
      if (!node)
        return nsnull;

      nsGenConInitializer* initializer =
        new nsGenConInitializer(node, counterList,
                                &nsCSSFrameConstructor::CountersDirty);
      return CreateGenConTextNode(aState, EmptyString(), &node->mText,
                                  initializer);
    }

  case eStyleContentType_Image:
    NS_NOTREACHED("handled by if above");
    return nsnull;

  case eStyleContentType_OpenQuote:
  case eStyleContentType_CloseQuote:
  case eStyleContentType_NoOpenQuote:
  case eStyleContentType_NoCloseQuote:
    {
      nsQuoteNode* node =
        new nsQuoteNode(type, aContentIndex);
      if (!node)
        return nsnull;

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

      if (aParentContent->IsHTML() &&
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
        return CreateGenConTextNode(aState, temp, nsnull, nsnull);
      }

      break;
    }
  } 

  return nsnull;
}
















void
nsCSSFrameConstructor::CreateGeneratedContentItem(nsFrameConstructorState& aState,
                                                  nsIFrame*        aParentFrame,
                                                  nsIContent*      aParentContent,
                                                  nsStyleContext*  aStyleContext,
                                                  nsCSSPseudoElements::Type aPseudoElement,
                                                  FrameConstructionItemList& aItems)
{
  
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
  
  
  nsCOMPtr<nsINodeInfo> nodeInfo;
  nsIAtom* elemName = aPseudoElement == nsCSSPseudoElements::ePseudo_before ?
    nsGkAtoms::mozgeneratedcontentbefore : nsGkAtoms::mozgeneratedcontentafter;
  nodeInfo = mDocument->NodeInfoManager()->GetNodeInfo(elemName, nsnull,
                                                       kNameSpaceID_None);
  nsCOMPtr<nsIContent> container;
  nsresult rv = NS_NewXMLElement(getter_AddRefs(container), nodeInfo.forget());
  if (NS_FAILED(rv))
    return;
  container->SetNativeAnonymous();

  rv = container->BindToTree(mDocument, aParentContent, aParentContent, PR_TRUE);
  if (NS_FAILED(rv)) {
    container->UnbindFromTree();
    return;
  }

  PRUint32 contentCount = pseudoStyleContext->GetStyleContent()->ContentCount();
  for (PRUint32 contentIndex = 0; contentIndex < contentCount; contentIndex++) {
    nsCOMPtr<nsIContent> content =
      CreateGeneratedContent(aState, aParentContent, pseudoStyleContext,
                             contentIndex);
    if (content) {
      container->AppendChildTo(content, PR_FALSE);
    }
  }

  AddFrameConstructionItemsInternal(aState, container, aParentFrame, elemName,
                                    kNameSpaceID_None, PR_TRUE,
                                    pseudoStyleContext,
                                    ITEM_IS_GENERATED_CONTENT, aItems);
}
    







static PRBool
IsTableRelated(nsIAtom* aParentType)
{
  return
    nsGkAtoms::tableOuterFrame    == aParentType ||
    nsGkAtoms::tableFrame         == aParentType ||
    nsGkAtoms::tableRowGroupFrame == aParentType ||
    nsGkAtoms::tableRowFrame      == aParentType ||
    nsGkAtoms::tableCaptionFrame  == aParentType ||
    nsGkAtoms::tableColGroupFrame == aParentType ||
    nsGkAtoms::tableColFrame      == aParentType ||
    IS_TABLE_CELL(aParentType);
}








static PRBool
IsTablePseudo(nsIFrame* aFrame)
{
  nsIAtom* pseudoType = aFrame->GetStyleContext()->GetPseudo();
  return pseudoType &&
    (pseudoType == nsCSSAnonBoxes::table ||
     pseudoType == nsCSSAnonBoxes::inlineTable ||
     pseudoType == nsCSSAnonBoxes::tableColGroup ||
     pseudoType == nsCSSAnonBoxes::tableRowGroup ||
     pseudoType == nsCSSAnonBoxes::tableRow ||
     pseudoType == nsCSSAnonBoxes::tableCell ||
     (pseudoType == nsCSSAnonBoxes::cellContent &&
      aFrame->GetParent()->GetStyleContext()->GetPseudo() ==
        nsCSSAnonBoxes::tableCell) ||
     (pseudoType == nsCSSAnonBoxes::tableOuter &&
      (aFrame->GetFirstChild(nsnull)->GetStyleContext()->GetPseudo() ==
         nsCSSAnonBoxes::table ||
       aFrame->GetFirstChild(nsnull)->GetStyleContext()->GetPseudo() ==
         nsCSSAnonBoxes::inlineTable)));
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

  return eTypeBlock;
}
           
static nsIFrame*
AdjustCaptionParentFrame(nsIFrame* aParentFrame) 
{
  if (nsGkAtoms::tableFrame == aParentFrame->GetType()) {
    return aParentFrame->GetParent();;
  }
  return aParentFrame;
}
 






static PRBool
GetCaptionAdjustedParent(nsIFrame*        aParentFrame,
                         const nsIFrame*  aChildFrame,
                         nsIFrame**       aAdjParentFrame)
{
  *aAdjParentFrame = aParentFrame;
  PRBool haveCaption = PR_FALSE;

  if (nsGkAtoms::tableCaptionFrame == aChildFrame->GetType()) {
    haveCaption = PR_TRUE;
    *aAdjParentFrame = AdjustCaptionParentFrame(aParentFrame);
  }
  return haveCaption;
}

void
nsCSSFrameConstructor::AdjustParentFrame(nsIFrame* &                  aParentFrame,
                                         const FrameConstructionData* aFCData,
                                         nsStyleContext*              aStyleContext)
{
  NS_PRECONDITION(aStyleContext, "Must have child's style context");
  NS_PRECONDITION(aFCData, "Must have frame construction data");

  PRBool tablePart = ((aFCData->mBits & FCDATA_IS_TABLE_PART) != 0);

  if (tablePart && aStyleContext->GetStyleDisplay()->mDisplay ==
      NS_STYLE_DISPLAY_TABLE_CAPTION) {
    aParentFrame = AdjustCaptionParentFrame(aParentFrame);
  }
}


static void
PullOutCaptionFrames(nsFrameItems& aItems, nsFrameItems& aCaptions)
{
  nsIFrame *child = aItems.FirstChild();
  while (child) {
    nsIFrame *nextSibling = child->GetNextSibling();
    if (nsGkAtoms::tableCaptionFrame == child->GetType()) {
      aItems.RemoveFrame(child);
      aCaptions.AddChild(child);
    }
    child = nextSibling;
  }
}






nsresult
nsCSSFrameConstructor::ConstructTable(nsFrameConstructorState& aState,
                                      FrameConstructionItem&   aItem,
                                      nsIFrame*                aParentFrame,
                                      const nsStyleDisplay*    aDisplay,
                                      nsFrameItems&            aFrameItems,
                                      nsIFrame**               aNewFrame)
{
  NS_PRECONDITION(aDisplay->mDisplay == NS_STYLE_DISPLAY_TABLE ||
                  aDisplay->mDisplay == NS_STYLE_DISPLAY_INLINE_TABLE,
                  "Unexpected call");

  nsIContent* const content = aItem.mContent;
  nsStyleContext* const styleContext = aItem.mStyleContext;
  const PRUint32 nameSpaceID = aItem.mNameSpaceID;

  nsresult rv = NS_OK;

  
  nsRefPtr<nsStyleContext> outerStyleContext;
  outerStyleContext = mPresShell->StyleSet()->
    ResolveAnonymousBoxStyle(nsCSSAnonBoxes::tableOuter, styleContext);

  
  nsIFrame* newFrame;
  if (kNameSpaceID_MathML == nameSpaceID)
    newFrame = NS_NewMathMLmtableOuterFrame(mPresShell, outerStyleContext);
  else
    newFrame = NS_NewTableOuterFrame(mPresShell, outerStyleContext);

  nsIFrame* geometricParent =
    aState.GetGeometricParent(outerStyleContext->GetStyleDisplay(),
                              aParentFrame);

  
  InitAndRestoreFrame(aState, content, geometricParent, nsnull, newFrame);  

  
  nsIFrame* innerFrame;
  if (kNameSpaceID_MathML == nameSpaceID)
    innerFrame = NS_NewMathMLmtableFrame(mPresShell, styleContext);
  else
    innerFrame = NS_NewTableFrame(mPresShell, styleContext);
 
  InitAndRestoreFrame(aState, content, newFrame, nsnull, innerFrame);

  
  SetInitialSingleChild(newFrame, innerFrame);

  rv = aState.AddChild(newFrame, aFrameItems, content, styleContext,
                       aParentFrame);
  if (NS_FAILED(rv)) {
    return rv;
  }

  if (!mRootElementFrame) {
    
    
    mRootElementFrame = newFrame;
  }

  nsFrameItems childItems;
  if (aItem.mFCData->mBits & FCDATA_USE_CHILD_ITEMS) {
    rv = ConstructFramesFromItemList(aState, aItem.mChildItems,
                                     innerFrame, childItems);
  } else {
    rv = ProcessChildren(aState, content, styleContext, innerFrame,
                         PR_TRUE, childItems, PR_FALSE, aItem.mPendingBinding);
  }
  
  if (NS_FAILED(rv)) return rv;

  nsFrameItems captionItems;
  PullOutCaptionFrames(childItems, captionItems);

  
  innerFrame->SetInitialChildList(nsnull, childItems);

  
  if (captionItems.NotEmpty()) {
    newFrame->SetInitialChildList(nsGkAtoms::captionList, captionItems);
  }

  *aNewFrame = newFrame;
  return rv;
}

nsresult
nsCSSFrameConstructor::ConstructTableRow(nsFrameConstructorState& aState,
                                         FrameConstructionItem&   aItem,
                                         nsIFrame*                aParentFrame,
                                         const nsStyleDisplay*    aDisplay,
                                         nsFrameItems&            aFrameItems,
                                         nsIFrame**               aNewFrame)
{
  NS_PRECONDITION(aDisplay->mDisplay == NS_STYLE_DISPLAY_TABLE_ROW,
                  "Unexpected call");
  nsIContent* const content = aItem.mContent;
  nsStyleContext* const styleContext = aItem.mStyleContext;
  const PRUint32 nameSpaceID = aItem.mNameSpaceID;

  nsIFrame* newFrame;
  if (kNameSpaceID_MathML == nameSpaceID)
    newFrame = NS_NewMathMLmtrFrame(mPresShell, styleContext);
  else
    newFrame = NS_NewTableRowFrame(mPresShell, styleContext);

  if (NS_UNLIKELY(!newFrame)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  InitAndRestoreFrame(aState, content, aParentFrame, nsnull, newFrame);

  nsFrameItems childItems;
  nsresult rv;
  if (aItem.mFCData->mBits & FCDATA_USE_CHILD_ITEMS) {
    rv = ConstructFramesFromItemList(aState, aItem.mChildItems, newFrame,
                                     childItems);
  } else {
    rv = ProcessChildren(aState, content, styleContext, newFrame,
                         PR_TRUE, childItems, PR_FALSE, aItem.mPendingBinding);
  }
  if (NS_FAILED(rv)) return rv;

  newFrame->SetInitialChildList(nsnull, childItems);
  aFrameItems.AddChild(newFrame);
  *aNewFrame = newFrame;

  return NS_OK;
}

nsresult
nsCSSFrameConstructor::ConstructTableCol(nsFrameConstructorState& aState,
                                         FrameConstructionItem&   aItem,
                                         nsIFrame*                aParentFrame,
                                         const nsStyleDisplay*    aStyleDisplay,
                                         nsFrameItems&            aFrameItems,
                                         nsIFrame**               aNewFrame)
{
  nsIContent* const content = aItem.mContent;
  nsStyleContext* const styleContext = aItem.mStyleContext;

  nsTableColFrame* colFrame = NS_NewTableColFrame(mPresShell, styleContext);
  if (NS_UNLIKELY(!colFrame)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  InitAndRestoreFrame(aState, content, aParentFrame, nsnull, colFrame);

  NS_ASSERTION(colFrame->GetStyleContext() == styleContext,
               "Unexpected style context");

  aFrameItems.AddChild(colFrame);
  *aNewFrame = colFrame;

  
  PRInt32 span = colFrame->GetSpan();
  for (PRInt32 spanX = 1; spanX < span; spanX++) {
    nsTableColFrame* newCol = NS_NewTableColFrame(mPresShell, styleContext);
    if (NS_UNLIKELY(!newCol)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    InitAndRestoreFrame(aState, content, aParentFrame, nsnull, newCol,
                        PR_FALSE);
    aFrameItems.LastChild()->SetNextContinuation(newCol);
    newCol->SetPrevContinuation(aFrameItems.LastChild());
    aFrameItems.AddChild(newCol);
    newCol->SetColType(eColAnonymousCol);
  }

  return NS_OK;
}

nsresult
nsCSSFrameConstructor::ConstructTableCell(nsFrameConstructorState& aState,
                                          FrameConstructionItem&   aItem,
                                          nsIFrame*                aParentFrame,
                                          const nsStyleDisplay*    aDisplay,
                                          nsFrameItems&            aFrameItems,
                                          nsIFrame**               aNewFrame)
{
  NS_PRECONDITION(aDisplay->mDisplay == NS_STYLE_DISPLAY_TABLE_CELL,
                  "Unexpected call");

  nsIContent* const content = aItem.mContent;
  nsStyleContext* const styleContext = aItem.mStyleContext;
  const PRUint32 nameSpaceID = aItem.mNameSpaceID;

  PRBool borderCollapse = IsBorderCollapse(aParentFrame);
  nsIFrame* newFrame;
  
  
  
  
  
  
  
  if (kNameSpaceID_MathML == nameSpaceID && !borderCollapse)
    newFrame = NS_NewMathMLmtdFrame(mPresShell, styleContext);
  else
    
    
    
    newFrame = NS_NewTableCellFrame(mPresShell, styleContext, borderCollapse);

  if (NS_UNLIKELY(!newFrame)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  InitAndRestoreFrame(aState, content, aParentFrame, nsnull, newFrame);
  
  
  nsRefPtr<nsStyleContext> innerPseudoStyle;
  innerPseudoStyle = mPresShell->StyleSet()->
    ResolveAnonymousBoxStyle(nsCSSAnonBoxes::cellContent, styleContext);

  
  PRBool isBlock;
  nsIFrame* cellInnerFrame;
  if (kNameSpaceID_MathML == nameSpaceID) {
    cellInnerFrame = NS_NewMathMLmtdInnerFrame(mPresShell, innerPseudoStyle);
    isBlock = PR_FALSE;
  } else {
    cellInnerFrame = NS_NewBlockFormattingContext(mPresShell, innerPseudoStyle);
    isBlock = PR_TRUE;
  }

  if (NS_UNLIKELY(!cellInnerFrame)) {
    newFrame->Destroy();
    return NS_ERROR_OUT_OF_MEMORY;
  }

  InitAndRestoreFrame(aState, content, newFrame, nsnull, cellInnerFrame);

  nsFrameItems childItems;
  nsresult rv;
  if (aItem.mFCData->mBits & FCDATA_USE_CHILD_ITEMS) {
    
    
    
    
    nsFrameConstructorSaveState floatSaveState;
    if (!isBlock) { 
      aState.PushFloatContainingBlock(nsnull, floatSaveState);
    } else {
      aState.PushFloatContainingBlock(cellInnerFrame, floatSaveState);
    }

    rv = ConstructFramesFromItemList(aState, aItem.mChildItems, cellInnerFrame,
                                     childItems);
  } else {
    
    rv = ProcessChildren(aState, content, styleContext, cellInnerFrame,
                         PR_TRUE, childItems, isBlock, aItem.mPendingBinding);
  }
  
  if (NS_FAILED(rv)) {
    
    
    cellInnerFrame->Destroy();
    newFrame->Destroy();
    return rv;
  }

  cellInnerFrame->SetInitialChildList(nsnull, childItems);
  SetInitialSingleChild(newFrame, cellInnerFrame);
  aFrameItems.AddChild(newFrame);
  *aNewFrame = newFrame;

  return NS_OK;
}

static inline PRBool 
NeedFrameFor(const nsFrameConstructorState& aState,
             nsIFrame*   aParentFrame,
             nsIContent* aChildContent) 
{
  
  
  NS_PRECONDITION(!aChildContent->GetPrimaryFrame() ||
                  aState.mCreatingExtraFrames ||
                  aChildContent->GetPrimaryFrame()->GetContent() != aChildContent,
                  "Why did we get called?");

  
  
  
  
  
  

  
  
  
  
  
  
  if (!aParentFrame->IsFrameOfType(nsIFrame::eExcludesIgnorableWhitespace) ||
      aParentFrame->IsGeneratedContentFrame() ||
      !aChildContent->IsNodeOfType(nsINode::eTEXT)) {
    return PR_TRUE;
  }

  aChildContent->SetFlags(NS_CREATE_FRAME_IF_NON_WHITESPACE |
                          NS_REFRAME_IF_WHITESPACE);
  return !aChildContent->TextIsOnlyWhitespace();
}





static PRBool CheckOverflow(nsPresContext* aPresContext,
                            const nsStyleDisplay* aDisplay)
{
  if (aDisplay->mOverflowX == NS_STYLE_OVERFLOW_VISIBLE)
    return PR_FALSE;

  if (aDisplay->mOverflowX == NS_STYLE_OVERFLOW_CLIP)
    aPresContext->SetViewportOverflowOverride(NS_STYLE_OVERFLOW_HIDDEN,
                                              NS_STYLE_OVERFLOW_HIDDEN);
  else
    aPresContext->SetViewportOverflowOverride(aDisplay->mOverflowX,
                                              aDisplay->mOverflowY);
  return PR_TRUE;
}









nsIContent*
nsCSSFrameConstructor::PropagateScrollToViewport()
{
  
  nsPresContext* presContext = mPresShell->GetPresContext();
  presContext->SetViewportOverflowOverride(NS_STYLE_OVERFLOW_AUTO,
                                           NS_STYLE_OVERFLOW_AUTO);

  
  
  if (presContext->IsPaginated()) {
    return nsnull;
  }

  Element* docElement = mDocument->GetRootElement();

  
  nsStyleSet *styleSet = mPresShell->StyleSet();
  nsRefPtr<nsStyleContext> rootStyle;
  rootStyle = styleSet->ResolveStyleFor(docElement, nsnull);
  if (!rootStyle) {
    return nsnull;
  }
  if (CheckOverflow(presContext, rootStyle->GetStyleDisplay())) {
    
    return docElement;
  }
  
  
  
  
  
  
  
  nsCOMPtr<nsIDOMHTMLDocument> htmlDoc(do_QueryInterface(mDocument));
  if (!htmlDoc || !docElement->IsHTML()) {
    return nsnull;
  }
  
  nsCOMPtr<nsIDOMHTMLElement> body;
  htmlDoc->GetBody(getter_AddRefs(body));
  nsCOMPtr<nsIContent> bodyElement = do_QueryInterface(body);
  
  if (!bodyElement ||
      !bodyElement->NodeInfo()->Equals(nsGkAtoms::body)) {
    
    return nsnull;
  }

  nsRefPtr<nsStyleContext> bodyStyle;
  bodyStyle = styleSet->ResolveStyleFor(bodyElement->AsElement(), rootStyle);
  if (!bodyStyle) {
    return nsnull;
  }

  if (CheckOverflow(presContext, bodyStyle->GetStyleDisplay())) {
    
    return bodyElement;
  }

  return nsnull;
}

nsresult
nsCSSFrameConstructor::ConstructDocElementFrame(Element*                 aDocElement,
                                                nsILayoutHistoryState*   aFrameState,
                                                nsIFrame**               aNewFrame)
{
  NS_PRECONDITION(mFixedContainingBlock,
                  "No viewport?  Someone forgot to call ConstructRootFrame!");
  NS_PRECONDITION(mFixedContainingBlock == mPresShell->FrameManager()->GetRootFrame(),
                  "Unexpected mFixedContainingBlock");
  NS_PRECONDITION(!mDocElementContainingBlock,
                  "Shouldn't have a doc element containing block here");

  *aNewFrame = nsnull;

  
  
  
#ifdef DEBUG
  nsIContent* propagatedScrollFrom =
#endif
    PropagateScrollToViewport();

  SetUpDocElementContainingBlock(aDocElement);

  NS_ASSERTION(mDocElementContainingBlock, "Should have parent by now");

  nsFrameConstructorState state(mPresShell, mFixedContainingBlock, nsnull,
                                nsnull, aFrameState);

  
  if (!mTempFrameTreeState)
    state.mPresShell->CaptureHistoryState(getter_AddRefs(mTempFrameTreeState));

  
  
  
  
  
  
  aDocElement->UnsetFlags(ELEMENT_ALL_RESTYLE_FLAGS);

  
  nsRefPtr<nsStyleContext> styleContext;
  styleContext = mPresShell->StyleSet()->ResolveStyleFor(aDocElement,
                                                         nsnull);

  const nsStyleDisplay* display = styleContext->GetStyleDisplay();

  
  if (display->mBinding) {
    
    nsresult rv;
    PRBool resolveStyle;
    
    nsIXBLService * xblService = GetXBLService();
    if (!xblService)
      return NS_ERROR_FAILURE;

    nsRefPtr<nsXBLBinding> binding;
    rv = xblService->LoadBindings(aDocElement, display->mBinding->GetURI(),
                                  display->mBinding->mOriginPrincipal,
                                  PR_FALSE, getter_AddRefs(binding),
                                  &resolveStyle);
    if (NS_FAILED(rv) && rv != NS_ERROR_XBL_BLOCKED)
      return NS_OK; 

    if (binding) {
      
      
      
      mDocument->BindingManager()->AddToAttachedQueue(binding);
    }

    if (resolveStyle) {
      styleContext = mPresShell->StyleSet()->ResolveStyleFor(aDocElement,
                                                             nsnull);
      display = styleContext->GetStyleDisplay();
    }
  }

  

#ifdef DEBUG
  NS_ASSERTION(!display->IsScrollableOverflow() || 
               state.mPresContext->IsPaginated() ||
               propagatedScrollFrom == aDocElement,
               "Scrollbars should have been propagated to the viewport");
#endif

  if (NS_UNLIKELY(display->mDisplay == NS_STYLE_DISPLAY_NONE)) {
    state.mFrameManager->SetUndisplayedContent(aDocElement, styleContext);
    return NS_OK;
  }

  
  styleContext->StartBackgroundImageLoads();

  nsFrameConstructorSaveState absoluteSaveState;
  if (mHasRootAbsPosContainingBlock) {
    
    
    state.PushAbsoluteContainingBlock(mDocElementContainingBlock,
                                      absoluteSaveState);
  }

  nsresult rv;

  
  
  

  
  
  
  
  
  
  nsIFrame* contentFrame;
  PRBool processChildren = PR_FALSE;

  
#ifdef MOZ_XUL
  if (aDocElement->IsXUL()) {
    contentFrame = NS_NewDocElementBoxFrame(mPresShell, styleContext);
    if (NS_UNLIKELY(!contentFrame)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    InitAndRestoreFrame(state, aDocElement, mDocElementContainingBlock, nsnull,
                        contentFrame);
    *aNewFrame = contentFrame;
    processChildren = PR_TRUE;
  }
  else
#endif
  if (aDocElement->GetNameSpaceID() == kNameSpaceID_SVG) {
    if (aDocElement->Tag() == nsGkAtoms::svg) {
      contentFrame = NS_NewSVGOuterSVGFrame(mPresShell, styleContext);
      if (NS_UNLIKELY(!contentFrame)) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
      InitAndRestoreFrame(state, aDocElement,
                          state.GetGeometricParent(display,
                                                   mDocElementContainingBlock),
                          nsnull, contentFrame);

      
      
      nsFrameItems frameItems;
      rv = state.AddChild(contentFrame, frameItems, aDocElement,
                          styleContext, mDocElementContainingBlock);
      if (NS_FAILED(rv) || frameItems.IsEmpty()) {
        return rv;
      }
      *aNewFrame = frameItems.FirstChild();
      processChildren = PR_TRUE;
    } else {
      return NS_ERROR_FAILURE;
    }
  } else {
    PRBool docElemIsTable = (display->mDisplay == NS_STYLE_DISPLAY_TABLE);
    if (docElemIsTable) {
      
      

      
      
      
      
      
      static const FrameConstructionData rootTableData = FCDATA_DECL(0, nsnull);
      nsRefPtr<nsStyleContext> extraRef(styleContext);
      FrameConstructionItem item(&rootTableData, aDocElement,
                                 aDocElement->Tag(), kNameSpaceID_None,
                                 nsnull, extraRef.forget(), PR_TRUE);

      nsFrameItems frameItems;
      
      rv = ConstructTable(state, item, mDocElementContainingBlock,
                          styleContext->GetStyleDisplay(),
                          frameItems, &contentFrame);
      if (NS_FAILED(rv))
        return rv;
      if (!contentFrame || frameItems.IsEmpty())
        return NS_ERROR_FAILURE;
      *aNewFrame = frameItems.FirstChild();
      NS_ASSERTION(frameItems.OnlyChild(), "multiple root element frames");
    } else {
      contentFrame = NS_NewBlockFormattingContext(mPresShell, styleContext);
      if (!contentFrame)
        return NS_ERROR_OUT_OF_MEMORY;
      nsFrameItems frameItems;
      
      rv = ConstructBlock(state, display, aDocElement,
                          state.GetGeometricParent(display,
                                                   mDocElementContainingBlock),
                          mDocElementContainingBlock, styleContext,
                          &contentFrame, frameItems, display->IsPositioned(),
                          nsnull);
      if (NS_FAILED(rv) || frameItems.IsEmpty())
        return rv;
      *aNewFrame = frameItems.FirstChild();
      NS_ASSERTION(frameItems.OnlyChild(), "multiple root element frames");
    }
  }

  
  aDocElement->SetPrimaryFrame(contentFrame);

  NS_ASSERTION(processChildren ? !mRootElementFrame :
                 mRootElementFrame == contentFrame,
               "unexpected mRootElementFrame");
  mRootElementFrame = contentFrame;

  
  
  
  PRBool isChild;
  contentFrame->GetParentStyleContextFrame(state.mPresContext,
          &mRootElementStyleFrame, &isChild);
  if (!isChild) {
    mRootElementStyleFrame = mRootElementFrame;
  }

  if (processChildren) {
    
    nsFrameItems childItems;

    NS_ASSERTION(!nsLayoutUtils::GetAsBlock(contentFrame),
                 "Only XUL and SVG frames should reach here");
    
    ProcessChildren(state, aDocElement, styleContext, contentFrame, PR_TRUE,
                    childItems, PR_FALSE, nsnull);

    
    contentFrame->SetInitialChildList(nsnull, childItems);
  }

  SetInitialSingleChild(mDocElementContainingBlock, *aNewFrame);

  return NS_OK;
}


nsresult
nsCSSFrameConstructor::ConstructRootFrame(nsIFrame** aNewFrame)
{
  AUTO_LAYOUT_PHASE_ENTRY_POINT(mPresShell->GetPresContext(), FrameC);
  NS_PRECONDITION(aNewFrame, "null out param");

  nsStyleSet *styleSet = mPresShell->StyleSet();

  
  
  {
    styleSet->SetBindingManager(mDocument->BindingManager());
  }

  
  nsIFrame*                 viewportFrame = nsnull;
  nsRefPtr<nsStyleContext> viewportPseudoStyle;

  viewportPseudoStyle =
    styleSet->ResolveAnonymousBoxStyle(nsCSSAnonBoxes::viewport, nsnull);

  viewportFrame = NS_NewViewportFrame(mPresShell, viewportPseudoStyle);

  
  
  
  viewportFrame->Init(nsnull, nsnull, nsnull);

  
  nsIView* rootView = mPresShell->GetViewManager()->GetRootView();
  viewportFrame->SetView(rootView);

  nsContainerFrame::SyncFrameViewProperties(mPresShell->GetPresContext(), viewportFrame,
                                            viewportPseudoStyle, rootView);
  nsContainerFrame::SyncWindowProperties(mPresShell->GetPresContext(), viewportFrame,
                                         rootView);

  
  mFixedContainingBlock = viewportFrame;
  
  mFixedContainingBlock->MarkAsAbsoluteContainingBlock();

  *aNewFrame = viewportFrame;
  return NS_OK;
}

nsresult
nsCSSFrameConstructor::SetUpDocElementContainingBlock(nsIContent* aDocElement)
{
  NS_PRECONDITION(aDocElement, "No element?");
  NS_PRECONDITION(!aDocElement->GetParent(), "Not root content?");
  NS_PRECONDITION(aDocElement->GetCurrentDoc(), "Not in a document?");
  NS_PRECONDITION(aDocElement->GetCurrentDoc()->GetRootElement() ==
                  aDocElement, "Not the root of the document?");

  

























































  


  
  
  
  
  
  
  

  nsPresContext* presContext = mPresShell->GetPresContext();
  PRBool isPaginated = presContext->IsRootPaginatedDocument();
  nsIFrame* viewportFrame = mFixedContainingBlock;
  nsStyleContext* viewportPseudoStyle = viewportFrame->GetStyleContext();

  nsIFrame* rootFrame = nsnull;
  nsIAtom* rootPseudo;
        
  if (!isPaginated) {
#ifdef MOZ_XUL
    if (aDocElement->IsXUL())
    {
      
      rootFrame = NS_NewRootBoxFrame(mPresShell, viewportPseudoStyle);
    } else
#endif
    {
      
      rootFrame = NS_NewCanvasFrame(mPresShell, viewportPseudoStyle);
      mHasRootAbsPosContainingBlock = PR_TRUE;
    }

    rootPseudo = nsCSSAnonBoxes::canvas;
    mDocElementContainingBlock = rootFrame;
  } else {
    
    rootFrame = NS_NewSimplePageSequenceFrame(mPresShell, viewportPseudoStyle);
    mPageSequenceFrame = rootFrame;
    rootPseudo = nsCSSAnonBoxes::pageSequence;
  }


  

  
  
  
  

  
  
  
  
  
  
  
  
  

  PRBool isHTML = aDocElement->IsHTML();
  PRBool isXUL = PR_FALSE;

  if (!isHTML) {
    isXUL = aDocElement->IsXUL();
  }

  
  PRBool isScrollable = !isXUL;

  
  if (isHTML) {
    nsCOMPtr<nsIHTMLDocument> htmlDoc = do_QueryInterface(mDocument);
    if (htmlDoc && htmlDoc->GetIsFrameset())
      isScrollable = PR_FALSE;
  }

  if (isPaginated) {
    isScrollable = presContext->HasPaginatedScrolling();
  }

  
  
  
  NS_ASSERTION(!isScrollable || !isXUL,
               "XUL documents should never be scrollable - see above");

  nsIFrame* newFrame = rootFrame;
  nsRefPtr<nsStyleContext> rootPseudoStyle;
  
  
  nsFrameConstructorState state(mPresShell, nsnull, nsnull, nsnull);

  
  nsIFrame* parentFrame = viewportFrame;

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

      
      
      
      
      
      
      
      newFrame = nsnull;
      rootPseudoStyle = BeginBuildingScrollFrame( state,
                                                  aDocElement,
                                                  styleContext,
                                                  viewportFrame,
                                                  rootPseudo,
                                                  PR_TRUE,
                                                  newFrame);
      parentFrame = newFrame;
      mGfxScrollFrame = newFrame;
  }
  
  rootFrame->SetStyleContextWithoutNotification(rootPseudoStyle);
  rootFrame->Init(aDocElement, parentFrame, nsnull);
  
  if (isScrollable) {
    FinishBuildingScrollFrame(parentFrame, rootFrame);
  }
  
  if (isPaginated) { 
    
    
    nsIFrame *pageFrame, *canvasFrame;
    ConstructPageFrame(mPresShell, presContext, rootFrame, nsnull,
                       pageFrame, canvasFrame);
    SetInitialSingleChild(rootFrame, pageFrame);

    
    
    mDocElementContainingBlock = canvasFrame;
    mHasRootAbsPosContainingBlock = PR_TRUE;
  }

  if (viewportFrame->GetStateBits() & NS_FRAME_FIRST_REFLOW) {
    SetInitialSingleChild(viewportFrame, newFrame);
  } else {
    nsFrameList newFrameList(newFrame, newFrame);
    viewportFrame->AppendFrames(nsnull, newFrameList);
  }

  return NS_OK;
}

nsresult
nsCSSFrameConstructor::ConstructPageFrame(nsIPresShell*  aPresShell,
                                          nsPresContext* aPresContext,
                                          nsIFrame*      aParentFrame,
                                          nsIFrame*      aPrevPageFrame,
                                          nsIFrame*&     aPageFrame,
                                          nsIFrame*&     aCanvasFrame)
{
  nsStyleContext* parentStyleContext = aParentFrame->GetStyleContext();
  nsStyleSet *styleSet = aPresShell->StyleSet();

  nsRefPtr<nsStyleContext> pagePseudoStyle;
  pagePseudoStyle = styleSet->ResolveAnonymousBoxStyle(nsCSSAnonBoxes::page,
                                                       parentStyleContext);

  aPageFrame = NS_NewPageFrame(aPresShell, pagePseudoStyle);
  if (NS_UNLIKELY(!aPageFrame))
    return NS_ERROR_OUT_OF_MEMORY;

  
  
  aPageFrame->Init(nsnull, aParentFrame, aPrevPageFrame);

  nsRefPtr<nsStyleContext> pageContentPseudoStyle;
  pageContentPseudoStyle =
    styleSet->ResolveAnonymousBoxStyle(nsCSSAnonBoxes::pageContent,
                                       pagePseudoStyle);

  nsIFrame* pageContentFrame = NS_NewPageContentFrame(aPresShell, pageContentPseudoStyle);
  if (NS_UNLIKELY(!pageContentFrame))
    return NS_ERROR_OUT_OF_MEMORY;

  
  
  nsIFrame* prevPageContentFrame = nsnull;
  if (aPrevPageFrame) {
    prevPageContentFrame = aPrevPageFrame->GetFirstChild(nsnull);
    NS_ASSERTION(prevPageContentFrame, "missing page content frame");
  }
  pageContentFrame->Init(nsnull, aPageFrame, prevPageContentFrame);
  SetInitialSingleChild(aPageFrame, pageContentFrame);
  mFixedContainingBlock = pageContentFrame;
  
  mFixedContainingBlock->MarkAsAbsoluteContainingBlock();

  nsRefPtr<nsStyleContext> canvasPseudoStyle;
  canvasPseudoStyle = styleSet->ResolveAnonymousBoxStyle(nsCSSAnonBoxes::canvas,
                                                         pageContentPseudoStyle);

  aCanvasFrame = NS_NewCanvasFrame(aPresShell, canvasPseudoStyle);
  if (NS_UNLIKELY(!aCanvasFrame))
    return NS_ERROR_OUT_OF_MEMORY;

  nsIFrame* prevCanvasFrame = nsnull;
  if (prevPageContentFrame) {
    prevCanvasFrame = prevPageContentFrame->GetFirstChild(nsnull);
    NS_ASSERTION(prevCanvasFrame, "missing canvas frame");
  }
  aCanvasFrame->Init(nsnull, pageContentFrame, prevCanvasFrame);
  SetInitialSingleChild(pageContentFrame, aCanvasFrame);

  return NS_OK;
}


nsresult
nsCSSFrameConstructor::CreatePlaceholderFrameFor(nsIPresShell*    aPresShell, 
                                                 nsIContent*      aContent,
                                                 nsIFrame*        aFrame,
                                                 nsStyleContext*  aStyleContext,
                                                 nsIFrame*        aParentFrame,
                                                 nsIFrame*        aPrevInFlow,
                                                 nsFrameState     aTypeBit,
                                                 nsIFrame**       aPlaceholderFrame)
{
  nsRefPtr<nsStyleContext> placeholderStyle = aPresShell->StyleSet()->
    ResolveStyleForNonElement(aStyleContext->GetParent());
  
  
  nsPlaceholderFrame* placeholderFrame =
    (nsPlaceholderFrame*)NS_NewPlaceholderFrame(aPresShell, placeholderStyle,
                                                aTypeBit);

  if (placeholderFrame) {
    placeholderFrame->Init(aContent, aParentFrame, aPrevInFlow);
  
    
    placeholderFrame->SetOutOfFlowFrame(aFrame);
  
    aFrame->AddStateBits(NS_FRAME_OUT_OF_FLOW);

    
    aPresShell->FrameManager()->RegisterPlaceholderFrame(placeholderFrame);

    *aPlaceholderFrame = static_cast<nsIFrame*>(placeholderFrame);
    
    return NS_OK;
  }
  else {
    return NS_ERROR_OUT_OF_MEMORY;
  }
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

nsresult
nsCSSFrameConstructor::ConstructButtonFrame(nsFrameConstructorState& aState,
                                            FrameConstructionItem&   aItem,
                                            nsIFrame*                aParentFrame,
                                            const nsStyleDisplay*    aStyleDisplay,
                                            nsFrameItems&            aFrameItems,
                                            nsIFrame**               aNewFrame)
{
  *aNewFrame = nsnull;
  nsIFrame* buttonFrame = nsnull;
  nsIContent* const content = aItem.mContent;
  nsStyleContext* const styleContext = aItem.mStyleContext;

  if (nsGkAtoms::button == aItem.mTag) {
    buttonFrame = NS_NewHTMLButtonControlFrame(mPresShell, styleContext);
  }
  else {
    buttonFrame = NS_NewGfxButtonControlFrame(mPresShell, styleContext);
  }
  if (NS_UNLIKELY(!buttonFrame)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  nsresult rv = InitAndRestoreFrame(aState, content,
                                    aState.GetGeometricParent(aStyleDisplay, aParentFrame),
                                    nsnull, buttonFrame);
  if (NS_FAILED(rv)) {
    buttonFrame->Destroy();
    return rv;
  }

  nsRefPtr<nsStyleContext> innerBlockContext;
  innerBlockContext =
    mPresShell->StyleSet()->ResolveAnonymousBoxStyle(nsCSSAnonBoxes::buttonContent,
                                                     styleContext);
                                                               
  nsIFrame* blockFrame = NS_NewBlockFrame(mPresShell, innerBlockContext,
                                          NS_BLOCK_FLOAT_MGR);

  if (NS_UNLIKELY(!blockFrame)) {
    buttonFrame->Destroy();
    return NS_ERROR_OUT_OF_MEMORY;
  }
  rv = InitAndRestoreFrame(aState, content, buttonFrame, nsnull, blockFrame);
  if (NS_FAILED(rv)) {
    blockFrame->Destroy();
    buttonFrame->Destroy();
    return rv;
  }

  rv = aState.AddChild(buttonFrame, aFrameItems, content, styleContext,
                       aParentFrame);
  if (NS_FAILED(rv)) {
    blockFrame->Destroy();
    buttonFrame->Destroy();
    return rv;
  }

  PRBool isLeaf = buttonFrame->IsLeaf();
#ifdef DEBUG
  
  
  nsIAnonymousContentCreator* creator = do_QueryFrame(buttonFrame);
  NS_ASSERTION(!creator == !isLeaf,
               "Should be creator exactly when we're a leaf");
#endif
  
  if (!isLeaf) { 
    
    nsFrameConstructorSaveState absoluteSaveState;
    nsFrameItems                childItems;

    if (aStyleDisplay->IsPositioned()) {
      
      
      aState.PushAbsoluteContainingBlock(blockFrame, absoluteSaveState);
    }

#ifdef DEBUG
    
    nsIAnonymousContentCreator* creator = do_QueryFrame(blockFrame);
    NS_ASSERTION(!creator, "Shouldn't be an anonymous content creator!");
#endif

    rv = ProcessChildren(aState, content, styleContext, blockFrame, PR_TRUE,
                         childItems, aStyleDisplay->IsBlockInside(),
                         aItem.mPendingBinding);
    if (NS_FAILED(rv)) return rv;
  
    
    blockFrame->SetInitialChildList(nsnull, childItems);
  }

  SetInitialSingleChild(buttonFrame, blockFrame);

  if (isLeaf) {
    ClearLazyBits(content->GetFirstChild(), nsnull);

    nsFrameItems  anonymousChildItems;
    
    
    
    CreateAnonymousFrames(aState, content, buttonFrame, aItem.mPendingBinding,
                          anonymousChildItems);
    if (anonymousChildItems.NotEmpty()) {
      
      aState.mFrameManager->AppendFrames(blockFrame, nsnull,
                                         anonymousChildItems);
    }
  }

  
  *aNewFrame = buttonFrame; 

  return NS_OK;  
}

nsresult
nsCSSFrameConstructor::ConstructSelectFrame(nsFrameConstructorState& aState,
                                            FrameConstructionItem&   aItem,
                                            nsIFrame*                aParentFrame,
                                            const nsStyleDisplay*    aStyleDisplay,
                                            nsFrameItems&            aFrameItems,
                                            nsIFrame**               aNewFrame)
{
  nsresult rv = NS_OK;
  const PRInt32 kNoSizeSpecified = -1;

  nsIContent* const content = aItem.mContent;
  nsStyleContext* const styleContext = aItem.mStyleContext;

  
  nsCOMPtr<nsIDOMHTMLSelectElement> sel(do_QueryInterface(content));
  PRInt32 size = 1;
  if (sel) {
    sel->GetSize(&size); 
    PRBool multipleSelect = PR_FALSE;
    sel->GetMultiple(&multipleSelect);
     
    if (((1 == size || 0 == size) || (kNoSizeSpecified  == size)) && (PR_FALSE == multipleSelect)) {
        
        
        
        
        
      PRUint32 flags = NS_BLOCK_FLOAT_MGR;
      nsIFrame* comboboxFrame = NS_NewComboboxControlFrame(mPresShell, styleContext, flags);

      
      
      nsILayoutHistoryState *historyState = aState.mFrameState;
      aState.mFrameState = nsnull;
      
      InitAndRestoreFrame(aState, content,
                          aState.GetGeometricParent(aStyleDisplay, aParentFrame),
                          nsnull, comboboxFrame);

      rv = aState.AddChild(comboboxFrame, aFrameItems, content, styleContext,
                           aParentFrame);
      if (NS_FAILED(rv)) {
        return rv;
      }
      
      nsIComboboxControlFrame* comboBox = do_QueryFrame(comboboxFrame);
      NS_ASSERTION(comboBox, "NS_NewComboboxControlFrame returned frame that "
                             "doesn't implement nsIComboboxControlFrame");

        
      nsRefPtr<nsStyleContext> listStyle;
      listStyle = mPresShell->StyleSet()->
        ResolveAnonymousBoxStyle(nsCSSAnonBoxes::dropDownList, styleContext);

        
      nsIFrame* listFrame = NS_NewListControlFrame(mPresShell, listStyle);

        
      nsIListControlFrame * listControlFrame = do_QueryFrame(listFrame);
      if (listControlFrame) {
        listControlFrame->SetComboboxFrame(comboboxFrame);
      }
         
      comboBox->SetDropDown(listFrame);

      NS_ASSERTION(!listStyle->GetStyleDisplay()->IsPositioned(),
                   "Ended up with positioned dropdown list somehow.");
      NS_ASSERTION(!listStyle->GetStyleDisplay()->IsFloating(),
                   "Ended up with floating dropdown list somehow.");
      
      
      
      nsIFrame* scrolledFrame = NS_NewSelectsAreaFrame(mPresShell, styleContext, flags);

      InitializeSelectFrame(aState, listFrame, scrolledFrame, content,
                            comboboxFrame, listStyle, PR_TRUE,
                            aItem.mPendingBinding, aFrameItems);

      NS_ASSERTION(listFrame->GetView(), "ListFrame's view is nsnull");

      
      
      

      nsFrameItems childItems;
      CreateAnonymousFrames(aState, content, comboboxFrame,
                            aItem.mPendingBinding, childItems);
  
      comboboxFrame->SetInitialChildList(nsnull, childItems);

      
      
      nsFrameItems popupItems;
      popupItems.AddChild(listFrame);
      comboboxFrame->SetInitialChildList(nsGkAtoms::selectPopupList,
                                         popupItems);

      *aNewFrame = comboboxFrame;
      aState.mFrameState = historyState;
      if (aState.mFrameState && aState.mFrameManager) {
        
        aState.mFrameManager->RestoreFrameState(comboboxFrame,
                                                aState.mFrameState);
      }
    } else {
      nsIFrame* listFrame = NS_NewListControlFrame(mPresShell, styleContext);
      if (listFrame) {
        rv = NS_OK;
      }
      else {
        rv = NS_ERROR_OUT_OF_MEMORY;
      }

      nsIFrame* scrolledFrame = NS_NewSelectsAreaFrame(
        mPresShell, styleContext, NS_BLOCK_FLOAT_MGR);

      
      

      InitializeSelectFrame(aState, listFrame, scrolledFrame, content,
                            aParentFrame, styleContext, PR_FALSE,
                            aItem.mPendingBinding, aFrameItems);

      *aNewFrame = listFrame;
    }
  }
  return rv;

}






nsresult
nsCSSFrameConstructor::InitializeSelectFrame(nsFrameConstructorState& aState,
                                             nsIFrame*                scrollFrame,
                                             nsIFrame*                scrolledFrame,
                                             nsIContent*              aContent,
                                             nsIFrame*                aParentFrame,
                                             nsStyleContext*          aStyleContext,
                                             PRBool                   aBuildCombobox,
                                             PendingBinding*          aPendingBinding,
                                             nsFrameItems&            aFrameItems)
{
  const nsStyleDisplay* display = aStyleContext->GetStyleDisplay();

  
  nsIFrame* geometricParent = aState.GetGeometricParent(display, aParentFrame);
    
  
  
  

  
  scrollFrame->Init(aContent, geometricParent, nsnull);

  if (!aBuildCombobox) {
    nsresult rv = aState.AddChild(scrollFrame, aFrameItems, aContent,
                                  aStyleContext, aParentFrame);
    if (NS_FAILED(rv)) {
      return rv;
    }
  }
      
  if (aBuildCombobox) {
    nsContainerFrame::CreateViewForFrame(scrollFrame, PR_TRUE);
  }

  BuildScrollFrame(aState, aContent, aStyleContext, scrolledFrame,
                   geometricParent, scrollFrame);

  if (aState.mFrameState && aState.mFrameManager) {
    
    aState.mFrameManager->RestoreFrameStateFor(scrollFrame, aState.mFrameState);
  }

  
  nsFrameConstructorSaveState absoluteSaveState;
  nsFrameItems                childItems;

  if (display->IsPositioned()) {
    
    
    aState.PushAbsoluteContainingBlock(scrolledFrame, absoluteSaveState);
  }

  ProcessChildren(aState, aContent, aStyleContext, scrolledFrame, PR_FALSE,
                  childItems, PR_FALSE, aPendingBinding);

  
  scrolledFrame->SetInitialChildList(nsnull, childItems);
  return NS_OK;
}

nsresult
nsCSSFrameConstructor::ConstructFieldSetFrame(nsFrameConstructorState& aState,
                                              FrameConstructionItem&   aItem,
                                              nsIFrame*                aParentFrame,
                                              const nsStyleDisplay*    aStyleDisplay,
                                              nsFrameItems&            aFrameItems,
                                              nsIFrame**               aNewFrame)
{
  nsIContent* const content = aItem.mContent;
  nsStyleContext* const styleContext = aItem.mStyleContext;

  nsIFrame* newFrame = NS_NewFieldSetFrame(mPresShell, styleContext);
  if (NS_UNLIKELY(!newFrame)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  InitAndRestoreFrame(aState, content,
                      aState.GetGeometricParent(aStyleDisplay, aParentFrame),
                      nsnull, newFrame);

  
  nsRefPtr<nsStyleContext> fieldsetContentStyle;
  fieldsetContentStyle = mPresShell->StyleSet()->
    ResolveAnonymousBoxStyle(nsCSSAnonBoxes::fieldsetContent, styleContext);

  nsIFrame* blockFrame = NS_NewBlockFrame(mPresShell, fieldsetContentStyle,
                                          NS_BLOCK_FLOAT_MGR |
                                          NS_BLOCK_MARGIN_ROOT);
  InitAndRestoreFrame(aState, content, newFrame, nsnull, blockFrame);

  nsresult rv = aState.AddChild(newFrame, aFrameItems, content, styleContext,
                                aParentFrame);
  if (NS_FAILED(rv)) {
    return rv;
  }
  
  
  nsFrameConstructorSaveState absoluteSaveState;
  nsFrameItems                childItems;

  if (aStyleDisplay->IsPositioned()) {
    
    
    
    
    aState.PushAbsoluteContainingBlock(blockFrame, absoluteSaveState);
  }

  ProcessChildren(aState, content, styleContext, blockFrame, PR_TRUE,
                  childItems, PR_TRUE, aItem.mPendingBinding);

  nsFrameItems fieldsetKids;
  fieldsetKids.AddChild(blockFrame);

  for (nsFrameList::Enumerator e(childItems); !e.AtEnd(); e.Next()) {
    nsLegendFrame* legendFrame = do_QueryFrame(e.get());
    if (legendFrame) {
      
      
      
      
      
      childItems.RemoveFrame(legendFrame);
      
      fieldsetKids.InsertFrame(newFrame, nsnull, legendFrame);
      break;
    }
  }

  
  blockFrame->SetInitialChildList(nsnull, childItems);

  
  newFrame->SetInitialChildList(nsnull, fieldsetKids);

  
  *aNewFrame = newFrame; 

  return NS_OK;
}

static nsIFrame*
FindAncestorWithGeneratedContentPseudo(nsIFrame* aFrame)
{
  for (nsIFrame* f = aFrame->GetParent(); f; f = f->GetParent()) {
    NS_ASSERTION(f->IsGeneratedContentFrame(),
                 "should not have exited generated content");
    nsIAtom* pseudo = f->GetStyleContext()->GetPseudo();
    if (pseudo == nsCSSPseudoElements::before ||
        pseudo == nsCSSPseudoElements::after)
      return f;
  }
  return nsnull;
}

#define SIMPLE_FCDATA(_func) FCDATA_DECL(0, _func)
#define FULL_CTOR_FCDATA(_flags, _func)                     \
  { _flags | FCDATA_FUNC_IS_FULL_CTOR, { nsnull }, _func }


const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindTextData(nsIFrame* aParentFrame)
{
  if (aParentFrame && aParentFrame->IsFrameOfType(nsIFrame::eSVG)) {
    nsIFrame *ancestorFrame =
      nsSVGUtils::GetFirstNonAAncestorFrame(aParentFrame);
    if (ancestorFrame) {
      nsSVGTextContainerFrame* metrics = do_QueryFrame(ancestorFrame);
      if (metrics) {
        static const FrameConstructionData sSVGGlyphData =
          SIMPLE_FCDATA(NS_NewSVGGlyphFrame);
        return &sSVGGlyphData;
      }
    }
    return nsnull;
  }

  static const FrameConstructionData sTextData =
    FCDATA_DECL(FCDATA_IS_LINE_PARTICIPANT, NS_NewTextFrame);
  return &sTextData;
}

nsresult
nsCSSFrameConstructor::ConstructTextFrame(const FrameConstructionData* aData,
                                          nsFrameConstructorState& aState,
                                          nsIContent*              aContent,
                                          nsIFrame*                aParentFrame,
                                          nsStyleContext*          aStyleContext,
                                          nsFrameItems&            aFrameItems)
{
  NS_PRECONDITION(aData, "Must have frame construction data");

  nsIFrame* newFrame = (*aData->mFunc.mCreationFunc)(mPresShell, aStyleContext);

  if (NS_UNLIKELY(!newFrame))
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv = InitAndRestoreFrame(aState, aContent, aParentFrame,
                                    nsnull, newFrame);

  if (NS_FAILED(rv)) {
    newFrame->Destroy();
    return rv;
  }

  

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
  
  return rv;
}


const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindDataByInt(PRInt32 aInt,
                                     Element* aElement,
                                     nsStyleContext* aStyleContext,
                                     const FrameConstructionDataByInt* aDataPtr,
                                     PRUint32 aDataLength)
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

  return nsnull;
}


const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindDataByTag(nsIAtom* aTag,
                                     Element* aElement,
                                     nsStyleContext* aStyleContext,
                                     const FrameConstructionDataByTag* aDataPtr,
                                     PRUint32 aDataLength)
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

  return nsnull;
}

#define SUPPRESS_FCDATA() FCDATA_DECL(FCDATA_SUPPRESS_FRAME, nsnull)
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


const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindHTMLData(Element* aElement,
                                    nsIAtom* aTag,
                                    PRInt32 aNameSpaceID,
                                    nsIFrame* aParentFrame,
                                    nsStyleContext* aStyleContext)
{
  
  
  
  if (aNameSpaceID != kNameSpaceID_XHTML) {
    return nsnull;
  }

  NS_ASSERTION(!aParentFrame ||
               aParentFrame->GetStyleContext()->GetPseudo() !=
                 nsCSSAnonBoxes::fieldsetContent ||
               aParentFrame->GetParent()->GetType() == nsGkAtoms::fieldSetFrame,
               "Unexpected parent for fieldset content anon box");
  if (aTag == nsGkAtoms::legend &&
      (!aParentFrame ||
       (aParentFrame->GetType() != nsGkAtoms::fieldSetFrame &&
        aParentFrame->GetStyleContext()->GetPseudo() !=
          nsCSSAnonBoxes::fieldsetContent) ||
       !aElement->GetParent() ||
       !aElement->GetParent()->IsHTML(nsGkAtoms::fieldset) ||
       aStyleContext->GetStyleDisplay()->IsFloating() ||
       aStyleContext->GetStyleDisplay()->IsAbsolutelyPositioned())) {
    
    
    
    
    
    
    
    
    return nsnull;
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
      FCDATA_DECL(FCDATA_ALLOW_BLOCK_STYLES, NS_NewLegendFrame) },
    SIMPLE_TAG_CREATE(frameset, NS_NewHTMLFramesetFrame),
    SIMPLE_TAG_CREATE(iframe, NS_NewSubDocumentFrame),
    COMPLEX_TAG_CREATE(button, &nsCSSFrameConstructor::ConstructButtonFrame),
    SIMPLE_TAG_CREATE(canvas, NS_NewHTMLCanvasFrame),
#if defined(MOZ_MEDIA)
    SIMPLE_TAG_CREATE(video, NS_NewHTMLVideoFrame),
    SIMPLE_TAG_CREATE(audio, NS_NewHTMLVideoFrame),
#endif
    SIMPLE_TAG_CREATE(isindex, NS_NewIsIndexFrame),
    SIMPLE_TAG_CREATE(progress, NS_NewProgressFrame)
  };

  return FindDataByTag(aTag, aElement, aStyleContext, sHTMLData,
                       NS_ARRAY_LENGTH(sHTMLData));
}


const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindImgData(Element* aElement,
                                   nsStyleContext* aStyleContext)
{
  if (!nsImageFrame::ShouldCreateImageFrameFor(aElement, aStyleContext)) {
    return nsnull;
  }

  static const FrameConstructionData sImgData = SIMPLE_FCDATA(NS_NewImageFrame);
  return &sImgData;
}


const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindImgControlData(Element* aElement,
                                          nsStyleContext* aStyleContext)
{
  if (!nsImageFrame::ShouldCreateImageFrameFor(aElement, aStyleContext)) {
    return nsnull;
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
    SIMPLE_INT_CREATE(NS_FORM_INPUT_PASSWORD, NS_NewTextControlFrame),
    COMPLEX_INT_CREATE(NS_FORM_INPUT_SUBMIT,
                       &nsCSSFrameConstructor::ConstructButtonFrame),
    COMPLEX_INT_CREATE(NS_FORM_INPUT_RESET,
                       &nsCSSFrameConstructor::ConstructButtonFrame),
    COMPLEX_INT_CREATE(NS_FORM_INPUT_BUTTON,
                       &nsCSSFrameConstructor::ConstructButtonFrame)
    
    
  };

  nsCOMPtr<nsIFormControl> control = do_QueryInterface(aElement);
  NS_ASSERTION(control, "input doesn't implement nsIFormControl?");

  return FindDataByInt(control->GetType(), aElement, aStyleContext,
                       sInputData, NS_ARRAY_LENGTH(sInputData));
}


const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindObjectData(Element* aElement,
                                      nsStyleContext* aStyleContext)
{
  
  
  
  PRUint32 type;
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

  return FindDataByInt((PRInt32)type, aElement, aStyleContext,
                       sObjectData, NS_ARRAY_LENGTH(sObjectData));
}

nsresult
nsCSSFrameConstructor::ConstructFrameFromItemInternal(FrameConstructionItem& aItem,
                                                      nsFrameConstructorState& aState,
                                                      nsIFrame* aParentFrame,
                                                      nsFrameItems& aFrameItems)
{
  const FrameConstructionData* data = aItem.mFCData;
  NS_ASSERTION(data, "Must have frame construction data");

  PRUint32 bits = data->mBits;

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
#undef CHECK_ONLY_ONE_BIT
  NS_ASSERTION(!(bits & FCDATA_FORCED_NON_SCROLLABLE_BLOCK) ||
               ((bits & FCDATA_FUNC_IS_FULL_CTOR) &&
                data->mFullConstructor ==
                  &nsCSSFrameConstructor::ConstructNonScrollableBlock),
               "Unexpected FCDATA_FORCED_NON_SCROLLABLE_BLOCK flag");

  
  if (aState.mCreatingExtraFrames && aItem.mContent->IsHTML() &&
      aItem.mContent->Tag() == nsGkAtoms::iframe)
  {
    return NS_OK;
  }

  nsStyleContext* const styleContext = aItem.mStyleContext;
  const nsStyleDisplay* display = styleContext->GetStyleDisplay();

  nsIFrame* newFrame;
  nsIFrame* primaryFrame;
  if (bits & FCDATA_FUNC_IS_FULL_CTOR) {
    nsresult rv =
      (this->*(data->mFullConstructor))(aState, aItem, aParentFrame,
                                        display, aFrameItems, &newFrame);
    if (NS_FAILED(rv)) {
      return rv;
    }

    primaryFrame = newFrame;
  } else {
    nsIContent* const content = aItem.mContent;

    newFrame =
      (*data->mFunc.mCreationFunc)(mPresShell, styleContext);
    if (!newFrame) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    PRBool allowOutOfFlow = !(bits & FCDATA_DISALLOW_OUT_OF_FLOW);
    PRBool isPopup = aItem.mIsPopup;
    NS_ASSERTION(!isPopup ||
                 (aState.mPopupItems.containingBlock &&
                  aState.mPopupItems.containingBlock->GetType() ==
                    nsGkAtoms::popupSetFrame),
                 "Should have a containing block here!");

    nsIFrame* geometricParent =
      isPopup ? aState.mPopupItems.containingBlock :
      (allowOutOfFlow ? aState.GetGeometricParent(display, aParentFrame)
                      : aParentFrame);

    nsresult rv = NS_OK;

    
    nsIFrame* frameToAddToList = nsnull;
    if ((bits & FCDATA_MAY_NEED_SCROLLFRAME) &&
        display->IsScrollableOverflow()) {
      BuildScrollFrame(aState, content, styleContext, newFrame,
                       geometricParent, frameToAddToList);
    } else {
      rv = InitAndRestoreFrame(aState, content, geometricParent, nsnull,
                               newFrame);
      NS_ASSERTION(NS_SUCCEEDED(rv), "InitAndRestoreFrame failed");
      
      nsContainerFrame::CreateViewForFrame(newFrame, PR_FALSE);
      frameToAddToList = newFrame;
    }

    
    
    
    
    primaryFrame = frameToAddToList;

    rv = aState.AddChild(frameToAddToList, aFrameItems, content, styleContext,
                         aParentFrame, allowOutOfFlow, allowOutOfFlow, isPopup);
    if (NS_FAILED(rv)) {
      return rv;
    }

#ifdef MOZ_XUL
    

    if (aItem.mIsRootPopupgroup) {
      NS_ASSERTION(nsIRootBox::GetRootBox(mPresShell) &&
                   nsIRootBox::GetRootBox(mPresShell)->GetPopupSetFrame() ==
                     newFrame,
                   "Unexpected PopupSetFrame");
      aState.mPopupItems.containingBlock = newFrame;
      aState.mHavePendingPopupgroup = PR_FALSE;
    }
#endif 

    
    nsFrameItems childItems;
    nsFrameConstructorSaveState absoluteSaveState;

    if (bits & FCDATA_FORCE_NULL_ABSPOS_CONTAINER) {
      aState.PushAbsoluteContainingBlock(nsnull, absoluteSaveState);
    } else if (!(bits & FCDATA_SKIP_ABSPOS_PUSH) && display->IsPositioned()) {
      aState.PushAbsoluteContainingBlock(newFrame, absoluteSaveState);
    }

    if (bits & FCDATA_USE_CHILD_ITEMS) {
      rv = ConstructFramesFromItemList(aState, aItem.mChildItems, newFrame,
                                       childItems);
    } else {
      
      rv = ProcessChildren(aState, content, styleContext, newFrame,
                           !(bits & FCDATA_DISALLOW_GENERATED_CONTENT),
                           childItems,
                           (bits & FCDATA_ALLOW_BLOCK_STYLES) != 0,
                           aItem.mPendingBinding);
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

    if (NS_SUCCEEDED(rv) && (bits & FCDATA_WRAP_KIDS_IN_BLOCKS)) {
      nsFrameItems newItems;
      nsFrameItems currentBlock;
      nsIFrame* f;
      while ((f = childItems.FirstChild()) != nsnull) {
        PRBool wrapFrame = IsInlineFrame(f) || IsFrameSpecial(f);
        if (!wrapFrame) {
          rv = FlushAccumulatedBlock(aState, content, newFrame, &currentBlock, &newItems);
          if (NS_FAILED(rv))
            break;
        }

        childItems.RemoveFrame(f);
        if (wrapFrame) {
          currentBlock.AddChild(f);
        } else {
          newItems.AddChild(f);
        }
      }
      rv = FlushAccumulatedBlock(aState, content, newFrame, &currentBlock, &newItems);

      if (childItems.NotEmpty()) {
        
        childItems.DestroyFrames();
      }

      childItems = newItems;
    }

    
    
    
    newFrame->SetInitialChildList(nsnull, childItems);
  }

  NS_ASSERTION(newFrame->IsFrameOfType(nsIFrame::eLineParticipant) ==
               ((bits & FCDATA_IS_LINE_PARTICIPANT) != 0),
               "Incorrectly set FCDATA_IS_LINE_PARTICIPANT bits");

  if (!aState.mCreatingExtraFrames && !(bits & FCDATA_SKIP_FRAMESET)) {
    aItem.mContent->SetPrimaryFrame(primaryFrame);
  }

  return NS_OK;
}



nsresult
nsCSSFrameConstructor::CreateAnonymousFrames(nsFrameConstructorState& aState,
                                             nsIContent*              aParent,
                                             nsIFrame*                aParentFrame,
                                             PendingBinding*          aPendingBinding,
                                             nsFrameItems&            aChildItems)
{
  nsAutoTArray<nsIAnonymousContentCreator::ContentInfo, 4> newAnonymousItems;
  nsresult rv = GetAnonymousContent(aParent, aParentFrame, newAnonymousItems);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 count = newAnonymousItems.Length();
  if (count == 0) {
    return NS_OK;
  }

  nsFrameConstructorState::PendingBindingAutoPusher pusher(aState,
                                                           aPendingBinding);

  nsIAnonymousContentCreator* creator = do_QueryFrame(aParentFrame);
  NS_ASSERTION(creator,
               "How can that happen if we have nodes to construct frames for?");

  for (PRUint32 i=0; i < count; i++) {
    nsIContent* content = newAnonymousItems[i].mContent;
    NS_ASSERTION(content, "null anonymous content?");
    NS_ASSERTION(!newAnonymousItems[i].mStyleContext, "Unexpected style context");

    nsIFrame* newFrame = creator->CreateFrameFor(content);
    if (newFrame) {
      NS_ASSERTION(content->GetPrimaryFrame(),
                   "Content must have a primary frame now");
      aChildItems.AddChild(newFrame);
    }
    else {
      
      ConstructFrame(aState, content, aParentFrame, aChildItems);
    }
  }

  return NS_OK;
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

  PRUint32 count = aContent.Length();
  for (PRUint32 i=0; i < count; i++) {
    
    nsIContent* content = aContent[i].mContent;
    NS_ASSERTION(content, "null anonymous content?");

    
    
    if (aParent &&
        aParent->NodeInfo()->Equals(nsGkAtoms::use, kNameSpaceID_SVG)) {
      content->SetFlags(NODE_IS_ANONYMOUS);
    } else {
      content->SetNativeAnonymous();
    }

    rv = content->BindToTree(mDocument, aParent, aParent, PR_TRUE);
    if (NS_FAILED(rv)) {
      content->UnbindFromTree();
      return rv;
    }
  }

  return NS_OK;
}

static
PRBool IsXULDisplayType(const nsStyleDisplay* aDisplay)
{
  return (aDisplay->mDisplay == NS_STYLE_DISPLAY_INLINE_BOX || 
#ifdef MOZ_XUL
          aDisplay->mDisplay == NS_STYLE_DISPLAY_INLINE_GRID || 
          aDisplay->mDisplay == NS_STYLE_DISPLAY_INLINE_STACK ||
#endif
          aDisplay->mDisplay == NS_STYLE_DISPLAY_BOX
#ifdef MOZ_XUL
          || aDisplay->mDisplay == NS_STYLE_DISPLAY_GRID ||
          aDisplay->mDisplay == NS_STYLE_DISPLAY_STACK ||
          aDisplay->mDisplay == NS_STYLE_DISPLAY_GRID_GROUP ||
          aDisplay->mDisplay == NS_STYLE_DISPLAY_GRID_LINE ||
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
#define SIMPLE_XUL_CREATE(_tag, _func)            \
  { &nsGkAtoms::_tag, SIMPLE_XUL_FCDATA(_func) }
#define SCROLLABLE_XUL_CREATE(_tag, _func)            \
  { &nsGkAtoms::_tag, SCROLLABLE_XUL_FCDATA(_func) }
#define SIMPLE_XUL_INT_CREATE(_int, _func)      \
  { _int, SIMPLE_XUL_FCDATA(_func) }
#define SCROLLABLE_XUL_INT_CREATE(_int, _func)                          \
  { _int, SCROLLABLE_XUL_FCDATA(_func) }

static
nsIFrame* NS_NewGridBoxFrame(nsIPresShell* aPresShell,
                             nsStyleContext* aStyleContext)
{
  nsCOMPtr<nsIBoxLayout> layout;
  NS_NewGridLayout2(aPresShell, getter_AddRefs(layout));
  if (!layout) {
    return nsnull;
  }

  return NS_NewBoxFrame(aPresShell, aStyleContext, PR_FALSE, layout);
}


const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindXULTagData(Element* aElement,
                                      nsIAtom* aTag,
                                      PRInt32 aNameSpaceID,
                                      nsStyleContext* aStyleContext)
{
  if (aNameSpaceID != kNameSpaceID_XUL) {
    return nsnull;
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
                       NS_ARRAY_LENGTH(sXULTagData));
}

#ifdef MOZ_XUL

const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindPopupGroupData(Element* aElement,
                                          nsStyleContext* )
{
  if (!aElement->IsRootOfNativeAnonymousSubtree()) {
    return nsnull;
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
  nsCOMPtr<nsISupports> container =
    aStyleContext->PresContext()->GetContainer();
  if (container) {
    nsCOMPtr<nsIDocShellTreeItem> treeItem(do_QueryInterface(container));
    if (treeItem) {
      PRInt32 type;
      treeItem->GetItemType(&type);
      if (nsIDocShellTreeItem::typeChrome == type) {
        nsCOMPtr<nsIDocShellTreeItem> parent;
        treeItem->GetParent(getter_AddRefs(parent));
        if (!parent) {
          
          
          static const FrameConstructionData sSuppressData = SUPPRESS_FCDATA();
          return &sSuppressData;
        }
      }
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
  if (aStyleContext->GetStyleDisplay()->mDisplay !=
        NS_STYLE_DISPLAY_GRID_GROUP) {
    return nsnull;
  }

  static const FrameConstructionData sListBoxBodyData =
    SCROLLABLE_XUL_FCDATA(NS_NewListBoxBodyFrame);
  return &sListBoxBodyData;
}


const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindXULListItemData(Element* aElement,
                                           nsStyleContext* aStyleContext)
{
  if (aStyleContext->GetStyleDisplay()->mDisplay !=
        NS_STYLE_DISPLAY_GRID_LINE) {
    return nsnull;
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
    SCROLLABLE_XUL_INT_CREATE(NS_STYLE_DISPLAY_INLINE_BOX, NS_NewBoxFrame),
    SCROLLABLE_XUL_INT_CREATE(NS_STYLE_DISPLAY_BOX, NS_NewBoxFrame),
#ifdef MOZ_XUL
    SCROLLABLE_XUL_INT_CREATE(NS_STYLE_DISPLAY_INLINE_GRID, NS_NewGridBoxFrame),
    SCROLLABLE_XUL_INT_CREATE(NS_STYLE_DISPLAY_GRID, NS_NewGridBoxFrame),
    SCROLLABLE_XUL_INT_CREATE(NS_STYLE_DISPLAY_GRID_GROUP,
                              NS_NewGridRowGroupFrame),
    SCROLLABLE_XUL_INT_CREATE(NS_STYLE_DISPLAY_GRID_LINE,
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
                       sXULDisplayData, NS_ARRAY_LENGTH(sXULDisplayData));
}

already_AddRefed<nsStyleContext>
nsCSSFrameConstructor::BeginBuildingScrollFrame(nsFrameConstructorState& aState,
                                                nsIContent*              aContent,
                                                nsStyleContext*          aContentStyle,
                                                nsIFrame*                aParentFrame,
                                                nsIAtom*                 aScrolledPseudo,
                                                PRBool                   aIsRoot,
                                                nsIFrame*&               aNewFrame)
{
  nsIFrame* gfxScrollFrame = aNewFrame;

  nsFrameItems anonymousItems;

  nsRefPtr<nsStyleContext> contentStyle = aContentStyle;

  if (!gfxScrollFrame) {
    
    
    
    
    if (IsXULDisplayType(aContentStyle->GetStyleDisplay())) {
      gfxScrollFrame = NS_NewXULScrollFrame(mPresShell, contentStyle, aIsRoot);
    } else {
      gfxScrollFrame = NS_NewHTMLScrollFrame(mPresShell, contentStyle, aIsRoot);
    }

    InitAndRestoreFrame(aState, aContent, aParentFrame, nsnull, gfxScrollFrame);
  }

  
  
  
  
  
  CreateAnonymousFrames(aState, aContent, gfxScrollFrame, nsnull,
                        anonymousItems);

  aNewFrame = gfxScrollFrame;

  
  nsStyleSet *styleSet = mPresShell->StyleSet();
  nsStyleContext* aScrolledChildStyle =
    styleSet->ResolveAnonymousBoxStyle(aScrolledPseudo, contentStyle).get();

  if (gfxScrollFrame) {
     gfxScrollFrame->SetInitialChildList(nsnull, anonymousItems);
  }

  return aScrolledChildStyle;
}

void
nsCSSFrameConstructor::FinishBuildingScrollFrame(nsIFrame* aScrollFrame,
                                                 nsIFrame* aScrolledFrame)
{
  nsFrameList scrolled(aScrolledFrame, aScrolledFrame);
  aScrollFrame->AppendFrames(nsnull, scrolled);
}

































nsresult
nsCSSFrameConstructor::BuildScrollFrame(nsFrameConstructorState& aState,
                                        nsIContent*              aContent,
                                        nsStyleContext*          aContentStyle,
                                        nsIFrame*                aScrolledFrame,
                                        nsIFrame*                aParentFrame,
                                        nsIFrame*&               aNewFrame)
{
    nsRefPtr<nsStyleContext> scrolledContentStyle =
      BeginBuildingScrollFrame(aState, aContent, aContentStyle, aParentFrame,
                               nsCSSAnonBoxes::scrolledContent,
                               PR_FALSE, aNewFrame);
    
    aScrolledFrame->SetStyleContextWithoutNotification(scrolledContentStyle);
    InitAndRestoreFrame(aState, aContent, aNewFrame, nsnull, aScrolledFrame);

    FinishBuildingScrollFrame(aNewFrame, aScrolledFrame);
    return NS_OK;
}

const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindDisplayData(const nsStyleDisplay* aDisplay,
                                       Element* aElement,
                                       nsStyleContext* aStyleContext)
{
  PR_STATIC_ASSERT(eParentTypeCount < (1 << (32 - FCDATA_PARENT_TYPE_OFFSET)));

  
  
  NS_ASSERTION(!(aDisplay->IsFloating() ||
                 aDisplay->IsAbsolutelyPositioned()) ||
               aDisplay->IsBlockOutside(),
               "Style system did not apply CSS2.1 section 9.7 fixups");

  
  
  
  
  
  
  PRBool propagatedScrollToViewport = PR_FALSE;
  if (aElement->IsHTML(nsGkAtoms::body)) {
    propagatedScrollToViewport =
      PropagateScrollToViewport() == aElement;
  }

  NS_ASSERTION(!propagatedScrollToViewport ||
               !mPresShell->GetPresContext()->IsPaginated(),
               "Shouldn't propagate scroll in paginated contexts");

  
  
  
  
  
  if (aDisplay->IsBlockInside() &&
      aDisplay->IsScrollableOverflow() &&
      !propagatedScrollToViewport) {
    
    
    
    if (mPresShell->GetPresContext()->IsPaginated() &&
        aDisplay->IsBlockOutside() &&
        !aElement->IsInNativeAnonymousSubtree()) {
      static const FrameConstructionData sForcedNonScrollableBlockData =
        FULL_CTOR_FCDATA(FCDATA_FORCED_NON_SCROLLABLE_BLOCK,
                         &nsCSSFrameConstructor::ConstructNonScrollableBlock);
      return &sForcedNonScrollableBlockData;
    }

    static const FrameConstructionData sScrollableBlockData =
      FULL_CTOR_FCDATA(0, &nsCSSFrameConstructor::ConstructScrollableBlock);
    return &sScrollableBlockData;
  }

  
  if (aDisplay->IsBlockInside()) {
    static const FrameConstructionData sNonScrollableBlockData =
      FULL_CTOR_FCDATA(0, &nsCSSFrameConstructor::ConstructNonScrollableBlock);
    return &sNonScrollableBlockData;
  }

  static const FrameConstructionDataByInt sDisplayData[] = {
    
    
    
    
    { NS_STYLE_DISPLAY_INLINE,
      FULL_CTOR_FCDATA(FCDATA_IS_INLINE | FCDATA_IS_LINE_PARTICIPANT,
                       &nsCSSFrameConstructor::ConstructInline) },
    { NS_STYLE_DISPLAY_TABLE,
      FULL_CTOR_FCDATA(0, &nsCSSFrameConstructor::ConstructTable) },
    { NS_STYLE_DISPLAY_INLINE_TABLE,
      FULL_CTOR_FCDATA(0, &nsCSSFrameConstructor::ConstructTable) },
    { NS_STYLE_DISPLAY_TABLE_CAPTION,
      FCDATA_DECL(FCDATA_IS_TABLE_PART | FCDATA_ALLOW_BLOCK_STYLES |
                  FCDATA_DISALLOW_OUT_OF_FLOW | FCDATA_SKIP_ABSPOS_PUSH |
                  FCDATA_DESIRED_PARENT_TYPE_TO_BITS(eTypeTable),
                  NS_NewTableCaptionFrame) },
    { NS_STYLE_DISPLAY_TABLE_ROW_GROUP,
      FCDATA_DECL(FCDATA_IS_TABLE_PART | FCDATA_DISALLOW_OUT_OF_FLOW |
                  FCDATA_SKIP_ABSPOS_PUSH |
                  FCDATA_DESIRED_PARENT_TYPE_TO_BITS(eTypeTable),
                  NS_NewTableRowGroupFrame) },
    { NS_STYLE_DISPLAY_TABLE_HEADER_GROUP,
      FCDATA_DECL(FCDATA_IS_TABLE_PART | FCDATA_DISALLOW_OUT_OF_FLOW |
                  FCDATA_SKIP_ABSPOS_PUSH |
                  FCDATA_DESIRED_PARENT_TYPE_TO_BITS(eTypeTable),
                  NS_NewTableRowGroupFrame) },
    { NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP,
      FCDATA_DECL(FCDATA_IS_TABLE_PART | FCDATA_DISALLOW_OUT_OF_FLOW |
                  FCDATA_SKIP_ABSPOS_PUSH |
                  FCDATA_DESIRED_PARENT_TYPE_TO_BITS(eTypeTable),
                  NS_NewTableRowGroupFrame) },
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
                       &nsCSSFrameConstructor::ConstructTableRow) },
    { NS_STYLE_DISPLAY_TABLE_CELL,
      FULL_CTOR_FCDATA(FCDATA_IS_TABLE_PART |
                       FCDATA_DESIRED_PARENT_TYPE_TO_BITS(eTypeRow),
                       &nsCSSFrameConstructor::ConstructTableCell) }
  };

  return FindDataByInt(aDisplay->mDisplay, aElement, aStyleContext,
                       sDisplayData, NS_ARRAY_LENGTH(sDisplayData));
}

nsresult
nsCSSFrameConstructor::ConstructScrollableBlock(nsFrameConstructorState& aState,
                                                FrameConstructionItem&   aItem,
                                                nsIFrame*                aParentFrame,
                                                const nsStyleDisplay*    aDisplay,
                                                nsFrameItems&            aFrameItems,
                                                nsIFrame**               aNewFrame)
{
  nsIContent* const content = aItem.mContent;
  nsStyleContext* const styleContext = aItem.mStyleContext;

  *aNewFrame = nsnull;
  nsRefPtr<nsStyleContext> scrolledContentStyle
    = BeginBuildingScrollFrame(aState, content, styleContext,
                               aState.GetGeometricParent(aDisplay, aParentFrame),
                               nsCSSAnonBoxes::scrolledContent,
                               PR_FALSE, *aNewFrame);

  
  
  nsIFrame* scrolledFrame =
    NS_NewBlockFormattingContext(mPresShell, styleContext);

  nsFrameItems blockItem;
  nsresult rv = ConstructBlock(aState,
                               scrolledContentStyle->GetStyleDisplay(), content,
                               *aNewFrame, *aNewFrame, scrolledContentStyle,
                               &scrolledFrame, blockItem, aDisplay->IsPositioned(),
                               aItem.mPendingBinding);
  if (NS_UNLIKELY(NS_FAILED(rv))) {
    
    return rv;
  }

  NS_ASSERTION(blockItem.FirstChild() == scrolledFrame,
               "Scrollframe's frameItems should be exactly the scrolled frame");
  FinishBuildingScrollFrame(*aNewFrame, scrolledFrame);

  rv = aState.AddChild(*aNewFrame, aFrameItems, content, styleContext,
                       aParentFrame);
  return rv;
}

nsresult
nsCSSFrameConstructor::ConstructNonScrollableBlock(nsFrameConstructorState& aState,
                                                   FrameConstructionItem&   aItem,
                                                   nsIFrame*                aParentFrame,
                                                   const nsStyleDisplay*    aDisplay,
                                                   nsFrameItems&            aFrameItems,
                                                   nsIFrame**               aNewFrame)
{
  nsStyleContext* const styleContext = aItem.mStyleContext;

  
  
  
  
  PRBool clipPaginatedOverflow =
    (aItem.mFCData->mBits & FCDATA_FORCED_NON_SCROLLABLE_BLOCK) != 0;
  if (aDisplay->IsAbsolutelyPositioned() ||
      aDisplay->IsFloating() ||
      NS_STYLE_DISPLAY_INLINE_BLOCK == aDisplay->mDisplay ||
      clipPaginatedOverflow) {
    *aNewFrame = NS_NewBlockFormattingContext(mPresShell, styleContext);
    if (clipPaginatedOverflow) {
      (*aNewFrame)->AddStateBits(NS_BLOCK_CLIP_PAGINATED_OVERFLOW);
    }
  } else {
    *aNewFrame = NS_NewBlockFrame(mPresShell, styleContext);
  }

  return ConstructBlock(aState, aDisplay, aItem.mContent,
                        aState.GetGeometricParent(aDisplay, aParentFrame),
                        aParentFrame, styleContext, aNewFrame,
                        aFrameItems, aDisplay->IsPositioned(),
                        aItem.mPendingBinding);
}


nsresult 
nsCSSFrameConstructor::InitAndRestoreFrame(const nsFrameConstructorState& aState,
                                           nsIContent*              aContent,
                                           nsIFrame*                aParentFrame,
                                           nsIFrame*                aPrevInFlow,
                                           nsIFrame*                aNewFrame,
                                           PRBool                   aAllowCounters)
{
  NS_PRECONDITION(mUpdateCount != 0,
                  "Should be in an update while creating frames");
  
  nsresult rv = NS_OK;
  
  NS_ASSERTION(aNewFrame, "Null frame cannot be initialized");
  if (!aNewFrame)
    return NS_ERROR_NULL_POINTER;

  
  rv = aNewFrame->Init(aContent, aParentFrame, aPrevInFlow);
  aNewFrame->AddStateBits(aState.mAdditionalStateBits);

  if (aState.mFrameState && aState.mFrameManager) {
    
    aState.mFrameManager->RestoreFrameStateFor(aNewFrame, aState.mFrameState);
  }

  if (aAllowCounters && !aPrevInFlow &&
      mCounterManager.AddCounterResetsAndIncrements(aNewFrame)) {
    CountersDirty();
  }

  return rv;
}

already_AddRefed<nsStyleContext>
nsCSSFrameConstructor::ResolveStyleContext(nsIFrame*         aParentFrame,
                                           nsIContent*       aContent,
                                           nsFrameConstructorState* aState)
{
  nsStyleContext* parentStyleContext = nsnull;
  NS_ASSERTION(aContent->GetParent(), "Must have parent here");

  aParentFrame = nsFrame::CorrectStyleParentFrame(aParentFrame, nsnull);

  if (aParentFrame) {
    
    
    parentStyleContext = aParentFrame->GetStyleContext();
  } else {
    
    
    
    
    
  }

  return ResolveStyleContext(parentStyleContext, aContent, aState);
}

already_AddRefed<nsStyleContext>
nsCSSFrameConstructor::ResolveStyleContext(nsStyleContext* aParentStyleContext,
                                           nsIContent* aContent,
                                           nsFrameConstructorState* aState)
{
  nsStyleSet *styleSet = mPresShell->StyleSet();

  if (aContent->IsElement()) {
    if (aState) {
      return styleSet->ResolveStyleFor(aContent->AsElement(),
                                       aParentStyleContext,
                                       aState->mTreeMatchContext);
    }
    return styleSet->ResolveStyleFor(aContent->AsElement(), aParentStyleContext);

  }

  NS_ASSERTION(aContent->IsNodeOfType(nsINode::eTEXT),
               "shouldn't waste time creating style contexts for "
               "comments and processing instructions");

  return styleSet->ResolveStyleForNonElement(aParentStyleContext);
}


nsresult
nsCSSFrameConstructor::FlushAccumulatedBlock(nsFrameConstructorState& aState,
                                             nsIContent* aContent,
                                             nsIFrame* aParentFrame,
                                             nsFrameItems* aBlockItems,
                                             nsFrameItems* aNewItems)
{
  if (aBlockItems->IsEmpty()) {
    
    return NS_OK;
  }

  nsStyleContext* parentContext =
    nsFrame::CorrectStyleParentFrame(aParentFrame,
                                     nsCSSAnonBoxes::mozMathMLAnonymousBlock)->GetStyleContext(); 
  nsStyleSet *styleSet = mPresShell->StyleSet();
  nsRefPtr<nsStyleContext> blockContext;
  blockContext = styleSet->
    ResolveAnonymousBoxStyle(nsCSSAnonBoxes::mozMathMLAnonymousBlock,
                             parentContext);

  
  
  
  nsIFrame* blockFrame = NS_NewMathMLmathBlockFrame(mPresShell, blockContext,
                          NS_BLOCK_FLOAT_MGR | NS_BLOCK_MARGIN_ROOT);
  if (NS_UNLIKELY(!blockFrame))
    return NS_ERROR_OUT_OF_MEMORY;

  InitAndRestoreFrame(aState, aContent, aParentFrame, nsnull, blockFrame);
  ReparentFrames(aState.mFrameManager, blockFrame, *aBlockItems);
  
  
  blockFrame->SetInitialChildList(nsnull, *aBlockItems);
  NS_ASSERTION(aBlockItems->IsEmpty(), "What happened?");
  aBlockItems->Clear();
  aNewItems->AddChild(blockFrame);
  return NS_OK;
}



#define SIMPLE_MATHML_CREATE(_tag, _func)                               \
  { &nsGkAtoms::_tag,                                                   \
      FCDATA_DECL(FCDATA_DISALLOW_OUT_OF_FLOW |                         \
                  FCDATA_FORCE_NULL_ABSPOS_CONTAINER |                  \
                  FCDATA_WRAP_KIDS_IN_BLOCKS, _func) }


const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindMathMLData(Element* aElement,
                                      nsIAtom* aTag,
                                      PRInt32 aNameSpaceID,
                                      nsStyleContext* aStyleContext)
{
  
  if (aNameSpaceID != kNameSpaceID_MathML) 
    return nsnull;

  
  if (aTag == nsGkAtoms::math) {
    
    
    
    if (aStyleContext->GetStyleDisplay()->IsBlockOutside()) {
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
    SIMPLE_MATHML_CREATE(mi_, NS_NewMathMLTokenFrame),
    SIMPLE_MATHML_CREATE(mn_, NS_NewMathMLTokenFrame),
    SIMPLE_MATHML_CREATE(ms_, NS_NewMathMLTokenFrame),
    SIMPLE_MATHML_CREATE(mtext_, NS_NewMathMLTokenFrame),
    SIMPLE_MATHML_CREATE(mo_, NS_NewMathMLmoFrame),
    SIMPLE_MATHML_CREATE(mfrac_, NS_NewMathMLmfracFrame),
    SIMPLE_MATHML_CREATE(msup_, NS_NewMathMLmsupFrame),
    SIMPLE_MATHML_CREATE(msub_, NS_NewMathMLmsubFrame),
    SIMPLE_MATHML_CREATE(msubsup_, NS_NewMathMLmsubsupFrame),
    SIMPLE_MATHML_CREATE(munder_, NS_NewMathMLmunderFrame),
    SIMPLE_MATHML_CREATE(mover_, NS_NewMathMLmoverFrame),
    SIMPLE_MATHML_CREATE(munderover_, NS_NewMathMLmunderoverFrame),
    SIMPLE_MATHML_CREATE(mphantom_, NS_NewMathMLmphantomFrame),
    SIMPLE_MATHML_CREATE(mpadded_, NS_NewMathMLmpaddedFrame),
    SIMPLE_MATHML_CREATE(mspace_, NS_NewMathMLmspaceFrame),
    SIMPLE_MATHML_CREATE(none, NS_NewMathMLmspaceFrame),
    SIMPLE_MATHML_CREATE(mprescripts_, NS_NewMathMLmspaceFrame),
    SIMPLE_MATHML_CREATE(mfenced_, NS_NewMathMLmfencedFrame),
    SIMPLE_MATHML_CREATE(mmultiscripts_, NS_NewMathMLmmultiscriptsFrame),
    SIMPLE_MATHML_CREATE(mstyle_, NS_NewMathMLmstyleFrame),
    SIMPLE_MATHML_CREATE(msqrt_, NS_NewMathMLmsqrtFrame),
    SIMPLE_MATHML_CREATE(mroot_, NS_NewMathMLmrootFrame),
    SIMPLE_MATHML_CREATE(maction_, NS_NewMathMLmactionFrame),
    SIMPLE_MATHML_CREATE(mrow_, NS_NewMathMLmrowFrame),
    SIMPLE_MATHML_CREATE(merror_, NS_NewMathMLmrowFrame),
    SIMPLE_MATHML_CREATE(menclose_, NS_NewMathMLmencloseFrame),
    SIMPLE_MATHML_CREATE(semantics_, NS_NewMathMLsemanticsFrame)
  };

  return FindDataByTag(aTag, aElement, aStyleContext, sMathMLData,
                       NS_ARRAY_LENGTH(sMathMLData));
}



#define SIMPLE_SVG_FCDATA(_func)                                        \
  FCDATA_DECL(FCDATA_DISALLOW_OUT_OF_FLOW |                             \
              FCDATA_SKIP_ABSPOS_PUSH |                                 \
              FCDATA_DISALLOW_GENERATED_CONTENT,  _func)
#define SIMPLE_SVG_CREATE(_tag, _func)            \
  { &nsGkAtoms::_tag, SIMPLE_SVG_FCDATA(_func) }


const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindSVGData(Element* aElement,
                                   nsIAtom* aTag,
                                   PRInt32 aNameSpaceID,
                                   nsIFrame* aParentFrame,
                                   nsStyleContext* aStyleContext)
{
  if (aNameSpaceID != kNameSpaceID_SVG) {
    return nsnull;
  }

  static const FrameConstructionData sSuppressData = SUPPRESS_FCDATA();
  static const FrameConstructionData sContainerData =
    SIMPLE_SVG_FCDATA(NS_NewSVGContainerFrame);

  PRBool parentIsSVG = PR_FALSE;
  nsIContent* parentContent =
    aParentFrame ? aParentFrame->GetContent() : nsnull;
  
  
  
  if (parentContent) {
    PRInt32 parentNSID;
    nsIAtom* parentTag =
      parentContent->GetOwnerDoc()->BindingManager()->
        ResolveTag(aParentFrame->GetContent(), &parentNSID);

    
    
    
    
    parentIsSVG = parentNSID == kNameSpaceID_SVG &&
                  parentTag != nsGkAtoms::foreignObject;
  }

  if ((aTag != nsGkAtoms::svg && !parentIsSVG) ||
      (aTag == nsGkAtoms::desc || aTag == nsGkAtoms::title ||
       aTag == nsGkAtoms::feFuncR || aTag == nsGkAtoms::feFuncG ||
       aTag == nsGkAtoms::feFuncB || aTag == nsGkAtoms::feFuncA ||
       aTag == nsGkAtoms::feDistantLight || aTag == nsGkAtoms::fePointLight ||
       aTag == nsGkAtoms::feSpotLight)) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    return &sSuppressData;
  }

  
  if (aElement->IsNodeOfType(nsINode::eANIMATION)) {
    return &sSuppressData;
  }

  if (aTag == nsGkAtoms::svg && !parentIsSVG) {
    
    
    
    
    
    
    static const FrameConstructionData sOuterSVGData =
      FCDATA_DECL(FCDATA_SKIP_ABSPOS_PUSH | FCDATA_DISALLOW_GENERATED_CONTENT,
                  NS_NewSVGOuterSVGFrame);
    return &sOuterSVGData;
  }
  
  if (!nsSVGFeatures::PassesConditionalProcessingTests(aElement)) {
    
    
    
    return &sContainerData;
  }

  
  
  
  nsIFrame *ancestorFrame =
    nsSVGUtils::GetFirstNonAAncestorFrame(aParentFrame);
  if (ancestorFrame) {
    if (aTag == nsGkAtoms::tspan || aTag == nsGkAtoms::altGlyph) {
      
      nsSVGTextContainerFrame* metrics = do_QueryFrame(ancestorFrame);
      if (!metrics) {
        return &sSuppressData;
      }
    } else if (aTag == nsGkAtoms::textPath) {
      
      nsIAtom* ancestorFrameType = ancestorFrame->GetType();
      if (ancestorFrameType != nsGkAtoms::svgTextFrame) {
        return &sSuppressData;
      }
    } else if (aTag != nsGkAtoms::a) {
      
      
      nsSVGTextContainerFrame* metrics = do_QueryFrame(ancestorFrame);
      if (metrics) {
        return &sSuppressData;
      }
    }
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
    SIMPLE_SVG_CREATE(generic, NS_NewSVGGenericContainerFrame),
    { &nsGkAtoms::foreignObject,
      FULL_CTOR_FCDATA(FCDATA_DISALLOW_OUT_OF_FLOW,
                       &nsCSSFrameConstructor::ConstructSVGForeignObjectFrame) },
    SIMPLE_SVG_CREATE(a, NS_NewSVGAFrame),
    SIMPLE_SVG_CREATE(altGlyph, NS_NewSVGTSpanFrame),
    SIMPLE_SVG_CREATE(text, NS_NewSVGTextFrame),
    SIMPLE_SVG_CREATE(tspan, NS_NewSVGTSpanFrame),
    SIMPLE_SVG_CREATE(linearGradient, NS_NewSVGLinearGradientFrame),
    SIMPLE_SVG_CREATE(radialGradient, NS_NewSVGRadialGradientFrame),
    SIMPLE_SVG_CREATE(stop, NS_NewSVGStopFrame),
    SIMPLE_SVG_CREATE(use, NS_NewSVGUseFrame),
    SIMPLE_SVG_CREATE(marker, NS_NewSVGMarkerFrame),
    SIMPLE_SVG_CREATE(image, NS_NewSVGImageFrame),
    SIMPLE_SVG_CREATE(clipPath, NS_NewSVGClipPathFrame),
    SIMPLE_SVG_CREATE(textPath, NS_NewSVGTextPathFrame),
    SIMPLE_SVG_CREATE(filter, NS_NewSVGFilterFrame),
    SIMPLE_SVG_CREATE(pattern, NS_NewSVGPatternFrame),
    SIMPLE_SVG_CREATE(mask, NS_NewSVGMaskFrame),
    SIMPLE_SVG_CREATE(feBlend, NS_NewSVGLeafFrame),
    SIMPLE_SVG_CREATE(feColorMatrix, NS_NewSVGLeafFrame),
    SIMPLE_SVG_CREATE(feComposite, NS_NewSVGLeafFrame),
    SIMPLE_SVG_CREATE(feComponentTransfer, NS_NewSVGLeafFrame),
    SIMPLE_SVG_CREATE(feConvolveMatrix, NS_NewSVGLeafFrame),
    SIMPLE_SVG_CREATE(feDiffuseLighting, NS_NewSVGLeafFrame),
    SIMPLE_SVG_CREATE(feDisplacementMap, NS_NewSVGLeafFrame),
    SIMPLE_SVG_CREATE(feFlood, NS_NewSVGLeafFrame),
    SIMPLE_SVG_CREATE(feGaussianBlur, NS_NewSVGLeafFrame),
    SIMPLE_SVG_CREATE(feImage, NS_NewSVGLeafFrame),
    SIMPLE_SVG_CREATE(feMergeNode, NS_NewSVGLeafFrame),
    SIMPLE_SVG_CREATE(feMorphology, NS_NewSVGLeafFrame), 
    SIMPLE_SVG_CREATE(feOffset, NS_NewSVGLeafFrame), 
    SIMPLE_SVG_CREATE(feSpecularLighting, NS_NewSVGLeafFrame),
    SIMPLE_SVG_CREATE(feTile, NS_NewSVGLeafFrame), 
    SIMPLE_SVG_CREATE(feTurbulence, NS_NewSVGLeafFrame) 
  };

  const FrameConstructionData* data =
    FindDataByTag(aTag, aElement, aStyleContext, sSVGData,
                  NS_ARRAY_LENGTH(sSVGData));

  if (!data) {
    data = &sContainerData;
  }

  return data;
}

nsresult
nsCSSFrameConstructor::ConstructSVGForeignObjectFrame(nsFrameConstructorState& aState,
                                                      FrameConstructionItem&   aItem,
                                                      nsIFrame* aParentFrame,
                                                      const nsStyleDisplay* aStyleDisplay,
                                                      nsFrameItems& aFrameItems,
                                                      nsIFrame** aNewFrame)
{
  nsIContent* const content = aItem.mContent;
  nsStyleContext* const styleContext = aItem.mStyleContext;

  nsIFrame* newFrame = NS_NewSVGForeignObjectFrame(mPresShell, styleContext);
  if (NS_UNLIKELY(!newFrame)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  InitAndRestoreFrame(aState, content, aParentFrame, nsnull, newFrame);

  nsresult rv = aState.AddChild(newFrame, aFrameItems, content, styleContext,
                                aParentFrame, PR_FALSE, PR_FALSE);
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsRefPtr<nsStyleContext> innerPseudoStyle;
  innerPseudoStyle = mPresShell->StyleSet()->
    ResolveAnonymousBoxStyle(nsCSSAnonBoxes::mozSVGForeignContent, styleContext);

  nsIFrame* blockFrame = NS_NewBlockFrame(mPresShell, innerPseudoStyle,
                                          NS_BLOCK_FLOAT_MGR |
                                          NS_BLOCK_MARGIN_ROOT);
  if (NS_UNLIKELY(!blockFrame)) {
    newFrame->Destroy();
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsFrameItems childItems;
  
  
  rv = ConstructBlock(aState, innerPseudoStyle->GetStyleDisplay(), content,
                      newFrame, newFrame, innerPseudoStyle,
                      &blockFrame, childItems, PR_TRUE,
                      aItem.mPendingBinding);

  newFrame->SetInitialChildList(nsnull, childItems);

  *aNewFrame = newFrame;

  return rv;
}

void
nsCSSFrameConstructor::AddPageBreakItem(nsIContent* aContent,
                                        nsStyleContext* aMainStyleContext,
                                        FrameConstructionItemList& aItems)
{
  nsRefPtr<nsStyleContext> pseudoStyle;
  
  
  
  
  pseudoStyle =
    mPresShell->StyleSet()->
      ResolveAnonymousBoxStyle(nsCSSAnonBoxes::pageBreak,
                               aMainStyleContext->GetParent());

  NS_ASSERTION(pseudoStyle->GetStyleDisplay()->mDisplay ==
                 NS_STYLE_DISPLAY_BLOCK, "Unexpected display");

  static const FrameConstructionData sPageBreakData =
    FCDATA_DECL(FCDATA_SKIP_FRAMESET, NS_NewPageBreakFrame);

  
  
  aItems.AppendItem(&sPageBreakData, aContent, nsCSSAnonBoxes::pageBreak,
                    kNameSpaceID_None, nsnull, pseudoStyle.forget(), PR_TRUE);
}

nsresult
nsCSSFrameConstructor::ConstructFrame(nsFrameConstructorState& aState,
                                      nsIContent*              aContent,
                                      nsIFrame*                aParentFrame,
                                      nsFrameItems&            aFrameItems)

{
  NS_PRECONDITION(nsnull != aParentFrame, "no parent frame");
  FrameConstructionItemList items;
  AddFrameConstructionItems(aState, aContent, PR_TRUE, aParentFrame, items);

  for (FCItemIterator iter(items); !iter.IsDone(); iter.Next()) {
    NS_ASSERTION(iter.item().DesiredParentType() == GetParentType(aParentFrame),
                 "This is not going to work");
    nsresult rv =
      ConstructFramesFromItem(aState, iter, aParentFrame, aFrameItems);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

void
nsCSSFrameConstructor::AddFrameConstructionItems(nsFrameConstructorState& aState,
                                                 nsIContent* aContent,
                                                 PRBool aSuppressWhiteSpaceOptimizations,
                                                 nsIFrame* aParentFrame,
                                                 FrameConstructionItemList& aItems)
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
    return;
  }

  
  if (!NeedFrameFor(aState, aParentFrame, aContent)) {
    return;
  }

  
  if (aContent->IsNodeOfType(nsINode::eCOMMENT) ||
      aContent->IsNodeOfType(nsINode::ePROCESSING_INSTRUCTION))
    return;

  nsRefPtr<nsStyleContext> styleContext;
  styleContext = ResolveStyleContext(aParentFrame, aContent, &aState);

  AddFrameConstructionItemsInternal(aState, aContent, aParentFrame,
                                    aContent->Tag(), aContent->GetNameSpaceID(),
                                    aSuppressWhiteSpaceOptimizations,
                                    styleContext,
                                    ITEM_ALLOW_XBL_BASE | ITEM_ALLOW_PAGE_BREAK,
                                    aItems);
}







static void
SetAsUndisplayedContent(nsFrameManager* aFrameManager, nsIContent* aContent,
                        nsStyleContext* aStyleContext,
                        PRBool aIsGeneratedContent)
{
  if (aStyleContext->GetPseudo()) {
    if (aIsGeneratedContent) {
      aContent->UnbindFromTree();
    }
    return;
  }

  NS_ASSERTION(!aIsGeneratedContent, "Should have had pseudo type");
  aFrameManager->SetUndisplayedContent(aContent, aStyleContext);
}

void
nsCSSFrameConstructor::AddFrameConstructionItemsInternal(nsFrameConstructorState& aState,
                                                         nsIContent* aContent,
                                                         nsIFrame* aParentFrame,
                                                         nsIAtom* aTag,
                                                         PRInt32 aNameSpaceID,
                                                         PRBool aSuppressWhiteSpaceOptimizations,
                                                         nsStyleContext* aStyleContext,
                                                         PRUint32 aFlags,
                                                         FrameConstructionItemList& aItems)
{
  NS_PRECONDITION(aContent->IsNodeOfType(nsINode::eTEXT) ||
                  aContent->IsElement(),
                  "Shouldn't get anything else here!");

  
  
  
  const nsStyleDisplay* display = aStyleContext->GetStyleDisplay();
  nsRefPtr<nsStyleContext> styleContext(aStyleContext);
  PendingBinding* pendingBinding = nsnull;
  if ((aFlags & ITEM_ALLOW_XBL_BASE) && display->mBinding)
  {
    

    nsIXBLService * xblService = GetXBLService();
    if (!xblService)
      return;

    PRBool resolveStyle;

    nsAutoPtr<PendingBinding> newPendingBinding(new PendingBinding());
    if (!newPendingBinding) {
      return;
    }
    nsresult rv = xblService->LoadBindings(aContent, display->mBinding->GetURI(),
                                           display->mBinding->mOriginPrincipal,
                                           PR_FALSE,
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
      display = styleContext->GetStyleDisplay();
      aStyleContext = styleContext;
    }

    aTag = mDocument->BindingManager()->ResolveTag(aContent, &aNameSpaceID);
  }

  PRBool isGeneratedContent = ((aFlags & ITEM_IS_GENERATED_CONTENT) != 0);

  
  
  if (NS_STYLE_DISPLAY_NONE == display->mDisplay) {
    SetAsUndisplayedContent(aState.mFrameManager, aContent, styleContext,
                            isGeneratedContent);
    return;
  }

  PRBool isText = !aContent->IsElement();

  
  
  
  nsIContent *parent = aContent->GetParent();
  if (parent) {
    
    nsIAtom* parentTag = parent->Tag();
    if ((parentTag == nsGkAtoms::select || parentTag == nsGkAtoms::optgroup) &&
        parent->IsHTML() &&
        
        !aContent->IsHTML(nsGkAtoms::option) &&
        
        (!aContent->IsHTML(nsGkAtoms::optgroup) ||
         parentTag != nsGkAtoms::select) &&
        
        !aContent->IsRootOfNativeAnonymousSubtree()) {
      
      if (!isText) {
        SetAsUndisplayedContent(aState.mFrameManager, aContent, styleContext,
                                isGeneratedContent);
      }
      return;
    }
  }

  PRBool isPopup = PR_FALSE;
  
  const FrameConstructionData* data;
  if (isText) {
    data = FindTextData(aParentFrame);
    if (!data) {
      
      return;
    }
  } else {
    Element* element = aContent->AsElement();

    
    if (aNameSpaceID != kNameSpaceID_SVG &&
        aParentFrame &&
        aParentFrame->IsFrameOfType(nsIFrame::eSVG) &&
        !aParentFrame->IsFrameOfType(nsIFrame::eSVGForeignObject)
        ) {
      SetAsUndisplayedContent(aState.mFrameManager, element, styleContext,
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
                         styleContext);
    }

    
    if (!data) {
      data = FindXULDisplayData(display, element, styleContext);
    }

    
    if (!data) {
      data = FindDisplayData(display, element, styleContext);
    }

    NS_ASSERTION(data, "Should have frame construction data now");

    if (data->mBits & FCDATA_SUPPRESS_FRAME) {
      SetAsUndisplayedContent(aState.mFrameManager, element, styleContext,
                              isGeneratedContent);
      return;
    }

#ifdef MOZ_XUL
    if ((data->mBits & FCDATA_IS_POPUP) &&
        (!aParentFrame || 
         aParentFrame->GetType() != nsGkAtoms::menuFrame)) {
      if (!aState.mPopupItems.containingBlock &&
          !aState.mHavePendingPopupgroup) {
        SetAsUndisplayedContent(aState.mFrameManager, element, styleContext,
                                isGeneratedContent);
        return;
      }

      isPopup = PR_TRUE;
    }
#endif 
  }

  PRUint32 bits = data->mBits;

  
  if (aParentFrame &&
      aParentFrame->GetType() == nsGkAtoms::tableColGroupFrame &&
      (!(bits & FCDATA_IS_TABLE_PART) ||
       display->mDisplay != NS_STYLE_DISPLAY_TABLE_COLUMN)) {
    SetAsUndisplayedContent(aState.mFrameManager, aContent, styleContext,
                            isGeneratedContent);
    return;
  }

  PRBool canHavePageBreak =
    (aFlags & ITEM_ALLOW_PAGE_BREAK) &&
    aState.mPresContext->IsPaginated() &&
    !display->IsAbsolutelyPositioned() &&
    !(bits & FCDATA_IS_TABLE_PART);

  if (canHavePageBreak && display->mBreakBefore) {
    AddPageBreakItem(aContent, aStyleContext, aItems);
  }

  FrameConstructionItem* item =
    aItems.AppendItem(data, aContent, aTag, aNameSpaceID,
                      pendingBinding, styleContext.forget(),
                      aSuppressWhiteSpaceOptimizations);
  if (!item) {
    if (isGeneratedContent) {
      aContent->UnbindFromTree();
    }
    return;
  }

  item->mIsText = isText;
  item->mIsGeneratedContent = isGeneratedContent;
  if (isGeneratedContent) {
    NS_ADDREF(item->mContent);
  }
  item->mIsRootPopupgroup =
    aNameSpaceID == kNameSpaceID_XUL && aTag == nsGkAtoms::popupgroup &&
    aContent->IsRootOfNativeAnonymousSubtree();
  if (item->mIsRootPopupgroup) {
    aState.mHavePendingPopupgroup = PR_TRUE;
  }
  item->mIsPopup = isPopup;

  if (canHavePageBreak && display->mBreakAfter) {
    AddPageBreakItem(aContent, aStyleContext, aItems);
  }

  if (bits & FCDATA_IS_INLINE) {
    
    
    BuildInlineChildItems(aState, *item);
    item->mHasInlineEnds = PR_TRUE;
    item->mIsBlock = PR_FALSE;
  } else {
    
    
    PRBool isInline =
      
      
      
      ((bits & FCDATA_IS_TABLE_PART) &&
       (!aParentFrame || 
        aParentFrame->GetStyleDisplay()->mDisplay == NS_STYLE_DISPLAY_INLINE)) ||
      
      display->IsInlineOutside() ||
      
      isPopup;

    
    
    
    
    item->mIsAllInline = item->mHasInlineEnds = isInline ||
      
      
      
      
      
      
      
      
      
      
      
      
      
      (!(bits & FCDATA_DISALLOW_OUT_OF_FLOW) &&
       aState.GetGeometricParent(display, nsnull));

    
    
    
    
    
    item->mIsBlock =
      !isInline && !display->IsAbsolutelyPositioned() && !display->IsFloating();
  }

  if (item->mIsAllInline) {
    aItems.InlineItemAdded();
  } else if (item->mIsBlock) {
    aItems.BlockItemAdded();
  }

  
  
  
  
  if ((bits & FCDATA_IS_LINE_PARTICIPANT) &&
      ((bits & FCDATA_DISALLOW_OUT_OF_FLOW) ||
       !aState.GetGeometricParent(display, nsnull))) {
    item->mIsLineParticipant = PR_TRUE;
    aItems.LineParticipantItemAdded();
  }
}

static void
DestroyContent(void* aPropertyValue)
{
  nsIContent* content = static_cast<nsIContent*>(aPropertyValue);
  content->UnbindFromTree();
  NS_RELEASE(content);
}

NS_DECLARE_FRAME_PROPERTY(BeforeProperty, DestroyContent)
NS_DECLARE_FRAME_PROPERTY(AfterProperty, DestroyContent)

static const FramePropertyDescriptor*
GenConPseudoToProperty(nsIAtom* aPseudo)
{
  NS_ASSERTION(aPseudo == nsCSSPseudoElements::before ||
               aPseudo == nsCSSPseudoElements::after,
               "Bad gen-con pseudo");
  return aPseudo == nsCSSPseudoElements::before ? BeforeProperty()
      : AfterProperty();
}









PRBool
nsCSSFrameConstructor::AtLineBoundary(FCItemIterator& aIter)
{
  if (aIter.item().mSuppressWhiteSpaceOptimizations) {
    return PR_FALSE;
  }

  if (aIter.AtStart()) {
    if (aIter.List()->HasLineBoundaryAtStart() &&
        !aIter.item().mContent->GetPreviousSibling())
      return PR_TRUE;
  } else {
    FCItemIterator prev = aIter;
    prev.Prev();
    if (prev.item().IsLineBoundary() &&
        !prev.item().mSuppressWhiteSpaceOptimizations &&
        aIter.item().mContent->GetPreviousSibling() == prev.item().mContent)
      return PR_TRUE;
  }

  FCItemIterator next = aIter;
  next.Next();
  if (next.IsDone()) {
    if (aIter.List()->HasLineBoundaryAtEnd() &&
        !aIter.item().mContent->GetNextSibling())
      return PR_TRUE;
  } else {
    if (next.item().IsLineBoundary() &&
        !next.item().mSuppressWhiteSpaceOptimizations &&
        aIter.item().mContent->GetNextSibling() == next.item().mContent)
      return PR_TRUE;
  }

  return PR_FALSE;
}

nsresult
nsCSSFrameConstructor::ConstructFramesFromItem(nsFrameConstructorState& aState,
                                               FCItemIterator& aIter,
                                               nsIFrame* aParentFrame,
                                               nsFrameItems& aFrameItems)
{
  nsIFrame* adjParentFrame = aParentFrame;
  FrameConstructionItem& item = aIter.item();
  nsStyleContext* styleContext = item.mStyleContext;
  AdjustParentFrame(adjParentFrame, item.mFCData, styleContext);

  if (item.mIsText) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (AtLineBoundary(aIter) &&
        !styleContext->GetStyleText()->NewlineIsSignificant() &&
        aIter.List()->ParentHasNoXBLChildren() &&
        !(aState.mAdditionalStateBits & NS_FRAME_GENERATED_CONTENT) &&
        (item.mFCData->mBits & FCDATA_IS_LINE_PARTICIPANT) &&
        item.IsWhitespace(aState))
      return NS_OK;

    return ConstructTextFrame(item.mFCData, aState, item.mContent,
                              adjParentFrame, styleContext,
                              aFrameItems);
  }

  
  
  styleContext->StartBackgroundImageLoads();

  nsFrameState savedStateBits = aState.mAdditionalStateBits;
  if (item.mIsGeneratedContent) {
    
    
    aState.mAdditionalStateBits |= NS_FRAME_GENERATED_CONTENT;

    
    
    
    
    
    aParentFrame->Properties().Set(GenConPseudoToProperty(styleContext->GetPseudo()),
                                   item.mContent);

    
    
    item.mIsGeneratedContent = PR_FALSE;
  }

  
  nsresult rv = ConstructFrameFromItemInternal(item, aState, adjParentFrame,
                                               aFrameItems);

  aState.mAdditionalStateBits = savedStateBits;

  return rv;
}


inline PRBool
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
  return RecreateFramesForContent(rootElement, PR_FALSE);
}

nsIFrame*
nsCSSFrameConstructor::GetFrameFor(nsIContent* aContent)
{
  
  nsIFrame* frame = aContent->GetPrimaryFrame();

  if (!frame)
    return nsnull;

  
  
  
  if (frame->GetContent() != aContent) {
    return nsnull;
  }

  nsIFrame* insertionFrame = frame->GetContentInsertionFrame();

  NS_ASSERTION(insertionFrame == frame || !frame->IsLeaf(),
    "The insertion frame is the primary frame or the primary frame isn't a leaf");

  return insertionFrame;
}

nsIFrame*
nsCSSFrameConstructor::GetAbsoluteContainingBlock(nsIFrame* aFrame)
{
  NS_PRECONDITION(nsnull != mRootElementFrame, "no root element frame");
  
  
  
  nsIFrame* containingBlock = nsnull;
  for (nsIFrame* frame = aFrame; frame && !containingBlock;
       frame = frame->GetParent()) {
    if (frame->IsFrameOfType(nsIFrame::eMathML)) {
      
      
      
      return nsnull;
    }
    
    
    
    
    
    const nsStyleDisplay* disp = frame->GetStyleDisplay();

    if (disp->IsPositioned() && !IsTableRelated(frame->GetType())) {
      
      for (nsIFrame* wrappedFrame = aFrame; wrappedFrame != frame->GetParent();
           wrappedFrame = wrappedFrame->GetParent()) {
        nsIAtom* frameType = wrappedFrame->GetType();
        if (nsGkAtoms::blockFrame == frameType ||
#ifdef MOZ_XUL
            nsGkAtoms::XULLabelFrame == frameType ||
#endif
            (nsGkAtoms::inlineFrame == frameType && wrappedFrame->IsAbsoluteContainer())) {
          containingBlock = wrappedFrame;
        } else if (nsGkAtoms::fieldSetFrame == frameType) {
          
          
          containingBlock = GetFieldSetBlockFrame(wrappedFrame);
        }
      }

      
      
      
    }
  }

  
  
  if (containingBlock)
    return AdjustAbsoluteContainingBlock(containingBlock);

  
  return mHasRootAbsPosContainingBlock ? mDocElementContainingBlock : nsnull;
}

nsIFrame*
nsCSSFrameConstructor::GetFloatContainingBlock(nsIFrame* aFrame)
{
  
  
  
  
  for (nsIFrame* containingBlock = aFrame;
       containingBlock && !containingBlock->IsFrameOfType(nsIFrame::eMathML) &&
       !containingBlock->IsBoxFrame();
       containingBlock = containingBlock->GetParent()) {
    if (containingBlock->IsFloatContainingBlock()) {
      return containingBlock;
    }
  }

  
  
  return nsnull;
}







static nsIFrame*
AdjustAppendParentForAfterContent(nsPresContext* aPresContext,
                                  nsIContent* aContainer,
                                  nsIFrame* aParentFrame,
                                  nsIFrame** aAfterFrame)
{
  
  
  nsStyleContext* parentStyle = aParentFrame->GetStyleContext();
  if (nsLayoutUtils::HasPseudoStyle(aContainer, parentStyle,
                                    nsCSSPseudoElements::ePseudo_after,
                                    aPresContext)) {
    nsIFrame* afterFrame = nsLayoutUtils::GetAfterFrame(aParentFrame);
    if (afterFrame) {
      *aAfterFrame = afterFrame;
      return afterFrame->GetParent();
    }
  }

  *aAfterFrame = nsnull;

  if (IsFrameSpecial(aParentFrame)) {
    
    
    
    
    
    nsIFrame* trailingInline = GetSpecialSibling(aParentFrame);
    if (trailingInline) {
      aParentFrame = trailingInline;
    }

    
    
    
    
    
    
    aParentFrame = aParentFrame->GetLastContinuation();
  }

  return aParentFrame;
}






static nsIFrame*
FindAppendPrevSibling(nsIFrame* aParentFrame, nsIFrame* aAfterFrame)
{
  if (aAfterFrame) {
    NS_ASSERTION(aAfterFrame->GetParent() == aParentFrame, "Wrong parent");
    return aAfterFrame->GetPrevSibling();
  }

  return aParentFrame->GetLastChild(nsnull);
}





static nsIFrame*
GetInsertNextSibling(nsIFrame* aParentFrame, nsIFrame* aPrevSibling)
{
  if (aPrevSibling) {
    return aPrevSibling->GetNextSibling();
  }

  return aParentFrame->GetFirstChild(nsnull);
}






nsresult
nsCSSFrameConstructor::AppendFrames(nsFrameConstructorState&       aState,
                                    nsIFrame*                      aParentFrame,
                                    nsFrameItems&                  aFrameList,
                                    nsIFrame*                      aPrevSibling,
                                    PRBool                         aIsRecursiveCall)
{
  NS_PRECONDITION(!IsFrameSpecial(aParentFrame) ||
                  !GetSpecialSibling(aParentFrame) ||
                  !GetSpecialSibling(aParentFrame)->GetFirstChild(nsnull),
                  "aParentFrame has a special sibling with kids?");
  NS_PRECONDITION(!aPrevSibling || aPrevSibling->GetParent() == aParentFrame,
                  "Parent and prevsibling don't match");

  nsIFrame* nextSibling = ::GetInsertNextSibling(aParentFrame, aPrevSibling);

  NS_ASSERTION(nextSibling ||
               !aParentFrame->GetNextContinuation() ||
               !aParentFrame->GetNextContinuation()->GetFirstChild(nsnull) ||
               aIsRecursiveCall,
               "aParentFrame has later continuations with kids?");
  NS_ASSERTION(nextSibling ||
               !IsFrameSpecial(aParentFrame) ||
               (IsInlineFrame(aParentFrame) &&
                !GetSpecialSibling(aParentFrame) &&
                !aParentFrame->GetNextContinuation()) ||
               aIsRecursiveCall,
               "aParentFrame is not last?");

  
  
  
  if (!nextSibling && IsFrameSpecial(aParentFrame)) {
    
    
    
    
    
    
    
    if (aFrameList.NotEmpty() && !IsInlineOutside(aFrameList.FirstChild())) {
      
      nsIFrame* firstContinuation = aParentFrame->GetFirstContinuation();
      if (firstContinuation->GetChildList(nsnull).IsEmpty()) {
        
        
        nsFrameList::FrameLinkEnumerator firstNonBlockEnumerator =
          FindFirstNonBlock(aFrameList);
        nsFrameList blockKids = aFrameList.ExtractHead(firstNonBlockEnumerator);
        NS_ASSERTION(blockKids.NotEmpty(), "No blocks?");

        nsIFrame* prevBlock =
          GetSpecialPrevSibling(firstContinuation)->GetLastContinuation();
        NS_ASSERTION(prevBlock, "Should have previous block here");

        MoveChildrenTo(aState.mPresContext, aParentFrame, prevBlock, blockKids);
      }
    }

    
    nsFrameList::FrameLinkEnumerator firstBlockEnumerator(aFrameList);
    FindFirstBlock(firstBlockEnumerator);

    nsFrameList inlineKids = aFrameList.ExtractHead(firstBlockEnumerator);
    if (!inlineKids.IsEmpty()) {
      aState.mFrameManager->AppendFrames(aParentFrame, nsnull, inlineKids);
    }

    if (!aFrameList.IsEmpty()) {
      const nsStyleDisplay* parentDisplay = aParentFrame->GetStyleDisplay();
      PRBool positioned =
        parentDisplay->mPosition == NS_STYLE_POSITION_RELATIVE ||
        parentDisplay->HasTransform();
      nsFrameItems ibSiblings;
      CreateIBSiblings(aState, aParentFrame, positioned, aFrameList,
                       ibSiblings);

      
      
      
      mPresShell->FrameNeedsReflow(aParentFrame,
                                   nsIPresShell::eTreeChange,
                                   NS_FRAME_HAS_DIRTY_CHILDREN);

      
      return AppendFrames(aState, aParentFrame->GetParent(), ibSiblings,
                          aParentFrame, PR_TRUE);
    }

    return NS_OK;
  }
  
  
  return aState.mFrameManager->InsertFrames(aParentFrame, nsnull, aPrevSibling,
                                            aFrameList);
}

#define UNSET_DISPLAY 255






PRBool
nsCSSFrameConstructor::IsValidSibling(nsIFrame*              aSibling,
                                      nsIContent*            aContent,
                                      PRUint8&               aDisplay)
{
  nsIFrame* parentFrame = aSibling->GetParent();
  nsIAtom* parentType = nsnull;
  nsIAtom* grandparentType = nsnull;
  if (parentFrame) {
    parentType = parentFrame->GetType();
    nsIFrame* grandparentFrame = parentFrame->GetParent();
    if (grandparentFrame) {
      grandparentType = grandparentFrame->GetType();
    }
  }
    
  PRUint8 siblingDisplay = aSibling->GetStyleDisplay()->mDisplay;
  if ((NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP == siblingDisplay) ||
      (NS_STYLE_DISPLAY_TABLE_COLUMN       == siblingDisplay) ||
      (NS_STYLE_DISPLAY_TABLE_CAPTION      == siblingDisplay) ||
      (NS_STYLE_DISPLAY_TABLE_HEADER_GROUP == siblingDisplay) ||
      (NS_STYLE_DISPLAY_TABLE_ROW_GROUP    == siblingDisplay) ||
      (NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP == siblingDisplay) ||
      nsGkAtoms::menuFrame == parentType) {
    
    if (UNSET_DISPLAY == aDisplay) {
      nsRefPtr<nsStyleContext> styleContext;
      nsIFrame* styleParent;
      PRBool providerIsChild;
      if (NS_FAILED(aSibling->
                      GetParentStyleContextFrame(aSibling->PresContext(),
                                                 &styleParent,
                                                 &providerIsChild)) ||
          !styleParent) {
        NS_NOTREACHED("Shouldn't happen");
        return PR_FALSE;
      }
      
      
      styleContext = ResolveStyleContext(styleParent, aContent, nsnull);
      if (!styleContext) return PR_FALSE;
      const nsStyleDisplay* display = styleContext->GetStyleDisplay();
      aDisplay = display->mDisplay;
    }
    if (nsGkAtoms::menuFrame == parentType) {
      return
        (NS_STYLE_DISPLAY_POPUP == aDisplay) ==
        (NS_STYLE_DISPLAY_POPUP == siblingDisplay);
    }
    
    
    
    
    
    
    
    
    
    
    
    
    if ((siblingDisplay == NS_STYLE_DISPLAY_TABLE_CAPTION) !=
        (aDisplay == NS_STYLE_DISPLAY_TABLE_CAPTION)) {
      
      return PR_FALSE;
    }

    if ((siblingDisplay == NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP ||
         siblingDisplay == NS_STYLE_DISPLAY_TABLE_COLUMN) !=
        (aDisplay == NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP ||
         aDisplay == NS_STYLE_DISPLAY_TABLE_COLUMN)) {
      
      
      return PR_FALSE;
    }

    return PR_TRUE;
  }
  else if (nsGkAtoms::fieldSetFrame == parentType ||
           (nsGkAtoms::fieldSetFrame == grandparentType &&
            nsGkAtoms::blockFrame == parentType)) {
    
    nsIAtom* sibType = aSibling->GetType();
    nsCOMPtr<nsIDOMHTMLLegendElement> legendContent(do_QueryInterface(aContent));

    if ((legendContent  && (nsGkAtoms::legendFrame != sibType)) ||
        (!legendContent && (nsGkAtoms::legendFrame == sibType)))
      return PR_FALSE;
  }

  return PR_TRUE;
}

nsIFrame*
nsCSSFrameConstructor::FindFrameForContentSibling(nsIContent* aContent,
                                                  nsIContent* aTargetContent,
                                                  PRUint8& aTargetContentDisplay,
                                                  PRBool aPrevSibling)
{
  nsIFrame* sibling = aContent->GetPrimaryFrame();
  if (!sibling || sibling->GetContent() != aContent) {
    
    
    return nsnull;
  }

  
  
  if (sibling->GetStateBits() & NS_FRAME_OUT_OF_FLOW) {
    nsIFrame* placeholderFrame = mPresShell->FrameManager()->GetPlaceholderFrameFor(sibling);
    NS_ASSERTION(placeholderFrame, "no placeholder for out-of-flow frame");
    sibling = placeholderFrame;
  }

  
  NS_ASSERTION(!sibling->GetPrevContinuation(), "How did that happen?");

  if (aPrevSibling) {
    
    
    if (IsFrameSpecial(sibling)) {
      sibling = GetLastSpecialSibling(sibling, PR_TRUE);
    }

    
    
    sibling = sibling->GetTailContinuation();
  }

  if (aTargetContent &&
      !IsValidSibling(sibling, aTargetContent, aTargetContentDisplay)) {
    sibling = nsnull;
  }

  return sibling;
}

nsIFrame*
nsCSSFrameConstructor::FindPreviousSibling(const ChildIterator& aFirst,
                                           ChildIterator aIter,
                                           PRUint8& aTargetContentDisplay)
{
  nsIContent* child = *aIter;

  
  
  while (aIter != aFirst) {
    --aIter;
    nsIFrame* prevSibling =
      FindFrameForContentSibling(*aIter, child, aTargetContentDisplay, PR_TRUE);

    if (prevSibling) {
      
      return prevSibling;
    }
  }

  return nsnull;
}

nsIFrame*
nsCSSFrameConstructor::FindNextSibling(ChildIterator aIter,
                                       const ChildIterator& aLast,
                                       PRUint8& aTargetContentDisplay)
{
  if (aIter == aLast) {
    
    
    return nsnull;
  }

  nsIContent* child = *aIter;

  while (++aIter != aLast) {
    nsIFrame* nextSibling =
      FindFrameForContentSibling(*aIter, child, aTargetContentDisplay, PR_FALSE);

    if (nextSibling) {
      
      return nextSibling;
    }
  }

  return nsnull;
}


static nsIFrame*
GetAdjustedParentFrame(nsIFrame*       aParentFrame,
                       nsIAtom*        aParentFrameType,
                       nsIContent*     aChildContent)
{
  NS_PRECONDITION(nsGkAtoms::tableOuterFrame != aParentFrameType,
                  "Shouldn't be happening!");
  
  nsIFrame* newParent = nsnull;

  if (nsGkAtoms::fieldSetFrame == aParentFrameType) {
    
    
    nsCOMPtr<nsIDOMHTMLLegendElement> legendContent(do_QueryInterface(aChildContent));
    if (!legendContent) {
      newParent = GetFieldSetBlockFrame(aParentFrame);
    }
  }
  return (newParent) ? newParent : aParentFrame;
}

nsIFrame*
nsCSSFrameConstructor::GetInsertionPrevSibling(nsIFrame*& aParentFrame,
                                               nsIContent* aContainer,
                                               nsIContent* aChild,
                                               PRBool* aIsAppend,
                                               PRBool* aIsRangeInsertSafe,
                                               nsIContent* aStartSkipChild,
                                               nsIContent* aEndSkipChild)
{
  *aIsAppend = PR_FALSE;

  
  
  
  

  NS_PRECONDITION(aParentFrame, "Must have parent frame to start with");
  nsIContent* container = aParentFrame->GetContent();

  ChildIterator first, last;
  ChildIterator::Init(container, &first, &last);
  ChildIterator iter(first);
  PRBool xblCase = iter.XBLInvolved() || container != aContainer;
  if (xblCase || !aChild->IsRootOfAnonymousSubtree()) {
    
    
    
    if (aStartSkipChild) {
      iter.seek(aStartSkipChild);
    } else {
      iter.seek(aChild);
    }
  }
#ifdef DEBUG
  else {
    NS_WARNING("Someone passed native anonymous content directly into frame "
               "construction.  Stop doing that!");
  }
#endif

  PRUint8 childDisplay = UNSET_DISPLAY;
  nsIFrame* prevSibling = FindPreviousSibling(first, iter, childDisplay);

  
  
  
  if (prevSibling) {
    aParentFrame = prevSibling->GetParent()->GetContentInsertionFrame();
  }
  else {
    
    if (aEndSkipChild) {
      iter.seek(aEndSkipChild);
      iter--;
    }
    nsIFrame* nextSibling = FindNextSibling(iter, last, childDisplay);

    if (nextSibling) {
      aParentFrame = nextSibling->GetParent()->GetContentInsertionFrame();
    }
    else {
      
      *aIsAppend = PR_TRUE;
      if (IsFrameSpecial(aParentFrame)) {
        
        
        
        aParentFrame = GetLastSpecialSibling(aParentFrame, PR_FALSE);
      }
      
      
      aParentFrame = nsLayoutUtils::GetLastContinuationWithChild(aParentFrame);
      
      aParentFrame = ::GetAdjustedParentFrame(aParentFrame,
                                              aParentFrame->GetType(),
                                              aChild);
      nsIFrame* appendAfterFrame;
      aParentFrame =
        ::AdjustAppendParentForAfterContent(mPresShell->GetPresContext(),
                                            container, aParentFrame,
                                            &appendAfterFrame);
      prevSibling = ::FindAppendPrevSibling(aParentFrame, appendAfterFrame);
    }
  }

  *aIsRangeInsertSafe = (childDisplay == UNSET_DISPLAY);
  return prevSibling;
}

static PRBool
IsSpecialFramesetChild(nsIContent* aContent)
{
  
  return aContent->IsHTML() &&
    (aContent->Tag() == nsGkAtoms::frameset ||
     aContent->Tag() == nsGkAtoms::frame);
}

static void
InvalidateCanvasIfNeeded(nsIPresShell* presShell, nsIContent* node);

#ifdef MOZ_XUL

static
PRBool
IsXULListBox(nsIContent* aContainer)
{
  return (aContainer->IsXUL() && aContainer->Tag() == nsGkAtoms::listbox);
}

static
nsListBoxBodyFrame*
MaybeGetListBoxBodyFrame(nsIContent* aContainer, nsIContent* aChild)
{
  if (!aContainer)
    return nsnull;

  if (IsXULListBox(aContainer) &&
      aChild->IsXUL() && aChild->Tag() == nsGkAtoms::listitem) {
    nsCOMPtr<nsIDOMXULElement> xulElement = do_QueryInterface(aContainer);
    nsCOMPtr<nsIBoxObject> boxObject;
    xulElement->GetBoxObject(getter_AddRefs(boxObject));
    nsCOMPtr<nsPIListBoxObject> listBoxObject = do_QueryInterface(boxObject);
    if (listBoxObject) {
      return listBoxObject->GetListBoxBody(PR_FALSE);
    }
  }

  return nsnull;
}
#endif

void
nsCSSFrameConstructor::AddTextItemIfNeeded(nsFrameConstructorState& aState,
                                           nsIFrame* aParentFrame,
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
  AddFrameConstructionItems(aState, aPossibleTextContent, PR_FALSE,
                            aParentFrame, aItems);
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
  ContentInserted(aParentContent, aContent, nsnull, PR_FALSE);
}












static inline PRBool
IsActuallyEditable(nsIContent* aContainer, nsIContent* aChild)
{
  return (aChild->IsEditable() &&
          (aContainer->IsEditable() ||
           aChild->MayHaveContentEditableAttr()));
}



PRBool
nsCSSFrameConstructor::MaybeConstructLazily(Operation aOperation,
                                            nsIContent* aContainer,
                                            nsIContent* aChild)
{
  if (mPresShell->GetPresContext()->IsChrome() || !aContainer ||
      aContainer->IsInNativeAnonymousSubtree() || aContainer->IsXUL()) {
    return PR_FALSE;
  }

  if (aOperation == CONTENTINSERT) {
    if (aChild->IsRootOfAnonymousSubtree() ||
        aChild->IsXUL() || IsActuallyEditable(aContainer, aChild)) {
      return PR_FALSE;
    }
  } else { 
    NS_ASSERTION(aOperation == CONTENTAPPEND,
                 "operation should be either insert or append");
    for (nsIContent* child = aChild; child; child = child->GetNextSibling()) {
      NS_ASSERTION(!child->IsRootOfAnonymousSubtree(),
                   "Should be coming through the CONTENTAPPEND case");
      if (child->IsXUL() || IsActuallyEditable(aContainer, child)) {
        return PR_FALSE;
      }
    }
  }

  
  

  
  nsIContent* content = aContainer;
#ifdef DEBUG
  
  
  
  
  
  PRBool noPrimaryFrame = PR_FALSE;
  PRBool needsFrameBitSet = PR_FALSE;
#endif
  while (content &&
         !content->HasFlag(NODE_DESCENDANTS_NEED_FRAMES)) {
#ifdef DEBUG
    if (content->GetPrimaryFrame() && content->GetPrimaryFrame()->IsLeaf()) {
      noPrimaryFrame = needsFrameBitSet = PR_FALSE;
    }
    if (!noPrimaryFrame && !content->GetPrimaryFrame()) {
      noPrimaryFrame = PR_TRUE;
    }
    if (!needsFrameBitSet && content->HasFlag(NODE_NEEDS_FRAME)) {
      needsFrameBitSet = PR_TRUE;
    }
#endif
    content->SetFlags(NODE_DESCENDANTS_NEED_FRAMES);
    content = content->GetFlattenedTreeParent();
  }
#ifdef DEBUG
  if (content && content->GetPrimaryFrame() &&
      content->GetPrimaryFrame()->IsLeaf()) {
    noPrimaryFrame = needsFrameBitSet = PR_FALSE;
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

  PostRestyleEventInternal(PR_TRUE);
  return PR_TRUE;
}

void
nsCSSFrameConstructor::CreateNeededFrames(nsIContent* aContent)
{
  NS_ASSERTION(!aContent->HasFlag(NODE_NEEDS_FRAME),
    "shouldn't get here with a content node that has needs frame bit set");
  NS_ASSERTION(aContent->HasFlag(NODE_DESCENDANTS_NEED_FRAMES),
    "should only get here with a content node that has descendants needing frames");

  aContent->UnsetFlags(NODE_DESCENDANTS_NEED_FRAMES);

  
  
  

  
  
  
  

  
  
  PRUint32 childCount = aContent->GetChildCount();
  PRBool inRun = PR_FALSE;
  nsIContent* firstChildInRun = nsnull;
  for (PRUint32 i = 0; i < childCount; i++) {
    nsIContent* child = aContent->GetChildAt(i);
    if (child->HasFlag(NODE_NEEDS_FRAME)) {
      NS_ASSERTION(!child->GetPrimaryFrame() ||
                   child->GetPrimaryFrame()->GetContent() != child,
                   
                   
                   
                   "NEEDS_FRAME set on a node that already has a frame?");
      if (!inRun) {
        inRun = PR_TRUE;
        firstChildInRun = child;
      }
    } else {
      if (inRun) {
        inRun = PR_FALSE;
        
        ContentRangeInserted(aContent, firstChildInRun, child, nsnull,
                             PR_FALSE);
      }
    }
  }
  if (inRun) {
    ContentAppended(aContent, firstChildInRun, PR_FALSE);
  }

  
  ChildIterator iter, last;
  for (ChildIterator::Init(aContent, &iter, &last);
       iter != last;
       ++iter) {
    nsIContent* child = *iter;
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
                                                    PRBool aAllowLazyConstruction)
{
  for (nsIContent* child = aStartChild;
       child != aEndChild;
       child = child->GetNextSibling()) {
    if ((child->GetPrimaryFrame() ||
         mPresShell->FrameManager()->GetUndisplayedContent(child))
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

nsIFrame*
nsCSSFrameConstructor::GetRangeInsertionPoint(nsIContent* aContainer,
                                              nsIFrame* aParentFrame,
                                              nsIContent* aStartChild,
                                              nsIContent* aEndChild,
                                              PRBool aAllowLazyConstruction)
{
  
  
  
  nsIFrame* insertionPoint;
  PRBool multiple = PR_FALSE;
  GetInsertionPoint(aParentFrame, nsnull, &insertionPoint, &multiple);
  if (! insertionPoint)
    return nsnull; 
 
  PRBool hasInsertion = PR_FALSE;
  if (!multiple) {
    
    nsIDocument* document = aStartChild->GetDocument();
    
    if (document &&
        document->BindingManager()->GetInsertionParent(aStartChild)) {
      hasInsertion = PR_TRUE;
    }
  }

  if (multiple || hasInsertion) {
    
    
    PRUint32 childCount = 0;

    if (!multiple) {
      
      
      
      
      
      
      
      
      
      
      childCount = insertionPoint->GetContent()->GetChildCount();
    }
 
    
    
    
    if (multiple || aEndChild != nsnull || childCount > 0) {
      
      
      
      IssueSingleInsertNofications(aContainer, aStartChild, aEndChild,
                                   aAllowLazyConstruction);
      return nsnull;
    }
  }

  return insertionPoint;
}

PRBool
nsCSSFrameConstructor::MaybeRecreateForFrameset(nsIFrame* aParentFrame,
                                                nsIContent* aStartChild,
                                                nsIContent* aEndChild)
{
  if (aParentFrame->GetType() == nsGkAtoms::frameSetFrame) {
    
    for (nsIContent* cur = aStartChild;
         cur != aEndChild;
         cur = cur->GetNextSibling()) {
      if (IsSpecialFramesetChild(cur)) {
        
        RecreateFramesForContent(aParentFrame->GetContent(), PR_FALSE);
        return PR_TRUE;
      }
    }
  }
  return PR_FALSE;
}

nsresult
nsCSSFrameConstructor::ContentAppended(nsIContent*     aContainer,
                                       nsIContent*     aFirstNewContent,
                                       PRBool          aAllowLazyConstruction)
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
    PRInt32 namespaceID;
    nsIAtom* tag =
      mDocument->BindingManager()->ResolveTag(aContainer, &namespaceID);

    
    if (tag == nsGkAtoms::treechildren ||
        tag == nsGkAtoms::treeitem ||
        tag == nsGkAtoms::treerow)
      return NS_OK;

  }
#endif 

  
  nsIFrame* parentFrame = GetFrameFor(aContainer);
  if (! parentFrame)
    return NS_OK;

  if (aAllowLazyConstruction &&
      MaybeConstructLazily(CONTENTAPPEND, aContainer, aFirstNewContent)) {
    return NS_OK;
  }

  LAYOUT_PHASE_TEMP_EXIT();
  parentFrame = GetRangeInsertionPoint(aContainer, parentFrame,
                                       aFirstNewContent, nsnull,
                                       aAllowLazyConstruction);
  LAYOUT_PHASE_TEMP_REENTER();
  if (!parentFrame) {
    return NS_OK;
  }

  LAYOUT_PHASE_TEMP_EXIT();
  if (MaybeRecreateForFrameset(parentFrame, aFirstNewContent, nsnull)) {
    LAYOUT_PHASE_TEMP_REENTER();
    return NS_OK;
  }
  LAYOUT_PHASE_TEMP_REENTER();  

  if (parentFrame->IsLeaf()) {
    
    
    ClearLazyBits(aFirstNewContent, nsnull);
    return NS_OK;
  }
  
  if (parentFrame->IsFrameOfType(nsIFrame::eMathML)) {
    LAYOUT_PHASE_TEMP_EXIT();
    nsresult rv = RecreateFramesForContent(parentFrame->GetContent(), PR_FALSE);
    LAYOUT_PHASE_TEMP_REENTER();
    return rv;
  }

  
  
  
  PRBool parentSpecial = IsFrameSpecial(parentFrame);
  if (parentSpecial) {
#ifdef DEBUG
    if (gNoisyContentUpdates) {
      printf("nsCSSFrameConstructor::ContentAppended: parentFrame=");
      nsFrame::ListTag(stdout, parentFrame);
      printf(" is special\n");
    }
#endif

    
    
    
    parentFrame = GetLastSpecialSibling(parentFrame, PR_FALSE);
  }

  
  
  parentFrame = nsLayoutUtils::GetLastContinuationWithChild(parentFrame);

  
  
  NS_ASSERTION(parentFrame->GetType() != nsGkAtoms::fieldSetFrame,
               "Unexpected parent");

  
  nsIFrame* parentAfterFrame;
  parentFrame =
    ::AdjustAppendParentForAfterContent(mPresShell->GetPresContext(),
                                        aContainer, parentFrame,
                                        &parentAfterFrame);
  
  
  nsFrameConstructorState state(mPresShell, mFixedContainingBlock,
                                GetAbsoluteContainingBlock(parentFrame),
                                GetFloatContainingBlock(parentFrame));

  
  PRBool haveFirstLetterStyle = PR_FALSE, haveFirstLineStyle = PR_FALSE;
  nsIFrame* containingBlock = state.mFloatedItems.containingBlock;
  if (containingBlock) {
    haveFirstLetterStyle = HasFirstLetterStyle(containingBlock);
    haveFirstLineStyle =
      ShouldHaveFirstLineStyle(containingBlock->GetContent(),
                               containingBlock->GetStyleContext());
  }

  if (haveFirstLetterStyle) {
    
    RemoveLetterFrames(state.mPresContext, state.mPresShell,
                       state.mFrameManager, containingBlock);
  }

  nsIAtom* frameType = parentFrame->GetType();
  PRBool haveNoXBLChildren =
    mDocument->BindingManager()->GetXBLChildNodesFor(aContainer) == nsnull;
  FrameConstructionItemList items;
  if (aFirstNewContent->GetPreviousSibling() &&
      GetParentType(frameType) == eTypeBlock &&
      haveNoXBLChildren) {
    
    
    
    
    
    
    
    
    
    
    
    AddTextItemIfNeeded(state, parentFrame,
                        aFirstNewContent->GetPreviousSibling(), items);
  }
  for (nsIContent* child = aFirstNewContent;
       child;
       child = child->GetNextSibling()) {
    AddFrameConstructionItems(state, child, PR_FALSE, parentFrame, items);
  }

  nsIFrame* prevSibling = ::FindAppendPrevSibling(parentFrame, parentAfterFrame);

  
  
  
  
  LAYOUT_PHASE_TEMP_EXIT();
  if (WipeContainingBlock(state, containingBlock, parentFrame, items,
                          PR_TRUE, prevSibling)) {
    LAYOUT_PHASE_TEMP_REENTER();
    return NS_OK;
  }
  LAYOUT_PHASE_TEMP_REENTER();

  
  
  
  if (nsLayoutUtils::GetAsBlock(parentFrame) && !haveFirstLetterStyle &&
      !haveFirstLineStyle && !parentSpecial) {
    items.SetLineBoundaryAtStart(!prevSibling ||
        !prevSibling->GetStyleDisplay()->IsInlineOutside() ||
        prevSibling->GetType() == nsGkAtoms::brFrame);
    
    items.SetLineBoundaryAtEnd(!parentAfterFrame ||
        !parentAfterFrame->GetStyleDisplay()->IsInlineOutside());
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
    nsIFrame* outerTable = parentFrame->GetParent();
    if (outerTable) {
      state.mFrameManager->AppendFrames(outerTable, nsGkAtoms::captionList,
                                        captionItems);
    }
  }

  if (frameItems.NotEmpty()) { 
    AppendFrames(state, parentFrame, frameItems, prevSibling);
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
                                     aFirstNewContent, nsnull);
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
PRBool NotifyListBoxBody(nsPresContext*    aPresContext,
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
        return PR_TRUE;
      }
    } else {
      listBoxBodyFrame->OnContentInserted(aPresContext, aChild);
      return PR_TRUE;
    }
  }

  return PR_FALSE;
}
#endif 

nsresult
nsCSSFrameConstructor::ContentInserted(nsIContent*            aContainer,
                                       nsIContent*            aChild,
                                       nsILayoutHistoryState* aFrameState,
                                       PRBool                 aAllowLazyConstruction)
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
                                            PRBool                 aAllowLazyConstruction)
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

  nsresult rv = NS_OK;

  PRBool isSingleInsert = (aStartChild->GetNextSibling() == aEndChild);
  NS_ASSERTION(isSingleInsert || !aAllowLazyConstruction,
               "range insert shouldn't be lazy");
  NS_ASSERTION(isSingleInsert || aEndChild,
               "range should not include all nodes after aStartChild");

#ifdef MOZ_XUL
  if (aContainer && IsXULListBox(aContainer)) {
    if (isSingleInsert) {
      if (NotifyListBoxBody(mPresShell->GetPresContext(), aContainer,
                            
                            
                            aStartChild, nsnull, 
                            mDocument, nsnull, CONTENT_INSERTED)) {
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

    NS_PRECONDITION(nsnull == mRootElementFrame,
                    "root element frame already created");

    
    nsIFrame*               docElementFrame;
    rv = ConstructDocElementFrame(docElement, aFrameState, &docElementFrame);

    if (NS_SUCCEEDED(rv) && docElementFrame) {
      InvalidateCanvasIfNeeded(mPresShell, aStartChild);
#ifdef DEBUG
      if (gReallyNoisyContentUpdates) {
        printf("nsCSSFrameConstructor::ContentRangeInserted: resulting frame "
               "model:\n");
        mFixedContainingBlock->List(stdout, 0);
      }
#endif
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

  
  nsIFrame* parentFrame = GetFrameFor(aContainer);
  if (! parentFrame)
    return NS_OK;

  if (aAllowLazyConstruction &&
      MaybeConstructLazily(CONTENTINSERT, aContainer, aStartChild)) {
    return NS_OK;
  }

  if (isSingleInsert) {
    
    
    
    nsIFrame* insertionPoint;
    GetInsertionPoint(parentFrame, aStartChild, &insertionPoint);
    if (! insertionPoint)
      return NS_OK; 

    parentFrame = insertionPoint;
  } else {
    
    
    LAYOUT_PHASE_TEMP_EXIT();
    parentFrame = GetRangeInsertionPoint(aContainer, parentFrame,
                                         aStartChild, aEndChild,
                                         aAllowLazyConstruction);
    LAYOUT_PHASE_TEMP_REENTER();
    if (!parentFrame) {
      return NS_OK;
    }
  }

  PRBool isAppend, isRangeInsertSafe;
  nsIFrame* prevSibling =
    GetInsertionPrevSibling(parentFrame, aContainer, aStartChild,
                            &isAppend, &isRangeInsertSafe);

  
  if (!isSingleInsert && !isRangeInsertSafe) {
    
    LAYOUT_PHASE_TEMP_EXIT();
    IssueSingleInsertNofications(aContainer, aStartChild, aEndChild,
                                 aAllowLazyConstruction);
    LAYOUT_PHASE_TEMP_REENTER();
    return NS_OK;
  }

  nsIContent* container = parentFrame->GetContent();

  nsIAtom* frameType = parentFrame->GetType();
  LAYOUT_PHASE_TEMP_EXIT();
  if (MaybeRecreateForFrameset(parentFrame, aStartChild, aEndChild)) {
    LAYOUT_PHASE_TEMP_REENTER();
    return NS_OK;
  }
  LAYOUT_PHASE_TEMP_REENTER();

  
  
  NS_ASSERTION(isSingleInsert || frameType != nsGkAtoms::fieldSetFrame,
               "Unexpected parent");
  if (frameType == nsGkAtoms::fieldSetFrame &&
      aStartChild->Tag() == nsGkAtoms::legend) {
    
    
    
    
    
    
    LAYOUT_PHASE_TEMP_EXIT();
    nsresult rv = RecreateFramesForContent(parentFrame->GetContent(), PR_FALSE);
    LAYOUT_PHASE_TEMP_REENTER();
    return rv;
  }

  
  if (parentFrame->IsLeaf()) {
    
    ClearLazyBits(aStartChild, aEndChild);
    return NS_OK;
  }

  if (parentFrame->IsFrameOfType(nsIFrame::eMathML)) {
    LAYOUT_PHASE_TEMP_EXIT();
    nsresult rv = RecreateFramesForContent(parentFrame->GetContent(), PR_FALSE);
    LAYOUT_PHASE_TEMP_REENTER();
    return rv;
  }

  nsFrameConstructorState state(mPresShell, mFixedContainingBlock,
                                GetAbsoluteContainingBlock(parentFrame),
                                GetFloatContainingBlock(parentFrame),
                                aFrameState);


  
  
  
  
  
  nsIFrame* containingBlock = state.mFloatedItems.containingBlock;
  PRBool haveFirstLetterStyle = PR_FALSE;
  PRBool haveFirstLineStyle = PR_FALSE;

  
  
  
  
  const nsStyleDisplay* parentDisplay = parentFrame->GetStyleDisplay();

  
  
  
  if ((NS_STYLE_DISPLAY_BLOCK == parentDisplay->mDisplay) ||
      (NS_STYLE_DISPLAY_LIST_ITEM == parentDisplay->mDisplay) ||
      (NS_STYLE_DISPLAY_INLINE == parentDisplay->mDisplay) ||
      (NS_STYLE_DISPLAY_INLINE_BLOCK == parentDisplay->mDisplay)) {
    
    if (containingBlock) {
      haveFirstLetterStyle = HasFirstLetterStyle(containingBlock);
      haveFirstLineStyle =
        ShouldHaveFirstLineStyle(containingBlock->GetContent(),
                                 containingBlock->GetStyleContext());
    }

    if (haveFirstLetterStyle) {
      
      
      if (parentFrame->GetType() == nsGkAtoms::letterFrame) {
        
        
        if (parentFrame->GetStateBits() & NS_FRAME_OUT_OF_FLOW) {
          nsPlaceholderFrame* placeholderFrame =
            state.mFrameManager->GetPlaceholderFrameFor(parentFrame);
          NS_ASSERTION(placeholderFrame, "No placeholder for out-of-flow?");
          parentFrame = placeholderFrame->GetParent();
        } else {
          parentFrame = parentFrame->GetParent();
        }
      }

      
      RemoveLetterFrames(state.mPresContext, mPresShell,
                         state.mFrameManager,
                         state.mFloatedItems.containingBlock);

      
      
      
      prevSibling = GetInsertionPrevSibling(parentFrame, aContainer,
                                            aStartChild, &isAppend,
                                            &isRangeInsertSafe);

      
      if (!isSingleInsert && !isRangeInsertSafe) {
        
        RecoverLetterFrames(state.mFloatedItems.containingBlock);

        
        LAYOUT_PHASE_TEMP_EXIT();
        IssueSingleInsertNofications(aContainer, aStartChild, aEndChild,
                                     aAllowLazyConstruction);
        LAYOUT_PHASE_TEMP_REENTER();
        return NS_OK;
      }

      container = parentFrame->GetContent();
      frameType = parentFrame->GetType();
    }
  }

  if (!prevSibling) {
    
    
    nsIFrame* firstChild = parentFrame->GetFirstChild(nsnull);

    if (firstChild &&
        nsLayoutUtils::IsGeneratedContentFor(container, firstChild,
                                             nsCSSPseudoElements::before)) {
      
      prevSibling = firstChild->GetTailContinuation();
      parentFrame = prevSibling->GetParent()->GetContentInsertionFrame();
      
      
    }
  }

  FrameConstructionItemList items;
  ParentType parentType = GetParentType(frameType);
  PRBool haveNoXBLChildren =
    mDocument->BindingManager()->GetXBLChildNodesFor(aContainer) == nsnull;
  if (aStartChild->GetPreviousSibling() &&
      parentType == eTypeBlock && haveNoXBLChildren) {
    
    
    
    
    
    AddTextItemIfNeeded(state, parentFrame, aStartChild->GetPreviousSibling(),
                        items);
  }

  if (isSingleInsert) {
    AddFrameConstructionItems(state, aStartChild,
                              aStartChild->IsRootOfAnonymousSubtree(),
                              parentFrame, items);
  } else {
    for (nsIContent* child = aStartChild;
         child != aEndChild;
         child = child->GetNextSibling()){
      AddFrameConstructionItems(state, child, PR_FALSE, parentFrame, items);
    }
  }

  if (aEndChild && parentType == eTypeBlock && haveNoXBLChildren) {
    
    
    
    
    
    AddTextItemIfNeeded(state, parentFrame, aEndChild, items);
  }

  
  
  
  
  LAYOUT_PHASE_TEMP_EXIT();
  if (WipeContainingBlock(state, containingBlock, parentFrame, items,
                          isAppend, prevSibling)) {
    LAYOUT_PHASE_TEMP_REENTER();
    return NS_OK;
  }
  LAYOUT_PHASE_TEMP_REENTER();

  
  
  
  
  nsFrameItems frameItems, captionItems;
  ConstructFramesFromItemList(state, items, parentFrame, frameItems);

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
    NS_ASSERTION(!IsFrameSpecial(frame1) && !IsFrameSpecial(frame2),
                 "Neither should be special");
    NS_ASSERTION((frame1->GetType() == nsGkAtoms::tableFrame &&
                  frame2->GetType() == nsGkAtoms::tableOuterFrame) ||
                 (frame1->GetType() == nsGkAtoms::tableOuterFrame &&
                  frame2->GetType() == nsGkAtoms::tableFrame) ||
                 frame1->GetType() == nsGkAtoms::fieldSetFrame ||
                 (frame1->GetParent() &&
                  frame1->GetParent()->GetType() == nsGkAtoms::fieldSetFrame),
                 "Unexpected frame types");
#endif
    isAppend = PR_TRUE;
    nsIFrame* appendAfterFrame;
    parentFrame =
      ::AdjustAppendParentForAfterContent(mPresShell->GetPresContext(),
                                          container,
                                          frameItems.FirstChild()->GetParent(),
                                          &appendAfterFrame);
    prevSibling = ::FindAppendPrevSibling(parentFrame, appendAfterFrame);
  }

  if (haveFirstLineStyle && parentFrame == containingBlock) {
    
    
    if (isAppend) {
      
      AppendFirstLineFrames(state, containingBlock->GetContent(),
                            containingBlock, frameItems); 
    }
    else {
      
      
      
      
      InsertFirstLineFrames(state, container, containingBlock, &parentFrame,
                            prevSibling, frameItems);
    }
  }
      
  
  
  if (captionItems.NotEmpty()) {
    NS_ASSERTION(nsGkAtoms::tableFrame == frameType ||
                 nsGkAtoms::tableOuterFrame == frameType,
                 "parent for caption is not table?");
    
    
    
    nsIFrame* captionParent = parentFrame;
    PRBool captionIsAppend;
    nsIFrame* captionPrevSibling = nsnull;

    
    PRBool ignored;
    if (isSingleInsert) {
      captionPrevSibling =
        GetInsertionPrevSibling(captionParent, aContainer, aStartChild,
                                &captionIsAppend, &ignored);
    } else {
      nsIContent* firstCaption = captionItems.FirstChild()->GetContent();
      
      
      
      captionPrevSibling =
        GetInsertionPrevSibling(captionParent, aContainer, firstCaption,
                                &captionIsAppend, &ignored,
                                aStartChild, aEndChild);
    }

    nsIFrame* outerTable = nsnull;
    if (GetCaptionAdjustedParent(captionParent, captionItems.FirstChild(),
                                 &outerTable)) {
      
      
      
      NS_ASSERTION(nsGkAtoms::tableOuterFrame == outerTable->GetType(),
                   "Pseudo frame construction failure; "
                   "a caption can be only a child of an outer table frame");

      
      
      
      if (captionPrevSibling &&
          captionPrevSibling->GetParent() != outerTable) {
          captionPrevSibling = nsnull;
      }
      if (captionIsAppend) {
        state.mFrameManager->AppendFrames(outerTable, nsGkAtoms::captionList,
                                          captionItems);
      } else {
        state.mFrameManager->InsertFrames(outerTable, nsGkAtoms::captionList,
                                          captionPrevSibling, captionItems);
      }
    }
  }

  if (frameItems.NotEmpty()) {
    
    if (isAppend) {
      AppendFrames(state, parentFrame, frameItems, prevSibling);
    } else {
      state.mFrameManager->InsertFrames(parentFrame, nsnull, prevSibling,
                                        frameItems);
    }
  }

  if (haveFirstLetterStyle) {
    
    
    RecoverLetterFrames(state.mFloatedItems.containingBlock);
  }

#ifdef DEBUG
  if (gReallyNoisyContentUpdates && parentFrame) {
    printf("nsCSSFrameConstructor::ContentRangeInserted: resulting frame model:\n");
    parentFrame->List(stdout, 0);
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
nsCSSFrameConstructor::ContentRemoved(nsIContent* aContainer,
                                      nsIContent* aChild,
                                      nsIContent* aOldNextSibling,
                                      RemoveFlags aFlags,
                                      PRBool*     aDidReconstruct)
{
  AUTO_LAYOUT_PHASE_ENTRY_POINT(mPresShell->GetPresContext(), FrameC);
  NS_PRECONDITION(mUpdateCount != 0,
                  "Should be in an update while destroying frames");

  *aDidReconstruct = PR_FALSE;
  
  
  

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

  nsFrameManager *frameManager = mPresShell->FrameManager();
  nsPresContext *presContext = mPresShell->GetPresContext();
  nsresult                  rv = NS_OK;

  
  nsIFrame* childFrame = aChild->GetPrimaryFrame();

  if (!childFrame || childFrame->GetContent() != aChild) {
    
    
    frameManager->ClearUndisplayedContentIn(aChild, aContainer);
  }

#ifdef MOZ_XUL
  if (NotifyListBoxBody(presContext, aContainer, aChild, aOldNextSibling, 
                        mDocument, childFrame, CONTENT_REMOVED))
    return NS_OK;

#endif 

  
  
  
  
  
  
  
  
  
  
  PRBool isRoot = PR_FALSE;
  if (!aContainer) {
    nsIFrame* viewport = frameManager->GetRootFrame();
    if (viewport) {
      nsIFrame* firstChild = viewport->GetFirstChild(nsnull);
      if (firstChild && firstChild->GetContent() == aChild) {
        isRoot = PR_TRUE;
        childFrame = firstChild;
        NS_ASSERTION(!childFrame->GetNextSibling(), "How did that happen?");
      }
    }
  }

  if (childFrame) {
    InvalidateCanvasIfNeeded(mPresShell, aChild);
    
    
    LAYOUT_PHASE_TEMP_EXIT();
    if (MaybeRecreateContainerForFrameRemoval(childFrame, &rv)) {
      LAYOUT_PHASE_TEMP_REENTER();
      *aDidReconstruct = PR_TRUE;
      return rv;
    }
    LAYOUT_PHASE_TEMP_REENTER();

    
    nsIFrame* parentFrame = childFrame->GetParent();
    nsIAtom* parentType = parentFrame->GetType();

    if (parentType == nsGkAtoms::frameSetFrame &&
        IsSpecialFramesetChild(aChild)) {
      
      *aDidReconstruct = PR_TRUE;
      LAYOUT_PHASE_TEMP_EXIT();
      nsresult rv = RecreateFramesForContent(parentFrame->GetContent(), PR_FALSE);
      LAYOUT_PHASE_TEMP_REENTER();
      return rv;
    }

    
    
    
    nsIFrame* possibleMathMLAncestor = parentType == nsGkAtoms::blockFrame ? 
         parentFrame->GetParent() : parentFrame;
    if (possibleMathMLAncestor->IsFrameOfType(nsIFrame::eMathML)) {
      *aDidReconstruct = PR_TRUE;
      LAYOUT_PHASE_TEMP_EXIT();
      nsresult rv = RecreateFramesForContent(possibleMathMLAncestor->GetContent(), PR_FALSE);
      LAYOUT_PHASE_TEMP_REENTER();
      return rv;
    }

    
    
    
    nsIFrame* grandparentFrame = parentFrame->GetParent();
    if (grandparentFrame && grandparentFrame->IsBoxFrame() &&
        (grandparentFrame->GetStateBits() & NS_STATE_BOX_WRAPS_KIDS_IN_BLOCK) &&
        
        aChild == AnyKidsNeedBlockParent(parentFrame->GetFirstChild(nsnull)) &&
        !AnyKidsNeedBlockParent(childFrame->GetNextSibling())) {
      *aDidReconstruct = PR_TRUE;
      LAYOUT_PHASE_TEMP_EXIT();
      nsresult rv = RecreateFramesForContent(grandparentFrame->GetContent(), PR_TRUE);
      LAYOUT_PHASE_TEMP_REENTER();
      return rv;
    }

#ifdef ACCESSIBILITY
    nsAccessibilityService* accService = nsIPresShell::AccService();
    if (accService) {
      accService->ContentRemoved(mPresShell, aContainer, aChild);
    }
#endif

    
    
    nsIFrame* inflowChild = childFrame;
    if (childFrame->GetStateBits() & NS_FRAME_OUT_OF_FLOW) {
      inflowChild = frameManager->GetPlaceholderFrameFor(childFrame);
      NS_ASSERTION(inflowChild, "No placeholder for out-of-flow?");
    }
    nsIFrame* containingBlock =
      GetFloatContainingBlock(inflowChild->GetParent());
    PRBool haveFLS = containingBlock && HasFirstLetterStyle(containingBlock);
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

      
      
      
      RemoveLetterFrames(presContext, mPresShell, frameManager,
                         containingBlock);

      
      childFrame = aChild->GetPrimaryFrame();
      if (!childFrame || childFrame->GetContent() != aChild) {
        
        
        frameManager->ClearUndisplayedContentIn(aChild, aContainer);
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
      childFrame = frameManager->GetPlaceholderFrameFor(childFrame);
      NS_ASSERTION(childFrame, "Missing placeholder frame for out of flow.");
      parentFrame = childFrame->GetParent();
    }
    rv = frameManager->RemoveFrame(nsLayoutUtils::GetChildListNameFor(childFrame),
                                   childFrame);
    

    if (isRoot) {
      mRootElementFrame = nsnull;
      mRootElementStyleFrame = nsnull;
      mDocElementContainingBlock = nsnull;
      mPageSequenceFrame = nsnull;
      mGfxScrollFrame = nsnull;
      mHasRootAbsPosContainingBlock = PR_FALSE;
      mFixedContainingBlock = frameManager->GetRootFrame();
    }

    if (haveFLS && mRootElementFrame) {
      RecoverLetterFrames(containingBlock);
    }

    
    
    
    
    
    
    if (aContainer && !aChild->IsRootOfAnonymousSubtree() &&
        aFlags != REMOVE_FOR_RECONSTRUCTION &&
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

#ifdef DEBUG
  
  
static PRBool gInApplyRenderingChangeToTree = PR_FALSE;
#endif

static void
DoApplyRenderingChangeToTree(nsIFrame* aFrame,
                             nsIViewManager* aViewManager,
                             nsFrameManager* aFrameManager,
                             nsChangeHint aChange);





static void
UpdateViewsForTree(nsIFrame* aFrame, nsIViewManager* aViewManager,
                   nsFrameManager* aFrameManager,
                   nsChangeHint aChange)
{
  NS_PRECONDITION(gInApplyRenderingChangeToTree,
                  "should only be called within ApplyRenderingChangeToTree");

  nsIView* view = aFrame->GetView();
  if (view) {
    if (aChange & nsChangeHint_SyncFrameView) {
      nsContainerFrame::SyncFrameViewProperties(aFrame->PresContext(),
                                                aFrame, nsnull, view);
    }
  }

  
  PRInt32 listIndex = 0;
  nsIAtom* childList = nsnull;

  do {
    nsIFrame* child = aFrame->GetFirstChild(childList);
    while (child) {
      if (!(child->GetStateBits() & NS_FRAME_OUT_OF_FLOW)) {
        
        if (nsGkAtoms::placeholderFrame == child->GetType()) {
          
          nsIFrame* outOfFlowFrame =
            nsPlaceholderFrame::GetRealFrameForPlaceholder(child);
          do {
            DoApplyRenderingChangeToTree(outOfFlowFrame, aViewManager,
                                         aFrameManager, aChange);
          } while ((outOfFlowFrame = outOfFlowFrame->GetNextContinuation()));
        } else if (childList == nsGkAtoms::popupList) {
          DoApplyRenderingChangeToTree(child, aViewManager,
                                       aFrameManager, aChange);
        } else {  
          if ((child->GetStateBits() & NS_FRAME_HAS_CONTAINER_LAYER) &&
              (aChange & nsChangeHint_RepaintFrame)) {
            FrameLayerBuilder::InvalidateThebesLayerContents(child,
              child->GetVisualOverflowRectRelativeToSelf());
          }
          UpdateViewsForTree(child, aViewManager, aFrameManager, aChange);
        }
      }
      child = child->GetNextSibling();
    }
    childList = aFrame->GetAdditionalChildListName(listIndex++);
  } while (childList);
}

static void
DoApplyRenderingChangeToTree(nsIFrame* aFrame,
                             nsIViewManager* aViewManager,
                             nsFrameManager* aFrameManager,
                             nsChangeHint aChange)
{
  NS_PRECONDITION(gInApplyRenderingChangeToTree,
                  "should only be called within ApplyRenderingChangeToTree");

  for ( ; aFrame; aFrame = nsLayoutUtils::GetNextContinuationOrSpecialSibling(aFrame)) {
    
    
    
    
    UpdateViewsForTree(aFrame, aViewManager, aFrameManager, aChange);

    
    if (aChange & nsChangeHint_RepaintFrame) {
      if (aFrame->IsFrameOfType(nsIFrame::eSVG)) {
        if (!(aFrame->GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD)) {
          nsSVGOuterSVGFrame *outerSVGFrame = nsSVGUtils::GetOuterSVGFrame(aFrame);
          if (outerSVGFrame) {
            
            
            
            
            
            
            
            
            
            
            
            outerSVGFrame->UpdateAndInvalidateCoveredRegion(aFrame);
          }
        }
      } else {
        aFrame->InvalidateOverflowRect();
      }
    }
    if (aChange & nsChangeHint_UpdateOpacityLayer) {
      aFrame->MarkLayersActive();
      aFrame->InvalidateLayer(aFrame->GetVisualOverflowRectRelativeToSelf(),
                              nsDisplayItem::TYPE_OPACITY);
    }
    
    if (aChange & nsChangeHint_UpdateTransformLayer) {
      aFrame->MarkLayersActive();
      
      
      aFrame->InvalidateTransformLayer();
    }
  }
}

static void
ApplyRenderingChangeToTree(nsPresContext* aPresContext,
                           nsIFrame* aFrame,
                           nsChangeHint aChange)
{
  nsIPresShell *shell = aPresContext->PresShell();
  if (shell->IsPaintingSuppressed()) {
    
    aChange = NS_SubtractHint(aChange, nsChangeHint_RepaintFrame);
    if (!aChange) {
      return;
    }
  }

  
  
  nsStyleContext *bgSC;
  while (!nsCSSRendering::FindBackground(aPresContext, aFrame, &bgSC)) {
    aFrame = aFrame->GetParent();
    NS_ASSERTION(aFrame, "root frame must paint");
  }

  nsIViewManager* viewManager = shell->GetViewManager();

  
  

  

  nsIViewManager::UpdateViewBatch batch(viewManager);

#ifdef DEBUG
  gInApplyRenderingChangeToTree = PR_TRUE;
#endif
  DoApplyRenderingChangeToTree(aFrame, viewManager, shell->FrameManager(),
                               aChange);
#ifdef DEBUG
  gInApplyRenderingChangeToTree = PR_FALSE;
#endif
  
  batch.EndUpdateViewBatch(NS_VMREFRESH_NO_SYNC);
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

    
    if (node->Tag() != nsGkAtoms::body ||
        !node->IsHTML()) {
      return;
    }
  }

  
  
  
  

  
  

  nsIViewManager::UpdateViewBatch batch(presShell->GetViewManager());
  nsIFrame* rootFrame = presShell->GetRootFrame();
  rootFrame->InvalidateFrameSubtree();
  batch.EndUpdateViewBatch(NS_VMREFRESH_DEFERRED);
}

nsresult
nsCSSFrameConstructor::StyleChangeReflow(nsIFrame* aFrame,
                                         nsChangeHint aHint)
{
  
  
  if (aFrame->GetStateBits() & NS_FRAME_FIRST_REFLOW)
    return NS_OK;

#ifdef DEBUG
  if (gNoisyContentUpdates) {
    printf("nsCSSFrameConstructor::StyleChangeReflow: aFrame=");
    nsFrame::ListTag(stdout, aFrame);
    printf("\n");
  }
#endif

  nsIPresShell::IntrinsicDirty dirtyType;
  if (aHint & nsChangeHint_ClearDescendantIntrinsics) {
    NS_ASSERTION(aHint & nsChangeHint_ClearAncestorIntrinsics,
                 "Please read the comments in nsChangeHint.h");
    dirtyType = nsIPresShell::eStyleChange;
  } else if (aHint & nsChangeHint_ClearAncestorIntrinsics) {
    dirtyType = nsIPresShell::eTreeChange;
  } else {
    dirtyType = nsIPresShell::eResize;
  }

  nsFrameState dirtyBits;
  if (aHint & nsChangeHint_NeedDirtyReflow) {
    dirtyBits = NS_FRAME_IS_DIRTY;
  } else {
    dirtyBits = NS_FRAME_HAS_DIRTY_CHILDREN;
  }

  do {
    mPresShell->FrameNeedsReflow(aFrame, dirtyType, dirtyBits);
    aFrame = nsLayoutUtils::GetNextContinuationOrSpecialSibling(aFrame);
  } while (aFrame);

  return NS_OK;
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
    nsresult rv = RecreateFramesForContent(aContent, PR_FALSE);
    LAYOUT_PHASE_TEMP_REENTER();
    return rv;
  }

  
  nsIFrame* frame = aContent->GetPrimaryFrame();

  
  

  
  
  if (nsnull != frame) {
#if 0
    NS_FRAME_LOG(NS_FRAME_TRACE_CALLS,
       ("nsCSSFrameConstructor::CharacterDataChanged: content=%p[%s] subcontent=%p frame=%p",
        aContent, ContentTag(aContent, 0),
        aSubContent, frame));
#endif

    
    
    
    
    
    
    
    
    
    nsIFrame* block = GetFloatContainingBlock(frame);
    PRBool haveFirstLetterStyle = PR_FALSE;
    if (block) {
      
      haveFirstLetterStyle = HasFirstLetterStyle(block);
      if (haveFirstLetterStyle) {
        RemoveLetterFrames(mPresShell->GetPresContext(), mPresShell,
                           mPresShell->FrameManager(), block);
        
        
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

NS_DECLARE_FRAME_PROPERTY(ChangeListProperty, nsnull)

nsresult
nsCSSFrameConstructor::ProcessRestyledFrames(nsStyleChangeList& aChangeList)
{
  NS_ASSERTION(!nsContentUtils::IsSafeToRunScript(),
               "Someone forgot a script blocker");
  PRInt32 count = aChangeList.Count();
  if (!count)
    return NS_OK;

  
  
  BeginUpdate();

  nsPresContext* presContext = mPresShell->GetPresContext();
  FramePropertyTable* propTable = presContext->PropertyTable();

  
  
  
  PRInt32 index = count;

  while (0 <= --index) {
    const nsStyleChangeData* changeData;
    aChangeList.ChangeAt(index, &changeData);
    if (changeData->mFrame) {
      propTable->Set(changeData->mFrame, ChangeListProperty(),
                     NS_INT32_TO_PTR(1));
    }
  }

  index = count;
  PRBool didInvalidate = PR_FALSE;
  PRBool didReflow = PR_FALSE;

  while (0 <= --index) {
    nsIFrame* frame;
    nsIContent* content;
    nsChangeHint hint;
    aChangeList.ChangeAt(index, frame, content, hint);

    NS_ASSERTION(!(hint & nsChangeHint_ReflowFrame) ||
                 (hint & nsChangeHint_NeedReflow),
                 "Reflow hint bits set without actually asking for a reflow");

    if (frame && frame->GetContent() != content) {
      
      
      frame = nsnull;
      if (!(hint & nsChangeHint_ReconstructFrame)) {
        continue;
      }
    }

    
    if (frame) {
      if (!propTable->Get(frame, ChangeListProperty()))
        continue;
    }

    if (hint & nsChangeHint_ReconstructFrame) {
      RecreateFramesForContent(content, PR_FALSE);
    } else {
      NS_ASSERTION(frame, "This shouldn't happen");
      if (hint & nsChangeHint_UpdateEffects) {
        nsSVGEffects::UpdateEffects(frame);
      }
      if (hint & nsChangeHint_NeedReflow) {
        StyleChangeReflow(frame, hint);
        didReflow = PR_TRUE;
      }
      if (hint & (nsChangeHint_RepaintFrame | nsChangeHint_SyncFrameView |
                  nsChangeHint_UpdateOpacityLayer | nsChangeHint_UpdateTransformLayer)) {
        ApplyRenderingChangeToTree(presContext, frame, hint);
        didInvalidate = PR_TRUE;
      }
      if (hint & nsChangeHint_UpdateCursor) {
        mPresShell->SynthesizeMouseMove(PR_FALSE);
      }
    }
  }

  EndUpdate();

  if (didInvalidate && !didReflow) {
    
    
    
    nsIFrame* rootFrame = mPresShell->FrameManager()->GetRootFrame();
    nsRootPresContext* rootPC = presContext->GetRootPresContext();
    if (rootPC) {
      rootPC->RequestUpdatePluginGeometry(rootFrame);
    }
  }

  
  
  
  index = count;
  while (0 <= --index) {
    const nsStyleChangeData* changeData;
    aChangeList.ChangeAt(index, &changeData);
    if (changeData->mFrame) {
      propTable->Delete(changeData->mFrame, ChangeListProperty());
    }

#ifdef DEBUG
    
    if (changeData->mContent) {
      nsIFrame* frame = changeData->mContent->GetPrimaryFrame();
      if (frame) {
        mPresShell->FrameManager()->DebugVerifyStyleTree(frame);
      }
    } else {
      NS_WARNING("Unable to test style tree integrity -- no content node");
    }
#endif
  }

  aChangeList.Clear();
  return NS_OK;
}

void
nsCSSFrameConstructor::RestyleElement(Element        *aElement,
                                      nsIFrame       *aPrimaryFrame,
                                      nsChangeHint   aMinHint,
                                      RestyleTracker& aRestyleTracker,
                                      PRBool          aRestyleDescendants)
{
  NS_ASSERTION(aPrimaryFrame == aElement->GetPrimaryFrame(),
               "frame/content mismatch");
  if (aPrimaryFrame && aPrimaryFrame->GetContent() != aElement) {
    
    
    aPrimaryFrame = nsnull;
  }
  NS_ASSERTION(!aPrimaryFrame || aPrimaryFrame->GetContent() == aElement,
               "frame/content mismatch");

  if (aMinHint & nsChangeHint_ReconstructFrame) {
    RecreateFramesForContent(aElement, PR_FALSE);
  } else if (aPrimaryFrame) {
    nsStyleChangeList changeList;
    mPresShell->FrameManager()->
      ComputeStyleChangeFor(aPrimaryFrame, &changeList, aMinHint,
                            aRestyleTracker, aRestyleDescendants);
    ProcessRestyledFrames(changeList);
  } else {
    
    MaybeRecreateFramesForElement(aElement);
  }
}

nsresult
nsCSSFrameConstructor::ContentStateChanged(nsIContent* aContent,
                                           nsEventStates aStateMask)
{
  
  
  if (!aContent->IsElement()) {
    return NS_OK;
  }

  Element* aElement = aContent->AsElement();

  nsStyleSet *styleSet = mPresShell->StyleSet();
  nsPresContext *presContext = mPresShell->GetPresContext();
  NS_ASSERTION(styleSet, "couldn't get style set");

  nsChangeHint hint = NS_STYLE_HINT_NONE;
  
  
  
  
  
  
  nsIFrame* primaryFrame = aElement->GetPrimaryFrame();
  if (primaryFrame) {
    
    if (!primaryFrame->IsGeneratedContentFrame() &&
        aStateMask.HasAtLeastOneOfStates(NS_EVENT_STATE_BROKEN |
                                         NS_EVENT_STATE_USERDISABLED |
                                         NS_EVENT_STATE_SUPPRESSED |
                                         NS_EVENT_STATE_LOADING)) {
      hint = nsChangeHint_ReconstructFrame;
    } else {
      PRUint8 app = primaryFrame->GetStyleDisplay()->mAppearance;
      if (app) {
        nsITheme *theme = presContext->GetTheme();
        if (theme && theme->ThemeSupportsWidget(presContext,
                                                primaryFrame, app)) {
          PRBool repaint = PR_FALSE;
          theme->WidgetStateChanged(primaryFrame, app, nsnull, &repaint);
          if (repaint) {
            NS_UpdateHint(hint, nsChangeHint_RepaintFrame);
          }
        }
      }
    }

    primaryFrame->ContentStatesChanged(aStateMask);
  }

  nsRestyleHint rshint = 
    styleSet->HasStateDependentStyle(presContext, aElement, aStateMask);
      
  if (aStateMask.HasState(NS_EVENT_STATE_HOVER) && rshint != 0) {
    ++mHoverGeneration;
  }

  if (aStateMask.HasState(NS_EVENT_STATE_VISITED)) {
    
    
    
    NS_UpdateHint(hint, nsChangeHint_RepaintFrame);
  }

  PostRestyleEvent(aElement, rshint, hint);
  return NS_OK;
}

void
nsCSSFrameConstructor::AttributeWillChange(Element* aElement,
                                           PRInt32 aNameSpaceID,
                                           nsIAtom* aAttribute,
                                           PRInt32 aModType)
{
  nsRestyleHint rshint =
    mPresShell->StyleSet()->HasAttributeDependentStyle(mPresShell->GetPresContext(),
                                                       aElement,
                                                       aAttribute,
                                                       aModType,
                                                       PR_FALSE);
  PostRestyleEvent(aElement, rshint, NS_STYLE_HINT_NONE);
}

void
nsCSSFrameConstructor::AttributeChanged(Element* aElement,
                                        PRInt32 aNameSpaceID,
                                        nsIAtom* aAttribute,
                                        PRInt32 aModType)
{
  
  
  
  nsCOMPtr<nsIPresShell> shell = mPresShell;

  
  nsIFrame* primaryFrame = aElement->GetPrimaryFrame();

#if 0
  NS_FRAME_LOG(NS_FRAME_TRACE_CALLS,
     ("HTMLStyleSheet::AttributeChanged: content=%p[%s] frame=%p",
      aContent, ContentTag(aElement, 0), frame));
#endif

  
  nsChangeHint hint = aElement->GetAttributeChangeHint(aAttribute, aModType);

  PRBool reframe = (hint & nsChangeHint_ReconstructFrame) != 0;

#ifdef MOZ_XUL
  
  
  
  if (!primaryFrame && !reframe) {
    PRInt32 namespaceID;
    nsIAtom* tag =
      mDocument->BindingManager()->ResolveTag(aElement, &namespaceID);

    if (namespaceID == kNameSpaceID_XUL &&
        (tag == nsGkAtoms::listitem ||
         tag == nsGkAtoms::listcell))
      return;
  }

  if (aAttribute == nsGkAtoms::tooltiptext ||
      aAttribute == nsGkAtoms::tooltip) 
  {
    nsIRootBox* rootBox = nsIRootBox::GetRootBox(mPresShell);
    if (rootBox) {
      if (aModType == nsIDOMMutationEvent::REMOVAL)
        rootBox->RemoveTooltipSupport(aElement);
      if (aModType == nsIDOMMutationEvent::ADDITION)
        rootBox->AddTooltipSupport(aElement);
    }
  }

#endif 

  if (primaryFrame) {
    
    const nsStyleDisplay* disp = primaryFrame->GetStyleDisplay();
    if (disp->mAppearance) {
      nsPresContext* presContext = mPresShell->GetPresContext();
      nsITheme *theme = presContext->GetTheme();
      if (theme && theme->ThemeSupportsWidget(presContext, primaryFrame, disp->mAppearance)) {
        PRBool repaint = PR_FALSE;
        theme->WidgetStateChanged(primaryFrame, disp->mAppearance, aAttribute, &repaint);
        if (repaint)
          NS_UpdateHint(hint, nsChangeHint_RepaintFrame);
      }
    }
   
    
    primaryFrame->AttributeChanged(aNameSpaceID, aAttribute, aModType);
    
    
    
    
  }

  
  
  nsRestyleHint rshint =
    mPresShell->StyleSet()->HasAttributeDependentStyle(mPresShell->GetPresContext(),
                                                       aElement,
                                                       aAttribute,
                                                       aModType,
                                                       PR_TRUE);

  PostRestyleEvent(aElement, rshint, hint);
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
    mQuotesDirty = PR_FALSE;
    mQuoteList.RecalcAll();
  }

  if (mCountersDirty) {
    mCountersDirty = PR_FALSE;
    mCounterManager.RecalcAll();
  }

  NS_ASSERTION(!mQuotesDirty, "Quotes updates will be lost");
  NS_ASSERTION(!mCountersDirty, "Counter updates will be lost");  
}

void
nsCSSFrameConstructor::WillDestroyFrameTree()
{
#if defined(DEBUG_dbaron_off)
  mCounterManager.Dump();
#endif

  mIsDestroyingFrameTree = PR_TRUE;

  
  mQuoteList.Clear();
  mCounterManager.Clear();

  
  
  
  mPresShell->GetPresContext()->RefreshDriver()->
    RemoveStyleFlushObserver(mPresShell);
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

nsresult
nsCSSFrameConstructor::CreateContinuingOuterTableFrame(nsIPresShell*    aPresShell,
                                                       nsPresContext*  aPresContext,
                                                       nsIFrame*        aFrame,
                                                       nsIFrame*        aParentFrame,
                                                       nsIContent*      aContent,
                                                       nsStyleContext*  aStyleContext,
                                                       nsIFrame**       aContinuingFrame)
{
  nsIFrame* newFrame = NS_NewTableOuterFrame(aPresShell, aStyleContext);

  if (newFrame) {
    newFrame->Init(aContent, aParentFrame, aFrame);

    
    
    nsFrameItems  newChildFrames;

    nsIFrame* childFrame = aFrame->GetFirstChild(nsnull);
    if (childFrame) {
      nsIFrame* continuingTableFrame;
      nsresult rv = CreateContinuingFrame(aPresContext, childFrame, newFrame,
                                          &continuingTableFrame);
      if (NS_FAILED(rv)) {
        newFrame->Destroy();
        *aContinuingFrame = nsnull;
        return rv;
      }
      newChildFrames.AddChild(continuingTableFrame);
      
      NS_ASSERTION(!childFrame->GetNextSibling(),"there can be only one inner table frame");
    }

    
    newFrame->SetInitialChildList(nsnull, newChildFrames);
    
    *aContinuingFrame = newFrame;
    return NS_OK;
  }
  else {
    *aContinuingFrame = nsnull;
    return NS_ERROR_OUT_OF_MEMORY;
  }
}

nsresult
nsCSSFrameConstructor::CreateContinuingTableFrame(nsIPresShell* aPresShell, 
                                                  nsPresContext*  aPresContext,
                                                  nsIFrame*        aFrame,
                                                  nsIFrame*        aParentFrame,
                                                  nsIContent*      aContent,
                                                  nsStyleContext*  aStyleContext,
                                                  nsIFrame**       aContinuingFrame)
{
  nsIFrame* newFrame = NS_NewTableFrame(aPresShell, aStyleContext);

  if (newFrame) {
    newFrame->Init(aContent, aParentFrame, aFrame);

    
    nsFrameItems  childFrames;
    nsIFrame* childFrame = aFrame->GetFirstChild(nsnull);
    for ( ; childFrame; childFrame = childFrame->GetNextSibling()) {
      
      nsTableRowGroupFrame* rowGroupFrame =
        static_cast<nsTableRowGroupFrame*>(childFrame);
      
      nsIFrame* rgNextInFlow = rowGroupFrame->GetNextInFlow();
      if (rgNextInFlow) {
        rowGroupFrame->SetRepeatable(PR_FALSE);
      }
      else if (rowGroupFrame->IsRepeatable()) {
        
        nsTableRowGroupFrame*   headerFooterFrame;
        nsFrameItems            childItems;
        nsFrameConstructorState state(mPresShell, mFixedContainingBlock,
                                      GetAbsoluteContainingBlock(newFrame),
                                      nsnull);
        state.mCreatingExtraFrames = PR_TRUE;

        headerFooterFrame = static_cast<nsTableRowGroupFrame*>
                                       (NS_NewTableRowGroupFrame(aPresShell, rowGroupFrame->GetStyleContext()));
        nsIContent* headerFooter = rowGroupFrame->GetContent();
        headerFooterFrame->Init(headerFooter, newFrame, nsnull);
        ProcessChildren(state, headerFooter, rowGroupFrame->GetStyleContext(),
                        headerFooterFrame, PR_TRUE, childItems, PR_FALSE,
                        nsnull);
        NS_ASSERTION(state.mFloatedItems.IsEmpty(), "unexpected floated element");
        headerFooterFrame->SetInitialChildList(nsnull, childItems);
        headerFooterFrame->SetRepeatable(PR_TRUE);

        
        headerFooterFrame->InitRepeatedFrame(aPresContext, rowGroupFrame);

        
        childFrames.AddChild(headerFooterFrame);
      }
    }
    
    
    newFrame->SetInitialChildList(nsnull, childFrames);
    
    *aContinuingFrame = newFrame;
    return NS_OK;
  }
  else {
    *aContinuingFrame = nsnull;
    return NS_ERROR_OUT_OF_MEMORY;
  }
}

nsresult
nsCSSFrameConstructor::CreateContinuingFrame(nsPresContext* aPresContext,
                                             nsIFrame*       aFrame,
                                             nsIFrame*       aParentFrame,
                                             nsIFrame**      aContinuingFrame,
                                             PRBool          aIsFluid)
{
  nsIPresShell*              shell = aPresContext->PresShell();
  nsStyleContext*            styleContext = aFrame->GetStyleContext();
  nsIFrame*                  newFrame = nsnull;
  nsresult                   rv = NS_OK;
  nsIFrame*                  nextContinuation = aFrame->GetNextContinuation();
  nsIFrame*                  nextInFlow = aFrame->GetNextInFlow();

  
  nsIAtom* frameType = aFrame->GetType();
  nsIContent* content = aFrame->GetContent();

  NS_ASSERTION(aFrame->GetSplittableType() != NS_FRAME_NOT_SPLITTABLE,
               "why CreateContinuingFrame for a non-splittable frame?");
  
  if (nsGkAtoms::textFrame == frameType) {
    newFrame = NS_NewContinuingTextFrame(shell, styleContext);

    if (newFrame) {
      newFrame->Init(content, aParentFrame, aFrame);
    }
    
  } else if (nsGkAtoms::inlineFrame == frameType) {
    newFrame = NS_NewInlineFrame(shell, styleContext);

    if (newFrame) {
      newFrame->Init(content, aParentFrame, aFrame);
    }
  
  } else if (nsGkAtoms::blockFrame == frameType) {
    newFrame = NS_NewBlockFrame(shell, styleContext);

    if (newFrame) {
      newFrame->Init(content, aParentFrame, aFrame);
    }
  
#ifdef MOZ_XUL
  } else if (nsGkAtoms::XULLabelFrame == frameType) {
    newFrame = NS_NewXULLabelFrame(shell, styleContext);

    if (newFrame) {
      newFrame->Init(content, aParentFrame, aFrame);
    }
#endif  
  } else if (nsGkAtoms::columnSetFrame == frameType) {
    newFrame = NS_NewColumnSetFrame(shell, styleContext, 0);

    if (newFrame) {
      newFrame->Init(content, aParentFrame, aFrame);
    }

  } else if (nsGkAtoms::pageFrame == frameType) {
    nsIFrame* canvasFrame;
    rv = ConstructPageFrame(shell, aPresContext, aParentFrame, aFrame,
                            newFrame, canvasFrame);
  } else if (nsGkAtoms::tableOuterFrame == frameType) {
    rv = CreateContinuingOuterTableFrame(shell, aPresContext, aFrame, aParentFrame,
                                         content, styleContext, &newFrame);

  } else if (nsGkAtoms::tableFrame == frameType) {
    rv = CreateContinuingTableFrame(shell, aPresContext, aFrame, aParentFrame,
                                    content, styleContext, &newFrame);

  } else if (nsGkAtoms::tableRowGroupFrame == frameType) {
    newFrame = NS_NewTableRowGroupFrame(shell, styleContext);

    if (newFrame) {
      newFrame->Init(content, aParentFrame, aFrame);
    }

  } else if (nsGkAtoms::tableRowFrame == frameType) {
    newFrame = NS_NewTableRowFrame(shell, styleContext);

    if (newFrame) {
      newFrame->Init(content, aParentFrame, aFrame);

      
      nsFrameItems  newChildList;
      nsIFrame* cellFrame = aFrame->GetFirstChild(nsnull);
      while (cellFrame) {
        
        if (IS_TABLE_CELL(cellFrame->GetType())) {
          nsIFrame* continuingCellFrame;
          rv = CreateContinuingFrame(aPresContext, cellFrame, newFrame,
                                     &continuingCellFrame);
          if (NS_FAILED(rv)) {
            newChildList.DestroyFrames();
            newFrame->Destroy();
            *aContinuingFrame = nsnull;
            return NS_ERROR_OUT_OF_MEMORY;
          }
          newChildList.AddChild(continuingCellFrame);
        }
        cellFrame = cellFrame->GetNextSibling();
      }
      
      
      newFrame->SetInitialChildList(nsnull, newChildList);
    }

  } else if (IS_TABLE_CELL(frameType)) {
    
    
    
    newFrame = NS_NewTableCellFrame(shell, styleContext, IsBorderCollapse(aParentFrame));

    if (newFrame) {
      newFrame->Init(content, aParentFrame, aFrame);

      
      nsIFrame* continuingBlockFrame;
      nsIFrame* blockFrame = aFrame->GetFirstChild(nsnull);
      rv = CreateContinuingFrame(aPresContext, blockFrame, newFrame,
                                 &continuingBlockFrame);
      if (NS_FAILED(rv)) {
        newFrame->Destroy();
        *aContinuingFrame = nsnull;
        return rv;
      }

      
      SetInitialSingleChild(newFrame, continuingBlockFrame);
    }
  
  } else if (nsGkAtoms::lineFrame == frameType) {
    newFrame = NS_NewFirstLineFrame(shell, styleContext);

    if (newFrame) {
      newFrame->Init(content, aParentFrame, aFrame);
    }
  
  } else if (nsGkAtoms::letterFrame == frameType) {
    newFrame = NS_NewFirstLetterFrame(shell, styleContext);

    if (newFrame) {
      newFrame->Init(content, aParentFrame, aFrame);
    }

  } else if (nsGkAtoms::imageFrame == frameType) {
    newFrame = NS_NewImageFrame(shell, styleContext);

    if (newFrame) {
      newFrame->Init(content, aParentFrame, aFrame);
    }
  } else if (nsGkAtoms::imageControlFrame == frameType) {
    newFrame = NS_NewImageControlFrame(shell, styleContext);

    if (newFrame) {
      newFrame->Init(content, aParentFrame, aFrame);
    }    
  } else if (nsGkAtoms::placeholderFrame == frameType) {
    
    nsIFrame* oofFrame = nsPlaceholderFrame::GetRealFrameForPlaceholder(aFrame);
    nsIFrame* oofContFrame;
    rv = CreateContinuingFrame(aPresContext, oofFrame, aParentFrame, &oofContFrame);
    if (NS_FAILED(rv)) {
      *aContinuingFrame = nsnull;
      return rv;
    }
    
    rv = CreatePlaceholderFrameFor(shell, content, oofContFrame, styleContext,
                                   aParentFrame, aFrame,
                                   aFrame->GetStateBits() & PLACEHOLDER_TYPE_MASK,
                                   &newFrame);
    if (NS_FAILED(rv)) {
      oofContFrame->Destroy();
      *aContinuingFrame = nsnull;
      return rv;
    }
  } else if (nsGkAtoms::fieldSetFrame == frameType) {
    newFrame = NS_NewFieldSetFrame(shell, styleContext);

    if (newFrame) {
      newFrame->Init(content, aParentFrame, aFrame);

      
      
      nsIFrame* continuingBlockFrame;
      nsIFrame* blockFrame = GetFieldSetBlockFrame(aFrame);
      rv = CreateContinuingFrame(aPresContext, blockFrame, newFrame,
                                 &continuingBlockFrame);
      if (NS_FAILED(rv)) {
        newFrame->Destroy();
        *aContinuingFrame = nsnull;
        return rv;
      }
      
      SetInitialSingleChild(newFrame, continuingBlockFrame);
    }
  } else if (nsGkAtoms::legendFrame == frameType) {
    newFrame = NS_NewLegendFrame(shell, styleContext);

    if (newFrame) {
      newFrame->Init(content, aParentFrame, aFrame);
    }
  } else {
    NS_NOTREACHED("unexpected frame type");
    *aContinuingFrame = nsnull;
    return NS_ERROR_UNEXPECTED;
  }

  *aContinuingFrame = newFrame;

  if (!newFrame) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  
  
  if (!aIsFluid) {
    newFrame->SetPrevContinuation(aFrame);
  }

  
  if (aFrame->GetStateBits() & NS_FRAME_GENERATED_CONTENT) {
    newFrame->AddStateBits(NS_FRAME_GENERATED_CONTENT);
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
  return NS_OK;
}

nsresult
nsCSSFrameConstructor::ReplicateFixedFrames(nsPageContentFrame* aParentFrame)
{
  
  
  

  nsIFrame* prevPageContentFrame = aParentFrame->GetPrevInFlow();
  if (!prevPageContentFrame) {
    return NS_OK;
  }
  nsIFrame* canvasFrame = aParentFrame->GetFirstChild(nsnull);
  nsIFrame* prevCanvasFrame = prevPageContentFrame->GetFirstChild(nsnull);
  if (!canvasFrame || !prevCanvasFrame) {
    
    return NS_ERROR_UNEXPECTED;
  }

  nsFrameItems fixedPlaceholders;
  nsIFrame* firstFixed = prevPageContentFrame->GetFirstChild(nsGkAtoms::fixedList);
  if (!firstFixed) {
    return NS_OK;
  }

  
  
  
  
  nsFrameConstructorState state(mPresShell, aParentFrame,
                                nsnull,
                                mRootElementFrame);
  state.mCreatingExtraFrames = PR_TRUE;

  
  
  
  
  for (nsIFrame* fixed = firstFixed; fixed; fixed = fixed->GetNextSibling()) {
    nsIFrame* prevPlaceholder = mPresShell->FrameManager()->GetPlaceholderFrameFor(fixed);
    if (prevPlaceholder &&
        nsLayoutUtils::IsProperAncestorFrame(prevCanvasFrame, prevPlaceholder)) {
      
      
      nsIContent* content = fixed->GetContent();
      nsStyleContext* styleContext =
        nsLayoutUtils::GetStyleFrame(content->GetPrimaryFrame())->
          GetStyleContext();
      FrameConstructionItemList items;
      AddFrameConstructionItemsInternal(state, content, canvasFrame,
                                        content->Tag(),
                                        content->GetNameSpaceID(),
                                        PR_TRUE,
                                        styleContext,
                                        ITEM_ALLOW_XBL_BASE |
                                          ITEM_ALLOW_PAGE_BREAK,
                                        items);
      for (FCItemIterator iter(items); !iter.IsDone(); iter.Next()) {
        NS_ASSERTION(iter.item().DesiredParentType() ==
                       GetParentType(canvasFrame),
                     "This is not going to work");
        nsresult rv =
          ConstructFramesFromItem(state, iter, canvasFrame, fixedPlaceholders);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }

  
  
  
  NS_ASSERTION(!canvasFrame->GetFirstChild(nsnull),
               "leaking frames; doc root continuation must be empty");
  canvasFrame->SetInitialChildList(nsnull, fixedPlaceholders);
  return NS_OK;
}

nsresult
nsCSSFrameConstructor::GetInsertionPoint(nsIFrame*     aParentFrame,
                                         nsIContent*   aChildContent,
                                         nsIFrame**    aInsertionPoint,
                                         PRBool*       aMultiple)
{
  
  
  *aInsertionPoint = aParentFrame;

  nsIContent* container = aParentFrame->GetContent();
  if (!container)
    return NS_OK;

  nsBindingManager *bindingManager = mDocument->BindingManager();

  nsIContent* insertionElement;
  if (aChildContent) {
    
    
    if (aChildContent->GetBindingParent() == container) {
      
      
      return NS_OK;
    }

    PRUint32 index;
    insertionElement = bindingManager->GetInsertionPoint(container,
                                                         aChildContent,
                                                         &index);
  }
  else {
    PRBool multiple;
    PRUint32 index;
    insertionElement = bindingManager->GetSingleInsertionPoint(container,
                                                               &index,
                                                               &multiple);
    if (multiple && aMultiple)
      *aMultiple = multiple; 
  }

  if (insertionElement) {
    nsIFrame* insertionPoint = insertionElement->GetPrimaryFrame();
    if (insertionPoint) {
      
      insertionPoint = insertionPoint->GetContentInsertionFrame();
      if (insertionPoint && insertionPoint != aParentFrame) 
        GetInsertionPoint(insertionPoint, aChildContent, aInsertionPoint, aMultiple);
    }
    else {
      
      *aInsertionPoint = nsnull;
    }
  }

  
  
  if (aMultiple && !*aMultiple) {
    nsIContent* content = insertionElement ? insertionElement : container;
    if (content->IsHTML(nsGkAtoms::fieldset)) {
      *aMultiple = PR_TRUE;
    }
  }

  return NS_OK;
}



nsresult
nsCSSFrameConstructor::CaptureStateForFramesOf(nsIContent* aContent,
                                               nsILayoutHistoryState* aHistoryState)
{
  nsIFrame* frame = aContent->GetPrimaryFrame();
  if (frame == mRootElementFrame) {
    frame = mFixedContainingBlock;
  }
  if (frame) {
    CaptureStateFor(frame, aHistoryState);
  }
  return NS_OK;
}


nsresult
nsCSSFrameConstructor::CaptureStateFor(nsIFrame* aFrame,
                                       nsILayoutHistoryState* aHistoryState)
{
  if (aFrame && aHistoryState) {
    mPresShell->FrameManager()->CaptureFrameState(aFrame, aHistoryState);
  }
  return NS_OK;
}

nsresult
nsCSSFrameConstructor::MaybeRecreateFramesForElement(Element* aElement)
{
  nsresult result = NS_OK;
  nsFrameManager *frameManager = mPresShell->FrameManager();

  nsStyleContext *oldContext = frameManager->GetUndisplayedContent(aElement);
  if (oldContext) {
    
    nsRefPtr<nsStyleContext> newContext = mPresShell->StyleSet()->
      ResolveStyleFor(aElement, oldContext->GetParent());

    frameManager->ChangeUndisplayedContent(aElement, newContext);
    if (newContext->GetStyleDisplay()->mDisplay != NS_STYLE_DISPLAY_NONE) {
      result = RecreateFramesForContent(aElement, PR_FALSE);
    }
  }
  return result;
}

static nsIFrame*
FindFirstNonWhitespaceChild(nsIFrame* aParentFrame)
{
  nsIFrame* f = aParentFrame->GetFirstChild(nsnull);
  while (f && f->GetType() == nsGkAtoms::textFrame &&
         f->GetContent()->TextIsOnlyWhitespace()) {
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
  } while (f && f->GetType() == nsGkAtoms::textFrame &&
           f->GetContent()->TextIsOnlyWhitespace());
  return f;
}

PRBool
nsCSSFrameConstructor::MaybeRecreateContainerForFrameRemoval(nsIFrame* aFrame,
                                                             nsresult* aResult)
{
  NS_PRECONDITION(aFrame, "Must have a frame");
  NS_PRECONDITION(aFrame->GetParent(), "Frame shouldn't be root");
  NS_PRECONDITION(aResult, "Null out param?");
  NS_PRECONDITION(aFrame == aFrame->GetFirstContinuation(),
                  "aFrame not the result of GetPrimaryFrame()?");

  if (IsFrameSpecial(aFrame)) {
    
    
#ifdef DEBUG
    if (gNoisyContentUpdates) {
      printf("nsCSSFrameConstructor::MaybeRecreateContainerForFrameRemoval: "
             "frame=");
      nsFrame::ListTag(stdout, aFrame);
      printf(" is special\n");
    }
#endif

    *aResult = ReframeContainingBlock(aFrame);
    return PR_TRUE;
  }

  if (aFrame->GetType() == nsGkAtoms::legendFrame &&
      aFrame->GetParent()->GetType() == nsGkAtoms::fieldSetFrame) {
    
    
    *aResult = RecreateFramesForContent(aFrame->GetParent()->GetContent(), PR_FALSE);
    return PR_TRUE;
  }

  
  nsIFrame* inFlowFrame =
    (aFrame->GetStateBits() & NS_FRAME_OUT_OF_FLOW) ?
      mPresShell->FrameManager()->GetPlaceholderFrameFor(aFrame) : aFrame;
  NS_ASSERTION(inFlowFrame, "How did that happen?");
  nsIFrame* parent = inFlowFrame->GetParent();
  if (IsTablePseudo(parent)) {
    if (FindFirstNonWhitespaceChild(parent) == inFlowFrame ||
        !FindNextNonWhitespaceSibling(inFlowFrame->GetLastContinuation()) ||
        
        
        (inFlowFrame->GetType() == nsGkAtoms::tableColGroupFrame &&
         parent->GetFirstChild(nsGkAtoms::colGroupList) == inFlowFrame) ||
        
        (inFlowFrame->GetType() == nsGkAtoms::tableCaptionFrame &&
         parent->GetFirstChild(nsGkAtoms::captionList) == inFlowFrame)) {
      
      
      *aResult = RecreateFramesForContent(parent->GetContent(), PR_TRUE);
      return PR_TRUE;
    }
  }

  
  
  
  
  
  nsIFrame* nextSibling =
    FindNextNonWhitespaceSibling(inFlowFrame->GetLastContinuation());
  NS_ASSERTION(!IsTablePseudo(inFlowFrame), "Shouldn't happen here");
  if (nextSibling && IsTablePseudo(nextSibling)) {
#ifdef DEBUG
    if (gNoisyContentUpdates) {
      printf("nsCSSFrameConstructor::MaybeRecreateContainerForFrameRemoval: "
             "frame=");
      nsFrame::ListTag(stdout, aFrame);
      printf(" has a table pseudo next sibling of different type\n");
    }
#endif
    
    
    *aResult = RecreateFramesForContent(parent->GetContent(), PR_TRUE);
    return PR_TRUE;
  }

#ifdef MOZ_XUL
  if (aFrame->GetType() == nsGkAtoms::popupSetFrame) {
    nsIRootBox* rootBox = nsIRootBox::GetRootBox(mPresShell);
    if (rootBox && rootBox->GetPopupSetFrame() == aFrame) {
      *aResult = ReconstructDocElementHierarchy();
      return PR_TRUE;
    }
  }
#endif

  
  
  
  if (!IsFrameSpecial(parent)) {
    return PR_FALSE;
  }

  
  
  if (inFlowFrame != parent->GetFirstChild(nsnull) ||
      inFlowFrame->GetLastContinuation()->GetNextSibling()) {
    return PR_FALSE;
  }

  
  
  
  nsIFrame* parentFirstContinuation = parent->GetFirstContinuation();
  if (!GetSpecialSibling(parentFirstContinuation) ||
      !GetSpecialPrevSibling(parentFirstContinuation)) {
    return PR_FALSE;
  }

#ifdef DEBUG
  if (gNoisyContentUpdates) {
    printf("nsCSSFrameConstructor::MaybeRecreateContainerForFrameRemoval: "
           "frame=");
    nsFrame::ListTag(stdout, parent);
    printf(" is special\n");
  }
#endif

  *aResult = ReframeContainingBlock(parent);
  return PR_TRUE;
}
 
nsresult
nsCSSFrameConstructor::RecreateFramesForContent(nsIContent* aContent,
                                                PRBool aAsyncInsert)
{
  NS_PRECONDITION(!aAsyncInsert || aContent->IsElement(),
                  "Can only insert elements async");
  
  
  
  
  
  NS_ENSURE_TRUE(aContent->GetDocument(), NS_ERROR_FAILURE);

  
  
  
  
  
  

  nsIFrame* frame = aContent->GetPrimaryFrame();
  if (frame && frame->IsFrameOfType(nsIFrame::eMathML)) {
    
    
    while (PR_TRUE) {
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
      return RecreateFramesForContent(nonGeneratedAncestor->GetContent(), aAsyncInsert);
    }

    nsIFrame* parent = frame->GetParent();
    nsIContent* parentContent = parent ? parent->GetContent() : nsnull;
    
    
    
    if (parent && parent->IsLeaf() && parentContent &&
        parentContent != aContent) {
      return RecreateFramesForContent(parentContent, aAsyncInsert);
    }
  }

  nsresult rv = NS_OK;

  if (frame && MaybeRecreateContainerForFrameRemoval(frame, &rv)) {
    return rv;
  }

  nsINode* containerNode = aContent->GetNodeParent();
  
  if (containerNode) {
    
    
    CaptureStateForFramesOf(aContent, mTempFrameTreeState);

    
    
    nsCOMPtr<nsIContent> container = aContent->GetParent();

    
    PRBool didReconstruct;
    rv = ContentRemoved(container, aContent,
                        aContent->IsRootOfAnonymousSubtree() ?
                          nsnull :
                          aContent->GetNextSibling(),
                        REMOVE_FOR_RECONSTRUCTION, &didReconstruct);

    if (NS_SUCCEEDED(rv) && !didReconstruct) {
      
      
      
      if (aAsyncInsert) {
        PostRestyleEvent(aContent->AsElement(), nsRestyleHint(0),
                         nsChangeHint_ReconstructFrame);
      } else {
        rv = ContentInserted(container, aContent, mTempFrameTreeState, PR_FALSE);
      }
    }
  }

  return rv;
}





already_AddRefed<nsStyleContext>
nsCSSFrameConstructor::GetFirstLetterStyle(nsIContent* aContent,
                                           nsStyleContext* aStyleContext)
{
  if (aContent) {
    return mPresShell->StyleSet()->
      ResolvePseudoElementStyle(aContent->AsElement(),
                                nsCSSPseudoElements::ePseudo_firstLetter,
                                aStyleContext);
  }
  return nsnull;
}

already_AddRefed<nsStyleContext>
nsCSSFrameConstructor::GetFirstLineStyle(nsIContent* aContent,
                                         nsStyleContext* aStyleContext)
{
  if (aContent) {
    return mPresShell->StyleSet()->
      ResolvePseudoElementStyle(aContent->AsElement(),
                                nsCSSPseudoElements::ePseudo_firstLine,
                                aStyleContext);
  }
  return nsnull;
}



PRBool
nsCSSFrameConstructor::ShouldHaveFirstLetterStyle(nsIContent* aContent,
                                                  nsStyleContext* aStyleContext)
{
  return nsLayoutUtils::HasPseudoStyle(aContent, aStyleContext,
                                       nsCSSPseudoElements::ePseudo_firstLetter,
                                       mPresShell->GetPresContext());
}

PRBool
nsCSSFrameConstructor::HasFirstLetterStyle(nsIFrame* aBlockFrame)
{
  NS_PRECONDITION(aBlockFrame, "Need a frame");
  NS_ASSERTION(nsLayoutUtils::GetAsBlock(aBlockFrame),
               "Not a block frame?");

  return (aBlockFrame->GetStateBits() & NS_BLOCK_HAS_FIRST_LETTER_STYLE) != 0;
}

PRBool
nsCSSFrameConstructor::ShouldHaveFirstLineStyle(nsIContent* aContent,
                                                nsStyleContext* aStyleContext)
{
  PRBool hasFirstLine =
    nsLayoutUtils::HasPseudoStyle(aContent, aStyleContext,
                                  nsCSSPseudoElements::ePseudo_firstLine,
                                  mPresShell->GetPresContext());
  if (hasFirstLine) {
    
    PRInt32 namespaceID;
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
                                                   PRBool* aHaveFirstLetterStyle,
                                                   PRBool* aHaveFirstLineStyle)
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
                     &nsCSSFrameConstructor::ConstructTableRow),
    &nsCSSAnonBoxes::tableRow
  },
  { 
    FCDATA_DECL(FCDATA_IS_TABLE_PART | FCDATA_SKIP_FRAMESET |
                FCDATA_DISALLOW_OUT_OF_FLOW | FCDATA_USE_CHILD_ITEMS |
                FCDATA_SKIP_ABSPOS_PUSH |
                FCDATA_DESIRED_PARENT_TYPE_TO_BITS(eTypeTable),
                NS_NewTableRowGroupFrame),
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
  }
};












nsresult
nsCSSFrameConstructor::CreateNeededTablePseudos(nsFrameConstructorState& aState,
                                                FrameConstructionItemList& aItems,
                                                nsIFrame* aParentFrame)
{
  ParentType ourParentType = GetParentType(aParentFrame);
  if (aItems.AllWantParentType(ourParentType)) {
    
    return NS_OK;
  }

  FCItemIterator iter(aItems);
  do {
    if (iter.SkipItemsWantingParentType(ourParentType)) {
      
      return NS_OK;
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
          PRBool trailingSpaces = spaceEndIter.SkipWhitespace(aState);

          
          if (trailingSpaces ||
              spaceEndIter.item().DesiredParentType() != eTypeBlock) {
            PRBool updateStart = (iter == endIter);
            endIter.DeleteItemsTo(spaceEndIter);
            NS_ASSERTION(trailingSpaces == endIter.IsDone(), "These should match");

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
        if (prevParentType == ourParentType) {
          
          break;
        }

        if (ourParentType == eTypeTable &&
            (prevParentType == eTypeColGroup) !=
            (groupingParentType == eTypeColGroup)) {
          
          
          break;
        }

        
        
        
        
        endIter = spaceEndIter;

        endIter.Next();
      } while (!endIter.IsDone());
    }

    if (iter == endIter) {
      
      continue;
    }

    
    
    ParentType wrapperType;
    switch (ourParentType) {
      case eTypeBlock:
        wrapperType = eTypeTable;
        break;
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
      default:
        NS_NOTREACHED("Colgroups should be suppresing non-col child items");
        break;
    }

    const PseudoParentData& pseudoData = sPseudoParentData[wrapperType];
    nsIAtom* pseudoType = *pseudoData.mPseudoType;
    nsStyleContext* parentStyle = aParentFrame->GetStyleContext();
    nsIContent* parentContent = aParentFrame->GetContent();

    if (pseudoType == nsCSSAnonBoxes::table &&
        parentStyle->GetStyleDisplay()->mDisplay == NS_STYLE_DISPLAY_INLINE) {
      pseudoType = nsCSSAnonBoxes::inlineTable;
    }

    nsRefPtr<nsStyleContext> wrapperStyle =
      mPresShell->StyleSet()->ResolveAnonymousBoxStyle(pseudoType, parentStyle);
    FrameConstructionItem* newItem =
      new FrameConstructionItem(&pseudoData.mFCData,
                                
                                parentContent,
                                
                                pseudoType,
                                
                                
                                
                                iter.item().mNameSpaceID,
                                
                                nsnull,
                                wrapperStyle.forget(),
                                PR_TRUE);

    if (!newItem) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    
    
    
    
    
    newItem->mIsAllInline = newItem->mHasInlineEnds =
      newItem->mStyleContext->GetStyleDisplay()->IsInlineOutside();

    
    
    newItem->mChildItems.SetLineBoundaryAtStart(PR_TRUE);
    newItem->mChildItems.SetLineBoundaryAtEnd(PR_TRUE);
    
    
    newItem->mChildItems.SetParentHasNoXBLChildren(
      aItems.ParentHasNoXBLChildren());

    
    
    iter.AppendItemsToList(endIter, newItem->mChildItems);

    iter.InsertItem(newItem);

    
    
    
  } while (!iter.IsDone());

  return NS_OK;
}

inline nsresult
nsCSSFrameConstructor::ConstructFramesFromItemList(nsFrameConstructorState& aState,
                                                   FrameConstructionItemList& aItems,
                                                   nsIFrame* aParentFrame,
                                                   nsFrameItems& aFrameItems)
{
  nsresult rv = CreateNeededTablePseudos(aState, aItems, aParentFrame);
  NS_ENSURE_SUCCESS(rv, rv);

#ifdef DEBUG
  for (FCItemIterator iter(aItems); !iter.IsDone(); iter.Next()) {
    NS_ASSERTION(iter.item().DesiredParentType() == GetParentType(aParentFrame),
                 "Needed pseudos didn't get created; expect bad things");
  }
#endif

  for (FCItemIterator iter(aItems); !iter.IsDone(); iter.Next()) {
    rv = ConstructFramesFromItem(aState, iter, aParentFrame, aFrameItems);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_ASSERTION(!aState.mHavePendingPopupgroup,
               "Should have proccessed it by now");

  return NS_OK;
}

nsresult
nsCSSFrameConstructor::ProcessChildren(nsFrameConstructorState& aState,
                                       nsIContent*              aContent,
                                       nsStyleContext*          aStyleContext,
                                       nsIFrame*                aFrame,
                                       const PRBool             aCanHaveGeneratedContent,
                                       nsFrameItems&            aFrameItems,
                                       const PRBool             aAllowBlockStyles,
                                       PendingBinding*          aPendingBinding)
{
  NS_PRECONDITION(aFrame, "Must have parent frame here");
  NS_PRECONDITION(aFrame->GetContentInsertionFrame() == aFrame,
                  "Parent frame in ProcessChildren should be its own "
                  "content insertion frame");

  
  

  PRBool haveFirstLetterStyle = PR_FALSE, haveFirstLineStyle = PR_FALSE;
  if (aAllowBlockStyles) {
    ShouldHaveSpecialBlockStyle(aContent, aStyleContext, &haveFirstLetterStyle,
                                &haveFirstLineStyle);
  }

  
  nsFrameConstructorSaveState floatSaveState;
  if (aFrame->IsFrameOfType(nsIFrame::eMathML) ||
      aFrame->IsBoxFrame()) {
    aState.PushFloatContainingBlock(nsnull, floatSaveState);
  } else if (aFrame->IsFloatContainingBlock()) {
    aState.PushFloatContainingBlock(aFrame, floatSaveState);
  }

  nsFrameConstructorState::PendingBindingAutoPusher pusher(aState,
                                                           aPendingBinding);

  FrameConstructionItemList itemsToConstruct;
  nsresult rv = NS_OK;

  
  
  if (aAllowBlockStyles && !haveFirstLetterStyle && !haveFirstLineStyle) {
    itemsToConstruct.SetLineBoundaryAtStart(PR_TRUE);
    itemsToConstruct.SetLineBoundaryAtEnd(PR_TRUE);
  }

  
  
  
  nsAutoTArray<nsIAnonymousContentCreator::ContentInfo, 4> anonymousItems;
  GetAnonymousContent(aContent, aFrame, anonymousItems);
  for (PRUint32 i = 0; i < anonymousItems.Length(); ++i) {
    nsIContent* content = anonymousItems[i].mContent;
#ifdef DEBUG
    nsIAnonymousContentCreator* creator = do_QueryFrame(aFrame);
    NS_ASSERTION(!creator || !creator->CreateFrameFor(content),
                 "If you need to use CreateFrameFor, you need to call "
                 "CreateAnonymousFrames manually and not follow the standard "
                 "ProcessChildren() codepath for this frame");
#endif
    
    NS_ABORT_IF_FALSE(!(content->GetFlags() &
                        (NODE_DESCENDANTS_NEED_FRAMES | NODE_NEEDS_FRAME)),
                      "Should not be marked as needing frames");
    NS_ABORT_IF_FALSE(!content->IsElement() ||
                      !(content->GetFlags() & ELEMENT_ALL_RESTYLE_FLAGS),
                      "Should have no pending restyle flags");
    NS_ABORT_IF_FALSE(!content->GetPrimaryFrame(),
                      "Should have no existing frame");
    NS_ABORT_IF_FALSE(!content->IsNodeOfType(nsINode::eCOMMENT) &&
                      !content->IsNodeOfType(nsINode::ePROCESSING_INSTRUCTION),
                      "Why is someone creating garbage anonymous content");

    nsRefPtr<nsStyleContext> styleContext;
    if (anonymousItems[i].mStyleContext) {
      styleContext = anonymousItems[i].mStyleContext.forget();
    } else {
      styleContext = ResolveStyleContext(aFrame, content, &aState);
    }

    AddFrameConstructionItemsInternal(aState, content, aFrame,
                                      content->Tag(), content->GetNameSpaceID(),
                                      PR_TRUE, styleContext,
                                      ITEM_ALLOW_XBL_BASE | ITEM_ALLOW_PAGE_BREAK,
                                      itemsToConstruct);
  }

  if (!aFrame->IsLeaf()) {
    
    
    
    
    
    nsStyleContext* styleContext;

    if (aCanHaveGeneratedContent) {
      styleContext =
        nsFrame::CorrectStyleParentFrame(aFrame, nsnull)->GetStyleContext();
      
      CreateGeneratedContentItem(aState, aFrame, aContent, styleContext,
                                 nsCSSPseudoElements::ePseudo_before,
                                 itemsToConstruct);
    }

    ChildIterator iter, last;
    for (ChildIterator::Init(aContent, &iter, &last);
         iter != last;
         ++iter) {
      nsIContent* child = *iter;
      
      
      if (child->IsElement()) {
        child->UnsetFlags(ELEMENT_ALL_RESTYLE_FLAGS);
      }
      AddFrameConstructionItems(aState, child, iter.XBLInvolved(), aFrame,
                                itemsToConstruct);
    }
    itemsToConstruct.SetParentHasNoXBLChildren(!iter.XBLInvolved());

    if (aCanHaveGeneratedContent) {
      
      CreateGeneratedContentItem(aState, aFrame, aContent, styleContext,
                                 nsCSSPseudoElements::ePseudo_after,
                                 itemsToConstruct);
    }
  } else {
    ClearLazyBits(aContent->GetFirstChild(), nsnull);
  }

  rv = ConstructFramesFromItemList(aState, itemsToConstruct, aFrame,
                                   aFrameItems);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ASSERTION(!aAllowBlockStyles || !aFrame->IsBoxFrame(),
               "can't be both block and box");

  if (haveFirstLetterStyle) {
    rv = WrapFramesInFirstLetterFrame(aContent, aFrame, aFrameItems);
  }
  if (haveFirstLineStyle) {
    rv = WrapFramesInFirstLineFrame(aState, aContent, aFrame, nsnull,
                                    aFrameItems);
  }

  
  
  
  NS_ASSERTION(!haveFirstLineStyle || !aFrame->IsBoxFrame(),
               "Shouldn't have first-line style if we're a box");
  NS_ASSERTION(!aFrame->IsBoxFrame() ||
               itemsToConstruct.AnyItemsNeedBlockParent() ==
                 (AnyKidsNeedBlockParent(aFrameItems.FirstChild()) != nsnull),
               "Something went awry in our block parent calculations");

  if (aFrame->IsBoxFrame() && itemsToConstruct.AnyItemsNeedBlockParent()) {
    
    
    
    nsIContent *badKid = AnyKidsNeedBlockParent(aFrameItems.FirstChild());
    nsDependentAtomString parentTag(aContent->Tag()), kidTag(badKid->Tag());
    const PRUnichar* params[] = { parentTag.get(), kidTag.get() };
    nsStyleContext *frameStyleContext = aFrame->GetStyleContext();
    const nsStyleDisplay *display = frameStyleContext->GetStyleDisplay();
    const char *message =
      (display->mDisplay == NS_STYLE_DISPLAY_INLINE_BOX)
        ? "NeededToWrapXULInlineBox" : "NeededToWrapXUL";
    nsContentUtils::ReportToConsole(nsContentUtils::eXUL_PROPERTIES,
                                    message,
                                    params, NS_ARRAY_LENGTH(params),
                                    nsnull,
                                    EmptyString(), 0, 0, 
                                    nsIScriptError::warningFlag,
                                    "FrameConstructor", mDocument);

    nsRefPtr<nsStyleContext> blockSC = mPresShell->StyleSet()->
      ResolveAnonymousBoxStyle(nsCSSAnonBoxes::mozXULAnonymousBlock,
                               frameStyleContext);
    nsIFrame *blockFrame = NS_NewBlockFrame(mPresShell, blockSC);
    
    
    

    InitAndRestoreFrame(aState, aContent, aFrame, nsnull,
                        blockFrame, PR_FALSE);

    NS_ASSERTION(!blockFrame->HasView(), "need to do view reparenting");
    ReparentFrames(aState.mFrameManager, blockFrame, aFrameItems);

    blockFrame->SetInitialChildList(nsnull, aFrameItems);
    NS_ASSERTION(aFrameItems.IsEmpty(), "How did that happen?");
    aFrameItems.Clear();
    aFrameItems.AddChild(blockFrame);

    aFrame->AddStateBits(NS_STATE_BOX_WRAPS_KIDS_IN_BLOCK);
  }

  return rv;
}















nsresult
nsCSSFrameConstructor::WrapFramesInFirstLineFrame(
  nsFrameConstructorState& aState,
  nsIContent*              aBlockContent,
  nsIFrame*                aBlockFrame,
  nsIFrame*                aLineFrame,
  nsFrameItems&            aFrameItems)
{
  nsresult rv = NS_OK;

  
  nsFrameList::FrameLinkEnumerator link(aFrameItems);
  while (!link.AtEnd() && IsInlineOutside(link.NextFrame())) {
    link.Next();
  }

  nsFrameList firstLineChildren = aFrameItems.ExtractHead(link);

  if (firstLineChildren.IsEmpty()) {
    
    return NS_OK;
  }

  if (!aLineFrame) {
    
    nsStyleContext* parentStyle =
      nsFrame::CorrectStyleParentFrame(aBlockFrame,
                                       nsCSSPseudoElements::firstLine)->
        GetStyleContext();
    nsRefPtr<nsStyleContext> firstLineStyle = GetFirstLineStyle(aBlockContent,
                                                                parentStyle);

    aLineFrame = NS_NewFirstLineFrame(mPresShell, firstLineStyle);

    if (aLineFrame) {
      
      rv = InitAndRestoreFrame(aState, aBlockContent, aBlockFrame, nsnull,
                               aLineFrame);

      
      
      
      aFrameItems.InsertFrame(nsnull, nsnull, aLineFrame);

      NS_ASSERTION(aLineFrame->GetStyleContext() == firstLineStyle,
                   "Bogus style context on line frame");
    }
  }

  if (aLineFrame) {
    
    ReparentFrames(aState.mFrameManager, aLineFrame, firstLineChildren);
    if (aLineFrame->GetChildList(nsnull).IsEmpty() &&
        (aLineFrame->GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
      aLineFrame->SetInitialChildList(nsnull, firstLineChildren);
    } else {
      aState.mFrameManager->AppendFrames(aLineFrame, nsnull, firstLineChildren);
    }
  }
  else {
    rv = NS_ERROR_OUT_OF_MEMORY;
  }

  return rv;
}




nsresult
nsCSSFrameConstructor::AppendFirstLineFrames(
  nsFrameConstructorState& aState,
  nsIContent*              aBlockContent,
  nsIFrame*                aBlockFrame,
  nsFrameItems&            aFrameItems)
{
  
  
  const nsFrameList& blockKids = aBlockFrame->GetChildList(nsnull);
  if (blockKids.IsEmpty()) {
    return WrapFramesInFirstLineFrame(aState, aBlockContent,
                                      aBlockFrame, nsnull, aFrameItems);
  }

  
  
  nsIFrame* lastBlockKid = blockKids.LastChild();
  if (lastBlockKid->GetType() != nsGkAtoms::lineFrame) {
    
    
    
    
    return NS_OK;
  }

  return WrapFramesInFirstLineFrame(aState, aBlockContent, aBlockFrame,
                                    lastBlockKid, aFrameItems);
}




nsresult
nsCSSFrameConstructor::InsertFirstLineFrames(
  nsFrameConstructorState& aState,
  nsIContent*              aContent,
  nsIFrame*                aBlockFrame,
  nsIFrame**               aParentFrame,
  nsIFrame*                aPrevSibling,
  nsFrameItems&            aFrameItems)
{
  nsresult rv = NS_OK;
  
  
  
  
#if 0
  nsIFrame* parentFrame = *aParentFrame;
  nsIFrame* newFrame = aFrameItems.childList;
  PRBool isInline = IsInlineOutside(newFrame);

  if (!aPrevSibling) {
    
    
    nsIFrame* firstBlockKid = aBlockFrame->GetFirstChild(nsnull);
    if (firstBlockKid->GetType() == nsGkAtoms::lineFrame) {
      
      nsIFrame* lineFrame = firstBlockKid;

      if (isInline) {
        
        ReparentFrame(aState.mFrameManager, lineFrame, newFrame);
        aState.mFrameManager->InsertFrames(lineFrame, nsnull, nsnull,
                                           newFrame);

        
        
        aFrameItems.childList = nsnull;
        aFrameItems.lastChild = nsnull;
      }
      else {
        
        
        
      }
    }
    else {
      
      if (isInline) {
        
        nsIFrame* lineFrame = NS_NewFirstLineFrame(firstLineStyle);
        if (!lineFrame) {
          rv = NS_ERROR_OUT_OF_MEMORY;
        }

        if (NS_SUCCEEDED(rv)) {
          
          nsStyleContext* parentStyle =
            nsFrame::CorrectStyleParentFrame(aBlockFrame,
                                             nsCSSPseudoElements::firstLine)->
              GetStyleContext();
          nsRefPtr<nsStyleContext> firstLineStyle =
            GetFirstLineStyle(aContent, parentStyle);

          
          rv = InitAndRestoreFrame(aState, aContent, aBlockFrame,
                                   nsnull, lineFrame);

          
          
          aFrameItems.childList = lineFrame;
          aFrameItems.lastChild = lineFrame;

          
          
          NS_ASSERTION(lineFrame->GetStyleContext() == firstLineStyle,
                       "Bogus style context on line frame");
          ReparentFrame(aPresContext, lineFrame, newFrame);
          lineFrame->SetInitialChildList(nsnull, newFrame);
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
            nsIFrame* kids = nextLineFrame->GetFirstChild(nsnull);
          }
        }
        else {
          
          
          ReparentFrame(aState.mFrameManager, aBlockFrame, newFrame);
          aState.mFrameManager->InsertFrames(aBlockFrame, nsnull,
                                             prevSiblingParent, newFrame);
          aFrameItems.childList = nsnull;
          aFrameItems.lastChild = nsnull;
        }
      }
    }
  }

#endif
  return rv;
}







static PRInt32
FirstLetterCount(const nsTextFragment* aFragment)
{
  PRInt32 count = 0;
  PRInt32 firstLetterLength = 0;
  PRBool done = PR_FALSE;

  PRInt32 i, n = aFragment->GetLength();
  for (i = 0; i < n; i++) {
    PRUnichar ch = aFragment->CharAt(i);
    if (XP_IS_SPACE(ch)) {
      if (firstLetterLength) {
        done = PR_TRUE;
        break;
      }
      count++;
      continue;
    }
    
    if ((ch == '\'') || (ch == '\"')) {
      if (firstLetterLength) {
        done = PR_TRUE;
        break;
      }
      
      firstLetterLength = 1;
    }
    else {
      count++;
      done = PR_TRUE;
      break;
    }
  }

  return count;
}

static PRBool
NeedFirstLetterContinuation(nsIContent* aContent)
{
  NS_PRECONDITION(aContent, "null ptr");

  PRBool result = PR_FALSE;
  if (aContent) {
    const nsTextFragment* frag = aContent->GetText();
    if (frag) {
      PRInt32 flc = FirstLetterCount(frag);
      PRInt32 tl = frag->GetLength();
      if (flc < tl) {
        result = PR_TRUE;
      }
    }
  }
  return result;
}

static PRBool IsFirstLetterContent(nsIContent* aContent)
{
  return aContent->TextLength() &&
         !aContent->TextIsOnlyWhitespace();
}




void
nsCSSFrameConstructor::CreateFloatingLetterFrame(
  nsFrameConstructorState& aState,
  nsIFrame* aBlockFrame,
  nsIContent* aTextContent,
  nsIFrame* aTextFrame,
  nsIContent* aBlockContent,
  nsIFrame* aParentFrame,
  nsStyleContext* aStyleContext,
  nsFrameItems& aResult)
{
  
  nsresult rv;
  nsIFrame* letterFrame;
  nsStyleSet *styleSet = mPresShell->StyleSet();

  letterFrame = NS_NewFirstLetterFrame(mPresShell, aStyleContext);
  
  
  
  nsIContent* letterContent = aTextContent->GetParent();
  nsIFrame* containingBlock = aState.GetGeometricParent(
    aStyleContext->GetStyleDisplay(), aParentFrame);
  InitAndRestoreFrame(aState, letterContent, containingBlock, nsnull,
                      letterFrame);

  
  
  
  
  nsRefPtr<nsStyleContext> textSC;
  textSC = styleSet->ResolveStyleForNonElement(aStyleContext);
  aTextFrame->SetStyleContextWithoutNotification(textSC);
  InitAndRestoreFrame(aState, aTextContent, letterFrame, nsnull, aTextFrame);

  
  SetInitialSingleChild(letterFrame, aTextFrame);

  
  
  
  nsIFrame* nextTextFrame = nsnull;
  if (NeedFirstLetterContinuation(aTextContent)) {
    
    rv = CreateContinuingFrame(aState.mPresContext, aTextFrame, aParentFrame,
                               &nextTextFrame);
    if (NS_FAILED(rv)) {
      letterFrame->Destroy();
      return;
    }
    
    nsStyleContext* parentStyleContext = aStyleContext->GetParent();
    if (parentStyleContext) {
      nsRefPtr<nsStyleContext> newSC;
      newSC = styleSet->ResolveStyleForNonElement(parentStyleContext);
      if (newSC) {
        nextTextFrame->SetStyleContext(newSC);
      }
    }
  }

  NS_ASSERTION(aResult.IsEmpty(), "aResult should be an empty nsFrameItems!");
  
  
  
  nsFrameList::FrameLinkEnumerator link(aState.mFloatedItems);
  while (!link.AtEnd() && link.NextFrame()->GetParent() != containingBlock) {
    link.Next();
  }

  rv = aState.AddChild(letterFrame, aResult, letterContent, aStyleContext,
                       aParentFrame, PR_FALSE, PR_TRUE, PR_FALSE, PR_TRUE,
                       link.PrevFrame());

  if (nextTextFrame) {
    if (NS_FAILED(rv)) {
      nextTextFrame->Destroy();
    } else {
      aResult.AddChild(nextTextFrame);
    }
  }
}





nsresult
nsCSSFrameConstructor::CreateLetterFrame(nsIFrame* aBlockFrame,
                                         nsIFrame* aBlockContinuation,
                                         nsIContent* aTextContent,
                                         nsIFrame* aParentFrame,
                                         nsFrameItems& aResult)
{
  NS_PRECONDITION(aTextContent->IsNodeOfType(nsINode::eTEXT),
                  "aTextContent isn't text");
  NS_ASSERTION(nsLayoutUtils::GetAsBlock(aBlockFrame),
                 "Not a block frame?");

  
  nsStyleContext* parentStyleContext =
    nsFrame::CorrectStyleParentFrame(aParentFrame,
                                     nsCSSPseudoElements::firstLetter)->
      GetStyleContext();

  
  
  nsIContent* blockContent = aBlockFrame->GetContent();

  
  nsRefPtr<nsStyleContext> sc = GetFirstLetterStyle(blockContent,
                                                    parentStyleContext);
  if (sc) {
    nsRefPtr<nsStyleContext> textSC;
    textSC = mPresShell->StyleSet()->ResolveStyleForNonElement(sc);
    
    
    
    
    
    
    
    
    aTextContent->SetPrimaryFrame(nsnull);
    nsIFrame* textFrame = NS_NewTextFrame(mPresShell, textSC);

    NS_ASSERTION(aBlockContinuation == GetFloatContainingBlock(aParentFrame),
                 "Containing block is confused");
    nsFrameConstructorState state(mPresShell, mFixedContainingBlock,
                                  GetAbsoluteContainingBlock(aParentFrame),
                                  aBlockContinuation);

    
    const nsStyleDisplay* display = sc->GetStyleDisplay();
    if (display->IsFloating()) {
      
      CreateFloatingLetterFrame(state, aBlockFrame, aTextContent, textFrame,
                                blockContent, aParentFrame, sc, aResult);
    }
    else {
      
      nsIFrame* letterFrame = NS_NewFirstLetterFrame(mPresShell, sc);

      if (letterFrame) {
        
        
        
        nsIContent* letterContent = aTextContent->GetParent();
        letterFrame->Init(letterContent, aParentFrame, nsnull);

        InitAndRestoreFrame(state, aTextContent, letterFrame, nsnull,
                            textFrame);

        SetInitialSingleChild(letterFrame, textFrame);
        aResult.Clear();
        aResult.AddChild(letterFrame);
        NS_ASSERTION(!aBlockFrame->GetPrevContinuation(),
                     "should have the first continuation here");
        aBlockFrame->AddStateBits(NS_BLOCK_HAS_FIRST_LETTER_CHILD);
      }
    }
    aTextContent->SetPrimaryFrame(textFrame);
  }

  return NS_OK;
}

nsresult
nsCSSFrameConstructor::WrapFramesInFirstLetterFrame(
  nsIContent*              aBlockContent,
  nsIFrame*                aBlockFrame,
  nsFrameItems&            aBlockFrames)
{
  nsresult rv = NS_OK;

  aBlockFrame->AddStateBits(NS_BLOCK_HAS_FIRST_LETTER_STYLE);

  nsIFrame* parentFrame = nsnull;
  nsIFrame* textFrame = nsnull;
  nsIFrame* prevFrame = nsnull;
  nsFrameItems letterFrames;
  PRBool stopLooking = PR_FALSE;
  rv = WrapFramesInFirstLetterFrame(aBlockFrame, aBlockFrame, aBlockFrame,
                                    aBlockFrames.FirstChild(),
                                    &parentFrame, &textFrame, &prevFrame,
                                    letterFrames, &stopLooking);
  if (NS_FAILED(rv)) {
    return rv;
  }
  if (parentFrame) {
    if (parentFrame == aBlockFrame) {
      
      
      aBlockFrames.DestroyFrame(textFrame);
      aBlockFrames.InsertFrames(nsnull, prevFrame, letterFrames);
    }
    else {
      
      mPresShell->FrameManager()->RemoveFrame(nsnull, textFrame);

      
      parentFrame->InsertFrames(nsnull, prevFrame, letterFrames);
    }
  }

  return rv;
}

nsresult
nsCSSFrameConstructor::WrapFramesInFirstLetterFrame(
  nsIFrame*                aBlockFrame,
  nsIFrame*                aBlockContinuation,
  nsIFrame*                aParentFrame,
  nsIFrame*                aParentFrameList,
  nsIFrame**               aModifiedParent,
  nsIFrame**               aTextFrame,
  nsIFrame**               aPrevFrame,
  nsFrameItems&            aLetterFrames,
  PRBool*                  aStopLooking)
{
  nsresult rv = NS_OK;

  nsIFrame* prevFrame = nsnull;
  nsIFrame* frame = aParentFrameList;

  while (frame) {
    nsIFrame* nextFrame = frame->GetNextSibling();

    nsIAtom* frameType = frame->GetType();
    if (nsGkAtoms::textFrame == frameType) {
      
      nsIContent* textContent = frame->GetContent();
      if (IsFirstLetterContent(textContent)) {
        
        rv = CreateLetterFrame(aBlockFrame, aBlockContinuation, textContent,
                               aParentFrame, aLetterFrames);
        if (NS_FAILED(rv)) {
          return rv;
        }

        
        *aModifiedParent = aParentFrame;
        *aTextFrame = frame;
        *aPrevFrame = prevFrame;
        *aStopLooking = PR_TRUE;
        return NS_OK;
      }
    }
    else if (IsInlineFrame(frame) && frameType != nsGkAtoms::brFrame) {
      nsIFrame* kids = frame->GetFirstChild(nsnull);
      WrapFramesInFirstLetterFrame(aBlockFrame, aBlockContinuation, frame,
                                   kids, aModifiedParent, aTextFrame,
                                   aPrevFrame, aLetterFrames, aStopLooking);
      if (*aStopLooking) {
        return NS_OK;
      }
    }
    else {
      
      
      
      
      
      
      *aStopLooking = PR_TRUE;
      break;
    }

    prevFrame = frame;
    frame = nextFrame;
  }

  return rv;
}

nsresult
nsCSSFrameConstructor::RemoveFloatingFirstLetterFrames(
  nsPresContext* aPresContext,
  nsIPresShell* aPresShell,
  nsFrameManager* aFrameManager,
  nsIFrame* aBlockFrame,
  PRBool* aStopLooking)
{
  
  nsIFrame* floatFrame = aBlockFrame->GetFirstChild(nsGkAtoms::floatList);
  while (floatFrame) {
    
    if (nsGkAtoms::letterFrame == floatFrame->GetType()) {
      break;
    }
    floatFrame = floatFrame->GetNextSibling();
  }
  if (!floatFrame) {
    
    return NS_OK;
  }

  
  
  nsIFrame* textFrame = floatFrame->GetFirstChild(nsnull);
  if (!textFrame) {
    return NS_OK;
  }

  
  nsIFrame* parentFrame;
  nsPlaceholderFrame* placeholderFrame = 
    aFrameManager->GetPlaceholderFrameFor(floatFrame);

  if (!placeholderFrame) {
    
    return NS_OK;
  }
  parentFrame = placeholderFrame->GetParent();
  if (!parentFrame) {
    
    return NS_OK;
  }

  
  
  
  nsStyleContext* parentSC = parentFrame->GetStyleContext();
  if (!parentSC) {
    return NS_OK;
  }
  nsIContent* textContent = textFrame->GetContent();
  if (!textContent) {
    return NS_OK;
  }
  nsRefPtr<nsStyleContext> newSC;
  newSC = aPresShell->StyleSet()->ResolveStyleForNonElement(parentSC);
  if (!newSC) {
    return NS_OK;
  }
  nsIFrame* newTextFrame = NS_NewTextFrame(aPresShell, newSC);
  if (NS_UNLIKELY(!newTextFrame)) {
    return NS_ERROR_OUT_OF_MEMORY;;
  }
  newTextFrame->Init(textContent, parentFrame, nsnull);

  
  
  nsIFrame* frameToDelete = textFrame->GetLastContinuation();
  while (frameToDelete != textFrame) {
    nsIFrame* nextFrameToDelete = frameToDelete->GetPrevContinuation();
    aFrameManager->RemoveFrame(nsnull, frameToDelete);
    frameToDelete = nextFrameToDelete;
  }

  nsIFrame* prevSibling = placeholderFrame->GetPrevSibling();

  
#ifdef NOISY_FIRST_LETTER
  printf("RemoveFloatingFirstLetterFrames: textContent=%p oldTextFrame=%p newTextFrame=%p\n",
         textContent.get(), textFrame, newTextFrame);
#endif

  
  aFrameManager->RemoveFrame(nsnull, placeholderFrame);

  
  
  textContent->SetPrimaryFrame(newTextFrame);

  
  nsFrameList textList(newTextFrame, newTextFrame);
  aFrameManager->InsertFrames(parentFrame, nsnull, prevSibling, textList);

  return NS_OK;
}

nsresult
nsCSSFrameConstructor::RemoveFirstLetterFrames(nsPresContext* aPresContext,
                                               nsIPresShell* aPresShell,
                                               nsFrameManager* aFrameManager,
                                               nsIFrame* aFrame,
                                               nsIFrame* aBlockFrame,
                                               PRBool* aStopLooking)
{
  nsIFrame* prevSibling = nsnull;
  nsIFrame* kid = aFrame->GetFirstChild(nsnull);

  while (kid) {
    if (nsGkAtoms::letterFrame == kid->GetType()) {
      
      nsIFrame* textFrame = kid->GetFirstChild(nsnull);
      if (!textFrame) {
        break;
      }

      
      nsStyleContext* parentSC = aFrame->GetStyleContext();
      if (!parentSC) {
        break;
      }
      nsIContent* textContent = textFrame->GetContent();
      if (!textContent) {
        break;
      }
      nsRefPtr<nsStyleContext> newSC;
      newSC = aPresShell->StyleSet()->ResolveStyleForNonElement(parentSC);
      if (!newSC) {
        break;
      }
      textFrame = NS_NewTextFrame(aPresShell, newSC);
      textFrame->Init(textContent, aFrame, nsnull);

      
      aFrameManager->RemoveFrame(nsnull, kid);

      
      
      textContent->SetPrimaryFrame(textFrame);

      
      nsFrameList textList(textFrame, textFrame);
      aFrameManager->InsertFrames(aFrame, nsnull, prevSibling, textList);

      *aStopLooking = PR_TRUE;
      NS_ASSERTION(!aBlockFrame->GetPrevContinuation(),
                   "should have the first continuation here");
      aBlockFrame->RemoveStateBits(NS_BLOCK_HAS_FIRST_LETTER_CHILD);
      break;
    }
    else if (IsInlineFrame(kid)) {
      
      RemoveFirstLetterFrames(aPresContext, aPresShell, aFrameManager,
                              kid, aBlockFrame, aStopLooking);
      if (*aStopLooking) {
        break;
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
                                          nsFrameManager* aFrameManager,
                                          nsIFrame* aBlockFrame)
{
  aBlockFrame = aBlockFrame->GetFirstContinuation();
  nsIFrame* continuation = aBlockFrame;

  PRBool stopLooking = PR_FALSE;
  nsresult rv;
  do {
    rv = RemoveFloatingFirstLetterFrames(aPresContext, aPresShell,
                                         aFrameManager,
                                         continuation, &stopLooking);
    if (NS_SUCCEEDED(rv) && !stopLooking) {
      rv = RemoveFirstLetterFrames(aPresContext, aPresShell, aFrameManager,
                                   continuation, aBlockFrame, &stopLooking);
    }
    if (stopLooking) {
      break;
    }
    continuation = continuation->GetNextContinuation();
  }  while (continuation);
  return rv;
}


nsresult
nsCSSFrameConstructor::RecoverLetterFrames(nsIFrame* aBlockFrame)
{
  aBlockFrame = aBlockFrame->GetFirstContinuation();
  nsIFrame* continuation = aBlockFrame;

  nsIFrame* parentFrame = nsnull;
  nsIFrame* textFrame = nsnull;
  nsIFrame* prevFrame = nsnull;
  nsFrameItems letterFrames;
  PRBool stopLooking = PR_FALSE;
  nsresult rv;
  do {
    
    continuation->AddStateBits(NS_BLOCK_HAS_FIRST_LETTER_STYLE);
    rv = WrapFramesInFirstLetterFrame(aBlockFrame, continuation, continuation,
                                      continuation->GetFirstChild(nsnull),
                                      &parentFrame, &textFrame, &prevFrame,
                                      letterFrames, &stopLooking);
    if (NS_FAILED(rv)) {
      return rv;
    }
    if (stopLooking) {
      break;
    }
    continuation = continuation->GetNextContinuation();
  } while (continuation);

  if (parentFrame) {
    
    mPresShell->FrameManager()->RemoveFrame(nsnull, textFrame);

    
    parentFrame->InsertFrames(nsnull, prevFrame, letterFrames);
  }
  return rv;
}





nsresult
nsCSSFrameConstructor::CreateListBoxContent(nsPresContext* aPresContext,
                                            nsIFrame*       aParentFrame,
                                            nsIFrame*       aPrevFrame,
                                            nsIContent*     aChild,
                                            nsIFrame**      aNewFrame,
                                            PRBool          aIsAppend,
                                            PRBool          aIsScrollbar,
                                            nsILayoutHistoryState* aFrameState)
{
#ifdef MOZ_XUL
  nsresult rv = NS_OK;

  
  if (nsnull != aParentFrame) {
    nsFrameItems            frameItems;
    nsFrameConstructorState state(mPresShell, mFixedContainingBlock,
                                  GetAbsoluteContainingBlock(aParentFrame),
                                  GetFloatContainingBlock(aParentFrame), 
                                  mTempFrameTreeState);

    nsRefPtr<nsStyleContext> styleContext;
    styleContext = ResolveStyleContext(aParentFrame, aChild, &state);

    
    
    const nsStyleDisplay* display = styleContext->GetStyleDisplay();

    if (NS_STYLE_DISPLAY_NONE == display->mDisplay) {
      *aNewFrame = nsnull;
      return NS_OK;
    }

    BeginUpdate();

    FrameConstructionItemList items;
    AddFrameConstructionItemsInternal(state, aChild, aParentFrame,
                                      aChild->Tag(), aChild->GetNameSpaceID(),
                                      PR_TRUE, styleContext,
                                      ITEM_ALLOW_XBL_BASE, items);
    ConstructFramesFromItemList(state, items, aParentFrame, frameItems);

    nsIFrame* newFrame = frameItems.FirstChild();
    *aNewFrame = newFrame;

    if (NS_SUCCEEDED(rv) && (nsnull != newFrame)) {
      
      if (aIsAppend)
        rv = ((nsListBoxBodyFrame*)aParentFrame)->ListBoxAppendFrames(frameItems);
      else
        rv = ((nsListBoxBodyFrame*)aParentFrame)->ListBoxInsertFrames(aPrevFrame, frameItems);
    }

    EndUpdate();
  }

  return rv;
#else
  return NS_ERROR_FAILURE;
#endif
}



nsresult
nsCSSFrameConstructor::ConstructBlock(nsFrameConstructorState& aState,
                                      const nsStyleDisplay*    aDisplay,
                                      nsIContent*              aContent,
                                      nsIFrame*                aParentFrame,
                                      nsIFrame*                aContentParentFrame,
                                      nsStyleContext*          aStyleContext,
                                      nsIFrame**               aNewFrame,
                                      nsFrameItems&            aFrameItems,
                                      PRBool                   aAbsPosContainer,
                                      PendingBinding*          aPendingBinding)
{
  
  nsIFrame* blockFrame = *aNewFrame;
  NS_ASSERTION(blockFrame->GetType() == nsGkAtoms::blockFrame, "not a block frame?");
  nsIFrame* parent = aParentFrame;
  nsRefPtr<nsStyleContext> blockStyle = aStyleContext;
  const nsStyleColumn* columns = aStyleContext->GetStyleColumn();

  if (columns->mColumnCount != NS_STYLE_COLUMN_COUNT_AUTO
      || columns->mColumnWidth.GetUnit() != eStyleUnit_Auto) {
    nsIFrame* columnSetFrame = nsnull;
    columnSetFrame = NS_NewColumnSetFrame(mPresShell, aStyleContext, 0);
    if (!columnSetFrame) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    InitAndRestoreFrame(aState, aContent, aParentFrame, nsnull, columnSetFrame);
    blockStyle = mPresShell->StyleSet()->
      ResolveAnonymousBoxStyle(nsCSSAnonBoxes::columnContent, aStyleContext);
    parent = columnSetFrame;
    *aNewFrame = columnSetFrame;

    SetInitialSingleChild(columnSetFrame, blockFrame);
  }

  blockFrame->SetStyleContextWithoutNotification(blockStyle);
  InitAndRestoreFrame(aState, aContent, parent, nsnull, blockFrame);

  nsresult rv = aState.AddChild(*aNewFrame, aFrameItems, aContent,
                                aStyleContext,
                                aContentParentFrame ? aContentParentFrame :
                                                      aParentFrame);
  if (NS_FAILED(rv)) {
    return rv;
  }

  if (!mRootElementFrame) {
    
    
    mRootElementFrame = *aNewFrame;
  }

  
  
  
  
  
  
  
  nsFrameConstructorSaveState absoluteSaveState;
  if (aAbsPosContainer) {
    
    aState.PushAbsoluteContainingBlock(blockFrame, absoluteSaveState);
  }

  
  nsFrameItems childItems;
  rv = ProcessChildren(aState, aContent, aStyleContext, blockFrame, PR_TRUE,
                       childItems, PR_TRUE, aPendingBinding);

  
  blockFrame->SetInitialChildList(nsnull, childItems);

  return rv;
}

nsresult
nsCSSFrameConstructor::ConstructInline(nsFrameConstructorState& aState,
                                       FrameConstructionItem&   aItem,
                                       nsIFrame*                aParentFrame,
                                       const nsStyleDisplay*    aDisplay,
                                       nsFrameItems&            aFrameItems,
                                       nsIFrame**               aNewFrame)
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  nsIContent* const content = aItem.mContent;
  nsStyleContext* const styleContext = aItem.mStyleContext;

  nsIFrame *newFrame;

  PRBool positioned =
    NS_STYLE_DISPLAY_INLINE == aDisplay->mDisplay &&
    (NS_STYLE_POSITION_RELATIVE == aDisplay->mPosition ||
     aDisplay->HasTransform());
  newFrame = NS_NewInlineFrame(mPresShell, styleContext);

  
  InitAndRestoreFrame(aState, content, aParentFrame, nsnull, newFrame);

  nsFrameConstructorSaveState absoluteSaveState;  
                                                  
                                                  

  if (positioned) {
    
    
    aState.PushAbsoluteContainingBlock(newFrame, absoluteSaveState);
  }

  
  nsFrameItems childItems;
  nsresult rv = ConstructFramesFromItemList(aState, aItem.mChildItems, newFrame,
                                            childItems);
  if (NS_FAILED(rv)) {
    
    return rv;
  }

  nsFrameList::FrameLinkEnumerator firstBlockEnumerator(childItems);
  if (!aItem.mIsAllInline) {
    FindFirstBlock(firstBlockEnumerator);
  }

  if (aItem.mIsAllInline || firstBlockEnumerator.AtEnd()) { 
    
    
    
    
    
    
    newFrame->SetInitialChildList(nsnull, childItems);
    if (NS_SUCCEEDED(rv)) {
      aState.AddChild(newFrame, aFrameItems, content, styleContext, aParentFrame);
      *aNewFrame = newFrame;
    }
    return rv;
  }

  
  

  
  nsFrameList firstInlineKids = childItems.ExtractHead(firstBlockEnumerator);
  newFrame->SetInitialChildList(nsnull, firstInlineKids);

  aFrameItems.AddChild(newFrame);

  CreateIBSiblings(aState, newFrame, positioned, childItems, aFrameItems);

  *aNewFrame = newFrame;
  return NS_OK;
}

void
nsCSSFrameConstructor::CreateIBSiblings(nsFrameConstructorState& aState,
                                        nsIFrame* aInitialInline,
                                        PRBool aIsPositioned,
                                        nsFrameItems& aChildItems,
                                        nsFrameItems& aSiblings)
{
  nsIContent* content = aInitialInline->GetContent();
  nsStyleContext* styleContext = aInitialInline->GetStyleContext();
  nsIFrame* parentFrame = aInitialInline->GetParent();

  
  nsRefPtr<nsStyleContext> blockSC =
    mPresShell->StyleSet()->
      ResolveAnonymousBoxStyle(aIsPositioned ?
                                 nsCSSAnonBoxes::mozAnonymousPositionedBlock :
                                 nsCSSAnonBoxes::mozAnonymousBlock,
                               styleContext);

  nsIFrame* lastNewInline = aInitialInline->GetFirstContinuation();
  do {
    
    
    NS_PRECONDITION(aChildItems.NotEmpty(), "Should have child items");
    NS_PRECONDITION(!IsInlineOutside(aChildItems.FirstChild()),
                    "Must have list starting with block");

    
    
    
    nsIFrame* blockFrame;
    blockFrame = NS_NewBlockFrame(mPresShell, blockSC);

    InitAndRestoreFrame(aState, content, parentFrame, nsnull, blockFrame,
                        PR_FALSE);

    
    
    nsFrameList::FrameLinkEnumerator firstNonBlock =
      FindFirstNonBlock(aChildItems);
    nsFrameList blockKids = aChildItems.ExtractHead(firstNonBlock);

    MoveChildrenTo(aState.mPresContext, aInitialInline, blockFrame, blockKids);

    SetFrameIsSpecial(lastNewInline, blockFrame);
    aSiblings.AddChild(blockFrame);

    
    
    nsIFrame* inlineFrame = NS_NewInlineFrame(mPresShell, styleContext);

    InitAndRestoreFrame(aState, content, parentFrame, nsnull, inlineFrame,
                        PR_FALSE);

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

    SetFrameIsSpecial(blockFrame, inlineFrame);
    aSiblings.AddChild(inlineFrame);
    lastNewInline = inlineFrame;
  } while (aChildItems.NotEmpty());

  SetFrameIsSpecial(lastNewInline, nsnull);
}

void
nsCSSFrameConstructor::BuildInlineChildItems(nsFrameConstructorState& aState,
                                             FrameConstructionItem& aParentItem)
{
  
  
  nsFrameConstructorState::PendingBindingAutoPusher
    pusher(aState, aParentItem.mPendingBinding);

  
  nsStyleContext* const parentStyleContext = aParentItem.mStyleContext;
  nsIContent* const parentContent = aParentItem.mContent;
  CreateGeneratedContentItem(aState, nsnull, parentContent, parentStyleContext,
                             nsCSSPseudoElements::ePseudo_before,
                             aParentItem.mChildItems);

  ChildIterator iter, last;
  for (ChildIterator::Init(parentContent, &iter, &last);
       iter != last;
       ++iter) {
    
    
    
    nsIContent* content = *iter;
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

    AddFrameConstructionItemsInternal(aState, content, nsnull, content->Tag(),
                                      content->GetNameSpaceID(),
                                      iter.XBLInvolved(), childContext,
                                      ITEM_ALLOW_XBL_BASE | ITEM_ALLOW_PAGE_BREAK,
                                      aParentItem.mChildItems);
  }

  
  CreateGeneratedContentItem(aState, nsnull, parentContent, parentStyleContext,
                             nsCSSPseudoElements::ePseudo_after,
                             aParentItem.mChildItems);

  aParentItem.mIsAllInline = aParentItem.mChildItems.AreAllItemsInline();
}




static PRBool
IsSafeToAppendToSpecialInline(nsIFrame* aParentFrame, nsIFrame* aNextSibling)
{
  NS_PRECONDITION(IsInlineFrame(aParentFrame),
                  "Must have an inline parent here");
  do {
    NS_ASSERTION(IsFrameSpecial(aParentFrame), "How is this not special?");
    if (aNextSibling || aParentFrame->GetNextContinuation() ||
        GetSpecialSibling(aParentFrame)) {
      return PR_FALSE;
    }

    aNextSibling = aParentFrame->GetNextSibling();
    aParentFrame = aParentFrame->GetParent();
  } while (IsInlineFrame(aParentFrame));

  return PR_TRUE;
}

PRBool
nsCSSFrameConstructor::WipeContainingBlock(nsFrameConstructorState& aState,
                                           nsIFrame* aContainingBlock,
                                           nsIFrame* aFrame,
                                           FrameConstructionItemList& aItems,
                                           PRBool aIsAppend,
                                           nsIFrame* aPrevSibling)
{
  if (aItems.IsEmpty()) {
    return PR_FALSE;
  }
  
  
  

  
  
  if (aFrame->IsBoxFrame() &&
      !(aFrame->GetStateBits() & NS_STATE_BOX_WRAPS_KIDS_IN_BLOCK) &&
      aItems.AnyItemsNeedBlockParent()) {
    RecreateFramesForContent(aFrame->GetContent(), PR_TRUE);
    return PR_TRUE;
  }

  nsIFrame* nextSibling = ::GetInsertNextSibling(aFrame, aPrevSibling);

  
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
              prevSibling = parentPrevCont->GetLastChild(nsnull);
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
        
        PRBool trailingSpaces = spaceEndIter.SkipWhitespace(aState);

        PRBool okToDrop;
        if (trailingSpaces) {
          
          
          okToDrop = aIsAppend && !nextSibling;
          if (!okToDrop) {
            if (!nextSibling) {
              
              nsIFrame* parentNextCont = aFrame->GetNextContinuation();
              while (parentNextCont) {
                nextSibling = parentNextCont->GetFirstChild(nsnull);
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
      return PR_FALSE;
    }

    if (!aItems.AllWantParentType(parentType)) {
      
      
      RecreateFramesForContent(aFrame->GetContent(), PR_TRUE);
      return PR_TRUE;
    }
  }

  
  
  do {
    if (IsInlineFrame(aFrame)) {
      if (aItems.AreAllItemsInline()) {
        
        return PR_FALSE;
      }

      if (!IsFrameSpecial(aFrame)) {
        
        break;
      }

      
      
      
      
      
      
      
      
      if (aIsAppend && IsSafeToAppendToSpecialInline(aFrame, nextSibling)) {
        return PR_FALSE;
      }

      
      break;
    }

    
    if (!IsFrameSpecial(aFrame)) {
      return PR_FALSE;
    }

    
    
    if (aItems.AreAllItemsBlock()) {
      return PR_FALSE;
    }

    
    break;
  } while (0);

  
  if (!aContainingBlock) {
    aContainingBlock = aFrame;
  }
  
  
  
  
  
  
  
  
  
  
  
  
  
  while (IsFrameSpecial(aContainingBlock) || IsInlineOutside(aContainingBlock) ||
         aContainingBlock->GetStyleContext()->GetPseudo()) {
    aContainingBlock = aContainingBlock->GetParent();
    NS_ASSERTION(aContainingBlock,
                 "Must have non-inline, non-special, non-pseudo frame as root "
                 "(or child of root, for a table root)!");
  }

  
  
  

  nsIContent *blockContent = aContainingBlock->GetContent();
#ifdef DEBUG
  if (gNoisyContentUpdates) {
    printf("nsCSSFrameConstructor::WipeContainingBlock: blockContent=%p\n",
           static_cast<void*>(blockContent));
  }
#endif
  RecreateFramesForContent(blockContent, PR_TRUE);
  return PR_TRUE;
}

nsresult
nsCSSFrameConstructor::ReframeContainingBlock(nsIFrame* aFrame)
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
      return RecreateFramesForContent(blockContent, PR_TRUE);
    }
  }

  
  return RecreateFramesForContent(mPresShell->GetDocument()->GetRootElement(),
				  PR_TRUE);
}

void
nsCSSFrameConstructor::RestyleForEmptyChange(Element* aContainer)
{
  
  
  nsRestyleHint hint = eRestyle_Subtree;
  nsIContent* grandparent = aContainer->GetParent();
  if (grandparent &&
      (grandparent->GetFlags() & NODE_HAS_SLOW_SELECTOR_LATER_SIBLINGS)) {
    hint = nsRestyleHint(hint | eRestyle_LaterSiblings);
  }
  PostRestyleEvent(aContainer, hint, NS_STYLE_HINT_NONE);
}

void
nsCSSFrameConstructor::RestyleForAppend(Element* aContainer,
                                        nsIContent* aFirstNewContent)
{
  NS_ASSERTION(aContainer, "must have container for append");
#ifdef DEBUG
  {
    for (nsIContent* cur = aFirstNewContent; cur; cur = cur->GetNextSibling()) {
      NS_ASSERTION(!cur->IsRootOfAnonymousSubtree(),
                   "anonymous nodes should not be in child lists");
    }
  }
#endif
  PRUint32 selectorFlags =
    aContainer->GetFlags() & (NODE_ALL_SELECTOR_FLAGS &
                              ~NODE_HAS_SLOW_SELECTOR_LATER_SIBLINGS);
  if (selectorFlags == 0)
    return;

  if (selectorFlags & NODE_HAS_EMPTY_SELECTOR) {
    
    PRBool wasEmpty = PR_TRUE; 
    for (nsIContent* cur = aContainer->GetFirstChild();
         cur != aFirstNewContent;
         cur = cur->GetNextSibling()) {
      
      
      
      
      if (nsStyleUtil::IsSignificantChild(cur, PR_TRUE, PR_FALSE)) {
        wasEmpty = PR_FALSE;
        break;
      }
    }
    if (wasEmpty) {
      RestyleForEmptyChange(aContainer);
      return;
    }
  }

  if (selectorFlags & NODE_HAS_SLOW_SELECTOR) {
    PostRestyleEvent(aContainer, eRestyle_Subtree, NS_STYLE_HINT_NONE);
    
    return;
  }

  if (selectorFlags & NODE_HAS_EDGE_CHILD_SELECTOR) {
    
    for (nsIContent* cur = aFirstNewContent->GetPreviousSibling();
         cur;
         cur = cur->GetPreviousSibling()) {
      if (cur->IsElement()) {
        PostRestyleEvent(cur->AsElement(), eRestyle_Subtree, NS_STYLE_HINT_NONE);
        break;
      }
    }
  }
}




static void
RestyleSiblingsStartingWith(nsCSSFrameConstructor *aFrameConstructor,
                            nsIContent *aStartingSibling )
{
  for (nsIContent *sibling = aStartingSibling; sibling;
       sibling = sibling->GetNextSibling()) {
    if (sibling->IsElement()) {
      aFrameConstructor->
        PostRestyleEvent(sibling->AsElement(),
                         nsRestyleHint(eRestyle_Subtree | eRestyle_LaterSiblings),
                         NS_STYLE_HINT_NONE);
      break;
    }
  }
}







void
nsCSSFrameConstructor::RestyleForInsertOrChange(Element* aContainer,
                                                nsIContent* aChild)
{
  NS_ASSERTION(!aChild->IsRootOfAnonymousSubtree(),
               "anonymous nodes should not be in child lists");
  PRUint32 selectorFlags =
    aContainer ? (aContainer->GetFlags() & NODE_ALL_SELECTOR_FLAGS) : 0;
  if (selectorFlags == 0)
    return;

  if (selectorFlags & NODE_HAS_EMPTY_SELECTOR) {
    
    PRBool wasEmpty = PR_TRUE; 
    for (nsIContent* child = aContainer->GetFirstChild();
         child;
         child = child->GetNextSibling()) {
      if (child == aChild)
        continue;
      
      
      
      
      if (nsStyleUtil::IsSignificantChild(child, PR_TRUE, PR_FALSE)) {
        wasEmpty = PR_FALSE;
        break;
      }
    }
    if (wasEmpty) {
      RestyleForEmptyChange(aContainer);
      return;
    }
  }

  if (selectorFlags & NODE_HAS_SLOW_SELECTOR) {
    PostRestyleEvent(aContainer, eRestyle_Subtree, NS_STYLE_HINT_NONE);
    
    return;
  }

  if (selectorFlags & NODE_HAS_SLOW_SELECTOR_LATER_SIBLINGS) {
    
    RestyleSiblingsStartingWith(this, aChild->GetNextSibling());
  }

  if (selectorFlags & NODE_HAS_EDGE_CHILD_SELECTOR) {
    
    PRBool passedChild = PR_FALSE;
    for (nsIContent* content = aContainer->GetFirstChild();
         content;
         content = content->GetNextSibling()) {
      if (content == aChild) {
        passedChild = PR_TRUE;
        continue;
      }
      if (content->IsElement()) {
        if (passedChild) {
          PostRestyleEvent(content->AsElement(), eRestyle_Subtree,
                           NS_STYLE_HINT_NONE);
        }
        break;
      }
    }
    
    passedChild = PR_FALSE;
    for (nsIContent* content = aContainer->GetLastChild();
         content;
         content = content->GetPreviousSibling()) {
      if (content == aChild) {
        passedChild = PR_TRUE;
        continue;
      }
      if (content->IsElement()) {
        if (passedChild) {
          PostRestyleEvent(content->AsElement(), eRestyle_Subtree,
                           NS_STYLE_HINT_NONE);
        }
        break;
      }
    }
  }
}

void
nsCSSFrameConstructor::RestyleForRemove(Element* aContainer,
                                        nsIContent* aOldChild,
                                        nsIContent* aFollowingSibling)
{
  NS_ASSERTION(!aOldChild->IsRootOfAnonymousSubtree(),
               "anonymous nodes should not be in child lists");
  PRUint32 selectorFlags =
    aContainer ? (aContainer->GetFlags() & NODE_ALL_SELECTOR_FLAGS) : 0;
  if (selectorFlags == 0)
    return;

  if (selectorFlags & NODE_HAS_EMPTY_SELECTOR) {
    
    PRBool isEmpty = PR_TRUE; 
    for (nsIContent* child = aContainer->GetFirstChild();
         child;
         child = child->GetNextSibling()) {
      
      
      
      
      if (nsStyleUtil::IsSignificantChild(child, PR_TRUE, PR_FALSE)) {
        isEmpty = PR_FALSE;
        break;
      }
    }
    if (isEmpty) {
      RestyleForEmptyChange(aContainer);
      return;
    }
  }

  if (selectorFlags & NODE_HAS_SLOW_SELECTOR) {
    PostRestyleEvent(aContainer, eRestyle_Subtree, NS_STYLE_HINT_NONE);
    
    return;
  }

  if (selectorFlags & NODE_HAS_SLOW_SELECTOR_LATER_SIBLINGS) {
    
    RestyleSiblingsStartingWith(this, aFollowingSibling);
  }

  if (selectorFlags & NODE_HAS_EDGE_CHILD_SELECTOR) {
    
    PRBool reachedFollowingSibling = PR_FALSE;
    for (nsIContent* content = aContainer->GetFirstChild();
         content;
         content = content->GetNextSibling()) {
      if (content == aFollowingSibling) {
        reachedFollowingSibling = PR_TRUE;
        
      }
      if (content->IsElement()) {
        if (reachedFollowingSibling) {
          PostRestyleEvent(content->AsElement(), eRestyle_Subtree,
                           NS_STYLE_HINT_NONE);
        }
        break;
      }
    }
    
    reachedFollowingSibling = (aFollowingSibling == nsnull);
    for (nsIContent* content = aContainer->GetLastChild();
         content;
         content = content->GetPreviousSibling()) {
      if (content->IsElement()) {
        if (reachedFollowingSibling) {
          PostRestyleEvent(content->AsElement(), eRestyle_Subtree, NS_STYLE_HINT_NONE);
        }
        break;
      }
      if (content == aFollowingSibling) {
        reachedFollowingSibling = PR_TRUE;
      }
    }
  }
}


void
nsCSSFrameConstructor::RebuildAllStyleData(nsChangeHint aExtraHint)
{
  NS_ASSERTION(!(aExtraHint & nsChangeHint_ReconstructFrame),
               "Should not reconstruct the root of the frame tree.  "
               "Use ReconstructDocElementHierarchy instead.");

  mRebuildAllStyleData = PR_FALSE;
  NS_UpdateHint(aExtraHint, mRebuildAllExtraHint);
  mRebuildAllExtraHint = nsChangeHint(0);

  if (!mPresShell || !mPresShell->GetRootFrame())
    return;

  
  nsIViewManager::UpdateViewBatch batch(mPresShell->GetViewManager());

  
  
  nsCOMPtr<nsIPresShell> kungFuDeathGrip(mPresShell);

  
  
  mPresShell->GetDocument()->FlushPendingNotifications(Flush_ContentAndNotify);

  nsAutoScriptBlocker scriptBlocker;

  
  
  nsresult rv = mPresShell->StyleSet()->BeginReconstruct();
  if (NS_FAILED(rv)) {
    batch.EndUpdateViewBatch(NS_VMREFRESH_NO_SYNC);
    return;
  }

  nsPresContext *presContext = mPresShell->GetPresContext();
  presContext->SetProcessingRestyles(PR_TRUE);
  
  
  
  
  
  
  nsStyleChangeList changeList;
  
  
  
  mPresShell->FrameManager()->ComputeStyleChangeFor(mPresShell->GetRootFrame(),
                                                    &changeList, aExtraHint,
                                                    mPendingRestyles, PR_TRUE);
  
  ProcessRestyledFrames(changeList);
  presContext->SetProcessingRestyles(PR_FALSE);

  
  
  
  
  ProcessPendingRestyles();

  
  
  
  
  
  mPresShell->StyleSet()->EndReconstruct();
  batch.EndUpdateViewBatch(NS_VMREFRESH_NO_SYNC);
}

void
nsCSSFrameConstructor::ProcessPendingRestyles()
{
  NS_PRECONDITION(mDocument, "No document?  Pshaw!");
  NS_PRECONDITION(!nsContentUtils::IsSafeToRunScript(),
                  "Missing a script blocker!");

  
  nsPresContext *presContext = mPresShell->GetPresContext();
  NS_ABORT_IF_FALSE(!presContext->IsProcessingRestyles(),
                    "Nesting calls to ProcessPendingRestyles?");
  presContext->SetProcessingRestyles(PR_TRUE);

  mPendingRestyles.ProcessRestyles();

#ifdef DEBUG
  PRUint32 oldPendingRestyleCount = mPendingRestyles.Count();
#endif

  
  
  
  
  
  
  
  
  presContext->SetProcessingAnimationStyleChange(PR_TRUE);
  mPendingAnimationRestyles.ProcessRestyles();
  presContext->SetProcessingAnimationStyleChange(PR_FALSE);

  presContext->SetProcessingRestyles(PR_FALSE);
  NS_POSTCONDITION(mPendingRestyles.Count() == oldPendingRestyleCount,
                   "We should not have posted new non-animation restyles while "
                   "processing animation restyles");

  if (mRebuildAllStyleData) {
    
    
    
    RebuildAllStyleData(nsChangeHint(0));
  }
}

void
nsCSSFrameConstructor::PostRestyleEventCommon(Element* aElement,
                                              nsRestyleHint aRestyleHint,
                                              nsChangeHint aMinChangeHint,
                                              PRBool aForAnimation)
{
  if (NS_UNLIKELY(mPresShell->IsDestroying())) {
    return;
  }

  if (aRestyleHint == 0 && !aMinChangeHint) {
    
    return;
  }

  RestyleTracker& tracker =
    aForAnimation ? mPendingAnimationRestyles : mPendingRestyles;
  tracker.AddPendingRestyle(aElement, aRestyleHint, aMinChangeHint);

  PostRestyleEventInternal(PR_FALSE);
}
    
void
nsCSSFrameConstructor::PostRestyleEventInternal(PRBool aForLazyConstruction)
{
  
  
  
  PRBool inRefresh = !aForLazyConstruction && mInStyleRefresh;
  if (!mObservingRefreshDriver && !inRefresh) {
    mObservingRefreshDriver = mPresShell->GetPresContext()->RefreshDriver()->
      AddStyleFlushObserver(mPresShell);
  }
}

void
nsCSSFrameConstructor::PostRebuildAllStyleDataEvent(nsChangeHint aExtraHint)
{
  NS_ASSERTION(!(aExtraHint & nsChangeHint_ReconstructFrame),
               "Should not reconstruct the root of the frame tree.  "
               "Use ReconstructDocElementHierarchy instead.");

  mRebuildAllStyleData = PR_TRUE;
  NS_UpdateHint(mRebuildAllExtraHint, aExtraHint);
  
  PostRestyleEventInternal(PR_FALSE);
}

nsresult
nsCSSFrameConstructor::GenerateChildFrames(nsIFrame* aFrame)
{
  {
    nsAutoScriptBlocker scriptBlocker;
    BeginUpdate();

    nsFrameItems childItems;
    nsFrameConstructorState state(mPresShell, nsnull, nsnull, nsnull);
    
    
    
    nsresult rv = ProcessChildren(state, aFrame->GetContent(), aFrame->GetStyleContext(),
                                  aFrame, PR_FALSE, childItems, PR_FALSE,
                                  nsnull);
    if (NS_FAILED(rv)) {
      EndUpdate();
      return rv;
    }

    aFrame->SetInitialChildList(nsnull, childItems);

    EndUpdate();
  }

#ifdef ACCESSIBILITY
  nsAccessibilityService* accService = nsIPresShell::AccService();
  if (accService) {
    nsIContent* container = aFrame->GetContent();
    nsIContent* child = container->GetFirstChild();
    if (child) {
      accService->ContentRangeInserted(mPresShell, container, child, nsnull);
    }
  }
#endif

  
  mPresShell->GetDocument()->BindingManager()->ProcessAttachedQueue();

  return NS_OK;
}




PRBool
nsCSSFrameConstructor::
FrameConstructionItem::IsWhitespace(nsFrameConstructorState& aState) const
{
  NS_PRECONDITION(aState.mCreatingExtraFrames ||
                  !mContent->GetPrimaryFrame(), "How did that happen?");
  if (!mIsText) {
    return PR_FALSE;
  }
  mContent->SetFlags(NS_CREATE_FRAME_IF_NON_WHITESPACE |
                     NS_REFRAME_IF_WHITESPACE);
  return mContent->TextIsOnlyWhitespace();
}




void
nsCSSFrameConstructor::FrameConstructionItemList::
AdjustCountsForItem(FrameConstructionItem* aItem, PRInt32 aDelta)
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




inline PRBool
nsCSSFrameConstructor::FrameConstructionItemList::
Iterator::SkipItemsWantingParentType(ParentType aParentType)
{
  NS_PRECONDITION(!IsDone(), "Shouldn't be done yet");
  while (item().DesiredParentType() == aParentType) {
    Next();
    if (IsDone()) {
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}

inline PRBool
nsCSSFrameConstructor::FrameConstructionItemList::
Iterator::SkipWhitespace(nsFrameConstructorState& aState)
{
  NS_PRECONDITION(!IsDone(), "Shouldn't be done yet");
  NS_PRECONDITION(item().IsWhitespace(aState), "Not pointing to whitespace?");
  do {
    Next();
    if (IsDone()) {
      return PR_TRUE;
    }
  } while (item().IsWhitespace(aState));

  return PR_FALSE;
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

  if (!AtStart() || !aEnd.IsDone() || !aTargetList.IsEmpty()) {
    do {
      AppendItemToList(aTargetList);
    } while (*this != aEnd);
    return;
  }

  
  PR_INSERT_AFTER(&aTargetList.mItems, &mList.mItems);
  PR_REMOVE_LINK(&mList.mItems);

  
  aTargetList.mInlineCount = mList.mInlineCount;
  aTargetList.mBlockCount = mList.mBlockCount;
  aTargetList.mLineParticipantCount = mList.mLineParticipantCount;
  aTargetList.mItemCount = mList.mItemCount;
  memcpy(aTargetList.mDesiredParentCounts, mList.mDesiredParentCounts,
         sizeof(aTargetList.mDesiredParentCounts));

  
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
