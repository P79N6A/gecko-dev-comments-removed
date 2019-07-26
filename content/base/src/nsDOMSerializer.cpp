




#include "nsDOMSerializer.h"
#include "nsIDOMNode.h"
#include "nsDOMClassInfoID.h"
#include "nsIOutputStream.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDocumentEncoder.h"
#include "nsString.h"
#include "nsContentCID.h"
#include "nsContentUtils.h"
#include "nsError.h"

using namespace mozilla;

nsDOMSerializer::nsDOMSerializer()
{
}

nsDOMSerializer::~nsDOMSerializer()
{
}

DOMCI_DATA(XMLSerializer, nsDOMSerializer)


NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsDOMSerializer)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSerializer)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(XMLSerializer)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_1(nsDOMSerializer, mOwner)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsDOMSerializer)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsDOMSerializer)


static nsresult
SetUpEncoder(nsIDOMNode *aRoot, const nsACString& aCharset,
             nsIDocumentEncoder **aEncoder)
{
  *aEncoder = nullptr;
   
  nsresult rv;
  nsCOMPtr<nsIDocumentEncoder> encoder =
    do_CreateInstance(NS_DOC_ENCODER_CONTRACTID_BASE "application/xhtml+xml", &rv);
  if (NS_FAILED(rv))
    return rv;

  bool entireDocument = true;
  nsCOMPtr<nsIDOMDocument> domDoc(do_QueryInterface(aRoot));
  if (!domDoc) {
    entireDocument = false;
    rv = aRoot->GetOwnerDocument(getter_AddRefs(domDoc));
    if (NS_FAILED(rv))
      return rv;
  }

  
  rv = encoder->Init(domDoc, NS_LITERAL_STRING("application/xhtml+xml"),
                     nsIDocumentEncoder::OutputRaw |
                     nsIDocumentEncoder::OutputDontRewriteEncodingDeclaration);

  if (NS_FAILED(rv))
    return rv;

  nsAutoCString charset(aCharset);
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

void
nsDOMSerializer::SerializeToString(nsINode& aRoot, nsAString& aStr,
                                   ErrorResult& rv)
{
  rv = nsDOMSerializer::SerializeToString(aRoot.AsDOMNode(), aStr);
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

void
nsDOMSerializer::SerializeToStream(nsINode& aRoot, nsIOutputStream* aStream,
                                   const nsAString& aCharset, ErrorResult& rv)
{
  rv = nsDOMSerializer::SerializeToStream(aRoot.AsDOMNode(), aStream,
                                          NS_ConvertUTF16toUTF8(aCharset));
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
