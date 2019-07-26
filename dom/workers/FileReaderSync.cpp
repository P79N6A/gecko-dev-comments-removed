





#include "FileReaderSync.h"

#include "jsfriendapi.h"
#include "mozilla/Base64.h"
#include "mozilla/dom/EncodingUtils.h"
#include "nsContentUtils.h"
#include "mozilla/dom/FileReaderSyncBinding.h"
#include "nsCExternalHandlerService.h"
#include "nsComponentManagerUtils.h"
#include "nsCOMPtr.h"
#include "nsDOMClassInfoID.h"
#include "nsError.h"
#include "nsIDOMFile.h"
#include "nsIConverterInputStream.h"
#include "nsIInputStream.h"
#include "nsISeekableStream.h"
#include "nsISupportsImpl.h"
#include "nsNetUtil.h"
#include "nsServiceManagerUtils.h"

#include "File.h"
#include "RuntimeService.h"

USING_WORKERS_NAMESPACE
using namespace mozilla;
using mozilla::dom::Optional;
using mozilla::dom::GlobalObject;


already_AddRefed<FileReaderSync>
FileReaderSync::Constructor(const GlobalObject& aGlobal, ErrorResult& aRv)
{
  nsRefPtr<FileReaderSync> frs = new FileReaderSync();

  return frs.forget();
}

JSObject*
FileReaderSync::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return FileReaderSyncBinding_workers::Wrap(aCx, aScope, this);
}

JSObject*
FileReaderSync::ReadAsArrayBuffer(JSContext* aCx,
                                  JS::Handle<JSObject*> aScopeObj,
                                  JS::Handle<JSObject*> aBlob,
                                  ErrorResult& aRv)
{
  nsIDOMBlob* blob = file::GetDOMBlobFromJSObject(aBlob);
  if (!blob) {
    aRv.Throw(NS_ERROR_INVALID_ARG);
    return nullptr;
  }

  uint64_t blobSize;
  nsresult rv = blob->GetSize(&blobSize);
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return nullptr;
  }

  JS::Rooted<JSObject*> jsArrayBuffer(aCx, JS_NewArrayBuffer(aCx, blobSize));
  if (!jsArrayBuffer) {
    
    
    aRv.Throw(NS_ERROR_OUT_OF_MEMORY);
    return nullptr;
  }

  uint32_t bufferLength = JS_GetArrayBufferByteLength(jsArrayBuffer);
  uint8_t* arrayBuffer = JS_GetStableArrayBufferData(aCx, jsArrayBuffer);
  if (!arrayBuffer) {
    aRv.Throw(NS_ERROR_OUT_OF_MEMORY);
    return nullptr;
  }

  nsCOMPtr<nsIInputStream> stream;
  rv = blob->GetInternalStream(getter_AddRefs(stream));
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return nullptr;
  }

  uint32_t numRead;
  rv = stream->Read((char*)arrayBuffer, bufferLength, &numRead);
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return nullptr;
  }
  NS_ASSERTION(numRead == bufferLength, "failed to read data");

  return jsArrayBuffer;
}

void
FileReaderSync::ReadAsBinaryString(JS::Handle<JSObject*> aBlob,
                                   nsAString& aResult,
                                   ErrorResult& aRv)
{
  nsIDOMBlob* blob = file::GetDOMBlobFromJSObject(aBlob);
  if (!blob) {
    aRv.Throw(NS_ERROR_INVALID_ARG);
    return;
  }

  nsCOMPtr<nsIInputStream> stream;
  nsresult rv = blob->GetInternalStream(getter_AddRefs(stream));
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return;
  }

  uint32_t numRead;
  do {
    char readBuf[4096];
    rv = stream->Read(readBuf, sizeof(readBuf), &numRead);
    if (NS_FAILED(rv)) {
      aRv.Throw(rv);
      return;
    }

    uint32_t oldLength = aResult.Length();
    AppendASCIItoUTF16(Substring(readBuf, readBuf + numRead), aResult);
    if (aResult.Length() - oldLength != numRead) {
      aRv.Throw(NS_ERROR_OUT_OF_MEMORY);
      return;
    }
  } while (numRead > 0);
}

void
FileReaderSync::ReadAsText(JS::Handle<JSObject*> aBlob,
                           const Optional<nsAString>& aEncoding,
                           nsAString& aResult,
                           ErrorResult& aRv)
{
  nsIDOMBlob* blob = file::GetDOMBlobFromJSObject(aBlob);
  if (!blob) {
    aRv.Throw(NS_ERROR_INVALID_ARG);
    return;
  }

  nsCOMPtr<nsIInputStream> stream;
  nsresult rv = blob->GetInternalStream(getter_AddRefs(stream));
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return;
  }

  nsAutoCString encoding;
  unsigned char sniffBuf[3] = { 0, 0, 0 };
  uint32_t numRead;
  rv = stream->Read(reinterpret_cast<char*>(sniffBuf),
                    sizeof(sniffBuf), &numRead);
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return;
  }

  
  
  if (!nsContentUtils::CheckForBOM(sniffBuf, numRead, encoding)) {
    
    if (!aEncoding.WasPassed() ||
        !EncodingUtils::FindEncodingForLabel(aEncoding.Value(),
                                             encoding)) {
      
      nsAutoString type16;
      blob->GetType(type16);
      NS_ConvertUTF16toUTF8 type(type16);
      nsAutoCString specifiedCharset;
      bool haveCharset;
      int32_t charsetStart, charsetEnd;
      NS_ExtractCharsetFromContentType(type,
                                       specifiedCharset,
                                       &haveCharset,
                                       &charsetStart,
                                       &charsetEnd);
      if (!EncodingUtils::FindEncodingForLabel(specifiedCharset, encoding)) {
        
        encoding.AssignLiteral("UTF-8");
      }
    }
  }

  nsCOMPtr<nsISeekableStream> seekable = do_QueryInterface(stream);
  if (!seekable) {
    aRv.Throw(NS_ERROR_FAILURE);
    return;
  }

  
  
  rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET, 0);
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return;
  }

  rv = ConvertStream(stream, encoding.get(), aResult);
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return;
  }
}

void
FileReaderSync::ReadAsDataURL(JS::Handle<JSObject*> aBlob, nsAString& aResult,
                              ErrorResult& aRv)
{
  nsIDOMBlob* blob = file::GetDOMBlobFromJSObject(aBlob);
  if (!blob) {
    aRv.Throw(NS_ERROR_INVALID_ARG);
    return;
  }

  nsAutoString scratchResult;
  scratchResult.AssignLiteral("data:");

  nsString contentType;
  blob->GetType(contentType);

  if (contentType.IsEmpty()) {
    scratchResult.AppendLiteral("application/octet-stream");
  } else {
    scratchResult.Append(contentType);
  }
  scratchResult.AppendLiteral(";base64,");

  nsCOMPtr<nsIInputStream> stream;
  nsresult rv = blob->GetInternalStream(getter_AddRefs(stream));
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return;
  }

  uint64_t size;
  rv = blob->GetSize(&size);
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return;
  }

  nsCOMPtr<nsIInputStream> bufferedStream;
  rv = NS_NewBufferedInputStream(getter_AddRefs(bufferedStream), stream, size);
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return;
  }

  nsAutoString encodedData;
  rv = Base64EncodeInputStream(bufferedStream, encodedData, size);
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return;
  }

  scratchResult.Append(encodedData);

  aResult = scratchResult;
}

nsresult
FileReaderSync::ConvertStream(nsIInputStream *aStream,
                              const char *aCharset,
                              nsAString &aResult)
{
  nsCOMPtr<nsIConverterInputStream> converterStream =
    do_CreateInstance("@mozilla.org/intl/converter-input-stream;1");
  NS_ENSURE_TRUE(converterStream, NS_ERROR_FAILURE);

  nsresult rv = converterStream->Init(aStream, aCharset, 8192,
                  nsIConverterInputStream::DEFAULT_REPLACEMENT_CHARACTER);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIUnicharInputStream> unicharStream =
    do_QueryInterface(converterStream);
  NS_ENSURE_TRUE(unicharStream, NS_ERROR_FAILURE);

  uint32_t numChars;
  nsString result;
  while (NS_SUCCEEDED(unicharStream->ReadString(8192, result, &numChars)) &&
         numChars > 0) {
    uint32_t oldLength = aResult.Length();
    aResult.Append(result);
    if (aResult.Length() - oldLength != result.Length()) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  return rv;
}

