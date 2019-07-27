





#ifndef mozilla_dom_File_h
#define mozilla_dom_File_h

#include "mozilla/Attributes.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsDOMFile.h"

namespace mozilla {
namespace dom {

class FileHandle;
class File;

class FileImpl : public DOMFileImplBase
{
  friend class File;

public:
  NS_DECL_ISUPPORTS_INHERITED

  
  FileImpl(const nsAString& aName, const nsAString& aContentType,
           uint64_t aLength, nsIFile* aFile, FileHandle* aFileHandle);

  
  FileImpl(const nsAString& aName, const nsAString& aContentType,
           uint64_t aLength, nsIFile* aFile, FileHandle* aFileHandle,
           indexedDB::FileInfo* aFileInfo);

  
  virtual nsresult GetMozFullPathInternal(nsAString& aFullPath) MOZ_OVERRIDE;

  virtual nsresult GetInternalStream(nsIInputStream** aStream) MOZ_OVERRIDE;

  virtual void Unlink() MOZ_OVERRIDE;
  virtual void Traverse(nsCycleCollectionTraversalCallback &aCb) MOZ_OVERRIDE;

  virtual bool IsCCed() const MOZ_OVERRIDE
  {
    return true;
  }

protected:
  
  FileImpl(const FileImpl* aOther, uint64_t aStart, uint64_t aLength,
           const nsAString& aContentType);

  virtual ~FileImpl();

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
  nsRefPtr<FileHandle> mFileHandle;

  bool mWholeFile;
  bool mStoredFile;
};

} 
} 

#endif 
