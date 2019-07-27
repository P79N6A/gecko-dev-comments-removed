



#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsDebug.h"
#include "nsEditor.h"
#include "nsError.h"
#include "nsHTMLEditUtils.h"
#include "nsHTMLEditor.h"
#include "nsHTMLEditorEventListener.h"
#include "nsIDOMElement.h"
#include "nsIDOMEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMMouseEvent.h"
#include "nsIDOMNode.h"
#include "nsIDOMRange.h"
#include "nsIEditor.h"
#include "nsIHTMLEditor.h"
#include "nsIHTMLInlineTableEditor.h"
#include "nsIHTMLObjectResizer.h"
#include "nsISelection.h"
#include "nsISupportsImpl.h"
#include "nsLiteralString.h"






#ifdef DEBUG
nsresult
nsHTMLEditorEventListener::Connect(nsEditor* aEditor)
{
  nsCOMPtr<nsIHTMLEditor> htmlEditor = do_QueryObject(aEditor);
  nsCOMPtr<nsIHTMLInlineTableEditor> htmlInlineTableEditor = do_QueryObject(aEditor);
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
  NS_ENSURE_TRUE(aMouseEvent, NS_OK);

  nsHTMLEditor* htmlEditor = GetHTMLEditor();

  nsCOMPtr<nsIDOMEventTarget> target;
  nsresult res = aMouseEvent->GetTarget(getter_AddRefs(target));
  NS_ENSURE_SUCCESS(res, res);
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
  NS_ENSURE_TRUE(aMouseEvent, NS_OK);

  nsHTMLEditor* htmlEditor = GetHTMLEditor();

  
  
  
  int16_t buttonNumber;
  nsresult res = aMouseEvent->GetButton(&buttonNumber);
  NS_ENSURE_SUCCESS(res, res);

  bool isContextClick = buttonNumber == 2;

  int32_t clickCount;
  res = aMouseEvent->GetDetail(&clickCount);
  NS_ENSURE_SUCCESS(res, res);

  nsCOMPtr<nsIDOMEventTarget> target;
  res = aMouseEvent->GetExplicitOriginalTarget(getter_AddRefs(target));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(target, NS_ERROR_NULL_POINTER);
  nsCOMPtr<nsIDOMElement> element = do_QueryInterface(target);

  
  if (element && !htmlEditor->IsDescendantOfEditorRoot(element)) {
    return NS_OK;
  }

  if (isContextClick || (buttonNumber == 0 && clickCount == 2))
  {
    nsCOMPtr<nsISelection> selection;
    mEditor->GetSelection(getter_AddRefs(selection));
    NS_ENSURE_TRUE(selection, NS_OK);

    
    nsCOMPtr<nsIDOMNode> parent;
    res = aMouseEvent->GetRangeParent(getter_AddRefs(parent));
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(parent, NS_ERROR_FAILURE);

    int32_t offset = 0;
    res = aMouseEvent->GetRangeOffset(&offset);
    NS_ENSURE_SUCCESS(res, res);

    
    bool nodeIsInSelection = false;
    if (isContextClick && !selection->Collapsed()) {
      int32_t rangeCount;
      res = selection->GetRangeCount(&rangeCount);
      NS_ENSURE_SUCCESS(res, res);

      for (int32_t i = 0; i < rangeCount; i++) {
        nsCOMPtr<nsIDOMRange> range;

        res = selection->GetRangeAt(i, getter_AddRefs(range));
        if (NS_FAILED(res) || !range)
          continue;

        res = range->IsPointInRange(parent, offset, &nodeIsInSelection);

        
        if (nodeIsInSelection)
          break;
      }
    }
    nsCOMPtr<nsIDOMNode> node = do_QueryInterface(target);
    if (node && !nodeIsInSelection)
    {
      if (!element)
      {
        if (isContextClick)
        {
          
          selection->Collapse(parent, offset);
        }
        else
        {
          
          nsCOMPtr<nsIDOMElement> linkElement;
          res = htmlEditor->GetElementOrParentByTagName(NS_LITERAL_STRING("href"), node, getter_AddRefs(linkElement));
          NS_ENSURE_SUCCESS(res, res);
          if (linkElement)
            element = linkElement;
        }
      }
      
      
      if (element)
      {
        nsCOMPtr<nsIDOMNode> selectAllNode =
          htmlEditor->FindUserSelectAllNode(element);

        if (selectAllNode)
        {
          nsCOMPtr<nsIDOMElement> newElement = do_QueryInterface(selectAllNode);
          if (newElement)
          {
            node = selectAllNode;
            element = newElement;
          }
        }

        if (isContextClick && !nsHTMLEditUtils::IsImage(node))
        {
          selection->Collapse(parent, offset);
        }
        else
        {
          htmlEditor->SelectElement(element);
        }
      }
    }
    
    
    htmlEditor->CheckSelectionStateForAnonymousButtons(selection);

    
    
    if (element || isContextClick)
    {
      aMouseEvent->PreventDefault();
      return NS_OK;
    }
  }
  else if (!isContextClick && buttonNumber == 0 && clickCount == 1)
  {
    
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
  NS_ENSURE_TRUE(aMouseEvent, NS_OK);

  nsCOMPtr<nsIDOMEventTarget> target;
  nsresult res = aMouseEvent->GetTarget(getter_AddRefs(target));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(target, NS_ERROR_NULL_POINTER);
  nsCOMPtr<nsIDOMElement> element = do_QueryInterface(target);

  GetHTMLEditor()->DoInlineTableEditingAction(element);

  return nsEditorEventListener::MouseClick(aMouseEvent);
}
