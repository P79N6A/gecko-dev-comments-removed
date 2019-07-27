





#ifndef mozilla_dom_indexeddb_fileinfo_h__
#define mozilla_dom_indexeddb_fileinfo_h__

#include "nsAutoPtr.h"
#include "nsISupportsImpl.h"

namespace mozilla {
namespace dom {
namespace indexedDB {

class FileManager;

class FileInfo
{
  friend class FileManager;

  ThreadSafeAutoRefCnt mRefCnt;
  ThreadSafeAutoRefCnt mDBRefCnt;
  ThreadSafeAutoRefCnt mSliceRefCnt;

  nsRefPtr<FileManager> mFileManager;

public:
  static
  FileInfo* Create(FileManager* aFileManager, int64_t aId);

  explicit FileInfo(FileManager* aFileManager);

  void
  AddRef()
  {
    UpdateReferences(mRefCnt, 1);
  }

  void
  Release()
  {
    UpdateReferences(mRefCnt, -1);
  }

  void
  UpdateDBRefs(int32_t aDelta)
  {
    UpdateReferences(mDBRefCnt, aDelta);
  }

  void
  ClearDBRefs()
  {
    UpdateReferences(mDBRefCnt, 0, true);
  }

  void
  UpdateSliceRefs(int32_t aDelta)
  {
    UpdateReferences(mSliceRefCnt, aDelta);
  }

  void
  GetReferences(int32_t* aRefCnt, int32_t* aDBRefCnt, int32_t* aSliceRefCnt);

  FileManager*
  Manager() const
  {
    return mFileManager;
  }

  virtual int64_t
  Id() const = 0;

protected:
  virtual ~FileInfo();

private:
  void
  UpdateReferences(ThreadSafeAutoRefCnt& aRefCount,
                   int32_t aDelta,
                   bool aClear = false);

  void
  Cleanup();
};

} 
} 
} 

#endif 
