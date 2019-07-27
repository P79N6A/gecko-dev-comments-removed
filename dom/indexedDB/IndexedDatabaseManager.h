





#ifndef mozilla_dom_indexeddb_indexeddatabasemanager_h__
#define mozilla_dom_indexeddb_indexeddatabasemanager_h__

#include "nsIObserver.h"

#include "js/TypeDecls.h"
#include "mozilla/Atomics.h"
#include "mozilla/dom/quota/PersistenceType.h"
#include "mozilla/Mutex.h"
#include "nsClassHashtable.h"
#include "nsHashKeys.h"

class nsPIDOMWindow;

namespace mozilla {

class EventChainPostVisitor;

namespace dom {

class TabContext;

namespace quota {

class OriginOrPatternString;

} 

namespace indexedDB {

class FileManager;
class FileManagerInfo;

class IndexedDatabaseManager MOZ_FINAL : public nsIObserver
{
  typedef mozilla::dom::quota::OriginOrPatternString OriginOrPatternString;
  typedef mozilla::dom::quota::PersistenceType PersistenceType;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  
  static IndexedDatabaseManager*
  GetOrCreate();

  
  static IndexedDatabaseManager*
  Get();

  static bool
  IsClosed();

  static bool
  IsMainProcess()
#ifdef DEBUG
  ;
#else
  {
    return sIsMainProcess;
  }
#endif

  static bool
  InLowDiskSpaceMode()
#ifdef DEBUG
  ;
#else
  {
    return !!sLowDiskSpaceMode;
  }
#endif

  static bool
  InTestingMode();

  already_AddRefed<FileManager>
  GetFileManager(PersistenceType aPersistenceType,
                 const nsACString& aOrigin,
                 const nsAString& aDatabaseName);

  void
  AddFileManager(FileManager* aFileManager);

  void
  InvalidateAllFileManagers();

  void
  InvalidateFileManagers(PersistenceType aPersistenceType,
                         const OriginOrPatternString& aOriginOrPattern);

  void
  InvalidateFileManager(PersistenceType aPersistenceType,
                        const nsACString& aOrigin,
                        const nsAString& aDatabaseName);

  nsresult
  AsyncDeleteFile(FileManager* aFileManager,
                  int64_t aFileId);

  
  
  
  nsresult
  BlockAndGetFileReferences(PersistenceType aPersistenceType,
                            const nsACString& aOrigin,
                            const nsAString& aDatabaseName,
                            int64_t aFileId,
                            int32_t* aRefCnt,
                            int32_t* aDBRefCnt,
                            int32_t* aSliceRefCnt,
                            bool* aResult);

  static mozilla::Mutex&
  FileMutex()
  {
    IndexedDatabaseManager* mgr = Get();
    NS_ASSERTION(mgr, "Must have a manager here!");

    return mgr->mFileMutex;
  }

  static nsresult
  FireWindowOnError(nsPIDOMWindow* aOwner,
                    EventChainPostVisitor& aVisitor);

  static bool
  TabContextMayAccessOrigin(const mozilla::dom::TabContext& aContext,
                            const nsACString& aOrigin);

  static bool
  DefineIndexedDB(JSContext* aCx, JS::Handle<JSObject*> aGlobal);

private:
  IndexedDatabaseManager();
  ~IndexedDatabaseManager();

  nsresult
  Init();

  void
  Destroy();

  static PLDHashOperator
  InvalidateAndRemoveFileManagers(const nsACString& aKey,
                                  nsAutoPtr<FileManagerInfo>& aValue,
                                  void* aUserArg);

  
  
  nsClassHashtable<nsCStringHashKey, FileManagerInfo> mFileManagerInfos;

  
  
  
  mozilla::Mutex mFileMutex;

  static bool sIsMainProcess;
  static mozilla::Atomic<bool> sLowDiskSpaceMode;
};

} 
} 
} 

#endif 
