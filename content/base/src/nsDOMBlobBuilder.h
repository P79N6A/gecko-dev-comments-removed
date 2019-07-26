




#ifndef nsDOMBlobBuilder_h
#define nsDOMBlobBuilder_h

#include "nsDOMFile.h"

#include "mozilla/CheckedInt.h"
#include "mozilla/Attributes.h"
#include <algorithm>

class nsDOMMultipartFile : public nsDOMFile,
                           public nsIJSNativeInitializer
{
public:
  
  nsDOMMultipartFile(nsTArray<nsCOMPtr<nsIDOMBlob> > aBlobs,
                     const nsAString& aName,
                     const nsAString& aContentType)
    : nsDOMFile(aName, aContentType, UINT64_MAX),
      mBlobs(aBlobs)
  {
  }

  
  nsDOMMultipartFile(nsTArray<nsCOMPtr<nsIDOMBlob> >& aBlobs,
                     const nsAString& aContentType)
    : nsDOMFile(aContentType, UINT64_MAX),
      mBlobs(aBlobs)
  {
  }

  
  nsDOMMultipartFile(const nsAString& aName)
    : nsDOMFile(aName, EmptyString(), UINT64_MAX)
  {
  }

  
  nsDOMMultipartFile()
    : nsDOMFile(EmptyString(), UINT64_MAX)
  {
  }

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD Initialize(nsISupports* aOwner,
                        JSContext* aCx,
                        JSObject* aObj,
                        uint32_t aArgc,
                        JS::Value* aArgv);

  typedef nsIDOMBlob* (*UnwrapFuncPtr)(JSContext*, JSObject*);
  nsresult InitBlob(JSContext* aCx,
                    uint32_t aArgc,
                    JS::Value* aArgv,
                    UnwrapFuncPtr aUnwrapFunc);
  nsresult InitFile(JSContext* aCx,
                    uint32_t aArgc,
                    JS::Value* aArgv);

  already_AddRefed<nsIDOMBlob>
  CreateSlice(uint64_t aStart, uint64_t aLength, const nsAString& aContentType);

  NS_IMETHOD GetSize(uint64_t*);
  NS_IMETHOD GetInternalStream(nsIInputStream**);

  static nsresult
  NewFile(const nsAString& aName, nsISupports* *aNewObject);

  
  static nsresult
  NewBlob(nsISupports* *aNewObject);

  
  
  inline static nsresult
  NewFile(nsISupports* *aNewObject)
  {
    
    
    return NewFile(EmptyString(), aNewObject);
  }

  virtual const nsTArray<nsCOMPtr<nsIDOMBlob> >*
  GetSubBlobs() const { return &mBlobs; }

protected:
  nsTArray<nsCOMPtr<nsIDOMBlob> > mBlobs;
};

class BlobSet {
public:
  BlobSet()
    : mData(nullptr), mDataLen(0), mDataBufferLen(0)
  {}

  nsresult AppendVoidPtr(const void* aData, uint32_t aLength);
  nsresult AppendString(JSString* aString, bool nativeEOL, JSContext* aCx);
  nsresult AppendBlob(nsIDOMBlob* aBlob);
  nsresult AppendArrayBuffer(JSObject* aBuffer);
  nsresult AppendBlobs(const nsTArray<nsCOMPtr<nsIDOMBlob> >& aBlob);

  nsTArray<nsCOMPtr<nsIDOMBlob> >& GetBlobs() { Flush(); return mBlobs; }

  already_AddRefed<nsIDOMBlob>
  GetBlobInternal(const nsACString& aContentType)
  {
    nsCOMPtr<nsIDOMBlob> blob =
      new nsDOMMultipartFile(GetBlobs(), NS_ConvertASCIItoUTF16(aContentType));
    return blob.forget();
  }

protected:
  bool ExpandBufferSize(uint64_t aSize)
  {
    using mozilla::CheckedUint32;

    if (mDataBufferLen >= mDataLen + aSize) {
      mDataLen += aSize;
      return true;
    }

    
    CheckedUint32 bufferLen =
      std::max<uint32_t>(static_cast<uint32_t>(mDataBufferLen), 1);
    while (bufferLen.isValid() && bufferLen.value() < mDataLen + aSize)
      bufferLen *= 2;

    if (!bufferLen.isValid())
      return false;

    void* data = moz_realloc(mData, bufferLen.value());
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
      mData = nullptr; 
      mDataLen = 0;
      mDataBufferLen = 0;
    }
  }

  nsTArray<nsCOMPtr<nsIDOMBlob> > mBlobs;
  void* mData;
  uint64_t mDataLen;
  uint64_t mDataBufferLen;
};

#endif
