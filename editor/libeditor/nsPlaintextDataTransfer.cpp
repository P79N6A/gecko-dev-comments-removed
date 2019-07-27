




#include "mozilla/ArrayUtils.h"
#include "mozilla/MouseEvents.h"
#include "mozilla/dom/Selection.h"
#include "nsAString.h"
#include "nsCOMPtr.h"
#include "nsCRT.h"
#include "nsComponentManagerUtils.h"
#include "nsContentUtils.h"
#include "nsDebug.h"
#include "nsEditor.h"
#include "nsEditorUtils.h"
#include "nsError.h"
#include "nsIClipboard.h"
#include "nsIContent.h"
#include "nsIDOMDataTransfer.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDragEvent.h"
#include "nsIDOMEvent.h"
#include "nsIDOMNode.h"
#include "nsIDOMUIEvent.h"
#include "nsIDocument.h"
#include "nsIDragService.h"
#include "nsIDragSession.h"
#include "nsIEditor.h"
#include "nsIEditorIMESupport.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIPrincipal.h"
#include "nsIFormControl.h"
#include "nsIPlaintextEditor.h"
#include "nsISupportsPrimitives.h"
#include "nsITransferable.h"
#include "nsIVariant.h"
#include "nsLiteralString.h"
#include "nsPlaintextEditor.h"
#include "nsRange.h"
#include "nsSelectionState.h"
#include "nsServiceManagerUtils.h"
#include "nsString.h"
#include "nsXPCOM.h"
#include "nscore.h"

class nsILoadContext;
class nsISupports;

using namespace mozilla;
using namespace mozilla::dom;

NS_IMETHODIMP nsPlaintextEditor::PrepareTransferable(nsITransferable **transferable)
{
  
  nsresult rv = CallCreateInstance("@mozilla.org/widget/transferable;1", transferable);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (transferable) {
    nsCOMPtr<nsIDocument> destdoc = GetDocument();
    nsILoadContext* loadContext = destdoc ? destdoc->GetLoadContext() : nullptr;
    (*transferable)->Init(loadContext);

    (*transferable)->AddDataFlavor(kUnicodeMime);
    (*transferable)->AddDataFlavor(kMozTextInternal);
  };
  return NS_OK;
}

nsresult nsPlaintextEditor::InsertTextAt(const nsAString &aStringToInsert,
                                         nsIDOMNode *aDestinationNode,
                                         int32_t aDestOffset,
                                         bool aDoDeleteSelection)
{
  if (aDestinationNode)
  {
    nsresult res;
    nsRefPtr<Selection> selection = GetSelection();
    NS_ENSURE_STATE(selection);

    nsCOMPtr<nsIDOMNode> targetNode = aDestinationNode;
    int32_t targetOffset = aDestOffset;

    if (aDoDeleteSelection)
    {
      
      
      nsAutoTrackDOMPoint tracker(mRangeUpdater, &targetNode, &targetOffset);
      res = DeleteSelection(eNone, eStrip);
      NS_ENSURE_SUCCESS(res, res);
    }

    res = selection->Collapse(targetNode, targetOffset);
    NS_ENSURE_SUCCESS(res, res);
  }

  return InsertText(aStringToInsert);
}

NS_IMETHODIMP nsPlaintextEditor::InsertTextFromTransferable(nsITransferable *aTransferable,
                                                            nsIDOMNode *aDestinationNode,
                                                            int32_t aDestOffset,
                                                            bool aDoDeleteSelection)
{
  nsresult rv = NS_OK;
  char* bestFlavor = nullptr;
  nsCOMPtr<nsISupports> genericDataObj;
  uint32_t len = 0;
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
  free(bestFlavor);
      
  

  if (NS_SUCCEEDED(rv))
    ScrollSelectionIntoView(false);

  return rv;
}

nsresult nsPlaintextEditor::InsertFromDataTransfer(DataTransfer *aDataTransfer,
                                                   int32_t aIndex,
                                                   nsIDOMDocument *aSourceDoc,
                                                   nsIDOMNode *aDestinationNode,
                                                   int32_t aDestOffset,
                                                   bool aDoDeleteSelection)
{
  nsCOMPtr<nsIVariant> data;
  aDataTransfer->MozGetDataAt(NS_LITERAL_STRING("text/plain"), aIndex,
                              getter_AddRefs(data));
  if (data) {
    nsAutoString insertText;
    data->GetAsAString(insertText);
    nsContentUtils::PlatformToDOMLineBreaks(insertText);

    nsAutoEditBatch beginBatching(this);
    return InsertTextAt(insertText, aDestinationNode, aDestOffset, aDoDeleteSelection);
  }

  return NS_OK;
}

nsresult nsPlaintextEditor::InsertFromDrop(nsIDOMEvent* aDropEvent)
{
  ForceCompositionEnd();

  nsCOMPtr<nsIDOMDragEvent> dragEvent(do_QueryInterface(aDropEvent));
  NS_ENSURE_TRUE(dragEvent, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDOMDataTransfer> domDataTransfer;
  dragEvent->GetDataTransfer(getter_AddRefs(domDataTransfer));
  nsCOMPtr<DataTransfer> dataTransfer = do_QueryInterface(domDataTransfer);
  NS_ENSURE_TRUE(dataTransfer, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDragSession> dragSession = nsContentUtils::GetDragSession();
  NS_ASSERTION(dragSession, "No drag session");

  nsCOMPtr<nsIDOMNode> sourceNode;
  dataTransfer->GetMozSourceNode(getter_AddRefs(sourceNode));

  nsCOMPtr<nsIDOMDocument> srcdomdoc;
  if (sourceNode) {
    sourceNode->GetOwnerDocument(getter_AddRefs(srcdomdoc));
    NS_ENSURE_TRUE(sourceNode, NS_ERROR_FAILURE);
  }

  if (nsContentUtils::CheckForSubFrameDrop(dragSession,
        aDropEvent->GetInternalNSEvent()->AsDragEvent())) {
    
    
    if (srcdomdoc && !IsSafeToInsertData(srcdomdoc))
      return NS_OK;
  }

  
  nsCOMPtr<nsIDOMDocument> destdomdoc = GetDOMDocument();
  NS_ENSURE_TRUE(destdomdoc, NS_ERROR_NOT_INITIALIZED);

  uint32_t numItems = 0;
  nsresult rv = dataTransfer->GetMozItemCount(&numItems);
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

  int32_t newSelectionOffset;
  rv = uiEvent->GetRangeOffset(&newSelectionOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<Selection> selection = GetSelection();
  NS_ENSURE_TRUE(selection, NS_ERROR_FAILURE);

  bool isCollapsed = selection->Collapsed();

  
  nsCOMPtr<nsIDOMNode> userSelectNode = FindUserSelectAllNode(newSelectionParent);
  if (userSelectNode)
  {
    
    
    
    
    
    
    
    

    newSelectionParent = GetNodeLocation(userSelectNode, &newSelectionOffset);

    NS_ENSURE_TRUE(newSelectionParent, NS_ERROR_FAILURE);
  }

  
  
  
  if (!isCollapsed)
  {
    
    bool cursorIsInSelection = false;

    int32_t rangeCount;
    rv = selection->GetRangeCount(&rangeCount);
    NS_ENSURE_SUCCESS(rv, rv);

    for (int32_t j = 0; j < rangeCount; j++)
    {
      nsRefPtr<nsRange> range = selection->GetRangeAt(j);
      if (!range) {
        
        continue;
      }

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
        
        uint32_t dropEffect;
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

  for (uint32_t i = 0; i < numItems; ++i) {
    InsertFromDataTransfer(dataTransfer, i, srcdomdoc, newSelectionParent,
                           newSelectionOffset, deleteSelection);
  }

  if (NS_SUCCEEDED(rv))
    ScrollSelectionIntoView(false);

  return rv;
}

NS_IMETHODIMP nsPlaintextEditor::Paste(int32_t aSelectionType)
{
  if (!FireClipboardEvent(NS_PASTE, aSelectionType))
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
      
      nsCOMPtr<nsIDOMDocument> domdoc = GetDOMDocument();
      if (!nsEditorHookUtils::DoInsertionHook(domdoc, nullptr, trans))
        return NS_OK;

      rv = InsertTextFromTransferable(trans, nullptr, 0, true);
    }
  }

  return rv;
}

NS_IMETHODIMP nsPlaintextEditor::PasteTransferable(nsITransferable *aTransferable)
{
  
  
  if (!FireClipboardEvent(NS_PASTE, -1))
    return NS_OK;

  if (!IsModifiable())
    return NS_OK;

  
  nsCOMPtr<nsIDOMDocument> domdoc = GetDOMDocument();
  if (!nsEditorHookUtils::DoInsertionHook(domdoc, nullptr, aTransferable))
    return NS_OK;

  return InsertTextFromTransferable(aTransferable, nullptr, 0, true);
}

NS_IMETHODIMP nsPlaintextEditor::CanPaste(int32_t aSelectionType, bool *aCanPaste)
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
  uint32_t dataLen;
  nsresult rv = aTransferable->GetTransferData(kUnicodeMime,
                                               getter_AddRefs(data),
                                               &dataLen);
  if (NS_SUCCEEDED(rv) && data)
    *aCanPaste = true;
  else
    *aCanPaste = false;
  
  return NS_OK;
}

bool nsPlaintextEditor::IsSafeToInsertData(nsIDOMDocument* aSourceDoc)
{
  
  bool isSafe = false;

  nsCOMPtr<nsIDocument> destdoc = GetDocument();
  NS_ASSERTION(destdoc, "Where is our destination doc?");
  nsCOMPtr<nsIDocShellTreeItem> dsti = destdoc->GetDocShell();
  nsCOMPtr<nsIDocShellTreeItem> root;
  if (dsti)
    dsti->GetRootTreeItem(getter_AddRefs(root));
  nsCOMPtr<nsIDocShell> docShell = do_QueryInterface(root);
  uint32_t appType;
  if (docShell && NS_SUCCEEDED(docShell->GetAppType(&appType)))
    isSafe = appType == nsIDocShell::APP_TYPE_EDITOR;
  if (!isSafe && aSourceDoc) {
    nsCOMPtr<nsIDocument> srcdoc = do_QueryInterface(aSourceDoc);
    NS_ASSERTION(srcdoc, "Where is our source doc?");

    nsIPrincipal* srcPrincipal = srcdoc->NodePrincipal();
    nsIPrincipal* destPrincipal = destdoc->NodePrincipal();
    NS_ASSERTION(srcPrincipal && destPrincipal, "How come we don't have a principal?");
    srcPrincipal->Subsumes(destPrincipal, &isSafe);
  }

  return isSafe;
}

