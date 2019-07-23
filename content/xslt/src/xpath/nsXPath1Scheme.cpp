












































#include "nsXPath1Scheme.h"
#include "nsXPathEvaluator.h"
#include "nsDOMError.h"
#include "nsXPathResult.h"
#include "nsIDOMNode.h"
#include "nsIDOMDocument.h"
#include "nsIDOMXPathNSResolver.h"
#include "nsIDOMRange.h"
#include "nsDOMString.h"
#include "nsIModifyableXPointer.h"
#include "nsAutoPtr.h"
#include "nsString.h"
#include "nsIComponentManager.h"

#include "nsContentCID.h"
static NS_DEFINE_IID(kRangeCID, NS_RANGE_CID);






class nsXPath1SchemeNSResolver : public nsIDOMXPathNSResolver
{
public:
  nsXPath1SchemeNSResolver(nsIXPointerSchemeContext *aContext)
    : mContext(aContext)
  {
  }
  
  virtual ~nsXPath1SchemeNSResolver()
  {
  }

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMXPATHNSRESOLVER

private:
  nsCOMPtr<nsIXPointerSchemeContext> mContext;
};

NS_IMPL_ISUPPORTS1(nsXPath1SchemeNSResolver, nsIDOMXPathNSResolver)


NS_IMETHODIMP
nsXPath1SchemeNSResolver::LookupNamespaceURI(const nsAString &aPrefix,
                                             nsAString &aURI)
{
  aURI.Truncate();

  
  
  
  
  
  
  
  

  if (!mContext) {
    return NS_OK;
  }

  NS_NAMED_LITERAL_STRING(xmlns, "xmlns");

  PRUint32 count;
  mContext->GetCount(&count);
  PRUint32 i;
  for (i = 0; i < count; ++i) {
    nsAutoString scheme, data;
    mContext->GetSchemeData(i, scheme, data);
    if (scheme.Equals(xmlns)) {
      PRInt32 sep = data.FindChar('=');
      if (sep > 0 && aPrefix.Equals(StringHead(data, sep))) {
        aURI.Assign(Substring(data, sep + 1, data.Length() - sep - 1));
        return NS_OK;
      }
    }
  }

  SetDOMStringToNull(aURI);

  return NS_OK;
}


nsXPath1SchemeProcessor::nsXPath1SchemeProcessor()
{
}

nsXPath1SchemeProcessor::~nsXPath1SchemeProcessor()
{
}

NS_IMPL_ISUPPORTS1(nsXPath1SchemeProcessor, nsIXPointerSchemeProcessor)









NS_IMETHODIMP
nsXPath1SchemeProcessor::Evaluate(nsIDOMDocument *aDocument,
                         nsIXPointerSchemeContext *aContext,
                         const nsAString &aData,
                         nsIXPointerResult **aResult)
{
  NS_ENSURE_ARG_POINTER(aDocument);
  NS_ENSURE_ARG_POINTER(aContext);
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = nsnull;
 
  
  nsCOMPtr<nsIDOMXPathNSResolver> nsresolver(new nsXPath1SchemeNSResolver(aContext));
  if (!nsresolver) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  nsRefPtr<nsXPathEvaluator> e(new nsXPathEvaluator(nsnull));
  if (!e) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsresult rv = e->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMXPathResult> result;
  rv = e->Evaluate(aData,
                   aDocument,
                   nsresolver,
                   nsIDOMXPathResult::ORDERED_NODE_ITERATOR_TYPE,
                   nsnull,
                   getter_AddRefs(result));
  if (NS_FAILED(rv)) {
    if ((rv == NS_ERROR_DOM_INVALID_EXPRESSION_ERR) ||
        (rv == NS_ERROR_DOM_NAMESPACE_ERR) ||
        (rv == NS_ERROR_DOM_TYPE_ERR)) {
      
      
      rv = NS_OK;
    }
    return rv;
  }

  
  
  nsCOMPtr<nsIXPointerResult> xpointerResult(
    do_CreateInstance("@mozilla.org/xmlextras/xpointerresult;1", &rv));
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsCOMPtr<nsIModifyableXPointerResult> privatePointerResult(do_QueryInterface(xpointerResult));
  if (!privatePointerResult) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIDOMNode> node;
  rv = result->IterateNext(getter_AddRefs(node));
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  while (node) {
    nsCOMPtr<nsIDOMRange> range(do_CreateInstance(kRangeCID, &rv));
    if (NS_FAILED(rv))
      break;

    rv = range->SelectNode(node);
    if (NS_FAILED(rv))
      break;

    rv = privatePointerResult->AppendRange(range);
    if (NS_FAILED(rv))
      break;

    rv = result->IterateNext(getter_AddRefs(node));
    if (NS_FAILED(rv))
      break;
  }

  PRUint32 count;
  xpointerResult->GetLength(&count);
  if (NS_SUCCEEDED(rv) && (count > 0)) {
    *aResult = xpointerResult;
    NS_ADDREF(*aResult);
  }

  return rv;
}
