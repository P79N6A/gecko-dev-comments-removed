





#ifndef mozilla_dom_indexeddb_idbfilehandle_h__
#define mozilla_dom_indexeddb_idbfilehandle_h__

#include "IndexedDatabase.h"

#include "MainThreadUtils.h"
#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/dom/FileHandle.h"
#include "mozilla/dom/indexedDB/FileInfo.h"
#include "nsCycleCollectionParticipant.h"

BEGIN_INDEXEDDB_NAMESPACE

class IDBDatabase;

class IDBFileHandle : public FileHandle
{
  typedef mozilla::dom::LockedFile LockedFile;

public:
  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(IDBFileHandle, FileHandle)

  static already_AddRefed<IDBFileHandle>
  Create(const nsAString& aName, const nsAString& aType,
         IDBDatabase* aDatabase, already_AddRefed<FileInfo> aFileInfo);


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

  virtual bool
  IsShuttingDown() MOZ_OVERRIDE;

  virtual bool
  IsInvalid() MOZ_OVERRIDE;

  virtual nsIOfflineStorage*
  Storage() MOZ_OVERRIDE;

  virtual already_AddRefed<nsISupports>
  CreateStream(nsIFile* aFile, bool aReadOnly) MOZ_OVERRIDE;

  virtual void
  SetThreadLocals() MOZ_OVERRIDE;

  virtual void
  UnsetThreadLocals() MOZ_OVERRIDE;

  virtual already_AddRefed<nsIDOMFile>
  CreateFileObject(LockedFile* aLockedFile, uint32_t aFileSize) MOZ_OVERRIDE;

  
  virtual JSObject*
  WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  
  IDBDatabase*
  Database()
  {
    MOZ_ASSERT(NS_IsMainThread(), "Wrong thread!");

    return mDatabase;
  }

private:
  IDBFileHandle(IDBDatabase* aOwner);

  ~IDBFileHandle()
  {
  }

  nsRefPtr<IDBDatabase> mDatabase;
  nsRefPtr<FileInfo> mFileInfo;
};

END_INDEXEDDB_NAMESPACE

#endif 
