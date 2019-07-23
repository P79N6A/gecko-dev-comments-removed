








































#include "nsContextMenuInfo.h"

#include "nsIImageLoadingContent.h"
#include "imgILoader.h"
#include "nsIDOMDocument.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMHTMLElement.h"
#include "nsIDOMHTMLHtmlElement.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsIDOMHTMLAreaElement.h"
#include "nsIDOMHTMLLinkElement.h"
#include "nsIDOMDocumentView.h"
#include "nsIDOMAbstractView.h"
#include "nsIDOMViewCSS.h"
#include "nsIDOMCSSStyleDeclaration.h"
#include "nsIDOMCSSValue.h"
#include "nsIDOMCSSPrimitiveValue.h"
#include "nsNetUtil.h"
#include "nsUnicharUtils.h"





NS_IMPL_ISUPPORTS1(nsContextMenuInfo, nsIContextMenuInfo)

nsContextMenuInfo::nsContextMenuInfo()
{
}

nsContextMenuInfo::~nsContextMenuInfo()
{
}


NS_IMETHODIMP
nsContextMenuInfo::GetMouseEvent(nsIDOMEvent **aEvent)
{
  NS_ENSURE_ARG_POINTER(aEvent);
  NS_IF_ADDREF(*aEvent = mMouseEvent);
  return NS_OK;
}


NS_IMETHODIMP
nsContextMenuInfo::GetTargetNode(nsIDOMNode **aNode)
{
  NS_ENSURE_ARG_POINTER(aNode);
  NS_IF_ADDREF(*aNode = mDOMNode);
  return NS_OK;
}


NS_IMETHODIMP
nsContextMenuInfo::GetAssociatedLink(nsAString& aHRef)
{
  NS_ENSURE_STATE(mAssociatedLink);
  aHRef.Truncate(0);
    
  nsCOMPtr<nsIDOMElement> content(do_QueryInterface(mAssociatedLink));
  nsAutoString localName;
  if (content)
    content->GetLocalName(localName);

  nsCOMPtr<nsIDOMElement> linkContent;
  ToLowerCase(localName);
  if (localName.EqualsLiteral("a") ||
      localName.EqualsLiteral("area") ||
      localName.EqualsLiteral("link")) {
    PRBool hasAttr;
    content->HasAttribute(NS_LITERAL_STRING("href"), &hasAttr);
    if (hasAttr) {
      linkContent = content;
      nsCOMPtr<nsIDOMHTMLAnchorElement> anchor(do_QueryInterface(linkContent));
      if (anchor)
        anchor->GetHref(aHRef);
      else {
        nsCOMPtr<nsIDOMHTMLAreaElement> area(do_QueryInterface(linkContent));
        if (area)
          area->GetHref(aHRef);
        else {
          nsCOMPtr<nsIDOMHTMLLinkElement> link(do_QueryInterface(linkContent));
          if (link)
            link->GetHref(aHRef);
        }
      }
    }
  }
  else {
    nsCOMPtr<nsIDOMNode> curr;
    mAssociatedLink->GetParentNode(getter_AddRefs(curr));
    while (curr) {
      content = do_QueryInterface(curr);
      if (!content)
        break;
      content->GetLocalName(localName);
      ToLowerCase(localName);
      if (localName.EqualsLiteral("a")) {
        PRBool hasAttr;
        content->HasAttribute(NS_LITERAL_STRING("href"), &hasAttr);
        if (hasAttr) {
          linkContent = content;
          nsCOMPtr<nsIDOMHTMLAnchorElement> anchor(do_QueryInterface(linkContent));
          if (anchor)
            anchor->GetHref(aHRef);
        }
        else
          linkContent = nsnull; 
        break;
      }

      nsCOMPtr<nsIDOMNode> temp = curr;
      temp->GetParentNode(getter_AddRefs(curr));
    }
  }

  return NS_OK;
}


NS_IMETHODIMP
nsContextMenuInfo::GetImageContainer(imgIContainer **aImageContainer)
{
  NS_ENSURE_ARG_POINTER(aImageContainer);
  NS_ENSURE_STATE(mDOMNode);
  
  nsCOMPtr<imgIRequest> request;
  GetImageRequest(mDOMNode, getter_AddRefs(request));
  if (request)
    return request->GetImage(aImageContainer);

  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
nsContextMenuInfo::GetImageSrc(nsIURI **aURI)
{
  NS_ENSURE_ARG_POINTER(aURI);
  NS_ENSURE_STATE(mDOMNode);
  
  nsCOMPtr<nsIImageLoadingContent> content(do_QueryInterface(mDOMNode));
  NS_ENSURE_TRUE(content, NS_ERROR_FAILURE);
  return content->GetCurrentURI(aURI);
}


NS_IMETHODIMP
nsContextMenuInfo::GetBackgroundImageContainer(imgIContainer **aImageContainer)
{
  NS_ENSURE_ARG_POINTER(aImageContainer);
  NS_ENSURE_STATE(mDOMNode);
  
  nsCOMPtr<imgIRequest> request;
  GetBackgroundImageRequest(mDOMNode, getter_AddRefs(request));
  if (request)
    return request->GetImage(aImageContainer);

  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
nsContextMenuInfo::GetBackgroundImageSrc(nsIURI **aURI)
{
  NS_ENSURE_ARG_POINTER(aURI);
  NS_ENSURE_STATE(mDOMNode);
  
  nsCOMPtr<imgIRequest> request;
  GetBackgroundImageRequest(mDOMNode, getter_AddRefs(request));
  if (request)
    return request->GetURI(aURI);

  return NS_ERROR_FAILURE;
}



nsresult
nsContextMenuInfo::GetImageRequest(nsIDOMNode *aDOMNode, imgIRequest **aRequest)
{
  NS_ENSURE_ARG(aDOMNode);
  NS_ENSURE_ARG_POINTER(aRequest);

  
  nsCOMPtr<nsIImageLoadingContent> content(do_QueryInterface(aDOMNode));
  NS_ENSURE_TRUE(content, NS_ERROR_FAILURE);

  return content->GetRequest(nsIImageLoadingContent::CURRENT_REQUEST,
                             aRequest);
}

PRBool
nsContextMenuInfo::HasBackgroundImage(nsIDOMNode * aDOMNode)
{
  NS_ENSURE_ARG(aDOMNode);

  nsCOMPtr<imgIRequest> request;
  GetBackgroundImageRequest(aDOMNode, getter_AddRefs(request));
  
  return (request != nsnull);
}

nsresult
nsContextMenuInfo::GetBackgroundImageRequest(nsIDOMNode *aDOMNode, imgIRequest **aRequest)
{

  NS_ENSURE_ARG(aDOMNode);
  NS_ENSURE_ARG_POINTER(aRequest);

  nsCOMPtr<nsIDOMNode> domNode = aDOMNode;

  
  
  nsCOMPtr<nsIDOMHTMLHtmlElement> htmlElement = do_QueryInterface(domNode);
  if (htmlElement) {
    nsAutoString nameSpace;
    htmlElement->GetNamespaceURI(nameSpace);
    if (nameSpace.IsEmpty()) {
      nsresult rv = GetBackgroundImageRequestInternal(domNode, aRequest);
      if (NS_SUCCEEDED(rv) && *aRequest)
        return NS_OK;

      
      nsCOMPtr<nsIDOMDocument> document;
      domNode->GetOwnerDocument(getter_AddRefs(document));
      nsCOMPtr<nsIDOMHTMLDocument> htmlDocument(do_QueryInterface(document));
      NS_ENSURE_TRUE(htmlDocument, NS_ERROR_FAILURE);

      nsCOMPtr<nsIDOMHTMLElement> body;
      htmlDocument->GetBody(getter_AddRefs(body));
      domNode = do_QueryInterface(body);
      NS_ENSURE_TRUE(domNode, NS_ERROR_FAILURE);
    }
  }
  return GetBackgroundImageRequestInternal(domNode, aRequest);
}

nsresult
nsContextMenuInfo::GetBackgroundImageRequestInternal(nsIDOMNode *aDOMNode, imgIRequest **aRequest)
{
  NS_ENSURE_ARG_POINTER(aDOMNode);

  nsCOMPtr<nsIDOMNode> domNode = aDOMNode;
  nsCOMPtr<nsIDOMNode> parentNode;

  nsCOMPtr<nsIDOMDocument> document;
  domNode->GetOwnerDocument(getter_AddRefs(document));
  nsCOMPtr<nsIDOMDocumentView> docView(do_QueryInterface(document));
  NS_ENSURE_TRUE(docView, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDOMAbstractView> defaultView;
  docView->GetDefaultView(getter_AddRefs(defaultView));
  nsCOMPtr<nsIDOMViewCSS> defaultCSSView(do_QueryInterface(defaultView));
  NS_ENSURE_TRUE(defaultCSSView, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDOMCSSPrimitiveValue> primitiveValue;
  nsAutoString bgStringValue;

  while (PR_TRUE) {
    nsCOMPtr<nsIDOMElement> domElement(do_QueryInterface(domNode));
    
    if (!domElement)
      break;
    
    nsCOMPtr<nsIDOMCSSStyleDeclaration> computedStyle;
    defaultCSSView->GetComputedStyle(domElement, EmptyString(),
                                     getter_AddRefs(computedStyle));
    if (computedStyle) {
      nsCOMPtr<nsIDOMCSSValue> cssValue;
      computedStyle->GetPropertyCSSValue(NS_LITERAL_STRING("background-image"),
                                         getter_AddRefs(cssValue));
      primitiveValue = do_QueryInterface(cssValue);
      if (primitiveValue) {
        primitiveValue->GetStringValue(bgStringValue);
        if (!bgStringValue.EqualsLiteral("none")) {
          nsCOMPtr<nsIURI> bgUri;
          NS_NewURI(getter_AddRefs(bgUri), bgStringValue);
          NS_ENSURE_TRUE(bgUri, NS_ERROR_FAILURE);

          nsCOMPtr<imgILoader> il(do_GetService(
                                    "@mozilla.org/image/loader;1"));
          NS_ENSURE_TRUE(il, NS_ERROR_FAILURE);

          return il->LoadImage(bgUri, nsnull, nsnull, nsnull, nsnull, nsnull,
                               nsIRequest::LOAD_NORMAL, nsnull, nsnull,
                               aRequest);
        }
      }

      
      computedStyle->GetPropertyCSSValue(NS_LITERAL_STRING("background-color"),
                                         getter_AddRefs(cssValue));
      primitiveValue = do_QueryInterface(cssValue);
      if (primitiveValue) {
        primitiveValue->GetStringValue(bgStringValue);
        if (!bgStringValue.EqualsLiteral("transparent"))
          return NS_ERROR_FAILURE;
      }
    }

    domNode->GetParentNode(getter_AddRefs(parentNode));
    domNode = parentNode;
  }

  return NS_ERROR_FAILURE;
}
