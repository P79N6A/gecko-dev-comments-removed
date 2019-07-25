




































#include "mozilla/Util.h"

#include "nsPlaintextEditor.h"

#include "nsIDOMDocument.h"
#include "nsIDocument.h"
#include "nsIContent.h"
#include "nsIFormControl.h"
#include "nsIDOMEventTarget.h" 
#include "nsIDOMNSEvent.h"
#include "nsIDOMMouseEvent.h"
#include "nsIDOMDragEvent.h"
#include "nsISelection.h"
#include "nsCRT.h"
#include "nsServiceManagerUtils.h"

#include "nsIDOMRange.h"
#include "nsIDOMDOMStringList.h"
#include "nsIDocumentEncoder.h"
#include "nsISupportsPrimitives.h"


#include "nsIClipboard.h"
#include "nsITransferable.h"
#include "nsIDragService.h"
#include "nsIDOMUIEvent.h"
#include "nsCopySupport.h"
#include "nsITransferable.h"


#include "nsEditorUtils.h"
#include "nsContentCID.h"
#include "nsISelectionPrivate.h"
#include "nsFrameSelection.h"
#include "nsEventDispatcher.h"
#include "nsContentUtils.h"

using namespace mozilla;

NS_IMETHODIMP nsPlaintextEditor::PrepareTransferable(nsITransferable **transferable)
{
  
  nsresult rv = CallCreateInstance("@mozilla.org/widget/transferable;1", transferable);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (transferable) {
    (*transferable)->AddDataFlavor(kUnicodeMime);
    (*transferable)->AddDataFlavor(kMozTextInternal);
  };
  return NS_OK;
}

nsresult nsPlaintextEditor::InsertTextAt(const nsAString &aStringToInsert,
                                         nsIDOMNode *aDestinationNode,
                                         PRInt32 aDestOffset,
                                         bool aDoDeleteSelection)
{
  if (aDestinationNode)
  {
    nsresult res;
    nsCOMPtr<nsISelection>selection;
    res = GetSelection(getter_AddRefs(selection));
    NS_ENSURE_SUCCESS(res, res);

    nsCOMPtr<nsIDOMNode> targetNode = aDestinationNode;
    PRInt32 targetOffset = aDestOffset;

    if (aDoDeleteSelection)
    {
      
      
      nsAutoTrackDOMPoint tracker(mRangeUpdater, &targetNode, &targetOffset);
      res = DeleteSelection(eNone);
      NS_ENSURE_SUCCESS(res, res);
    }

    res = selection->Collapse(targetNode, targetOffset);
    NS_ENSURE_SUCCESS(res, res);
  }

  return InsertText(aStringToInsert);
}

NS_IMETHODIMP nsPlaintextEditor::InsertTextFromTransferable(nsITransferable *aTransferable,
                                                            nsIDOMNode *aDestinationNode,
                                                            PRInt32 aDestOffset,
                                                            bool aDoDeleteSelection)
{
  FireTrustedInputEvent trusted(this);

  nsresult rv = NS_OK;
  char* bestFlavor = nsnull;
  nsCOMPtr<nsISupports> genericDataObj;
  PRUint32 len = 0;
  if (NS_SUCCEEDED(aTransferable->GetAnyTransferData(&bestFlavor, getter_AddRefs(genericDataObj), &len))
      && bestFlavor && (0 == nsCRT::strcmp(bestFlavor, kUnicodeMime) ||
                        0 == nsCRT::strcmp(bestFlavor, kMozTextInternal)))
  {
    nsAutoTxnsConserveSelection dontSpazMySelection(this);
    nsCOMPtr<nsISupportsString> textDataObj ( do_QueryInterface(genericDataObj) );
    if (textDataObj && len > 0)
    {
      nsAutoString stuffToPaste;
      textDataObj->GetData(stuffToPaste);
      NS_ASSERTION(stuffToPaste.Length() <= (len/2), "Invalid length!");

      
      nsContentUtils::PlatformToDOMLineBreaks(stuffToPaste);

      nsAutoEditBatch beginBatching(this);
      rv = InsertTextAt(stuffToPaste, aDestinationNode, aDestOffset, aDoDeleteSelection);
    }
  }
  NS_Free(bestFlavor);
      
  

  if (NS_SUCCEEDED(rv))
    ScrollSelectionIntoView(false);

  return rv;
}

nsresult nsPlaintextEditor::InsertFromDataTransfer(nsIDOMDataTransfer *aDataTransfer,
                                                   PRInt32 aIndex,
                                                   nsIDOMDocument *aSourceDoc,
                                                   nsIDOMNode *aDestinationNode,
                                                   PRInt32 aDestOffset,
                                                   bool aDoDeleteSelection)
{
  nsCOMPtr<nsIVariant> data;
  aDataTransfer->MozGetDataAt(NS_LITERAL_STRING("text/plain"), aIndex,
                              getter_AddRefs(data));
  nsAutoString insertText;
  data->GetAsAString(insertText);
  nsContentUtils::PlatformToDOMLineBreaks(insertText);

  nsAutoEditBatch beginBatching(this);
  return InsertTextAt(insertText, aDestinationNode, aDestOffset, aDoDeleteSelection);
}

nsresult nsPlaintextEditor::InsertFromDrop(nsIDOMEvent* aDropEvent)
{
  ForceCompositionEnd();

  nsCOMPtr<nsIDOMDragEvent> dragEvent(do_QueryInterface(aDropEvent));
  NS_ENSURE_TRUE(dragEvent, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDOMDataTransfer> dataTransfer;
  nsresult rv = dragEvent->GetDataTransfer(getter_AddRefs(dataTransfer));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIDOMDocument> destdomdoc; 
  rv = GetDocument(getter_AddRefs(destdomdoc)); 
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 numItems = 0;
  rv = dataTransfer->GetMozItemCount(&numItems);
  NS_ENSURE_SUCCESS(rv, rv);
  if (numItems < 1) return NS_ERROR_FAILURE;  

  
  nsAutoEditBatch beginBatching(this);

  bool deleteSelection = false;

  
  
  nsCOMPtr<nsIDOMUIEvent> uiEvent = do_QueryInterface(aDropEvent);
  NS_ENSURE_TRUE(uiEvent, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDOMNode> newSelectionParent;
  rv = uiEvent->GetRangeParent(getter_AddRefs(newSelectionParent));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(newSelectionParent, NS_ERROR_FAILURE);

  PRInt32 newSelectionOffset;
  rv = uiEvent->GetRangeOffset(&newSelectionOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISelection> selection;
  rv = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(selection, NS_ERROR_FAILURE);

  bool isCollapsed;
  rv = selection->GetIsCollapsed(&isCollapsed);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMNode> sourceNode;
  dataTransfer->GetMozSourceNode(getter_AddRefs(sourceNode));

  nsCOMPtr<nsIDOMDocument> srcdomdoc;
  if (sourceNode) {
    sourceNode->GetOwnerDocument(getter_AddRefs(srcdomdoc));
    NS_ENSURE_TRUE(sourceNode, NS_ERROR_FAILURE);
  }

  
  nsCOMPtr<nsIDOMNode> userSelectNode = FindUserSelectAllNode(newSelectionParent);
  if (userSelectNode)
  {
    
    
    
    
    
    
    
    

    rv = GetNodeLocation(userSelectNode, address_of(newSelectionParent),
                         &newSelectionOffset);

    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(newSelectionParent, NS_ERROR_FAILURE);
  }

  
  
  
  if (!isCollapsed)
  {
    
    bool cursorIsInSelection = false;

    PRInt32 rangeCount;
    rv = selection->GetRangeCount(&rangeCount);
    NS_ENSURE_SUCCESS(rv, rv);

    for (PRInt32 j = 0; j < rangeCount; j++)
    {
      nsCOMPtr<nsIDOMRange> range;
      rv = selection->GetRangeAt(j, getter_AddRefs(range));
      if (NS_FAILED(rv) || !range) 
        continue;  

      rv = range->IsPointInRange(newSelectionParent, newSelectionOffset, &cursorIsInSelection);
      if (cursorIsInSelection)
        break;
    }

    if (cursorIsInSelection)
    {
      
      if (srcdomdoc == destdomdoc)
        return NS_OK;

      
      
      
      
    }
    else 
    {
      
      if (srcdomdoc == destdomdoc)
      {
        
        PRUint32 dropEffect;
        dataTransfer->GetDropEffectInt(&dropEffect);
        deleteSelection = !(dropEffect & nsIDragService::DRAGDROP_ACTION_COPY);
      }
      else
      {
        
        deleteSelection = false;
      }
    }
  }

  if (IsPlaintextEditor()) {
    nsCOMPtr<nsIContent> content = do_QueryInterface(newSelectionParent);
    while (content) {
      nsCOMPtr<nsIFormControl> formControl(do_QueryInterface(content));
      if (formControl && !formControl->AllowDrop()) {
        
        
        return NS_OK;
      }
      content = content->GetParent();
    }
  }

  for (PRUint32 i = 0; i < numItems; ++i) {
    InsertFromDataTransfer(dataTransfer, i, srcdomdoc, newSelectionParent,
                           newSelectionOffset, deleteSelection);
  }

  if (NS_SUCCEEDED(rv))
    ScrollSelectionIntoView(false);

  return rv;
}

NS_IMETHODIMP nsPlaintextEditor::Paste(PRInt32 aSelectionType)
{
  if (!FireClipboardEvent(NS_PASTE))
    return NS_OK;

  
  nsresult rv;
  nsCOMPtr<nsIClipboard> clipboard(do_GetService("@mozilla.org/widget/clipboard;1", &rv));
  if ( NS_FAILED(rv) )
    return rv;

  
  nsCOMPtr<nsITransferable> trans;
  rv = PrepareTransferable(getter_AddRefs(trans));
  if (NS_SUCCEEDED(rv) && trans)
  {
    
    if (NS_SUCCEEDED(clipboard->GetData(trans, aSelectionType)) && IsModifiable())
    {
      
      nsCOMPtr<nsIDOMDocument> domdoc;
      GetDocument(getter_AddRefs(domdoc));
      if (!nsEditorHookUtils::DoInsertionHook(domdoc, nsnull, trans))
        return NS_OK;

      rv = InsertTextFromTransferable(trans, nsnull, nsnull, true);
    }
  }

  return rv;
}

NS_IMETHODIMP nsPlaintextEditor::PasteTransferable(nsITransferable *aTransferable)
{
  if (!FireClipboardEvent(NS_PASTE))
    return NS_OK;

  if (!IsModifiable())
    return NS_OK;

  
  nsCOMPtr<nsIDOMDocument> domdoc;
  GetDocument(getter_AddRefs(domdoc));
  if (!nsEditorHookUtils::DoInsertionHook(domdoc, nsnull, aTransferable))
    return NS_OK;

  return InsertTextFromTransferable(aTransferable, nsnull, nsnull, true);
}

NS_IMETHODIMP nsPlaintextEditor::CanPaste(PRInt32 aSelectionType, bool *aCanPaste)
{
  NS_ENSURE_ARG_POINTER(aCanPaste);
  *aCanPaste = false;

  
  if (!IsModifiable())
    return NS_OK;

  nsresult rv;
  nsCOMPtr<nsIClipboard> clipboard(do_GetService("@mozilla.org/widget/clipboard;1", &rv));
  NS_ENSURE_SUCCESS(rv, rv);
  
  
  const char* textEditorFlavors[] = { kUnicodeMime };

  bool haveFlavors;
  rv = clipboard->HasDataMatchingFlavors(textEditorFlavors,
                                         ArrayLength(textEditorFlavors),
                                         aSelectionType, &haveFlavors);
  NS_ENSURE_SUCCESS(rv, rv);
  
  *aCanPaste = haveFlavors;
  return NS_OK;
}


NS_IMETHODIMP nsPlaintextEditor::CanPasteTransferable(nsITransferable *aTransferable, bool *aCanPaste)
{
  NS_ENSURE_ARG_POINTER(aCanPaste);

  
  if (!IsModifiable()) {
    *aCanPaste = false;
    return NS_OK;
  }

  
  if (!aTransferable) {
    *aCanPaste = true;
    return NS_OK;
  }

  nsCOMPtr<nsISupports> data;
  PRUint32 dataLen;
  nsresult rv = aTransferable->GetTransferData(kUnicodeMime,
                                               getter_AddRefs(data),
                                               &dataLen);
  if (NS_SUCCEEDED(rv) && data)
    *aCanPaste = true;
  else
    *aCanPaste = false;
  
  return NS_OK;
}
