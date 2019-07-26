





#include "mozilla/Util.h"

#include "nsFormSubmission.h"

#include "nsCOMPtr.h"
#include "nsIForm.h"
#include "nsILinkHandler.h"
#include "nsIDocument.h"
#include "nsGkAtoms.h"
#include "nsIFormControl.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsError.h"
#include "nsGenericHTMLElement.h"
#include "nsAttrValueInlines.h"
#include "nsISaveAsCharset.h"
#include "nsIFile.h"
#include "nsIDOMFile.h"
#include "nsDirectoryServiceDefs.h"
#include "nsStringStream.h"
#include "nsIURI.h"
#include "nsIURL.h"
#include "nsNetUtil.h"
#include "nsLinebreakConverter.h"
#include "nsICharsetConverterManager.h"
#include "nsEscape.h"
#include "nsUnicharUtils.h"
#include "nsIMultiplexInputStream.h"
#include "nsIMIMEInputStream.h"
#include "nsIMIMEService.h"
#include "nsIConsoleService.h"
#include "nsIScriptError.h"
#include "nsIStringBundle.h"
#include "nsCExternalHandlerService.h"
#include "nsIFileStreams.h"
#include "nsContentUtils.h"

#include "mozilla/dom/EncodingUtils.h"

using namespace mozilla;
using mozilla::dom::EncodingUtils;

static void
SendJSWarning(nsIDocument* aDocument,
              const char* aWarningName,
              const PRUnichar** aWarningArgs, uint32_t aWarningArgsLen)
{
  nsContentUtils::ReportToConsole(nsIScriptError::warningFlag,
                                  "HTML", aDocument,
                                  nsContentUtils::eFORMS_PROPERTIES,
                                  aWarningName,
                                  aWarningArgs, aWarningArgsLen);
}



class nsFSURLEncoded : public nsEncodingFormSubmission
{
public:
  




  nsFSURLEncoded(const nsACString& aCharset,
                 int32_t aMethod,
                 nsIDocument* aDocument,
                 nsIContent* aOriginatingElement)
    : nsEncodingFormSubmission(aCharset, aOriginatingElement),
      mMethod(aMethod),
      mDocument(aDocument),
      mWarnedFileControl(false)
  {
  }

  virtual nsresult AddNameValuePair(const nsAString& aName,
                                    const nsAString& aValue);
  virtual nsresult AddNameFilePair(const nsAString& aName,
                                   nsIDOMBlob* aBlob,
                                   const nsString& aFilename);
  virtual nsresult GetEncodedSubmission(nsIURI* aURI,
                                        nsIInputStream** aPostDataStream);

  virtual bool SupportsIsindexSubmission()
  {
    return true;
  }

  virtual nsresult AddIsindex(const nsAString& aValue);

protected:

  







  nsresult URLEncode(const nsAString& aStr, nsCString& aEncoded);

private:
  



  int32_t mMethod;

  
  nsCString mQueryString;

  
  nsCOMPtr<nsIDocument> mDocument;

  
  bool mWarnedFileControl;
};

nsresult
nsFSURLEncoded::AddNameValuePair(const nsAString& aName,
                                 const nsAString& aValue)
{
  
  nsCString convValue;
  nsresult rv = URLEncode(aValue, convValue);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsAutoCString convName;
  rv = URLEncode(aName, convName);
  NS_ENSURE_SUCCESS(rv, rv);


  
  if (mQueryString.IsEmpty()) {
    mQueryString += convName + NS_LITERAL_CSTRING("=") + convValue;
  } else {
    mQueryString += NS_LITERAL_CSTRING("&") + convName
                  + NS_LITERAL_CSTRING("=") + convValue;
  }

  return NS_OK;
}

nsresult
nsFSURLEncoded::AddIsindex(const nsAString& aValue)
{
  
  nsCString convValue;
  nsresult rv = URLEncode(aValue, convValue);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (mQueryString.IsEmpty()) {
    mQueryString.Assign(convValue);
  } else {
    mQueryString += NS_LITERAL_CSTRING("&isindex=") + convValue;
  }

  return NS_OK;
}

nsresult
nsFSURLEncoded::AddNameFilePair(const nsAString& aName,
                                nsIDOMBlob* aBlob,
                                const nsString& aFilename)
{
  if (!mWarnedFileControl) {
    SendJSWarning(mDocument, "ForgotFileEnctypeWarning", nullptr, 0);
    mWarnedFileControl = true;
  }

  nsAutoString filename;
  nsCOMPtr<nsIDOMFile> file = do_QueryInterface(aBlob);
  if (file) {
    file->GetName(filename);
  }

  return AddNameValuePair(aName, filename);
}

static void
HandleMailtoSubject(nsCString& aPath) {

  
  bool hasSubject = false;
  bool hasParams = false;
  int32_t paramSep = aPath.FindChar('?');
  while (paramSep != kNotFound && paramSep < (int32_t)aPath.Length()) {
    hasParams = true;

    
    
    int32_t nameEnd = aPath.FindChar('=', paramSep+1);
    int32_t nextParamSep = aPath.FindChar('&', paramSep+1);
    if (nextParamSep == kNotFound) {
      nextParamSep = aPath.Length();
    }

    
    
    if (nameEnd == kNotFound || nextParamSep < nameEnd) {
      nameEnd = nextParamSep;
    }

    if (nameEnd != kNotFound) {
      if (Substring(aPath, paramSep+1, nameEnd-(paramSep+1)).
          LowerCaseEqualsLiteral("subject")) {
        hasSubject = true;
        break;
      }
    }

    paramSep = nextParamSep;
  }

  
  if (!hasSubject) {
    if (hasParams) {
      aPath.Append('&');
    } else {
      aPath.Append('?');
    }

    
    nsXPIDLString brandName;
    nsresult rv =
      nsContentUtils::GetLocalizedString(nsContentUtils::eBRAND_PROPERTIES,
                                         "brandShortName", brandName);
    if (NS_FAILED(rv))
      return;
    const PRUnichar *formatStrings[] = { brandName.get() };
    nsXPIDLString subjectStr;
    rv = nsContentUtils::FormatLocalizedString(
                                           nsContentUtils::eFORMS_PROPERTIES,
                                           "DefaultFormSubject",
                                           formatStrings,
                                           subjectStr);
    if (NS_FAILED(rv))
      return;
    aPath.AppendLiteral("subject=");
    nsCString subjectStrEscaped;
    aPath.Append(NS_EscapeURL(NS_ConvertUTF16toUTF8(subjectStr), esc_Query,
                              subjectStrEscaped));
  }
}

nsresult
nsFSURLEncoded::GetEncodedSubmission(nsIURI* aURI,
                                     nsIInputStream** aPostDataStream)
{
  nsresult rv = NS_OK;

  *aPostDataStream = nullptr;

  if (mMethod == NS_FORM_METHOD_POST) {

    bool isMailto = false;
    aURI->SchemeIs("mailto", &isMailto);
    if (isMailto) {

      nsAutoCString path;
      rv = aURI->GetPath(path);
      NS_ENSURE_SUCCESS(rv, rv);

      HandleMailtoSubject(path);

      
      nsCString escapedBody;
      escapedBody.Adopt(nsEscape(mQueryString.get(), url_XAlphas));

      path += NS_LITERAL_CSTRING("&force-plain-text=Y&body=") + escapedBody;

      rv = aURI->SetPath(path);

    } else {

      nsCOMPtr<nsIInputStream> dataStream;
      
      
      
      rv = NS_NewCStringInputStream(getter_AddRefs(dataStream), mQueryString);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsIMIMEInputStream> mimeStream(
        do_CreateInstance("@mozilla.org/network/mime-input-stream;1", &rv));
      NS_ENSURE_SUCCESS(rv, rv);

#ifdef SPECIFY_CHARSET_IN_CONTENT_TYPE
      mimeStream->AddHeader("Content-Type",
                            PromiseFlatString(
                              "application/x-www-form-urlencoded; charset="
                              + mCharset
                            ).get());
#else
      mimeStream->AddHeader("Content-Type",
                            "application/x-www-form-urlencoded");
#endif
      mimeStream->SetAddContentLength(true);
      mimeStream->SetData(dataStream);

      *aPostDataStream = mimeStream;
      NS_ADDREF(*aPostDataStream);
    }

  } else {
    
    bool schemeIsJavaScript;
    rv = aURI->SchemeIs("javascript", &schemeIsJavaScript);
    NS_ENSURE_SUCCESS(rv, rv);
    if (schemeIsJavaScript) {
      return NS_OK;
    }

    nsCOMPtr<nsIURL> url = do_QueryInterface(aURI);
    if (url) {
      url->SetQuery(mQueryString);
    }
    else {
      nsAutoCString path;
      rv = aURI->GetPath(path);
      NS_ENSURE_SUCCESS(rv, rv);
      
      int32_t namedAnchorPos = path.FindChar('#');
      nsAutoCString namedAnchor;
      if (kNotFound != namedAnchorPos) {
        path.Right(namedAnchor, (path.Length() - namedAnchorPos));
        path.Truncate(namedAnchorPos);
      }

      
      
      int32_t queryStart = path.FindChar('?');
      if (kNotFound != queryStart) {
        path.Truncate(queryStart);
      }

      path.Append('?');
      
      path.Append(mQueryString + namedAnchor);

      aURI->SetPath(path);
    }
  }

  return rv;
}


nsresult
nsFSURLEncoded::URLEncode(const nsAString& aStr, nsCString& aEncoded)
{
  
  PRUnichar* convertedBuf =
    nsLinebreakConverter::ConvertUnicharLineBreaks(PromiseFlatString(aStr).get(),
                                                   nsLinebreakConverter::eLinebreakAny,
                                                   nsLinebreakConverter::eLinebreakNet);
  NS_ENSURE_TRUE(convertedBuf, NS_ERROR_OUT_OF_MEMORY);

  nsAutoCString encodedBuf;
  nsresult rv = EncodeVal(nsDependentString(convertedBuf), encodedBuf, false);
  nsMemory::Free(convertedBuf);
  NS_ENSURE_SUCCESS(rv, rv);

  char* escapedBuf = nsEscape(encodedBuf.get(), url_XPAlphas);
  NS_ENSURE_TRUE(escapedBuf, NS_ERROR_OUT_OF_MEMORY);
  aEncoded.Adopt(escapedBuf);

  return NS_OK;
}



nsFSMultipartFormData::nsFSMultipartFormData(const nsACString& aCharset,
                                             nsIContent* aOriginatingElement)
    : nsEncodingFormSubmission(aCharset, aOriginatingElement)
{
  mPostDataStream =
    do_CreateInstance("@mozilla.org/io/multiplex-input-stream;1");
  mTotalLength = 0;

  mBoundary.AssignLiteral("---------------------------");
  mBoundary.AppendInt(rand());
  mBoundary.AppendInt(rand());
  mBoundary.AppendInt(rand());
}

nsFSMultipartFormData::~nsFSMultipartFormData()
{
  NS_ASSERTION(mPostDataChunk.IsEmpty(), "Left unsubmitted data");
}

nsIInputStream*
nsFSMultipartFormData::GetSubmissionBody(uint64_t* aContentLength)
{
  
  mPostDataChunk += NS_LITERAL_CSTRING("--") + mBoundary
                  + NS_LITERAL_CSTRING("--" CRLF);

  
  AddPostDataStream();

  *aContentLength = mTotalLength;
  return mPostDataStream;
}

nsresult
nsFSMultipartFormData::AddNameValuePair(const nsAString& aName,
                                        const nsAString& aValue)
{
  nsCString valueStr;
  nsAutoCString encodedVal;
  nsresult rv = EncodeVal(aValue, encodedVal, false);
  NS_ENSURE_SUCCESS(rv, rv);

  valueStr.Adopt(nsLinebreakConverter::
                 ConvertLineBreaks(encodedVal.get(),
                                   nsLinebreakConverter::eLinebreakAny,
                                   nsLinebreakConverter::eLinebreakNet));

  nsAutoCString nameStr;
  rv = EncodeVal(aName, nameStr, true);
  NS_ENSURE_SUCCESS(rv, rv);

  

  
  
  
  mPostDataChunk += NS_LITERAL_CSTRING("--") + mBoundary
                 + NS_LITERAL_CSTRING(CRLF)
                 + NS_LITERAL_CSTRING("Content-Disposition: form-data; name=\"")
                 + nameStr + NS_LITERAL_CSTRING("\"" CRLF CRLF)
                 + valueStr + NS_LITERAL_CSTRING(CRLF);

  return NS_OK;
}

nsresult
nsFSMultipartFormData::AddNameFilePair(const nsAString& aName,
                                       nsIDOMBlob* aBlob,
                                       const nsString& aFilename)
{
  
  nsAutoCString nameStr;
  nsresult rv = EncodeVal(aName, nameStr, true);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCString filename, contentType;
  nsCOMPtr<nsIInputStream> fileStream;
  if (aBlob) {
    
    if (!aFilename.IsVoid()) {
      rv = EncodeVal(aFilename, filename, true);
      NS_ENSURE_SUCCESS(rv, rv);
    } else {
      
      nsAutoString filename16;
      nsCOMPtr<nsIDOMFile> file = do_QueryInterface(aBlob);
      if (file) {
        rv = file->GetName(filename16);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      if (filename16.IsEmpty()) {
        filename16.AssignLiteral("blob");
      }

      rv = EncodeVal(filename16, filename, true);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    
    nsAutoString contentType16;
    rv = aBlob->GetType(contentType16);
    if (NS_FAILED(rv) || contentType16.IsEmpty()) {
      contentType16.AssignLiteral("application/octet-stream");
    }
    contentType.Adopt(nsLinebreakConverter::
                      ConvertLineBreaks(NS_ConvertUTF16toUTF8(contentType16).get(),
                                        nsLinebreakConverter::eLinebreakAny,
                                        nsLinebreakConverter::eLinebreakSpace));

    
    rv = aBlob->GetInternalStream(getter_AddRefs(fileStream));
    NS_ENSURE_SUCCESS(rv, rv);
    if (fileStream) {
      
      nsCOMPtr<nsIInputStream> bufferedStream;
      rv = NS_NewBufferedInputStream(getter_AddRefs(bufferedStream),
                                     fileStream, 8192);
      NS_ENSURE_SUCCESS(rv, rv);

      fileStream = bufferedStream;
    }
  }
  else {
    contentType.AssignLiteral("application/octet-stream");
  }

  
  
  
  
  mPostDataChunk += NS_LITERAL_CSTRING("--") + mBoundary
                 + NS_LITERAL_CSTRING(CRLF);
  
  
  
  mPostDataChunk +=
         NS_LITERAL_CSTRING("Content-Disposition: form-data; name=\"")
       + nameStr + NS_LITERAL_CSTRING("\"; filename=\"")
       + filename + NS_LITERAL_CSTRING("\"" CRLF)
       + NS_LITERAL_CSTRING("Content-Type: ")
       + contentType + NS_LITERAL_CSTRING(CRLF CRLF);

  
  
  uint64_t size;
  if (fileStream && NS_SUCCEEDED(aBlob->GetSize(&size))) {
    
    
    AddPostDataStream();

    mPostDataStream->AppendStream(fileStream);
    mTotalLength += size;
  }

  
  mPostDataChunk.AppendLiteral(CRLF);

  return NS_OK;
}

nsresult
nsFSMultipartFormData::GetEncodedSubmission(nsIURI* aURI,
                                            nsIInputStream** aPostDataStream)
{
  nsresult rv;

  
  nsCOMPtr<nsIMIMEInputStream> mimeStream
    = do_CreateInstance("@mozilla.org/network/mime-input-stream;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoCString contentType;
  GetContentType(contentType);
  mimeStream->AddHeader("Content-Type", contentType.get());
  mimeStream->SetAddContentLength(true);
  uint64_t unused;
  mimeStream->SetData(GetSubmissionBody(&unused));

  *aPostDataStream = mimeStream.forget().get();

  return NS_OK;
}

nsresult
nsFSMultipartFormData::AddPostDataStream()
{
  nsresult rv = NS_OK;
  
  nsCOMPtr<nsIInputStream> postDataChunkStream;
  rv = NS_NewCStringInputStream(getter_AddRefs(postDataChunkStream),
                                mPostDataChunk);
  NS_ASSERTION(postDataChunkStream, "Could not open a stream for POST!");
  if (postDataChunkStream) {
    mPostDataStream->AppendStream(postDataChunkStream);
    mTotalLength += mPostDataChunk.Length();
  }

  mPostDataChunk.Truncate();

  return rv;
}



class nsFSTextPlain : public nsEncodingFormSubmission
{
public:
  nsFSTextPlain(const nsACString& aCharset, nsIContent* aOriginatingElement)
    : nsEncodingFormSubmission(aCharset, aOriginatingElement)
  {
  }

  virtual nsresult AddNameValuePair(const nsAString& aName,
                                    const nsAString& aValue);
  virtual nsresult AddNameFilePair(const nsAString& aName,
                                   nsIDOMBlob* aBlob,
                                   const nsString& aFilename);
  virtual nsresult GetEncodedSubmission(nsIURI* aURI,
                                        nsIInputStream** aPostDataStream);

private:
  nsString mBody;
};

nsresult
nsFSTextPlain::AddNameValuePair(const nsAString& aName,
                                const nsAString& aValue)
{
  
  
  
  mBody.Append(aName + NS_LITERAL_STRING("=") + aValue +
               NS_LITERAL_STRING(CRLF));

  return NS_OK;
}

nsresult
nsFSTextPlain::AddNameFilePair(const nsAString& aName,
                               nsIDOMBlob* aBlob,
                               const nsString& aFilename)
{
  nsAutoString filename;
  nsCOMPtr<nsIDOMFile> file = do_QueryInterface(aBlob);
  if (file) {
    file->GetName(filename);
  }

  AddNameValuePair(aName, filename);
  return NS_OK;
}

nsresult
nsFSTextPlain::GetEncodedSubmission(nsIURI* aURI,
                                    nsIInputStream** aPostDataStream)
{
  nsresult rv = NS_OK;

  
  
  
  bool isMailto = false;
  aURI->SchemeIs("mailto", &isMailto);
  if (isMailto) {
    nsAutoCString path;
    rv = aURI->GetPath(path);
    NS_ENSURE_SUCCESS(rv, rv);

    HandleMailtoSubject(path);

    
    char* escapedBuf = nsEscape(NS_ConvertUTF16toUTF8(mBody).get(),
                                url_XAlphas);
    NS_ENSURE_TRUE(escapedBuf, NS_ERROR_OUT_OF_MEMORY);
    nsCString escapedBody;
    escapedBody.Adopt(escapedBuf);

    path += NS_LITERAL_CSTRING("&force-plain-text=Y&body=") + escapedBody;

    rv = aURI->SetPath(path);

  } else {
    
    
    
    
    
    
    nsCString cbody;
    EncodeVal(mBody, cbody, false);
    cbody.Adopt(nsLinebreakConverter::
                ConvertLineBreaks(cbody.get(),
                                  nsLinebreakConverter::eLinebreakAny,
                                  nsLinebreakConverter::eLinebreakNet));
    nsCOMPtr<nsIInputStream> bodyStream;
    rv = NS_NewCStringInputStream(getter_AddRefs(bodyStream), cbody);
    if (!bodyStream) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    
    nsCOMPtr<nsIMIMEInputStream> mimeStream
        = do_CreateInstance("@mozilla.org/network/mime-input-stream;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    mimeStream->AddHeader("Content-Type", "text/plain");
    mimeStream->SetAddContentLength(true);
    mimeStream->SetData(bodyStream);
    CallQueryInterface(mimeStream, aPostDataStream);
  }

  return rv;
}



nsEncodingFormSubmission::nsEncodingFormSubmission(const nsACString& aCharset,
                                                   nsIContent* aOriginatingElement)
  : nsFormSubmission(aCharset, aOriginatingElement)
{
  nsAutoCString charset(aCharset);
  
  
  if (charset.EqualsLiteral("ISO-8859-1")) {
    charset.AssignLiteral("windows-1252");
  }

  if (!(charset.EqualsLiteral("UTF-8") || charset.EqualsLiteral("gb18030"))) {
    NS_ConvertUTF8toUTF16 charsetUtf16(charset);
    const PRUnichar* charsetPtr = charsetUtf16.get();
    SendJSWarning(aOriginatingElement ? aOriginatingElement->GetOwnerDocument()
                                      : nullptr,
                  "CannotEncodeAllUnicode",
                  &charsetPtr,
                  1);
  }

  mEncoder = do_CreateInstance(NS_SAVEASCHARSET_CONTRACTID);
  if (mEncoder) {
    nsresult rv =
      mEncoder->Init(charset.get(),
                     (nsISaveAsCharset::attr_EntityAfterCharsetConv + 
                      nsISaveAsCharset::attr_FallbackDecimalNCR),
                     0);
    if (NS_FAILED(rv)) {
      mEncoder = nullptr;
    }
  }
}

nsEncodingFormSubmission::~nsEncodingFormSubmission()
{
}


nsresult
nsEncodingFormSubmission::EncodeVal(const nsAString& aStr, nsCString& aOut,
                                    bool aHeaderEncode)
{
  if (mEncoder && !aStr.IsEmpty()) {
    aOut.Truncate();
    nsresult rv = mEncoder->Convert(PromiseFlatString(aStr).get(),
                                    getter_Copies(aOut));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    
    CopyUTF16toUTF8(aStr, aOut);
  }

  if (aHeaderEncode) {
    aOut.Adopt(nsLinebreakConverter::
               ConvertLineBreaks(aOut.get(),
                                 nsLinebreakConverter::eLinebreakAny,
                                 nsLinebreakConverter::eLinebreakSpace));
    aOut.ReplaceSubstring(NS_LITERAL_CSTRING("\""),
                          NS_LITERAL_CSTRING("\\\""));
  }


  return NS_OK;
}



static void
GetSubmitCharset(nsGenericHTMLElement* aForm,
                 nsACString& oCharset)
{
  oCharset.AssignLiteral("UTF-8"); 

  nsAutoString acceptCharsetValue;
  aForm->GetAttr(kNameSpaceID_None, nsGkAtoms::acceptcharset,
                 acceptCharsetValue);

  int32_t charsetLen = acceptCharsetValue.Length();
  if (charsetLen > 0) {
    int32_t offset=0;
    int32_t spPos=0;
    
    do {
      spPos = acceptCharsetValue.FindChar(PRUnichar(' '), offset);
      int32_t cnt = ((-1==spPos)?(charsetLen-offset):(spPos-offset));
      if (cnt > 0) {
        nsAutoString uCharset;
        acceptCharsetValue.Mid(uCharset, offset, cnt);

        if (EncodingUtils::FindEncodingForLabel(uCharset, oCharset))
          return;
      }
      offset = spPos + 1;
    } while (spPos != -1);
  }
  
  
  nsIDocument* doc = aForm->GetDocument();
  if (doc) {
    oCharset = doc->GetDocumentCharacterSet();
  }
}

static void
GetEnumAttr(nsGenericHTMLElement* aContent,
            nsIAtom* atom, int32_t* aValue)
{
  const nsAttrValue* value = aContent->GetParsedAttr(atom);
  if (value && value->Type() == nsAttrValue::eEnum) {
    *aValue = value->GetEnumValue();
  }
}

nsresult
GetSubmissionFromForm(nsGenericHTMLElement* aForm,
                      nsGenericHTMLElement* aOriginatingElement,
                      nsFormSubmission** aFormSubmission)
{
  
  NS_ASSERTION(aForm->GetCurrentDoc(),
               "Should have doc if we're building submission!");

  
  int32_t enctype = NS_FORM_ENCTYPE_URLENCODED;
  if (aOriginatingElement &&
      aOriginatingElement->HasAttr(kNameSpaceID_None, nsGkAtoms::formenctype)) {
    GetEnumAttr(aOriginatingElement, nsGkAtoms::formenctype, &enctype);
  } else {
    GetEnumAttr(aForm, nsGkAtoms::enctype, &enctype);
  }

  
  int32_t method = NS_FORM_METHOD_GET;
  if (aOriginatingElement &&
      aOriginatingElement->HasAttr(kNameSpaceID_None, nsGkAtoms::formmethod)) {
    GetEnumAttr(aOriginatingElement, nsGkAtoms::formmethod, &method);
  } else {
    GetEnumAttr(aForm, nsGkAtoms::method, &method);
  }

  
  nsAutoCString charset;
  GetSubmitCharset(aForm, charset);

  
  

  
  
  if (StringBeginsWith(charset, NS_LITERAL_CSTRING("UTF-16"))) {
    charset.AssignLiteral("UTF-8");
  }

  
  if (method == NS_FORM_METHOD_POST &&
      enctype == NS_FORM_ENCTYPE_MULTIPART) {
    *aFormSubmission = new nsFSMultipartFormData(charset, aOriginatingElement);
  } else if (method == NS_FORM_METHOD_POST &&
             enctype == NS_FORM_ENCTYPE_TEXTPLAIN) {
    *aFormSubmission = new nsFSTextPlain(charset, aOriginatingElement);
  } else {
    nsIDocument* doc = aForm->OwnerDoc();
    if (enctype == NS_FORM_ENCTYPE_MULTIPART ||
        enctype == NS_FORM_ENCTYPE_TEXTPLAIN) {
      nsAutoString enctypeStr;
      if (aOriginatingElement &&
          aOriginatingElement->HasAttr(kNameSpaceID_None,
                                       nsGkAtoms::formenctype)) {
        aOriginatingElement->GetAttr(kNameSpaceID_None, nsGkAtoms::formenctype,
                                     enctypeStr);
      } else {
        aForm->GetAttr(kNameSpaceID_None, nsGkAtoms::enctype, enctypeStr);
      }
      const PRUnichar* enctypeStrPtr = enctypeStr.get();
      SendJSWarning(doc, "ForgotPostWarning",
                    &enctypeStrPtr, 1);
    }
    *aFormSubmission = new nsFSURLEncoded(charset, method, doc,
                                          aOriginatingElement);
  }
  NS_ENSURE_TRUE(*aFormSubmission, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}
