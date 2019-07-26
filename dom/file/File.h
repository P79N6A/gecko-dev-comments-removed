





#ifndef mozilla_dom_file_file_h__
#define mozilla_dom_file_file_h__

#include "mozilla/Attributes.h"
#include "FileCommon.h"

#include "nsDOMFile.h"

#include "LockedFile.h"

BEGIN_FILE_NAMESPACE

class File : public nsDOMFileCC
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(File, nsDOMFileCC)

  
  File(const nsAString& aName, const nsAString& aContentType,
       uint64_t aLength, nsIFile* aFile, LockedFile* aLockedFile)
  : nsDOMFileCC(aName, aContentType, aLength),
    mFile(aFile), mLockedFile(aLockedFile),
    mWholeFile(true), mStoredFile(false)
  {
    NS_ASSERTION(mFile, "Null file!");
    NS_ASSERTION(mLockedFile, "Null locked file!");
  }

  
  File(const nsAString& aName, const nsAString& aContentType,
       uint64_t aLength, nsIFile* aFile, LockedFile* aLockedFile,
       FileInfo* aFileInfo)
  : nsDOMFileCC(aName, aContentType, aLength),
    mFile(aFile), mLockedFile(aLockedFile),
    mWholeFile(true), mStoredFile(true)
  {
    NS_ASSERTION(mFile, "Null file!");
    NS_ASSERTION(mLockedFile, "Null locked file!");
    mFileInfos.AppendElement(aFileInfo);
  }

  
  NS_IMETHOD
  GetMozFullPathInternal(nsAString& aFullPath) MOZ_OVERRIDE;

  NS_IMETHOD
  GetInternalStream(nsIInputStream** aStream) MOZ_OVERRIDE;

protected:
  
  File(const File* aOther, uint64_t aStart, uint64_t aLength,
       const nsAString& aContentType);

  virtual ~File()
  { }

  virtual already_AddRefed<nsIDOMBlob>
  CreateSlice(uint64_t aStart, uint64_t aLength,
              const nsAString& aContentType) MOZ_OVERRIDE;

  virtual bool
  IsStoredFile() const MOZ_OVERRIDE
  {
    return mStoredFile;
  }

  virtual bool
  IsWholeFile() const MOZ_OVERRIDE
  {
    return mWholeFile;
  }

  virtual bool
  IsSnapshot() const MOZ_OVERRIDE
  {
    return true;
  }

private:
  nsCOMPtr<nsIFile> mFile;
  nsRefPtr<LockedFile> mLockedFile;

  bool mWholeFile;
  bool mStoredFile;
};

END_FILE_NAMESPACE

#endif 
