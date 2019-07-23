





































#include "nsIFormSubmission.h"

#include "nsPresContext.h"
#include "nsCOMPtr.h"
#include "nsIForm.h"
#include "nsILinkHandler.h"
#include "nsIDocument.h"
#include "nsGkAtoms.h"
#include "nsIHTMLDocument.h"
#include "nsIFormControl.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsDOMError.h"
#include "nsGenericHTMLElement.h"
#include "nsISaveAsCharset.h"


#include "nsIFile.h"
#include "nsDirectoryServiceDefs.h"
#include "nsStringStream.h"
#include "nsIFormProcessor.h"
#include "nsIURI.h"
#include "nsNetUtil.h"
#include "nsLinebreakConverter.h"
#include "nsICharsetConverterManager.h"
#include "nsICharsetAlias.h"
#include "nsEscape.h"
#include "nsUnicharUtils.h"
#include "nsIMultiplexInputStream.h"
#include "nsIMIMEInputStream.h"
#include "nsIConsoleService.h"
#include "nsIScriptError.h"
#include "nsIStringBundle.h"


#include "nsBidiUtils.h"


static NS_DEFINE_CID(kFormProcessorCID, NS_FORMPROCESSOR_CID);





class nsFormSubmission : public nsIFormSubmission {

public:

  






  nsFormSubmission(const nsACString& aCharset,
                   nsISaveAsCharset* aEncoder,
                   nsIFormProcessor* aFormProcessor,
                   PRInt32 aBidiOptions)
    : mCharset(aCharset),
      mEncoder(aEncoder),
      mFormProcessor(aFormProcessor),
      mBidiOptions(aBidiOptions)
  {
  }
  virtual ~nsFormSubmission()
  {
  }

  NS_DECL_ISUPPORTS

  
  
  
  virtual nsresult SubmitTo(nsIURI* aActionURI, const nsAString& aTarget,
                            nsIContent* aSource, nsILinkHandler* aLinkHandler,
                            nsIDocShell** aDocShell, nsIRequest** aRequest);

  



  NS_IMETHOD Init() = 0;

protected:
  
  






  NS_IMETHOD GetEncodedSubmission(nsIURI* aURI,
                                  nsIInputStream** aPostDataStream) = 0;

  
  








  nsresult ProcessValue(nsIDOMHTMLElement* aSource, const nsAString& aName, 
                        const nsAString& aValue, nsAString& aResult);

  
  






  nsresult EncodeVal(const nsAString& aStr, nsACString& aResult);
  






  nsresult UnicodeToNewBytes(const nsAString& aStr, nsISaveAsCharset* aEncoder,
                             nsACString& aOut);

  
  nsCString mCharset;
  


  nsCOMPtr<nsISaveAsCharset> mEncoder;
  
  nsCOMPtr<nsIFormProcessor> mFormProcessor;
  
  PRInt32 mBidiOptions;

public:
  

  






  static void GetSubmitCharset(nsGenericHTMLElement* aForm,
                               PRUint8 aCtrlsModAtSubmit,
                               nsACString& aCharset);
  





  static nsresult GetEncoder(nsGenericHTMLElement* aForm,
                             const nsACString& aCharset,
                             nsISaveAsCharset** aEncoder);
  







  static void GetEnumAttr(nsGenericHTMLElement* aForm,
                          nsIAtom* aAtom, PRInt32* aValue);
};











static nsresult
SendJSWarning(nsIContent* aContent,
              const char* aWarningName);







static nsresult
SendJSWarning(nsIContent* aContent,
              const char* aWarningName,
              const nsAFlatString& aWarningArg1);








static nsresult
SendJSWarning(nsIContent* aContent,
              const char* aWarningName,
              const PRUnichar** aWarningArgs, PRUint32 aWarningArgsLen);


class nsFSURLEncoded : public nsFormSubmission
{
public:
  








  nsFSURLEncoded(const nsACString& aCharset,
                 nsISaveAsCharset* aEncoder,
                 nsIFormProcessor* aFormProcessor,
                 PRInt32 aBidiOptions,
                 PRInt32 aMethod)
    : nsFormSubmission(aCharset, aEncoder, aFormProcessor, aBidiOptions),
      mMethod(aMethod)
  {
  }
  virtual ~nsFSURLEncoded()
  {
  }

  
  virtual nsresult AddNameValuePair(nsIDOMHTMLElement* aSource,
                                    const nsAString& aName,
                                    const nsAString& aValue);
  virtual nsresult AddNameFilePair(nsIDOMHTMLElement* aSource,
                                   const nsAString& aName,
                                   const nsAString& aFilename,
                                   nsIInputStream* aStream,
                                   const nsACString& aContentType,
                                   PRBool aMoreFilesToCome);
  virtual PRBool AcceptsFiles() const
  {
    return PR_FALSE;
  }

  NS_IMETHOD Init();

protected:
  
  NS_IMETHOD GetEncodedSubmission(nsIURI* aURI,
                                  nsIInputStream** aPostDataStream);

  
  







  nsresult URLEncode(const nsAString& aStr, nsCString& aEncoded);

private:
  



  PRInt32 mMethod;

  
  nsCString mQueryString;

  
  PRBool mWarnedFileControl;
};

nsresult
nsFSURLEncoded::AddNameValuePair(nsIDOMHTMLElement* aSource,
                                 const nsAString& aName,
                                 const nsAString& aValue)
{
  
  
  
  if (!mWarnedFileControl) {
    nsCOMPtr<nsIFormControl> formControl = do_QueryInterface(aSource);
    if (formControl->GetType() == NS_FORM_INPUT_FILE) {
      nsCOMPtr<nsIContent> content = do_QueryInterface(aSource);
      SendJSWarning(content, "ForgotFileEnctypeWarning");
      mWarnedFileControl = PR_TRUE;
    }
  }

  
  
  
  nsAutoString processedValue;
  nsresult rv = ProcessValue(aSource, aName, aValue, processedValue);

  
  
  
  nsCString convValue;
  if (NS_SUCCEEDED(rv)) {
    rv = URLEncode(processedValue, convValue);
  }
  else {
    rv = URLEncode(aValue, convValue);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  nsCAutoString convName;
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
nsFSURLEncoded::AddNameFilePair(nsIDOMHTMLElement* aSource,
                                const nsAString& aName,
                                const nsAString& aFilename,
                                nsIInputStream* aStream,
                                const nsACString& aContentType,
                                PRBool aMoreFilesToCome)
{
  return AddNameValuePair(aSource, aName, aFilename);
}




NS_IMETHODIMP
nsFSURLEncoded::Init()
{
  mQueryString.Truncate();
  mWarnedFileControl = PR_FALSE;
  return NS_OK;
}

static void
HandleMailtoSubject(nsCString& aPath) {

  
  PRBool hasSubject = PR_FALSE;
  PRBool hasParams = PR_FALSE;
  PRInt32 paramSep = aPath.FindChar('?');
  while (paramSep != kNotFound && paramSep < (PRInt32)aPath.Length()) {
    hasParams = PR_TRUE;

    
    
    PRInt32 nameEnd = aPath.FindChar('=', paramSep+1);
    PRInt32 nextParamSep = aPath.FindChar('&', paramSep+1);
    if (nextParamSep == kNotFound) {
      nextParamSep = aPath.Length();
    }

    
    
    if (nameEnd == kNotFound || nextParamSep < nameEnd) {
      nameEnd = nextParamSep;
    }

    if (nameEnd != kNotFound) {
      if (Substring(aPath, paramSep+1, nameEnd-(paramSep+1)).
          LowerCaseEqualsLiteral("subject")) {
        hasSubject = PR_TRUE;
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
                                           NS_ARRAY_LENGTH(formatStrings),
                                           subjectStr);
    if (NS_FAILED(rv))
      return;
    aPath.AppendLiteral("subject=");
    nsCString subjectStrEscaped;
    aPath.Append(NS_EscapeURL(NS_ConvertUTF16toUTF8(subjectStr), esc_Query,
                              subjectStrEscaped));
  }
}

NS_IMETHODIMP
nsFSURLEncoded::GetEncodedSubmission(nsIURI* aURI,
                                     nsIInputStream** aPostDataStream)
{
  nsresult rv = NS_OK;

  *aPostDataStream = nsnull;

  if (mMethod == NS_FORM_METHOD_POST) {

    PRBool isMailto = PR_FALSE;
    aURI->SchemeIs("mailto", &isMailto);
    if (isMailto) {

      nsCAutoString path;
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
      mimeStream->SetAddContentLength(PR_TRUE);
      mimeStream->SetData(dataStream);

      *aPostDataStream = mimeStream;
      NS_ADDREF(*aPostDataStream);
    }

  } else {
    
    
    
    PRBool schemeIsJavaScript;
    rv = aURI->SchemeIs("javascript", &schemeIsJavaScript);
    NS_ENSURE_SUCCESS(rv, rv);
    if (schemeIsJavaScript) {
      return NS_OK;
    }

    nsCAutoString path;
    rv = aURI->GetPath(path);
    NS_ENSURE_SUCCESS(rv, rv);
    
    PRInt32 namedAnchorPos = path.FindChar('#');
    nsCAutoString namedAnchor;
    if (kNotFound != namedAnchorPos) {
      path.Right(namedAnchor, (path.Length() - namedAnchorPos));
      path.Truncate(namedAnchorPos);
    }

    
    
    PRInt32 queryStart = path.FindChar('?');
    if (kNotFound != queryStart) {
      path.Truncate(queryStart);
    }

    path.Append('?');
    
    path.Append(mQueryString + namedAnchor);

    aURI->SetPath(path);
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

  nsCAutoString encodedBuf;
  nsresult rv = EncodeVal(nsDependentString(convertedBuf), encodedBuf);
  nsMemory::Free(convertedBuf);
  NS_ENSURE_SUCCESS(rv, rv);

  char* escapedBuf = nsEscape(encodedBuf.get(), url_XPAlphas);
  NS_ENSURE_TRUE(escapedBuf, NS_ERROR_OUT_OF_MEMORY);
  aEncoded.Adopt(escapedBuf);

  return NS_OK;
}







class nsFSMultipartFormData : public nsFormSubmission
{
public:
  






  nsFSMultipartFormData(const nsACString& aCharset,
                        nsISaveAsCharset* aEncoder,
                        nsIFormProcessor* aFormProcessor,
                        PRInt32 aBidiOptions);
  virtual ~nsFSMultipartFormData() { }
 
  
  virtual nsresult AddNameValuePair(nsIDOMHTMLElement* aSource,
                                    const nsAString& aName,
                                    const nsAString& aValue);
  virtual nsresult AddNameFilePair(nsIDOMHTMLElement* aSource,
                                   const nsAString& aName,
                                   const nsAString& aFilename,
                                   nsIInputStream* aStream,
                                   const nsACString& aContentType,
                                   PRBool aMoreFilesToCome);
  virtual PRBool AcceptsFiles() const
  {
    return PR_TRUE;
  }

  NS_IMETHOD Init();

protected:
  
  NS_IMETHOD GetEncodedSubmission(nsIURI* aURI,
                                  nsIInputStream** aPostDataStream);

  
  


  nsresult AddPostDataStream();
  









  nsresult ProcessAndEncode(nsIDOMHTMLElement* aSource,
                            const nsAString& aName,
                            const nsAString& aValue,
                            nsCString& aProcessedName,
                            nsCString& aProcessedValue);

private:
  








  PRBool mBackwardsCompatibleSubmit;

  




  nsCOMPtr<nsIMultiplexInputStream> mPostDataStream;

  






  nsCString mPostDataChunk;

  




  nsCString mBoundary;
};




nsFSMultipartFormData::nsFSMultipartFormData(const nsACString& aCharset,
                                             nsISaveAsCharset* aEncoder,
                                             nsIFormProcessor* aFormProcessor,
                                             PRInt32 aBidiOptions)
    : nsFormSubmission(aCharset, aEncoder, aFormProcessor, aBidiOptions)
{
  
  mBackwardsCompatibleSubmit =
    nsContentUtils::GetBoolPref("browser.forms.submit.backwards_compatible");
}

nsresult
nsFSMultipartFormData::ProcessAndEncode(nsIDOMHTMLElement* aSource,
                                        const nsAString& aName,
                                        const nsAString& aValue,
                                        nsCString& aProcessedName,
                                        nsCString& aProcessedValue)
{
  
  
  
  nsAutoString processedValue;
  nsresult rv = ProcessValue(aSource, aName, aValue, processedValue);

  
  
  
  nsCAutoString encodedVal;
  if (NS_SUCCEEDED(rv)) {
    rv = EncodeVal(processedValue, encodedVal);
  } else {
    rv = EncodeVal(aValue, encodedVal);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  rv  = EncodeVal(aName, aProcessedName);
  NS_ENSURE_SUCCESS(rv, rv);


  
  
  
  aProcessedValue.Adopt(nsLinebreakConverter::ConvertLineBreaks(encodedVal.get(),
                        nsLinebreakConverter::eLinebreakAny,
                        nsLinebreakConverter::eLinebreakNet));
  return NS_OK;
}




nsresult
nsFSMultipartFormData::AddNameValuePair(nsIDOMHTMLElement* aSource,
                                        const nsAString& aName,
                                        const nsAString& aValue)
{
  nsCAutoString nameStr;
  nsCString valueStr;
  nsresult rv = ProcessAndEncode(aSource, aName, aValue, nameStr, valueStr);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  
  
  
  
  mPostDataChunk += NS_LITERAL_CSTRING("--") + mBoundary
                 + NS_LITERAL_CSTRING(CRLF)
                 + NS_LITERAL_CSTRING("Content-Disposition: form-data; name=\"")
                 + nameStr + NS_LITERAL_CSTRING("\"" CRLF)
                 + NS_LITERAL_CSTRING("Content-Type: text/plain; charset=")
                 + mCharset
                 + NS_LITERAL_CSTRING(CRLF CRLF)
                 + valueStr + NS_LITERAL_CSTRING(CRLF);

  return NS_OK;
}

nsresult
nsFSMultipartFormData::AddNameFilePair(nsIDOMHTMLElement* aSource,
                                       const nsAString& aName,
                                       const nsAString& aFilename,
                                       nsIInputStream* aStream,
                                       const nsACString& aContentType,
                                       PRBool aMoreFilesToCome)
{
  nsCAutoString nameStr;
  nsCAutoString filenameStr;
  nsresult rv = ProcessAndEncode(aSource, aName, aFilename, nameStr, filenameStr);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  mPostDataChunk += NS_LITERAL_CSTRING("--") + mBoundary
                 + NS_LITERAL_CSTRING(CRLF);
  if (!mBackwardsCompatibleSubmit) {
    
    
    
    
    
    
    
    
    
    mPostDataChunk +=
          NS_LITERAL_CSTRING("Content-Transfer-Encoding: binary" CRLF);
  }
  
  
  
  
  
  
  mPostDataChunk +=
         NS_LITERAL_CSTRING("Content-Disposition: form-data; name=\"")
       + nameStr + NS_LITERAL_CSTRING("\"; filename=\"")
       + filenameStr + NS_LITERAL_CSTRING("\"" CRLF)
       + NS_LITERAL_CSTRING("Content-Type: ") + aContentType
       + NS_LITERAL_CSTRING(CRLF CRLF);

  
  
  
  if (aStream) {
    
    
    AddPostDataStream();

    mPostDataStream->AppendStream(aStream);
  }

  
  
  
  mPostDataChunk.AppendLiteral(CRLF);

  return NS_OK;
}




NS_IMETHODIMP
nsFSMultipartFormData::Init()
{
  nsresult rv;

  
  
  
  mPostDataStream =
    do_CreateInstance("@mozilla.org/io/multiplex-input-stream;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!mPostDataStream) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  
  
  mBoundary.AssignLiteral("---------------------------");
  mBoundary.AppendInt(rand());
  mBoundary.AppendInt(rand());
  mBoundary.AppendInt(rand());

  return NS_OK;
}

NS_IMETHODIMP
nsFSMultipartFormData::GetEncodedSubmission(nsIURI* aURI,
                                            nsIInputStream** aPostDataStream)
{
  nsresult rv;

  
  
  
  mPostDataChunk += NS_LITERAL_CSTRING("--") + mBoundary
                  + NS_LITERAL_CSTRING("--" CRLF);

  
  
  
  AddPostDataStream();

  
  
  
  nsCOMPtr<nsIMIMEInputStream> mimeStream
    = do_CreateInstance("@mozilla.org/network/mime-input-stream;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString boundaryHeaderValue(
    NS_LITERAL_CSTRING("multipart/form-data; boundary=") + mBoundary);

  mimeStream->AddHeader("Content-Type", boundaryHeaderValue.get());
  mimeStream->SetAddContentLength(PR_TRUE);
  mimeStream->SetData(mPostDataStream);

  *aPostDataStream = mimeStream;

  NS_ADDREF(*aPostDataStream);

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
  }

  mPostDataChunk.Truncate();

  return rv;
}





class nsFSTextPlain : public nsFormSubmission
{
public:
  nsFSTextPlain(const nsACString& aCharset,
                nsISaveAsCharset* aEncoder,
                nsIFormProcessor* aFormProcessor,
                PRInt32 aBidiOptions)
    : nsFormSubmission(aCharset, aEncoder, aFormProcessor, aBidiOptions)
  {
  }
  virtual ~nsFSTextPlain()
  {
  }

  
  virtual nsresult AddNameValuePair(nsIDOMHTMLElement* aSource,
                                    const nsAString& aName,
                                    const nsAString& aValue);
  virtual nsresult AddNameFilePair(nsIDOMHTMLElement* aSource,
                                   const nsAString& aName,
                                   const nsAString& aFilename,
                                   nsIInputStream* aStream,
                                   const nsACString& aContentType,
                                   PRBool aMoreFilesToCome);

  NS_IMETHOD Init();

protected:
  
  NS_IMETHOD GetEncodedSubmission(nsIURI* aURI,
                                  nsIInputStream** aPostDataStream);
  virtual PRBool AcceptsFiles() const
  {
    return PR_FALSE;
  }

private:
  nsString mBody;
};

nsresult
nsFSTextPlain::AddNameValuePair(nsIDOMHTMLElement* aSource,
                                const nsAString& aName,
                                const nsAString& aValue)
{
  
  
  
  nsString processedValue;
  nsresult rv = ProcessValue(aSource, aName, aValue, processedValue);

  
  
  
  if (NS_SUCCEEDED(rv)) {
    mBody.Append(aName + NS_LITERAL_STRING("=") + processedValue +
                 NS_LITERAL_STRING(CRLF));

  } else {
    mBody.Append(aName + NS_LITERAL_STRING("=") + aValue +
                 NS_LITERAL_STRING(CRLF));
  }

  return NS_OK;
}

nsresult
nsFSTextPlain::AddNameFilePair(nsIDOMHTMLElement* aSource,
                               const nsAString& aName,
                               const nsAString& aFilename,
                               nsIInputStream* aStream,
                               const nsACString& aContentType,
                               PRBool aMoreFilesToCome)
{
  AddNameValuePair(aSource,aName,aFilename);
  return NS_OK;
}




NS_IMETHODIMP
nsFSTextPlain::Init()
{
  mBody.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
nsFSTextPlain::GetEncodedSubmission(nsIURI* aURI,
                                    nsIInputStream** aPostDataStream)
{
  nsresult rv = NS_OK;

  
  
  
  PRBool isMailto = PR_FALSE;
  aURI->SchemeIs("mailto", &isMailto);
  if (isMailto) {
    nsCAutoString path;
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

    
    nsCOMPtr<nsIInputStream> bodyStream;
    rv = NS_NewStringInputStream(getter_AddRefs(bodyStream),
                                          mBody);
    if (!bodyStream) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    
    nsCOMPtr<nsIMIMEInputStream> mimeStream
        = do_CreateInstance("@mozilla.org/network/mime-input-stream;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    mimeStream->AddHeader("Content-Type", "text/plain");
    mimeStream->SetAddContentLength(PR_TRUE);
    mimeStream->SetData(bodyStream);
    CallQueryInterface(mimeStream, aPostDataStream);
    NS_ADDREF(*aPostDataStream);
  }

  return rv;
}










NS_IMPL_ISUPPORTS1(nsFormSubmission, nsIFormSubmission)




static nsresult
SendJSWarning(nsIContent* aContent,
               const char* aWarningName)
{
  return SendJSWarning(aContent, aWarningName, nsnull, 0);
}

static nsresult
SendJSWarning(nsIContent* aContent,
               const char* aWarningName,
               const nsAFlatString& aWarningArg1)
{
  const PRUnichar* formatStrings[1] = { aWarningArg1.get() };
  return SendJSWarning(aContent, aWarningName, formatStrings, 1);
}

static nsresult
SendJSWarning(nsIContent* aContent,
              const char* aWarningName,
              const PRUnichar** aWarningArgs, PRUint32 aWarningArgsLen)
{
  

  nsIDocument* document = aContent->GetDocument();
  nsIURI *documentURI = nsnull;
  if (document) {
    documentURI = document->GetDocumentURI();
    NS_ENSURE_TRUE(documentURI, NS_ERROR_UNEXPECTED);
  }

  return nsContentUtils::ReportToConsole(nsContentUtils::eFORMS_PROPERTIES,
                                         aWarningName,
                                         aWarningArgs, aWarningArgsLen,
                                         documentURI,
                                         EmptyString(), 0, 0,
                                         nsIScriptError::warningFlag,
                                         "HTML");
}

nsresult
GetSubmissionFromForm(nsGenericHTMLElement* aForm,
                      nsIFormSubmission** aFormSubmission)
{
  nsresult rv = NS_OK;

  
  
  
  nsIDocument* doc = aForm->GetCurrentDoc();
  NS_ASSERTION(doc, "Should have doc if we're building submission!");

  
  PRUint8 ctrlsModAtSubmit = 0;
  PRUint32 bidiOptions = doc->GetBidiOptions();
  ctrlsModAtSubmit = GET_BIDI_OPTION_CONTROLSTEXTMODE(bidiOptions);

  
  PRInt32 enctype = NS_FORM_ENCTYPE_URLENCODED;
  nsFormSubmission::GetEnumAttr(aForm, nsGkAtoms::enctype, &enctype);

  
  PRInt32 method = NS_FORM_METHOD_GET;
  nsFormSubmission::GetEnumAttr(aForm, nsGkAtoms::method, &method);

  
  nsCAutoString charset;
  nsFormSubmission::GetSubmitCharset(aForm, ctrlsModAtSubmit, charset);

  
  nsCOMPtr<nsISaveAsCharset> encoder;
  nsFormSubmission::GetEncoder(aForm, charset, getter_AddRefs(encoder));

  if (!encoder)
    charset.AssignLiteral("UTF-8");

  
  nsCOMPtr<nsIFormProcessor> formProcessor =
    do_GetService(kFormProcessorCID, &rv);

  
  
  
  
  
  
  
  
  if (method == NS_FORM_METHOD_POST &&
      enctype == NS_FORM_ENCTYPE_MULTIPART) {
    *aFormSubmission = new nsFSMultipartFormData(charset, encoder,
                                                 formProcessor, bidiOptions);
  } else if (method == NS_FORM_METHOD_POST &&
             enctype == NS_FORM_ENCTYPE_TEXTPLAIN) {
    *aFormSubmission = new nsFSTextPlain(charset, encoder,
                                         formProcessor, bidiOptions);
  } else {
    if (enctype == NS_FORM_ENCTYPE_MULTIPART ||
        enctype == NS_FORM_ENCTYPE_TEXTPLAIN) {
      nsAutoString enctypeStr;
      aForm->GetAttr(kNameSpaceID_None, nsGkAtoms::enctype, enctypeStr);
      SendJSWarning(aForm, "ForgotPostWarning", PromiseFlatString(enctypeStr));
    }
    *aFormSubmission = new nsFSURLEncoded(charset, encoder,
                                          formProcessor, bidiOptions, method);
  }
  NS_ENSURE_TRUE(*aFormSubmission, NS_ERROR_OUT_OF_MEMORY);
  NS_ADDREF(*aFormSubmission);


  
  
  static_cast<nsFormSubmission*>(*aFormSubmission)->Init();

  return NS_OK;
}

nsresult
nsFormSubmission::SubmitTo(nsIURI* aActionURI, const nsAString& aTarget,
                           nsIContent* aSource, nsILinkHandler* aLinkHandler,
                           nsIDocShell** aDocShell, nsIRequest** aRequest)
{
  nsresult rv;

  
  
  
  nsCOMPtr<nsIInputStream> postDataStream;
  rv = GetEncodedSubmission(aActionURI, getter_AddRefs(postDataStream));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  NS_ENSURE_ARG_POINTER(aLinkHandler);

  return aLinkHandler->OnLinkClickSync(aSource, aActionURI,
                                       PromiseFlatString(aTarget).get(),
                                       postDataStream, nsnull,
                                       aDocShell, aRequest);
}



void
nsFormSubmission::GetSubmitCharset(nsGenericHTMLElement* aForm,
                                   PRUint8 aCtrlsModAtSubmit,
                                   nsACString& oCharset)
{
  oCharset.AssignLiteral("UTF-8"); 

  nsresult rv = NS_OK;
  nsAutoString acceptCharsetValue;
  aForm->GetAttr(kNameSpaceID_None, nsGkAtoms::acceptcharset,
                 acceptCharsetValue);

  PRInt32 charsetLen = acceptCharsetValue.Length();
  if (charsetLen > 0) {
    PRInt32 offset=0;
    PRInt32 spPos=0;
    
    nsCOMPtr<nsICharsetAlias> calias(do_GetService(NS_CHARSETALIAS_CONTRACTID, &rv));
    if (NS_FAILED(rv)) {
      return;
    }
    if (calias) {
      do {
        spPos = acceptCharsetValue.FindChar(PRUnichar(' '), offset);
        PRInt32 cnt = ((-1==spPos)?(charsetLen-offset):(spPos-offset));
        if (cnt > 0) {
          nsAutoString uCharset;
          acceptCharsetValue.Mid(uCharset, offset, cnt);

          if (NS_SUCCEEDED(calias->
                           GetPreferred(NS_LossyConvertUTF16toASCII(uCharset),
                                        oCharset)))
            return;
        }
        offset = spPos + 1;
      } while (spPos != -1);
    }
  }
  
  
  nsIDocument* doc = aForm->GetDocument();
  if (doc) {
    oCharset = doc->GetDocumentCharacterSet();
  }

  if (aCtrlsModAtSubmit==IBMBIDI_CONTROLSTEXTMODE_VISUAL
     && oCharset.Equals(NS_LITERAL_CSTRING("windows-1256"),
                        nsCaseInsensitiveCStringComparator())) {

    oCharset.AssignLiteral("IBM864");
  }
  else if (aCtrlsModAtSubmit==IBMBIDI_CONTROLSTEXTMODE_LOGICAL
          && oCharset.Equals(NS_LITERAL_CSTRING("IBM864"),
                             nsCaseInsensitiveCStringComparator())) {
    oCharset.AssignLiteral("IBM864i");
  }
  else if (aCtrlsModAtSubmit==IBMBIDI_CONTROLSTEXTMODE_VISUAL
          && oCharset.Equals(NS_LITERAL_CSTRING("ISO-8859-6"),
                             nsCaseInsensitiveCStringComparator())) {
    oCharset.AssignLiteral("IBM864");
  }
  else if (aCtrlsModAtSubmit==IBMBIDI_CONTROLSTEXTMODE_VISUAL
          && oCharset.Equals(NS_LITERAL_CSTRING("UTF-8"),
                             nsCaseInsensitiveCStringComparator())) {
    oCharset.AssignLiteral("IBM864");
  }

}



nsresult
nsFormSubmission::GetEncoder(nsGenericHTMLElement* aForm,
                             const nsACString& aCharset,
                             nsISaveAsCharset** aEncoder)
{
  *aEncoder = nsnull;
  nsresult rv = NS_OK;

  nsCAutoString charset(aCharset);
  
  
  if (charset.EqualsLiteral("ISO-8859-1")) {
    charset.AssignLiteral("windows-1252");
  }

  
  
  if (StringBeginsWith(charset, NS_LITERAL_CSTRING("UTF-16")) || 
      StringBeginsWith(charset, NS_LITERAL_CSTRING("UTF-32"))) {
    charset.AssignLiteral("UTF-8");
  }

  rv = CallCreateInstance( NS_SAVEASCHARSET_CONTRACTID, aEncoder);
  NS_ASSERTION(NS_SUCCEEDED(rv), "create nsISaveAsCharset failed");
  NS_ENSURE_SUCCESS(rv, rv);

  rv = (*aEncoder)->Init(charset.get(),
                         (nsISaveAsCharset::attr_EntityAfterCharsetConv + 
                          nsISaveAsCharset::attr_FallbackDecimalNCR),
                         0);
  NS_ASSERTION(NS_SUCCEEDED(rv), "initialize nsISaveAsCharset failed");
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


nsresult
nsFormSubmission::UnicodeToNewBytes(const nsAString& aStr, 
                                    nsISaveAsCharset* aEncoder,
                                    nsACString& aOut)
{
  PRUint8 ctrlsModAtSubmit = GET_BIDI_OPTION_CONTROLSTEXTMODE(mBidiOptions);
  PRUint8 textDirAtSubmit = GET_BIDI_OPTION_DIRECTION(mBidiOptions);
  
  nsAutoString newBuffer;
  
  if (ctrlsModAtSubmit == IBMBIDI_CONTROLSTEXTMODE_VISUAL
     && mCharset.Equals(NS_LITERAL_CSTRING("windows-1256"),
                        nsCaseInsensitiveCStringComparator())) {
    Conv_06_FE_WithReverse(nsString(aStr),
                           newBuffer,
                           textDirAtSubmit);
  }
  else if (ctrlsModAtSubmit == IBMBIDI_CONTROLSTEXTMODE_LOGICAL
          && mCharset.Equals(NS_LITERAL_CSTRING("IBM864"),
                             nsCaseInsensitiveCStringComparator())) {
    
    
    Conv_FE_06(nsString(aStr), newBuffer);
    if (textDirAtSubmit == 2) { 
    
      PRInt32 len = newBuffer.Length();
      PRUint32 z = 0;
      nsAutoString temp;
      temp.SetLength(len);
      while (--len >= 0)
        temp.SetCharAt(newBuffer.CharAt(len), z++);
      newBuffer = temp;
    }
  }
  else if (ctrlsModAtSubmit == IBMBIDI_CONTROLSTEXTMODE_VISUAL
          && mCharset.Equals(NS_LITERAL_CSTRING("IBM864"),
                             nsCaseInsensitiveCStringComparator())
                  && textDirAtSubmit == IBMBIDI_TEXTDIRECTION_RTL) {

    Conv_FE_06(nsString(aStr), newBuffer);
    
    PRInt32 len = newBuffer.Length();
    PRUint32 z = 0;
    nsAutoString temp;
    temp.SetLength(len);
    while (--len >= 0)
      temp.SetCharAt(newBuffer.CharAt(len), z++);
    newBuffer = temp;
  }
  else {
    newBuffer = aStr;
  }

  nsXPIDLCString res;
  if (!newBuffer.IsEmpty()) {
    aOut.Truncate();
    nsresult rv = aEncoder->Convert(newBuffer.get(), getter_Copies(res));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  aOut = res;
  return NS_OK;
}



void
nsFormSubmission::GetEnumAttr(nsGenericHTMLElement* aContent,
                              nsIAtom* atom, PRInt32* aValue)
{
  const nsAttrValue* value = aContent->GetParsedAttr(atom);
  if (value && value->Type() == nsAttrValue::eEnum) {
    *aValue = value->GetEnumValue();
  }
}

nsresult
nsFormSubmission::EncodeVal(const nsAString& aStr, nsACString& aOut)
{
  NS_ASSERTION(mEncoder, "Encoder not available. Losing data !");
  if (mEncoder)
    return UnicodeToNewBytes(aStr, mEncoder, aOut);

  
  CopyUTF16toUTF8(aStr, aOut);
  return NS_OK;
}

nsresult
nsFormSubmission::ProcessValue(nsIDOMHTMLElement* aSource,
                               const nsAString& aName, const nsAString& aValue,
                               nsAString& aResult) 
{
  
  if (aName.EqualsLiteral("_charset_")) {
    nsCOMPtr<nsIFormControl> formControl = do_QueryInterface(aSource);
    if (formControl && formControl->GetType() == NS_FORM_INPUT_HIDDEN) {
        CopyASCIItoUTF16(mCharset, aResult);
        return NS_OK;
    }
  }

  nsresult rv = NS_OK;
  aResult = aValue;
  if (mFormProcessor) {
    rv = mFormProcessor->ProcessValue(aSource, aName, aResult);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Unable to Notify form process observer");
  }

  return rv;
}
