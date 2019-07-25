




































#include "nsDOMBlobBuilder.h"
#include "jstypedarray.h"
#include "nsAutoPtr.h"
#include "nsDOMClassInfoID.h"
#include "nsIMultiplexInputStream.h"
#include "nsStringStream.h"
#include "nsTArray.h"
#include "nsJSUtils.h"
#include "nsContentUtils.h"

using namespace mozilla;

NS_IMETHODIMP
nsDOMMultipartFile::GetSize(PRUint64* aLength)
{
  if (mLength == UINT64_MAX) {
    CheckedUint64 length = 0;
  
    PRUint32 i;
    PRUint32 len = mBlobs.Length();
    for (i = 0; i < len; i++) {
      nsIDOMBlob* blob = mBlobs.ElementAt(i).get();
      PRUint64 l = 0;
  
      nsresult rv = blob->GetSize(&l);
      NS_ENSURE_SUCCESS(rv, rv);
  
      length += l;
    }
  
    NS_ENSURE_TRUE(length.valid(), NS_ERROR_FAILURE);

    mLength = length.value();
  }

  *aLength = mLength;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMMultipartFile::GetInternalStream(nsIInputStream** aStream)
{
  nsresult rv;
  *aStream = nsnull;

  nsCOMPtr<nsIMultiplexInputStream> stream =
    do_CreateInstance("@mozilla.org/io/multiplex-input-stream;1");
  NS_ENSURE_TRUE(stream, NS_ERROR_FAILURE);

  PRUint32 i;
  for (i = 0; i < mBlobs.Length(); i++) {
    nsCOMPtr<nsIInputStream> scratchStream;
    nsIDOMBlob* blob = mBlobs.ElementAt(i).get();

    rv = blob->GetInternalStream(getter_AddRefs(scratchStream));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = stream->AppendStream(scratchStream);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return CallQueryInterface(stream, aStream);
}

already_AddRefed<nsIDOMBlob>
nsDOMMultipartFile::CreateSlice(PRUint64 aStart, PRUint64 aLength,
                                const nsAString& aContentType)
{
  
  nsTArray<nsCOMPtr<nsIDOMBlob> > blobs;

  PRUint64 length = aLength;
  PRUint64 skipStart = aStart;

  
  PRUint32 i;
  for (i = 0; length && skipStart && i < mBlobs.Length(); i++) {
    nsIDOMBlob* blob = mBlobs[i].get();

    PRUint64 l;
    nsresult rv = blob->GetSize(&l);
    NS_ENSURE_SUCCESS(rv, nsnull);

    if (skipStart < l) {
      PRUint64 upperBound = NS_MIN<PRUint64>(l - skipStart, length);

      nsCOMPtr<nsIDOMBlob> firstBlob;
      rv = blob->Slice(skipStart, skipStart + upperBound,
                       aContentType, 3,
                       getter_AddRefs(firstBlob));
      NS_ENSURE_SUCCESS(rv, nsnull);

      
      if (length == upperBound) {
        return firstBlob.forget();
      }

      blobs.AppendElement(firstBlob);
      length -= upperBound;
      i++;
      break;
    }
    skipStart -= l;
  }

  
  for (; length && i < mBlobs.Length(); i++) {
    nsIDOMBlob* blob = mBlobs[i].get();

    PRUint64 l;
    nsresult rv = blob->GetSize(&l);
    NS_ENSURE_SUCCESS(rv, nsnull);

    if (length < l) {
      nsCOMPtr<nsIDOMBlob> lastBlob;
      rv = blob->Slice(0, length, aContentType, 3,
                       getter_AddRefs(lastBlob));
      NS_ENSURE_SUCCESS(rv, nsnull);

      blobs.AppendElement(lastBlob);
    } else {
      blobs.AppendElement(blob);
    }
    length -= NS_MIN<PRUint64>(l, length);
  }

  
  nsCOMPtr<nsIDOMBlob> blob = new nsDOMMultipartFile(blobs, aContentType);
  return blob.forget();
}

DOMCI_DATA(MozBlobBuilder, nsDOMBlobBuilder)

NS_IMPL_ADDREF(nsDOMBlobBuilder)
NS_IMPL_RELEASE(nsDOMBlobBuilder)
NS_INTERFACE_MAP_BEGIN(nsDOMBlobBuilder)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMozBlobBuilder)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(MozBlobBuilder)
NS_INTERFACE_MAP_END

nsresult
nsDOMBlobBuilder::AppendVoidPtr(const void* aData, PRUint32 aLength)
{
  NS_ENSURE_ARG_POINTER(aData);

  PRUint64 offset = mDataLen;

  if (!ExpandBufferSize(aLength))
    return NS_ERROR_OUT_OF_MEMORY;

  memcpy((char*)mData + offset, aData, aLength);
  return NS_OK;
}

nsresult
nsDOMBlobBuilder::AppendString(JSString* aString, JSContext* aCx)
{
  nsDependentJSString xpcomStr;
  if (!xpcomStr.init(aCx, aString)) {
    return NS_ERROR_XPC_BAD_CONVERT_JS;
  }

  NS_ConvertUTF16toUTF8 utf8Str(xpcomStr);

  return AppendVoidPtr((void*)utf8Str.Data(),
                       utf8Str.Length());
}

nsresult
nsDOMBlobBuilder::AppendBlob(nsIDOMBlob* aBlob)
{
  NS_ENSURE_ARG_POINTER(aBlob);

  Flush();
  mBlobs.AppendElement(aBlob);

  return NS_OK;
}

nsresult
nsDOMBlobBuilder::AppendArrayBuffer(JSObject* aBuffer)
{
  return AppendVoidPtr(JS_GetArrayBufferData(aBuffer), JS_GetArrayBufferByteLength(aBuffer));
}


NS_IMETHODIMP
nsDOMBlobBuilder::GetBlob(const nsAString& aContentType,
                          nsIDOMBlob** aBlob)
{
  return GetBlobInternal(aContentType, true, aBlob);
}

nsresult
nsDOMBlobBuilder::GetBlobInternal(const nsAString& aContentType,
                                  bool aClearBuffer,
                                  nsIDOMBlob** aBlob)
{
  NS_ENSURE_ARG(aBlob);

  Flush();

  nsCOMPtr<nsIDOMBlob> blob = new nsDOMMultipartFile(mBlobs,
                                                     aContentType);
  blob.forget(aBlob);

  
  
  
  
  if (aClearBuffer) {
    mBlobs.Clear();
  }

  return NS_OK;
}


NS_IMETHODIMP
nsDOMBlobBuilder::GetFile(const nsAString& aName,
                          const nsAString& aContentType,
                          nsIDOMFile** aFile)
{
  NS_ENSURE_ARG(aFile);

  Flush();

  nsCOMPtr<nsIDOMFile> file = new nsDOMMultipartFile(mBlobs,
                                                     aName,
                                                     aContentType);
  file.forget(aFile);

  
  
  
  
  mBlobs.Clear();

  return NS_OK;
}


NS_IMETHODIMP
nsDOMBlobBuilder::Append(const jsval& aData, JSContext* aCx)
{
  

  
  if (JSVAL_IS_OBJECT(aData)) {
    JSObject* obj = JSVAL_TO_OBJECT(aData);
    if (!obj) {
      
      return NS_OK;
    }

    
    nsCOMPtr<nsIDOMBlob> blob = do_QueryInterface(
      nsContentUtils::XPConnect()->
        GetNativeOfWrapper(aCx, obj));
    if (blob)
      return AppendBlob(blob);

    
    if (js_IsArrayBuffer(obj)) {
      JSObject* buffer = js::ArrayBuffer::getArrayBuffer(obj);
      if (buffer)
        return AppendArrayBuffer(buffer);
    }
  }

  
  JSString* str = JS_ValueToString(aCx, aData);
  NS_ENSURE_TRUE(str, NS_ERROR_FAILURE);

  return AppendString(str, aCx);
}

nsresult NS_NewBlobBuilder(nsISupports* *aSupports)
{
  nsDOMBlobBuilder* builder = new nsDOMBlobBuilder();
  return CallQueryInterface(builder, aSupports);
}
