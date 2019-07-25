




































#include "nsCaretAccessible.h"

#include "nsAccessibilityService.h"
#include "nsAccUtils.h"
#include "nsCoreUtils.h"
#include "nsIAccessibleEvent.h"

#include "nsCaret.h"
#include "nsIDOMDocument.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMHTMLTextAreaElement.h"
#include "nsIFrame.h"
#include "nsIPresShell.h"
#include "nsRootAccessible.h"
#include "nsISelectionPrivate.h"
#include "nsServiceManagerUtils.h"

class nsIWidget;

NS_IMPL_ISUPPORTS1(nsCaretAccessible, nsISelectionListener)
  
nsCaretAccessible::nsCaretAccessible( nsRootAccessible *aRootAccessible):
mLastCaretOffset(-1), mRootAccessible(aRootAccessible)
{
}

nsCaretAccessible::~nsCaretAccessible()
{
}

void nsCaretAccessible::Shutdown()
{
  
  
  

  ClearControlSelectionListener(); 
  mLastTextAccessible = nsnull;
  mLastUsedSelection = nsnull;
  mRootAccessible = nsnull;
}

nsresult nsCaretAccessible::ClearControlSelectionListener()
{
  nsCOMPtr<nsISelectionController> controller =
    GetSelectionControllerForNode(mCurrentControl);

  mCurrentControl = nsnull;

  if (!controller)
    return NS_OK;

  
  nsCOMPtr<nsISelection> normalSel;
  controller->GetSelection(nsISelectionController::SELECTION_NORMAL,
                           getter_AddRefs(normalSel));
  nsCOMPtr<nsISelectionPrivate> selPrivate(do_QueryInterface(normalSel));
  NS_ENSURE_TRUE(selPrivate, NS_ERROR_FAILURE);

  nsresult rv = selPrivate->RemoveSelectionListener(this);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nsCOMPtr<nsISelection> spellcheckSel;
  controller->GetSelection(nsISelectionController::SELECTION_SPELLCHECK,
                           getter_AddRefs(spellcheckSel));
  selPrivate = do_QueryInterface(spellcheckSel);
  NS_ENSURE_TRUE(selPrivate, NS_ERROR_FAILURE);

  return selPrivate->RemoveSelectionListener(this);
}

nsresult
nsCaretAccessible::SetControlSelectionListener(nsIContent *aCurrentNode)
{
  NS_ENSURE_TRUE(mRootAccessible, NS_ERROR_FAILURE);

  ClearControlSelectionListener();

  mCurrentControl = aCurrentNode;
  mLastTextAccessible = nsnull;

  
  
  

  nsCOMPtr<nsISelectionController> controller =
    GetSelectionControllerForNode(mCurrentControl);
#ifdef DEBUG
  NS_ASSERTION(controller || aCurrentNode->IsNodeOfType(nsINode::eDOCUMENT),
               "No selection controller for non document node!");
#endif
  if (!controller)
    return NS_OK;

  
  nsCOMPtr<nsISelection> normalSel;
  controller->GetSelection(nsISelectionController::SELECTION_NORMAL,
                           getter_AddRefs(normalSel));
  nsCOMPtr<nsISelectionPrivate> selPrivate(do_QueryInterface(normalSel));
  NS_ENSURE_TRUE(selPrivate, NS_ERROR_FAILURE);

  nsresult rv = selPrivate->AddSelectionListener(this);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsISelection> spellcheckSel;
  controller->GetSelection(nsISelectionController::SELECTION_SPELLCHECK,
                           getter_AddRefs(spellcheckSel));
  selPrivate = do_QueryInterface(spellcheckSel);
  NS_ENSURE_TRUE(selPrivate, NS_ERROR_FAILURE);
  
  return selPrivate->AddSelectionListener(this);
}

nsresult
nsCaretAccessible::AddDocSelectionListener(nsIPresShell *aShell)
{
  NS_ENSURE_TRUE(mRootAccessible, NS_ERROR_FAILURE);

  nsCOMPtr<nsISelectionController> selCon = do_QueryInterface(aShell);
  NS_ENSURE_TRUE(selCon, NS_ERROR_FAILURE);

  nsCOMPtr<nsISelection> domSel;
  selCon->GetSelection(nsISelectionController::SELECTION_NORMAL, getter_AddRefs(domSel));
  nsCOMPtr<nsISelectionPrivate> selPrivate = do_QueryInterface(domSel);
  NS_ENSURE_TRUE(selPrivate, NS_ERROR_FAILURE);

  nsresult rv = selPrivate->AddSelectionListener(this);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISelection> spellcheckSel;
  selCon->GetSelection(nsISelectionController::SELECTION_SPELLCHECK,
                       getter_AddRefs(spellcheckSel));
  selPrivate = do_QueryInterface(spellcheckSel);
  NS_ENSURE_TRUE(selPrivate, NS_ERROR_FAILURE);
  
  return selPrivate->AddSelectionListener(this);
}

nsresult
nsCaretAccessible::RemoveDocSelectionListener(nsIPresShell *aShell)
{
  nsCOMPtr<nsISelectionController> selCon = do_QueryInterface(aShell);
  NS_ENSURE_TRUE(selCon, NS_ERROR_FAILURE);

  nsCOMPtr<nsISelection> domSel;
  selCon->GetSelection(nsISelectionController::SELECTION_NORMAL, getter_AddRefs(domSel));
  nsCOMPtr<nsISelectionPrivate> selPrivate = do_QueryInterface(domSel);
  NS_ENSURE_TRUE(selPrivate, NS_ERROR_FAILURE);

  selPrivate->RemoveSelectionListener(this);

  nsCOMPtr<nsISelection> spellcheckSel;
  selCon->GetSelection(nsISelectionController::SELECTION_SPELLCHECK,
                       getter_AddRefs(spellcheckSel));
  selPrivate = do_QueryInterface(spellcheckSel);
  NS_ENSURE_TRUE(selPrivate, NS_ERROR_FAILURE);

  return selPrivate->RemoveSelectionListener(this);
}

NS_IMETHODIMP
nsCaretAccessible::NotifySelectionChanged(nsIDOMDocument* aDOMDocument,
                                          nsISelection* aSelection,
                                          PRInt16 aReason)
{
  NS_ENSURE_ARG(aDOMDocument);
  NS_ENSURE_STATE(mRootAccessible);

  nsCOMPtr<nsIDocument> documentNode(do_QueryInterface(aDOMDocument));
  nsDocAccessible* document = GetAccService()->GetDocAccessible(documentNode);

#ifdef DEBUG_NOTIFICATIONS
  nsCOMPtr<nsISelection2> sel2(do_QueryInterface(aSelection));

  PRInt16 type = 0;
  sel2->GetType(&type);

  if (type == nsISelectionController::SELECTION_NORMAL ||
      type == nsISelectionController::SELECTION_SPELLCHECK) {

    bool isNormalSelection =
      (type == nsISelectionController::SELECTION_NORMAL);

    bool isIgnored = !document || !document->IsContentLoaded();
    printf("\nSelection changed, selection type: %s, notification %s\n",
           (isNormalSelection ? "normal" : "spellcheck"),
           (isIgnored ? "ignored" : "pending"));
  }
#endif

  
  if (document && document->IsContentLoaded()) {
    
    
    
    
    document->HandleNotification<nsCaretAccessible, nsISelection>
      (this, &nsCaretAccessible::ProcessSelectionChanged, aSelection);
  }

  return NS_OK;
}

void
nsCaretAccessible::ProcessSelectionChanged(nsISelection* aSelection)
{
  nsCOMPtr<nsISelection2> sel2(do_QueryInterface(aSelection));

  PRInt16 type = 0;
  sel2->GetType(&type);

  if (type == nsISelectionController::SELECTION_NORMAL)
    NormalSelectionChanged(aSelection);

  else if (type == nsISelectionController::SELECTION_SPELLCHECK)
    SpellcheckSelectionChanged(aSelection);
}

void
nsCaretAccessible::NormalSelectionChanged(nsISelection* aSelection)
{
  mLastUsedSelection = do_GetWeakReference(aSelection);

  PRInt32 rangeCount = 0;
  aSelection->GetRangeCount(&rangeCount);
  if (rangeCount == 0) {
    mLastTextAccessible = nsnull;
    return; 
  }

  nsHyperTextAccessible* textAcc =
    nsAccUtils::GetTextAccessibleFromSelection(aSelection);
  if (!textAcc)
    return;

  PRInt32 caretOffset = -1;
  nsresult rv = textAcc->GetCaretOffset(&caretOffset);
  if (NS_FAILED(rv))
    return;

  if (textAcc == mLastTextAccessible && caretOffset == mLastCaretOffset) {
    PRInt32 selectionCount = 0;
    textAcc->GetSelectionCount(&selectionCount);   
    if (!selectionCount)
      return;  
  }

  mLastCaretOffset = caretOffset;
  mLastTextAccessible = textAcc;

  nsRefPtr<AccEvent> event =
    new AccCaretMoveEvent(mLastTextAccessible->GetNode());
  if (event)
    mLastTextAccessible->GetDocAccessible()->FireDelayedAccessibleEvent(event);
}

void
nsCaretAccessible::SpellcheckSelectionChanged(nsISelection* aSelection)
{
  
  
  
  
  

  nsHyperTextAccessible* textAcc =
    nsAccUtils::GetTextAccessibleFromSelection(aSelection);
  if (!textAcc)
    return;

  nsRefPtr<AccEvent> event =
    new AccEvent(nsIAccessibleEvent::EVENT_TEXT_ATTRIBUTE_CHANGED, textAcc);
  if (event)
    textAcc->GetDocAccessible()->FireDelayedAccessibleEvent(event);
}

nsIntRect
nsCaretAccessible::GetCaretRect(nsIWidget **aOutWidget)
{
  nsIntRect caretRect;
  NS_ENSURE_TRUE(aOutWidget, caretRect);
  *aOutWidget = nsnull;
  NS_ENSURE_TRUE(mRootAccessible, caretRect);

  if (!mLastTextAccessible) {
    return caretRect;    
  }

  nsINode *lastNodeWithCaret = mLastTextAccessible->GetNode();
  NS_ENSURE_TRUE(lastNodeWithCaret, caretRect);

  nsIPresShell *presShell = nsCoreUtils::GetPresShellFor(lastNodeWithCaret);
  NS_ENSURE_TRUE(presShell, caretRect);

  nsRefPtr<nsCaret> caret = presShell->GetCaret();
  NS_ENSURE_TRUE(caret, caretRect);

  nsCOMPtr<nsISelection> caretSelection(do_QueryReferent(mLastUsedSelection));
  NS_ENSURE_TRUE(caretSelection, caretRect);
  
  PRBool isVisible;
  caret->GetCaretVisible(&isVisible);
  if (!isVisible) {
    return nsIntRect();  
  }

  nsRect rect;
  nsIFrame* frame = caret->GetGeometry(caretSelection, &rect);
  if (!frame || rect.IsEmpty()) {
    return nsIntRect(); 
  }

  nsPoint offset;
  
  
  *aOutWidget = frame->GetNearestWidget(offset);
  NS_ENSURE_TRUE(*aOutWidget, nsIntRect());
  rect.MoveBy(offset);

  caretRect = rect.ToOutsidePixels(frame->PresContext()->AppUnitsPerDevPixel());
  
  caretRect.MoveBy((*aOutWidget)->WidgetToScreenOffset() - (*aOutWidget)->GetClientOffset());

  
  
  
  PRInt32 charX, charY, charWidth, charHeight;
  if (NS_SUCCEEDED(mLastTextAccessible->GetCharacterExtents(mLastCaretOffset, &charX, &charY,
                                                            &charWidth, &charHeight,
                                                            nsIAccessibleCoordinateType::COORDTYPE_SCREEN_RELATIVE))) {
    caretRect.height -= charY - caretRect.y;
    caretRect.y = charY;
  }

  return caretRect;
}

already_AddRefed<nsISelectionController>
nsCaretAccessible::GetSelectionControllerForNode(nsIContent *aContent)
{
  if (!aContent)
    return nsnull;

  nsIDocument *document = aContent->GetOwnerDoc();
  if (!document)
    return nsnull;

  nsIPresShell *presShell = document->GetShell();
  if (!presShell)
    return nsnull;

  nsIFrame *frame = aContent->GetPrimaryFrame();
  if (!frame)
    return nsnull;

  nsPresContext *presContext = presShell->GetPresContext();
  if (!presContext)
    return nsnull;

  nsISelectionController *controller = nsnull;
  frame->GetSelectionController(presContext, &controller);
  return controller;
}

