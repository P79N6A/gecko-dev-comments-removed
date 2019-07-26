


































































#include "mozilla/mozalloc.h"
#include "nsAString.h"
#include "nsDebug.h"
#include "nsError.h"
#include "nsHTMLURIRefObject.h"
#include "nsID.h"
#include "nsIDOMAttr.h"
#include "nsIDOMElement.h"
#include "nsIDOMMozNamedAttrMap.h"
#include "nsIDOMNode.h"
#include "nsISupportsUtils.h"
#include "nsString.h"



#define MATCHES(tagName, str) tagName.EqualsIgnoreCase(str)

nsHTMLURIRefObject::nsHTMLURIRefObject()
{
  mCurAttrIndex = mAttributeCnt = 0;
}

nsHTMLURIRefObject::~nsHTMLURIRefObject()
{
}


NS_IMPL_ISUPPORTS1(nsHTMLURIRefObject, nsIURIRefObject)

NS_IMETHODIMP
nsHTMLURIRefObject::Reset()
{
  mCurAttrIndex = 0;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLURIRefObject::GetNextURI(nsAString & aURI)
{
  NS_ENSURE_TRUE(mNode, NS_ERROR_NOT_INITIALIZED);

  nsAutoString tagName;
  nsresult rv = mNode->GetNodeName(tagName);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (!mAttributes)
  {
    nsCOMPtr<nsIDOMElement> element (do_QueryInterface(mNode));
    NS_ENSURE_TRUE(element, NS_ERROR_INVALID_ARG);

    mCurAttrIndex = 0;
    element->GetAttributes(getter_AddRefs(mAttributes));
    NS_ENSURE_TRUE(mAttributes, NS_ERROR_NOT_INITIALIZED);

    rv = mAttributes->GetLength(&mAttributeCnt);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(mAttributeCnt, NS_ERROR_FAILURE);
    mCurAttrIndex = 0;
  }
#ifdef DEBUG_akkana
  printf("Looking at tag '%s'\n",
         NS_LossyConvertUTF16toASCII(tagName).get());
#endif
  while (mCurAttrIndex < mAttributeCnt)
  {
    nsCOMPtr<nsIDOMAttr> attrNode;
    rv = mAttributes->Item(mCurAttrIndex++, getter_AddRefs(attrNode));
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_ARG_POINTER(attrNode);
    nsString curAttr;
    rv = attrNode->GetName(curAttr);
    NS_ENSURE_SUCCESS(rv, rv);

    
#ifdef DEBUG_akkana
    printf("Trying to match attribute '%s'\n",
           NS_LossyConvertUTF16toASCII(curAttr).get());
#endif
    if (MATCHES(curAttr, "href"))
    {
      if (!MATCHES(tagName, "a") && !MATCHES(tagName, "area")
          && !MATCHES(tagName, "base") && !MATCHES(tagName, "link"))
        continue;
      rv = attrNode->GetValue(aURI);
      NS_ENSURE_SUCCESS(rv, rv);
      nsString uri (aURI);
      
      if (aURI.First() != PRUnichar('#'))
        return NS_OK;
      aURI.Truncate();
      return NS_ERROR_INVALID_ARG;
    }
    
    else if (MATCHES(curAttr, "src"))
    {
      if (!MATCHES(tagName, "img")
          && !MATCHES(tagName, "frame") && !MATCHES(tagName, "iframe")
          && !MATCHES(tagName, "input") && !MATCHES(tagName, "script"))
        continue;
      return attrNode->GetValue(aURI);
    }
    
    else if (MATCHES(curAttr, "content"))
    {
      if (!MATCHES(tagName, "meta"))
        continue;
    }
    
    else if (MATCHES(curAttr, "longdesc"))
    {
      if (!MATCHES(tagName, "img")
          && !MATCHES(tagName, "frame") && !MATCHES(tagName, "iframe"))
        continue;
    }
    
    else if (MATCHES(curAttr, "usemap"))
    {
      if (!MATCHES(tagName, "img")
          && !MATCHES(tagName, "input") && !MATCHES(tagName, "object"))
        continue;
    }
    
    else if (MATCHES(curAttr, "action"))
    {
      if (!MATCHES(tagName, "form"))
        continue;
    }
    
    else if (MATCHES(curAttr, "background"))
    {
      if (!MATCHES(tagName, "body"))
        continue;
    }
    
    else if (MATCHES(curAttr, "codebase"))
    {
      if (!MATCHES(tagName, "meta"))
        continue;
    }
    
    else if (MATCHES(curAttr, "classid"))
    {
      if (!MATCHES(tagName, "object"))
        continue;
    }
    
    else if (MATCHES(curAttr, "data"))
    {
      if (!MATCHES(tagName, "object"))
        continue;
    }
    
    else if (MATCHES(curAttr, "cite"))
    {
      if (!MATCHES(tagName, "blockquote") && !MATCHES(tagName, "q")
          && !MATCHES(tagName, "del") && !MATCHES(tagName, "ins"))
        continue;
    }
    
    else if (MATCHES(curAttr, "profile"))
    {
      if (!MATCHES(tagName, "head"))
        continue;
    }
    
    else if (MATCHES(curAttr, "archive"))
    {
      if (!MATCHES(tagName, "applet"))
        continue;
    }
  }
  
  
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsHTMLURIRefObject::RewriteAllURIs(const nsAString & aOldPat,
                            const nsAString & aNewPat,
                            bool aMakeRel)
{
#ifdef DEBUG_akkana
  printf("Can't rewrite URIs yet\n");
#endif
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHTMLURIRefObject::GetNode(nsIDOMNode** aNode)
{
  NS_ENSURE_TRUE(mNode, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(aNode, NS_ERROR_NULL_POINTER);
  *aNode = mNode.get();
  NS_ADDREF(*aNode);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLURIRefObject::SetNode(nsIDOMNode *aNode)
{
  mNode = aNode;
  nsAutoString dummyURI;
  if (NS_SUCCEEDED(GetNextURI(dummyURI)))
  {
    mCurAttrIndex = 0;    
    return NS_OK;
  }

  
  
  mNode = 0;
  return NS_ERROR_INVALID_ARG;
}

nsresult NS_NewHTMLURIRefObject(nsIURIRefObject** aResult, nsIDOMNode* aNode)
{
  nsHTMLURIRefObject* refObject = new nsHTMLURIRefObject();
  nsresult rv = refObject->SetNode(aNode);
  if (NS_FAILED(rv)) {
    *aResult = 0;
    delete refObject;
    return rv;
  }
  return refObject->QueryInterface(NS_GET_IID(nsIURIRefObject),
                                   (void**)aResult);
}

