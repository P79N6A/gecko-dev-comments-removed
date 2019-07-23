












































#include "nsSplitterFrame.h"
#include "nsGkAtoms.h"
#include "nsIDOMElement.h"
#include "nsIDOMXULElement.h"
#include "nsIDOMDocument.h"
#include "nsPresContext.h"
#include "nsIDocument.h"
#include "nsINameSpaceManager.h"
#include "nsScrollbarButtonFrame.h"
#include "nsIDOMMouseListener.h"
#include "nsIDOMMouseMotionListener.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMMouseEvent.h"
#include "nsIPresShell.h"
#include "nsFrameList.h"
#include "nsHTMLParts.h"
#include "nsILookAndFeel.h"
#include "nsStyleContext.h"
#include "nsBoxLayoutState.h"
#include "nsIXBLService.h"
#include "nsIServiceManager.h"
#include "nsHTMLContainerFrame.h"
#include "nsGUIEvent.h"
#include "nsAutoPtr.h"
#include "nsContentCID.h"
#include "nsStyleSet.h"
#include "nsLayoutUtils.h"
#include "nsDisplayList.h"
#include "nsContentUtils.h"

class nsSplitterInfo {
public:
  nscoord min;
  nscoord max;
  nscoord current;
  nscoord changed;
  nsCOMPtr<nsIContent> childElem;
  PRInt32 flex;
  PRInt32 index;
};

class nsSplitterFrameInner : public nsIDOMMouseListener, public nsIDOMMouseMotionListener {

public:

  NS_DECL_ISUPPORTS

  nsSplitterFrameInner(nsSplitterFrame* aSplitter)
  {
    mOuter = aSplitter;
    mPressed = PR_FALSE;
  }
  virtual ~nsSplitterFrameInner();

  void Disconnect() { mOuter = nsnull; }

  
  NS_IMETHOD MouseDown(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseUp(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseClick(nsIDOMEvent* aMouseEvent) { return NS_OK; }
  NS_IMETHOD MouseDblClick(nsIDOMEvent* aMouseEvent) { return NS_OK; }
  NS_IMETHOD MouseOver(nsIDOMEvent* aMouseEvent) { return NS_OK; }
  NS_IMETHOD MouseOut(nsIDOMEvent* aMouseEvent) { return MouseMove(aMouseEvent); }
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent) { return NS_OK; }

  
  NS_IMETHOD MouseMove(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD DragMove(nsIDOMEvent* aMouseEvent) { return NS_OK; }

  void MouseDrag(nsPresContext* aPresContext, nsGUIEvent* aEvent);
  void MouseUp(nsPresContext* aPresContext, nsGUIEvent* aEvent);

  void AdjustChildren(nsPresContext* aPresContext);
  void AdjustChildren(nsPresContext* aPresContext, nsSplitterInfo* aChildInfos, PRInt32 aCount, PRBool aIsHorizontal);

  void AddRemoveSpace(nscoord aDiff,
                    nsSplitterInfo* aChildInfos,
                    PRInt32 aCount,
                    PRInt32& aSpaceLeft);

  void ResizeChildTo(nsPresContext* aPresContext,
                   nscoord& aDiff, 
                   nsSplitterInfo* aChildrenBeforeInfos, 
                   nsSplitterInfo* aChildrenAfterInfos, 
                   PRInt32 aChildrenBeforeCount, 
                   PRInt32 aChildrenAfterCount, 
                   PRBool aBounded);

  void UpdateState();

  void AddListener(nsPresContext* aPresContext);
  void RemoveListener();

  enum ResizeType { Closest, Farthest, Flex, Grow };
  enum State { Open, CollapsedBefore, CollapsedAfter, Dragging };
  enum CollapseDirection { Before, After };

  ResizeType GetResizeBefore();
  ResizeType GetResizeAfter();
  State GetState();

  void Reverse(nsSplitterInfo*& aIndexes, PRInt32 aCount);
  PRBool SupportsCollapseDirection(CollapseDirection aDirection);

  void EnsureOrient();
  void SetPreferredSize(nsBoxLayoutState& aState, nsIBox* aChildBox, nscoord aOnePixel, PRBool aIsHorizontal, nscoord* aSize);

  nsSplitterFrame* mOuter;
  PRBool mDidDrag;
  nscoord mDragStart;
  nscoord mCurrentPos;
  nsIBox* mParentBox;
  PRBool mPressed;
  nsSplitterInfo* mChildInfosBefore;
  nsSplitterInfo* mChildInfosAfter;
  PRInt32 mChildInfosBeforeCount;
  PRInt32 mChildInfosAfterCount;
  State mState;
  nscoord mSplitterPos;
  PRBool mDragging;

};


NS_IMPL_ADDREF(nsSplitterFrameInner)
NS_IMPL_RELEASE(nsSplitterFrameInner)
NS_INTERFACE_MAP_BEGIN(nsSplitterFrameInner)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMouseListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsIDOMEventListener,nsIDOMMouseListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMouseMotionListener)
NS_INTERFACE_MAP_END

nsSplitterFrameInner::ResizeType
nsSplitterFrameInner::GetResizeBefore()
{
  static nsIContent::AttrValuesArray strings[] =
    {&nsGkAtoms::farthest, &nsGkAtoms::flex, nsnull};
  switch (mOuter->GetContent()->FindAttrValueIn(kNameSpaceID_None,
                                                nsGkAtoms::resizebefore,
                                                strings, eCaseMatters)) {
    case 0: return Farthest;
    case 1: return Flex;
  }
  return Closest;
}

nsSplitterFrameInner::~nsSplitterFrameInner() 
{
  delete[] mChildInfosBefore;
  delete[] mChildInfosAfter;
}

nsSplitterFrameInner::ResizeType
nsSplitterFrameInner::GetResizeAfter()
{
  static nsIContent::AttrValuesArray strings[] =
    {&nsGkAtoms::farthest, &nsGkAtoms::flex, &nsGkAtoms::grow, nsnull};
  switch (mOuter->GetContent()->FindAttrValueIn(kNameSpaceID_None,
                                                nsGkAtoms::resizeafter,
                                                strings, eCaseMatters)) {
    case 0: return Farthest;
    case 1: return Flex;
    case 2: return Grow;
  }
  return Closest;
}

nsSplitterFrameInner::State
nsSplitterFrameInner::GetState()
{
  static nsIContent::AttrValuesArray strings[] =
    {&nsGkAtoms::dragging, &nsGkAtoms::collapsed, nsnull};
  static nsIContent::AttrValuesArray strings_substate[] =
    {&nsGkAtoms::before, &nsGkAtoms::after, nsnull};
  switch (mOuter->GetContent()->FindAttrValueIn(kNameSpaceID_None,
                                                nsGkAtoms::state,
                                                strings, eCaseMatters)) {
    case 0: return Dragging;
    case 1:
      switch (mOuter->GetContent()->FindAttrValueIn(kNameSpaceID_None,
                                                    nsGkAtoms::substate,
                                                    strings_substate,
                                                    eCaseMatters)) {
        case 0: return CollapsedBefore;
        case 1: return CollapsedAfter;
        default:
          if (SupportsCollapseDirection(After))
            return CollapsedAfter;
          return CollapsedBefore;
      }
  }
  return Open;
}






nsIFrame*
NS_NewSplitterFrame (nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsSplitterFrame(aPresShell, aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSplitterFrame)

nsSplitterFrame::nsSplitterFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
: nsBoxFrame(aPresShell, aContext),
  mInner(0)
{
}

void
nsSplitterFrame::Destroy()
{
  if (mInner) {
    mInner->RemoveListener();
    mInner->Disconnect();
    mInner->Release();
    mInner = nsnull;
  }
  nsBoxFrame::Destroy();
}


NS_IMETHODIMP
nsSplitterFrame::GetCursor(const nsPoint&    aPoint,
                           nsIFrame::Cursor& aCursor)
{
  return nsBoxFrame::GetCursor(aPoint, aCursor);

  







}

NS_IMETHODIMP
nsSplitterFrame::AttributeChanged(PRInt32 aNameSpaceID,
                                  nsIAtom* aAttribute,
                                  PRInt32 aModType)
{
  nsresult rv = nsBoxFrame::AttributeChanged(aNameSpaceID, aAttribute,
                                             aModType);
  
  if (aAttribute == nsGkAtoms::align) {
    
    
    nsIFrame* grippy = nsnull;
    nsScrollbarButtonFrame::GetChildWithTag(PresContext(), nsGkAtoms::grippy, this, grippy);
    if (grippy)
      grippy->AttributeChanged(aNameSpaceID, aAttribute, aModType);
  } else if (aAttribute == nsGkAtoms::state) {
    mInner->UpdateState();
  }

  return rv;
}




NS_IMETHODIMP
nsSplitterFrame::Init(nsIContent*      aContent,
                      nsIFrame*        aParent,
                      nsIFrame*        aPrevInFlow)
{
  NS_ENSURE_FALSE(mInner, NS_ERROR_ALREADY_INITIALIZED);
  mInner = new nsSplitterFrameInner(this);
  if (!mInner)
    return NS_ERROR_OUT_OF_MEMORY;

  mInner->AddRef();
  mInner->mChildInfosAfter = nsnull;
  mInner->mChildInfosBefore = nsnull;
  mInner->mState = nsSplitterFrameInner::Open;
  mInner->mDragging = PR_FALSE;

  
  
  
  
  if (aParent && aParent->IsBoxFrame()) {
    if (!aParent->IsHorizontal()) {
      if (!nsContentUtils::HasNonEmptyAttr(aContent, kNameSpaceID_None,
                                           nsGkAtoms::orient)) {
        aContent->SetAttr(kNameSpaceID_None, nsGkAtoms::orient,
                          NS_LITERAL_STRING("vertical"), PR_FALSE);
        nsStyleContext* parentStyleContext = GetStyleContext()->GetParent();
        nsRefPtr<nsStyleContext> newContext = PresContext()->StyleSet()->
          ResolveStyleFor(aContent, parentStyleContext);
        SetStyleContextWithoutNotification(newContext);
      }
    }
  }

  nsresult  rv = nsBoxFrame::Init(aContent, aParent, aPrevInFlow);
  NS_ENSURE_SUCCESS(rv, rv);

  mInner->mState = nsSplitterFrameInner::Open;
  mInner->AddListener(PresContext());
  mInner->mParentBox = nsnull;
  return rv;
}

NS_IMETHODIMP
nsSplitterFrame::DoLayout(nsBoxLayoutState& aState)
{
  if (GetStateBits() & NS_FRAME_FIRST_REFLOW) 
  {
    mInner->mParentBox = GetParentBox();
    mInner->UpdateState();
  }

  return nsBoxFrame::DoLayout(aState);
}


void
nsSplitterFrame::GetInitialOrientation(PRBool& aIsHorizontal)
{
  nsIBox* box = GetParentBox();
  if (box) {
    aIsHorizontal = !box->IsHorizontal();
  }
  else
    nsBoxFrame::GetInitialOrientation(aIsHorizontal);
}

NS_IMETHODIMP
nsSplitterFrame::HandlePress(nsPresContext* aPresContext,
                         nsGUIEvent *    aEvent,
                         nsEventStatus*  aEventStatus)
{
  return NS_OK;
}

NS_IMETHODIMP
nsSplitterFrame::HandleMultiplePress(nsPresContext* aPresContext,
                                     nsGUIEvent *    aEvent,
                                     nsEventStatus*  aEventStatus,
                                     PRBool aControlHeld)
{
  return NS_OK;
}

NS_IMETHODIMP
nsSplitterFrame::HandleDrag(nsPresContext* aPresContext,
                        nsGUIEvent *    aEvent,
                        nsEventStatus*  aEventStatus)
{
  return NS_OK;
}

NS_IMETHODIMP
nsSplitterFrame::HandleRelease(nsPresContext* aPresContext,
                           nsGUIEvent *    aEvent,
                           nsEventStatus*  aEventStatus)
{
  return NS_OK;
}

NS_IMETHODIMP
nsSplitterFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                  const nsRect&           aDirtyRect,
                                  const nsDisplayListSet& aLists)
{
  nsresult rv = nsBoxFrame::BuildDisplayList(aBuilder, aDirtyRect, aLists);
  NS_ENSURE_SUCCESS(rv, rv);
  
  
  if (mInner->mDragging)
  {
    
    return aLists.Outlines()->AppendNewToTop(new (aBuilder)
        nsDisplayEventReceiver(this));
  }

  return NS_OK;
}

NS_IMETHODIMP
nsSplitterFrame::HandleEvent(nsPresContext* aPresContext, 
                                      nsGUIEvent* aEvent,
                                      nsEventStatus* aEventStatus)
{
  NS_ENSURE_ARG_POINTER(aEventStatus);
  if (nsEventStatus_eConsumeNoDefault == *aEventStatus) {
    return NS_OK;
  }

  nsWeakFrame weakFrame(this);
  nsRefPtr<nsSplitterFrameInner> kungFuDeathGrip(mInner);
  switch (aEvent->message) {
    case NS_MOUSE_MOVE: 
      mInner->MouseDrag(aPresContext, aEvent);
    break;
  
    case NS_MOUSE_BUTTON_UP:
      if (aEvent->eventStructType == NS_MOUSE_EVENT &&
          static_cast<nsMouseEvent*>(aEvent)->button ==
            nsMouseEvent::eLeftButton) {
        mInner->MouseUp(aPresContext, aEvent);
      }
    break;
  }

  NS_ENSURE_STATE(weakFrame.IsAlive());
  return nsBoxFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
}

void
nsSplitterFrameInner::MouseUp(nsPresContext* aPresContext, nsGUIEvent* aEvent)
{
  if (mDragging && mOuter) {
    AdjustChildren(aPresContext);
    AddListener(aPresContext);
    nsIPresShell::SetCapturingContent(nsnull, 0); 
    mDragging = PR_FALSE;
    State newState = GetState(); 
    
    if (newState == Dragging)
      mOuter->mContent->SetAttr(kNameSpaceID_None, nsGkAtoms::state, EmptyString(), PR_TRUE);

    mPressed = PR_FALSE;

    
    if (mDidDrag) {
      nsCOMPtr<nsIDOMXULElement> element = do_QueryInterface(mOuter->GetContent());
      element->DoCommand();
    }

    
  }

  delete[] mChildInfosBefore;
  delete[] mChildInfosAfter;
  mChildInfosBefore = nsnull;
  mChildInfosAfter = nsnull;
  mChildInfosBeforeCount = 0;
  mChildInfosAfterCount = 0;
}

void
nsSplitterFrameInner::MouseDrag(nsPresContext* aPresContext, nsGUIEvent* aEvent)
{
  if (mDragging && mOuter) {

    

    PRBool isHorizontal = !mOuter->IsHorizontal();
    
    nsPoint pt = nsLayoutUtils::GetEventCoordinatesRelativeTo(aEvent,
                                                              mParentBox);
    nscoord pos = isHorizontal ? pt.x : pt.y;

    
    nscoord start = mDragStart;

    
    pos -= start;

    

    ResizeType resizeAfter  = GetResizeAfter();

    PRBool bounded;

    if (resizeAfter == nsSplitterFrameInner::Grow)
      bounded = PR_FALSE;
    else 
      bounded = PR_TRUE;

    int i;
    for (i=0; i < mChildInfosBeforeCount; i++) 
      mChildInfosBefore[i].changed = mChildInfosBefore[i].current;

    for (i=0; i < mChildInfosAfterCount; i++) 
      mChildInfosAfter[i].changed = mChildInfosAfter[i].current;

    nscoord oldPos = pos;

    ResizeChildTo(aPresContext, pos, mChildInfosBefore, mChildInfosAfter, mChildInfosBeforeCount, mChildInfosAfterCount, bounded);

    State currentState = GetState();
    PRBool supportsBefore = SupportsCollapseDirection(Before);
    PRBool supportsAfter = SupportsCollapseDirection(After);

    
    if ((oldPos > 0 && oldPos > pos && supportsAfter) ||
        (oldPos < 0 && oldPos < pos && supportsBefore))
    {
      
      if (currentState == Dragging) {
        if (oldPos > 0 && oldPos > pos)
        {
          
          if (supportsAfter) 
          {
            nsCOMPtr<nsIContent> outer = mOuter->mContent;
            outer->SetAttr(kNameSpaceID_None, nsGkAtoms::substate,
                           NS_LITERAL_STRING("after"),
                           PR_TRUE);
            outer->SetAttr(kNameSpaceID_None, nsGkAtoms::state,
                           NS_LITERAL_STRING("collapsed"),
                           PR_TRUE);
          }

        } else if (oldPos < 0 && oldPos < pos)
        {
          
          if (supportsBefore)
          {
            nsCOMPtr<nsIContent> outer = mOuter->mContent;
            outer->SetAttr(kNameSpaceID_None, nsGkAtoms::substate,
                           NS_LITERAL_STRING("before"),
                           PR_TRUE);
            outer->SetAttr(kNameSpaceID_None, nsGkAtoms::state,
                           NS_LITERAL_STRING("collapsed"),
                           PR_TRUE);
          }
        }
      }
    } else {
      
      
      if (currentState != Dragging)
        mOuter->mContent->SetAttr(kNameSpaceID_None, nsGkAtoms::state, NS_LITERAL_STRING("dragging"), PR_TRUE);
      AdjustChildren(aPresContext);
    }

    mDidDrag = PR_TRUE;
  }
}

void
nsSplitterFrameInner::AddListener(nsPresContext* aPresContext)
{
  mOuter->GetContent()->
    AddEventListenerByIID(static_cast<nsIDOMMouseListener*>(this),
                          NS_GET_IID(nsIDOMMouseListener));
  mOuter->GetContent()->
    AddEventListenerByIID(static_cast<nsIDOMMouseMotionListener*>(this),
                          NS_GET_IID(nsIDOMMouseMotionListener));
}

void
nsSplitterFrameInner::RemoveListener()
{
  ENSURE_TRUE(mOuter);
  mOuter->GetContent()->
    RemoveEventListenerByIID(static_cast<nsIDOMMouseListener*>(this),
                             NS_GET_IID(nsIDOMMouseListener));
  mOuter->GetContent()->
    RemoveEventListenerByIID(static_cast<nsIDOMMouseMotionListener*>(this),
                             NS_GET_IID(nsIDOMMouseMotionListener));
}

nsresult
nsSplitterFrameInner::MouseUp(nsIDOMEvent* aMouseEvent)
{  
  NS_ENSURE_TRUE(mOuter, NS_OK);
  mPressed = PR_FALSE;

  nsIPresShell::SetCapturingContent(nsnull, 0);

  return NS_OK;
}

nsresult
nsSplitterFrameInner::MouseDown(nsIDOMEvent* aMouseEvent)
{  
  NS_ENSURE_TRUE(mOuter, NS_OK);
  nsCOMPtr<nsIDOMMouseEvent> mouseEvent(do_QueryInterface(aMouseEvent));

  PRUint16 button = 0;
  mouseEvent->GetButton(&button);

  
  if (button != 0)
     return NS_OK;

  if (mOuter->GetContent()->
        AttrValueIs(kNameSpaceID_None, nsGkAtoms::disabled,
                    nsGkAtoms::_true, eCaseMatters))
    return NS_OK;

  mParentBox = mOuter->GetParentBox();
  if (!mParentBox)
    return NS_OK;

  
  nsPresContext* outerPresContext = mOuter->PresContext();
  const nsFrameList& siblingList(mParentBox->GetChildList(nsnull));
  PRInt32 childIndex = siblingList.IndexOf(mOuter);
  
  
  if (childIndex <= 0)
    return NS_OK;

  PRInt32 childCount = siblingList.GetLength();
  
  if (childIndex == childCount - 1 && GetResizeAfter() != Grow)
    return NS_OK;

  nsCOMPtr<nsIRenderingContext> rc;
  nsresult rv = outerPresContext->PresShell()->
                  CreateRenderingContext(mOuter, getter_AddRefs(rc));
  NS_ENSURE_SUCCESS(rv, rv);
  nsBoxLayoutState state(outerPresContext, rc);
  mCurrentPos = 0;
  mPressed = PR_TRUE;

  mDidDrag = PR_FALSE;

  EnsureOrient();
  PRBool isHorizontal = !mOuter->IsHorizontal();
  
  ResizeType resizeBefore = GetResizeBefore();
  ResizeType resizeAfter  = GetResizeAfter();

  delete[] mChildInfosBefore;
  delete[] mChildInfosAfter;
  mChildInfosBefore = new nsSplitterInfo[childCount];
  mChildInfosAfter  = new nsSplitterInfo[childCount];

  
  PRInt32 count = 0;
  mChildInfosBeforeCount = 0;
  mChildInfosAfterCount = 0;

  nsIBox* childBox = mParentBox->GetChildBox();

  while (nsnull != childBox) 
  { 
    nsIContent* content = childBox->GetContent();
    nsIDocument* doc = content->GetOwnerDoc();
    nsIAtom* atom;
    if (doc) {
      PRInt32 dummy;
      atom = doc->BindingManager()->ResolveTag(content, &dummy);
    } else {
      atom = content->Tag();
    }

    
    if (atom != nsGkAtoms::splitter) { 
        nsSize prefSize = childBox->GetPrefSize(state);
        nsSize minSize = childBox->GetMinSize(state);
        nsSize maxSize = nsBox::BoundsCheckMinMax(minSize, childBox->GetMaxSize(state));
        prefSize = nsBox::BoundsCheck(minSize, prefSize, maxSize);

        mOuter->AddMargin(childBox, minSize);
        mOuter->AddMargin(childBox, prefSize);
        mOuter->AddMargin(childBox, maxSize);

        nscoord flex = childBox->GetFlex(state);

        nsMargin margin(0,0,0,0);
        childBox->GetMargin(margin);
        nsRect r(childBox->GetRect());
        r.Inflate(margin);

        
        
        if (!content->AttrValueIs(kNameSpaceID_None, nsGkAtoms::fixed,
                                  nsGkAtoms::_true, eCaseMatters) &&
            !content->AttrValueIs(kNameSpaceID_None, nsGkAtoms::hidden,
                                  nsGkAtoms::_true, eCaseMatters)) {
            if (count < childIndex && (resizeBefore != Flex || flex > 0)) {
                mChildInfosBefore[mChildInfosBeforeCount].childElem = content;
                mChildInfosBefore[mChildInfosBeforeCount].min     = isHorizontal ? minSize.width : minSize.height;
                mChildInfosBefore[mChildInfosBeforeCount].max     = isHorizontal ? maxSize.width : maxSize.height;
                mChildInfosBefore[mChildInfosBeforeCount].current = isHorizontal ? r.width : r.height;
                mChildInfosBefore[mChildInfosBeforeCount].flex    = flex;
                mChildInfosBefore[mChildInfosBeforeCount].index   = count;
                mChildInfosBefore[mChildInfosBeforeCount].changed = mChildInfosBefore[mChildInfosBeforeCount].current;
                mChildInfosBeforeCount++;
            } else if (count > childIndex && (resizeAfter != Flex || flex > 0)) {
                mChildInfosAfter[mChildInfosAfterCount].childElem = content;
                mChildInfosAfter[mChildInfosAfterCount].min     = isHorizontal ? minSize.width : minSize.height;
                mChildInfosAfter[mChildInfosAfterCount].max     = isHorizontal ? maxSize.width : maxSize.height;
                mChildInfosAfter[mChildInfosAfterCount].current = isHorizontal ? r.width : r.height;
                mChildInfosAfter[mChildInfosAfterCount].flex    = flex;
                mChildInfosAfter[mChildInfosAfterCount].index   = count;
                mChildInfosAfter[mChildInfosAfterCount].changed = mChildInfosAfter[mChildInfosAfterCount].current;
                mChildInfosAfterCount++;
            }
        } 
    }
    
    childBox = childBox->GetNextBox();
    count++;
  }

  if (!mParentBox->IsNormalDirection()) {
    
    
    Reverse(mChildInfosBefore, mChildInfosBeforeCount);
    Reverse(mChildInfosAfter, mChildInfosAfterCount);

    
    nscoord newAfterCount = mChildInfosBeforeCount;
    mChildInfosBeforeCount = mChildInfosAfterCount;
    mChildInfosAfterCount = newAfterCount;
    nsSplitterInfo* temp = mChildInfosAfter;
    mChildInfosAfter = mChildInfosBefore;
    mChildInfosBefore = temp;
  }

  
  
  if (resizeBefore != Farthest)
     Reverse(mChildInfosBefore, mChildInfosBeforeCount);

  
  
  if (resizeAfter == Farthest)
     Reverse(mChildInfosAfter, mChildInfosAfterCount);

  
  
  if (resizeAfter == Grow)
     mChildInfosAfterCount = 0;

  PRInt32 c;
  nsPoint pt = nsLayoutUtils::GetDOMEventCoordinatesRelativeTo(mouseEvent,
                                                               mParentBox);
  if (isHorizontal) {
     c = pt.x;
     mSplitterPos = mOuter->mRect.x;
  } else {
     c = pt.y;
     mSplitterPos = mOuter->mRect.y;
  }

  mDragStart = c;

  

  nsIPresShell::SetCapturingContent(mOuter->GetContent(), CAPTURE_IGNOREALLOWED);

  return NS_OK;
}

nsresult
nsSplitterFrameInner::MouseMove(nsIDOMEvent* aMouseEvent)
{  
  NS_ENSURE_TRUE(mOuter, NS_OK);
  if (!mPressed)
    return NS_OK;

  if (mDragging)
    return NS_OK;

  nsCOMPtr<nsIDOMMouseListener> kungfuDeathGrip(this);
  mOuter->mContent->SetAttr(kNameSpaceID_None, nsGkAtoms::state, NS_LITERAL_STRING("dragging"), PR_TRUE);

  RemoveListener();
  mDragging = PR_TRUE;

  return NS_OK;
}

void
nsSplitterFrameInner::Reverse(nsSplitterInfo*& aChildInfos, PRInt32 aCount)
{
    nsSplitterInfo* infos = new nsSplitterInfo[aCount];

    for (int i=0; i < aCount; i++)
       infos[i] = aChildInfos[aCount - 1 - i];

    delete[] aChildInfos;
    aChildInfos = infos;
}

PRBool
nsSplitterFrameInner::SupportsCollapseDirection
(
  nsSplitterFrameInner::CollapseDirection aDirection
)
{
  static nsIContent::AttrValuesArray strings[] =
    {&nsGkAtoms::before, &nsGkAtoms::after, &nsGkAtoms::both, nsnull};

  switch (mOuter->mContent->FindAttrValueIn(kNameSpaceID_None,
                                            nsGkAtoms::collapse,
                                            strings, eCaseMatters)) {
    case 0:
      return (aDirection == Before);
    case 1:
      return (aDirection == After);
    case 2:
      return PR_TRUE;
  }

  return PR_FALSE;
}

void
nsSplitterFrameInner::UpdateState()
{
  
  
  
  
  
  
  
  
  
  
  

  State newState = GetState(); 

  if (newState == mState) {
    
    return;
  }

  if ((SupportsCollapseDirection(Before) || SupportsCollapseDirection(After)) &&
      mOuter->GetParent()->IsBoxFrame()) {
    
    nsIFrame* splitterSibling;
    if (newState == CollapsedBefore || mState == CollapsedBefore) {
      splitterSibling =
        mOuter->GetParent()->GetChildList(nsnull).GetPrevSiblingFor(mOuter);
    } else {
      splitterSibling = mOuter->GetNextSibling();
    }

    if (splitterSibling) {
      nsCOMPtr<nsIContent> sibling = splitterSibling->GetContent();
      if (sibling) {
        if (mState == CollapsedBefore || mState == CollapsedAfter) {
          
          
          
          
          nsContentUtils::AddScriptRunner(
            new nsUnsetAttrRunnable(sibling, nsGkAtoms::collapsed));
        } else if ((mState == Open || mState == Dragging)
                   && (newState == CollapsedBefore ||
                       newState == CollapsedAfter)) {
          
          
          nsContentUtils::AddScriptRunner(
            new nsSetAttrRunnable(sibling, nsGkAtoms::collapsed,
                                  NS_LITERAL_STRING("true")));
        }
      }
    }
  }
  mState = newState;
}

void
nsSplitterFrameInner::EnsureOrient()
{
  PRBool isHorizontal = !(mParentBox->GetStateBits() & NS_STATE_IS_HORIZONTAL);
  if (isHorizontal)
    mOuter->mState |= NS_STATE_IS_HORIZONTAL;
  else
    mOuter->mState &= ~NS_STATE_IS_HORIZONTAL;
}

void
nsSplitterFrameInner::AdjustChildren(nsPresContext* aPresContext)
{
  EnsureOrient();
  PRBool isHorizontal = !mOuter->IsHorizontal();

  AdjustChildren(aPresContext, mChildInfosBefore, mChildInfosBeforeCount, isHorizontal);
  AdjustChildren(aPresContext, mChildInfosAfter, mChildInfosAfterCount, isHorizontal);
   
   

  aPresContext->PresShell()->FlushPendingNotifications(Flush_Display);
}

static nsIBox* GetChildBoxForContent(nsIBox* aParentBox, nsIContent* aContent)
{
  nsIBox* childBox = aParentBox->GetChildBox();

  while (nsnull != childBox) {
    if (childBox->GetContent() == aContent) {
      return childBox;
    }
    childBox = childBox->GetNextBox();
  }
  return nsnull;
}

void
nsSplitterFrameInner::AdjustChildren(nsPresContext* aPresContext, nsSplitterInfo* aChildInfos, PRInt32 aCount, PRBool aIsHorizontal)
{
  

  nsBoxLayoutState state(aPresContext);

  nscoord onePixel = nsPresContext::CSSPixelsToAppUnits(1);

  
  nsIBox* child =  mOuter->GetChildBox();
  while(child)
  {
    SetPreferredSize(state, child, onePixel, aIsHorizontal, nsnull);
    child = child->GetNextBox();
  }

  
  for (int i=0; i < aCount; i++) 
  {
    nscoord   pref       = aChildInfos[i].changed;
    nsIBox* childBox     = GetChildBoxForContent(mParentBox, aChildInfos[i].childElem);

    if (childBox) {
      SetPreferredSize(state, childBox, onePixel, aIsHorizontal, &pref);
    }
  }
}

void
nsSplitterFrameInner::SetPreferredSize(nsBoxLayoutState& aState, nsIBox* aChildBox, nscoord aOnePixel, PRBool aIsHorizontal, nscoord* aSize)
{
  
 
  nscoord current = 0;

  nsRect rect(aChildBox->GetRect());
  if (aIsHorizontal) 
    current = rect.width;
  else
    current = rect.height;

  nscoord pref = 0;

  if (!aSize)
  {
    if (aIsHorizontal) 
      pref = rect.width;
    else
      pref = rect.height;
  } else {
    pref = *aSize;
  }

  nsMargin margin(0,0,0,0);
  aChildBox->GetMargin(margin);

  nsCOMPtr<nsIAtom> attribute;

  if (aIsHorizontal) {
    pref -= (margin.left + margin.right);
    attribute = nsGkAtoms::width;
  } else {
    pref -= (margin.top + margin.bottom);
    attribute = nsGkAtoms::height;
  }

  nsIContent* content = aChildBox->GetContent();

  
  nsAutoString prefValue;
  prefValue.AppendInt(pref/aOnePixel);
  if (content->AttrValueIs(kNameSpaceID_None, attribute,
                           prefValue, eCaseMatters))
     return;

  nsWeakFrame weakBox(aChildBox);
  content->SetAttr(kNameSpaceID_None, attribute, prefValue, PR_TRUE);
  ENSURE_TRUE(weakBox.IsAlive());
  aState.PresShell()->FrameNeedsReflow(aChildBox, nsIPresShell::eStyleChange,
                                       NS_FRAME_IS_DIRTY);
}


void 
nsSplitterFrameInner::AddRemoveSpace(nscoord aDiff,
                                    nsSplitterInfo* aChildInfos,
                                    PRInt32 aCount,
                                    PRInt32& aSpaceLeft)
{
  aSpaceLeft = 0;

  for (int i=0; i < aCount; i++) {   
    nscoord min    = aChildInfos[i].min;
    nscoord max    = aChildInfos[i].max;
    nscoord& c     = aChildInfos[i].changed;

    
    if (c + aDiff < min) {
      aDiff += (c - min);
      c = min;
    } else if (c + aDiff > max) {
      aDiff -= (max - c);
      c = max;
    } else {
      c += aDiff;
      aDiff = 0;
    }

    
    if (aDiff == 0)
      break;
  }

  aSpaceLeft = aDiff;
}








void
nsSplitterFrameInner::ResizeChildTo(nsPresContext* aPresContext,
                                   nscoord& aDiff, 
                                   nsSplitterInfo* aChildrenBeforeInfos, 
                                   nsSplitterInfo* aChildrenAfterInfos, 
                                   PRInt32 aChildrenBeforeCount, 
                                   PRInt32 aChildrenAfterCount, 
                                   PRBool aBounded)
{ 
  nscoord spaceLeft;
  AddRemoveSpace(aDiff, aChildrenBeforeInfos,aChildrenBeforeCount,spaceLeft);

  
  aDiff -= spaceLeft;
  AddRemoveSpace(-aDiff, aChildrenAfterInfos,aChildrenAfterCount,spaceLeft);

  if (spaceLeft != 0) {
    if (aBounded) {
       aDiff += spaceLeft;
       AddRemoveSpace(spaceLeft, aChildrenBeforeInfos,aChildrenBeforeCount,spaceLeft);
    } else {
      spaceLeft = 0;
    }
  }
}
