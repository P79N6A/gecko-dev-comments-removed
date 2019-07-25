




































#include "jstypedarray.h"
#include "nsAutoPtr.h"
#include "nsDOMClassInfo.h"
#include "nsDOMFile.h"
#include "nsIMultiplexInputStream.h"
#include "nsStringStream.h"
#include "nsTArray.h"
#include "nsJSUtils.h"
#include "nsContentUtils.h"
#include "CheckedInt.h"


#define PR_INT64_MAX (~((PRInt64)(1) << 63))
#define PR_INT64_MIN (-PR_INT64_MAX - 1)

using namespace mozilla;

class nsDOMMultipartBlob : public nsDOMFile
{
public:
  nsDOMMultipartBlob(nsTArray<nsCOMPtr<nsIDOMBlob> > aBlobs,
                     const nsAString& aContentType)
    : nsDOMFile(nsnull, aContentType),
      mBlobs(aBlobs)
  {
    mIsFullFile = false;
    mStart = 0;
    mLength = 0;
  }

  NS_IMETHOD GetSize(PRUint64*);
  NS_IMETHOD GetInternalStream(nsIInputStream**);
  NS_IMETHOD MozSlice(PRInt64 aStart, PRInt64 aEnd,
                      const nsAString& aContentType, PRUint8 optional_argc,
                      nsIDOMBlob **aBlob);

protected:
  nsTArray<nsCOMPtr<nsIDOMBlob> > mBlobs;
};

NS_IMETHODIMP
nsDOMMultipartBlob::GetSize(PRUint64* aLength)
{
  nsresult rv;
  *aLength = 0;

  if (mLength) {
    *aLength = mLength;
    return NS_OK;
  }

  CheckedUint64 length = 0;

  PRUint32 i;
  PRUint32 len = mBlobs.Length();
  for (i = 0; i < len; i++) {
    nsIDOMBlob* blob = mBlobs.ElementAt(i).get();
    PRUint64 l = 0;

    rv = blob->GetSize(&l);
    NS_ENSURE_SUCCESS(rv, rv);

    length += l;
  }

  if (!length.valid())
    return NS_ERROR_FAILURE;

  mLength = length.value();
  *aLength = mLength;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMMultipartBlob::GetInternalStream(nsIInputStream** aStream)
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

NS_IMETHODIMP
nsDOMMultipartBlob::MozSlice(PRInt64 aStart, PRInt64 aEnd,
                             const nsAString& aContentType,
                             PRUint8 optional_argc,
                             nsIDOMBlob **aBlob)
{
  nsresult rv;
  *aBlob = nsnull;

  
  PRUint64 thisLength;
  rv = GetSize(&thisLength);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!optional_argc) {
    aEnd = (PRInt64)thisLength;
  }

  ParseSize((PRInt64)thisLength, aStart, aEnd);

  
  nsTArray<nsCOMPtr<nsIDOMBlob> > blobs;

  PRInt64 length = aEnd - aStart;
  PRUint64 finalLength = length;
  PRUint64 skipStart = aStart;

  NS_ABORT_IF_FALSE(aStart + length <= thisLength, "Er, what?");

  
  PRUint32 i;
  for (i = 0; length && skipStart && i < mBlobs.Length(); i++) {
    nsIDOMBlob* blob = mBlobs[i].get();

    PRUint64 l;
    rv = blob->GetSize(&l);
    NS_ENSURE_SUCCESS(rv, rv);

    if (skipStart < l) {
      PRInt64 upperBound = NS_MIN<PRInt64>(l - skipStart, length);

      nsCOMPtr<nsIDOMBlob> firstBlob;
      rv = mBlobs.ElementAt(i)->MozSlice(skipStart, skipStart + upperBound,
                                         aContentType, 2,
                                         getter_AddRefs(firstBlob));
      NS_ENSURE_SUCCESS(rv, rv);

      
      if (length == upperBound) {
        firstBlob.forget(aBlob);
        return NS_OK;
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
    rv = blob->GetSize(&l);
    NS_ENSURE_SUCCESS(rv, rv);

    if (length < l) {
      nsCOMPtr<nsIDOMBlob> lastBlob;
      rv = mBlobs.ElementAt(i)->MozSlice(0, length, aContentType, 2,
                                         getter_AddRefs(lastBlob));
      NS_ENSURE_SUCCESS(rv, rv);

      blobs.AppendElement(lastBlob);
    } else {
      blobs.AppendElement(blob);
    }
    length -= NS_MIN<PRInt64>(l, length);
  }

  
  nsCOMPtr<nsIDOMBlob> blob = new nsDOMMultipartBlob(blobs, aContentType);
  blob.forget(aBlob);
  return NS_OK;
}

class nsDOMBlobBuilder : public nsIDOMBlobBuilder
{
public:
  nsDOMBlobBuilder()
    : mData(nsnull), mDataLen(0), mDataBufferLen(0)
  {}

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMBLOBBUILDER
protected:
  nsresult AppendVoidPtr(void* aData, PRUint32 aLength);
  nsresult AppendString(JSString* aString, JSContext* aCx);
  nsresult AppendBlob(nsIDOMBlob* aBlob);
  nsresult AppendArrayBuffer(js::ArrayBuffer* aBuffer);

  bool ExpandBufferSize(PRUint64 aSize)
  {
    if (mDataBufferLen >= mDataLen + aSize) {
      mDataLen += aSize;
      return true;
    }

    
    CheckedUint32 bufferLen = NS_MAX<PRUint32>(mDataBufferLen, 1);
    while (bufferLen.valid() && bufferLen.value() < mDataLen + aSize)
      bufferLen *= 2;

    if (!bufferLen.valid())
      return false;

    
    void* data = PR_Realloc(mData, bufferLen.value());
    if (!data)
      return false;

    mData = data;
    mDataBufferLen = bufferLen.value();
    mDataLen += aSize;
    return true;
  }

  void Flush() {
    if (mData) {
      
      

      nsCOMPtr<nsIDOMBlob> blob =
        new nsDOMMemoryFile(mData, mDataLen, EmptyString(), EmptyString());
      mBlobs.AppendElement(blob);
      mData = nsnull; 
      mDataLen = 0;
      mDataBufferLen = 0;
    }
  }

  nsTArray<nsCOMPtr<nsIDOMBlob> > mBlobs;
  void* mData;
  PRUint64 mDataLen;
  PRUint64 mDataBufferLen;
};

DOMCI_DATA(MozBlobBuilder, nsDOMBlobBuilder)

NS_IMPL_ADDREF(nsDOMBlobBuilder)
NS_IMPL_RELEASE(nsDOMBlobBuilder)
NS_INTERFACE_MAP_BEGIN(nsDOMBlobBuilder)
  NS_INTERFACE_MAP_ENTRY(nsIDOMBlobBuilder)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(MozBlobBuilder)
NS_INTERFACE_MAP_END

nsresult
nsDOMBlobBuilder::AppendVoidPtr(void* aData, PRUint32 aLength)
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
nsDOMBlobBuilder::AppendArrayBuffer(js::ArrayBuffer* aBuffer)
{
  return AppendVoidPtr(aBuffer->data, aBuffer->byteLength);
}


NS_IMETHODIMP
nsDOMBlobBuilder::GetBlob(const nsAString& aContentType,
                          nsIDOMBlob** aBlob)
{
  NS_ENSURE_ARG(aBlob);

  Flush();

  nsCOMPtr<nsIDOMBlob> blob = new nsDOMMultipartBlob(mBlobs,
                                                     aContentType);
  blob.forget(aBlob);

  
  
  
  
  mBlobs.Clear();

  return NS_OK;
}


NS_IMETHODIMP
nsDOMBlobBuilder::Append(const jsval& aData, JSContext* aCx)
{
  

  
  if (JSVAL_IS_OBJECT(aData)) {
    JSObject* obj = JSVAL_TO_OBJECT(aData);
    NS_ASSERTION(obj, "Er, what?");

    
    nsCOMPtr<nsIDOMBlob> blob = do_QueryInterface(
      nsContentUtils::XPConnect()->
        GetNativeOfWrapper(aCx, obj));
    if (blob)
      return AppendBlob(blob);

    
    if (js_IsArrayBuffer(obj)) {
      js::ArrayBuffer* buffer = js::ArrayBuffer::fromJSObject(obj);
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
