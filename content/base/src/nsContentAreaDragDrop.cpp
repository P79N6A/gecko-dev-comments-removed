





































#include "nsReadableUtils.h"


#include "nsContentAreaDragDrop.h"


#include "nsString.h"


#include "nsCopySupport.h"
#include "nsIDOMNSUIEvent.h"
#include "nsIDOMUIEvent.h"
#include "nsISelection.h"
#include "nsIDOMNode.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMEvent.h"
#include "nsIDOMNSEvent.h"
#include "nsIDOMDragEvent.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDocumentRange.h"
#include "nsIDOMRange.h"
#include "nsIFormControl.h"
#include "nsIDOMHTMLAreaElement.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsITransferable.h"
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
#include "nsUnicharUtils.h"
#include "nsIURL.h"
#include "nsIDocument.h"
#include "nsIScriptSecurityManager.h"
#include "nsIPrincipal.h"
#include "nsIDocShellTreeItem.h"
#include "nsIWebBrowserPersist.h"
#include "nsEscape.h"
#include "nsContentUtils.h"
#include "nsIMIMEService.h"
#include "imgIContainer.h"
#include "imgIRequest.h"
#include "nsDOMDataTransfer.h"


#define kHTMLContext   "text/_moz_htmlcontext"
#define kHTMLInfo      "text/_moz_htmlinfo"


static nsresult
GetTransferableForNodeOrSelection(nsIDOMWindow*     aWindow,
                                  nsIContent*       aNode,
                                  nsITransferable** aTransferable)
{
  NS_ENSURE_ARG_POINTER(aWindow);

  nsCOMPtr<nsIDOMDocument> domDoc;
  aWindow->GetDocument(getter_AddRefs(domDoc));
  NS_ENSURE_TRUE(domDoc, NS_ERROR_FAILURE);
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);

  nsresult rv;
  if (aNode) {
    rv = nsCopySupport::GetTransferableForNode(aNode, doc, aTransferable);
  } else {
    nsCOMPtr<nsISelection> selection;
    aWindow->GetSelection(getter_AddRefs(selection));
    rv = nsCopySupport::GetTransferableForSelection(selection, doc,
                                                    aTransferable);
  }

  NS_ENSURE_SUCCESS(rv, rv);
  return rv;
}

class NS_STACK_CLASS DragDataProducer
{
public:
  DragDataProducer(nsIDOMWindow* aWindow,
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

  DragDataProducer
    provider(aWindow, aTarget, aSelectionTargetNode, aIsAltKeyPressed);
  return provider.Produce(aDataTransfer, aCanDrag, aDragSelection, aDragNode);
}


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

DragDataProducer::DragDataProducer(nsIDOMWindow* aWindow,
                                   nsIContent* aTarget,
                                   nsIContent* aSelectionTargetNode,
                                   PRBool aIsAltKeyPressed)
  : mWindow(aWindow),
    mTarget(aTarget),
    mSelectionTargetNode(aSelectionTargetNode),
    mIsAltKeyPressed(aIsAltKeyPressed),
    mIsAnchor(PR_FALSE)
{
}









already_AddRefed<nsIContent>
DragDataProducer::FindParentLinkNode(nsIContent* inNode)
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
DragDataProducer::GetAnchorURL(nsIContent* inNode, nsAString& outURL)
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
DragDataProducer::CreateLinkText(const nsAString& inURL,
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
DragDataProducer::GetNodeString(nsIContent* inNode,
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
DragDataProducer::Produce(nsDOMDataTransfer* aDataTransfer,
                          PRBool* aCanDrag,
                          PRBool* aDragSelection,
                          nsIContent** aDragNode)
{
  NS_PRECONDITION(aCanDrag && aDragSelection && aDataTransfer && aDragNode,
                  "null pointer passed to Produce");
  NS_ASSERTION(mWindow, "window not set");
  NS_ASSERTION(mSelectionTargetNode, "selection target node should have been set");

  *aDragNode = nsnull;

  nsresult rv;
  nsIContent* dragNode = nsnull;

  
  
  nsCOMPtr<nsISelection> selection;
  mWindow->GetSelection(getter_AddRefs(selection));
  if (!selection) {
    return NS_OK;
  }

  
  
  
  nsCOMPtr<nsIContent> findFormNode = mSelectionTargetNode;
  nsIContent* findFormParent = findFormNode->GetParent();
  while (findFormParent) {
    nsCOMPtr<nsIFormControl> form(do_QueryInterface(findFormParent));
    if (form && !form->AllowDraggableChildren()) {
      return NS_OK;
    }
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

    mHtmlString.Truncate();
    mContextString.Truncate();
    mInfoString.Truncate();
    mTitleString.Truncate();
    nsCOMPtr<nsITransferable> transferable;
    rv = ::GetTransferableForNodeOrSelection(mWindow, nodeToSerialize,
                                             getter_AddRefs(transferable));
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsISupportsString> data;
    PRUint32 dataSize;
    rv = transferable->GetTransferData(kHTMLMime, getter_AddRefs(data), &dataSize);
    if (NS_SUCCEEDED(rv)) {
      data->GetData(mHtmlString);
    }
    rv = transferable->GetTransferData(kHTMLContext, getter_AddRefs(data), &dataSize);
    if (NS_SUCCEEDED(rv)) {
      data->GetData(mContextString);
    }
    rv = transferable->GetTransferData(kHTMLInfo, getter_AddRefs(data), &dataSize);
    if (NS_SUCCEEDED(rv)) {
      data->GetData(mInfoString);
    }
    rv = transferable->GetTransferData(kUnicodeMime, getter_AddRefs(data), &dataSize);
    NS_ENSURE_SUCCESS(rv, rv); 
    data->GetData(mTitleString);
  }

  
  if (mTitleString.IsEmpty()) {
    mTitleString = mUrlString;
  }

  
  if (mHtmlString.IsEmpty() && !mUrlString.IsEmpty())
    CreateLinkText(mUrlString, mTitleString, mHtmlString);

  
  
  rv = AddStringsToDataTransfer(
         dragNode ? dragNode : mSelectionTargetNode.get(), aDataTransfer);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_IF_ADDREF(*aDragNode = dragNode);
  return NS_OK;
}

void
DragDataProducer::AddString(nsDOMDataTransfer* aDataTransfer,
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
DragDataProducer::AddStringsToDataTransfer(nsIContent* aDragNode,
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
DragDataProducer::GetDraggableSelectionData(nsISelection* inSelection,
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


void
DragDataProducer::GetSelectedLink(nsISelection* inSelection,
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
