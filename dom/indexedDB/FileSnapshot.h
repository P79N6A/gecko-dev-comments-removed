





#ifndef mozilla_dom_indexeddb_filesnapshot_h__
#define mozilla_dom_indexeddb_filesnapshot_h__

#include "mozilla/Attributes.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsDOMFile.h"

namespace mozilla {
namespace dom {
namespace indexedDB {

class IDBFileHandle;

class FileImplSnapshot : public DOMFileImplBase
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  
  FileImplSnapshot(const nsAString& aName, const nsAString& aContentType,
                   uint64_t aLength, nsIFile* aFile, IDBFileHandle* aFileHandle,
                   FileInfo* aFileInfo);

  
  virtual nsresult
  GetMozFullPathInternal(nsAString& aFullPath) MOZ_OVERRIDE;

  virtual nsresult
  GetInternalStream(nsIInputStream** aStream) MOZ_OVERRIDE;

  virtual void
  Unlink() MOZ_OVERRIDE;

  virtual void
  Traverse(nsCycleCollectionTraversalCallback &aCb) MOZ_OVERRIDE;

  virtual bool
  IsCCed() const MOZ_OVERRIDE
  {
    return true;
  }

protected:
  
  FileImplSnapshot(const FileImplSnapshot* aOther, uint64_t aStart,
                   uint64_t aLength, const nsAString& aContentType);

  virtual ~FileImplSnapshot();

  virtual already_AddRefed<nsIDOMBlob>
  CreateSlice(uint64_t aStart, uint64_t aLength,
              const nsAString& aContentType) MOZ_OVERRIDE;

  virtual bool
  IsStoredFile() const MOZ_OVERRIDE
  {
    return true;
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
  nsRefPtr<IDBFileHandle> mFileHandle;

  bool mWholeFile;
};

} 
} 
} 

#endif 
