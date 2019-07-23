







































#include "nsTreeBoxObject.h"
#include "nsCOMPtr.h"
#include "nsIDOMXULElement.h"
#include "nsIXULTemplateBuilder.h"
#include "nsTreeContentView.h"
#include "nsITreeSelection.h"
#include "nsChildIterator.h"
#include "nsContentUtils.h"
#include "nsDOMError.h"
#include "nsTreeBodyFrame.h"

NS_IMPL_ISUPPORTS_INHERITED1(nsTreeBoxObject, nsBoxObject, nsITreeBoxObject)

void
nsTreeBoxObject::Clear()
{
  ClearCachedValues();

  
  if (mView) {
    nsCOMPtr<nsITreeSelection> sel;
    mView->GetSelection(getter_AddRefs(sel));
    if (sel)
      sel->SetTree(nsnull);
    mView->SetTree(nsnull); 
  }
  mView = nsnull;

  nsBoxObject::Clear();
}


nsTreeBoxObject::nsTreeBoxObject()
  : mTreeBody(nsnull)
{
}

nsTreeBoxObject::~nsTreeBoxObject()
{
  
}


static void FindBodyElement(nsIContent* aParent, nsIContent** aResult)
{
  *aResult = nsnull;
  ChildIterator iter, last;
  for (ChildIterator::Init(aParent, &iter, &last); iter != last; ++iter) {
    nsCOMPtr<nsIContent> content = *iter;

    nsINodeInfo *ni = content->NodeInfo();
    if (ni->Equals(nsGkAtoms::treechildren, kNameSpaceID_XUL)) {
      *aResult = content;
      NS_ADDREF(*aResult);
      break;
    } else if (ni->Equals(nsGkAtoms::tree, kNameSpaceID_XUL)) {
      
      
      break;
    } else if (content->IsNodeOfType(nsINode::eELEMENT) &&
               !ni->Equals(nsGkAtoms::_template, kNameSpaceID_XUL)) {
      FindBodyElement(content, aResult);
      if (*aResult)
        break;
    }
  }
}

nsITreeBoxObject*
nsTreeBoxObject::GetTreeBody()
{
  if (mTreeBody) {
    return mTreeBody;
  }

  nsIFrame* frame = GetFrame(PR_FALSE);
  if (!frame)
    return nsnull;

  
  nsCOMPtr<nsIContent> content;
  FindBodyElement(frame->GetContent(), getter_AddRefs(content));

  nsIPresShell* shell = GetPresShell(PR_FALSE);
  if (!shell) {
    return nsnull;
  }

  frame = shell->GetPrimaryFrameFor(content);
  if (!frame)
     return nsnull;

  
  
  
  nsITreeBoxObject* innerTreeBoxObject = nsnull;
  CallQueryInterface(frame, &innerTreeBoxObject);
  NS_ENSURE_TRUE(innerTreeBoxObject &&
    NS_STATIC_CAST(nsTreeBodyFrame*, innerTreeBoxObject)->GetTreeBoxObject() ==
    NS_STATIC_CAST(nsITreeBoxObject*, this), nsnull);

  mTreeBody = innerTreeBoxObject;
  return mTreeBody;
}

NS_IMETHODIMP nsTreeBoxObject::GetView(nsITreeView * *aView)
{
  if (!mTreeBody) {
    if (!GetTreeBody()) {
      
      *aView = nsnull;
      return NS_OK;
    }

    if (mView)
      
      return mTreeBody->GetView(aView);
  }
  if (!mView) {
    nsCOMPtr<nsIDOMXULElement> xulele = do_QueryInterface(mContent);
    if (xulele) {
      
      nsCOMPtr<nsIXULTemplateBuilder> builder;
      xulele->GetBuilder(getter_AddRefs(builder));
      mView = do_QueryInterface(builder);

      if (!mView) {
        
        nsresult rv = NS_NewTreeContentView(getter_AddRefs(mView));
        NS_ENSURE_SUCCESS(rv, rv);
      }

      
      mTreeBody->SetView(mView);
    }
  }
  NS_IF_ADDREF(*aView = mView);
  return NS_OK;
}

static PRBool
CanTrustView(nsISupports* aValue)
{
  
  if (nsContentUtils::IsCallerTrustedForWrite())
    return PR_TRUE;
  nsCOMPtr<nsINativeTreeView> nativeTreeView = do_QueryInterface(aValue);
  if (!nativeTreeView || NS_FAILED(nativeTreeView->EnsureNative())) {
    
    return PR_FALSE;
  }
  return PR_TRUE;
}

NS_IMETHODIMP nsTreeBoxObject::SetView(nsITreeView * aView)
{
  if (!CanTrustView(aView))
    return NS_ERROR_DOM_SECURITY_ERR;
  
  mView = aView;
  nsITreeBoxObject* body = GetTreeBody();
  if (body)
    body->SetView(aView);

  return NS_OK;
}

NS_IMETHODIMP nsTreeBoxObject::GetFocused(PRBool* aFocused)
{
  nsITreeBoxObject* body = GetTreeBody();
  if (body)
    return body->GetFocused(aFocused);
  return NS_OK;
}

NS_IMETHODIMP nsTreeBoxObject::SetFocused(PRBool aFocused)
{
  nsITreeBoxObject* body = GetTreeBody();
  if (body)
    return body->SetFocused(aFocused);
  return NS_OK;
}

NS_IMETHODIMP nsTreeBoxObject::GetTreeBody(nsIDOMElement** aElement)
{
  nsITreeBoxObject* body = GetTreeBody();
  if (body) 
    return body->GetTreeBody(aElement);
  return NS_OK;
}

NS_IMETHODIMP nsTreeBoxObject::GetColumns(nsITreeColumns** aColumns)
{
  nsITreeBoxObject* body = GetTreeBody();
  if (body) 
    return body->GetColumns(aColumns);
  return NS_OK;
}

NS_IMETHODIMP nsTreeBoxObject::GetRowHeight(PRInt32* _retval)
{
  nsITreeBoxObject* body = GetTreeBody();
  if (body) 
    return body->GetRowHeight(_retval);
  return NS_OK;
}

NS_IMETHODIMP nsTreeBoxObject::GetRowWidth(PRInt32 *aRowWidth)
{
  nsITreeBoxObject* body = GetTreeBody();
  if (body) 
    return body->GetRowWidth(aRowWidth);
  return NS_OK;
}

NS_IMETHODIMP nsTreeBoxObject::GetFirstVisibleRow(PRInt32 *_retval)
{
  nsITreeBoxObject* body = GetTreeBody();
  if (body)
    return body->GetFirstVisibleRow(_retval);
  return NS_OK;
}

NS_IMETHODIMP nsTreeBoxObject::GetLastVisibleRow(PRInt32 *_retval)
{
  nsITreeBoxObject* body = GetTreeBody();
  if (body)
    return body->GetLastVisibleRow(_retval);
  return NS_OK;
}

NS_IMETHODIMP nsTreeBoxObject::GetHorizontalPosition(PRInt32 *aHorizontalPosition)
{
  nsITreeBoxObject* body = GetTreeBody();
  if (body)
    return body->GetHorizontalPosition(aHorizontalPosition);
  return NS_OK;
}

NS_IMETHODIMP nsTreeBoxObject::GetPageLength(PRInt32 *_retval)
{
  nsITreeBoxObject* body = GetTreeBody();
  if (body)
    return body->GetPageLength(_retval);
  return NS_OK;
}

NS_IMETHODIMP
nsTreeBoxObject::EnsureRowIsVisible(PRInt32 aRow)
{
  nsITreeBoxObject* body = GetTreeBody();
  if (body)
    return body->EnsureRowIsVisible(aRow);
  return NS_OK;
}

NS_IMETHODIMP 
nsTreeBoxObject::EnsureCellIsVisible(PRInt32 aRow, nsITreeColumn* aCol)
{
  nsITreeBoxObject* body = GetTreeBody();
  if (body)
    return body->EnsureCellIsVisible(aRow, aCol);
  return NS_OK;
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsTreeBoxObject::ScrollToRow(PRInt32 aRow)
{
  nsITreeBoxObject* body = GetTreeBody();
  if (body)
    return body->ScrollToRow(aRow);
  return NS_OK;
}

NS_IMETHODIMP
nsTreeBoxObject::ScrollByLines(PRInt32 aNumLines)
{
  nsITreeBoxObject* body = GetTreeBody();
  if (body)
    return body->ScrollByLines(aNumLines);
  return NS_OK;
}

NS_IMETHODIMP
nsTreeBoxObject::ScrollByPages(PRInt32 aNumPages)
{
  nsITreeBoxObject* body = GetTreeBody();
  if (body)
    return body->ScrollByPages(aNumPages);
  return NS_OK;
}

NS_IMETHODIMP 
nsTreeBoxObject::ScrollToCell(PRInt32 aRow, nsITreeColumn* aCol)
{
  nsITreeBoxObject* body = GetTreeBody();
  if (body)
    return body->ScrollToCell(aRow, aCol);
  return NS_OK;
}

NS_IMETHODIMP 
nsTreeBoxObject::ScrollToColumn(nsITreeColumn* aCol)
{
  nsITreeBoxObject* body = GetTreeBody();
  if (body)
    return body->ScrollToColumn(aCol);
  return NS_OK;
}

NS_IMETHODIMP 
nsTreeBoxObject::ScrollToHorizontalPosition(PRInt32 aHorizontalPosition)
{
  nsITreeBoxObject* body = GetTreeBody();
  if (body)
    return body->ScrollToHorizontalPosition(aHorizontalPosition);
  return NS_OK;
}

NS_IMETHODIMP nsTreeBoxObject::Invalidate()
{
  nsITreeBoxObject* body = GetTreeBody();
  if (body)
    return body->Invalidate();
  return NS_OK;
}

NS_IMETHODIMP nsTreeBoxObject::InvalidateColumn(nsITreeColumn* aCol)
{
  nsITreeBoxObject* body = GetTreeBody();
  if (body)
    return body->InvalidateColumn(aCol);
  return NS_OK;
}

NS_IMETHODIMP nsTreeBoxObject::InvalidateRow(PRInt32 aIndex)
{
  nsITreeBoxObject* body = GetTreeBody();
  if (body)
    return body->InvalidateRow(aIndex);
  return NS_OK;
}

NS_IMETHODIMP nsTreeBoxObject::InvalidateCell(PRInt32 aRow, nsITreeColumn* aCol)
{
  nsITreeBoxObject* body = GetTreeBody();
  if (body)
    return body->InvalidateCell(aRow, aCol);
  return NS_OK;
}

NS_IMETHODIMP nsTreeBoxObject::InvalidateRange(PRInt32 aStart, PRInt32 aEnd)
{
  nsITreeBoxObject* body = GetTreeBody();
  if (body)
    return body->InvalidateRange(aStart, aEnd);
  return NS_OK;
}

NS_IMETHODIMP nsTreeBoxObject::InvalidateColumnRange(PRInt32 aStart, PRInt32 aEnd, nsITreeColumn* aCol)
{
  nsITreeBoxObject* body = GetTreeBody();
  if (body)
    return body->InvalidateColumnRange(aStart, aEnd, aCol);
  return NS_OK;
}

NS_IMETHODIMP nsTreeBoxObject::GetRowAt(PRInt32 x, PRInt32 y, PRInt32 *_retval)
{
  nsITreeBoxObject* body = GetTreeBody();
  if (body)
    return body->GetRowAt(x, y, _retval);
  return NS_OK;
}

NS_IMETHODIMP nsTreeBoxObject::GetCellAt(PRInt32 x, PRInt32 y, PRInt32 *row, nsITreeColumn** col,
                                         nsACString& childElt)
{
  nsITreeBoxObject* body = GetTreeBody();
  if (body)
    return body->GetCellAt(x, y, row, col, childElt);
  return NS_OK;
}

NS_IMETHODIMP
nsTreeBoxObject::GetCoordsForCellItem(PRInt32 aRow, nsITreeColumn* aCol, const nsACString& aElement, 
                                      PRInt32 *aX, PRInt32 *aY, PRInt32 *aWidth, PRInt32 *aHeight)
{
  nsITreeBoxObject* body = GetTreeBody();
  if (body)
    return body->GetCoordsForCellItem(aRow, aCol, aElement, aX, aY, aWidth, aHeight);
  return NS_OK;
}

NS_IMETHODIMP
nsTreeBoxObject::IsCellCropped(PRInt32 aRow, nsITreeColumn* aCol, PRBool *_retval)
{  
  nsITreeBoxObject* body = GetTreeBody();
  if (body)
    return body->IsCellCropped(aRow, aCol, _retval);
  return NS_OK;
}

NS_IMETHODIMP nsTreeBoxObject::RowCountChanged(PRInt32 aIndex, PRInt32 aDelta)
{
  nsITreeBoxObject* body = GetTreeBody();
  if (body)
    return body->RowCountChanged(aIndex, aDelta);
  return NS_OK;
}

NS_IMETHODIMP nsTreeBoxObject::BeginUpdateBatch()
{
  nsITreeBoxObject* body = GetTreeBody();
  if (body)
    return body->BeginUpdateBatch();
  return NS_OK;
}

NS_IMETHODIMP nsTreeBoxObject::EndUpdateBatch()
{
  nsITreeBoxObject* body = GetTreeBody();
  if (body)
    return body->EndUpdateBatch();
  return NS_OK;
}

NS_IMETHODIMP nsTreeBoxObject::ClearStyleAndImageCaches()
{
  nsITreeBoxObject* body = GetTreeBody();
  if (body)
    return body->ClearStyleAndImageCaches();
  return NS_OK;
}

void
nsTreeBoxObject::ClearCachedValues()
{
  mTreeBody = nsnull;
}    



nsresult
NS_NewTreeBoxObject(nsIBoxObject** aResult)
{
  *aResult = new nsTreeBoxObject;
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*aResult);
  return NS_OK;
}

