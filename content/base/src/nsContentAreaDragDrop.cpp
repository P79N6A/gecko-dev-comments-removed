





































#include "nsReadableUtils.h"


#include "nsContentAreaDragDrop.h"


#include "nsString.h"


#include "nsIDOMNSUIEvent.h"
#include "nsIDOMUIEvent.h"
#include "nsISelection.h"
#include "nsIDOMNode.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMEventReceiver.h"
#include "nsIDOMEvent.h"
#include "nsIDOMNSEvent.h"
#include "nsIDOMMouseEvent.h"
#include "nsIDOMAbstractView.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDocumentRange.h"
#include "nsIDOMRange.h"
#include "nsIDocumentEncoder.h"
#include "nsIFormControl.h"
#include "nsISelectionPrivate.h"
#include "nsIDOMHTMLAreaElement.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsITransferable.h"
#include "nsIDragService.h"
#include "nsIDragSession.h"
#include "nsComponentManagerUtils.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsServiceManagerUtils.h"
#include "nsNetUtil.h"
#include "nsIFile.h"
#include "nsIWebNavigation.h"
#include "nsIClipboardDragDropHooks.h"
#include "nsIClipboardDragDropHookList.h"
#include "nsIDocShell.h"
#include "nsIContent.h"
#include "nsIImageLoadingContent.h"
#include "nsINameSpaceManager.h"
#include "nsUnicharUtils.h"
#include "nsIURL.h"
#include "nsIImage.h"
#include "nsIDocument.h"
#include "nsIScriptSecurityManager.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsIDocShellTreeItem.h"
#include "nsIFrame.h"
#include "nsRange.h"
#include "nsIWebBrowserPersist.h"
#include "nsEscape.h"
#include "nsContentUtils.h"
#include "nsIMIMEService.h"
#include "imgIRequest.h"
#include "nsContentCID.h"
#include "nsISelectionController.h"
#include "nsFrameSelection.h"


#define kHTMLContext   "text/_moz_htmlcontext"
#define kHTMLInfo      "text/_moz_htmlinfo"


NS_IMPL_ADDREF(nsContentAreaDragDrop)
NS_IMPL_RELEASE(nsContentAreaDragDrop)

NS_INTERFACE_MAP_BEGIN(nsContentAreaDragDrop)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMDragListener)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsIDOMEventListener, nsIDOMDragListener)
    NS_INTERFACE_MAP_ENTRY(nsIDOMDragListener)
    NS_INTERFACE_MAP_ENTRY(nsIFlavorDataProvider)
    NS_INTERFACE_MAP_ENTRY(nsIDragDropHandler)
NS_INTERFACE_MAP_END


class nsTransferableFactory
{
public:
  nsTransferableFactory(nsIDOMEvent* inMouseEvent,
                        nsIFlavorDataProvider *inFlavorDataProvider);
  nsresult Produce(PRBool *aDragSelection, nsITransferable** outTrans);

private:
  nsresult ConvertStringsToTransferable(nsITransferable** outTrans);
  static nsresult GetDraggableSelectionData(nsISelection* inSelection,
                                            nsIDOMNode* inRealTargetNode,
                                            nsIDOMNode **outImageOrLinkNode,
                                            PRBool* outDragSelectedText);
  static already_AddRefed<nsIDOMNode> FindParentLinkNode(nsIDOMNode* inNode);
  static void GetAnchorURL(nsIDOMNode* inNode, nsAString& outURL);
  static void GetNodeString(nsIDOMNode* inNode, nsAString & outNodeString);
  static void CreateLinkText(const nsAString& inURL, const nsAString & inText,
                              nsAString& outLinkText);
  static void GetSelectedLink(nsISelection* inSelection,
                              nsIDOMNode **outLinkNode);

  enum serializationMode {serializeAsText, serializeAsHTML};
  
  static nsresult SerializeNodeOrSelection(serializationMode inMode,
                                           PRUint32 inFlags,
                                           nsIDOMWindow* inWindow,
                                           nsIDOMNode* inNode,
                                           nsAString& outResultString,
                                           nsAString& outHTMLContext,
                                           nsAString& outHTMLInfo);

  PRBool mInstanceAlreadyUsed;

  nsCOMPtr<nsIDOMEvent> mMouseEvent;
  nsCOMPtr<nsIFlavorDataProvider> mFlavorDataProvider;

  nsString mUrlString;
  nsString mImageSourceString;
  nsString mImageDestFileName;
  nsString mTitleString;
  
  nsString mHtmlString;
  nsString mContextString;
  nsString mInfoString;

  PRBool mIsAnchor;
  nsCOMPtr<nsIImage> mImage;
};





nsContentAreaDragDrop::nsContentAreaDragDrop()
  : mListenerInstalled(PR_FALSE), mNavigator(nsnull)
{
} 





nsContentAreaDragDrop::~nsContentAreaDragDrop()
{
  RemoveDragListener();

} 


NS_IMETHODIMP
nsContentAreaDragDrop::HookupTo(nsIDOMEventTarget *inAttachPoint,
                                nsIWebNavigation* inNavigator)
{
  NS_ASSERTION(inAttachPoint, "Can't hookup Drag Listeners to NULL receiver");
  mEventReceiver = do_QueryInterface(inAttachPoint);
  NS_ASSERTION(mEventReceiver,
               "Target doesn't implement nsIDOMEventReceiver as needed");
  mNavigator = inNavigator;

  return AddDragListener();
}


NS_IMETHODIMP
nsContentAreaDragDrop::Detach()
{
  return RemoveDragListener();
}







nsresult
nsContentAreaDragDrop::AddDragListener()
{
  nsresult rv = NS_ERROR_FAILURE;

  if ( mEventReceiver ) {
    nsIDOMDragListener *pListener = NS_STATIC_CAST(nsIDOMDragListener *, this);
    rv = mEventReceiver->AddEventListenerByIID(pListener,
                                               NS_GET_IID(nsIDOMDragListener));
    if (NS_SUCCEEDED(rv))
      mListenerInstalled = PR_TRUE;
  }

  return rv;
}







nsresult
nsContentAreaDragDrop::RemoveDragListener()
{
  nsresult rv = NS_ERROR_FAILURE;

  if (mEventReceiver) {
    nsIDOMDragListener *pListener = NS_STATIC_CAST(nsIDOMDragListener *, this);
    rv =
      mEventReceiver->RemoveEventListenerByIID(pListener,
                                               NS_GET_IID(nsIDOMDragListener));
    if (NS_SUCCEEDED(rv))
      mListenerInstalled = PR_FALSE;
    mEventReceiver = nsnull;
  }

  return rv;
}









NS_IMETHODIMP
nsContentAreaDragDrop::DragEnter(nsIDOMEvent* aMouseEvent)
{
  
  return NS_OK;
}












NS_IMETHODIMP
nsContentAreaDragDrop::DragOver(nsIDOMEvent* inEvent)
{
  
  PRBool preventDefault = PR_TRUE;
  nsCOMPtr<nsIDOMNSUIEvent> nsuiEvent(do_QueryInterface(inEvent));
  if ( nsuiEvent )
    nsuiEvent->GetPreventDefault(&preventDefault);
  if ( preventDefault )
    return NS_OK;

  
  
  
  
  nsCOMPtr<nsIDragService> dragService =
    do_GetService("@mozilla.org/widget/dragservice;1");
  if (!dragService)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDragSession> session;
  dragService->GetCurrentSession(getter_AddRefs(session));

  if (session) {
    
    
    
    PRBool dropAllowed = PR_TRUE;
    nsCOMPtr<nsISimpleEnumerator> enumerator;
    GetHookEnumeratorFromEvent(inEvent, getter_AddRefs(enumerator));

    if (enumerator) {
      PRBool hasMoreHooks = PR_FALSE;
      while (NS_SUCCEEDED(enumerator->HasMoreElements(&hasMoreHooks))
             && hasMoreHooks) {
        nsCOMPtr<nsISupports> isupp;
        if (NS_FAILED(enumerator->GetNext(getter_AddRefs(isupp)))) {
          break;
        }

        nsCOMPtr<nsIClipboardDragDropHooks> override =
          do_QueryInterface(isupp);

        if (override) {
#ifdef DEBUG
          nsresult hookResult =
#endif
          override->AllowDrop(inEvent, session, &dropAllowed);
          NS_ASSERTION(NS_SUCCEEDED(hookResult), "hook failure in AllowDrop");

          if (!dropAllowed) {
            break;
          }
        }
      }
    }

    nsCOMPtr<nsIDOMDocument> sourceDoc;
    session->GetSourceDocument(getter_AddRefs(sourceDoc));
    nsCOMPtr<nsIDOMDocument> eventDoc;
    GetEventDocument(inEvent, getter_AddRefs(eventDoc));

    if (sourceDoc == eventDoc) {  
      dropAllowed = PR_FALSE;
    } else if (sourceDoc && eventDoc) {
      
      
      nsCOMPtr<nsIDocument> sourceDocument(do_QueryInterface(sourceDoc));
      nsCOMPtr<nsIDocument> eventDocument(do_QueryInterface(eventDoc));
      NS_ASSERTION(sourceDocument, "Confused document object");
      NS_ASSERTION(eventDocument, "Confused document object");

      nsPIDOMWindow* sourceWindow = sourceDocument->GetWindow();
      nsPIDOMWindow* eventWindow = eventDocument->GetWindow();

      if (sourceWindow && eventWindow) {
        nsCOMPtr<nsIDocShellTreeItem> sourceShell =
          do_QueryInterface(sourceWindow->GetDocShell());
        nsCOMPtr<nsIDocShellTreeItem> eventShell =
          do_QueryInterface(eventWindow->GetDocShell());

        if (sourceShell && eventShell) {
          
          
          
          nsCOMPtr<nsIDocShellTreeItem> sourceRoot;
          nsCOMPtr<nsIDocShellTreeItem> eventRoot;
          sourceShell->GetSameTypeRootTreeItem(getter_AddRefs(sourceRoot));
          eventShell->GetSameTypeRootTreeItem(getter_AddRefs(eventRoot));

          if (sourceRoot && sourceRoot == eventRoot) {
            dropAllowed = PR_FALSE;
          }
        }
      }
    }

    session->SetCanDrop(dropAllowed);
  }

  return NS_OK;
}








NS_IMETHODIMP
nsContentAreaDragDrop::DragExit(nsIDOMEvent* aMouseEvent)
{
  
  return NS_OK;
}









void
nsContentAreaDragDrop::ExtractURLFromData(const nsACString & inFlavor,
                                          nsISupports* inDataWrapper,
                                          PRUint32 inDataLen,
                                          nsAString & outURL)
{
  if (!inDataWrapper) {
    return;
  }

  outURL.Truncate();

  if (inFlavor.Equals(kUnicodeMime)  || inFlavor.Equals(kURLDataMime)) {
    
    
    nsCOMPtr<nsISupportsString> stringData(do_QueryInterface(inDataWrapper));
    if (stringData) {
      stringData->GetData(outURL);
    }
  }
  else if (inFlavor.Equals(kURLMime)) {
    
    
    nsCOMPtr<nsISupportsString> stringData(do_QueryInterface(inDataWrapper));

    if (stringData) {
      nsAutoString data;
      stringData->GetData(data);
      PRInt32 separator = data.FindChar('\n');

      if (separator >= 0)
        outURL = Substring(data, 0, separator);
      else
        outURL = data;
    }
  }
  else if (inFlavor.Equals(kFileMime)) {
    
    
    nsCOMPtr<nsIFile> file(do_QueryInterface(inDataWrapper));
    if (file) {
      nsCAutoString url;
      NS_GetURLSpecFromFile(file, url);
      CopyUTF8toUTF16(url, outURL);
    }
  }
}









NS_IMETHODIMP
nsContentAreaDragDrop::DragDrop(nsIDOMEvent* inMouseEvent)
{
  
  
  if (!mNavigator) {
    return NS_OK;
  }

  
  PRBool preventDefault = PR_TRUE;
  nsCOMPtr<nsIDOMNSUIEvent> nsuiEvent(do_QueryInterface(inMouseEvent));
  if (nsuiEvent) {
    nsuiEvent->GetPreventDefault(&preventDefault);
  }

  if (preventDefault) {
    return NS_OK;
  }

  
  
  
  nsCOMPtr<nsIDragService> dragService =
    do_GetService("@mozilla.org/widget/dragservice;1");
  if (!dragService) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIDragSession> session;
  dragService->GetCurrentSession(getter_AddRefs(session));
  if (!session) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsITransferable> trans =
    do_CreateInstance("@mozilla.org/widget/transferable;1");
  if (!trans) {
    return NS_ERROR_FAILURE;
  }

  
  trans->AddDataFlavor(kURLDataMime);
  trans->AddDataFlavor(kURLMime);
  trans->AddDataFlavor(kFileMime);
  trans->AddDataFlavor(kUnicodeMime);

  
  nsresult rv = session->GetData(trans, 0);

  if (NS_SUCCEEDED(rv)) {
    
    
    nsCOMPtr<nsISimpleEnumerator> enumerator;
    GetHookEnumeratorFromEvent(inMouseEvent, getter_AddRefs(enumerator));

    if (enumerator) {
      PRBool actionCanceled = PR_TRUE;
      PRBool hasMoreHooks = PR_FALSE;
      while (NS_SUCCEEDED(enumerator->HasMoreElements(&hasMoreHooks))
             && hasMoreHooks) {
        nsCOMPtr<nsISupports> isupp;
        if (NS_FAILED(enumerator->GetNext(getter_AddRefs(isupp))))
          break;
        nsCOMPtr<nsIClipboardDragDropHooks> override =
          do_QueryInterface(isupp);

        if (override) {
#ifdef DEBUG
          nsresult hookResult =
#endif
          override->OnPasteOrDrop(inMouseEvent, trans, &actionCanceled);
          NS_ASSERTION(NS_SUCCEEDED(hookResult),
                       "hook failure in OnPasteOrDrop");
          if (!actionCanceled)
            return NS_OK;
        }
      }
    }

    nsXPIDLCString flavor;
    nsCOMPtr<nsISupports> dataWrapper;
    PRUint32 dataLen = 0;
    rv = trans->GetAnyTransferData(getter_Copies(flavor),
                                   getter_AddRefs(dataWrapper), &dataLen);
    if (NS_SUCCEEDED(rv) && dataLen > 0) {
      
      nsAutoString url;
      ExtractURLFromData(flavor, dataWrapper, dataLen, url);
      NS_ASSERTION(!url.IsEmpty(), "Didn't get anything we can use as a url");

      
      if (url.IsEmpty() || url.FindChar(' ') >= 0)
        return NS_OK;

      nsCOMPtr<nsIURI> uri;
      NS_NewURI(getter_AddRefs(uri), url);
      if (!uri) {
        
        return NS_OK;
      }

      nsCOMPtr<nsIDOMDocument> sourceDocument;
      session->GetSourceDocument(getter_AddRefs(sourceDocument));

      nsCOMPtr<nsIDocument> sourceDoc(do_QueryInterface(sourceDocument));
      if (sourceDoc) {
        rv = nsContentUtils::GetSecurityManager()->
          CheckLoadURIWithPrincipal(sourceDoc->NodePrincipal(), uri,
                                    nsIScriptSecurityManager::STANDARD);

        if (NS_FAILED(rv)) {
          
          
          inMouseEvent->StopPropagation();

          return rv;
        }
      }

      
      mNavigator->LoadURI(url.get(), nsIWebNavigation::LOAD_FLAGS_NONE, nsnull,
                          nsnull, nsnull);
    }
  }

  return NS_OK;
}




void
nsContentAreaDragDrop::NormalizeSelection(nsIDOMNode* inBaseNode,
                                          nsISelection* inSelection)
{
  nsCOMPtr<nsIDOMNode> parent;
  inBaseNode->GetParentNode(getter_AddRefs(parent));
  if (!parent || !inSelection)
    return;

  nsCOMPtr<nsIDOMNodeList> childNodes;
  parent->GetChildNodes(getter_AddRefs(childNodes));
  if (!childNodes)
    return;
  PRUint32 listLen = 0;
  childNodes->GetLength(&listLen);

  PRUint32 index = 0;
  for (; index < listLen; ++index) {
    nsCOMPtr<nsIDOMNode> indexedNode;
    childNodes->Item(index, getter_AddRefs(indexedNode));
    if (indexedNode == inBaseNode) {
      break;
    }
  }

  if (index >= listLen) {
    return;
  }

  
  
  inSelection->Collapse(parent, index);
  inSelection->Extend(parent, index+1);
}







void
nsContentAreaDragDrop::GetEventDocument(nsIDOMEvent* inEvent,
                                        nsIDOMDocument** outDocument)
{
  *outDocument = nsnull;

  nsCOMPtr<nsIDOMUIEvent> uiEvent(do_QueryInterface(inEvent));
  if (uiEvent) {
    nsCOMPtr<nsIDOMAbstractView> view;
    uiEvent->GetView(getter_AddRefs(view));
    nsCOMPtr<nsIDOMWindow> window(do_QueryInterface(view));

    if (window) {
      window->GetDocument(outDocument);
    }
  }
}

nsresult
nsContentAreaDragDrop::GetHookEnumeratorFromEvent(nsIDOMEvent* inEvent,
                                                  nsISimpleEnumerator **outEnumerator)
{
  *outEnumerator = nsnull;

  nsCOMPtr<nsIDOMDocument> domdoc;
  GetEventDocument(inEvent, getter_AddRefs(domdoc));
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(domdoc);
  NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);

  nsCOMPtr<nsISupports> container = doc->GetContainer();
  nsCOMPtr<nsIDocShell> docShell = do_QueryInterface(container);
  NS_ENSURE_TRUE(docShell, NS_ERROR_FAILURE);

  nsCOMPtr<nsIClipboardDragDropHookList> hookList = do_GetInterface(docShell);
  NS_ENSURE_TRUE(hookList, NS_ERROR_FAILURE);
  nsCOMPtr<nsISimpleEnumerator> enumerator;
  hookList->GetHookEnumerator(getter_AddRefs(enumerator));
  NS_ENSURE_TRUE(enumerator, NS_ERROR_FAILURE);

  *outEnumerator = enumerator;
  NS_ADDREF(*outEnumerator);

  return NS_OK;
}







NS_IMETHODIMP
nsContentAreaDragDrop::DragGesture(nsIDOMEvent* inMouseEvent)
{
  
  PRBool preventDefault = PR_TRUE;
  nsCOMPtr<nsIDOMNSUIEvent> nsuiEvent(do_QueryInterface(inMouseEvent));
  if (nsuiEvent) {
    nsuiEvent->GetPreventDefault(&preventDefault);
  }

  if (preventDefault) {
    return NS_OK;
  }

  
  
  nsCOMPtr<nsISimpleEnumerator> enumerator;
  GetHookEnumeratorFromEvent(inMouseEvent, getter_AddRefs(enumerator));

  if (enumerator) {
    PRBool allow = PR_TRUE;
    PRBool hasMoreHooks = PR_FALSE;
    while (NS_SUCCEEDED(enumerator->HasMoreElements(&hasMoreHooks))
           && hasMoreHooks) {
      nsCOMPtr<nsISupports> isupp;
      if (NS_FAILED(enumerator->GetNext(getter_AddRefs(isupp))))
        break;

      nsCOMPtr<nsIClipboardDragDropHooks> override = do_QueryInterface(isupp);
      if (override) {
#ifdef DEBUG
        nsresult hookResult =
#endif
        override->AllowStartDrag(inMouseEvent, &allow);
        NS_ASSERTION(NS_SUCCEEDED(hookResult),
                     "hook failure in AllowStartDrag");

        if (!allow)
          return NS_OK;
      }
    }
  }

  PRBool isSelection = PR_FALSE;
  nsCOMPtr<nsITransferable> trans;
  nsTransferableFactory factory(inMouseEvent, NS_STATIC_CAST(nsIFlavorDataProvider*, this));
  factory.Produce(&isSelection, getter_AddRefs(trans));

  if (trans) {
    
    
    nsCOMPtr<nsISimpleEnumerator> enumerator;
    GetHookEnumeratorFromEvent(inMouseEvent, getter_AddRefs(enumerator));
    if (enumerator) {
      PRBool hasMoreHooks = PR_FALSE;
      PRBool doContinueDrag = PR_TRUE;
      while (NS_SUCCEEDED(enumerator->HasMoreElements(&hasMoreHooks))
             && hasMoreHooks) {
        nsCOMPtr<nsISupports> isupp;
        if (NS_FAILED(enumerator->GetNext(getter_AddRefs(isupp))))
          break;
        nsCOMPtr<nsIClipboardDragDropHooks> override =
          do_QueryInterface(isupp);

        if (override) {
#ifdef DEBUG
          nsresult hookResult =
#endif
          override->OnCopyOrDrag(inMouseEvent, trans, &doContinueDrag);
          NS_ASSERTION(NS_SUCCEEDED(hookResult),
                       "hook failure in OnCopyOrDrag");

          if (!doContinueDrag) {
            return NS_OK;
          }
        }
      }
    }

    nsCOMPtr<nsISupportsArray> transArray =
      do_CreateInstance("@mozilla.org/supports-array;1");
    if (!transArray) {
      return NS_ERROR_FAILURE;
    }

    transArray->InsertElementAt(trans, 0);

    
    nsCOMPtr<nsIDOMEventTarget> target;
    inMouseEvent->GetTarget(getter_AddRefs(target));
    nsCOMPtr<nsIDragService> dragService =
      do_GetService("@mozilla.org/widget/dragservice;1");

    if (!dragService) {
      return NS_ERROR_FAILURE;
    }

    PRUint32 action = nsIDragService::DRAGDROP_ACTION_COPY +
                      nsIDragService::DRAGDROP_ACTION_MOVE +
                      nsIDragService::DRAGDROP_ACTION_LINK;

    nsCOMPtr<nsIDOMMouseEvent> mouseEvent(do_QueryInterface(inMouseEvent));

    if (isSelection) {
      nsCOMPtr<nsIContent> targetContent(do_QueryInterface(target));
      nsIDocument* doc = targetContent->GetCurrentDoc();
      if (doc) {
        nsIPresShell* presShell = doc->GetShellAt(0);
        if (presShell) {
          nsISelection* selection =
            presShell->GetCurrentSelection(nsISelectionController::SELECTION_NORMAL);
          return dragService->InvokeDragSessionWithSelection(selection,
                                                             transArray,
                                                             action,
                                                             mouseEvent);
        }
      }
    }

    nsCOMPtr<nsIDOMNode> targetNode(do_QueryInterface(target));
    dragService->InvokeDragSessionWithImage(targetNode, transArray, nsnull,
                                            action, nsnull, 0, 0, mouseEvent);
  }

  return NS_OK;
}


NS_IMETHODIMP
nsContentAreaDragDrop::HandleEvent(nsIDOMEvent *event)
{
  return NS_OK;

}

#if 0
#pragma mark -
#endif




nsresult
nsContentAreaDragDrop::SaveURIToFile(nsAString& inSourceURIString,
                                     nsIFile* inDestFile)
{
  nsCOMPtr<nsIURI> sourceURI;
  nsresult rv = NS_NewURI(getter_AddRefs(sourceURI), inSourceURIString);
  if (NS_FAILED(rv)) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIURL> sourceURL = do_QueryInterface(sourceURI);
  if (!sourceURL) {
    return NS_ERROR_NO_INTERFACE;
  }

  rv = inDestFile->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0600);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nsCOMPtr<nsIWebBrowserPersist> persist =
    do_CreateInstance("@mozilla.org/embedding/browser/nsWebBrowserPersist;1",
                      &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return persist->SaveURI(sourceURI, nsnull, nsnull, nsnull, nsnull,
                          inDestFile);
}














NS_IMETHODIMP
nsContentAreaDragDrop::GetFlavorData(nsITransferable *aTransferable,
                                     const char *aFlavor, nsISupports **aData,
                                     PRUint32 *aDataLen)
{
  NS_ENSURE_ARG_POINTER(aData && aDataLen);
  *aData = nsnull;
  *aDataLen = 0;

  nsresult rv = NS_ERROR_NOT_IMPLEMENTED;

  if (strcmp(aFlavor, kFilePromiseMime) == 0) {
    
    NS_ENSURE_ARG(aTransferable);
    nsCOMPtr<nsISupports> tmp;
    PRUint32 dataSize = 0;
    aTransferable->GetTransferData(kFilePromiseURLMime,
                                   getter_AddRefs(tmp), &dataSize);
    nsCOMPtr<nsISupportsString> supportsString =
      do_QueryInterface(tmp);
    if (!supportsString)
      return NS_ERROR_FAILURE;

    nsAutoString sourceURLString;
    supportsString->GetData(sourceURLString);
    if (sourceURLString.IsEmpty())
      return NS_ERROR_FAILURE;

    aTransferable->GetTransferData(kFilePromiseDestFilename,
                                   getter_AddRefs(tmp), &dataSize);
    supportsString = do_QueryInterface(tmp);
    if (!supportsString)
      return NS_ERROR_FAILURE;

    nsAutoString targetFilename;
    supportsString->GetData(targetFilename);
    if (targetFilename.IsEmpty())
      return NS_ERROR_FAILURE;

    
    
    nsCOMPtr<nsISupports> dirPrimitive;
    dataSize = 0;
    aTransferable->GetTransferData(kFilePromiseDirectoryMime,
                                   getter_AddRefs(dirPrimitive), &dataSize);
    nsCOMPtr<nsILocalFile> destDirectory = do_QueryInterface(dirPrimitive);
    if (!destDirectory)
      return NS_ERROR_FAILURE;

    nsCOMPtr<nsIFile> file;
    rv = destDirectory->Clone(getter_AddRefs(file));
    NS_ENSURE_SUCCESS(rv, rv);

    file->Append(targetFilename);

    
    rv = SaveURIToFile(sourceURLString, file);

    
    if (NS_SUCCEEDED(rv)) {
      CallQueryInterface(file, aData);
      *aDataLen = sizeof(nsIFile*);
    }
  }

  return rv;
}

nsTransferableFactory::nsTransferableFactory(nsIDOMEvent* inMouseEvent,
                                             nsIFlavorDataProvider *dataProvider)
  : mInstanceAlreadyUsed(PR_FALSE),
    mMouseEvent(inMouseEvent),
    mFlavorDataProvider(dataProvider)
{
}









already_AddRefed<nsIDOMNode>
nsTransferableFactory::FindParentLinkNode(nsIDOMNode* inNode)
{
  nsCOMPtr<nsIContent> content(do_QueryInterface(inNode));
  if (!content) {
    
    return nsnull;
  }

  for (; content; content = content->GetParent()) {
    if (nsContentUtils::IsDraggableLink(content)) {
      nsIDOMNode* node = nsnull;
      CallQueryInterface(content, &node);
      return node;
    }
  }

  return nsnull;
}





void
nsTransferableFactory::GetAnchorURL(nsIDOMNode* inNode, nsAString& outURL)
{
  nsCOMPtr<nsIURI> linkURI;
  nsCOMPtr<nsIContent> content = do_QueryInterface(inNode);
  if (!content || !content->IsLink(getter_AddRefs(linkURI))) {
    
    outURL.Truncate();
    return;
  }

  nsCAutoString spec;
  linkURI->GetSpec(spec);
  CopyUTF8toUTF16(spec, outURL);
}








void
nsTransferableFactory::CreateLinkText(const nsAString& inURL,
                                      const nsAString & inText,
                                      nsAString& outLinkText)
{
  
  
  
  nsAutoString linkText(NS_LITERAL_STRING("<a href=\"") +
                        inURL +
                        NS_LITERAL_STRING("\">") +
                        inText +
                        NS_LITERAL_STRING("</a>") );

  outLinkText = linkText;
}







void
nsTransferableFactory::GetNodeString(nsIDOMNode* inNode,
                                     nsAString & outNodeString)
{
  outNodeString.Truncate();

  
  nsCOMPtr<nsIDOMDocument> doc;
  inNode->GetOwnerDocument(getter_AddRefs(doc));
  nsCOMPtr<nsIDOMDocumentRange> docRange(do_QueryInterface(doc));
  if (docRange) {
    nsCOMPtr<nsIDOMRange> range;
    docRange->CreateRange(getter_AddRefs(range));
    if (range) {
      range->SelectNode(inNode);
      range->ToString(outNodeString);
    }
  }
}


nsresult
nsTransferableFactory::Produce(PRBool* aDragSelection,
                               nsITransferable** outTrans)
{
  if (mInstanceAlreadyUsed) {
    return NS_ERROR_FAILURE;
  }

  if (!outTrans || !mMouseEvent || !mFlavorDataProvider) {
    return NS_ERROR_FAILURE;
  }

  mInstanceAlreadyUsed = PR_TRUE;
  *outTrans = nsnull;

  nsCOMPtr<nsIDOMWindow> window;
  PRBool isAltKeyDown = PR_FALSE;
  mIsAnchor = PR_FALSE;

  {
    nsCOMPtr<nsIDOMUIEvent> uiEvent(do_QueryInterface(mMouseEvent));
    if (!uiEvent) {
      return NS_OK;
    }

    
    
    nsCOMPtr<nsIDOMAbstractView> view;
    uiEvent->GetView(getter_AddRefs(view));
    window = do_QueryInterface(view);
    if (!window) {
      return NS_OK;
    }
  }

  {
    nsCOMPtr<nsIDOMMouseEvent> mouseEvent(do_QueryInterface(mMouseEvent));
    if (mouseEvent) {
      mouseEvent->GetAltKey(&isAltKeyDown);
    }
  }

  nsCOMPtr<nsISelection> selection;
  window->GetSelection(getter_AddRefs(selection));
  if (!selection) {
    return NS_OK;
  }

  
  nsCOMPtr<nsIDOMNode> nodeToSerialize;
  PRBool useSelectedText = PR_FALSE;
  *aDragSelection = PR_FALSE;

  {
    PRBool haveSelectedContent = PR_FALSE;

    
    nsCOMPtr<nsIDOMNode> parentLink;
    nsCOMPtr<nsIDOMNode> draggedNode;

    {
      nsCOMPtr<nsIDOMEventTarget> target;
      mMouseEvent->GetTarget(getter_AddRefs(target));

      
      

      
      
      nsCOMPtr<nsIFormControl> form(do_QueryInterface(target));
      if (form && !isAltKeyDown && form->GetType() != NS_FORM_OBJECT) {
        return NS_OK;
      }

      draggedNode = do_QueryInterface(target);
    }

    nsCOMPtr<nsIDOMHTMLAreaElement>   area;   
    nsCOMPtr<nsIImageLoadingContent>  image;
    nsCOMPtr<nsIDOMHTMLAnchorElement> link;

    {
      
      nsCOMPtr<nsIDOMNode> realTargetNode;

      {
        nsCOMPtr<nsIDOMNSEvent> internalEvent = do_QueryInterface(mMouseEvent);
        if (internalEvent) {
          nsCOMPtr<nsIDOMEventTarget> realTarget;
          internalEvent->GetExplicitOriginalTarget(getter_AddRefs(realTarget));
          realTargetNode = do_QueryInterface(realTarget);
        }
      }

      {
        nsCOMPtr<nsIDOMNode> selectedImageOrLinkNode;
        GetDraggableSelectionData(selection, realTargetNode,
                                  getter_AddRefs(selectedImageOrLinkNode),
                                  &haveSelectedContent);

        
        if (haveSelectedContent) {
          link = do_QueryInterface(selectedImageOrLinkNode);
          if (link && isAltKeyDown) {
            return NS_OK;
          }

          useSelectedText = PR_TRUE;
          *aDragSelection = PR_TRUE;
        } else if (selectedImageOrLinkNode) {
          
          image = do_QueryInterface(selectedImageOrLinkNode);
        } else {
          
          
          
          
          
          
          parentLink = FindParentLinkNode(draggedNode);
          if (parentLink && isAltKeyDown) {
            return NS_OK;
          }

          area  = do_QueryInterface(draggedNode);
          image = do_QueryInterface(draggedNode);
          link  = do_QueryInterface(draggedNode);
        }
      }
    }

    {
      
      nsCOMPtr<nsIDOMNode> linkNode;

      if (area) {
        
        area->GetAttribute(NS_LITERAL_STRING("alt"), mTitleString);
        if (mTitleString.IsEmpty()) {
          
          area->GetAttribute(NS_LITERAL_STRING("href"), mTitleString);
        }

        
        mIsAnchor = PR_TRUE;

        
        GetAnchorURL(area, mUrlString);

        mHtmlString.AssignLiteral("<a href=\"");
        mHtmlString.Append(mUrlString);
        mHtmlString.AppendLiteral("\">");
        mHtmlString.Append(mTitleString);
        mHtmlString.AppendLiteral("</a>");
      } else if (image) {
        mIsAnchor = PR_TRUE;
        
        
        
        nsCOMPtr<nsIURI> imageURI;
        image->GetCurrentURI(getter_AddRefs(imageURI));
        if (imageURI) {
          nsCAutoString spec;
          imageURI->GetSpec(spec);
          CopyUTF8toUTF16(spec, mUrlString);
        }

        nsCOMPtr<nsIDOMElement> imageElement(do_QueryInterface(image));
        
        
        if (imageElement) {
          imageElement->GetAttribute(NS_LITERAL_STRING("alt"), mTitleString);
        }

        if (mTitleString.IsEmpty()) {
          mTitleString = mUrlString;
        }

        nsCOMPtr<imgIRequest> imgRequest;

        
        nsCOMPtr<nsIImage> img =
          nsContentUtils::GetImageFromContent(image,
                                              getter_AddRefs(imgRequest));

        nsCOMPtr<nsIMIMEService> mimeService =
          do_GetService("@mozilla.org/mime;1");

        
        if (imgRequest && mimeService) {
          nsCOMPtr<nsIURI> imgUri;
          imgRequest->GetURI(getter_AddRefs(imgUri));

          nsCOMPtr<nsIURL> imgUrl(do_QueryInterface(imgUri));

          if (imgUrl) {
            nsCAutoString extension;
            imgUrl->GetFileExtension(extension);

            nsXPIDLCString mimeType;
            imgRequest->GetMimeType(getter_Copies(mimeType));

            nsCOMPtr<nsIMIMEInfo> mimeInfo;
            mimeService->GetFromTypeAndExtension(mimeType, EmptyCString(),
                                                 getter_AddRefs(mimeInfo));

            if (mimeInfo) {
              nsCAutoString spec;
              imgUrl->GetSpec(spec);

              
              CopyUTF8toUTF16(spec, mImageSourceString);

              PRBool validExtension;
              if (extension.IsEmpty() || 
                  NS_FAILED(mimeInfo->ExtensionExists(extension,
                                                      &validExtension)) ||
                  !validExtension) {
                
                nsresult rv = imgUrl->Clone(getter_AddRefs(imgUri));
                NS_ENSURE_SUCCESS(rv, rv);

                imgUrl = do_QueryInterface(imgUri);

                nsCAutoString primaryExtension;
                mimeInfo->GetPrimaryExtension(primaryExtension);

                imgUrl->SetFileExtension(primaryExtension);
              }

              nsCAutoString fileName;
              imgUrl->GetFileName(fileName);

              NS_UnescapeURL(fileName);

              
              fileName.ReplaceChar(FILE_PATH_SEPARATOR FILE_ILLEGAL_CHARACTERS,
                                   '-');

              CopyUTF8toUTF16(fileName, mImageDestFileName);

              
              mImage = img;
            }
          }
        }

        if (parentLink) {
          
          
          linkNode = parentLink;
          nodeToSerialize = linkNode;
        } else {
          nodeToSerialize = draggedNode;
        }
      } else if (link) {
        
        linkNode = link;    
        GetNodeString(draggedNode, mTitleString);
      } else if (parentLink) {
        
        linkNode = parentLink;
        nodeToSerialize = linkNode;
      } else if (!haveSelectedContent) {
        
        return NS_OK;
      }

      if (linkNode) {
        mIsAnchor = PR_TRUE;
        GetAnchorURL(linkNode, mUrlString);
      }
    }
  }

  if (nodeToSerialize || useSelectedText) {
    
    if (useSelectedText) {
      nodeToSerialize = nsnull;
    }

    SerializeNodeOrSelection(serializeAsHTML,
                             nsIDocumentEncoder::OutputAbsoluteLinks |
                             nsIDocumentEncoder::OutputEncodeW3CEntities,
                             window, nodeToSerialize,
                             mHtmlString, mContextString, mInfoString);

    nsAutoString dummy1, dummy2;
    SerializeNodeOrSelection(serializeAsText, 0,
                             window, nodeToSerialize,
                             mTitleString, dummy1, dummy2);

#ifdef CHANGE_SELECTION_ON_DRAG
    
    
    
    NormalizeSelection(selectionNormalizeNode, selection);
#endif
  }

  
  if (mTitleString.IsEmpty()) {
    mTitleString = mUrlString;
  }

  
  if (mHtmlString.IsEmpty() && !mUrlString.IsEmpty())
    CreateLinkText(mUrlString, mTitleString, mHtmlString);

  return ConvertStringsToTransferable(outTrans);
}

nsresult
nsTransferableFactory::ConvertStringsToTransferable(nsITransferable** outTrans)
{
  
  nsCOMPtr<nsITransferable> trans =
    do_CreateInstance("@mozilla.org/widget/transferable;1");
  NS_ENSURE_TRUE(trans, NS_ERROR_FAILURE);

  
  
  if (!mUrlString.IsEmpty() && mIsAnchor) {
    nsAutoString dragData(mUrlString);
    dragData.AppendLiteral("\n");
    dragData += mTitleString;

    nsCOMPtr<nsISupportsString> urlPrimitive =
      do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID);
    NS_ENSURE_TRUE(urlPrimitive, NS_ERROR_FAILURE);

    urlPrimitive->SetData(dragData);
    trans->SetTransferData(kURLMime, urlPrimitive,
                           dragData.Length() * sizeof(PRUnichar));

    nsCOMPtr<nsISupportsString> urlDataPrimitive =
      do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID);
    NS_ENSURE_TRUE(urlDataPrimitive, NS_ERROR_FAILURE);

    urlDataPrimitive->SetData(mUrlString);
    trans->SetTransferData(kURLDataMime, urlDataPrimitive,
                           mUrlString.Length() * sizeof(PRUnichar));

    nsCOMPtr<nsISupportsString> urlDescPrimitive =
      do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID);
    NS_ENSURE_TRUE(urlDescPrimitive, NS_ERROR_FAILURE);

    urlDescPrimitive->SetData(mTitleString);
    trans->SetTransferData(kURLDescriptionMime, urlDescPrimitive,
                           mTitleString.Length() * sizeof(PRUnichar));
  }

  
  nsCOMPtr<nsISupportsString> context =
    do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID);
  NS_ENSURE_TRUE(context, NS_ERROR_FAILURE);

  nsAutoString contextData(mContextString);
  context->SetData(contextData);
  trans->SetTransferData(kHTMLContext, context, contextData.Length() * 2);

  
  if (!mInfoString.IsEmpty()) {
    nsCOMPtr<nsISupportsString> info =
      do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID);
    NS_ENSURE_TRUE(info, NS_ERROR_FAILURE);

    nsAutoString infoData(mInfoString);
    info->SetData(infoData);
    trans->SetTransferData(kHTMLInfo, info, infoData.Length() * 2);
  }

  
  nsCOMPtr<nsISupportsString> htmlPrimitive =
    do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID);
  NS_ENSURE_TRUE(htmlPrimitive, NS_ERROR_FAILURE);

  htmlPrimitive->SetData(mHtmlString);
  trans->SetTransferData(kHTMLMime, htmlPrimitive,
                         mHtmlString.Length() * sizeof(PRUnichar));

  
  
  
  nsCOMPtr<nsISupportsString> textPrimitive =
    do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID);
  NS_ENSURE_TRUE(textPrimitive, NS_ERROR_FAILURE);

  textPrimitive->SetData(mIsAnchor ? mUrlString : mTitleString);
  trans->SetTransferData(kUnicodeMime, textPrimitive,
                         (mIsAnchor ? mUrlString.Length() :
                          mTitleString.Length()) * sizeof(PRUnichar));

  
  
  
  
  if (mImage) {
    nsCOMPtr<nsISupportsInterfacePointer> ptrPrimitive =
      do_CreateInstance(NS_SUPPORTS_INTERFACE_POINTER_CONTRACTID);
    NS_ENSURE_TRUE(ptrPrimitive, NS_ERROR_FAILURE);

    ptrPrimitive->SetData(mImage);
    trans->SetTransferData(kNativeImageMime, ptrPrimitive,
                           sizeof(nsISupportsInterfacePointer*));
    
    
    
    trans->SetTransferData(kFilePromiseMime, mFlavorDataProvider,
                           nsITransferable::kFlavorHasDataProvider);

    nsCOMPtr<nsISupportsString> imageUrlPrimitive =
      do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID);
    NS_ENSURE_TRUE(imageUrlPrimitive, NS_ERROR_FAILURE);

    imageUrlPrimitive->SetData(mImageSourceString);
    trans->SetTransferData(kFilePromiseURLMime, imageUrlPrimitive,
                           mImageSourceString.Length() * sizeof(PRUnichar));

    nsCOMPtr<nsISupportsString> imageFileNamePrimitive =
      do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID);
    NS_ENSURE_TRUE(imageFileNamePrimitive, NS_ERROR_FAILURE);

    imageFileNamePrimitive->SetData(mImageDestFileName);
    trans->SetTransferData(kFilePromiseDestFilename, imageFileNamePrimitive,
                           mImageDestFileName.Length() * sizeof(PRUnichar));

    
    if (!mIsAnchor) {
      nsCOMPtr<nsISupportsString> urlDataPrimitive =
        do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID);
      NS_ENSURE_TRUE(urlDataPrimitive, NS_ERROR_FAILURE);

      urlDataPrimitive->SetData(mUrlString);
      trans->SetTransferData(kURLDataMime, urlDataPrimitive,
                             mUrlString.Length() * sizeof(PRUnichar));
    }
  }

  *outTrans = trans;
  NS_IF_ADDREF(*outTrans);

  return NS_OK;
}



nsresult
nsTransferableFactory::GetDraggableSelectionData(nsISelection* inSelection,
                                                 nsIDOMNode* inRealTargetNode,
                                                 nsIDOMNode **outImageOrLinkNode,
                                                 PRBool* outDragSelectedText)
{
  NS_ENSURE_ARG(inSelection);
  NS_ENSURE_ARG(inRealTargetNode);
  NS_ENSURE_ARG_POINTER(outImageOrLinkNode);

  *outImageOrLinkNode = nsnull;
  *outDragSelectedText = PR_FALSE;

  PRBool selectionContainsTarget = PR_FALSE;

  PRBool isCollapsed = PR_FALSE;
  inSelection->GetIsCollapsed(&isCollapsed);
  if (!isCollapsed) {
    inSelection->ContainsNode(inRealTargetNode, PR_FALSE,
                              &selectionContainsTarget);

    if (selectionContainsTarget) {
      
      nsCOMPtr<nsIDOMNode> selectionStart;
      inSelection->GetAnchorNode(getter_AddRefs(selectionStart));

      nsCOMPtr<nsIDOMNode> selectionEnd;
      inSelection->GetFocusNode(getter_AddRefs(selectionEnd));

      
      
      
      if (selectionStart == selectionEnd) {
        PRBool hasChildren;
        selectionStart->HasChildNodes(&hasChildren);
        if (hasChildren) {
          
          PRInt32 anchorOffset, focusOffset;
          inSelection->GetAnchorOffset(&anchorOffset);
          inSelection->GetFocusOffset(&focusOffset);
          if (abs(anchorOffset - focusOffset) == 1) {
            nsCOMPtr<nsIContent> selStartContent =
              do_QueryInterface(selectionStart);

            if (selStartContent) {
              PRInt32 childOffset =
                (anchorOffset < focusOffset) ? anchorOffset : focusOffset;
              nsIContent *childContent =
                selStartContent->GetChildAt(childOffset);
              
              
              if (nsContentUtils::IsDraggableImage(childContent)) {
                CallQueryInterface(childContent, outImageOrLinkNode);

                return NS_OK;
              }
            }
          }
        }
      }

      
      GetSelectedLink(inSelection, outImageOrLinkNode);

      
      *outDragSelectedText = PR_TRUE;
    }
  }

  return NS_OK;
}


void nsTransferableFactory::GetSelectedLink(nsISelection* inSelection,
                                            nsIDOMNode **outLinkNode)
{
  *outLinkNode = nsnull;

  nsCOMPtr<nsIDOMNode> selectionStart;
  inSelection->GetAnchorNode(getter_AddRefs(selectionStart));
  nsCOMPtr<nsIDOMNode> selectionEnd;
  inSelection->GetFocusNode(getter_AddRefs(selectionEnd));

  
  

  if (selectionStart == selectionEnd) {
    nsCOMPtr<nsIDOMNode> link = FindParentLinkNode(selectionStart);
    if (link) {
      link.swap(*outLinkNode);
    }

    return;
  }

  

  
  
  
  

  
  

  PRInt32 startOffset, endOffset;
  {
    nsCOMPtr<nsIDOMRange> range;
    inSelection->GetRangeAt(0, getter_AddRefs(range));
    if (!range) {
      return;
    }

    nsCOMPtr<nsIDOMNode> tempNode;
    range->GetStartContainer( getter_AddRefs(tempNode));
    if (tempNode != selectionStart) {
      selectionEnd = selectionStart;
      selectionStart = tempNode;
      inSelection->GetAnchorOffset(&endOffset);
      inSelection->GetFocusOffset(&startOffset);
    } else {
      inSelection->GetAnchorOffset(&startOffset);
      inSelection->GetFocusOffset(&endOffset);
    }
  }

  
  

  nsAutoString nodeStr;
  selectionStart->GetNodeValue(nodeStr);
  if (nodeStr.IsEmpty() ||
      startOffset+1 >= NS_STATIC_CAST(PRInt32, nodeStr.Length())) {
    nsCOMPtr<nsIDOMNode> curr = selectionStart;
    nsIDOMNode* next;

    while (curr) {
      curr->GetNextSibling(&next);

      if (next) {
        selectionStart = dont_AddRef(next);
        break;
      }

      curr->GetParentNode(&next);
      curr = dont_AddRef(next);
    }
  }

  

  if (endOffset == 0) {
    nsCOMPtr<nsIDOMNode> curr = selectionEnd;
    nsIDOMNode* next;

    while (curr) {
      curr->GetPreviousSibling(&next);

      if (next){
        selectionEnd = dont_AddRef(next);
        break;
      }

      curr->GetParentNode(&next);
      curr = dont_AddRef(next);
    }
  }

  
  
  nsCOMPtr<nsIDOMNode> link = FindParentLinkNode(selectionStart);
  if (link) {
    nsCOMPtr<nsIDOMNode> link2 = FindParentLinkNode(selectionEnd);

    if (link == link2) {
      NS_IF_ADDREF(*outLinkNode = link);
    }
  }

  return;
}


nsresult
nsTransferableFactory::SerializeNodeOrSelection(serializationMode inMode,
                                                PRUint32 inFlags,
                                                nsIDOMWindow* inWindow,
                                                nsIDOMNode* inNode,
                                                nsAString& outResultString,
                                                nsAString& outContext,
                                                nsAString& outInfo)
{
  NS_ENSURE_ARG_POINTER(inWindow);

  nsresult rv;
  nsCOMPtr<nsIDocumentEncoder> encoder;
  static const char *textplain = "text/plain";

  if (inMode == serializeAsText) {
    nsCAutoString formatType(NS_DOC_ENCODER_CONTRACTID_BASE);
    formatType.Append(textplain);
    encoder = do_CreateInstance(formatType.get(), &rv);
  } else {
    encoder = do_CreateInstance(NS_HTMLCOPY_ENCODER_CONTRACTID, &rv);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMDocument> domDoc;
  inWindow->GetDocument(getter_AddRefs(domDoc));
  NS_ENSURE_TRUE(domDoc, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDOMRange> range;
  nsCOMPtr<nsISelection> selection;
  if (inNode) {
    
    rv = NS_NewRange(getter_AddRefs(range));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = range->SelectNode(inNode);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    inWindow->GetSelection(getter_AddRefs(selection));
    inFlags |= nsIDocumentEncoder::OutputSelectionOnly;
  }

  if (inMode == serializeAsText) {
    rv = encoder->Init(domDoc, NS_ConvertASCIItoUTF16(textplain), inFlags);
  } else {
    rv = encoder->Init(domDoc, NS_LITERAL_STRING(kHTMLMime), inFlags);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  if (range) {
    encoder->SetRange(range);
  } else if (selection) {
    encoder->SetSelection(selection);
  }

  if (inMode == serializeAsText) {
    outContext.Truncate();
    outInfo.Truncate();
    return encoder->EncodeToString(outResultString);
  }

  return encoder->EncodeToStringWithContext(outContext, outInfo,
                                            outResultString);
}
