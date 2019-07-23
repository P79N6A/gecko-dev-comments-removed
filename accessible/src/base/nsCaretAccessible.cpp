





































#include "nsAccessibilityService.h"
#include "nsCaretAccessible.h"
#include "nsIAccessibleEvent.h"
#include "nsICaret.h"
#include "nsIDOMDocument.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMHTMLTextAreaElement.h"
#include "nsIFrame.h"
#include "nsIPresShell.h"
#include "nsRootAccessible.h"
#include "nsISelectionController.h"
#include "nsISelectionPrivate.h"
#include "nsServiceManagerUtils.h"
#include "nsIViewManager.h"
#include "nsIWidget.h"

NS_IMPL_ISUPPORTS_INHERITED2(nsCaretAccessible, nsLeafAccessible, nsIAccessibleCaret, nsISelectionListener)

nsCaretAccessible::nsCaretAccessible(nsIDOMNode* aDocumentNode, nsIWeakReference* aShell, nsRootAccessible *aRootAccessible):
nsLeafAccessible(aDocumentNode, aShell), mLastCaretOffset(-1), mLastNodeWithCaret(nsnull),
mCurrentControl(nsnull), mRootAccessible(aRootAccessible)
{
}

NS_IMETHODIMP nsCaretAccessible::Shutdown()
{
  
  
  
  ClearControlSelectionListener(); 
  mLastNodeWithCaret = nsnull;
  mLastUsedSelection = nsnull;
  
  return nsLeafAccessible::Shutdown();
}

NS_IMETHODIMP nsCaretAccessible::ClearControlSelectionListener()
{
  nsCOMPtr<nsISelectionPrivate> selPrivate(do_QueryReferent(mCurrentControlSelection));
  if (selPrivate) {
    mCurrentControlSelection = nsnull;
    mCurrentControl = nsnull;
    return selPrivate->RemoveSelectionListener(this);
  }
  return NS_OK;
}

NS_IMETHODIMP nsCaretAccessible::SetControlSelectionListener(nsIDOMNode *aCurrentNode)
{
  mCurrentControl = aCurrentNode;
  mLastNodeWithCaret = nsnull;

  
  
  nsCOMPtr<nsIPresShell> presShell = 
    nsRootAccessible::GetPresShellFor(aCurrentNode);
  if (!presShell)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDocument> doc = presShell->GetDocument();
  NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);

  nsCOMPtr<nsIContent> content(do_QueryInterface(aCurrentNode));
  
  
  if (!content) {
    return NS_OK;
  }

  nsIFrame *frame = presShell->GetPrimaryFrameFor(content);
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

  nsPresContext *presContext = presShell->GetPresContext();
  NS_ENSURE_TRUE(presContext, NS_ERROR_FAILURE);

  nsCOMPtr<nsISelectionController> selCon;
  frame->GetSelectionController(presContext, getter_AddRefs(selCon));
  NS_ENSURE_TRUE(selCon, NS_ERROR_FAILURE);
  
  nsCOMPtr<nsISelection> domSel;
  selCon->GetSelection(nsISelectionController::SELECTION_NORMAL, getter_AddRefs(domSel));

  ClearControlSelectionListener();
  nsCOMPtr<nsISelectionPrivate> selPrivate(do_QueryInterface(domSel));
  NS_ENSURE_TRUE(selPrivate, NS_ERROR_FAILURE);

  mCurrentControlSelection = do_GetWeakReference(domSel);
  return selPrivate->AddSelectionListener(this);
}

NS_IMETHODIMP nsCaretAccessible::AddDocSelectionListener(nsIDOMDocument *aDoc)
{
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(aDoc);
  NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);
  nsCOMPtr<nsISelectionController> selCon = do_QueryInterface(doc->GetPrimaryShell());
  NS_ENSURE_TRUE(selCon, NS_ERROR_FAILURE);

  nsCOMPtr<nsISelection> domSel;
  selCon->GetSelection(nsISelectionController::SELECTION_NORMAL, getter_AddRefs(domSel));
  nsCOMPtr<nsISelectionPrivate> selPrivate = do_QueryInterface(domSel);
  NS_ENSURE_TRUE(selPrivate, NS_ERROR_FAILURE);

  return selPrivate->AddSelectionListener(this);
}

NS_IMETHODIMP nsCaretAccessible::RemoveDocSelectionListener(nsIDOMDocument *aDoc)
{
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(aDoc);
  NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);

  nsCOMPtr<nsISelectionController> selCon = do_QueryInterface(doc->GetPrimaryShell());
  NS_ENSURE_TRUE(selCon, NS_ERROR_FAILURE);

  nsCOMPtr<nsISelection> domSel;
  selCon->GetSelection(nsISelectionController::SELECTION_NORMAL, getter_AddRefs(domSel));
  nsCOMPtr<nsISelectionPrivate> selPrivate = do_QueryInterface(domSel);
  NS_ENSURE_TRUE(selPrivate, NS_ERROR_FAILURE);

  return selPrivate->RemoveSelectionListener(this);
}

NS_IMETHODIMP nsCaretAccessible::NotifySelectionChanged(nsIDOMDocument *aDoc, nsISelection *aSel, PRInt16 aReason)
{
  mLastUsedSelection = do_GetWeakReference(aSel);

  nsCOMPtr<nsIDocument> doc = do_QueryInterface(aDoc);
  nsIPresShell *presShell = doc->GetPrimaryShell();
  NS_ENSURE_TRUE(presShell, NS_OK);

  
  nsCOMPtr<nsIAccessible> accessible;
  nsIAccessibilityService *accService = GetAccService();
  NS_ENSURE_TRUE(accService, NS_ERROR_FAILURE);
  
  nsCOMPtr<nsIDOMNode> focusNode;
  aSel->GetFocusNode(getter_AddRefs(focusNode));
  if (!focusNode) {
    mLastNodeWithCaret = nsnull;
    return NS_OK; 
  }
  nsCOMPtr<nsIDOMNode> nodeWithCaret = focusNode;

  nsCOMPtr<nsIAccessibleText> textAcc;
  while (focusNode) {
    
    nsCOMPtr<nsIDOMNode> relevantNode;
    if (NS_SUCCEEDED(accService->GetRelevantContentNodeFor(focusNode, getter_AddRefs(relevantNode))) && relevantNode) {
      focusNode  = relevantNode;
    }

    nsCOMPtr<nsIContent> content = do_QueryInterface(focusNode);
    if (!content || !content->IsNodeOfType(nsINode::eTEXT)) {
      accService->GetAccessibleInShell(focusNode, presShell,  getter_AddRefs(accessible));
      textAcc = do_QueryInterface(accessible);
      if (textAcc) {
        break;
      }
    }
    nsCOMPtr<nsIDOMNode> parentNode;
    focusNode->GetParentNode(getter_AddRefs(parentNode));
    focusNode.swap(parentNode);
  }
  NS_ASSERTION(textAcc, "No nsIAccessibleText for caret move event!"); 
  NS_ENSURE_TRUE(textAcc, NS_ERROR_FAILURE);

  PRInt32 caretOffset;
  textAcc->GetCaretOffset(&caretOffset);

  if (nodeWithCaret == mLastNodeWithCaret && caretOffset == mLastCaretOffset) {
    PRInt32 selectionCount;
    textAcc->GetSelectionCount(&selectionCount);   
    if (!selectionCount) {
      return NS_OK;  
    }
  }
  mLastCaretOffset = caretOffset;
  mLastNodeWithCaret = nodeWithCaret;

  return mRootAccessible->FireDelayedToolkitEvent(nsIAccessibleEvent::EVENT_TEXT_CARET_MOVED,
                                                  focusNode, nsnull, PR_FALSE);
}

already_AddRefed<nsICaret>
nsCaretAccessible::GetLastCaret(nsRect *aRect, PRBool *aIsVisible)
{
  *aIsVisible = PR_FALSE;
  if (!mLastNodeWithCaret) {
    return nsnull;
  }

  nsCOMPtr<nsIPresShell> presShell = GetPresShellFor(mLastNodeWithCaret);
  NS_ENSURE_TRUE(presShell, nsnull);

  nsICaret *caret;
  presShell->GetCaret(&caret);
  NS_ENSURE_TRUE(caret, nsnull);

  nsRect caretRect;
  PRBool isCollapsed;
  nsCOMPtr<nsISelection> caretSelection(do_QueryReferent(mLastUsedSelection));
  caret->GetCaretCoordinates(nsICaret::eTopLevelWindowCoordinates, caretSelection,
                             aRect, &isCollapsed, nsnull);
  if (!aRect->IsEmpty()) {
    caret->GetCaretVisible(aIsVisible);
  }
  return caret;
}


NS_IMETHODIMP nsCaretAccessible::GetBounds(PRInt32 *aX, PRInt32 *aY, PRInt32 *aWidth, PRInt32 *aHeight)
{
  *aX = *aY = *aWidth = *aHeight = 0;
  nsRect caretRect;
  PRBool isVisible;
  nsCOMPtr<nsICaret> caret = GetLastCaret(&caretRect, &isVisible);
  if (!caret || caretRect.IsEmpty()) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIPresShell> presShell = GetPresShellFor(mLastNodeWithCaret);
  NS_ENSURE_TRUE(presShell, NS_ERROR_FAILURE);

  nsPresContext *presContext = presShell->GetPresContext();
  NS_ENSURE_TRUE(presContext, NS_ERROR_FAILURE);

  nsIViewManager* viewManager = presShell->GetViewManager();
  NS_ENSURE_TRUE(viewManager, NS_ERROR_FAILURE);

  nsIView *view = nsnull;
  viewManager->GetRootView(view);
  NS_ENSURE_TRUE(view, NS_ERROR_FAILURE);

  nsIWidget* widget = view->GetWidget();
  NS_ENSURE_TRUE(widget, NS_ERROR_FAILURE);

  caretRect.x      = presContext->AppUnitsToDevPixels(caretRect.x);
  caretRect.y      = presContext->AppUnitsToDevPixels(caretRect.y);
  caretRect.width  = presContext->AppUnitsToDevPixels(caretRect.width);
  caretRect.height = presContext->AppUnitsToDevPixels(caretRect.height);

  nsRect caretScreenRect;
  widget->WidgetToScreen(caretRect, caretScreenRect);

  *aX = caretScreenRect.x;
  *aY = caretScreenRect.y;
  *aWidth = caretScreenRect.width;
  *aHeight = caretScreenRect.height;

  return NS_OK;
}

NS_IMETHODIMP nsCaretAccessible::GetRole(PRUint32 *_retval)
{
  *_retval = nsIAccessibleRole::ROLE_CARET;
  return NS_OK;
}

NS_IMETHODIMP
nsCaretAccessible::GetState(PRUint32 *aState, PRUint32 *aExtraState)
{
  *aState = 0;
  if (aExtraState)
    *aExtraState = 0;

  NS_ENSURE_TRUE(mLastNodeWithCaret, NS_ERROR_FAILURE);

  nsCOMPtr<nsIPresShell> presShell = GetPresShellFor(mLastNodeWithCaret);
  NS_ENSURE_TRUE(presShell, NS_ERROR_FAILURE);

  nsRect caretRect;
  PRBool isVisible;
  GetLastCaret(&caretRect, &isVisible);
  if (!isVisible) {
    *aState = nsIAccessibleStates::STATE_INVISIBLE;
  }
  return NS_OK;
}

NS_IMETHODIMP nsCaretAccessible::GetParent(nsIAccessible **aParent)
{   
  NS_ADDREF(*aParent = mRootAccessible);
  return NS_OK;
}
NS_IMETHODIMP nsCaretAccessible::GetPreviousSibling(nsIAccessible **_retval)
{ 
  *_retval = nsnull;
  return NS_OK;
}

NS_IMETHODIMP nsCaretAccessible::GetNextSibling(nsIAccessible **_retval)
{
  *_retval = nsnull;
  return NS_OK;
}

