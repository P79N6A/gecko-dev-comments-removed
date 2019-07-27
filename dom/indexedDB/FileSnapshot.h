





#ifndef mozilla_dom_indexeddb_filesnapshot_h__
#define mozilla_dom_indexeddb_filesnapshot_h__

#include "mozilla/Attributes.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsDOMFile.h"

namespace mozilla {
namespace dom {

class MetadataParameters;

namespace indexedDB {

class IDBFileHandle;

class FileImplSnapshot MOZ_FINAL
  : public DOMFileImplBase
{
  typedef mozilla::dom::MetadataParameters MetadataParameters;

  nsCOMPtr<nsIFile> mFile;
  nsRefPtr<IDBFileHandle> mFileHandle;

  bool mWholeFile;

public:
  
  FileImplSnapshot(const nsAString& aName,
                   const nsAString& aContentType,
                   MetadataParameters* aMetadataParams,
                   nsIFile* aFile,
                   IDBFileHandle* aFileHandle,
                   FileInfo* aFileInfo);

  NS_DECL_ISUPPORTS_INHERITED

private:
  
  FileImplSnapshot(const FileImplSnapshot* aOther,
                   uint64_t aStart,
                   uint64_t aLength,
                   const nsAString& aContentType);

  ~FileImplSnapshot();

  static void
  AssertSanity()
#ifdef DEBUG
  ;
#else
  { }
#endif

  virtual nsresult
  GetMozFullPathInternal(nsAString& aFullPath) MOZ_OVERRIDE;

  virtual nsresult
  GetInternalStream(nsIInputStream** aStream) MOZ_OVERRIDE;

  virtual void
  Unlink() MOZ_OVERRIDE;

  virtual void
  Traverse(nsCycleCollectionTraversalCallback &aCb) MOZ_OVERRIDE;

  virtual bool
  IsCCed() const MOZ_OVERRIDE;

  virtual already_AddRefed<DOMFileImpl>
  CreateSlice(uint64_t aStart,
              uint64_t aLength,
              const nsAString& aContentType) MOZ_OVERRIDE;

  virtual bool
  IsStoredFile() const MOZ_OVERRIDE;

  virtual bool
  IsWholeFile() const MOZ_OVERRIDE;

  virtual bool
  IsSnapshot() const MOZ_OVERRIDE;
};

} 
} 
} 

#endif 
