






































#include "nsHTMLEditorEventListener.h"
#include "nsHTMLEditor.h"
#include "nsString.h"

#include "nsIDOMEvent.h"
#include "nsIDOMNSEvent.h"
#include "nsIDOMElement.h"
#include "nsIDOMMouseEvent.h"
#include "nsISelection.h"
#include "nsIDOMRange.h"
#include "nsIDOMNSRange.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMNSUIEvent.h"
#include "nsIDOMHTMLTableElement.h"
#include "nsIDOMHTMLTableCellElement.h"
#include "nsIContent.h"

#include "nsIHTMLObjectResizer.h"
#include "nsEditProperty.h"
#include "nsTextEditUtils.h"
#include "nsHTMLEditUtils.h"








#ifdef DEBUG
nsresult
nsHTMLEditorEventListener::Connect(nsEditor* aEditor)
{
  nsCOMPtr<nsIHTMLEditor> htmlEditor =
    do_QueryInterface(static_cast<nsIEditor*>(aEditor));
  nsCOMPtr<nsIHTMLInlineTableEditor> htmlInlineTableEditor =
    do_QueryInterface(static_cast<nsIEditor*>(aEditor));
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

NS_IMETHODIMP
nsHTMLEditorEventListener::MouseUp(nsIDOMEvent* aMouseEvent)
{
  NS_ENSURE_TRUE(mEditor, NS_ERROR_NOT_AVAILABLE);

  nsCOMPtr<nsIDOMMouseEvent> mouseEvent ( do_QueryInterface(aMouseEvent) );
  if (!mouseEvent) {
    
    return NS_OK;
  }

  nsHTMLEditor* htmlEditor = GetHTMLEditor();

  nsCOMPtr<nsIDOMEventTarget> target;
  nsresult res = aMouseEvent->GetTarget(getter_AddRefs(target));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(target, NS_ERROR_NULL_POINTER);
  nsCOMPtr<nsIDOMElement> element = do_QueryInterface(target);

  PRInt32 clientX, clientY;
  mouseEvent->GetClientX(&clientX);
  mouseEvent->GetClientY(&clientY);
  htmlEditor->MouseUp(clientX, clientY, element);

  return nsEditorEventListener::MouseUp(aMouseEvent);
}

NS_IMETHODIMP
nsHTMLEditorEventListener::MouseDown(nsIDOMEvent* aMouseEvent)
{
  NS_ENSURE_TRUE(mEditor, NS_ERROR_NOT_AVAILABLE);

  nsCOMPtr<nsIDOMMouseEvent> mouseEvent ( do_QueryInterface(aMouseEvent) );
  if (!mouseEvent) {
    
    return NS_OK;
  }

  nsHTMLEditor* htmlEditor = GetHTMLEditor();

  
  
  
  PRUint16 buttonNumber;
  nsresult res = mouseEvent->GetButton(&buttonNumber);
  NS_ENSURE_SUCCESS(res, res);

  PRBool isContextClick;

#if defined(XP_MACOSX)
  
  res = mouseEvent->GetCtrlKey(&isContextClick);
  NS_ENSURE_SUCCESS(res, res);
#else
  
  isContextClick = buttonNumber == 2;
#endif
  
  PRInt32 clickCount;
  res = mouseEvent->GetDetail(&clickCount);
  NS_ENSURE_SUCCESS(res, res);

  nsCOMPtr<nsIDOMEventTarget> target;
  nsCOMPtr<nsIDOMNSEvent> internalEvent = do_QueryInterface(aMouseEvent);
  res = internalEvent->GetExplicitOriginalTarget(getter_AddRefs(target));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(target, NS_ERROR_NULL_POINTER);
  nsCOMPtr<nsIDOMElement> element = do_QueryInterface(target);

  if (isContextClick || (buttonNumber == 0 && clickCount == 2))
  {
    nsCOMPtr<nsISelection> selection;
    mEditor->GetSelection(getter_AddRefs(selection));
    NS_ENSURE_TRUE(selection, NS_OK);

    
    nsCOMPtr<nsIDOMNSUIEvent> uiEvent = do_QueryInterface(aMouseEvent);
    NS_ENSURE_TRUE(uiEvent, NS_ERROR_FAILURE);

    nsCOMPtr<nsIDOMNode> parent;
    PRInt32 offset = 0;

    res = uiEvent->GetRangeParent(getter_AddRefs(parent));
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(parent, NS_ERROR_FAILURE);

    res = uiEvent->GetRangeOffset(&offset);
    NS_ENSURE_SUCCESS(res, res);

    
    PRBool nodeIsInSelection = PR_FALSE;
    if (isContextClick)
    {
      PRBool isCollapsed;
      selection->GetIsCollapsed(&isCollapsed);
      if (!isCollapsed)
      {
        PRInt32 rangeCount;
        res = selection->GetRangeCount(&rangeCount);
        NS_ENSURE_SUCCESS(res, res);

        for (PRInt32 i = 0; i < rangeCount; i++)
        {
          nsCOMPtr<nsIDOMRange> range;

          res = selection->GetRangeAt(i, getter_AddRefs(range));
          if (NS_FAILED(res) || !range) 
            continue;

          nsCOMPtr<nsIDOMNSRange> nsrange(do_QueryInterface(range));
          if (NS_FAILED(res) || !nsrange) 
            continue;

          res = nsrange->IsPointInRange(parent, offset, &nodeIsInSelection);

          
          if (nodeIsInSelection)
            break;
        }
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



        if (nsTextEditUtils::IsBody(node) ||
            nsHTMLEditUtils::IsTableCellOrCaption(node) ||
            nsHTMLEditUtils::IsTableRow(node) ||
            nsHTMLEditUtils::IsTable(node))
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
    #ifndef XP_OS2
      mouseEvent->PreventDefault();
    #endif
      return NS_OK;
    }
  }
  else if (!isContextClick && buttonNumber == 0 && clickCount == 1)
  {
    
    PRInt32 clientX, clientY;
    mouseEvent->GetClientX(&clientX);
    mouseEvent->GetClientY(&clientY);
    htmlEditor->MouseDown(clientX, clientY, element, aMouseEvent);
  }

  return nsEditorEventListener::MouseDown(aMouseEvent);
}

NS_IMETHODIMP
nsHTMLEditorEventListener::MouseClick(nsIDOMEvent* aMouseEvent)
{
  NS_ENSURE_TRUE(mEditor, NS_ERROR_NOT_AVAILABLE);

  nsCOMPtr<nsIDOMMouseEvent> mouseEvent ( do_QueryInterface(aMouseEvent) );
  if (!mouseEvent) {
    
    return NS_OK;
  }

  nsCOMPtr<nsIDOMEventTarget> target;
  nsresult res = aMouseEvent->GetTarget(getter_AddRefs(target));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(target, NS_ERROR_NULL_POINTER);
  nsCOMPtr<nsIDOMElement> element = do_QueryInterface(target);

  GetHTMLEditor()->DoInlineTableEditingAction(element);

  return nsEditorEventListener::MouseClick(aMouseEvent);
}
