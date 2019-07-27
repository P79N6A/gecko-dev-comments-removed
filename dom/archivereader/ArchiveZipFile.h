





#ifndef mozilla_dom_archivereader_domarchivefile_h__
#define mozilla_dom_archivereader_domarchivefile_h__

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/File.h"

#include "ArchiveReader.h"

#include "ArchiveReaderCommon.h"
#include "zipstruct.h"

BEGIN_ARCHIVEREADER_NAMESPACE




class ArchiveZipFileImpl : public FileImplBase
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  ArchiveZipFileImpl(const nsAString& aName,
                     const nsAString& aContentType,
                     uint64_t aLength,
                     ZipCentral& aCentral,
                     ArchiveReader* aReader)
  : FileImplBase(aName, aContentType, aLength),
    mCentral(aCentral),
    mArchiveReader(aReader),
    mFilename(aName)
  {
    NS_ASSERTION(mArchiveReader, "must have a reader");
    MOZ_COUNT_CTOR(ArchiveZipFileImpl);
  }

  ArchiveZipFileImpl(const nsAString& aName,
                     const nsAString& aContentType,
                     uint64_t aStart,
                     uint64_t aLength,
                     ZipCentral& aCentral,
                     ArchiveReader* aReader)
  : FileImplBase(aContentType, aStart, aLength),
    mCentral(aCentral),
    mArchiveReader(aReader),
    mFilename(aName)
  {
    NS_ASSERTION(mArchiveReader, "must have a reader");
    MOZ_COUNT_CTOR(ArchiveZipFileImpl);
  }

  
  virtual nsresult GetInternalStream(nsIInputStream**) MOZ_OVERRIDE;

  virtual void Unlink() MOZ_OVERRIDE;
  virtual void Traverse(nsCycleCollectionTraversalCallback &aCb) MOZ_OVERRIDE;

  virtual bool IsCCed() const MOZ_OVERRIDE
  {
    return true;
  }

protected:
  virtual ~ArchiveZipFileImpl()
  {
    MOZ_COUNT_DTOR(ArchiveZipFileImpl);
  }

  virtual already_AddRefed<FileImpl>
  CreateSlice(uint64_t aStart, uint64_t aLength, const nsAString& aContentType,
              mozilla::ErrorResult& aRv) MOZ_OVERRIDE;

private: 
  ZipCentral mCentral;
  nsRefPtr<ArchiveReader> mArchiveReader;

  nsString mFilename;
};

END_ARCHIVEREADER_NAMESPACE

#endif 
