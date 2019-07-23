
















































#include "nsCOMPtr.h"
#include "nsISupportsArray.h"
#include "nsPresContext.h"
#include "nsINameSpaceManager.h"

#include "nsTreeBodyFrame.h"
#include "nsTreeSelection.h"

#include "nsGkAtoms.h"
#include "nsCSSAnonBoxes.h"

#include "nsIContent.h"
#include "nsStyleContext.h"
#include "nsIBoxObject.h"
#include "nsGUIEvent.h"
#include "nsPLDOMEvent.h"
#include "nsIDOMDataContainerEvent.h"
#include "nsIDOMMouseEvent.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIDOMElement.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMDocument.h"
#include "nsIDOMNSDocument.h"
#include "nsIDOMDocumentEvent.h"
#include "nsIDOMXULElement.h"
#include "nsIDocument.h"
#include "nsIContent.h"
#include "nsICSSStyleRule.h"
#include "nsCSSRendering.h"
#include "nsIFontMetrics.h"
#include "nsIDeviceContext.h"
#include "nsIXULTemplateBuilder.h"
#include "nsXPIDLString.h"
#include "nsHTMLContainerFrame.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsWidgetsCID.h"
#include "nsBoxFrame.h"
#include "nsBoxObject.h"
#include "nsIURL.h"
#include "nsNetUtil.h"
#include "nsBoxLayoutState.h"
#include "nsIDragService.h"
#include "nsTreeContentView.h"
#include "nsTreeUtils.h"
#include "nsChildIterator.h"
#include "nsIScrollableView.h"
#include "nsITheme.h"
#include "nsITimelineService.h"
#include "imgIRequest.h"
#include "imgIContainer.h"
#include "imgIContainerObserver.h"
#include "imgILoader.h"
#include "nsINodeInfo.h"
#include "nsContentUtils.h"
#include "nsLayoutUtils.h"
#include "nsIScrollableFrame.h"
#include "nsEventDispatcher.h"
#include "nsDisplayList.h"
#include "nsTreeBoxObject.h"

#ifdef IBMBIDI
#include "nsBidiPresUtils.h"
#endif

static NS_DEFINE_CID(kWidgetCID, NS_CHILD_CID);


static PLDHashOperator
CancelImageRequest(const nsAString& aKey,
                   nsTreeImageCacheEntry aEntry, void* aData)
{
  aEntry.request->CancelAndForgetObserver(NS_BINDING_ABORTED);
  return PL_DHASH_NEXT;
}






nsIFrame*
NS_NewTreeBodyFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsTreeBodyFrame(aPresShell, aContext);
} 


NS_QUERYFRAME_HEAD(nsTreeBodyFrame)
  NS_QUERYFRAME_ENTRY(nsICSSPseudoComparator)
  NS_QUERYFRAME_ENTRY(nsIScrollbarMediator)
  NS_QUERYFRAME_ENTRY(nsTreeBodyFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsLeafBoxFrame)


nsTreeBodyFrame::nsTreeBodyFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
:nsLeafBoxFrame(aPresShell, aContext),
 mSlots(nsnull),
 mTopRowIndex(0),
 mHorzPosition(0),
 mHorzWidth(0),
 mAdjustWidth(0),
 mRowHeight(0),
 mIndentation(0),
 mStringWidth(-1),
 mUpdateBatchNest(0),
 mRowCount(0),
 mMouseOverRow(-1),
 mFocused(PR_FALSE),
 mHasFixedRowCount(PR_FALSE),
 mVerticalOverflow(PR_FALSE),
 mHorizontalOverflow(PR_FALSE),
 mReflowCallbackPosted(PR_FALSE)
{
  mColumns = new nsTreeColumns(nsnull);
  NS_NewISupportsArray(getter_AddRefs(mScratchArray));
}


nsTreeBodyFrame::~nsTreeBodyFrame()
{
  mImageCache.EnumerateRead(CancelImageRequest, nsnull);
  delete mSlots;
}

static void
GetBorderPadding(nsStyleContext* aContext, nsMargin& aMargin)
{
  aMargin.SizeTo(0, 0, 0, 0);
  if (!aContext->GetStylePadding()->GetPadding(aMargin)) {
    NS_NOTYETIMPLEMENTED("percentage padding");
  }
  aMargin += aContext->GetStyleBorder()->GetActualBorder();
}

static void
AdjustForBorderPadding(nsStyleContext* aContext, nsRect& aRect)
{
  nsMargin borderPadding(0, 0, 0, 0);
  GetBorderPadding(aContext, borderPadding);
  aRect.Deflate(borderPadding);
}

NS_IMETHODIMP
nsTreeBodyFrame::Init(nsIContent*     aContent,
                      nsIFrame*       aParent,
                      nsIFrame*       aPrevInFlow)
{
  nsresult rv = nsLeafBoxFrame::Init(aContent, aParent, aPrevInFlow);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = nsBoxFrame::CreateViewForFrame(PresContext(), this, GetStyleContext(), PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  nsIView* view = nsLeafBoxFrame::GetView();
  if (!view->HasWidget()) {
    view->CreateWidget(kWidgetCID);
  }

  mIndentation = GetIndentation();
  mRowHeight = GetRowHeight();

  NS_ENSURE_TRUE(mImageCache.Init(16), NS_ERROR_OUT_OF_MEMORY);
  EnsureBoxObject();
  return rv;
}

nsSize
nsTreeBodyFrame::GetMinSize(nsBoxLayoutState& aBoxLayoutState)
{
  EnsureView();

  nsIContent* baseElement = GetBaseElement();

  nsSize min(0,0);
  PRInt32 desiredRows;
  if (NS_UNLIKELY(!baseElement)) {
    desiredRows = 0;
  }
  else if (baseElement->Tag() == nsGkAtoms::select &&
           baseElement->IsNodeOfType(nsINode::eHTML)) {
    min.width = CalcMaxRowWidth();
    nsAutoString size;
    baseElement->GetAttr(kNameSpaceID_None, nsGkAtoms::size, size);
    if (!size.IsEmpty()) {
      PRInt32 err;
      desiredRows = size.ToInteger(&err);
      mHasFixedRowCount = PR_TRUE;
      mPageLength = desiredRows;
    }
    else {
      desiredRows = 1;
    }
  }
  else {
    
    nsAutoString rows;
    baseElement->GetAttr(kNameSpaceID_None, nsGkAtoms::rows, rows);
    if (!rows.IsEmpty()) {
      PRInt32 err;
      desiredRows = rows.ToInteger(&err);
      mPageLength = desiredRows;
    }
    else {
      desiredRows = 0;
    }
  }

  min.height = mRowHeight * desiredRows;

  AddBorderAndPadding(min);
  nsIBox::AddCSSMinSize(aBoxLayoutState, this, min);

  return min;
}

nscoord
nsTreeBodyFrame::CalcMaxRowWidth()
{
  if (mStringWidth != -1)
    return mStringWidth;

  if (!mView)
    return 0;

  nsStyleContext* rowContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreerow);
  nsMargin rowMargin(0,0,0,0);
  GetBorderPadding(rowContext, rowMargin);

  nscoord rowWidth;
  nsTreeColumn* col;

  nsCOMPtr<nsIRenderingContext> rc;
  PresContext()->PresShell()->CreateRenderingContext(this, getter_AddRefs(rc));

  for (PRInt32 row = 0; row < mRowCount; ++row) {
    rowWidth = 0;

    for (col = mColumns->GetFirstColumn(); col; col = col->GetNext()) {
      nscoord desiredWidth, currentWidth;
      nsresult rv = GetCellWidth(row, col, rc, desiredWidth, currentWidth);
      if (NS_FAILED(rv)) {
        NS_NOTREACHED("invalid column");
        continue;
      }
      rowWidth += desiredWidth;
    }

    if (rowWidth > mStringWidth)
      mStringWidth = rowWidth;
  }

  mStringWidth += rowMargin.left + rowMargin.right;
  return mStringWidth;
}

void
nsTreeBodyFrame::Destroy()
{
  mScrollEvent.Revoke();
  
  if (mReflowCallbackPosted) {
    PresContext()->PresShell()->CancelReflowCallback(this);
    mReflowCallbackPosted = PR_FALSE;
  }

  if (mColumns)
    mColumns->SetTree(nsnull);

  
  nsCOMPtr<nsPIBoxObject> box(do_QueryInterface(mTreeBoxObject));
  if (box) {
    if (mTopRowIndex > 0) {
      nsAutoString topRowStr; topRowStr.AssignLiteral("topRow");
      nsAutoString topRow;
      topRow.AppendInt(mTopRowIndex);
      box->SetProperty(topRowStr.get(), topRow.get());
    }

    
    box->ClearCachedValues();

    mTreeBoxObject = nsnull; 
  }

  if (mView) {
    nsCOMPtr<nsITreeSelection> sel;
    mView->GetSelection(getter_AddRefs(sel));
    if (sel)
      sel->SetTree(nsnull);
    mView->SetTree(nsnull);
    mView = nsnull;
  }

  nsLeafBoxFrame::Destroy();
}

void
nsTreeBodyFrame::EnsureBoxObject()
{
  if (!mTreeBoxObject) {
    nsIContent* parent = GetBaseElement();
    if (parent) {
      nsIDocument* nsDoc = parent->GetDocument();
      if (!nsDoc) 
        return;
      nsCOMPtr<nsIBoxObject> box;
      nsCOMPtr<nsIDOMElement> domElem = do_QueryInterface(parent);
      nsDoc->GetBoxObjectFor(domElem, getter_AddRefs(box));
      
      nsCOMPtr<nsPIBoxObject> pBox = do_QueryInterface(box);
      if (pBox) {
        nsCOMPtr<nsITreeBoxObject> realTreeBoxObject = do_QueryInterface(pBox);
        if (realTreeBoxObject) {
          nsTreeBodyFrame* innerTreeBoxObject =
            static_cast<nsTreeBoxObject*>(realTreeBoxObject.get())
              ->GetCachedTreeBody();
          ENSURE_TRUE(!innerTreeBoxObject || innerTreeBoxObject == this);
          mTreeBoxObject = realTreeBoxObject;
          mColumns->SetTree(mTreeBoxObject);
        }
      }
    }
  }
}

void
nsTreeBodyFrame::EnsureView()
{
  if (!mView) {
    PRBool isInReflow;
    PresContext()->PresShell()->IsReflowLocked(&isInReflow);
    if (isInReflow) {
      if (!mReflowCallbackPosted) {
        mReflowCallbackPosted = PR_TRUE;
        PresContext()->PresShell()->PostReflowCallback(this);
      }
      return;
    }
    nsCOMPtr<nsIBoxObject> box = do_QueryInterface(mTreeBoxObject);
    if (box) {
      nsWeakFrame weakFrame(this);
      nsCOMPtr<nsITreeView> treeView;
      mTreeBoxObject->GetView(getter_AddRefs(treeView));
      if (treeView && weakFrame.IsAlive()) {
        nsXPIDLString rowStr;
        box->GetProperty(NS_LITERAL_STRING("topRow").get(),
                         getter_Copies(rowStr));
        nsAutoString rowStr2(rowStr);
        PRInt32 error;
        PRInt32 rowIndex = rowStr2.ToInteger(&error);

        
        SetView(treeView);
        ENSURE_TRUE(weakFrame.IsAlive());

        
        
        ScrollToRow(rowIndex);

        
        
        box->RemoveProperty(NS_LITERAL_STRING("topRow").get());
      }
    }
  }
}

void
nsTreeBodyFrame::SetBounds(nsBoxLayoutState& aBoxLayoutState, const nsRect& aRect,
                           PRBool aRemoveOverflowArea)
{
  nscoord horzWidth = CalcHorzWidth(GetScrollParts());
  if ((aRect != mRect || mHorzWidth != horzWidth) && !mReflowCallbackPosted) {
    mReflowCallbackPosted = PR_TRUE;
    PresContext()->PresShell()->PostReflowCallback(this);
  }

  mHorzWidth = horzWidth;

  nsLeafBoxFrame::SetBounds(aBoxLayoutState, aRect, aRemoveOverflowArea);
}


PRBool
nsTreeBodyFrame::ReflowFinished()
{
  if (!mView) {
    nsWeakFrame weakFrame(this);
    EnsureView();
    NS_ENSURE_TRUE(weakFrame.IsAlive(), PR_FALSE);
  }
  if (mView) {
    CalcInnerBox();
    ScrollParts parts = GetScrollParts();
    mHorzWidth = CalcHorzWidth(parts);
    if (!mHasFixedRowCount) {
      mPageLength = mInnerBox.height / mRowHeight;
    }

    PRInt32 lastPageTopRow = PR_MAX(0, mRowCount - mPageLength);
    if (mTopRowIndex > lastPageTopRow)
      ScrollToRowInternal(parts, lastPageTopRow);

    nsIContent *treeContent = GetBaseElement();
    if (treeContent->AttrValueIs(kNameSpaceID_None,
                                 nsGkAtoms::keepcurrentinview,
                                 nsGkAtoms::_true, eCaseMatters)) {
      
      
      nsCOMPtr<nsITreeSelection> sel;
      mView->GetSelection(getter_AddRefs(sel));
      if (sel) {
        PRInt32 currentIndex;
        sel->GetCurrentIndex(&currentIndex);
        if (currentIndex != -1)
          EnsureRowIsVisibleInternal(parts, currentIndex);
      }
    }

    if (!FullScrollbarsUpdate(PR_FALSE)) {
      return PR_FALSE;
    }
  }

  mReflowCallbackPosted = PR_FALSE;
  return PR_FALSE;
}

void
nsTreeBodyFrame::ReflowCallbackCanceled()
{
  mReflowCallbackPosted = PR_FALSE;
}

nsresult
nsTreeBodyFrame::GetView(nsITreeView * *aView)
{
  *aView = nsnull;
  nsWeakFrame weakFrame(this);
  EnsureView();
  NS_ENSURE_STATE(weakFrame.IsAlive());
  NS_IF_ADDREF(*aView = mView);
  return NS_OK;
}

nsresult
nsTreeBodyFrame::SetView(nsITreeView * aView)
{
  
  if (mView) {
    nsCOMPtr<nsITreeSelection> sel;
    mView->GetSelection(getter_AddRefs(sel));
    if (sel)
      sel->SetTree(nsnull);
    mView->SetTree(nsnull);

    
    mTopRowIndex = 0;
  }

  
  mView = aView;
 
  
  
  Invalidate();
 
  nsIContent *treeContent = GetBaseElement();
  if (treeContent) {
    FireDOMEvent(NS_LITERAL_STRING("TreeViewChanged"), treeContent);
  }

  if (mView) {
    
    
    nsCOMPtr<nsITreeSelection> sel;
    mView->GetSelection(getter_AddRefs(sel));
    if (sel) {
      sel->SetTree(mTreeBoxObject);
    }
    else {
      NS_NewTreeSelection(mTreeBoxObject, getter_AddRefs(sel));
      mView->SetSelection(sel);
    }

    
    nsWeakFrame weakFrame(this);
    mView->SetTree(mTreeBoxObject);
    NS_ENSURE_STATE(weakFrame.IsAlive());
    mView->GetRowCount(&mRowCount);
 
    PRBool isInReflow;
    PresContext()->PresShell()->IsReflowLocked(&isInReflow);
    if (!isInReflow) {
      
      FullScrollbarsUpdate(PR_FALSE);
    } else if (!mReflowCallbackPosted) {
      mReflowCallbackPosted = PR_TRUE;
      PresContext()->PresShell()->PostReflowCallback(this);
    }
  }
 
  return NS_OK;
}

nsresult
nsTreeBodyFrame::GetFocused(PRBool* aFocused)
{
  *aFocused = mFocused;
  return NS_OK;
}

nsresult
nsTreeBodyFrame::SetFocused(PRBool aFocused)
{
  if (mFocused != aFocused) {
    mFocused = aFocused;
    if (mView) {
      nsCOMPtr<nsITreeSelection> sel;
      mView->GetSelection(getter_AddRefs(sel));
      if (sel)
        sel->InvalidateSelection();
    }
  }
  return NS_OK;
}

nsresult
nsTreeBodyFrame::GetTreeBody(nsIDOMElement** aElement)
{
  
  if (!mContent)
    return NS_ERROR_NULL_POINTER;

  return mContent->QueryInterface(NS_GET_IID(nsIDOMElement), (void**)aElement);
}

nsresult
nsTreeBodyFrame::GetColumns(nsITreeColumns** aColumns)
{
  NS_IF_ADDREF(*aColumns = mColumns);
  return NS_OK;
}

nsresult
nsTreeBodyFrame::GetRowHeight(PRInt32* _retval)
{
  *_retval = nsPresContext::AppUnitsToIntCSSPixels(mRowHeight);
  return NS_OK;
}

nsresult
nsTreeBodyFrame::GetRowWidth(PRInt32 *aRowWidth)
{
  *aRowWidth = nsPresContext::AppUnitsToIntCSSPixels(CalcHorzWidth(GetScrollParts()));
  return NS_OK;
}

nsresult
nsTreeBodyFrame::GetFirstVisibleRow(PRInt32 *_retval)
{
  *_retval = mTopRowIndex;
  return NS_OK;
}

nsresult
nsTreeBodyFrame::GetLastVisibleRow(PRInt32 *_retval)
{
  *_retval = GetLastVisibleRow();
  return NS_OK;
}

nsresult
nsTreeBodyFrame::GetHorizontalPosition(PRInt32 *aHorizontalPosition)
{
  *aHorizontalPosition = nsPresContext::AppUnitsToIntCSSPixels(mHorzPosition); 
  return NS_OK;
}

nsresult
nsTreeBodyFrame::GetPageLength(PRInt32 *_retval)
{
  *_retval = mPageLength;
  return NS_OK;
}

nsresult
nsTreeBodyFrame::GetSelectionRegion(nsIScriptableRegion **aRegion)
{
  *aRegion = nsnull;

  nsCOMPtr<nsITreeSelection> selection;
  mView->GetSelection(getter_AddRefs(selection));
  NS_ENSURE_TRUE(selection, NS_OK);

  nsCOMPtr<nsIScriptableRegion> region = do_CreateInstance("@mozilla.org/gfx/region;1");
  NS_ENSURE_TRUE(region, NS_ERROR_FAILURE);
  region->Init();

  nsRefPtr<nsPresContext> presContext = PresContext();
  nsIntRect rect = mRect.ToOutsidePixels(presContext->AppUnitsPerCSSPixel());

  nsIFrame* rootFrame = presContext->PresShell()->GetRootFrame();
  nsPoint origin = GetOffsetTo(rootFrame);

  
  
  PRInt32 x = nsPresContext::AppUnitsToIntCSSPixels(origin.x);
  PRInt32 y = nsPresContext::AppUnitsToIntCSSPixels(origin.y);
  PRInt32 top = y;
  PRInt32 end = GetLastVisibleRow();
  PRInt32 rowHeight = nsPresContext::AppUnitsToIntCSSPixels(mRowHeight);
  for (PRInt32 i = mTopRowIndex; i <= end; i++) {
    PRBool isSelected;
    selection->IsSelected(i, &isSelected);
    if (isSelected)
      region->UnionRect(x, y, rect.width, rowHeight);
    y += rowHeight;
  }

  
  region->IntersectRect(x, top, rect.width, rect.height);

  NS_ADDREF(*aRegion = region);
  return NS_OK;
}

nsresult
nsTreeBodyFrame::Invalidate()
{
  if (mUpdateBatchNest)
    return NS_OK;

  InvalidateOverflowRect();

  return NS_OK;
}

nsresult
nsTreeBodyFrame::InvalidateColumn(nsITreeColumn* aCol)
{
  if (mUpdateBatchNest)
    return NS_OK;

  nsRefPtr<nsTreeColumn> col = GetColumnImpl(aCol);
  if (!col)
    return NS_ERROR_INVALID_ARG;

#ifdef ACCESSIBILITY
  nsIPresShell *presShell = PresContext()->PresShell();
  if (presShell->IsAccessibilityActive())
    FireInvalidateEvent(-1, -1, aCol, aCol);
#endif

  nsRect columnRect;
  nsresult rv = col->GetRect(this, mInnerBox.y, mInnerBox.height, &columnRect);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (OffsetForHorzScroll(columnRect, PR_TRUE))
      nsIFrame::Invalidate(columnRect);

  return NS_OK;
}

nsresult
nsTreeBodyFrame::InvalidateRow(PRInt32 aIndex)
{
  if (mUpdateBatchNest)
    return NS_OK;

#ifdef ACCESSIBILITY
  nsIPresShell *presShell = PresContext()->PresShell();
  if (presShell->IsAccessibilityActive())
    FireInvalidateEvent(aIndex, aIndex, nsnull, nsnull);
#endif

  aIndex -= mTopRowIndex;
  if (aIndex < 0 || aIndex > mPageLength)
    return NS_OK;

  nsRect rowRect(mInnerBox.x, mInnerBox.y+mRowHeight*aIndex, mInnerBox.width, mRowHeight);
  nsLeafBoxFrame::Invalidate(rowRect);

  return NS_OK;
}

nsresult
nsTreeBodyFrame::InvalidateCell(PRInt32 aIndex, nsITreeColumn* aCol)
{
  if (mUpdateBatchNest)
    return NS_OK;

#ifdef ACCESSIBILITY
  nsIPresShell *presShell = PresContext()->PresShell();
  if (presShell->IsAccessibilityActive())
    FireInvalidateEvent(aIndex, aIndex, aCol, aCol);
#endif

  aIndex -= mTopRowIndex;
  if (aIndex < 0 || aIndex > mPageLength)
    return NS_OK;

  nsRefPtr<nsTreeColumn> col = GetColumnImpl(aCol);
  if (!col)
    return NS_ERROR_INVALID_ARG;

  nsRect cellRect;
  nsresult rv = col->GetRect(this, mInnerBox.y+mRowHeight*aIndex, mRowHeight,
                             &cellRect);
  NS_ENSURE_SUCCESS(rv, rv);

  if (OffsetForHorzScroll(cellRect, PR_TRUE))
    nsIFrame::Invalidate(cellRect);

  return NS_OK;
}

nsresult
nsTreeBodyFrame::InvalidateRange(PRInt32 aStart, PRInt32 aEnd)
{
  if (mUpdateBatchNest)
    return NS_OK;

  if (aStart == aEnd)
    return InvalidateRow(aStart);

  PRInt32 last = GetLastVisibleRow();
  if (aStart > aEnd || aEnd < mTopRowIndex || aStart > last)
    return NS_OK;

  if (aStart < mTopRowIndex)
    aStart = mTopRowIndex;

  if (aEnd > last)
    aEnd = last;

#ifdef ACCESSIBILITY
  nsIPresShell *presShell = PresContext()->PresShell();
  if (presShell->IsAccessibilityActive()) {
    PRInt32 end =
      mRowCount > 0 ? ((mRowCount <= aEnd) ? mRowCount - 1 : aEnd) : 0;
    FireInvalidateEvent(aStart, end, nsnull, nsnull);
  }
#endif

  nsRect rangeRect(mInnerBox.x, mInnerBox.y+mRowHeight*(aStart-mTopRowIndex), mInnerBox.width, mRowHeight*(aEnd-aStart+1));
  nsIFrame::Invalidate(rangeRect);

  return NS_OK;
}

nsresult
nsTreeBodyFrame::InvalidateColumnRange(PRInt32 aStart, PRInt32 aEnd, nsITreeColumn* aCol)
{
  if (mUpdateBatchNest)
    return NS_OK;

  nsRefPtr<nsTreeColumn> col = GetColumnImpl(aCol);
  if (!col)
    return NS_ERROR_INVALID_ARG;

  if (aStart == aEnd)
    return InvalidateCell(aStart, col);

  PRInt32 last = GetLastVisibleRow();
  if (aStart > aEnd || aEnd < mTopRowIndex || aStart > last)
    return NS_OK;

  if (aStart < mTopRowIndex)
    aStart = mTopRowIndex;

  if (aEnd > last)
    aEnd = last;

#ifdef ACCESSIBILITY
  nsIPresShell *presShell = PresContext()->PresShell();
  if (presShell->IsAccessibilityActive()) {
    PRInt32 end =
      mRowCount > 0 ? ((mRowCount <= aEnd) ? mRowCount - 1 : aEnd) : 0;
    FireInvalidateEvent(aStart, end, aCol, aCol);
  }
#endif

  nsRect rangeRect;
  nsresult rv = col->GetRect(this, 
                             mInnerBox.y+mRowHeight*(aStart-mTopRowIndex),
                             mRowHeight*(aEnd-aStart+1),
                             &rangeRect);
  NS_ENSURE_SUCCESS(rv, rv);

  nsIFrame::Invalidate(rangeRect);

  return NS_OK;
}

static void
FindScrollParts(nsIFrame* aCurrFrame, nsTreeBodyFrame::ScrollParts* aResult)
{
  if (!aResult->mColumnsScrollableView) {
    nsIScrollableFrame* f = do_QueryFrame(aCurrFrame);
    if (f) {
      aResult->mColumnsFrame = aCurrFrame;
      aResult->mColumnsScrollableView = f->GetScrollableView();
    }
  }
  
  nsIScrollbarFrame *sf = do_QueryFrame(aCurrFrame);
  if (sf) {
    if (!aCurrFrame->IsHorizontal()) {
      if (!aResult->mVScrollbar) {
        aResult->mVScrollbar = sf;
      }
    } else {
      if (!aResult->mHScrollbar) {
        aResult->mHScrollbar = sf;
      }
    }
    
    return;
  }
  
  nsIFrame* child = aCurrFrame->GetFirstChild(nsnull);
  while (child &&
         (!aResult->mVScrollbar || !aResult->mHScrollbar ||
          !aResult->mColumnsScrollableView)) {
    FindScrollParts(child, aResult);
    child = child->GetNextSibling();
  }
}

nsTreeBodyFrame::ScrollParts nsTreeBodyFrame::GetScrollParts()
{
  nsPresContext* presContext = PresContext();
  ScrollParts result = { nsnull, nsnull, nsnull, nsnull, nsnull, nsnull };
  nsIContent* baseElement = GetBaseElement();
  nsIFrame* treeFrame =
    baseElement ? presContext->PresShell()->GetPrimaryFrameFor(baseElement) : nsnull;
  if (treeFrame) {
    
    
    FindScrollParts(treeFrame, &result);
    if (result.mHScrollbar) {
      result.mHScrollbar->SetScrollbarMediatorContent(GetContent());
      nsIFrame* f = do_QueryFrame(result.mHScrollbar);
      result.mHScrollbarContent = f->GetContent();
    }
    if (result.mVScrollbar) {
      result.mVScrollbar->SetScrollbarMediatorContent(GetContent());
      nsIFrame* f = do_QueryFrame(result.mVScrollbar);
      result.mVScrollbarContent = f->GetContent();
    }
  }
  return result;
}

void
nsTreeBodyFrame::UpdateScrollbars(const ScrollParts& aParts)
{
  nscoord rowHeightAsPixels = nsPresContext::AppUnitsToIntCSSPixels(mRowHeight);

  if (aParts.mVScrollbar) {
    nsAutoString curPos;
    curPos.AppendInt(mTopRowIndex*rowHeightAsPixels);
    aParts.mVScrollbarContent->
      SetAttr(kNameSpaceID_None, nsGkAtoms::curpos, curPos, PR_TRUE);
  }

  if (aParts.mHScrollbar) {
    nsAutoString curPos;
    curPos.AppendInt(mHorzPosition);
    aParts.mHScrollbarContent->
      SetAttr(kNameSpaceID_None, nsGkAtoms::curpos, curPos, PR_TRUE);
  }
}

void
nsTreeBodyFrame::CheckOverflow(const ScrollParts& aParts)
{
  PRBool verticalOverflowChanged = PR_FALSE;
  PRBool horizontalOverflowChanged = PR_FALSE;

  if (!mVerticalOverflow && mRowCount > mPageLength) {
    mVerticalOverflow = PR_TRUE;
    verticalOverflowChanged = PR_TRUE;
  }
  else if (mVerticalOverflow && mRowCount <= mPageLength) {
    mVerticalOverflow = PR_FALSE;
    verticalOverflowChanged = PR_TRUE;
  }

  if (aParts.mColumnsFrame) {
    nsRect bounds = aParts.mColumnsFrame->GetRect();
    if (bounds.width != 0) {
      


      bounds.width += nsPresContext::CSSPixelsToAppUnits(0.5f);
      if (!mHorizontalOverflow && bounds.width < mHorzWidth) {
        mHorizontalOverflow = PR_TRUE;
        horizontalOverflowChanged = PR_TRUE;
      } else if (mHorizontalOverflow && bounds.width >= mHorzWidth) {
        mHorizontalOverflow = PR_FALSE;
        horizontalOverflowChanged = PR_TRUE;
      }
    }
  }
 
  nsRefPtr<nsPresContext> presContext = PresContext();
  nsCOMPtr<nsIContent> content = mContent;

  if (verticalOverflowChanged) {
    nsScrollPortEvent event(PR_TRUE, mVerticalOverflow ? NS_SCROLLPORT_OVERFLOW
                            : NS_SCROLLPORT_UNDERFLOW, nsnull);
    event.orient = nsScrollPortEvent::vertical;
    nsEventDispatcher::Dispatch(content, presContext, &event);
  }

  if (horizontalOverflowChanged) {
    nsScrollPortEvent event(PR_TRUE,
                            mHorizontalOverflow ? NS_SCROLLPORT_OVERFLOW
                            : NS_SCROLLPORT_UNDERFLOW, nsnull);
    event.orient = nsScrollPortEvent::horizontal;
    nsEventDispatcher::Dispatch(content, presContext, &event);
  }
}

void
nsTreeBodyFrame::InvalidateScrollbars(const ScrollParts& aParts, nsWeakFrame& aWeakColumnsFrame)
{
  if (mUpdateBatchNest || !mView)
    return;
  nsWeakFrame weakFrame(this);

  if (aParts.mVScrollbar) {
    
    nsAutoString maxposStr;

    nscoord rowHeightAsPixels = nsPresContext::AppUnitsToIntCSSPixels(mRowHeight);

    PRInt32 size = rowHeightAsPixels * (mRowCount > mPageLength ? mRowCount - mPageLength : 0);
    maxposStr.AppendInt(size);
    aParts.mVScrollbarContent->
      SetAttr(kNameSpaceID_None, nsGkAtoms::maxpos, maxposStr, PR_TRUE);
    ENSURE_TRUE(weakFrame.IsAlive());

    
    nscoord pageincrement = mPageLength*rowHeightAsPixels;
    nsAutoString pageStr;
    pageStr.AppendInt(pageincrement);
    aParts.mVScrollbarContent->
      SetAttr(kNameSpaceID_None, nsGkAtoms::pageincrement, pageStr, PR_TRUE);
    ENSURE_TRUE(weakFrame.IsAlive());
  }

  if (aParts.mHScrollbar && aParts.mColumnsFrame && aWeakColumnsFrame.IsAlive()) {
    
    nsRect bounds = aParts.mColumnsFrame->GetRect();
    nsAutoString maxposStr;

    maxposStr.AppendInt(mHorzWidth > bounds.width ? mHorzWidth - bounds.width : 0);
    aParts.mHScrollbarContent->
      SetAttr(kNameSpaceID_None, nsGkAtoms::maxpos, maxposStr, PR_TRUE);
    ENSURE_TRUE(weakFrame.IsAlive());
  
    nsAutoString pageStr;
    pageStr.AppendInt(bounds.width);
    aParts.mHScrollbarContent->
      SetAttr(kNameSpaceID_None, nsGkAtoms::pageincrement, pageStr, PR_TRUE);
    ENSURE_TRUE(weakFrame.IsAlive());
  
    pageStr.Truncate();
    pageStr.AppendInt(nsPresContext::CSSPixelsToAppUnits(16));
    aParts.mHScrollbarContent->
      SetAttr(kNameSpaceID_None, nsGkAtoms::increment, pageStr, PR_TRUE);
  }
}



void
nsTreeBodyFrame::AdjustClientCoordsToBoxCoordSpace(PRInt32 aX, PRInt32 aY,
                                                   nscoord* aResultX,
                                                   nscoord* aResultY)
{
  nsPresContext* presContext = PresContext();

  nsPoint point(nsPresContext::CSSPixelsToAppUnits(aX),
                nsPresContext::CSSPixelsToAppUnits(aY));

  
  
  nsPoint clientOffset;
  nsIView* closestView = GetClosestView(&clientOffset);
  point -= clientOffset;

  nsIView* rootView;
  presContext->GetPresShell()->GetViewManager()->GetRootView(rootView);
  NS_ASSERTION(closestView && rootView, "No view?");
  point -= closestView->GetOffsetTo(rootView);

  
  
  point -= mInnerBox.TopLeft();

  *aResultX = point.x;
  *aResultY = point.y;
} 

nsresult
nsTreeBodyFrame::GetRowAt(PRInt32 aX, PRInt32 aY, PRInt32* _retval)
{
  if (!mView)
    return NS_OK;

  nscoord x;
  nscoord y;
  AdjustClientCoordsToBoxCoordSpace(aX, aY, &x, &y);

  
  if (y < 0) {
    *_retval = -1;
    return NS_OK;
  }

  *_retval = GetRowAt(x, y);

  return NS_OK;
}

nsresult
nsTreeBodyFrame::GetCellAt(PRInt32 aX, PRInt32 aY, PRInt32* aRow, nsITreeColumn** aCol,
                           nsACString& aChildElt)
{
  if (!mView)
    return NS_OK;

  nscoord x;
  nscoord y;
  AdjustClientCoordsToBoxCoordSpace(aX, aY, &x, &y);

  
  if (y < 0) {
    *aRow = -1;
    return NS_OK;
  }

  nsTreeColumn* col;
  nsIAtom* child;
  GetCellAt(x, y, aRow, &col, &child);

  if (col) {
    NS_ADDREF(*aCol = col);
    if (child == nsCSSAnonBoxes::moztreecell)
      aChildElt.AssignLiteral("cell");
    else if (child == nsCSSAnonBoxes::moztreetwisty)
      aChildElt.AssignLiteral("twisty");
    else if (child == nsCSSAnonBoxes::moztreeimage)
      aChildElt.AssignLiteral("image");
    else if (child == nsCSSAnonBoxes::moztreecelltext)
      aChildElt.AssignLiteral("text");
  }

  return NS_OK;
}























nsresult
nsTreeBodyFrame::GetCoordsForCellItem(PRInt32 aRow, nsITreeColumn* aCol, const nsACString& aElement, 
                                      PRInt32 *aX, PRInt32 *aY, PRInt32 *aWidth, PRInt32 *aHeight)
{
  *aX = 0;
  *aY = 0;
  *aWidth = 0;
  *aHeight = 0;

  PRBool isRTL = GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL;
  nscoord currX = mInnerBox.x - mHorzPosition;

  
  nsRect theRect;

  nsPresContext* presContext = PresContext();

  for (nsTreeColumn* currCol = mColumns->GetFirstColumn(); currCol; currCol = currCol->GetNext()) {

    
    nscoord colWidth;
#ifdef DEBUG
    nsresult rv =
#endif
      currCol->GetWidthInTwips(this, &colWidth);
    NS_ASSERTION(NS_SUCCEEDED(rv), "invalid column");

    nsRect cellRect(currX, mInnerBox.y + mRowHeight * (aRow - mTopRowIndex),
                    colWidth, mRowHeight);

    
    
    if (currCol != aCol) {
      currX += cellRect.width;
      continue;
    }
    
    PrefillPropertyArray(aRow, currCol);
    mView->GetCellProperties(aRow, currCol, mScratchArray);

    nsStyleContext* rowContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreerow);

    
    
    
    AdjustForBorderPadding(rowContext, cellRect);

    nsStyleContext* cellContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreecell);

    NS_NAMED_LITERAL_CSTRING(cell, "cell");
    if (currCol->IsCycler() || cell.Equals(aElement)) {
      
      

      theRect = cellRect;
      nsMargin cellMargin;
      cellContext->GetStyleMargin()->GetMargin(cellMargin);
      theRect.Deflate(cellMargin);
      break;
    }

    
    
    
    
    AdjustForBorderPadding(cellContext, cellRect);

    nsCOMPtr<nsIRenderingContext> rc;
    presContext->PresShell()->CreateRenderingContext(this, getter_AddRefs(rc));

    
    
    
    nscoord cellX = cellRect.x;
    nscoord remainWidth = cellRect.width;

    if (currCol->IsPrimary()) {
      
      

      
      PRInt32 level;
      mView->GetLevel(aRow, &level);
      if (!isRTL)
        cellX += mIndentation * level;
      remainWidth -= mIndentation * level;

      
      nsRect imageRect;
      nsRect twistyRect(cellRect);
      nsStyleContext* twistyContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreetwisty);
      GetTwistyRect(aRow, currCol, imageRect, twistyRect, presContext,
                    *rc, twistyContext);

      if (NS_LITERAL_CSTRING("twisty").Equals(aElement)) {
        
        theRect = twistyRect;
        break;
      }
      
      
      
      nsMargin twistyMargin;
      twistyContext->GetStyleMargin()->GetMargin(twistyMargin);
      twistyRect.Inflate(twistyMargin);

      
      
      if (!isRTL)
        cellX += twistyRect.width;
    }

    
    nsStyleContext* imageContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreeimage);

    nsRect imageSize = GetImageSize(aRow, currCol, PR_FALSE, imageContext);
    if (NS_LITERAL_CSTRING("image").Equals(aElement)) {
      theRect = imageSize;
      theRect.x = cellX;
      theRect.y = cellRect.y;
      break;
    }

    
    nsMargin imageMargin;
    imageContext->GetStyleMargin()->GetMargin(imageMargin);
    imageSize.Inflate(imageMargin);

    
    if (!isRTL)
      cellX += imageSize.width;
    
    
    nsAutoString cellText;
    mView->GetCellText(aRow, currCol, cellText);
    
    
    CheckTextForBidi(cellText);

    
    
    
    
    nsRect textRect(cellX, cellRect.y, remainWidth, cellRect.height);

    
    
    
    
    nsStyleContext* textContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreecelltext);

    nsCOMPtr<nsIFontMetrics> fm;
    nsLayoutUtils::GetFontMetricsForStyleContext(textContext,
                                                 getter_AddRefs(fm));
    nscoord height;
    fm->GetHeight(height);

    nsMargin textMargin;
    textContext->GetStyleMargin()->GetMargin(textMargin);
    textRect.Deflate(textMargin);

    
    if (height < textRect.height) {
      textRect.y += (textRect.height - height) / 2;
      textRect.height = height;
    }

    nsMargin bp(0,0,0,0);
    GetBorderPadding(textContext, bp);
    textRect.height += bp.top + bp.bottom;

    rc->SetFont(fm);
    nscoord width =
      nsLayoutUtils::GetStringWidth(this, rc, cellText.get(), cellText.Length());

    nscoord totalTextWidth = width + bp.left + bp.right;
    if (totalTextWidth < remainWidth) {
      
      
      textRect.width = totalTextWidth;
    }

    theRect = textRect;
  }

  if (isRTL)
    theRect.x = mInnerBox.width - theRect.x - theRect.width;

  *aX = nsPresContext::AppUnitsToIntCSSPixels(theRect.x);
  *aY = nsPresContext::AppUnitsToIntCSSPixels(theRect.y);
  *aWidth = nsPresContext::AppUnitsToIntCSSPixels(theRect.width);
  *aHeight = nsPresContext::AppUnitsToIntCSSPixels(theRect.height);

  return NS_OK;
}

PRInt32
nsTreeBodyFrame::GetRowAt(PRInt32 aX, PRInt32 aY)
{
  
  PRInt32 row = (aY/mRowHeight)+mTopRowIndex;

  
  
  if (row > mTopRowIndex + mPageLength || row >= mRowCount)
    return -1;

  return row;
}

void
nsTreeBodyFrame::CheckTextForBidi(nsAutoString& aText)
{
  
  
  
  if (HasRTLChars(aText)) {
    PresContext()->SetBidiEnabled();
  }
}

void
nsTreeBodyFrame::AdjustForCellText(nsAutoString& aText,
                                   PRInt32 aRowIndex,  nsTreeColumn* aColumn,
                                   nsIRenderingContext& aRenderingContext,
                                   nsRect& aTextRect)
{
  NS_PRECONDITION(aColumn && aColumn->GetFrame(this), "invalid column passed");

  nscoord width =
    nsLayoutUtils::GetStringWidth(this, &aRenderingContext, aText.get(), aText.Length());
  nscoord maxWidth = aTextRect.width;

  if (aColumn->Overflow()) {
    nsresult rv;
    nsTreeColumn* nextColumn = aColumn->GetNext();
    while (nextColumn && width > maxWidth) {
      while (nextColumn) {
        nscoord width;
        rv = nextColumn->GetWidthInTwips(this, &width);
        NS_ASSERTION(NS_SUCCEEDED(rv), "nextColumn is invalid");

        if (width != 0)
          break;

        nextColumn = nextColumn->GetNext();
      }

      if (nextColumn) {
        nsAutoString nextText;
        mView->GetCellText(aRowIndex, nextColumn, nextText);
        
        

        if (nextText.Length() == 0) {
          nscoord width;
          rv = nextColumn->GetWidthInTwips(this, &width);
          NS_ASSERTION(NS_SUCCEEDED(rv), "nextColumn is invalid");

          maxWidth += width;

          nextColumn = nextColumn->GetNext();
        }
        else {
          nextColumn = nsnull;
        }
      }
    }
  }

  if (width > maxWidth) {
    
    
    const nsDependentString& kEllipsis = nsContentUtils::GetLocalizedEllipsis();
    nscoord ellipsisWidth;
    aRenderingContext.SetTextRunRTL(PR_FALSE);
    aRenderingContext.GetWidth(kEllipsis, ellipsisWidth);

    width = maxWidth;
    if (ellipsisWidth > width)
      aText.SetLength(0);
    else if (ellipsisWidth == width)
      aText.Assign(kEllipsis);
    else {
      
      
      
      width -= ellipsisWidth;

      
      switch (aColumn->GetCropStyle()) {
        default:
        case 0: {
          
          nscoord cwidth;
          nscoord twidth = 0;
          int length = aText.Length();
          int i;
          for (i = 0; i < length; ++i) {
            PRUnichar ch = aText[i];
            
            aRenderingContext.GetWidth(ch,cwidth);
            if (twidth + cwidth > width)
              break;
            twidth += cwidth;
          }
          aText.Truncate(i);
          aText.Append(kEllipsis);
        }
        break;

        case 2: {
          
          nscoord cwidth;
          nscoord twidth = 0;
          int length = aText.Length();
          int i;
          for (i=length-1; i >= 0; --i) {
            PRUnichar ch = aText[i];
            aRenderingContext.GetWidth(ch,cwidth);
            if (twidth + cwidth > width)
              break;
            twidth += cwidth;
          }

          nsAutoString copy;
          aText.Right(copy, length-1-i);
          aText.Assign(kEllipsis);
          aText += copy;
        }
        break;

        case 1:
        {
          
          nsAutoString leftStr, rightStr;
          nscoord cwidth, twidth = 0;
          int length = aText.Length();
          int rightPos = length - 1;
          for (int leftPos = 0; leftPos < rightPos; ++leftPos) {
            PRUnichar ch = aText[leftPos];
            aRenderingContext.GetWidth(ch, cwidth);
            twidth += cwidth;
            if (twidth > width)
              break;
            leftStr.Append(ch);

            ch = aText[rightPos];
            aRenderingContext.GetWidth(ch, cwidth);
            twidth += cwidth;
            if (twidth > width)
              break;
            rightStr.Insert(ch, 0);
            --rightPos;
          }
          aText = leftStr;
          aText.Append(kEllipsis);
          aText += rightStr;
        }
        break;
      }
    }
  }
  else {
    switch (aColumn->GetTextAlignment()) {
      case NS_STYLE_TEXT_ALIGN_RIGHT: {
        aTextRect.x += aTextRect.width - width;
      }
      break;
      case NS_STYLE_TEXT_ALIGN_CENTER: {
        aTextRect.x += (aTextRect.width - width) / 2;
      }
      break;
    }
  }

  aTextRect.width =
    nsLayoutUtils::GetStringWidth(this, &aRenderingContext, aText.get(), aText.Length());
}

nsIAtom*
nsTreeBodyFrame::GetItemWithinCellAt(nscoord aX, const nsRect& aCellRect, 
                                     PRInt32 aRowIndex,
                                     nsTreeColumn* aColumn)
{
  NS_PRECONDITION(aColumn && aColumn->GetFrame(this), "invalid column passed");

  
  PrefillPropertyArray(aRowIndex, aColumn);
  mView->GetCellProperties(aRowIndex, aColumn, mScratchArray);

  
  nsStyleContext* cellContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreecell);

  
  
  nsRect cellRect(aCellRect);
  nsMargin cellMargin;
  cellContext->GetStyleMargin()->GetMargin(cellMargin);
  cellRect.Deflate(cellMargin);

  
  AdjustForBorderPadding(cellContext, cellRect);

  if (aX < cellRect.x || aX >= cellRect.x + cellRect.width) {
    
    return nsCSSAnonBoxes::moztreecell;
  }

  nscoord currX = cellRect.x;
  nscoord remainingWidth = cellRect.width;

  
  PRBool isRTL = GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL;

  if (aColumn->IsPrimary()) {
    
    PRInt32 level;
    mView->GetLevel(aRowIndex, &level);

    if (!isRTL)
      currX += mIndentation*level;
    remainingWidth -= mIndentation*level;

    if (isRTL && aX > currX + remainingWidth ||
        !isRTL && aX < currX) {
      
      return nsCSSAnonBoxes::moztreecell;
    }

    
    nsRect twistyRect(currX, cellRect.y, remainingWidth, cellRect.height);
    PRBool hasTwisty = PR_FALSE;
    PRBool isContainer = PR_FALSE;
    mView->IsContainer(aRowIndex, &isContainer);
    if (isContainer) {
      PRBool isContainerEmpty = PR_FALSE;
      mView->IsContainerEmpty(aRowIndex, &isContainerEmpty);
      if (!isContainerEmpty)
        hasTwisty = PR_TRUE;
    }

    nsPresContext* presContext = PresContext();
    nsCOMPtr<nsIRenderingContext> rc;
    presContext->PresShell()->CreateRenderingContext(this, getter_AddRefs(rc));

    
    nsStyleContext* twistyContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreetwisty);

    nsRect imageSize;
    GetTwistyRect(aRowIndex, aColumn, imageSize, twistyRect, presContext,
                  *rc, twistyContext);

    
    
    
    nsMargin twistyMargin;
    twistyContext->GetStyleMargin()->GetMargin(twistyMargin);
    twistyRect.Inflate(twistyMargin);
    if (isRTL)
      twistyRect.x = currX + remainingWidth - twistyRect.width;

    
    
    
    if (aX >= twistyRect.x && aX < twistyRect.x + twistyRect.width) {
      if (hasTwisty)
        return nsCSSAnonBoxes::moztreetwisty;
      else
        return nsCSSAnonBoxes::moztreecell;
    }

    if (!isRTL)
      currX += twistyRect.width;
    remainingWidth -= twistyRect.width;    
  }
  
  
  nsRect iconRect(currX, cellRect.y, remainingWidth, cellRect.height);
  
  
  nsStyleContext* imageContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreeimage);

  nsRect iconSize = GetImageSize(aRowIndex, aColumn, PR_FALSE, imageContext);
  nsMargin imageMargin;
  imageContext->GetStyleMargin()->GetMargin(imageMargin);
  iconSize.Inflate(imageMargin);
  iconRect.width = iconSize.width;
  if (isRTL)
    iconRect.x = currX + remainingWidth - iconRect.width;

  if (aX >= iconRect.x && aX < iconRect.x + iconRect.width) {
    
    return nsCSSAnonBoxes::moztreeimage;
  }

  if (!isRTL)
    currX += iconRect.width;
  remainingWidth -= iconRect.width;    

  nsAutoString cellText;
  mView->GetCellText(aRowIndex, aColumn, cellText);
  
  
  CheckTextForBidi(cellText);

  nsRect textRect(currX, cellRect.y, remainingWidth, cellRect.height);

  nsStyleContext* textContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreecelltext);

  nsMargin textMargin;
  textContext->GetStyleMargin()->GetMargin(textMargin);
  textRect.Deflate(textMargin);

  AdjustForBorderPadding(textContext, textRect);

  nsCOMPtr<nsIRenderingContext> renderingContext;
  PresContext()->PresShell()->CreateRenderingContext(this, getter_AddRefs(renderingContext));

  nsLayoutUtils::SetFontFromStyle(renderingContext, textContext);

  AdjustForCellText(cellText, aRowIndex, aColumn, *renderingContext, textRect);
  if (isRTL)
    textRect.x = currX + remainingWidth - textRect.width;

  if (aX >= textRect.x && aX < textRect.x + textRect.width)
    return nsCSSAnonBoxes::moztreecelltext;
  else
    return nsCSSAnonBoxes::moztreecell;
}

void
nsTreeBodyFrame::GetCellAt(nscoord aX, nscoord aY, PRInt32* aRow,
                           nsTreeColumn** aCol, nsIAtom** aChildElt)
{
  *aCol = nsnull;
  *aChildElt = nsnull;

  *aRow = GetRowAt(aX, aY);
  if (*aRow < 0)
    return;

  
  for (nsTreeColumn* currCol = mColumns->GetFirstColumn(); currCol; 
       currCol = currCol->GetNext()) {
    nsRect cellRect;
    nsresult rv = currCol->GetRect(this,
                                   mInnerBox.y +
                                         mRowHeight * (*aRow - mTopRowIndex),
                                   mRowHeight,
                                   &cellRect);
    if (NS_FAILED(rv)) {
      NS_NOTREACHED("column has no frame");
      continue;
    }

    if (!OffsetForHorzScroll(cellRect, PR_FALSE))
      continue;

    if (aX >= cellRect.x && aX < cellRect.x + cellRect.width) {
      
      if (aCol)
        *aCol = currCol;

      if (currCol->IsCycler())
        
        *aChildElt = nsCSSAnonBoxes::moztreeimage;
      else
        *aChildElt = GetItemWithinCellAt(aX, cellRect, *aRow, currCol);
      break;
    }
  }
}

nsresult
nsTreeBodyFrame::GetCellWidth(PRInt32 aRow, nsTreeColumn* aCol,
                              nsIRenderingContext* aRenderingContext,
                              nscoord& aDesiredSize, nscoord& aCurrentSize)
{
  NS_PRECONDITION(aCol, "aCol must not be null");
  NS_PRECONDITION(aRenderingContext, "aRenderingContext must not be null");

  
  nscoord colWidth;
  nsresult rv = aCol->GetWidthInTwips(this, &colWidth);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRect cellRect(0, 0, colWidth, mRowHeight);

  PRInt32 overflow = cellRect.x+cellRect.width-(mInnerBox.x+mInnerBox.width);
  if (overflow > 0)
    cellRect.width -= overflow;

  
  nsStyleContext* cellContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreecell);
  nsMargin bp(0,0,0,0);
  GetBorderPadding(cellContext, bp);

  aCurrentSize = cellRect.width;
  aDesiredSize = bp.left + bp.right;

  if (aCol->IsPrimary()) {
    
    

    
    PRInt32 level;
    mView->GetLevel(aRow, &level);
    aDesiredSize += mIndentation * level;
    
    
    nsStyleContext* twistyContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreetwisty);

    nsRect imageSize;
    nsRect twistyRect(cellRect);
    GetTwistyRect(aRow, aCol, imageSize, twistyRect, PresContext(),
                  *aRenderingContext, twistyContext);

    
    nsMargin twistyMargin;
    twistyContext->GetStyleMargin()->GetMargin(twistyMargin);
    twistyRect.Inflate(twistyMargin);

    aDesiredSize += twistyRect.width;
  }

  nsStyleContext* imageContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreeimage);

  
  nsRect imageSize = GetImageSize(aRow, aCol, PR_FALSE, imageContext);
  
  nsMargin imageMargin;
  imageContext->GetStyleMargin()->GetMargin(imageMargin);
  imageSize.Inflate(imageMargin);

  aDesiredSize += imageSize.width;
  
  
  nsAutoString cellText;
  mView->GetCellText(aRow, aCol, cellText);
  
  
  CheckTextForBidi(cellText);

  nsStyleContext* textContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreecelltext);

  
  GetBorderPadding(textContext, bp);

  nsLayoutUtils::SetFontFromStyle(aRenderingContext, textContext);

  
  nscoord width =
    nsLayoutUtils::GetStringWidth(this, aRenderingContext, cellText.get(), cellText.Length());
  nscoord totalTextWidth = width + bp.left + bp.right;
  aDesiredSize += totalTextWidth;
  return NS_OK;
}

nsresult
nsTreeBodyFrame::IsCellCropped(PRInt32 aRow, nsITreeColumn* aCol, PRBool *_retval)
{  
  nscoord currentSize, desiredSize;
  nsresult rv;

  nsRefPtr<nsTreeColumn> col = GetColumnImpl(aCol);
  if (!col)
    return NS_ERROR_INVALID_ARG;

  nsCOMPtr<nsIRenderingContext> rc;
  rv = PresContext()->PresShell()->
    CreateRenderingContext(this, getter_AddRefs(rc));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = GetCellWidth(aRow, col, rc, desiredSize, currentSize);
  NS_ENSURE_SUCCESS(rv, rv);

  *_retval = desiredSize > currentSize;

  return NS_OK;
}

void
nsTreeBodyFrame::MarkDirtyIfSelect()
{
  nsIContent* baseElement = GetBaseElement();

  if (baseElement && baseElement->Tag() == nsGkAtoms::select &&
      baseElement->IsNodeOfType(nsINode::eHTML)) {
    
    
    

    mStringWidth = -1;
    PresContext()->PresShell()->FrameNeedsReflow(this,
                                                 nsIPresShell::eTreeChange,
                                                 NS_FRAME_IS_DIRTY);
  }
}

nsresult
nsTreeBodyFrame::CreateTimer(const nsILookAndFeel::nsMetricID aID,
                             nsTimerCallbackFunc aFunc, PRInt32 aType,
                             nsITimer** aTimer)
{
  
  PRInt32 delay = 0;
  PresContext()->LookAndFeel()->GetMetric(aID, delay);

  nsCOMPtr<nsITimer> timer;

  
  
  if (delay > 0) {
    timer = do_CreateInstance("@mozilla.org/timer;1");
    if (timer)
      timer->InitWithFuncCallback(aFunc, this, delay, aType);
  }

  NS_IF_ADDREF(*aTimer = timer);

  return NS_OK;
}

nsresult
nsTreeBodyFrame::RowCountChanged(PRInt32 aIndex, PRInt32 aCount)
{
  if (aCount == 0 || !mView)
    return NS_OK; 

#ifdef ACCESSIBILITY
  nsIPresShell *presShell = PresContext()->PresShell();
  if (presShell->IsAccessibilityActive())
    FireRowCountChangedEvent(aIndex, aCount);
#endif

  
  nsCOMPtr<nsITreeSelection> sel;
  mView->GetSelection(getter_AddRefs(sel));
  if (sel)
    sel->AdjustSelection(aIndex, aCount);

  if (mUpdateBatchNest)
    return NS_OK;

  mRowCount += aCount;
#ifdef DEBUG
  PRInt32 rowCount = mRowCount;
  mView->GetRowCount(&rowCount);
  NS_ASSERTION(rowCount == mRowCount, "row count did not change by the amount suggested, check caller");
#endif

  PRInt32 count = PR_ABS(aCount);
  PRInt32 last = GetLastVisibleRow();
  if (aIndex >= mTopRowIndex && aIndex <= last)
    InvalidateRange(aIndex, last);
    
  ScrollParts parts = GetScrollParts();

  if (mTopRowIndex == 0) {    
    
    if (FullScrollbarsUpdate(PR_FALSE)) {
      MarkDirtyIfSelect();
    }
    return NS_OK;
  }

  PRBool needsInvalidation = PR_FALSE;
  
  if (aCount > 0) {
    if (mTopRowIndex > aIndex) {
      
      mTopRowIndex += aCount;
    }
  }
  else if (aCount < 0) {
    if (mTopRowIndex > aIndex+count-1) {
      
      
      mTopRowIndex -= count;
    }
    else if (mTopRowIndex >= aIndex) {
      
      if (mTopRowIndex + mPageLength > mRowCount - 1) {
        mTopRowIndex = PR_MAX(0, mRowCount - 1 - mPageLength);
      }
      needsInvalidation = PR_TRUE;
    }
  }

  if (FullScrollbarsUpdate(needsInvalidation)) {
    MarkDirtyIfSelect();
  }
  return NS_OK;
}

nsresult
nsTreeBodyFrame::BeginUpdateBatch()
{
  ++mUpdateBatchNest;

  return NS_OK;
}

nsresult
nsTreeBodyFrame::EndUpdateBatch()
{
  NS_ASSERTION(mUpdateBatchNest > 0, "badly nested update batch");

  if (--mUpdateBatchNest == 0) {
    if (mView) {
      Invalidate();
      PRInt32 countBeforeUpdate = mRowCount;
      mView->GetRowCount(&mRowCount);
      if (countBeforeUpdate != mRowCount) {
        if (mTopRowIndex + mPageLength > mRowCount - 1) {
          mTopRowIndex = PR_MAX(0, mRowCount - 1 - mPageLength);
        }
        FullScrollbarsUpdate(PR_FALSE);
      }
    }
  }

  return NS_OK;
}

void
nsTreeBodyFrame::PrefillPropertyArray(PRInt32 aRowIndex, nsTreeColumn* aCol)
{
  NS_PRECONDITION(!aCol || aCol->GetFrame(this), "invalid column passed");
  mScratchArray->Clear();
  
  
  if (mFocused)
    mScratchArray->AppendElement(nsGkAtoms::focus);

  
  PRBool sorted = PR_FALSE;
  mView->IsSorted(&sorted);
  if (sorted)
    mScratchArray->AppendElement(nsGkAtoms::sorted);

  
  if (mSlots && mSlots->mDragSession)
    mScratchArray->AppendElement(nsGkAtoms::dragSession);

  if (aRowIndex != -1) {
    if (aRowIndex == mMouseOverRow)
      mScratchArray->AppendElement(nsGkAtoms::hover);
  
    nsCOMPtr<nsITreeSelection> selection;
    mView->GetSelection(getter_AddRefs(selection));

    if (selection) {
      
      PRBool isSelected;
      selection->IsSelected(aRowIndex, &isSelected);
      if (isSelected)
        mScratchArray->AppendElement(nsGkAtoms::selected);

      
      PRInt32 currentIndex;
      selection->GetCurrentIndex(&currentIndex);
      if (aRowIndex == currentIndex)
        mScratchArray->AppendElement(nsGkAtoms::current);
  
      
      if (aCol) {
        nsCOMPtr<nsITreeColumn> currentColumn;
        selection->GetCurrentColumn(getter_AddRefs(currentColumn));
        if (aCol == currentColumn)
          mScratchArray->AppendElement(nsGkAtoms::active);
      }
    }

    
    PRBool isContainer = PR_FALSE;
    mView->IsContainer(aRowIndex, &isContainer);
    if (isContainer) {
      mScratchArray->AppendElement(nsGkAtoms::container);

      
      PRBool isOpen = PR_FALSE;
      mView->IsContainerOpen(aRowIndex, &isOpen);
      if (isOpen)
        mScratchArray->AppendElement(nsGkAtoms::open);
      else
        mScratchArray->AppendElement(nsGkAtoms::closed);
    }
    else {
      mScratchArray->AppendElement(nsGkAtoms::leaf);
    }

    
    if (mSlots && mSlots->mDropAllowed && mSlots->mDropRow == aRowIndex) {
      if (mSlots->mDropOrient == nsITreeView::DROP_BEFORE)
        mScratchArray->AppendElement(nsGkAtoms::dropBefore);
      else if (mSlots->mDropOrient == nsITreeView::DROP_ON)
        mScratchArray->AppendElement(nsGkAtoms::dropOn);
      else if (mSlots->mDropOrient == nsITreeView::DROP_AFTER)
        mScratchArray->AppendElement(nsGkAtoms::dropAfter);
    }

    
    if (aRowIndex % 2)
      mScratchArray->AppendElement(nsGkAtoms::odd);
    else
      mScratchArray->AppendElement(nsGkAtoms::even);

    nsIContent* baseContent = GetBaseElement();
    if (baseContent && baseContent->HasAttr(kNameSpaceID_None, nsGkAtoms::editing))
      mScratchArray->AppendElement(nsGkAtoms::editing);
  }

  if (aCol) {
    mScratchArray->AppendElement(aCol->GetAtom());

    if (aCol->IsPrimary())
      mScratchArray->AppendElement(nsGkAtoms::primary);

    if (aCol->GetType() == nsITreeColumn::TYPE_CHECKBOX) {
      mScratchArray->AppendElement(nsGkAtoms::checkbox);

      if (aRowIndex != -1) {
        nsAutoString value;
        mView->GetCellValue(aRowIndex, aCol, value);
        if (value.EqualsLiteral("true"))
          mScratchArray->AppendElement(nsGkAtoms::checked);
      }
    }
    else if (aCol->GetType() == nsITreeColumn::TYPE_PROGRESSMETER) {
      mScratchArray->AppendElement(nsGkAtoms::progressmeter);

      if (aRowIndex != -1) {
        PRInt32 state;
        mView->GetProgressMode(aRowIndex, aCol, &state);
        if (state == nsITreeView::PROGRESS_NORMAL)
          mScratchArray->AppendElement(nsGkAtoms::progressNormal);
        else if (state == nsITreeView::PROGRESS_UNDETERMINED)
          mScratchArray->AppendElement(nsGkAtoms::progressUndetermined);
      }
    }

    
    if (aCol->mContent->AttrValueIs(kNameSpaceID_None,
                                    nsGkAtoms::insertbefore,
                                    nsGkAtoms::_true, eCaseMatters))
      mScratchArray->AppendElement(nsGkAtoms::insertbefore);
    if (aCol->mContent->AttrValueIs(kNameSpaceID_None,
                                    nsGkAtoms::insertafter,
                                    nsGkAtoms::_true, eCaseMatters))
      mScratchArray->AppendElement(nsGkAtoms::insertafter);
  }
}

nsITheme*
nsTreeBodyFrame::GetTwistyRect(PRInt32 aRowIndex,
                               nsTreeColumn* aColumn,
                               nsRect& aImageRect,
                               nsRect& aTwistyRect,
                               nsPresContext* aPresContext,
                               nsIRenderingContext& aRenderingContext,
                               nsStyleContext* aTwistyContext)
{
  
  
  
  
  
  
  aImageRect = GetImageSize(aRowIndex, aColumn, PR_TRUE, aTwistyContext);
  if (aImageRect.height > aTwistyRect.height)
    aImageRect.height = aTwistyRect.height;
  if (aImageRect.width > aTwistyRect.width)
    aImageRect.width = aTwistyRect.width;
  else
    aTwistyRect.width = aImageRect.width;

  PRBool useTheme = PR_FALSE;
  nsITheme *theme = nsnull;
  const nsStyleDisplay* twistyDisplayData = aTwistyContext->GetStyleDisplay();
  if (twistyDisplayData->mAppearance) {
    theme = aPresContext->GetTheme();
    if (theme && theme->ThemeSupportsWidget(aPresContext, nsnull, twistyDisplayData->mAppearance))
      useTheme = PR_TRUE;
  }

  if (useTheme) {
    nsIntSize minTwistySizePx(0,0);
    PRBool canOverride = PR_TRUE;
    theme->GetMinimumWidgetSize(&aRenderingContext, this, twistyDisplayData->mAppearance,
                                &minTwistySizePx, &canOverride);

    
    nsSize minTwistySize;
    minTwistySize.width = aPresContext->DevPixelsToAppUnits(minTwistySizePx.width);
    minTwistySize.height = aPresContext->DevPixelsToAppUnits(minTwistySizePx.height);

    if (aTwistyRect.width < minTwistySize.width || !canOverride)
      aTwistyRect.width = minTwistySize.width;
  }

  return useTheme ? theme : nsnull;
}

nsresult
nsTreeBodyFrame::GetImage(PRInt32 aRowIndex, nsTreeColumn* aCol, PRBool aUseContext,
                          nsStyleContext* aStyleContext, PRBool& aAllowImageRegions, imgIContainer** aResult)
{
  *aResult = nsnull;

  nsAutoString imageSrc;
  mView->GetImageSrc(aRowIndex, aCol, imageSrc);
  nsCOMPtr<imgIRequest> styleRequest;
  if (!aUseContext && !imageSrc.IsEmpty()) {
    aAllowImageRegions = PR_FALSE;
  }
  else {
    
    aAllowImageRegions = PR_TRUE;
    styleRequest = aStyleContext->GetStyleList()->mListStyleImage;
    if (!styleRequest)
      return NS_OK;
    nsCOMPtr<nsIURI> uri;
    styleRequest->GetURI(getter_AddRefs(uri));
    nsCAutoString spec;
    uri->GetSpec(spec);
    CopyUTF8toUTF16(spec, imageSrc);
  }

  
  nsTreeImageCacheEntry entry;
  if (mImageCache.Get(imageSrc, &entry)) {
    
    PRUint32 status;
    imgIRequest *imgReq = entry.request;
    imgReq->GetImageStatus(&status);
    imgReq->GetImage(aResult); 
    PRBool animated = PR_FALSE;
    if (*aResult)
      (*aResult)->GetAnimated(&animated);

    if ((!(status & imgIRequest::STATUS_LOAD_COMPLETE)) || animated) {
      
      nsCOMPtr<imgIDecoderObserver> obs;
      imgReq->GetDecoderObserver(getter_AddRefs(obs));
      nsCOMPtr<nsITreeImageListener> listener(do_QueryInterface(obs));
      if (listener)
        listener->AddCell(aRowIndex, aCol);
      return NS_OK;
    }
  }

  if (!*aResult) {
    
    
    nsTreeImageListener* listener = new nsTreeImageListener(mTreeBoxObject);
    if (!listener)
      return NS_ERROR_OUT_OF_MEMORY;

    listener->AddCell(aRowIndex, aCol);
    nsCOMPtr<imgIDecoderObserver> imgDecoderObserver = listener;

    nsCOMPtr<imgIRequest> imageRequest;
    if (styleRequest) {
      styleRequest->Clone(imgDecoderObserver, getter_AddRefs(imageRequest));
    } else {
      nsIDocument* doc = mContent->GetDocument();
      if (!doc)
        
        return NS_ERROR_FAILURE;

      nsCOMPtr<nsIURI> baseURI = mContent->GetBaseURI();

      nsCOMPtr<nsIURI> srcURI;
      nsContentUtils::NewURIWithDocumentCharset(getter_AddRefs(srcURI),
                                                imageSrc,
                                                doc,
                                                baseURI);
      if (!srcURI)
        return NS_ERROR_FAILURE;

      
      
      if (nsContentUtils::CanLoadImage(srcURI, mContent, doc,
                                       mContent->NodePrincipal())) {
        nsresult rv = nsContentUtils::LoadImage(srcURI,
                                                doc,
                                                mContent->NodePrincipal(),
                                                doc->GetDocumentURI(),
                                                imgDecoderObserver,
                                                nsIRequest::LOAD_NORMAL,
                                                getter_AddRefs(imageRequest));
        NS_ENSURE_SUCCESS(rv, rv);
                                  
      }
    }
    listener->UnsuppressInvalidation();

    if (!imageRequest)
      return NS_ERROR_FAILURE;

    
    imageRequest->GetImage(aResult);
    nsTreeImageCacheEntry cacheEntry(imageRequest, imgDecoderObserver);
    mImageCache.Put(imageSrc, cacheEntry);
  }
  return NS_OK;
}

nsRect nsTreeBodyFrame::GetImageSize(PRInt32 aRowIndex, nsTreeColumn* aCol, PRBool aUseContext,
                                     nsStyleContext* aStyleContext)
{
  

  
  
  
  
  nsRect r(0,0,0,0);
  nsMargin bp(0,0,0,0);
  GetBorderPadding(aStyleContext, bp);
  r.Inflate(bp);

  
  
  PRBool needWidth = PR_FALSE;
  PRBool needHeight = PR_FALSE;

  
  
  PRBool useImageRegion = PR_TRUE;
  nsCOMPtr<imgIContainer> image;
  GetImage(aRowIndex, aCol, aUseContext, aStyleContext, useImageRegion, getter_AddRefs(image));

  const nsStylePosition* myPosition = aStyleContext->GetStylePosition();
  const nsStyleList* myList = aStyleContext->GetStyleList();

  if (useImageRegion) {
    r.x += myList->mImageRegion.x;
    r.y += myList->mImageRegion.y;
  }

  if (myPosition->mWidth.GetUnit() == eStyleUnit_Coord)  {
    PRInt32 val = myPosition->mWidth.GetCoordValue();
    r.width += val;
  }
  else if (useImageRegion && myList->mImageRegion.width > 0)
    r.width += myList->mImageRegion.width;
  else 
    needWidth = PR_TRUE;

  if (myPosition->mHeight.GetUnit() == eStyleUnit_Coord)  {
    PRInt32 val = myPosition->mHeight.GetCoordValue();
    r.height += val;
  }
  else if (useImageRegion && myList->mImageRegion.height > 0)
    r.height += myList->mImageRegion.height;
  else 
    needHeight = PR_TRUE;

  if (image) {
    if (needWidth || needHeight) {
      

      if (needWidth) {
        
        nscoord width;
        image->GetWidth(&width);
        r.width += nsPresContext::CSSPixelsToAppUnits(width); 
      }
    
      if (needHeight) {
        nscoord height;
        image->GetHeight(&height);
        r.height += nsPresContext::CSSPixelsToAppUnits(height); 
      }
    }
  }

  return r;
}












nsSize
nsTreeBodyFrame::GetImageDestSize(nsStyleContext* aStyleContext,
                                  PRBool useImageRegion,
                                  imgIContainer* image)
{
  nsSize size(0,0);

  
  PRBool needWidth = PR_FALSE;
  PRBool needHeight = PR_FALSE;

  
  
  const nsStylePosition* myPosition = aStyleContext->GetStylePosition();

  if (myPosition->mWidth.GetUnit() == eStyleUnit_Coord) {
    
    size.width = myPosition->mWidth.GetCoordValue();
  }
  else {
    
    needWidth = PR_TRUE;
  }

  if (myPosition->mHeight.GetUnit() == eStyleUnit_Coord)  {
    
    size.height = myPosition->mHeight.GetCoordValue();
  }
  else {
    
    needHeight = PR_TRUE;
  }

  if (needWidth || needHeight) {
    
    nsSize imageSize(0,0);

    const nsStyleList* myList = aStyleContext->GetStyleList();

    if (useImageRegion && myList->mImageRegion.width > 0) {
      
      
      imageSize.width = myList->mImageRegion.width;
    }
    else if (image) {
      nscoord width;
      image->GetWidth(&width);
      imageSize.width = nsPresContext::CSSPixelsToAppUnits(width);
    }

    if (useImageRegion && myList->mImageRegion.height > 0) {
      
      
      imageSize.height = myList->mImageRegion.height;
    }
    else if (image) {
      nscoord height;
      image->GetHeight(&height);
      imageSize.height = nsPresContext::CSSPixelsToAppUnits(height);
    }

    if (needWidth) {
      if (!needHeight && imageSize.height != 0) {
        
        
        
        size.width = imageSize.width * size.height / imageSize.height;
      }
      else {
        size.width = imageSize.width;
      }
    }

    if (needHeight) {
      if (!needWidth && imageSize.width != 0) {
        
        
        
        size.height = imageSize.height * size.width / imageSize.width;
      }
      else {
        size.height = imageSize.height;
      }
    }
  }

  return size;
}








nsRect
nsTreeBodyFrame::GetImageSourceRect(nsStyleContext* aStyleContext,
                                    PRBool useImageRegion,
                                    imgIContainer* image)
{
  nsRect r(0,0,0,0);

  const nsStyleList* myList = aStyleContext->GetStyleList();

  if (useImageRegion &&
      (myList->mImageRegion.width > 0 || myList->mImageRegion.height > 0)) {
    
    r = myList->mImageRegion;
  }
  else if (image) {
    
    nscoord coord;
    image->GetWidth(&coord);
    r.width = nsPresContext::CSSPixelsToAppUnits(coord);
    image->GetHeight(&coord);
    r.height = nsPresContext::CSSPixelsToAppUnits(coord);
  }

  return r;
}

PRInt32 nsTreeBodyFrame::GetRowHeight()
{
  
  
  mScratchArray->Clear();
  nsStyleContext* rowContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreerow);
  if (rowContext) {
    const nsStylePosition* myPosition = rowContext->GetStylePosition();

    nscoord minHeight = 0;
    if (myPosition->mMinHeight.GetUnit() == eStyleUnit_Coord)
      minHeight = myPosition->mMinHeight.GetCoordValue();

    nscoord height = 0;
    if (myPosition->mHeight.GetUnit() == eStyleUnit_Coord)
      height = myPosition->mHeight.GetCoordValue();

    if (height < minHeight)
      height = minHeight;

    if (height > 0) {
      height = nsPresContext::AppUnitsToIntCSSPixels(height);
      height += height % 2;
      height = nsPresContext::CSSPixelsToAppUnits(height);

      
      
      nsRect rowRect(0,0,0,height);
      nsMargin rowMargin;
      rowContext->GetStyleMargin()->GetMargin(rowMargin);
      rowRect.Inflate(rowMargin);
      height = rowRect.height;
      return height;
    }
  }

  return nsPresContext::CSSPixelsToAppUnits(18); 
}

PRInt32 nsTreeBodyFrame::GetIndentation()
{
  
  mScratchArray->Clear();
  nsStyleContext* indentContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreeindentation);
  if (indentContext) {
    const nsStylePosition* myPosition = indentContext->GetStylePosition();
    if (myPosition->mWidth.GetUnit() == eStyleUnit_Coord)  {
      nscoord val = myPosition->mWidth.GetCoordValue();
      return val;
    }
  }

  return nsPresContext::CSSPixelsToAppUnits(16); 
}

void nsTreeBodyFrame::CalcInnerBox()
{
  mInnerBox.SetRect(0, 0, mRect.width, mRect.height);
  AdjustForBorderPadding(mStyleContext, mInnerBox);
}

nscoord
nsTreeBodyFrame::CalcHorzWidth(const ScrollParts& aParts)
{
  
  
  if (aParts.mColumnsFrame)
    mAdjustWidth = mRect.width - aParts.mColumnsFrame->GetRect().width;
  else
    mAdjustWidth = 0;

  nscoord width = 0;
  nscoord height;

  
  
  

  if (aParts.mColumnsScrollableView) {
    if (NS_FAILED (aParts.mColumnsScrollableView->GetContainerSize(&width, &height)))
      width = 0;
  }

  
  if (width == 0)
    width = mRect.width;

  return width;
}

NS_IMETHODIMP
nsTreeBodyFrame::GetCursor(const nsPoint& aPoint,
                           nsIFrame::Cursor& aCursor)
{
  if (mView) {
    PRInt32 row;
    nsTreeColumn* col;
    nsIAtom* child;
    GetCellAt(aPoint.x, aPoint.y, &row, &col, &child);

    if (child) {
      
      nsStyleContext* childContext = GetPseudoStyleContext(child);

      FillCursorInformationFromStyle(childContext->GetStyleUserInterface(),
                                     aCursor);
      if (aCursor.mCursor == NS_STYLE_CURSOR_AUTO)
        aCursor.mCursor = NS_STYLE_CURSOR_DEFAULT;

      return NS_OK;
    }
  }

  return nsLeafBoxFrame::GetCursor(aPoint, aCursor);
}

NS_IMETHODIMP
nsTreeBodyFrame::HandleEvent(nsPresContext* aPresContext,
                             nsGUIEvent* aEvent,
                             nsEventStatus* aEventStatus)
{
  if (aEvent->message == NS_MOUSE_ENTER_SYNTH || aEvent->message == NS_MOUSE_MOVE) {
    nsPoint pt = nsLayoutUtils::GetEventCoordinatesRelativeTo(aEvent, this);
    PRInt32 xTwips = pt.x - mInnerBox.x;
    PRInt32 yTwips = pt.y - mInnerBox.y;
    PRInt32 newrow = GetRowAt(xTwips, yTwips);
    if (mMouseOverRow != newrow) {
      
      if (mMouseOverRow != -1)
        InvalidateRow(mMouseOverRow);
      mMouseOverRow = newrow;
      if (mMouseOverRow != -1)
        InvalidateRow(mMouseOverRow);
    }
  }
  else if (aEvent->message == NS_MOUSE_EXIT_SYNTH) {
    if (mMouseOverRow != -1) {
      InvalidateRow(mMouseOverRow);
      mMouseOverRow = -1;
    }
  }
  else if (aEvent->message == NS_DRAGDROP_ENTER) {
    if (!mSlots)
      mSlots = new Slots();

    
    

    if (mSlots->mTimer) {
      mSlots->mTimer->Cancel();
      mSlots->mTimer = nsnull;
    }

    
    nsCOMPtr<nsIDragService> dragService = 
             do_GetService("@mozilla.org/widget/dragservice;1");
    dragService->GetCurrentSession(getter_AddRefs(mSlots->mDragSession));
    NS_ASSERTION(mSlots->mDragSession, "can't get drag session");

    if (mSlots->mDragSession)
      mSlots->mDragSession->GetDragAction(&mSlots->mDragAction);
    else
      mSlots->mDragAction = 0;
    mSlots->mDropRow = -1;
    mSlots->mDropOrient = -1;
  }
  else if (aEvent->message == NS_DRAGDROP_OVER) {
    
    
    
    
    
    
    
    

    if (!mView || !mSlots)
      return NS_OK;

    
    PRInt32 lastDropRow = mSlots->mDropRow;
    PRInt16 lastDropOrient = mSlots->mDropOrient;
#ifndef XP_MACOSX
    PRInt16 lastScrollLines = mSlots->mScrollLines;
#endif

    
    PRUint32 lastDragAction = mSlots->mDragAction;
    if (mSlots->mDragSession)
      mSlots->mDragSession->GetDragAction(&mSlots->mDragAction);

    
    
    
    ComputeDropPosition(aEvent, &mSlots->mDropRow, &mSlots->mDropOrient, &mSlots->mScrollLines);

    
    if (mSlots->mScrollLines) {
      if (mSlots->mDropAllowed) {
        
        mSlots->mDropAllowed = PR_FALSE;
        InvalidateDropFeedback(lastDropRow, lastDropOrient);
      }
#ifdef XP_MACOSX
      ScrollByLines(mSlots->mScrollLines);
#else
      if (!lastScrollLines) {
        
        if (mSlots->mTimer) {
          mSlots->mTimer->Cancel();
          mSlots->mTimer = nsnull;
        }

        
        CreateTimer(nsILookAndFeel::eMetric_TreeLazyScrollDelay,
                    LazyScrollCallback, nsITimer::TYPE_ONE_SHOT,
                    getter_AddRefs(mSlots->mTimer));
       }
#endif
      
      return NS_OK;
    }

    
    
    if (mSlots->mDropRow != lastDropRow ||
        mSlots->mDropOrient != lastDropOrient ||
        mSlots->mDragAction != lastDragAction) {

      
      if (mSlots->mDropAllowed) {
        mSlots->mDropAllowed = PR_FALSE;
        InvalidateDropFeedback(lastDropRow, lastDropOrient);
      }

      if (mSlots->mTimer) {
        
        mSlots->mTimer->Cancel();
        mSlots->mTimer = nsnull;
      }

      if (mSlots->mDropRow >= 0) {
        if (!mSlots->mTimer && mSlots->mDropOrient == nsITreeView::DROP_ON) {
          
          
          PRBool isContainer = PR_FALSE;
          mView->IsContainer(mSlots->mDropRow, &isContainer);
          if (isContainer) {
            PRBool isOpen = PR_FALSE;
            mView->IsContainerOpen(mSlots->mDropRow, &isOpen);
            if (!isOpen) {
              
              CreateTimer(nsILookAndFeel::eMetric_TreeOpenDelay,
                          OpenCallback, nsITimer::TYPE_ONE_SHOT,
                          getter_AddRefs(mSlots->mTimer));
            }
          }
        }

        NS_ASSERTION(aEvent->eventStructType == NS_DRAG_EVENT, "wrong event type");
        nsDragEvent* dragEvent = static_cast<nsDragEvent*>(aEvent);
        nsContentUtils::SetDataTransferInEvent(dragEvent);

        PRBool canDropAtNewLocation = PR_FALSE;
        mView->CanDrop(mSlots->mDropRow, mSlots->mDropOrient,
                       dragEvent->dataTransfer, &canDropAtNewLocation);

        if (canDropAtNewLocation) {
          
          mSlots->mDropAllowed = canDropAtNewLocation;
          InvalidateDropFeedback(mSlots->mDropRow, mSlots->mDropOrient);
        }
      }
    }

    
    
    if (mSlots->mDropAllowed && mSlots->mDragSession)
      mSlots->mDragSession->SetCanDrop(PR_TRUE);
  }
  else if (aEvent->message == NS_DRAGDROP_DROP) {
     
     if (!mSlots)
       return NS_OK;

    

    
    PRInt32 parentIndex;
    nsresult rv = mView->GetParentIndex(mSlots->mDropRow, &parentIndex);
    while (NS_SUCCEEDED(rv) && parentIndex >= 0) {
      mSlots->mArray.RemoveElement(parentIndex);
      rv = mView->GetParentIndex(parentIndex, &parentIndex);
    }

    NS_ASSERTION(aEvent->eventStructType == NS_DRAG_EVENT, "wrong event type");
    nsDragEvent* dragEvent = static_cast<nsDragEvent*>(aEvent);
    nsContentUtils::SetDataTransferInEvent(dragEvent);

    mView->Drop(mSlots->mDropRow, mSlots->mDropOrient, dragEvent->dataTransfer);
    mSlots->mDropRow = -1;
    mSlots->mDropOrient = -1;
    *aEventStatus = nsEventStatus_eConsumeNoDefault; 
  }
  else if (aEvent->message == NS_DRAGDROP_EXIT) {
    
    if (!mSlots)
      return NS_OK;

    

    if (mSlots->mDropAllowed) {
      mSlots->mDropAllowed = PR_FALSE;
      InvalidateDropFeedback(mSlots->mDropRow, mSlots->mDropOrient);
    }
    else
      mSlots->mDropAllowed = PR_FALSE;
    mSlots->mDragSession = nsnull;
    mSlots->mScrollLines = 0;
    
    
    
    if (mSlots->mTimer) {
      mSlots->mTimer->Cancel();
      mSlots->mTimer = nsnull;
    }

    if (!mSlots->mArray.IsEmpty()) {
      
      CreateTimer(nsILookAndFeel::eMetric_TreeCloseDelay,
                  CloseCallback, nsITimer::TYPE_ONE_SHOT,
                  getter_AddRefs(mSlots->mTimer));
    }
  }

  return NS_OK;
}


nsLineStyle nsTreeBodyFrame::ConvertBorderStyleToLineStyle(PRUint8 aBorderStyle)
{
  switch (aBorderStyle) {
    case NS_STYLE_BORDER_STYLE_DOTTED:
      return nsLineStyle_kDotted;
    case NS_STYLE_BORDER_STYLE_DASHED:
      return nsLineStyle_kDashed;
    default:
      return nsLineStyle_kSolid;
  }
}

static void
PaintTreeBody(nsIFrame* aFrame, nsIRenderingContext* aCtx,
              const nsRect& aDirtyRect, nsPoint aPt)
{
  static_cast<nsTreeBodyFrame*>(aFrame)->PaintTreeBody(*aCtx, aDirtyRect, aPt);
}


NS_IMETHODIMP
nsTreeBodyFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                  const nsRect&           aDirtyRect,
                                  const nsDisplayListSet& aLists)
{
  
  if (!IsVisibleForPainting(aBuilder))
    return NS_OK; 

  
  nsresult rv = nsLeafBoxFrame::BuildDisplayList(aBuilder, aDirtyRect, aLists);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mView)
    return NS_OK;

  return aLists.Content()->AppendNewToTop(new (aBuilder)
      nsDisplayGeneric(this, ::PaintTreeBody, "XULTreeBody"));
}

void
nsTreeBodyFrame::PaintTreeBody(nsIRenderingContext& aRenderingContext,
                               const nsRect& aDirtyRect, nsPoint aPt)
{
  
  CalcInnerBox();
  aRenderingContext.PushState();
  aRenderingContext.SetClipRect(mInnerBox + aPt, nsClipCombine_kIntersect);
  PRInt32 oldPageCount = mPageLength;
  if (!mHasFixedRowCount)
    mPageLength = mInnerBox.height/mRowHeight;

  if (oldPageCount != mPageLength || mHorzWidth != CalcHorzWidth(GetScrollParts())) {
    
    PresContext()->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eResize, NS_FRAME_IS_DIRTY);
  }
#ifdef DEBUG
  PRInt32 rowCount = mRowCount;
  mView->GetRowCount(&rowCount);
  NS_WARN_IF_FALSE(mRowCount == rowCount, "row count changed unexpectedly");
#endif

  
  
  
  for (nsTreeColumn* currCol = mColumns->GetFirstColumn(); currCol;
       currCol = currCol->GetNext()) {
    nsRect colRect;
    nsresult rv = currCol->GetRect(this, mInnerBox.y, mInnerBox.height,
                                   &colRect);
    
    if (NS_FAILED(rv) || colRect.width == 0) continue;

    if (OffsetForHorzScroll(colRect, PR_FALSE)) {
      nsRect dirtyRect;
      colRect += aPt;
      if (dirtyRect.IntersectRect(aDirtyRect, colRect)) {
        PaintColumn(currCol, colRect, PresContext(), aRenderingContext, aDirtyRect);
      }
    }
  }
  
  for (PRInt32 i = mTopRowIndex; i < mRowCount && i <= mTopRowIndex+mPageLength; i++) {
    nsRect rowRect(mInnerBox.x, mInnerBox.y+mRowHeight*(i-mTopRowIndex), mInnerBox.width, mRowHeight);
    nsRect dirtyRect;
    if (dirtyRect.IntersectRect(aDirtyRect, rowRect + aPt) &&
        rowRect.y < (mInnerBox.y+mInnerBox.height)) {
      PaintRow(i, rowRect + aPt, PresContext(), aRenderingContext, aDirtyRect, aPt);
    }
  }

  if (mSlots && mSlots->mDropAllowed && (mSlots->mDropOrient == nsITreeView::DROP_BEFORE ||
      mSlots->mDropOrient == nsITreeView::DROP_AFTER)) {
    nscoord yPos = mInnerBox.y + mRowHeight * (mSlots->mDropRow - mTopRowIndex) - mRowHeight / 2;
    nsRect feedbackRect(mInnerBox.x, yPos, mInnerBox.width, mRowHeight);
    if (mSlots->mDropOrient == nsITreeView::DROP_AFTER)
      feedbackRect.y += mRowHeight;

    nsRect dirtyRect;
    feedbackRect += aPt;
    if (dirtyRect.IntersectRect(aDirtyRect, feedbackRect)) {
      PaintDropFeedback(feedbackRect, PresContext(), aRenderingContext, aDirtyRect, aPt);
    }
  }
  aRenderingContext.PopState();
}



void
nsTreeBodyFrame::PaintColumn(nsTreeColumn*        aColumn,
                             const nsRect&        aColumnRect,
                             nsPresContext*      aPresContext,
                             nsIRenderingContext& aRenderingContext,
                             const nsRect&        aDirtyRect)
{
  NS_PRECONDITION(aColumn && aColumn->GetFrame(this), "invalid column passed");

  
  PrefillPropertyArray(-1, aColumn);
  mView->GetColumnProperties(aColumn, mScratchArray);

  
  
  nsStyleContext* colContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreecolumn);

  
  
  nsRect colRect(aColumnRect);
  nsMargin colMargin;
  colContext->GetStyleMargin()->GetMargin(colMargin);
  colRect.Deflate(colMargin);

  PaintBackgroundLayer(colContext, aPresContext, aRenderingContext, colRect, aDirtyRect);
}

void
nsTreeBodyFrame::PaintRow(PRInt32              aRowIndex,
                          const nsRect&        aRowRect,
                          nsPresContext*       aPresContext,
                          nsIRenderingContext& aRenderingContext,
                          const nsRect&        aDirtyRect,
                          nsPoint              aPt)
{
  
  
  
  
  if (!mView)
    return;

  nsresult rv;

  
  
  PrefillPropertyArray(aRowIndex, nsnull);
  mView->GetRowProperties(aRowIndex, mScratchArray);

  
  
  nsStyleContext* rowContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreerow);

  
  
  nsRect rowRect(aRowRect);
  nsMargin rowMargin;
  rowContext->GetStyleMargin()->GetMargin(rowMargin);
  rowRect.Deflate(rowMargin);

  
  
  
  PRBool useTheme = PR_FALSE;
  nsITheme *theme = nsnull;
  const nsStyleDisplay* displayData = rowContext->GetStyleDisplay();
  if (displayData->mAppearance) {
    theme = aPresContext->GetTheme();
    if (theme && theme->ThemeSupportsWidget(aPresContext, nsnull, displayData->mAppearance))
      useTheme = PR_TRUE;
  }
  PRBool isSelected = PR_FALSE;
  nsCOMPtr<nsITreeSelection> selection;
  mView->GetSelection(getter_AddRefs(selection));
  if (selection) 
    selection->IsSelected(aRowIndex, &isSelected);
  if (useTheme && !isSelected) {
    nsRect dirty;
    dirty.IntersectRect(rowRect, aDirtyRect);
    theme->DrawWidgetBackground(&aRenderingContext, this, 
                                displayData->mAppearance, rowRect, dirty);
  } else {
    PaintBackgroundLayer(rowContext, aPresContext, aRenderingContext, rowRect, aDirtyRect);
  }
  
  
  nsRect originalRowRect = rowRect;
  AdjustForBorderPadding(rowContext, rowRect);

  PRBool isSeparator = PR_FALSE;
  mView->IsSeparator(aRowIndex, &isSeparator);
  if (isSeparator) {
    

    nscoord primaryX = rowRect.x;
    nsTreeColumn* primaryCol = mColumns->GetPrimaryColumn();
    if (primaryCol) {
      
      nsRect cellRect;
      rv = primaryCol->GetRect(this, rowRect.y, rowRect.height, &cellRect);
      if (NS_FAILED(rv)) {
        NS_NOTREACHED("primary column is invalid");
        return;
      }

      if (OffsetForHorzScroll(cellRect, PR_FALSE)) {
        cellRect.x += aPt.x;
        nsRect dirtyRect;
        nsRect checkRect(cellRect.x, originalRowRect.y,
                         cellRect.width, originalRowRect.height);
        if (dirtyRect.IntersectRect(aDirtyRect, checkRect))
          PaintCell(aRowIndex, primaryCol, cellRect, aPresContext,
                    aRenderingContext, aDirtyRect, primaryX, aPt);
      }

      
      nscoord currX;
      nsTreeColumn* previousCol = primaryCol->GetPrevious();
      if (previousCol) {
        nsRect prevColRect;
        rv = previousCol->GetRect(this, 0, 0, &prevColRect);
        if (NS_SUCCEEDED(rv)) {
          currX = (prevColRect.x - mHorzPosition) + prevColRect.width + aPt.x;
        } else {
          NS_NOTREACHED("The column before the primary column is invalid");
          currX = rowRect.x;
        }
      } else {
        currX = rowRect.x;
      }

      PRInt32 level;
      mView->GetLevel(aRowIndex, &level);
      if (level == 0)
        currX += mIndentation;

      if (currX > rowRect.x) {
        nsRect separatorRect(rowRect);
        separatorRect.width -= rowRect.x + rowRect.width - currX;
        PaintSeparator(aRowIndex, separatorRect, aPresContext, aRenderingContext, aDirtyRect);
      }
    }

    
    nsRect separatorRect(rowRect);
    if (primaryX > rowRect.x) {
      separatorRect.width -= primaryX - rowRect.x;
      separatorRect.x += primaryX - rowRect.x;
    }
    PaintSeparator(aRowIndex, separatorRect, aPresContext, aRenderingContext, aDirtyRect);
  }
  else {
    
    for (nsTreeColumn* currCol = mColumns->GetFirstColumn(); currCol;
         currCol = currCol->GetNext()) {
      nsRect cellRect;
      rv = currCol->GetRect(this, rowRect.y, rowRect.height, &cellRect);
      
      if (NS_FAILED(rv) || cellRect.width == 0)
        continue;

      if (OffsetForHorzScroll(cellRect, PR_FALSE)) {
        cellRect.x += aPt.x;

        
        
        nsRect checkRect = cellRect;
        if (currCol->IsPrimary())
          checkRect = nsRect(cellRect.x, originalRowRect.y,
                             cellRect.width, originalRowRect.height);

        nsRect dirtyRect;
        nscoord dummy;
        if (dirtyRect.IntersectRect(aDirtyRect, checkRect))
          PaintCell(aRowIndex, currCol, cellRect, aPresContext,
                    aRenderingContext, aDirtyRect, dummy, aPt);
      }
    }
  }
}

void
nsTreeBodyFrame::PaintSeparator(PRInt32              aRowIndex,
                                const nsRect&        aSeparatorRect,
                                nsPresContext*      aPresContext,
                                nsIRenderingContext& aRenderingContext,
                                const nsRect&        aDirtyRect)
{
  
  nsStyleContext* separatorContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreeseparator);
  PRBool useTheme = PR_FALSE;
  nsITheme *theme = nsnull;
  const nsStyleDisplay* displayData = separatorContext->GetStyleDisplay();
  if ( displayData->mAppearance ) {
    theme = aPresContext->GetTheme();
    if (theme && theme->ThemeSupportsWidget(aPresContext, nsnull, displayData->mAppearance))
      useTheme = PR_TRUE;
  }

  
  if (useTheme) {
    nsRect dirty;
    dirty.IntersectRect(aSeparatorRect, aDirtyRect);
    theme->DrawWidgetBackground(&aRenderingContext, this,
                                displayData->mAppearance, aSeparatorRect, dirty); 
  }
  else {
    const nsStylePosition* stylePosition = separatorContext->GetStylePosition();

    
    nscoord height;
    if (stylePosition->mHeight.GetUnit() == eStyleUnit_Coord)
      height = stylePosition->mHeight.GetCoordValue();
    else {
      
      height = nsPresContext::CSSPixelsToAppUnits(2);
    }

    
    
    nsRect separatorRect(aSeparatorRect.x, aSeparatorRect.y, aSeparatorRect.width, height);
    nsMargin separatorMargin;
    separatorContext->GetStyleMargin()->GetMargin(separatorMargin);
    separatorRect.Deflate(separatorMargin);

    
    separatorRect.y += (aSeparatorRect.height - height) / 2;

    PaintBackgroundLayer(separatorContext, aPresContext, aRenderingContext, separatorRect, aDirtyRect);
  }
}

void
nsTreeBodyFrame::PaintCell(PRInt32              aRowIndex,
                           nsTreeColumn*        aColumn,
                           const nsRect&        aCellRect,
                           nsPresContext*       aPresContext,
                           nsIRenderingContext& aRenderingContext,
                           const nsRect&        aDirtyRect,
                           nscoord&             aCurrX,
                           nsPoint              aPt)
{
  NS_PRECONDITION(aColumn && aColumn->GetFrame(this), "invalid column passed");

  
  
  PrefillPropertyArray(aRowIndex, aColumn);
  mView->GetCellProperties(aRowIndex, aColumn, mScratchArray);

  
  
  nsStyleContext* cellContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreecell);

  PRBool isRTL = GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL;

  
  
  nsRect cellRect(aCellRect);
  nsMargin cellMargin;
  cellContext->GetStyleMargin()->GetMargin(cellMargin);
  cellRect.Deflate(cellMargin);

  
  PaintBackgroundLayer(cellContext, aPresContext, aRenderingContext, cellRect, aDirtyRect);

  
  AdjustForBorderPadding(cellContext, cellRect);

  nscoord currX = cellRect.x;
  nscoord remainingWidth = cellRect.width;

  
  
  
  

  if (aColumn->IsPrimary()) {
    
    

    PRInt32 level;
    mView->GetLevel(aRowIndex, &level);

    if (!isRTL)
      currX += mIndentation * level;
    remainingWidth -= mIndentation * level;

    
    nsStyleContext* lineContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreeline);
    
    if (mIndentation && level &&
        lineContext->GetStyleVisibility()->IsVisibleOrCollapsed()) {
      

      
      
      
      nsStyleContext* twistyContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreetwisty);

      nsRect imageSize;
      nsRect twistyRect(aCellRect);
      GetTwistyRect(aRowIndex, aColumn, imageSize, twistyRect, aPresContext,
                    aRenderingContext, twistyContext);

      nsMargin twistyMargin;
      twistyContext->GetStyleMargin()->GetMargin(twistyMargin);
      twistyRect.Inflate(twistyMargin);

      aRenderingContext.PushState();

      const nsStyleBorder* borderStyle = lineContext->GetStyleBorder();
      nscolor color;
      PRBool foreground;
      borderStyle->GetBorderColor(NS_SIDE_LEFT, color, foreground);
      if (foreground) {
        
        color = lineContext->GetStyleColor()->mColor;
      }
      aRenderingContext.SetColor(color);
      PRUint8 style;
      style = borderStyle->GetBorderStyle(NS_SIDE_LEFT);
      aRenderingContext.SetLineStyle(ConvertBorderStyleToLineStyle(style));

      nscoord srcX = currX + twistyRect.width - mIndentation / 2;
      nscoord lineY = (aRowIndex - mTopRowIndex) * mRowHeight + aPt.y;

      
      if (srcX <= cellRect.x + cellRect.width) {
        nscoord destX = currX + twistyRect.width;
        if (destX > cellRect.x + cellRect.width)
          destX = cellRect.x + cellRect.width;
        if (isRTL) {
          srcX = currX + remainingWidth - (srcX - cellRect.x);
          destX = currX + remainingWidth - (destX - cellRect.x);
        }
        aRenderingContext.DrawLine(srcX, lineY + mRowHeight / 2, destX, lineY + mRowHeight / 2);
      }

      PRInt32 currentParent = aRowIndex;
      for (PRInt32 i = level; i > 0; i--) {
        if (srcX <= cellRect.x + cellRect.width) {
          
          PRBool hasNextSibling;
          mView->HasNextSibling(currentParent, aRowIndex, &hasNextSibling);
          if (hasNextSibling)
            aRenderingContext.DrawLine(srcX, lineY, srcX, lineY + mRowHeight);
          else if (i == level)
            aRenderingContext.DrawLine(srcX, lineY, srcX, lineY + mRowHeight / 2);
        }

        PRInt32 parent;
        if (NS_FAILED(mView->GetParentIndex(currentParent, &parent)) || parent < 0)
          break;
        currentParent = parent;
        srcX -= mIndentation;
      }

      aRenderingContext.PopState();
    }

    
    nsRect twistyRect(currX, cellRect.y, remainingWidth, cellRect.height);
    PaintTwisty(aRowIndex, aColumn, twistyRect, aPresContext, aRenderingContext, aDirtyRect,
                remainingWidth, currX);
  }
  
  
  nsRect iconRect(currX, cellRect.y, remainingWidth, cellRect.height);
  nsRect dirtyRect;
  if (dirtyRect.IntersectRect(aDirtyRect, iconRect))
    PaintImage(aRowIndex, aColumn, iconRect, aPresContext, aRenderingContext, aDirtyRect,
               remainingWidth, currX);

  
  
  
  if (!aColumn->IsCycler()) {
    nsRect elementRect(currX, cellRect.y, remainingWidth, cellRect.height);
    nsRect dirtyRect;
    if (dirtyRect.IntersectRect(aDirtyRect, elementRect)) {
      PRBool textRTL = cellContext->GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL;
      switch (aColumn->GetType()) {
        case nsITreeColumn::TYPE_TEXT:
          PaintText(aRowIndex, aColumn, elementRect, aPresContext, aRenderingContext, aDirtyRect, currX,
                    textRTL);
          break;
        case nsITreeColumn::TYPE_CHECKBOX:
          PaintCheckbox(aRowIndex, aColumn, elementRect, aPresContext, aRenderingContext, aDirtyRect);
          break;
        case nsITreeColumn::TYPE_PROGRESSMETER:
          PRInt32 state;
          mView->GetProgressMode(aRowIndex, aColumn, &state);
          switch (state) {
            case nsITreeView::PROGRESS_NORMAL:
            case nsITreeView::PROGRESS_UNDETERMINED:
              PaintProgressMeter(aRowIndex, aColumn, elementRect, aPresContext, aRenderingContext, aDirtyRect);
              break;
            case nsITreeView::PROGRESS_NONE:
            default:
              PaintText(aRowIndex, aColumn, elementRect, aPresContext, aRenderingContext, aDirtyRect, currX,
                        textRTL);
              break;
          }
          break;
      }
    }
  }

  aCurrX = currX;
}

void
nsTreeBodyFrame::PaintTwisty(PRInt32              aRowIndex,
                             nsTreeColumn*        aColumn,
                             const nsRect&        aTwistyRect,
                             nsPresContext*      aPresContext,
                             nsIRenderingContext& aRenderingContext,
                             const nsRect&        aDirtyRect,
                             nscoord&             aRemainingWidth,
                             nscoord&             aCurrX)
{
  NS_PRECONDITION(aColumn && aColumn->GetFrame(this), "invalid column passed");

  PRBool isRTL = GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL;
  nscoord rightEdge = aCurrX + aRemainingWidth;
  
  PRBool shouldPaint = PR_FALSE;
  PRBool isContainer = PR_FALSE;
  mView->IsContainer(aRowIndex, &isContainer);
  if (isContainer) {
    PRBool isContainerEmpty = PR_FALSE;
    mView->IsContainerEmpty(aRowIndex, &isContainerEmpty);
    if (!isContainerEmpty)
      shouldPaint = PR_TRUE;
  }

  
  nsStyleContext* twistyContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreetwisty);

  
  
  nsRect twistyRect(aTwistyRect);
  nsMargin twistyMargin;
  twistyContext->GetStyleMargin()->GetMargin(twistyMargin);
  twistyRect.Deflate(twistyMargin);

  nsRect imageSize;
  nsITheme* theme = GetTwistyRect(aRowIndex, aColumn, imageSize, twistyRect,
                                  aPresContext, aRenderingContext, twistyContext);

  
  
  nsRect copyRect(twistyRect);
  copyRect.Inflate(twistyMargin);
  aRemainingWidth -= copyRect.width;
  if (!isRTL)
    aCurrX += copyRect.width;

  if (shouldPaint) {
    
    PaintBackgroundLayer(twistyContext, aPresContext, aRenderingContext, twistyRect, aDirtyRect);

    if (theme) {
      if (isRTL)
        twistyRect.x = rightEdge - twistyRect.width;
      
      
      
      nsRect dirty;
      dirty.IntersectRect(twistyRect, aDirtyRect);
      theme->DrawWidgetBackground(&aRenderingContext, this, 
                                  twistyContext->GetStyleDisplay()->mAppearance, twistyRect, dirty);
    }
    else {
      
      
      nsMargin bp(0,0,0,0);
      GetBorderPadding(twistyContext, bp);
      twistyRect.Deflate(bp);
      if (isRTL)
        twistyRect.x = rightEdge - twistyRect.width;
      imageSize.Deflate(bp);

      
      nsCOMPtr<imgIContainer> image;
      PRBool useImageRegion = PR_TRUE;
      GetImage(aRowIndex, aColumn, PR_TRUE, twistyContext, useImageRegion, getter_AddRefs(image));
      if (image) {
        nsPoint pt = twistyRect.TopLeft();

        
        if (imageSize.height < twistyRect.height) {
          pt.y += (twistyRect.height - imageSize.height)/2;
        }
          
        
        nsLayoutUtils::DrawSingleUnscaledImage(&aRenderingContext, image,
            pt, aDirtyRect, &imageSize);
      }
    }
  }
}

void
nsTreeBodyFrame::PaintImage(PRInt32              aRowIndex,
                            nsTreeColumn*        aColumn,
                            const nsRect&        aImageRect,
                            nsPresContext*       aPresContext,
                            nsIRenderingContext& aRenderingContext,
                            const nsRect&        aDirtyRect,
                            nscoord&             aRemainingWidth,
                            nscoord&             aCurrX)
{
  NS_PRECONDITION(aColumn && aColumn->GetFrame(this), "invalid column passed");

  PRBool isRTL = GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL;
  nscoord rightEdge = aCurrX + aRemainingWidth;
  
  nsStyleContext* imageContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreeimage);

  
  
  nsRect imageRect(aImageRect);
  nsMargin imageMargin;
  imageContext->GetStyleMargin()->GetMargin(imageMargin);
  imageRect.Deflate(imageMargin);

  
  PRBool useImageRegion = PR_TRUE;
  nsCOMPtr<imgIContainer> image;
  GetImage(aRowIndex, aColumn, PR_FALSE, imageContext, useImageRegion, getter_AddRefs(image));

  
  nsSize imageDestSize = GetImageDestSize(imageContext, useImageRegion, image);
  if (!imageDestSize.width || !imageDestSize.height)
    return;

  
  nsMargin bp(0,0,0,0);
  GetBorderPadding(imageContext, bp);

  
  
  nsRect destRect(0, 0, imageDestSize.width, imageDestSize.height);
  
  
  destRect.Inflate(bp);

  
  
  
  
  
  

  if (destRect.width > imageRect.width) {
    
    
    destRect.width = imageRect.width;
  }
  else {
    
    
    if (!aColumn->IsCycler()) {
      
      
      
      imageRect.width = destRect.width;
    }
  }

  if (image) {
    if (isRTL)
      imageRect.x = rightEdge - imageRect.width;
    
    PaintBackgroundLayer(imageContext, aPresContext, aRenderingContext, imageRect, aDirtyRect);

    
    
    destRect.x = imageRect.x;
    destRect.y = imageRect.y;

    if (destRect.width < imageRect.width) {
      
      
      
      destRect.x += (imageRect.width - destRect.width)/2;
    }

    
    if (destRect.height > imageRect.height) {
      
      
      destRect.height = imageRect.height;
    }
    else if (destRect.height < imageRect.height) {
      
      
      
      destRect.y += (imageRect.height - destRect.height)/2;
    }

    
    
    destRect.Deflate(bp);

    
    
    
    nsRect sourceRect = GetImageSourceRect(imageContext, useImageRegion, image);

    
    
    
    
    
    
    
    
    nsIntSize rawImageSize;
    image->GetWidth(&rawImageSize.width);
    image->GetHeight(&rawImageSize.height);
    nsRect wholeImageDest =
      nsLayoutUtils::GetWholeImageDestination(rawImageSize, sourceRect,
          nsRect(destRect.TopLeft(), imageDestSize));

    nsLayoutUtils::DrawImage(&aRenderingContext, image,
        nsLayoutUtils::GetGraphicsFilterForFrame(this),
        wholeImageDest, destRect, destRect.TopLeft(), aDirtyRect);
  }

  
  imageRect.Inflate(imageMargin);
  aRemainingWidth -= imageRect.width;
  if (!isRTL)
    aCurrX += imageRect.width;
}

void
nsTreeBodyFrame::PaintText(PRInt32              aRowIndex,
                           nsTreeColumn*        aColumn,
                           const nsRect&        aTextRect,
                           nsPresContext*      aPresContext,
                           nsIRenderingContext& aRenderingContext,
                           const nsRect&        aDirtyRect,
                           nscoord&             aCurrX,
                           PRBool               aTextRTL)
{
  NS_PRECONDITION(aColumn && aColumn->GetFrame(this), "invalid column passed");

  PRBool isRTL = GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL;
  nscoord rightEdge = aTextRect.XMost();

  
  nsAutoString text;
  mView->GetCellText(aRowIndex, aColumn, text);
  
  
  CheckTextForBidi(text);

  if (text.Length() == 0)
    return; 

  
  
  nsStyleContext* textContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreecelltext);

  
  
  nsRect textRect(aTextRect);
  nsMargin textMargin;
  textContext->GetStyleMargin()->GetMargin(textMargin);
  textRect.Deflate(textMargin);

  
  nsMargin bp(0,0,0,0);
  GetBorderPadding(textContext, bp);
  textRect.Deflate(bp);

  
  nsCOMPtr<nsIFontMetrics> fontMet;
  nsLayoutUtils::GetFontMetricsForStyleContext(textContext,
                                               getter_AddRefs(fontMet));

  nscoord height, baseline;
  fontMet->GetHeight(height);
  fontMet->GetMaxAscent(baseline);

  
  if (height < textRect.height) {
    textRect.y += (textRect.height - height)/2;
    textRect.height = height;
  }

  
  aRenderingContext.SetFont(fontMet);

  AdjustForCellText(text, aRowIndex, aColumn, aRenderingContext, textRect);

  
  nsRect copyRect(textRect);
  copyRect.Inflate(textMargin);
  if (!isRTL)
    aCurrX += copyRect.width;

  textRect.Inflate(bp);
  PaintBackgroundLayer(textContext, aPresContext, aRenderingContext, textRect, aDirtyRect);

  
  textRect.Deflate(bp);
  if (isRTL)
    textRect.x = rightEdge - textRect.width;

  
  aRenderingContext.SetColor(textContext->GetStyleColor()->mColor);

  
  PRUint8 decorations = textContext->GetStyleTextReset()->mTextDecoration;

  nscoord offset;
  nscoord size;
  if (decorations & (NS_FONT_DECORATION_OVERLINE | NS_FONT_DECORATION_UNDERLINE)) {
    fontMet->GetUnderline(offset, size);
    if (decorations & NS_FONT_DECORATION_OVERLINE)
      aRenderingContext.FillRect(textRect.x, textRect.y, textRect.width, size);
    if (decorations & NS_FONT_DECORATION_UNDERLINE)
      aRenderingContext.FillRect(textRect.x, textRect.y + baseline - offset, textRect.width, size);
  }
  if (decorations & NS_FONT_DECORATION_LINE_THROUGH) {
    fontMet->GetStrikeout(offset, size);
    aRenderingContext.FillRect(textRect.x, textRect.y + baseline - offset, textRect.width, size);
  }
#ifdef MOZ_TIMELINE
  NS_TIMELINE_START_TIMER("Render Outline Text");
#endif
  PRUint8 direction = aTextRTL ? NS_STYLE_DIRECTION_RTL :
                                 NS_STYLE_DIRECTION_LTR;

  nsLayoutUtils::DrawString(this, &aRenderingContext, text.get(), text.Length(),
                            textRect.TopLeft() + nsPoint(0, baseline), direction);
#ifdef MOZ_TIMELINE
  NS_TIMELINE_STOP_TIMER("Render Outline Text");
  NS_TIMELINE_MARK_TIMER("Render Outline Text");
#endif
}

void
nsTreeBodyFrame::PaintCheckbox(PRInt32              aRowIndex,
                               nsTreeColumn*        aColumn,
                               const nsRect&        aCheckboxRect,
                               nsPresContext*      aPresContext,
                               nsIRenderingContext& aRenderingContext,
                               const nsRect&        aDirtyRect)
{
  NS_PRECONDITION(aColumn && aColumn->GetFrame(this), "invalid column passed");

  
  nsStyleContext* checkboxContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreecheckbox);

  nscoord rightEdge = aCheckboxRect.XMost();

  
  
  nsRect checkboxRect(aCheckboxRect);
  nsMargin checkboxMargin;
  checkboxContext->GetStyleMargin()->GetMargin(checkboxMargin);
  checkboxRect.Deflate(checkboxMargin);
  
  nsRect imageSize = GetImageSize(aRowIndex, aColumn, PR_TRUE, checkboxContext);

  if (imageSize.height > checkboxRect.height)
    imageSize.height = checkboxRect.height;
  if (imageSize.width > checkboxRect.width)
    imageSize.width = checkboxRect.width;

  if (GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL)
    checkboxRect.x = rightEdge - checkboxRect.width;

  
  PaintBackgroundLayer(checkboxContext, aPresContext, aRenderingContext, checkboxRect, aDirtyRect);

  
  
  nsMargin bp(0,0,0,0);
  GetBorderPadding(checkboxContext, bp);
  checkboxRect.Deflate(bp);

  
  nsCOMPtr<imgIContainer> image;
  PRBool useImageRegion = PR_TRUE;
  GetImage(aRowIndex, aColumn, PR_TRUE, checkboxContext, useImageRegion, getter_AddRefs(image));
  if (image) {
    nsPoint pt = checkboxRect.TopLeft();
          
    if (imageSize.height < checkboxRect.height) {
      pt.y += (checkboxRect.height - imageSize.height)/2;
    }

    if (imageSize.width < checkboxRect.width) {
      pt.x += (checkboxRect.width - imageSize.width)/2;
    }

    
    nsLayoutUtils::DrawSingleUnscaledImage(&aRenderingContext, image,
        pt, aDirtyRect, &imageSize);
  }
}

void
nsTreeBodyFrame::PaintProgressMeter(PRInt32              aRowIndex,
                                    nsTreeColumn*        aColumn,
                                    const nsRect&        aProgressMeterRect,
                                    nsPresContext*      aPresContext,
                                    nsIRenderingContext& aRenderingContext,
                                    const nsRect&        aDirtyRect)
{
  NS_PRECONDITION(aColumn && aColumn->GetFrame(this), "invalid column passed");

  
  
  nsStyleContext* meterContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreeprogressmeter);

  
  
  
  nsRect meterRect(aProgressMeterRect);
  nsMargin meterMargin;
  meterContext->GetStyleMargin()->GetMargin(meterMargin);
  meterRect.Deflate(meterMargin);

  
  PaintBackgroundLayer(meterContext, aPresContext, aRenderingContext, meterRect, aDirtyRect);

  
  PRInt32 state;
  mView->GetProgressMode(aRowIndex, aColumn, &state);
  if (state == nsITreeView::PROGRESS_NORMAL) {
    
    AdjustForBorderPadding(meterContext, meterRect);

    
    aRenderingContext.SetColor(meterContext->GetStyleColor()->mColor);

    
    nsAutoString value;
    mView->GetCellValue(aRowIndex, aColumn, value);

    PRInt32 rv;
    PRInt32 intValue = value.ToInteger(&rv);
    if (intValue < 0)
      intValue = 0;
    else if (intValue > 100)
      intValue = 100;

    nscoord meterWidth = NSToCoordRound((float)intValue / 100 * meterRect.width);
    if (GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL)
      meterRect.x += meterRect.width - meterWidth; 
    meterRect.width = meterWidth;
    PRBool useImageRegion = PR_TRUE;
    nsCOMPtr<imgIContainer> image;
    GetImage(aRowIndex, aColumn, PR_TRUE, meterContext, useImageRegion, getter_AddRefs(image));
    if (image) {
      PRInt32 width, height;
      image->GetWidth(&width);
      image->GetHeight(&height);
      nsSize size(width*nsIDeviceContext::AppUnitsPerCSSPixel(),
                  height*nsIDeviceContext::AppUnitsPerCSSPixel());
      nsLayoutUtils::DrawImage(&aRenderingContext, image,
          nsLayoutUtils::GetGraphicsFilterForFrame(this),
          nsRect(meterRect.TopLeft(), size), meterRect, meterRect.TopLeft(), aDirtyRect);
    } else {
      aRenderingContext.FillRect(meterRect);
    }
  }
  else if (state == nsITreeView::PROGRESS_UNDETERMINED) {
    
    AdjustForBorderPadding(meterContext, meterRect);

    PRBool useImageRegion = PR_TRUE;
    nsCOMPtr<imgIContainer> image;
    GetImage(aRowIndex, aColumn, PR_TRUE, meterContext, useImageRegion, getter_AddRefs(image));
    if (image) {
      PRInt32 width, height;
      image->GetWidth(&width);
      image->GetHeight(&height);
      nsSize size(width*nsIDeviceContext::AppUnitsPerCSSPixel(),
                  height*nsIDeviceContext::AppUnitsPerCSSPixel());
      nsLayoutUtils::DrawImage(&aRenderingContext, image,
          nsLayoutUtils::GetGraphicsFilterForFrame(this),
          nsRect(meterRect.TopLeft(), size), meterRect, meterRect.TopLeft(), aDirtyRect);
    }
  }
}


void
nsTreeBodyFrame::PaintDropFeedback(const nsRect&        aDropFeedbackRect,
                                   nsPresContext*      aPresContext,
                                   nsIRenderingContext& aRenderingContext,
                                   const nsRect&        aDirtyRect,
                                   nsPoint              aPt)
{
  

  nscoord currX;

  
  nsTreeColumn* primaryCol = mColumns->GetPrimaryColumn();

  if (primaryCol) {
#ifdef DEBUG
    nsresult rv =
#endif
      primaryCol->GetXInTwips(this, &currX);
    NS_ASSERTION(NS_SUCCEEDED(rv), "primary column is invalid?");

    currX += aPt.x - mHorzPosition;
  } else {
    currX = aDropFeedbackRect.x;
  }

  PrefillPropertyArray(mSlots->mDropRow, primaryCol);

  
  nsStyleContext* feedbackContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreedropfeedback);

  
  if (feedbackContext->GetStyleVisibility()->IsVisibleOrCollapsed()) {
    PRInt32 level;
    mView->GetLevel(mSlots->mDropRow, &level);

    
    
    if (mSlots->mDropOrient == nsITreeView::DROP_BEFORE) {
      if (mSlots->mDropRow > 0) {
        PRInt32 previousLevel;
        mView->GetLevel(mSlots->mDropRow - 1, &previousLevel);
        if (previousLevel > level)
          level = previousLevel;
      }
    }
    else {
      if (mSlots->mDropRow < mRowCount - 1) {
        PRInt32 nextLevel;
        mView->GetLevel(mSlots->mDropRow + 1, &nextLevel);
        if (nextLevel > level)
          level = nextLevel;
      }
    }

    currX += mIndentation * level;

    if (primaryCol){
      nsStyleContext* twistyContext = GetPseudoStyleContext(nsCSSAnonBoxes::moztreetwisty);
      nsRect imageSize;
      nsRect twistyRect;
      GetTwistyRect(mSlots->mDropRow, primaryCol, imageSize, twistyRect, aPresContext,
                    aRenderingContext, twistyContext);
      nsMargin twistyMargin;
      twistyContext->GetStyleMargin()->GetMargin(twistyMargin);
      twistyRect.Inflate(twistyMargin);
      currX += twistyRect.width;
    }

    const nsStylePosition* stylePosition = feedbackContext->GetStylePosition();

    
    nscoord width;
    if (stylePosition->mWidth.GetUnit() == eStyleUnit_Coord)
      width = stylePosition->mWidth.GetCoordValue();
    else {
      
      width = nsPresContext::CSSPixelsToAppUnits(50);
    }

    
    nscoord height;
    if (stylePosition->mHeight.GetUnit() == eStyleUnit_Coord)
      height = stylePosition->mHeight.GetCoordValue();
    else {
      
      height = nsPresContext::CSSPixelsToAppUnits(2);
    }

    
    
    nsRect feedbackRect(currX, aDropFeedbackRect.y, width, height);
    nsMargin margin;
    feedbackContext->GetStyleMargin()->GetMargin(margin);
    feedbackRect.Deflate(margin);

    feedbackRect.y += (aDropFeedbackRect.height - height) / 2;

    
    PaintBackgroundLayer(feedbackContext, aPresContext, aRenderingContext, feedbackRect, aDirtyRect);
  }
}

void
nsTreeBodyFrame::PaintBackgroundLayer(nsStyleContext*      aStyleContext,
                                      nsPresContext*      aPresContext,
                                      nsIRenderingContext& aRenderingContext,
                                      const nsRect&        aRect,
                                      const nsRect&        aDirtyRect)
{
  const nsStyleBackground* myColor = aStyleContext->GetStyleBackground();
  const nsStyleBorder* myBorder = aStyleContext->GetStyleBorder();
  const nsStyleOutline* myOutline = aStyleContext->GetStyleOutline();
  
  nsCSSRendering::PaintBackgroundWithSC(aPresContext, aRenderingContext,
                                        this, aDirtyRect, aRect,
                                        *myColor, *myBorder, 0);

  nsCSSRendering::PaintBorder(aPresContext, aRenderingContext, this,
                              aDirtyRect, aRect, *myBorder, mStyleContext);

  nsCSSRendering::PaintOutline(aPresContext, aRenderingContext, this,
                               aDirtyRect, aRect, *myBorder, *myOutline,
                               aStyleContext);
}


nsresult
nsTreeBodyFrame::EnsureRowIsVisible(PRInt32 aRow)
{
  ScrollParts parts = GetScrollParts();
  nsresult rv = EnsureRowIsVisibleInternal(parts, aRow);
  NS_ENSURE_SUCCESS(rv, rv);
  UpdateScrollbars(parts);
  return rv;
}

nsresult nsTreeBodyFrame::EnsureRowIsVisibleInternal(const ScrollParts& aParts, PRInt32 aRow)
{
  if (!mView)
    return NS_OK;

  if (mTopRowIndex <= aRow && mTopRowIndex+mPageLength > aRow)
    return NS_OK;

  if (aRow < mTopRowIndex)
    ScrollToRowInternal(aParts, aRow);
  else {
    
    PRInt32 distance = aRow - (mTopRowIndex+mPageLength)+1;
    ScrollToRowInternal(aParts, mTopRowIndex+distance);
  }

  return NS_OK;
}

nsresult
nsTreeBodyFrame::EnsureCellIsVisible(PRInt32 aRow, nsITreeColumn* aCol)
{
  nsRefPtr<nsTreeColumn> col = GetColumnImpl(aCol);
  if (!col)
    return NS_ERROR_INVALID_ARG;

  ScrollParts parts = GetScrollParts();

  nscoord result = -1;
  nsresult rv;

  nscoord columnPos;
  rv = col->GetXInTwips(this, &columnPos);
  if(NS_FAILED(rv)) return rv;

  nscoord columnWidth;
  rv = col->GetWidthInTwips(this, &columnWidth);
  if(NS_FAILED(rv)) return rv;

  
  
  if (columnPos < mHorzPosition)
    result = columnPos;
  
  
  else if ((columnPos + columnWidth) > (mHorzPosition + mInnerBox.width))
    result = ((columnPos + columnWidth) - (mHorzPosition + mInnerBox.width)) + mHorzPosition;

  if (result != -1) {
    rv = ScrollHorzInternal(parts, result);
    if(NS_FAILED(rv)) return rv;
  }

  rv = EnsureRowIsVisibleInternal(parts, aRow);
  NS_ENSURE_SUCCESS(rv, rv);
  UpdateScrollbars(parts);
  return rv;
}

nsresult
nsTreeBodyFrame::ScrollToCell(PRInt32 aRow, nsITreeColumn* aCol)
{
  ScrollParts parts = GetScrollParts();
  nsresult rv = ScrollToRowInternal(parts, aRow);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = ScrollToColumnInternal(parts, aCol);
  NS_ENSURE_SUCCESS(rv, rv);

  UpdateScrollbars(parts);
  return rv;
}

nsresult
nsTreeBodyFrame::ScrollToColumn(nsITreeColumn* aCol)
{
  ScrollParts parts = GetScrollParts();
  nsresult rv = ScrollToColumnInternal(parts, aCol);
  NS_ENSURE_SUCCESS(rv, rv);
  UpdateScrollbars(parts);
  return rv;
}

nsresult nsTreeBodyFrame::ScrollToColumnInternal(const ScrollParts& aParts,
                                                 nsITreeColumn* aCol)
{
  nsRefPtr<nsTreeColumn> col = GetColumnImpl(aCol);
  if (!col)
    return NS_ERROR_INVALID_ARG;

  nscoord x;
  nsresult rv = col->GetXInTwips(this, &x);
  if (NS_FAILED(rv))
    return rv;

  return ScrollHorzInternal(aParts, x);
}

nsresult
nsTreeBodyFrame::ScrollToHorizontalPosition(PRInt32 aHorizontalPosition)
{
  ScrollParts parts = GetScrollParts();
  PRInt32 position = nsPresContext::CSSPixelsToAppUnits(aHorizontalPosition);
  nsresult rv = ScrollHorzInternal(parts, position);
  NS_ENSURE_SUCCESS(rv, rv);
  UpdateScrollbars(parts);
  return rv;
}

nsresult
nsTreeBodyFrame::ScrollToRow(PRInt32 aRow)
{
  ScrollParts parts = GetScrollParts();
  nsresult rv = ScrollToRowInternal(parts, aRow);
  NS_ENSURE_SUCCESS(rv, rv);
  UpdateScrollbars(parts);
  return rv;
}

nsresult nsTreeBodyFrame::ScrollToRowInternal(const ScrollParts& aParts, PRInt32 aRow)
{
  ScrollInternal(aParts, aRow);

  return NS_OK;
}

nsresult
nsTreeBodyFrame::ScrollByLines(PRInt32 aNumLines)
{
  if (!mView)
    return NS_OK;

  PRInt32 newIndex = mTopRowIndex + aNumLines;
  if (newIndex < 0)
    newIndex = 0;
  else {
    PRInt32 lastPageTopRow = mRowCount - mPageLength;
    if (newIndex > lastPageTopRow)
      newIndex = lastPageTopRow;
  }
  ScrollToRow(newIndex);
  
  return NS_OK;
}

nsresult
nsTreeBodyFrame::ScrollByPages(PRInt32 aNumPages)
{
  if (!mView)
    return NS_OK;

  PRInt32 newIndex = mTopRowIndex + aNumPages * mPageLength;
  if (newIndex < 0)
    newIndex = 0;
  else {
    PRInt32 lastPageTopRow = mRowCount - mPageLength;
    if (newIndex > lastPageTopRow)
      newIndex = lastPageTopRow;
  }
  ScrollToRow(newIndex);
    
  return NS_OK;
}

nsresult
nsTreeBodyFrame::ScrollInternal(const ScrollParts& aParts, PRInt32 aRow)
{
  if (!mView)
    return NS_OK;

  PRInt32 delta = aRow - mTopRowIndex;

  if (delta > 0) {
    if (mTopRowIndex == (mRowCount - mPageLength + 1))
      return NS_OK;
  }
  else {
    if (mTopRowIndex == 0)
      return NS_OK;
  }

  mTopRowIndex += delta;

  
  
  const nsStyleBackground* background = GetStyleBackground();
  if (background->BottomLayer().mImage.GetType() != eBackgroundImage_Null ||
      background->mImageCount > 1 ||
      NS_GET_A(background->mBackgroundColor) < 255 ||
      PR_ABS(delta)*mRowHeight >= mRect.height) {
    Invalidate();
  } else {
    nsIWidget* widget = nsLeafBoxFrame::GetView()->GetWidget();
    if (widget) {
      nscoord rowHeightAsPixels =
        PresContext()->AppUnitsToDevPixels(mRowHeight);
      nsIntPoint deltaPt = nsIntPoint(0, -delta*rowHeightAsPixels);

      nsIntRect bounds;
      widget->GetBounds(bounds);
      bounds.x = bounds.y = 0;
      nsTArray<nsIntRect> destRects;
      destRects.AppendElement(bounds);

      
      
      nsTArray<nsIWidget::Configuration> emptyConfigurations;
      widget->Scroll(deltaPt, destRects, emptyConfigurations);
      nsIntRect invalid = bounds;
      if (deltaPt.y < 0) {
        invalid.y = bounds.height + deltaPt.y;
        invalid.height = -deltaPt.y;
      } else {
        invalid.height = deltaPt.y;
      }
      widget->Invalidate(invalid, PR_FALSE);
    }
  }

  PostScrollEvent();
  return NS_OK;
}

nsresult
nsTreeBodyFrame::ScrollHorzInternal(const ScrollParts& aParts, PRInt32 aPosition)
{
  if (!mView || !aParts.mColumnsScrollableView || !aParts.mHScrollbar)
    return NS_OK;

  if (aPosition == mHorzPosition)
    return NS_OK;

  if (aPosition < 0 || aPosition > mHorzWidth)
    return NS_OK;

  nsRect bounds = aParts.mColumnsFrame->GetRect();
  if (aPosition > (mHorzWidth - bounds.width)) 
    aPosition = mHorzWidth - bounds.width;

  PRInt32 delta = aPosition - mHorzPosition;
  mHorzPosition = aPosition;

  
  
  const nsStyleBackground* background = GetStyleBackground();
  if (background->BottomLayer().mImage.GetType() != eBackgroundImage_Null ||
      background->mImageCount > 1 ||
      NS_GET_A(background->mBackgroundColor) < 255 ||
      PR_ABS(delta) >= mRect.width) {
    Invalidate();
  } else {
    nsIWidget* widget = nsLeafBoxFrame::GetView()->GetWidget();
    if (widget) {
      nsIntPoint deltaPt(PresContext()->AppUnitsToDevPixels(-delta), 0);

      nsIntRect bounds;
      widget->GetBounds(bounds);
      bounds.x = bounds.y = 0;
      nsTArray<nsIntRect> destRects;
      destRects.AppendElement(bounds);

      
      
      nsTArray<nsIWidget::Configuration> emptyConfigurations;
      widget->Scroll(deltaPt, destRects, emptyConfigurations);
      nsIntRect invalid = bounds;
      if (deltaPt.x < 0) {
        invalid.x = bounds.width + deltaPt.x;
        invalid.width = -deltaPt.x;
      } else {
        invalid.width = deltaPt.x;
      }
      widget->Invalidate(invalid, PR_FALSE);
    }
  }

  
  aParts.mColumnsScrollableView->ScrollTo(mHorzPosition, 0, 0);

  
  PostScrollEvent();
  return NS_OK;
}

NS_IMETHODIMP
nsTreeBodyFrame::ScrollbarButtonPressed(nsIScrollbarFrame* aScrollbar, PRInt32 aOldIndex, PRInt32 aNewIndex)
{
  ScrollParts parts = GetScrollParts();

  if (aScrollbar == parts.mVScrollbar) {
    if (aNewIndex > aOldIndex)
      ScrollToRowInternal(parts, mTopRowIndex+1);
    else if (aNewIndex < aOldIndex)
      ScrollToRowInternal(parts, mTopRowIndex-1);
  } else {
    ScrollHorzInternal(parts, aNewIndex);
  }

  UpdateScrollbars(parts);

  return NS_OK;
}
  
NS_IMETHODIMP
nsTreeBodyFrame::PositionChanged(nsIScrollbarFrame* aScrollbar, PRInt32 aOldIndex, PRInt32& aNewIndex)
{
  ScrollParts parts = GetScrollParts();
  
  if (aOldIndex == aNewIndex)
    return NS_OK;

  
  if (parts.mVScrollbar == aScrollbar) {
    nscoord rh = nsPresContext::AppUnitsToIntCSSPixels(mRowHeight);

    nscoord newrow = aNewIndex/rh;
    ScrollInternal(parts, newrow);
  
  } else if (parts.mHScrollbar == aScrollbar) {
    ScrollHorzInternal(parts, aNewIndex);
  }

  UpdateScrollbars(parts);
  return NS_OK;
}


nsStyleContext*
nsTreeBodyFrame::GetPseudoStyleContext(nsIAtom* aPseudoElement)
{
  return mStyleCache.GetStyleContext(this, PresContext(), mContent,
                                     mStyleContext, aPseudoElement,
                                     mScratchArray);
}


NS_IMETHODIMP
nsTreeBodyFrame::PseudoMatches(nsIAtom* aTag, nsCSSSelector* aSelector, PRBool* aResult)
{
  NS_ABORT_IF_FALSE(aSelector->mLowercaseTag == aTag,
                   "should not have been called");
  
  
  
  
  nsPseudoClassList* curr = aSelector->mPseudoClassList;
  while (curr) {
    PRInt32 index;
    mScratchArray->GetIndexOf(curr->mAtom, &index);
    if (index == -1) {
      *aResult = PR_FALSE;
      return NS_OK;
    }
    curr = curr->mNext;
  }
  *aResult = PR_TRUE;
  return NS_OK;
}

nsIContent*
nsTreeBodyFrame::GetBaseElement()
{
  nsIFrame* parent = GetParent();
  while (parent) {
    nsIContent* content = parent->GetContent();
    if (content) {
      nsINodeInfo* ni = content->NodeInfo();

      if (ni->Equals(nsGkAtoms::tree, kNameSpaceID_XUL) ||
          (ni->Equals(nsGkAtoms::select) &&
           content->IsNodeOfType(nsINode::eHTML)))
        return content;
    }

    parent = parent->GetParent();
  }

  return nsnull;
}

nsresult
nsTreeBodyFrame::ClearStyleAndImageCaches()
{
  mStyleCache.Clear();
  mImageCache.EnumerateRead(CancelImageRequest, nsnull);
  mImageCache.Clear();
  return NS_OK;
}

 void
nsTreeBodyFrame::DidSetStyleContext(nsStyleContext* aOldStyleContext)
{
  nsLeafBoxFrame::DidSetStyleContext(aOldStyleContext);

  
  mStyleCache.Clear();
  
  
  
  mIndentation = GetIndentation();
  mRowHeight = GetRowHeight();
  mStringWidth = -1;
}

PRBool 
nsTreeBodyFrame::OffsetForHorzScroll(nsRect& rect, PRBool clip)
{
  rect.x -= mHorzPosition;

  
  if (rect.XMost() <= mInnerBox.x)
    return PR_FALSE;

  
  if (rect.x > mInnerBox.XMost())
    return PR_FALSE;

  if (clip) {
    nscoord leftEdge = PR_MAX(rect.x, mInnerBox.x);
    nscoord rightEdge = PR_MIN(rect.XMost(), mInnerBox.XMost());
    rect.x = leftEdge;
    rect.width = rightEdge - leftEdge;

    
    NS_ASSERTION(rect.width >= 0, "horz scroll code out of sync");
  }

  return PR_TRUE;
}

PRBool
nsTreeBodyFrame::CanAutoScroll(PRInt32 aRowIndex)
{
  
  if (aRowIndex == mRowCount - 1) {
    nscoord y = mInnerBox.y + (aRowIndex - mTopRowIndex) * mRowHeight;
    if (y < mInnerBox.height && y + mRowHeight > mInnerBox.height)
      return PR_TRUE;
  }

  if (aRowIndex > 0 && aRowIndex < mRowCount - 1)
    return PR_TRUE;

  return PR_FALSE;
}











void 
nsTreeBodyFrame::ComputeDropPosition(nsGUIEvent* aEvent, PRInt32* aRow, PRInt16* aOrient,
                                     PRInt16* aScrollLines)
{
  *aOrient = -1;
  *aScrollLines = 0;

  
  
  nsPoint pt = nsLayoutUtils::GetEventCoordinatesRelativeTo(aEvent, this);
  PRInt32 xTwips = pt.x - mInnerBox.x;
  PRInt32 yTwips = pt.y - mInnerBox.y;

  *aRow = GetRowAt(xTwips, yTwips);
  if (*aRow >=0) {
    
    PRInt32 yOffset = yTwips - mRowHeight * (*aRow - mTopRowIndex);
   
    PRBool isContainer = PR_FALSE;
    mView->IsContainer (*aRow, &isContainer);
    if (isContainer) {
      
      if (yOffset < mRowHeight / 4)
        *aOrient = nsITreeView::DROP_BEFORE;
      else if (yOffset > mRowHeight - (mRowHeight / 4))
        *aOrient = nsITreeView::DROP_AFTER;
      else
        *aOrient = nsITreeView::DROP_ON;
    }
    else {
      
      if (yOffset < mRowHeight / 2)
        *aOrient = nsITreeView::DROP_BEFORE;
      else
        *aOrient = nsITreeView::DROP_AFTER;
    }
  }

  if (CanAutoScroll(*aRow)) {
    
    PRInt32 scrollLinesMax = 0;
    PresContext()->LookAndFeel()->
      GetMetric(nsILookAndFeel::eMetric_TreeScrollLinesMax, scrollLinesMax);
    scrollLinesMax--;
    if (scrollLinesMax < 0)
      scrollLinesMax = 0;

    
    
    nscoord height = (3 * mRowHeight) / 4;
    if (yTwips < height) {
      
      *aScrollLines = NSToIntRound(-scrollLinesMax * (1 - (float)yTwips / height) - 1);
    }
    else if (yTwips > mRect.height - height) {
      
      *aScrollLines = NSToIntRound(scrollLinesMax * (1 - (float)(mRect.height - yTwips) / height) + 1);
    }
  }
} 

void
nsTreeBodyFrame::OpenCallback(nsITimer *aTimer, void *aClosure)
{
  nsTreeBodyFrame* self = static_cast<nsTreeBodyFrame*>(aClosure);
  if (self) {
    aTimer->Cancel();
    self->mSlots->mTimer = nsnull;

    if (self->mSlots->mDropRow >= 0) {
      self->mSlots->mArray.AppendElement(self->mSlots->mDropRow);
      self->mView->ToggleOpenState(self->mSlots->mDropRow);
    }
  }
}

void
nsTreeBodyFrame::CloseCallback(nsITimer *aTimer, void *aClosure)
{
  nsTreeBodyFrame* self = static_cast<nsTreeBodyFrame*>(aClosure);
  if (self) {
    aTimer->Cancel();
    self->mSlots->mTimer = nsnull;

    for (PRUint32 i = self->mSlots->mArray.Length(); i--; ) {
      if (self->mView)
        self->mView->ToggleOpenState(self->mSlots->mArray[i]);
    }
    self->mSlots->mArray.Clear();
  }
}

void
nsTreeBodyFrame::LazyScrollCallback(nsITimer *aTimer, void *aClosure)
{
  nsTreeBodyFrame* self = static_cast<nsTreeBodyFrame*>(aClosure);
  if (self) {
    aTimer->Cancel();
    self->mSlots->mTimer = nsnull;

    if (self->mView) {
      
      self->CreateTimer(nsILookAndFeel::eMetric_TreeScrollDelay,
                        ScrollCallback, nsITimer::TYPE_REPEATING_SLACK,
                        getter_AddRefs(self->mSlots->mTimer));
      self->ScrollByLines(self->mSlots->mScrollLines);
      
    }
  }
}

void
nsTreeBodyFrame::ScrollCallback(nsITimer *aTimer, void *aClosure)
{
  nsTreeBodyFrame* self = static_cast<nsTreeBodyFrame*>(aClosure);
  if (self) {
    
    if (self->mView && self->CanAutoScroll(self->mSlots->mDropRow)) {
      self->ScrollByLines(self->mSlots->mScrollLines);
    }
    else {
      aTimer->Cancel();
      self->mSlots->mTimer = nsnull;
    }
  }
}

NS_IMETHODIMP
nsTreeBodyFrame::ScrollEvent::Run()
{
  if (mInner) {
    mInner->FireScrollEvent();
  }
  return NS_OK;
}


void
nsTreeBodyFrame::FireScrollEvent()
{
  mScrollEvent.Forget();
  nsScrollbarEvent event(PR_TRUE, NS_SCROLL_EVENT, nsnull);
  
  event.flags |= NS_EVENT_FLAG_CANT_BUBBLE;
  nsEventDispatcher::Dispatch(GetContent(), PresContext(), &event);
}

void
nsTreeBodyFrame::PostScrollEvent()
{
  if (mScrollEvent.IsPending())
    return;

  nsRefPtr<ScrollEvent> ev = new ScrollEvent(this);
  if (NS_FAILED(NS_DispatchToCurrentThread(ev))) {
    NS_WARNING("failed to dispatch ScrollEvent");
  } else {
    mScrollEvent = ev;
  }
}

#ifdef ACCESSIBILITY
void
nsTreeBodyFrame::FireRowCountChangedEvent(PRInt32 aIndex, PRInt32 aCount)
{
  nsCOMPtr<nsIContent> content(GetBaseElement());
  if (!content)
    return;

  nsCOMPtr<nsIDOMDocumentEvent> domEventDoc =
    do_QueryInterface(content->GetOwnerDoc());
  if (!domEventDoc)
    return;

  nsCOMPtr<nsIDOMEvent> event;
  domEventDoc->CreateEvent(NS_LITERAL_STRING("datacontainerevents"),
                           getter_AddRefs(event));

  nsCOMPtr<nsIDOMDataContainerEvent> treeEvent(do_QueryInterface(event));
  if (!treeEvent)
    return;

  event->InitEvent(NS_LITERAL_STRING("TreeRowCountChanged"), PR_TRUE, PR_FALSE);

  
  nsCOMPtr<nsIWritableVariant> indexVariant(
    do_CreateInstance("@mozilla.org/variant;1"));
  if (!indexVariant)
    return;

  indexVariant->SetAsInt32(aIndex);
  treeEvent->SetData(NS_LITERAL_STRING("index"), indexVariant);

  
  nsCOMPtr<nsIWritableVariant> countVariant(
    do_CreateInstance("@mozilla.org/variant;1"));
  if (!countVariant)
    return;

  countVariant->SetAsInt32(aCount);
  treeEvent->SetData(NS_LITERAL_STRING("count"), countVariant);

  
  nsCOMPtr<nsIPrivateDOMEvent> privateEvent(do_QueryInterface(event));
  if (!privateEvent)
    return;

  privateEvent->SetTrusted(PR_TRUE);

  nsRefPtr<nsPLDOMEvent> plevent = new nsPLDOMEvent(content, event);
  if (!plevent)
    return;

  plevent->PostDOMEvent();
}

void
nsTreeBodyFrame::FireInvalidateEvent(PRInt32 aStartRowIdx, PRInt32 aEndRowIdx,
                                     nsITreeColumn *aStartCol,
                                     nsITreeColumn *aEndCol)
{
  nsCOMPtr<nsIContent> content(GetBaseElement());
  if (!content)
    return;

  nsCOMPtr<nsIDOMDocumentEvent> domEventDoc =
    do_QueryInterface(content->GetOwnerDoc());
  if (!domEventDoc)
    return;

  nsCOMPtr<nsIDOMEvent> event;
  domEventDoc->CreateEvent(NS_LITERAL_STRING("datacontainerevents"),
                           getter_AddRefs(event));

  nsCOMPtr<nsIDOMDataContainerEvent> treeEvent(do_QueryInterface(event));
  if (!treeEvent)
    return;

  event->InitEvent(NS_LITERAL_STRING("TreeInvalidated"), PR_TRUE, PR_FALSE);

  if (aStartRowIdx != -1 && aEndRowIdx != -1) {
    
    nsCOMPtr<nsIWritableVariant> startRowVariant(
      do_CreateInstance("@mozilla.org/variant;1"));
    if (!startRowVariant)
      return;

    startRowVariant->SetAsInt32(aStartRowIdx);
    treeEvent->SetData(NS_LITERAL_STRING("startrow"), startRowVariant);

    
    nsCOMPtr<nsIWritableVariant> endRowVariant(
      do_CreateInstance("@mozilla.org/variant;1"));
    if (!endRowVariant)
      return;

    endRowVariant->SetAsInt32(aEndRowIdx);
    treeEvent->SetData(NS_LITERAL_STRING("endrow"), endRowVariant);
  }

  if (aStartCol && aEndCol) {
    
    nsCOMPtr<nsIWritableVariant> startColVariant(
      do_CreateInstance("@mozilla.org/variant;1"));
    if (!startColVariant)
      return;

    PRInt32 startColIdx = 0;
    nsresult rv = aStartCol->GetIndex(&startColIdx);
    if (NS_FAILED(rv))
      return;

    startColVariant->SetAsInt32(startColIdx);
    treeEvent->SetData(NS_LITERAL_STRING("startcolumn"), startColVariant);

    
    nsCOMPtr<nsIWritableVariant> endColVariant(
      do_CreateInstance("@mozilla.org/variant;1"));
    if (!endColVariant)
      return;

    PRInt32 endColIdx = 0;
    rv = aEndCol->GetIndex(&endColIdx);
    if (NS_FAILED(rv))
      return;

    endColVariant->SetAsInt32(endColIdx);
    treeEvent->SetData(NS_LITERAL_STRING("endcolumn"), endColVariant);
  }

  
  nsCOMPtr<nsIPrivateDOMEvent> privateEvent(do_QueryInterface(event));
  if (!privateEvent)
    return;

  privateEvent->SetTrusted(PR_TRUE);

  nsRefPtr<nsPLDOMEvent> plevent = new nsPLDOMEvent(content, event);
  if (plevent)
    plevent->PostDOMEvent();
}
#endif

class nsOverflowChecker : public nsRunnable
{
public:
  nsOverflowChecker(nsTreeBodyFrame* aFrame) : mFrame(aFrame) {}
  NS_IMETHOD Run()
  {
    if (mFrame.IsAlive()) {
      nsTreeBodyFrame* tree = static_cast<nsTreeBodyFrame*>(mFrame.GetFrame());
      nsTreeBodyFrame::ScrollParts parts = tree->GetScrollParts();
      tree->CheckOverflow(parts);
    }
    return NS_OK;
  }
private:
  nsWeakFrame mFrame;
};

PRBool
nsTreeBodyFrame::FullScrollbarsUpdate(PRBool aNeedsFullInvalidation)
{
  ScrollParts parts = GetScrollParts();
  nsWeakFrame weakFrame(this);
  nsWeakFrame weakColumnsFrame(parts.mColumnsFrame);
  UpdateScrollbars(parts);
  NS_ENSURE_TRUE(weakFrame.IsAlive(), PR_FALSE);
  if (aNeedsFullInvalidation) {
    Invalidate();
  }
  InvalidateScrollbars(parts, weakColumnsFrame);
  NS_ENSURE_TRUE(weakFrame.IsAlive(), PR_FALSE);
  nsContentUtils::AddScriptRunner(new nsOverflowChecker(this));
  return weakFrame.IsAlive();
}
