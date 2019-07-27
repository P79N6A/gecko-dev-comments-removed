





#ifndef mozilla_dom_indexeddb_indexeddatabasemanager_h__
#define mozilla_dom_indexeddb_indexeddatabasemanager_h__

#include "nsIObserver.h"

#include "js/TypeDecls.h"
#include "mozilla/Atomics.h"
#include "mozilla/dom/quota/PersistenceType.h"
#include "mozilla/Mutex.h"
#include "nsClassHashtable.h"
#include "nsHashKeys.h"

struct PRLogModuleInfo;

namespace mozilla {

class EventChainPostVisitor;

namespace dom {

class TabContext;

namespace indexedDB {

class FileManager;
class FileManagerInfo;
class IDBFactory;

class IndexedDatabaseManager final : public nsIObserver
{
  typedef mozilla::dom::quota::PersistenceType PersistenceType;

public:
  enum LoggingMode
  {
    Logging_Disabled = 0,
    Logging_Concise,
    Logging_Detailed,
    Logging_ConciseProfilerMarks,
    Logging_DetailedProfilerMarks
  };

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

  static bool
  FullSynchronous();

  static LoggingMode
  GetLoggingMode()
#ifdef DEBUG
  ;
#else
  {
    return sLoggingMode;
  }
#endif

  static PRLogModuleInfo*
  GetLoggingModule()
#ifdef DEBUG
  ;
#else
  {
    return sLoggingModule;
  }
#endif

  static bool
  ExperimentalFeaturesEnabled();

  static bool
  ExperimentalFeaturesEnabled(JSContext* , JSObject* )
  {
    return ExperimentalFeaturesEnabled();
  }

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
                         const nsACString& aOrigin);

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
  CommonPostHandleEvent(EventChainPostVisitor& aVisitor, IDBFactory* aFactory);

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

  static void
  LoggingModePrefChangedCallback(const char* aPrefName, void* aClosure);

  
  
  nsClassHashtable<nsCStringHashKey, FileManagerInfo> mFileManagerInfos;

  
  
  
  mozilla::Mutex mFileMutex;

  static bool sIsMainProcess;
  static bool sFullSynchronousMode;
  static PRLogModuleInfo* sLoggingModule;
  static Atomic<LoggingMode> sLoggingMode;
  static mozilla::Atomic<bool> sLowDiskSpaceMode;
};

} 
} 
} 

#endif
