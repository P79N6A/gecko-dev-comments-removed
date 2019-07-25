




































#ifndef nsDOMBlobBuilder_h
#define nsDOMBlobBuilder_h

#include "nsDOMFile.h"
#include "CheckedInt.h"

#include "mozilla/StdInt.h"

using namespace mozilla;

class nsDOMMultipartFile : public nsDOMFileBase
{
public:
  
  nsDOMMultipartFile(nsTArray<nsCOMPtr<nsIDOMBlob> > aBlobs,
                     const nsAString& aName,
                     const nsAString& aContentType)
    : nsDOMFileBase(aName, aContentType, UINT64_MAX),
      mBlobs(aBlobs)
  {
  }

  
  nsDOMMultipartFile(nsTArray<nsCOMPtr<nsIDOMBlob> > aBlobs,
                     const nsAString& aContentType)
    : nsDOMFileBase(aContentType, UINT64_MAX),
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

class nsDOMBlobBuilder : public nsIDOMMozBlobBuilder
{
public:
  nsDOMBlobBuilder()
    : mData(nsnull), mDataLen(0), mDataBufferLen(0)
  {}

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMMOZBLOBBUILDER

  nsresult GetBlobInternal(const nsAString& aContentType,
                           bool aClearBuffer, nsIDOMBlob** aBlob);
  nsresult AppendVoidPtr(const void* aData, PRUint32 aLength);

protected:
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

#endif
