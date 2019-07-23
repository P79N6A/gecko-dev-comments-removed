





































#include "nsReadableUtils.h"


#include "nsContentAreaDragDrop.h"


#include "nsString.h"


#include "nsIVariant.h"
#include "nsIDOMNSUIEvent.h"
#include "nsIDOMUIEvent.h"
#include "nsISelection.h"
#include "nsIDOMNode.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMEvent.h"
#include "nsIDOMNSEvent.h"
#include "nsIDOMDragEvent.h"
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
#include "nsIDocShell.h"
#include "nsIContent.h"
#include "nsIImageLoadingContent.h"
#include "nsINameSpaceManager.h"
#include "nsUnicharUtils.h"
#include "nsIURL.h"
#include "nsIDocument.h"
#include "nsIScriptSecurityManager.h"
#include "nsIPrincipal.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsIDocShellTreeItem.h"
#include "nsIFrame.h"
#include "nsRange.h"
#include "nsIWebBrowserPersist.h"
#include "nsEscape.h"
#include "nsContentUtils.h"
#include "nsIMIMEService.h"
#include "imgIContainer.h"
#include "imgIRequest.h"
#include "nsContentCID.h"
#include "nsDOMDataTransfer.h"
#include "nsISelectionController.h"
#include "nsFrameSelection.h"
#include "nsIDOMEventTarget.h"
#include "nsWidgetsCID.h"

static NS_DEFINE_CID(kHTMLConverterCID,        NS_HTMLFORMATCONVERTER_CID);


#define kHTMLContext   "text/_moz_htmlcontext"
#define kHTMLInfo      "text/_moz_htmlinfo"


NS_IMPL_ADDREF(nsContentAreaDragDrop)
NS_IMPL_RELEASE(nsContentAreaDragDrop)

NS_INTERFACE_MAP_BEGIN(nsContentAreaDragDrop)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMEventListener)
    NS_INTERFACE_MAP_ENTRY(nsIDOMEventListener)
    NS_INTERFACE_MAP_ENTRY(nsIDragDropHandler)
NS_INTERFACE_MAP_END


class NS_STACK_CLASS nsTransferableFactory
{
public:
  nsTransferableFactory(nsIDOMWindow* aWindow,
                        nsIContent* aTarget,
                        nsIContent* aSelectionTargetNode,
                        PRBool aIsAltKeyPressed);
  nsresult Produce(nsDOMDataTransfer* aDataTransfer,
                   PRBool* aCanDrag,
                   PRBool* aDragSelection,
                   nsIContent** aDragNode);

private:
  void AddString(nsDOMDataTransfer* aDataTransfer,
                 const nsAString& aFlavor,
                 const nsAString& aData,
                 nsIPrincipal* aPrincipal);
  nsresult AddStringsToDataTransfer(nsIContent* aDragNode,
                                    nsDOMDataTransfer* aDataTransfer);
  static nsresult GetDraggableSelectionData(nsISelection* inSelection,
                                            nsIContent* inRealTargetNode,
                                            nsIContent **outImageOrLinkNode,
                                            PRBool* outDragSelectedText);
  static already_AddRefed<nsIContent> FindParentLinkNode(nsIContent* inNode);
  static void GetAnchorURL(nsIContent* inNode, nsAString& outURL);
  static void GetNodeString(nsIContent* inNode, nsAString & outNodeString);
  static void CreateLinkText(const nsAString& inURL, const nsAString & inText,
                              nsAString& outLinkText);
  static void GetSelectedLink(nsISelection* inSelection,
                              nsIContent **outLinkNode);

  
  static nsresult SerializeNodeOrSelection(nsIDOMWindow* inWindow,
                                           nsIContent* inNode,
                                           nsAString& outResultString,
                                           nsAString& outHTMLContext,
                                           nsAString& outHTMLInfo);

  nsCOMPtr<nsIDOMWindow> mWindow;
  nsCOMPtr<nsIContent> mTarget;
  nsCOMPtr<nsIContent> mSelectionTargetNode;
  PRPackedBool mIsAltKeyPressed;

  nsString mUrlString;
  nsString mImageSourceString;
  nsString mImageDestFileName;
  nsString mTitleString;
  
  nsString mHtmlString;
  nsString mContextString;
  nsString mInfoString;

  PRBool mIsAnchor;
  nsCOMPtr<imgIContainer> mImage;
};





nsContentAreaDragDrop::nsContentAreaDragDrop()
  : mNavigator(nsnull)
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
  mEventTarget = inAttachPoint;
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
  if (mEventTarget) {
    nsresult rv = mEventTarget->AddEventListener(NS_LITERAL_STRING("dragover"),
                                                 this, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mEventTarget->AddEventListener(NS_LITERAL_STRING("drop"), this,
                                        PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}







nsresult
nsContentAreaDragDrop::RemoveDragListener()
{
  if (mEventTarget) {
    nsresult rv =
      mEventTarget->RemoveEventListener(NS_LITERAL_STRING("dragover"), this,
                                        PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mEventTarget->RemoveEventListener(NS_LITERAL_STRING("drop"), this,
                                           PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
    mEventTarget = nsnull;
  }

  return NS_OK;
}












nsresult
nsContentAreaDragDrop::DragOver(nsIDOMDragEvent* inEvent)
{
  
  PRBool preventDefault = PR_TRUE;
  nsCOMPtr<nsIDOMNSUIEvent> nsuiEvent(do_QueryInterface(inEvent));
  if ( nsuiEvent )
    nsuiEvent->GetPreventDefault(&preventDefault);
  if ( preventDefault )
    return NS_OK;

  
  
  
  

  nsCOMPtr<nsIDragSession> session = nsContentUtils::GetDragSession();
  NS_ENSURE_TRUE(session, NS_OK);

  PRBool dropAllowed = PR_TRUE;

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









nsresult
nsContentAreaDragDrop::Drop(nsIDOMDragEvent* inDragEvent)
{
  
  
  if (!mNavigator) {
    return NS_OK;
  }

  
  PRBool preventDefault = PR_TRUE;
  nsCOMPtr<nsIDOMNSUIEvent> nsuiEvent(do_QueryInterface(inDragEvent));
  if (nsuiEvent) {
    nsuiEvent->GetPreventDefault(&preventDefault);
  }

  if (preventDefault) {
    return NS_OK;
  }

  
  
  
  nsCOMPtr<nsIDragSession> session = nsContentUtils::GetDragSession();
  NS_ENSURE_TRUE(session, NS_OK);

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
          
          
          inDragEvent->StopPropagation();

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
nsContentAreaDragDrop::GetDragData(nsIDOMWindow* aWindow,
                                   nsIContent* aTarget,
                                   nsIContent* aSelectionTargetNode,
                                   PRBool aIsAltKeyPressed,
                                   nsDOMDataTransfer* aDataTransfer,
                                   PRBool* aCanDrag,
                                   PRBool* aDragSelection,
                                   nsIContent** aDragNode)
{
  NS_ENSURE_TRUE(aSelectionTargetNode, NS_ERROR_INVALID_ARG);

  *aCanDrag = PR_TRUE;

  nsTransferableFactory
    factory(aWindow, aTarget, aSelectionTargetNode, aIsAltKeyPressed);
  return factory.Produce(aDataTransfer, aCanDrag, aDragSelection, aDragNode);
}


NS_IMETHODIMP
nsContentAreaDragDrop::HandleEvent(nsIDOMEvent *event)
{
  
  nsCOMPtr<nsIDOMDragEvent> dragEvent = do_QueryInterface(event);
  if (dragEvent) {
    nsAutoString eventType;
    event->GetType(eventType);
    if (eventType.EqualsLiteral("dragover"))
      return DragOver(dragEvent);
    if (eventType.EqualsLiteral("drop"))
      return Drop(dragEvent);
  }

  return NS_OK;
}

#if 0
#pragma mark -
#endif

NS_IMPL_ISUPPORTS1(nsContentAreaDragDropDataProvider, nsIFlavorDataProvider)




nsresult
nsContentAreaDragDropDataProvider::SaveURIToFile(nsAString& inSourceURIString,
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

  return persist->SaveURI(sourceURI, nsnull, nsnull, nsnull, nsnull, inDestFile);
}














NS_IMETHODIMP
nsContentAreaDragDropDataProvider::GetFlavorData(nsITransferable *aTransferable,
                                                 const char *aFlavor,
                                                 nsISupports **aData,
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

nsTransferableFactory::nsTransferableFactory(nsIDOMWindow* aWindow,
                                             nsIContent* aTarget,
                                             nsIContent* aSelectionTargetNode,
                                             PRBool aIsAltKeyPressed)
  : mWindow(aWindow),
    mTarget(aTarget),
    mSelectionTargetNode(aSelectionTargetNode),
    mIsAltKeyPressed(aIsAltKeyPressed)
{
}









already_AddRefed<nsIContent>
nsTransferableFactory::FindParentLinkNode(nsIContent* inNode)
{
  nsIContent* content = inNode;
  if (!content) {
    
    return nsnull;
  }

  for (; content; content = content->GetParent()) {
    if (nsContentUtils::IsDraggableLink(content)) {
      NS_ADDREF(content);
      return content;
    }
  }

  return nsnull;
}





void
nsTransferableFactory::GetAnchorURL(nsIContent* inNode, nsAString& outURL)
{
  nsCOMPtr<nsIURI> linkURI;
  if (!inNode || !inNode->IsLink(getter_AddRefs(linkURI))) {
    
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
nsTransferableFactory::GetNodeString(nsIContent* inNode,
                                     nsAString & outNodeString)
{
  nsCOMPtr<nsIDOMNode> node = do_QueryInterface(inNode);

  outNodeString.Truncate();

  
  nsCOMPtr<nsIDOMDocument> doc;
  node->GetOwnerDocument(getter_AddRefs(doc));
  nsCOMPtr<nsIDOMDocumentRange> docRange(do_QueryInterface(doc));
  if (docRange) {
    nsCOMPtr<nsIDOMRange> range;
    docRange->CreateRange(getter_AddRefs(range));
    if (range) {
      range->SelectNode(node);
      range->ToString(outNodeString);
    }
  }
}


nsresult
nsTransferableFactory::Produce(nsDOMDataTransfer* aDataTransfer,
                               PRBool* aCanDrag,
                               PRBool* aDragSelection,
                               nsIContent** aDragNode)
{
  NS_PRECONDITION(aCanDrag && aDragSelection && aDataTransfer && aDragNode,
                  "null pointer passed to Produce");
  NS_ASSERTION(mWindow, "window not set");
  NS_ASSERTION(mSelectionTargetNode, "selection target node should have been set");

  *aDragNode = nsnull;

  nsIContent* dragNode = nsnull;

  mIsAnchor = PR_FALSE;

  
  
  nsCOMPtr<nsISelection> selection;
  mWindow->GetSelection(getter_AddRefs(selection));
  if (!selection) {
    return NS_OK;
  }

  
  
  
  nsCOMPtr<nsIContent> findFormNode = mSelectionTargetNode;
  nsIContent* findFormParent = findFormNode->GetParent();
  while (findFormParent) {
    nsCOMPtr<nsIFormControl> form(do_QueryInterface(findFormParent));
    if (form && form->GetType() != NS_FORM_OBJECT &&
                form->GetType() != NS_FORM_FIELDSET &&
                form->GetType() != NS_FORM_LEGEND &&
                form->GetType() != NS_FORM_LABEL)
      return NS_OK;
    findFormParent = findFormParent->GetParent();
  }
    
  
  nsCOMPtr<nsIContent> nodeToSerialize;
  *aDragSelection = PR_FALSE;

  {
    PRBool haveSelectedContent = PR_FALSE;

    
    nsCOMPtr<nsIContent> parentLink;
    nsCOMPtr<nsIContent> draggedNode;

    {
      
      

      
      
      nsCOMPtr<nsIFormControl> form(do_QueryInterface(mTarget));
      if (form && !mIsAltKeyPressed && form->GetType() != NS_FORM_OBJECT) {
        *aCanDrag = PR_FALSE;
        return NS_OK;
      }

      draggedNode = mTarget;
    }

    nsCOMPtr<nsIDOMHTMLAreaElement>   area;   
    nsCOMPtr<nsIImageLoadingContent>  image;
    nsCOMPtr<nsIDOMHTMLAnchorElement> link;

    nsCOMPtr<nsIContent> selectedImageOrLinkNode;
    GetDraggableSelectionData(selection, mSelectionTargetNode,
                              getter_AddRefs(selectedImageOrLinkNode),
                              &haveSelectedContent);

    
    if (haveSelectedContent) {
      link = do_QueryInterface(selectedImageOrLinkNode);
      if (link && mIsAltKeyPressed) {
        
        *aCanDrag = PR_FALSE;
        return NS_OK;
      }

      *aDragSelection = PR_TRUE;
    } else if (selectedImageOrLinkNode) {
      
      image = do_QueryInterface(selectedImageOrLinkNode);
    } else {
      
      
      
      
      
      
      parentLink = FindParentLinkNode(draggedNode);
      if (parentLink && mIsAltKeyPressed) {
        *aCanDrag = PR_FALSE;
        return NS_OK;
      }

      area  = do_QueryInterface(draggedNode);
      image = do_QueryInterface(draggedNode);
      link  = do_QueryInterface(draggedNode);
    }

    {
      
      nsCOMPtr<nsIContent> linkNode;

      if (area) {
        
        area->GetAttribute(NS_LITERAL_STRING("alt"), mTitleString);
        if (mTitleString.IsEmpty()) {
          
          area->GetAttribute(NS_LITERAL_STRING("href"), mTitleString);
        }

        
        mIsAnchor = PR_TRUE;

        
        GetAnchorURL(draggedNode, mUrlString);

        mHtmlString.AssignLiteral("<a href=\"");
        mHtmlString.Append(mUrlString);
        mHtmlString.AppendLiteral("\">");
        mHtmlString.Append(mTitleString);
        mHtmlString.AppendLiteral("</a>");

        dragNode = draggedNode;
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

        
        nsCOMPtr<imgIContainer> img =
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
          nodeToSerialize = do_QueryInterface(draggedNode);
        }
        dragNode = nodeToSerialize;
      } else if (link) {
        
        linkNode = do_QueryInterface(link);    
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
        dragNode = linkNode;
      }
    }
  }

  if (nodeToSerialize || *aDragSelection) {
    
    if (*aDragSelection) {
      nodeToSerialize = nsnull;
    }

    SerializeNodeOrSelection(mWindow, nodeToSerialize,
                             mHtmlString, mContextString, mInfoString);

    nsCOMPtr<nsIFormatConverter> htmlConverter =
      do_CreateInstance(kHTMLConverterCID);
    NS_ENSURE_TRUE(htmlConverter, NS_ERROR_FAILURE);

    nsCOMPtr<nsISupportsString> html =
      do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID);
    NS_ENSURE_TRUE(html, NS_ERROR_FAILURE);
    html->SetData(mHtmlString);

    nsCOMPtr<nsISupportsString> text;
    PRUint32 textLen;
    htmlConverter->Convert(kHTMLMime, html, mHtmlString.Length() * 2,
                           kUnicodeMime, getter_AddRefs(text), &textLen);
    NS_ENSURE_TRUE(text, NS_ERROR_FAILURE);
    text->GetData(mTitleString);

#ifdef CHANGE_SELECTION_ON_DRAG
    
    
    
    NormalizeSelection(selectionNormalizeNode, selection);
#endif
  }

  
  if (mTitleString.IsEmpty()) {
    mTitleString = mUrlString;
  }

  
  if (mHtmlString.IsEmpty() && !mUrlString.IsEmpty())
    CreateLinkText(mUrlString, mTitleString, mHtmlString);

  
  
  nsresult rv = AddStringsToDataTransfer(
           dragNode ? dragNode : mSelectionTargetNode.get(), aDataTransfer);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_IF_ADDREF(*aDragNode = dragNode);
  return NS_OK;
}

void
nsTransferableFactory::AddString(nsDOMDataTransfer* aDataTransfer,
                                 const nsAString& aFlavor,
                                 const nsAString& aData,
                                 nsIPrincipal* aPrincipal)
{
  nsCOMPtr<nsIWritableVariant> variant = do_CreateInstance(NS_VARIANT_CONTRACTID);
  if (variant) {
    variant->SetAsAString(aData);
    aDataTransfer->SetDataWithPrincipal(aFlavor, variant, 0, aPrincipal);
  }
}

nsresult
nsTransferableFactory::AddStringsToDataTransfer(nsIContent* aDragNode,
                                                nsDOMDataTransfer* aDataTransfer)
{
  NS_ASSERTION(aDragNode, "adding strings for null node");

  
  nsIPrincipal* principal = aDragNode->NodePrincipal();

  
  
  if (!mUrlString.IsEmpty() && mIsAnchor) {
    nsAutoString dragData(mUrlString);
    dragData.AppendLiteral("\n");
    dragData += mTitleString;

    AddString(aDataTransfer, NS_LITERAL_STRING(kURLMime), dragData, principal);
    AddString(aDataTransfer, NS_LITERAL_STRING(kURLDataMime), mUrlString, principal);
    AddString(aDataTransfer, NS_LITERAL_STRING(kURLDescriptionMime), mTitleString, principal);
    AddString(aDataTransfer, NS_LITERAL_STRING("text/uri-list"), mUrlString, principal);
  }

  
  AddString(aDataTransfer, NS_LITERAL_STRING(kHTMLContext), mContextString, principal);

  
  if (!mInfoString.IsEmpty())
    AddString(aDataTransfer, NS_LITERAL_STRING(kHTMLInfo), mInfoString, principal);

  
  AddString(aDataTransfer, NS_LITERAL_STRING(kHTMLMime), mHtmlString, principal);

  
  
  
  AddString(aDataTransfer, NS_LITERAL_STRING(kTextMime),
            mIsAnchor ? mUrlString : mTitleString, principal);

  
  
  
  
  if (mImage) {
    nsCOMPtr<nsIWritableVariant> variant = do_CreateInstance(NS_VARIANT_CONTRACTID);
    if (variant) {
      variant->SetAsISupports(mImage);
      aDataTransfer->SetDataWithPrincipal(NS_LITERAL_STRING(kNativeImageMime),
                                          variant, 0, principal);
    }

    
    
    

    nsCOMPtr<nsIFlavorDataProvider> dataProvider =
      new nsContentAreaDragDropDataProvider();
    if (dataProvider) {
      nsCOMPtr<nsIWritableVariant> variant = do_CreateInstance(NS_VARIANT_CONTRACTID);
      if (variant) {
        variant->SetAsISupports(dataProvider);
        aDataTransfer->SetDataWithPrincipal(NS_LITERAL_STRING(kFilePromiseMime),
                                            variant, 0, principal);
      }
    }

    AddString(aDataTransfer, NS_LITERAL_STRING(kFilePromiseURLMime),
              mImageSourceString, principal);
    AddString(aDataTransfer, NS_LITERAL_STRING(kFilePromiseDestFilename),
              mImageDestFileName, principal);

    
    if (!mIsAnchor) {
      AddString(aDataTransfer, NS_LITERAL_STRING(kURLDataMime), mUrlString, principal);
      AddString(aDataTransfer, NS_LITERAL_STRING("text/uri-list"), mUrlString, principal);
    }
  }

  return NS_OK;
}



nsresult
nsTransferableFactory::GetDraggableSelectionData(nsISelection* inSelection,
                                                 nsIContent* inRealTargetNode,
                                                 nsIContent **outImageOrLinkNode,
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
    nsCOMPtr<nsIDOMNode> realTargetNode = do_QueryInterface(inRealTargetNode);
    inSelection->ContainsNode(realTargetNode, PR_FALSE,
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
                NS_ADDREF(*outImageOrLinkNode = childContent);
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
                                            nsIContent **outLinkNode)
{
  *outLinkNode = nsnull;

  nsCOMPtr<nsIDOMNode> selectionStartNode;
  inSelection->GetAnchorNode(getter_AddRefs(selectionStartNode));
  nsCOMPtr<nsIDOMNode> selectionEndNode;
  inSelection->GetFocusNode(getter_AddRefs(selectionEndNode));

  
  

  if (selectionStartNode == selectionEndNode) {
    nsCOMPtr<nsIContent> selectionStart = do_QueryInterface(selectionStartNode);
    nsCOMPtr<nsIContent> link = FindParentLinkNode(selectionStart);
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
    if (tempNode != selectionStartNode) {
      selectionEndNode = selectionStartNode;
      selectionStartNode = tempNode;
      inSelection->GetAnchorOffset(&endOffset);
      inSelection->GetFocusOffset(&startOffset);
    } else {
      inSelection->GetAnchorOffset(&startOffset);
      inSelection->GetFocusOffset(&endOffset);
    }
  }

  
  

  nsAutoString nodeStr;
  selectionStartNode->GetNodeValue(nodeStr);
  if (nodeStr.IsEmpty() ||
      startOffset+1 >= static_cast<PRInt32>(nodeStr.Length())) {
    nsCOMPtr<nsIDOMNode> curr = selectionStartNode;
    nsIDOMNode* next;

    while (curr) {
      curr->GetNextSibling(&next);

      if (next) {
        selectionStartNode = dont_AddRef(next);
        break;
      }

      curr->GetParentNode(&next);
      curr = dont_AddRef(next);
    }
  }

  

  if (endOffset == 0) {
    nsCOMPtr<nsIDOMNode> curr = selectionEndNode;
    nsIDOMNode* next;

    while (curr) {
      curr->GetPreviousSibling(&next);

      if (next){
        selectionEndNode = dont_AddRef(next);
        break;
      }

      curr->GetParentNode(&next);
      curr = dont_AddRef(next);
    }
  }

  
  
  nsCOMPtr<nsIContent> selectionStart = do_QueryInterface(selectionStartNode);
  nsCOMPtr<nsIContent> link = FindParentLinkNode(selectionStart);
  if (link) {
    nsCOMPtr<nsIContent> selectionEnd = do_QueryInterface(selectionEndNode);
    nsCOMPtr<nsIContent> link2 = FindParentLinkNode(selectionEnd);

    if (link == link2) {
      NS_IF_ADDREF(*outLinkNode = link);
    }
  }

  return;
}


nsresult
nsTransferableFactory::SerializeNodeOrSelection(nsIDOMWindow* inWindow,
                                                nsIContent* inNode,
                                                nsAString& outResultString,
                                                nsAString& outContext,
                                                nsAString& outInfo)
{
  NS_ENSURE_ARG_POINTER(inWindow);

  nsresult rv;
  nsCOMPtr<nsIDocumentEncoder> encoder =
    do_CreateInstance(NS_HTMLCOPY_ENCODER_CONTRACTID);
  NS_ENSURE_TRUE(encoder, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDOMDocument> domDoc;
  inWindow->GetDocument(getter_AddRefs(domDoc));
  NS_ENSURE_TRUE(domDoc, NS_ERROR_FAILURE);

  PRUint32 flags = nsIDocumentEncoder::OutputAbsoluteLinks |
                   nsIDocumentEncoder::OutputEncodeHTMLEntities |
                   nsIDocumentEncoder::OutputRaw;
  nsCOMPtr<nsIDOMRange> range;
  nsCOMPtr<nsISelection> selection;
  nsCOMPtr<nsIDOMNode> node = do_QueryInterface(inNode);
  if (node) {
    
    rv = NS_NewRange(getter_AddRefs(range));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = range->SelectNode(node);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    inWindow->GetSelection(getter_AddRefs(selection));
    flags |= nsIDocumentEncoder::OutputSelectionOnly;
  }

  rv = encoder->Init(domDoc, NS_LITERAL_STRING(kHTMLMime), flags);
  NS_ENSURE_SUCCESS(rv, rv);

  if (range) {
    encoder->SetRange(range);
  } else if (selection) {
    encoder->SetSelection(selection);
  }

  return encoder->EncodeToStringWithContext(outContext, outInfo,
                                            outResultString);
}
