





#ifndef mozilla_dom_file_domarchivefile_h__
#define mozilla_dom_file_domarchivefile_h__

#include "nsDOMFile.h"

#include "ArchiveReader.h"

#include "FileCommon.h"
#include "zipstruct.h"

BEGIN_FILE_NAMESPACE




class ArchiveZipFile : public nsDOMFileCC
{
public:
  ArchiveZipFile(const nsAString& aName,
                 const nsAString& aContentType,
                 uint64_t aLength,
                 ZipCentral& aCentral,
                 ArchiveReader* aReader)
  : nsDOMFileCC(aName, aContentType, aLength),
    mCentral(aCentral),
    mArchiveReader(aReader),
    mFilename(aName)
  {
    NS_ASSERTION(mArchiveReader, "must have a reader");
    MOZ_COUNT_CTOR(ArchiveZipFile);
  }

  ArchiveZipFile(const nsAString& aName,
                 const nsAString& aContentType,
                 uint64_t aStart,
                 uint64_t aLength,
                 ZipCentral& aCentral,
                 ArchiveReader* aReader)
  : nsDOMFileCC(aContentType, aStart, aLength),
    mCentral(aCentral),
    mArchiveReader(aReader),
    mFilename(aName)
  {
    NS_ASSERTION(mArchiveReader, "must have a reader");
    MOZ_COUNT_CTOR(ArchiveZipFile);
  }

  virtual ~ArchiveZipFile()
  {
    MOZ_COUNT_DTOR(ArchiveZipFile);
  }

  
  NS_IMETHOD GetInternalStream(nsIInputStream**);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(ArchiveZipFile, nsDOMFileCC)

protected:
  virtual already_AddRefed<nsIDOMBlob> CreateSlice(uint64_t aStart,
                                                   uint64_t aLength,
                                                   const nsAString& aContentType);

private: 
  ZipCentral mCentral;
  nsRefPtr<ArchiveReader> mArchiveReader;

  nsString mFilename;
};

END_FILE_NAMESPACE

#endif 
