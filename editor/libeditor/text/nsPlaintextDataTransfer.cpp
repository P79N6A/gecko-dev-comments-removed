





































#include "nsPlaintextEditor.h"

#include "nsIDOMDocument.h"
#include "nsIDocument.h"
#include "nsIContent.h"
#include "nsIFormControl.h"
#include "nsIDOMEventTarget.h" 
#include "nsIDOMNSEvent.h"
#include "nsIDOMMouseEvent.h"
#include "nsISelection.h"
#include "nsCRT.h"
#include "nsServiceManagerUtils.h"

#include "nsIDOMRange.h"
#include "nsIDOMNSRange.h"
#include "nsISupportsArray.h"
#include "nsIDocumentEncoder.h"
#include "nsISupportsPrimitives.h"


#include "nsIClipboard.h"
#include "nsITransferable.h"
#include "nsIDragService.h"
#include "nsIDOMNSUIEvent.h"


#include "nsEditorUtils.h"
#include "nsContentCID.h"
#include "nsISelectionPrivate.h"
#include "nsFrameSelection.h"

NS_IMETHODIMP nsPlaintextEditor::PrepareTransferable(nsITransferable **transferable)
{
  
  nsresult rv = CallCreateInstance("@mozilla.org/widget/transferable;1", transferable);
  if (NS_FAILED(rv))
    return rv;

  
  if (transferable) (*transferable)->AddDataFlavor(kUnicodeMime);
  return NS_OK;
}

nsresult nsPlaintextEditor::InsertTextAt(const nsAString &aStringToInsert,
                                         nsIDOMNode *aDestinationNode,
                                         PRInt32 aDestOffset,
                                         PRBool aDoDeleteSelection)
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
                                                            PRBool aDoDeleteSelection)
{
  nsresult rv = NS_OK;
  char* bestFlavor = nsnull;
  nsCOMPtr<nsISupports> genericDataObj;
  PRUint32 len = 0;
  if (NS_SUCCEEDED(aTransferable->GetAnyTransferData(&bestFlavor, getter_AddRefs(genericDataObj), &len))
      && bestFlavor && 0 == nsCRT::strcmp(bestFlavor, kUnicodeMime))
  {
    nsAutoTxnsConserveSelection dontSpazMySelection(this);
    nsCOMPtr<nsISupportsString> textDataObj ( do_QueryInterface(genericDataObj) );
    if (textDataObj && len > 0)
    {
      nsAutoString stuffToPaste;
      textDataObj->GetData(stuffToPaste);
      NS_ASSERTION(stuffToPaste.Length() <= (len/2), "Invalid length!");
      nsAutoEditBatch beginBatching(this);
      rv = InsertTextAt(stuffToPaste, aDestinationNode, aDestOffset, aDoDeleteSelection);
    }
  }
  NS_Free(bestFlavor);
      
  
  if (NS_SUCCEEDED(rv))
    ScrollSelectionIntoView(PR_FALSE);

  return rv;
}

NS_IMETHODIMP nsPlaintextEditor::InsertFromDrop(nsIDOMEvent* aDropEvent)
{
  ForceCompositionEnd();
  
  nsresult rv;
  nsCOMPtr<nsIDragService> dragService = 
           do_GetService("@mozilla.org/widget/dragservice;1", &rv);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIDragSession> dragSession;
  dragService->GetCurrentSession(getter_AddRefs(dragSession));
  if (!dragSession) return NS_OK;

  
  nsCOMPtr<nsIDOMDocument> destdomdoc; 
  rv = GetDocument(getter_AddRefs(destdomdoc)); 
  if (NS_FAILED(rv)) return rv;

  
  if (!nsEditorHookUtils::DoAllowDropHook(destdomdoc, aDropEvent, dragSession))
    return NS_OK;

  
  nsCOMPtr<nsITransferable> trans;
  rv = PrepareTransferable(getter_AddRefs(trans));
  if (NS_FAILED(rv)) return rv;
  if (!trans) return NS_OK;  

  PRUint32 numItems = 0; 
  rv = dragSession->GetNumDropItems(&numItems);
  if (NS_FAILED(rv)) return rv;
  if (numItems < 1) return NS_ERROR_FAILURE;  

  
  nsAutoEditBatch beginBatching(this);

  PRBool deleteSelection = PR_FALSE;

  
  
  nsCOMPtr<nsIDOMNSUIEvent> nsuiEvent (do_QueryInterface(aDropEvent));
  if (!nsuiEvent) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMNode> newSelectionParent;
  rv = nsuiEvent->GetRangeParent(getter_AddRefs(newSelectionParent));
  if (NS_FAILED(rv)) return rv;
  if (!newSelectionParent) return NS_ERROR_FAILURE;

  PRInt32 newSelectionOffset;
  rv = nsuiEvent->GetRangeOffset(&newSelectionOffset);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsISelection> selection;
  rv = GetSelection(getter_AddRefs(selection));
  if (NS_FAILED(rv)) return rv;
  if (!selection) return NS_ERROR_FAILURE;

  PRBool isCollapsed;
  rv = selection->GetIsCollapsed(&isCollapsed);
  if (NS_FAILED(rv)) return rv;

  
  
  
  if (!isCollapsed)
  {
    
    PRBool cursorIsInSelection = PR_FALSE;

    PRInt32 rangeCount;
    rv = selection->GetRangeCount(&rangeCount);
    if (NS_FAILED(rv)) return rv;

    for (PRInt32 j = 0; j < rangeCount; j++)
    {
      nsCOMPtr<nsIDOMRange> range;
      rv = selection->GetRangeAt(j, getter_AddRefs(range));
      nsCOMPtr<nsIDOMNSRange> nsrange(do_QueryInterface(range));
      if (NS_FAILED(rv) || !nsrange) 
        continue;  

      rv = nsrange->IsPointInRange(newSelectionParent, newSelectionOffset, &cursorIsInSelection);
      if (cursorIsInSelection)
        break;
    }

    
    
    nsCOMPtr<nsIDOMDocument> srcdomdoc;
    rv = dragSession->GetSourceDocument(getter_AddRefs(srcdomdoc));
    if (NS_FAILED(rv)) return rv;

    if (cursorIsInSelection)
    {
      
      if (srcdomdoc == destdomdoc)
        return NS_OK;

      
      
      
      
    }
    else 
    {
      
      if (srcdomdoc == destdomdoc)
      {
        
 
        
        
        PRBool userWantsCopy = PR_FALSE;

        nsCOMPtr<nsIDOMMouseEvent> mouseEvent ( do_QueryInterface(aDropEvent) );
        if (mouseEvent)
#if defined(XP_MAC) || defined(XP_MACOSX)
          mouseEvent->GetAltKey(&userWantsCopy);
#else
          mouseEvent->GetCtrlKey(&userWantsCopy);
#endif

        deleteSelection = !userWantsCopy;
      }
      else
      {
        
        deleteSelection = PR_FALSE;
      }
    }
  }

  nsCOMPtr<nsIContent> newSelectionContent =
    do_QueryInterface(newSelectionParent);
  nsIContent *content = newSelectionContent;

  while (content) {
    nsCOMPtr<nsIFormControl> formControl(do_QueryInterface(content));

    if (formControl && !formControl->AllowDrop()) {
      
      

      return NS_OK;
    }

    content = content->GetParent();
  }

  PRUint32 i; 
  for (i = 0; i < numItems; ++i)
  {
    rv = dragSession->GetData(trans, i);
    if (NS_FAILED(rv)) return rv;
    if (!trans) return NS_OK; 

    if (!nsEditorHookUtils::DoInsertionHook(destdomdoc, aDropEvent, trans))
      return NS_OK;

    rv = InsertTextFromTransferable(trans, newSelectionParent, newSelectionOffset, deleteSelection);
  }

  return rv;
}

NS_IMETHODIMP nsPlaintextEditor::CanDrag(nsIDOMEvent *aDragEvent, PRBool *aCanDrag)
{
  if (!aCanDrag)
    return NS_ERROR_NULL_POINTER;
  


  *aCanDrag = PR_FALSE;
 
  
  
  
  if (mIgnoreSpuriousDragEvent)
  {
    mIgnoreSpuriousDragEvent = PR_FALSE;
    return NS_OK;
  }
   
  nsCOMPtr<nsISelection> selection;
  nsresult res = GetSelection(getter_AddRefs(selection));
  if (NS_FAILED(res)) return res;
    
  PRBool isCollapsed;
  res = selection->GetIsCollapsed(&isCollapsed);
  if (NS_FAILED(res)) return res;
  
  
  if ( isCollapsed )
    return NS_OK;

  nsCOMPtr<nsIDOMEventTarget> eventTarget;

  nsCOMPtr<nsIDOMNSEvent> nsevent(do_QueryInterface(aDragEvent));
  if (nsevent) {
    res = nsevent->GetTmpRealOriginalTarget(getter_AddRefs(eventTarget));
    if (NS_FAILED(res)) return res;
  }

  if (eventTarget)
  {
    nsCOMPtr<nsIDOMNode> eventTargetDomNode = do_QueryInterface(eventTarget);
    if ( eventTargetDomNode )
    {
      PRBool isTargetedCorrectly = PR_FALSE;
      res = selection->ContainsNode(eventTargetDomNode, PR_FALSE, &isTargetedCorrectly);
      if (NS_FAILED(res)) return res;

      *aCanDrag = isTargetedCorrectly;
    }
  }

  if (NS_FAILED(res)) return res;
  if (!*aCanDrag) return NS_OK;

  nsCOMPtr<nsIDOMDocument> domdoc;
  GetDocument(getter_AddRefs(domdoc));
  *aCanDrag = nsEditorHookUtils::DoAllowDragHook(domdoc, aDragEvent);
  return NS_OK;
}

NS_IMETHODIMP nsPlaintextEditor::DoDrag(nsIDOMEvent *aDragEvent)
{
  nsresult rv;

  nsCOMPtr<nsITransferable> trans;
  rv = PutDragDataInTransferable(getter_AddRefs(trans));
  if (NS_FAILED(rv)) return rv;
  if (!trans) return NS_OK; 

 
  nsCOMPtr<nsIDragService> dragService = 
           do_GetService("@mozilla.org/widget/dragservice;1", &rv);
  if (NS_FAILED(rv)) return rv;

  
  nsCOMPtr<nsISupportsArray> transferableArray;
  NS_NewISupportsArray(getter_AddRefs(transferableArray));
  if (!transferableArray)
    return NS_ERROR_OUT_OF_MEMORY;

  
  rv = transferableArray->AppendElement(trans);
  if (NS_FAILED(rv)) return rv;

  
  nsCOMPtr<nsIDOMDocument> domdoc;
  GetDocument(getter_AddRefs(domdoc));
  if (!nsEditorHookUtils::DoDragHook(domdoc, aDragEvent, trans))
    return NS_OK;

  
  nsCOMPtr<nsIDOMEventTarget> eventTarget;
  rv = aDragEvent->GetTarget(getter_AddRefs(eventTarget));
  if (NS_FAILED(rv)) return rv;
  nsCOMPtr<nsIDOMNode> domnode = do_QueryInterface(eventTarget);

  nsCOMPtr<nsIScriptableRegion> selRegion;
  nsCOMPtr<nsISelection> selection;
  rv = GetSelection(getter_AddRefs(selection));
  if (NS_FAILED(rv)) return rv;

  unsigned int flags;
  
  flags = nsIDragService::DRAGDROP_ACTION_COPY + nsIDragService::DRAGDROP_ACTION_MOVE;

  nsCOMPtr<nsIDOMMouseEvent> mouseEvent(do_QueryInterface(aDragEvent));
  rv = dragService->InvokeDragSessionWithSelection(selection, transferableArray,
                                                   flags, mouseEvent);
  if (NS_FAILED(rv)) return rv;

  aDragEvent->StopPropagation();

  return rv;
}

NS_IMETHODIMP nsPlaintextEditor::Paste(PRInt32 aSelectionType)
{
  ForceCompositionEnd();

  
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

      rv = InsertTextFromTransferable(trans, nsnull, nsnull, PR_TRUE);
    }
  }

  return rv;
}


NS_IMETHODIMP nsPlaintextEditor::CanPaste(PRInt32 aSelectionType, PRBool *aCanPaste)
{
  if (!aCanPaste)
    return NS_ERROR_NULL_POINTER;
  *aCanPaste = PR_FALSE;
  
  
  if (!IsModifiable())
    return NS_OK;

  nsresult rv;
  nsCOMPtr<nsIClipboard> clipboard(do_GetService("@mozilla.org/widget/clipboard;1", &rv));
  if (NS_FAILED(rv)) return rv;
  
  
  const char* const textEditorFlavors[] = { kUnicodeMime, nsnull };

  nsCOMPtr<nsISupportsArray> flavorsList = do_CreateInstance(NS_SUPPORTSARRAY_CONTRACTID);

  PRUint32 editorFlags;
  GetFlags(&editorFlags);
  
  
  for (const char* const* flavor = textEditorFlavors; *flavor; flavor++)
  {
    nsCOMPtr<nsISupportsCString> flavorString =
        do_CreateInstance(NS_SUPPORTS_CSTRING_CONTRACTID);
    if (flavorString)
    {
      flavorString->SetData(nsDependentCString(*flavor));
      flavorsList->AppendElement(flavorString);
    }
  }
  
  PRBool haveFlavors;
  rv = clipboard->HasDataMatchingFlavors(flavorsList, aSelectionType, &haveFlavors);
  if (NS_FAILED(rv)) return rv;
  
  *aCanPaste = haveFlavors;
  return NS_OK;
}

nsresult
nsPlaintextEditor::SetupDocEncoder(nsIDocumentEncoder **aDocEncoder)
{
  nsCOMPtr<nsIDOMDocument> domDoc;
  nsresult rv = GetDocument(getter_AddRefs(domDoc));
  if (NS_FAILED(rv)) return rv;

  
  PRUint32 editorFlags = 0;
  rv = GetFlags(&editorFlags);
  if (NS_FAILED(rv)) return rv;

  PRBool bIsPlainTextControl = ((editorFlags & eEditorPlaintextMask) != 0);

  
  nsAutoString mimeType;
  PRUint32 docEncoderFlags = 0;
  if (bIsPlainTextControl)
  {
    docEncoderFlags |= nsIDocumentEncoder::OutputBodyOnly | nsIDocumentEncoder::OutputPreformatted;
    mimeType.AssignLiteral(kUnicodeMime);
  }
  else
    mimeType.AssignLiteral(kHTMLMime);

  
  nsCOMPtr<nsIDocumentEncoder> encoder = do_CreateInstance(NS_HTMLCOPY_ENCODER_CONTRACTID);
  if (!encoder)
    return NS_ERROR_OUT_OF_MEMORY;

  rv = encoder->Init(domDoc, mimeType, docEncoderFlags);
  if (NS_FAILED(rv)) return rv;
    
  
  nsCOMPtr<nsISelection> selection;
  rv = GetSelection(getter_AddRefs(selection));
  if (NS_FAILED(rv)) return rv;

  rv = encoder->SetSelection(selection);
  if (NS_FAILED(rv)) return rv;

  *aDocEncoder = encoder;
  NS_ADDREF(*aDocEncoder);
  return NS_OK;
}

nsresult
nsPlaintextEditor::PutDragDataInTransferable(nsITransferable **aTransferable)
{
  *aTransferable = nsnull;
  nsCOMPtr<nsIDocumentEncoder> docEncoder;
  nsresult rv = SetupDocEncoder(getter_AddRefs(docEncoder));
  if (NS_FAILED(rv)) return rv;

  
  nsAutoString buffer;
  rv = docEncoder->EncodeToString(buffer);
  if (NS_FAILED(rv)) return rv;

  
  if (buffer.IsEmpty())
    return NS_OK;

  nsCOMPtr<nsISupportsString> dataWrapper =
                        do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = dataWrapper->SetData(buffer);
  if (NS_FAILED(rv)) return rv;

  
  nsCOMPtr<nsITransferable> trans = do_CreateInstance("@mozilla.org/widget/transferable;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRUint32 editorFlags = 0;
  rv = GetFlags(&editorFlags);
  if (NS_FAILED(rv)) return rv;

  PRBool bIsPlainTextControl = ((editorFlags & eEditorPlaintextMask) != 0);
  if (bIsPlainTextControl)
  {
    
    rv = trans->AddDataFlavor(kUnicodeMime);
    if (NS_FAILED(rv)) return rv;
  }
  else
  {
    rv = trans->AddDataFlavor(kHTMLMime);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIFormatConverter> htmlConverter = do_CreateInstance("@mozilla.org/widget/htmlformatconverter;1");
    NS_ENSURE_TRUE(htmlConverter, NS_ERROR_FAILURE);

    rv = trans->SetConverter(htmlConverter);
    if (NS_FAILED(rv)) return rv;
  }

  
  
  nsCOMPtr<nsISupports> nsisupportsDataWrapper = do_QueryInterface(dataWrapper);
  rv = trans->SetTransferData(bIsPlainTextControl ? kUnicodeMime : kHTMLMime,
                   nsisupportsDataWrapper, buffer.Length() * sizeof(PRUnichar));
  if (NS_FAILED(rv)) return rv;

  *aTransferable = trans;
  NS_ADDREF(*aTransferable);
  return NS_OK;
}
