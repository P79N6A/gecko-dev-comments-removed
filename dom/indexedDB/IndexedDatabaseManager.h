





#ifndef mozilla_dom_indexeddb_indexeddatabasemanager_h__
#define mozilla_dom_indexeddb_indexeddatabasemanager_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"

#include "nsIIndexedDatabaseManager.h"

#include "mozilla/Mutex.h"
#include "nsClassHashtable.h"
#include "nsHashKeys.h"

#define INDEXEDDB_MANAGER_CONTRACTID "@mozilla.org/dom/indexeddb/manager;1"

class nsIAtom;
class nsPIDOMWindow;
class nsEventChainPostVisitor;

namespace mozilla {
namespace dom {
class TabContext;
}
}

BEGIN_INDEXEDDB_NAMESPACE

class FileManager;

class IndexedDatabaseManager MOZ_FINAL : public nsIIndexedDatabaseManager
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIINDEXEDDATABASEMANAGER

  
  static IndexedDatabaseManager*
  GetOrCreate();

  
  static IndexedDatabaseManager*
  Get();

  
  static IndexedDatabaseManager*
  FactoryCreate();

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

  already_AddRefed<FileManager>
  GetFileManager(const nsACString& aOrigin,
                 const nsAString& aDatabaseName);

  void
  AddFileManager(FileManager* aFileManager);

  void
  InvalidateAllFileManagers();

  void
  InvalidateFileManagersForPattern(const nsACString& aPattern);

  void
  InvalidateFileManager(const nsACString& aOrigin,
                        const nsAString& aDatabaseName);

  nsresult
  AsyncDeleteFile(FileManager* aFileManager,
                  int64_t aFileId);

  static mozilla::Mutex&
  FileMutex()
  {
    IndexedDatabaseManager* mgr = Get();
    NS_ASSERTION(mgr, "Must have a manager here!");

    return mgr->mFileMutex;
  }

  static nsresult
  FireWindowOnError(nsPIDOMWindow* aOwner,
                    nsEventChainPostVisitor& aVisitor);

  static bool
  TabContextMayAccessOrigin(const mozilla::dom::TabContext& aContext,
                            const nsACString& aOrigin);

private:
  IndexedDatabaseManager();
  ~IndexedDatabaseManager();

  nsresult
  Init();

  void
  Destroy();

  
  
  nsClassHashtable<nsCStringHashKey,
                   nsTArray<nsRefPtr<FileManager> > > mFileManagers;

  
  
  
  mozilla::Mutex mFileMutex;

  static bool sIsMainProcess;
};

END_INDEXEDDB_NAMESPACE

#endif 
