




#include "mozilla/dom/DOMParser.h"

#include "nsIDOMDocument.h"
#include "nsNetUtil.h"
#include "nsStringStream.h"
#include "nsIScriptSecurityManager.h"
#include "nsCRT.h"
#include "nsStreamUtils.h"
#include "nsContentUtils.h"
#include "nsDOMJSUtils.h"
#include "nsError.h"
#include "nsPIDOMWindow.h"
#include "nsNullPrincipal.h"
#include "mozilla/LoadInfo.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/ScriptSettings.h"

using namespace mozilla;
using namespace mozilla::dom;

DOMParser::DOMParser()
  : mAttemptedInit(false)
{
}

DOMParser::~DOMParser()
{
}


NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DOMParser)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMParser)
  NS_INTERFACE_MAP_ENTRY(nsIDOMParser)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(DOMParser, mOwner)

NS_IMPL_CYCLE_COLLECTING_ADDREF(DOMParser)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DOMParser)

static const char*
StringFromSupportedType(SupportedType aType)
{
  return SupportedTypeValues::strings[static_cast<int>(aType)].value;
}

already_AddRefed<nsIDocument>
DOMParser::ParseFromString(const nsAString& aStr, SupportedType aType,
                           ErrorResult& rv)
{
  nsCOMPtr<nsIDOMDocument> domDocument;
  rv = ParseFromString(aStr,
                       StringFromSupportedType(aType),
                       getter_AddRefs(domDocument));
  nsCOMPtr<nsIDocument> document(do_QueryInterface(domDocument));
  return document.forget();
}

NS_IMETHODIMP 
DOMParser::ParseFromString(const char16_t *str, 
                           const char *contentType,
                           nsIDOMDocument **aResult)
{
  NS_ENSURE_ARG(str);
  
  
  return ParseFromString(nsDependentString(str), contentType, aResult);
}

nsresult
DOMParser::ParseFromString(const nsAString& str,
                           const char *contentType,
                           nsIDOMDocument **aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);

  nsresult rv;

  if (!nsCRT::strcmp(contentType, "text/html")) {
    nsCOMPtr<nsIDOMDocument> domDocument;
    rv = SetUpDocument(DocumentFlavorHTML, getter_AddRefs(domDocument));
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIDocument> document = do_QueryInterface(domDocument);

    
    

    if (nsContentUtils::IsSystemPrincipal(mOriginalPrincipal)) {
      document->ForceEnableXULXBL();
    }

    
    document->SetBaseURI(mBaseURI);
    
    document->SetPrincipal(mPrincipal);

    rv = nsContentUtils::ParseDocumentHTML(str, document, false);
    NS_ENSURE_SUCCESS(rv, rv);

    domDocument.forget(aResult);
    return rv;
  }

  nsAutoCString utf8str;
  
  if (!AppendUTF16toUTF8(str, utf8str, mozilla::fallible)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  nsCOMPtr<nsIInputStream> stream;
  rv = NS_NewByteInputStream(getter_AddRefs(stream),
                             utf8str.get(), utf8str.Length(),
                             NS_ASSIGNMENT_DEPEND);
  if (NS_FAILED(rv))
    return rv;

  return ParseFromStream(stream, "UTF-8", utf8str.Length(), contentType, aResult);
}

already_AddRefed<nsIDocument>
DOMParser::ParseFromBuffer(const Sequence<uint8_t>& aBuf, uint32_t aBufLen,
                           SupportedType aType, ErrorResult& rv)
{
  if (aBufLen > aBuf.Length()) {
    rv.Throw(NS_ERROR_XPC_NOT_ENOUGH_ELEMENTS_IN_ARRAY);
    return nullptr;
  }
  nsCOMPtr<nsIDOMDocument> domDocument;
  rv = DOMParser::ParseFromBuffer(aBuf.Elements(), aBufLen,
                                  StringFromSupportedType(aType),
                                  getter_AddRefs(domDocument));
  nsCOMPtr<nsIDocument> document(do_QueryInterface(domDocument));
  return document.forget();
}

already_AddRefed<nsIDocument>
DOMParser::ParseFromBuffer(const Uint8Array& aBuf, uint32_t aBufLen,
                           SupportedType aType, ErrorResult& rv)
{
  aBuf.ComputeLengthAndData();

  if (aBufLen > aBuf.Length()) {
    rv.Throw(NS_ERROR_XPC_NOT_ENOUGH_ELEMENTS_IN_ARRAY);
    return nullptr;
  }
  nsCOMPtr<nsIDOMDocument> domDocument;
  rv = DOMParser::ParseFromBuffer(aBuf.Data(), aBufLen,
                                    StringFromSupportedType(aType),
                                    getter_AddRefs(domDocument));
  nsCOMPtr<nsIDocument> document(do_QueryInterface(domDocument));
  return document.forget();
}

NS_IMETHODIMP 
DOMParser::ParseFromBuffer(const uint8_t *buf,
                           uint32_t bufLen,
                           const char *contentType,
                           nsIDOMDocument **aResult)
{
  NS_ENSURE_ARG_POINTER(buf);
  NS_ENSURE_ARG_POINTER(aResult);

  
  nsCOMPtr<nsIInputStream> stream;
  nsresult rv = NS_NewByteInputStream(getter_AddRefs(stream),
                                      reinterpret_cast<const char *>(buf),
                                      bufLen, NS_ASSIGNMENT_DEPEND);
  if (NS_FAILED(rv))
    return rv;

  return ParseFromStream(stream, nullptr, bufLen, contentType, aResult);
}


already_AddRefed<nsIDocument>
DOMParser::ParseFromStream(nsIInputStream* aStream,
                           const nsAString& aCharset,
                           int32_t aContentLength,
                           SupportedType aType,
                           ErrorResult& rv)
{
  nsCOMPtr<nsIDOMDocument> domDocument;
  rv = DOMParser::ParseFromStream(aStream,
                                  NS_ConvertUTF16toUTF8(aCharset).get(),
                                  aContentLength,
                                  StringFromSupportedType(aType),
                                  getter_AddRefs(domDocument));
  nsCOMPtr<nsIDocument> document(do_QueryInterface(domDocument));
  return document.forget();
}

NS_IMETHODIMP 
DOMParser::ParseFromStream(nsIInputStream *stream, 
                           const char *charset, 
                           int32_t contentLength,
                           const char *contentType,
                           nsIDOMDocument **aResult)
{
  NS_ENSURE_ARG(stream);
  NS_ENSURE_ARG(contentType);
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = nullptr;

  bool svg = nsCRT::strcmp(contentType, "image/svg+xml") == 0;

  
  
  
  if ((nsCRT::strcmp(contentType, "text/xml") != 0) &&
      (nsCRT::strcmp(contentType, "application/xml") != 0) &&
      (nsCRT::strcmp(contentType, "application/xhtml+xml") != 0) &&
      !svg)
    return NS_ERROR_NOT_IMPLEMENTED;

  nsresult rv;

  
  nsCOMPtr<nsIInputStream> bufferedStream;
  if (!NS_InputStreamIsBuffered(stream)) {
    rv = NS_NewBufferedInputStream(getter_AddRefs(bufferedStream), stream,
                                   4096);
    NS_ENSURE_SUCCESS(rv, rv);

    stream = bufferedStream;
  }

  nsCOMPtr<nsIDOMDocument> domDocument;
  rv = SetUpDocument(svg ? DocumentFlavorSVG : DocumentFlavorLegacyGuess,
                     getter_AddRefs(domDocument));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIChannel> parserChannel;
  NS_NewInputStreamChannel(getter_AddRefs(parserChannel),
                           mDocumentURI,
                           nullptr, 
                           mOriginalPrincipal,
                           nsILoadInfo::SEC_FORCE_INHERIT_PRINCIPAL,
                           nsIContentPolicy::TYPE_OTHER,
                           nsDependentCString(contentType));
  NS_ENSURE_STATE(parserChannel);

  if (charset) {
    parserChannel->SetContentCharset(nsDependentCString(charset));
  }

  
  nsCOMPtr<nsIStreamListener> listener;

  
  
  
  
  nsCOMPtr<nsIDocument> document(do_QueryInterface(domDocument));
  if (!document) return NS_ERROR_FAILURE;

  
  

  if (nsContentUtils::IsSystemPrincipal(mOriginalPrincipal)) {
    document->ForceEnableXULXBL();
  }

  rv = document->StartDocumentLoad(kLoadAsData, parserChannel, 
                                   nullptr, nullptr, 
                                   getter_AddRefs(listener),
                                   false);

  
  document->SetBaseURI(mBaseURI);

  
  document->SetPrincipal(mPrincipal);

  if (NS_FAILED(rv) || !listener) {
    return NS_ERROR_FAILURE;
  }

  
  nsresult status;

  rv = listener->OnStartRequest(parserChannel, nullptr);
  if (NS_FAILED(rv))
    parserChannel->Cancel(rv);
  parserChannel->GetStatus(&status);

  if (NS_SUCCEEDED(rv) && NS_SUCCEEDED(status)) {
    rv = listener->OnDataAvailable(parserChannel, nullptr, stream, 0,
                                   contentLength);
    if (NS_FAILED(rv))
      parserChannel->Cancel(rv);
    parserChannel->GetStatus(&status);
  }

  rv = listener->OnStopRequest(parserChannel, nullptr, status);
  
  

  if (NS_FAILED(rv)) {
    return NS_ERROR_FAILURE;
  }

  domDocument.swap(*aResult);

  return NS_OK;
}

NS_IMETHODIMP
DOMParser::Init(nsIPrincipal* principal, nsIURI* documentURI,
                nsIURI* baseURI, nsIGlobalObject* aScriptObject)
{
  NS_ENSURE_STATE(!mAttemptedInit);
  mAttemptedInit = true;
  
  NS_ENSURE_ARG(principal || documentURI);

  mDocumentURI = documentURI;
  
  if (!mDocumentURI) {
    principal->GetURI(getter_AddRefs(mDocumentURI));
    
    
    if (!mDocumentURI && !nsContentUtils::IsSystemPrincipal(principal)) {
      return NS_ERROR_INVALID_ARG;
    }
  }

  mScriptHandlingObject = do_GetWeakReference(aScriptObject);
  mPrincipal = principal;
  nsresult rv;
  if (!mPrincipal) {
    nsIScriptSecurityManager* secMan = nsContentUtils::GetSecurityManager();
    NS_ENSURE_TRUE(secMan, NS_ERROR_NOT_AVAILABLE);
    rv =
      secMan->GetSimpleCodebasePrincipal(mDocumentURI,
                                         getter_AddRefs(mPrincipal));
    NS_ENSURE_SUCCESS(rv, rv);
    mOriginalPrincipal = mPrincipal;
  } else {
    mOriginalPrincipal = mPrincipal;
    if (nsContentUtils::IsSystemPrincipal(mPrincipal)) {
      
      
      mPrincipal = nsNullPrincipal::Create();
      NS_ENSURE_TRUE(mPrincipal, NS_ERROR_FAILURE);

      if (!mDocumentURI) {
        rv = mPrincipal->GetURI(getter_AddRefs(mDocumentURI));
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }
  
  mBaseURI = baseURI;
  
  
  

  NS_POSTCONDITION(mPrincipal, "Must have principal");
  NS_POSTCONDITION(mOriginalPrincipal, "Must have original principal");
  NS_POSTCONDITION(mDocumentURI, "Must have document URI");
  return NS_OK;
}

already_AddRefed<DOMParser>
DOMParser::Constructor(const GlobalObject& aOwner,
                       nsIPrincipal* aPrincipal, nsIURI* aDocumentURI,
                       nsIURI* aBaseURI, ErrorResult& rv)
{
  if (!nsContentUtils::IsCallerChrome()) {
    rv.Throw(NS_ERROR_DOM_SECURITY_ERR);
    return nullptr;
  }
  nsRefPtr<DOMParser> domParser = new DOMParser(aOwner.GetAsSupports());
  rv = domParser->InitInternal(aOwner.GetAsSupports(), aPrincipal, aDocumentURI,
                               aBaseURI);
  if (rv.Failed()) {
    return nullptr;
  }
  return domParser.forget();
}

already_AddRefed<DOMParser>
DOMParser::Constructor(const GlobalObject& aOwner,
                       ErrorResult& rv)
{
  nsRefPtr<DOMParser> domParser = new DOMParser(aOwner.GetAsSupports());
  rv = domParser->InitInternal(aOwner.GetAsSupports(),
                               nsContentUtils::SubjectPrincipal(),
                               nullptr, nullptr);
  if (rv.Failed()) {
    return nullptr;
  }
  return domParser.forget();
}

nsresult
DOMParser::InitInternal(nsISupports* aOwner, nsIPrincipal* prin,
                        nsIURI* documentURI, nsIURI* baseURI)
{
  AttemptedInitMarker marker(&mAttemptedInit);
  if (!documentURI) {
    
    

    
    
    
    

    
    
    

    nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aOwner);
    if (!window) {
      return NS_ERROR_UNEXPECTED;
    }

    baseURI = window->GetDocBaseURI();
    documentURI = window->GetDocumentURI();
    if (!documentURI) {
      return NS_ERROR_UNEXPECTED;
    }
  }

  nsCOMPtr<nsIGlobalObject> scriptglobal = do_QueryInterface(aOwner);
  return Init(prin, documentURI, baseURI, scriptglobal);
}

void
DOMParser::Init(nsIPrincipal* aPrincipal, nsIURI* aDocumentURI,
                nsIURI* aBaseURI, mozilla::ErrorResult& rv)
{
  AttemptedInitMarker marker(&mAttemptedInit);

  nsCOMPtr<nsIPrincipal> principal = aPrincipal;
  if (!principal && !aDocumentURI) {
    principal = nsContentUtils::SubjectPrincipal();
  }

  rv = Init(principal, aDocumentURI, aBaseURI, GetEntryGlobal());
}

nsresult
DOMParser::SetUpDocument(DocumentFlavor aFlavor, nsIDOMDocument** aResult)
{
  
  
  
  
  
  nsCOMPtr<nsIScriptGlobalObject> scriptHandlingObject =
    do_QueryReferent(mScriptHandlingObject);
  nsresult rv;
  if (!mPrincipal) {
    NS_ENSURE_TRUE(!mAttemptedInit, NS_ERROR_NOT_INITIALIZED);
    AttemptedInitMarker marker(&mAttemptedInit);

    nsCOMPtr<nsIPrincipal> prin = nsNullPrincipal::Create();
    NS_ENSURE_TRUE(prin, NS_ERROR_FAILURE);

    rv = Init(prin, nullptr, nullptr, scriptHandlingObject);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_ASSERTION(mPrincipal, "Must have principal by now");
  NS_ASSERTION(mDocumentURI, "Must have document URI by now");

  
  
  
  
  return NS_NewDOMDocument(aResult, EmptyString(), EmptyString(), nullptr,
                           mDocumentURI, mBaseURI,
                           mOriginalPrincipal,
                           true,
                           scriptHandlingObject,
                           aFlavor);
}
