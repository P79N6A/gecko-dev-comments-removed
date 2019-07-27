




#ifndef mozilla_dom_BlobSet_h
#define mozilla_dom_BlobSet_h

#include "mozilla/CheckedInt.h"
#include "mozilla/dom/File.h"

namespace mozilla {
namespace dom {

class BlobSet {
public:
  BlobSet()
    : mData(nullptr), mDataLen(0), mDataBufferLen(0)
  {}

  ~BlobSet()
  {
    moz_free(mData);
  }

  nsresult AppendVoidPtr(const void* aData, uint32_t aLength);
  nsresult AppendString(const nsAString& aString, bool nativeEOL, JSContext* aCx);
  nsresult AppendBlobImpl(FileImpl* aBlobImpl);
  nsresult AppendBlobImpls(const nsTArray<nsRefPtr<FileImpl>>& aBlobImpls);

  nsTArray<nsRefPtr<FileImpl>>& GetBlobImpls() { Flush(); return mBlobImpls; }

  already_AddRefed<File> GetBlobInternal(nsISupports* aParent,
                                         const nsACString& aContentType);

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
      
      

      nsRefPtr<FileImpl> blobImpl =
        new FileImplMemory(mData, mDataLen, EmptyString());
      mBlobImpls.AppendElement(blobImpl);
      mData = nullptr; 
      mDataLen = 0;
      mDataBufferLen = 0;
    }
  }

  nsTArray<nsRefPtr<FileImpl>> mBlobImpls;
  void* mData;
  uint64_t mDataLen;
  uint64_t mDataBufferLen;
};

} 
} 

#endif 
