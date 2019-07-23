












































#include "nsCSSFrameConstructor.h"
#include "nsCRT.h"
#include "nsIAtom.h"
#include "nsIURL.h"
#include "nsISupportsArray.h"
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
#include "nsStyleSet.h"
#include "nsIViewManager.h"
#include "nsIEventStateManager.h"
#include "nsIScrollableView.h"
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
#include "nsIRadioControlFrame.h"
#include "nsICheckboxControlFrame.h"
#include "nsIDOMCharacterData.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsPlaceholderFrame.h"
#include "nsTableRowGroupFrame.h"
#include "nsStyleChangeList.h"
#include "nsIFormControl.h"
#include "nsCSSAnonBoxes.h"
#include "nsCSSPseudoElements.h"
#include "nsIDeviceContext.h"
#include "nsTextFragment.h"
#include "nsISupportsArray.h"
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
#include "nsISelectElement.h"
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
#include "nsIFocusEventSuppressor.h"
#include "nsBox.h"
#include "nsTArray.h"

#ifdef MOZ_XUL
#include "nsIRootBox.h"
#include "nsIDOMXULCommandDispatcher.h"
#include "nsIDOMXULDocument.h"
#include "nsIXULDocument.h"
#endif
#ifdef ACCESSIBILITY
#include "nsIAccessibilityService.h"
#include "nsIAccessibleEvent.h"
#endif

#include "nsInlineFrame.h"
#include "nsBlockFrame.h"

#include "nsIScrollableFrame.h"

#include "nsIXBLService.h"

#undef NOISY_FIRST_LETTER

#ifdef MOZ_MATHML
#include "nsMathMLParts.h"
#endif
#ifdef MOZ_SVG
#include "nsSVGEffects.h"
#include "nsSVGUtils.h"
#include "nsSVGOuterSVGFrame.h"
#endif

nsIFrame*
NS_NewHTMLCanvasFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);

#if defined(MOZ_MEDIA)
nsIFrame*
NS_NewHTMLVideoFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);
#endif

#ifdef MOZ_SVG
#include "nsISVGTextContentMetrics.h"

PRBool
NS_SVGEnabled();
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
PRBool 
NS_SVG_PassesConditionalProcessingTests(nsIContent *aContent);
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
#endif

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
static PRBool gVerifyFastFindFrame = PR_FALSE;
static PRBool gTablePseudoFrame = PR_FALSE;

struct FrameCtorDebugFlags {
  const char* name;
  PRBool* on;
};

static FrameCtorDebugFlags gFlags[] = {
  { "content-updates",              &gNoisyContentUpdates },
  { "really-noisy-content-updates", &gReallyNoisyContentUpdates },
  { "noisy-inline",                 &gNoisyInlineConstruction },
  { "fast-find-frame",              &gVerifyFastFindFrame },
  { "table-pseudo",                 &gTablePseudoFrame },
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

static nsresult
DeletingFrameSubtree(nsFrameManager* aFrameManager,
                     nsIFrame*       aFrame);

static inline nsIFrame*
GetFieldSetBlockFrame(nsIFrame* aFieldsetFrame)
{
  
  nsIFrame* firstChild = aFieldsetFrame->GetFirstChild(nsnull);
  return firstChild && firstChild->GetNextSibling() ? firstChild->GetNextSibling() : firstChild;
}



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
  aFrameManager->ReParentStyleContext(aFrame);
  if (aFrame->GetStateBits() &
      (NS_FRAME_HAS_VIEW | NS_FRAME_HAS_CHILD_WITH_VIEW)) {
    
    
    NS_ASSERTION(aNewParentFrame->GetParent()->GetStateBits() &
                   NS_FRAME_HAS_CHILD_WITH_VIEW,
                 "aNewParentFrame's parent should have this bit set!");
    aNewParentFrame->AddStateBits(NS_FRAME_HAS_CHILD_WITH_VIEW);
  }
}







static inline PRBool
IsFrameSpecial(nsIFrame* aFrame)
{
  return (aFrame->GetStateBits() & NS_FRAME_IS_SPECIAL) != 0;
}

static nsIFrame* GetSpecialSibling(nsIFrame* aFrame)
{
  
  
  aFrame = aFrame->GetFirstContinuation();

  void* value = aFrame->GetProperty(nsGkAtoms::IBSplitSpecialSibling);

  return static_cast<nsIFrame*>(value);
}

static nsIFrame*
GetIBSplitSpecialPrevSiblingForAnonymousBlock(nsIFrame* aFrame)
{
  NS_PRECONDITION(IsFrameSpecial(aFrame) && !IsInlineFrame(aFrame),
                  "Shouldn't call this");
  
  
  
  return
    static_cast<nsIFrame*>
    (aFrame->GetFirstContinuation()->
       GetProperty(nsGkAtoms::IBSplitSpecialPrevSibling));
}

static nsIFrame*
GetLastSpecialSibling(nsIFrame* aFrame, PRBool aIgnoreEmpty)
{
  for (nsIFrame *frame = aFrame, *next; ; frame = next) {
    next = GetSpecialSibling(frame);
    if (!next ||
        (aIgnoreEmpty && !next->GetFirstChild(nsnull)))
      return frame;
  }
  NS_NOTREACHED("unreachable code");
  return nsnull;
}

static void
SetFrameIsSpecial(nsIFrame* aFrame, nsIFrame* aSpecialSibling)
{
  NS_PRECONDITION(aFrame, "bad args!");

  
  for (nsIFrame* frame = aFrame; frame != nsnull; frame = frame->GetNextContinuation()) {
    frame->AddStateBits(NS_FRAME_IS_SPECIAL);
  }

  if (aSpecialSibling) {
    
    NS_ASSERTION(!aFrame->GetPrevInFlow(),
                 "assigning special sibling to other than first-in-flow!");

    
    
    aFrame->SetProperty(nsGkAtoms::IBSplitSpecialSibling, aSpecialSibling);
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
        !parentFrame->GetStyleContext()->GetPseudoType())
      break;

    aFrame = parentFrame;
  } while (1);
 
  
  NS_ASSERTION(parentFrame, "no normal ancestor found for special frame in GetIBContainingBlockFor");
  NS_ASSERTION(parentFrame != aFrame, "parentFrame is actually the child frame - bogus reslt");

  return parentFrame;
}











static nsIFrame*
FindFirstBlock(nsIFrame* aKid, nsIFrame** aPrevKid)
{
  nsIFrame* prevKid = nsnull;
  while (aKid) {
    if (!IsInlineOutside(aKid)) {
      *aPrevKid = prevKid;
      return aKid;
    }
    prevKid = aKid;
    aKid = aKid->GetNextSibling();
  }
  *aPrevKid = nsnull;
  return nsnull;
}

static nsIFrame*
FindLastBlock(nsIFrame* aKid)
{
  nsIFrame* lastBlock = nsnull;
  while (aKid) {
    if (!IsInlineOutside(aKid)) {
      lastBlock = aKid;
    }
    aKid = aKid->GetNextSibling();
  }
  return lastBlock;
}








inline void
MarkIBSpecialPrevSibling(nsIFrame *aAnonymousFrame,
                         nsIFrame *aSpecialParent)
{
  aAnonymousFrame->SetProperty(nsGkAtoms::IBSplitSpecialPrevSibling,
                               aSpecialParent, nsnull, nsnull);
}



static PRBool
IsOutOfFlowList(nsIAtom* aListName)
{
  return
    aListName == nsGkAtoms::floatList ||
    aListName == nsGkAtoms::absoluteList ||
    aListName == nsGkAtoms::overflowOutOfFlowList ||
    aListName == nsGkAtoms::fixedList;
}






static void
DoCleanupFrameReferences(nsFrameManager*  aFrameManager,
                         nsIFrame*        aFrameIn)
{
  nsIContent* content = aFrameIn->GetContent();

  if (aFrameIn->GetType() == nsGkAtoms::placeholderFrame) {
    nsPlaceholderFrame* placeholder = static_cast<nsPlaceholderFrame*>
                                                 (aFrameIn);
    
    aFrameIn = nsPlaceholderFrame::GetRealFrameForPlaceholder(placeholder);

    
    
    
    
    aFrameManager->UnregisterPlaceholderFrame(placeholder);
  }

  
  aFrameManager->RemoveAsPrimaryFrame(content, aFrameIn);
  aFrameManager->ClearAllUndisplayedContentIn(content);

  
  nsIAtom* childListName = nsnull;
  PRInt32 childListIndex = 0;
  do {
    nsIFrame* childFrame = aFrameIn->GetFirstChild(childListName);
    while (childFrame) {
      DoCleanupFrameReferences(aFrameManager, childFrame);
    
      
      childFrame = childFrame->GetNextSibling();
    }

    childListName = aFrameIn->GetAdditionalChildListName(childListIndex++);
  } while (childListName);
}


static void
CleanupFrameReferences(nsFrameManager*  aFrameManager,
                       nsIFrame*        aFrameList)
{
  while (aFrameList) {
    DoCleanupFrameReferences(aFrameManager, aFrameList);

    
    aFrameList = aFrameList->GetNextSibling();
  }
}




struct nsFrameItems {
  nsIFrame* childList;
  nsIFrame* lastChild;
  
  nsFrameItems(nsIFrame* aFrame = nsnull);

  
  void AddChild(nsIFrame* aChild);

  
  void InsertChildAfter(nsIFrame* aChild, nsIFrame* aAfter);

  
  
  
  PRBool RemoveChild(nsIFrame* aChild, nsIFrame* aPrevSibling);
};

nsFrameItems::nsFrameItems(nsIFrame* aFrame)
  : childList(aFrame), lastChild(aFrame)
{
}

void 
nsFrameItems::AddChild(nsIFrame* aChild)
{
#ifdef DEBUG
  nsIFrame* oldLastChild = lastChild;
#endif
  
  if (childList == nsnull) {
    childList = lastChild = aChild;
  }
  else
  {
    NS_ASSERTION(aChild != lastChild,
                 "Same frame being added to frame list twice?");
    lastChild->SetNextSibling(aChild);
    lastChild = aChild;
  }
  
  for (nsIFrame* sib = lastChild->GetNextSibling(); sib;
       sib = sib->GetNextSibling()) {
    NS_ASSERTION(oldLastChild != sib, "Loop in frame list");
    lastChild = sib;
  }
}

void
nsFrameItems::InsertChildAfter(nsIFrame* aChild, nsIFrame* aAfter)
{
  if (!childList || (aAfter && !aAfter->GetNextSibling())) {
    
    AddChild(aChild);
    return;
  }
  if (!aAfter) {
    
    aChild->SetNextSibling(childList);
    childList = aChild;
    return;
  }
  aChild->SetNextSibling(aAfter->GetNextSibling());
  aAfter->SetNextSibling(aChild);
}

PRBool
nsFrameItems::RemoveChild(nsIFrame* aFrame, nsIFrame* aPrevSibling)
{
  NS_PRECONDITION(aFrame, "null ptr");

  nsIFrame* prev;
  if (aPrevSibling) {
    prev = aPrevSibling;
  } else {
    prev = nsnull;
    nsIFrame* sib;
    for (sib = childList; sib && sib != aFrame; sib = sib->GetNextSibling()) {
      prev = sib;
    }
    if (!sib) {
      return PR_FALSE;
    }
  }

  NS_ASSERTION(!prev || prev->GetNextSibling() == aFrame,
               "Unexpected prevsibling");

  if (aFrame == childList) {
    childList = aFrame->GetNextSibling();
  } else {
    prev->SetNextSibling(aFrame->GetNextSibling());
  }
  if (aFrame == lastChild) {
    lastChild = prev;
  }
  aFrame->SetNextSibling(nsnull);
  return PR_TRUE;
}





struct nsAbsoluteItems : nsFrameItems {
  
  nsIFrame* containingBlock;
  
  nsAbsoluteItems(nsIFrame* aContainingBlock);
#ifdef DEBUG
  
  
  
  ~nsAbsoluteItems() {
    NS_ASSERTION(!childList,
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

































struct nsPseudoFrameData {
  nsIFrame*    mFrame; 
  nsFrameItems mChildList;  
  nsFrameItems mChildList2; 

  nsPseudoFrameData();
  nsPseudoFrameData(nsPseudoFrameData& aOther);
  void Reset();
#ifdef DEBUG
  void Dump();
#endif
};

struct nsPseudoFrames {
  nsPseudoFrameData mTableOuter; 
  nsPseudoFrameData mTableInner;  
  nsPseudoFrameData mRowGroup;   
  nsPseudoFrameData mColGroup;
  nsPseudoFrameData mRow;   
  nsPseudoFrameData mCellOuter;
  nsPseudoFrameData mCellInner;

  
  nsIAtom*          mLowestType;

  nsPseudoFrames();
  nsPseudoFrames& operator=(const nsPseudoFrames& aOther);
  void Reset(nsPseudoFrames* aSave = nsnull);
  PRBool IsEmpty() { return (!mLowestType && !mColGroup.mFrame); }
#ifdef DEBUG
  void Dump();
#endif
};

nsPseudoFrameData::nsPseudoFrameData()
: mFrame(nsnull), mChildList(), mChildList2()
{}

nsPseudoFrameData::nsPseudoFrameData(nsPseudoFrameData& aOther)
: mFrame(aOther.mFrame), mChildList(aOther.mChildList), 
  mChildList2(aOther.mChildList2)
{}

void
nsPseudoFrameData::Reset()
{
  mFrame = nsnull;
  mChildList.childList  = mChildList.lastChild  = nsnull;
  mChildList2.childList = mChildList2.lastChild = nsnull;
}

#ifdef DEBUG
void
nsPseudoFrameData::Dump()
{
  nsIFrame* main = nsnull;
  nsIFrame* second = nsnull;
  printf("        %p\n", static_cast<void*>(mFrame));
  main = mChildList.childList;

 
  second = mChildList2.childList;
  while (main || second) {
    printf("          %p   %p\n", static_cast<void*>(main),
           static_cast<void*>(second));
    if (main)
      main = main->GetNextSibling();
    if (second)
      second = second->GetNextSibling();
  }
}
#endif
nsPseudoFrames::nsPseudoFrames() 
: mTableOuter(), mTableInner(), mRowGroup(), mColGroup(), 
  mRow(), mCellOuter(), mCellInner(), mLowestType(nsnull)
{}

nsPseudoFrames& nsPseudoFrames::operator=(const nsPseudoFrames& aOther)
{
  mTableOuter = aOther.mTableOuter;
  mTableInner = aOther.mTableInner;
  mColGroup   = aOther.mColGroup;
  mRowGroup   = aOther.mRowGroup;
  mRow        = aOther.mRow;
  mCellOuter  = aOther.mCellOuter;
  mCellInner  = aOther.mCellInner;
  mLowestType = aOther.mLowestType;

  return *this;
}
void
nsPseudoFrames::Reset(nsPseudoFrames* aSave) 
{
  if (aSave) {
    *aSave = *this;
  }

  mTableOuter.Reset();
  mTableInner.Reset();
  mColGroup.Reset();
  mRowGroup.Reset();
  mRow.Reset();
  mCellOuter.Reset();
  mCellInner.Reset();
  mLowestType = nsnull;
}

#ifdef DEBUG
void
nsPseudoFrames::Dump()
{
  if (IsEmpty()) {
    
    NS_ASSERTION(!mTableOuter.mFrame,    "Pseudo Outer Table Frame not empty");
    NS_ASSERTION(!mTableOuter.mChildList.childList, "Pseudo Outer Table Frame has primary children");
    NS_ASSERTION(!mTableOuter.mChildList2.childList,"Pseudo Outer Table Frame has secondary children");
    NS_ASSERTION(!mTableInner.mFrame,    "Pseudo Inner Table Frame not empty");
    NS_ASSERTION(!mTableInner.mChildList.childList, "Pseudo Inner Table Frame has primary children");
    NS_ASSERTION(!mTableInner.mChildList2.childList,"Pseudo Inner Table Frame has secondary children");
    NS_ASSERTION(!mColGroup.mFrame,      "Pseudo Colgroup Frame not empty");
    NS_ASSERTION(!mColGroup.mChildList.childList,   "Pseudo Colgroup Table Frame has primary children");
    NS_ASSERTION(!mColGroup.mChildList2.childList,  "Pseudo Colgroup Table Frame has secondary children");
    NS_ASSERTION(!mRowGroup.mFrame,      "Pseudo Rowgroup Frame not empty");
    NS_ASSERTION(!mRowGroup.mChildList.childList,   "Pseudo Rowgroup Frame has primary children");
    NS_ASSERTION(!mRowGroup.mChildList2.childList,  "Pseudo Rowgroup Frame has secondary children");
    NS_ASSERTION(!mRow.mFrame,           "Pseudo Row Frame not empty");
    NS_ASSERTION(!mRow.mChildList.childList,        "Pseudo Row Frame has primary children");
    NS_ASSERTION(!mRow.mChildList2.childList,       "Pseudo Row Frame has secondary children");
    NS_ASSERTION(!mCellOuter.mFrame,     "Pseudo Outer Cell Frame not empty");
    NS_ASSERTION(!mCellOuter.mChildList.childList,  "Pseudo Outer Cell Frame has primary children");
    NS_ASSERTION(!mCellOuter.mChildList2.childList, "Pseudo Outer Cell Frame has secondary children");
    NS_ASSERTION(!mCellInner.mFrame,     "Pseudo Inner Cell Frame not empty");
    NS_ASSERTION(!mCellInner.mChildList.childList,  "Pseudo Inner Cell Frame has primary children");
    NS_ASSERTION(!mCellInner.mChildList2.childList, "Pseudo inner Cell Frame has secondary children");
  }
  else {
    if (mTableOuter.mFrame || mTableOuter.mChildList.childList || mTableOuter.mChildList2.childList) {
      if (nsGkAtoms::tableOuterFrame == mLowestType) {
        printf("LOW OuterTable\n");
      }
      else {
        printf("    OuterTable\n");
      }
      mTableOuter.Dump();
    }
    if (mTableInner.mFrame || mTableInner.mChildList.childList || mTableInner.mChildList2.childList) {
      if (nsGkAtoms::tableFrame == mLowestType) {
        printf("LOW InnerTable\n");
      }
      else {
        printf("    InnerTable\n");
      }
      mTableInner.Dump();
    }
    if (mColGroup.mFrame || mColGroup.mChildList.childList || mColGroup.mChildList2.childList) {
      if (nsGkAtoms::tableColGroupFrame == mLowestType) {
        printf("LOW ColGroup\n");
      }
      else {
        printf("    ColGroup\n");
      }
      mColGroup.Dump();
    }
    if (mRowGroup.mFrame || mRowGroup.mChildList.childList || mRowGroup.mChildList2.childList) {
      if (nsGkAtoms::tableRowGroupFrame == mLowestType) {
        printf("LOW RowGroup\n");
      }
      else {
        printf("    RowGroup\n");
      }
      mRowGroup.Dump();
    }
    if (mRow.mFrame || mRow.mChildList.childList || mRow.mChildList2.childList) {
      if (nsGkAtoms::tableRowFrame == mLowestType) {
        printf("LOW Row\n");
      }
      else {
        printf("    Row\n");
      }
      mRow.Dump();
    }
    
    if (mCellOuter.mFrame || mCellOuter.mChildList.childList || mCellOuter.mChildList2.childList) {
      if (IS_TABLE_CELL(mLowestType)) {
        printf("LOW OuterCell\n");
      }
      else {
        printf("    OuterCell\n");
      }
      mCellOuter.Dump();
    }
    if (mCellInner.mFrame || mCellInner.mChildList.childList || mCellInner.mChildList2.childList) {
      printf("    InnerCell\n");
      mCellInner.Dump();
    }
  }
}
#endif




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

  
  
  
  
  
  PRPackedBool              mFixedPosIsAbsPos;

  
  
  
  PRPackedBool              mHavePendingPopupgroup;

  nsCOMPtr<nsILayoutHistoryState> mFrameState;
  nsPseudoFrames            mPseudoFrames;
  
  
  nsFrameState              mAdditionalStateBits; 

  
  
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

protected:
  friend class nsFrameConstructorSaveState;

  



  void ProcessFrameInsertions(nsAbsoluteItems& aFrameItems,
                              nsIAtom* aChildListName);
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
    
    mFixedPosIsAbsPos(aAbsoluteContainingBlock &&
                      aAbsoluteContainingBlock->GetStyleDisplay()->
                        HasTransform()),
    mHavePendingPopupgroup(PR_FALSE),
    mFrameState(aHistoryState),
    mPseudoFrames(),
    mAdditionalStateBits(0)
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
    
    mFixedPosIsAbsPos(aAbsoluteContainingBlock &&
                      aAbsoluteContainingBlock->GetStyleDisplay()->
                        HasTransform()),
    mHavePendingPopupgroup(PR_FALSE),
    mPseudoFrames(),
    mAdditionalStateBits(0)
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
  ProcessFrameInsertions(mFloatedItems, nsGkAtoms::floatList);
  ProcessFrameInsertions(mAbsoluteItems, nsGkAtoms::absoluteList);
  ProcessFrameInsertions(mFixedItems, nsGkAtoms::fixedList);
#ifdef MOZ_XUL
  ProcessFrameInsertions(mPopupItems, nsGkAtoms::popupList);
#endif
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
  const nsStyleDisplay* disp = aNewFrame->GetStyleDisplay();
  
  
  

  PRBool needPlaceholder = PR_FALSE;
  nsFrameItems* frameItems = &aFrameItems;
#ifdef MOZ_XUL
  if (NS_UNLIKELY(aIsOutOfFlowPopup)) {
      NS_ASSERTION(aNewFrame->GetParent() == mPopupItems.containingBlock,
                   "Popup whose parent is not the popup containing block?");
      NS_ASSERTION(mPopupItems.containingBlock, "Must have a popup set frame!");
      needPlaceholder = PR_TRUE;
      frameItems = &mPopupItems;
  }
  else
#endif 
  if (aCanBeFloated && disp->IsFloating() &&
      mFloatedItems.containingBlock) {
    NS_ASSERTION(aNewFrame->GetParent() == mFloatedItems.containingBlock,
                 "Float whose parent is not the float containing block?");
    needPlaceholder = PR_TRUE;
    frameItems = &mFloatedItems;
  }
  else if (aCanBePositioned) {
    if (disp->mPosition == NS_STYLE_POSITION_ABSOLUTE &&
        mAbsoluteItems.containingBlock) {
      NS_ASSERTION(aNewFrame->GetParent() == mAbsoluteItems.containingBlock,
                   "Abs pos whose parent is not the abs pos containing block?");
      needPlaceholder = PR_TRUE;
      frameItems = &mAbsoluteItems;
    }
    if (disp->mPosition == NS_STYLE_POSITION_FIXED &&
        GetFixedItems().containingBlock) {
      NS_ASSERTION(aNewFrame->GetParent() == GetFixedItems().containingBlock,
                   "Fixed pos whose parent is not the fixed pos containing block?");
      needPlaceholder = PR_TRUE;
      frameItems = &GetFixedItems();
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
                                                       &placeholderFrame);
    if (NS_FAILED(rv)) {
      
      
      
      
      CleanupFrameReferences(mFrameManager, aNewFrame);
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
    frameItems->InsertChildAfter(aNewFrame, aInsertAfterFrame);
  } else {
    frameItems->AddChild(aNewFrame);
  }

  
  nsIFrame* specialSibling = aNewFrame;
  while (specialSibling && IsFrameSpecial(specialSibling)) {
    specialSibling = GetSpecialSibling(specialSibling);
    if (specialSibling) {
      NS_ASSERTION(frameItems == &aFrameItems,
                   "IB split ending up in an out-of-flow childlist?");
      frameItems->AddChild(specialSibling);
    }
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

  nsIFrame* firstNewFrame = aFrameItems.childList;
  
  if (!firstNewFrame) {
    return;
  }
  
  nsIFrame* containingBlock = aFrameItems.containingBlock;

  NS_ASSERTION(containingBlock,
               "Child list without containing block?");
  
  
  
  
  nsIFrame* firstChild = containingBlock->GetFirstChild(aChildListName);
  nsresult rv = NS_OK;
  if (!firstChild && (containingBlock->GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
    rv = containingBlock->SetInitialChildList(aChildListName, firstNewFrame);
  } else {
    
    
    
    
    
    
    
    nsIFrame* lastChild = nsLayoutUtils::GetLastSibling(firstChild);

    
    
    
    if (!lastChild ||
        nsLayoutUtils::CompareTreePosition(lastChild, firstNewFrame, containingBlock) < 0) {
      
      rv = containingBlock->AppendFrames(aChildListName, firstNewFrame);
    } else {
      nsIFrame* insertionPoint = nsnull;
      
      for (nsIFrame* f = firstChild; f != lastChild; f = f->GetNextSibling()) {
        PRInt32 compare =
          nsLayoutUtils::CompareTreePosition(f, firstNewFrame, containingBlock);
        if (compare > 0) {
          
          
          break;
        }
        insertionPoint = f;
      }

      rv = containingBlock->InsertFrames(aChildListName, insertionPoint,
                                         firstNewFrame);
    }
  }
  aFrameItems.childList = nsnull;
  
  
  
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
    
    
    mSavedItems.childList = nsnull;
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
AdjustFloatParentPtrs(nsIFrame*                aFrame,
                      nsFrameConstructorState& aState,
                      nsFrameConstructorState& aOuterState)
{
  NS_PRECONDITION(aFrame, "must have frame to work with");

  nsIFrame *outOfFlowFrame = nsPlaceholderFrame::GetRealFrameFor(aFrame);
  if (outOfFlowFrame != aFrame) {
    if (outOfFlowFrame->GetStyleDisplay()->IsFloating()) {
      
      
      
      
      nsIFrame *parent = aState.mFloatedItems.containingBlock;
      NS_ASSERTION(parent, "Should have float containing block here!");
      NS_ASSERTION(outOfFlowFrame->GetParent() == aOuterState.mFloatedItems.containingBlock,
                   "expected the float to be a child of the outer CB");

      if (aOuterState.mFloatedItems.RemoveChild(outOfFlowFrame, nsnull)) {
        aState.mFloatedItems.AddChild(outOfFlowFrame);
      } else {
        NS_NOTREACHED("float wasn't in the outer state float list");
      }

      outOfFlowFrame->SetParent(parent);
      if (outOfFlowFrame->GetStateBits() &
          (NS_FRAME_HAS_VIEW | NS_FRAME_HAS_CHILD_WITH_VIEW)) {
        
        
        parent->AddStateBits(NS_FRAME_HAS_CHILD_WITH_VIEW);
      }
    }

    
    
    return;
  }

  if (aFrame->IsFloatContainingBlock()) {
    
    
    return;
  }

  
  
  nsIFrame *childFrame = aFrame->GetFirstChild(nsnull);
  while (childFrame) {
    

    AdjustFloatParentPtrs(childFrame, aState, aOuterState);
    childFrame = childFrame->GetNextSibling();
  }
}






static void
MoveChildrenTo(nsFrameManager*          aFrameManager,
               nsIFrame*                aNewParent,
               nsIFrame*                aFrameList,
               nsIFrame*                aFrameListEnd,
               nsFrameConstructorState* aState,
               nsFrameConstructorState* aOuterState)
{
  PRBool setHasChildWithView = PR_FALSE;

  while (aFrameList && aFrameList != aFrameListEnd) {
    if (!setHasChildWithView
        && (aFrameList->GetStateBits() & (NS_FRAME_HAS_VIEW | NS_FRAME_HAS_CHILD_WITH_VIEW))) {
      setHasChildWithView = PR_TRUE;
    }

    aFrameList->SetParent(aNewParent);

    
    
    
    if (aState) {
      NS_ASSERTION(aOuterState, "need an outer state too");
      AdjustFloatParentPtrs(aFrameList, *aState, *aOuterState);
    }

    aFrameList = aFrameList->GetNextSibling();
  }

  if (setHasChildWithView) {
    do {
      aNewParent->AddStateBits(NS_FRAME_HAS_CHILD_WITH_VIEW);
      aNewParent = aNewParent->GetParent();
    } while (aNewParent &&
             !(aNewParent->GetStateBits() & NS_FRAME_HAS_CHILD_WITH_VIEW));
  }
}






struct NS_STACK_CLASS nsAutoEnqueueBinding
{
  nsAutoEnqueueBinding(nsIDocument* aDocument) :
    mDocument(aDocument)
  {}

  ~nsAutoEnqueueBinding();

  nsRefPtr<nsXBLBinding> mBinding;
private:
  nsIDocument* mDocument;
};

nsAutoEnqueueBinding::~nsAutoEnqueueBinding()
{
  if (mBinding) {
    mDocument->BindingManager()->AddToAttachedQueue(mBinding);
  }
}




static nsIAtom*
GetChildListNameFor(nsIFrame*       aChildFrame)
{
  nsIAtom*      listName;

  if (aChildFrame->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER) {
    listName = nsGkAtoms::overflowContainersList;
  }
  
  else if (aChildFrame->GetStateBits() & NS_FRAME_OUT_OF_FLOW) {
    
    const nsStyleDisplay* disp = aChildFrame->GetStyleDisplay();
    
    if (NS_STYLE_POSITION_ABSOLUTE == disp->mPosition) {
      listName = nsGkAtoms::absoluteList;
    } else if (NS_STYLE_POSITION_FIXED == disp->mPosition) {
      if (nsLayoutUtils::IsReallyFixedPos(aChildFrame)) {
        listName = nsGkAtoms::fixedList;
      } else {
        listName = nsGkAtoms::absoluteList;
      }
#ifdef MOZ_XUL
    } else if (NS_STYLE_DISPLAY_POPUP == disp->mDisplay) {
      
#ifdef DEBUG
      nsIFrame* parent = aChildFrame->GetParent();
      NS_ASSERTION(parent && parent->GetType() == nsGkAtoms::popupSetFrame,
                   "Unexpected parent");
#endif 

      
      
      
      
      return nsGkAtoms::popupList;      
#endif
    } else {
      NS_ASSERTION(aChildFrame->GetStyleDisplay()->IsFloating(),
                   "not a floated frame");
      listName = nsGkAtoms::floatList;
    }

  } else {
    listName = nsnull;
  }

#ifdef NS_DEBUG
  
  
  nsIFrame* parent = aChildFrame->GetParent();
  PRBool found = nsFrameList(parent->GetFirstChild(listName))
                   .ContainsFrame(aChildFrame);
  if (!found) {
    if (!(aChildFrame->GetStateBits() & NS_FRAME_OUT_OF_FLOW)) {
      found = nsFrameList(parent->GetFirstChild(nsGkAtoms::overflowList))
                .ContainsFrame(aChildFrame);
    }
    else if (aChildFrame->GetStyleDisplay()->IsFloating()) {
      found = nsFrameList(parent->GetFirstChild(nsGkAtoms::overflowOutOfFlowList))
                .ContainsFrame(aChildFrame);
    }
    
    NS_POSTCONDITION(found, "not in child list");
  }
#endif

  return listName;
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
  , mFocusSuppressCount(0)
  , mQuotesDirty(PR_FALSE)
  , mCountersDirty(PR_FALSE)
  , mIsDestroyingFrameTree(PR_FALSE)
  , mRebuildAllStyleData(PR_FALSE)
  , mHasRootAbsPosContainingBlock(PR_FALSE)
  , mHoverGeneration(0)
  , mRebuildAllExtraHint(nsChangeHint(0))
{
  
  if (!mPendingRestyles.Init()) {
    
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
nsCSSFrameConstructor::CreateGenConTextNode(const nsString& aString,
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
  }
  return content.forget();
}

already_AddRefed<nsIContent>
nsCSSFrameConstructor::CreateGeneratedContent(nsIContent*     aParentContent,
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
    NS_NewGenConImageContent(getter_AddRefs(content), nodeInfo,
                             data.mContent.mImage);
    return content.forget();
  }

  switch (type) {
  case eStyleContentType_String:
    return CreateGenConTextNode(nsDependentString(data.mContent.mString), nsnull,
                                nsnull);

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
          attrName = do_GetAtom(contentString);
        }
      }
      else {
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
      return CreateGenConTextNode(EmptyString(), &node->mText, initializer);
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
      return CreateGenConTextNode(EmptyString(), &node->mText, initializer);
    }
  
  case eStyleContentType_AltContent:
    {
      
      
      
      
      
      if (aParentContent->HasAttr(kNameSpaceID_None, nsGkAtoms::alt)) {
        nsCOMPtr<nsIContent> content;
        NS_NewAttributeContent(mDocument->NodeInfoManager(),
                               kNameSpaceID_None, nsGkAtoms::alt, getter_AddRefs(content));
        return content.forget();
      }

      if (aParentContent->IsNodeOfType(nsINode::eHTML) &&
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
        return CreateGenConTextNode(temp, nsnull, nsnull);
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
                                                  nsIAtom*         aPseudoElement,
                                                  FrameConstructionItemList& aItems)
{
  
  if (!aParentContent->IsNodeOfType(nsINode::eELEMENT))
    return;

  nsStyleSet *styleSet = mPresShell->StyleSet();

  
  nsRefPtr<nsStyleContext> pseudoStyleContext;
  pseudoStyleContext = styleSet->ProbePseudoStyleFor(aParentContent,
                                                     aPseudoElement,
                                                     aStyleContext);
  if (!pseudoStyleContext)
    return;
  
  
  nsCOMPtr<nsINodeInfo> nodeInfo;
  nsIAtom* elemName = aPseudoElement == nsCSSPseudoElements::before ?
    nsGkAtoms::mozgeneratedcontentbefore : nsGkAtoms::mozgeneratedcontentafter;
  nodeInfo = mDocument->NodeInfoManager()->GetNodeInfo(elemName, nsnull,
                                                       kNameSpaceID_None);
  nsCOMPtr<nsIContent> container;
  nsresult rv = NS_NewXMLElement(getter_AddRefs(container), nodeInfo);
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
      CreateGeneratedContent(aParentContent, pseudoStyleContext, contentIndex);
    if (content) {
      container->AppendChildTo(content, PR_FALSE);
    }
  }

  AddFrameConstructionItemsInternal(aState, container, aParentFrame, elemName,
                                    kNameSpaceID_None, pseudoStyleContext,
                                    ITEM_IS_GENERATED_CONTENT, aItems);
}

static PRBool
TextIsOnlyWhitespace(nsIContent* aContent)
{
  return aContent->IsNodeOfType(nsINode::eTEXT) &&
         aContent->TextIsOnlyWhitespace();
}
    












static PRBool
IsTableRelated(nsIAtom* aParentType,
               PRBool   aIncludeSpecial)
{
  if ((nsGkAtoms::tableFrame         == aParentType)  ||
      (nsGkAtoms::tableRowGroupFrame == aParentType)  ||
      (nsGkAtoms::tableRowFrame      == aParentType)) {
    return PR_TRUE;
  }
  else if (aIncludeSpecial && 
           ((nsGkAtoms::tableCaptionFrame  == aParentType)  ||
            (nsGkAtoms::tableColGroupFrame == aParentType)  ||
            (nsGkAtoms::tableColFrame      == aParentType)  ||
            IS_TABLE_CELL(aParentType))) {
    return PR_TRUE;
  }
  else return PR_FALSE;
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
   
static nsresult 
ProcessPseudoFrame(nsPseudoFrameData& aPseudoData,
                   nsIFrame*&         aParent)
{
  nsresult rv = NS_OK;

  aParent = aPseudoData.mFrame;
  nsFrameItems* items = &aPseudoData.mChildList;
  if (items && items->childList) {
    rv = aParent->SetInitialChildList(nsnull, items->childList);
    if (NS_FAILED(rv)) return rv;
  }
  aPseudoData.Reset();
  return rv;
}

static nsresult 
ProcessPseudoRowGroupFrame(nsPseudoFrameData& aPseudoData,
                           nsIFrame*&         aParent)
{
  nsresult rv = NS_OK;

  aParent = aPseudoData.mFrame;
  nsFrameItems* items = &aPseudoData.mChildList;
  if (items && items->childList) {
    nsTableRowGroupFrame* rgFrame = nsTableFrame::GetRowGroupFrame(aParent);
    rv = rgFrame->SetInitialChildList(nsnull, items->childList);
    if (NS_FAILED(rv)) return rv;
  }
  aPseudoData.Reset();
  return rv;
}

static nsresult 
ProcessPseudoTableFrame(nsPseudoFrames& aPseudoFrames,
                        nsIFrame*&      aParent)
{
  nsresult rv = NS_OK;

  
  if (aPseudoFrames.mColGroup.mFrame) {
    rv = ProcessPseudoFrame(aPseudoFrames.mColGroup, aParent);
  }

  
  rv = ProcessPseudoFrame(aPseudoFrames.mTableInner, aParent);

  
  aParent = aPseudoFrames.mTableOuter.mFrame;
  nsFrameItems* items = &aPseudoFrames.mTableOuter.mChildList;
  if (items && items->childList) {
    rv = aParent->SetInitialChildList(nsnull, items->childList);
    if (NS_FAILED(rv)) return rv;
  }
  nsFrameItems* captions = &aPseudoFrames.mTableOuter.mChildList2;
  if (captions && captions->childList) {
    rv = aParent->SetInitialChildList(nsGkAtoms::captionList, captions->childList);
  }
  aPseudoFrames.mTableOuter.Reset();
  return rv;
}

static nsresult 
ProcessPseudoCellFrame(nsPseudoFrames& aPseudoFrames,
                       nsIFrame*&      aParent)
{
  nsresult rv = NS_OK;

  rv = ProcessPseudoFrame(aPseudoFrames.mCellInner, aParent);
  if (NS_FAILED(rv)) return rv;
  rv = ProcessPseudoFrame(aPseudoFrames.mCellOuter, aParent);
  return rv;
}



static nsresult 
ProcessPseudoFrames(nsFrameConstructorState& aState,
                    nsIAtom*        aHighestType,
                    nsIFrame*&      aHighestFrame)
{
  nsresult rv = NS_OK;

  aHighestFrame = nsnull;

#ifdef DEBUG
  if (gTablePseudoFrame) {
    printf("*** ProcessPseudoFrames enter***\n");
    aState.mPseudoFrames.Dump();
  }
#endif

  nsPseudoFrames& pseudoFrames = aState.mPseudoFrames;

  if (nsGkAtoms::tableFrame == pseudoFrames.mLowestType) {
    if (pseudoFrames.mColGroup.mFrame) {
      rv = ProcessPseudoFrame(pseudoFrames.mColGroup, aHighestFrame);
      if (nsGkAtoms::tableColGroupFrame == aHighestType) return rv;
    }
    rv = ProcessPseudoTableFrame(pseudoFrames, aHighestFrame);
    if (nsGkAtoms::tableOuterFrame == aHighestType) return rv;
    
    if (pseudoFrames.mCellOuter.mFrame) {
      rv = ProcessPseudoCellFrame(pseudoFrames, aHighestFrame);
      if (IS_TABLE_CELL(aHighestType)) return rv;
    }
    if (pseudoFrames.mRow.mFrame) {
      rv = ProcessPseudoFrame(pseudoFrames.mRow, aHighestFrame);
      if (nsGkAtoms::tableRowFrame == aHighestType) return rv;
    }
    if (pseudoFrames.mRowGroup.mFrame) {
      rv = ProcessPseudoRowGroupFrame(pseudoFrames.mRowGroup, aHighestFrame);
      if (nsGkAtoms::tableRowGroupFrame == aHighestType) return rv;
    }
  }
  else if (nsGkAtoms::tableRowGroupFrame == pseudoFrames.mLowestType) {
    rv = ProcessPseudoRowGroupFrame(pseudoFrames.mRowGroup, aHighestFrame);
    if (nsGkAtoms::tableRowGroupFrame == aHighestType) return rv;
    if (pseudoFrames.mColGroup.mFrame) {
      nsIFrame* colGroupHigh;
      rv = ProcessPseudoFrame(pseudoFrames.mColGroup, colGroupHigh);
      if (aHighestFrame &&
          nsGkAtoms::tableRowGroupFrame == aHighestFrame->GetType() &&
          !pseudoFrames.mTableInner.mFrame) {
        
        
        
        
        colGroupHigh->SetNextSibling(aHighestFrame); 
      }
      aHighestFrame = colGroupHigh;
      if (nsGkAtoms::tableColGroupFrame == aHighestType) return rv;
    }
    if (pseudoFrames.mTableOuter.mFrame) {
      rv = ProcessPseudoTableFrame(pseudoFrames, aHighestFrame);
      if (nsGkAtoms::tableOuterFrame == aHighestType) return rv;
    }
    if (pseudoFrames.mCellOuter.mFrame) {
      rv = ProcessPseudoCellFrame(pseudoFrames, aHighestFrame);
      if (IS_TABLE_CELL(aHighestType)) return rv;
    }
    if (pseudoFrames.mRow.mFrame) {
      rv = ProcessPseudoFrame(pseudoFrames.mRow, aHighestFrame);
      if (nsGkAtoms::tableRowFrame == aHighestType) return rv;
    }
  }
  else if (nsGkAtoms::tableRowFrame == pseudoFrames.mLowestType) {
    rv = ProcessPseudoFrame(pseudoFrames.mRow, aHighestFrame);
    if (nsGkAtoms::tableRowFrame == aHighestType) return rv;

    if (pseudoFrames.mRowGroup.mFrame) {
      rv = ProcessPseudoRowGroupFrame(pseudoFrames.mRowGroup, aHighestFrame);
      if (nsGkAtoms::tableRowGroupFrame == aHighestType) return rv;
    }
    if (pseudoFrames.mColGroup.mFrame) {
      nsIFrame* colGroupHigh;
      rv = ProcessPseudoFrame(pseudoFrames.mColGroup, colGroupHigh);
      if (aHighestFrame &&
          nsGkAtoms::tableRowGroupFrame == aHighestFrame->GetType() &&
          !pseudoFrames.mTableInner.mFrame) {
        
        
        
        
        colGroupHigh->SetNextSibling(aHighestFrame); 
      }
      aHighestFrame = colGroupHigh;
      if (nsGkAtoms::tableColGroupFrame == aHighestType) return rv;
    }
    if (pseudoFrames.mTableOuter.mFrame) {
      rv = ProcessPseudoTableFrame(pseudoFrames, aHighestFrame);
      if (nsGkAtoms::tableOuterFrame == aHighestType) return rv;
    }
    if (pseudoFrames.mCellOuter.mFrame) {
      rv = ProcessPseudoCellFrame(pseudoFrames, aHighestFrame);
      if (IS_TABLE_CELL(aHighestType)) return rv;
    }
  }
  else if (IS_TABLE_CELL(pseudoFrames.mLowestType)) {
    rv = ProcessPseudoCellFrame(pseudoFrames, aHighestFrame);
    if (IS_TABLE_CELL(aHighestType)) return rv;

    if (pseudoFrames.mRow.mFrame) {
      rv = ProcessPseudoFrame(pseudoFrames.mRow, aHighestFrame);
      if (nsGkAtoms::tableRowFrame == aHighestType) return rv;
    }
    if (pseudoFrames.mRowGroup.mFrame) {
      rv = ProcessPseudoRowGroupFrame(pseudoFrames.mRowGroup, aHighestFrame);
      if (nsGkAtoms::tableRowGroupFrame == aHighestType) return rv;
    }
    if (pseudoFrames.mColGroup.mFrame) {
      nsIFrame* colGroupHigh;
      rv = ProcessPseudoFrame(pseudoFrames.mColGroup, colGroupHigh);
      if (aHighestFrame &&
          nsGkAtoms::tableRowGroupFrame == aHighestFrame->GetType() &&
          !pseudoFrames.mTableInner.mFrame) {
        
        
        
        
        colGroupHigh->SetNextSibling(aHighestFrame); 
      }
      aHighestFrame = colGroupHigh;
      if (nsGkAtoms::tableColGroupFrame == aHighestType) return rv;
    }
    if (pseudoFrames.mTableOuter.mFrame) {
      rv = ProcessPseudoTableFrame(pseudoFrames, aHighestFrame);
    }
  }
  else if (pseudoFrames.mColGroup.mFrame) { 
    
    rv = ProcessPseudoFrame(pseudoFrames.mColGroup, aHighestFrame);
  }

  return rv;
}

static nsresult 
ProcessPseudoFrames(nsFrameConstructorState& aState,
                    nsFrameItems&   aItems)
{

#ifdef DEBUG
  if (gTablePseudoFrame) {
    printf("*** ProcessPseudoFrames complete enter***\n");
    aState.mPseudoFrames.Dump();
  }
#endif
 
  nsIFrame* highestFrame;
  nsresult rv = ProcessPseudoFrames(aState, nsnull, highestFrame);
  if (highestFrame) {
    aItems.AddChild(highestFrame);
  }
 
#ifdef DEBUG
  if (gTablePseudoFrame) {
    printf("*** ProcessPseudoFrames complete leave, highestframe:%p***\n",
           static_cast<void*>(highestFrame));
    aState.mPseudoFrames.Dump();
  }
#endif
  aState.mPseudoFrames.Reset();
  return rv;
}

static nsresult 
ProcessPseudoFrames(nsFrameConstructorState& aState,
                    nsIAtom*        aHighestType)
{
#ifdef DEBUG
  if (gTablePseudoFrame) {
    printf("*** ProcessPseudoFrames limited enter highest:");
    if (nsGkAtoms::tableOuterFrame == aHighestType) 
      printf("OuterTable");
    else if (nsGkAtoms::tableFrame == aHighestType) 
      printf("InnerTable");
    else if (nsGkAtoms::tableColGroupFrame == aHighestType) 
      printf("ColGroup");
    else if (nsGkAtoms::tableRowGroupFrame == aHighestType) 
      printf("RowGroup");
    else if (nsGkAtoms::tableRowFrame == aHighestType) 
      printf("Row");
    else if (IS_TABLE_CELL(aHighestType)) 
      printf("Cell");
    else 
      NS_ASSERTION(PR_FALSE, "invalid call to ProcessPseudoFrames ");
    printf("***\n");
    aState.mPseudoFrames.Dump();
  }
#endif
 
  nsIFrame* highestFrame;
  nsresult rv = ProcessPseudoFrames(aState, aHighestType, highestFrame);

#ifdef DEBUG
  if (gTablePseudoFrame) {
    printf("*** ProcessPseudoFrames limited leave:%p***\n",
           static_cast<void*>(highestFrame));
    aState.mPseudoFrames.Dump();
  }
#endif
  return rv;
}

nsresult
nsCSSFrameConstructor::CreatePseudoTableFrame(PRInt32                  aNameSpaceID,
                                              nsFrameConstructorState& aState, 
                                              nsIFrame*                aParentFrameIn)
{
  nsresult rv = NS_OK;

  nsIFrame* parentFrame = (aState.mPseudoFrames.mCellInner.mFrame) 
                          ? aState.mPseudoFrames.mCellInner.mFrame : aParentFrameIn;
  if (!parentFrame) return rv;

  nsStyleContext *parentStyle;
  nsRefPtr<nsStyleContext> childStyle;

  parentStyle = parentFrame->GetStyleContext(); 
  nsIContent* parentContent = parentFrame->GetContent();   

  
  
  nsIAtom *pseudoType;
  if (parentStyle->GetStyleDisplay()->mDisplay == NS_STYLE_DISPLAY_INLINE)
    pseudoType = nsCSSAnonBoxes::inlineTable;
  else
    pseudoType = nsCSSAnonBoxes::table;

  
  childStyle = mPresShell->StyleSet()->ResolvePseudoStyleFor(parentContent,
                                                             pseudoType,
                                                             parentStyle);

  nsPseudoFrameData& pseudoOuter = aState.mPseudoFrames.mTableOuter;
  nsPseudoFrameData& pseudoInner = aState.mPseudoFrames.mTableInner;

  
  nsFrameItems items;
  rv = ConstructTableFrame(aState, parentContent,
                           parentFrame, childStyle, aNameSpaceID,
                           PR_TRUE, items, pseudoOuter.mFrame, 
                           pseudoInner.mFrame);

  if (NS_FAILED(rv)) return rv;

  
  pseudoOuter.mChildList.AddChild(pseudoInner.mFrame);
  aState.mPseudoFrames.mLowestType = nsGkAtoms::tableFrame;

  
  if (aState.mPseudoFrames.mCellInner.mFrame) {
    aState.mPseudoFrames.mCellInner.mChildList.AddChild(pseudoOuter.mFrame);
  }
#ifdef DEBUG
  if (gTablePseudoFrame) {
     printf("*** CreatePseudoTableFrame ***\n");
    aState.mPseudoFrames.Dump();
  }
#endif
  return rv;
}

nsresult
nsCSSFrameConstructor::CreatePseudoRowGroupFrame(PRInt32                  aNameSpaceID,
                                                 nsFrameConstructorState& aState, 
                                                 nsIFrame*                aParentFrameIn)
{
  nsresult rv = NS_OK;

  nsIFrame* parentFrame = (aState.mPseudoFrames.mTableInner.mFrame) 
                          ? aState.mPseudoFrames.mTableInner.mFrame : aParentFrameIn;
  if (!parentFrame) return rv;

  nsStyleContext *parentStyle;
  nsRefPtr<nsStyleContext> childStyle;

  parentStyle = parentFrame->GetStyleContext();
  nsIContent* parentContent = parentFrame->GetContent();

  childStyle = mPresShell->StyleSet()->ResolvePseudoStyleFor(parentContent,
                                                             nsCSSAnonBoxes::tableRowGroup, 
                                                             parentStyle);

  nsPseudoFrameData& pseudo = aState.mPseudoFrames.mRowGroup;

  
  pseudo.mFrame = NS_NewTableRowGroupFrame(mPresShell, childStyle);
  if (NS_UNLIKELY(!pseudo.mFrame)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  InitAndRestoreFrame(aState, parentContent, parentFrame, nsnull,
                      pseudo.mFrame);
  nsHTMLContainerFrame::CreateViewForFrame(pseudo.mFrame, PR_FALSE);
  
  
  aState.mPseudoFrames.mLowestType = nsGkAtoms::tableRowGroupFrame;

  
  if (aState.mPseudoFrames.mTableInner.mFrame) {
    aState.mPseudoFrames.mTableInner.mChildList.AddChild(pseudo.mFrame);
  }
#ifdef DEBUG
  if (gTablePseudoFrame) {
     printf("*** CreatePseudoRowGroupFrame ***\n");
    aState.mPseudoFrames.Dump();
  }
#endif
  return rv;
}

nsresult 
nsCSSFrameConstructor::CreatePseudoColGroupFrame(PRInt32                  aNameSpaceID,
                                                 nsFrameConstructorState& aState, 
                                                 nsIFrame*                aParentFrameIn)
{
  nsresult rv = NS_OK;

  nsIFrame* parentFrame = (aState.mPseudoFrames.mTableInner.mFrame) 
                          ? aState.mPseudoFrames.mTableInner.mFrame : aParentFrameIn;
  if (!parentFrame) return rv;

  nsStyleContext *parentStyle;
  nsRefPtr<nsStyleContext> childStyle;

  parentStyle = parentFrame->GetStyleContext();
  nsIContent* parentContent = parentFrame->GetContent();

  childStyle = mPresShell->StyleSet()->ResolvePseudoStyleFor(parentContent,
                                                             nsCSSAnonBoxes::tableColGroup, 
                                                             parentStyle);

  nsPseudoFrameData& pseudo = aState.mPseudoFrames.mColGroup;

  
  pseudo.mFrame = NS_NewTableColGroupFrame(mPresShell, childStyle);
  if (NS_UNLIKELY(!pseudo.mFrame)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  InitAndRestoreFrame(aState, parentContent, parentFrame, nsnull,
                      pseudo.mFrame);

  if (NS_FAILED(rv)) return rv;
  ((nsTableColGroupFrame*)pseudo.mFrame)->SetColType(eColGroupAnonymousCol);

  
  
  

  
  if (aState.mPseudoFrames.mTableInner.mFrame) {
    aState.mPseudoFrames.mTableInner.mChildList.AddChild(pseudo.mFrame);
  }
#ifdef DEBUG
  if (gTablePseudoFrame) {
     printf("*** CreatePseudoColGroupFrame ***\n");
    aState.mPseudoFrames.Dump();
  }
#endif
  return rv;
}

nsresult
nsCSSFrameConstructor::CreatePseudoRowFrame(PRInt32                  aNameSpaceID,
                                            nsFrameConstructorState& aState, 
                                            nsIFrame*                aParentFrameIn)
{
  nsresult rv = NS_OK;

  nsIFrame* parentFrame = aParentFrameIn;
  if (aState.mPseudoFrames.mRowGroup.mFrame) {
    parentFrame = (nsIFrame*) nsTableFrame::GetRowGroupFrame(aState.mPseudoFrames.mRowGroup.mFrame);
  }
  if (!parentFrame) return rv;

  nsStyleContext *parentStyle;
  nsRefPtr<nsStyleContext> childStyle;

  parentStyle = parentFrame->GetStyleContext();
  nsIContent* parentContent = parentFrame->GetContent();

  childStyle = mPresShell->StyleSet()->ResolvePseudoStyleFor(parentContent,
                                                             nsCSSAnonBoxes::tableRow, 
                                                             parentStyle);

  nsPseudoFrameData& pseudo = aState.mPseudoFrames.mRow;

  
  nsFrameItems items;
  rv = ConstructTableRowFrame(aState, parentContent,
                              parentFrame, childStyle, aNameSpaceID,
                              PR_TRUE, items, pseudo.mFrame);
  if (NS_FAILED(rv)) return rv;

  aState.mPseudoFrames.mLowestType = nsGkAtoms::tableRowFrame;

  
  if (aState.mPseudoFrames.mRowGroup.mFrame) {
    aState.mPseudoFrames.mRowGroup.mChildList.AddChild(pseudo.mFrame);
  }
#ifdef DEBUG
  if (gTablePseudoFrame) {
     printf("*** CreatePseudoRowFrame ***\n");
    aState.mPseudoFrames.Dump();
  }
#endif
  return rv;
}

nsresult
nsCSSFrameConstructor::CreatePseudoCellFrame(PRInt32                  aNameSpaceID,
                                             nsFrameConstructorState& aState, 
                                             nsIFrame*                aParentFrameIn)
{
  nsresult rv = NS_OK;

  nsIFrame* parentFrame = (aState.mPseudoFrames.mRow.mFrame) 
                          ? aState.mPseudoFrames.mRow.mFrame : aParentFrameIn;
  if (!parentFrame) return rv;

  nsStyleContext *parentStyle;
  nsRefPtr<nsStyleContext> childStyle;

  parentStyle = parentFrame->GetStyleContext();
  nsIContent* parentContent = parentFrame->GetContent();

  childStyle = mPresShell->StyleSet()->ResolvePseudoStyleFor(parentContent,
                                                             nsCSSAnonBoxes::tableCell, 
                                                             parentStyle);

  nsPseudoFrameData& pseudoOuter = aState.mPseudoFrames.mCellOuter;
  nsPseudoFrameData& pseudoInner = aState.mPseudoFrames.mCellInner;

  
  nsFrameItems items;
  rv = ConstructTableCellFrame(aState, parentContent, parentFrame, childStyle,
                               aNameSpaceID, PR_TRUE, items,
                               pseudoOuter.mFrame, pseudoInner.mFrame);
  if (NS_FAILED(rv)) return rv;

  
  pseudoOuter.mChildList.AddChild(pseudoInner.mFrame);
  
  aState.mPseudoFrames.mLowestType = nsGkAtoms::tableCellFrame;

  
  if (aState.mPseudoFrames.mRow.mFrame) {
    aState.mPseudoFrames.mRow.mChildList.AddChild(pseudoOuter.mFrame);
  }
#ifdef DEBUG
  if (gTablePseudoFrame) {
     printf("*** CreatePseudoCellFrame ***\n");
    aState.mPseudoFrames.Dump();
  }
#endif
  return rv;
}


nsresult 
nsCSSFrameConstructor::GetPseudoTableFrame(PRInt32                  aNameSpaceID,
                                           nsFrameConstructorState& aState, 
                                           nsIFrame&                aParentFrameIn)
{
  nsresult rv = NS_OK;

  nsPseudoFrames& pseudoFrames = aState.mPseudoFrames;
  nsIAtom* parentFrameType = aParentFrameIn.GetType();

  if (pseudoFrames.IsEmpty()) {
    PRBool created = PR_FALSE;
    if (nsGkAtoms::tableRowGroupFrame == parentFrameType) { 
      rv = CreatePseudoRowFrame(aNameSpaceID, aState, &aParentFrameIn);
      if (NS_FAILED(rv)) return rv;
      created = PR_TRUE;
    }
    if (created || (nsGkAtoms::tableRowFrame == parentFrameType)) { 
      rv = CreatePseudoCellFrame(aNameSpaceID, aState, &aParentFrameIn);
      if (NS_FAILED(rv)) return rv;
    }
    rv = CreatePseudoTableFrame(aNameSpaceID, aState, &aParentFrameIn);
  }
  else {
    if (!pseudoFrames.mTableInner.mFrame) { 
      if (pseudoFrames.mRowGroup.mFrame && !(pseudoFrames.mRow.mFrame)) {
        rv = CreatePseudoRowFrame(aNameSpaceID, aState);
        if (NS_FAILED(rv)) return rv;
      }
      if (pseudoFrames.mRow.mFrame && !(pseudoFrames.mCellOuter.mFrame)) {
        rv = CreatePseudoCellFrame(aNameSpaceID, aState);
        if (NS_FAILED(rv)) return rv;
      }
      CreatePseudoTableFrame(aNameSpaceID, aState);
    }
  }
  return rv;
}


nsresult 
nsCSSFrameConstructor::GetPseudoColGroupFrame(PRInt32                  aNameSpaceID,
                                              nsFrameConstructorState& aState, 
                                              nsIFrame&                aParentFrameIn)
{
  nsresult rv = NS_OK;

  nsPseudoFrames& pseudoFrames = aState.mPseudoFrames;
  nsIAtom* parentFrameType = aParentFrameIn.GetType();

  if (pseudoFrames.IsEmpty()) {
    PRBool created = PR_FALSE;
    if (nsGkAtoms::tableRowGroupFrame == parentFrameType) {  
      rv = CreatePseudoRowFrame(aNameSpaceID, aState, &aParentFrameIn);
      created = PR_TRUE;
    }
    if (created || (nsGkAtoms::tableRowFrame == parentFrameType)) { 
      rv = CreatePseudoCellFrame(aNameSpaceID, aState, &aParentFrameIn);
      created = PR_TRUE;
    }
    if (created || IS_TABLE_CELL(parentFrameType) || 
        (nsGkAtoms::tableCaptionFrame == parentFrameType)  || 
        !IsTableRelated(parentFrameType, PR_TRUE)) { 
      rv = CreatePseudoTableFrame(aNameSpaceID, aState, &aParentFrameIn);
    }
    rv = CreatePseudoColGroupFrame(aNameSpaceID, aState, &aParentFrameIn);
  }
  else {
    if (!pseudoFrames.mColGroup.mFrame) {
      if (!pseudoFrames.mTableInner.mFrame) {
        if (pseudoFrames.mRowGroup.mFrame && !(pseudoFrames.mRow.mFrame)) {
          rv = CreatePseudoRowFrame(aNameSpaceID, aState);
        }
        if (pseudoFrames.mRow.mFrame && !(pseudoFrames.mCellOuter.mFrame)) {
          rv = CreatePseudoCellFrame(aNameSpaceID, aState);
        }
        if (pseudoFrames.mCellOuter.mFrame && !(pseudoFrames.mTableOuter.mFrame)) {
          rv = CreatePseudoTableFrame(aNameSpaceID, aState);
        }
      }
      rv = CreatePseudoColGroupFrame(aNameSpaceID, aState);
    }
  }
  return rv;
}


nsresult 
nsCSSFrameConstructor::GetPseudoRowGroupFrame(PRInt32                  aNameSpaceID,
                                              nsFrameConstructorState& aState, 
                                              nsIFrame&                aParentFrameIn)
{
  nsresult rv = NS_OK;

  nsPseudoFrames& pseudoFrames = aState.mPseudoFrames;
  nsIAtom* parentFrameType = aParentFrameIn.GetType();

  if (!pseudoFrames.mLowestType) {
    PRBool created = PR_FALSE;
    if (nsGkAtoms::tableRowFrame == parentFrameType) {  
      rv = CreatePseudoCellFrame(aNameSpaceID, aState, &aParentFrameIn);
      created = PR_TRUE;
    }
    if (created || IS_TABLE_CELL(parentFrameType) || 
        (nsGkAtoms::tableCaptionFrame == parentFrameType)  || 
        !IsTableRelated(parentFrameType, PR_TRUE)) { 
      rv = CreatePseudoTableFrame(aNameSpaceID, aState, &aParentFrameIn);
    }
    rv = CreatePseudoRowGroupFrame(aNameSpaceID, aState, &aParentFrameIn);
  }
  else {
    if (!pseudoFrames.mRowGroup.mFrame) { 
      if (pseudoFrames.mRow.mFrame && !(pseudoFrames.mCellOuter.mFrame)) {
        rv = CreatePseudoCellFrame(aNameSpaceID, aState);
      }
      if (pseudoFrames.mCellOuter.mFrame && !(pseudoFrames.mTableOuter.mFrame)) {
        rv = CreatePseudoTableFrame(aNameSpaceID, aState);
      }
      rv = CreatePseudoRowGroupFrame(aNameSpaceID, aState);
    }
  }
  return rv;
}


nsresult
nsCSSFrameConstructor::GetPseudoRowFrame(PRInt32                  aNameSpaceID,
                                         nsFrameConstructorState& aState, 
                                         nsIFrame&                aParentFrameIn)
{
  nsresult rv = NS_OK;

  nsPseudoFrames& pseudoFrames = aState.mPseudoFrames;
  nsIAtom* parentFrameType = aParentFrameIn.GetType();

  if (!pseudoFrames.mLowestType) {
    PRBool created = PR_FALSE;
    if (IS_TABLE_CELL(parentFrameType) || 
       (nsGkAtoms::tableCaptionFrame == parentFrameType)  || 
        !IsTableRelated(parentFrameType, PR_TRUE)) { 
      rv = CreatePseudoTableFrame(aNameSpaceID, aState, &aParentFrameIn);
      created = PR_TRUE;
    }
    if (created || (nsGkAtoms::tableFrame == parentFrameType)) { 
      rv = CreatePseudoRowGroupFrame(aNameSpaceID, aState, &aParentFrameIn);
    }
    rv = CreatePseudoRowFrame(aNameSpaceID, aState, &aParentFrameIn);
  }
  else {
    if (!pseudoFrames.mRow.mFrame) { 
      if (pseudoFrames.mCellOuter.mFrame && !pseudoFrames.mTableOuter.mFrame) {
        rv = CreatePseudoTableFrame(aNameSpaceID, aState);
      }
      if (pseudoFrames.mTableInner.mFrame && !(pseudoFrames.mRowGroup.mFrame)) {
        rv = CreatePseudoRowGroupFrame(aNameSpaceID, aState);
      }
      rv = CreatePseudoRowFrame(aNameSpaceID, aState);
    }
  }
  return rv;
}


nsresult 
nsCSSFrameConstructor::GetPseudoCellFrame(PRInt32                  aNameSpaceID,
                                          nsFrameConstructorState& aState, 
                                          nsIFrame&                aParentFrameIn)
{
  nsresult rv = NS_OK;

  nsPseudoFrames& pseudoFrames = aState.mPseudoFrames;
  nsIAtom* parentFrameType = aParentFrameIn.GetType();

  if (!pseudoFrames.mLowestType) {
    PRBool created = PR_FALSE;
    if (nsGkAtoms::tableFrame == parentFrameType) { 
      rv = CreatePseudoRowGroupFrame(aNameSpaceID, aState, &aParentFrameIn);
      created = PR_TRUE;
    }
    if (created || (nsGkAtoms::tableRowGroupFrame == parentFrameType)) { 
      rv = CreatePseudoRowFrame(aNameSpaceID, aState, &aParentFrameIn);
      created = PR_TRUE;
    }
    rv = CreatePseudoCellFrame(aNameSpaceID, aState, &aParentFrameIn);
  }
  else if (!pseudoFrames.mCellOuter.mFrame) { 
    if (pseudoFrames.mTableInner.mFrame && !(pseudoFrames.mRowGroup.mFrame)) {
      rv = CreatePseudoRowGroupFrame(aNameSpaceID, aState);
    }
    if (pseudoFrames.mRowGroup.mFrame && !(pseudoFrames.mRow.mFrame)) {
      rv = CreatePseudoRowFrame(aNameSpaceID, aState);
    }
    rv = CreatePseudoCellFrame(aNameSpaceID, aState);
  }
  return rv;
}

nsresult 
nsCSSFrameConstructor::CreateRequiredPseudoFrames(PRInt32                  aNameSpaceID,
                                                  nsIFrame&                aParentFrameIn,
                                                  nsIAtom*                 aChildFrameType,
                                                  nsFrameConstructorState& aState,
                                                  nsIFrame*&               aParentFrame,
                                                  PRBool&                  aIsPseudoParent)
{
  nsresult rv = NS_OK;

  nsIAtom* parentFrameType = aParentFrameIn.GetType();
  nsIFrame* pseudoParentFrame = nsnull;
  nsPseudoFrames& pseudoFrames = aState.mPseudoFrames;
  aParentFrame = &aParentFrameIn;
  aIsPseudoParent = PR_FALSE;

  nsFrameState savedStateBits  = aState.mAdditionalStateBits;
  aState.mAdditionalStateBits &= ~NS_FRAME_GENERATED_CONTENT;

  if (nsGkAtoms::tableCaptionFrame == aChildFrameType) { 
    if (nsGkAtoms::tableOuterFrame != parentFrameType) { 
      rv = GetPseudoTableFrame(aNameSpaceID, aState, aParentFrameIn);
      if (NS_FAILED(rv)) return rv;
      pseudoParentFrame = pseudoFrames.mTableOuter.mFrame;
    }
  }
  else if (nsGkAtoms::tableColGroupFrame == aChildFrameType) { 
    if (nsGkAtoms::tableFrame != parentFrameType) { 
      rv = GetPseudoTableFrame(aNameSpaceID, aState, aParentFrameIn);
      if (NS_FAILED(rv)) return rv;
      pseudoParentFrame = pseudoFrames.mTableInner.mFrame;
    }
  }
  else if (nsGkAtoms::tableColFrame == aChildFrameType) { 
    if (nsGkAtoms::tableColGroupFrame != parentFrameType) { 
      rv = GetPseudoColGroupFrame(aNameSpaceID, aState, aParentFrameIn);
      if (NS_FAILED(rv)) return rv;
      pseudoParentFrame = pseudoFrames.mColGroup.mFrame;
    }
  }
  else if (nsGkAtoms::tableRowGroupFrame == aChildFrameType) { 
    
    if (nsGkAtoms::tableFrame != parentFrameType) {
      
        rv = GetPseudoTableFrame(aNameSpaceID, aState, aParentFrameIn);
        if (NS_FAILED(rv)) return rv;
        pseudoParentFrame = pseudoFrames.mTableInner.mFrame;
     }
  }
  else if (nsGkAtoms::tableRowFrame == aChildFrameType) { 
    if (nsGkAtoms::tableRowGroupFrame != parentFrameType) { 
      rv = GetPseudoRowGroupFrame(aNameSpaceID, aState, aParentFrameIn);
      if (NS_FAILED(rv)) return rv;
      pseudoParentFrame = pseudoFrames.mRowGroup.mFrame;
    }
  }
  else if (IS_TABLE_CELL(aChildFrameType)) { 
    if (nsGkAtoms::tableRowFrame != parentFrameType) { 
      rv = GetPseudoRowFrame(aNameSpaceID, aState, aParentFrameIn);
      if (NS_FAILED(rv)) return rv;
      pseudoParentFrame = pseudoFrames.mRow.mFrame;
    }
  }
#ifdef DEBUG
  else {
    NS_ERROR("Unexpected frame type in CreateRequiredPseudoFrames");
  }
#endif
  
  if (pseudoParentFrame) {
    aParentFrame = pseudoParentFrame;
    aIsPseudoParent = PR_TRUE;
  }

  aState.mAdditionalStateBits = savedStateBits;
  return rv;
}

nsresult
nsCSSFrameConstructor::AdjustParentFrame(nsFrameConstructorState&     aState,
                                         nsIContent*                  aChildContent,
                                         nsIFrame* &                  aParentFrame,
                                         const FrameConstructionData* aFCData,
                                         PRInt32                      aNameSpaceID,
                                         nsStyleContext*              aStyleContext,
                                         nsFrameItems* &              aFrameItems,
                                         nsFrameConstructorSaveState& aSaveState,
                                         PRBool&                      aCreatedPseudo)
{
  NS_PRECONDITION(aStyleContext, "Must have child's style context");
  NS_PRECONDITION(aFrameItems, "Must have frame items to work with");
  NS_PRECONDITION(aFCData, "Must have frame construction data");

  aCreatedPseudo = PR_FALSE;
  if (!aParentFrame) {
    
    return NS_OK;
  }

  PRBool tablePart = ((aFCData->mBits & FCDATA_IS_TABLE_PART) != 0);

  nsIAtom* parentType = aParentFrame->GetType();
  NS_ASSERTION(parentType != nsGkAtoms::tableOuterFrame,
               "Shouldn't be happening");
 
  
  
  
  if (!tablePart) {
    if (IsTableRelated(parentType, PR_FALSE)) {
      nsFrameState savedStateBits  = aState.mAdditionalStateBits;
      aState.mAdditionalStateBits &= ~NS_FRAME_GENERATED_CONTENT;
      nsresult rv = GetPseudoCellFrame(aNameSpaceID, aState, *aParentFrame);
      if (NS_FAILED(rv)) {
        return rv;
      }
      aState.mAdditionalStateBits = savedStateBits;

      NS_ASSERTION(aState.mPseudoFrames.mCellInner.mFrame,
                   "Must have inner cell frame now!");

      aParentFrame = aState.mPseudoFrames.mCellInner.mFrame;
      aFrameItems = &aState.mPseudoFrames.mCellInner.mChildList;
      
      
      aState.PushFloatContainingBlock(aParentFrame, aSaveState);
      aCreatedPseudo = PR_TRUE;

      
      
      
      
      
      if (aState.mPseudoFrames.mTableOuter.mFrame) {
        ProcessPseudoFrames(aState, nsGkAtoms::tableOuterFrame);
      }
    }
  } else {
    
    
    switch (aStyleContext->GetStyleDisplay()->mDisplay) {
      case NS_STYLE_DISPLAY_TABLE_CAPTION:
        aParentFrame = AdjustCaptionParentFrame(aParentFrame);
        CreateRequiredPseudoFrames(aNameSpaceID, *aParentFrame,
                                   nsGkAtoms::tableCaptionFrame, aState,
                                   aParentFrame, aCreatedPseudo);
        if (aCreatedPseudo) {
          aFrameItems = &aState.mPseudoFrames.mTableOuter.mChildList2;
        }
        break;
      case NS_STYLE_DISPLAY_TABLE_ROW_GROUP:
      case NS_STYLE_DISPLAY_TABLE_HEADER_GROUP:
      case NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP:
        CreateRequiredPseudoFrames(aNameSpaceID, *aParentFrame,
                                   nsGkAtoms::tableRowGroupFrame, aState,
                                   aParentFrame, aCreatedPseudo);
        if (aCreatedPseudo) {
          if (aState.mPseudoFrames.mRowGroup.mFrame) {
            ProcessPseudoFrames(aState, nsGkAtoms::tableRowGroupFrame);
          }
          aFrameItems = &aState.mPseudoFrames.mTableInner.mChildList;
        }
        break;
      case NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP:
        CreateRequiredPseudoFrames(aNameSpaceID, *aParentFrame,
                                   nsGkAtoms::tableColGroupFrame, aState,
                                   aParentFrame, aCreatedPseudo);
        if (aCreatedPseudo) {
          if (aState.mPseudoFrames.mColGroup.mFrame) {
            ProcessPseudoFrames(aState, nsGkAtoms::tableColGroupFrame);
          }
          aFrameItems = &aState.mPseudoFrames.mTableInner.mChildList;
        }
        break;
      case NS_STYLE_DISPLAY_TABLE_COLUMN:
        CreateRequiredPseudoFrames(aNameSpaceID, *aParentFrame,
                                   nsGkAtoms::tableColFrame, aState,
                                   aParentFrame, aCreatedPseudo);
        if (aCreatedPseudo) {
          aFrameItems = &aState.mPseudoFrames.mColGroup.mChildList;
        }
        break;
      case NS_STYLE_DISPLAY_TABLE_ROW:
        CreateRequiredPseudoFrames(aNameSpaceID, *aParentFrame,
                                   nsGkAtoms::tableRowFrame, aState,
                                   aParentFrame, aCreatedPseudo);
        if (aCreatedPseudo) {
          if (aState.mPseudoFrames.mRow.mFrame) {
            ProcessPseudoFrames(aState, nsGkAtoms::tableRowFrame);
          }
          aFrameItems = &aState.mPseudoFrames.mRowGroup.mChildList;
        }
        break;
      case NS_STYLE_DISPLAY_TABLE_CELL:
        
        
        CreateRequiredPseudoFrames(aNameSpaceID, *aParentFrame,
                                   nsGkAtoms::tableCellFrame, aState,
                                   aParentFrame, aCreatedPseudo);
        if (aCreatedPseudo) {
          if (aState.mPseudoFrames.mCellOuter.mFrame) {
            ProcessPseudoFrames(aState, nsGkAtoms::tableCellFrame);
          }
          aFrameItems = &aState.mPseudoFrames.mRow.mChildList;
        }
        break;
    }
  }
  return NS_OK;
}


static void
PullOutCaptionFrames(nsFrameItems& aItems, nsFrameItems& aCaptions)
{
  nsIFrame *child = aItems.childList;
  nsIFrame* prev = nsnull;
  while (child) {
    nsIFrame *nextSibling = child->GetNextSibling();
    if (nsGkAtoms::tableCaptionFrame == child->GetType()) {
      aItems.RemoveChild(child, prev);
      aCaptions.AddChild(child);
    } else {
      prev = child;
    }
    child = nextSibling;
  }
}






nsresult
nsCSSFrameConstructor::ConstructTableFrame(nsFrameConstructorState& aState,
                                           nsIContent*              aContent,
                                           nsIFrame*                aContentParent,
                                           nsStyleContext*          aStyleContext,
                                           PRInt32                  aNameSpaceID,
                                           PRBool                   aIsPseudo,
                                           nsFrameItems&            aChildItems,
                                           nsIFrame*&               aNewOuterFrame,
                                           nsIFrame*&               aNewInnerFrame)
{
  nsresult rv = NS_OK;


  
  nsRefPtr<nsStyleContext> outerStyleContext;
  outerStyleContext = mPresShell->StyleSet()->
    ResolvePseudoStyleFor(aContent, nsCSSAnonBoxes::tableOuter, aStyleContext);

  
#ifdef MOZ_MATHML
  if (kNameSpaceID_MathML == aNameSpaceID)
    aNewOuterFrame = NS_NewMathMLmtableOuterFrame(mPresShell,
                                                  outerStyleContext);
  else
#endif
    aNewOuterFrame = NS_NewTableOuterFrame(mPresShell, outerStyleContext);

  NS_ASSERTION(!IsTableRelated(aContentParent->GetType(), PR_TRUE) ||
               aContentParent->GetType() == nsGkAtoms::tableCaptionFrame,
               "Unexpected parent frame for table");

  nsIFrame* geometricParent = aState.GetGeometricParent
                                (outerStyleContext->GetStyleDisplay(),
                                 aContentParent);

  
  
  InitAndRestoreFrame(aState, aContent, geometricParent, nsnull, aNewOuterFrame);  
  nsHTMLContainerFrame::CreateViewForFrame(aNewOuterFrame, PR_FALSE);

  
#ifdef MOZ_MATHML
  if (kNameSpaceID_MathML == aNameSpaceID)
    aNewInnerFrame = NS_NewMathMLmtableFrame(mPresShell, aStyleContext);
  else
#endif
    aNewInnerFrame = NS_NewTableFrame(mPresShell, aStyleContext);
 
  InitAndRestoreFrame(aState, aContent, aNewOuterFrame, nsnull,
                      aNewInnerFrame);

  if (!aIsPseudo) {
    
    aNewOuterFrame->SetInitialChildList(nsnull, aNewInnerFrame);

    rv = aState.AddChild(aNewOuterFrame, aChildItems, aContent,
                         aStyleContext, aContentParent);
    if (NS_FAILED(rv)) {
      return rv;
    }

    if (!mRootElementFrame) {
      
      
      mRootElementFrame = aNewOuterFrame;
    }

    nsFrameItems childItems;
    rv = ProcessChildren(aState, aContent, aStyleContext, aNewInnerFrame,
                         PR_TRUE, childItems, PR_FALSE);
    
    if (NS_FAILED(rv)) return rv;

    nsFrameItems captionItems;
    PullOutCaptionFrames(childItems, captionItems);

    
    aNewInnerFrame->SetInitialChildList(nsnull, childItems.childList);

    
    if (captionItems.childList) {
        aNewOuterFrame->SetInitialChildList(nsGkAtoms::captionList,
                                            captionItems.childList);
    }
 }

  return rv;
}

nsresult
nsCSSFrameConstructor::ConstructTableRowFrame(nsFrameConstructorState& aState,
                                              nsIContent*              aContent,
                                              nsIFrame*                aParentFrameIn,
                                              nsStyleContext*          aStyleContext,
                                              PRInt32                  aNameSpaceID,
                                              PRBool                   aIsPseudo,
                                              nsFrameItems&            aChildItems,
                                              nsIFrame*&               aNewFrame)
{
#ifdef MOZ_MATHML
  if (kNameSpaceID_MathML == aNameSpaceID)
    aNewFrame = NS_NewMathMLmtrFrame(mPresShell, aStyleContext);
  else
#endif
    aNewFrame = NS_NewTableRowFrame(mPresShell, aStyleContext);

  if (NS_UNLIKELY(!aNewFrame)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  InitAndRestoreFrame(aState, aContent, aParentFrameIn, nsnull, aNewFrame);
  nsHTMLContainerFrame::CreateViewForFrame(aNewFrame, PR_FALSE);
  if (!aIsPseudo) {
    nsFrameItems childItems;
    nsresult rv = ProcessChildren(aState, aContent, aStyleContext, aNewFrame,
                                  PR_TRUE, childItems, PR_FALSE);
    if (NS_FAILED(rv)) return rv;

    aNewFrame->SetInitialChildList(nsnull, childItems.childList);
    aChildItems.AddChild(aNewFrame);
  }

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
  if (NS_UNLIKELY(!aNewFrame)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  InitAndRestoreFrame(aState, content, aParentFrame, nsnull, colFrame);

  NS_ASSERTION(colFrame->GetStyleContext() == styleContext,
               "Unexpected style context");

  
  PRInt32 span = colFrame->GetSpan();
  nsIFrame* lastCol = colFrame;
  for (PRInt32 spanX = 1; spanX < span; spanX++) {
    nsTableColFrame* newCol = NS_NewTableColFrame(mPresShell, styleContext);
    if (NS_UNLIKELY(!newCol)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    InitAndRestoreFrame(aState, content, aParentFrame, nsnull, newCol,
                        PR_FALSE);
    lastCol->SetNextSibling(newCol);
    lastCol->SetNextContinuation(newCol);
    newCol->SetPrevContinuation(lastCol);
    newCol->SetColType(eColAnonymousCol);
    lastCol = newCol;
  }

  aFrameItems.AddChild(colFrame);
  *aNewFrame = colFrame;
  
  return NS_OK;
}

nsresult
nsCSSFrameConstructor::ConstructTableCellFrame(nsFrameConstructorState& aState,
                                               nsIContent*              aContent,
                                               nsIFrame*                aParentFrameIn,
                                               nsStyleContext*          aStyleContext,
                                               PRInt32                  aNameSpaceID,
                                               PRBool                   aIsPseudo,
                                               nsFrameItems&            aChildItems,
                                               nsIFrame*&               aNewCellOuterFrame,
                                               nsIFrame*&               aNewCellInnerFrame)
{

  nsIFrame* parentFrame = aParentFrameIn;
#ifdef MOZ_MATHML
  
  
  
  
  
  
  
  if (kNameSpaceID_MathML == aNameSpaceID && !IsBorderCollapse(parentFrame))
    aNewCellOuterFrame = NS_NewMathMLmtdFrame(mPresShell, aStyleContext);
  else
#endif
    
    
    
    aNewCellOuterFrame = NS_NewTableCellFrame(mPresShell, aStyleContext,
                                              IsBorderCollapse(parentFrame));

  if (NS_UNLIKELY(!aNewCellOuterFrame)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  InitAndRestoreFrame(aState, aContent, parentFrame, nsnull, aNewCellOuterFrame);
  nsHTMLContainerFrame::CreateViewForFrame(aNewCellOuterFrame, PR_FALSE);
  
  
  nsRefPtr<nsStyleContext> innerPseudoStyle;
  innerPseudoStyle = mPresShell->StyleSet()->
    ResolvePseudoStyleFor(aContent,
                          nsCSSAnonBoxes::cellContent, aStyleContext);

  
  PRBool isBlock;
#ifdef MOZ_MATHML
  if (kNameSpaceID_MathML == aNameSpaceID) {
    aNewCellInnerFrame = NS_NewMathMLmtdInnerFrame(mPresShell, innerPseudoStyle);
    isBlock = PR_FALSE;
  }
  else
#endif
  {
    aNewCellInnerFrame = NS_NewBlockFormattingContext(mPresShell,
                                                      innerPseudoStyle);
    isBlock = PR_TRUE;
  }


  if (NS_UNLIKELY(!aNewCellInnerFrame)) {
    aNewCellOuterFrame->Destroy();
    aNewCellOuterFrame = nsnull;
    return NS_ERROR_OUT_OF_MEMORY;
  }

  InitAndRestoreFrame(aState, aContent, aNewCellOuterFrame, nsnull, aNewCellInnerFrame);

  if (!aIsPseudo) {
    
    nsFrameItems childItems;
    nsresult rv = ProcessChildren(aState, aContent, aStyleContext,
                                  aNewCellInnerFrame, PR_TRUE, childItems, isBlock);

    if (NS_FAILED(rv)) {
      
      
      aNewCellInnerFrame->Destroy();
      aNewCellInnerFrame = nsnull;
      aNewCellOuterFrame->Destroy();
      aNewCellOuterFrame = nsnull;
      return rv;
    }

    aNewCellInnerFrame->SetInitialChildList(nsnull, childItems.childList);
    aNewCellOuterFrame->SetInitialChildList(nsnull, aNewCellInnerFrame);
    aChildItems.AddChild(aNewCellOuterFrame);
  }

  return NS_OK;
}

static PRBool 
NeedFrameFor(nsIFrame*   aParentFrame,
             nsIContent* aChildContent) 
{
  
  
  
  
  
  
  return !aParentFrame->IsFrameOfType(nsIFrame::eExcludesIgnorableWhitespace)
    || !TextIsOnlyWhitespace(aChildContent)
    || aParentFrame->IsGeneratedContentFrame();
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

  nsIContent* docElement = mDocument->GetRootContent();

  
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
  if (!htmlDoc || !docElement->IsNodeOfType(nsINode::eHTML)) {
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
  bodyStyle = styleSet->ResolveStyleFor(bodyElement, rootStyle);
  if (!bodyStyle) {
    return nsnull;
  }

  if (CheckOverflow(presContext, bodyStyle->GetStyleDisplay())) {
    
    return bodyElement;
  }

  return nsnull;
}




nsresult
nsCSSFrameConstructor::ConstructDocElementFrame(nsFrameConstructorState& aState,
                                                nsIContent*              aDocElement,
                                                nsIFrame*                aParentFrame,
                                                nsIFrame**               aNewFrame)
{
  *aNewFrame = nsnull;

  if (!mTempFrameTreeState)
    aState.mPresShell->CaptureHistoryState(getter_AddRefs(mTempFrameTreeState));

  
  
  
  
  if (mGfxScrollFrame) {
    nsIFrame* gfxScrollbarFrame1 = mGfxScrollFrame->GetFirstChild(nsnull);
    
    
    if (gfxScrollbarFrame1 &&
        gfxScrollbarFrame1->GetType() == nsGkAtoms::scrollbarFrame) {
      
      aState.mFrameManager->
        SetPrimaryFrameFor(gfxScrollbarFrame1->GetContent(), gfxScrollbarFrame1);

      nsIFrame* gfxScrollbarFrame2 = gfxScrollbarFrame1->GetNextSibling();
      if (gfxScrollbarFrame2 &&
          gfxScrollbarFrame2->GetType() == nsGkAtoms::scrollbarFrame) {
        
        aState.mFrameManager->
          SetPrimaryFrameFor(gfxScrollbarFrame2->GetContent(), gfxScrollbarFrame2);
      }
    }
  }

  
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
    rv = xblService->LoadBindings(aDocElement, display->mBinding->mURI,
                                  display->mBinding->mOriginPrincipal,
                                  PR_FALSE, getter_AddRefs(binding),
                                  &resolveStyle);
    if (NS_FAILED(rv))
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
  PRBool propagatedScrollToViewport =
    PropagateScrollToViewport() == aDocElement;

  NS_ASSERTION(!display->IsScrollableOverflow() || 
               aState.mPresContext->IsPaginated() ||
               propagatedScrollToViewport,
               "Scrollbars should have been propagated to the viewport");
#endif

  if (NS_UNLIKELY(display->mDisplay == NS_STYLE_DISPLAY_NONE)) {
    aState.mFrameManager->SetUndisplayedContent(aDocElement, styleContext);
    mRootElementFrame = nsnull;
    mRootElementStyleFrame = nsnull;
    return NS_OK;
  }

  nsFrameConstructorSaveState absoluteSaveState;
  if (mHasRootAbsPosContainingBlock) {
    
    
    aState.PushAbsoluteContainingBlock(mDocElementContainingBlock, absoluteSaveState);
  }

  nsresult rv;

  
  
  

  
  
  
  
  
  
  nsIFrame* contentFrame;
  PRBool processChildren = PR_FALSE;

  
#ifdef MOZ_XUL
  if (aDocElement->IsNodeOfType(nsINode::eXUL)) {
    contentFrame = NS_NewDocElementBoxFrame(mPresShell, styleContext);
    if (NS_UNLIKELY(!contentFrame)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    InitAndRestoreFrame(aState, aDocElement, aParentFrame, nsnull, contentFrame);
    *aNewFrame = contentFrame;
    processChildren = PR_TRUE;
  }
  else
#endif
#ifdef MOZ_SVG
  if (aDocElement->GetNameSpaceID() == kNameSpaceID_SVG) {
    if (aDocElement->Tag() == nsGkAtoms::svg && NS_SVGEnabled()) {
      contentFrame = NS_NewSVGOuterSVGFrame(mPresShell, styleContext);
      if (NS_UNLIKELY(!contentFrame)) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
      InitAndRestoreFrame(aState, aDocElement,
                          aState.GetGeometricParent(display, aParentFrame),
                          nsnull, contentFrame);

      
      
      nsFrameItems frameItems;
      rv = aState.AddChild(contentFrame, frameItems, aDocElement,
                           styleContext, aParentFrame);
      if (NS_FAILED(rv) || !frameItems.childList) {
        return rv;
      }
      *aNewFrame = frameItems.childList;
      processChildren = PR_TRUE;

      
      nsHTMLContainerFrame::CreateViewForFrame(contentFrame, PR_FALSE);
    } else {
      return NS_ERROR_FAILURE;
    }
  }
  else
#endif
  {
    PRBool docElemIsTable = (display->mDisplay == NS_STYLE_DISPLAY_TABLE);
    if (docElemIsTable) {
      nsIFrame* innerTableFrame;
      nsFrameItems frameItems;
      
      rv = ConstructTableFrame(aState, aDocElement,
                               aParentFrame, styleContext,
                               kNameSpaceID_None, PR_FALSE, frameItems,
                               contentFrame, innerTableFrame);
      if (NS_FAILED(rv))
        return rv;
      if (!contentFrame || !frameItems.childList)
        return NS_ERROR_FAILURE;
      *aNewFrame = frameItems.childList;
      NS_ASSERTION(!frameItems.childList->GetNextSibling(),
                   "multiple root element frames");
    } else {
      contentFrame = NS_NewBlockFrame(mPresShell, styleContext,
        NS_BLOCK_FLOAT_MGR|NS_BLOCK_MARGIN_ROOT);
      if (!contentFrame)
        return NS_ERROR_OUT_OF_MEMORY;
      nsFrameItems frameItems;
      rv = ConstructBlock(aState, display, aDocElement,
                          aState.GetGeometricParent(display, aParentFrame),
                          aParentFrame, styleContext, &contentFrame,
                          frameItems, display->IsPositioned());
      if (NS_FAILED(rv) || !frameItems.childList)
        return rv;
      *aNewFrame = frameItems.childList;
      NS_ASSERTION(!frameItems.childList->GetNextSibling(),
                   "multiple root element frames");
    }
  }

  
  aState.mFrameManager->SetPrimaryFrameFor(aDocElement, contentFrame);

  NS_ASSERTION(processChildren ? !mRootElementFrame :
                 mRootElementFrame == contentFrame,
               "unexpected mRootElementFrame");
  mRootElementFrame = contentFrame;

  
  
  
  PRBool isChild;
  contentFrame->GetParentStyleContextFrame(aState.mPresContext,
          &mRootElementStyleFrame, &isChild);
  if (!isChild) {
    mRootElementStyleFrame = mRootElementFrame;
  }

  if (processChildren) {
    
    nsFrameItems childItems;

    NS_ASSERTION(!nsLayoutUtils::GetAsBlock(contentFrame),
                 "Only XUL and SVG frames should reach here");
    ProcessChildren(aState, aDocElement, styleContext, contentFrame, PR_TRUE,
                    childItems, PR_FALSE);

    
    contentFrame->SetInitialChildList(nsnull, childItems.childList);
  }

  return NS_OK;
}


nsresult
nsCSSFrameConstructor::ConstructRootFrame(nsIContent*     aDocElement,
                                          nsIFrame**      aNewFrame)
{
  AUTO_LAYOUT_PHASE_ENTRY_POINT(mPresShell->GetPresContext(), FrameC);
  NS_PRECONDITION(aNewFrame, "null out param");
  
    
























































  
  {
    mPresShell->StyleSet()->SetBindingManager(mDocument->BindingManager());
  }

  
  nsIFrame*                 viewportFrame = nsnull;
  nsRefPtr<nsStyleContext> viewportPseudoStyle;
  nsStyleSet *styleSet = mPresShell->StyleSet();

  viewportPseudoStyle = styleSet->ResolvePseudoStyleFor(nsnull,
                                                        nsCSSAnonBoxes::viewport,
                                                        nsnull);

  viewportFrame = NS_NewViewportFrame(mPresShell, viewportPseudoStyle);

  nsPresContext* presContext = mPresShell->GetPresContext();

  
  
  
  viewportFrame->Init(nsnull, nsnull, nsnull);

  
  nsIViewManager* viewManager = mPresShell->GetViewManager();
  nsIView*        rootView;

  viewManager->GetRootView(rootView);
  viewportFrame->SetView(rootView);

  nsContainerFrame::SyncFrameViewProperties(presContext, viewportFrame,
                                            viewportPseudoStyle, rootView);

  
  mFixedContainingBlock = viewportFrame;

  


  
  
  
  
  
  
  

  PRBool isPaginated = presContext->IsRootPaginatedDocument();

  nsIFrame* rootFrame = nsnull;
  nsIAtom* rootPseudo;
        
  if (!isPaginated) {
#ifdef MOZ_XUL
    if (aDocElement->IsNodeOfType(nsINode::eXUL))
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


  

  
  
  
  

  
  
  
  
  
  
  
  
  

  PRBool isHTML = aDocElement->IsNodeOfType(nsINode::eHTML);
  PRBool isXUL = PR_FALSE;

  if (!isHTML) {
    isXUL = aDocElement->IsNodeOfType(nsINode::eXUL);
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

  
  if (!isScrollable) {
    rootPseudoStyle = styleSet->ResolvePseudoStyleFor(nsnull,
                                                      rootPseudo,
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
      styleContext = styleSet->ResolvePseudoStyleFor(nsnull,
                                                     nsCSSAnonBoxes::viewportScroll,
                                                     viewportPseudoStyle);

      
      
      
      
      
      
      
      newFrame = nsnull;
      rootPseudoStyle = BeginBuildingScrollFrame( state,
                                                  aDocElement,
                                                  styleContext,
                                                  viewportFrame,
                                                  rootPseudo,
                                                  PR_TRUE,
                                                  newFrame);

      nsIScrollableFrame* scrollable = do_QueryFrame(newFrame);
      NS_ENSURE_TRUE(scrollable, NS_ERROR_FAILURE);

      nsIScrollableView* scrollableView = scrollable->GetScrollableView();
      NS_ENSURE_TRUE(scrollableView, NS_ERROR_FAILURE);

      viewManager->SetRootScrollableView(scrollableView);
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
    rootFrame->SetInitialChildList(nsnull, pageFrame);

    
    
    mDocElementContainingBlock = canvasFrame;
    mHasRootAbsPosContainingBlock = PR_TRUE;
  }

  viewportFrame->SetInitialChildList(nsnull, newFrame);
  
  *aNewFrame = viewportFrame;

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
  pagePseudoStyle = styleSet->ResolvePseudoStyleFor(nsnull,
                                                    nsCSSAnonBoxes::page,
                                                    parentStyleContext);

  aPageFrame = NS_NewPageFrame(aPresShell, pagePseudoStyle);
  if (NS_UNLIKELY(!aPageFrame))
    return NS_ERROR_OUT_OF_MEMORY;

  
  
  aPageFrame->Init(nsnull, aParentFrame, aPrevPageFrame);

  nsRefPtr<nsStyleContext> pageContentPseudoStyle;
  pageContentPseudoStyle = styleSet->ResolvePseudoStyleFor(nsnull,
                                                           nsCSSAnonBoxes::pageContent,
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
  aPageFrame->SetInitialChildList(nsnull, pageContentFrame);
  mFixedContainingBlock = pageContentFrame;

  nsRefPtr<nsStyleContext> canvasPseudoStyle;
  canvasPseudoStyle = styleSet->ResolvePseudoStyleFor(nsnull,
                                                      nsCSSAnonBoxes::canvas,
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
  pageContentFrame->SetInitialChildList(nsnull, aCanvasFrame);

  return NS_OK;
}


nsresult
nsCSSFrameConstructor::CreatePlaceholderFrameFor(nsIPresShell*    aPresShell, 
                                                 nsIContent*      aContent,
                                                 nsIFrame*        aFrame,
                                                 nsStyleContext*  aStyleContext,
                                                 nsIFrame*        aParentFrame,
                                                 nsIFrame*        aPrevInFlow,
                                                 nsIFrame**       aPlaceholderFrame)
{
  nsRefPtr<nsStyleContext> placeholderStyle = aPresShell->StyleSet()->
    ResolveStyleForNonElement(aStyleContext->GetParent());
  
  
  nsPlaceholderFrame* placeholderFrame =
    (nsPlaceholderFrame*)NS_NewPlaceholderFrame(aPresShell, placeholderStyle);

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
  
  nsHTMLContainerFrame::CreateViewForFrame(buttonFrame, PR_FALSE);

  nsRefPtr<nsStyleContext> innerBlockContext;
  innerBlockContext =
    mPresShell->StyleSet()->ResolvePseudoStyleFor(content,
                                                  nsCSSAnonBoxes::buttonContent,
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
                         childItems, aStyleDisplay->IsBlockOutside());
    if (NS_FAILED(rv)) return rv;
  
    
    blockFrame->SetInitialChildList(nsnull, childItems.childList);
  }

  buttonFrame->SetInitialChildList(nsnull, blockFrame);

  if (isLeaf) {
    nsFrameItems  anonymousChildItems;
    
    
    
    CreateAnonymousFrames(aState, content, buttonFrame, anonymousChildItems);
    if (anonymousChildItems.childList) {
      
      aState.mFrameManager->AppendFrames(blockFrame, nsnull,
                                         anonymousChildItems.childList);
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

      nsHTMLContainerFrame::CreateViewForFrame(comboboxFrame, PR_FALSE);

      rv = aState.AddChild(comboboxFrame, aFrameItems, content, styleContext,
                           aParentFrame);
      if (NS_FAILED(rv)) {
        return rv;
      }
      
      
      
      
      nsIComboboxControlFrame* comboBox = do_QueryFrame(comboboxFrame);
      NS_ASSERTION(comboBox, "NS_NewComboboxControlFrame returned frame that "
                             "doesn't implement nsIComboboxControlFrame");

        
      nsRefPtr<nsStyleContext> listStyle;
      listStyle = mPresShell->StyleSet()->ResolvePseudoStyleFor(content,
                                                                nsCSSAnonBoxes::dropDownList, 
                                                                styleContext);

        
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
                            comboboxFrame, listStyle, PR_TRUE, aFrameItems);

        
        
        
      NS_ASSERTION(listFrame->GetView(), "ListFrame's view is nsnull");
      

      
      
      

      nsFrameItems childItems;
      CreateAnonymousFrames(aState, content, comboboxFrame, childItems);
  
      comboboxFrame->SetInitialChildList(nsnull, childItems.childList);

      
      
      nsFrameItems popupItems;
      popupItems.AddChild(listFrame);
      comboboxFrame->SetInitialChildList(nsGkAtoms::selectPopupList,
                                         popupItems.childList);

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
                            aParentFrame, styleContext, PR_FALSE, aFrameItems);

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
      
  nsHTMLContainerFrame::CreateViewForFrame(scrollFrame, aBuildCombobox);
  if (aBuildCombobox) {
    
    nsIView* view = scrollFrame->GetView();
    NS_ASSERTION(view, "We asked for a view but didn't get one");
    if (view) {
      view->GetViewManager()->SetViewFloating(view, PR_TRUE);

      nsWidgetInitData widgetData;
      widgetData.mWindowType  = eWindowType_popup;
      widgetData.mBorderStyle = eBorderStyle_default;

#if defined(XP_MACOSX) || defined(XP_BEOS) 
      static NS_DEFINE_IID(kCPopUpCID,  NS_POPUP_CID);
      view->CreateWidget(kCPopUpCID, &widgetData, nsnull);
#else
      static NS_DEFINE_IID(kCChildCID, NS_CHILD_CID);
      view->CreateWidget(kCChildCID, &widgetData, nsnull);
#endif
    }
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
                  childItems, PR_TRUE);

  
  scrolledFrame->SetInitialChildList(nsnull, childItems.childList);
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

  
  
  nsHTMLContainerFrame::CreateViewForFrame(newFrame, PR_FALSE);

  
  nsRefPtr<nsStyleContext> fieldsetContentStyle;
  fieldsetContentStyle =
    mPresShell->StyleSet()->ResolvePseudoStyleFor(content,
                                                  nsCSSAnonBoxes::fieldsetContent,
                                                  styleContext);

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
                  childItems, PR_TRUE);

  nsIFrame * child      = childItems.childList;
  nsIFrame * previous   = nsnull;
  nsLegendFrame* legendFrame = nsnull;
  while (nsnull != child) {
    legendFrame = do_QueryFrame(child);
    if (legendFrame) {
      
      
      
      
      
      if (nsnull != previous) {
        previous->SetNextSibling(legendFrame->GetNextSibling());
      } else {
        childItems.childList = legendFrame->GetNextSibling();
      }
      legendFrame->SetNextSibling(blockFrame);
      legendFrame->SetParent(newFrame);
      break;
    }
    previous = child;
    child = child->GetNextSibling();
  }

  
  blockFrame->SetInitialChildList(nsnull, childItems.childList);

  
  newFrame->SetInitialChildList(nsnull, legendFrame ? legendFrame : blockFrame);

  
  *aNewFrame = newFrame; 

  return NS_OK;
}

static nsIFrame*
FindAncestorWithGeneratedContentPseudo(nsIFrame* aFrame)
{
  for (nsIFrame* f = aFrame->GetParent(); f; f = f->GetParent()) {
    NS_ASSERTION(f->IsGeneratedContentFrame(),
                 "should not have exited generated content");
    nsIAtom* pseudo = f->GetStyleContext()->GetPseudoType();
    if (pseudo == nsCSSPseudoElements::before ||
        pseudo == nsCSSPseudoElements::after)
      return f;
  }
  return nsnull;
}

#define FCDATA_DECL(_flags, _func) \
  { _flags, { (FrameCreationFunc)_func } }
#define SIMPLE_FCDATA(_func) FCDATA_DECL(0, _func)
#define FULL_CTOR_FCDATA(_flags, _func)                     \
  { _flags | FCDATA_FUNC_IS_FULL_CTOR, { nsnull }, _func }


const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindTextData(nsIFrame* aParentFrame)
{
#ifdef MOZ_SVG
  if (aParentFrame && aParentFrame->IsFrameOfType(nsIFrame::eSVG)) {
    nsIFrame *ancestorFrame =
      nsSVGUtils::GetFirstNonAAncestorFrame(aParentFrame);
    if (ancestorFrame) {
      nsISVGTextContentMetrics* metrics = do_QueryFrame(ancestorFrame);
      if (metrics) {
        static const FrameConstructionData sSVGGlyphData =
          SIMPLE_FCDATA(NS_NewSVGGlyphFrame);
        return &sSVGGlyphData;
      }
    }
    return nsnull;
  }
#endif

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
                                          nsFrameItems&            aFrameItems,
                                          PRBool                   aPseudoParent)
{
  NS_PRECONDITION(aData, "Must have frame construction data");

  
  if (!aPseudoParent && !aState.mPseudoFrames.IsEmpty() &&
      !TextIsOnlyWhitespace(aContent))
    ProcessPseudoFrames(aState, aFrameItems);

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

  
  

  return rv;
}


const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindDataByInt(PRInt32 aInt,
                                     nsIContent* aContent,
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
        return data->mFunc.mDataGetter(aContent, aStyleContext);
      }

      return data;
    }
  }

  return nsnull;
}


const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindDataByTag(nsIAtom* aTag,
                                     nsIContent* aContent,
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
        return data->mFunc.mDataGetter(aContent, aStyleContext);
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
nsCSSFrameConstructor::FindHTMLData(nsIContent* aContent,
                                    nsIAtom* aTag,
                                    PRInt32 aNameSpaceID,
                                    nsIFrame* aParentFrame,
                                    nsStyleContext* aStyleContext)
{
  
  
  
  if (!aContent->IsNodeOfType(nsINode::eHTML) &&
      aNameSpaceID != kNameSpaceID_XHTML) {
    return nsnull;
  }

  NS_ASSERTION(!aParentFrame ||
               aParentFrame->GetStyleContext()->GetPseudoType() !=
                 nsCSSAnonBoxes::fieldsetContent ||
               aParentFrame->GetParent()->GetType() == nsGkAtoms::fieldSetFrame,
               "Unexpected parent for fieldset content anon box");
  if (aTag == nsGkAtoms::legend &&
      (!aParentFrame ||
       (aParentFrame->GetType() != nsGkAtoms::fieldSetFrame &&
        aParentFrame->GetStyleContext()->GetPseudoType() !=
          nsCSSAnonBoxes::fieldsetContent))) {
    
    
    
    
    
    return nsnull;
  }

  static const FrameConstructionDataByTag sHTMLData[] = {
    SIMPLE_TAG_CHAIN(img, nsCSSFrameConstructor::FindImgData),
    SIMPLE_TAG_CHAIN(mozgeneratedcontentimage,
                     nsCSSFrameConstructor::FindImgData),
    { &nsGkAtoms::br,
      FCDATA_DECL(FCDATA_SKIP_FRAMEMAP | FCDATA_IS_LINE_PARTICIPANT,
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
    SIMPLE_TAG_CREATE(legend, NS_NewLegendFrame),
    SIMPLE_TAG_CREATE(frameset, NS_NewHTMLFramesetFrame),
    SIMPLE_TAG_CREATE(iframe, NS_NewSubDocumentFrame),
    SIMPLE_TAG_CREATE(spacer, NS_NewSpacerFrame),
    COMPLEX_TAG_CREATE(button, &nsCSSFrameConstructor::ConstructButtonFrame),
    SIMPLE_TAG_CREATE(canvas, NS_NewHTMLCanvasFrame),
#if defined(MOZ_MEDIA)
    SIMPLE_TAG_CREATE(video, NS_NewHTMLVideoFrame),
    SIMPLE_TAG_CREATE(audio, NS_NewHTMLVideoFrame),
#endif
    SIMPLE_TAG_CREATE(isindex, NS_NewIsIndexFrame)
  };

  return FindDataByTag(aTag, aContent, aStyleContext, sHTMLData,
                       NS_ARRAY_LENGTH(sHTMLData));
}


const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindImgData(nsIContent* aContent,
                                   nsStyleContext* aStyleContext)
{
  if (!nsImageFrame::ShouldCreateImageFrameFor(aContent, aStyleContext)) {
    return nsnull;
  }

  static const FrameConstructionData sImgData = SIMPLE_FCDATA(NS_NewImageFrame);
  return &sImgData;
}


const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindImgControlData(nsIContent* aContent,
                                          nsStyleContext* aStyleContext)
{
  if (!nsImageFrame::ShouldCreateImageFrameFor(aContent, aStyleContext)) {
    return nsnull;
  }

  static const FrameConstructionData sImgControlData =
    SIMPLE_FCDATA(NS_NewImageControlFrame);
  return &sImgControlData;
}


const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindInputData(nsIContent* aContent,
                                     nsStyleContext* aStyleContext)
{
  static const FrameConstructionDataByInt sInputData[] = {
    SIMPLE_INT_CREATE(NS_FORM_INPUT_CHECKBOX, NS_NewGfxCheckboxControlFrame),
    SIMPLE_INT_CREATE(NS_FORM_INPUT_RADIO, NS_NewGfxRadioControlFrame),
    SIMPLE_INT_CREATE(NS_FORM_INPUT_FILE, NS_NewFileControlFrame),
    SIMPLE_INT_CHAIN(NS_FORM_INPUT_IMAGE,
                     nsCSSFrameConstructor::FindImgControlData),
    SIMPLE_INT_CREATE(NS_FORM_INPUT_TEXT, NS_NewTextControlFrame),
    SIMPLE_INT_CREATE(NS_FORM_INPUT_PASSWORD, NS_NewTextControlFrame),
    COMPLEX_INT_CREATE(NS_FORM_INPUT_SUBMIT,
                       &nsCSSFrameConstructor::ConstructButtonFrame),
    COMPLEX_INT_CREATE(NS_FORM_INPUT_RESET,
                       &nsCSSFrameConstructor::ConstructButtonFrame),
    COMPLEX_INT_CREATE(NS_FORM_INPUT_BUTTON,
                       &nsCSSFrameConstructor::ConstructButtonFrame)
    
    
  };

  nsCOMPtr<nsIFormControl> control = do_QueryInterface(aContent);
  NS_ASSERTION(control, "input doesn't implement nsIFormControl?");

  return FindDataByInt(control->GetType(), aContent, aStyleContext,
                       sInputData, NS_ARRAY_LENGTH(sInputData));
}


const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindObjectData(nsIContent* aContent,
                                      nsStyleContext* aStyleContext)
{
  
  
  
  PRUint32 type;
  if (aContent->IntrinsicState() &
      (NS_EVENT_STATE_BROKEN | NS_EVENT_STATE_USERDISABLED |
       NS_EVENT_STATE_SUPPRESSED)) {
    type = nsIObjectLoadingContent::TYPE_NULL;
  } else {
    nsCOMPtr<nsIObjectLoadingContent> objContent(do_QueryInterface(aContent));
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

  return FindDataByInt((PRInt32)type, aContent, aStyleContext,
                       sObjectData, NS_ARRAY_LENGTH(sObjectData));
}

nsresult
nsCSSFrameConstructor::ConstructFrameFromItemInternal(FrameConstructionItem& aItem,
                                                      nsFrameConstructorState& aState,
                                                      nsIFrame* aParentFrame,
                                                      nsFrameItems& aFrameItems,
                                                      PRBool aHasPseudoParent)
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
#ifdef MOZ_MATHML
  CHECK_ONLY_ONE_BIT(FCDATA_FUNC_IS_FULL_CTOR, FCDATA_WRAP_KIDS_IN_BLOCKS);
#endif
  CHECK_ONLY_ONE_BIT(FCDATA_FUNC_IS_FULL_CTOR, FCDATA_MAY_NEED_SCROLLFRAME);
  CHECK_ONLY_ONE_BIT(FCDATA_FUNC_IS_FULL_CTOR, FCDATA_IS_POPUP);
  CHECK_ONLY_ONE_BIT(FCDATA_FUNC_IS_FULL_CTOR, FCDATA_SKIP_ABSPOS_PUSH);
  CHECK_ONLY_ONE_BIT(FCDATA_FUNC_IS_FULL_CTOR, FCDATA_FORCE_VIEW);
  CHECK_ONLY_ONE_BIT(FCDATA_FUNC_IS_FULL_CTOR,
                     FCDATA_DISALLOW_GENERATED_CONTENT);
  CHECK_ONLY_ONE_BIT(FCDATA_FUNC_IS_FULL_CTOR, FCDATA_ALLOW_BLOCK_STYLES);
  CHECK_ONLY_ONE_BIT(FCDATA_MAY_NEED_SCROLLFRAME, FCDATA_FORCE_VIEW);
#undef CHECK_ONLY_ONE_BIT

  
  if (!aHasPseudoParent && !aState.mPseudoFrames.IsEmpty()) {
    ProcessPseudoFrames(aState, aFrameItems); 
  }

  nsStyleContext* const styleContext = aItem.mStyleContext;
  const nsStyleDisplay* display = styleContext->GetStyleDisplay();

  nsIFrame* newFrame;
  if (bits & FCDATA_FUNC_IS_FULL_CTOR) {
    nsresult rv =
      (this->*(data->mFullConstructor))(aState, aItem, aParentFrame,
                                        display, aFrameItems, &newFrame);
    if (NS_FAILED(rv)) {
      return rv;
    }
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
      
      
      bits |= FCDATA_SKIP_FRAMEMAP;
    } else {
      rv = InitAndRestoreFrame(aState, content, geometricParent, nsnull,
                               newFrame);
      NS_ASSERTION(NS_SUCCEEDED(rv), "InitAndRestoreFrame failed");
      
      nsHTMLContainerFrame::CreateViewForFrame(newFrame,
                                               (bits & FCDATA_FORCE_VIEW) != 0);
      frameToAddToList = newFrame;
    }

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

    
    rv = ProcessChildren(aState, content, styleContext, newFrame,
                         !(bits & FCDATA_DISALLOW_GENERATED_CONTENT),
                         childItems,
                         (bits & FCDATA_ALLOW_BLOCK_STYLES) != 0);

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

#ifdef MOZ_MATHML
    if (NS_SUCCEEDED(rv) && (bits & FCDATA_WRAP_KIDS_IN_BLOCKS)) {
      nsFrameItems newItems;
      nsFrameItems currentBlock;
      nsIFrame* f;
      while ((f = childItems.childList) != nsnull) {
        PRBool wrapFrame = IsInlineFrame(f) || IsFrameSpecial(f);
        if (!wrapFrame) {
          rv = FlushAccumulatedBlock(aState, content, newFrame, &currentBlock, &newItems);
          if (NS_FAILED(rv))
            break;
        }

        childItems.RemoveChild(f, nsnull);
        if (wrapFrame) {
          currentBlock.AddChild(f);
        } else {
          newItems.AddChild(f);
        }
      }
      rv = FlushAccumulatedBlock(aState, content, newFrame, &currentBlock, &newItems);

      if (childItems.childList) {
        
        CleanupFrameReferences(aState.mFrameManager, childItems.childList);
        nsFrameList(childItems.childList).DestroyFrames();
      }

      childItems = newItems;
    }
#endif

    
    
    
    newFrame->SetInitialChildList(nsnull, childItems.childList);
  }

  NS_ASSERTION(newFrame->IsFrameOfType(nsIFrame::eLineParticipant) ==
               ((bits & FCDATA_IS_LINE_PARTICIPANT) != 0),
               "Incorrectly set FCDATA_IS_LINE_PARTICIPANT bits");

  if (!(bits & FCDATA_SKIP_FRAMEMAP)) {
    aState.mFrameManager->SetPrimaryFrameFor(aItem.mContent, newFrame);
  }

  return NS_OK;
}



nsresult
nsCSSFrameConstructor::CreateAnonymousFrames(nsFrameConstructorState& aState,
                                             nsIContent*              aParent,
                                             nsIFrame*                aParentFrame,
                                             nsFrameItems&            aChildItems)
{
  nsAutoTArray<nsIContent*, 4> newAnonymousItems;
  nsresult rv = GetAnonymousContent(aParent, aParentFrame, newAnonymousItems);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 count = newAnonymousItems.Length();
  if (count == 0) {
    return NS_OK;
  }

  nsIAnonymousContentCreator* creator = do_QueryFrame(aParentFrame);
  NS_ASSERTION(creator,
               "How can that happen if we have nodes to construct frames for?");

  
  
  nsPseudoFrames priorPseudoFrames; 
  aState.mPseudoFrames.Reset(&priorPseudoFrames);

  for (PRUint32 i=0; i < count; i++) {
    nsIContent* content = newAnonymousItems[i];
    NS_ASSERTION(content, "null anonymous content?");

    nsIFrame* newFrame = creator->CreateFrameFor(content);
    if (newFrame) {
      aChildItems.AddChild(newFrame);
    }
    else {
      
      ConstructFrame(aState, content, aParentFrame, aChildItems);
    }
  }

  
  if (!aState.mPseudoFrames.IsEmpty()) {
    ProcessPseudoFrames(aState, aChildItems);
  }

  
  aState.mPseudoFrames = priorPseudoFrames;

  return NS_OK;
}

nsresult
nsCSSFrameConstructor::GetAnonymousContent(nsIContent* aParent,
                                           nsIFrame* aParentFrame,
                                           nsTArray<nsIContent*>& aContent)
{
  nsIAnonymousContentCreator* creator = do_QueryFrame(aParentFrame);
  if (!creator)
    return NS_OK;

  nsresult rv = creator->CreateAnonymousContent(aContent);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 count = aContent.Length();
  for (PRUint32 i=0; i < count; i++) {
    
    nsIContent* content = aContent[i];
    NS_ASSERTION(content, "null anonymous content?");

#ifdef MOZ_SVG
    
    
    if (aParent &&
        aParent->NodeInfo()->Equals(nsGkAtoms::use, kNameSpaceID_SVG)) {
      content->SetFlags(NODE_IS_ANONYMOUS);
    } else
#endif
    {
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
nsCSSFrameConstructor::FindXULTagData(nsIContent* aContent,
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

  return FindDataByTag(aTag, aContent, aStyleContext, sXULTagData,
                       NS_ARRAY_LENGTH(sXULTagData));
}

#ifdef MOZ_XUL

const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindPopupGroupData(nsIContent* aContent,
                                          nsStyleContext* )
{
  if (!aContent->IsRootOfNativeAnonymousSubtree()) {
    return nsnull;
  }

  static const FrameConstructionData sPopupSetData =
    SIMPLE_XUL_FCDATA(NS_NewPopupSetFrame);
  return &sPopupSetData;
}


const nsCSSFrameConstructor::FrameConstructionData
nsCSSFrameConstructor::sXULTextBoxData = SIMPLE_XUL_FCDATA(NS_NewTextBoxFrame);


const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindXULLabelData(nsIContent* aContent,
                                        nsStyleContext* )
{
  if (aContent->HasAttr(kNameSpaceID_None, nsGkAtoms::value)) {
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
nsCSSFrameConstructor::FindXULDescriptionData(nsIContent* aContent,
                                              nsStyleContext* )
{
  if (aContent->HasAttr(kNameSpaceID_None, nsGkAtoms::value)) {
    return &sXULTextBoxData;
  }

  static const FrameConstructionData sDescriptionData =
    SIMPLE_XUL_FCDATA(NS_NewXULDescriptionFrame);
  return &sDescriptionData;
}

#ifdef XP_MACOSX

const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindXULMenubarData(nsIContent* aContent,
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
nsCSSFrameConstructor::FindXULListBoxBodyData(nsIContent* aContent,
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
nsCSSFrameConstructor::FindXULListItemData(nsIContent* aContent,
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
                                          nsIContent* aContent,
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

  
  return FindDataByInt(aDisplay->mDisplay, aContent, aStyleContext,
                       sXULDisplayData, NS_ARRAY_LENGTH(sXULDisplayData));
}

nsresult
nsCSSFrameConstructor::AddLazyChildren(nsIContent* aContent,
                                       nsLazyFrameConstructionCallback* aCallback,
                                       void* aArg, PRBool aIsSynch)
{
  nsCOMPtr<nsIRunnable> event =
    new LazyGenerateChildrenEvent(aContent, mPresShell, aCallback, aArg);
  return aIsSynch ? event->Run() :
                    NS_DispatchToCurrentThread(event);
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

    
    nsHTMLContainerFrame::CreateViewForFrame(gfxScrollFrame, PR_FALSE);
  }

  
  
  CreateAnonymousFrames(aState, aContent, gfxScrollFrame, anonymousItems);

  aNewFrame = gfxScrollFrame;

  
  nsStyleSet *styleSet = mPresShell->StyleSet();
  nsStyleContext* aScrolledChildStyle = styleSet->ResolvePseudoStyleFor(aContent,
                                                                        aScrolledPseudo,
                                                                        contentStyle).get();

  if (gfxScrollFrame) {
     gfxScrollFrame->SetInitialChildList(nsnull, anonymousItems.childList);
  }

  return aScrolledChildStyle;
}

void
nsCSSFrameConstructor::FinishBuildingScrollFrame(nsIFrame* aScrollFrame,
                                                 nsIFrame* aScrolledFrame)
{
  aScrollFrame->AppendFrames(nsnull, aScrolledFrame);

  
  
  
  nsHTMLContainerFrame::CreateViewForFrame(aScrolledFrame, PR_TRUE);

  
  nsIView* view = aScrolledFrame->GetView();
  if (!view)
    return;
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

    
    aState.mFrameManager->SetPrimaryFrameFor( aContent, aNewFrame );
    return NS_OK;

}

const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindDisplayData(const nsStyleDisplay* aDisplay,
                                       nsIContent* aContent,
                                       nsStyleContext* aStyleContext)
{
  
  
  NS_ASSERTION(!(aDisplay->IsFloating() ||
                 aDisplay->IsAbsolutelyPositioned()) ||
               aDisplay->IsBlockOutside(),
               "Style system did not apply CSS2.1 section 9.7 fixups");

  
  
  
  
  
  
  PRBool propagatedScrollToViewport = PR_FALSE;
  if (aContent->NodeInfo()->Equals(nsGkAtoms::body) &&
      aContent->IsNodeOfType(nsINode::eHTML)) {
    propagatedScrollToViewport =
      PropagateScrollToViewport() == aContent;
  }

  
  
  
  
  
  if (aDisplay->IsBlockInside() &&
      aDisplay->IsScrollableOverflow() &&
      !propagatedScrollToViewport) {
    static const FrameConstructionData sScrollableBlockData =
      FULL_CTOR_FCDATA(0, &nsCSSFrameConstructor::ConstructScrollableBlock);
    return &sScrollableBlockData;
  }

  
  if (aDisplay->IsBlockInside() ||
      NS_STYLE_DISPLAY_RUN_IN == aDisplay->mDisplay ||
      NS_STYLE_DISPLAY_COMPACT == aDisplay->mDisplay) {  
    static const FrameConstructionData sNonScrollableBlockData =
      FULL_CTOR_FCDATA(0, &nsCSSFrameConstructor::ConstructNonScrollableBlock);
    return &sNonScrollableBlockData;
  }

  static const FrameConstructionDataByInt sDisplayData[] = {
    
    
    
    
    { NS_STYLE_DISPLAY_INLINE,
      FULL_CTOR_FCDATA(FCDATA_SKIP_FRAMEMAP | FCDATA_IS_INLINE |
                       FCDATA_IS_LINE_PARTICIPANT,
                       &nsCSSFrameConstructor::ConstructInline) },
    { NS_STYLE_DISPLAY_MARKER,
      FULL_CTOR_FCDATA(FCDATA_SKIP_FRAMEMAP | FCDATA_IS_INLINE |
                       FCDATA_IS_LINE_PARTICIPANT,
                       &nsCSSFrameConstructor::ConstructInline) },
    COMPLEX_INT_CREATE(NS_STYLE_DISPLAY_TABLE,
                       &nsCSSFrameConstructor::ConstructTable),
    COMPLEX_INT_CREATE(NS_STYLE_DISPLAY_INLINE_TABLE,
                       &nsCSSFrameConstructor::ConstructTable),
    { NS_STYLE_DISPLAY_TABLE_CAPTION,
      FCDATA_DECL(FCDATA_IS_TABLE_PART | FCDATA_SKIP_FRAMEMAP |
                  FCDATA_ALLOW_BLOCK_STYLES | FCDATA_DISALLOW_OUT_OF_FLOW,
                  NS_NewTableCaptionFrame) },
    { NS_STYLE_DISPLAY_TABLE_ROW_GROUP,
      FCDATA_DECL(FCDATA_IS_TABLE_PART | FCDATA_SKIP_FRAMEMAP |
                  FCDATA_DISALLOW_OUT_OF_FLOW | FCDATA_MAY_NEED_SCROLLFRAME,
                  NS_NewTableRowGroupFrame) },
    { NS_STYLE_DISPLAY_TABLE_HEADER_GROUP,
      FCDATA_DECL(FCDATA_IS_TABLE_PART | FCDATA_SKIP_FRAMEMAP |
                  FCDATA_DISALLOW_OUT_OF_FLOW | FCDATA_MAY_NEED_SCROLLFRAME,
                  NS_NewTableRowGroupFrame) },
    { NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP,
      FCDATA_DECL(FCDATA_IS_TABLE_PART | FCDATA_SKIP_FRAMEMAP |
                  FCDATA_DISALLOW_OUT_OF_FLOW | FCDATA_MAY_NEED_SCROLLFRAME,
                  NS_NewTableRowGroupFrame) },
    { NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP,
      FCDATA_DECL(FCDATA_IS_TABLE_PART | FCDATA_SKIP_FRAMEMAP |
                  FCDATA_DISALLOW_OUT_OF_FLOW,
                  NS_NewTableColGroupFrame) },
    { NS_STYLE_DISPLAY_TABLE_COLUMN,
      FULL_CTOR_FCDATA(FCDATA_IS_TABLE_PART | FCDATA_SKIP_FRAMEMAP,
                       &nsCSSFrameConstructor::ConstructTableCol) },
    { NS_STYLE_DISPLAY_TABLE_ROW,
      FULL_CTOR_FCDATA(FCDATA_IS_TABLE_PART | FCDATA_SKIP_FRAMEMAP,
                       &nsCSSFrameConstructor::ConstructTableRow) },
    { NS_STYLE_DISPLAY_TABLE_CELL,
      FULL_CTOR_FCDATA(FCDATA_IS_TABLE_PART | FCDATA_SKIP_FRAMEMAP,
                       &nsCSSFrameConstructor::ConstructTableCell) }
  };

  return FindDataByInt(aDisplay->mDisplay, aContent, aStyleContext,
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
                               &scrolledFrame, blockItem, aDisplay->IsPositioned());
  if (NS_UNLIKELY(NS_FAILED(rv))) {
    
    return rv;
  }

  NS_ASSERTION(blockItem.childList == scrolledFrame,
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

  if (aDisplay->IsAbsolutelyPositioned() ||
      aDisplay->IsFloating() ||
      NS_STYLE_DISPLAY_INLINE_BLOCK == aDisplay->mDisplay) {
    *aNewFrame = NS_NewBlockFormattingContext(mPresShell, styleContext);
  } else {
    *aNewFrame = NS_NewBlockFrame(mPresShell, styleContext);
  }

  return ConstructBlock(aState, aDisplay, aItem.mContent,
                        aState.GetGeometricParent(aDisplay, aParentFrame),
                        aParentFrame, styleContext, aNewFrame,
                        aFrameItems, aDisplay->IsPositioned());
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
  
  nsIFrame* innerTable;
  return ConstructTableFrame(aState, aItem.mContent, aParentFrame,
                             aItem.mStyleContext, aItem.mNameSpaceID,
                             PR_FALSE, aFrameItems, *aNewFrame, innerTable);
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
  return ConstructTableRowFrame(aState, aItem.mContent, aParentFrame,
                                aItem.mStyleContext, aItem.mNameSpaceID,
                                PR_FALSE, aFrameItems, *aNewFrame);
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

  nsIFrame* innerTableCell;
  return ConstructTableCellFrame(aState, aItem.mContent, aParentFrame,
                                 aItem.mStyleContext, aItem.mNameSpaceID,
                                 PR_FALSE, aFrameItems, *aNewFrame,
                                 innerTableCell);
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
                                           nsIContent*       aContent)
{
  nsStyleContext* parentStyleContext = nsnull;
  if (aContent->GetParent()) {
    aParentFrame = nsFrame::CorrectStyleParentFrame(aParentFrame, nsnull);
  
    if (aParentFrame) {
      
      
      parentStyleContext = aParentFrame->GetStyleContext();
    } else {
      
      
      
      
      
    }
  } else {
    
    
  }

  return ResolveStyleContext(parentStyleContext, aContent);
}

already_AddRefed<nsStyleContext>
nsCSSFrameConstructor::ResolveStyleContext(nsStyleContext* aParentStyleContext,
                                           nsIContent* aContent)
{
  nsStyleSet *styleSet = mPresShell->StyleSet();

  if (aContent->IsNodeOfType(nsINode::eELEMENT)) {
    return styleSet->ResolveStyleFor(aContent, aParentStyleContext);
  }

  NS_ASSERTION(aContent->IsNodeOfType(nsINode::eTEXT),
               "shouldn't waste time creating style contexts for "
               "comments and processing instructions");

  return styleSet->ResolveStyleForNonElement(aParentStyleContext);
}


#ifdef MOZ_MATHML
nsresult
nsCSSFrameConstructor::FlushAccumulatedBlock(nsFrameConstructorState& aState,
                                             nsIContent* aContent,
                                             nsIFrame* aParentFrame,
                                             nsFrameItems* aBlockItems,
                                             nsFrameItems* aNewItems)
{
  if (!aBlockItems->childList) {
    
    return NS_OK;
  }

  nsStyleContext* parentContext =
    nsFrame::CorrectStyleParentFrame(aParentFrame,
                                     nsCSSAnonBoxes::mozMathMLAnonymousBlock)->GetStyleContext(); 
  nsStyleSet *styleSet = mPresShell->StyleSet();
  nsRefPtr<nsStyleContext> blockContext;
  blockContext = styleSet->ResolvePseudoStyleFor(aContent,
                                                 nsCSSAnonBoxes::mozMathMLAnonymousBlock,
                                                 parentContext);

  
  
  
  nsIFrame* blockFrame = NS_NewMathMLmathBlockFrame(mPresShell, blockContext,
                          NS_BLOCK_FLOAT_MGR | NS_BLOCK_MARGIN_ROOT);
  if (NS_UNLIKELY(!blockFrame))
    return NS_ERROR_OUT_OF_MEMORY;

  InitAndRestoreFrame(aState, aContent, aParentFrame, nsnull, blockFrame);
  for (nsIFrame* f = aBlockItems->childList; f; f = f->GetNextSibling()) {
    ReparentFrame(aState.mFrameManager, blockFrame, f);
  }
  
  
  blockFrame->SetInitialChildList(nsnull, aBlockItems->childList);
  *aBlockItems = nsFrameItems();
  aNewItems->AddChild(blockFrame);
  return NS_OK;
}



#define SIMPLE_MATHML_CREATE(_tag, _func)                               \
  { &nsGkAtoms::_tag,                                                   \
      FCDATA_DECL(FCDATA_DISALLOW_OUT_OF_FLOW |                         \
                  FCDATA_FORCE_NULL_ABSPOS_CONTAINER |                  \
                  FCDATA_WRAP_KIDS_IN_BLOCKS |                          \
                  FCDATA_SKIP_FRAMEMAP, _func) }


const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindMathMLData(nsIContent* aContent,
                                      nsIAtom* aTag,
                                      PRInt32 aNameSpaceID,
                                      nsStyleContext* aStyleContext)
{
  
  if (aNameSpaceID != kNameSpaceID_MathML) 
    return nsnull;

  
  if (aTag == nsGkAtoms::math) {
    if (aStyleContext->GetStyleDisplay()->mDisplay == NS_STYLE_DISPLAY_BLOCK) {
      static const FrameConstructionData sBlockMathData =
        FCDATA_DECL(FCDATA_FORCE_NULL_ABSPOS_CONTAINER |
                    FCDATA_WRAP_KIDS_IN_BLOCKS |
                    FCDATA_SKIP_FRAMEMAP,
                    NS_CreateNewMathMLmathBlockFrame);
      return &sBlockMathData;
    }

    static const FrameConstructionData sInlineMathData =
      FCDATA_DECL(FCDATA_FORCE_NULL_ABSPOS_CONTAINER |
                  FCDATA_WRAP_KIDS_IN_BLOCKS |
                  FCDATA_SKIP_FRAMEMAP |
                  FCDATA_IS_LINE_PARTICIPANT,
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
    SIMPLE_MATHML_CREATE(merror_, NS_NewMathMLmrowFrame)
  };

  return FindDataByTag(aTag, aContent, aStyleContext, sMathMLData,
                       NS_ARRAY_LENGTH(sMathMLData));
}
#endif 

#ifdef MOZ_SVG


#define SIMPLE_SVG_FCDATA(_func)                                        \
  FCDATA_DECL(FCDATA_DISALLOW_OUT_OF_FLOW |                             \
              FCDATA_SKIP_ABSPOS_PUSH | FCDATA_SKIP_FRAMEMAP |          \
              FCDATA_DISALLOW_GENERATED_CONTENT,  _func)
#define SIMPLE_SVG_CREATE(_tag, _func)            \
  { &nsGkAtoms::_tag, SIMPLE_SVG_FCDATA(_func) }


const nsCSSFrameConstructor::FrameConstructionData*
nsCSSFrameConstructor::FindSVGData(nsIContent* aContent,
                                   nsIAtom* aTag,
                                   PRInt32 aNameSpaceID,
                                   nsIFrame* aParentFrame,
                                   nsStyleContext* aStyleContext)
{
  if (aNameSpaceID != kNameSpaceID_SVG || !NS_SVGEnabled()) {
    return nsnull;
  }

  static const FrameConstructionData sSuppressData = SUPPRESS_FCDATA();
  static const FrameConstructionData sGenericContainerData =
    SIMPLE_SVG_FCDATA(NS_NewSVGGenericContainerFrame);

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
      (aTag == nsGkAtoms::desc || aTag == nsGkAtoms::title)) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    return &sSuppressData;
  }

  
  
  
  if (!NS_SVG_PassesConditionalProcessingTests(aContent)) {
    
    
    
    
    
    
    return &sSuppressData;
  }

  
  
  if (aTag == nsGkAtoms::svg) {
    if (parentIsSVG) {
      static const FrameConstructionData sInnerSVGData =
        SIMPLE_SVG_FCDATA(NS_NewSVGInnerSVGFrame);
      return &sInnerSVGData;
    }

    static const FrameConstructionData sOuterSVGData =
      FCDATA_DECL(FCDATA_FORCE_VIEW | FCDATA_SKIP_ABSPOS_PUSH |
                  FCDATA_SKIP_FRAMEMAP | FCDATA_DISALLOW_GENERATED_CONTENT,
                  NS_NewSVGOuterSVGFrame);
    return &sOuterSVGData;
  }

  
  
  if (aTag == nsGkAtoms::text) {
    NS_ASSERTION(aParentFrame, "Should have aParentFrame here");
    nsIFrame *ancestorFrame =
      nsSVGUtils::GetFirstNonAAncestorFrame(aParentFrame);
    if (ancestorFrame) {
      nsISVGTextContentMetrics* metrics = do_QueryFrame(ancestorFrame);
      
      if (metrics) {
        return &sGenericContainerData;
      }
    }
  }
  else if (aTag == nsGkAtoms::tspan) {
    NS_ASSERTION(aParentFrame, "Should have aParentFrame here");
    nsIFrame *ancestorFrame =
      nsSVGUtils::GetFirstNonAAncestorFrame(aParentFrame);
    if (ancestorFrame) {
      nsISVGTextContentMetrics* metrics = do_QueryFrame(ancestorFrame);
      if (!metrics) {
        return &sGenericContainerData;
      }
    }
  }
  else if (aTag == nsGkAtoms::textPath) {
    NS_ASSERTION(aParentFrame, "Should have aParentFrame here");
    nsIFrame *ancestorFrame =
      nsSVGUtils::GetFirstNonAAncestorFrame(aParentFrame);
    if (!ancestorFrame ||
        ancestorFrame->GetType() != nsGkAtoms::svgTextFrame) {
      return &sGenericContainerData;
    }
  }

  static const FrameConstructionDataByTag sSVGData[] = {
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
    { &nsGkAtoms::foreignObject,
      FULL_CTOR_FCDATA(FCDATA_DISALLOW_OUT_OF_FLOW,
                       &nsCSSFrameConstructor::ConstructSVGForeignObjectFrame) },
    SIMPLE_SVG_CREATE(a, NS_NewSVGAFrame),
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
    SIMPLE_SVG_CREATE(feDistantLight, NS_NewSVGLeafFrame),
    SIMPLE_SVG_CREATE(fePointLight, NS_NewSVGLeafFrame),
    SIMPLE_SVG_CREATE(feSpotLight, NS_NewSVGLeafFrame),
    SIMPLE_SVG_CREATE(feBlend, NS_NewSVGLeafFrame),
    SIMPLE_SVG_CREATE(feColorMatrix, NS_NewSVGLeafFrame),
    SIMPLE_SVG_CREATE(feFuncR, NS_NewSVGLeafFrame),
    SIMPLE_SVG_CREATE(feFuncG, NS_NewSVGLeafFrame),
    SIMPLE_SVG_CREATE(feFuncB, NS_NewSVGLeafFrame),
    SIMPLE_SVG_CREATE(feFuncA, NS_NewSVGLeafFrame),
    SIMPLE_SVG_CREATE(feComposite, NS_NewSVGLeafFrame),
    SIMPLE_SVG_CREATE(feConvolveMatrix, NS_NewSVGLeafFrame),
    SIMPLE_SVG_CREATE(feDisplacementMap, NS_NewSVGLeafFrame),
    SIMPLE_SVG_CREATE(feFlood, NS_NewSVGLeafFrame),
    SIMPLE_SVG_CREATE(feGaussianBlur, NS_NewSVGLeafFrame),
    SIMPLE_SVG_CREATE(feImage, NS_NewSVGLeafFrame),
    SIMPLE_SVG_CREATE(feMergeNode, NS_NewSVGLeafFrame),
    SIMPLE_SVG_CREATE(feMorphology, NS_NewSVGLeafFrame), 
    SIMPLE_SVG_CREATE(feOffset, NS_NewSVGLeafFrame), 
    SIMPLE_SVG_CREATE(feTile, NS_NewSVGLeafFrame), 
    SIMPLE_SVG_CREATE(feTurbulence, NS_NewSVGLeafFrame) 
  };

  const FrameConstructionData* data =
    FindDataByTag(aTag, aContent, aStyleContext, sSVGData,
                  NS_ARRAY_LENGTH(sSVGData));

  if (!data) {
    data = &sGenericContainerData;
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
  nsHTMLContainerFrame::CreateViewForFrame(newFrame, PR_FALSE);

  nsresult rv = aState.AddChild(newFrame, aFrameItems, content, styleContext,
                                aParentFrame, PR_FALSE, PR_FALSE);
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsRefPtr<nsStyleContext> innerPseudoStyle;
  innerPseudoStyle = mPresShell->StyleSet()->
    ResolvePseudoStyleFor(content,
                          nsCSSAnonBoxes::mozSVGForeignContent, styleContext);

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
                      &blockFrame, childItems, PR_TRUE);

  
  
  nsHTMLContainerFrame::CreateViewForFrame(blockFrame, PR_TRUE);

  newFrame->SetInitialChildList(nsnull, childItems.childList);

  *aNewFrame = newFrame;

  return rv;
}

#endif 

void
nsCSSFrameConstructor::AddPageBreakItem(nsIContent* aContent,
                                        nsStyleContext* aMainStyleContext,
                                        FrameConstructionItemList& aItems)
{
  nsRefPtr<nsStyleContext> pseudoStyle;
  
  
  
  
  pseudoStyle =
    mPresShell->StyleSet()->
      ResolvePseudoStyleFor(nsnull, nsCSSAnonBoxes::pageBreak,
                            aMainStyleContext->GetParent());

  NS_ASSERTION(pseudoStyle->GetStyleDisplay()->mDisplay ==
                 NS_STYLE_DISPLAY_BLOCK, "Unexpected display");

  static const FrameConstructionData sPageBreakData =
    FCDATA_DECL(FCDATA_SKIP_FRAMEMAP, NS_NewPageBreakFrame);

  
  
  aItems.AppendItem(&sPageBreakData, aContent, nsCSSAnonBoxes::pageBreak,
                    kNameSpaceID_None, pseudoStyle.forget());
}

nsresult
nsCSSFrameConstructor::ConstructFrame(nsFrameConstructorState& aState,
                                      nsIContent*              aContent,
                                      nsIFrame*                aParentFrame,
                                      nsFrameItems&            aFrameItems)

{
  NS_PRECONDITION(nsnull != aParentFrame, "no parent frame");
  FrameConstructionItemList items;
  AddFrameConstructionItems(aState, aContent, aParentFrame, items);

  for (FCItemIterator iter(items); !iter.IsDone(); iter.Next()) {
    nsresult rv =
      ConstructFramesFromItem(aState, iter, aParentFrame, aFrameItems);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

void
nsCSSFrameConstructor::AddFrameConstructionItems(nsFrameConstructorState& aState,
                                                 nsIContent* aContent,
                                                 nsIFrame* aParentFrame,
                                                 FrameConstructionItemList& aItems)
{
  
  if (!NeedFrameFor(aParentFrame, aContent)) {
    return;
  }

  
  if (aContent->IsNodeOfType(nsINode::eCOMMENT) ||
      aContent->IsNodeOfType(nsINode::ePROCESSING_INSTRUCTION))
    return;

  nsRefPtr<nsStyleContext> styleContext;
  styleContext = ResolveStyleContext(aParentFrame, aContent);

  AddFrameConstructionItemsInternal(aState, aContent, aParentFrame,
                                    aContent->Tag(), aContent->GetNameSpaceID(),
                                    styleContext,
                                    ITEM_ALLOW_XBL_BASE | ITEM_ALLOW_PAGE_BREAK,
                                    aItems);
}


void
nsCSSFrameConstructor::AddFrameConstructionItemsInternal(nsFrameConstructorState& aState,
                                                         nsIContent* aContent,
                                                         nsIFrame* aParentFrame,
                                                         nsIAtom* aTag,
                                                         PRInt32 aNameSpaceID,
                                                         nsStyleContext* aStyleContext,
                                                         PRUint32 aFlags,
                                                         FrameConstructionItemList& aItems)
{
  
  
  
  const nsStyleDisplay* display = aStyleContext->GetStyleDisplay();
  nsRefPtr<nsStyleContext> styleContext(aStyleContext);
  nsAutoEnqueueBinding binding(mDocument);
  if ((aFlags & ITEM_ALLOW_XBL_BASE) && display->mBinding)
  {
    

    nsIXBLService * xblService = GetXBLService();
    if (!xblService)
      return;

    PRBool resolveStyle;

    nsresult rv = xblService->LoadBindings(aContent, display->mBinding->mURI,
                                           display->mBinding->mOriginPrincipal,
                                           PR_FALSE,
                                           getter_AddRefs(binding.mBinding),
                                           &resolveStyle);
    if (NS_FAILED(rv))
      return;

    if (resolveStyle) {
      styleContext = ResolveStyleContext(styleContext->GetParent(), aContent);
      display = styleContext->GetStyleDisplay();
      aStyleContext = styleContext;
    }

    aTag = mDocument->BindingManager()->ResolveTag(aContent, &aNameSpaceID);
  }

  
  
  if (NS_STYLE_DISPLAY_NONE == display->mDisplay) {
    aState.mFrameManager->SetUndisplayedContent(aContent, styleContext);
    return;
  }

  PRBool isText = aContent->IsNodeOfType(nsINode::eTEXT);
  PRBool isPopup = PR_FALSE;
  
  const FrameConstructionData* data;
  if (isText) {
    data = FindTextData(aParentFrame);
#ifdef MOZ_SVG
    if (!data) {
      
      return;
    }
#endif 
  } else {
#ifdef MOZ_SVG
    
    if (aNameSpaceID != kNameSpaceID_SVG &&
        aParentFrame &&
        aParentFrame->IsFrameOfType(nsIFrame::eSVG) &&
        !aParentFrame->IsFrameOfType(nsIFrame::eSVGForeignObject)
        ) {
      return;
    }
#endif 

    data = FindHTMLData(aContent, aTag, aNameSpaceID, aParentFrame,
                        styleContext);
    if (!data) {
      data = FindXULTagData(aContent, aTag, aNameSpaceID, styleContext);
    }
#ifdef MOZ_MATHML
    if (!data) {
      data = FindMathMLData(aContent, aTag, aNameSpaceID, styleContext);
    }
#endif
#ifdef MOZ_SVG
    if (!data) {
      data = FindSVGData(aContent, aTag, aNameSpaceID, aParentFrame,
                         styleContext);
    }
#endif 

    
    if (!data) {
      data = FindXULDisplayData(display, aContent, styleContext);
    }

    
    if (!data) {
      data = FindDisplayData(display, aContent, styleContext);
    }

    NS_ASSERTION(data, "Should have frame construction data now");

    if (data->mBits & FCDATA_SUPPRESS_FRAME) {
      return;
    }

#ifdef MOZ_XUL
    if ((data->mBits & FCDATA_IS_POPUP) &&
        (!aParentFrame || 
         aParentFrame->GetType() != nsGkAtoms::menuFrame)) {
      if (!aState.mPopupItems.containingBlock &&
          !aState.mHavePendingPopupgroup) {
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

  PRBool isGeneratedContent = ((aFlags & ITEM_IS_GENERATED_CONTENT) != 0);

  FrameConstructionItem* item =
    aItems.AppendItem(data, aContent, aTag, aNameSpaceID,
                      styleContext.forget());
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
  } else {
    item->mIsAllInline = item->mHasInlineEnds =
      
      
      
      ((bits & FCDATA_IS_TABLE_PART) &&
       (!aParentFrame || 
        aParentFrame->GetStyleDisplay()->mDisplay == NS_STYLE_DISPLAY_INLINE)) ||
      
      display->IsInlineOutside() ||
      
      
      
      
      
      
      
      
      
      
      (!(bits & FCDATA_DISALLOW_OUT_OF_FLOW) &&
       aState.GetGeometricParent(display, nsnull)) ||
      
      isPopup;
  }

  if (item->mIsAllInline) {
    aItems.InlineItemAdded();
  }

  if (bits & FCDATA_IS_LINE_PARTICIPANT) {
    aItems.LineParticipantItemAdded();
  }
}

static void DestroyContent(void *aObject,
                           nsIAtom *aPropertyName,
                           void *aPropertyValue,
                           void *aData)
{
  nsIContent* content = static_cast<nsIContent*>(aPropertyValue);
  content->UnbindFromTree();
  NS_RELEASE(content);
}

nsresult
nsCSSFrameConstructor::ConstructFramesFromItem(nsFrameConstructorState& aState,
                                               FrameConstructionItem& aItem,
                                               nsIFrame* aParentFrame,
                                               nsFrameItems& aFrameItems)
{
  nsIFrame* adjParentFrame = aParentFrame;
  nsFrameItems* frameItems = &aFrameItems;
  PRBool pseudoParent = PR_FALSE;
  nsStyleContext* styleContext = aItem.mStyleContext;
  nsFrameConstructorSaveState pseudoSaveState;
  nsresult rv = AdjustParentFrame(aState, aItem.mContent, adjParentFrame,
                                  aItem.mFCData, aItem.mNameSpaceID,
                                  styleContext, frameItems, pseudoSaveState,
                                  pseudoParent);
  if (NS_FAILED(rv)) {
    return rv;
  }

  if (aItem.mIsText) {
    return ConstructTextFrame(aItem.mFCData, aState, aItem.mContent,
                              adjParentFrame, styleContext,
                              *frameItems, pseudoParent);
  }

  
  
  
  
  
  {
    if (styleContext->GetStyleVisibility()->mDirection ==
        NS_STYLE_DIRECTION_RTL)
      aState.mPresContext->SetBidiEnabled();
  }
  
  
  {
    styleContext->GetStyleBackground();
  }

  nsFrameState savedStateBits = aState.mAdditionalStateBits;
  if (aItem.mIsGeneratedContent) {
    
    
    aState.mAdditionalStateBits |= NS_FRAME_GENERATED_CONTENT;

    aParentFrame->SetProperty(styleContext->GetPseudoType(),
                              aItem.mContent, DestroyContent);

    
    
    aItem.mIsGeneratedContent = PR_FALSE;
  }

  
  rv = ConstructFrameFromItemInternal(aItem, aState, adjParentFrame,
                                      *frameItems, pseudoParent);

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
  AUTO_LAYOUT_PHASE_ENTRY_POINT(mPresShell->GetPresContext(), FrameC);
  return ReconstructDocElementHierarchyInternal();
}

nsresult
nsCSSFrameConstructor::ReconstructDocElementHierarchyInternal()
{
#ifdef DEBUG
  if (gNoisyContentUpdates) {
    printf("nsCSSFrameConstructor::ReconstructDocElementHierarchy\n");
  }
#endif

  nsresult rv = NS_OK;

  
  if (mDocument && mPresShell) {
    nsIContent *rootContent = mDocument->GetRootContent();
    
    if (rootContent) {
      nsFrameConstructorState state(mPresShell, mFixedContainingBlock,
                                    nsnull, nsnull, mTempFrameTreeState);

      
      
      CaptureStateFor(state.mFrameManager->GetRootFrame(), mTempFrameTreeState);

      
      nsIFrame* docElementFrame =
        state.mFrameManager->GetPrimaryFrameFor(rootContent, -1);

      if (docElementFrame) {
        
        
        ::DeletingFrameSubtree(state.mFrameManager, docElementFrame);
      } else {
        state.mFrameManager->ClearUndisplayedContentIn(rootContent, nsnull);
      }

      
      
      
      
      rv = RemoveFixedItems(state, docElementFrame);

      if (NS_SUCCEEDED(rv)) {
        nsPlaceholderFrame* placeholderFrame = nsnull;
        if (docElementFrame &&
            (docElementFrame->GetStateBits() & NS_FRAME_OUT_OF_FLOW)) {
          
          
          placeholderFrame =
            state.mFrameManager->GetPlaceholderFrameFor(docElementFrame);
          NS_ASSERTION(placeholderFrame, "No placeholder for out-of-flow?");
        }

        
        
        state.mFrameManager->ClearPrimaryFrameMap();
        state.mFrameManager->ClearPlaceholderFrameMap();
        state.mFrameManager->ClearUndisplayedContentMap();

        if (docElementFrame) {
          
          

          
          
          
          PRBool wasDestroyingFrameTree = mIsDestroyingFrameTree;
          WillDestroyFrameTree(PR_FALSE);

          rv = state.mFrameManager->RemoveFrame(docElementFrame->GetParent(),
                    GetChildListNameFor(docElementFrame), docElementFrame);
          
          if (placeholderFrame) {
            
            
            rv |= state.mFrameManager->RemoveFrame(placeholderFrame->GetParent(),
                                            nsnull, placeholderFrame);
          }

          mIsDestroyingFrameTree = wasDestroyingFrameTree;
          if (NS_FAILED(rv)) {
            return rv;
          }
        }
      }
    }
    
    if (rootContent && NS_SUCCEEDED(rv)) {
      mRootElementFrame = nsnull;
      mRootElementStyleFrame = nsnull;

      
      
      nsFrameConstructorState state(mPresShell, mFixedContainingBlock,
                                    nsnull, nsnull, mTempFrameTreeState);

      
      nsIFrame* newChild;
      rv = ConstructDocElementFrame(state, rootContent,
                                    mDocElementContainingBlock, &newChild);

      
      if (NS_SUCCEEDED(rv) && newChild) {
        rv = state.mFrameManager->InsertFrames(mDocElementContainingBlock,
                                               nsnull, nsnull, newChild);
      }
    }
  }

  return rv;
}


nsIFrame*
nsCSSFrameConstructor::GetFrameFor(nsIContent* aContent)
{
  
  nsIFrame* frame = mPresShell->GetPrimaryFrameFor(aContent);

  if (!frame)
    return nsnull;

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

    if (disp->IsPositioned() && !IsTableRelated(frame->GetType(), PR_TRUE)) {
      
      for (nsIFrame* wrappedFrame = aFrame; wrappedFrame != frame->GetParent();
           wrappedFrame = wrappedFrame->GetParent()) {
        nsIAtom* frameType = wrappedFrame->GetType();
        if (nsGkAtoms::blockFrame == frameType ||
#ifdef MOZ_XUL
            nsGkAtoms::XULLabelFrame == frameType ||
#endif
            nsGkAtoms::positionedInlineFrame == frameType) {
          containingBlock = wrappedFrame;
        } else if (nsGkAtoms::fieldSetFrame == frameType) {
          
          
          containingBlock = GetFieldSetBlockFrame(wrappedFrame);
        }
      }

#ifdef DEBUG
      if (!containingBlock)
        NS_WARNING("Positioned frame that does not handle positioned kids; looking further up the parent chain");
#endif
    }
  }

  
  
  if (containingBlock)
    return AdjustAbsoluteContainingBlock(containingBlock);

  
  return mHasRootAbsPosContainingBlock ? mDocElementContainingBlock : nsnull;
}

nsIFrame*
nsCSSFrameConstructor::GetFloatContainingBlock(nsIFrame* aFrame)
{
  NS_PRECONDITION(mRootElementFrame, "no root element frame");
  
  
  
  
  
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
                                    nsCSSPseudoElements::after,
                                    aPresContext)) {
    nsIFrame* afterFrame = nsLayoutUtils::GetAfterFrame(aParentFrame);
    if (afterFrame) {
      *aAfterFrame = afterFrame;
      return afterFrame->GetParent();
    }
  }

  *aAfterFrame = nsnull;
  return aParentFrame;
}







nsresult
nsCSSFrameConstructor::AppendFrames(nsFrameConstructorState&       aState,
                                    nsIContent*                    aContainer,
                                    nsIFrame*                      aParentFrame,
                                    nsFrameItems&                  aFrameList,
                                    nsIFrame*                      aAfterFrame)
{
#ifdef DEBUG
  nsIFrame* debugAfterFrame;
  nsIFrame* debugNewParent =
    ::AdjustAppendParentForAfterContent(aState.mPresContext, aContainer,
                                        aParentFrame, &debugAfterFrame);
  NS_ASSERTION(debugNewParent == aParentFrame, "Incorrect parent");
  NS_ASSERTION(debugAfterFrame == aAfterFrame, "Incorrect after frame");
#endif

  nsFrameManager* frameManager = aState.mFrameManager;
  if (aAfterFrame) {
    NS_ASSERTION(!IsFrameSpecial(aParentFrame) ||
                 IsInlineFrame(aParentFrame) ||
                 !IsInlineOutside(aAfterFrame),
                 "Shouldn't have inline :after content on the block in an "
                 "{ib} split");
    nsFrameList frames(aParentFrame->GetFirstChild(nsnull));

    
    return frameManager->InsertFrames(aParentFrame, nsnull,
                                      frames.GetPrevSiblingFor(aAfterFrame),
                                      aFrameList.childList);
  }

  if (IsFrameSpecial(aParentFrame) &&
      !IsInlineFrame(aParentFrame) &&
      IsInlineOutside(aFrameList.lastChild)) {
    NS_ASSERTION(!aParentFrame->GetNextContinuation() ||
                 !aParentFrame->GetNextContinuation()->GetFirstChild(nsnull),
                 "Shouldn't happen");
    
    
    nsIFrame* lastBlock = FindLastBlock(aFrameList.childList);
    nsIFrame* firstTrailingInline;
    if (lastBlock) {
      firstTrailingInline = lastBlock->GetNextSibling();
      lastBlock->SetNextSibling(nsnull);
      aFrameList.lastChild = lastBlock;
    } else {
      firstTrailingInline = aFrameList.childList;
      aFrameList = nsFrameItems();
    }

    NS_ASSERTION(firstTrailingInline, "How did that happen?");

    nsIFrame* inlineSibling = GetSpecialSibling(aParentFrame);
    NS_ASSERTION(inlineSibling, "How did that happen?");

    nsIFrame* stateParent = inlineSibling->GetParent();

    nsFrameConstructorState targetState(mPresShell, mFixedContainingBlock,
                                        GetAbsoluteContainingBlock(stateParent),
                                        GetFloatContainingBlock(stateParent));

    MoveFramesToEndOfIBSplit(aState, inlineSibling, firstTrailingInline,
                             aParentFrame, &targetState);
  }
    
  if (!aFrameList.childList) {
    
    return NS_OK;
  }
  
  return frameManager->AppendFrames(aParentFrame, nsnull,
                                    aFrameList.childList);
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
      styleContext = ResolveStyleContext(styleParent, aContent);
      if (!styleContext) return PR_FALSE;
      const nsStyleDisplay* display = styleContext->GetStyleDisplay();
      aDisplay = display->mDisplay;
    }
    if (nsGkAtoms::menuFrame == parentType) {
      return
        (NS_STYLE_DISPLAY_POPUP == aDisplay) ==
        (NS_STYLE_DISPLAY_POPUP == siblingDisplay);
    }
    switch (siblingDisplay) {
    case NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP:
      return (NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP == aDisplay);
    case NS_STYLE_DISPLAY_TABLE_COLUMN:
      return (NS_STYLE_DISPLAY_TABLE_COLUMN == aDisplay);
    case NS_STYLE_DISPLAY_TABLE_CAPTION:
      return (NS_STYLE_DISPLAY_TABLE_CAPTION == aDisplay);
    default: 
      return (NS_STYLE_DISPLAY_TABLE_HEADER_GROUP == aDisplay) ||
             (NS_STYLE_DISPLAY_TABLE_ROW_GROUP    == aDisplay) ||
             (NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP == aDisplay) ||
             (NS_STYLE_DISPLAY_TABLE_CAPTION      == aDisplay);
    }
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
  nsIFrame* sibling = mPresShell->GetPrimaryFrameFor(aContent);
  if (!sibling) {
    return nsnull;
  }

  
  
  if (sibling->GetStateBits() & NS_FRAME_OUT_OF_FLOW) {
    nsIFrame* placeholderFrame;
    mPresShell->GetPlaceholderFrameFor(sibling, &placeholderFrame);
    NS_ASSERTION(placeholderFrame, "no placeholder for out-of-flow frame");
    sibling = placeholderFrame;
  }

  
  NS_ASSERTION(!sibling->GetPrevContinuation(), "How did that happen?");

  if (aPrevSibling) {
    
    
    if (IsFrameSpecial(sibling)) {
      sibling = GetLastSpecialSibling(sibling, PR_FALSE);
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
                                           ChildIterator aIter)
{
  nsIContent* child = *aIter;

  PRUint8 childDisplay = UNSET_DISPLAY;
  
  
  while (aIter-- != aFirst) {
    nsIFrame* prevSibling =
      FindFrameForContentSibling(*aIter, child, childDisplay, PR_TRUE);

    if (prevSibling) {
      
      return prevSibling;
    }
  }

  return nsnull;
}

nsIFrame*
nsCSSFrameConstructor::FindNextSibling(ChildIterator aIter,
                                       const ChildIterator& aLast)
{
  if (aIter == aLast) {
    
    
    return nsnull;
  }

  nsIContent* child = *aIter;
  PRUint8 childDisplay = UNSET_DISPLAY;

  while (++aIter != aLast) {
    nsIFrame* nextSibling =
      FindFrameForContentSibling(*aIter, child, childDisplay, PR_FALSE);

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

static void
InvalidateCanvasIfNeeded(nsIFrame* aFrame);

static PRBool
IsSpecialFramesetChild(nsIContent* aContent)
{
  
  return aContent->IsNodeOfType(nsINode::eHTML) &&
    (aContent->Tag() == nsGkAtoms::frameset ||
     aContent->Tag() == nsGkAtoms::frame);
}

nsresult
nsCSSFrameConstructor::ContentAppended(nsIContent*     aContainer,
                                       PRInt32         aNewIndexInContainer)
{
  AUTO_LAYOUT_PHASE_ENTRY_POINT(mPresShell->GetPresContext(), FrameC);
  NS_PRECONDITION(mUpdateCount != 0,
                  "Should be in an update while creating frames");

#ifdef DEBUG
  if (gNoisyContentUpdates) {
    printf("nsCSSFrameConstructor::ContentAppended container=%p index=%d\n",
           static_cast<void*>(aContainer), aNewIndexInContainer);
    if (gReallyNoisyContentUpdates && aContainer) {
      aContainer->List(stdout, 0);
    }
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

  
  
  
  
  nsIFrame* insertionPoint;
  PRBool multiple = PR_FALSE;
  GetInsertionPoint(parentFrame, nsnull, &insertionPoint, &multiple);
  if (! insertionPoint)
    return NS_OK; 

  PRBool hasInsertion = PR_FALSE;
  if (!multiple) {
    nsIDocument* document = nsnull; 
    nsIContent *firstAppendedChild =
      aContainer->GetChildAt(aNewIndexInContainer);
    if (firstAppendedChild) {
      document = firstAppendedChild->GetDocument();
    }
    if (document &&
        document->BindingManager()->GetInsertionParent(firstAppendedChild)) {
      hasInsertion = PR_TRUE;
    }
  }
  
  if (multiple || hasInsertion) {
    
    
    PRUint32 childCount = 0;
      
    if (!multiple) {
      
      
      
      
      
      
      
      
      
      
      childCount = insertionPoint->GetContent()->GetChildCount();
    }

    if (multiple || childCount > 0) {
      
      
      
      
      
      PRUint32 containerCount = aContainer->GetChildCount();
      for (PRUint32 i = aNewIndexInContainer; i < containerCount; i++) {
        LAYOUT_PHASE_TEMP_EXIT();
        
        ContentInserted(aContainer, aContainer->GetChildAt(i), i,
                        mTempFrameTreeState);
        LAYOUT_PHASE_TEMP_REENTER();
      }

      return NS_OK;
    }
  }

  parentFrame = insertionPoint;

  if (parentFrame->GetType() == nsGkAtoms::frameSetFrame) {
    
    PRUint32 count = aContainer->GetChildCount();
    for (PRUint32 i = aNewIndexInContainer; i < count; ++i) {
      if (IsSpecialFramesetChild(aContainer->GetChildAt(i))) {
        
        return RecreateFramesForContent(parentFrame->GetContent());
      }
    }
  }
  
  if (parentFrame->IsLeaf()) {
    
    return NS_OK;
  }
  
#ifdef MOZ_MATHML
  if (parentFrame->IsFrameOfType(nsIFrame::eMathML))
    return RecreateFramesForContent(parentFrame->GetContent());
#endif

  
  
  
  if (IsFrameSpecial(parentFrame)) {
#ifdef DEBUG
    if (gNoisyContentUpdates) {
      printf("nsCSSFrameConstructor::ContentAppended: parentFrame=");
      nsFrame::ListTag(stdout, parentFrame);
      printf(" is special\n");
    }
#endif

    
    
    
    parentFrame = GetLastSpecialSibling(parentFrame, PR_TRUE);
  }

  
  parentFrame = nsLayoutUtils::GetLastContinuationWithChild(parentFrame);

  nsIAtom* frameType = parentFrame->GetType();
  
  
  NS_ASSERTION(frameType != nsGkAtoms::fieldSetFrame,
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

  FrameConstructionItemList items;
  for (PRUint32 i = aNewIndexInContainer, count = aContainer->GetChildCount();
       i < count;
       ++i) {
    AddFrameConstructionItems(state, aContainer->GetChildAt(i), parentFrame,
                              items);
  }

  
  
  
  
  if (WipeContainingBlock(state, containingBlock, parentFrame, items,
                          !parentAfterFrame, nsnull)) {
    return NS_OK;
  }

  
  
  nsFrameItems frameItems;
  nsFrameItems captionItems;

  for (FCItemIterator iter(items); !iter.IsDone(); iter.Next()) {
    nsresult rv =
      ConstructFramesFromItem(state, iter, parentFrame, frameItems);
    if (NS_FAILED(rv)) {
      break;
    }

    
    
    
    
    if (frameItems.lastChild)
      InvalidateCanvasIfNeeded(frameItems.lastChild);
  }

  if (nsGkAtoms::tableFrame == frameType) {
    
    
    
    
    PullOutCaptionFrames(frameItems, captionItems);
  }
  

  
  if (!state.mPseudoFrames.IsEmpty()) {
    ProcessPseudoFrames(state, frameItems);
  }

  if (haveFirstLineStyle && parentFrame == containingBlock) {
    
    
    AppendFirstLineFrames(state, containingBlock->GetContent(),
                          containingBlock, frameItems); 
  }

  nsresult result = NS_OK;

  
  if (NS_SUCCEEDED(result) &&
      (frameItems.childList || captionItems.childList)) {
    
    if (nsGkAtoms::tableFrame == frameType) {
      if (captionItems.childList) { 
        nsIFrame* outerTable = parentFrame->GetParent();
        if (outerTable) { 
          state.mFrameManager->AppendFrames(outerTable,
                                            nsGkAtoms::captionList,
                                            captionItems.childList);
        }
      }
      if (frameItems.childList) { 
        AppendFrames(state, aContainer, parentFrame, frameItems,
                     parentAfterFrame);
      }
    }
    else {
      AppendFrames(state, aContainer, parentFrame, frameItems,
                   parentAfterFrame);
    }
  }

  
  if (haveFirstLetterStyle) {
    RecoverLetterFrames(containingBlock);
  }

#ifdef DEBUG
  if (gReallyNoisyContentUpdates) {
    nsIFrameDebug* fdbg = do_QueryFrame(parentFrame);
    if (fdbg) {
      printf("nsCSSFrameConstructor::ContentAppended: resulting frame model:\n");
      fdbg->List(stdout, 0);
    }
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
                         PRInt32            aIndexInContainer,
                         nsIDocument*       aDocument,                         
                         nsIFrame*          aChildFrame,
                         content_operation  aOperation)
{
  if (!aContainer)
    return PR_FALSE;

  if (aContainer->IsNodeOfType(nsINode::eXUL) &&
      aChild->IsNodeOfType(nsINode::eXUL) &&
      aContainer->Tag() == nsGkAtoms::listbox &&
      aChild->Tag() == nsGkAtoms::listitem) {
    nsCOMPtr<nsIDOMXULElement> xulElement = do_QueryInterface(aContainer);
    nsCOMPtr<nsIBoxObject> boxObject;
    xulElement->GetBoxObject(getter_AddRefs(boxObject));
    nsCOMPtr<nsPIListBoxObject> listBoxObject = do_QueryInterface(boxObject);
    if (listBoxObject) {
      nsListBoxBodyFrame* listBoxBodyFrame = listBoxObject->GetListBoxBody(PR_FALSE);
      if (listBoxBodyFrame) {
        if (aOperation == CONTENT_REMOVED) {
          
          
          if (!aChildFrame || aChildFrame->GetParent() == listBoxBodyFrame) {
            listBoxBodyFrame->OnContentRemoved(aPresContext, aChildFrame,
                                               aIndexInContainer);
            return PR_TRUE;
          }
        } else {
          
          
          
          listBoxBodyFrame->OnContentInserted(aPresContext, aChild);
          return PR_TRUE;
        }
      }
    }
  }

  return PR_FALSE;
}
#endif 

nsresult
nsCSSFrameConstructor::ContentInserted(nsIContent*            aContainer,
                                       nsIContent*            aChild,
                                       PRInt32                aIndexInContainer,
                                       nsILayoutHistoryState* aFrameState)
{
  AUTO_LAYOUT_PHASE_ENTRY_POINT(mPresShell->GetPresContext(), FrameC);
  NS_PRECONDITION(mUpdateCount != 0,
                  "Should be in an update while creating frames");

  
  
#ifdef DEBUG
  if (gNoisyContentUpdates) {
    printf("nsCSSFrameConstructor::ContentInserted container=%p child=%p index=%d\n",
           static_cast<void*>(aContainer),
           static_cast<void*>(aChild),
           aIndexInContainer);
    if (gReallyNoisyContentUpdates) {
      (aContainer ? aContainer : aChild)->List(stdout, 0);
    }
  }
#endif

  nsresult rv = NS_OK;

#ifdef MOZ_XUL
  
  
  if (NotifyListBoxBody(mPresShell->GetPresContext(), aContainer, aChild,
                        aIndexInContainer, 
                        mDocument, nsnull, CONTENT_INSERTED))
    return NS_OK;
#endif 
  
  
  
  if (! aContainer) {
    nsIContent *docElement = mDocument->GetRootContent();

    if (aChild == docElement) {
      NS_PRECONDITION(nsnull == mRootElementFrame, "root element frame already created");
      
      if (!mDocElementContainingBlock)
        return NS_OK; 
                      
                      
                      

      
      nsIFrame*               docElementFrame;
      nsFrameConstructorState state(mPresShell, mFixedContainingBlock, nsnull,
                                    nsnull, aFrameState);
      rv = ConstructDocElementFrame(state,
                                    docElement, 
                                    mDocElementContainingBlock,
                                    &docElementFrame);
    
      if (NS_SUCCEEDED(rv) && docElementFrame) {
        if (mDocElementContainingBlock->GetStateBits() & NS_FRAME_FIRST_REFLOW) {
          
          
          mDocElementContainingBlock->SetInitialChildList(nsnull, 
                                                          docElementFrame);
        } else {
          
          
          
          NS_ASSERTION(mDocElementContainingBlock->GetFirstChild(nsnull) == nsnull,
                       "Unexpected child of document element containing block");
          mDocElementContainingBlock->AppendFrames(nsnull, docElementFrame);
        }
        InvalidateCanvasIfNeeded(docElementFrame);
#ifdef DEBUG
        if (gReallyNoisyContentUpdates) {
          nsIFrameDebug* fdbg = do_QueryFrame(docElementFrame);
          if (fdbg) {
            printf("nsCSSFrameConstructor::ContentInserted: resulting frame model:\n");
            fdbg->List(stdout, 0);
          }
        }
#endif
      }
    }

    
    
    return NS_OK;
  }

  
  nsIFrame* parentFrame = GetFrameFor(aContainer);
  if (! parentFrame)
    return NS_OK; 
    

  
  
  
  nsIFrame* insertionPoint;
  GetInsertionPoint(parentFrame, aChild, &insertionPoint);
  if (! insertionPoint)
    return NS_OK; 

  parentFrame = insertionPoint;

  
  
  
  
  nsIContent* container = parentFrame->GetContent();

  ChildIterator first, last;
  ChildIterator::Init(container, &first, &last);
  ChildIterator iter(first);
  if (iter.XBLInvolved() || container != aContainer) {
    iter.seek(aChild);
    
    
    
    
  } else if (aIndexInContainer != -1) {
    
    
    
    iter.seek(aIndexInContainer);
    NS_ASSERTION(*iter == aChild, "Someone screwed up the indexing");
  }
#ifdef DEBUG
  else {
    NS_WARNING("Someone passed native anonymous content directly into frame "
               "construction.  Stop doing that!");
  }
#endif
  
  nsIFrame* prevSibling = FindPreviousSibling(first, iter);

  PRBool    isAppend = PR_FALSE;
  nsIFrame* appendAfterFrame;  

  
  
  
  if (prevSibling) {
    parentFrame = prevSibling->GetParent()->GetContentInsertionFrame();
  }
  else {
    
    nsIFrame* nextSibling = FindNextSibling(iter, last);

    if (nextSibling) {
      parentFrame = nextSibling->GetParent()->GetContentInsertionFrame();
    }
    else {
      
      isAppend = PR_TRUE;
      
      parentFrame = nsLayoutUtils::GetLastContinuationWithChild(parentFrame);
      
      parentFrame = ::GetAdjustedParentFrame(parentFrame,
                                             parentFrame->GetType(),
                                             aChild);
      parentFrame =
        ::AdjustAppendParentForAfterContent(mPresShell->GetPresContext(),
                                            container, parentFrame,
                                            &appendAfterFrame);
    }
  }

  if (parentFrame->GetType() == nsGkAtoms::frameSetFrame &&
      IsSpecialFramesetChild(aChild)) {
    
    return RecreateFramesForContent(parentFrame->GetContent());
  }
  
  
  if (parentFrame->IsLeaf()) {
    return NS_OK;
  }

#ifdef MOZ_MATHML
  if (parentFrame->IsFrameOfType(nsIFrame::eMathML))
    return RecreateFramesForContent(parentFrame->GetContent());
#endif

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
        parentFrame = parentFrame->GetParent();
        container = parentFrame->GetContent();
      }

      
      RemoveLetterFrames(state.mPresContext, mPresShell,
                         state.mFrameManager,
                         state.mFloatedItems.containingBlock);

      
      
      ChildIterator::Init(container, &first, &last);
      if (last.XBLInvolved() || container != aContainer) {
        last.seek(aChild);
      } else if (aIndexInContainer != -1) {
        last.seek(aIndexInContainer);
        NS_ASSERTION(*iter == aChild, "Someone screwed up the indexing");
      }

      prevSibling = FindPreviousSibling(first, last);
    }
  }

  if (!prevSibling) {
    
    
    nsIFrame* firstChild = parentFrame->GetFirstChild(nsnull);

    if (firstChild &&
        nsLayoutUtils::IsGeneratedContentFor(container, firstChild,
                                             nsCSSPseudoElements::before)) {
      
      prevSibling = firstChild->GetTailContinuation();
      parentFrame = prevSibling->GetParent();
      
      
      
      
      isAppend = PR_FALSE;
    }
  }

  FrameConstructionItemList items;
  AddFrameConstructionItems(state, aChild, parentFrame, items);

  
  
  
  
  if (WipeContainingBlock(state, containingBlock, parentFrame, items,
                          isAppend && !appendAfterFrame, prevSibling))
    return NS_OK;


  
  
  
  nsFrameItems frameItems, captionItems;
  for (FCItemIterator iter(items); !iter.IsDone(); iter.Next()) {
    ConstructFramesFromItem(state, iter, parentFrame, frameItems);
  }
  if (frameItems.childList) {
    InvalidateCanvasIfNeeded(frameItems.childList);
    
    if (nsGkAtoms::tableCaptionFrame == frameItems.childList->GetType()) {
      NS_ASSERTION(frameItems.childList == frameItems.lastChild ,
                   "adding a non caption frame to the caption childlist?");
      captionItems.AddChild(frameItems.childList);
      frameItems = nsFrameItems();
    }
  }

  
  if (!state.mPseudoFrames.IsEmpty())
    ProcessPseudoFrames(state, frameItems);

  
  
  
  
  
  
  
  
  
  
  
  
  if (prevSibling && frameItems.childList &&
      frameItems.childList->GetParent() != prevSibling->GetParent()) {
#ifdef DEBUG
    nsIFrame* frame1 = frameItems.childList->GetParent();
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
    prevSibling = nsnull;
    isAppend = PR_TRUE;
    parentFrame =
      ::AdjustAppendParentForAfterContent(mPresShell->GetPresContext(),
                                          container,
                                          frameItems.childList->GetParent(),
                                          &appendAfterFrame);
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
      
  nsIFrame* const newFrame = frameItems.childList;
  if (NS_SUCCEEDED(rv) && newFrame) {
    NS_ASSERTION(!captionItems.childList, "leaking caption frames");
    
    if (isAppend) {
      AppendFrames(state, container, parentFrame, frameItems,
                   appendAfterFrame);
    } else {
      state.mFrameManager->InsertFrames(parentFrame,
                                        nsnull, prevSibling, newFrame);
    }
  }
  else {
    
    nsIFrame* newCaptionFrame = captionItems.childList;
    if (NS_SUCCEEDED(rv) && newCaptionFrame) {
      nsIFrame* outerTableFrame;
      if (GetCaptionAdjustedParent(parentFrame, newCaptionFrame, &outerTableFrame)) {
        
        
        
        if (prevSibling && prevSibling->GetParent() != outerTableFrame) {
          prevSibling = nsnull;
        }
        
        
        
        NS_ASSERTION(nsGkAtoms::tableOuterFrame == outerTableFrame->GetType(),
                     "Pseudo frame construction failure, "
                     "a caption can be only a child of a outer table frame");
        if (isAppend) {
          state.mFrameManager->AppendFrames(outerTableFrame,
                                            nsGkAtoms::captionList,
                                            newCaptionFrame);
        }
        else {
          state.mFrameManager->InsertFrames(outerTableFrame,
                                            nsGkAtoms::captionList,
                                            prevSibling, newCaptionFrame);
        }
      }
    }
  }

  if (haveFirstLetterStyle) {
    
    
    RecoverLetterFrames(state.mFloatedItems.containingBlock);
  }

#ifdef DEBUG
  if (gReallyNoisyContentUpdates && parentFrame) {
    nsIFrameDebug* fdbg = do_QueryFrame(parentFrame);
    if (fdbg) {
      printf("nsCSSFrameConstructor::ContentInserted: resulting frame model:\n");
      fdbg->List(stdout, 0);
    }
  }
#endif

  return NS_OK;
}

nsresult
nsCSSFrameConstructor::ReinsertContent(nsIContent* aContainer,
                                       nsIContent* aChild)
{
  PRInt32 ix = aContainer->IndexOf(aChild);
  
  
  
  
  PRBool didReconstruct;
  nsresult res = ContentRemoved(aContainer, aChild, ix, &didReconstruct);

  if (NS_SUCCEEDED(res) && !didReconstruct) {
    
    
    res = ContentInserted(aContainer, aChild, ix, nsnull);
  }

  return res;
}

static void
DoDeletingFrameSubtree(nsFrameManager*      aFrameManager,
                       nsTArray<nsIFrame*>& aDestroyQueue,
                       nsIFrame*            aRemovedFrame,
                       nsIFrame*            aFrame);

static void
DoDeletingOverflowContainers(nsFrameManager*      aFrameManager,
                             nsTArray<nsIFrame*>& aDestroyQueue,
                             nsIFrame*            aRemovedFrame,
                             nsIFrame*            aFrame)
{
  
  
  
  
  
  const PRBool orphanSubtree = aRemovedFrame == aFrame;
  for (nsIFrame* next = aFrame->GetNextContinuation();
       next && (next->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER);
       next = next->GetNextContinuation()) {
    DoDeletingFrameSubtree(aFrameManager, aDestroyQueue,
                           orphanSubtree ? next : aRemovedFrame,
                           next);
  }
}
























static void
DoDeletingFrameSubtree(nsFrameManager*      aFrameManager,
                       nsTArray<nsIFrame*>& aDestroyQueue,
                       nsIFrame*            aRemovedFrame,
                       nsIFrame*            aFrame)
{
#undef RECURSE
#define RECURSE(top, child)                                                  \
  DoDeletingFrameSubtree(aFrameManager, aDestroyQueue, (top), (child));      \
  DoDeletingOverflowContainers(aFrameManager, aDestroyQueue, (top), (child));

  
  nsIContent* content = aFrame->GetContent();
  if (content) {
    aFrameManager->RemoveAsPrimaryFrame(content, aFrame);
    aFrameManager->ClearAllUndisplayedContentIn(content);
  }

  nsIAtom* childListName = nsnull;
  PRInt32 childListIndex = 0;

  do {
    
    nsIFrame* childFrame = aFrame->GetFirstChild(childListName);
    for (; childFrame; childFrame = childFrame->GetNextSibling()) {
      NS_ASSERTION(!(childFrame->GetStateBits() & NS_FRAME_OUT_OF_FLOW),
                   "out-of-flow on wrong child list");
      if (NS_LIKELY(nsGkAtoms::placeholderFrame != childFrame->GetType())) {
        RECURSE(aRemovedFrame, childFrame);
      } else {
        nsIFrame* outOfFlowFrame =
          nsPlaceholderFrame::GetRealFrameForPlaceholder(childFrame);
  
        
        aFrameManager->UnregisterPlaceholderFrame((nsPlaceholderFrame*)childFrame);
        
        
        
        
        
        
        
        if (outOfFlowFrame->GetStyleDisplay()->mDisplay == NS_STYLE_DISPLAY_POPUP ||
            !nsLayoutUtils::IsProperAncestorFrame(aRemovedFrame, outOfFlowFrame)) {
          NS_ASSERTION(aDestroyQueue.IndexOf(outOfFlowFrame) == kNotFound,
                       "out-of-flow is already in the destroy queue");
          aDestroyQueue.AppendElement(outOfFlowFrame);
          
          RECURSE(outOfFlowFrame, outOfFlowFrame);
        }
        else {
          
          
          RECURSE(aRemovedFrame, outOfFlowFrame);
        }
      }
    }

    
    
    
    do {
      childListName = aFrame->GetAdditionalChildListName(childListIndex++);
    } while (IsOutOfFlowList(childListName) ||
             childListName == nsGkAtoms::overflowContainersList ||
             childListName == nsGkAtoms::excessOverflowContainersList);
  } while (childListName);
}





static nsresult
DeletingFrameSubtree(nsFrameManager* aFrameManager,
                     nsIFrame*       aFrame)
{
  NS_ENSURE_TRUE(aFrame, NS_OK); 

  
  
  if (NS_UNLIKELY(!aFrameManager)) {
    return NS_OK;
  }

  nsAutoTArray<nsIFrame*, 8> destroyQueue;

  
  
  NS_ASSERTION(!IsFrameSpecial(aFrame),
               "DeletingFrameSubtree on a special frame.  Prepare to crash.");

  do {
    DoDeletingFrameSubtree(aFrameManager, destroyQueue, aFrame, aFrame);

    
    
    
    
    
    
    
    
    
    aFrame = aFrame->GetNextContinuation();
  } while (aFrame);

  
  
  for (PRInt32 i = destroyQueue.Length() - 1; i >= 0; --i) {
    nsIFrame* outOfFlowFrame = destroyQueue[i];

    
    
    aFrameManager->RemoveFrame(outOfFlowFrame->GetParent(),
                               GetChildListNameFor(outOfFlowFrame),
                               outOfFlowFrame);
  }

  return NS_OK;
}

nsresult
nsCSSFrameConstructor::RemoveMappingsForFrameSubtree(nsIFrame* aRemovedFrame)
{
  NS_ASSERTION(!(aRemovedFrame->GetStateBits() & NS_FRAME_OUT_OF_FLOW),
               "RemoveMappingsForFrameSubtree doesn't handle out-of-flows");

  if (NS_UNLIKELY(mIsDestroyingFrameTree)) {
    
    
    
    
    return NS_OK;
  }

  nsFrameManager *frameManager = mPresShell->FrameManager();
  if (nsGkAtoms::placeholderFrame == aRemovedFrame->GetType()) {
    nsIFrame *placeholderFrame = aRemovedFrame;
    do {
      NS_ASSERTION(placeholderFrame->GetType() == nsGkAtoms::placeholderFrame,
                   "continuation must be of same type");
      nsIFrame* outOfFlowFrame =
        nsPlaceholderFrame::GetRealFrameForPlaceholder(placeholderFrame);
      
      frameManager->UnregisterPlaceholderFrame(
        static_cast<nsPlaceholderFrame*>(placeholderFrame));
      ::DeletingFrameSubtree(frameManager, outOfFlowFrame);
      frameManager->RemoveFrame(outOfFlowFrame->GetParent(),
                                GetChildListNameFor(outOfFlowFrame),
                                outOfFlowFrame);
      placeholderFrame = placeholderFrame->GetNextContinuation();
    } while (placeholderFrame);
  }

  
  CaptureStateFor(aRemovedFrame, mTempFrameTreeState);

  return ::DeletingFrameSubtree(frameManager, aRemovedFrame);
}

static void UnregisterPlaceholderChain(nsFrameManager* frameManager,
                                       nsPlaceholderFrame* placeholderFrame)
{
  
  nsPlaceholderFrame* curFrame = placeholderFrame;
  do {
    frameManager->UnregisterPlaceholderFrame(curFrame);
    curFrame->SetOutOfFlowFrame(nsnull);
    curFrame = static_cast<nsPlaceholderFrame*>(curFrame->GetNextContinuation());
  } while (curFrame);
}

nsresult
nsCSSFrameConstructor::ContentRemoved(nsIContent* aContainer,
                                      nsIContent* aChild,
                                      PRInt32     aIndexInContainer,
                                      PRBool*     aDidReconstruct)
{
  AUTO_LAYOUT_PHASE_ENTRY_POINT(mPresShell->GetPresContext(), FrameC);
  NS_PRECONDITION(mUpdateCount != 0,
                  "Should be in an update while destroying frames");

  *aDidReconstruct = PR_FALSE;
  
  
  

#ifdef DEBUG
  if (gNoisyContentUpdates) {
    printf("nsCSSFrameConstructor::ContentRemoved container=%p child=%p index=%d\n",
           static_cast<void*>(aContainer),
           static_cast<void*>(aChild),
           aIndexInContainer);
    if (gReallyNoisyContentUpdates) {
      aContainer->List(stdout, 0);
    }
  }
#endif

  nsFrameManager *frameManager = mPresShell->FrameManager();
  nsPresContext *presContext = mPresShell->GetPresContext();
  nsresult                  rv = NS_OK;

  
  nsIFrame* childFrame =
    mPresShell->FrameManager()->GetPrimaryFrameFor(aChild, aIndexInContainer);

  if (!childFrame || childFrame->GetContent() != aChild) {
    
    
    frameManager->ClearUndisplayedContentIn(aChild, aContainer);
  }

#ifdef MOZ_XUL
  if (NotifyListBoxBody(presContext, aContainer, aChild, aIndexInContainer, 
                        mDocument, childFrame, CONTENT_REMOVED))
    return NS_OK;

#endif 

  if (childFrame) {
    InvalidateCanvasIfNeeded(childFrame);
    
    
    
    
    
    
    if (MaybeRecreateContainerForIBSplitterFrame(childFrame, &rv)) {
      *aDidReconstruct = PR_TRUE;
      return rv;
    }

    
    nsIFrame* parentFrame = childFrame->GetParent();
    nsIAtom* parentType = parentFrame->GetType();

    if (parentType == nsGkAtoms::frameSetFrame &&
        IsSpecialFramesetChild(aChild)) {
      
      *aDidReconstruct = PR_TRUE;
      return RecreateFramesForContent(parentFrame->GetContent());
    }

#ifdef MOZ_MATHML
    
    
    
    nsIFrame* possibleMathMLAncestor = parentType == nsGkAtoms::blockFrame ? 
         parentFrame->GetParent() : parentFrame;
    if (possibleMathMLAncestor->IsFrameOfType(nsIFrame::eMathML)) {
      *aDidReconstruct = PR_TRUE;
      return RecreateFramesForContent(possibleMathMLAncestor->GetContent());
    }
#endif

    
    
    
    nsIFrame* grandparentFrame = parentFrame->GetParent();
    if (grandparentFrame && grandparentFrame->IsBoxFrame() &&
        (grandparentFrame->GetStateBits() & NS_STATE_BOX_WRAPS_KIDS_IN_BLOCK) &&
        
        aChild == AnyKidsNeedBlockParent(parentFrame->GetFirstChild(nsnull)) &&
        !AnyKidsNeedBlockParent(childFrame->GetNextSibling())) {
      *aDidReconstruct = PR_TRUE;
      return RecreateFramesForContent(grandparentFrame->GetContent());
    }
    
    
    
    nsIFrame* containingBlock = GetFloatContainingBlock(parentFrame);
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

      
      childFrame = mPresShell->GetPrimaryFrameFor(aChild);
      if (!childFrame || childFrame->GetContent() != aChild) {
        
        
        frameManager->ClearUndisplayedContentIn(aChild, aContainer);
        return NS_OK;
      }
      parentFrame = childFrame->GetParent();

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
      printf("\n");

      nsIFrameDebug* fdbg = do_QueryFrame(parentFrame);
      if (fdbg)
        fdbg->List(stdout, 0);
    }
#endif

    
    
    ::DeletingFrameSubtree(frameManager, childFrame);

    
    if (childFrame->GetStateBits() & NS_FRAME_OUT_OF_FLOW) {
      nsPlaceholderFrame* placeholderFrame =
        frameManager->GetPlaceholderFrameFor(childFrame);
      NS_ASSERTION(placeholderFrame, "No placeholder for out-of-flow?");

      UnregisterPlaceholderChain(frameManager, placeholderFrame);

      
      
      
      
      
      
      rv = frameManager->RemoveFrame(parentFrame,
                                     GetChildListNameFor(childFrame),
                                     childFrame);

      
      
      nsIFrame* placeholderParent = placeholderFrame->GetParent();
      ::DeletingFrameSubtree(frameManager, placeholderFrame);
      rv |= frameManager->RemoveFrame(placeholderParent,
                                      nsnull, placeholderFrame);
    } else {
      
      
      nsIFrame* outerTableFrame; 
      if (GetCaptionAdjustedParent(parentFrame, childFrame, &outerTableFrame)) {
        rv = frameManager->RemoveFrame(outerTableFrame,
                                       nsGkAtoms::captionList,
                                       childFrame);
      }
      else {
        rv = frameManager->RemoveFrame(parentFrame, nsnull, childFrame);
      }
    }

    if (mRootElementFrame == childFrame) {
      mRootElementFrame = nsnull;
      mRootElementStyleFrame = nsnull;
    }

    if (haveFLS && mRootElementFrame) {
      NS_ASSERTION(containingBlock == GetFloatContainingBlock(parentFrame),
                   "What happened here?");
      nsFrameConstructorState state(mPresShell, mFixedContainingBlock,
                                    GetAbsoluteContainingBlock(parentFrame),
                                    containingBlock);
      RecoverLetterFrames(containingBlock);
    }

#ifdef DEBUG
    if (gReallyNoisyContentUpdates && parentFrame) {
      nsIFrameDebug* fdbg = do_QueryFrame(parentFrame);
      if (fdbg) {
        printf("nsCSSFrameConstructor::ContentRemoved: resulting frame model:\n");
        fdbg->List(stdout, 0);
      }
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
      if (!(child->GetStateBits() & NS_FRAME_OUT_OF_FLOW)
          || (child->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER)) {
        
        if (nsGkAtoms::placeholderFrame == child->GetType()) { 
          
          nsIFrame* outOfFlowFrame =
            nsPlaceholderFrame::GetRealFrameForPlaceholder(child);

          DoApplyRenderingChangeToTree(outOfFlowFrame, aViewManager,
                                       aFrameManager, aChange);
        }
        else {  
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
#ifdef MOZ_SVG
        if (!(aFrame->GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD)) {
          nsSVGOuterSVGFrame *outerSVGFrame = nsSVGUtils::GetOuterSVGFrame(aFrame);
          if (outerSVGFrame) {
            
            outerSVGFrame->UpdateAndInvalidateCoveredRegion(aFrame);
          }
        }
#endif
      } else {
        aFrame->Invalidate(aFrame->GetOverflowRect());
      }
    }
  }
}

static void
ApplyRenderingChangeToTree(nsPresContext* aPresContext,
                           nsIFrame* aFrame,
                           nsChangeHint aChange)
{
  nsIPresShell *shell = aPresContext->PresShell();
  PRBool isPaintingSuppressed = PR_FALSE;
  shell->IsPaintingSuppressed(&isPaintingSuppressed);
  if (isPaintingSuppressed) {
    
    aChange = NS_SubtractHint(aChange, nsChangeHint_RepaintFrame);
    if (!aChange) {
      return;
    }
  }

  
  
  const nsStyleBackground *bg;
  while (!nsCSSRendering::FindBackground(aPresContext, aFrame, &bg)) {
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
InvalidateCanvasIfNeeded(nsIFrame* aFrame)
{
  NS_ASSERTION(aFrame, "Must have frame!");

  
  
  
  nsIContent* node = aFrame->GetContent();
  nsIContent* parent = node->GetParent();
  if (parent) {
    
    nsIContent* grandParent = parent->GetParent();
    if (grandParent) {
      
      return;
    }

    
    if (node->Tag() != nsGkAtoms::body ||
        !node->IsNodeOfType(nsINode::eHTML)) {
      return;
    }
  }

  
  
  
  
  nsIFrame *ancestor = aFrame;
  const nsStyleBackground *bg;
  nsPresContext* presContext = aFrame->PresContext();
  while (!nsCSSRendering::FindBackground(presContext, ancestor, &bg)) {
    ancestor = ancestor->GetParent();
    NS_ASSERTION(ancestor, "canvas must paint");
  }

  if (ancestor->GetType() == nsGkAtoms::canvasFrame) {
    
    
    ancestor = ancestor->GetParent();
  }

  if (ancestor != aFrame) {
    
    

    nsIViewManager::UpdateViewBatch batch(presContext->GetPresShell()->GetViewManager());
    ApplyRenderingChangeToTree(presContext, ancestor,
                               nsChangeHint_RepaintFrame);
    batch.EndUpdateViewBatch(NS_VMREFRESH_DEFERRED);
  }
}

nsresult
nsCSSFrameConstructor::StyleChangeReflow(nsIFrame* aFrame)
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

  
  
  
  
  if (IsFrameSpecial(aFrame))
    aFrame = GetIBContainingBlockFor(aFrame);

  do {
    mPresShell->FrameNeedsReflow(aFrame, nsIPresShell::eStyleChange,
                                 NS_FRAME_IS_DIRTY);
    aFrame = aFrame->GetNextContinuation();
  } while (aFrame);

  return NS_OK;
}

nsresult
nsCSSFrameConstructor::CharacterDataChanged(nsIContent* aContent,
                                            PRBool aAppend)
{
  AUTO_LAYOUT_PHASE_ENTRY_POINT(mPresShell->GetPresContext(), FrameC);
  nsresult      rv = NS_OK;

  
  nsIFrame* frame = mPresShell->GetPrimaryFrameFor(aContent);

  
  

  
  
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
        
        
        frame = mPresShell->GetPrimaryFrameFor(aContent);
        NS_ASSERTION(frame, "Should have frame here!");
      }
    }

    frame->CharacterDataChanged(mPresShell->GetPresContext(), aContent,
                                aAppend);

    if (haveFirstLetterStyle) {
      nsFrameConstructorState state(mPresShell, mFixedContainingBlock,
                                    GetAbsoluteContainingBlock(frame),
                                    block, nsnull);
      RecoverLetterFrames(block);
    }
  }

  return rv;
}

nsresult
nsCSSFrameConstructor::ProcessRestyledFrames(nsStyleChangeList& aChangeList)
{
  NS_ASSERTION(!nsContentUtils::IsSafeToRunScript(),
               "Someone forgot a script blocker");
  PRInt32 count = aChangeList.Count();
  if (!count)
    return NS_OK;

  
  
  BeginUpdate();

  nsPropertyTable *propTable = mPresShell->GetPresContext()->PropertyTable();

  
  
  
  PRInt32 index = count;

  while (0 <= --index) {
    const nsStyleChangeData* changeData;
    aChangeList.ChangeAt(index, &changeData);
    if (changeData->mFrame) {
      propTable->SetProperty(changeData->mFrame,
                             nsGkAtoms::changeListProperty,
                             nsnull, nsnull, nsnull);
    }
  }

  index = count;
  while (0 <= --index) {
    nsIFrame* frame;
    nsIContent* content;
    nsChangeHint hint;
    aChangeList.ChangeAt(index, frame, content, hint);
    if (frame && frame->GetContent() != content) {
      
      
      frame = nsnull;
      if (!(hint & nsChangeHint_ReconstructFrame)) {
        continue;
      }
    }

    
    if (frame) {
      nsresult res;

      propTable->GetProperty(frame, nsGkAtoms::changeListProperty, &res);

      if (NS_PROPTABLE_PROP_NOT_THERE == res)
        continue;
    }

    if (hint & nsChangeHint_ReconstructFrame) {
      RecreateFramesForContent(content);
    } else {
      NS_ASSERTION(frame, "This shouldn't happen");
#ifdef MOZ_SVG
      if (hint & nsChangeHint_UpdateEffects) {
        nsSVGEffects::UpdateEffects(frame);
      }
#endif
      if (hint & nsChangeHint_ReflowFrame) {
        StyleChangeReflow(frame);
      }
      if (hint & (nsChangeHint_RepaintFrame | nsChangeHint_SyncFrameView)) {
        ApplyRenderingChangeToTree(mPresShell->GetPresContext(), frame, hint);
      }
      if (hint & nsChangeHint_UpdateCursor) {
        nsIViewManager* viewMgr = mPresShell->GetViewManager();
        if (viewMgr)
          viewMgr->SynthesizeMouseMove(PR_FALSE);
      }
    }
  }

  EndUpdate();
  
  
  
  
  index = count;
  while (0 <= --index) {
    const nsStyleChangeData* changeData;
    aChangeList.ChangeAt(index, &changeData);
    if (changeData->mFrame) {
      propTable->DeleteProperty(changeData->mFrame,
                                nsGkAtoms::changeListProperty);
    }

#ifdef DEBUG
    
    if (changeData->mContent) {
      nsIFrame* frame = mPresShell->GetPrimaryFrameFor(changeData->mContent);
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
nsCSSFrameConstructor::RestyleElement(nsIContent     *aContent,
                                      nsIFrame       *aPrimaryFrame,
                                      nsChangeHint   aMinHint)
{
  NS_ASSERTION(aPrimaryFrame == mPresShell->GetPrimaryFrameFor(aContent),
               "frame/content mismatch");
  if (aPrimaryFrame && aPrimaryFrame->GetContent() != aContent) {
    
    
    aPrimaryFrame = nsnull;
  }
  NS_ASSERTION(!aPrimaryFrame || aPrimaryFrame->GetContent() == aContent,
               "frame/content mismatch");

  if (aMinHint & nsChangeHint_ReconstructFrame) {
    RecreateFramesForContent(aContent);
  } else if (aPrimaryFrame) {
    nsStyleChangeList changeList;
    mPresShell->FrameManager()->
      ComputeStyleChangeFor(aPrimaryFrame, &changeList, aMinHint);
    ProcessRestyledFrames(changeList);
  } else {
    
    MaybeRecreateFramesForContent(aContent);
  }
}

void
nsCSSFrameConstructor::RestyleLaterSiblings(nsIContent *aContent)
{
  nsIContent *parent = aContent->GetParent();
  if (!parent)
    return; 

  for (PRInt32 index = parent->IndexOf(aContent) + 1,
               index_end = parent->GetChildCount();
       index != index_end; ++index) {
    nsIContent *child = parent->GetChildAt(index);
    if (!child->IsNodeOfType(nsINode::eELEMENT))
      continue;

    nsIFrame* primaryFrame = mPresShell->GetPrimaryFrameFor(child);
    RestyleElement(child, primaryFrame, NS_STYLE_HINT_NONE);
  }
}

nsresult
nsCSSFrameConstructor::ContentStatesChanged(nsIContent* aContent1,
                                            nsIContent* aContent2,
                                            PRInt32 aStateMask) 
{
  DoContentStateChanged(aContent1, aStateMask);
  DoContentStateChanged(aContent2, aStateMask);
  return NS_OK;
}

void
nsCSSFrameConstructor::DoContentStateChanged(nsIContent* aContent,
                                             PRInt32 aStateMask) 
{
  nsStyleSet *styleSet = mPresShell->StyleSet();
  nsPresContext *presContext = mPresShell->GetPresContext();
  NS_ASSERTION(styleSet, "couldn't get style set");

  if (aContent) {
    nsChangeHint hint = NS_STYLE_HINT_NONE;
    
    
    
    
    
    
    nsIFrame* primaryFrame = mPresShell->GetPrimaryFrameFor(aContent);
    if (primaryFrame) {
      
      if (!primaryFrame->IsGeneratedContentFrame() &&
          (aStateMask & (NS_EVENT_STATE_BROKEN | NS_EVENT_STATE_USERDISABLED |
                         NS_EVENT_STATE_SUPPRESSED | NS_EVENT_STATE_LOADING))) {
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
    }

    nsReStyleHint rshint = 
      styleSet->HasStateDependentStyle(presContext, aContent, aStateMask);
      
    if ((aStateMask & NS_EVENT_STATE_HOVER) && rshint != 0) {
      ++mHoverGeneration;
    }

    PostRestyleEvent(aContent, rshint, hint);
  }
}

nsresult
nsCSSFrameConstructor::AttributeChanged(nsIContent* aContent,
                                        PRInt32 aNameSpaceID,
                                        nsIAtom* aAttribute,
                                        PRInt32 aModType,
                                        PRUint32 aStateMask)
{
  nsresult  result = NS_OK;

  
  
  
  nsCOMPtr<nsIPresShell> shell = mPresShell;

  
  nsIFrame* primaryFrame = shell->GetPrimaryFrameFor(aContent); 

#if 0
  NS_FRAME_LOG(NS_FRAME_TRACE_CALLS,
     ("HTMLStyleSheet::AttributeChanged: content=%p[%s] frame=%p",
      aContent, ContentTag(aContent, 0), frame));
#endif

  
  nsChangeHint hint = aContent->GetAttributeChangeHint(aAttribute, aModType);

  PRBool reframe = (hint & nsChangeHint_ReconstructFrame) != 0;

#ifdef MOZ_XUL
  
  
  
  if (!primaryFrame && !reframe) {
    PRInt32 namespaceID;
    nsIAtom* tag =
      mDocument->BindingManager()->ResolveTag(aContent, &namespaceID);

    if (namespaceID == kNameSpaceID_XUL &&
        (tag == nsGkAtoms::listitem ||
         tag == nsGkAtoms::listcell))
      return NS_OK;
  }

  if (aAttribute == nsGkAtoms::tooltiptext ||
      aAttribute == nsGkAtoms::tooltip) 
  {
    nsIRootBox* rootBox = nsIRootBox::GetRootBox(mPresShell);
    if (rootBox) {
      if (aModType == nsIDOMMutationEvent::REMOVAL)
        rootBox->RemoveTooltipSupport(aContent);
      if (aModType == nsIDOMMutationEvent::ADDITION)
        rootBox->AddTooltipSupport(aContent);
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
   
    
    result = primaryFrame->AttributeChanged(aNameSpaceID, aAttribute,
                                            aModType);
    
    
    
    
  }

  
  
  nsFrameManager *frameManager = shell->FrameManager();
  nsReStyleHint rshint = frameManager->HasAttributeDependentStyle(aContent,
                                                                  aAttribute,
                                                                  aModType,
                                                                  aStateMask);

  PostRestyleEvent(aContent, rshint, hint);

  return result;
}

void
nsCSSFrameConstructor::BeginUpdate() {
  NS_SuppressFocusEvent();
  ++mFocusSuppressCount;
  ++mUpdateCount;
}

void
nsCSSFrameConstructor::EndUpdate()
{
  if (mUpdateCount == 1) {
    
    

    RecalcQuotesAndCounters();
    NS_ASSERTION(mUpdateCount == 1, "Odd update count");
  }
  --mUpdateCount;
  if (mFocusSuppressCount) {
    NS_UnsuppressFocusEvent();
    --mFocusSuppressCount;
  }
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

class nsFocusUnsuppressEvent : public nsRunnable {
  public:
    NS_DECL_NSIRUNNABLE
    nsFocusUnsuppressEvent(PRUint32 aCount) : mCount(aCount) {}
  private:
    PRUint32 mCount;
  };

NS_IMETHODIMP nsFocusUnsuppressEvent::Run()
{
  while (mCount) {
    --mCount;
    NS_UnsuppressFocusEvent();
  }
  return NS_OK;
}

void
nsCSSFrameConstructor::WillDestroyFrameTree(PRBool aDestroyingPresShell)
{
#if defined(DEBUG_dbaron_off)
  mCounterManager.Dump();
#endif

  mIsDestroyingFrameTree = PR_TRUE;

  
  mQuoteList.Clear();
  mCounterManager.Clear();

  
  mRestyleEvent.Revoke();

  if (mFocusSuppressCount && aDestroyingPresShell) {
    nsRefPtr<nsFocusUnsuppressEvent> ev =
      new nsFocusUnsuppressEvent(mFocusSuppressCount);
    if (NS_SUCCEEDED(NS_DispatchToCurrentThread(ev))) {
      mFocusSuppressCount = 0;
    }
  }
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
    nsHTMLContainerFrame::CreateViewForFrame(newFrame, PR_FALSE);

    
    
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

    
    newFrame->SetInitialChildList(nsnull, newChildFrames.childList);
    
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
    nsHTMLContainerFrame::CreateViewForFrame(newFrame, PR_FALSE);

    
    nsFrameItems  childFrames;
    nsIFrame* childFrame = aFrame->GetFirstChild(nsnull);
    for ( ; childFrame; childFrame = childFrame->GetNextSibling()) {
      
      nsTableRowGroupFrame* rowGroupFrame =
        nsTableFrame::GetRowGroupFrame(childFrame);
      if (rowGroupFrame) {
        
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

          headerFooterFrame = static_cast<nsTableRowGroupFrame*>
                                         (NS_NewTableRowGroupFrame(aPresShell, rowGroupFrame->GetStyleContext()));
          nsIContent* headerFooter = rowGroupFrame->GetContent();
          headerFooterFrame->Init(headerFooter, newFrame, nsnull);
          ProcessChildren(state, headerFooter, rowGroupFrame->GetStyleContext(),
                          headerFooterFrame, PR_TRUE, childItems, PR_FALSE);
          NS_ASSERTION(!state.mFloatedItems.childList, "unexpected floated element");
          headerFooterFrame->SetInitialChildList(nsnull, childItems.childList);
          headerFooterFrame->SetRepeatable(PR_TRUE);

          
          headerFooterFrame->InitRepeatedFrame(aPresContext, rowGroupFrame);

          
          childFrames.AddChild(headerFooterFrame);
        }
      }
    }
    
    
    newFrame->SetInitialChildList(nsnull, childFrames.childList);
    
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
      nsHTMLContainerFrame::CreateViewForFrame(newFrame, PR_FALSE);
    }
    
  } else if (nsGkAtoms::inlineFrame == frameType) {
    newFrame = NS_NewInlineFrame(shell, styleContext);

    if (newFrame) {
      newFrame->Init(content, aParentFrame, aFrame);
      nsHTMLContainerFrame::CreateViewForFrame(newFrame, PR_FALSE);
    }
  
  } else if (nsGkAtoms::blockFrame == frameType) {
    newFrame = NS_NewBlockFrame(shell, styleContext);

    if (newFrame) {
      newFrame->Init(content, aParentFrame, aFrame);
      nsHTMLContainerFrame::CreateViewForFrame(newFrame, PR_FALSE);
    }
  
#ifdef MOZ_XUL
  } else if (nsGkAtoms::XULLabelFrame == frameType) {
    newFrame = NS_NewXULLabelFrame(shell, styleContext);

    if (newFrame) {
      newFrame->Init(content, aParentFrame, aFrame);
      nsHTMLContainerFrame::CreateViewForFrame(newFrame, PR_FALSE);
    }
#endif  
  } else if (nsGkAtoms::columnSetFrame == frameType) {
    newFrame = NS_NewColumnSetFrame(shell, styleContext, 0);

    if (newFrame) {
      newFrame->Init(content, aParentFrame, aFrame);
      nsHTMLContainerFrame::CreateViewForFrame(newFrame, PR_FALSE);
    }
  
  } else if (nsGkAtoms::positionedInlineFrame == frameType) {
    newFrame = NS_NewPositionedInlineFrame(shell, styleContext);

    if (newFrame) {
      newFrame->Init(content, aParentFrame, aFrame);
      nsHTMLContainerFrame::CreateViewForFrame(newFrame, PR_FALSE);
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
      nsHTMLContainerFrame::CreateViewForFrame(newFrame, PR_FALSE);
    }

  } else if (nsGkAtoms::tableRowFrame == frameType) {
    newFrame = NS_NewTableRowFrame(shell, styleContext);

    if (newFrame) {
      newFrame->Init(content, aParentFrame, aFrame);
      nsHTMLContainerFrame::CreateViewForFrame(newFrame, PR_FALSE);

      
      nsFrameItems  newChildList;
      nsIFrame* cellFrame = aFrame->GetFirstChild(nsnull);
      while (cellFrame) {
        
        if (IS_TABLE_CELL(cellFrame->GetType())) {
          nsIFrame* continuingCellFrame;
          rv = CreateContinuingFrame(aPresContext, cellFrame, newFrame,
                                     &continuingCellFrame);
          if (NS_FAILED(rv)) {
            nsFrameList tmp(newChildList.childList);
            tmp.DestroyFrames();
            newFrame->Destroy();
            *aContinuingFrame = nsnull;
            return NS_ERROR_OUT_OF_MEMORY;
          }
          newChildList.AddChild(continuingCellFrame);
        }
        cellFrame = cellFrame->GetNextSibling();
      }
      
      
      newFrame->SetInitialChildList(nsnull, newChildList.childList);
    }

  } else if (IS_TABLE_CELL(frameType)) {
    
    
    
    newFrame = NS_NewTableCellFrame(shell, styleContext, IsBorderCollapse(aParentFrame));

    if (newFrame) {
      newFrame->Init(content, aParentFrame, aFrame);
      nsHTMLContainerFrame::CreateViewForFrame(newFrame, PR_FALSE);

      
      nsIFrame* continuingBlockFrame;
      nsIFrame* blockFrame = aFrame->GetFirstChild(nsnull);
      rv = CreateContinuingFrame(aPresContext, blockFrame, newFrame,
                                 &continuingBlockFrame);
      if (NS_FAILED(rv)) {
        newFrame->Destroy();
        *aContinuingFrame = nsnull;
        return rv;
      }

      
      newFrame->SetInitialChildList(nsnull, continuingBlockFrame);
    }
  
  } else if (nsGkAtoms::lineFrame == frameType) {
    newFrame = NS_NewFirstLineFrame(shell, styleContext);

    if (newFrame) {
      newFrame->Init(content, aParentFrame, aFrame);
      nsHTMLContainerFrame::CreateViewForFrame(newFrame, PR_FALSE);
    }
  
  } else if (nsGkAtoms::letterFrame == frameType) {
    newFrame = NS_NewFirstLetterFrame(shell, styleContext);

    if (newFrame) {
      newFrame->Init(content, aParentFrame, aFrame);
      nsHTMLContainerFrame::CreateViewForFrame(newFrame, PR_FALSE);
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
                                   aParentFrame, aFrame, &newFrame);
    if (NS_FAILED(rv)) {
      oofContFrame->Destroy();
      *aContinuingFrame = nsnull;
      return rv;
    }
  } else if (nsGkAtoms::fieldSetFrame == frameType) {
    newFrame = NS_NewFieldSetFrame(shell, styleContext);

    if (newFrame) {
      newFrame->Init(content, aParentFrame, aFrame);

      nsHTMLContainerFrame::CreateViewForFrame(newFrame, PR_FALSE);

      
      
      nsIFrame* continuingBlockFrame;
      nsIFrame* blockFrame = GetFieldSetBlockFrame(aFrame);
      rv = CreateContinuingFrame(aPresContext, blockFrame, newFrame,
                                 &continuingBlockFrame);
      if (NS_FAILED(rv)) {
        newFrame->Destroy();
        *aContinuingFrame = nsnull;
        return rv;
      }
      
      newFrame->SetInitialChildList(nsnull, continuingBlockFrame);
    }
  } else if (nsGkAtoms::legendFrame == frameType) {
    newFrame = NS_NewLegendFrame(shell, styleContext);

    if (newFrame) {
      newFrame->Init(content, aParentFrame, aFrame);
      nsHTMLContainerFrame::CreateViewForFrame(newFrame, PR_FALSE);
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

  
  
  
  
  for (nsIFrame* fixed = firstFixed; fixed; fixed = fixed->GetNextSibling()) {
    nsIFrame* prevPlaceholder = nsnull;
    mPresShell->GetPlaceholderFrameFor(fixed, &prevPlaceholder);
    if (prevPlaceholder &&
        nsLayoutUtils::IsProperAncestorFrame(prevCanvasFrame, prevPlaceholder)) {
      nsresult rv = ConstructFrame(state, fixed->GetContent(),
                                   canvasFrame, fixedPlaceholders);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  
  
  
  NS_ASSERTION(!canvasFrame->GetFirstChild(nsnull),
               "leaking frames; doc root continuation must be empty");
  canvasFrame->SetInitialChildList(nsnull, fixedPlaceholders.childList);
  return NS_OK;
}

static PRBool
IsBindingAncestor(nsIContent* aContent, nsIContent* aBindingRoot)
{
  while (PR_TRUE) {
    
    
    if (aContent->IsRootOfNativeAnonymousSubtree())
      return PR_FALSE;
    nsIContent* bindingParent = aContent->GetBindingParent();
    if (!bindingParent)
      return PR_FALSE;
    if (bindingParent == aBindingRoot)
      return PR_TRUE;
    aContent = bindingParent;
  }
}




nsIFrame*
nsCSSFrameConstructor::FindFrameWithContent(nsFrameManager*  aFrameManager,
                                            nsIFrame*        aParentFrame,
                                            nsIContent*      aParentContent,
                                            nsIContent*      aContent,
                                            nsFindFrameHint* aHint)
{
  NS_PRECONDITION(aParentFrame, "Must have a frame");
  
#ifdef NOISY_FINDFRAME
  FFWC_totalCount++;
  printf("looking for content=%p, given aParentFrame %p parentContent %p, hint is %s\n", 
         aContent, aParentFrame, aParentContent, aHint ? "set" : "NULL");
#endif

  
  nsIAtom* listName = nsnull;
  PRInt32 listIndex = 0;
  PRBool searchAgain;

  do {
#ifdef NOISY_FINDFRAME
    FFWC_doLoop++;
#endif
    nsIFrame* kidFrame = nsnull;

    searchAgain = PR_FALSE;

    
    
    if (aHint) {
#ifdef NOISY_FINDFRAME
      printf("  hint frame is %p\n", aHint->mPrimaryFrameForPrevSibling);
#endif
      
      kidFrame = aHint->mPrimaryFrameForPrevSibling;
      
      if (kidFrame && (kidFrame->GetStateBits() & NS_FRAME_OUT_OF_FLOW)) {
        kidFrame = aFrameManager->GetPlaceholderFrameFor(kidFrame);
      }

      if (kidFrame) {
        
        if (kidFrame->GetNextSibling()) {
          kidFrame = kidFrame->GetNextSibling();
        }
        else {
          
          
          
          nsIFrame *parentFrame = kidFrame->GetParent();
          kidFrame = nsnull;
          if (parentFrame) {
            parentFrame = nsLayoutUtils::GetNextContinuationOrSpecialSibling(parentFrame);
          }
          if (parentFrame) {
            
            kidFrame = parentFrame->GetFirstChild(listName);
            
            
          }
        }
#ifdef NOISY_FINDFRAME
        printf("  hint gives us kidFrame=%p with parent frame %p content %p\n", 
               kidFrame, aParentFrame, aParentContent);
#endif
      }
    }
    if (!kidFrame) {  
      kidFrame = aParentFrame->GetFirstChild(listName);
    }
    while (kidFrame) {
      
      
      nsIContent* kidContent = kidFrame->GetContent();
      if (kidContent == aContent) {
        
        return nsPlaceholderFrame::GetRealFrameFor(kidFrame);
      }

      
      if (kidContent) {
        
        
        
        
        
        
        
        if (aParentContent == kidContent ||
            (aParentContent && IsBindingAncestor(kidContent, aParentContent))) {
#ifdef NOISY_FINDFRAME
          FFWC_recursions++;
          printf("  recursing with new parent set to kidframe=%p, parentContent=%p\n", 
                 kidFrame, aParentContent);
#endif
          nsIFrame* matchingFrame =
              FindFrameWithContent(aFrameManager,
                                   nsPlaceholderFrame::GetRealFrameFor(kidFrame),
                                   aParentContent, aContent, nsnull);

          if (matchingFrame) {
            return matchingFrame;
          }
        }
      }

      kidFrame = kidFrame->GetNextSibling();

#ifdef NOISY_FINDFRAME
      if (kidFrame) {
        FFWC_doSibling++;
        printf("  searching sibling frame %p\n", kidFrame);
      }
#endif
    }

    if (aHint) {
      
      
      
      
      
      
      
      
      aHint = nsnull;
      searchAgain = PR_TRUE;
    }
    else {
      do {
        listName = aParentFrame->GetAdditionalChildListName(listIndex++);
      } while (IsOutOfFlowList(listName));
    }
  } while (listName || searchAgain);

  return nsnull;
}




nsresult
nsCSSFrameConstructor::FindPrimaryFrameFor(nsFrameManager*  aFrameManager,
                                           nsIContent*      aContent,
                                           nsIFrame**       aFrame,
                                           nsFindFrameHint* aHint)
{
  NS_ASSERTION(aFrameManager && aContent && aFrame, "bad arg");

  *aFrame = nsnull;  

  
  
  
  
  
  
  
  
  
  nsIFrame*              parentFrame;   

  
  
  
  nsCOMPtr<nsIContent> parentContent = aContent->GetParent(); 
  if (parentContent) {
    parentFrame = aFrameManager->GetPrimaryFrameFor(parentContent, -1);
    while (parentFrame) {
      
      *aFrame = FindFrameWithContent(aFrameManager, parentFrame,
                                     parentContent, aContent, aHint);
#ifdef NOISY_FINDFRAME
      printf("FindFrameWithContent returned %p\n", *aFrame);
#endif

#ifdef DEBUG
      
      
      
      
      if (gVerifyFastFindFrame && aHint) {
#ifdef NOISY_FINDFRAME
        printf("VERIFYING...\n");
#endif
        nsIFrame *verifyTestFrame =
            FindFrameWithContent(aFrameManager, parentFrame,
                                 parentContent, aContent, nsnull);
#ifdef NOISY_FINDFRAME
        printf("VERIFY returned %p\n", verifyTestFrame);
#endif
        NS_ASSERTION(verifyTestFrame == *aFrame, "hint shortcut found wrong frame");
      }
#endif
      
      
      if (*aFrame) {
        aFrameManager->SetPrimaryFrameFor(aContent, *aFrame);
        break;
      }

      
      
      parentFrame = nsLayoutUtils::GetNextContinuationOrSpecialSibling(parentFrame);
#ifdef NOISY_FINDFRAME
      if (parentFrame) {
        FFWC_nextInFlows++;
        printf("  searching NIF frame %p\n", parentFrame);
      }
#endif
    }
  }

#ifdef NOISY_FINDFRAME
  printf("%10s %10s %10s %10s %10s\n", 
         "total", "doLoop", "doSibling", "recur", "nextIF");
  printf("%10d %10d %10d %10d %10d\n", 
         FFWC_totalCount, FFWC_doLoop, FFWC_doSibling, FFWC_recursions, 
         FFWC_nextInFlows);
#endif

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
    nsIFrame* insertionPoint = mPresShell->GetPrimaryFrameFor(insertionElement);
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
    if (content->IsNodeOfType(nsINode::eHTML) &&
        content->Tag() == nsGkAtoms::fieldset) {
      *aMultiple = PR_TRUE;
    }
  }

  return NS_OK;
}



nsresult
nsCSSFrameConstructor::CaptureStateForFramesOf(nsIContent* aContent,
                                               nsILayoutHistoryState* aHistoryState)
{
  nsIFrame* frame = mPresShell->GetPrimaryFrameFor(aContent);
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
nsCSSFrameConstructor::MaybeRecreateFramesForContent(nsIContent* aContent)
{
  nsresult result = NS_OK;
  nsFrameManager *frameManager = mPresShell->FrameManager();

  nsStyleContext *oldContext = frameManager->GetUndisplayedContent(aContent);
  if (oldContext) {
    
    nsRefPtr<nsStyleContext> newContext = mPresShell->StyleSet()->
      ResolveStyleFor(aContent, oldContext->GetParent());

    frameManager->ChangeUndisplayedContent(aContent, newContext);
    if (newContext->GetStyleDisplay()->mDisplay != NS_STYLE_DISPLAY_NONE) {
      result = RecreateFramesForContent(aContent);
    }
  }
  return result;
}

PRBool
nsCSSFrameConstructor::MaybeRecreateContainerForIBSplitterFrame(nsIFrame* aFrame,
                                                                nsresult* aResult)
{
  NS_PRECONDITION(aFrame, "Must have a frame");
  NS_PRECONDITION(aFrame->GetParent(), "Frame shouldn't be root");
  NS_PRECONDITION(aResult, "Null out param?");
  NS_PRECONDITION(aFrame == aFrame->GetFirstContinuation(),
                  "aFrame not the result of GetPrimaryFrameFor()?");

  if (IsFrameSpecial(aFrame)) {
    
    
#ifdef DEBUG
    if (gNoisyContentUpdates) {
      printf("nsCSSFrameConstructor::MaybeRecreateContainerForIBSplitterFrame: "
             "frame=");
      nsFrame::ListTag(stdout, aFrame);
      printf(" is special\n");
    }
#endif

    *aResult = ReframeContainingBlock(aFrame);
    return PR_TRUE;
  }

  
  
  
  nsIFrame* parent = aFrame->GetParent();
  if (!IsFrameSpecial(parent)) {
    return PR_FALSE;
  }

  
  
  
  
  if (IsInlineOutside(aFrame)) {
    return PR_FALSE;
  }

#ifdef DEBUG
  if (gNoisyContentUpdates) {
    printf("nsCSSFrameConstructor::MaybeRecreateContainerForIBSplitterFrame: "
           "frame=");
    nsFrame::ListTag(stdout, parent);
    printf(" is special\n");
  }
#endif

  *aResult = ReframeContainingBlock(parent);
  return PR_TRUE;
}
 
nsresult
nsCSSFrameConstructor::RecreateFramesForContent(nsIContent* aContent)
{
  
  
  
  
  
  NS_ENSURE_TRUE(aContent->GetDocument(), NS_ERROR_FAILURE);

  
  
  
  
  
  

  nsIFrame* frame = mPresShell->GetPrimaryFrameFor(aContent);
  if (frame && frame->IsFrameOfType(nsIFrame::eMathML)) {
    
    
    while (PR_TRUE) {
      nsIContent* parentContent = aContent->GetParent();
      nsIFrame* parentContentFrame = mPresShell->GetPrimaryFrameFor(parentContent);
      if (!parentContentFrame || !parentContentFrame->IsFrameOfType(nsIFrame::eMathML))
        break;
      aContent = parentContent;
      frame = parentContentFrame;
    }
  }

  if (frame) {
    nsIFrame* nonGeneratedAncestor = nsLayoutUtils::GetNonGeneratedAncestor(frame);
    if (nonGeneratedAncestor->GetContent() != aContent) {
      return RecreateFramesForContent(nonGeneratedAncestor->GetContent());
    }
  }

  nsresult rv = NS_OK;

  if (frame && MaybeRecreateContainerForIBSplitterFrame(frame, &rv)) {
    return rv;
  }

  nsCOMPtr<nsIContent> container = aContent->GetParent();
  if (container) {
    
    PRInt32 indexInContainer = container->IndexOf(aContent);
    
    
    CaptureStateForFramesOf(aContent, mTempFrameTreeState);

    
    
    PRBool didReconstruct;
    rv = ContentRemoved(container, aContent, indexInContainer, &didReconstruct);

    if (NS_SUCCEEDED(rv) && !didReconstruct) {
      
      
      
      rv = ContentInserted(container, aContent,
                           indexInContainer, mTempFrameTreeState);
    }
  } else {
    
    ReconstructDocElementHierarchy();
  }

#ifdef ACCESSIBILITY
  if (mPresShell->IsAccessibilityActive()) {
    PRUint32 event;
    if (frame) {
      nsIFrame *newFrame = mPresShell->GetPrimaryFrameFor(aContent);
      event = newFrame ? PRUint32(nsIAccessibleEvent::EVENT_ASYNCH_SIGNIFICANT_CHANGE) :
                         PRUint32(nsIAccessibleEvent::EVENT_ASYNCH_HIDE);
    }
    else {
      event = nsIAccessibleEvent::EVENT_ASYNCH_SHOW;
    }

    
    
    nsCOMPtr<nsIAccessibilityService> accService = 
      do_GetService("@mozilla.org/accessibilityService;1");
    if (accService) {
      accService->InvalidateSubtreeFor(mPresShell, aContent, event);
    }
  }
#endif

  return rv;
}





already_AddRefed<nsStyleContext>
nsCSSFrameConstructor::GetFirstLetterStyle(nsIContent* aContent,
                                           nsStyleContext* aStyleContext)
{
  if (aContent) {
    return mPresShell->StyleSet()->
      ResolvePseudoStyleFor(aContent,
                            nsCSSPseudoElements::firstLetter, aStyleContext);
  }
  return nsnull;
}

already_AddRefed<nsStyleContext>
nsCSSFrameConstructor::GetFirstLineStyle(nsIContent* aContent,
                                         nsStyleContext* aStyleContext)
{
  if (aContent) {
    return mPresShell->StyleSet()->
      ResolvePseudoStyleFor(aContent,
                            nsCSSPseudoElements::firstLine, aStyleContext);
  }
  return nsnull;
}



PRBool
nsCSSFrameConstructor::ShouldHaveFirstLetterStyle(nsIContent* aContent,
                                                  nsStyleContext* aStyleContext)
{
  return nsLayoutUtils::HasPseudoStyle(aContent, aStyleContext,
                                       nsCSSPseudoElements::firstLetter,
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
                                  nsCSSPseudoElements::firstLine,
                                  mPresShell->GetPresContext());
  if (hasFirstLine) {
    
    PRInt32 namespaceID;
    nsIAtom* tag = mDocument->BindingManager()->ResolveTag(aContent,
                                                           &namespaceID);
    
    hasFirstLine = tag != nsGkAtoms::fieldset ||
      (namespaceID != kNameSpaceID_XHTML &&
       !aContent->IsNodeOfType(nsINode::eHTML));
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

nsresult
nsCSSFrameConstructor::ConstructFramesFromItemList(nsFrameConstructorState& aState,
                                                   FrameConstructionItemList& aItems,
                                                   nsIFrame* aParentFrame,
                                                   nsFrameItems& aFrameItems)
{
  
  nsPseudoFrames priorPseudoFrames;
  aState.mPseudoFrames.Reset(&priorPseudoFrames);

  for (FCItemIterator iter(aItems); !iter.IsDone(); iter.Next()) {
    nsresult rv =
      ConstructFramesFromItem(aState, iter, aParentFrame, aFrameItems);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_ASSERTION(!aState.mHavePendingPopupgroup,
               "Should have proccessed it by now");

  
  if (!aState.mPseudoFrames.IsEmpty()) {
    ProcessPseudoFrames(aState, aFrameItems);
  }

  
  aState.mPseudoFrames = priorPseudoFrames;

  return NS_OK;
}









nsresult
nsCSSFrameConstructor::ProcessChildren(nsFrameConstructorState& aState,
                                       nsIContent*              aContent,
                                       nsStyleContext*          aStyleContext,
                                       nsIFrame*                aFrame,
                                       const PRBool             aCanHaveGeneratedContent,
                                       nsFrameItems&            aFrameItems,
                                       const PRBool             aAllowBlockStyles)
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

  FrameConstructionItemList itemsToConstruct;
  nsresult rv = NS_OK;

  
  
  
  nsAutoTArray<nsIContent*, 4> anonymousItems;
  GetAnonymousContent(aContent, aFrame, anonymousItems);
  for (PRUint32 i = 0; i < anonymousItems.Length(); ++i) {
#ifdef DEBUG
    nsIAnonymousContentCreator* creator = do_QueryFrame(aFrame);
    NS_ASSERTION(!creator || !creator->CreateFrameFor(anonymousItems[i]),
                 "If you need to use CreateFrameFor, you need to call "
                 "CreateAnonymousFrames manually and not follow the standard "
                 "ProcessChildren() codepath for this frame");
#endif
    AddFrameConstructionItems(aState, anonymousItems[i], aFrame,
                              itemsToConstruct);
  }

  if (!aFrame->IsLeaf() &&
      mDocument->BindingManager()->ShouldBuildChildFrames(aContent)) {
    
    
    
    
    
    nsStyleContext* styleContext;

    if (aCanHaveGeneratedContent) {
      styleContext =
        nsFrame::CorrectStyleParentFrame(aFrame, nsnull)->GetStyleContext();
      
      CreateGeneratedContentItem(aState, aFrame, aContent,
                                 styleContext, nsCSSPseudoElements::before,
                                 itemsToConstruct);
    }

    ChildIterator iter, last;
    for (ChildIterator::Init(aContent, &iter, &last);
         iter != last;
         ++iter) {
      AddFrameConstructionItems(aState, *iter, aFrame, itemsToConstruct);
    }

    if (aCanHaveGeneratedContent) {
      
      CreateGeneratedContentItem(aState, aFrame, aContent,
                                 styleContext, nsCSSPseudoElements::after,
                                 itemsToConstruct);
    }
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
    rv = WrapFramesInFirstLineFrame(aState, aContent, aFrame, aFrameItems);
  }

  nsIContent *badKid;
  if (aFrame->IsBoxFrame() &&
      (badKid = AnyKidsNeedBlockParent(aFrameItems.childList))) {
    nsAutoString parentTag, kidTag;
    aContent->Tag()->ToString(parentTag);
    badKid->Tag()->ToString(kidTag);
    const PRUnichar* params[] = { parentTag.get(), kidTag.get() };
    nsStyleContext *frameStyleContext = aFrame->GetStyleContext();
    const nsStyleDisplay *display = frameStyleContext->GetStyleDisplay();
    const char *message =
      (display->mDisplay == NS_STYLE_DISPLAY_INLINE_BOX)
        ? "NeededToWrapXULInlineBox" : "NeededToWrapXUL";
    nsContentUtils::ReportToConsole(nsContentUtils::eXUL_PROPERTIES,
                                    message,
                                    params, NS_ARRAY_LENGTH(params),
                                    mDocument->GetDocumentURI(),
                                    EmptyString(), 0, 0, 
                                    nsIScriptError::warningFlag,
                                    "FrameConstructor");

    nsRefPtr<nsStyleContext> blockSC = mPresShell->StyleSet()->
      ResolvePseudoStyleFor(aContent,
                            nsCSSAnonBoxes::mozXULAnonymousBlock,
                            frameStyleContext);
    nsIFrame *blockFrame = NS_NewBlockFrame(mPresShell, blockSC);
    
    
    

    InitAndRestoreFrame(aState, aContent, aFrame, nsnull,
                        blockFrame, PR_FALSE);

    NS_ASSERTION(!blockFrame->HasView(), "need to do view reparenting");
    for (nsIFrame *f = aFrameItems.childList; f; f = f->GetNextSibling()) {
      ReparentFrame(aState.mFrameManager, blockFrame, f);
    }

    blockFrame->AppendFrames(nsnull, aFrameItems.childList);
    aFrameItems = nsFrameItems();
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
  nsFrameItems&            aFrameItems)
{
  nsresult rv = NS_OK;

  
  nsIFrame* kid = aFrameItems.childList;
  nsIFrame* firstInlineFrame = nsnull;
  nsIFrame* lastInlineFrame = nsnull;
  while (kid) {
    if (IsInlineOutside(kid)) {
      if (!firstInlineFrame) firstInlineFrame = kid;
      lastInlineFrame = kid;
    }
    else {
      break;
    }
    kid = kid->GetNextSibling();
  }

  
  if (!firstInlineFrame) {
    return rv;
  }

  
  nsStyleContext* parentStyle =
    nsFrame::CorrectStyleParentFrame(aBlockFrame,
                                     nsCSSPseudoElements::firstLine)->
      GetStyleContext();
  nsRefPtr<nsStyleContext> firstLineStyle = GetFirstLineStyle(aBlockContent,
                                                              parentStyle);

  nsIFrame* lineFrame = NS_NewFirstLineFrame(mPresShell, firstLineStyle);

  if (lineFrame) {
    
    rv = InitAndRestoreFrame(aState, aBlockContent, aBlockFrame, nsnull,
                             lineFrame);

    
    
    nsIFrame* secondBlockFrame = lastInlineFrame->GetNextSibling();
    lastInlineFrame->SetNextSibling(nsnull);

    
    
    
    
    if (secondBlockFrame) {
      lineFrame->SetNextSibling(secondBlockFrame);
    }
    if (aFrameItems.childList == lastInlineFrame) {
      
      aFrameItems.lastChild = lineFrame;
    }
    aFrameItems.childList = lineFrame;

    
    kid = firstInlineFrame;
    NS_ASSERTION(lineFrame->GetStyleContext() == firstLineStyle,
                 "Bogus style context on line frame");
    while (kid) {
      ReparentFrame(aState.mFrameManager, lineFrame, kid);
      kid = kid->GetNextSibling();
    }
    lineFrame->SetInitialChildList(nsnull, firstInlineFrame);
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
  
  
  nsIFrame* blockKid = aBlockFrame->GetFirstChild(nsnull);
  if (!blockKid) {
    return WrapFramesInFirstLineFrame(aState, aBlockContent,
                                      aBlockFrame, aFrameItems);
  }

  
  
  nsresult rv = NS_OK;
  nsFrameList blockFrames(blockKid);
  nsIFrame* lastBlockKid = blockFrames.LastChild();
  if (lastBlockKid->GetType() != nsGkAtoms::lineFrame) {
    
    
    
    
    return rv;
  }
  nsIFrame* lineFrame = lastBlockKid;

  
  nsIFrame* kid = aFrameItems.childList;
  nsIFrame* firstInlineFrame = nsnull;
  nsIFrame* lastInlineFrame = nsnull;
  while (kid) {
    if (IsInlineOutside(kid)) {
      if (!firstInlineFrame) firstInlineFrame = kid;
      lastInlineFrame = kid;
    }
    else {
      break;
    }
    kid = kid->GetNextSibling();
  }

  
  if (!firstInlineFrame) {
    return rv;
  }

  
  
  nsIFrame* remainingFrames = lastInlineFrame->GetNextSibling();
  lastInlineFrame->SetNextSibling(nsnull);
  kid = firstInlineFrame;
  while (kid) {
    ReparentFrame(aState.mFrameManager, lineFrame, kid);
    kid = kid->GetNextSibling();
  }
  aState.mFrameManager->AppendFrames(lineFrame, nsnull, firstInlineFrame);

  
  if (remainingFrames) {
    aFrameItems.childList = remainingFrames;
  }
  else {
    aFrameItems.childList = nsnull;
    aFrameItems.lastChild = nsnull;
  }

  return rv;
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
          
          
          
          nsFrameList list(nextSibling);
          if (nextSibling) {
            nsLineFrame* lineFrame = (nsLineFrame*) prevSiblingParent;
            lineFrame->StealFramesFrom(nextSibling);
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
  InitAndRestoreFrame(aState, letterContent,
                      aState.GetGeometricParent(aStyleContext->GetStyleDisplay(),
                                                aParentFrame),
                      nsnull, letterFrame);

  
  
  
  
  nsRefPtr<nsStyleContext> textSC;
  textSC = styleSet->ResolveStyleForNonElement(aStyleContext);
  aTextFrame->SetStyleContextWithoutNotification(textSC);
  InitAndRestoreFrame(aState, aTextContent, letterFrame, nsnull, aTextFrame);

  
  letterFrame->SetInitialChildList(nsnull, aTextFrame);

  
  
  
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

  NS_ASSERTION(aResult.childList == nsnull,
               "aResult should be an empty nsFrameItems!");
  nsIFrame* insertAfter = nsnull;
  nsIFrame* f;
  
  
  for (f = aState.mFloatedItems.childList; f; f = f->GetNextSibling()) {
    if (f->GetParent() == aBlockFrame)
      break;
    insertAfter = f;
  }

  rv = aState.AddChild(letterFrame, aResult, letterContent, aStyleContext,
                       aParentFrame, PR_FALSE, PR_TRUE, PR_FALSE, PR_TRUE,
                       insertAfter);

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
    
    
    
    nsIFrame* textFrame = NS_NewTextFrame(mPresShell, textSC);

    NS_ASSERTION(aBlockFrame == GetFloatContainingBlock(aParentFrame),
                 "Containing block is confused");
    nsFrameConstructorState state(mPresShell, mFixedContainingBlock,
                                  GetAbsoluteContainingBlock(aParentFrame),
                                  aBlockFrame);

    
    const nsStyleDisplay* display = sc->GetStyleDisplay();
    if (display->IsFloating()) {
      
      CreateFloatingLetterFrame(state, aBlockFrame, aTextContent, textFrame,
                                blockContent, aParentFrame,
                                sc, aResult);
    }
    else {
      
      nsIFrame* letterFrame = NS_NewFirstLetterFrame(mPresShell, sc);

      if (letterFrame) {
        
        
        
        nsIContent* letterContent = aTextContent->GetParent();
        letterFrame->Init(letterContent, aParentFrame, nsnull);

        InitAndRestoreFrame(state, aTextContent, letterFrame, nsnull,
                            textFrame);

        letterFrame->SetInitialChildList(nsnull, textFrame);
        aResult.childList = aResult.lastChild = letterFrame;
        aBlockFrame->AddStateBits(NS_BLOCK_HAS_FIRST_LETTER_CHILD);
      }
    }
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
  rv = WrapFramesInFirstLetterFrame(aBlockFrame, aBlockFrame,
                                    aBlockFrames.childList,
                                    &parentFrame, &textFrame, &prevFrame,
                                    letterFrames, &stopLooking);
  if (NS_FAILED(rv)) {
    return rv;
  }
  if (parentFrame) {
    if (parentFrame == aBlockFrame) {
      
      
      nsIFrame* nextSibling = textFrame->GetNextSibling();
      textFrame->SetNextSibling(nsnull);
      if (prevFrame) {
        prevFrame->SetNextSibling(letterFrames.childList);
      }
      else {
        aBlockFrames.childList = letterFrames.childList;
      }
      letterFrames.lastChild->SetNextSibling(nextSibling);

      
      textFrame->Destroy();

      
      
      if (!nextSibling) {
        aBlockFrames.lastChild = letterFrames.lastChild;
      }
    }
    else {
      
      ::DeletingFrameSubtree(mPresShell->FrameManager(), textFrame);
      parentFrame->RemoveFrame(nsnull, textFrame);

      
      parentFrame->InsertFrames(nsnull, prevFrame, letterFrames.childList);
    }
  }

  return rv;
}

nsresult
nsCSSFrameConstructor::WrapFramesInFirstLetterFrame(
  nsIFrame*                aBlockFrame,
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
        
        rv = CreateLetterFrame(aBlockFrame, textContent,
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
      WrapFramesInFirstLetterFrame(aBlockFrame, frame, kids,
                                   aModifiedParent, aTextFrame,
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

  
  
  nsIFrame* nextTextFrame = textFrame->GetNextContinuation();
  while (nextTextFrame) {
    nsIFrame* nextTextParent = nextTextFrame->GetParent();
    if (nextTextParent) {
      nsSplittableFrame::RemoveFromFlow(nextTextFrame);
      ::DeletingFrameSubtree(aFrameManager, nextTextFrame);
      aFrameManager->RemoveFrame(nextTextParent, nsnull, nextTextFrame);
    }
    nextTextFrame = textFrame->GetNextContinuation();
  }

  
  
  
  
  
  
  
  
  nsFrameList siblingList(parentFrame->GetFirstChild(nsnull));
  NS_ASSERTION(siblingList.ContainsFrame(placeholderFrame),
               "Placeholder not in parent's principal child list?");
  nsIFrame* prevSibling = siblingList.GetPrevSiblingFor(placeholderFrame);

  
#ifdef NOISY_FIRST_LETTER
  printf("RemoveFloatingFirstLetterFrames: textContent=%p oldTextFrame=%p newTextFrame=%p\n",
         textContent.get(), textFrame, newTextFrame);
#endif

  UnregisterPlaceholderChain(aFrameManager, placeholderFrame);

  
  ::DeletingFrameSubtree(aFrameManager, floatFrame);
  aFrameManager->RemoveFrame(aBlockFrame, nsGkAtoms::floatList,
                             floatFrame);

  
  ::DeletingFrameSubtree(aFrameManager, placeholderFrame);
  aFrameManager->RemoveFrame(parentFrame, nsnull, placeholderFrame);

  
  aFrameManager->InsertFrames(parentFrame, nsnull,
                              prevSibling, newTextFrame);

  return NS_OK;
}

nsresult
nsCSSFrameConstructor::RemoveFirstLetterFrames(nsPresContext* aPresContext,
                                               nsIPresShell* aPresShell,
                                               nsFrameManager* aFrameManager,
                                               nsIFrame* aFrame,
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

      
      ::DeletingFrameSubtree(aFrameManager, kid);
      aFrameManager->RemoveFrame(aFrame, nsnull, kid);

      
      aFrameManager->InsertFrames(aFrame, nsnull, prevSibling, textFrame);

      *aStopLooking = PR_TRUE;
      aFrame->RemoveStateBits(NS_BLOCK_HAS_FIRST_LETTER_CHILD);
      break;
    }
    else if (IsInlineFrame(kid)) {
      
      RemoveFirstLetterFrames(aPresContext, aPresShell, aFrameManager, kid,
                              aStopLooking);
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
  
  PRBool stopLooking = PR_FALSE;
  nsresult rv;
  do {
    rv = RemoveFloatingFirstLetterFrames(aPresContext, aPresShell,
                                         aFrameManager,
                                         aBlockFrame, &stopLooking);
    if (NS_SUCCEEDED(rv) && !stopLooking) {
      rv = RemoveFirstLetterFrames(aPresContext, aPresShell, aFrameManager,
                                   aBlockFrame, &stopLooking);
    }
    if (stopLooking) {
      break;
    }
    aBlockFrame = aBlockFrame->GetNextContinuation();
  }  while (aBlockFrame);
  return rv;
}


nsresult
nsCSSFrameConstructor::RecoverLetterFrames(nsIFrame* aBlockFrame)
{
  aBlockFrame = aBlockFrame->GetFirstContinuation();
  
  nsIFrame* parentFrame = nsnull;
  nsIFrame* textFrame = nsnull;
  nsIFrame* prevFrame = nsnull;
  nsFrameItems letterFrames;
  PRBool stopLooking = PR_FALSE;
  nsresult rv;
  do {
    
    aBlockFrame->AddStateBits(NS_BLOCK_HAS_FIRST_LETTER_STYLE);
    rv = WrapFramesInFirstLetterFrame(aBlockFrame, aBlockFrame,
                                      aBlockFrame->GetFirstChild(nsnull),
                                      &parentFrame, &textFrame, &prevFrame,
                                      letterFrames, &stopLooking);
    if (NS_FAILED(rv)) {
      return rv;
    }
    if (stopLooking) {
      break;
    }
    aBlockFrame = aBlockFrame->GetNextContinuation();
  } while (aBlockFrame);

  if (parentFrame) {
    
    ::DeletingFrameSubtree(mPresShell->FrameManager(), textFrame);
    parentFrame->RemoveFrame(nsnull, textFrame);

    
    parentFrame->InsertFrames(nsnull, prevFrame, letterFrames.childList);
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
    styleContext = ResolveStyleContext(aParentFrame, aChild);

    
    
    const nsStyleDisplay* display = styleContext->GetStyleDisplay();

    if (NS_STYLE_DISPLAY_NONE == display->mDisplay) {
      *aNewFrame = nsnull;
      return NS_OK;
    }

    BeginUpdate();

    FrameConstructionItemList items;
    AddFrameConstructionItemsInternal(state, aChild, aParentFrame,
                                      aChild->Tag(), aChild->GetNameSpaceID(),
                                      styleContext, ITEM_ALLOW_XBL_BASE,
                                      items);
    ConstructFramesFromItemList(state, items, aParentFrame, frameItems);

    nsIFrame* newFrame = frameItems.childList;
    *aNewFrame = newFrame;

    if (NS_SUCCEEDED(rv) && (nsnull != newFrame)) {
      
      if (aIsAppend)
        rv = ((nsListBoxBodyFrame*)aParentFrame)->ListBoxAppendFrames(newFrame);
      else
        rv = ((nsListBoxBodyFrame*)aParentFrame)->ListBoxInsertFrames(aPrevFrame, newFrame);
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
                                      PRBool                   aAbsPosContainer)
{
  
  nsIFrame* blockFrame = *aNewFrame;
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
    
    nsHTMLContainerFrame::CreateViewForFrame(columnSetFrame, PR_FALSE);
    blockStyle = mPresShell->StyleSet()->
      ResolvePseudoStyleFor(aContent, nsCSSAnonBoxes::columnContent,
                            aStyleContext);
    parent = columnSetFrame;
    *aNewFrame = columnSetFrame;

    columnSetFrame->SetInitialChildList(nsnull, blockFrame);
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

  
  nsHTMLContainerFrame::CreateViewForFrame(blockFrame, PR_FALSE);

  if (!mRootElementFrame) {
    
    
    mRootElementFrame = *aNewFrame;
  }

  
  
  
  
  
  
  
  nsFrameConstructorSaveState absoluteSaveState;
  if (aAbsPosContainer) {
    
    aState.PushAbsoluteContainingBlock(blockFrame, absoluteSaveState);
  }

  
  nsFrameItems childItems;
  rv = ProcessChildren(aState, aContent, aStyleContext, blockFrame, PR_TRUE,
                       childItems, PR_TRUE);

  
  blockFrame->SetInitialChildList(nsnull, childItems.childList);

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
  if (positioned) {
    newFrame = NS_NewPositionedInlineFrame(mPresShell, styleContext);
  } else {
    newFrame = NS_NewInlineFrame(mPresShell, styleContext);
  }

  
  InitAndRestoreFrame(aState, content, aParentFrame, nsnull, newFrame);

  nsFrameConstructorSaveState absoluteSaveState;  
                                                  
                                                  

  
  nsHTMLContainerFrame::CreateViewForFrame(newFrame, PR_FALSE);

  if (positioned) {                            
    
    
    aState.PushAbsoluteContainingBlock(newFrame, absoluteSaveState);
  }

  
  nsFrameItems childItems;
  nsresult rv = ConstructFramesFromItemList(aState, aItem.mChildItems,
                                            newFrame, childItems);
  if (NS_FAILED(rv)) {
    
    return rv;
  }

  nsIFrame* list1 = childItems.childList;
  nsIFrame* prevToFirstBlock;
  nsIFrame* list2;

  if (aItem.mIsAllInline ||
      
      
      !(list2 = FindFirstBlock(list1, &prevToFirstBlock))) {
    
    
    
    
    
    
    newFrame->SetInitialChildList(nsnull, childItems.childList);
    if (NS_SUCCEEDED(rv)) {
      aState.AddChild(newFrame, aFrameItems, content, styleContext, aParentFrame);
      *aNewFrame = newFrame;
    }
    return rv;
  }

  
  
  
  
  
  
  
  

  
  if (prevToFirstBlock) {
    prevToFirstBlock->SetNextSibling(nsnull);
  }
  else {
    list1 = nsnull;
  }

  
  
  nsIFrame* afterFirstBlock = list2->GetNextSibling();
  nsIFrame* list3 = nsnull;
  nsIFrame* lastBlock = FindLastBlock(afterFirstBlock);
  if (!lastBlock) {
    lastBlock = list2;
  }
  list3 = lastBlock->GetNextSibling();
  lastBlock->SetNextSibling(nsnull);

  
  newFrame->SetInitialChildList(nsnull, list1);
                                             
  
  
  
  nsIAtom* blockStyle;
  nsRefPtr<nsStyleContext> blockSC;
  nsIFrame* blockFrame;
  if (positioned) {
    blockStyle = nsCSSAnonBoxes::mozAnonymousPositionedBlock;
    
    blockSC = mPresShell->StyleSet()->
      ResolvePseudoStyleFor(content, blockStyle, styleContext);
  }
  else {
    blockStyle = nsCSSAnonBoxes::mozAnonymousBlock;

    blockSC = mPresShell->StyleSet()->
      ResolvePseudoStyleFor(content, blockStyle, styleContext);
  }
  blockFrame = NS_NewBlockFrame(mPresShell, blockSC);

  InitAndRestoreFrame(aState, content, aParentFrame, nsnull, blockFrame, PR_FALSE);

  
  nsHTMLContainerFrame::CreateViewForFrame(blockFrame, PR_FALSE);

  if (blockFrame->HasView() || newFrame->HasView()) {
    
    nsHTMLContainerFrame::ReparentFrameViewList(aState.mPresContext, list2,
                                                list2->GetParent(), blockFrame);
  }

  blockFrame->SetInitialChildList(nsnull, list2);

  nsFrameConstructorState state(mPresShell, mFixedContainingBlock,
                                GetAbsoluteContainingBlock(blockFrame),
                                GetFloatContainingBlock(blockFrame));

  
  
  
  
  
  MoveChildrenTo(state.mFrameManager, blockFrame, list2, nsnull, &state,
                 &aState);

  
  nsIFrame* inlineFrame;
  if (positioned) {
    inlineFrame = NS_NewPositionedInlineFrame(mPresShell, styleContext);
  }
  else {
    inlineFrame = NS_NewInlineFrame(mPresShell, styleContext);
  }

  InitAndRestoreFrame(aState, content, aParentFrame, nsnull, inlineFrame,
                      PR_FALSE);

  
  nsHTMLContainerFrame::CreateViewForFrame(inlineFrame, PR_FALSE);

  if (list3) {
    MoveFramesToEndOfIBSplit(aState, inlineFrame, list3, blockFrame, nsnull);
  }

  
  
  
  SetFrameIsSpecial(newFrame, blockFrame);
  SetFrameIsSpecial(blockFrame, inlineFrame);
  SetFrameIsSpecial(inlineFrame, nsnull);
  MarkIBSpecialPrevSibling(blockFrame, newFrame);
  MarkIBSpecialPrevSibling(inlineFrame, blockFrame);

  #ifdef DEBUG
  if (gNoisyInlineConstruction) {
    nsIFrameDebug*  frameDebug;

    printf("nsCSSFrameConstructor::ConstructInline:\n");
    if ( (frameDebug = do_QueryFrame(*aNewFrame)) ) {
      printf("  ==> leading inline frame:\n");
      frameDebug->List(stdout, 2);
    }
    if ( (frameDebug = do_QueryFrame(blockFrame)) ) {
      printf("  ==> block frame:\n");
      frameDebug->List(stdout, 2);
    }
    if ( (frameDebug = do_QueryFrame(inlineFrame)) ) {
      printf("  ==> trailing inline frame:\n");
      frameDebug->List(stdout, 2);
    }
  }
#endif

  if (NS_SUCCEEDED(rv)) {
    aState.AddChild(newFrame, aFrameItems, content, styleContext, aParentFrame);
    *aNewFrame = newFrame;
  }
  return rv;
}

void
nsCSSFrameConstructor::MoveFramesToEndOfIBSplit(nsFrameConstructorState& aState,
                                                nsIFrame* aExistingEndFrame,
                                                nsIFrame* aFramesToMove,
                                                nsIFrame* aBlockPart,
                                                nsFrameConstructorState* aTargetState)
{
  NS_PRECONDITION(aBlockPart, "Must have a block part");
  NS_PRECONDITION(aExistingEndFrame, "Must have trailing inline");
  NS_PRECONDITION(aFramesToMove, "Must have frames to move");

  if (aExistingEndFrame->HasView() || aFramesToMove->GetParent()->HasView()) {
    
    nsHTMLContainerFrame::ReparentFrameViewList(aState.mPresContext,
                                                aFramesToMove,
                                                aFramesToMove->GetParent(),
                                                aExistingEndFrame);
  }

  
  nsIFrame* existingFirstChild = aExistingEndFrame->GetFirstChild(nsnull);
  if (!existingFirstChild &&
      (aExistingEndFrame->GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
    aExistingEndFrame->SetInitialChildList(nsnull, aFramesToMove);
  } else {
    aExistingEndFrame->InsertFrames(nsnull, nsnull, aFramesToMove);
  }
  nsFrameConstructorState* startState = aTargetState ? &aState : nsnull;
  MoveChildrenTo(aState.mFrameManager, aExistingEndFrame, aFramesToMove,
                 existingFirstChild, aTargetState, startState);
}

void
nsCSSFrameConstructor::BuildInlineChildItems(nsFrameConstructorState& aState,
                                             FrameConstructionItem& aParentItem)
{
  
  

  
  nsStyleContext* const parentStyleContext = aParentItem.mStyleContext;
  nsIContent* const parentContent = aParentItem.mContent;
  CreateGeneratedContentItem(aState, nsnull, parentContent,
                             parentStyleContext, nsCSSPseudoElements::before,
                             aParentItem.mChildItems);

  ChildIterator iter, last;
  for (ChildIterator::Init(parentContent, &iter, &last);
       iter != last;
       ++iter) {
    
    
    
    nsIContent* content = *iter;
    if (content->IsNodeOfType(nsINode::eCOMMENT) ||
        content->IsNodeOfType(nsINode::ePROCESSING_INSTRUCTION)) {
      continue;
    }

    nsRefPtr<nsStyleContext> childContext =
      ResolveStyleContext(parentStyleContext, content);

    AddFrameConstructionItemsInternal(aState, content, nsnull, content->Tag(),
                                      content->GetNameSpaceID(), childContext,
                                      ITEM_ALLOW_XBL_BASE | ITEM_ALLOW_PAGE_BREAK,
                                      aParentItem.mChildItems);
  }

  
  CreateGeneratedContentItem(aState, nsnull, parentContent,
                             parentStyleContext, nsCSSPseudoElements::after,
                             aParentItem.mChildItems);

  aParentItem.mIsAllInline = aParentItem.mChildItems.AreAllItemsInline();
}

PRBool
nsCSSFrameConstructor::WipeContainingBlock(nsFrameConstructorState& aState,
                                           nsIFrame* aContainingBlock,
                                           nsIFrame* aFrame,
                                           const FrameConstructionItemList& aItems,
                                           PRBool aIsAppend,
                                           nsIFrame* aPrevSibling)
{
  if (aItems.IsEmpty()) {
    return PR_FALSE;
  }
  
  
  

  
  
  if (aFrame->IsBoxFrame() &&
      !(aFrame->GetStateBits() & NS_STATE_BOX_WRAPS_KIDS_IN_BLOCK) &&
      aItems.AnyItemsNeedBlockParent()) {
    RecreateFramesForContent(aFrame->GetContent());
    return PR_TRUE;
  }

  
  
  
  
  

  
  
  
  
  

  if (IsInlineFrame(aFrame)) {
    
    if (aItems.AreAllItemsInline()) {
      return PR_FALSE;
    }
  } else if (!IsFrameSpecial(aFrame)) {
    return PR_FALSE;
  } else {
    
    
    if (aIsAppend) {
      
      
      

      
      
      
      
      
      
      
      nsIFrame* floatContainer = aFrame;
      do {
        floatContainer = GetFloatContainingBlock(
          GetIBSplitSpecialPrevSiblingForAnonymousBlock(floatContainer));
        if (!floatContainer) {
          break;
        }
        if (!IsFrameSpecial(floatContainer)) {
          return PR_FALSE;
        }
      } while (1);
    }
    
    if (aPrevSibling && !aPrevSibling->GetNextSibling()) {
      
      
      if (!aItems.IsStartInline()) {
        return PR_FALSE;
      }
    } else {
      
      
      if (aPrevSibling || !aItems.IsEndInline()) {
        return PR_FALSE;
      }
    }
  }

  
  if (!aContainingBlock) {
    aContainingBlock = aFrame;
  }
  
  
  
  
  
  
  
  
  
  
  
  
  
  while (IsFrameSpecial(aContainingBlock) || IsInlineOutside(aContainingBlock) ||
         aContainingBlock->GetStyleContext()->GetPseudoType()) {
    aContainingBlock = aContainingBlock->GetParent();
    NS_ASSERTION(aContainingBlock,
                 "Must have non-inline, non-special, non-pseudo frame as root "
                 "(or child of root, for a table root)!");
  }

  
  
  

  nsIContent *blockContent = aContainingBlock->GetContent();
  nsCOMPtr<nsIContent> parentContainer = blockContent->GetParent();
#ifdef DEBUG
  if (gNoisyContentUpdates) {
    printf("nsCSSFrameConstructor::WipeContainingBlock: blockContent=%p parentContainer=%p\n",
           static_cast<void*>(blockContent),
           static_cast<void*>(parentContainer));
  }
#endif
  if (parentContainer) {
    ReinsertContent(parentContainer, blockContent);
  }
  else if (blockContent->GetCurrentDoc() == mDocument) {
    ReconstructDocElementHierarchyInternal();
  }
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

  PRBool isReflowing;
  mPresShell->IsReflowLocked(&isReflowing);
  if(isReflowing) {
    
    
    NS_ASSERTION(0, "Atemptted to nsCSSFrameConstructor::ReframeContainingBlock during a Reflow!!!");
    return NS_OK;
  }

  
  nsIFrame* containingBlock = GetIBContainingBlockFor(aFrame);
  if (containingBlock) {
    
    
    
    
   
    
    
    

    
    nsCOMPtr<nsIContent> blockContent = containingBlock->GetContent();
    if (blockContent) {
      
      nsCOMPtr<nsIContent> parentContainer = blockContent->GetParent();
      if (parentContainer) {
#ifdef DEBUG
        if (gNoisyContentUpdates) {
          printf("  ==> blockContent=%p, parentContainer=%p\n",
                 static_cast<void*>(blockContent),
                 static_cast<void*>(parentContainer));
        }
#endif
        return ReinsertContent(parentContainer, blockContent);
      }
    }
  }

  
  return ReconstructDocElementHierarchyInternal();
}

nsresult
nsCSSFrameConstructor::RemoveFixedItems(const nsFrameConstructorState& aState,
                                        nsIFrame *aRootElementFrame)
{
  nsresult rv=NS_OK;

  if (mFixedContainingBlock) {
    nsIFrame *fixedChild = nsnull;
    do {
      fixedChild = mFixedContainingBlock->GetFirstChild(nsGkAtoms::fixedList);
      if (fixedChild && fixedChild == aRootElementFrame) {
        
        
        
        fixedChild = fixedChild->GetNextSibling();
      }
      if (fixedChild) {
        
        
        nsPlaceholderFrame *placeholderFrame =
          aState.mFrameManager->GetPlaceholderFrameFor(fixedChild);
        NS_ASSERTION(placeholderFrame, "no placeholder for fixed-pos frame");
        NS_ASSERTION(placeholderFrame->GetType() ==
                     nsGkAtoms::placeholderFrame,
                     "Wrong type");
        UnregisterPlaceholderChain(aState.mFrameManager, placeholderFrame);
        nsIFrame* placeholderParent = placeholderFrame->GetParent();
        ::DeletingFrameSubtree(aState.mFrameManager, placeholderFrame);
        rv = aState.mFrameManager->RemoveFrame(placeholderParent, nsnull,
                                               placeholderFrame);
        if (NS_FAILED(rv)) {
          NS_WARNING("Error removing placeholder for fixed frame in RemoveFixedItems");
          break;
        }

        ::DeletingFrameSubtree(aState.mFrameManager, fixedChild);
        rv = aState.mFrameManager->RemoveFrame(mFixedContainingBlock,
                                               nsGkAtoms::fixedList,
                                               fixedChild);
        if (NS_FAILED(rv)) {
          NS_WARNING("Error removing frame from fixed containing block in RemoveFixedItems");
          break;
        }
      }
    } while(fixedChild);
  } else {
    NS_WARNING( "RemoveFixedItems called with no FixedContainingBlock data member set");
  }
  return rv;
}

void
nsCSSFrameConstructor::RestyleForAppend(nsIContent* aContainer,
                                        PRInt32 aNewIndexInContainer)
{
  NS_ASSERTION(aContainer, "must have container for append");
#ifdef DEBUG
  {
    for (PRInt32 index = aNewIndexInContainer;; ++index) {
      nsIContent *content = aContainer->GetChildAt(index);
      if (!content) {
        NS_ASSERTION(index != aNewIndexInContainer, "yikes, nothing appended");
        break;
      }
      NS_ASSERTION(!content->IsRootOfAnonymousSubtree(),
                   "anonymous nodes should not be in child lists");
    }
  }
#endif
  PRUint32 selectorFlags =
    aContainer->GetFlags() & (NODE_ALL_SELECTOR_FLAGS &
                              ~NODE_HAS_SLOW_SELECTOR_NOAPPEND);
  if (selectorFlags == 0)
    return;

  if (selectorFlags & NODE_HAS_SLOW_SELECTOR) {
    PostRestyleEvent(aContainer, eReStyle_Self, NS_STYLE_HINT_NONE);
    
    return;
  }

  if (selectorFlags & NODE_HAS_EMPTY_SELECTOR) {
    
    PRBool wasEmpty = PR_TRUE; 
    for (PRInt32 index = 0; index < aNewIndexInContainer; ++index) {
      
      
      
      
      if (nsStyleUtil::IsSignificantChild(aContainer->GetChildAt(index),
                                          PR_TRUE, PR_FALSE)) {
        wasEmpty = PR_FALSE;
        break;
      }
    }
    if (wasEmpty) {
      PostRestyleEvent(aContainer, eReStyle_Self, NS_STYLE_HINT_NONE);
      
      return;
    }
  }
  if (selectorFlags & NODE_HAS_EDGE_CHILD_SELECTOR) {
    
    for (PRInt32 index = aNewIndexInContainer - 1; index >= 0; --index) {
      nsIContent *content = aContainer->GetChildAt(index);
      if (content->IsNodeOfType(nsINode::eELEMENT)) {
        PostRestyleEvent(content, eReStyle_Self, NS_STYLE_HINT_NONE);
        break;
      }
    }
  }
}







void
nsCSSFrameConstructor::RestyleForInsertOrChange(nsIContent* aContainer,
                                                nsIContent* aChild)
{
  NS_ASSERTION(!aChild->IsRootOfAnonymousSubtree(),
               "anonymous nodes should not be in child lists");
  PRUint32 selectorFlags =
    aContainer ? (aContainer->GetFlags() & NODE_ALL_SELECTOR_FLAGS) : 0;
  if (selectorFlags == 0)
    return;

  if (selectorFlags & (NODE_HAS_SLOW_SELECTOR |
                       NODE_HAS_SLOW_SELECTOR_NOAPPEND)) {
    PostRestyleEvent(aContainer, eReStyle_Self, NS_STYLE_HINT_NONE);
    
    return;
  }

  if (selectorFlags & NODE_HAS_EMPTY_SELECTOR) {
    
    PRBool wasEmpty = PR_TRUE; 
    for (PRInt32 index = 0; ; ++index) {
      nsIContent *child = aContainer->GetChildAt(index);
      if (!child) 
        break;
      if (child == aChild)
        continue;
      
      
      
      
      if (nsStyleUtil::IsSignificantChild(child, PR_TRUE, PR_FALSE)) {
        wasEmpty = PR_FALSE;
        break;
      }
    }
    if (wasEmpty) {
      PostRestyleEvent(aContainer, eReStyle_Self, NS_STYLE_HINT_NONE);
      
      return;
    }
  }

  if (selectorFlags & NODE_HAS_EDGE_CHILD_SELECTOR) {
    
    PRBool passedChild = PR_FALSE;
    for (PRInt32 index = 0; ; ++index) {
      nsIContent *content = aContainer->GetChildAt(index);
      if (!content)
        break; 
      if (content == aChild) {
        passedChild = PR_TRUE;
        continue;
      }
      if (content->IsNodeOfType(nsINode::eELEMENT)) {
        if (passedChild) {
          PostRestyleEvent(content, eReStyle_Self, NS_STYLE_HINT_NONE);
        }
        break;
      }
    }
    
    passedChild = PR_FALSE;
    for (PRInt32 index = aContainer->GetChildCount() - 1;
         index >= 0; --index) {
      nsIContent *content = aContainer->GetChildAt(index);
      if (content == aChild) {
        passedChild = PR_TRUE;
        continue;
      }
      if (content->IsNodeOfType(nsINode::eELEMENT)) {
        if (passedChild) {
          PostRestyleEvent(content, eReStyle_Self, NS_STYLE_HINT_NONE);
        }
        break;
      }
    }
  }
}

void
nsCSSFrameConstructor::RestyleForRemove(nsIContent* aContainer,
                                        nsIContent* aOldChild,
                                        PRInt32 aIndexInContainer)
{
  NS_ASSERTION(!aOldChild->IsRootOfAnonymousSubtree(),
               "anonymous nodes should not be in child lists");
  PRUint32 selectorFlags =
    aContainer ? (aContainer->GetFlags() & NODE_ALL_SELECTOR_FLAGS) : 0;
  if (selectorFlags == 0)
    return;

  if (selectorFlags & (NODE_HAS_SLOW_SELECTOR |
                       NODE_HAS_SLOW_SELECTOR_NOAPPEND)) {
    PostRestyleEvent(aContainer, eReStyle_Self, NS_STYLE_HINT_NONE);
    
    return;
  }

  if (selectorFlags & NODE_HAS_EMPTY_SELECTOR) {
    
    PRBool isEmpty = PR_TRUE; 
    for (PRInt32 index = 0; ; ++index) {
      nsIContent *child = aContainer->GetChildAt(index);
      if (!child) 
        break;
      
      
      
      
      if (nsStyleUtil::IsSignificantChild(child, PR_TRUE, PR_FALSE)) {
        isEmpty = PR_FALSE;
        break;
      }
    }
    if (isEmpty) {
      PostRestyleEvent(aContainer, eReStyle_Self, NS_STYLE_HINT_NONE);
      
      return;
    }
  }

  if (selectorFlags & NODE_HAS_EDGE_CHILD_SELECTOR) {
    
    for (PRInt32 index = 0; ; ++index) {
      nsIContent *content = aContainer->GetChildAt(index);
      if (!content)
        break; 
      if (content->IsNodeOfType(nsINode::eELEMENT)) {
        if (index >= aIndexInContainer) {
          PostRestyleEvent(content, eReStyle_Self, NS_STYLE_HINT_NONE);
        }
        break;
      }
    }
    
    for (PRInt32 index = aContainer->GetChildCount() - 1;
         index >= 0; --index) {
      nsIContent *content = aContainer->GetChildAt(index);
      if (content->IsNodeOfType(nsINode::eELEMENT)) {
        if (index < aIndexInContainer) {
          PostRestyleEvent(content, eReStyle_Self, NS_STYLE_HINT_NONE);
        }
        break;
      }
    }
  }
}


static PLDHashOperator
CollectRestyles(nsISupports* aContent,
                nsCSSFrameConstructor::RestyleData& aData,
                void* aRestyleArrayPtr)
{
  nsCSSFrameConstructor::RestyleEnumerateData** restyleArrayPtr =
    static_cast<nsCSSFrameConstructor::RestyleEnumerateData**>
               (aRestyleArrayPtr);
  nsCSSFrameConstructor::RestyleEnumerateData* currentRestyle =
    *restyleArrayPtr;
  currentRestyle->mContent = static_cast<nsIContent*>(aContent);
  currentRestyle->mRestyleHint = aData.mRestyleHint;
  currentRestyle->mChangeHint = aData.mChangeHint;

  
  *restyleArrayPtr = currentRestyle + 1; 

  return PL_DHASH_NEXT;
}

void
nsCSSFrameConstructor::ProcessOneRestyle(nsIContent* aContent,
                                         nsReStyleHint aRestyleHint,
                                         nsChangeHint aChangeHint)
{
  NS_PRECONDITION(aContent, "Must have content node");
  
  if (!aContent->IsInDoc() ||
      aContent->GetCurrentDoc() != mDocument) {
    
    
    return;
  }
  
  nsIFrame* primaryFrame = mPresShell->GetPrimaryFrameFor(aContent);
  if (aRestyleHint & eReStyle_Self) {
    RestyleElement(aContent, primaryFrame, aChangeHint);
  } else if (aChangeHint &&
               (primaryFrame ||
                (aChangeHint & nsChangeHint_ReconstructFrame))) {
    
    nsStyleChangeList changeList;
    changeList.AppendChange(primaryFrame, aContent, aChangeHint);
    ProcessRestyledFrames(changeList);
  }

  if (aRestyleHint & eReStyle_LaterSiblings) {
    RestyleLaterSiblings(aContent);
  }
}

#define RESTYLE_ARRAY_STACKSIZE 128

void
nsCSSFrameConstructor::RebuildAllStyleData(nsChangeHint aExtraHint)
{
  NS_ASSERTION(!(aExtraHint & nsChangeHint_ReconstructFrame),
               "Should not reconstruct the root of the frame tree.  "
               "Use ReconstructDocElementHierarchy instead.");

  mRebuildAllStyleData = PR_FALSE;
  NS_UpdateHint(aExtraHint, mRebuildAllExtraHint);
  mRebuildAllExtraHint = nsChangeHint(0);

  if (!mPresShell || !mPresShell->GetRootFrame() ||
      !mPresShell->GetPresContext()->IsDynamic())
    return;

  nsAutoScriptBlocker scriptBlocker;

  
  nsIViewManager::UpdateViewBatch batch(mPresShell->GetViewManager());

  
  
  nsCOMPtr<nsIPresShell> kungFuDeathGrip(mPresShell);

  
  
  nsresult rv = mPresShell->StyleSet()->BeginReconstruct();
  if (NS_FAILED(rv)) {
    batch.EndUpdateViewBatch(NS_VMREFRESH_NO_SYNC);
    return;
  }

  
  
  
  
  
  
  nsStyleChangeList changeList;
  
  
  mPresShell->FrameManager()->ComputeStyleChangeFor(mPresShell->GetRootFrame(),
                                                    &changeList, aExtraHint);
  
  ProcessRestyledFrames(changeList);
  
  
  
  
  
  mPresShell->StyleSet()->EndReconstruct();
  batch.EndUpdateViewBatch(NS_VMREFRESH_NO_SYNC);
}

void
nsCSSFrameConstructor::ProcessPendingRestyles()
{
  NS_PRECONDITION(mDocument, "No document?  Pshaw!\n");
  NS_PRECONDITION(!nsContentUtils::IsSafeToRunScript(),
                  "Missing a script blocker!");

  PRUint32 count = mPendingRestyles.Count();

  if (count) {
    
    nsAutoTArray<RestyleEnumerateData, RESTYLE_ARRAY_STACKSIZE> restyleArr;
    RestyleEnumerateData* restylesToProcess = restyleArr.AppendElements(count);
  
    if (!restylesToProcess) {
      return;
    }

    RestyleEnumerateData* lastRestyle = restylesToProcess;
    mPendingRestyles.Enumerate(CollectRestyles, &lastRestyle);

    NS_ASSERTION(lastRestyle - restylesToProcess == PRInt32(count),
                 "Enumeration screwed up somehow");

    
    
    mPendingRestyles.Clear();

    
    
    BeginUpdate();

    for (RestyleEnumerateData* currentRestyle = restylesToProcess;
         currentRestyle != lastRestyle;
         ++currentRestyle) {
      ProcessOneRestyle(currentRestyle->mContent,
                        currentRestyle->mRestyleHint,
                        currentRestyle->mChangeHint);
    }

    EndUpdate();

#ifdef DEBUG
    mPresShell->VerifyStyleTree();
#endif
  }

  if (mRebuildAllStyleData) {
    
    
    RebuildAllStyleData(nsChangeHint(0));
  }
}

void
nsCSSFrameConstructor::PostRestyleEvent(nsIContent* aContent,
                                        nsReStyleHint aRestyleHint,
                                        nsChangeHint aMinChangeHint)
{
  if (NS_UNLIKELY(mIsDestroyingFrameTree)) {
    NS_NOTREACHED("PostRestyleEvent after the shell is destroyed (bug 279505)");
    return;
  }

  if (aRestyleHint == 0 && !aMinChangeHint) {
    
    return;
  }

  NS_ASSERTION(aContent->IsNodeOfType(nsINode::eELEMENT),
               "Shouldn't be trying to restyle non-elements directly");

  RestyleData existingData;
  existingData.mRestyleHint = nsReStyleHint(0);
  existingData.mChangeHint = NS_STYLE_HINT_NONE;

  mPendingRestyles.Get(aContent, &existingData);
  existingData.mRestyleHint =
    nsReStyleHint(existingData.mRestyleHint | aRestyleHint);
  NS_UpdateHint(existingData.mChangeHint, aMinChangeHint);

  mPendingRestyles.Put(aContent, existingData);

  PostRestyleEventInternal();
}
    
void
nsCSSFrameConstructor::PostRestyleEventInternal()
{
  if (!mRestyleEvent.IsPending()) {
    nsRefPtr<RestyleEvent> ev = new RestyleEvent(this);
    if (NS_FAILED(NS_DispatchToCurrentThread(ev))) {
      NS_WARNING("failed to dispatch restyle event");
      
    } else {
      mRestyleEvent = ev;
    }
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
  
  PostRestyleEventInternal();
}

NS_IMETHODIMP nsCSSFrameConstructor::RestyleEvent::Run()
{
  if (!mConstructor)
    return NS_OK;  

  
  
  mConstructor->mRestyleEvent.Forget();  
  
  return mConstructor->mPresShell->FlushPendingNotifications(Flush_Style);
}

NS_IMETHODIMP
nsCSSFrameConstructor::LazyGenerateChildrenEvent::Run()
{
  mPresShell->GetDocument()->FlushPendingNotifications(Flush_Layout);

  
  nsIFrame* frame = mPresShell->GetPrimaryFrameFor(mContent);
  if (frame && frame->GetType() == nsGkAtoms::menuPopupFrame) {
#ifdef MOZ_XUL
    
    
    
    nsMenuPopupFrame* menuPopupFrame = static_cast<nsMenuPopupFrame *>(frame);
    if (menuPopupFrame->HasGeneratedChildren()) {
      if (mCallback)
        mCallback(mContent, frame, mArg);
      
      return NS_OK;
    }     

    
    menuPopupFrame->SetGeneratedChildren();
#endif

   {
      nsAutoScriptBlocker scriptBlocker;
      nsCSSFrameConstructor* fc = mPresShell->FrameConstructor();
      fc->BeginUpdate();

      nsFrameItems childItems;
      nsFrameConstructorState state(mPresShell, nsnull, nsnull, nsnull);
      nsresult rv = fc->ProcessChildren(state, mContent, frame->GetStyleContext(),
                                        frame, PR_FALSE, childItems, PR_FALSE);
      if (NS_FAILED(rv)) {
        fc->EndUpdate();
        return rv;
      }

      frame->SetInitialChildList(nsnull, childItems.childList);

      fc->EndUpdate();
    }

    if (mCallback)
      mCallback(mContent, frame, mArg);

    
    mPresShell->GetDocument()->BindingManager()->ProcessAttachedQueue();
  }

  return NS_OK;
}
