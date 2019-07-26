




#include "nsCaretAccessible.h"

#include "DocAccessible-inl.h"
#include "nsAccessibilityService.h"
#include "nsAccUtils.h"
#include "nsCoreUtils.h"
#include "nsIAccessibleEvent.h"
#include "RootAccessible.h"

#include "nsCaret.h"
#include "nsIDOMDocument.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsIDOMHTMLTextAreaElement.h"
#include "nsIFrame.h"
#include "nsIPresShell.h"
#include "nsISelectionPrivate.h"
#include "nsServiceManagerUtils.h"

class nsIWidget;

using namespace mozilla::a11y;

NS_IMPL_ISUPPORTS1(nsCaretAccessible, nsISelectionListener)
  
nsCaretAccessible::nsCaretAccessible(RootAccessible* aRootAccessible) :
mLastCaretOffset(-1), mRootAccessible(aRootAccessible)
{
}

nsCaretAccessible::~nsCaretAccessible()
{
}

void nsCaretAccessible::Shutdown()
{
  
  
  

  ClearControlSelectionListener(); 
  mLastTextAccessible = nullptr;
  mLastUsedSelection = nullptr;
  mRootAccessible = nullptr;
}

nsresult nsCaretAccessible::ClearControlSelectionListener()
{
  nsCOMPtr<nsISelectionController> controller =
    GetSelectionControllerForNode(mCurrentControl);

  mCurrentControl = nullptr;

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
  mLastTextAccessible = nullptr;

  
  
  

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
                                          int16_t aReason)
{
  NS_ENSURE_ARG(aDOMDocument);
  NS_ENSURE_STATE(mRootAccessible);

  nsCOMPtr<nsIDocument> documentNode(do_QueryInterface(aDOMDocument));
  DocAccessible* document = GetAccService()->GetDocAccessible(documentNode);

#ifdef DEBUG
  if (logging::IsEnabled(logging::eSelection))
    logging::SelChange(aSelection, document);
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
  nsCOMPtr<nsISelectionPrivate> privSel(do_QueryInterface(aSelection));

  int16_t type = 0;
  privSel->GetType(&type);

  if (type == nsISelectionController::SELECTION_NORMAL)
    NormalSelectionChanged(aSelection);

  else if (type == nsISelectionController::SELECTION_SPELLCHECK)
    SpellcheckSelectionChanged(aSelection);
}

void
nsCaretAccessible::NormalSelectionChanged(nsISelection* aSelection)
{
  mLastUsedSelection = do_GetWeakReference(aSelection);

  int32_t rangeCount = 0;
  aSelection->GetRangeCount(&rangeCount);
  if (rangeCount == 0) {
    mLastTextAccessible = nullptr;
    return; 
  }

  HyperTextAccessible* textAcc =
    nsAccUtils::GetTextAccessibleFromSelection(aSelection);
  if (!textAcc)
    return;

  int32_t caretOffset = -1;
  nsresult rv = textAcc->GetCaretOffset(&caretOffset);
  if (NS_FAILED(rv))
    return;

  if (textAcc == mLastTextAccessible && caretOffset == mLastCaretOffset) {
    int32_t selectionCount = 0;
    textAcc->GetSelectionCount(&selectionCount);   
    if (!selectionCount)
      return;  
  }

  mLastCaretOffset = caretOffset;
  mLastTextAccessible = textAcc;

  nsRefPtr<AccEvent> event =
    new AccCaretMoveEvent(mLastTextAccessible->GetNode());
  if (event)
    mLastTextAccessible->Document()->FireDelayedAccessibleEvent(event);
}

void
nsCaretAccessible::SpellcheckSelectionChanged(nsISelection* aSelection)
{
  
  
  
  
  

  HyperTextAccessible* textAcc =
    nsAccUtils::GetTextAccessibleFromSelection(aSelection);
  if (!textAcc)
    return;

  nsRefPtr<AccEvent> event =
    new AccEvent(nsIAccessibleEvent::EVENT_TEXT_ATTRIBUTE_CHANGED, textAcc);
  if (event)
    textAcc->Document()->FireDelayedAccessibleEvent(event);
}

nsIntRect
nsCaretAccessible::GetCaretRect(nsIWidget **aOutWidget)
{
  nsIntRect caretRect;
  NS_ENSURE_TRUE(aOutWidget, caretRect);
  *aOutWidget = nullptr;
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
  
  bool isVisible;
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

  
  
  
  int32_t charX, charY, charWidth, charHeight;
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
    return nullptr;

  nsIPresShell *presShell = aContent->OwnerDoc()->GetShell();
  if (!presShell)
    return nullptr;

  nsIFrame *frame = aContent->GetPrimaryFrame();
  if (!frame)
    return nullptr;

  nsPresContext *presContext = presShell->GetPresContext();
  if (!presContext)
    return nullptr;

  nsISelectionController *controller = nullptr;
  frame->GetSelectionController(presContext, &controller);
  return controller;
}

