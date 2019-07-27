




#include "nsCopySupport.h"
#include "nsIDocumentEncoder.h"
#include "nsISupports.h"
#include "nsIContent.h"
#include "nsIComponentManager.h" 
#include "nsIServiceManager.h"
#include "nsIClipboard.h"
#include "nsISelection.h"
#include "nsWidgetsCID.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsIDOMRange.h"
#include "nsRange.h"
#include "imgIContainer.h"
#include "nsIPresShell.h"
#include "nsFocusManager.h"
#include "mozilla/dom/DataTransfer.h"

#include "nsIDocShell.h"
#include "nsIContentViewerEdit.h"
#include "nsIClipboardDragDropHooks.h"
#include "nsIClipboardDragDropHookList.h"
#include "nsIClipboardHelper.h"
#include "nsISelectionController.h"

#include "nsPIDOMWindow.h"
#include "nsIDocument.h"
#include "nsIDOMNode.h"
#include "nsIDOMElement.h"
#include "nsIDOMDocument.h"
#include "nsIHTMLDocument.h"
#include "nsGkAtoms.h"
#include "nsIFrame.h"
#include "nsIURI.h"
#include "nsISimpleEnumerator.h"


#include "nsIImageLoadingContent.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsContentUtils.h"
#include "nsContentCID.h"

#include "mozilla/ContentEvents.h"
#include "mozilla/dom/Element.h"
#include "mozilla/EventDispatcher.h"
#include "mozilla/Preferences.h"
#include "mozilla/dom/Selection.h"

using namespace mozilla;
using namespace mozilla::dom;

nsresult NS_NewDomSelection(nsISelection **aDomSelection);

static NS_DEFINE_CID(kCClipboardCID,           NS_CLIPBOARD_CID);
static NS_DEFINE_CID(kCTransferableCID,        NS_TRANSFERABLE_CID);
static NS_DEFINE_CID(kHTMLConverterCID,        NS_HTMLFORMATCONVERTER_CID);


static nsresult AppendString(nsITransferable *aTransferable,
                             const nsAString& aString,
                             const char* aFlavor);


static nsresult AppendDOMNode(nsITransferable *aTransferable,
                              nsINode* aDOMNode);



static nsresult
SelectionCopyHelper(nsISelection *aSel, nsIDocument *aDoc,
                    bool doPutOnClipboard, int16_t aClipboardID,
                    uint32_t aFlags, nsITransferable ** aTransferable)
{
  
  if (aTransferable) {
    *aTransferable = nullptr;
  }

  nsresult rv;

  nsCOMPtr<nsIDocumentEncoder> docEncoder;
  docEncoder = do_CreateInstance(NS_HTMLCOPY_ENCODER_CONTRACTID);
  NS_ENSURE_TRUE(docEncoder, NS_ERROR_FAILURE);

  
  
  
  
  
  nsAutoString mimeType;
  mimeType.AssignLiteral(kUnicodeMime);

  
  uint32_t flags = aFlags | nsIDocumentEncoder::OutputPreformatted
                          | nsIDocumentEncoder::OutputRaw;

  nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(aDoc);
  NS_ASSERTION(domDoc, "Need a document");

  rv = docEncoder->Init(domDoc, mimeType, flags);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = docEncoder->SetSelection(aSel);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  rv = docEncoder->GetMimeType(mimeType);
  NS_ENSURE_SUCCESS(rv, rv);
  bool selForcedTextPlain = mimeType.EqualsLiteral(kTextMime);

  nsAutoString buf;
  rv = docEncoder->EncodeToString(buf);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = docEncoder->GetMimeType(mimeType);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!selForcedTextPlain && mimeType.EqualsLiteral(kTextMime)) {
    
    
    
    nsCOMPtr<nsIHTMLDocument> htmlDoc = do_QueryInterface(aDoc);
    if (!htmlDoc) {
      selForcedTextPlain = true;
    }
  }

  
  
  bool encodedTextHTML = mimeType.EqualsLiteral(kHTMLMime);

  
  nsAutoString textPlainBuf;
  if (selForcedTextPlain) {
    
    textPlainBuf.Assign(buf);
  } else {
    
    flags =
      nsIDocumentEncoder::OutputSelectionOnly |
      nsIDocumentEncoder::OutputAbsoluteLinks |
      nsIDocumentEncoder::SkipInvisibleContent |
      nsIDocumentEncoder::OutputDropInvisibleBreak |
      (aFlags & nsIDocumentEncoder::OutputNoScriptContent);

    mimeType.AssignLiteral(kTextMime);
    rv = docEncoder->Init(domDoc, mimeType, flags);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = docEncoder->SetSelection(aSel);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = docEncoder->EncodeToString(textPlainBuf);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  nsAutoString textHTMLBuf;
  nsAutoString htmlParentsBuf;
  nsAutoString htmlInfoBuf;
  if (encodedTextHTML) {
    
    mimeType.AssignLiteral(kHTMLMime);
    rv = docEncoder->Init(domDoc, mimeType, aFlags);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = docEncoder->SetSelection(aSel);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = docEncoder->EncodeToStringWithContext(htmlParentsBuf, htmlInfoBuf,
                                               textHTMLBuf);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  nsCOMPtr<nsIClipboard> clipboard;
  if (doPutOnClipboard) {
    clipboard = do_GetService(kCClipboardCID, &rv);
    if (NS_FAILED(rv))
      return rv;
  }

  if ((doPutOnClipboard && clipboard) || aTransferable != nullptr) {
    
    nsCOMPtr<nsITransferable> trans = do_CreateInstance(kCTransferableCID);
    if (trans) {
      trans->Init(aDoc->GetLoadContext());
      if (encodedTextHTML) {
        
        
        nsCOMPtr<nsIFormatConverter> htmlConverter =
          do_CreateInstance(kHTMLConverterCID);
        trans->SetConverter(htmlConverter);

        if (!textHTMLBuf.IsEmpty()) {
          
          rv = AppendString(trans, textHTMLBuf, kHTMLMime);
          NS_ENSURE_SUCCESS(rv, rv);
        }

        
        
        
        rv = AppendString(trans, htmlParentsBuf, kHTMLContext);
        NS_ENSURE_SUCCESS(rv, rv);

        if (!htmlInfoBuf.IsEmpty()) {
          
          rv = AppendString(trans, htmlInfoBuf, kHTMLInfo);
          NS_ENSURE_SUCCESS(rv, rv);
        }

        if (!textPlainBuf.IsEmpty()) {
          
          
          
          
          rv = AppendString(trans, textPlainBuf, kUnicodeMime);
          NS_ENSURE_SUCCESS(rv, rv);
        }

        
        nsIURI *uri = aDoc->GetDocumentURI();
        if (uri) {
          nsAutoCString spec;
          uri->GetSpec(spec);
          if (!spec.IsEmpty()) {
            nsAutoString shortcut;
            AppendUTF8toUTF16(spec, shortcut);

            
            
            
            
            
            
            rv = AppendString(trans, shortcut, kURLPrivateMime);
            NS_ENSURE_SUCCESS(rv, rv);
          }
        }
      } else {
        if (!textPlainBuf.IsEmpty()) {
          
          rv = AppendString(trans, textPlainBuf, kUnicodeMime);
          NS_ENSURE_SUCCESS(rv, rv);
        }
      }

      if (doPutOnClipboard && clipboard) {
        bool actuallyPutOnClipboard = true;
        nsCopySupport::DoHooks(aDoc, trans, &actuallyPutOnClipboard);

        
        if (actuallyPutOnClipboard)
          clipboard->SetData(trans, nullptr, aClipboardID);
      }

      
      if (aTransferable != nullptr) {
        trans.swap(*aTransferable);
      }
    }
  }
  return rv;
}

nsresult
nsCopySupport::HTMLCopy(nsISelection* aSel, nsIDocument* aDoc,
                        int16_t aClipboardID)
{
  return SelectionCopyHelper(aSel, aDoc, true, aClipboardID,
                             nsIDocumentEncoder::SkipInvisibleContent,
                             nullptr);
}

nsresult
nsCopySupport::GetTransferableForSelection(nsISelection* aSel,
                                           nsIDocument* aDoc,
                                           nsITransferable** aTransferable)
{
  return SelectionCopyHelper(aSel, aDoc, false, 0,
                             nsIDocumentEncoder::SkipInvisibleContent,
                             aTransferable);
}

nsresult
nsCopySupport::GetTransferableForNode(nsINode* aNode,
                                      nsIDocument* aDoc,
                                      nsITransferable** aTransferable)
{
  nsCOMPtr<nsISelection> selection;
  
  nsresult rv = NS_NewDomSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIDOMNode> node = do_QueryInterface(aNode);
  NS_ENSURE_TRUE(node, NS_ERROR_FAILURE);
  nsRefPtr<nsRange> range = new nsRange(aNode);
  rv = range->SelectNode(node);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = selection->AddRange(range);
  NS_ENSURE_SUCCESS(rv, rv);
  
  uint32_t flags = 0;
  return SelectionCopyHelper(selection, aDoc, false, 0, flags,
                             aTransferable);
}

nsresult nsCopySupport::DoHooks(nsIDocument *aDoc, nsITransferable *aTrans,
                                bool *aDoPutOnClipboard)
{
  NS_ENSURE_ARG(aDoc);

  *aDoPutOnClipboard = true;

  nsCOMPtr<nsISupports> container = aDoc->GetContainer();
  nsCOMPtr<nsIClipboardDragDropHookList> hookObj = do_GetInterface(container);
  if (!hookObj) return NS_ERROR_FAILURE;

  nsCOMPtr<nsISimpleEnumerator> enumerator;
  hookObj->GetHookEnumerator(getter_AddRefs(enumerator));
  if (!enumerator) return NS_ERROR_FAILURE;

  
  

  nsCOMPtr<nsIClipboardDragDropHooks> override;
  nsCOMPtr<nsISupports> isupp;
  bool hasMoreHooks = false;
  nsresult rv = NS_OK;
  while (NS_SUCCEEDED(enumerator->HasMoreElements(&hasMoreHooks))
         && hasMoreHooks)
  {
    rv = enumerator->GetNext(getter_AddRefs(isupp));
    if (NS_FAILED(rv)) break;
    override = do_QueryInterface(isupp);
    if (override)
    {
#ifdef DEBUG
      nsresult hookResult =
#endif
      override->OnCopyOrDrag(nullptr, aTrans, aDoPutOnClipboard);
      NS_ASSERTION(NS_SUCCEEDED(hookResult), "OnCopyOrDrag hook failed");
      if (!*aDoPutOnClipboard)
        break;
    }
  }

  return rv;
}

nsresult
nsCopySupport::GetContents(const nsACString& aMimeType, uint32_t aFlags, nsISelection *aSel, nsIDocument *aDoc, nsAString& outdata)
{
  nsresult rv = NS_OK;
  
  nsCOMPtr<nsIDocumentEncoder> docEncoder;

  nsAutoCString encoderContractID(NS_DOC_ENCODER_CONTRACTID_BASE);
  encoderContractID.Append(aMimeType);
    
  docEncoder = do_CreateInstance(encoderContractID.get());
  NS_ENSURE_TRUE(docEncoder, NS_ERROR_FAILURE);

  uint32_t flags = aFlags | nsIDocumentEncoder::SkipInvisibleContent;
  
  if (aMimeType.EqualsLiteral("text/plain"))
    flags |= nsIDocumentEncoder::OutputPreformatted;

  NS_ConvertASCIItoUTF16 unicodeMimeType(aMimeType);

  nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(aDoc);
  NS_ASSERTION(domDoc, "Need a document");

  rv = docEncoder->Init(domDoc, unicodeMimeType, flags);
  if (NS_FAILED(rv)) return rv;
  
  if (aSel)
  {
    rv = docEncoder->SetSelection(aSel);
    if (NS_FAILED(rv)) return rv;
  } 
  
  
  return docEncoder->EncodeToString(outdata);
}


nsresult
nsCopySupport::ImageCopy(nsIImageLoadingContent* aImageElement,
                         nsILoadContext* aLoadContext,
                         int32_t aCopyFlags)
{
  nsresult rv;

  
  nsCOMPtr<nsITransferable> trans(do_CreateInstance(kCTransferableCID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);
  trans->Init(aLoadContext);

  if (aCopyFlags & nsIContentViewerEdit::COPY_IMAGE_TEXT) {
    
    nsCOMPtr<nsIURI> uri;
    rv = aImageElement->GetCurrentURI(getter_AddRefs(uri));
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(uri, NS_ERROR_FAILURE);

    nsAutoCString location;
    rv = uri->GetSpec(location);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = AppendString(trans, NS_ConvertUTF8toUTF16(location), kUnicodeMime);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (aCopyFlags & nsIContentViewerEdit::COPY_IMAGE_HTML) {
    
    nsCOMPtr<nsINode> node(do_QueryInterface(aImageElement, &rv));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = AppendDOMNode(trans, node);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (aCopyFlags & nsIContentViewerEdit::COPY_IMAGE_DATA) {
    
    nsCOMPtr<imgIContainer> image =
      nsContentUtils::GetImageFromContent(aImageElement);
    NS_ENSURE_TRUE(image, NS_ERROR_FAILURE);

    nsCOMPtr<nsISupportsInterfacePointer>
      imgPtr(do_CreateInstance(NS_SUPPORTS_INTERFACE_POINTER_CONTRACTID, &rv));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = imgPtr->SetData(image);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = trans->SetTransferData(kNativeImageMime, imgPtr,
                                sizeof(nsISupports*));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  nsCOMPtr<nsIClipboard> clipboard(do_GetService(kCClipboardCID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  
  bool selectionSupported;
  rv = clipboard->SupportsSelectionClipboard(&selectionSupported);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (selectionSupported) {
    rv = clipboard->SetData(trans, nullptr, nsIClipboard::kSelectionClipboard);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return clipboard->SetData(trans, nullptr, nsIClipboard::kGlobalClipboard);
}

static nsresult AppendString(nsITransferable *aTransferable,
                             const nsAString& aString,
                             const char* aFlavor)
{
  nsresult rv;

  nsCOMPtr<nsISupportsString>
    data(do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = data->SetData(aString);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aTransferable->AddDataFlavor(aFlavor);
  NS_ENSURE_SUCCESS(rv, rv);

  return aTransferable->SetTransferData(aFlavor, data,
                                        aString.Length() * sizeof(char16_t));
}

static nsresult AppendDOMNode(nsITransferable *aTransferable,
                              nsINode *aDOMNode)
{
  nsresult rv;

  
  nsCOMPtr<nsIDocumentEncoder>
    docEncoder(do_CreateInstance(NS_HTMLCOPY_ENCODER_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIDocument> document = aDOMNode->OwnerDoc();

  
  
  
  nsCOMPtr<nsIHTMLDocument> htmlDoc = do_QueryInterface(document, &rv);
  NS_ENSURE_SUCCESS(rv, NS_OK);

  NS_ENSURE_TRUE(document->IsHTML(), NS_OK);

  
  rv = docEncoder->NativeInit(document, NS_LITERAL_STRING(kHTMLMime),
                              nsIDocumentEncoder::OutputAbsoluteLinks |
                              nsIDocumentEncoder::OutputEncodeW3CEntities);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = docEncoder->SetNativeNode(aDOMNode);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsAutoString html, context, info;
  rv = docEncoder->EncodeToStringWithContext(context, info, html);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (!html.IsEmpty()) {
    rv = AppendString(aTransferable, html, kHTMLMime);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (!info.IsEmpty()) {
    rv = AppendString(aTransferable, info, kHTMLInfo);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  return AppendString(aTransferable, context, kHTMLContext);
}

nsIContent*
nsCopySupport::GetSelectionForCopy(nsIDocument* aDocument, nsISelection** aSelection)
{
  *aSelection = nullptr;

  nsIPresShell* presShell = aDocument->GetShell();
  if (!presShell)
    return nullptr;

  
  nsCOMPtr<nsPIDOMWindow> focusedWindow;
  nsIContent* content =
    nsFocusManager::GetFocusedDescendant(aDocument->GetWindow(), false,
                                         getter_AddRefs(focusedWindow));
  if (content) {
    nsIFrame* frame = content->GetPrimaryFrame();
    if (frame) {
      nsCOMPtr<nsISelectionController> selCon;
      frame->GetSelectionController(presShell->GetPresContext(), getter_AddRefs(selCon));
      if (selCon) {
        selCon->GetSelection(nsISelectionController::SELECTION_NORMAL, aSelection);
        return content;
      }
    }
  }

  
  NS_IF_ADDREF(*aSelection = presShell->GetCurrentSelection(nsISelectionController::SELECTION_NORMAL));
  return nullptr;
}

bool
nsCopySupport::CanCopy(nsIDocument* aDocument)
{
  if (!aDocument)
    return false;

  nsCOMPtr<nsISelection> sel;
  GetSelectionForCopy(aDocument, getter_AddRefs(sel));
  NS_ENSURE_TRUE(sel, false);

  bool isCollapsed;
  sel->GetIsCollapsed(&isCollapsed);
  return !isCollapsed;
}

bool
nsCopySupport::FireClipboardEvent(int32_t aType, int32_t aClipboardType, nsIPresShell* aPresShell, nsISelection* aSelection)
{
  NS_ASSERTION(aType == NS_CUT || aType == NS_COPY || aType == NS_PASTE,
               "Invalid clipboard event type");

  nsCOMPtr<nsIPresShell> presShell = aPresShell;
  if (!presShell)
    return false;

  nsCOMPtr<nsIDocument> doc = presShell->GetDocument();
  if (!doc)
    return false;

  nsCOMPtr<nsPIDOMWindow> piWindow = doc->GetWindow();
  if (!piWindow)
    return false;

  
  nsCOMPtr<nsIContent> content;
  nsCOMPtr<nsISelection> sel = aSelection;
  if (!sel)
    content = GetSelectionForCopy(doc, getter_AddRefs(sel));

  
  nsresult rv;
  if (sel) {
    
    if (aType == NS_CUT || aType == NS_COPY) {
      bool isCollapsed;
      sel->GetIsCollapsed(&isCollapsed);
      if (isCollapsed)
        return false;
    }

    nsCOMPtr<nsIDOMRange> range;
    rv = sel->GetRangeAt(0, getter_AddRefs(range));
    if (NS_SUCCEEDED(rv) && range) {
      nsCOMPtr<nsIDOMNode> startContainer;
      range->GetStartContainer(getter_AddRefs(startContainer));
      if (startContainer)
        content = do_QueryInterface(startContainer);
    }
  }

  
  if (!content) {
    content = doc->GetRootElement();
    if (!content)
      return false;
  }

  
  if (!nsContentUtils::IsSafeToRunScript()) {
    nsContentUtils::WarnScriptWasIgnored(doc);
    return false;
  }

  nsCOMPtr<nsIDocShell> docShell = piWindow->GetDocShell();
  const bool chromeShell =
    docShell && docShell->ItemType() == nsIDocShellTreeItem::typeChrome;

  
  bool doDefault = true;
  nsRefPtr<DataTransfer> clipboardData;
  if (chromeShell || Preferences::GetBool("dom.event.clipboardevents.enabled", true)) {
    clipboardData =
      new DataTransfer(piWindow, aType, aType == NS_PASTE, aClipboardType);

    nsEventStatus status = nsEventStatus_eIgnore;
    InternalClipboardEvent evt(true, aType);
    evt.clipboardData = clipboardData;
    EventDispatcher::Dispatch(content, presShell->GetPresContext(), &evt,
                              nullptr, &status);
    
    doDefault = (status != nsEventStatus_eConsumeNoDefault);
  }
  
  
  
  
  if (aType == NS_PASTE) {
    
    
    if (clipboardData) {
      clipboardData->ClearAll();
      clipboardData->SetReadOnly();
    }

    return doDefault;
  }

  
  
  presShell->FlushPendingNotifications(Flush_Frames);
  if (presShell->IsDestroying())
    return false;

  
  
  uint32_t count = 0;
  if (doDefault) {
    
    bool isCollapsed;
    sel->GetIsCollapsed(&isCollapsed);
    if (isCollapsed) {
      return false;
    }
    
    rv = HTMLCopy(sel, doc, aClipboardType);
    if (NS_FAILED(rv)) {
      return false;
    }
  } else if (clipboardData) {
    
    clipboardData->GetMozItemCount(&count);
    if (count) {
      nsCOMPtr<nsIClipboard> clipboard(do_GetService("@mozilla.org/widget/clipboard;1"));
      NS_ENSURE_TRUE(clipboard, false);

      nsCOMPtr<nsITransferable> transferable =
        clipboardData->GetTransferable(0, doc->GetLoadContext());

      NS_ENSURE_TRUE(transferable, false);

      
      rv = clipboard->SetData(transferable, nullptr, aClipboardType);
      if (NS_FAILED(rv)) {
        return false;
      }
    }
  }

  
  
  if (doDefault || count) {
    piWindow->UpdateCommands(NS_LITERAL_STRING("clipboard"), nullptr, 0);
  }

  return doDefault;
}
