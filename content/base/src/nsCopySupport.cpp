







































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
#include "nsEventDispatcher.h"

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
#include "nsGUIEvent.h"
#include "nsIFrame.h"


#include "nsIImageLoadingContent.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsContentUtils.h"
#include "nsContentCID.h"

#include "mozilla/dom/Element.h"

nsresult NS_NewDomSelection(nsISelection **aDomSelection);

static NS_DEFINE_CID(kCClipboardCID,           NS_CLIPBOARD_CID);
static NS_DEFINE_CID(kCTransferableCID,        NS_TRANSFERABLE_CID);
static NS_DEFINE_CID(kHTMLConverterCID,        NS_HTMLFORMATCONVERTER_CID);


#define kHTMLContext   "text/_moz_htmlcontext"
#define kHTMLInfo      "text/_moz_htmlinfo"


static nsresult AppendString(nsITransferable *aTransferable,
                             const nsAString& aString,
                             const char* aFlavor);


static nsresult AppendDOMNode(nsITransferable *aTransferable,
                              nsIDOMNode *aDOMNode);



static nsresult
SelectionCopyHelper(nsISelection *aSel, nsIDocument *aDoc,
                    bool doPutOnClipboard, PRInt16 aClipboardID,
                    PRUint32 aFlags, nsITransferable ** aTransferable)
{
  
  if (aTransferable) {
    *aTransferable = nsnull;
  }

  nsresult rv = NS_OK;
  
  bool bIsPlainTextContext = false;

  rv = nsCopySupport::IsPlainTextContext(aSel, aDoc, &bIsPlainTextContext);
  if (NS_FAILED(rv)) 
    return rv;

  bool bIsHTMLCopy = !bIsPlainTextContext;
  nsAutoString mimeType;

  nsCOMPtr<nsIDocumentEncoder> docEncoder;

  docEncoder = do_CreateInstance(NS_HTMLCOPY_ENCODER_CONTRACTID);
  NS_ENSURE_TRUE(docEncoder, NS_ERROR_FAILURE);

  
  
  
  
  
  
  
  mimeType.AssignLiteral(kUnicodeMime);
  
  
  
  
  PRUint32 flags = aFlags | nsIDocumentEncoder::OutputPreformatted
                          | nsIDocumentEncoder::OutputRaw;

  nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(aDoc);
  NS_ASSERTION(domDoc, "Need a document");

  rv = docEncoder->Init(domDoc, mimeType, flags);
  if (NS_FAILED(rv)) 
    return rv;

  rv = docEncoder->SetSelection(aSel);
  if (NS_FAILED(rv)) 
    return rv;

  nsAutoString buffer, parents, info, textBuffer, plaintextBuffer;

  rv = docEncoder->EncodeToString(textBuffer);
  if (NS_FAILED(rv)) 
    return rv;

  nsCOMPtr<nsIFormatConverter> htmlConverter;

  
  if (bIsHTMLCopy) {

    
    htmlConverter = do_CreateInstance(kHTMLConverterCID);
    NS_ENSURE_TRUE(htmlConverter, NS_ERROR_FAILURE);

    nsCOMPtr<nsISupportsString> plainHTML = do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID);
    NS_ENSURE_TRUE(plainHTML, NS_ERROR_FAILURE);
    plainHTML->SetData(textBuffer);

    nsCOMPtr<nsISupportsString> ConvertedData;
    PRUint32 ConvertedLen;
    rv = htmlConverter->Convert(kHTMLMime, plainHTML, textBuffer.Length() * 2, kUnicodeMime, getter_AddRefs(ConvertedData), &ConvertedLen);
    NS_ENSURE_SUCCESS(rv, rv);

    ConvertedData->GetData(plaintextBuffer);

    mimeType.AssignLiteral(kHTMLMime);

    flags = aFlags;

    rv = docEncoder->Init(domDoc, mimeType, flags);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = docEncoder->SetSelection(aSel);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = docEncoder->EncodeToStringWithContext(parents, info, buffer);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  
  nsCOMPtr<nsIClipboard> clipboard;
  if (doPutOnClipboard) {
    clipboard = do_GetService(kCClipboardCID, &rv);
    if (NS_FAILED(rv))
      return rv;
  }

  if ((doPutOnClipboard && clipboard) || aTransferable != nsnull) {
    
    nsCOMPtr<nsITransferable> trans = do_CreateInstance(kCTransferableCID);
    if (trans) {
      if (bIsHTMLCopy) {
        
        trans->SetConverter(htmlConverter);

        if (!buffer.IsEmpty()) {
          
          rv = AppendString(trans, buffer, kHTMLMime);
          NS_ENSURE_SUCCESS(rv, rv);
        }

        
        
        
        rv = AppendString(trans, parents, kHTMLContext);
        NS_ENSURE_SUCCESS(rv, rv);

        if (!info.IsEmpty()) {
          
          rv = AppendString(trans, info, kHTMLInfo);
          NS_ENSURE_SUCCESS(rv, rv);
        }

        if (!plaintextBuffer.IsEmpty()) {
          
          
          
          
          rv = AppendString(trans, plaintextBuffer, kUnicodeMime);
          NS_ENSURE_SUCCESS(rv, rv);
        }

        
        nsIURI *uri = aDoc->GetDocumentURI();
        if (uri) {
          nsCAutoString spec;
          uri->GetSpec(spec);
          if (!spec.IsEmpty()) {
            nsAutoString shortcut;
            AppendUTF8toUTF16(spec, shortcut);

            
            
            
            
            
            
            rv = AppendString(trans, shortcut, kURLPrivateMime);
            NS_ENSURE_SUCCESS(rv, rv);
          }
        }
      } else {
        if (!textBuffer.IsEmpty()) {
          
          rv = AppendString(trans, textBuffer, kUnicodeMime);
          NS_ENSURE_SUCCESS(rv, rv);
        }
      }

      if (doPutOnClipboard && clipboard) {
        bool actuallyPutOnClipboard = true;
        nsCopySupport::DoHooks(aDoc, trans, &actuallyPutOnClipboard);

        
        if (actuallyPutOnClipboard)
          clipboard->SetData(trans, nsnull, aClipboardID);
      }

      
      if (aTransferable != nsnull) {
        trans.swap(*aTransferable);
      }
    }
  }
  return rv;
}

nsresult
nsCopySupport::HTMLCopy(nsISelection* aSel, nsIDocument* aDoc,
                        PRInt16 aClipboardID)
{
  return SelectionCopyHelper(aSel, aDoc, PR_TRUE, aClipboardID,
                             nsIDocumentEncoder::SkipInvisibleContent,
                             nsnull);
}

nsresult
nsCopySupport::GetTransferableForSelection(nsISelection* aSel,
                                           nsIDocument* aDoc,
                                           nsITransferable** aTransferable)
{
  return SelectionCopyHelper(aSel, aDoc, PR_FALSE, 0,
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
  nsCOMPtr<nsIDOMRange> range;
  rv = NS_NewRange(getter_AddRefs(range));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIDOMNode> node = do_QueryInterface(aNode);
  NS_ENSURE_TRUE(node, NS_ERROR_FAILURE);
  rv = range->SelectNode(node);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = selection->AddRange(range);
  NS_ENSURE_SUCCESS(rv, rv);
  
  PRUint32 flags = 0;
  return SelectionCopyHelper(selection, aDoc, PR_FALSE, 0, flags,
                             aTransferable);
}

nsresult nsCopySupport::DoHooks(nsIDocument *aDoc, nsITransferable *aTrans,
                                bool *aDoPutOnClipboard)
{
  NS_ENSURE_ARG(aDoc);

  *aDoPutOnClipboard = PR_TRUE;

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
      override->OnCopyOrDrag(nsnull, aTrans, aDoPutOnClipboard);
      NS_ASSERTION(NS_SUCCEEDED(hookResult), "OnCopyOrDrag hook failed");
      if (!*aDoPutOnClipboard)
        break;
    }
  }

  return rv;
}

nsresult nsCopySupport::IsPlainTextContext(nsISelection *aSel, nsIDocument *aDoc, bool *aIsPlainTextContext)
{
  nsresult rv;

  if (!aSel || !aIsPlainTextContext)
    return NS_ERROR_NULL_POINTER;

  *aIsPlainTextContext = PR_FALSE;
  
  nsCOMPtr<nsIDOMRange> range;
  nsCOMPtr<nsIDOMNode> commonParent;
  PRInt32 count = 0;

  rv = aSel->GetRangeCount(&count);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (!count)
    return NS_ERROR_FAILURE;
  
  
  
  
  rv = aSel->GetRangeAt(0, getter_AddRefs(range));
  NS_ENSURE_SUCCESS(rv, rv);
  if (!range)
    return NS_ERROR_NULL_POINTER;
  range->GetCommonAncestorContainer(getter_AddRefs(commonParent));

  for (nsCOMPtr<nsIContent> selContent(do_QueryInterface(commonParent));
       selContent;
       selContent = selContent->GetParent())
  {
    

    if (!selContent->IsHTML()) {
      continue;
    }

    nsIAtom *atom = selContent->Tag();

    if (atom == nsGkAtoms::input ||
        atom == nsGkAtoms::textarea)
    {
      *aIsPlainTextContext = PR_TRUE;
      break;
    }

    if (atom == nsGkAtoms::body)
    {
      
      
      
      nsCOMPtr<nsIDOMElement> bodyElem = do_QueryInterface(selContent);
      nsAutoString wsVal;
      rv = bodyElem->GetAttribute(NS_LITERAL_STRING("style"), wsVal);
      if (NS_SUCCEEDED(rv) && (kNotFound != wsVal.Find(NS_LITERAL_STRING("pre-wrap"))))
      {
        *aIsPlainTextContext = PR_TRUE;
        break;
      }
    }
  }
  
  
  
  
  
  nsCOMPtr<nsIHTMLDocument> htmlDoc = do_QueryInterface(aDoc);
  if (!(htmlDoc && aDoc->IsHTML()))
    *aIsPlainTextContext = PR_TRUE;

  return NS_OK;
}

nsresult
nsCopySupport::GetContents(const nsACString& aMimeType, PRUint32 aFlags, nsISelection *aSel, nsIDocument *aDoc, nsAString& outdata)
{
  nsresult rv = NS_OK;
  
  nsCOMPtr<nsIDocumentEncoder> docEncoder;

  nsCAutoString encoderContractID(NS_DOC_ENCODER_CONTRACTID_BASE);
  encoderContractID.Append(aMimeType);
    
  docEncoder = do_CreateInstance(encoderContractID.get());
  NS_ENSURE_TRUE(docEncoder, NS_ERROR_FAILURE);

  PRUint32 flags = aFlags | nsIDocumentEncoder::SkipInvisibleContent;
  
  if (aMimeType.Equals("text/plain"))
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
                         PRInt32 aCopyFlags)
{
  nsresult rv;

  
  nsCOMPtr<nsITransferable> trans(do_CreateInstance(kCTransferableCID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  if (aCopyFlags & nsIContentViewerEdit::COPY_IMAGE_TEXT) {
    
    nsCOMPtr<nsIURI> uri;
    rv = aImageElement->GetCurrentURI(getter_AddRefs(uri));
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(uri, NS_ERROR_FAILURE);

    nsCAutoString location;
    rv = uri->GetSpec(location);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = AppendString(trans, NS_ConvertUTF8toUTF16(location), kUnicodeMime);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (aCopyFlags & nsIContentViewerEdit::COPY_IMAGE_HTML) {
    
    nsCOMPtr<nsIDOMNode> node(do_QueryInterface(aImageElement, &rv));
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
    rv = clipboard->SetData(trans, nsnull, nsIClipboard::kSelectionClipboard);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return clipboard->SetData(trans, nsnull, nsIClipboard::kGlobalClipboard);
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
                                        aString.Length() * sizeof(PRUnichar));
}

static nsresult AppendDOMNode(nsITransferable *aTransferable,
                              nsIDOMNode *aDOMNode)
{
  nsresult rv;
  
  
  nsCOMPtr<nsIDocumentEncoder>
    docEncoder(do_CreateInstance(NS_HTMLCOPY_ENCODER_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIDOMDocument> domDocument;
  rv = aDOMNode->GetOwnerDocument(getter_AddRefs(domDocument));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIDocument> document(do_QueryInterface(domDocument, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  nsCOMPtr<nsIHTMLDocument> htmlDoc = do_QueryInterface(domDocument, &rv);
  NS_ENSURE_SUCCESS(rv, NS_OK);

  NS_ENSURE_TRUE(document->IsHTML(), NS_OK);

  
  rv = docEncoder->Init(domDocument, NS_LITERAL_STRING(kHTMLMime),
                        nsIDocumentEncoder::OutputAbsoluteLinks |
                        nsIDocumentEncoder::OutputEncodeW3CEntities);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = docEncoder->SetNode(aDOMNode);
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
  *aSelection = nsnull;

  nsIPresShell* presShell = aDocument->GetShell();
  if (!presShell)
    return nsnull;

  
  nsCOMPtr<nsPIDOMWindow> focusedWindow;
  nsIContent* content =
    nsFocusManager::GetFocusedDescendant(aDocument->GetWindow(), PR_FALSE,
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
  return nsnull;
}

bool
nsCopySupport::CanCopy(nsIDocument* aDocument)
{
  if (!aDocument)
    return PR_FALSE;

  nsCOMPtr<nsISelection> sel;
  GetSelectionForCopy(aDocument, getter_AddRefs(sel));
  NS_ENSURE_TRUE(sel, PR_FALSE);

  bool isCollapsed;
  sel->GetIsCollapsed(&isCollapsed);
  return !isCollapsed;
}

bool
nsCopySupport::FireClipboardEvent(PRInt32 aType, nsIPresShell* aPresShell, nsISelection* aSelection)
{
  NS_ASSERTION(aType == NS_CUT || aType == NS_COPY || aType == NS_PASTE,
               "Invalid clipboard event type");

  nsCOMPtr<nsIPresShell> presShell = aPresShell;
  if (!presShell)
    return PR_FALSE;

  nsCOMPtr<nsIDocument> doc = presShell->GetDocument();
  if (!doc)
    return PR_FALSE;

  nsCOMPtr<nsPIDOMWindow> piWindow = doc->GetWindow();
  if (!piWindow)
    return PR_FALSE;

  
  nsCOMPtr<nsIContent> content;
  nsCOMPtr<nsISelection> sel = aSelection;
  if (!sel)
    content = GetSelectionForCopy(doc, getter_AddRefs(sel));

  
  if (sel) {
    
    if (aType == NS_CUT || aType == NS_COPY) {
      bool isCollapsed;
      sel->GetIsCollapsed(&isCollapsed);
      if (isCollapsed)
        return PR_FALSE;
    }

    nsCOMPtr<nsIDOMRange> range;
    nsresult rv = sel->GetRangeAt(0, getter_AddRefs(range));
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
      return PR_FALSE;
  }

  
  if (!nsContentUtils::IsSafeToRunScript())
    return PR_FALSE;

  
  nsEventStatus status = nsEventStatus_eIgnore;
  nsEvent evt(PR_TRUE, aType);
  nsEventDispatcher::Dispatch(content, presShell->GetPresContext(), &evt, nsnull,
                              &status);
  
  if (status == nsEventStatus_eConsumeNoDefault)
    return PR_FALSE;

  if (presShell->IsDestroying())
    return PR_FALSE;

  
  
  
  if (aType == NS_PASTE)
    return PR_TRUE;

  
  
  presShell->FlushPendingNotifications(Flush_Frames);
  if (presShell->IsDestroying())
    return PR_FALSE;

  
  if (NS_FAILED(nsCopySupport::HTMLCopy(sel, doc, nsIClipboard::kGlobalClipboard)))
    return PR_FALSE;

  
  
  piWindow->UpdateCommands(NS_LITERAL_STRING("clipboard"));

  return PR_TRUE;
}
