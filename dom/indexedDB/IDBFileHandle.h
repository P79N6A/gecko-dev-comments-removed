





#ifndef mozilla_dom_indexeddb_idbfilehandle_h__
#define mozilla_dom_indexeddb_idbfilehandle_h__

#include "IndexedDatabase.h"

#include "mozilla/dom/FileHandle.h"
#include "mozilla/dom/indexedDB/FileInfo.h"

BEGIN_INDEXEDDB_NAMESPACE

class IDBDatabase;

class IDBFileHandle : public FileHandle
{
  typedef mozilla::dom::LockedFile LockedFile;

public:
  static already_AddRefed<IDBFileHandle>
  Create(IDBDatabase* aDatabase, const nsAString& aName,
         const nsAString& aType, already_AddRefed<FileInfo> aFileInfo);


  virtual int64_t
  GetFileId() MOZ_OVERRIDE
  {
    return mFileInfo->Id();
  }

  virtual FileInfo*
  GetFileInfo() MOZ_OVERRIDE
  {
    return mFileInfo;
  }

  virtual already_AddRefed<nsISupports>
  CreateStream(nsIFile* aFile, bool aReadOnly) MOZ_OVERRIDE;

  virtual already_AddRefed<nsIDOMFile>
  CreateFileObject(LockedFile* aLockedFile, uint32_t aFileSize) MOZ_OVERRIDE;

  
  virtual JSObject*
  WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  
  IDBDatabase*
  Database();

private:
  IDBFileHandle(IDBDatabase* aOwner);

  ~IDBFileHandle()
  {
  }

  nsRefPtr<FileInfo> mFileInfo;
};

END_INDEXEDDB_NAMESPACE

#endif 
