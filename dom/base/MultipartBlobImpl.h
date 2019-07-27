





#ifndef mozilla_dom_MultipartBlobImpl_h
#define mozilla_dom_MultipartBlobImpl_h

#include "mozilla/Attributes.h"
#include "mozilla/CheckedInt.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/File.h"
#include "mozilla/dom/BlobBinding.h"
#include "mozilla/dom/FileBinding.h"
#include <algorithm>

using namespace mozilla;
using namespace mozilla::dom;

class MultipartBlobImpl final : public BlobImplBase
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  
  MultipartBlobImpl(const nsTArray<nsRefPtr<BlobImpl>>& aBlobImpls,
                    const nsAString& aName,
                    const nsAString& aContentType)
    : BlobImplBase(aName, aContentType, UINT64_MAX),
      mBlobImpls(aBlobImpls),
      mIsFromNsIFile(false)
  {
    SetLengthAndModifiedDate();
  }

  
  MultipartBlobImpl(const nsTArray<nsRefPtr<BlobImpl>>& aBlobImpls,
                    const nsAString& aContentType)
    : BlobImplBase(aContentType, UINT64_MAX),
      mBlobImpls(aBlobImpls),
      mIsFromNsIFile(false)
  {
    SetLengthAndModifiedDate();
  }

  
  explicit MultipartBlobImpl(const nsAString& aName)
    : BlobImplBase(aName, EmptyString(), UINT64_MAX),
      mIsFromNsIFile(false)
  {
  }

  
  MultipartBlobImpl()
    : BlobImplBase(EmptyString(), UINT64_MAX),
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

  void InitializeChromeFile(Blob& aData,
                            const ChromeFilePropertyBag& aBag,
                            ErrorResult& aRv);

  void InitializeChromeFile(nsPIDOMWindow* aWindow,
                            const nsAString& aData,
                            const ChromeFilePropertyBag& aBag,
                            ErrorResult& aRv);

  void InitializeChromeFile(nsPIDOMWindow* aWindow,
                            nsIFile* aData,
                            const ChromeFilePropertyBag& aBag,
                            bool aIsFromNsIFile,
                            ErrorResult& aRv);

  virtual already_AddRefed<BlobImpl>
  CreateSlice(uint64_t aStart, uint64_t aLength,
              const nsAString& aContentType,
              ErrorResult& aRv) override;

  virtual uint64_t GetSize(ErrorResult& aRv) override
  {
    return mLength;
  }

  virtual void GetInternalStream(nsIInputStream** aInputStream,
                                 ErrorResult& aRv) override;

  virtual const nsTArray<nsRefPtr<BlobImpl>>* GetSubBlobImpls() const override
  {
    return &mBlobImpls;
  }

  virtual void GetMozFullPathInternal(nsAString& aFullPath,
                                      ErrorResult& aRv) const override;

  virtual nsresult
  SetMutable(bool aMutable) override;

  void SetName(const nsAString& aName)
  {
    mName = aName;
  }

  void SetFromNsIFile(bool aValue)
  {
    mIsFromNsIFile = aValue;
  }

  virtual bool MayBeClonedToOtherThreads() const override;

protected:
  virtual ~MultipartBlobImpl() {}

  void SetLengthAndModifiedDate();

  nsTArray<nsRefPtr<BlobImpl>> mBlobImpls;
  bool mIsFromNsIFile;
};

#endif 
