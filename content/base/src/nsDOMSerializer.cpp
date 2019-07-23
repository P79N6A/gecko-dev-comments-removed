




































#include "nsDOMSerializer.h"
#include "nsIDOMNode.h"
#include "nsIDOMClassInfo.h"
#include "nsIOutputStream.h"
#include "nsINode.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDocumentEncoder.h"
#include "nsIContentSerializer.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsContentCID.h"
#include "nsContentUtils.h"
#include "nsDOMError.h"

nsDOMSerializer::nsDOMSerializer()
{
}

nsDOMSerializer::~nsDOMSerializer()
{
}



NS_INTERFACE_MAP_BEGIN(nsDOMSerializer)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSerializer)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(XMLSerializer)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(nsDOMSerializer)
NS_IMPL_RELEASE(nsDOMSerializer)


static nsresult
SetUpEncoder(nsIDOMNode *aRoot, const nsACString& aCharset,
             nsIDocumentEncoder **aEncoder)
{
  *aEncoder = nsnull;
   
  nsresult rv;
  nsCOMPtr<nsIDocumentEncoder> encoder =
    do_CreateInstance(NS_DOC_ENCODER_CONTRACTID_BASE "application/xhtml+xml", &rv);
  if (NS_FAILED(rv))
    return rv;

  PRBool entireDocument = PR_TRUE;
  nsCOMPtr<nsIDOMDocument> domDoc(do_QueryInterface(aRoot));
  if (!domDoc) {
    entireDocument = PR_FALSE;
    rv = aRoot->GetOwnerDocument(getter_AddRefs(domDoc));
    if (NS_FAILED(rv))
      return rv;
  }

  
  rv = encoder->Init(domDoc, NS_LITERAL_STRING("application/xhtml+xml"),
                     nsIDocumentEncoder::OutputRaw |
                     nsIDocumentEncoder::OutputDontRewriteEncodingDeclaration);

  if (NS_FAILED(rv))
    return rv;

  nsCAutoString charset(aCharset);
  if (charset.IsEmpty()) {
    nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);
    NS_ASSERTION(doc, "Need a document");
    charset = doc->GetDocumentCharacterSet();
  }
  rv = encoder->SetCharset(charset);
  if (NS_FAILED(rv))
    return rv;

  
  
  if (!entireDocument) {
    rv = encoder->SetNode(aRoot);
  }

  if (NS_SUCCEEDED(rv)) {
    *aEncoder = encoder.get();
    NS_ADDREF(*aEncoder);
  }

  return rv;
}

NS_IMETHODIMP
nsDOMSerializer::SerializeToString(nsIDOMNode *aRoot, nsAString& _retval)
{
  NS_ENSURE_ARG_POINTER(aRoot);
  
  _retval.Truncate();

  if (!nsContentUtils::CanCallerAccess(aRoot)) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  nsCOMPtr<nsIDocumentEncoder> encoder;
  nsresult rv = SetUpEncoder(aRoot, EmptyCString(), getter_AddRefs(encoder));
  if (NS_FAILED(rv))
    return rv;

  return encoder->EncodeToString(_retval);
}

NS_IMETHODIMP
nsDOMSerializer::SerializeToStream(nsIDOMNode *aRoot, 
                                   nsIOutputStream *aStream, 
                                   const nsACString& aCharset)
{
  NS_ENSURE_ARG_POINTER(aRoot);
  NS_ENSURE_ARG_POINTER(aStream);
  
  

  if (!nsContentUtils::CanCallerAccess(aRoot)) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  nsCOMPtr<nsIDocumentEncoder> encoder;
  nsresult rv = SetUpEncoder(aRoot, aCharset, getter_AddRefs(encoder));
  if (NS_FAILED(rv))
    return rv;

  return encoder->EncodeToStream(aStream);
}
