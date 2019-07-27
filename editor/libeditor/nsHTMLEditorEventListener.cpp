




#include "nsHTMLEditorEventListener.h"

#include "mozilla/dom/Selection.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsDebug.h"
#include "nsEditor.h"
#include "nsError.h"
#include "nsHTMLEditUtils.h"
#include "nsHTMLEditor.h"
#include "nsIDOMElement.h"
#include "nsIDOMEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMMouseEvent.h"
#include "nsIDOMNode.h"
#include "nsIEditor.h"
#include "nsIHTMLEditor.h"
#include "nsIHTMLInlineTableEditor.h"
#include "nsIHTMLObjectResizer.h"
#include "nsISupportsImpl.h"
#include "nsLiteralString.h"
#include "nsQueryObject.h"
#include "nsRange.h"

using namespace mozilla;
using namespace mozilla::dom;






#ifdef DEBUG
nsresult
nsHTMLEditorEventListener::Connect(nsEditor* aEditor)
{
  nsCOMPtr<nsIHTMLEditor> htmlEditor = do_QueryObject(aEditor);
  nsCOMPtr<nsIHTMLInlineTableEditor> htmlInlineTableEditor =
    do_QueryObject(aEditor);
  NS_PRECONDITION(htmlEditor && htmlInlineTableEditor,
                  "Set nsHTMLEditor or its sub class");
  return nsEditorEventListener::Connect(aEditor);
}
#endif

nsHTMLEditor*
nsHTMLEditorEventListener::GetHTMLEditor()
{
  
  return static_cast<nsHTMLEditor*>(mEditor);
}

nsresult
nsHTMLEditorEventListener::MouseUp(nsIDOMMouseEvent* aMouseEvent)
{
  nsHTMLEditor* htmlEditor = GetHTMLEditor();

  nsCOMPtr<nsIDOMEventTarget> target;
  nsresult rv = aMouseEvent->GetTarget(getter_AddRefs(target));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(target, NS_ERROR_NULL_POINTER);
  nsCOMPtr<nsIDOMElement> element = do_QueryInterface(target);

  int32_t clientX, clientY;
  aMouseEvent->GetClientX(&clientX);
  aMouseEvent->GetClientY(&clientY);
  htmlEditor->MouseUp(clientX, clientY, element);

  return nsEditorEventListener::MouseUp(aMouseEvent);
}

nsresult
nsHTMLEditorEventListener::MouseDown(nsIDOMMouseEvent* aMouseEvent)
{
  nsHTMLEditor* htmlEditor = GetHTMLEditor();
  
  
  if (!htmlEditor->IsAcceptableInputEvent(aMouseEvent)) {
    return NS_OK;
  }

  
  
  
  
  int16_t buttonNumber;
  nsresult rv = aMouseEvent->GetButton(&buttonNumber);
  NS_ENSURE_SUCCESS(rv, rv);

  bool isContextClick = buttonNumber == 2;

  int32_t clickCount;
  rv = aMouseEvent->GetDetail(&clickCount);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMEventTarget> target;
  rv = aMouseEvent->GetExplicitOriginalTarget(getter_AddRefs(target));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(target, NS_ERROR_NULL_POINTER);
  nsCOMPtr<nsIDOMElement> element = do_QueryInterface(target);

  if (isContextClick || (buttonNumber == 0 && clickCount == 2)) {
    nsRefPtr<Selection> selection = mEditor->GetSelection();
    NS_ENSURE_TRUE(selection, NS_OK);

    
    nsCOMPtr<nsIDOMNode> parent;
    rv = aMouseEvent->GetRangeParent(getter_AddRefs(parent));
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(parent, NS_ERROR_FAILURE);

    int32_t offset = 0;
    rv = aMouseEvent->GetRangeOffset(&offset);
    NS_ENSURE_SUCCESS(rv, rv);

    
    bool nodeIsInSelection = false;
    if (isContextClick && !selection->Collapsed()) {
      int32_t rangeCount;
      rv = selection->GetRangeCount(&rangeCount);
      NS_ENSURE_SUCCESS(rv, rv);

      for (int32_t i = 0; i < rangeCount; i++) {
        nsRefPtr<nsRange> range = selection->GetRangeAt(i);
        if (!range) {
          
          continue;
        }

        range->IsPointInRange(parent, offset, &nodeIsInSelection);

        
        if (nodeIsInSelection) {
          break;
        }
      }
    }
    nsCOMPtr<nsIDOMNode> node = do_QueryInterface(target);
    if (node && !nodeIsInSelection) {
      if (!element) {
        if (isContextClick) {
          
          selection->Collapse(parent, offset);
        } else {
          
          nsCOMPtr<nsIDOMElement> linkElement;
          rv = htmlEditor->GetElementOrParentByTagName(
                             NS_LITERAL_STRING("href"), node,
                             getter_AddRefs(linkElement));
          NS_ENSURE_SUCCESS(rv, rv);
          if (linkElement) {
            element = linkElement;
          }
        }
      }
      
      
      if (element) {
        nsCOMPtr<nsIDOMNode> selectAllNode =
          htmlEditor->FindUserSelectAllNode(element);

        if (selectAllNode) {
          nsCOMPtr<nsIDOMElement> newElement = do_QueryInterface(selectAllNode);
          if (newElement) {
            node = selectAllNode;
            element = newElement;
          }
        }

        if (isContextClick && !nsHTMLEditUtils::IsImage(node)) {
          selection->Collapse(parent, offset);
        } else {
          htmlEditor->SelectElement(element);
        }
      }
    }
    
    
    htmlEditor->CheckSelectionStateForAnonymousButtons(selection);

    
    
    if (element || isContextClick) {
      aMouseEvent->PreventDefault();
      return NS_OK;
    }
  } else if (!isContextClick && buttonNumber == 0 && clickCount == 1) {
    
    int32_t clientX, clientY;
    aMouseEvent->GetClientX(&clientX);
    aMouseEvent->GetClientY(&clientY);
    htmlEditor->MouseDown(clientX, clientY, element, aMouseEvent);
  }

  return nsEditorEventListener::MouseDown(aMouseEvent);
}

nsresult
nsHTMLEditorEventListener::MouseClick(nsIDOMMouseEvent* aMouseEvent)
{
  nsCOMPtr<nsIDOMEventTarget> target;
  nsresult rv = aMouseEvent->GetTarget(getter_AddRefs(target));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(target, NS_ERROR_NULL_POINTER);
  nsCOMPtr<nsIDOMElement> element = do_QueryInterface(target);

  GetHTMLEditor()->DoInlineTableEditingAction(element);

  return nsEditorEventListener::MouseClick(aMouseEvent);
}
