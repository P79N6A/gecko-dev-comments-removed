




































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
#include "nsIDOMNSRange.h"
#include "nsIDocumentEncoder.h"
#include "nsISupportsPrimitives.h"


#include "nsIClipboard.h"
#include "nsITransferable.h"
#include "nsIDragService.h"
#include "nsIDOMUIEvent.h"
#include "nsCopySupport.h"


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
    ScrollSelectionIntoView(PR_FALSE);

  return rv;
}

NS_IMETHODIMP nsPlaintextEditor::InsertFromDrop(nsIDOMEvent* aDropEvent)
{
  ForceCompositionEnd();
  
  nsresult rv;
  nsCOMPtr<nsIDragService> dragService = 
           do_GetService("@mozilla.org/widget/dragservice;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDragSession> dragSession;
  dragService->GetCurrentSession(getter_AddRefs(dragSession));
  NS_ENSURE_TRUE(dragSession, NS_OK);

  
  nsCOMPtr<nsIDOMDocument> destdomdoc; 
  rv = GetDocument(getter_AddRefs(destdomdoc)); 
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsITransferable> trans;
  rv = PrepareTransferable(getter_AddRefs(trans));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(trans, NS_OK);  

  PRUint32 numItems = 0; 
  rv = dragSession->GetNumDropItems(&numItems);
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
      nsCOMPtr<nsIDOMNSRange> nsrange(do_QueryInterface(range));
      if (NS_FAILED(rv) || !nsrange) 
        continue;  

      rv = nsrange->IsPointInRange(newSelectionParent, newSelectionOffset, &cursorIsInSelection);
      if (cursorIsInSelection)
        break;
    }

    
    
    nsCOMPtr<nsIDOMDocument> srcdomdoc;
    rv = dragSession->GetSourceDocument(getter_AddRefs(srcdomdoc));
    NS_ENSURE_SUCCESS(rv, rv);

    if (cursorIsInSelection)
    {
      
      if (srcdomdoc == destdomdoc)
        return NS_OK;

      
      
      
      
    }
    else 
    {
      
      if (srcdomdoc == destdomdoc)
      {
        
        PRUint32 action;
        dragSession->GetDragAction(&action);
        deleteSelection = !(action & nsIDragService::DRAGDROP_ACTION_COPY);
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
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(trans, NS_OK); 

    
    
    rv = InsertTextFromTransferable(trans, newSelectionParent, newSelectionOffset, deleteSelection);
  }

  return rv;
}

NS_IMETHODIMP nsPlaintextEditor::CanDrag(nsIDOMEvent *aDragEvent, bool *aCanDrag)
{
  NS_ENSURE_TRUE(aCanDrag, NS_ERROR_NULL_POINTER);
  


  *aCanDrag = PR_FALSE;
 
  
  
  
  if (mIgnoreSpuriousDragEvent)
  {
    mIgnoreSpuriousDragEvent = PR_FALSE;
    return NS_OK;
  }
   
  nsCOMPtr<nsISelection> selection;
  nsresult res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
    
  bool isCollapsed;
  res = selection->GetIsCollapsed(&isCollapsed);
  NS_ENSURE_SUCCESS(res, res);
  
  
  if ( isCollapsed )
    return NS_OK;

  nsCOMPtr<nsIDOMEventTarget> eventTarget;

  nsCOMPtr<nsIDOMNSEvent> nsevent(do_QueryInterface(aDragEvent));
  if (nsevent) {
    res = nsevent->GetTmpRealOriginalTarget(getter_AddRefs(eventTarget));
    NS_ENSURE_SUCCESS(res, res);
  }

  if (eventTarget)
  {
    nsCOMPtr<nsIDOMNode> eventTargetDomNode = do_QueryInterface(eventTarget);
    if ( eventTargetDomNode )
    {
      bool isTargetedCorrectly = false;
      res = selection->ContainsNode(eventTargetDomNode, PR_FALSE, &isTargetedCorrectly);
      NS_ENSURE_SUCCESS(res, res);

      *aCanDrag = isTargetedCorrectly;
    }
  }

  return res;
}

NS_IMETHODIMP nsPlaintextEditor::DoDrag(nsIDOMEvent *aDragEvent)
{
  nsresult rv;

  nsCOMPtr<nsITransferable> trans;
  rv = PutDragDataInTransferable(getter_AddRefs(trans));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(trans, NS_OK); 

 
  nsCOMPtr<nsIDragService> dragService = 
           do_GetService("@mozilla.org/widget/dragservice;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsISupportsArray> transferableArray;
  NS_NewISupportsArray(getter_AddRefs(transferableArray));
  NS_ENSURE_TRUE(transferableArray, NS_ERROR_OUT_OF_MEMORY);

  
  rv = transferableArray->AppendElement(trans);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIDOMDocument> domdoc;
  GetDocument(getter_AddRefs(domdoc));

  
  nsCOMPtr<nsIDOMEventTarget> eventTarget;
  rv = aDragEvent->GetTarget(getter_AddRefs(eventTarget));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIDOMNode> domnode = do_QueryInterface(eventTarget);

  nsCOMPtr<nsIScriptableRegion> selRegion;
  nsCOMPtr<nsISelection> selection;
  rv = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(rv, rv);

  unsigned int flags;
  
  flags = nsIDragService::DRAGDROP_ACTION_COPY + nsIDragService::DRAGDROP_ACTION_MOVE;

  nsCOMPtr<nsIDOMDragEvent> dragEvent(do_QueryInterface(aDragEvent));
  rv = dragService->InvokeDragSessionWithSelection(selection, transferableArray,
                                                   flags, dragEvent, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  aDragEvent->StopPropagation();
  aDragEvent->PreventDefault();

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

      
      
      rv = InsertTextFromTransferable(trans, nsnull, nsnull, PR_TRUE);
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

  
  
  return InsertTextFromTransferable(aTransferable, nsnull, nsnull, PR_TRUE);
}

NS_IMETHODIMP nsPlaintextEditor::CanPaste(PRInt32 aSelectionType, bool *aCanPaste)
{
  NS_ENSURE_ARG_POINTER(aCanPaste);
  *aCanPaste = PR_FALSE;

  
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
    *aCanPaste = PR_FALSE;
    return NS_OK;
  }

  
  if (!aTransferable) {
    *aCanPaste = PR_TRUE;
    return NS_OK;
  }

  nsCOMPtr<nsISupports> data;
  PRUint32 dataLen;
  nsresult rv = aTransferable->GetTransferData(kUnicodeMime,
                                               getter_AddRefs(data),
                                               &dataLen);
  if (NS_SUCCEEDED(rv) && data)
    *aCanPaste = PR_TRUE;
  else
    *aCanPaste = PR_FALSE;
  
  return NS_OK;
}


nsresult
nsPlaintextEditor::SetupDocEncoder(nsIDocumentEncoder **aDocEncoder)
{
  nsCOMPtr<nsIDOMDocument> domDoc;
  nsresult rv = GetDocument(getter_AddRefs(domDoc));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nsAutoString mimeType;
  PRUint32 docEncoderFlags = 0;
  if (IsPlaintextEditor())
  {
    docEncoderFlags |= nsIDocumentEncoder::OutputBodyOnly | nsIDocumentEncoder::OutputPreformatted;
    mimeType.AssignLiteral(kUnicodeMime);
  }
  else
    mimeType.AssignLiteral(kHTMLMime);

  
  nsCOMPtr<nsIDocumentEncoder> encoder = do_CreateInstance(NS_HTMLCOPY_ENCODER_CONTRACTID);
  NS_ENSURE_TRUE(encoder, NS_ERROR_OUT_OF_MEMORY);

  rv = encoder->Init(domDoc, mimeType, docEncoderFlags);
  NS_ENSURE_SUCCESS(rv, rv);
    
  
  nsCOMPtr<nsISelection> selection;
  rv = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = encoder->SetSelection(selection);
  NS_ENSURE_SUCCESS(rv, rv);

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
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsAutoString buffer;
  rv = docEncoder->EncodeToString(buffer);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (buffer.IsEmpty())
    return NS_OK;

  nsCOMPtr<nsISupportsString> dataWrapper =
                        do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = dataWrapper->SetData(buffer);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsITransferable> trans = do_CreateInstance("@mozilla.org/widget/transferable;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (IsPlaintextEditor())
  {
    
    rv = trans->AddDataFlavor(kUnicodeMime);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else
  {
    rv = trans->AddDataFlavor(kHTMLMime);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIFormatConverter> htmlConverter = do_CreateInstance("@mozilla.org/widget/htmlformatconverter;1");
    NS_ENSURE_TRUE(htmlConverter, NS_ERROR_FAILURE);

    rv = trans->SetConverter(htmlConverter);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  nsCOMPtr<nsISupports> nsisupportsDataWrapper = do_QueryInterface(dataWrapper);
  rv = trans->SetTransferData(IsPlaintextEditor() ? kUnicodeMime : kHTMLMime,
                   nsisupportsDataWrapper, buffer.Length() * sizeof(PRUnichar));
  NS_ENSURE_SUCCESS(rv, rv);

  *aTransferable = trans;
  NS_ADDREF(*aTransferable);
  return NS_OK;
}
