


































































































#include "nsHTMLURIRefObject.h"

#include "nsAString.h"
#include "nsString.h"
#include "nsIDOMAttr.h"
#include "nsIDOMElement.h"



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
  if (!mNode)
    return NS_ERROR_NOT_INITIALIZED;

  nsAutoString tagName;
  nsresult rv = mNode->GetNodeName(tagName);
  if (NS_FAILED(rv))
  return rv;

  
  if (!mAttributes)
  {
    nsCOMPtr<nsIDOMElement> element (do_QueryInterface(mNode));
    if (!element)
      return NS_ERROR_INVALID_ARG;

    mCurAttrIndex = 0;
    mNode->GetAttributes(getter_AddRefs(mAttributes));
    if (!mAttributes)
      return NS_ERROR_NOT_INITIALIZED;

    rv = mAttributes->GetLength(&mAttributeCnt);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!mAttributeCnt) return NS_ERROR_FAILURE;
    mCurAttrIndex = 0;
  }
#ifdef DEBUG_akkana
  printf("Looking at tag '%s'\n",
         NS_LossyConvertUTF16toASCII(tagName).get());
#endif
  while (mCurAttrIndex < mAttributeCnt)
  {
    nsCOMPtr<nsIDOMNode> attrNode;
    rv = mAttributes->Item(mCurAttrIndex++, getter_AddRefs(attrNode));
      
      
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_ARG_POINTER(attrNode);
    nsCOMPtr<nsIDOMAttr> curAttrNode (do_QueryInterface(attrNode));
    NS_ENSURE_ARG_POINTER(curAttrNode);
    nsString curAttr;
    rv = curAttrNode->GetName(curAttr);
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
      rv = curAttrNode->GetValue(aURI);
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
      return curAttrNode->GetValue(aURI);
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
                            PRBool aMakeRel)
{
#ifdef DEBUG_akkana
  printf("Can't rewrite URIs yet\n");
#endif
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHTMLURIRefObject::GetNode(nsIDOMNode** aNode)
{
  if (!mNode)
    return NS_ERROR_NOT_INITIALIZED;
  if (!aNode)
    return NS_ERROR_NULL_POINTER;
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
  if (!refObject) return NS_ERROR_OUT_OF_MEMORY;
  nsresult rv = refObject->SetNode(aNode);
  if (NS_FAILED(rv)) {
    *aResult = 0;
    delete refObject;
    return rv;
  }
  return refObject->QueryInterface(NS_GET_IID(nsIURIRefObject),
                                   (void**)aResult);
}

