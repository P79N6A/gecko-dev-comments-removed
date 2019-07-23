












































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

#include "nsBox.h"

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

nsIFrame*
NS_NewHTMLCanvasFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);

#ifdef MOZ_SVG
#include "nsISVGTextContentMetrics.h"
#include "nsStyleUtil.h"

PRBool
NS_SVGEnabled();
nsIFrame*
NS_NewSVGOuterSVGFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGInnerSVGFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGPathGeometryFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGGFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGGenericContainerFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext);
#ifdef MOZ_SVG_FOREIGNOBJECT
nsIFrame*
NS_NewSVGForeignObjectFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext);
#endif
nsIFrame*
NS_NewSVGAFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGGlyphFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsIFrame* parent, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGTextFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGTSpanFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsIFrame* parent, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGContainerFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGUseFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext);
PRBool 
NS_SVG_TestFeatures (const nsAString& value);
PRBool 
NS_SVG_TestsSupported (const nsIAtom *atom);
PRBool 
NS_SVG_LangSupported (const nsIAtom *atom);
extern nsIFrame*
NS_NewSVGLinearGradientFrame(nsIPresShell *aPresShell, nsIContent *aContent, nsStyleContext* aContext);
extern nsIFrame*
NS_NewSVGRadialGradientFrame(nsIPresShell *aPresShell, nsIContent *aContent, nsStyleContext* aContext);
extern nsIFrame*
NS_NewSVGStopFrame(nsIPresShell *aPresShell, nsIContent *aContent, nsIFrame *aParentFrame, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGMarkerFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext);
extern nsIFrame*
NS_NewSVGImageFrame(nsIPresShell *aPresShell, nsIContent *aContent, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGClipPathFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGTextPathFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsIFrame* parent, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGFilterFrame(nsIPresShell *aPresShell, nsIContent *aContent, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGPatternFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext);
nsIFrame*
NS_NewSVGMaskFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext);
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


static PRBool gGotXBLFormPrefs = PR_FALSE;
static PRBool gUseXBLForms = PR_FALSE;

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



nsIFrame*
NS_NewAutoRepeatBoxFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

nsIFrame*
NS_NewRootBoxFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);

nsIFrame*
NS_NewDocElementBoxFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

nsIFrame*
NS_NewThumbFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);

nsIFrame*
NS_NewDeckFrame (nsIPresShell* aPresShell, nsStyleContext* aContext, nsIBoxLayout* aLayoutManager = nsnull);

nsIFrame*
NS_NewLeafBoxFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);

nsIFrame*
NS_NewStackFrame (nsIPresShell* aPresShell, nsStyleContext* aContext, nsIBoxLayout* aLayoutManager = nsnull);

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
nsresult
NS_NewGridRowLeafLayout ( nsIPresShell* aPresShell, nsIBoxLayout** aNewLayout );
nsIFrame*
NS_NewGridRowLeafFrame (nsIPresShell* aPresShell, nsStyleContext* aContext, PRBool aIsRoot, nsIBoxLayout* aLayout);
nsresult
NS_NewGridRowGroupLayout ( nsIPresShell* aPresShell, nsIBoxLayout** aNewLayout );
nsIFrame*
NS_NewGridRowGroupFrame (nsIPresShell* aPresShell, nsStyleContext* aContext, PRBool aIsRoot, nsIBoxLayout* aLayout);

nsresult
NS_NewListBoxLayout ( nsIPresShell* aPresShell, nsCOMPtr<nsIBoxLayout>& aNewLayout );



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
static PRInt32 FFWC_slowSearchForText=0;
#endif

static nsresult
DeletingFrameSubtree(nsFrameManager* aFrameManager,
                     nsIFrame*       aFrame);

#ifdef  MOZ_SVG

static nsIFrame *
SVG_GetFirstNonAAncestorFrame(nsIFrame *aParentFrame)
{
  for (nsIFrame *ancestorFrame = aParentFrame; ancestorFrame != nsnull;
       ancestorFrame = ancestorFrame->GetParent()) {
    if (ancestorFrame->GetType() != nsGkAtoms::svgAFrame) {
      return ancestorFrame;
    }
  }
  return nsnull;
}


static PRBool
SVG_TestLanguage(const nsSubstring& lstr, const nsSubstring& prefs) 
{
  
  
  
  
  
  PRInt32 vbegin = 0;
  PRInt32 vlen = lstr.Length();
  while (vbegin < vlen) {
    PRInt32 vend = lstr.FindChar(PRUnichar(','), vbegin);
    if (vend == kNotFound) {
      vend = vlen;
    }
    PRInt32 gbegin = 0;
    PRInt32 glen = prefs.Length();
    while (gbegin < glen) {
      PRInt32 gend = prefs.FindChar(PRUnichar(','), gbegin);
      if (gend == kNotFound) {
        gend = glen;
      }
      const nsDefaultStringComparator defaultComparator;
      const nsStringComparator& comparator = 
                  static_cast<const nsStringComparator&>(defaultComparator);
      if (nsStyleUtil::DashMatchCompare(Substring(lstr, vbegin, vend-vbegin),
                                        Substring(prefs, gbegin, gend-gbegin),
                                        comparator)) {
        return PR_TRUE;
      }
      gbegin = gend + 1;
    }
    vbegin = vend + 1;
  }
  return PR_FALSE;
}
#endif

static inline nsIFrame*
GetFieldSetAreaFrame(nsIFrame* aFieldsetFrame)
{
  
  nsIFrame* firstChild = aFieldsetFrame->GetFirstChild(nsnull);
  return firstChild && firstChild->GetNextSibling() ? firstChild->GetNextSibling() : firstChild;
}







static inline PRBool
IsFrameSpecial(nsIFrame* aFrame)
{
  return (aFrame->GetStateBits() & NS_FRAME_IS_SPECIAL) != 0;
}

static void
GetSpecialSibling(nsFrameManager* aFrameManager, nsIFrame* aFrame, nsIFrame** aResult)
{
  
  
  aFrame = aFrame->GetFirstInFlow();

  void* value = aFrame->GetProperty(nsGkAtoms::IBSplitSpecialSibling);

  *aResult = static_cast<nsIFrame*>(value);
}

static nsIFrame*
GetLastSpecialSibling(nsFrameManager* aFrameManager, nsIFrame* aFrame)
{
  for (nsIFrame *frame = aFrame, *next; ; frame = next) {
    GetSpecialSibling(aFrameManager, frame, &next);
    if (!next)
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



static PRBool
IsInlineOutside(nsIFrame* aFrame)
{
  return aFrame->GetStyleDisplay()->IsInlineOutside();
}

static PRBool
IsBlockOutside(nsIFrame* aFrame)
{
  return aFrame->GetStyleDisplay()->IsBlockOutside();
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
MarkIBSpecialPrevSibling(nsPresContext* aPresContext,
                         nsIFrame *aAnonymousFrame,
                         nsIFrame *aSpecialParent)
{
  aPresContext->PropertyTable()->SetProperty(aAnonymousFrame,
                                      nsGkAtoms::IBSplitSpecialPrevSibling,
                                             aSpecialParent, nsnull, nsnull);
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

  
  PRBool RemoveChild(nsIFrame* aChild);
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
nsFrameItems::RemoveChild(nsIFrame* aFrame)
{
  NS_PRECONDITION(aFrame, "null ptr");
  nsIFrame* prev = nsnull;
  nsIFrame* sib = childList;
  for (; sib && sib != aFrame; sib = sib->GetNextSibling()) {
    prev = sib;
  }
  if (!sib) {
    return PR_FALSE;
  }
  if (sib == childList) {
    childList = sib->GetNextSibling();
  } else {
    prev->SetNextSibling(sib->GetNextSibling());
  }
  if (sib == lastChild) {
    lastChild = prev;
  }
  sib->SetNextSibling(nsnull);
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




class nsFrameConstructorSaveState {
public:
  nsFrameConstructorSaveState();
  ~nsFrameConstructorSaveState();

private:
  nsAbsoluteItems* mItems;                
  PRBool*          mFirstLetterStyle;
  PRBool*          mFirstLineStyle;

  nsAbsoluteItems  mSavedItems;           
  PRBool           mSavedFirstLetterStyle;
  PRBool           mSavedFirstLineStyle;

  
  nsIAtom* mChildListName;
  nsFrameConstructorState* mState;

  friend class nsFrameConstructorState;
};



class nsFrameConstructorState {
public:
  nsPresContext            *mPresContext;
  nsIPresShell             *mPresShell;
  nsFrameManager           *mFrameManager;

#ifdef MOZ_XUL
  
  nsIRootBox*               mRootBox;
  
  nsAbsoluteItems           mPopupItems;
#endif

  
  nsAbsoluteItems           mFixedItems;
  nsAbsoluteItems           mAbsoluteItems;
  nsAbsoluteItems           mFloatedItems;
  PRBool                    mFirstLetterStyle;
  PRBool                    mFirstLineStyle;
  nsCOMPtr<nsILayoutHistoryState> mFrameState;
  nsPseudoFrames            mPseudoFrames;

  
  
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
                                nsFrameConstructorSaveState& aSaveState,
                                PRBool aFirstLetterStyle,
                                PRBool aFirstLineStyle);

  
  
  
  
  nsIFrame* GetGeometricParent(const nsStyleDisplay* aStyleDisplay,
                               nsIFrame* aContentParentFrame);

  























  nsresult AddChild(nsIFrame* aNewFrame,
                    nsFrameItems& aFrameItems,
                    const nsStyleDisplay* aStyleDisplay,
                    nsIContent* aContent,
                    nsStyleContext* aStyleContext,
                    nsIFrame* aParentFrame,
                    PRBool aCanBePositioned = PR_TRUE,
                    PRBool aCanBeFloated = PR_TRUE,
                    PRBool aIsOutOfFlowPopup = PR_FALSE,
                    PRBool aInsertAfter = PR_FALSE,
                    nsIFrame* aInsertAfterFrame = nsnull);

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
    mRootBox(nsIRootBox::GetRootBox(aPresShell)),
    mPopupItems(mRootBox ? mRootBox->GetPopupSetFrame() : nsnull),
#endif
    mFixedItems(aFixedContainingBlock),
    mAbsoluteItems(aAbsoluteContainingBlock),
    mFloatedItems(aFloatContainingBlock),
    mFirstLetterStyle(PR_FALSE),
    mFirstLineStyle(PR_FALSE),
    mFrameState(aHistoryState),
    mPseudoFrames()
{
}

nsFrameConstructorState::nsFrameConstructorState(nsIPresShell* aPresShell,
                                                 nsIFrame*     aFixedContainingBlock,
                                                 nsIFrame*     aAbsoluteContainingBlock,
                                                 nsIFrame*     aFloatContainingBlock)
  : mPresContext(aPresShell->GetPresContext()),
    mPresShell(aPresShell),
    mFrameManager(aPresShell->FrameManager()),
#ifdef MOZ_XUL    
    mRootBox(nsIRootBox::GetRootBox(aPresShell)),
    mPopupItems(mRootBox ? mRootBox->GetPopupSetFrame() : nsnull),
#endif
    mFixedItems(aFixedContainingBlock),
    mAbsoluteItems(aAbsoluteContainingBlock),
    mFloatedItems(aFloatContainingBlock),
    mFirstLetterStyle(PR_FALSE),
    mFirstLineStyle(PR_FALSE),
    mPseudoFrames()
{
  mFrameState = aPresShell->GetDocument()->GetLayoutHistoryState();
}

nsFrameConstructorState::~nsFrameConstructorState()
{
  
  
  
  
  
  
  
  
  ProcessFrameInsertions(mFloatedItems, nsGkAtoms::floatList);
  ProcessFrameInsertions(mAbsoluteItems, nsGkAtoms::absoluteList);
  ProcessFrameInsertions(mFixedItems, nsGkAtoms::fixedList);
#ifdef MOZ_XUL
  ProcessFrameInsertions(mPopupItems, nsGkAtoms::popupList);
#endif
}

static nsIFrame*
AdjustAbsoluteContainingBlock(nsPresContext* aPresContext,
                              nsIFrame*       aContainingBlockIn)
{
  if (!aContainingBlockIn) {
    return nsnull;
  }
  
  
  return aContainingBlockIn->GetFirstInFlow();
}

void
nsFrameConstructorState::PushAbsoluteContainingBlock(nsIFrame* aNewAbsoluteContainingBlock,
                                                     nsFrameConstructorSaveState& aSaveState)
{
  aSaveState.mItems = &mAbsoluteItems;
  aSaveState.mSavedItems = mAbsoluteItems;
  aSaveState.mChildListName = nsGkAtoms::absoluteList;
  aSaveState.mState = this;
  mAbsoluteItems = 
    nsAbsoluteItems(AdjustAbsoluteContainingBlock(mPresContext,
                                                  aNewAbsoluteContainingBlock));
}

void
nsFrameConstructorState::PushFloatContainingBlock(nsIFrame* aNewFloatContainingBlock,
                                                  nsFrameConstructorSaveState& aSaveState,
                                                  PRBool aFirstLetterStyle,
                                                  PRBool aFirstLineStyle)
{
  
  
  
  NS_PRECONDITION(!aNewFloatContainingBlock ||
                  aNewFloatContainingBlock->GetContentInsertionFrame()->
                    IsFloatContainingBlock(),
                  "Please push a real float containing block!");
  aSaveState.mItems = &mFloatedItems;
  aSaveState.mFirstLetterStyle = &mFirstLetterStyle;
  aSaveState.mFirstLineStyle = &mFirstLineStyle;
  aSaveState.mSavedItems = mFloatedItems;
  aSaveState.mSavedFirstLetterStyle = mFirstLetterStyle;
  aSaveState.mSavedFirstLineStyle = mFirstLineStyle;
  aSaveState.mChildListName = nsGkAtoms::floatList;
  aSaveState.mState = this;
  mFloatedItems = nsAbsoluteItems(aNewFloatContainingBlock);
  mFirstLetterStyle = aFirstLetterStyle;
  mFirstLineStyle = aFirstLineStyle;
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
      mFixedItems.containingBlock) {
    return mFixedItems.containingBlock;
  }

  return aContentParentFrame;
}

nsresult
nsFrameConstructorState::AddChild(nsIFrame* aNewFrame,
                                  nsFrameItems& aFrameItems,
                                  const nsStyleDisplay* aStyleDisplay,
                                  nsIContent* aContent,
                                  nsStyleContext* aStyleContext,
                                  nsIFrame* aParentFrame,
                                  PRBool aCanBePositioned,
                                  PRBool aCanBeFloated,
                                  PRBool aIsOutOfFlowPopup,
                                  PRBool aInsertAfter,
                                  nsIFrame* aInsertAfterFrame)
{
  
  

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
  if (aCanBeFloated && aStyleDisplay->IsFloating() &&
      mFloatedItems.containingBlock) {
    NS_ASSERTION(aNewFrame->GetParent() == mFloatedItems.containingBlock,
                 "Float whose parent is not the float containing block?");
    needPlaceholder = PR_TRUE;
    frameItems = &mFloatedItems;
  }
  else if (aCanBePositioned) {
    if (aStyleDisplay->mPosition == NS_STYLE_POSITION_ABSOLUTE &&
        mAbsoluteItems.containingBlock) {
      NS_ASSERTION(aNewFrame->GetParent() == mAbsoluteItems.containingBlock,
                   "Abs pos whose parent is not the abs pos containing block?");
      needPlaceholder = PR_TRUE;
      frameItems = &mAbsoluteItems;
    }
    if (aStyleDisplay->mPosition == NS_STYLE_POSITION_FIXED &&
        mFixedItems.containingBlock) {
      NS_ASSERTION(aNewFrame->GetParent() == mFixedItems.containingBlock,
                   "Fixed pos whose parent is not the fixed pos containing block?");
      needPlaceholder = PR_TRUE;
      frameItems = &mFixedItems;
    }
  }

  if (needPlaceholder) {
    NS_ASSERTION(frameItems != &aFrameItems,
                 "Putting frame in-flow _and_ want a placeholder?");
    nsIFrame* placeholderFrame;
    nsresult rv =
      nsCSSFrameConstructor::CreatePlaceholderFrameFor(mPresShell,
                                                       mPresContext,
                                                       mFrameManager,
                                                       aContent,
                                                       aNewFrame,
                                                       aStyleContext,
                                                       aParentFrame,
                                                       &placeholderFrame);
    if (NS_FAILED(rv)) {
      
      
      
      
      CleanupFrameReferences(mFrameManager, aNewFrame);
      aNewFrame->Destroy();
      return rv;
    }

    
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
    GetSpecialSibling(mFrameManager, specialSibling, &specialSibling);
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
    mFirstLetterStyle(nsnull),
    mFirstLineStyle(nsnull),
    mSavedItems(nsnull),
    mSavedFirstLetterStyle(PR_FALSE),
    mSavedFirstLineStyle(PR_FALSE),
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
  if (mFirstLetterStyle) {
    *mFirstLetterStyle = mSavedFirstLetterStyle;
  }
  if (mFirstLineStyle) {
    *mFirstLineStyle = mSavedFirstLineStyle;
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

      if (aOuterState.mFloatedItems.RemoveChild(outOfFlowFrame)) {
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
               nsStyleContext*          aNewParentSC,
               nsIFrame*                aNewParent,
               nsIFrame*                aFrameList,
               nsFrameConstructorState* aState,
               nsFrameConstructorState* aOuterState)
{
  PRBool setHasChildWithView = PR_FALSE;

  while (aFrameList) {
    if (!setHasChildWithView
        && (aFrameList->GetStateBits() & (NS_FRAME_HAS_VIEW | NS_FRAME_HAS_CHILD_WITH_VIEW))) {
      setHasChildWithView = PR_TRUE;
    }

    aFrameList->SetParent(aNewParent);

    
    
    
    if (aState) {
      NS_ASSERTION(aOuterState, "need an outer state too");
      AdjustFloatParentPtrs(aFrameList, *aState, *aOuterState);
    }

#if 0
    
    
    
    
    
    
    
    if (aNewParentSC)
      aPresContext->FrameManager()->ReParentStyleContext(aFrameList,
                                                         aNewParentSC);
#endif

    aFrameList = aFrameList->GetNextSibling();
  }

  if (setHasChildWithView) {
    aNewParent->AddStateBits(NS_FRAME_HAS_CHILD_WITH_VIEW);
  }
}






struct nsAutoEnqueueBinding
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
  
  
  if (aChildFrame->GetStateBits() & NS_FRAME_OUT_OF_FLOW) {
    
    const nsStyleDisplay* disp = aChildFrame->GetStyleDisplay();
    
    if (NS_STYLE_POSITION_ABSOLUTE == disp->mPosition) {
      listName = nsGkAtoms::absoluteList;
    } else if (NS_STYLE_POSITION_FIXED == disp->mPosition) {
      listName = nsGkAtoms::fixedList;
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
  , mInitialContainingBlock(nsnull)
  , mFixedContainingBlock(nsnull)
  , mDocElementContainingBlock(nsnull)
  , mGfxScrollFrame(nsnull)
  , mPageSequenceFrame(nsnull)
  , mUpdateCount(0)
  , mQuotesDirty(PR_FALSE)
  , mCountersDirty(PR_FALSE)
  , mInitialContainingBlockIsAbsPosContainer(PR_FALSE)
  , mIsDestroyingFrameTree(PR_FALSE)
{
  if (!gGotXBLFormPrefs) {
    gGotXBLFormPrefs = PR_TRUE;

    gUseXBLForms =
      nsContentUtils::GetBoolPref("nglayout.debug.enable_xbl_forms");
  }

  
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
  if (aFrame->GetStateBits() & NS_FRAME_GENERATED_CONTENT) {
    if (mQuoteList.DestroyNodesFor(aFrame))
      QuotesDirty();
  }

  if (mCounterManager.DestroyNodesFor(aFrame)) {
    
    
    
    CountersDirty();
  }
}

nsresult
nsCSSFrameConstructor::CreateAttributeContent(nsIContent* aParentContent,
                                              nsIFrame* aParentFrame,
                                              PRInt32 aAttrNamespace,
                                              nsIAtom* aAttrName,
                                              nsStyleContext* aStyleContext,
                                              nsCOMArray<nsIContent>& aGeneratedContent,
                                              nsIContent** aNewContent,
                                              nsIFrame** aNewFrame)
{
  *aNewFrame = nsnull;
  *aNewContent = nsnull;
  nsCOMPtr<nsIContent> content;
  nsresult rv = NS_NewAttributeContent(mDocument->NodeInfoManager(),
                                       aAttrNamespace, aAttrName,
                                       getter_AddRefs(content));
  NS_ENSURE_SUCCESS(rv, rv);

  content->SetNativeAnonymous(PR_TRUE);

  
  rv = content->BindToTree(mDocument, aParentContent, content, PR_TRUE);
  if (NS_FAILED(rv)) {
    content->UnbindFromTree();
    return rv;
  }

  
  nsIFrame* textFrame = NS_NewTextFrame(mPresShell, aStyleContext);
  rv = textFrame->Init(content, aParentFrame, nsnull);
  if (NS_SUCCEEDED(rv)) {
    if (NS_UNLIKELY(!aGeneratedContent.AppendObject(content))) {
      rv = NS_ERROR_OUT_OF_MEMORY;
    }
  }

  if (NS_FAILED(rv)) {
    content->UnbindFromTree();
    textFrame->Destroy();
    textFrame = nsnull;
    content = nsnull;
  }

  *aNewFrame = textFrame;
  content.swap(*aNewContent);
  return rv;
}

nsresult
nsCSSFrameConstructor::CreateGeneratedFrameFor(nsIFrame*             aParentFrame,
                                               nsIContent*           aContent,
                                               nsStyleContext*       aStyleContext,
                                               const nsStyleContent* aStyleContent,
                                               PRUint32              aContentIndex,
                                               nsCOMArray<nsIContent>& aGeneratedContent,
                                               nsIFrame**            aFrame)
{
  *aFrame = nsnull;  

  
  nsCOMPtr<nsIDOMCharacterData>* textPtr = nsnull;

  
  const nsStyleContentData &data = aStyleContent->ContentAt(aContentIndex);
  nsStyleContentType  type = data.mType;

  nsCOMPtr<nsIContent> content;

  if (eStyleContentType_Image == type) {
    if (!data.mContent.mImage) {
      
      
      return NS_ERROR_FAILURE;
    }
    
    
    

    nsCOMPtr<nsINodeInfo> nodeInfo;
    mDocument->NodeInfoManager()->GetNodeInfo(nsGkAtoms::img, nsnull,
                                              kNameSpaceID_None,
                                              getter_AddRefs(nodeInfo));

    nsresult rv = NS_NewGenConImageContent(getter_AddRefs(content), nodeInfo,
                                           data.mContent.mImage);
    NS_ENSURE_SUCCESS(rv, rv);

    content->SetNativeAnonymous(PR_TRUE);
  
    
    
    
    
    
    rv = content->BindToTree(mDocument, aContent, content, PR_TRUE);
    if (NS_FAILED(rv)) {
      content->UnbindFromTree();
      return rv;
    }
    
    
    nsIFrame* imageFrame = NS_NewImageFrame(mPresShell, aStyleContext);
    if (NS_UNLIKELY(!imageFrame)) {
      content->UnbindFromTree();
      return NS_ERROR_OUT_OF_MEMORY;
    }

    rv = imageFrame->Init(content, aParentFrame, nsnull);
    if (NS_FAILED(rv) || NS_UNLIKELY(!aGeneratedContent.AppendObject(content))) {
      content->UnbindFromTree();
      imageFrame->Destroy();
      return NS_FAILED(rv) ? rv : NS_ERROR_OUT_OF_MEMORY;
    }

    
    *aFrame = imageFrame;

  } else {

    nsAutoString contentString;

    switch (type) {
    case eStyleContentType_String:
      contentString = data.mContent.mString;
      break;
  
    case eStyleContentType_Attr:
      {
        nsCOMPtr<nsIAtom> attrName;
        PRInt32 attrNameSpace = kNameSpaceID_None;
        contentString = data.mContent.mString;
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
          return NS_ERROR_OUT_OF_MEMORY;
        }

        nsresult rv =
          CreateAttributeContent(aContent, aParentFrame, attrNameSpace,
                                 attrName, aStyleContext, aGeneratedContent,
                                 getter_AddRefs(content), aFrame);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      break;
  
    case eStyleContentType_Counter:
    case eStyleContentType_Counters:
      {
        nsCSSValue::Array *counters = data.mContent.mCounters;
        nsCounterList *counterList = mCounterManager.CounterListFor(
            nsDependentString(counters->Item(0).GetStringBufferValue()));
        if (!counterList)
            return NS_ERROR_OUT_OF_MEMORY;

        nsCounterUseNode* node =
          new nsCounterUseNode(counters, aParentFrame, aContentIndex,
                               type == eStyleContentType_Counters);
        if (!node)
          return NS_ERROR_OUT_OF_MEMORY;

        counterList->Insert(node);
        PRBool dirty = counterList->IsDirty();
        if (!dirty) {
          if (counterList->IsLast(node)) {
            node->Calc(counterList);
            node->GetText(contentString);
          }
          
          
          
          else {
            counterList->SetDirty();
            CountersDirty();
          }
        }

        textPtr = &node->mText; 
      }
      break;

    case eStyleContentType_Image:
      NS_NOTREACHED("handled by if above");
      return NS_ERROR_UNEXPECTED;
  
    case eStyleContentType_OpenQuote:
    case eStyleContentType_CloseQuote:
    case eStyleContentType_NoOpenQuote:
    case eStyleContentType_NoCloseQuote:
      {
        nsQuoteNode* node = new nsQuoteNode(type, aParentFrame, aContentIndex);
        if (!node)
          return NS_ERROR_OUT_OF_MEMORY;
        mQuoteList.Insert(node);
        if (mQuoteList.IsLast(node))
          mQuoteList.Calc(node);
        else
          QuotesDirty();

        
        
        if (node->IsHiddenQuote())
          return NS_OK;

        textPtr = &node->mText; 
        contentString = *node->Text();
      }
      break;
  
    case eStyleContentType_AltContent:
      {
        
        
        
        nsresult rv = NS_OK;
        if (aContent->HasAttr(kNameSpaceID_None, nsGkAtoms::alt)) {
          rv = CreateAttributeContent(aContent, aParentFrame,
                                      kNameSpaceID_None, nsGkAtoms::alt,
                                      aStyleContext, aGeneratedContent,
                                      getter_AddRefs(content), aFrame);
        } else if (aContent->IsNodeOfType(nsINode::eHTML) &&
                   aContent->NodeInfo()->Equals(nsGkAtoms::input)) {
          if (aContent->HasAttr(kNameSpaceID_None, nsGkAtoms::value)) {
            rv = CreateAttributeContent(aContent, aParentFrame,
                                        kNameSpaceID_None, nsGkAtoms::value,
                                        aStyleContext, aGeneratedContent,
                                        getter_AddRefs(content), aFrame);
          } else {
            nsXPIDLString temp;
            rv = nsContentUtils::GetLocalizedString(nsContentUtils::eFORMS_PROPERTIES,
                                                    "Submit", temp);
            contentString = temp;
          }
        } else {
          *aFrame = nsnull;
          rv = NS_ERROR_NOT_AVAILABLE;
          return rv; 
        }
        NS_ENSURE_SUCCESS(rv, rv);
      }
      break;
    } 
  

    if (!content) {
      
      nsIFrame* textFrame = nsnull;
      nsCOMPtr<nsIContent> textContent;
      NS_NewTextNode(getter_AddRefs(textContent),
                     mDocument->NodeInfoManager());
      if (textContent) {
        
        textContent->SetText(contentString, PR_TRUE);

        if (textPtr) {
          *textPtr = do_QueryInterface(textContent);
          NS_ASSERTION(*textPtr, "must implement nsIDOMCharacterData");
        }

        textContent->SetNativeAnonymous(PR_TRUE);

        
        nsresult rv = textContent->BindToTree(mDocument, aContent, textContent,
                                              PR_TRUE);
        if (NS_FAILED(rv)) {
          textContent->UnbindFromTree();
          return rv;
        }

        
        textFrame = NS_NewTextFrame(mPresShell, aStyleContext);
        if (!textFrame) {
          
          
          NS_NOTREACHED("this OOM case isn't handled very well");
          return NS_ERROR_OUT_OF_MEMORY;
        }

        textFrame->Init(textContent, aParentFrame, nsnull);

        content = textContent;
        if (NS_UNLIKELY(!aGeneratedContent.AppendObject(content))) {
          NS_NOTREACHED("this OOM case isn't handled very well");
          return NS_ERROR_OUT_OF_MEMORY;
        }
      } else {
        
        
        NS_NOTREACHED("this OOM case isn't handled very well");
      }

      
      *aFrame = textFrame;
    }
  }

  return NS_OK;
}







PRBool
nsCSSFrameConstructor::CreateGeneratedContentFrame(nsFrameConstructorState& aState,
                                                   nsIFrame*        aFrame,
                                                   nsIContent*      aContent,
                                                   nsStyleContext*  aStyleContext,
                                                   nsIAtom*         aPseudoElement,
                                                   nsIFrame**       aResult)
{
  *aResult = nsnull; 

  if (!aContent->IsNodeOfType(nsINode::eELEMENT))
    return PR_FALSE;

  nsStyleSet *styleSet = mPresShell->StyleSet();

  
  nsRefPtr<nsStyleContext> pseudoStyleContext;
  pseudoStyleContext = styleSet->ProbePseudoStyleFor(aContent,
                                                     aPseudoElement,
                                                     aStyleContext);

  if (pseudoStyleContext) {
    
    

    
    
    nsIFrame*     containerFrame;
    nsFrameItems  childFrames;
    nsresult rv;

    const PRUint8 disp = pseudoStyleContext->GetStyleDisplay()->mDisplay;
    if (disp == NS_STYLE_DISPLAY_BLOCK ||
        disp == NS_STYLE_DISPLAY_INLINE_BLOCK) {
      PRUint32 flags = 0;
      if (disp == NS_STYLE_DISPLAY_INLINE_BLOCK) {
        flags = NS_BLOCK_SPACE_MGR | NS_BLOCK_MARGIN_ROOT;
      }
      containerFrame = NS_NewBlockFrame(mPresShell, pseudoStyleContext, flags);
    } else {
      containerFrame = NS_NewInlineFrame(mPresShell, pseudoStyleContext);
    }

    if (NS_UNLIKELY(!containerFrame)) {
      return PR_FALSE;
    }
    InitAndRestoreFrame(aState, aContent, aFrame, nsnull, containerFrame);
    
    nsHTMLContainerFrame::CreateViewForFrame(containerFrame, nsnull, PR_FALSE);

    
    containerFrame->AddStateBits(NS_FRAME_GENERATED_CONTENT);

    
    
    
    
    nsCOMArray<nsIContent>* generatedContent = new nsCOMArray<nsIContent>;
    rv = containerFrame->SetProperty(nsGkAtoms::generatedContent,
                                     generatedContent);
    if (NS_UNLIKELY(!generatedContent) || NS_FAILED(rv)) {
      containerFrame->Destroy(); 
      delete generatedContent;
      return PR_FALSE;
    }

    
    
    nsRefPtr<nsStyleContext> textStyleContext;
    textStyleContext = styleSet->ResolveStyleForNonElement(pseudoStyleContext);

    
    

    const nsStyleContent* styleContent = pseudoStyleContext->GetStyleContent();
    PRUint32 contentCount = styleContent->ContentCount();
    for (PRUint32 contentIndex = 0; contentIndex < contentCount; contentIndex++) {
      nsIFrame* frame;

      
      rv = CreateGeneratedFrameFor(containerFrame,
                                   aContent, textStyleContext,
                                   styleContent, contentIndex,
                                   *generatedContent, &frame);
      
      if (NS_SUCCEEDED(rv) && frame) {
        
        childFrames.AddChild(frame);
      }
    }

    if (childFrames.childList) {
      containerFrame->SetInitialChildList(nsnull, childFrames.childList);
    }
    *aResult = containerFrame;
    return PR_TRUE;
  }

  return PR_FALSE;
}

nsresult
nsCSSFrameConstructor::CreateInputFrame(nsFrameConstructorState& aState,
                                        nsIContent*              aContent,
                                        nsIFrame*                aParentFrame,
                                        nsIAtom*                 aTag,
                                        nsStyleContext*          aStyleContext,
                                        nsIFrame**               aFrame,
                                        const nsStyleDisplay*    aStyleDisplay,
                                        PRBool&                  aFrameHasBeenInitialized,
                                        PRBool&                  aAddedToFrameList,
                                        nsFrameItems&            aFrameItems)
{
  
  
  
  
  
  nsCOMPtr<nsIFormControl> control = do_QueryInterface(aContent);
  NS_ASSERTION(control, "input is not an nsIFormControl!");

  switch (control->GetType()) {
    case NS_FORM_INPUT_SUBMIT:
    case NS_FORM_INPUT_RESET:
    case NS_FORM_INPUT_BUTTON:
    {
      if (gUseXBLForms)
        return NS_OK; 

      nsresult rv = ConstructButtonFrame(aState, aContent, aParentFrame,
                                         aTag, aStyleContext, aFrame,
                                         aStyleDisplay, aFrameItems);
      aAddedToFrameList = PR_TRUE;
      aFrameHasBeenInitialized = PR_TRUE;
      return rv;
    }

    case NS_FORM_INPUT_CHECKBOX:
      if (gUseXBLForms)
        return NS_OK; 
      return ConstructCheckboxControlFrame(aFrame, aContent, aStyleContext);

    case NS_FORM_INPUT_RADIO:
      if (gUseXBLForms)
        return NS_OK; 
      return ConstructRadioControlFrame(aFrame, aContent, aStyleContext);

    case NS_FORM_INPUT_FILE:
    {
      *aFrame = NS_NewFileControlFrame(mPresShell, aStyleContext);

      if (*aFrame) {
        
        (*aFrame)->AddStateBits(NS_BLOCK_SPACE_MGR);
        return NS_OK;
      }
      else {
        return NS_ERROR_OUT_OF_MEMORY;
      }
    }

    case NS_FORM_INPUT_HIDDEN:
      return NS_OK; 
                    

    case NS_FORM_INPUT_IMAGE:
      return CreateHTMLImageFrame(aContent, aStyleContext,
                                  NS_NewImageControlFrame, aFrame);

    case NS_FORM_INPUT_TEXT:
    case NS_FORM_INPUT_PASSWORD:
    {
      *aFrame = NS_NewTextControlFrame(mPresShell, aStyleContext);
      
      return NS_UNLIKELY(!*aFrame) ? NS_ERROR_OUT_OF_MEMORY : NS_OK;
    }

    default:
      NS_ASSERTION(0, "Unknown input type!");
      return NS_ERROR_INVALID_ARG;
  }
}

nsresult
nsCSSFrameConstructor::CreateHTMLImageFrame(nsIContent* aContent,
                                            nsStyleContext* aStyleContext,
                                            ImageFrameCreatorFunc aFunc,
                                            nsIFrame** aFrame)
{
  *aFrame = nsnull;

  
  if (nsImageFrame::ShouldCreateImageFrameFor(aContent, aStyleContext)) {
    *aFrame = (*aFunc)(mPresShell, aStyleContext);
     
    if (NS_UNLIKELY(!*aFrame))
      return NS_ERROR_OUT_OF_MEMORY;
  }

  return NS_OK;
}

static PRBool
TextIsOnlyWhitespace(nsIContent* aContent)
{
  return aContent->IsNodeOfType(nsINode::eTEXT) &&
         aContent->TextIsOnlyWhitespace();
}
    










static PRBool
IsTableRelated(PRUint8 aDisplay,
               PRBool  aIncludeSpecial) 
{
  if ((aDisplay == NS_STYLE_DISPLAY_TABLE)              ||
      (aDisplay == NS_STYLE_DISPLAY_INLINE_TABLE)       ||
      (aDisplay == NS_STYLE_DISPLAY_TABLE_HEADER_GROUP) ||
      (aDisplay == NS_STYLE_DISPLAY_TABLE_ROW_GROUP)    ||
      (aDisplay == NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP) ||
      (aDisplay == NS_STYLE_DISPLAY_TABLE_ROW)) {
    return PR_TRUE;
  }
  else if (aIncludeSpecial && 
           ((aDisplay == NS_STYLE_DISPLAY_TABLE_CAPTION)      ||
            (aDisplay == NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP) ||
            (aDisplay == NS_STYLE_DISPLAY_TABLE_COLUMN)       ||
            (aDisplay == NS_STYLE_DISPLAY_TABLE_CELL))) {
    return PR_TRUE;
  }
  else return PR_FALSE;
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
                           PR_TRUE, items, PR_TRUE, pseudoOuter.mFrame, 
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

  
  PRBool pseudoParent;
  nsFrameItems items;
  rv = ConstructTableRowGroupFrame(aState, parentContent,
                                   parentFrame, childStyle, aNameSpaceID,
                                   PR_TRUE, items, pseudo.mFrame, pseudoParent);
  if (NS_FAILED(rv)) return rv;

  
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

  
  PRBool pseudoParent;
  nsFrameItems items;
  rv = ConstructTableColGroupFrame(aState, parentContent,
                                   parentFrame, childStyle, aNameSpaceID,
                                   PR_TRUE, items, pseudo.mFrame, pseudoParent);
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

  
  PRBool pseudoParent;
  nsFrameItems items;
  rv = ConstructTableRowFrame(aState, parentContent,
                              parentFrame, childStyle, aNameSpaceID,
                              PR_TRUE, items, pseudo.mFrame, pseudoParent);
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

  
  PRBool pseudoParent;
  nsFrameItems items;
  rv = ConstructTableCellFrame(aState, parentContent, parentFrame, childStyle,
                               aNameSpaceID, PR_TRUE, items,
                               pseudoOuter.mFrame, pseudoInner.mFrame,
                               pseudoParent);
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
nsCSSFrameConstructor::GetParentFrame(PRInt32                  aNameSpaceID,
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

  if (nsGkAtoms::tableOuterFrame == aChildFrameType) { 
    if (IsTableRelated(parentFrameType, PR_TRUE) &&
        (nsGkAtoms::tableCaptionFrame != parentFrameType) ) { 
      rv = GetPseudoCellFrame(aNameSpaceID, aState, aParentFrameIn);
      if (NS_FAILED(rv)) return rv;
      pseudoParentFrame = pseudoFrames.mCellInner.mFrame;
    }
  } 
  else if (nsGkAtoms::tableCaptionFrame == aChildFrameType) { 
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
  else if (nsGkAtoms::tableFrame == aChildFrameType) { 
    NS_ASSERTION(PR_FALSE, "GetParentFrame called on nsGkAtoms::tableFrame child");
  }
  else { 
    if (IsTableRelated(parentFrameType, PR_FALSE)) { 
      rv = GetPseudoCellFrame(aNameSpaceID, aState, aParentFrameIn);
      if (NS_FAILED(rv)) return rv;
      pseudoParentFrame = pseudoFrames.mCellInner.mFrame;
    }
  }
  
  if (pseudoParentFrame) {
    aParentFrame = pseudoParentFrame;
    aIsPseudoParent = PR_TRUE;
  }

  return rv;
}

static PRBool
IsSpecialContent(nsIContent*     aContent,
                 nsIAtom*        aTag,
                 PRInt32         aNameSpaceID,
                 nsStyleContext* aStyleContext)
{
  
  
  
  if (aContent->IsNodeOfType(nsINode::eHTML) ||
      aNameSpaceID == kNameSpaceID_XHTML) {
    
    

    if (aTag == nsGkAtoms::input) {
      nsCOMPtr<nsIFormControl> control = do_QueryInterface(aContent);
      if (control) {
        PRInt32 type = control->GetType();
        if (NS_FORM_INPUT_HIDDEN == type) {
          return PR_FALSE; 
        }
        else if (NS_FORM_INPUT_IMAGE == type) {
          return nsImageFrame::ShouldCreateImageFrameFor(aContent, aStyleContext);
        }
      }

      return PR_TRUE;
    }

    if (aTag == nsGkAtoms::img) {
      return nsImageFrame::ShouldCreateImageFrameFor(aContent, aStyleContext);
    }

    if (aTag == nsGkAtoms::object ||
        aTag == nsGkAtoms::applet ||
        aTag == nsGkAtoms::embed) {
      return !(aContent->IntrinsicState() &
             (NS_EVENT_STATE_BROKEN | NS_EVENT_STATE_USERDISABLED |
              NS_EVENT_STATE_SUPPRESSED));
    }

    return
      aTag == nsGkAtoms::br ||
      aTag == nsGkAtoms::wbr ||
      aTag == nsGkAtoms::textarea ||
      aTag == nsGkAtoms::select ||
      aTag == nsGkAtoms::fieldset ||
      aTag == nsGkAtoms::legend ||
      aTag == nsGkAtoms::frameset ||
      aTag == nsGkAtoms::iframe ||
      aTag == nsGkAtoms::spacer ||
      aTag == nsGkAtoms::button ||
      aTag == nsGkAtoms::isindex ||
      aTag == nsGkAtoms::canvas;
  }


  if (aNameSpaceID == kNameSpaceID_XUL)
    return
#ifdef MOZ_XUL
      aTag == nsGkAtoms::button ||
      aTag == nsGkAtoms::checkbox ||
      aTag == nsGkAtoms::radio ||
      aTag == nsGkAtoms::autorepeatbutton ||
      aTag == nsGkAtoms::titlebar ||
      aTag == nsGkAtoms::resizer ||
      aTag == nsGkAtoms::image ||
      aTag == nsGkAtoms::spring ||
      aTag == nsGkAtoms::spacer ||
      aTag == nsGkAtoms::treechildren ||
      aTag == nsGkAtoms::treecol ||
      aTag == nsGkAtoms::text ||
      aTag == nsGkAtoms::description ||
      aTag == nsGkAtoms::label ||
      aTag == nsGkAtoms::menu ||
      aTag == nsGkAtoms::menuitem ||
      aTag == nsGkAtoms::menubutton ||
      aTag == nsGkAtoms::menubar ||
      aTag == nsGkAtoms::popupgroup ||
      aTag == nsGkAtoms::iframe ||
      aTag == nsGkAtoms::editor ||
      aTag == nsGkAtoms::browser ||
      aTag == nsGkAtoms::progressmeter ||
#endif
      aTag == nsGkAtoms::slider ||
      aTag == nsGkAtoms::scrollbar ||
      aTag == nsGkAtoms::scrollbarbutton ||
#ifdef MOZ_XUL
      aTag == nsGkAtoms::splitter ||
#endif
      PR_FALSE;

#ifdef MOZ_SVG
  if (aNameSpaceID == kNameSpaceID_SVG && NS_SVGEnabled()) {
    
    return PR_TRUE;
  }
#endif

#ifdef MOZ_MATHML
  if (aNameSpaceID == kNameSpaceID_MathML)
    return
      aTag == nsGkAtoms::mi_ ||
      aTag == nsGkAtoms::mn_ ||
      aTag == nsGkAtoms::ms_ ||
      aTag == nsGkAtoms::mtext_ ||
      aTag == nsGkAtoms::mo_ ||
      aTag == nsGkAtoms::mfrac_ ||
      aTag == nsGkAtoms::msup_ ||
      aTag == nsGkAtoms::msub_ ||
      aTag == nsGkAtoms::msubsup_ ||
      aTag == nsGkAtoms::munder_ ||
      aTag == nsGkAtoms::mover_ ||
      aTag == nsGkAtoms::munderover_ ||
      aTag == nsGkAtoms::mphantom_ ||
      aTag == nsGkAtoms::mpadded_ ||
      aTag == nsGkAtoms::mspace_ ||
      aTag == nsGkAtoms::mfenced_ ||
      aTag == nsGkAtoms::mmultiscripts_ ||
      aTag == nsGkAtoms::mstyle_ ||
      aTag == nsGkAtoms::msqrt_ ||
      aTag == nsGkAtoms::mroot_ ||
      aTag == nsGkAtoms::maction_ ||
      aTag == nsGkAtoms::mrow_   ||
      aTag == nsGkAtoms::merror_ ||
      aTag == nsGkAtoms::none   ||
      aTag == nsGkAtoms::mprescripts_ ||
      (aTag == nsGkAtoms::mtable_ &&
       aStyleContext->GetStyleDisplay()->mDisplay == NS_STYLE_DISPLAY_TABLE) ||
      aTag == nsGkAtoms::math;
#endif
  return PR_FALSE;
}
                                      
nsresult
nsCSSFrameConstructor::AdjustParentFrame(nsFrameConstructorState&     aState,
                                         nsIContent*                  aChildContent,
                                         nsIFrame* &                  aParentFrame,
                                         nsIAtom*                     aTag,
                                         PRInt32                      aNameSpaceID,
                                         nsStyleContext*              aChildStyle,
                                         nsFrameItems* &              aFrameItems,
                                         nsFrameConstructorSaveState& aSaveState,
                                         PRBool&                      aSuppressFrame,
                                         PRBool&                      aCreatedPseudo)
{
  NS_PRECONDITION(aChildStyle, "Must have child's style context");
  NS_PRECONDITION(aFrameItems, "Must have frame items to work with");

  aSuppressFrame = PR_FALSE;
  aCreatedPseudo = PR_FALSE;
  if (!aParentFrame) {
    
    return NS_OK;
  }

  PRBool childIsSpecialContent = PR_FALSE; 
  
  
  
  nsIAtom* parentType = aParentFrame->GetType();
  NS_ASSERTION(parentType != nsGkAtoms::tableOuterFrame,
               "Shouldn't be happening");
  if (parentType == nsGkAtoms::tableColGroupFrame) {
    childIsSpecialContent = IsSpecialContent(aChildContent, aTag, aNameSpaceID,
                                             aChildStyle);
    if (childIsSpecialContent ||
        (aChildStyle->GetStyleDisplay()->mDisplay !=
         NS_STYLE_DISPLAY_TABLE_COLUMN)) {
      aSuppressFrame = PR_TRUE;
      return NS_OK;
    }
  }
 
  
  
  
  if (IsTableRelated(aParentFrame->GetType(), PR_FALSE) &&
      (!IsTableRelated(aChildStyle->GetStyleDisplay()->mDisplay, PR_TRUE) ||
       
       
       childIsSpecialContent || 
       IsSpecialContent(aChildContent, aTag, aNameSpaceID, aChildStyle))) {
    nsresult rv = GetPseudoCellFrame(aNameSpaceID, aState, *aParentFrame);
    if (NS_FAILED(rv)) {
      return rv;
    }

    NS_ASSERTION(aState.mPseudoFrames.mCellInner.mFrame,
                 "Must have inner cell frame now!");

    aParentFrame = aState.mPseudoFrames.mCellInner.mFrame;
    aFrameItems = &aState.mPseudoFrames.mCellInner.mChildList;
    
    
    aState.PushFloatContainingBlock(aParentFrame, aSaveState, PR_FALSE,
                                    PR_FALSE);
    aCreatedPseudo = PR_TRUE;
  }
  return NS_OK;
}





nsresult
nsCSSFrameConstructor::ConstructTableFrame(nsFrameConstructorState& aState,
                                           nsIContent*              aContent,
                                           nsIFrame*                aContentParent,
                                           nsStyleContext*          aStyleContext,
                                           PRInt32                  aNameSpaceID,
                                           PRBool                   aIsPseudo,
                                           nsFrameItems&            aChildItems,
                                           PRBool                   aAllowOutOfFlow,
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

  nsIFrame* parentFrame = aContentParent;
  nsFrameItems* frameItems = &aChildItems;
  
  nsFrameConstructorSaveState floatSaveState;
  if (!aIsPseudo) {
    
    PRBool hasPseudoParent = PR_FALSE;
    GetParentFrame(aNameSpaceID,*parentFrame, nsGkAtoms::tableOuterFrame,
                   aState, parentFrame, hasPseudoParent);
    if (!hasPseudoParent && !aState.mPseudoFrames.IsEmpty()) {
      ProcessPseudoFrames(aState, aChildItems);
    }
    if (hasPseudoParent) {
      aState.PushFloatContainingBlock(parentFrame, floatSaveState,
                                      PR_FALSE, PR_FALSE);
      frameItems = &aState.mPseudoFrames.mCellInner.mChildList;
      if (aState.mPseudoFrames.mTableOuter.mFrame) {
        ProcessPseudoFrames(aState, nsGkAtoms::tableOuterFrame);
      }
    }
  }

  const nsStyleDisplay* disp = outerStyleContext->GetStyleDisplay();
  
  
  
  nsIFrame* geometricParent =
    aAllowOutOfFlow ? aState.GetGeometricParent(disp, parentFrame) :
                      parentFrame;

  
  
  InitAndRestoreFrame(aState, aContent, geometricParent, nsnull, aNewOuterFrame);  
  nsHTMLContainerFrame::CreateViewForFrame(aNewOuterFrame, aContentParent,
                                           PR_FALSE);

  
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

    rv = aState.AddChild(aNewOuterFrame, *frameItems, disp, aContent,
                         outerStyleContext, parentFrame, aAllowOutOfFlow,
                         aAllowOutOfFlow);
    if (NS_FAILED(rv)) {
      return rv;
    }

    nsFrameItems childItems;
    rv = ProcessChildren(aState, aContent, aNewInnerFrame, PR_FALSE, childItems,
                         PR_FALSE);
    
    if (NS_FAILED(rv)) return rv;

    
    CreateAnonymousFrames(nsnull, aState, aContent, aNewInnerFrame,
                          PR_FALSE, childItems);

    nsFrameItems captionItems;
    nsIFrame *child = childItems.childList;
    while (child) {
      nsIFrame *nextSibling = child->GetNextSibling();
      if (nsGkAtoms::tableCaptionFrame == child->GetType()) {
        childItems.RemoveChild(child);
        captionItems.AddChild(child);
      }
      child = nextSibling;
    }
    
    aNewInnerFrame->SetInitialChildList(nsnull, childItems.childList);

    
    if (captionItems.childList) {
        aNewOuterFrame->SetInitialChildList(nsGkAtoms::captionList,
                                            captionItems.childList);
    }
 }

  return rv;
}

nsresult
nsCSSFrameConstructor::ConstructTableCaptionFrame(nsFrameConstructorState& aState,
                                                  nsIContent*              aContent,
                                                  nsIFrame*                aParentFrameIn,
                                                  nsStyleContext*          aStyleContext,
                                                  PRInt32                  aNameSpaceID,
                                                  nsFrameItems&            aChildItems,
                                                  nsIFrame*&               aNewFrame,
                                                  PRBool&                  aIsPseudoParent)

{
  nsresult rv = NS_OK;
  if (!aParentFrameIn) return rv;

  nsIFrame* parentFrame = aParentFrameIn;
  aIsPseudoParent = PR_FALSE;
  
  GetParentFrame(aNameSpaceID, *aParentFrameIn, 
                 nsGkAtoms::tableCaptionFrame, aState, parentFrame,
                 aIsPseudoParent);
  if (!aIsPseudoParent && !aState.mPseudoFrames.IsEmpty()) {
    ProcessPseudoFrames(aState, aChildItems);
  }

  aNewFrame = NS_NewTableCaptionFrame(mPresShell, aStyleContext);
  if (NS_UNLIKELY(!aNewFrame)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  InitAndRestoreFrame(aState, aContent, parentFrame, nsnull, aNewFrame);
  
  nsHTMLContainerFrame::CreateViewForFrame(aNewFrame, nsnull, PR_FALSE);

  PRBool haveFirstLetterStyle, haveFirstLineStyle;
  ShouldHaveSpecialBlockStyle(aContent, aStyleContext,
                              &haveFirstLetterStyle, &haveFirstLineStyle);

  
  nsFrameConstructorSaveState floatSaveState;
  aState.PushFloatContainingBlock(aNewFrame, floatSaveState,
                                  haveFirstLetterStyle, haveFirstLineStyle);
  nsFrameItems childItems;
  rv = ProcessChildren(aState, aContent, aNewFrame,
                       PR_TRUE, childItems, PR_TRUE);
  if (NS_FAILED(rv)) return rv;
  aNewFrame->SetInitialChildList(nsnull, childItems.childList);
  if (aIsPseudoParent) {
    aState.mPseudoFrames.mTableOuter.mChildList2.AddChild(aNewFrame);
  }
  
  return rv;
}


nsresult
nsCSSFrameConstructor::ConstructTableRowGroupFrame(nsFrameConstructorState& aState,
                                                   nsIContent*              aContent,
                                                   nsIFrame*                aParentFrameIn,
                                                   nsStyleContext*          aStyleContext,
                                                   PRInt32                  aNameSpaceID,
                                                   PRBool                   aIsPseudo,
                                                   nsFrameItems&            aChildItems,
                                                   nsIFrame*&               aNewFrame, 
                                                   PRBool&                  aIsPseudoParent)
{
  nsresult rv = NS_OK;
  if (!aParentFrameIn) return rv;

  nsIFrame* parentFrame = aParentFrameIn;
  aIsPseudoParent = PR_FALSE;
  if (!aIsPseudo) {
    
    GetParentFrame(aNameSpaceID, *aParentFrameIn, 
                   nsGkAtoms::tableRowGroupFrame, aState, parentFrame,
                   aIsPseudoParent);
    if (!aIsPseudoParent && !aState.mPseudoFrames.IsEmpty()) {
      ProcessPseudoFrames(aState, aChildItems);
    }
    if (!aIsPseudo && aIsPseudoParent && aState.mPseudoFrames.mRowGroup.mFrame) {
      ProcessPseudoFrames(aState, nsGkAtoms::tableRowGroupFrame);
    }
  }

  const nsStyleDisplay* styleDisplay = aStyleContext->GetStyleDisplay();

  aNewFrame = NS_NewTableRowGroupFrame(mPresShell, aStyleContext);

  nsIFrame* scrollFrame = nsnull;
  if (styleDisplay->IsScrollableOverflow()) {
    
    BuildScrollFrame(aState, aContent, aStyleContext, aNewFrame, parentFrame,
                     nsnull, scrollFrame, aStyleContext);

  } 
  else {
    if (NS_UNLIKELY(!aNewFrame)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    InitAndRestoreFrame(aState, aContent, parentFrame, nsnull, aNewFrame);
    
    nsHTMLContainerFrame::CreateViewForFrame(aNewFrame, nsnull, PR_FALSE);
  }

  if (!aIsPseudo) {
    nsFrameItems childItems;
    rv = ProcessChildren(aState, aContent, aNewFrame, PR_FALSE, childItems,
                         PR_FALSE);
    
    if (NS_FAILED(rv)) return rv;

    
    CreateAnonymousFrames(nsnull, aState, aContent, aNewFrame,
                          PR_FALSE, childItems);

    aNewFrame->SetInitialChildList(nsnull, childItems.childList);
    if (aIsPseudoParent) {
      nsIFrame* child = (scrollFrame) ? scrollFrame : aNewFrame;
      aState.mPseudoFrames.mTableInner.mChildList.AddChild(child);
    }
  } 

  
  if (scrollFrame) {
    aNewFrame = scrollFrame;
  }

  return rv;
}

nsresult
nsCSSFrameConstructor::ConstructTableColGroupFrame(nsFrameConstructorState& aState,
                                                   nsIContent*              aContent,
                                                   nsIFrame*                aParentFrameIn,
                                                   nsStyleContext*          aStyleContext,
                                                   PRInt32                  aNameSpaceID,
                                                   PRBool                   aIsPseudo,
                                                   nsFrameItems&            aChildItems,
                                                   nsIFrame*&               aNewFrame, 
                                                   PRBool&                  aIsPseudoParent)
{
  nsresult rv = NS_OK;
  if (!aParentFrameIn) return rv;

  nsIFrame* parentFrame = aParentFrameIn;
  aIsPseudoParent = PR_FALSE;
  if (!aIsPseudo) {
    
    GetParentFrame(aNameSpaceID, *aParentFrameIn,
                   nsGkAtoms::tableColGroupFrame, aState, parentFrame,
                   aIsPseudoParent);
    if (!aIsPseudoParent && !aState.mPseudoFrames.IsEmpty()) {
      ProcessPseudoFrames(aState, aChildItems);
    }
    if (!aIsPseudo && aIsPseudoParent && aState.mPseudoFrames.mColGroup.mFrame) {
      ProcessPseudoFrames(aState, nsGkAtoms::tableColGroupFrame);
    }
  }

  aNewFrame = NS_NewTableColGroupFrame(mPresShell, aStyleContext);
  if (NS_UNLIKELY(!aNewFrame)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  InitAndRestoreFrame(aState, aContent, parentFrame, nsnull, aNewFrame);

  if (!aIsPseudo) {
    nsFrameItems childItems;
    rv = ProcessChildren(aState, aContent, aNewFrame, PR_FALSE, childItems,
                         PR_FALSE);
    if (NS_FAILED(rv)) return rv;
    aNewFrame->SetInitialChildList(nsnull, childItems.childList);
    if (aIsPseudoParent) {
      aState.mPseudoFrames.mTableInner.mChildList.AddChild(aNewFrame);
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
                                              nsIFrame*&               aNewFrame,
                                              PRBool&                  aIsPseudoParent)
{
  nsresult rv = NS_OK;
  if (!aParentFrameIn) return rv;

  nsIFrame* parentFrame = aParentFrameIn;
  aIsPseudoParent = PR_FALSE;
  if (!aIsPseudo) {
    
    GetParentFrame(aNameSpaceID, *aParentFrameIn,
                   nsGkAtoms::tableRowFrame, aState, parentFrame,
                   aIsPseudoParent);
    if (!aIsPseudoParent && !aState.mPseudoFrames.IsEmpty()) {
      ProcessPseudoFrames(aState, aChildItems);
    }
    if (!aIsPseudo && aIsPseudoParent && aState.mPseudoFrames.mRow.mFrame) {
      ProcessPseudoFrames(aState, nsGkAtoms::tableRowFrame);
    }
  }

#ifdef MOZ_MATHML
  if (kNameSpaceID_MathML == aNameSpaceID)
    aNewFrame = NS_NewMathMLmtrFrame(mPresShell, aStyleContext);
  else
#endif
    aNewFrame = NS_NewTableRowFrame(mPresShell, aStyleContext);

  if (NS_UNLIKELY(!aNewFrame)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  InitAndRestoreFrame(aState, aContent, parentFrame, nsnull, aNewFrame);
  
  nsHTMLContainerFrame::CreateViewForFrame(aNewFrame, nsnull, PR_FALSE);
  if (!aIsPseudo) {
    nsFrameItems childItems;
    rv = ProcessChildren(aState, aContent, aNewFrame, PR_FALSE, childItems,
                         PR_FALSE);
    if (NS_FAILED(rv)) return rv;
    
    CreateAnonymousFrames(nsnull, aState, aContent, aNewFrame,
                          PR_FALSE, childItems);

    aNewFrame->SetInitialChildList(nsnull, childItems.childList);
    if (aIsPseudoParent) {
      aState.mPseudoFrames.mRowGroup.mChildList.AddChild(aNewFrame);
    }
  }

  return rv;
}
      
nsresult
nsCSSFrameConstructor::ConstructTableColFrame(nsFrameConstructorState& aState,
                                              nsIContent*              aContent,
                                              nsIFrame*                aParentFrameIn,
                                              nsStyleContext*          aStyleContext,
                                              PRInt32                  aNameSpaceID,
                                              PRBool                   aIsPseudo,
                                              nsFrameItems&            aChildItems,
                                              nsIFrame*&               aNewFrame,
                                              PRBool&                  aIsPseudoParent)
{
  nsresult rv = NS_OK;
  if (!aParentFrameIn || !aStyleContext) return rv;

  nsIFrame* parentFrame = aParentFrameIn;
  aIsPseudoParent = PR_FALSE;
  if (!aIsPseudo) {
    
    GetParentFrame(aNameSpaceID, *aParentFrameIn,
                   nsGkAtoms::tableColFrame, aState, parentFrame,
                   aIsPseudoParent);
    if (!aIsPseudoParent && !aState.mPseudoFrames.IsEmpty()) {
      ProcessPseudoFrames(aState, aChildItems);
    }
  }

  aNewFrame = NS_NewTableColFrame(mPresShell, aStyleContext);
  if (NS_UNLIKELY(!aNewFrame)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  InitAndRestoreFrame(aState, aContent, parentFrame, nsnull, aNewFrame);

  
  PRInt32 span = 1;
  nsCOMPtr<nsIDOMHTMLTableColElement> cgContent(do_QueryInterface(aContent));
  if (cgContent) { 
    cgContent->GetSpan(&span);
    nsIFrame* lastCol = aNewFrame;
    nsStyleContext* styleContext = nsnull;
    for (PRInt32 spanX = 1; spanX < span; spanX++) {
      
      if (1 == spanX)
        styleContext = aNewFrame->GetStyleContext();
      nsIFrame* newCol = NS_NewTableColFrame(mPresShell, styleContext);
      if (NS_UNLIKELY(!newCol)) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
      InitAndRestoreFrame(aState, aContent, parentFrame, nsnull, newCol, PR_FALSE);
      ((nsTableColFrame*)newCol)->SetColType(eColAnonymousCol);
      lastCol->SetNextSibling(newCol);
      lastCol = newCol;
    }
  }

  if (!aIsPseudo && aIsPseudoParent) {
      aState.mPseudoFrames.mColGroup.mChildList.AddChild(aNewFrame);
  }
  
  return rv;
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
                                               nsIFrame*&               aNewCellInnerFrame,
                                               PRBool&                  aIsPseudoParent)
{
  nsresult rv = NS_OK;
  if (!aParentFrameIn) return rv;

  nsIFrame* parentFrame = aParentFrameIn;
  aIsPseudoParent = PR_FALSE;
  if (!aIsPseudo) {
    
    
    GetParentFrame(aNameSpaceID, *aParentFrameIn,
                   nsGkAtoms::tableCellFrame, aState, parentFrame,
                   aIsPseudoParent);
    if (!aIsPseudoParent && !aState.mPseudoFrames.IsEmpty()) {
      ProcessPseudoFrames(aState, aChildItems);
    }
    if (!aIsPseudo && aIsPseudoParent && aState.mPseudoFrames.mCellOuter.mFrame) {
      ProcessPseudoFrames(aState, nsGkAtoms::tableCellFrame);
    }
  }
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
  
  nsHTMLContainerFrame::CreateViewForFrame(aNewCellOuterFrame, nsnull, PR_FALSE);
  
  
  nsRefPtr<nsStyleContext> innerPseudoStyle;
  innerPseudoStyle = mPresShell->StyleSet()->
    ResolvePseudoStyleFor(aContent,
                          nsCSSAnonBoxes::cellContent, aStyleContext);

  
#ifdef MOZ_MATHML
  if (kNameSpaceID_MathML == aNameSpaceID)
    aNewCellInnerFrame = NS_NewMathMLmtdInnerFrame(mPresShell, innerPseudoStyle);
  else
#endif
    aNewCellInnerFrame = NS_NewTableCellInnerFrame(mPresShell, innerPseudoStyle);


  if (NS_UNLIKELY(!aNewCellInnerFrame)) {
    aNewCellOuterFrame->Destroy();
    aNewCellOuterFrame = nsnull;
    return NS_ERROR_OUT_OF_MEMORY;
  }

  InitAndRestoreFrame(aState, aContent, aNewCellOuterFrame, nsnull, aNewCellInnerFrame);

  if (!aIsPseudo) {
    PRBool haveFirstLetterStyle, haveFirstLineStyle;
    ShouldHaveSpecialBlockStyle(aContent, aStyleContext,
                                &haveFirstLetterStyle, &haveFirstLineStyle);

    
    nsFrameConstructorSaveState floatSaveState;
    aState.PushFloatContainingBlock(aNewCellInnerFrame, floatSaveState,
                                    haveFirstLetterStyle, haveFirstLineStyle);

    
    nsFrameItems childItems;
    rv = ProcessChildren(aState, aContent, aNewCellInnerFrame, 
                         PR_TRUE, childItems, PR_TRUE);

    if (NS_FAILED(rv)) {
      
      
      aNewCellInnerFrame->Destroy();
      aNewCellInnerFrame = nsnull;
      aNewCellOuterFrame->Destroy();
      aNewCellOuterFrame = nsnull;
      return rv;
    }

    aNewCellInnerFrame->SetInitialChildList(nsnull, childItems.childList);
    aNewCellOuterFrame->SetInitialChildList(nsnull, aNewCellInnerFrame);
    if (aIsPseudoParent) {
      aState.mPseudoFrames.mRow.mChildList.AddChild(aNewCellOuterFrame);
    }
  }

  return rv;
}

static PRBool 
NeedFrameFor(nsIFrame*   aParentFrame,
             nsIContent* aChildContent) 
{
  
  if ((NS_FRAME_EXCLUDE_IGNORABLE_WHITESPACE & aParentFrame->GetStateBits())
      && TextIsOnlyWhitespace(aChildContent)) {
    return PR_FALSE;
  }
  return PR_TRUE;
}

const nsStyleDisplay* 
nsCSSFrameConstructor::GetDisplay(nsIFrame* aFrame)
{
  if (nsnull == aFrame) {
    return nsnull;
  }
  return aFrame->GetStyleContext()->GetStyleDisplay();
}





nsresult
nsCSSFrameConstructor::ConstructDocElementTableFrame(nsIContent*     aDocElement,
                                                     nsIFrame*       aParentFrame,
                                                     nsIFrame**      aNewTableFrame,
                                                     nsFrameConstructorState& aState)
{
  nsFrameItems    frameItems;

  
  
  
  
  
  
  nsFrameConstructorState state(mPresShell, nsnull, nsnull, nsnull,
                                aState.mFrameState);
  ConstructFrame(state, aDocElement, aParentFrame, frameItems);
  *aNewTableFrame = frameItems.childList;
  if (!*aNewTableFrame) {
    NS_WARNING("cannot get table contentFrame");
    
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
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
    if (gfxScrollbarFrame1) {
      
      aState.mFrameManager->
        SetPrimaryFrameFor(gfxScrollbarFrame1->GetContent(), gfxScrollbarFrame1);

      nsIFrame* gfxScrollbarFrame2 = gfxScrollbarFrame1->GetNextSibling();
      if (gfxScrollbarFrame2) {
        
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

  nsIFrame* contentFrame = nsnull;
  PRBool isBlockFrame = PR_FALSE;
  nsresult rv;

  
  
  

  PRBool docElemIsTable = (display->mDisplay == NS_STYLE_DISPLAY_TABLE) &&
                          !IsSpecialContent(aDocElement, aDocElement->Tag(),
                                            aDocElement->GetNameSpaceID(),
                                            styleContext);

  if (docElemIsTable) {
    
    rv = ConstructDocElementTableFrame(aDocElement, aParentFrame, &contentFrame,
                                       aState);
    if (NS_FAILED(rv)) {
      return rv;
    }
    styleContext = contentFrame->GetStyleContext();
  } else {
    
#ifdef MOZ_XUL
    if (aDocElement->IsNodeOfType(nsINode::eXUL)) {
      contentFrame = NS_NewDocElementBoxFrame(mPresShell, styleContext);
    }
    else
#endif 
#ifdef MOZ_SVG
    if (aDocElement->GetNameSpaceID() == kNameSpaceID_SVG) {
      if (aDocElement->Tag() == nsGkAtoms::svg && NS_SVGEnabled()) {
        contentFrame = NS_NewSVGOuterSVGFrame(mPresShell, aDocElement, styleContext);
      } else {
        return NS_ERROR_FAILURE;
      }
    }
    else 
#endif
    {
      contentFrame = NS_NewDocumentElementFrame(mPresShell, styleContext);
      isBlockFrame = PR_TRUE;
    }
    
    if (NS_UNLIKELY(!contentFrame)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    
    InitAndRestoreFrame(aState, aDocElement, aParentFrame, nsnull, contentFrame);
  }

  
  aState.mFrameManager->SetPrimaryFrameFor(aDocElement, contentFrame);

  *aNewFrame = contentFrame;

  mInitialContainingBlock = contentFrame;
  mInitialContainingBlockIsAbsPosContainer = PR_FALSE;

  
  if (!docElemIsTable) {
    
    nsFrameConstructorSaveState absoluteSaveState;
    nsFrameConstructorSaveState floatSaveState;
    nsFrameItems                childItems;

    if (isBlockFrame) {
      PRBool haveFirstLetterStyle, haveFirstLineStyle;
      ShouldHaveSpecialBlockStyle(aDocElement, styleContext,
                                  &haveFirstLetterStyle, &haveFirstLineStyle);
      mInitialContainingBlockIsAbsPosContainer = PR_TRUE;
      aState.PushAbsoluteContainingBlock(contentFrame, absoluteSaveState);
      aState.PushFloatContainingBlock(contentFrame, floatSaveState,
                                      haveFirstLetterStyle,
                                      haveFirstLineStyle);
    }

    
    
    
    CreateAnonymousFrames(nsnull, aState, aDocElement, contentFrame,
                          PR_FALSE, childItems, PR_TRUE);
    ProcessChildren(aState, aDocElement, contentFrame, PR_TRUE, childItems,
                    isBlockFrame);

    
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
                                                  nsnull,
                                                  rootPseudo,
                                                  PR_TRUE,
                                                  newFrame);

      nsIScrollableFrame* scrollable;
      CallQueryInterface(newFrame, &scrollable);
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
    
    
    nsIFrame *pageFrame, *pageContentFrame;
    ConstructPageFrame(mPresShell, presContext, rootFrame, nsnull,
                       pageFrame, pageContentFrame);
    rootFrame->SetInitialChildList(nsnull, pageFrame);

    
    
    mDocElementContainingBlock = pageContentFrame;
  }

  viewportFrame->SetInitialChildList(nsnull, newFrame);
  
  *aNewFrame = viewportFrame;

  return NS_OK;
}

nsresult
nsCSSFrameConstructor::ConstructPageFrame(nsIPresShell*   aPresShell,
                                          nsPresContext* aPresContext,
                                          nsIFrame*       aParentFrame,
                                          nsIFrame*       aPrevPageFrame,
                                          nsIFrame*&      aPageFrame,
                                          nsIFrame*&      aPageContentFrame)
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

  aPageContentFrame = NS_NewPageContentFrame(aPresShell, pageContentPseudoStyle);
  if (NS_UNLIKELY(!aPageContentFrame))
    return NS_ERROR_OUT_OF_MEMORY;

  
  
  nsIFrame* prevPageContentFrame = nsnull;
  if (aPrevPageFrame) {
    prevPageContentFrame = aPrevPageFrame->GetFirstChild(nsnull);
    NS_ASSERTION(prevPageContentFrame, "missing page content frame");
  }
  aPageContentFrame->Init(nsnull, aPageFrame, prevPageContentFrame);
  mFixedContainingBlock = aPageContentFrame;

  aPageFrame->SetInitialChildList(nsnull, aPageContentFrame);

  return NS_OK;
}


nsresult
nsCSSFrameConstructor::CreatePlaceholderFrameFor(nsIPresShell*    aPresShell, 
                                                 nsPresContext*  aPresContext,
                                                 nsFrameManager*  aFrameManager,
                                                 nsIContent*      aContent,
                                                 nsIFrame*        aFrame,
                                                 nsStyleContext*  aStyleContext,
                                                 nsIFrame*        aParentFrame,
                                                 nsIFrame**       aPlaceholderFrame)
{
  nsRefPtr<nsStyleContext> placeholderStyle = aPresShell->StyleSet()->
    ResolveStyleForNonElement(aStyleContext->GetParent());
  
  
  nsPlaceholderFrame* placeholderFrame =
    (nsPlaceholderFrame*)NS_NewPlaceholderFrame(aPresShell, placeholderStyle);

  if (placeholderFrame) {
    placeholderFrame->Init(aContent, aParentFrame, nsnull);
  
    
    placeholderFrame->SetOutOfFlowFrame(aFrame);
  
    aFrame->AddStateBits(NS_FRAME_OUT_OF_FLOW);

    
    aFrameManager->RegisterPlaceholderFrame(placeholderFrame);

    *aPlaceholderFrame = static_cast<nsIFrame*>(placeholderFrame);
    
    return NS_OK;
  }
  else {
    return NS_ERROR_OUT_OF_MEMORY;
  }
}

nsresult
nsCSSFrameConstructor::ConstructRadioControlFrame(nsIFrame**      aNewFrame,
                                                  nsIContent*     aContent,
                                                  nsStyleContext* aStyleContext)
{
  *aNewFrame = NS_NewGfxRadioControlFrame(mPresShell, aStyleContext);
  if (NS_UNLIKELY(!*aNewFrame)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsRefPtr<nsStyleContext> radioStyle;
  radioStyle = mPresShell->StyleSet()->ResolvePseudoStyleFor(aContent,
                                                             nsCSSAnonBoxes::radio,
                                                             aStyleContext);
  nsIRadioControlFrame* radio = nsnull;
  if (*aNewFrame && NS_SUCCEEDED(CallQueryInterface(*aNewFrame, &radio))) {
    radio->SetRadioButtonFaceStyleContext(radioStyle);
  }
  return NS_OK;
}

nsresult
nsCSSFrameConstructor::ConstructCheckboxControlFrame(nsIFrame**      aNewFrame,
                                                     nsIContent*     aContent,
                                                     nsStyleContext* aStyleContext)
{
  *aNewFrame = NS_NewGfxCheckboxControlFrame(mPresShell, aStyleContext);
  if (NS_UNLIKELY(!*aNewFrame)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsRefPtr<nsStyleContext> checkboxStyle;
  checkboxStyle = mPresShell->StyleSet()->ResolvePseudoStyleFor(aContent,
                                                                nsCSSAnonBoxes::check, 
                                                                aStyleContext);
  nsICheckboxControlFrame* checkbox = nsnull;
  if (*aNewFrame && NS_SUCCEEDED(CallQueryInterface(*aNewFrame, &checkbox))) {
    checkbox->SetCheckboxFaceStyleContext(checkboxStyle);
  }
  return NS_OK;
}

nsresult
nsCSSFrameConstructor::ConstructButtonFrame(nsFrameConstructorState& aState,
                                            nsIContent*              aContent,
                                            nsIFrame*                aParentFrame,
                                            nsIAtom*                 aTag,
                                            nsStyleContext*          aStyleContext,
                                            nsIFrame**               aNewFrame,
                                            const nsStyleDisplay*    aStyleDisplay,
                                            nsFrameItems&            aFrameItems)
{
  *aNewFrame = nsnull;
  nsIFrame* buttonFrame = nsnull;
  
  if (nsGkAtoms::button == aTag) {
    buttonFrame = NS_NewHTMLButtonControlFrame(mPresShell, aStyleContext);
  }
  else {
    buttonFrame = NS_NewGfxButtonControlFrame(mPresShell, aStyleContext);
  }
  if (NS_UNLIKELY(!buttonFrame)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  nsresult rv = InitAndRestoreFrame(aState, aContent,
                                    aState.GetGeometricParent(aStyleDisplay, aParentFrame),
                                    nsnull, buttonFrame);
  if (NS_FAILED(rv)) {
    buttonFrame->Destroy();
    return rv;
  }
  
  nsHTMLContainerFrame::CreateViewForFrame(buttonFrame, aParentFrame, PR_FALSE);

  
  
  nsRefPtr<nsStyleContext> styleContext;
  styleContext = mPresShell->StyleSet()->ResolvePseudoStyleFor(aContent,
                                                               nsCSSAnonBoxes::buttonContent,
                                                               aStyleContext);
                                                               
  nsIFrame* areaFrame = NS_NewAreaFrame(mPresShell, styleContext,
                                        NS_BLOCK_SPACE_MGR);

  if (NS_UNLIKELY(!areaFrame)) {
    buttonFrame->Destroy();
    return NS_ERROR_OUT_OF_MEMORY;
  }
  rv = InitAndRestoreFrame(aState, aContent, buttonFrame, nsnull, areaFrame);
  if (NS_FAILED(rv)) {
    areaFrame->Destroy();
    buttonFrame->Destroy();
    return rv;
  }

  rv = aState.AddChild(buttonFrame, aFrameItems, aStyleDisplay, aContent,
                                aStyleContext, aParentFrame);
  if (NS_FAILED(rv)) {
    areaFrame->Destroy();
    buttonFrame->Destroy();
    return rv;
  }

  
  if (!buttonFrame->IsLeaf()) { 
    
    
    PRBool haveFirstLetterStyle, haveFirstLineStyle;
    ShouldHaveSpecialBlockStyle(aContent, aStyleContext,
                                &haveFirstLetterStyle, &haveFirstLineStyle);
    nsFrameConstructorSaveState floatSaveState;
    aState.PushFloatContainingBlock(areaFrame, floatSaveState,
                                    haveFirstLetterStyle,
                                    haveFirstLineStyle);

    
    nsFrameConstructorSaveState absoluteSaveState;
    nsFrameItems                childItems;

    if (aStyleDisplay->IsPositioned()) {
      
      
      aState.PushAbsoluteContainingBlock(areaFrame, absoluteSaveState);
    }

    rv = ProcessChildren(aState, aContent, areaFrame, PR_TRUE, childItems,
                         buttonFrame->GetStyleDisplay()->IsBlockOutside());
    if (NS_FAILED(rv)) return rv;
  
    
    areaFrame->SetInitialChildList(nsnull, childItems.childList);
  }

  buttonFrame->SetInitialChildList(nsnull, areaFrame);

  nsFrameItems  anonymousChildItems;
  
  CreateAnonymousFrames(aTag, aState, aContent, buttonFrame,
                          PR_FALSE, anonymousChildItems);
  if (anonymousChildItems.childList) {
    
    aState.mFrameManager->AppendFrames(areaFrame, nsnull, anonymousChildItems.childList);
  }

  
  *aNewFrame = buttonFrame; 

  return NS_OK;  
}

nsresult
nsCSSFrameConstructor::ConstructSelectFrame(nsFrameConstructorState& aState,
                                            nsIContent*              aContent,
                                            nsIFrame*                aParentFrame,
                                            nsIAtom*                 aTag,
                                            nsStyleContext*          aStyleContext,
                                            nsIFrame*&               aNewFrame,
                                            const nsStyleDisplay*    aStyleDisplay,
                                            PRBool&                  aFrameHasBeenInitialized,
                                            nsFrameItems&            aFrameItems)
{
  nsresult rv = NS_OK;
  const PRInt32 kNoSizeSpecified = -1;

  
  nsCOMPtr<nsIDOMHTMLSelectElement> sel(do_QueryInterface(aContent));
  PRInt32 size = 1;
  if (sel) {
    sel->GetSize(&size); 
    PRBool multipleSelect = PR_FALSE;
    sel->GetMultiple(&multipleSelect);
     
    if (((1 == size || 0 == size) || (kNoSizeSpecified  == size)) && (PR_FALSE == multipleSelect)) {
        
        
        
        
        
      PRUint32 flags = NS_BLOCK_SPACE_MGR;
      nsIFrame* comboboxFrame = NS_NewComboboxControlFrame(mPresShell, aStyleContext, flags);

      
      
      nsILayoutHistoryState *historyState = aState.mFrameState;
      aState.mFrameState = nsnull;
      
      InitAndRestoreFrame(aState, aContent,
                          aState.GetGeometricParent(aStyleDisplay, aParentFrame),
                          nsnull, comboboxFrame);

      nsHTMLContainerFrame::CreateViewForFrame(comboboxFrame, aParentFrame, PR_FALSE);

      rv = aState.AddChild(comboboxFrame, aFrameItems, aStyleDisplay,
                           aContent, aStyleContext, aParentFrame);
      if (NS_FAILED(rv)) {
        return rv;
      }
      
      
      
      
      nsIComboboxControlFrame* comboBox = nsnull;
      CallQueryInterface(comboboxFrame, &comboBox);
      NS_ASSERTION(comboBox, "NS_NewComboboxControlFrame returned frame that "
                             "doesn't implement nsIComboboxControlFrame");

        
      nsRefPtr<nsStyleContext> listStyle;
      listStyle = mPresShell->StyleSet()->ResolvePseudoStyleFor(aContent,
                                                                nsCSSAnonBoxes::dropDownList, 
                                                                aStyleContext);

        
      nsIFrame* listFrame = NS_NewListControlFrame(mPresShell, listStyle);

        
      nsIListControlFrame * listControlFrame;
      rv = CallQueryInterface(listFrame, &listControlFrame);
      if (NS_SUCCEEDED(rv)) {
        listControlFrame->SetComboboxFrame(comboboxFrame);
      }
         
      comboBox->SetDropDown(listFrame);

      NS_ASSERTION(!listStyle->GetStyleDisplay()->IsPositioned(),
                   "Ended up with positioned dropdown list somehow.");
      NS_ASSERTION(!listStyle->GetStyleDisplay()->IsFloating(),
                   "Ended up with floating dropdown list somehow.");
      
      
      
      nsIFrame* scrolledFrame = NS_NewSelectsAreaFrame(mPresShell, aStyleContext, flags);

      InitializeSelectFrame(aState, listFrame, scrolledFrame, aContent,
                            comboboxFrame, listStyle, PR_TRUE, aFrameItems);

        
        
        
      NS_ASSERTION(listFrame->GetView(), "ListFrame's view is nsnull");
      

      
      
      

      nsFrameItems childItems;
      CreateAnonymousFrames(nsGkAtoms::combobox, aState, aContent,
                            comboboxFrame, PR_TRUE, childItems);
  
      comboboxFrame->SetInitialChildList(nsnull, childItems.childList);

      
      
      nsFrameItems popupItems;
      popupItems.AddChild(listFrame);
      comboboxFrame->SetInitialChildList(nsGkAtoms::popupList,
                                         popupItems.childList);

      aNewFrame = comboboxFrame;
      aFrameHasBeenInitialized = PR_TRUE;
      aState.mFrameState = historyState;
      if (aState.mFrameState && aState.mFrameManager) {
        
        aState.mFrameManager->RestoreFrameState(comboboxFrame,
                                                aState.mFrameState);
      }
    } else {
      
      
      
      nsIFrame* listFrame = NS_NewListControlFrame(mPresShell, aStyleContext);
      if (listFrame) {
        rv = NS_OK;
      }
      else {
        rv = NS_ERROR_OUT_OF_MEMORY;
      }

      nsIFrame* scrolledFrame = NS_NewSelectsAreaFrame(
        mPresShell, aStyleContext, NS_BLOCK_SPACE_MGR);

      
      

      InitializeSelectFrame(aState, listFrame, scrolledFrame, aContent,
                            aParentFrame, aStyleContext, PR_FALSE, aFrameItems);

      aNewFrame = listFrame;

      aFrameHasBeenInitialized = PR_TRUE;
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
    nsresult rv = aState.AddChild(scrollFrame, aFrameItems, display,
                                  aContent, aStyleContext, aParentFrame);
    if (NS_FAILED(rv)) {
      return rv;
    }
  }
      
  nsHTMLContainerFrame::CreateViewForFrame(scrollFrame, aParentFrame,
                                           aBuildCombobox);
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

  nsStyleContext* scrolledPseudoStyle;
  BuildScrollFrame(aState, aContent, aStyleContext, scrolledFrame,
                   geometricParent, aParentFrame, scrollFrame,
                   scrolledPseudoStyle);

  if (aState.mFrameState && aState.mFrameManager) {
    
    aState.mFrameManager->RestoreFrameStateFor(scrollFrame, aState.mFrameState);
  }

  
  PRBool haveFirstLetterStyle, haveFirstLineStyle;
  ShouldHaveSpecialBlockStyle(aContent, aStyleContext,
                              &haveFirstLetterStyle, &haveFirstLineStyle);
  nsFrameConstructorSaveState floatSaveState;
  aState.PushFloatContainingBlock(scrolledFrame, floatSaveState,
                                  haveFirstLetterStyle, haveFirstLineStyle);

  
  nsFrameConstructorSaveState absoluteSaveState;
  nsFrameItems                childItems;

  if (display->IsPositioned()) {
    
    
    aState.PushAbsoluteContainingBlock(scrolledFrame, absoluteSaveState);
  }

  ProcessChildren(aState, aContent, scrolledFrame, PR_FALSE,
                  childItems, PR_TRUE);

  
  scrolledFrame->SetInitialChildList(nsnull, childItems.childList);
  return NS_OK;
}

nsresult
nsCSSFrameConstructor::ConstructFieldSetFrame(nsFrameConstructorState& aState,
                                              nsIContent*              aContent,
                                              nsIFrame*                aParentFrame,
                                              nsIAtom*                 aTag,
                                              nsStyleContext*          aStyleContext,
                                              nsIFrame*&               aNewFrame,
                                              nsFrameItems&            aFrameItems,
                                              const nsStyleDisplay*    aStyleDisplay,
                                              PRBool&                  aFrameHasBeenInitialized)
{
  nsIFrame* newFrame = NS_NewFieldSetFrame(mPresShell, aStyleContext);
  if (NS_UNLIKELY(!newFrame)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  InitAndRestoreFrame(aState, aContent, 
                      aState.GetGeometricParent(aStyleDisplay, aParentFrame),
                      nsnull, newFrame);

  
  
  nsHTMLContainerFrame::CreateViewForFrame(newFrame, aParentFrame, PR_FALSE);

  
  nsRefPtr<nsStyleContext> styleContext;
  styleContext = mPresShell->StyleSet()->ResolvePseudoStyleFor(aContent,
                                                               nsCSSAnonBoxes::fieldsetContent,
                                                               aStyleContext);
  
  nsIFrame* areaFrame = NS_NewAreaFrame(mPresShell, styleContext,
                                     NS_BLOCK_SPACE_MGR | NS_BLOCK_MARGIN_ROOT);
  InitAndRestoreFrame(aState, aContent, newFrame, nsnull, areaFrame);

  nsresult rv = aState.AddChild(newFrame, aFrameItems, aStyleDisplay, aContent,
                                aStyleContext, aParentFrame);
  if (NS_FAILED(rv)) {
    return rv;
  }
  

  
  PRBool haveFirstLetterStyle, haveFirstLineStyle;
  ShouldHaveSpecialBlockStyle(aContent, aStyleContext,
                              &haveFirstLetterStyle, &haveFirstLineStyle);
  nsFrameConstructorSaveState floatSaveState;
  aState.PushFloatContainingBlock(areaFrame, floatSaveState,
                                  haveFirstLetterStyle,
                                  haveFirstLineStyle);

  
  nsFrameConstructorSaveState absoluteSaveState;
  nsFrameItems                childItems;

  if (aStyleDisplay->IsPositioned()) {
    
    
    aState.PushAbsoluteContainingBlock(areaFrame, absoluteSaveState);
  }

  ProcessChildren(aState, aContent, areaFrame, PR_TRUE,
                  childItems, PR_TRUE);

  static NS_DEFINE_IID(kLegendFrameCID, NS_LEGEND_FRAME_CID);
  nsIFrame * child      = childItems.childList;
  nsIFrame * previous   = nsnull;
  nsIFrame* legendFrame = nsnull;
  while (nsnull != child) {
    nsresult result = child->QueryInterface(kLegendFrameCID, (void**)&legendFrame);
    if (NS_SUCCEEDED(result) && legendFrame) {
      
      
      
      
      
      if (nsnull != previous) {
        previous->SetNextSibling(legendFrame->GetNextSibling());
      } else {
        childItems.childList = legendFrame->GetNextSibling();
      }
      legendFrame->SetNextSibling(areaFrame);
      legendFrame->SetParent(newFrame);
      break;
    }
    previous = child;
    child = child->GetNextSibling();
  }

  
  areaFrame->SetInitialChildList(nsnull, childItems.childList);

  
  newFrame->SetInitialChildList(nsnull, legendFrame ? legendFrame : areaFrame);

  
  aNewFrame = newFrame; 

  
  aFrameHasBeenInitialized = PR_TRUE; 

  return NS_OK;
}

nsresult
nsCSSFrameConstructor::ConstructTextFrame(nsFrameConstructorState& aState,
                                          nsIContent*              aContent,
                                          nsIFrame*                aParentFrame,
                                          nsStyleContext*          aStyleContext,
                                          nsFrameItems&            aFrameItems,
                                          PRBool                   aPseudoParent)
{
  
  if (!aPseudoParent && !aState.mPseudoFrames.IsEmpty() &&
      !TextIsOnlyWhitespace(aContent))
    ProcessPseudoFrames(aState, aFrameItems);

  nsIFrame* newFrame = nsnull;

#ifdef MOZ_SVG
  if (aParentFrame->IsFrameOfType(nsIFrame::eSVG)) {
    nsIFrame *ancestorFrame = SVG_GetFirstNonAAncestorFrame(aParentFrame);
    if (ancestorFrame) {
      nsISVGTextContentMetrics* metrics;
      CallQueryInterface(ancestorFrame, &metrics);
      if (!metrics) {
        return NS_OK;
      }
      newFrame = NS_NewSVGGlyphFrame(mPresShell, aContent,
                                     ancestorFrame, aStyleContext);
    }
  }
  else {
    newFrame = NS_NewTextFrame(mPresShell, aStyleContext);
  }
#else
  newFrame = NS_NewTextFrame(mPresShell, aStyleContext);
#endif

  if (NS_UNLIKELY(!newFrame))
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv = InitAndRestoreFrame(aState, aContent, aParentFrame,
                                    nsnull, newFrame);

  if (NS_FAILED(rv)) {
    newFrame->Destroy();
    return rv;
  }

  

  
  newFrame->SetInitialChildList(nsnull, nsnull);

  
  aFrameItems.AddChild(newFrame);

  
  

  return rv;
}

nsresult
nsCSSFrameConstructor::ConstructHTMLFrame(nsFrameConstructorState& aState,
                                          nsIContent*              aContent,
                                          nsIFrame*                aParentFrame,
                                          nsIAtom*                 aTag,
                                          PRInt32                  aNameSpaceID,
                                          nsStyleContext*          aStyleContext,
                                          nsFrameItems&            aFrameItems,
                                          PRBool                   aHasPseudoParent)
{
  
  
  if (!aContent->IsNodeOfType(nsINode::eHTML) &&
      aNameSpaceID != kNameSpaceID_XHTML) {
    return NS_OK;
  }

  PRBool    frameHasBeenInitialized = PR_FALSE;
  nsIFrame* newFrame = nsnull;  
  PRBool    addToHashTable = PR_TRUE;
  PRBool    isFloatContainer = PR_FALSE;
  PRBool    addedToFrameList = PR_FALSE;
  nsresult  rv = NS_OK;
  
  PRBool triedFrame = PR_FALSE;

  
  const nsStyleDisplay* display = aStyleContext->GetStyleDisplay();

  
  if (nsGkAtoms::img == aTag) {
    
    rv = CreateHTMLImageFrame(aContent, aStyleContext, NS_NewImageFrame,
                              &newFrame);
    if (newFrame) {
      if (!aHasPseudoParent && !aState.mPseudoFrames.IsEmpty()) {
        ProcessPseudoFrames(aState, aFrameItems); 
      }
    }
  }
  else if (nsGkAtoms::br == aTag) {
    if (!aHasPseudoParent && !aState.mPseudoFrames.IsEmpty()) {
      ProcessPseudoFrames(aState, aFrameItems); 
    }
    newFrame = NS_NewBRFrame(mPresShell, aStyleContext);
    triedFrame = PR_TRUE;

    
    
    
    addToHashTable = PR_FALSE;
  }
  else if (nsGkAtoms::wbr == aTag) {
    if (!aHasPseudoParent && !aState.mPseudoFrames.IsEmpty()) {
      ProcessPseudoFrames(aState, aFrameItems); 
    }
    newFrame = NS_NewWBRFrame(mPresShell, aStyleContext);
    triedFrame = PR_TRUE;
  }
  else if (nsGkAtoms::input == aTag) {
    if (!aHasPseudoParent && !aState.mPseudoFrames.IsEmpty()) {
        ProcessPseudoFrames(aState, aFrameItems); 
    }
    
    rv = CreateInputFrame(aState, aContent, aParentFrame,
                          aTag, aStyleContext, &newFrame,
                          display, frameHasBeenInitialized,
                          addedToFrameList, aFrameItems);  
  }
  else if (nsGkAtoms::textarea == aTag) {
    if (!aHasPseudoParent && !aState.mPseudoFrames.IsEmpty()) {
      ProcessPseudoFrames(aState, aFrameItems); 
    }
    newFrame = NS_NewTextControlFrame(mPresShell, aStyleContext);
    triedFrame = PR_TRUE;
  }
  else if (nsGkAtoms::select == aTag) {
    if (!gUseXBLForms) {
      if (!aHasPseudoParent && !aState.mPseudoFrames.IsEmpty()) {
        ProcessPseudoFrames(aState, aFrameItems); 
      }
      rv = ConstructSelectFrame(aState, aContent, aParentFrame,
                                aTag, aStyleContext, newFrame,
                                display, frameHasBeenInitialized,
                                aFrameItems);
      if (newFrame) {
        NS_ASSERTION(nsPlaceholderFrame::GetRealFrameFor(aFrameItems.lastChild) ==
                     newFrame,
                     "Frame didn't get added to aFrameItems?");
        addedToFrameList = PR_TRUE;
      }
    }
  }
  else if (nsGkAtoms::object == aTag ||
           nsGkAtoms::applet == aTag ||
           nsGkAtoms::embed == aTag) {
    
    if (!(aContent->IntrinsicState() &
          (NS_EVENT_STATE_BROKEN | NS_EVENT_STATE_USERDISABLED |
           NS_EVENT_STATE_SUPPRESSED))) {
      if (!aHasPseudoParent && !aState.mPseudoFrames.IsEmpty()) {
        ProcessPseudoFrames(aState, aFrameItems); 
      }

      nsCOMPtr<nsIObjectLoadingContent> objContent(do_QueryInterface(aContent));
      NS_ASSERTION(objContent,
                   "applet, embed and object must implement nsIObjectLoadingContent!");
      if (!objContent) {
        
        return NS_ERROR_UNEXPECTED;
      }

      PRUint32 type;
      objContent->GetDisplayedType(&type);
      if (type == nsIObjectLoadingContent::TYPE_LOADING) {
        
        
        
        newFrame = NS_NewEmptyFrame(mPresShell, aStyleContext);
      }
      else if (type == nsIObjectLoadingContent::TYPE_PLUGIN)
        newFrame = NS_NewObjectFrame(mPresShell, aStyleContext);
      else if (type == nsIObjectLoadingContent::TYPE_IMAGE)
        newFrame = NS_NewImageFrame(mPresShell, aStyleContext);
      else if (type == nsIObjectLoadingContent::TYPE_DOCUMENT)
        newFrame = NS_NewSubDocumentFrame(mPresShell, aStyleContext);
#ifdef DEBUG
      else
        NS_ERROR("Shouldn't get here if we're not broken and not "
                 "suppressed and not blocked");
#endif

      triedFrame = PR_TRUE;
    }
  }
  else if (nsGkAtoms::fieldset == aTag) {
    if (!aHasPseudoParent && !aState.mPseudoFrames.IsEmpty()) {
      ProcessPseudoFrames(aState, aFrameItems); 
    }
    rv = ConstructFieldSetFrame(aState, aContent, aParentFrame,
                                aTag, aStyleContext, newFrame,
                                aFrameItems, display, frameHasBeenInitialized);
    NS_ASSERTION(nsPlaceholderFrame::GetRealFrameFor(aFrameItems.lastChild) ==
                 newFrame,
                 "Frame didn't get added to aFrameItems?");
    addedToFrameList = PR_TRUE;
  }
  else if (nsGkAtoms::legend == aTag) {
    NS_ASSERTION(!display->IsAbsolutelyPositioned() && !display->IsFloating(),
                 "Legends should not be positioned and should not float");
    
    if (!aHasPseudoParent && !aState.mPseudoFrames.IsEmpty()) {
      ProcessPseudoFrames(aState, aFrameItems); 
    }
    newFrame = NS_NewLegendFrame(mPresShell, aStyleContext);
    triedFrame = PR_TRUE;

    isFloatContainer = PR_TRUE;
  }
  else if (nsGkAtoms::frameset == aTag) {
    NS_ASSERTION(!display->IsAbsolutelyPositioned() && !display->IsFloating(),
                 "Framesets should not be positioned and should not float");
    
    if (!aHasPseudoParent && !aState.mPseudoFrames.IsEmpty()) {
      ProcessPseudoFrames(aState, aFrameItems); 
    }
   
    newFrame = NS_NewHTMLFramesetFrame(mPresShell, aStyleContext);
    triedFrame = PR_TRUE;
  }
  else if (nsGkAtoms::iframe == aTag) {
    if (!aHasPseudoParent && !aState.mPseudoFrames.IsEmpty()) {
      ProcessPseudoFrames(aState, aFrameItems); 
    }
    
    newFrame = NS_NewSubDocumentFrame(mPresShell, aStyleContext);
    triedFrame = PR_TRUE;

    if (newFrame) {
      
      
      
      nsCOMPtr<nsIAtom> contentParentAtom = do_GetAtom("contentParent");
      aState.mPresContext->PropertyTable()->
        SetProperty(newFrame, contentParentAtom,
                    aParentFrame, nsnull, nsnull);
    }
  }
  else if (nsGkAtoms::spacer == aTag) {
    NS_ASSERTION(!display->IsAbsolutelyPositioned() && !display->IsFloating(),
                 "Spacers should not be positioned and should not float");
    if (!aHasPseudoParent && !aState.mPseudoFrames.IsEmpty()) {
      ProcessPseudoFrames(aState, aFrameItems); 
    }
    newFrame = NS_NewSpacerFrame(mPresShell, aStyleContext);
    triedFrame = PR_TRUE;
  }
  else if (nsGkAtoms::button == aTag) {
    if (!aHasPseudoParent && !aState.mPseudoFrames.IsEmpty()) {
      ProcessPseudoFrames(aState, aFrameItems); 
    }
    
    rv = ConstructButtonFrame(aState, aContent, aParentFrame,
                              aTag, aStyleContext, &newFrame,
                              display, aFrameItems);
    
    
    
    
    frameHasBeenInitialized = PR_TRUE;
    addedToFrameList = PR_TRUE;
    isFloatContainer = PR_TRUE;
  }
  else if (nsGkAtoms::isindex == aTag) {
    if (!aHasPseudoParent && !aState.mPseudoFrames.IsEmpty()) {
      ProcessPseudoFrames(aState, aFrameItems);
    }
    newFrame = NS_NewIsIndexFrame(mPresShell, aStyleContext);
    triedFrame = PR_TRUE;
  }
  else if (nsGkAtoms::canvas == aTag) {
    if (!aHasPseudoParent && !aState.mPseudoFrames.IsEmpty()) {
      ProcessPseudoFrames(aState, aFrameItems); 
    }
    newFrame = NS_NewHTMLCanvasFrame(mPresShell, aStyleContext);
    triedFrame = PR_TRUE;
  }

  if (NS_UNLIKELY(triedFrame && !newFrame)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  else if (NS_FAILED(rv) || !newFrame) {
    return rv;
  }

  
  

  
  
  
  if (!frameHasBeenInitialized) {
    NS_ASSERTION(!addedToFrameList,
                 "Frames that were already added to the frame list should be "
                 "initialized by now!");
    nsIFrame* geometricParent = aState.GetGeometricParent(display,
                                                          aParentFrame);
     
    rv = InitAndRestoreFrame(aState, aContent, geometricParent, nsnull, newFrame);
    NS_ASSERTION(NS_SUCCEEDED(rv), "InitAndRestoreFrame failed");
    
    
    nsHTMLContainerFrame::CreateViewForFrame(newFrame, aParentFrame, PR_FALSE);

    rv = aState.AddChild(newFrame, aFrameItems, display, aContent,
                         aStyleContext, aParentFrame);
    if (NS_FAILED(rv)) {
      return rv;
    }
    addedToFrameList = PR_TRUE;
      
    
    nsFrameItems childItems;
    nsFrameConstructorSaveState absoluteSaveState;
    nsFrameConstructorSaveState floatSaveState;
    if (!newFrame->IsLeaf()) {
      if (display->IsPositioned()) {
        aState.PushAbsoluteContainingBlock(newFrame, absoluteSaveState);
      }
      if (isFloatContainer) {
        PRBool haveFirstLetterStyle, haveFirstLineStyle;
        ShouldHaveSpecialBlockStyle(aContent, aStyleContext,
                                    &haveFirstLetterStyle,
                                    &haveFirstLineStyle);
        aState.PushFloatContainingBlock(newFrame, floatSaveState,
                                        PR_FALSE, PR_FALSE);
      }

      
      rv = ProcessChildren(aState, aContent, newFrame,
                           PR_TRUE, childItems, PR_FALSE);
    }

    
    CreateAnonymousFrames(aTag, aState, aContent, newFrame,
                          PR_FALSE, childItems);

    
    if (childItems.childList) {
      newFrame->SetInitialChildList(nsnull, childItems.childList);
    }
  }

  if (!addedToFrameList) {
    
    
    
    rv = aState.AddChild(newFrame, aFrameItems, display, aContent,
                         aStyleContext, aParentFrame);
    if (NS_FAILED(rv)) {
      return rv;
    }
  }

  if (addToHashTable) {
    
    
    
    aState.mFrameManager->SetPrimaryFrameFor(aContent, newFrame);
  }

  return rv;
}

nsresult
nsCSSFrameConstructor::CreateAnonymousFrames(nsIAtom*                 aTag,
                                             nsFrameConstructorState& aState,
                                             nsIContent*              aParent,
                                             nsIFrame*                aNewFrame,
                                             PRBool                   aAppendToExisting,
                                             nsFrameItems&            aChildItems,
                                             PRBool                   aIsRoot)
{
  
  
  
  
  
  

  
  
  if (!aIsRoot &&
      aTag != nsGkAtoms::input &&
      aTag != nsGkAtoms::textarea &&
      aTag != nsGkAtoms::combobox &&
      aTag != nsGkAtoms::isindex &&
      aTag != nsGkAtoms::scrollbar
#ifdef MOZ_SVG
      && aTag != nsGkAtoms::use
#endif
      )
    return NS_OK;

  return CreateAnonymousFrames(aState, aParent, mDocument, aNewFrame,
                               aAppendToExisting, aChildItems);
}



nsresult
nsCSSFrameConstructor::CreateAnonymousFrames(nsFrameConstructorState& aState,
                                             nsIContent*              aParent,
                                             nsIDocument*             aDocument,
                                             nsIFrame*                aParentFrame,
                                             PRBool                   aAppendToExisting,
                                             nsFrameItems&            aChildItems)
{
  nsIAnonymousContentCreator* creator = nsnull;
  CallQueryInterface(aParentFrame, &creator);
  if (!creator)
    return NS_OK;

  nsresult rv;

  nsAutoTArray<nsIContent*, 4> newAnonymousItems;
  rv = creator->CreateAnonymousContent(newAnonymousItems);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 count = newAnonymousItems.Length();
  if (count == 0) {
    return NS_OK;
  }

  
  
  nsPseudoFrames priorPseudoFrames; 
  aState.mPseudoFrames.Reset(&priorPseudoFrames);

  for (PRUint32 i=0; i < count; i++) {
    
    nsIContent* content = newAnonymousItems[i];
    NS_ASSERTION(content, "null anonymous content?");

    content->SetNativeAnonymous(PR_TRUE);

    nsIContent* bindingParent = content;
#ifdef MOZ_SVG
    
    
    if (aParent &&
        aParent->NodeInfo()->Equals(nsGkAtoms::use, kNameSpaceID_SVG))
      bindingParent = aParent;
#endif

    rv = content->BindToTree(aDocument, aParent, bindingParent, PR_TRUE);
    if (NS_FAILED(rv)) {
      content->UnbindFromTree();
      return rv;
    }

    nsIFrame* newFrame = creator->CreateFrameFor(content);
    if (newFrame) {
      aChildItems.AddChild(newFrame);
    }
    else {
      
      ConstructFrame(aState, content, aParentFrame, aChildItems);
    }
  }

  creator->PostCreateFrames();

  
  if (!aState.mPseudoFrames.IsEmpty()) {
    ProcessPseudoFrames(aState, aChildItems);
  }

  
  aState.mPseudoFrames = priorPseudoFrames;

  return NS_OK;
}

static
PRBool IsXULDisplayType(const nsStyleDisplay* aDisplay)
{
  return (aDisplay->mDisplay == NS_STYLE_DISPLAY_INLINE_BOX || 
          aDisplay->mDisplay == NS_STYLE_DISPLAY_INLINE_GRID || 
          aDisplay->mDisplay == NS_STYLE_DISPLAY_INLINE_STACK ||
          aDisplay->mDisplay == NS_STYLE_DISPLAY_BOX ||
          aDisplay->mDisplay == NS_STYLE_DISPLAY_GRID ||
          aDisplay->mDisplay == NS_STYLE_DISPLAY_STACK ||
          aDisplay->mDisplay == NS_STYLE_DISPLAY_GRID_GROUP ||
          aDisplay->mDisplay == NS_STYLE_DISPLAY_GRID_LINE ||
          aDisplay->mDisplay == NS_STYLE_DISPLAY_DECK ||
          aDisplay->mDisplay == NS_STYLE_DISPLAY_POPUP ||
          aDisplay->mDisplay == NS_STYLE_DISPLAY_GROUPBOX
          );
}

nsresult
nsCSSFrameConstructor::ConstructXULFrame(nsFrameConstructorState& aState,
                                         nsIContent*              aContent,
                                         nsIFrame*                aParentFrame,
                                         nsIAtom*                 aTag,
                                         PRInt32                  aNameSpaceID,
                                         nsStyleContext*          aStyleContext,
                                         nsFrameItems&            aFrameItems,
                                         PRBool                   aXBLBaseTag,
                                         PRBool                   aHasPseudoParent,
                                         PRBool*                  aHaltProcessing)
{
  *aHaltProcessing = PR_FALSE;

  PRBool    primaryFrameSet = PR_FALSE;
  nsresult  rv = NS_OK;
  PRBool    isPopup = PR_FALSE;
  PRBool    frameHasBeenInitialized = PR_FALSE;

  
  
  
  nsIFrame* newFrame = nsnull;
  
  
  
  
  nsIFrame* topFrame = nsnull;

  
  nsIFrame* origParentFrame = aParentFrame;

  NS_ASSERTION(aTag != nsnull, "null XUL tag");
  if (aTag == nsnull)
    return NS_OK;

  const nsStyleDisplay* display = aStyleContext->GetStyleDisplay();
  
  PRBool isXULNS = (aNameSpaceID == kNameSpaceID_XUL);
  PRBool isXULDisplay = IsXULDisplayType(display);

   
  if (isXULDisplay && !isXULNS) {
    isXULDisplay = !IsSpecialContent(aContent, aTag, aNameSpaceID, aStyleContext);
  }

  PRBool triedFrame = PR_FALSE;

  if (isXULNS || isXULDisplay) {
    PRBool mayBeScrollable = PR_FALSE;

    if (isXULNS) {
      triedFrame = PR_TRUE;
    
      
      
#ifdef MOZ_XUL
      
      if (aTag == nsGkAtoms::button || aTag == nsGkAtoms::checkbox || aTag == nsGkAtoms::radio) {
        newFrame = NS_NewButtonBoxFrame(mPresShell, aStyleContext);

        
        mayBeScrollable = PR_TRUE;
      } 
      
      else if (aTag == nsGkAtoms::autorepeatbutton) {
        newFrame = NS_NewAutoRepeatBoxFrame(mPresShell, aStyleContext);

        
        mayBeScrollable = PR_TRUE;
      } 

      
      else if (aTag == nsGkAtoms::titlebar) {
        newFrame = NS_NewTitleBarFrame(mPresShell, aStyleContext);

        
        mayBeScrollable = PR_TRUE;
      } 

      
      else if (aTag == nsGkAtoms::resizer) {
        newFrame = NS_NewResizerFrame(mPresShell, aStyleContext);

        
        mayBeScrollable = PR_TRUE;
      } 

      else if (aTag == nsGkAtoms::image) {
        newFrame = NS_NewImageBoxFrame(mPresShell, aStyleContext);
      }
      else if (aTag == nsGkAtoms::spring ||
               aTag == nsGkAtoms::spacer) {
        newFrame = NS_NewLeafBoxFrame(mPresShell, aStyleContext);
      }
       else if (aTag == nsGkAtoms::treechildren) {
        newFrame = NS_NewTreeBodyFrame(mPresShell, aStyleContext);
      }
      else if (aTag == nsGkAtoms::treecol) {
        newFrame = NS_NewTreeColFrame(mPresShell, aStyleContext);
      }
      
      else if (aTag == nsGkAtoms::text || aTag == nsGkAtoms::label ||
               aTag == nsGkAtoms::description) {
        if ((aTag == nsGkAtoms::label || aTag == nsGkAtoms::description) && 
            (! aContent->HasAttr(kNameSpaceID_None, nsGkAtoms::value))) {
          
          
          newFrame = NS_NewAreaFrame(mPresShell, aStyleContext,
                                     NS_BLOCK_SPACE_MGR | NS_BLOCK_MARGIN_ROOT);
        }
        else {
          newFrame = NS_NewTextBoxFrame(mPresShell, aStyleContext);
        }
      }
      

       
      else if (aTag == nsGkAtoms::menu ||
               aTag == nsGkAtoms::menuitem || 
               aTag == nsGkAtoms::menubutton) {
        
        
        
        newFrame = NS_NewMenuFrame(mPresShell, aStyleContext,
          (aTag != nsGkAtoms::menuitem));
      }
      else if (aTag == nsGkAtoms::menubar) {
  #ifdef XP_MACOSX
        
        
        PRBool isRootChromeShell = PR_FALSE;
        nsCOMPtr<nsISupports> container = aState.mPresContext->GetContainer();
        if (container) {
          nsCOMPtr<nsIDocShellTreeItem> treeItem(do_QueryInterface(container));
          if (treeItem) {
            PRInt32 type;
            treeItem->GetItemType(&type);
            if (nsIDocShellTreeItem::typeChrome == type) {
              nsCOMPtr<nsIDocShellTreeItem> parent;
              treeItem->GetParent(getter_AddRefs(parent));
              isRootChromeShell = !parent;
            }
          }
        }

        if (isRootChromeShell) {
          *aHaltProcessing = PR_TRUE;
          return NS_OK;
        }
  #endif

        newFrame = NS_NewMenuBarFrame(mPresShell, aStyleContext);
      }
      else if (aTag == nsGkAtoms::popupgroup) {
        
        newFrame = NS_NewPopupSetFrame(mPresShell, aStyleContext);
      }
      else if (aTag == nsGkAtoms::iframe || aTag == nsGkAtoms::editor ||
               aTag == nsGkAtoms::browser) {
        newFrame = NS_NewSubDocumentFrame(mPresShell, aStyleContext);
      }
      
      else if (aTag == nsGkAtoms::progressmeter) {
        newFrame = NS_NewProgressMeterFrame(mPresShell, aStyleContext);
      }
      
      else
#endif
      
      if (aTag == nsGkAtoms::slider) {
        newFrame = NS_NewSliderFrame(mPresShell, aStyleContext);
      }
      

      
      else if (aTag == nsGkAtoms::scrollbar) {
        newFrame = NS_NewScrollbarFrame(mPresShell, aStyleContext);
      }
      

      
      else if (aTag == nsGkAtoms::scrollbarbutton) {
        newFrame = NS_NewScrollbarButtonFrame(mPresShell, aStyleContext);
      }
      

#ifdef MOZ_XUL
      
      else if (aTag == nsGkAtoms::splitter) {
        newFrame = NS_NewSplitterFrame(mPresShell, aStyleContext);
      }
      

      else {
        triedFrame = PR_FALSE;
      }
#endif
    }

    
    
    
    if (!newFrame && isXULDisplay) {
      triedFrame = PR_TRUE;
  
      if (display->mDisplay == NS_STYLE_DISPLAY_INLINE_BOX ||
               display->mDisplay == NS_STYLE_DISPLAY_BOX) {
        newFrame = NS_NewBoxFrame(mPresShell, aStyleContext, PR_FALSE, nsnull);

        
        mayBeScrollable = PR_TRUE;
      } 
#ifdef MOZ_XUL
      
      else if (display->mDisplay == NS_STYLE_DISPLAY_INLINE_GRID ||
               display->mDisplay == NS_STYLE_DISPLAY_GRID) {
        nsCOMPtr<nsIBoxLayout> layout;
        NS_NewGridLayout2(mPresShell, getter_AddRefs(layout));
        newFrame = NS_NewBoxFrame(mPresShell, aStyleContext, PR_FALSE, layout);

        
        mayBeScrollable = PR_TRUE;
      } 

      
      else if (display->mDisplay == NS_STYLE_DISPLAY_GRID_GROUP) {
        nsCOMPtr<nsIBoxLayout> layout;
      
        if (aTag == nsGkAtoms::listboxbody) {
          NS_NewListBoxLayout(mPresShell, layout);
          newFrame = NS_NewListBoxBodyFrame(mPresShell, aStyleContext, PR_FALSE, layout);
        }
        else
        {
          NS_NewGridRowGroupLayout(mPresShell, getter_AddRefs(layout));
          newFrame = NS_NewGridRowGroupFrame(mPresShell, aStyleContext, PR_FALSE, layout);
        }

        
        if (display->IsScrollableOverflow()) {
          
          BuildScrollFrame(aState, aContent, aStyleContext, newFrame,
                           aParentFrame, nsnull, topFrame, aStyleContext);

          
          aParentFrame = newFrame->GetParent();

          primaryFrameSet = PR_TRUE;

          frameHasBeenInitialized = PR_TRUE;
        }
      } 

      
      else if (display->mDisplay == NS_STYLE_DISPLAY_GRID_LINE) {
        nsCOMPtr<nsIBoxLayout> layout;


        NS_NewGridRowLeafLayout(mPresShell, getter_AddRefs(layout));

        if (aTag == nsGkAtoms::listitem)
          newFrame = NS_NewListItemFrame(mPresShell, aStyleContext, PR_FALSE, layout);
        else
          newFrame = NS_NewGridRowLeafFrame(mPresShell, aStyleContext, PR_FALSE, layout);

        
        mayBeScrollable = PR_TRUE;
      } 
      
       
      else if (display->mDisplay == NS_STYLE_DISPLAY_DECK) {
        newFrame = NS_NewDeckFrame(mPresShell, aStyleContext);
      }
      
      else if (display->mDisplay == NS_STYLE_DISPLAY_GROUPBOX) {
        newFrame = NS_NewGroupBoxFrame(mPresShell, aStyleContext);

        
        mayBeScrollable = PR_TRUE;
      } 
      
      else if (display->mDisplay == NS_STYLE_DISPLAY_STACK ||
               display->mDisplay == NS_STYLE_DISPLAY_INLINE_STACK) {
        newFrame = NS_NewStackFrame(mPresShell, aStyleContext);

        mayBeScrollable = PR_TRUE;
      }
      else if (display->mDisplay == NS_STYLE_DISPLAY_POPUP) {
        
        
        
        
        
        
        if (aParentFrame->GetType() != nsGkAtoms::menuFrame) {
          if (!aState.mPopupItems.containingBlock) {
            
            
            *aHaltProcessing = PR_TRUE;
            NS_ASSERTION(!aState.mRootBox, "Popup containing block is missing");
            return NS_OK;
          }

#ifdef NS_DEBUG
          NS_ASSERTION(aState.mPopupItems.containingBlock->GetType() ==
                       nsGkAtoms::popupSetFrame,
                       "Popup containing block isn't a nsIPopupSetFrame");
#endif
          isPopup = PR_TRUE;
        }

        
        newFrame = NS_NewMenuPopupFrame(mPresShell, aStyleContext);

        if (aTag == nsGkAtoms::tooltip) {
          if (aContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::_default,
                                    nsGkAtoms::_true, eIgnoreCase)) {
            
            if (aState.mRootBox)
              aState.mRootBox->SetDefaultTooltip(aContent);
          }
        }
      }
      
      else {
        triedFrame = PR_FALSE;
      }
#endif 
    }

    if (mayBeScrollable && display->IsScrollableOverflow()) {
      
      BuildScrollFrame(aState, aContent, aStyleContext, newFrame,
                       aParentFrame, nsnull, topFrame, aStyleContext);

      
      
      
      
      aParentFrame = newFrame->GetParent();
      primaryFrameSet = PR_TRUE;
      frameHasBeenInitialized = PR_TRUE;
    }
  }
  
  if (NS_UNLIKELY(triedFrame && !newFrame))
  {
    rv = NS_ERROR_OUT_OF_MEMORY;
  }

  
  
  if (NS_SUCCEEDED(rv) && newFrame != nsnull) {

    
    if (topFrame == nsnull)
        topFrame = newFrame;

    
    nsIFrame* geometricParent;
#ifdef MOZ_XUL
    if (isPopup) {
      NS_ASSERTION(aState.mPopupItems.containingBlock, "How did we get here?");
      geometricParent = aState.mPopupItems.containingBlock;
    }
    else
#endif
    {
      geometricParent = aParentFrame;
    }
    
    


    
    if (!frameHasBeenInitialized) {

      rv = InitAndRestoreFrame(aState, aContent, geometricParent, nsnull, newFrame);

      if (NS_FAILED(rv)) {
        newFrame->Destroy();
        return rv;
      }
      
      








        
        nsHTMLContainerFrame::CreateViewForFrame(newFrame, aParentFrame, PR_FALSE);

      






      
    }

    
    
    rv = aState.AddChild(topFrame, aFrameItems, display, aContent,
                         aStyleContext, origParentFrame, PR_FALSE, PR_FALSE,
                         isPopup);
    if (NS_FAILED(rv)) {
      return rv;
    }

#ifdef MOZ_XUL
    if (aTag == nsGkAtoms::popupgroup) {
      nsIRootBox* rootBox = nsIRootBox::GetRootBox(mPresShell);
      if (rootBox) {
        NS_ASSERTION(rootBox->GetPopupSetFrame() == newFrame,
                     "Unexpected PopupSetFrame");
        aState.mPopupItems.containingBlock = rootBox->GetPopupSetFrame();
      }      
    }
#endif

    
    
    
    nsFrameConstructorSaveState floatSaveState;
    PRBool isFloatContainingBlock =
      newFrame->GetContentInsertionFrame()->IsFloatContainingBlock();
    aState.PushFloatContainingBlock(isFloatContainingBlock ? newFrame : nsnull,
                                    floatSaveState, PR_FALSE, PR_FALSE);

    
    nsFrameItems childItems;
    if (!newFrame->IsLeaf()) {
      
      
      if (mDocument->BindingManager()->ShouldBuildChildFrames(aContent)) {
        rv = ProcessChildren(aState, aContent, newFrame, PR_FALSE,
                             childItems, PR_FALSE);
      }
    }
      
    CreateAnonymousFrames(aTag, aState, aContent, newFrame, PR_FALSE,
                          childItems);

    
    newFrame->SetInitialChildList(nsnull, childItems.childList);
  }

#ifdef MOZ_XUL
  
  if (aTag == nsGkAtoms::treechildren || 
      aContent->HasAttr(kNameSpaceID_None, nsGkAtoms::tooltiptext) ||
      aContent->HasAttr(kNameSpaceID_None, nsGkAtoms::tooltip))
  {
    nsIRootBox* rootBox = nsIRootBox::GetRootBox(mPresShell);
    if (rootBox)
      rootBox->AddTooltipSupport(aContent);
  }
#endif



  if (topFrame) {
    
    

    
    
    
    if (!primaryFrameSet)
        aState.mFrameManager->SetPrimaryFrameFor(aContent, topFrame);
  }

  return rv;
}

nsresult
nsCSSFrameConstructor::AddLazyChildren(nsIContent* aContent,
                                       nsLazyFrameConstructionCallback* aCallback,
                                       void* aArg)
{
  nsCOMPtr<nsIRunnable> event =
    new LazyGenerateChildrenEvent(aContent, mPresShell, aCallback, aArg);
  return NS_DispatchToCurrentThread(event);
}

already_AddRefed<nsStyleContext>
nsCSSFrameConstructor::BeginBuildingScrollFrame(nsFrameConstructorState& aState,
                                                nsIContent*              aContent,
                                                nsStyleContext*          aContentStyle,
                                                nsIFrame*                aParentFrame,
                                                nsIFrame*                aContentParentFrame,
                                                nsIAtom*                 aScrolledPseudo,
                                                PRBool                   aIsRoot,
                                                nsIFrame*&               aNewFrame)
{
  nsIFrame* parentFrame = nsnull;
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

    
    nsHTMLContainerFrame::CreateViewForFrame(gfxScrollFrame,
                                             aContentParentFrame, PR_FALSE);
  }

  
  
  CreateAnonymousFrames(aState, aContent, mDocument, gfxScrollFrame,
                        PR_FALSE, anonymousItems);

  parentFrame = gfxScrollFrame;
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

  
  
  
  nsHTMLContainerFrame::CreateViewForFrame(aScrolledFrame, nsnull, PR_TRUE);
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
                                        nsIFrame*                aContentParentFrame,
                                        nsIFrame*&               aNewFrame, 
                                        nsStyleContext*&         aScrolledContentStyle)
{
    nsRefPtr<nsStyleContext> scrolledContentStyle =
      BeginBuildingScrollFrame(aState, aContent, aContentStyle, aParentFrame,
                               aContentParentFrame, nsCSSAnonBoxes::scrolledContent,
                               PR_FALSE, aNewFrame);
    
    aScrolledFrame->SetStyleContextWithoutNotification(scrolledContentStyle);
    InitAndRestoreFrame(aState, aContent, aNewFrame, nsnull, aScrolledFrame);

    FinishBuildingScrollFrame(aNewFrame, aScrolledFrame);

    aScrolledContentStyle = scrolledContentStyle;

    
    aState.mFrameManager->SetPrimaryFrameFor( aContent, aNewFrame );
    return NS_OK;

}

nsresult
nsCSSFrameConstructor::ConstructFrameByDisplayType(nsFrameConstructorState& aState,
                                                   const nsStyleDisplay*    aDisplay,
                                                   nsIContent*              aContent,
                                                   PRInt32                  aNameSpaceID,
                                                   nsIAtom*                 aTag,
                                                   nsIFrame*                aParentFrame,
                                                   nsStyleContext*          aStyleContext,
                                                   nsFrameItems&            aFrameItems,
                                                   PRBool                   aHasPseudoParent)
{
  PRBool    primaryFrameSet = PR_FALSE;
  nsIFrame* newFrame = nsnull;  
  PRBool    addToHashTable = PR_TRUE;
  PRBool    addedToFrameList = PR_FALSE;
  nsresult  rv = NS_OK;

  
  
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

    if (!aHasPseudoParent && !aState.mPseudoFrames.IsEmpty()) {
      ProcessPseudoFrames(aState, aFrameItems); 
    }

    nsRefPtr<nsStyleContext> scrolledContentStyle
      = BeginBuildingScrollFrame(aState, aContent, aStyleContext,
                                 aState.GetGeometricParent(aDisplay, aParentFrame),
                                 aParentFrame,
                                 nsCSSAnonBoxes::scrolledContent,
                                 PR_FALSE, newFrame);
    
    
    
    nsIFrame* scrolledFrame =
        NS_NewAreaFrame(mPresShell, aStyleContext,
                        NS_BLOCK_SPACE_MGR | NS_BLOCK_MARGIN_ROOT);

    nsFrameItems blockItem;
    rv = ConstructBlock(aState,
                        scrolledContentStyle->GetStyleDisplay(), aContent,
                        newFrame, newFrame, scrolledContentStyle,
                        &scrolledFrame, blockItem, aDisplay->IsPositioned());
    NS_ASSERTION(blockItem.childList == scrolledFrame,
                 "Scrollframe's frameItems should be exactly the scrolled frame");
    FinishBuildingScrollFrame(newFrame, scrolledFrame);

    rv = aState.AddChild(newFrame, aFrameItems, aDisplay, aContent,
                         aStyleContext, aParentFrame);
    if (NS_FAILED(rv)) {
      return rv;
    }

    addedToFrameList = PR_TRUE;
  }
  
  else if (aDisplay->IsAbsolutelyPositioned() &&
           (NS_STYLE_DISPLAY_BLOCK == aDisplay->mDisplay ||
            NS_STYLE_DISPLAY_LIST_ITEM == aDisplay->mDisplay)) {

    if (!aHasPseudoParent && !aState.mPseudoFrames.IsEmpty()) {
      ProcessPseudoFrames(aState, aFrameItems); 
    }

    
    
    newFrame = NS_NewAbsoluteItemWrapperFrame(mPresShell, aStyleContext);

    rv = ConstructBlock(aState, aDisplay, aContent,
                        aState.GetGeometricParent(aDisplay, aParentFrame), aParentFrame,
                        aStyleContext, &newFrame, aFrameItems, PR_TRUE);
    if (NS_FAILED(rv)) {
      return rv;
    }

    addedToFrameList = PR_TRUE;
  }
  
  else if (aDisplay->IsFloating() &&
           (NS_STYLE_DISPLAY_BLOCK == aDisplay->mDisplay ||
            NS_STYLE_DISPLAY_LIST_ITEM == aDisplay->mDisplay)) {
    if (!aHasPseudoParent && !aState.mPseudoFrames.IsEmpty()) {
      ProcessPseudoFrames(aState, aFrameItems); 
    }
    
    
    newFrame = NS_NewFloatingItemWrapperFrame(mPresShell, aStyleContext);

    rv = ConstructBlock(aState, aDisplay, aContent, 
                        aState.GetGeometricParent(aDisplay, aParentFrame),
                        aParentFrame, aStyleContext, &newFrame, aFrameItems,
                        aDisplay->mPosition == NS_STYLE_POSITION_RELATIVE);
    if (NS_FAILED(rv)) {
      return rv;
    }

    addedToFrameList = PR_TRUE;
  }
  
  else if ((NS_STYLE_POSITION_RELATIVE == aDisplay->mPosition) &&
           ((NS_STYLE_DISPLAY_BLOCK == aDisplay->mDisplay) ||
            (NS_STYLE_DISPLAY_INLINE == aDisplay->mDisplay) ||
            (NS_STYLE_DISPLAY_LIST_ITEM == aDisplay->mDisplay))) {
    if (!aHasPseudoParent && !aState.mPseudoFrames.IsEmpty()) {
      ProcessPseudoFrames(aState, aFrameItems); 
    }
    
    if ((NS_STYLE_DISPLAY_BLOCK == aDisplay->mDisplay) ||
        (NS_STYLE_DISPLAY_LIST_ITEM == aDisplay->mDisplay)) {
      
      newFrame = NS_NewRelativeItemWrapperFrame(mPresShell, aStyleContext);
      
      ConstructBlock(aState, aDisplay, aContent,
                     aParentFrame, nsnull, aStyleContext, &newFrame,
                     aFrameItems, PR_TRUE);
      addedToFrameList = PR_TRUE;
    } else {
      
      newFrame = NS_NewPositionedInlineFrame(mPresShell, aStyleContext);
      
      
      ConstructInline(aState, aDisplay, aContent,
                      aParentFrame, aStyleContext, PR_TRUE, newFrame);
    }
  }
  
  else if ((NS_STYLE_DISPLAY_BLOCK == aDisplay->mDisplay) ||
           (NS_STYLE_DISPLAY_LIST_ITEM == aDisplay->mDisplay) ||
           (NS_STYLE_DISPLAY_RUN_IN == aDisplay->mDisplay) ||
           (NS_STYLE_DISPLAY_COMPACT == aDisplay->mDisplay) ||
           (NS_STYLE_DISPLAY_INLINE_BLOCK == aDisplay->mDisplay)) {
    if (!aHasPseudoParent && !aState.mPseudoFrames.IsEmpty()) {
      ProcessPseudoFrames(aState, aFrameItems); 
    }
    PRUint32 flags = 0;
    if (NS_STYLE_DISPLAY_INLINE_BLOCK == aDisplay->mDisplay) {
      flags = NS_BLOCK_SPACE_MGR | NS_BLOCK_MARGIN_ROOT;
    }
    
    newFrame = NS_NewBlockFrame(mPresShell, aStyleContext, flags);
    if (newFrame) { 
      
      rv = ConstructBlock(aState, aDisplay, aContent,
                          aParentFrame, nsnull, aStyleContext, &newFrame,
                          aFrameItems, PR_FALSE);
      addedToFrameList = PR_TRUE;
    }
    else {
      rv = NS_ERROR_OUT_OF_MEMORY;
    }
  }
  
  else if ((NS_STYLE_DISPLAY_INLINE == aDisplay->mDisplay) ||
           (NS_STYLE_DISPLAY_MARKER == aDisplay->mDisplay)) {
    if (!aHasPseudoParent && !aState.mPseudoFrames.IsEmpty()) {
      ProcessPseudoFrames(aState, aFrameItems); 
    }
    
    newFrame = NS_NewInlineFrame(mPresShell, aStyleContext);
    if (newFrame) { 
      
      
      rv = ConstructInline(aState, aDisplay, aContent,
                           aParentFrame, aStyleContext, PR_FALSE, newFrame);
    }
    else {
      rv = NS_ERROR_OUT_OF_MEMORY;
    }

    
    
    
    addToHashTable = PR_FALSE;
  }
  
  else {
    
    

    
    switch (aDisplay->mDisplay) {
    case NS_STYLE_DISPLAY_TABLE:
    case NS_STYLE_DISPLAY_INLINE_TABLE:
    {
      nsIFrame* innerTable;
      rv = ConstructTableFrame(aState, aContent, 
                               aParentFrame, aStyleContext,
                               aNameSpaceID, PR_FALSE, aFrameItems, PR_TRUE,
                               newFrame, innerTable);
      addedToFrameList = PR_TRUE;
      
      
      
      break;
    }
  
    
    case NS_STYLE_DISPLAY_TABLE_CAPTION:
    {
      
      
      nsIFrame* parentFrame = AdjustCaptionParentFrame(aParentFrame);
      rv = ConstructTableCaptionFrame(aState, aContent, parentFrame,
                                      aStyleContext, aNameSpaceID, aFrameItems,
                                      newFrame, aHasPseudoParent);
      if (NS_SUCCEEDED(rv) && !aHasPseudoParent) {
        aFrameItems.AddChild(newFrame);
      }
      return rv;
    }

    case NS_STYLE_DISPLAY_TABLE_ROW_GROUP:
    case NS_STYLE_DISPLAY_TABLE_HEADER_GROUP:
    case NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP:
      rv = ConstructTableRowGroupFrame(aState, aContent, aParentFrame,
                                       aStyleContext, aNameSpaceID, PR_FALSE,
                                       aFrameItems, newFrame,
                                       aHasPseudoParent);
      if (NS_SUCCEEDED(rv) && !aHasPseudoParent) {
        aFrameItems.AddChild(newFrame);
      }
      return rv;

    case NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP:
      rv = ConstructTableColGroupFrame(aState, aContent, aParentFrame,
                                       aStyleContext, aNameSpaceID,
                                       PR_FALSE, aFrameItems, newFrame,
                                       aHasPseudoParent);
      if (NS_SUCCEEDED(rv) && !aHasPseudoParent) {
        aFrameItems.AddChild(newFrame);
      }
      return rv;
   
    case NS_STYLE_DISPLAY_TABLE_COLUMN:
      rv = ConstructTableColFrame(aState, aContent, aParentFrame,
                                  aStyleContext, aNameSpaceID, PR_FALSE,
                                  aFrameItems, newFrame, aHasPseudoParent);
      if (NS_SUCCEEDED(rv) && !aHasPseudoParent) {
        aFrameItems.AddChild(newFrame);
      }
      return rv;
  
    case NS_STYLE_DISPLAY_TABLE_ROW:
      rv = ConstructTableRowFrame(aState, aContent, aParentFrame,
                                  aStyleContext, aNameSpaceID, PR_FALSE,
                                  aFrameItems, newFrame, aHasPseudoParent);
      if (NS_SUCCEEDED(rv) && !aHasPseudoParent) {
        aFrameItems.AddChild(newFrame);
      }
      return rv;
  
    case NS_STYLE_DISPLAY_TABLE_CELL:
      {
        nsIFrame* innerTable;
        rv = ConstructTableCellFrame(aState, aContent, aParentFrame,
                                     aStyleContext, aNameSpaceID,
                                     PR_FALSE, aFrameItems, newFrame,
                                     innerTable, aHasPseudoParent);
        if (NS_SUCCEEDED(rv) && !aHasPseudoParent) {
          aFrameItems.AddChild(newFrame);
        }
        return rv;
      }
  
    default:
      NS_NOTREACHED("How did we get here?");
      break;
    }
  }

  if (!addedToFrameList) {
    
    NS_ASSERTION(!aDisplay->IsAbsolutelyPositioned() &&
                 !aDisplay->IsFloating(),
                 "Things that could be out-of-flow need to handle adding "
                 "to the frame list themselves");
    
    rv = aState.AddChild(newFrame, aFrameItems, aDisplay, aContent,
                         aStyleContext, aParentFrame);
    NS_ASSERTION(NS_SUCCEEDED(rv),
                 "Cases where AddChild() can fail must handle it themselves");
  }

  if (newFrame && addToHashTable) {
    
    
    
    if (!primaryFrameSet) {
      aState.mFrameManager->SetPrimaryFrameFor(aContent, newFrame);
    }
  }

  return rv;
}

nsresult 
nsCSSFrameConstructor::InitAndRestoreFrame(const nsFrameConstructorState& aState,
                                           nsIContent*              aContent,
                                           nsIFrame*                aParentFrame,
                                           nsIFrame*                aPrevInFlow,
                                           nsIFrame*                aNewFrame,
                                           PRBool                   aAllowCounters)
{
  nsresult rv = NS_OK;
  
  NS_ASSERTION(aNewFrame, "Null frame cannot be initialized");
  if (!aNewFrame)
    return NS_ERROR_NULL_POINTER;

  
  rv = aNewFrame->Init(aContent, aParentFrame, aPrevInFlow);

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
  nsStyleContext* parentStyleContext;
  if (aContent->GetParent()) {
    aParentFrame = nsFrame::CorrectStyleParentFrame(aParentFrame, nsnull);
  
    
    
    parentStyleContext = aParentFrame->GetStyleContext();
  } else {
    
    
    parentStyleContext = nsnull;
  }

  nsStyleSet *styleSet = mPresShell->StyleSet();

  if (aContent->IsNodeOfType(nsINode::eELEMENT)) {
    return styleSet->ResolveStyleFor(aContent, parentStyleContext);
  } else {

    NS_ASSERTION(aContent->IsNodeOfType(nsINode::eTEXT),
                 "shouldn't waste time creating style contexts for "
                 "comments and processing instructions");

    return styleSet->ResolveStyleForNonElement(parentStyleContext);
  }
}


#ifdef MOZ_MATHML
nsresult
nsCSSFrameConstructor::ConstructMathMLFrame(nsFrameConstructorState& aState,
                                            nsIContent*              aContent,
                                            nsIFrame*                aParentFrame,
                                            nsIAtom*                 aTag,
                                            PRInt32                  aNameSpaceID,
                                            nsStyleContext*          aStyleContext,
                                            nsFrameItems&            aFrameItems,
                                            PRBool                   aHasPseudoParent)
{
  
  if (aNameSpaceID != kNameSpaceID_MathML) 
    return NS_OK;

  nsresult  rv = NS_OK;
  PRBool    ignoreInterTagWhitespace = PR_TRUE;

  NS_ASSERTION(aTag != nsnull, "null MathML tag");
  if (aTag == nsnull)
    return NS_OK;

  
  nsIFrame* newFrame = nsnull;

  
  const nsStyleDisplay* disp = aStyleContext->GetStyleDisplay();

  
  
  if (IsSpecialContent(aContent, aTag, aNameSpaceID, aStyleContext)) {
    
    if (!aHasPseudoParent && !aState.mPseudoFrames.IsEmpty()) {
      ProcessPseudoFrames(aState, aFrameItems); 
    }
  }

  if (aTag == nsGkAtoms::mi_ ||
      aTag == nsGkAtoms::mn_ ||
      aTag == nsGkAtoms::ms_ ||
      aTag == nsGkAtoms::mtext_)
    newFrame = NS_NewMathMLTokenFrame(mPresShell, aStyleContext);
  else if (aTag == nsGkAtoms::mo_)
    newFrame = NS_NewMathMLmoFrame(mPresShell, aStyleContext);
  else if (aTag == nsGkAtoms::mfrac_)
    newFrame = NS_NewMathMLmfracFrame(mPresShell, aStyleContext);
  else if (aTag == nsGkAtoms::msup_)
    newFrame = NS_NewMathMLmsupFrame(mPresShell, aStyleContext);
  else if (aTag == nsGkAtoms::msub_)
    newFrame = NS_NewMathMLmsubFrame(mPresShell, aStyleContext);
  else if (aTag == nsGkAtoms::msubsup_)
    newFrame = NS_NewMathMLmsubsupFrame(mPresShell, aStyleContext);
  else if (aTag == nsGkAtoms::munder_)
    newFrame = NS_NewMathMLmunderFrame(mPresShell, aStyleContext);
  else if (aTag == nsGkAtoms::mover_)
    newFrame = NS_NewMathMLmoverFrame(mPresShell, aStyleContext);
  else if (aTag == nsGkAtoms::munderover_)
    newFrame = NS_NewMathMLmunderoverFrame(mPresShell, aStyleContext);
  else if (aTag == nsGkAtoms::mphantom_)
    newFrame = NS_NewMathMLmphantomFrame(mPresShell, aStyleContext);
  else if (aTag == nsGkAtoms::mpadded_)
    newFrame = NS_NewMathMLmpaddedFrame(mPresShell, aStyleContext);
  else if (aTag == nsGkAtoms::mspace_ ||
           aTag == nsGkAtoms::none    ||
           aTag == nsGkAtoms::mprescripts_)
    newFrame = NS_NewMathMLmspaceFrame(mPresShell, aStyleContext);
  else if (aTag == nsGkAtoms::mfenced_)
    newFrame = NS_NewMathMLmfencedFrame(mPresShell, aStyleContext);
  else if (aTag == nsGkAtoms::mmultiscripts_)
    newFrame = NS_NewMathMLmmultiscriptsFrame(mPresShell, aStyleContext);
  else if (aTag == nsGkAtoms::mstyle_)
    newFrame = NS_NewMathMLmstyleFrame(mPresShell, aStyleContext);
  else if (aTag == nsGkAtoms::msqrt_)
    newFrame = NS_NewMathMLmsqrtFrame(mPresShell, aStyleContext);
  else if (aTag == nsGkAtoms::mroot_)
    newFrame = NS_NewMathMLmrootFrame(mPresShell, aStyleContext);
  else if (aTag == nsGkAtoms::maction_)
    newFrame = NS_NewMathMLmactionFrame(mPresShell, aStyleContext);
  else if (aTag == nsGkAtoms::mrow_ ||
           aTag == nsGkAtoms::merror_)
    newFrame = NS_NewMathMLmrowFrame(mPresShell, aStyleContext);
  
  else if (aTag == nsGkAtoms::mtable_ &&
           disp->mDisplay == NS_STYLE_DISPLAY_TABLE) {
    
    
    
    
    
    

    nsStyleContext* parentContext = aParentFrame->GetStyleContext();
    nsStyleSet *styleSet = mPresShell->StyleSet();

    
    nsRefPtr<nsStyleContext> mrowContext;
    mrowContext = styleSet->ResolvePseudoStyleFor(aContent,
                                                  nsCSSAnonBoxes::mozMathInline,
                                                  parentContext);
    newFrame = NS_NewMathMLmrowFrame(mPresShell, mrowContext);
    if (NS_UNLIKELY(!newFrame)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    InitAndRestoreFrame(aState, aContent, aParentFrame, nsnull, newFrame);

    
    nsRefPtr<nsStyleContext> blockContext;
    blockContext = styleSet->ResolvePseudoStyleFor(aContent,
                                                   nsCSSAnonBoxes::mozMathMLAnonymousBlock,
                                                   mrowContext);
    
    
    nsIFrame* blockFrame = NS_NewBlockFrame(mPresShell, blockContext,
                                            NS_BLOCK_SPACE_MGR |
                                            NS_BLOCK_MARGIN_ROOT);
    if (NS_UNLIKELY(!newFrame)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    InitAndRestoreFrame(aState, aContent, newFrame, nsnull, blockFrame);

    
    nsRefPtr<nsStyleContext> tableContext =
      ResolveStyleContext(blockFrame, aContent);

    nsFrameItems tempItems;
    nsIFrame* outerTable;
    nsIFrame* innerTable;
    
    
    
    
    
    nsPseudoFrames priorPseudoFrames; 
    aState.mPseudoFrames.Reset(&priorPseudoFrames);

    
    
    
    rv = ConstructTableFrame(aState, aContent, blockFrame, tableContext,
                             aNameSpaceID, PR_FALSE, tempItems, PR_FALSE,
                             outerTable, innerTable);
    
    

    NS_ASSERTION(aState.mPseudoFrames.IsEmpty(),
                 "How did we end up with pseudo-frames here?");

    
    
    aState.mPseudoFrames = priorPseudoFrames;
    
    
    blockFrame->SetInitialChildList(nsnull, outerTable);

    
    newFrame->SetInitialChildList(nsnull, blockFrame);

    
    
    
    
    aFrameItems.AddChild(newFrame);

    return rv; 
  }
  

  else if (aTag == nsGkAtoms::math) { 
    
    const nsStyleDisplay* display = aStyleContext->GetStyleDisplay();
    PRBool isBlock = (NS_STYLE_DISPLAY_BLOCK == display->mDisplay);
    newFrame = NS_NewMathMLmathFrame(mPresShell, isBlock, aStyleContext);
  }
  else {
    return NS_OK;
  }

  
  
  if (newFrame) {
    
    if (ignoreInterTagWhitespace) {
      newFrame->AddStateBits(NS_FRAME_EXCLUDE_IGNORABLE_WHITESPACE);
    }

    
    
    PRBool isMath = aTag == nsGkAtoms::math;

    nsIFrame* geometricParent =
      isMath ? aState.GetGeometricParent(disp, aParentFrame) : aParentFrame;
    
    InitAndRestoreFrame(aState, aContent, geometricParent, nsnull, newFrame);

    
    nsHTMLContainerFrame::CreateViewForFrame(newFrame, aParentFrame, PR_FALSE);

    rv = aState.AddChild(newFrame, aFrameItems, disp, aContent, aStyleContext,
                         aParentFrame, isMath, isMath);
    if (NS_FAILED(rv)) {
      return rv;
    }

    
    nsFrameConstructorSaveState floatSaveState;
    aState.PushFloatContainingBlock(nsnull, floatSaveState, PR_FALSE,
                                    PR_FALSE);

    
    nsFrameConstructorSaveState absoluteSaveState;
    aState.PushAbsoluteContainingBlock(nsnull, absoluteSaveState);

    
    nsFrameItems childItems;
    if (!newFrame->IsLeaf()) {
      rv = ProcessChildren(aState, aContent, newFrame, PR_TRUE,
                           childItems, PR_FALSE);
    }

    CreateAnonymousFrames(aTag, aState, aContent, newFrame, PR_FALSE,
                          childItems);

    
    newFrame->SetInitialChildList(nsnull, childItems.childList);
 
    return rv;
  }
  else {
    return NS_ERROR_OUT_OF_MEMORY;
  }
}
#endif 


#ifdef MOZ_SVG
nsresult
nsCSSFrameConstructor::TestSVGConditions(nsIContent* aContent,
                                         PRBool&     aHasRequiredExtensions,
                                         PRBool&     aHasRequiredFeatures,
                                         PRBool&     aHasSystemLanguage)
{
  nsAutoString value;

  
  if (! aContent->IsNodeOfType(nsINode::eELEMENT)) {
    aHasRequiredExtensions = PR_FALSE;
    aHasRequiredFeatures = PR_FALSE;
    aHasSystemLanguage = PR_FALSE;
    return NS_OK;
  }

  
  
  
  
  
  
  
  
  
  
  aHasRequiredExtensions = !aContent->HasAttr(kNameSpaceID_None,
                                              nsGkAtoms::requiredExtensions);

  
  aHasRequiredFeatures = PR_TRUE;
  if (aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::requiredFeatures, value)) {
    aHasRequiredFeatures = !value.IsEmpty() && NS_SVG_TestFeatures(value);
  }

  
  
  
  
  
  
  
  aHasSystemLanguage = PR_TRUE;
  if (aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::systemLanguage,
                        value)) {
    
    nsAutoString langPrefs(nsContentUtils::GetLocalizedStringPref("intl.accept_languages"));
    if (!langPrefs.IsEmpty()) {
      langPrefs.StripWhitespace();
      value.StripWhitespace();
#ifdef  DEBUG_scooter
      printf("Calling SVG_TestLanguage('%s','%s')\n", NS_ConvertUTF16toUTF8(value).get(), 
                                                      NS_ConvertUTF16toUTF8(langPrefs).get());
#endif
      aHasSystemLanguage = SVG_TestLanguage(value, langPrefs);
    } else {
      
      NS_WARNING("no default language specified for systemLanguage conditional test");
      aHasSystemLanguage = !value.IsEmpty();
    }
  }
  return NS_OK;
}

nsresult
nsCSSFrameConstructor::SVGSwitchProcessChildren(nsFrameConstructorState& aState,
                                                nsIContent*              aContent,
                                                nsIFrame*                aFrame,
                                                nsFrameItems&            aFrameItems)
{
  nsresult rv = NS_OK;
  PRBool hasRequiredExtensions = PR_FALSE;
  PRBool hasRequiredFeatures = PR_FALSE;
  PRBool hasSystemLanguage = PR_FALSE;

  
  nsPseudoFrames priorPseudoFrames;
  aState.mPseudoFrames.Reset(&priorPseudoFrames);

  
  
  
  
  
  PRInt32 childCount = aContent->GetChildCount();
  for (PRInt32 i = 0; i < childCount; ++i) {
    nsIContent* child = aContent->GetChildAt(i);

    
    if (!child->IsNodeOfType(nsINode::eELEMENT)) {
      continue;
    }

    rv = TestSVGConditions(child,
                           hasRequiredExtensions,
                           hasRequiredFeatures,
                           hasSystemLanguage);
#ifdef DEBUG_scooter
    nsAutoString str;
    child->Tag()->ToString(str);
    printf("Child tag: %s\n", NS_ConvertUTF16toUTF8(str).get());
    printf("SwitchProcessChildren: Required Extensions = %s, Required Features = %s, System Language = %s\n",
            hasRequiredExtensions ? "true" : "false",
            hasRequiredFeatures ? "true" : "false",
            hasSystemLanguage ? "true" : "false");
#endif
    if (NS_FAILED(rv))
      return rv;

    if (hasRequiredExtensions &&
        hasRequiredFeatures &&
        hasSystemLanguage) {

      rv = ConstructFrame(aState, child,
                          aFrame, aFrameItems);

      if (NS_FAILED(rv))
        return rv;

      
      break;
    }
  }

  
  if (!aState.mPseudoFrames.IsEmpty()) {
    ProcessPseudoFrames(aState, aFrameItems);
  }

  
  aState.mPseudoFrames = priorPseudoFrames;


  return rv;
}

nsresult
nsCSSFrameConstructor::ConstructSVGFrame(nsFrameConstructorState& aState,
                                         nsIContent*              aContent,
                                         nsIFrame*                aParentFrame,
                                         nsIAtom*                 aTag,
                                         PRInt32                  aNameSpaceID,
                                         nsStyleContext*          aStyleContext,
                                         nsFrameItems&            aFrameItems,
                                         PRBool                   aHasPseudoParent,
                                         PRBool*                  aHaltProcessing)
{
  NS_ASSERTION(aNameSpaceID == kNameSpaceID_SVG, "SVG frame constructed in wrong namespace");
  *aHaltProcessing = PR_FALSE;

  nsresult  rv = NS_OK;
  PRBool forceView = PR_FALSE;
  PRBool isOuterSVGNode = PR_FALSE;
  const nsStyleDisplay* disp = aStyleContext->GetStyleDisplay();
  
  NS_ASSERTION(aTag != nsnull, "null SVG tag");
  if (aTag == nsnull)
    return NS_OK;

  

  
  nsIFrame* newFrame = nsnull;
 
  
  
  nsIFrame* geometricParent = aParentFrame;

  PRBool parentIsSVG = PR_FALSE;
  if (aParentFrame && aParentFrame->GetContent()) {
    PRInt32 parentNSID;
    nsIAtom* parentTag =
      mDocument->BindingManager()->ResolveTag(aParentFrame->GetContent(),
                                              &parentNSID);

    parentIsSVG = parentNSID == kNameSpaceID_SVG
#ifdef MOZ_SVG_FOREIGNOBJECT
                  
                  
                  
                  
                  && parentTag != nsGkAtoms::foreignObject
#endif
                  ;
  }

  if ((aTag != nsGkAtoms::svg && !parentIsSVG) ||
      (aTag == nsGkAtoms::desc || aTag == nsGkAtoms::title)) {
    
    
    
    
    
    
    
    
    
    
    
    *aHaltProcessing = PR_TRUE;
    return NS_OK;
  }
  
  
  if (aParentFrame && 
      aParentFrame->GetType() == nsGkAtoms::svgSwitch &&
      aParentFrame->GetFirstChild(nsnull)) {
    *aHaltProcessing = PR_TRUE;
    return NS_OK;
  }

  
  
  if (((aContent->HasAttr(kNameSpaceID_None, nsGkAtoms::requiredFeatures) ||
        aContent->HasAttr(kNameSpaceID_None, nsGkAtoms::requiredExtensions)) &&
        NS_SVG_TestsSupported(aTag)) ||
      (aContent->HasAttr(kNameSpaceID_None, nsGkAtoms::systemLanguage) &&
       NS_SVG_LangSupported(aTag))) {

    PRBool hasRequiredExtentions = PR_FALSE;
    PRBool hasRequiredFeatures = PR_FALSE;
    PRBool hasSystemLanguage = PR_FALSE;
    TestSVGConditions(aContent, hasRequiredExtentions, 
                      hasRequiredFeatures, hasSystemLanguage);
    
    
    
    
    
    
    if (!hasRequiredExtentions || !hasRequiredFeatures ||
        !hasSystemLanguage) {
      *aHaltProcessing = PR_TRUE;
      return NS_OK;
    }
  }

  
  if (aTag == nsGkAtoms::svg) {
    if (!parentIsSVG) {
      
      isOuterSVGNode = PR_TRUE;

      
      geometricParent = aState.GetGeometricParent(disp, aParentFrame);
      
      forceView = PR_TRUE;
      newFrame = NS_NewSVGOuterSVGFrame(mPresShell, aContent, aStyleContext);
    }
    else {
      
      newFrame = NS_NewSVGInnerSVGFrame(mPresShell, aContent, aStyleContext);
    }
  }
  else if (aTag == nsGkAtoms::g ||
           aTag == nsGkAtoms::svgSwitch) {
    newFrame = NS_NewSVGGFrame(mPresShell, aContent, aStyleContext);
  }
  else if (aTag == nsGkAtoms::polygon ||
           aTag == nsGkAtoms::polyline ||
           aTag == nsGkAtoms::circle ||
           aTag == nsGkAtoms::ellipse ||
           aTag == nsGkAtoms::line ||
           aTag == nsGkAtoms::rect ||
           aTag == nsGkAtoms::path)
    newFrame = NS_NewSVGPathGeometryFrame(mPresShell, aContent, aStyleContext);
  else if (aTag == nsGkAtoms::defs) {
    newFrame = NS_NewSVGContainerFrame(mPresShell, aContent, aStyleContext);
  }
#ifdef MOZ_SVG_FOREIGNOBJECT
  else if (aTag == nsGkAtoms::foreignObject) {
    newFrame = NS_NewSVGForeignObjectFrame(mPresShell, aContent, aStyleContext);
  }
#endif
  else if (aTag == nsGkAtoms::a) {
    newFrame = NS_NewSVGAFrame(mPresShell, aContent, aStyleContext);
  }
  else if (aTag == nsGkAtoms::text) {
    newFrame = NS_NewSVGTextFrame(mPresShell, aContent, aStyleContext);
  }
  else if (aTag == nsGkAtoms::tspan) {
    nsIFrame *ancestorFrame = SVG_GetFirstNonAAncestorFrame(aParentFrame);
    if (ancestorFrame) {
      nsISVGTextContentMetrics* metrics;
      CallQueryInterface(ancestorFrame, &metrics);
      if (metrics)
        newFrame = NS_NewSVGTSpanFrame(mPresShell, aContent,
                                       ancestorFrame, aStyleContext);
    }
  }
  else if (aTag == nsGkAtoms::linearGradient) {
    newFrame = NS_NewSVGLinearGradientFrame(mPresShell, aContent, aStyleContext);
  }
  else if (aTag == nsGkAtoms::radialGradient) {
    newFrame = NS_NewSVGRadialGradientFrame(mPresShell, aContent, aStyleContext);
  }
  else if (aTag == nsGkAtoms::stop) {
    newFrame = NS_NewSVGStopFrame(mPresShell, aContent, aParentFrame, aStyleContext);
  }
  else if (aTag == nsGkAtoms::use) {
    newFrame = NS_NewSVGUseFrame(mPresShell, aContent, aStyleContext);
  }
  else if (aTag == nsGkAtoms::marker) {
    newFrame = NS_NewSVGMarkerFrame(mPresShell, aContent, aStyleContext);
  }
  else if (aTag == nsGkAtoms::image) {
    newFrame = NS_NewSVGImageFrame(mPresShell, aContent, aStyleContext);
  }
  else if (aTag == nsGkAtoms::clipPath) {
    newFrame = NS_NewSVGClipPathFrame(mPresShell, aContent, aStyleContext);
  }
  else if (aTag == nsGkAtoms::textPath) {
    nsIFrame *ancestorFrame = SVG_GetFirstNonAAncestorFrame(aParentFrame);
    if (ancestorFrame &&
        ancestorFrame->GetType() == nsGkAtoms::svgTextFrame) {
      newFrame = NS_NewSVGTextPathFrame(mPresShell, aContent,
                                        ancestorFrame, aStyleContext);
    }
  }
  else if (aTag == nsGkAtoms::filter) {
    newFrame = NS_NewSVGFilterFrame(mPresShell, aContent, aStyleContext);
  }
  else if (aTag == nsGkAtoms::pattern) {
    newFrame = NS_NewSVGPatternFrame(mPresShell, aContent, aStyleContext);
  }
  else if (aTag == nsGkAtoms::mask) {
    newFrame = NS_NewSVGMaskFrame(mPresShell, aContent, aStyleContext);
  }
  else if (aTag == nsGkAtoms::feDistantLight ||
           aTag == nsGkAtoms::fePointLight ||
           aTag == nsGkAtoms::feSpotLight ||
           aTag == nsGkAtoms::feBlend ||
           aTag == nsGkAtoms::feColorMatrix ||
           aTag == nsGkAtoms::feFuncR ||
           aTag == nsGkAtoms::feFuncG ||
           aTag == nsGkAtoms::feFuncB ||
           aTag == nsGkAtoms::feFuncA ||
           aTag == nsGkAtoms::feComposite ||
           aTag == nsGkAtoms::feConvolveMatrix ||
           aTag == nsGkAtoms::feDisplacementMap ||
           aTag == nsGkAtoms::feFlood ||
           aTag == nsGkAtoms::feGaussianBlur ||
           aTag == nsGkAtoms::feImage ||
           aTag == nsGkAtoms::feMergeNode ||
           aTag == nsGkAtoms::feMorphology ||
           aTag == nsGkAtoms::feOffset ||
           aTag == nsGkAtoms::feTile ||
           aTag == nsGkAtoms::feTurbulence) {
    
    
    newFrame = NS_NewSVGLeafFrame(mPresShell, aStyleContext);
  }

  
  if (newFrame == nsnull) {
    
    
    
    
    
    
    
    
    
#ifdef DEBUG
    
    
    
    
#endif
    newFrame = NS_NewSVGGenericContainerFrame(mPresShell, aContent, aStyleContext);
  }  
  
  
  if (newFrame != nsnull) {
    InitAndRestoreFrame(aState, aContent, geometricParent, nsnull, newFrame);
    nsHTMLContainerFrame::CreateViewForFrame(newFrame, aParentFrame, forceView);

    rv = aState.AddChild(newFrame, aFrameItems, disp, aContent, aStyleContext,
                         aParentFrame, isOuterSVGNode, isOuterSVGNode);
    if (NS_FAILED(rv)) {
      return rv;
    }

    nsFrameItems childItems;
#ifdef MOZ_SVG_FOREIGNOBJECT
    if (aTag == nsGkAtoms::foreignObject) { 
      
      
      nsRefPtr<nsStyleContext> innerPseudoStyle;
      innerPseudoStyle = mPresShell->StyleSet()->
        ResolvePseudoStyleFor(aContent,
                              nsCSSAnonBoxes::mozSVGForeignContent, aStyleContext);
    
      nsIFrame* blockFrame = NS_NewBlockFrame(mPresShell, innerPseudoStyle,
                                              NS_BLOCK_SPACE_MGR |
                                                NS_BLOCK_MARGIN_ROOT);
      if (NS_UNLIKELY(!blockFrame))
        return NS_ERROR_OUT_OF_MEMORY;
    
      
      
      nsFrameConstructorSaveState saveState;
      aState.PushFloatContainingBlock(nsnull, saveState, PR_FALSE, PR_FALSE);
      const nsStyleDisplay* disp = innerPseudoStyle->GetStyleDisplay();
      rv = ConstructBlock(aState, disp, aContent,
                          newFrame, newFrame, innerPseudoStyle,
                          &blockFrame, childItems, PR_TRUE);
      
      
      nsHTMLContainerFrame::CreateViewForFrame(blockFrame, nsnull, PR_TRUE);
    } else
#endif  
    {
      
      if (!newFrame->IsLeaf()) {
        if (aTag == nsGkAtoms::svgSwitch) {
          rv = SVGSwitchProcessChildren(aState, aContent, newFrame,
                                        childItems);
        } else {
          rv = ProcessChildren(aState, aContent, newFrame, PR_TRUE, childItems,
                               PR_FALSE);
        }

      }
      CreateAnonymousFrames(aTag, aState, aContent, newFrame,
                            PR_FALSE, childItems);
    }

    
    newFrame->SetInitialChildList(nsnull, childItems.childList);
    return rv;
  }
  else {
    return NS_ERROR_FAILURE;
  }
}
#endif 

PRBool
nsCSSFrameConstructor::PageBreakBefore(nsFrameConstructorState& aState,
                                       nsIContent*              aContent,
                                       nsIFrame*                aParentFrame,
                                       nsStyleContext*          aStyleContext,
                                       nsFrameItems&            aFrameItems)
{
  const nsStyleDisplay* display = aStyleContext->GetStyleDisplay();

  
  
  if (NS_STYLE_DISPLAY_NONE != display->mDisplay &&
      (NS_STYLE_DISPLAY_TABLE == display->mDisplay ||
       !IsTableRelated(display->mDisplay, PR_TRUE))) { 
    if (display->mBreakBefore) {
      ConstructPageBreakFrame(aState, aContent, aParentFrame, aStyleContext,
                              aFrameItems);
    }
    return display->mBreakAfter;
  }
  return PR_FALSE;
}

nsresult
nsCSSFrameConstructor::ConstructPageBreakFrame(nsFrameConstructorState& aState,
                                               nsIContent*              aContent,
                                               nsIFrame*                aParentFrame,
                                               nsStyleContext*          aStyleContext,
                                               nsFrameItems&            aFrameItems)
{
  nsRefPtr<nsStyleContext> pseudoStyle;
  pseudoStyle = mPresShell->StyleSet()->ResolvePseudoStyleFor(nsnull,
                                                              nsCSSAnonBoxes::pageBreak,
                                                              aStyleContext);
  nsIFrame* pageBreakFrame = NS_NewPageBreakFrame(mPresShell, pseudoStyle);
  if (pageBreakFrame) {
    InitAndRestoreFrame(aState, aContent, aParentFrame, nsnull, pageBreakFrame);
    aFrameItems.AddChild(pageBreakFrame);

    return NS_OK;
  }
  else {
    return NS_ERROR_OUT_OF_MEMORY;
  }
}

nsresult
nsCSSFrameConstructor::ConstructFrame(nsFrameConstructorState& aState,
                                      nsIContent*              aContent,
                                      nsIFrame*                aParentFrame,
                                      nsFrameItems&            aFrameItems)

{
  NS_PRECONDITION(nsnull != aParentFrame, "no parent frame");

  nsresult rv = NS_OK;

  
  if (!NeedFrameFor(aParentFrame, aContent)) {
    return rv;
  }

  
  if (aContent->IsNodeOfType(nsINode::eCOMMENT) ||
      aContent->IsNodeOfType(nsINode::ePROCESSING_INSTRUCTION))
    return rv;

  nsRefPtr<nsStyleContext> styleContext;
  styleContext = ResolveStyleContext(aParentFrame, aContent);

  PRBool pageBreakAfter = PR_FALSE;

  if (aState.mPresContext->IsPaginated()) {
    
    pageBreakAfter = PageBreakBefore(aState, aContent, aParentFrame,
                                     styleContext, aFrameItems);
  }

  
  rv = ConstructFrameInternal(aState, aContent, aParentFrame,
                              aContent->Tag(), aContent->GetNameSpaceID(),
                              styleContext, aFrameItems, PR_FALSE);

  if (NS_SUCCEEDED(rv) && pageBreakAfter) {
    
    ConstructPageBreakFrame(aState, aContent, aParentFrame, styleContext,
                            aFrameItems);
  }
  
  return rv;
}


nsresult
nsCSSFrameConstructor::ConstructFrameInternal( nsFrameConstructorState& aState,
                                               nsIContent*              aContent,
                                               nsIFrame*                aParentFrame,
                                               nsIAtom*                 aTag,
                                               PRInt32                  aNameSpaceID,
                                               nsStyleContext*          aStyleContext,
                                               nsFrameItems&            aFrameItems,
                                               PRBool                   aXBLBaseTag)
{
  
  
  
  const nsStyleDisplay* display = aStyleContext->GetStyleDisplay();
  nsRefPtr<nsStyleContext> styleContext(aStyleContext);
  nsAutoEnqueueBinding binding(mDocument);
  if (!aXBLBaseTag)
  {
    
    
    if (display->mBinding) {
      
      nsresult rv;
      
      PRBool resolveStyle;
      
      nsIXBLService * xblService = GetXBLService();
      if (!xblService)
        return NS_ERROR_FAILURE;

      rv = xblService->LoadBindings(aContent, display->mBinding->mURI,
                                    display->mBinding->mOriginPrincipal,
                                    PR_FALSE, getter_AddRefs(binding.mBinding),
                                    &resolveStyle);
      if (NS_FAILED(rv))
        return NS_OK;

      if (resolveStyle) {
        styleContext = ResolveStyleContext(aParentFrame, aContent);
        display = styleContext->GetStyleDisplay();
      }

      PRInt32 nameSpaceID;
      nsCOMPtr<nsIAtom> baseTag =
        mDocument->BindingManager()->ResolveTag(aContent, &nameSpaceID);

      if (baseTag != aTag || aNameSpaceID != nameSpaceID) {
        
        rv = ConstructFrameInternal(aState,
                                    aContent,
                                    aParentFrame,
                                    baseTag,
                                    nameSpaceID,
                                    styleContext,
                                    aFrameItems,
                                    PR_TRUE);
        return rv;
      }
    }
  }

  
  
  if (NS_STYLE_DISPLAY_NONE == display->mDisplay) {
    aState.mFrameManager->SetUndisplayedContent(aContent, styleContext);
    return NS_OK;
  }

  nsIFrame* adjParentFrame = aParentFrame;
  nsFrameItems* frameItems = &aFrameItems;
  PRBool pseudoParent = PR_FALSE;
  PRBool suppressFrame = PR_FALSE;
  nsFrameConstructorSaveState pseudoSaveState;
  nsresult rv = AdjustParentFrame(aState, aContent, adjParentFrame,
                                  aTag, aNameSpaceID, styleContext,
                                  frameItems, pseudoSaveState,
                                  suppressFrame, pseudoParent);
  if (NS_FAILED(rv) || suppressFrame) {
    return rv;
  }

  if (aContent->IsNodeOfType(nsINode::eTEXT)) 
    return ConstructTextFrame(aState, aContent, adjParentFrame, styleContext,
                              *frameItems, pseudoParent);

#ifdef MOZ_SVG
  
  if (aNameSpaceID != kNameSpaceID_SVG &&
      aParentFrame &&
      aParentFrame->IsFrameOfType(nsIFrame::eSVG)
#ifdef MOZ_SVG_FOREIGNOBJECT
      && !aParentFrame->IsFrameOfType(nsIFrame::eSVGForeignObject)
#endif
      ) {
    return NS_OK;
  }
#endif

  
  
  
  
  {
    styleContext->GetStyleVisibility();
  }
  
  
  {
    styleContext->GetStyleBackground();
  }

  nsIFrame* lastChild = frameItems->lastChild;

  
  rv = ConstructHTMLFrame(aState, aContent, adjParentFrame, aTag, aNameSpaceID,
                          styleContext, *frameItems, pseudoParent);

  
  
  
  if (NS_SUCCEEDED(rv) &&
      (!frameItems->childList || lastChild == frameItems->lastChild)) {
    PRBool haltProcessing;

    rv = ConstructXULFrame(aState, aContent, adjParentFrame, aTag,
                           aNameSpaceID, styleContext,
                           *frameItems, aXBLBaseTag, pseudoParent,
                           &haltProcessing);

    if (haltProcessing) {
      return rv;
    }
  } 


#ifdef MOZ_MATHML
  if (NS_SUCCEEDED(rv) &&
      (!frameItems->childList || lastChild == frameItems->lastChild)) {
    rv = ConstructMathMLFrame(aState, aContent, adjParentFrame, aTag,
                              aNameSpaceID, styleContext, *frameItems,
                              pseudoParent);
  }
#endif


#ifdef MOZ_SVG
  if (NS_SUCCEEDED(rv) &&
      (!frameItems->childList || lastChild == frameItems->lastChild) &&
      aNameSpaceID == kNameSpaceID_SVG &&
      NS_SVGEnabled()) {
    PRBool haltProcessing;
    rv = ConstructSVGFrame(aState, aContent, adjParentFrame, aTag,
                           aNameSpaceID, styleContext,
                           *frameItems, pseudoParent, &haltProcessing);
    if (haltProcessing) {
      return rv;
    }
  }
#endif

  if (NS_SUCCEEDED(rv) &&
      (!frameItems->childList || lastChild == frameItems->lastChild)) {
    
    
    rv = ConstructFrameByDisplayType(aState, display, aContent, aNameSpaceID,
                                     aTag, adjParentFrame, styleContext,
                                     *frameItems, pseudoParent);
  }

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
      
      
      CaptureStateForFramesOf(rootContent, mTempFrameTreeState);

      nsFrameConstructorState state(mPresShell, mFixedContainingBlock,
                                    nsnull, nsnull, mTempFrameTreeState);

      
      nsIFrame* docElementFrame =
        state.mFrameManager->GetPrimaryFrameFor(rootContent, -1);
        
      
      
      
      
      rv = RemoveFixedItems(state);
      if (NS_SUCCEEDED(rv)) {
        
        
        state.mFrameManager->ClearPrimaryFrameMap();
        state.mFrameManager->ClearPlaceholderFrameMap();
        state.mFrameManager->ClearUndisplayedContentMap();

        if (docElementFrame) {
          
        
          

          NS_ASSERTION(docElementFrame->GetParent() == mDocElementContainingBlock,
                       "Unexpected doc element parent frame");

          
          rv = state.mFrameManager->RemoveFrame(mDocElementContainingBlock,
                                                nsnull, docElementFrame);
          if (NS_FAILED(rv)) {
            return rv;
          }

        }

        
        nsIFrame*                 newChild;
        rv = ConstructDocElementFrame(state, rootContent,
                                      mDocElementContainingBlock, &newChild);

        
        if (NS_SUCCEEDED(rv) && newChild) {
          rv = state.mFrameManager->InsertFrames(mDocElementContainingBlock,
                                                 nsnull, nsnull, newChild);
        }
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
  NS_PRECONDITION(nsnull != mInitialContainingBlock, "no initial containing block");
  
  
  
  nsIFrame* containingBlock = nsnull;
  for (nsIFrame* frame = aFrame; frame && !containingBlock;
       frame = frame->GetParent()) {
    if (frame->IsFrameOfType(nsIFrame::eMathML)) {
      
      
      
      
      return nsnull;
    }
    
    
    
    
    
    const nsStyleDisplay* disp = frame->GetStyleDisplay();

    if (disp->IsPositioned() && !IsTableRelated(disp->mDisplay, PR_TRUE)) {
      
      for (nsIFrame* wrappedFrame = aFrame; wrappedFrame != frame->GetParent();
           wrappedFrame = wrappedFrame->GetParent()) {
        nsIAtom* frameType = wrappedFrame->GetType();
        if (nsGkAtoms::areaFrame == frameType ||
            nsGkAtoms::blockFrame == frameType ||
            nsGkAtoms::positionedInlineFrame == frameType) {
          containingBlock = wrappedFrame;
        } else if (nsGkAtoms::fieldSetFrame == frameType) {
          
          
          containingBlock = GetFieldSetAreaFrame(wrappedFrame);
        }
      }

#ifdef DEBUG
      if (!containingBlock)
        NS_WARNING("Positioned frame that does not handle positioned kids; looking further up the parent chain");
#endif
    }
  }

  
  if (containingBlock)
    return AdjustAbsoluteContainingBlock(mPresShell->GetPresContext(),
                                         containingBlock);

  
  
  return mInitialContainingBlockIsAbsPosContainer ? mInitialContainingBlock : nsnull;
}

nsIFrame*
nsCSSFrameConstructor::GetFloatContainingBlock(nsIFrame* aFrame)
{
  NS_PRECONDITION(mInitialContainingBlock, "no initial containing block");
  
  
  
  
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
nsCSSFrameConstructor::AppendFrames(const nsFrameConstructorState& aState,
                                    nsIContent*                    aContainer,
                                    nsIFrame*                      aParentFrame,
                                    nsIFrame*                      aFrameList,
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
    nsFrameList frames(aParentFrame->GetFirstChild(nsnull));

    
    return frameManager->InsertFrames(aParentFrame, nsnull,
                                      frames.GetPrevSiblingFor(aAfterFrame),
                                      aFrameList);
  }

  return frameManager->AppendFrames(aParentFrame, nsnull, aFrameList);
}





static nsIFrame*
FindPreviousAnonymousSibling(nsIPresShell* aPresShell,
                             nsIDocument*  aDocument,
                             nsIContent*   aContainer,
                             nsIContent*   aChild)
{
  NS_PRECONDITION(aDocument, "null document from content element in FindNextAnonymousSibling");

  nsCOMPtr<nsIDOMDocumentXBL> xblDoc(do_QueryInterface(aDocument));
  NS_ASSERTION(xblDoc, "null xblDoc for content element in FindNextAnonymousSibling");
  if (! xblDoc)
    return nsnull;

  
  
  nsCOMPtr<nsIDOMNodeList> nodeList;
  nsCOMPtr<nsIDOMElement> elt(do_QueryInterface(aContainer));
  xblDoc->GetAnonymousNodes(elt, getter_AddRefs(nodeList));

  if (! nodeList)
    return nsnull;

  PRUint32 length;
  nodeList->GetLength(&length);

  PRInt32 index;
  for (index = PRInt32(length) - 1; index >= 0; --index) {
    nsCOMPtr<nsIDOMNode> node;
    nodeList->Item(PRUint32(index), getter_AddRefs(node));

    nsCOMPtr<nsIContent> child = do_QueryInterface(node);
    if (child == aChild)
      break;
  }

  
  
  while (--index >= 0) {
    nsCOMPtr<nsIDOMNode> node;
    nodeList->Item(PRUint32(index), getter_AddRefs(node));

    nsCOMPtr<nsIContent> child = do_QueryInterface(node);

    
    
    nsIFrame* prevSibling = aPresShell->GetPrimaryFrameFor(child);
    if (prevSibling) {
      
      
      if (IsFrameSpecial(prevSibling)) {
        prevSibling = GetLastSpecialSibling(aPresShell->FrameManager(),
                                            prevSibling);
      }

      
      
      prevSibling = prevSibling->GetLastContinuation();

      
      
      if (prevSibling->GetStateBits() & NS_FRAME_OUT_OF_FLOW) {
        nsIFrame *placeholderFrame;
        aPresShell->GetPlaceholderFrameFor(prevSibling, &placeholderFrame);
        NS_ASSERTION(placeholderFrame, "no placeholder for out-of-flow frame");
        prevSibling = placeholderFrame;
      }

      
      return prevSibling;
    }
  }

  return nsnull;
}





static nsIFrame*
FindNextAnonymousSibling(nsIPresShell* aPresShell,
                         nsIDocument*  aDocument,
                         nsIContent*   aContainer,
                         nsIContent*   aChild)
{
  NS_PRECONDITION(aDocument, "null document from content element in FindNextAnonymousSibling");

  nsCOMPtr<nsIDOMDocumentXBL> xblDoc(do_QueryInterface(aDocument));
  NS_ASSERTION(xblDoc, "null xblDoc for content element in FindNextAnonymousSibling");
  if (! xblDoc)
    return nsnull;

  
  nsCOMPtr<nsIDOMNodeList> nodeList;
  nsCOMPtr<nsIDOMElement> elt(do_QueryInterface(aContainer));
  xblDoc->GetAnonymousNodes(elt, getter_AddRefs(nodeList));

  if (! nodeList)
    return nsnull;

  PRUint32 length;
  nodeList->GetLength(&length);

  PRInt32 index;
  for (index = 0; index < PRInt32(length); ++index) {
    nsCOMPtr<nsIDOMNode> node;
    nodeList->Item(PRUint32(index), getter_AddRefs(node));

    nsCOMPtr<nsIContent> child = do_QueryInterface(node);
    if (child == aChild)
      break;
  }

  
  
  while (++index < PRInt32(length)) {
    nsCOMPtr<nsIDOMNode> node;
    nodeList->Item(PRUint32(index), getter_AddRefs(node));

    nsCOMPtr<nsIContent> child = do_QueryInterface(node);

    
    nsIFrame* nextSibling = aPresShell->GetPrimaryFrameFor(child);
    if (nextSibling) {
      
      NS_ASSERTION(!nextSibling->GetPrevInFlow(),
                   "primary frame is a continuation!?");

      
      
      if (nextSibling->GetStateBits() & NS_FRAME_OUT_OF_FLOW) {
        nsIFrame* placeholderFrame;
        aPresShell->GetPlaceholderFrameFor(nextSibling, &placeholderFrame);
        NS_ASSERTION(placeholderFrame, "no placeholder for out-of-flow frame");
        nextSibling = placeholderFrame;
      }

      
      return nextSibling;
    }
  }

  return nsnull;
}

#define UNSET_DISPLAY 255





PRBool
nsCSSFrameConstructor::IsValidSibling(nsIFrame*              aParentFrame,
                                      nsIFrame*              aSibling,
                                      PRUint8                aSiblingDisplay,
                                      nsIContent&            aContent,
                                      PRUint8&               aDisplay)
{
  if ((NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP == aSiblingDisplay) ||
      (NS_STYLE_DISPLAY_TABLE_COLUMN       == aSiblingDisplay) ||
      (NS_STYLE_DISPLAY_TABLE_CAPTION      == aSiblingDisplay) ||
      (NS_STYLE_DISPLAY_TABLE_HEADER_GROUP == aSiblingDisplay) ||
      (NS_STYLE_DISPLAY_TABLE_ROW_GROUP    == aSiblingDisplay) ||
      (NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP == aSiblingDisplay)) {
    
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
      styleContext = ResolveStyleContext(styleParent, &aContent);
      if (!styleContext) return PR_FALSE;
      const nsStyleDisplay* display = styleContext->GetStyleDisplay();
      aDisplay = display->mDisplay;
    }
    switch (aSiblingDisplay) {
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
  else if (nsGkAtoms::fieldSetFrame == aParentFrame->GetType()) {
    
    nsIAtom* sibType = aSibling->GetType();
    nsCOMPtr<nsIDOMHTMLLegendElement> legendContent(do_QueryInterface(&aContent));

    if ((legendContent  && (nsGkAtoms::legendFrame != sibType)) ||
        (!legendContent && (nsGkAtoms::legendFrame == sibType)))
      return PR_FALSE;
  }

  return PR_TRUE;
}





nsIFrame*
nsCSSFrameConstructor::FindPreviousSibling(nsIContent*       aContainer,
                                           nsIFrame*         aContainerFrame,
                                           PRInt32           aIndexInContainer,
                                           const nsIContent* aChild)
{
  NS_ASSERTION(aContainer, "null argument");

  ChildIterator first, iter;
  nsresult rv = ChildIterator::Init(aContainer, &first, &iter);
  NS_ENSURE_SUCCESS(rv, nsnull);
  iter.seek(aIndexInContainer);

  PRUint8 childDisplay = UNSET_DISPLAY;
  
  
  while (iter-- != first) {
    nsIFrame* prevSibling = mPresShell->GetPrimaryFrameFor(nsCOMPtr<nsIContent>(*iter));

    if (prevSibling) {
      
      
      if (IsFrameSpecial(prevSibling)) {
        prevSibling = GetLastSpecialSibling(mPresShell->FrameManager(),
                                            prevSibling);
      }

      
      prevSibling = prevSibling->GetLastContinuation();

      
      
      const nsStyleDisplay* display = prevSibling->GetStyleDisplay();
  
      if (aChild && !IsValidSibling(aContainerFrame, prevSibling, 
                                    display->mDisplay, (nsIContent&)*aChild,
                                    childDisplay))
        continue;

      
      
      if (prevSibling->GetStateBits() & NS_FRAME_OUT_OF_FLOW) {
        nsIFrame* placeholderFrame;
        mPresShell->GetPlaceholderFrameFor(prevSibling, &placeholderFrame);
        NS_ASSERTION(placeholderFrame, "no placeholder for out-of-flow frame");
        prevSibling = placeholderFrame;
      }

#ifdef DEBUG
      nsIFrame* containerFrame = nsnull;
      containerFrame = mPresShell->GetPrimaryFrameFor(aContainer);
      NS_ASSERTION(prevSibling != containerFrame, "Previous Sibling is the Container's frame");
#endif
      
      return prevSibling;
    }
  }

  return nsnull;
}





nsIFrame*
nsCSSFrameConstructor::FindNextSibling(nsIContent*       aContainer,
                                       nsIFrame*         aContainerFrame,
                                       PRInt32           aIndexInContainer,
                                       const nsIContent* aChild)
{
  ChildIterator iter, last;
  nsresult rv = ChildIterator::Init(aContainer, &iter, &last);
  NS_ENSURE_SUCCESS(rv, nsnull);
  iter.seek(aIndexInContainer);

  
  if (iter == last)
    return nsnull;

  PRUint8 childDisplay = UNSET_DISPLAY;

  while (++iter != last) {
    nsIFrame* nextSibling =
      mPresShell->GetPrimaryFrameFor(nsCOMPtr<nsIContent>(*iter));

    if (nextSibling) {
      
      NS_ASSERTION(!nextSibling->GetPrevInFlow(),
                   "primary frame is a continuation!?");

      
      
      const nsStyleDisplay* display = nextSibling->GetStyleDisplay();

      if (aChild && !IsValidSibling(aContainerFrame, nextSibling, 
                                    display->mDisplay, (nsIContent&)*aChild,
                                    childDisplay))
        continue;

      
      
      if (nextSibling->GetStateBits() & NS_FRAME_OUT_OF_FLOW) {
        
        nsIFrame* placeholderFrame;
        mPresShell->GetPlaceholderFrameFor(nextSibling, &placeholderFrame);
        NS_ASSERTION(placeholderFrame, "no placeholder for out-of-flow frame");
        nextSibling = placeholderFrame;
      }

      
      return nextSibling;
    }
  }

  return nsnull;
}

inline PRBool
ShouldIgnoreSelectChild(nsIContent* aContainer)
{
  
  nsIAtom *containerTag = aContainer->Tag();

  if (containerTag == nsGkAtoms::optgroup ||
      containerTag == nsGkAtoms::select) {
    nsIContent* selectContent = aContainer;

    while (containerTag != nsGkAtoms::select) {
      selectContent = selectContent->GetParent();
      if (!selectContent) {
        break;
      }
      containerTag = selectContent->Tag();
    }

    nsCOMPtr<nsISelectElement> selectElement = do_QueryInterface(selectContent);
    if (selectElement) {
      nsAutoString selSize;
      aContainer->GetAttr(kNameSpaceID_None, nsGkAtoms::size, selSize);
      if (!selSize.IsEmpty()) {
        PRInt32 err;
        return (selSize.ToInteger(&err) > 1);
      }
    }
  }

  return PR_FALSE;
}


static nsIFrame*
GetAdjustedParentFrame(nsIFrame*       aParentFrame,
                       nsIAtom*        aParentFrameType,
                       nsIContent*     aParentContent,
                       PRInt32         aChildIndex)
{
  NS_PRECONDITION(nsGkAtoms::tableOuterFrame != aParentFrameType,
                  "Shouldn't be happening!");
  
  nsIContent *childContent = aParentContent->GetChildAt(aChildIndex);
  nsIFrame* newParent = nsnull;

  if (nsGkAtoms::fieldSetFrame == aParentFrameType) {
    
    
    nsCOMPtr<nsIDOMHTMLLegendElement> legendContent(do_QueryInterface(childContent));
    if (!legendContent) {
      newParent = GetFieldSetAreaFrame(aParentFrame);
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
        tag == nsGkAtoms::treerow ||
        (namespaceID == kNameSpaceID_XUL && gUseXBLForms &&
         ShouldIgnoreSelectChild(aContainer)))
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
      
      
      
      nsIContent* insertionContent = insertionPoint->GetContent();
      
      PRUint32 containerCount = aContainer->GetChildCount();
      for (PRUint32 i = aNewIndexInContainer; i < containerCount; i++) {
        nsIContent *child = aContainer->GetChildAt(i);
        if (multiple) {
          
          
          GetInsertionPoint(parentFrame, child, &insertionPoint);
          if (!insertionPoint) {
            
            
            continue;
          }
          insertionContent = insertionPoint->GetContent();
        }

        
        ChildIterator iter, last;
        for (ChildIterator::Init(insertionContent, &iter, &last);
         iter != last;
         ++iter) {
          LAYOUT_PHASE_TEMP_EXIT();
          nsIContent* item = nsCOMPtr<nsIContent>(*iter);
          if (item == child)
            
            ContentInserted(aContainer, child,
                            iter.index(), mTempFrameTreeState, PR_FALSE);
          LAYOUT_PHASE_TEMP_REENTER();
        }
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
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  if (IsFrameSpecial(parentFrame)) {
#ifdef DEBUG
    if (gNoisyContentUpdates) {
      printf("nsCSSFrameConstructor::ContentAppended: parentFrame=");
      nsFrame::ListTag(stdout, parentFrame);
      printf(" is special\n");
    }
#endif

    
    
    nsFrameManager *frameManager = mPresShell->FrameManager();

    while (1) {
      nsIFrame* sibling;
      GetSpecialSibling(frameManager, parentFrame, &sibling);
      if (! sibling)
        break;

      parentFrame = sibling;
    }

    
    
    const nsStyleDisplay* display = parentFrame->GetStyleDisplay();

    if (NS_STYLE_DISPLAY_BLOCK != display->mDisplay) {
      
      
      
      nsIContent *child = aContainer->GetChildAt(aNewIndexInContainer);
      PRBool needReframe = !child;
      if (child && child->IsNodeOfType(nsINode::eELEMENT)) {
        nsRefPtr<nsStyleContext> styleContext;
        styleContext = ResolveStyleContext(parentFrame, child);
        
        
        needReframe = styleContext->GetStyleDisplay()->IsBlockOutside();
      }
      if (needReframe)
        return ReframeContainingBlock(parentFrame);
    }
  }

  
  parentFrame = parentFrame->GetLastContinuation();

  nsIAtom* frameType = parentFrame->GetType();
  
  parentFrame = ::GetAdjustedParentFrame(parentFrame, frameType,
                                         aContainer, aNewIndexInContainer);

  
  nsIFrame* parentAfterFrame;
  parentFrame =
    ::AdjustAppendParentForAfterContent(mPresShell->GetPresContext(),
                                        aContainer, parentFrame,
                                        &parentAfterFrame);
  
  
  PRUint32                count;
  nsIFrame*               firstAppendedFrame = nsnull;
  nsFrameItems            frameItems;
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

  
  
  nsFrameItems captionItems;

  
  nsIFrame* oldNewFrame = nsnull;

  PRUint32 i;
  count = aContainer->GetChildCount();
  for (i = aNewIndexInContainer; i < count; i++) {
    nsIFrame* newFrame = nsnull;
    nsIContent *childContent = aContainer->GetChildAt(i);
    
    
    if (nsGkAtoms::tableFrame == frameType) {
      nsFrameItems tempItems;
      ConstructFrame(state, childContent, parentFrame, tempItems);
      if (tempItems.childList) {
        if (nsGkAtoms::tableCaptionFrame == tempItems.childList->GetType()) {
          captionItems.AddChild(tempItems.childList);
        }
        else {
          frameItems.AddChild(tempItems.childList);
        }
        newFrame = tempItems.childList;
      }
    }
    else {
      
      ConstructFrame(state, childContent, parentFrame, frameItems);
      newFrame = frameItems.lastChild;
    }

    if (newFrame && newFrame != oldNewFrame) {
      InvalidateCanvasIfNeeded(newFrame);
      oldNewFrame = newFrame;
    }
  }

  
  if (!state.mPseudoFrames.IsEmpty()) {
    ProcessPseudoFrames(state, frameItems);
  }

  if (haveFirstLineStyle && parentFrame == containingBlock) {
    
    
    AppendFirstLineFrames(state, containingBlock->GetContent(),
                          containingBlock, frameItems); 
  }

  nsresult result = NS_OK;
  firstAppendedFrame = frameItems.childList;
  if (!firstAppendedFrame) {
    firstAppendedFrame = captionItems.childList;
  }

  
  if (NS_SUCCEEDED(result) && firstAppendedFrame) {
    
    

    if (WipeContainingBlock(state, containingBlock, parentFrame,
                            frameItems.childList)) {
      return NS_OK;
    }

    
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
        AppendFrames(state, aContainer, parentFrame, frameItems.childList,
                     parentAfterFrame);
      }
    }
    else {
      AppendFrames(state, aContainer, parentFrame, firstAppendedFrame,
                   parentAfterFrame);
    }
  }

  
  if (haveFirstLetterStyle) {
    RecoverLetterFrames(state, containingBlock);
  }

#ifdef DEBUG
  if (gReallyNoisyContentUpdates) {
    nsIFrameDebug* fdbg = nsnull;
    CallQueryInterface(parentFrame, &fdbg);
    if (fdbg) {
      printf("nsCSSFrameConstructor::ContentAppended: resulting frame model:\n");
      fdbg->List(stdout, 0);
    }
  }
#endif

  return NS_OK;
}






PRBool
nsCSSFrameConstructor::NeedSpecialFrameReframe(nsIContent*     aParent1,     
                                               nsIContent*     aParent2,     
                                               nsIFrame*&      aParentFrame, 
                                               nsIContent*     aChild,
                                               PRInt32         aIndexInContainer,
                                               nsIFrame*&      aPrevSibling,
                                               nsIFrame*       aNextSibling)
{
  
  
  if (IsBlockOutside(aParentFrame)) 
    return PR_FALSE;

  
  PRBool childIsBlock = PR_FALSE;
  if (aChild->IsNodeOfType(nsINode::eELEMENT)) {
    nsRefPtr<nsStyleContext> styleContext;
    styleContext = ResolveStyleContext(aParentFrame, aChild);
    const nsStyleDisplay* display = styleContext->GetStyleDisplay();
    childIsBlock = display->IsBlockOutside();
  }
  nsIFrame* prevParent; 
  nsIFrame* nextParent; 

  if (childIsBlock) { 
    if (aPrevSibling) {
      prevParent = aPrevSibling->GetParent(); 
      NS_ASSERTION(prevParent, "program error - null parent frame");
      if (!IsBlockOutside(prevParent)) { 
        
        
        
        
        return PR_TRUE; 
      }        
      aParentFrame = prevParent; 
    }
    else {
      
      
      
      nsIFrame* nextSibling = (aIndexInContainer >= 0)
                              ? FindNextSibling(aParent2, aParentFrame,
                                                aIndexInContainer)
                              : FindNextAnonymousSibling(mPresShell, mDocument,
                                                         aParent1, aChild);
      if (nextSibling) {
        nextParent = nextSibling->GetParent(); 
        NS_ASSERTION(nextParent, "program error - null parent frame");
        if (!IsBlockOutside(nextParent)) {
          
          
          return PR_TRUE; 
        }
        
        aParentFrame = nextParent;
      }
    }           
  }
  else { 
    if (aPrevSibling) {
      prevParent = aPrevSibling->GetParent(); 
      NS_ASSERTION(prevParent, "program error - null parent frame");
      if (!IsBlockOutside(prevParent)) { 
        
        aParentFrame = aPrevSibling->GetParent();
        NS_ASSERTION(aParentFrame, "program error - null parent frame");
      }
      else { 
        
        
        
        nsIFrame* nextSibling = (aIndexInContainer >= 0)
                                ? FindNextSibling(aParent2, aParentFrame,
                                                  aIndexInContainer)
                                : FindNextAnonymousSibling(mPresShell,
                                                           mDocument, aParent1,
                                                           aChild);
        if (nextSibling) {
          nextParent = nextSibling->GetParent();
          NS_ASSERTION(nextParent, "program error - null parent frame");
          if (!IsBlockOutside(nextParent)) {
            
            
            aParentFrame = nextSibling->GetParent(); 
            NS_ASSERTION(aParentFrame, "program error - null parent frame");
            aPrevSibling = nsnull; 
          }
          else { 
            
            NS_ASSERTION(prevParent == nextParent, "special frame error");
            aParentFrame = prevParent;
          }
        }
        else {
          
          
          
          
          
          return PR_TRUE;
        }
      }
    }
    
  }
  return PR_FALSE;
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
                         PRBool             aUseXBLForms,
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
      nsIListBoxObject* listboxBody = listBoxObject->GetListBoxBody();
      if (listboxBody) {
        nsListBoxBodyFrame *listBoxBodyFrame = static_cast<nsListBoxBodyFrame*>(listboxBody);
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

  PRInt32 namespaceID;
  nsIAtom* tag =
    aDocument->BindingManager()->ResolveTag(aContainer, &namespaceID);

  
  if (aContainer->GetParent() &&
      (tag == nsGkAtoms::treechildren ||
       tag == nsGkAtoms::treeitem ||
       tag == nsGkAtoms::treerow ||
       (namespaceID == kNameSpaceID_XUL && aUseXBLForms &&
        ShouldIgnoreSelectChild(aContainer))))
    return PR_TRUE;

  return PR_FALSE;
}
#endif 

nsresult
nsCSSFrameConstructor::ContentInserted(nsIContent*            aContainer,
                                       nsIContent*            aChild,
                                       PRInt32                aIndexInContainer,
                                       nsILayoutHistoryState* aFrameState,
                                       PRBool                 aInReinsertContent)
{
  AUTO_LAYOUT_PHASE_ENTRY_POINT(mPresShell->GetPresContext(), FrameC);
  
  
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
                        mDocument, nsnull, gUseXBLForms, CONTENT_INSERTED))
    return NS_OK;
#endif 
  
  
  
  if (! aContainer) {
    nsIContent *docElement = mDocument->GetRootContent();

    if (aChild == docElement) {
      NS_PRECONDITION(nsnull == mInitialContainingBlock, "initial containing block already created");
      
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
          nsIFrameDebug* fdbg = nsnull;
          CallQueryInterface(docElementFrame, &fdbg);
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

  
  
  
  
  
  
  
  nsIFrame* prevSibling = (aIndexInContainer >= 0)
    ? FindPreviousSibling(container, parentFrame, aIndexInContainer, aChild)
    : FindPreviousAnonymousSibling(mPresShell, mDocument, aContainer, aChild);

  PRBool    isAppend = PR_FALSE;
  nsIFrame* appendAfterFrame;  
  nsIFrame* nextSibling = nsnull;
    
  
  if (! prevSibling) {
    nextSibling = (aIndexInContainer >= 0)
      ? FindNextSibling(container, parentFrame, aIndexInContainer, aChild)
      : FindNextAnonymousSibling(mPresShell, mDocument, aContainer, aChild);
  }

  PRBool handleSpecialFrame = IsFrameSpecial(parentFrame) && !aInReinsertContent;

  
  
  
  if (prevSibling) {
    if (!handleSpecialFrame)
      parentFrame = prevSibling->GetParent()->GetContentInsertionFrame();
  }
  else if (nextSibling) {
    if (!handleSpecialFrame)
      parentFrame = nextSibling->GetParent()->GetContentInsertionFrame();
  }
  else {
    
    isAppend = PR_TRUE;
    
    parentFrame = ::GetAdjustedParentFrame(parentFrame, parentFrame->GetType(),
                                           aContainer, aIndexInContainer);
    parentFrame =
      ::AdjustAppendParentForAfterContent(mPresShell->GetPresContext(),
                                          aContainer, parentFrame,
                                          &appendAfterFrame);
  }

  if (parentFrame->GetType() == nsGkAtoms::frameSetFrame &&
      IsSpecialFramesetChild(aChild)) {
    
    return RecreateFramesForContent(parentFrame->GetContent());
  }
  
  
  if (parentFrame->IsLeaf()) {
    return NS_OK;
  }
  
  
  
  if (handleSpecialFrame) {
    
    
#ifdef DEBUG
    if (gNoisyContentUpdates) {
      printf("nsCSSFrameConstructor::ContentInserted: parentFrame=");
      nsFrame::ListTag(stdout, parentFrame);
      printf(" is special\n");
    }
#endif
    
    if (NeedSpecialFrameReframe(aContainer, container, parentFrame, 
                                aChild, aIndexInContainer, prevSibling,
                                nextSibling)) {
      return ReframeContainingBlock(parentFrame);
    }
  }

  nsFrameConstructorState state(mPresShell, mFixedContainingBlock,
                                GetAbsoluteContainingBlock(parentFrame),
                                GetFloatContainingBlock(parentFrame),
                                aFrameState);


  
  
  
  
  
  nsIFrame* containingBlock = state.mFloatedItems.containingBlock;
  nsStyleContext* blockSC;
  nsIContent* blockContent = nsnull;
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

      
      
      
      prevSibling = (aIndexInContainer >= 0)
        ? FindPreviousSibling(container, parentFrame, aIndexInContainer,
                              aChild)
        : FindPreviousAnonymousSibling(mPresShell, mDocument, aContainer,
                                       aChild);

      
      if (! prevSibling) {
        nextSibling = (aIndexInContainer >= 0)
          ? FindNextSibling(container, parentFrame, aIndexInContainer, aChild)
          : FindNextAnonymousSibling(mPresShell, mDocument, aContainer, aChild);
      }

      handleSpecialFrame = IsFrameSpecial(parentFrame) && !aInReinsertContent;
      if (handleSpecialFrame &&
          NeedSpecialFrameReframe(aContainer, container, parentFrame,
                                  aChild, aIndexInContainer, prevSibling,
                                  nextSibling)) {
#ifdef DEBUG
        nsIContent* parentContainer = blockContent->GetParent();
        if (gNoisyContentUpdates) {
          printf("nsCSSFrameConstructor::ContentInserted: parentFrame=");
          nsFrame::ListTag(stdout, parentFrame);
          printf(" is special inline\n");
          printf("  ==> blockContent=%p, parentContainer=%p\n",
                 static_cast<void*>(blockContent),
                 static_cast<void*>(parentContainer));
        }
#endif

        NS_ASSERTION(GetFloatContainingBlock(parentFrame) == containingBlock,
                     "Unexpected block ancestor for parentFrame");

        
        
        
        
        return ReframeContainingBlock(parentFrame);
      }
    }
  }

  
  
  
  nsFrameItems frameItems, captionItems;

  ConstructFrame(state, aChild, parentFrame, frameItems);
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
    prevSibling = nsnull;
    isAppend = PR_TRUE;
    parentFrame =
      ::AdjustAppendParentForAfterContent(mPresShell->GetPresContext(),
                                          aContainer,
                                          frameItems.childList->GetParent(),
                                          &appendAfterFrame);
  }

  
  
  if (WipeContainingBlock(state, containingBlock, parentFrame,
                          frameItems.childList))
    return NS_OK;

  if (haveFirstLineStyle && parentFrame == containingBlock) {
    
    
    if (isAppend) {
      
      AppendFirstLineFrames(state, containingBlock->GetContent(),
                            containingBlock, frameItems); 
    }
    else {
      
      InsertFirstLineFrames(state, aContainer, containingBlock, &parentFrame,
                            prevSibling, frameItems);
    }
  }
      
  nsIFrame* newFrame = frameItems.childList;
  if (NS_SUCCEEDED(rv) && newFrame) {
    NS_ASSERTION(!captionItems.childList, "leaking caption frames");
    if (!prevSibling) {
      
      
      nsIFrame* firstChild = parentFrame->GetFirstChild(nsnull);

      if (firstChild &&
          nsLayoutUtils::IsGeneratedContentFor(aContainer, firstChild,
                                               nsCSSPseudoElements::before)) {
        
        prevSibling = firstChild->GetLastContinuation();
        nsIFrame* newParent = prevSibling->GetParent();
        if (newParent != parentFrame) {
          nsHTMLContainerFrame::ReparentFrameViewList(state.mPresContext,
                                                      newFrame, parentFrame,
                                                      newParent);
          parentFrame = newParent;
        }
        
        
        
        
        isAppend = PR_FALSE;
      }
    }

    
    if (isAppend) {
      AppendFrames(state, aContainer, parentFrame, newFrame, appendAfterFrame);
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
    
    
    RecoverLetterFrames(state, state.mFloatedItems.containingBlock);
  }

#ifdef DEBUG
  if (gReallyNoisyContentUpdates && parentFrame) {
    nsIFrameDebug* fdbg = nsnull;
    CallQueryInterface(parentFrame, &fdbg);
    if (fdbg) {
      printf("nsCSSFrameConstructor::ContentInserted: resulting frame model:\n");
      fdbg->List(stdout, 0);
    }
  }
#endif

  return NS_OK;
}

nsresult
nsCSSFrameConstructor::ReinsertContent(nsIContent*     aContainer,
                                       nsIContent*     aChild)
{
  PRInt32 ix = aContainer->IndexOf(aChild);
  
  
  
  
  nsresult res = ContentRemoved(aContainer, aChild, ix, PR_TRUE);

  if (NS_SUCCEEDED(res)) {
    res = ContentInserted(aContainer, aChild, ix, nsnull, PR_TRUE);
  }

  return res;
}
























static nsresult
DoDeletingFrameSubtree(nsFrameManager* aFrameManager,
                       nsVoidArray&    aDestroyQueue,
                       nsIFrame*       aRemovedFrame,
                       nsIFrame*       aFrame)
{
  
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
      if (NS_LIKELY(nsGkAtoms::placeholderFrame != childFrame->GetType())) {
        DoDeletingFrameSubtree(aFrameManager, aDestroyQueue,
                               aRemovedFrame, childFrame);

      } else {
        nsIFrame* outOfFlowFrame =
          nsPlaceholderFrame::GetRealFrameForPlaceholder(childFrame);
  
        
        aFrameManager->UnregisterPlaceholderFrame((nsPlaceholderFrame*)childFrame);
        
        
        
        
        
        
        
        if (outOfFlowFrame->GetStyleDisplay()->mDisplay == NS_STYLE_DISPLAY_POPUP ||
            !nsLayoutUtils::IsProperAncestorFrame(aRemovedFrame, outOfFlowFrame)) {
          NS_ASSERTION(aDestroyQueue.IndexOf(outOfFlowFrame) == kNotFound,
                       "out-of-flow is already in the destroy queue");
          aDestroyQueue.AppendElement(outOfFlowFrame);
          
          DoDeletingFrameSubtree(aFrameManager, aDestroyQueue,
                                 outOfFlowFrame, outOfFlowFrame);
        }
        else {
          
          
          DoDeletingFrameSubtree(aFrameManager, aDestroyQueue,
                                 aRemovedFrame, outOfFlowFrame);
        }
      }
    }

    
    
    do {
      childListName = aFrame->GetAdditionalChildListName(childListIndex++);
    } while (childListName == nsGkAtoms::floatList    ||
             childListName == nsGkAtoms::absoluteList ||
             childListName == nsGkAtoms::overflowOutOfFlowList ||
             childListName == nsGkAtoms::fixedList);
  } while (childListName);

  return NS_OK;
}





static nsresult
DeletingFrameSubtree(nsFrameManager* aFrameManager,
                     nsIFrame*       aFrame)
{
  NS_ENSURE_TRUE(aFrame, NS_OK); 

  
  
  if (NS_UNLIKELY(!aFrameManager)) {
    return NS_OK;
  }

  nsAutoVoidArray destroyQueue;

  
  
  NS_ASSERTION(!IsFrameSpecial(aFrame),
               "DeletingFrameSubtree on a special frame.  Prepare to crash.");

  do {
    DoDeletingFrameSubtree(aFrameManager, destroyQueue, aFrame, aFrame);

    
    
    
    
    
    aFrame = aFrame->GetNextContinuation();
  } while (aFrame);

  
  
  for (PRInt32 i = destroyQueue.Count() - 1; i >= 0; --i) {
    nsIFrame* outOfFlowFrame = static_cast<nsIFrame*>(destroyQueue[i]);

    
    
    aFrameManager->RemoveFrame(outOfFlowFrame->GetParent(),
                               GetChildListNameFor(outOfFlowFrame),
                               outOfFlowFrame);
  }

  return NS_OK;
}

nsresult
nsCSSFrameConstructor::RemoveMappingsForFrameSubtree(nsIFrame* aRemovedFrame)
{
  if (NS_UNLIKELY(mIsDestroyingFrameTree)) {
    
    
    
    
    return NS_OK;
  }

  
  CaptureStateFor(aRemovedFrame, mTempFrameTreeState);

  return ::DeletingFrameSubtree(mPresShell->FrameManager(), aRemovedFrame);
}

static void UnregisterPlaceholderChain(nsFrameManager* frameManager,
                                       nsPlaceholderFrame* placeholderFrame)
{
  
  nsPlaceholderFrame* curFrame = placeholderFrame;
  do {
    frameManager->UnregisterPlaceholderFrame(curFrame);
    curFrame = static_cast<nsPlaceholderFrame*>(curFrame->GetNextContinuation());
  } while (curFrame);
}

nsresult
nsCSSFrameConstructor::ContentRemoved(nsIContent*     aContainer,
                                      nsIContent*     aChild,
                                      PRInt32         aIndexInContainer,
                                      PRBool          aInReinsertContent)
{
  AUTO_LAYOUT_PHASE_ENTRY_POINT(mPresShell->GetPresContext(), FrameC);
  
  

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

  if (! childFrame) {
    frameManager->ClearUndisplayedContentIn(aChild, aContainer);
  }

#ifdef MOZ_XUL
  if (NotifyListBoxBody(presContext, aContainer, aChild, aIndexInContainer, 
                        mDocument, childFrame, gUseXBLForms, CONTENT_REMOVED))
    return NS_OK;

#endif 

  if (childFrame) {
    InvalidateCanvasIfNeeded(childFrame);
    
    
    
    
    
    
    if (IsFrameSpecial(childFrame) && !aInReinsertContent) {
      
      
      
      
      
      
#ifdef DEBUG
      if (gNoisyContentUpdates) {
        printf("nsCSSFrameConstructor::ContentRemoved: childFrame=");
        nsFrame::ListTag(stdout, childFrame);
        printf(" is special\n");
      }
#endif
      return ReframeContainingBlock(childFrame);
    }

    
    nsIFrame* parentFrame = childFrame->GetParent();

    if (parentFrame->GetType() == nsGkAtoms::frameSetFrame &&
        IsSpecialFramesetChild(aChild)) {
      
      return RecreateFramesForContent(parentFrame->GetContent());
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
      if (!childFrame) {
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

      nsIFrameDebug* fdbg = nsnull;
      CallQueryInterface(parentFrame, &fdbg);
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

    if (mInitialContainingBlock == childFrame) {
      mInitialContainingBlock = nsnull;
      mInitialContainingBlockIsAbsPosContainer = PR_FALSE;
    }

    if (haveFLS && mInitialContainingBlock) {
      NS_ASSERTION(containingBlock == GetFloatContainingBlock(parentFrame),
                   "What happened here?");
      nsFrameConstructorState state(mPresShell, mFixedContainingBlock,
                                    GetAbsoluteContainingBlock(parentFrame),
                                    containingBlock);
      RecoverLetterFrames(state, containingBlock);
    }

#ifdef DEBUG
    if (gReallyNoisyContentUpdates && parentFrame) {
      nsIFrameDebug* fdbg = nsnull;
      CallQueryInterface(parentFrame, &fdbg);
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
      if (!(child->GetStateBits() & NS_FRAME_OUT_OF_FLOW)) {
        
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
      aFrame->Invalidate(aFrame->GetOverflowRect());
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
  PRBool isCanvas;
  while (!nsCSSRendering::FindBackground(aPresContext, aFrame,
                                         &bg, &isCanvas)) {
    aFrame = aFrame->GetParent();
    NS_ASSERTION(aFrame, "root frame must paint");
  }

  nsIViewManager* viewManager = aPresContext->GetViewManager();

  
  

  

  viewManager->BeginUpdateViewBatch();

#ifdef DEBUG
  gInApplyRenderingChangeToTree = PR_TRUE;
#endif
  DoApplyRenderingChangeToTree(aFrame, viewManager, shell->FrameManager(),
                               aChange);
#ifdef DEBUG
  gInApplyRenderingChangeToTree = PR_FALSE;
#endif
  
  viewManager->EndUpdateViewBatch(NS_VMREFRESH_NO_SYNC);
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
  PRBool isCanvas;
  nsPresContext* presContext = aFrame->PresContext();
  while (!nsCSSRendering::FindBackground(presContext, ancestor,
                                         &bg, &isCanvas)) {
    ancestor = ancestor->GetParent();
    NS_ASSERTION(ancestor, "canvas must paint");
  }

  if (ancestor->GetType() == nsGkAtoms::canvasFrame) {
    
    
    ancestor = ancestor->GetParent();
  }

  if (ancestor != aFrame) {
    ApplyRenderingChangeToTree(presContext, ancestor,
                               nsChangeHint_RepaintFrame);
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

  mPresShell->FrameNeedsReflow(aFrame, nsIPresShell::eStyleChange,
                               NS_FRAME_IS_DIRTY);

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
      
      nsIContent* blockContent = block->GetContent();
      nsStyleContext* blockSC = block->GetStyleContext();
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
      RecoverLetterFrames(state, block);
    }
  }

  return rv;
}

nsresult
nsCSSFrameConstructor::ProcessRestyledFrames(nsStyleChangeList& aChangeList)
{
  PRInt32 count = aChangeList.Count();
  if (!count)
    return NS_OK;

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

#ifdef DEBUG
    
    if (content) {
      nsIFrame* frame = mPresShell->GetPrimaryFrameFor(content);
      if (frame) {
        mPresShell->FrameManager()->DebugVerifyStyleTree(frame);
      }
    } else {
      NS_WARNING("Unable to test style tree integrity -- no content node");
    }
#endif
  }

  
  index = count;
  while (0 <= --index) {
    const nsStyleChangeData* changeData;
    aChangeList.ChangeAt(index, &changeData);
    if (changeData->mFrame) {
      propTable->DeleteProperty(changeData->mFrame,
                                nsGkAtoms::changeListProperty);
    }
  }

  aChangeList.Clear();
  return NS_OK;
}

void
nsCSSFrameConstructor::RestyleElement(nsIContent     *aContent,
                                      nsIFrame       *aPrimaryFrame,
                                      nsChangeHint   aMinHint)
{
  if (aMinHint & nsChangeHint_ReconstructFrame) {
    RecreateFramesForContent(aContent);
  } else if (aPrimaryFrame) {
    nsStyleChangeList changeList;
    if (aMinHint) {
      changeList.AppendChange(aPrimaryFrame, aContent, aMinHint);
    }
    nsChangeHint frameChange = mPresShell->FrameManager()->
      ComputeStyleChangeFor(aPrimaryFrame, &changeList, aMinHint);

    if (frameChange & nsChangeHint_ReconstructFrame) {
      RecreateFramesForContent(aContent);
      changeList.Clear();
    } else {
      ProcessRestyledFrames(changeList);
    }
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
nsCSSFrameConstructor::EndUpdate()
{
  if (mUpdateCount == 1) {
    
    

    RecalcQuotesAndCounters();
    NS_ASSERTION(mUpdateCount == 1, "Odd update count");
  }

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

  
  mRestyleEvent.Revoke();
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
    
    nsHTMLContainerFrame::CreateViewForFrame(newFrame, nsnull, PR_FALSE);

    
    
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
    
    nsHTMLContainerFrame::CreateViewForFrame(newFrame, nsnull, PR_FALSE);

    
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
          ProcessChildren(state, headerFooter, headerFooterFrame,
                          PR_FALSE, childItems, PR_FALSE);
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
      
      nsHTMLContainerFrame::CreateViewForFrame(newFrame, nsnull, PR_FALSE);
    }
    
  } else if (nsGkAtoms::inlineFrame == frameType) {
    newFrame = NS_NewInlineFrame(shell, styleContext);

    if (newFrame) {
      newFrame->Init(content, aParentFrame, aFrame);
      
      nsHTMLContainerFrame::CreateViewForFrame(newFrame, nsnull, PR_FALSE);
    }
  
  } else if (nsGkAtoms::blockFrame == frameType) {
    newFrame = NS_NewBlockFrame(shell, styleContext);

    if (newFrame) {
      newFrame->Init(content, aParentFrame, aFrame);
      
      nsHTMLContainerFrame::CreateViewForFrame(newFrame, nsnull, PR_FALSE);
    }
  
  } else if (nsGkAtoms::areaFrame == frameType) {
    newFrame = NS_NewAreaFrame(shell, styleContext, 0);

    if (newFrame) {
      newFrame->Init(content, aParentFrame, aFrame);
      
      nsHTMLContainerFrame::CreateViewForFrame(newFrame, nsnull, PR_FALSE);
    }
  
  } else if (nsGkAtoms::columnSetFrame == frameType) {
    newFrame = NS_NewColumnSetFrame(shell, styleContext, 0);

    if (newFrame) {
      newFrame->Init(content, aParentFrame, aFrame);
      
      nsHTMLContainerFrame::CreateViewForFrame(newFrame, nsnull, PR_FALSE);
    }
  
  } else if (nsGkAtoms::positionedInlineFrame == frameType) {
    newFrame = NS_NewPositionedInlineFrame(shell, styleContext);

    if (newFrame) {
      newFrame->Init(content, aParentFrame, aFrame);
      
      nsHTMLContainerFrame::CreateViewForFrame(newFrame, nsnull, PR_FALSE);
    }

  } else if (nsGkAtoms::pageFrame == frameType) {
    nsIFrame* pageContentFrame;
    rv = ConstructPageFrame(shell, aPresContext, aParentFrame, aFrame,
                            newFrame, pageContentFrame);
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
      
      nsHTMLContainerFrame::CreateViewForFrame(newFrame, nsnull, PR_FALSE);
    }

  } else if (nsGkAtoms::tableRowFrame == frameType) {
    newFrame = NS_NewTableRowFrame(shell, styleContext);

    if (newFrame) {
      newFrame->Init(content, aParentFrame, aFrame);
      
      nsHTMLContainerFrame::CreateViewForFrame(newFrame, nsnull, PR_FALSE);

      
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
      
      nsHTMLContainerFrame::CreateViewForFrame(newFrame, nsnull, PR_FALSE);

      
      nsIFrame* continuingAreaFrame;
      nsIFrame* areaFrame = aFrame->GetFirstChild(nsnull);
      rv = CreateContinuingFrame(aPresContext, areaFrame, newFrame,
                                 &continuingAreaFrame);
      if (NS_FAILED(rv)) {
        newFrame->Destroy();
        *aContinuingFrame = nsnull;
        return rv;
      }

      
      newFrame->SetInitialChildList(nsnull, continuingAreaFrame);
    }
  
  } else if (nsGkAtoms::lineFrame == frameType) {
    newFrame = NS_NewFirstLineFrame(shell, styleContext);

    if (newFrame) {
      newFrame->Init(content, aParentFrame, aFrame);
      
      nsHTMLContainerFrame::CreateViewForFrame(newFrame, nsnull, PR_FALSE);
    }
  
  } else if (nsGkAtoms::letterFrame == frameType) {
    newFrame = NS_NewFirstLetterFrame(shell, styleContext);

    if (newFrame) {
      newFrame->Init(content, aParentFrame, aFrame);
      
      nsHTMLContainerFrame::CreateViewForFrame(newFrame, nsnull, PR_FALSE);
    }

  } else if (nsGkAtoms::imageFrame == frameType) {
    newFrame = NS_NewImageFrame(shell, styleContext);

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
    
    rv = CreatePlaceholderFrameFor(shell, aPresContext, shell->FrameManager(),
                                   content, oofContFrame, styleContext,
                                   aParentFrame, &newFrame);
    if (NS_FAILED(rv)) {
      oofContFrame->Destroy();
      *aContinuingFrame = nsnull;
      return rv;
    }
    newFrame->Init(content, aParentFrame, aFrame);
  } else if (nsGkAtoms::fieldSetFrame == frameType) {
    newFrame = NS_NewFieldSetFrame(shell, styleContext);

    if (newFrame) {
      newFrame->Init(content, aParentFrame, aFrame);

      
      nsHTMLContainerFrame::CreateViewForFrame(newFrame, nsnull, PR_FALSE);

      
      
      nsIFrame* continuingAreaFrame;
      nsIFrame* areaFrame = GetFieldSetAreaFrame(aFrame);
      rv = CreateContinuingFrame(aPresContext, areaFrame, newFrame,
                                 &continuingAreaFrame);
      if (NS_FAILED(rv)) {
        newFrame->Destroy();
        *aContinuingFrame = nsnull;
        return rv;
      }
      
      newFrame->SetInitialChildList(nsnull, continuingAreaFrame);
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
  nsIFrame* docRootFrame = aParentFrame->GetFirstChild(nsnull);
  nsIFrame* prevDocRootFrame = prevPageContentFrame->GetFirstChild(nsnull);
  if (!docRootFrame || !prevDocRootFrame) {
    
    return NS_ERROR_UNEXPECTED;
  }

  nsFrameItems fixedPlaceholders;
  nsIFrame* firstFixed = prevPageContentFrame->GetFirstChild(nsGkAtoms::fixedList);
  if (!firstFixed) {
    return NS_OK;
  }

  
  nsFrameConstructorState state(mPresShell, aParentFrame,
                                mInitialContainingBlock,
                                mInitialContainingBlock);

  
  
  
  
  for (nsIFrame* fixed = firstFixed; fixed; fixed = fixed->GetNextSibling()) {
    nsIFrame* prevPlaceholder = nsnull;
    mPresShell->GetPlaceholderFrameFor(fixed, &prevPlaceholder);
    if (prevPlaceholder &&
        nsLayoutUtils::IsProperAncestorFrame(prevDocRootFrame, prevPlaceholder)) {
      nsresult rv = ConstructFrame(state, fixed->GetContent(),
                                   docRootFrame, fixedPlaceholders);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  
  
  
  NS_ASSERTION(!docRootFrame->GetFirstChild(nsnull),
               "leaking frames; doc root continuation must be empty");
  docRootFrame->SetInitialChildList(nsnull, fixedPlaceholders.childList);
  return NS_OK;
}




nsIFrame*
nsCSSFrameConstructor::FindFrameWithContent(nsFrameManager*  aFrameManager,
                                            nsIFrame*        aParentFrame,
                                            nsIContent*      aParentContent,
                                            nsIContent*      aContent,
                                            nsFindFrameHint* aHint)
{
  
#ifdef NOISY_FINDFRAME
  FFWC_totalCount++;
  printf("looking for content=%p, given aParentFrame %p parentContent %p, hint is %s\n", 
         aContent, aParentFrame, aParentContent, aHint ? "set" : "NULL");
#endif

  NS_ENSURE_TRUE(aParentFrame != nsnull, nsnull);

  do {
    
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
              (aParentContent && (aParentContent == kidContent->GetBindingParent()))) 
          {
#ifdef NOISY_FINDFRAME
            FFWC_recursions++;
            printf("  recursing with new parent set to kidframe=%p, parentContent=%p\n", 
                   kidFrame, aParentContent);
#endif
            nsIFrame* matchingFrame =
                FindFrameWithContent(aFrameManager, kidFrame,
                                     aParentContent, aContent, nsnull);

            if (matchingFrame) {
              return matchingFrame;
            }
          }
        }

        
        kidFrame = kidFrame->GetNextSibling();
#ifdef NOISY_FINDFRAME
        FFWC_doSibling++;
        if (kidFrame) {
          printf("  searching sibling frame %p\n", kidFrame);
        }
#endif
      }

      if (aHint) {
        
        
        
        
        
        
        
        
        aHint = nsnull;
        searchAgain = PR_TRUE;
      } else {
        listName = aParentFrame->GetAdditionalChildListName(listIndex++);
      }
    } while(listName || searchAgain);

    
    
    aParentFrame = nsLayoutUtils::GetNextContinuationOrSpecialSibling(aParentFrame);
#ifdef NOISY_FINDFRAME
    if (aParentFrame) {
      FFWC_nextInFlows++;
      printf("  searching NIF frame %p\n", aParentFrame);
    }
#endif
  } while (aParentFrame);

  
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
      
      
      
      
      if (gVerifyFastFindFrame && aHint) 
      {
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
      else if (IsFrameSpecial(parentFrame)) {
        
        
        
        
        nsIFrame* specialSibling = nsnull;
        GetSpecialSibling(aFrameManager, parentFrame, &specialSibling);
        parentFrame = specialSibling;
      }
      else {
        break;
      }
    }
  }

  if (aHint && !*aFrame)
  { 
    if (aContent->IsNodeOfType(nsINode::eTEXT)) 
    {
#ifdef NOISY_FINDFRAME
      FFWC_slowSearchForText++;
#endif
      
      return FindPrimaryFrameFor(aFrameManager, aContent, aFrame, nsnull);
    }
  }

#ifdef NOISY_FINDFRAME
  printf("%10s %10s %10s %10s %10s \n", 
         "total", "doLoop", "doSibling", "recur", "nextIF", "slowSearch");
  printf("%10d %10d %10d %10d %10d \n", 
         FFWC_totalCount, FFWC_doLoop, FFWC_doSibling, FFWC_recursions, 
         FFWC_nextInFlows, FFWC_slowSearchForText);
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
nsCSSFrameConstructor::MaybeRecreateContainerForIBSplitterFrame(nsIFrame* aFrame, nsresult* aResult)
{
  if (!aFrame || !IsFrameSpecial(aFrame))
    return PR_FALSE;

#ifdef DEBUG
  if (gNoisyContentUpdates) {
    printf("nsCSSFrameConstructor::RecreateFramesForContent: frame=");
    nsFrame::ListTag(stdout, aFrame);
    printf(" is special\n");
  }
#endif
  *aResult = ReframeContainingBlock(aFrame);
  return PR_TRUE;
}
 
nsresult
nsCSSFrameConstructor::RecreateFramesForContent(nsIContent* aContent)
{
  
  
  
  
  
  NS_ENSURE_TRUE(aContent->GetDocument(), NS_ERROR_FAILURE);

  
  
  
  
  

  nsIFrame* frame = mPresShell->GetPrimaryFrameFor(aContent);

  nsresult rv = NS_OK;

  if (frame) {
    
    
    
    
    
    
    if (MaybeRecreateContainerForIBSplitterFrame(frame, &rv) ||
        (!IsInlineOutside(frame) &&
         MaybeRecreateContainerForIBSplitterFrame(frame->GetParent(), &rv)))
      return rv;
  }

  nsCOMPtr<nsIContent> container = aContent->GetParent();
  if (container) {
    
    PRInt32 indexInContainer = container->IndexOf(aContent);
    
    
    CaptureStateForFramesOf(aContent, mTempFrameTreeState);

    
    
    rv = ContentRemoved(container, aContent, indexInContainer,
                        PR_FALSE);

    if (NS_SUCCEEDED(rv)) {
      
      rv = ContentInserted(container, aContent,
                           indexInContainer, mTempFrameTreeState, PR_FALSE);
    }
  } else {
    
    ReconstructDocElementHierarchy();
  }

#ifdef ACCESSIBILITY
  if (mPresShell->IsAccessibilityActive()) {
    PRUint32 event;
    if (frame) {
      nsIFrame *newFrame = mPresShell->GetPrimaryFrameFor(aContent);
      event = newFrame ? nsIAccessibleEvent::EVENT_REORDER : nsIAccessibleEvent::EVENT_HIDE;
    }
    else {
      event = nsIAccessibleEvent::EVENT_SHOW;
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
  
#ifdef DEBUG
  nsBlockFrame* block;
  NS_ASSERTION(NS_SUCCEEDED(aBlockFrame->QueryInterface(kBlockFrameCID,
                                                        (void**)&block)) &&
               block,
               "Not a block frame?");
#endif

  return (aBlockFrame->GetStateBits() & NS_BLOCK_HAS_FIRST_LETTER_STYLE) != 0;
}

PRBool
nsCSSFrameConstructor::ShouldHaveFirstLineStyle(nsIContent* aContent,
                                                nsStyleContext* aStyleContext)
{
  return nsLayoutUtils::HasPseudoStyle(aContent, aStyleContext,
                                       nsCSSPseudoElements::firstLine,
                                       mPresShell->GetPresContext());
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
nsCSSFrameConstructor::ProcessChildren(nsFrameConstructorState& aState,
                                       nsIContent*              aContent,
                                       nsIFrame*                aFrame,
                                       PRBool                   aCanHaveGeneratedContent,
                                       nsFrameItems&            aFrameItems,
                                       PRBool                   aParentIsBlock)
{
  NS_PRECONDITION(!aFrame->IsLeaf(), "Bogus ProcessChildren caller!");
  
  
  nsresult rv = NS_OK;
  
  
  nsStyleContext* styleContext =
    nsFrame::CorrectStyleParentFrame(aFrame, nsnull)->GetStyleContext();
    
  if (aCanHaveGeneratedContent) {
    
    nsIFrame* generatedFrame;
    if (CreateGeneratedContentFrame(aState, aFrame, aContent,
                                    styleContext, nsCSSPseudoElements::before,
                                    &generatedFrame)) {
      
      aFrameItems.AddChild(generatedFrame);
    }
  }

 
  
  nsPseudoFrames priorPseudoFrames;
  aState.mPseudoFrames.Reset(&priorPseudoFrames);

  ChildIterator iter, last;
  for (ChildIterator::Init(aContent, &iter, &last);
       iter != last;
       ++iter) {
    rv = ConstructFrame(aState, nsCOMPtr<nsIContent>(*iter),
                        aFrame, aFrameItems);
    if (NS_FAILED(rv))
      return rv;
  }

  
  if (!aState.mPseudoFrames.IsEmpty()) {
    ProcessPseudoFrames(aState, aFrameItems);
  }

  
  aState.mPseudoFrames = priorPseudoFrames;

  if (aCanHaveGeneratedContent) {
    
    nsIFrame* generatedFrame;
    if (CreateGeneratedContentFrame(aState, aFrame, aContent,
                                    styleContext, nsCSSPseudoElements::after,
                                    &generatedFrame)) {
      
      aFrameItems.AddChild(generatedFrame);
    }
  }

  if (aParentIsBlock) {
    if (aState.mFirstLetterStyle) {
      rv = WrapFramesInFirstLetterFrame(aState, aContent, aFrame, aFrameItems);
    }
    if (aState.mFirstLineStyle) {
      rv = WrapFramesInFirstLineFrame(aState, aContent, aFrame, aFrameItems);
    }
  }

  return rv;
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

  
  nsStyleContext* parentStyle = aBlockFrame->GetStyleContext();
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
          
          nsStyleContext* parentStyle = aBlockFrame->GetStyleContext();
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
  NS_ASSERTION(letterContent->GetBindingParent() != letterContent,
               "Reframes of this letter frame will mess with the root of a "
               "native anonymous content subtree!");
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

  rv = aState.AddChild(letterFrame, aResult, letterFrame->GetStyleDisplay(),
                       letterContent, aStyleContext, aParentFrame, PR_FALSE,
                       PR_TRUE, PR_FALSE, PR_TRUE, insertAfter);

  if (nextTextFrame) {
    if (NS_FAILED(rv)) {
      nextTextFrame->Destroy();
    } else {
      aResult.AddChild(nextTextFrame);
    }
  }
}





nsresult
nsCSSFrameConstructor::CreateLetterFrame(nsFrameConstructorState& aState,
                                         nsIFrame* aBlockFrame,
                                         nsIContent* aTextContent,
                                         nsIFrame* aParentFrame,
                                         nsFrameItems& aResult)
{
  NS_PRECONDITION(aTextContent->IsNodeOfType(nsINode::eTEXT),
                  "aTextContent isn't text");

#ifdef DEBUG
  {
    nsBlockFrame* block;
    NS_ASSERTION(NS_SUCCEEDED(aBlockFrame->QueryInterface(kBlockFrameCID,
                                                          (void**)&block)) &&
                 block,
                 "Not a block frame?");
  }
#endif

  
  nsStyleContext* parentStyleContext = aParentFrame->GetStyleContext();
  if (parentStyleContext) {
    
    
    nsIContent* blockContent = aState.mFloatedItems.containingBlock->GetContent();

    NS_ASSERTION(blockContent == aBlockFrame->GetContent(),
                 "Unexpected block content");

    
    nsRefPtr<nsStyleContext> sc = GetFirstLetterStyle(blockContent,
                                                      parentStyleContext);
    if (sc) {
      nsRefPtr<nsStyleContext> textSC;
          textSC = mPresShell->StyleSet()->ResolveStyleForNonElement(sc);
    
      
      
      nsIFrame* textFrame = NS_NewTextFrame(mPresShell, textSC);

      
      const nsStyleDisplay* display = sc->GetStyleDisplay();
      if (display->IsFloating()) {
        
        CreateFloatingLetterFrame(aState, aBlockFrame, aTextContent, textFrame,
                                  blockContent, aParentFrame,
                                  sc, aResult);
      }
      else {
        
        nsIFrame* letterFrame = NS_NewFirstLetterFrame(mPresShell, sc);

        if (letterFrame) {
          
          
          
          nsIContent* letterContent = aTextContent->GetParent();
          NS_ASSERTION(letterContent->GetBindingParent() != letterContent,
                       "Reframes of this letter frame will mess with the root "
                       "of a native anonymous content subtree!");
          letterFrame->Init(letterContent, aParentFrame, nsnull);

          InitAndRestoreFrame(aState, aTextContent, letterFrame, nsnull,
                              textFrame);

          letterFrame->SetInitialChildList(nsnull, textFrame);
          aResult.childList = aResult.lastChild = letterFrame;
        }
      }
    }
  }

  return NS_OK;
}

nsresult
nsCSSFrameConstructor::WrapFramesInFirstLetterFrame(
  nsFrameConstructorState& aState,
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
  rv = WrapFramesInFirstLetterFrame(aState, aBlockFrame, aBlockFrame,
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
      
      ::DeletingFrameSubtree(aState.mFrameManager, textFrame);
      parentFrame->RemoveFrame(nsnull, textFrame);

      
      parentFrame->InsertFrames(nsnull, prevFrame, letterFrames.childList);
    }
  }

  return rv;
}

nsresult
nsCSSFrameConstructor::WrapFramesInFirstLetterFrame(
  nsFrameConstructorState& aState,
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
        
        rv = CreateLetterFrame(aState, aBlockFrame, textContent,
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
    else if ((nsGkAtoms::inlineFrame == frameType) ||
             (nsGkAtoms::lineFrame == frameType) ||
             (nsGkAtoms::positionedInlineFrame == frameType)) {
      nsIFrame* kids = frame->GetFirstChild(nsnull);
      WrapFramesInFirstLetterFrame(aState, aBlockFrame, frame, kids,
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

  
  
  nsIFrame* nextTextFrame = textFrame->GetNextInFlow();
  if (nextTextFrame) {
    nsIFrame* nextTextParent = nextTextFrame->GetParent();
    if (nextTextParent) {
      nsSplittableFrame::BreakFromPrevFlow(nextTextFrame);
      ::DeletingFrameSubtree(aFrameManager, nextTextFrame);
      aFrameManager->RemoveFrame(nextTextParent, nsnull, nextTextFrame);
    }
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
    nsIAtom* frameType = kid->GetType();
    if (nsGkAtoms::letterFrame == frameType) {
      
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
      break;
    }
    else if ((nsGkAtoms::inlineFrame == frameType) ||
             (nsGkAtoms::lineFrame == frameType) ||
             (nsGkAtoms::positionedInlineFrame == frameType)) {
      
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
  PRBool stopLooking = PR_FALSE;
  nsresult rv = RemoveFloatingFirstLetterFrames(aPresContext, aPresShell,
                                                aFrameManager,
                                                aBlockFrame, &stopLooking);
  if (NS_SUCCEEDED(rv) && !stopLooking) {
    rv = RemoveFirstLetterFrames(aPresContext, aPresShell, aFrameManager,
                                 aBlockFrame, &stopLooking);
  }
  return rv;
}


nsresult
nsCSSFrameConstructor::RecoverLetterFrames(nsFrameConstructorState& aState,
                                           nsIFrame* aBlockFrame)
{
  nsresult rv = NS_OK;

  aBlockFrame->AddStateBits(NS_BLOCK_HAS_FIRST_LETTER_STYLE);

  nsIFrame* blockKids = aBlockFrame->GetFirstChild(nsnull);
  nsIFrame* parentFrame = nsnull;
  nsIFrame* textFrame = nsnull;
  nsIFrame* prevFrame = nsnull;
  nsFrameItems letterFrames;
  PRBool stopLooking = PR_FALSE;
  rv = WrapFramesInFirstLetterFrame(aState, aBlockFrame, aBlockFrame, blockKids,
                                    &parentFrame, &textFrame, &prevFrame,
                                    letterFrames, &stopLooking);
  if (NS_FAILED(rv)) {
    return rv;
  }
  if (parentFrame) {
    
    ::DeletingFrameSubtree(aState.mFrameManager, textFrame);
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

    rv = ConstructFrameInternal(state, aChild,
                                aParentFrame, aChild->Tag(),
                                aChild->GetNameSpaceID(),
                                styleContext, frameItems, PR_FALSE);
    if (!state.mPseudoFrames.IsEmpty()) {
      ProcessPseudoFrames(state, frameItems); 
    }

    nsIFrame* newFrame = frameItems.childList;
    *aNewFrame = newFrame;

    if (NS_SUCCEEDED(rv) && (nsnull != newFrame)) {
      
      if (aIsAppend)
        rv = ((nsListBoxBodyFrame*)aParentFrame)->ListBoxAppendFrames(newFrame);
      else
        rv = ((nsListBoxBodyFrame*)aParentFrame)->ListBoxInsertFrames(aPrevFrame, newFrame);
    }
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
  nsIFrame* contentParent = aContentParentFrame;
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
    
    nsHTMLContainerFrame::CreateViewForFrame(columnSetFrame, aContentParentFrame,
                                             PR_FALSE);
    blockStyle = mPresShell->StyleSet()->
      ResolvePseudoStyleFor(aContent, nsCSSAnonBoxes::columnContent,
                            aStyleContext);
    contentParent = columnSetFrame;
    parent = columnSetFrame;
    *aNewFrame = columnSetFrame;

    columnSetFrame->SetInitialChildList(nsnull, blockFrame);
  }

  blockFrame->SetStyleContextWithoutNotification(blockStyle);
  InitAndRestoreFrame(aState, aContent, parent, nsnull, blockFrame);

  nsresult rv = aState.AddChild(*aNewFrame, aFrameItems, aDisplay,
                                aContent, aStyleContext,
                                aContentParentFrame ? aContentParentFrame :
                                                      aParentFrame);
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  nsHTMLContainerFrame::CreateViewForFrame(blockFrame, contentParent, PR_FALSE);

  
  
  
  
  
  
  
  nsFrameConstructorSaveState absoluteSaveState;
  if (aAbsPosContainer) {
    
    aState.PushAbsoluteContainingBlock(blockFrame, absoluteSaveState);
  }

  
  PRBool haveFirstLetterStyle, haveFirstLineStyle;
  ShouldHaveSpecialBlockStyle(aContent, aStyleContext,
                              &haveFirstLetterStyle, &haveFirstLineStyle);

  
  nsFrameItems childItems;
  nsFrameConstructorSaveState floatSaveState;
  aState.PushFloatContainingBlock(blockFrame, floatSaveState,
                                  haveFirstLetterStyle,
                                  haveFirstLineStyle);
  rv = ProcessChildren(aState, aContent, blockFrame, PR_TRUE, childItems,
                       PR_TRUE);

  CreateAnonymousFrames(aContent->Tag(), aState, aContent, blockFrame,
                        PR_FALSE, childItems);

  
  blockFrame->SetInitialChildList(nsnull, childItems.childList);

  return rv;
}

PRBool
nsCSSFrameConstructor::AreAllKidsInline(nsIFrame* aFrameList)
{
  nsIFrame* kid = aFrameList;
  while (kid) {
    if (!IsInlineOutside(kid)) {
      return PR_FALSE;
    }
    kid = kid->GetNextSibling();
  }
  return PR_TRUE;
}

nsresult
nsCSSFrameConstructor::ConstructInline(nsFrameConstructorState& aState,
                                       const nsStyleDisplay*    aDisplay,
                                       nsIContent*              aContent,
                                       nsIFrame*                aParentFrame,
                                       nsStyleContext*          aStyleContext,
                                       PRBool                   aIsPositioned,
                                       nsIFrame*                aNewFrame)
{
  
  InitAndRestoreFrame(aState, aContent, aParentFrame, nsnull, aNewFrame);

  nsFrameConstructorSaveState absoluteSaveState;  
                                                  
                                                  

  
  
  nsHTMLContainerFrame::CreateViewForFrame(aNewFrame, nsnull, PR_FALSE);

  if (aIsPositioned) {                            
    
    
    aState.PushAbsoluteContainingBlock(aNewFrame, absoluteSaveState);
  }

  
  nsFrameItems childItems;
  PRBool kidsAllInline;
  nsresult rv = ProcessInlineChildren(aState, aContent, aNewFrame, PR_TRUE,
                                      childItems, &kidsAllInline);
  if (kidsAllInline) {
    
    CreateAnonymousFrames(aContent->Tag(), aState, aContent, aNewFrame,
                          PR_FALSE, childItems);

    aNewFrame->SetInitialChildList(nsnull, childItems.childList);
    return rv;
  }

  
  
  
  
  
  
  
  

  
  nsIFrame* list1 = childItems.childList;
  nsIFrame* prevToFirstBlock;
  nsIFrame* list2 = FindFirstBlock(list1, &prevToFirstBlock);
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

  
  aNewFrame->SetInitialChildList(nsnull, list1);
                                             
  
  
  
  nsIAtom* blockStyle;
  nsRefPtr<nsStyleContext> blockSC;
  nsIFrame* blockFrame;
  if (aIsPositioned) {
    blockStyle = nsCSSAnonBoxes::mozAnonymousPositionedBlock;
    
    blockSC = mPresShell->StyleSet()->
      ResolvePseudoStyleFor(aContent, blockStyle, aStyleContext);
      
    blockFrame = NS_NewRelativeItemWrapperFrame(mPresShell, blockSC);
  }
  else {
    blockStyle = nsCSSAnonBoxes::mozAnonymousBlock;

    blockSC = mPresShell->StyleSet()->
      ResolvePseudoStyleFor(aContent, blockStyle, aStyleContext);

    blockFrame = NS_NewBlockFrame(mPresShell, blockSC);
  }

  InitAndRestoreFrame(aState, aContent, aParentFrame, nsnull, blockFrame, PR_FALSE);  

  
  
  nsHTMLContainerFrame::CreateViewForFrame(blockFrame, nsnull, PR_FALSE);

  if (blockFrame->HasView() || aNewFrame->HasView()) {
    
    nsHTMLContainerFrame::ReparentFrameViewList(aState.mPresContext, list2,
                                                list2->GetParent(), blockFrame);
  }

  blockFrame->SetInitialChildList(nsnull, list2);

  nsFrameConstructorState state(mPresShell, mFixedContainingBlock,
                                GetAbsoluteContainingBlock(blockFrame),
                                GetFloatContainingBlock(blockFrame));

  
  
  
  
  
  MoveChildrenTo(state.mFrameManager, blockSC, blockFrame, list2, &state, &aState);

  
  nsIFrame* inlineFrame = nsnull;

  if (list3) {
    if (aIsPositioned) {
      inlineFrame = NS_NewPositionedInlineFrame(mPresShell, aStyleContext);
    }
    else {
      inlineFrame = NS_NewInlineFrame(mPresShell, aStyleContext);
    }

    InitAndRestoreFrame(aState, aContent, aParentFrame, nsnull, inlineFrame, PR_FALSE);

    
    
    nsHTMLContainerFrame::CreateViewForFrame(inlineFrame, nsnull, PR_FALSE);

    if (inlineFrame->HasView() || aNewFrame->HasView()) {
      
      nsHTMLContainerFrame::ReparentFrameViewList(aState.mPresContext, list3,
                                                  list3->GetParent(), inlineFrame);
    }

    
    
    inlineFrame->SetInitialChildList(nsnull, list3);
    MoveChildrenTo(aState.mFrameManager, nsnull, inlineFrame, list3, nsnull, nsnull);
  }

  
  
  
  SetFrameIsSpecial(aNewFrame, blockFrame);
  SetFrameIsSpecial(blockFrame, inlineFrame);
  MarkIBSpecialPrevSibling(aState.mPresContext, blockFrame, aNewFrame);

  if (inlineFrame)
    SetFrameIsSpecial(inlineFrame, nsnull);

#ifdef DEBUG
  if (gNoisyInlineConstruction) {
    nsIFrameDebug*  frameDebug;

    printf("nsCSSFrameConstructor::ConstructInline:\n");
    if (NS_SUCCEEDED(CallQueryInterface(aNewFrame, &frameDebug))) {
      printf("  ==> leading inline frame:\n");
      frameDebug->List(stdout, 2);
    }
    if (NS_SUCCEEDED(CallQueryInterface(blockFrame, &frameDebug))) {
      printf("  ==> block frame:\n");
      frameDebug->List(stdout, 2);
    }
    if (inlineFrame &&
        NS_SUCCEEDED(CallQueryInterface(inlineFrame, &frameDebug))) {
      printf("  ==> trailing inline frame:\n");
      frameDebug->List(stdout, 2);
    }
  }
#endif

  return rv;
}

nsresult
nsCSSFrameConstructor::ProcessInlineChildren(nsFrameConstructorState& aState,
                                             nsIContent*              aContent,
                                             nsIFrame*                aFrame,
                                             PRBool                   aCanHaveGeneratedContent,
                                             nsFrameItems&            aFrameItems,
                                             PRBool*                  aKidsAllInline)
{
  nsresult rv = NS_OK;
  nsStyleContext* styleContext = nsnull;

  
  nsPseudoFrames prevPseudoFrames; 
  aState.mPseudoFrames.Reset(&prevPseudoFrames);

  if (aCanHaveGeneratedContent) {
    
    nsIFrame* generatedFrame;
    styleContext = aFrame->GetStyleContext();
    if (CreateGeneratedContentFrame(aState, aFrame, aContent,
                                    styleContext, nsCSSPseudoElements::before,
                                    &generatedFrame)) {
      
      aFrameItems.AddChild(generatedFrame);
    }
  }

  
  PRBool allKidsInline = PR_TRUE;
  ChildIterator iter, last;
  for (ChildIterator::Init(aContent, &iter, &last);
       iter != last;
       ++iter) {
    
    nsIFrame* oldLastChild = aFrameItems.lastChild;
    rv = ConstructFrame(aState, nsCOMPtr<nsIContent>(*iter),
                        aFrame, aFrameItems);

    if (NS_FAILED(rv)) {
      return rv;
    }

    
    
    
    if (allKidsInline) {
      nsIFrame* kid;
      if (oldLastChild) {
        kid = oldLastChild->GetNextSibling();
      }
      else {
        kid = aFrameItems.childList;
      }
      while (kid) {
        if (!IsInlineOutside(kid)) {
          allKidsInline = PR_FALSE;
          break;
        }
        kid = kid->GetNextSibling();
      }
    }
  }

  if (aCanHaveGeneratedContent) {
    
    nsIFrame* generatedFrame;
    if (CreateGeneratedContentFrame(aState, aFrame, aContent,
                                    styleContext, nsCSSPseudoElements::after,
                                    &generatedFrame)) {
      
      aFrameItems.AddChild(generatedFrame);
    }
  }

  
  if (!aState.mPseudoFrames.IsEmpty()) {
    ProcessPseudoFrames(aState, aFrameItems);
    
    
    
    
    
  }
  
  aState.mPseudoFrames = prevPseudoFrames;

  *aKidsAllInline = allKidsInline;

  return rv;
}

PRBool
nsCSSFrameConstructor::WipeContainingBlock(nsFrameConstructorState& aState,
                                           nsIFrame* aContainingBlock,
                                           nsIFrame* aFrame,
                                           nsIFrame* aFrameList)
{
  
  
  
  

  
  
  
  
  
  nsIAtom* frameType = aFrame->GetType();
  if ((frameType != nsGkAtoms::inlineFrame &&
       frameType != nsGkAtoms::positionedInlineFrame &&
       frameType != nsGkAtoms::lineFrame) ||
      AreAllKidsInline(aFrameList))
    return PR_FALSE;

  
  nsFrameManager *frameManager = aState.mFrameManager;

  
  
  frameManager->ClearAllUndisplayedContentIn(aFrame->GetContent());

  CleanupFrameReferences(frameManager, aFrameList);
  if (aState.mAbsoluteItems.childList) {
    CleanupFrameReferences(frameManager, aState.mAbsoluteItems.childList);
  }
  if (aState.mFixedItems.childList) {
    CleanupFrameReferences(frameManager, aState.mFixedItems.childList);
  }
  if (aState.mFloatedItems.childList) {
    CleanupFrameReferences(frameManager, aState.mFloatedItems.childList);
  }
#ifdef MOZ_XUL
  if (aState.mPopupItems.childList) {
    CleanupFrameReferences(frameManager, aState.mPopupItems.childList);
  }
#endif
  nsFrameList tmp(aFrameList);
  tmp.DestroyFrames();

  tmp.SetFrames(aState.mAbsoluteItems.childList);
  tmp.DestroyFrames();
  aState.mAbsoluteItems.childList = nsnull;

  tmp.SetFrames(aState.mFixedItems.childList);
  tmp.DestroyFrames();
  aState.mFixedItems.childList = nsnull;

  tmp.SetFrames(aState.mFloatedItems.childList);
  tmp.DestroyFrames();
  aState.mFloatedItems.childList = nsnull;

#ifdef MOZ_XUL
  tmp.SetFrames(aState.mPopupItems.childList);
  tmp.DestroyFrames();
  aState.mPopupItems.childList = nsnull;
#endif

  
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

nsresult nsCSSFrameConstructor::RemoveFixedItems(const nsFrameConstructorState& aState)
{
  nsresult rv=NS_OK;

  if (mFixedContainingBlock) {
    nsIFrame *fixedChild = nsnull;
    do {
      fixedChild = mFixedContainingBlock->GetFirstChild(nsGkAtoms::fixedList);
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

PR_STATIC_CALLBACK(PLDHashOperator)
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

void
nsCSSFrameConstructor::ProcessPendingRestyles()
{
  PRUint32 count = mPendingRestyles.Count();
  if (!count) {
    
    return;
  }
  
  NS_PRECONDITION(mDocument, "No document?  Pshaw!\n");

  nsCSSFrameConstructor::RestyleEnumerateData* restylesToProcess =
    new nsCSSFrameConstructor::RestyleEnumerateData[count];
  if (!restylesToProcess) {
    return;
  }

  nsCSSFrameConstructor::RestyleEnumerateData* lastRestyle = restylesToProcess;
  mPendingRestyles.Enumerate(CollectRestyles, &lastRestyle);

  NS_ASSERTION(lastRestyle - restylesToProcess ==
               PRInt32(count),
               "Enumeration screwed up somehow");

  
  
  mPendingRestyles.Clear();

  
  
  BeginUpdate();

  for (nsCSSFrameConstructor::RestyleEnumerateData* currentRestyle =
         restylesToProcess;
       currentRestyle != lastRestyle;
       ++currentRestyle) {
    ProcessOneRestyle(currentRestyle->mContent,
                      currentRestyle->mRestyleHint,
                      currentRestyle->mChangeHint);
  }

  delete [] restylesToProcess;

  
  
  
  mDocument->BindingManager()->ProcessAttachedQueue();

  EndUpdate();

#ifdef DEBUG
  mPresShell->VerifyStyleTree();
#endif
}

void
nsCSSFrameConstructor::PostRestyleEvent(nsIContent* aContent,
                                        nsReStyleHint aRestyleHint,
                                        nsChangeHint aMinChangeHint)
{
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
    
  if (!mRestyleEvent.IsPending()) {
    nsRefPtr<RestyleEvent> ev = new RestyleEvent(this);
    if (NS_FAILED(NS_DispatchToCurrentThread(ev))) {
      NS_WARNING("failed to dispatch restyle event");
      
    } else {
      mRestyleEvent = ev;
    }
  }
}

NS_IMETHODIMP nsCSSFrameConstructor::RestyleEvent::Run() {
  if (!mConstructor)
    return NS_OK;  

  nsIViewManager* viewManager =
    mConstructor->mPresShell->GetViewManager();
  NS_ASSERTION(viewManager, "Must have view manager for update");

  viewManager->BeginUpdateViewBatch();
  
  
  
  mConstructor->mPresShell->GetDocument()->
    FlushPendingNotifications(Flush_ContentAndNotify);

  
  
  mConstructor->mRestyleEvent.Forget();

  mConstructor->ProcessPendingRestyles();
  viewManager->EndUpdateViewBatch(NS_VMREFRESH_NO_SYNC);

  return NS_OK;
}

NS_IMETHODIMP
nsCSSFrameConstructor::LazyGenerateChildrenEvent::Run()
{
  mPresShell->GetDocument()->FlushPendingNotifications(Flush_Layout);

  
  nsIFrame* frame = mPresShell->GetPrimaryFrameFor(mContent);
  if (frame && frame->GetType() == nsGkAtoms::menuPopupFrame) {
#ifdef MOZ_XUL
    
    
    
    nsMenuPopupFrame* menuPopupFrame = static_cast<nsMenuPopupFrame *>(frame);
    if (menuPopupFrame->HasGeneratedChildren())
      return NS_OK;

    
    menuPopupFrame->SetGeneratedChildren();
#endif

    nsFrameItems childItems;
    nsFrameConstructorState state(mPresShell, nsnull, nsnull, nsnull);
    nsCSSFrameConstructor* fc = mPresShell->FrameConstructor();
    nsresult rv = fc->ProcessChildren(state, mContent, frame, PR_FALSE,
                                      childItems, PR_FALSE);
    if (NS_FAILED(rv))
      return rv;

    fc->CreateAnonymousFrames(mContent->Tag(), state, mContent, frame,
                              PR_FALSE, childItems);
    frame->SetInitialChildList(nsnull, childItems.childList);

    if (mCallback)
      mCallback(mContent, frame, mArg);

    
    mPresShell->GetDocument()->BindingManager()->ProcessAttachedQueue();
  }

  return NS_OK;
}
