




#ifndef nsDOMBlobBuilder_h
#define nsDOMBlobBuilder_h

#include "mozilla/Attributes.h"
#include "mozilla/CheckedInt.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/File.h"
#include "mozilla/dom/BlobBinding.h"
#include "mozilla/dom/FileBinding.h"
#include <algorithm>

using namespace mozilla;
using namespace mozilla::dom;

class MultipartFileImpl MOZ_FINAL : public FileImplBase
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  
  MultipartFileImpl(const nsTArray<nsRefPtr<FileImpl>>& aBlobImpls,
                    const nsAString& aName,
                    const nsAString& aContentType)
    : FileImplBase(aName, aContentType, UINT64_MAX),
      mBlobImpls(aBlobImpls),
      mIsFromNsIFile(false)
  {
    SetLengthAndModifiedDate();
  }

  
  MultipartFileImpl(const nsTArray<nsRefPtr<FileImpl>>& aBlobImpls,
                    const nsAString& aContentType)
    : FileImplBase(aContentType, UINT64_MAX),
      mBlobImpls(aBlobImpls),
      mIsFromNsIFile(false)
  {
    SetLengthAndModifiedDate();
  }

  
  explicit MultipartFileImpl(const nsAString& aName)
    : FileImplBase(aName, EmptyString(), UINT64_MAX),
      mIsFromNsIFile(false)
  {
  }

  
  MultipartFileImpl()
    : FileImplBase(EmptyString(), UINT64_MAX),
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

  void InitializeChromeFile(File& aData,
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

  virtual already_AddRefed<FileImpl>
  CreateSlice(uint64_t aStart, uint64_t aLength,
              const nsAString& aContentType,
              ErrorResult& aRv) MOZ_OVERRIDE;

  virtual uint64_t GetSize(ErrorResult& aRv) MOZ_OVERRIDE
  {
    return mLength;
  }

  virtual nsresult GetInternalStream(nsIInputStream** aInputStream) MOZ_OVERRIDE;

  virtual const nsTArray<nsRefPtr<FileImpl>>* GetSubBlobImpls() const MOZ_OVERRIDE
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
  virtual ~MultipartFileImpl() {}

  void SetLengthAndModifiedDate();

  nsTArray<nsRefPtr<FileImpl>> mBlobImpls;
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
  nsresult AppendBlobImpl(FileImpl* aBlobImpl);
  nsresult AppendBlobImpls(const nsTArray<nsRefPtr<FileImpl>>& aBlobImpls);

  nsTArray<nsRefPtr<FileImpl>>& GetBlobImpls() { Flush(); return mBlobImpls; }

  already_AddRefed<File>
  GetBlobInternal(nsISupports* aParent, const nsACString& aContentType)
  {
    nsRefPtr<File> blob = new File(aParent,
      new MultipartFileImpl(GetBlobImpls(), NS_ConvertASCIItoUTF16(aContentType)));
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

#endif
