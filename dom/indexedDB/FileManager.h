





#ifndef mozilla_dom_indexeddb_filemanager_h__
#define mozilla_dom_indexeddb_filemanager_h__

#include "mozilla/Attributes.h"
#include "mozilla/dom/quota/PersistenceType.h"
#include "mozilla/dom/quota/StoragePrivilege.h"
#include "nsDataHashtable.h"
#include "nsHashKeys.h"
#include "nsISupportsImpl.h"

class nsIFile;
class mozIStorageConnection;

namespace mozilla {
namespace dom {
namespace indexedDB {

class FileInfo;


class FileManager MOZ_FINAL
{
  friend class FileInfo;

  typedef mozilla::dom::quota::PersistenceType PersistenceType;
  typedef mozilla::dom::quota::StoragePrivilege StoragePrivilege;

  PersistenceType mPersistenceType;
  nsCString mGroup;
  nsCString mOrigin;
  StoragePrivilege mPrivilege;
  nsString mDatabaseName;

  nsString mDirectoryPath;
  nsString mJournalDirectoryPath;

  int64_t mLastFileId;

  
  nsDataHashtable<nsUint64HashKey, FileInfo*> mFileInfos;

  bool mInvalidated;

public:
  static already_AddRefed<nsIFile>
  GetFileForId(nsIFile* aDirectory, int64_t aId);

  static nsresult
  InitDirectory(nsIFile* aDirectory,
                nsIFile* aDatabaseFile,
                PersistenceType aPersistenceType,
                const nsACString& aGroup,
                const nsACString& aOrigin);

  static nsresult
  GetUsage(nsIFile* aDirectory, uint64_t* aUsage);

  FileManager(PersistenceType aPersistenceType,
              const nsACString& aGroup,
              const nsACString& aOrigin,
              StoragePrivilege aPrivilege,
              const nsAString& aDatabaseName);

  PersistenceType
  Type() const
  {
    return mPersistenceType;
  }

  const nsACString&
  Group() const
  {
    return mGroup;
  }

  const nsACString&
  Origin() const
  {
    return mOrigin;
  }

  const StoragePrivilege&
  Privilege() const
  {
    return mPrivilege;
  }

  const nsAString&
  DatabaseName() const
  {
    return mDatabaseName;
  }

  bool
  Invalidated() const
  {
    return mInvalidated;
  }

  nsresult
  Init(nsIFile* aDirectory, mozIStorageConnection* aConnection);

  nsresult
  Invalidate();

  already_AddRefed<nsIFile>
  GetDirectory();

  already_AddRefed<nsIFile>
  GetJournalDirectory();

  already_AddRefed<nsIFile>
  EnsureJournalDirectory();

  already_AddRefed<FileInfo>
  GetFileInfo(int64_t aId);

  already_AddRefed<FileInfo>
  GetNewFileInfo();

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(FileManager)

private:
  ~FileManager();
};

} 
} 
} 

#endif 
