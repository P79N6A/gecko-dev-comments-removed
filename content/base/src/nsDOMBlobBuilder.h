




#ifndef nsDOMBlobBuilder_h
#define nsDOMBlobBuilder_h

#include "nsDOMFile.h"

#include "mozilla/Attributes.h"
#include "mozilla/CheckedInt.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/BlobBinding.h"
#include "mozilla/dom/FileBinding.h"
#include <algorithm>

using namespace mozilla;
using namespace mozilla::dom;

class DOMMultipartFileImpl MOZ_FINAL : public DOMFileImplBase
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  
  DOMMultipartFileImpl(const nsTArray<nsRefPtr<DOMFileImpl>>& aBlobImpls,
                       const nsAString& aName,
                       const nsAString& aContentType)
    : DOMFileImplBase(aName, aContentType, UINT64_MAX),
      mBlobImpls(aBlobImpls),
      mIsFromNsIFile(false)
  {
    SetLengthAndModifiedDate();
  }

  
  DOMMultipartFileImpl(const nsTArray<nsRefPtr<DOMFileImpl>>& aBlobImpls,
                       const nsAString& aContentType)
    : DOMFileImplBase(aContentType, UINT64_MAX),
      mBlobImpls(aBlobImpls),
      mIsFromNsIFile(false)
  {
    SetLengthAndModifiedDate();
  }

  
  explicit DOMMultipartFileImpl(const nsAString& aName)
    : DOMFileImplBase(aName, EmptyString(), UINT64_MAX),
      mIsFromNsIFile(false)
  {
  }

  
  DOMMultipartFileImpl()
    : DOMFileImplBase(EmptyString(), UINT64_MAX),
      mIsFromNsIFile(false)
  {
  }

  void InitializeBlob();

  void InitializeBlob(
       JSContext* aCx,
       const Sequence<OwningArrayBufferOrArrayBufferViewOrBlobOrString>& aData,
       const nsAString& aContentType,
       bool aNativeEOL,
       ErrorResult& aRv);

  void InitializeChromeFile(DOMFile& aData,
                            const FilePropertyBag& aBag,
                            ErrorResult& aRv);

  void InitializeChromeFile(nsPIDOMWindow* aWindow,
                            const nsAString& aData,
                            const FilePropertyBag& aBag,
                            ErrorResult& aRv);

  void InitializeChromeFile(nsPIDOMWindow* aWindow,
                            nsIFile* aData,
                            const FilePropertyBag& aBag,
                            bool aIsFromNsIFile,
                            ErrorResult& aRv);

  virtual already_AddRefed<DOMFileImpl>
  CreateSlice(uint64_t aStart, uint64_t aLength,
              const nsAString& aContentType,
              ErrorResult& aRv) MOZ_OVERRIDE;

  virtual uint64_t GetSize(ErrorResult& aRv) MOZ_OVERRIDE
  {
    return mLength;
  }

  virtual nsresult GetInternalStream(nsIInputStream** aInputStream) MOZ_OVERRIDE;

  virtual const nsTArray<nsRefPtr<DOMFileImpl>>* GetSubBlobImpls() const MOZ_OVERRIDE
  {
    return &mBlobImpls;
  }

  virtual void GetMozFullPathInternal(nsAString& aFullPath,
                                      ErrorResult& aRv) MOZ_OVERRIDE;

  void SetName(const nsAString& aName)
  {
    mName = aName;
  }

  void SetFromNsIFile(bool aValue)
  {
    mIsFromNsIFile = aValue;
  }

protected:
  virtual ~DOMMultipartFileImpl() {}

  void SetLengthAndModifiedDate();

  nsTArray<nsRefPtr<DOMFileImpl>> mBlobImpls;
  bool mIsFromNsIFile;
};

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
  nsresult AppendBlobImpl(DOMFileImpl* aBlobImpl);
  nsresult AppendBlobImpls(const nsTArray<nsRefPtr<DOMFileImpl>>& aBlobImpls);

  nsTArray<nsRefPtr<DOMFileImpl>>& GetBlobImpls() { Flush(); return mBlobImpls; }

  already_AddRefed<DOMFile>
  GetBlobInternal(nsISupports* aParent, const nsACString& aContentType)
  {
    nsRefPtr<DOMFile> blob = new DOMFile(aParent,
      new DOMMultipartFileImpl(GetBlobImpls(), NS_ConvertASCIItoUTF16(aContentType)));
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
      
      

      nsRefPtr<DOMFileImpl> blobImpl =
        new DOMFileImplMemory(mData, mDataLen, EmptyString());
      mBlobImpls.AppendElement(blobImpl);
      mData = nullptr; 
      mDataLen = 0;
      mDataBufferLen = 0;
    }
  }

  nsTArray<nsRefPtr<DOMFileImpl>> mBlobImpls;
  void* mData;
  uint64_t mDataLen;
  uint64_t mDataBufferLen;
};

#endif
