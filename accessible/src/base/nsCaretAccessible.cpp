





































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
nsLeafAccessible(aDocumentNode, aShell), mVisible(PR_TRUE), mCurrentDOMNode(nsnull), mRootAccessible(aRootAccessible)
{
}

NS_IMETHODIMP nsCaretAccessible::Shutdown()
{
  mDomSelectionWeak = nsnull;
  mCurrentDOMNode = nsnull;
  RemoveSelectionListener();
  return NS_OK;
}

NS_IMETHODIMP nsCaretAccessible::RemoveSelectionListener()
{
  nsCOMPtr<nsISelection> prevDomSel(do_QueryReferent(mDomSelectionWeak));
  nsCOMPtr<nsISelectionPrivate> selPrivate(do_QueryInterface(prevDomSel));
  if (selPrivate) {
    mDomSelectionWeak = nsnull;
    return selPrivate->RemoveSelectionListener(this);
  }
  return NS_OK;
}

NS_IMETHODIMP nsCaretAccessible::AttachNewSelectionListener(nsIDOMNode *aCurrentNode)
{
  mCurrentDOMNode = aCurrentNode;

  
  
  nsCOMPtr<nsIPresShell> presShell = 
    nsRootAccessible::GetPresShellFor(aCurrentNode);
  if (!presShell)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDocument> doc = presShell->GetDocument();
  if (!doc)  
    doc = do_QueryInterface(aCurrentNode);
  nsCOMPtr<nsIContent> content(do_QueryInterface(aCurrentNode));
  if (!content)
    content = doc->GetRootContent();  

  nsIFrame *frame = presShell->GetPrimaryFrameFor(content);
  nsPresContext *presContext = presShell->GetPresContext();
  if (!frame || !presContext)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsISelectionController> selCon;
  frame->GetSelectionController(presContext, getter_AddRefs(selCon));
  if (!selCon)
    return NS_ERROR_FAILURE;
  
  nsCOMPtr<nsISelection> domSel, prevDomSel(do_QueryReferent(mDomSelectionWeak));
  selCon->GetSelection(nsISelectionController::SELECTION_NORMAL, getter_AddRefs(domSel));
  if (domSel == prevDomSel)
    return NS_OK; 
  RemoveSelectionListener();
  nsCOMPtr<nsISelectionPrivate> selPrivate(do_QueryInterface(domSel));

  if (!selPrivate)
    return NS_ERROR_FAILURE;

  mDomSelectionWeak = do_GetWeakReference(domSel);
  return selPrivate->AddSelectionListener(this);
}

NS_IMETHODIMP nsCaretAccessible::NotifySelectionChanged(nsIDOMDocument *aDoc, nsISelection *aSel, PRInt16 aReason)
{
  nsCOMPtr<nsIPresShell> presShell = GetPresShellFor(mCurrentDOMNode);
  nsCOMPtr<nsISelection> domSel(do_QueryReferent(mDomSelectionWeak));
  if (!presShell || domSel != aSel)
    return NS_OK;  

  nsCOMPtr<nsICaret> caret;
  presShell->GetCaret(getter_AddRefs(caret));
  if (!caret)
    return NS_OK;

  nsRect caretRect;
  PRBool isCollapsed;
  caret->GetCaretCoordinates(nsICaret::eTopLevelWindowCoordinates, domSel,
                             &caretRect, &isCollapsed, nsnull);
  PRBool visible = !caretRect.IsEmpty();
  if (visible)  
    caret->GetCaretVisible(&visible);
  if (visible != mVisible) {
    mVisible = visible;
#ifdef XP_WIN
    mRootAccessible->FireToolkitEvent(mVisible? nsIAccessibleEvent::EVENT_SHOW: 
                                      nsIAccessibleEvent::EVENT_HIDE, this, nsnull);
#endif
  }

#ifdef XP_WIN
  
  
  nsPresContext *presContext = presShell->GetPresContext();
  nsIViewManager* viewManager = presShell->GetViewManager();
  if (!presContext || !viewManager)
    return NS_OK;
  nsIView *view = nsnull;
  viewManager->GetRootView(view);
  if (!view)
    return NS_OK;
  nsIWidget* widget = view->GetWidget();
  if (!widget)
    return NS_OK;

  caretRect.x      = presContext->AppUnitsToDevPixels(caretRect.x);
  caretRect.y      = presContext->AppUnitsToDevPixels(caretRect.y);
  caretRect.width  = presContext->AppUnitsToDevPixels(caretRect.width);
  caretRect.height = presContext->AppUnitsToDevPixels(caretRect.height);

  widget->WidgetToScreen(caretRect, mCaretRect);

  mRootAccessible->FireToolkitEvent(nsIAccessibleEvent::EVENT_LOCATION_CHANGE, this, nsnull);
#endif

  
  nsCOMPtr<nsIAccessible> accessible;
  nsIAccessibilityService *accService = GetAccService();
  NS_ENSURE_TRUE(accService, NS_ERROR_FAILURE);
  
  nsCOMPtr<nsIDOMNode> focusNode;
  domSel->GetFocusNode(getter_AddRefs(focusNode));
  if (!focusNode) {
    return NS_OK; 
  }
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

  return mRootAccessible->FireDelayedToolkitEvent(nsIAccessibleEvent::EVENT_ATK_TEXT_CARET_MOVE, focusNode, nsnull, PR_FALSE);
}


NS_IMETHODIMP nsCaretAccessible::GetBounds(PRInt32 *x, PRInt32 *y, PRInt32 *width, PRInt32 *height)
{
  if (mCaretRect.IsEmpty()) {
    return NS_ERROR_FAILURE;
  }
  *x = mCaretRect.x;
  *y = mCaretRect.y;
  *width = mCaretRect.width;
  *height = mCaretRect.height;
  return NS_OK;
}

NS_IMETHODIMP nsCaretAccessible::GetRole(PRUint32 *_retval)
{
  *_retval = nsIAccessibleRole::ROLE_CARET;
  return NS_OK;
}

NS_IMETHODIMP nsCaretAccessible::GetState(PRUint32 *_retval)
{
  *_retval = mVisible? 0: nsIAccessibleStates::STATE_INVISIBLE;
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

