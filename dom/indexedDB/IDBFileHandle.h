





#ifndef mozilla_dom_indexeddb_idbfilehandle_h__
#define mozilla_dom_indexeddb_idbfilehandle_h__

#include "IndexedDatabase.h"

#include "mozilla/dom/file/FileHandle.h"
#include "mozilla/dom/indexedDB/FileInfo.h"

class nsIIDBDatabase;

BEGIN_INDEXEDDB_NAMESPACE

class IDBFileHandle : public mozilla::dom::file::FileHandle
{
public:
  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  NS_IMETHOD_(int64_t)
  GetFileId()
  {
    return mFileInfo->Id();
  }

  NS_IMETHOD_(FileInfo*)
  GetFileInfo()
  {
    return mFileInfo;
  }

  static already_AddRefed<IDBFileHandle>
  Create(IDBDatabase* aDatabase, const nsAString& aName,
         const nsAString& aType, already_AddRefed<FileInfo> aFileInfo);

  virtual already_AddRefed<nsISupports>
  CreateStream(nsIFile* aFile, bool aReadOnly);

  virtual already_AddRefed<nsIDOMFile>
  CreateFileObject(mozilla::dom::file::LockedFile* aLockedFile,
                   uint32_t aFileSize);

  nsIIDBDatabase* Database();

private:
  IDBFileHandle()
  { }

  ~IDBFileHandle()
  { }

  nsRefPtr<FileInfo> mFileInfo;
};

END_INDEXEDDB_NAMESPACE

#endif 
