




































#include "jstypedarray.h"
#include "nsAutoPtr.h"
#include "nsDOMClassInfoID.h"
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

class nsDOMMultipartFile : public nsDOMFileBase
{
public:
  
  nsDOMMultipartFile(nsTArray<nsCOMPtr<nsIDOMBlob> > aBlobs,
                     const nsAString& aName,
                     const nsAString& aContentType)
    : nsDOMFileBase(aName, aContentType, PR_UINT64_MAX),
      mBlobs(aBlobs)
  {
  }

  
  nsDOMMultipartFile(nsTArray<nsCOMPtr<nsIDOMBlob> > aBlobs,
                     const nsAString& aContentType)
    : nsDOMFileBase(aContentType, PR_UINT64_MAX),
      mBlobs(aBlobs)
  {
  }

  already_AddRefed<nsIDOMBlob>
  CreateSlice(PRUint64 aStart, PRUint64 aLength, const nsAString& aContentType);

  NS_IMETHOD GetSize(PRUint64*);
  NS_IMETHOD GetInternalStream(nsIInputStream**);

protected:
  nsTArray<nsCOMPtr<nsIDOMBlob> > mBlobs;
};

NS_IMETHODIMP
nsDOMMultipartFile::GetSize(PRUint64* aLength)
{
  if (mLength == PR_UINT64_MAX) {
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
      rv = blob->MozSlice(skipStart, skipStart + upperBound,
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
      rv = blob->MozSlice(0, length, aContentType, 3,
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

class nsDOMBlobBuilder : public nsIDOMMozBlobBuilder
{
public:
  nsDOMBlobBuilder()
    : mData(nsnull), mDataLen(0), mDataBufferLen(0)
  {}

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMMOZBLOBBUILDER
protected:
  nsresult AppendVoidPtr(void* aData, PRUint32 aLength);
  nsresult AppendString(JSString* aString, JSContext* aCx);
  nsresult AppendBlob(nsIDOMBlob* aBlob);
  nsresult AppendArrayBuffer(JSObject* aBuffer);

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
  NS_INTERFACE_MAP_ENTRY(nsIDOMMozBlobBuilder)
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
nsDOMBlobBuilder::AppendArrayBuffer(JSObject* aBuffer)
{
  return AppendVoidPtr(JS_GetArrayBufferData(aBuffer), JS_GetArrayBufferByteLength(aBuffer));
}


NS_IMETHODIMP
nsDOMBlobBuilder::GetBlob(const nsAString& aContentType,
                          nsIDOMBlob** aBlob)
{
  NS_ENSURE_ARG(aBlob);

  Flush();

  nsCOMPtr<nsIDOMBlob> blob = new nsDOMMultipartFile(mBlobs,
                                                     aContentType);
  blob.forget(aBlob);

  
  
  
  
  mBlobs.Clear();

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
    NS_ASSERTION(obj, "Er, what?");

    
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
