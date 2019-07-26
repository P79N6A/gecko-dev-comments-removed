




#ifndef nsDOMBlobBuilder_h
#define nsDOMBlobBuilder_h

#include "nsDOMFile.h"

#include "mozilla/CheckedInt.h"
#include "mozilla/Attributes.h"
#include <algorithm>

#define NS_DOMMULTIPARTBLOB_CID { 0x47bf0b43, 0xf37e, 0x49ef, \
  { 0x81, 0xa0, 0x18, 0xba, 0xc0, 0x57, 0xb5, 0xcc } }
#define NS_DOMMULTIPARTBLOB_CONTRACTID "@mozilla.org/dom/multipart-blob;1"

#define NS_DOMMULTIPARTFILE_CID { 0xc3361f77, 0x60d1, 0x4ea9, \
  { 0x94, 0x96, 0xdf, 0x5d, 0x6f, 0xcd, 0xd7, 0x8f } }
#define NS_DOMMULTIPARTFILE_CONTRACTID "@mozilla.org/dom/multipart-file;1"

class nsDOMMultipartFile : public nsDOMFile,
                           public nsIJSNativeInitializer
{
public:
  
  nsDOMMultipartFile(nsTArray<nsCOMPtr<nsIDOMBlob> > aBlobs,
                     const nsAString& aName,
                     const nsAString& aContentType)
    : nsDOMFile(aName, aContentType, UINT64_MAX),
      mBlobs(aBlobs),
      mIsFromNsiFile(false)
  {
    SetLengthAndModifiedDate();
  }

  
  nsDOMMultipartFile(nsTArray<nsCOMPtr<nsIDOMBlob> >& aBlobs,
                     const nsAString& aContentType)
    : nsDOMFile(aContentType, UINT64_MAX),
      mBlobs(aBlobs),
      mIsFromNsiFile(false)
  {
    SetLengthAndModifiedDate();
  }

  
  nsDOMMultipartFile(const nsAString& aName)
    : nsDOMFile(aName, EmptyString(), UINT64_MAX),
      mIsFromNsiFile(false)
  {
  }

  
  nsDOMMultipartFile()
    : nsDOMFile(EmptyString(), UINT64_MAX),
      mIsFromNsiFile(false)
  {
  }

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD Initialize(nsISupports* aOwner,
                        JSContext* aCx,
                        JSObject* aObj,
                        const JS::CallArgs& aArgs) MOZ_OVERRIDE;

  typedef nsIDOMBlob* (*UnwrapFuncPtr)(JSContext*, JSObject*);
  nsresult InitBlob(JSContext* aCx,
                    uint32_t aArgc,
                    JS::Value* aArgv,
                    UnwrapFuncPtr aUnwrapFunc);
  nsresult InitFile(JSContext* aCx,
                    uint32_t aArgc,
                    JS::Value* aArgv);
  nsresult InitChromeFile(JSContext* aCx,
                          uint32_t aArgc,
                          JS::Value* aArgv);

  already_AddRefed<nsIDOMBlob>
  CreateSlice(uint64_t aStart, uint64_t aLength, const nsAString& aContentType) MOZ_OVERRIDE;

  NS_IMETHOD GetSize(uint64_t*) MOZ_OVERRIDE;
  NS_IMETHOD GetInternalStream(nsIInputStream**) MOZ_OVERRIDE;

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
  GetSubBlobs() const MOZ_OVERRIDE { return &mBlobs; }

  NS_IMETHOD GetMozFullPathInternal(nsAString& aFullPath) MOZ_OVERRIDE;

protected:
  nsresult ParseBlobArrayArgument(JSContext* aCx, JS::Value& aValue,
                                  bool aNativeEOL, UnwrapFuncPtr aUnwrapFunc);

  void SetLengthAndModifiedDate();

  nsTArray<nsCOMPtr<nsIDOMBlob> > mBlobs;
  bool mIsFromNsiFile;
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
        new nsDOMMemoryFile(mData, mDataLen, EmptyString());
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
