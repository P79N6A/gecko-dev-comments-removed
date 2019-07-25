




#include "nsDOMFileReader.h"

#include "nsContentCID.h"
#include "nsContentUtils.h"
#include "nsDOMClassInfoID.h"
#include "nsDOMFile.h"
#include "nsError.h"
#include "nsCharsetAlias.h"
#include "nsICharsetConverterManager.h"
#include "nsIConverterInputStream.h"
#include "nsIFile.h"
#include "nsIFileStreams.h"
#include "nsIInputStream.h"
#include "nsIMIMEService.h"
#include "nsIUnicodeDecoder.h"
#include "nsNetCID.h"
#include "nsNetUtil.h"

#include "plbase64.h"
#include "prmem.h"

#include "nsLayoutCID.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsIURI.h"
#include "nsStreamUtils.h"
#include "nsXPCOM.h"
#include "nsIDOMEventListener.h"
#include "nsIJSContextStack.h"
#include "nsJSEnvironment.h"
#include "nsIScriptGlobalObject.h"
#include "nsCExternalHandlerService.h"
#include "nsIStreamConverterService.h"
#include "nsCycleCollectionParticipant.h"
#include "nsLayoutStatics.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsBlobProtocolHandler.h"
#include "mozilla/Preferences.h"
#include "xpcpublic.h"
#include "nsIScriptSecurityManager.h"
#include "nsDOMJSUtils.h"
#include "nsDOMEventTargetHelper.h"

#include "jsfriendapi.h"

using namespace mozilla;

#define LOAD_STR "load"
#define LOADSTART_STR "loadstart"
#define LOADEND_STR "loadend"

using mozilla::dom::FileIOObject;

NS_IMPL_CYCLE_COLLECTION_CLASS(nsDOMFileReader)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsDOMFileReader,
                                                  FileIOObject)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mFile)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mPrincipal)
  NS_CYCLE_COLLECTION_TRAVERSE_EVENT_HANDLER(load)
  NS_CYCLE_COLLECTION_TRAVERSE_EVENT_HANDLER(loadstart)
  NS_CYCLE_COLLECTION_TRAVERSE_EVENT_HANDLER(loadend)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsDOMFileReader,
                                                FileIOObject)
  tmp->mResultArrayBuffer = nullptr;
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mFile)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mPrincipal)
  NS_CYCLE_COLLECTION_UNLINK_EVENT_HANDLER(load)
  NS_CYCLE_COLLECTION_UNLINK_EVENT_HANDLER(loadstart)
  NS_CYCLE_COLLECTION_UNLINK_EVENT_HANDLER(loadend)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END


NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN_INHERITED(nsDOMFileReader,
                                               nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mResultArrayBuffer)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

DOMCI_DATA(FileReader, nsDOMFileReader)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsDOMFileReader)
  NS_INTERFACE_MAP_ENTRY(nsIDOMFileReader)
  NS_INTERFACE_MAP_ENTRY(nsIInterfaceRequestor)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsIJSNativeInitializer)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(FileReader)
NS_INTERFACE_MAP_END_INHERITING(FileIOObject)

NS_IMPL_ADDREF_INHERITED(nsDOMFileReader, FileIOObject)
NS_IMPL_RELEASE_INHERITED(nsDOMFileReader, FileIOObject)

void
nsDOMFileReader::RootResultArrayBuffer()
{
  nsContentUtils::PreserveWrapper(
    static_cast<nsIDOMEventTarget*>(
      static_cast<nsDOMEventTargetHelper*>(this)), this);
}



nsDOMFileReader::nsDOMFileReader()
  : mFileData(nullptr),
    mDataLen(0), mDataFormat(FILE_AS_BINARY),
    mResultArrayBuffer(nullptr)     
{
  nsLayoutStatics::AddRef();
  SetDOMStringToNull(mResult);
}

nsDOMFileReader::~nsDOMFileReader()
{
  FreeFileData();

  nsLayoutStatics::Release();
}

nsresult
nsDOMFileReader::Init()
{
  nsDOMEventTargetHelper::Init();

  nsIScriptSecurityManager *secMan = nsContentUtils::GetSecurityManager();
  nsCOMPtr<nsIPrincipal> subjectPrincipal;
  if (secMan) {
    nsresult rv = secMan->GetSubjectPrincipal(getter_AddRefs(subjectPrincipal));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  NS_ENSURE_STATE(subjectPrincipal);
  mPrincipal.swap(subjectPrincipal);

  return NS_OK;
}

NS_IMPL_EVENT_HANDLER(nsDOMFileReader, load)
NS_IMPL_EVENT_HANDLER(nsDOMFileReader, loadstart)
NS_IMPL_EVENT_HANDLER(nsDOMFileReader, loadend)
NS_IMPL_FORWARD_EVENT_HANDLER(nsDOMFileReader, abort, FileIOObject)
NS_IMPL_FORWARD_EVENT_HANDLER(nsDOMFileReader, progress, FileIOObject)
NS_IMPL_FORWARD_EVENT_HANDLER(nsDOMFileReader, error, FileIOObject)

NS_IMETHODIMP
nsDOMFileReader::Initialize(nsISupports* aOwner, JSContext* cx, JSObject* obj,
                            uint32_t argc, jsval *argv)
{
  nsCOMPtr<nsPIDOMWindow> owner = do_QueryInterface(aOwner);
  if (!owner) {
    NS_WARNING("Unexpected nsIJSNativeInitializer owner");
    return NS_OK;
  }

  BindToOwner(owner);

  
  
  nsCOMPtr<nsIScriptObjectPrincipal> scriptPrincipal = do_QueryInterface(aOwner);
  NS_ENSURE_STATE(scriptPrincipal);
  mPrincipal = scriptPrincipal->GetPrincipal();

  return NS_OK; 
}



NS_IMETHODIMP
nsDOMFileReader::GetInterface(const nsIID & aIID, void **aResult)
{
  return QueryInterface(aIID, aResult);
}



NS_IMETHODIMP
nsDOMFileReader::GetReadyState(uint16_t *aReadyState)
{
  return FileIOObject::GetReadyState(aReadyState);
}

NS_IMETHODIMP
nsDOMFileReader::GetResult(JSContext* aCx, jsval* aResult)
{
  if (mDataFormat == FILE_AS_ARRAYBUFFER) {
    if (mReadyState == nsIDOMFileReader::DONE && mResultArrayBuffer) {
      JSObject* tmp = mResultArrayBuffer;
      *aResult = OBJECT_TO_JSVAL(tmp);
    } else {
      *aResult = JSVAL_NULL;
    }
    if (!JS_WrapValue(aCx, aResult)) {
      return NS_ERROR_FAILURE;
    }
    return NS_OK;
  }
 
  nsString tmpResult = mResult;
  if (!xpc::StringToJsval(aCx, tmpResult, aResult)) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDOMFileReader::GetError(nsIDOMDOMError** aError)
{
  return FileIOObject::GetError(aError);
}

NS_IMETHODIMP
nsDOMFileReader::ReadAsArrayBuffer(nsIDOMBlob* aFile, JSContext* aCx)
{
  return ReadFileContent(aCx, aFile, EmptyString(), FILE_AS_ARRAYBUFFER);
}

NS_IMETHODIMP
nsDOMFileReader::ReadAsBinaryString(nsIDOMBlob* aFile)
{
  return ReadFileContent(nullptr, aFile, EmptyString(), FILE_AS_BINARY);
}

NS_IMETHODIMP
nsDOMFileReader::ReadAsText(nsIDOMBlob* aFile,
                            const nsAString &aCharset)
{
  return ReadFileContent(nullptr, aFile, aCharset, FILE_AS_TEXT);
}

NS_IMETHODIMP
nsDOMFileReader::ReadAsDataURL(nsIDOMBlob* aFile)
{
  return ReadFileContent(nullptr, aFile, EmptyString(), FILE_AS_DATAURL);
}

NS_IMETHODIMP
nsDOMFileReader::Abort()
{
  return FileIOObject::Abort();
}

nsresult
nsDOMFileReader::DoAbort(nsAString& aEvent)
{
  
  SetDOMStringToNull(mResult);
  mResultArrayBuffer = nullptr;
    
  
  if (mChannel) {
    
    mChannel->Cancel(NS_ERROR_FAILURE);
    mChannel = nullptr;
  }
  mFile = nullptr;

  
  FreeFileData();

  
  aEvent = NS_LITERAL_STRING(LOADEND_STR);
  return NS_OK;
}

static
NS_METHOD
ReadFuncBinaryString(nsIInputStream* in,
                     void* closure,
                     const char* fromRawSegment,
                     uint32_t toOffset,
                     uint32_t count,
                     uint32_t *writeCount)
{
  PRUnichar* dest = static_cast<PRUnichar*>(closure) + toOffset;
  PRUnichar* end = dest + count;
  const unsigned char* source = (const unsigned char*)fromRawSegment;
  while (dest != end) {
    *dest = *source;
    ++dest;
    ++source;
  }
  *writeCount = count;

  return NS_OK;
}

nsresult
nsDOMFileReader::DoOnDataAvailable(nsIRequest *aRequest,
                                   nsISupports *aContext,
                                   nsIInputStream *aInputStream,
                                   uint32_t aOffset,
                                   uint32_t aCount)
{
  if (mDataFormat == FILE_AS_BINARY) {
    
    NS_ASSERTION(mResult.Length() == aOffset,
                 "unexpected mResult length");
    uint32_t oldLen = mResult.Length();
    PRUnichar *buf = nullptr;
    mResult.GetMutableData(&buf, oldLen + aCount, fallible_t());
    NS_ENSURE_TRUE(buf, NS_ERROR_OUT_OF_MEMORY);

    uint32_t bytesRead = 0;
    aInputStream->ReadSegments(ReadFuncBinaryString, buf + oldLen, aCount,
                               &bytesRead);
    NS_ASSERTION(bytesRead == aCount, "failed to read data");
  }
  else if (mDataFormat == FILE_AS_ARRAYBUFFER) {
    uint32_t bytesRead = 0;
    aInputStream->Read((char*)JS_GetArrayBufferData(mResultArrayBuffer, NULL) + aOffset,
                       aCount, &bytesRead);
    NS_ASSERTION(bytesRead == aCount, "failed to read data");
  }
  else {
    
    mFileData = (char *)PR_Realloc(mFileData, aOffset + aCount);
    NS_ENSURE_TRUE(mFileData, NS_ERROR_OUT_OF_MEMORY);

    uint32_t bytesRead = 0;
    aInputStream->Read(mFileData + aOffset, aCount, &bytesRead);
    NS_ASSERTION(bytesRead == aCount, "failed to read data");

    mDataLen += aCount;
  }

  return NS_OK;
}

nsresult
nsDOMFileReader::DoOnStopRequest(nsIRequest *aRequest,
                                 nsISupports *aContext,
                                 nsresult aStatus,
                                 nsAString& aSuccessEvent,
                                 nsAString& aTerminationEvent)
{
  
  nsCOMPtr<nsIChannel> channel;
  mChannel.swap(channel);

  nsCOMPtr<nsIDOMBlob> file;
  mFile.swap(file);

  aSuccessEvent = NS_LITERAL_STRING(LOAD_STR);
  aTerminationEvent = NS_LITERAL_STRING(LOADEND_STR);

  
  if (NS_FAILED(aStatus)) {
    FreeFileData();
    return NS_OK;
  }

  nsresult rv = NS_OK;
  switch (mDataFormat) {
    case FILE_AS_ARRAYBUFFER:
      break; 
    case FILE_AS_BINARY:
      break; 
    case FILE_AS_TEXT:
      rv = GetAsText(mCharset, mFileData, mDataLen, mResult);
      break;
    case FILE_AS_DATAURL:
      rv = GetAsDataURL(file, mFileData, mDataLen, mResult);
      break;
  }
  
  mResult.SetIsVoid(false);

  FreeFileData();

  return rv;
}



nsresult
nsDOMFileReader::ReadFileContent(JSContext* aCx,
                                 nsIDOMBlob* aFile,
                                 const nsAString &aCharset,
                                 eDataFormat aDataFormat)
{
  nsresult rv;
  NS_ENSURE_TRUE(aFile, NS_ERROR_NULL_POINTER);

  
  Abort();
  mError = nullptr;
  SetDOMStringToNull(mResult);
  mTransferred = 0;
  mTotal = 0;
  mReadyState = nsIDOMFileReader::EMPTY;
  FreeFileData();

  mFile = aFile;
  mDataFormat = aDataFormat;
  CopyUTF16toUTF8(aCharset, mCharset);

  
  {
    
    
    
    nsDOMFileInternalUrlHolder urlHolder(mFile, mPrincipal);

    nsCOMPtr<nsIURI> uri;
    rv = NS_NewURI(getter_AddRefs(uri), urlHolder.mUrl);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = NS_NewChannel(getter_AddRefs(mChannel), uri);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  mTotal = mozilla::dom::kUnknownSize;
  mFile->GetSize(&mTotal);

  rv = mChannel->AsyncOpen(this, nullptr);
  NS_ENSURE_SUCCESS(rv, rv);

  
  mReadyState = nsIDOMFileReader::LOADING;
  DispatchProgressEvent(NS_LITERAL_STRING(LOADSTART_STR));
  
  if (mDataFormat == FILE_AS_ARRAYBUFFER) {
    RootResultArrayBuffer();
    mResultArrayBuffer = JS_NewArrayBuffer(aCx, mTotal);
    if (!mResultArrayBuffer) {
      NS_WARNING("Failed to create JS array buffer");
      return NS_ERROR_FAILURE;
    }
  }
 
  return NS_OK;
}

nsresult
nsDOMFileReader::GetAsText(const nsACString &aCharset,
                           const char *aFileData,
                           uint32_t aDataLen,
                           nsAString& aResult)
{
  nsresult rv;
  nsCAutoString charsetGuess;
  if (!aCharset.IsEmpty()) {
    charsetGuess = aCharset;
  } else {
    rv = nsContentUtils::GuessCharset(aFileData, aDataLen, charsetGuess);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCAutoString charset;
  rv = nsCharsetAlias::GetPreferred(charsetGuess, charset);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = ConvertStream(aFileData, aDataLen, charset.get(), aResult);

  return NS_OK;
}

nsresult
nsDOMFileReader::GetAsDataURL(nsIDOMBlob *aFile,
                              const char *aFileData,
                              uint32_t aDataLen,
                              nsAString& aResult)
{
  aResult.AssignLiteral("data:");

  nsresult rv;
  nsString contentType;
  rv = aFile->GetType(contentType);
  if (NS_SUCCEEDED(rv) && !contentType.IsEmpty()) {
    aResult.Append(contentType);
  } else {
    aResult.AppendLiteral("application/octet-stream");
  }
  aResult.AppendLiteral(";base64,");

  uint32_t totalRead = 0;
  while (aDataLen > totalRead) {
    uint32_t numEncode = 4096;
    uint32_t amtRemaining = aDataLen - totalRead;
    if (numEncode > amtRemaining)
      numEncode = amtRemaining;

    
    if (numEncode > 3) {
      uint32_t leftOver = numEncode % 3;
      numEncode -= leftOver;
    }

    
    char *base64 = PL_Base64Encode(aFileData + totalRead, numEncode, nullptr);
    AppendASCIItoUTF16(nsDependentCString(base64), aResult);
    PR_Free(base64);

    totalRead += numEncode;
  }

  return NS_OK;
}

nsresult
nsDOMFileReader::ConvertStream(const char *aFileData,
                               uint32_t aDataLen,
                               const char *aCharset,
                               nsAString &aResult)
{
  nsresult rv;
  nsCOMPtr<nsICharsetConverterManager> charsetConverter = 
    do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIUnicodeDecoder> unicodeDecoder;
  rv = charsetConverter->GetUnicodeDecoder(aCharset, getter_AddRefs(unicodeDecoder));
  NS_ENSURE_SUCCESS(rv, rv);

  int32_t destLength;
  rv = unicodeDecoder->GetMaxLength(aFileData, aDataLen, &destLength);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aResult.SetLength(destLength, fallible_t()))
    return NS_ERROR_OUT_OF_MEMORY;

  int32_t srcLength = aDataLen;
  rv = unicodeDecoder->Convert(aFileData, &srcLength, aResult.BeginWriting(), &destLength);
  aResult.SetLength(destLength); 

  return rv;
}
