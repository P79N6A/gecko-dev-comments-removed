





#ifndef mozilla_dom_archivereader_domarchivefile_h__
#define mozilla_dom_archivereader_domarchivefile_h__

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/File.h"

#include "ArchiveReader.h"

#include "ArchiveReaderCommon.h"
#include "zipstruct.h"

BEGIN_ARCHIVEREADER_NAMESPACE




class ArchiveZipBlobImpl : public BlobImplBase
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  ArchiveZipBlobImpl(const nsAString& aName,
                     const nsAString& aContentType,
                     uint64_t aLength,
                     ZipCentral& aCentral,
                     BlobImpl* aBlobImpl)
  : BlobImplBase(aName, aContentType, aLength),
    mCentral(aCentral),
    mBlobImpl(aBlobImpl),
    mFilename(aName)
  {
    MOZ_ASSERT(mBlobImpl);
    MOZ_COUNT_CTOR(ArchiveZipBlobImpl);
  }

  ArchiveZipBlobImpl(const nsAString& aName,
                     const nsAString& aContentType,
                     uint64_t aStart,
                     uint64_t aLength,
                     ZipCentral& aCentral,
                     BlobImpl* aBlobImpl)
  : BlobImplBase(aContentType, aStart, aLength),
    mCentral(aCentral),
    mBlobImpl(aBlobImpl),
    mFilename(aName)
  {
    MOZ_ASSERT(mBlobImpl);
    MOZ_COUNT_CTOR(ArchiveZipBlobImpl);
  }

  
  virtual void GetInternalStream(nsIInputStream** aInputStream,
                                 ErrorResult& aRv) override;

protected:
  virtual ~ArchiveZipBlobImpl()
  {
    MOZ_COUNT_DTOR(ArchiveZipBlobImpl);
  }

  virtual already_AddRefed<BlobImpl>
  CreateSlice(uint64_t aStart, uint64_t aLength, const nsAString& aContentType,
              mozilla::ErrorResult& aRv) override;

private: 
  ZipCentral mCentral;
  nsRefPtr<BlobImpl> mBlobImpl;

  nsString mFilename;
};

END_ARCHIVEREADER_NAMESPACE

#endif 
