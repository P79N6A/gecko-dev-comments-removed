





































#include "nsHTMLEditorMouseListener.h"
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

#include "nsIEditor.h"
#include "nsIHTMLEditor.h"
#include "nsIHTMLObjectResizer.h"
#include "nsEditProperty.h"
#include "nsTextEditUtils.h"
#include "nsHTMLEditUtils.h"
#include "nsIHTMLInlineTableEditor.h"








nsHTMLEditorMouseListener::nsHTMLEditorMouseListener(nsHTMLEditor *aHTMLEditor)
  : mHTMLEditor(aHTMLEditor)
{
  SetEditor(mHTMLEditor); 
}

nsHTMLEditorMouseListener::~nsHTMLEditorMouseListener() 
{
}

NS_IMPL_ISUPPORTS_INHERITED1(nsHTMLEditorMouseListener, nsTextEditorMouseListener, nsIDOMMouseListener)

nsresult
nsHTMLEditorMouseListener::MouseUp(nsIDOMEvent* aMouseEvent)
{
  nsCOMPtr<nsIDOMMouseEvent> mouseEvent ( do_QueryInterface(aMouseEvent) );
  if (!mouseEvent) {
    
    return NS_OK;
  }

  
  nsCOMPtr<nsIHTMLEditor> htmlEditor = do_QueryInterface(mEditor);
  if (htmlEditor)
  {
    nsCOMPtr<nsIDOMEventTarget> target;
    nsresult res = aMouseEvent->GetTarget(getter_AddRefs(target));
    if (NS_FAILED(res)) return res;
    if (!target) return NS_ERROR_NULL_POINTER;
    nsCOMPtr<nsIDOMElement> element = do_QueryInterface(target);

    nsCOMPtr<nsIHTMLObjectResizer> objectResizer = do_QueryInterface(htmlEditor);
    PRInt32 clientX, clientY;
    mouseEvent->GetClientX(&clientX);
    mouseEvent->GetClientY(&clientY);
    objectResizer->MouseUp(clientX, clientY, element);
  }

  return nsTextEditorMouseListener::MouseUp(aMouseEvent);
}

nsresult
nsHTMLEditorMouseListener::MouseDown(nsIDOMEvent* aMouseEvent)
{
  nsCOMPtr<nsIDOMMouseEvent> mouseEvent ( do_QueryInterface(aMouseEvent) );
  if (!mouseEvent) {
    
    return NS_OK;
  }

  
  nsCOMPtr<nsIHTMLEditor> htmlEditor = do_QueryInterface(mEditor);
  if (htmlEditor)
  {
    
    
    
    PRUint16 buttonNumber;
    nsresult res = mouseEvent->GetButton(&buttonNumber);
    if (NS_FAILED(res)) return res;

    PRBool isContextClick;

#if defined(XP_MAC) || defined(XP_MACOSX)
    
    res = mouseEvent->GetCtrlKey(&isContextClick);
    if (NS_FAILED(res)) return res;
#else
    
    isContextClick = buttonNumber == 2;
#endif
    
    PRInt32 clickCount;
    res = mouseEvent->GetDetail(&clickCount);
    if (NS_FAILED(res)) return res;

    nsCOMPtr<nsIDOMEventTarget> target;
    nsCOMPtr<nsIDOMNSEvent> internalEvent = do_QueryInterface(aMouseEvent);
    res = internalEvent->GetExplicitOriginalTarget(getter_AddRefs(target));
    if (NS_FAILED(res)) return res;
    if (!target) return NS_ERROR_NULL_POINTER;
    nsCOMPtr<nsIDOMElement> element = do_QueryInterface(target);

    if (isContextClick || (buttonNumber == 0 && clickCount == 2))
    {
      nsCOMPtr<nsISelection> selection;
      mEditor->GetSelection(getter_AddRefs(selection));
      if (!selection) return NS_OK;

      
      nsCOMPtr<nsIDOMNSUIEvent> uiEvent = do_QueryInterface(aMouseEvent);
      if (!uiEvent) return NS_ERROR_FAILURE;

      nsCOMPtr<nsIDOMNode> parent;
      PRInt32 offset = 0;

      res = uiEvent->GetRangeParent(getter_AddRefs(parent));
      if (NS_FAILED(res)) return res;
      if (!parent) return NS_ERROR_FAILURE;

      res = uiEvent->GetRangeOffset(&offset);
      if (NS_FAILED(res)) return res;

      
      PRBool nodeIsInSelection = PR_FALSE;
      if (isContextClick)
      {
        PRBool isCollapsed;
        selection->GetIsCollapsed(&isCollapsed);
        if (!isCollapsed)
        {
          PRInt32 rangeCount;
          res = selection->GetRangeCount(&rangeCount);
          if (NS_FAILED(res)) return res;

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
            if (NS_FAILED(res)) return res;
            if (linkElement)
              element = linkElement;
          }
        }
        
        
        if (element)
        {
          nsCOMPtr<nsIDOMNode> selectAllNode = mHTMLEditor->FindUserSelectAllNode(element);

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
      
      nsCOMPtr<nsIHTMLObjectResizer> objectResizer = do_QueryInterface(htmlEditor);
      PRInt32 clientX, clientY;
      mouseEvent->GetClientX(&clientX);
      mouseEvent->GetClientY(&clientY);
      objectResizer->MouseDown(clientX, clientY, element);
    }
  }

  return nsTextEditorMouseListener::MouseDown(aMouseEvent);
}

nsresult
nsHTMLEditorMouseListener::MouseClick(nsIDOMEvent* aMouseEvent)
{
  nsCOMPtr<nsIDOMMouseEvent> mouseEvent ( do_QueryInterface(aMouseEvent) );
  if (!mouseEvent) {
    
    return NS_OK;
  }

  
  nsCOMPtr<nsIHTMLInlineTableEditor> inlineTableEditing = do_QueryInterface(mEditor);
  if (inlineTableEditing)
  {
    nsCOMPtr<nsIDOMEventTarget> target;
    nsresult res = aMouseEvent->GetTarget(getter_AddRefs(target));
    if (NS_FAILED(res)) return res;
    if (!target) return NS_ERROR_NULL_POINTER;
    nsCOMPtr<nsIDOMElement> element = do_QueryInterface(target);

    inlineTableEditing->DoInlineTableEditingAction(element);
  }

  return nsTextEditorMouseListener::MouseClick(aMouseEvent);
}

nsresult
NS_NewHTMLEditorMouseListener(nsIDOMEventListener ** aInstancePtrResult, 
                              nsHTMLEditor *aHTMLEditor)
{
  nsHTMLEditorMouseListener* listener = new nsHTMLEditorMouseListener(aHTMLEditor);
  if (!listener)
    return NS_ERROR_OUT_OF_MEMORY;

  return listener->QueryInterface(NS_GET_IID(nsIDOMEventListener), (void **) aInstancePtrResult);   
}
