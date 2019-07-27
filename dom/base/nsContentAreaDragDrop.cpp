




#include "nsReadableUtils.h"


#include "nsContentAreaDragDrop.h"


#include "nsString.h"


#include "nsCopySupport.h"
#include "nsIDOMUIEvent.h"
#include "nsISelection.h"
#include "nsISelectionController.h"
#include "nsIDOMNode.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMEvent.h"
#include "nsIDOMDragEvent.h"
#include "nsPIDOMWindow.h"
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
#include "nsITextControlElement.h"
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
#include "mozilla/dom/DataTransfer.h"
#include "nsIMIMEInfo.h"
#include "nsRange.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/HTMLAreaElement.h"

using namespace mozilla::dom;

class MOZ_STACK_CLASS DragDataProducer
{
public:
  DragDataProducer(nsPIDOMWindow* aWindow,
                   nsIContent* aTarget,
                   nsIContent* aSelectionTargetNode,
                   bool aIsAltKeyPressed);
  nsresult Produce(DataTransfer* aDataTransfer,
                   bool* aCanDrag,
                   nsISelection** aSelection,
                   nsIContent** aDragNode);

private:
  void AddString(DataTransfer* aDataTransfer,
                 const nsAString& aFlavor,
                 const nsAString& aData,
                 nsIPrincipal* aPrincipal);
  nsresult AddStringsToDataTransfer(nsIContent* aDragNode,
                                    DataTransfer* aDataTransfer);
  static nsresult GetDraggableSelectionData(nsISelection* inSelection,
                                            nsIContent* inRealTargetNode,
                                            nsIContent **outImageOrLinkNode,
                                            bool* outDragSelectedText);
  static already_AddRefed<nsIContent> FindParentLinkNode(nsIContent* inNode);
  static void GetAnchorURL(nsIContent* inNode, nsAString& outURL);
  static void GetNodeString(nsIContent* inNode, nsAString & outNodeString);
  static void CreateLinkText(const nsAString& inURL, const nsAString & inText,
                              nsAString& outLinkText);

  nsCOMPtr<nsPIDOMWindow> mWindow;
  nsCOMPtr<nsIContent> mTarget;
  nsCOMPtr<nsIContent> mSelectionTargetNode;
  bool mIsAltKeyPressed;

  nsString mUrlString;
  nsString mImageSourceString;
  nsString mImageDestFileName;
  nsString mTitleString;
  
  nsString mHtmlString;
  nsString mContextString;
  nsString mInfoString;

  bool mIsAnchor;
  nsCOMPtr<imgIContainer> mImage;
};


nsresult
nsContentAreaDragDrop::GetDragData(nsPIDOMWindow* aWindow,
                                   nsIContent* aTarget,
                                   nsIContent* aSelectionTargetNode,
                                   bool aIsAltKeyPressed,
                                   DataTransfer* aDataTransfer,
                                   bool* aCanDrag,
                                   nsISelection** aSelection,
                                   nsIContent** aDragNode)
{
  NS_ENSURE_TRUE(aSelectionTargetNode, NS_ERROR_INVALID_ARG);

  *aCanDrag = true;

  DragDataProducer
    provider(aWindow, aTarget, aSelectionTargetNode, aIsAltKeyPressed);
  return provider.Produce(aDataTransfer, aCanDrag, aSelection, aDragNode);
}


NS_IMPL_ISUPPORTS(nsContentAreaDragDropDataProvider, nsIFlavorDataProvider)




nsresult
nsContentAreaDragDropDataProvider::SaveURIToFile(nsAString& inSourceURIString,
                                                 nsIFile* inDestFile,
                                                 bool isPrivate)
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

  persist->SetPersistFlags(nsIWebBrowserPersist::PERSIST_FLAGS_AUTODETECT_APPLY_CONVERSION);

  
  return persist->SavePrivacyAwareURI(sourceURI, nullptr, nullptr,
                                      mozilla::net::RP_Default,
                                      nullptr, nullptr,
                                      inDestFile, isPrivate);
}














NS_IMETHODIMP
nsContentAreaDragDropDataProvider::GetFlavorData(nsITransferable *aTransferable,
                                                 const char *aFlavor,
                                                 nsISupports **aData,
                                                 uint32_t *aDataLen)
{
  NS_ENSURE_ARG_POINTER(aData && aDataLen);
  *aData = nullptr;
  *aDataLen = 0;

  nsresult rv = NS_ERROR_NOT_IMPLEMENTED;

  if (strcmp(aFlavor, kFilePromiseMime) == 0) {
    
    NS_ENSURE_ARG(aTransferable);
    nsCOMPtr<nsISupports> tmp;
    uint32_t dataSize = 0;
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
    nsCOMPtr<nsIFile> destDirectory = do_QueryInterface(dirPrimitive);
    if (!destDirectory)
      return NS_ERROR_FAILURE;

    nsCOMPtr<nsIFile> file;
    rv = destDirectory->Clone(getter_AddRefs(file));
    NS_ENSURE_SUCCESS(rv, rv);

    file->Append(targetFilename);

    bool isPrivate;
    aTransferable->GetIsPrivateData(&isPrivate);

    rv = SaveURIToFile(sourceURLString, file, isPrivate);
    
    if (NS_SUCCEEDED(rv)) {
      CallQueryInterface(file, aData);
      *aDataLen = sizeof(nsIFile*);
    }
  }

  return rv;
}

DragDataProducer::DragDataProducer(nsPIDOMWindow* aWindow,
                                   nsIContent* aTarget,
                                   nsIContent* aSelectionTargetNode,
                                   bool aIsAltKeyPressed)
  : mWindow(aWindow),
    mTarget(aTarget),
    mSelectionTargetNode(aSelectionTargetNode),
    mIsAltKeyPressed(aIsAltKeyPressed),
    mIsAnchor(false)
{
}









already_AddRefed<nsIContent>
DragDataProducer::FindParentLinkNode(nsIContent* inNode)
{
  nsIContent* content = inNode;
  if (!content) {
    
    return nullptr;
  }

  for (; content; content = content->GetParent()) {
    if (nsContentUtils::IsDraggableLink(content)) {
      nsCOMPtr<nsIContent> ret = content;
      return ret.forget();
    }
  }

  return nullptr;
}





void
DragDataProducer::GetAnchorURL(nsIContent* inNode, nsAString& outURL)
{
  nsCOMPtr<nsIURI> linkURI;
  if (!inNode || !inNode->IsLink(getter_AddRefs(linkURI))) {
    
    outURL.Truncate();
    return;
  }

  nsAutoCString spec;
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
  nsCOMPtr<nsINode> node = inNode;

  outNodeString.Truncate();

  
  nsCOMPtr<nsIDocument> doc = node->OwnerDoc();
  mozilla::ErrorResult rv;
  nsRefPtr<nsRange> range = doc->CreateRange(rv);
  if (range) {
    range->SelectNode(*node, rv);
    range->ToString(outNodeString);
  }
}

nsresult
DragDataProducer::Produce(DataTransfer* aDataTransfer,
                          bool* aCanDrag,
                          nsISelection** aSelection,
                          nsIContent** aDragNode)
{
  NS_PRECONDITION(aCanDrag && aSelection && aDataTransfer && aDragNode,
                  "null pointer passed to Produce");
  NS_ASSERTION(mWindow, "window not set");
  NS_ASSERTION(mSelectionTargetNode, "selection target node should have been set");

  *aDragNode = nullptr;

  nsresult rv;
  nsIContent* dragNode = nullptr;
  *aSelection = nullptr;

  
  
  
  nsCOMPtr<nsISelection> selection;
  nsIContent* editingElement = mSelectionTargetNode->IsEditable() ?
                               mSelectionTargetNode->GetEditingHost() : nullptr;
  nsCOMPtr<nsITextControlElement> textControl =
    nsITextControlElement::GetTextControlElementFromEditingHost(editingElement);
  if (textControl) {
    nsISelectionController* selcon = textControl->GetSelectionController();
    if (selcon) {
      selcon->GetSelection(nsISelectionController::SELECTION_NORMAL, getter_AddRefs(selection));
    }

    if (!selection)
      return NS_OK;
  }
  else {
    mWindow->GetSelection(getter_AddRefs(selection));
    if (!selection)
      return NS_OK;

    
    
    nsCOMPtr<nsIContent> findFormNode = mSelectionTargetNode;
    nsIContent* findFormParent = findFormNode->GetParent();
    while (findFormParent) {
      nsCOMPtr<nsIFormControl> form(do_QueryInterface(findFormParent));
      if (form && !form->AllowDraggableChildren()) {
        return NS_OK;
      }
      findFormParent = findFormParent->GetParent();
    }
  }
    
  
  nsCOMPtr<nsIContent> nodeToSerialize;

  nsCOMPtr<nsIDocShellTreeItem> dsti = mWindow->GetDocShell();
  const bool isChromeShell =
    dsti && dsti->ItemType() == nsIDocShellTreeItem::typeChrome;

  
  if (isChromeShell && !editingElement)
    return NS_OK;

  if (isChromeShell && textControl) {
    
    bool selectionContainsTarget = false;
    nsCOMPtr<nsIDOMNode> targetNode = do_QueryInterface(mSelectionTargetNode);
    selection->ContainsNode(targetNode, false, &selectionContainsTarget);
    if (!selectionContainsTarget)
      return NS_OK;

    selection.swap(*aSelection);
  }
  else {
    
    
    
    

    bool haveSelectedContent = false;

    
    nsCOMPtr<nsIContent> parentLink;
    nsCOMPtr<nsIContent> draggedNode;

    {
      
      

      
      
      nsCOMPtr<nsIFormControl> form(do_QueryInterface(mTarget));
      if (form && !mIsAltKeyPressed && form->GetType() != NS_FORM_OBJECT) {
        *aCanDrag = false;
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
      selection.swap(*aSelection);
    } else if (selectedImageOrLinkNode) {
      
      image = do_QueryInterface(selectedImageOrLinkNode);
    } else {
      
      
      
      
      
      
      parentLink = FindParentLinkNode(draggedNode);
      if (parentLink && mIsAltKeyPressed) {
        *aCanDrag = false;
        return NS_OK;
      }

      area  = do_QueryInterface(draggedNode);
      image = do_QueryInterface(draggedNode);
      link  = do_QueryInterface(draggedNode);
    }

    {
      
      nsCOMPtr<nsIContent> linkNode;

      if (area) {
        
        HTMLAreaElement* areaElem = static_cast<HTMLAreaElement*>(area.get());
        areaElem->GetAttribute(NS_LITERAL_STRING("alt"), mTitleString);
        if (mTitleString.IsEmpty()) {
          
          areaElem->GetAttribute(NS_LITERAL_STRING("href"), mTitleString);
        }

        
        mIsAnchor = true;

        
        GetAnchorURL(draggedNode, mUrlString);

        mHtmlString.AssignLiteral("<a href=\"");
        mHtmlString.Append(mUrlString);
        mHtmlString.AppendLiteral("\">");
        mHtmlString.Append(mTitleString);
        mHtmlString.AppendLiteral("</a>");

        dragNode = draggedNode;
      } else if (image) {
        mIsAnchor = true;
        
        
        
        nsCOMPtr<nsIDOMElement> imageElement(do_QueryInterface(image));
        
        
        if (imageElement) {
          imageElement->GetAttribute(NS_LITERAL_STRING("alt"), mTitleString);
        }

        mUrlString.Truncate();

        
        nsCOMPtr<imgIRequest> imgRequest;
        nsCOMPtr<imgIContainer> img =
          nsContentUtils::GetImageFromContent(image,
                                              getter_AddRefs(imgRequest));

        nsCOMPtr<nsIMIMEService> mimeService =
          do_GetService("@mozilla.org/mime;1");

        
        if (imgRequest && mimeService) {
          nsCOMPtr<nsIURI> imgUri;
          imgRequest->GetCurrentURI(getter_AddRefs(imgUri));

          nsCOMPtr<nsIURL> imgUrl(do_QueryInterface(imgUri));

          if (imgUrl) {
            nsAutoCString extension;
            imgUrl->GetFileExtension(extension);

            nsXPIDLCString mimeType;
            imgRequest->GetMimeType(getter_Copies(mimeType));

            nsCOMPtr<nsIMIMEInfo> mimeInfo;
            mimeService->GetFromTypeAndExtension(mimeType, EmptyCString(),
                                                 getter_AddRefs(mimeInfo));

            if (mimeInfo) {
              nsAutoCString spec;
              imgUrl->GetSpec(spec);

              
              CopyUTF8toUTF16(spec, mImageSourceString);
              mUrlString = mImageSourceString;

              bool validExtension;
              if (extension.IsEmpty() || 
                  NS_FAILED(mimeInfo->ExtensionExists(extension,
                                                      &validExtension)) ||
                  !validExtension) {
                
                nsresult rv = imgUrl->Clone(getter_AddRefs(imgUri));
                NS_ENSURE_SUCCESS(rv, rv);

                imgUrl = do_QueryInterface(imgUri);

                nsAutoCString primaryExtension;
                mimeInfo->GetPrimaryExtension(primaryExtension);

                imgUrl->SetFileExtension(primaryExtension);
              }

              nsAutoCString fileName;
              imgUrl->GetFileName(fileName);

              NS_UnescapeURL(fileName);

              
              fileName.ReplaceChar(FILE_PATH_SEPARATOR FILE_ILLEGAL_CHARACTERS,
                                   '-');

              CopyUTF8toUTF16(fileName, mImageDestFileName);

              
              mImage = img;
            }
          }
        }
        if (mUrlString.IsEmpty()) {
          nsCOMPtr<nsIURI> imageURI;
          image->GetCurrentURI(getter_AddRefs(imageURI));
          if (imageURI) {
            nsAutoCString spec;
            imageURI->GetSpec(spec);
            CopyUTF8toUTF16(spec, mUrlString);
          }
        }
        if (mTitleString.IsEmpty()) {
          mTitleString = mUrlString;
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
        mIsAnchor = true;
        GetAnchorURL(linkNode, mUrlString);
        dragNode = linkNode;
      }
    }
  }

  if (nodeToSerialize || *aSelection) {
    mHtmlString.Truncate();
    mContextString.Truncate();
    mInfoString.Truncate();
    mTitleString.Truncate();

    nsCOMPtr<nsIDocument> doc = mWindow->GetDoc();
    NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);

    
    nsCOMPtr<nsITransferable> transferable;
    if (*aSelection) {
      rv = nsCopySupport::GetTransferableForSelection(*aSelection, doc,
                                                      getter_AddRefs(transferable));
    }
    else {
      rv = nsCopySupport::GetTransferableForNode(nodeToSerialize, doc,
                                                 getter_AddRefs(transferable));
    }
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsISupports> supports;
    nsCOMPtr<nsISupportsString> data;
    uint32_t dataSize;
    rv = transferable->GetTransferData(kHTMLMime, getter_AddRefs(supports),
                                       &dataSize);
    data = do_QueryInterface(supports);
    if (NS_SUCCEEDED(rv)) {
      data->GetData(mHtmlString);
    }
    rv = transferable->GetTransferData(kHTMLContext, getter_AddRefs(supports),
                                       &dataSize);
    data = do_QueryInterface(supports);
    if (NS_SUCCEEDED(rv)) {
      data->GetData(mContextString);
    }
    rv = transferable->GetTransferData(kHTMLInfo, getter_AddRefs(supports),
                                       &dataSize);
    data = do_QueryInterface(supports);
    if (NS_SUCCEEDED(rv)) {
      data->GetData(mInfoString);
    }
    rv = transferable->GetTransferData(kUnicodeMime, getter_AddRefs(supports),
                                       &dataSize);
    data = do_QueryInterface(supports);
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
DragDataProducer::AddString(DataTransfer* aDataTransfer,
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
                                           DataTransfer* aDataTransfer)
{
  NS_ASSERTION(aDragNode, "adding strings for null node");

  
  nsIPrincipal* principal = aDragNode->NodePrincipal();

  
  
  if (!mUrlString.IsEmpty() && mIsAnchor) {
    nsAutoString dragData(mUrlString);
    dragData.Append('\n');
    
    
    
    nsAutoString title(mTitleString);
    title.Trim("\r\n");
    title.ReplaceChar("\r\n", ' ');
    dragData += title;

    AddString(aDataTransfer, NS_LITERAL_STRING(kURLMime), dragData, principal);
    AddString(aDataTransfer, NS_LITERAL_STRING(kURLDataMime), mUrlString, principal);
    AddString(aDataTransfer, NS_LITERAL_STRING(kURLDescriptionMime), mTitleString, principal);
    AddString(aDataTransfer, NS_LITERAL_STRING("text/uri-list"), mUrlString, principal);
  }

  
  if (!mContextString.IsEmpty())
    AddString(aDataTransfer, NS_LITERAL_STRING(kHTMLContext), mContextString, principal);

  
  if (!mInfoString.IsEmpty())
    AddString(aDataTransfer, NS_LITERAL_STRING(kHTMLInfo), mInfoString, principal);

  
  if (!mHtmlString.IsEmpty())
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
                                            bool* outDragSelectedText)
{
  NS_ENSURE_ARG(inSelection);
  NS_ENSURE_ARG(inRealTargetNode);
  NS_ENSURE_ARG_POINTER(outImageOrLinkNode);

  *outImageOrLinkNode = nullptr;
  *outDragSelectedText = false;

  bool selectionContainsTarget = false;

  bool isCollapsed = false;
  inSelection->GetIsCollapsed(&isCollapsed);
  if (!isCollapsed) {
    nsCOMPtr<nsIDOMNode> realTargetNode = do_QueryInterface(inRealTargetNode);
    inSelection->ContainsNode(realTargetNode, false,
                              &selectionContainsTarget);

    if (selectionContainsTarget) {
      
      nsCOMPtr<nsIDOMNode> selectionStart;
      inSelection->GetAnchorNode(getter_AddRefs(selectionStart));

      nsCOMPtr<nsIDOMNode> selectionEnd;
      inSelection->GetFocusNode(getter_AddRefs(selectionEnd));

      
      
      
      if (selectionStart == selectionEnd) {
        bool hasChildren;
        selectionStart->HasChildNodes(&hasChildren);
        if (hasChildren) {
          
          int32_t anchorOffset, focusOffset;
          inSelection->GetAnchorOffset(&anchorOffset);
          inSelection->GetFocusOffset(&focusOffset);
          if (abs(anchorOffset - focusOffset) == 1) {
            nsCOMPtr<nsIContent> selStartContent =
              do_QueryInterface(selectionStart);

            if (selStartContent) {
              int32_t childOffset =
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

      
      *outDragSelectedText = true;
    }
  }

  return NS_OK;
}
