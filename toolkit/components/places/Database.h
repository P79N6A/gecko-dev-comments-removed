



#ifndef mozilla_places_Database_h_
#define mozilla_places_Database_h_

#include "MainThreadUtils.h"
#include "nsWeakReference.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIObserver.h"
#include "nsIAsyncShutdown.h"
#include "mozilla/storage.h"
#include "mozilla/storage/StatementCache.h"
#include "mozilla/Attributes.h"
#include "nsIEventTarget.h"



#define DATABASE_SCHEMA_VERSION 30


#define TOPIC_PLACES_INIT_COMPLETE "places-init-complete"

#define TOPIC_DATABASE_LOCKED "places-database-locked"



#define TOPIC_PROFILE_CHANGE_TEARDOWN "profile-change-teardown"




#define TOPIC_PLACES_SHUTDOWN "places-shutdown"



#define TOPIC_PLACES_WILL_CLOSE_CONNECTION "places-will-close-connection"

#define TOPIC_PLACES_CONNECTION_CLOSED "places-connection-closed"



#define TOPIC_SIMULATE_PLACES_MUST_CLOSE_1 "test-simulate-places-shutdown-phase-1"



#define TOPIC_SIMULATE_PLACES_MUST_CLOSE_2 "test-simulate-places-shutdown-phase-2"

class nsIRunnable;

namespace mozilla {
namespace places {

enum JournalMode {
  
  JOURNAL_DELETE = 0
  
  
, JOURNAL_TRUNCATE
  
, JOURNAL_MEMORY
  
, JOURNAL_WAL
};

class DatabaseShutdown;

class Database final : public nsIObserver
                     , public nsSupportsWeakReference
{
  typedef mozilla::storage::StatementCache<mozIStorageStatement> StatementCache;
  typedef mozilla::storage::StatementCache<mozIStorageAsyncStatement> AsyncStatementCache;

public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIOBSERVER

  Database();

  



  nsresult Init();

  


  already_AddRefed<nsIAsyncShutdownClient> GetConnectionShutdown();

  




  static already_AddRefed<Database> GetDatabase()
  {
    return GetSingleton();
  }

  




  uint16_t GetDatabaseStatus() const
  {
    return mDatabaseStatus;
  }

  




  mozIStorageConnection* MainConn() const
  {
    return mMainConn;
  }

  






  void DispatchToAsyncThread(nsIRunnable* aEvent) const
  {
    if (mClosed) {
      return;
    }
    nsCOMPtr<nsIEventTarget> target = do_GetInterface(mMainConn);
    if (target) {
      (void)target->Dispatch(aEvent, NS_DISPATCH_NORMAL);
    }
  }

  
  

  








  template<int N>
  already_AddRefed<mozIStorageStatement>
  GetStatement(const char (&aQuery)[N]) const
  {
    nsDependentCString query(aQuery, N - 1);
    return GetStatement(query);
  }

  








  already_AddRefed<mozIStorageStatement>  GetStatement(const nsACString& aQuery) const;

  








  template<int N>
  already_AddRefed<mozIStorageAsyncStatement>
  GetAsyncStatement(const char (&aQuery)[N]) const
  {
    nsDependentCString query(aQuery, N - 1);
    return GetAsyncStatement(query);
  }

  








  already_AddRefed<mozIStorageAsyncStatement> GetAsyncStatement(const nsACString& aQuery) const;

protected:
  



  void Shutdown();

  bool IsShutdownStarted() const;

  









  nsresult InitDatabaseFile(nsCOMPtr<mozIStorageService>& aStorage,
                            bool* aNewDatabaseCreated);

  






  nsresult BackupAndReplaceDatabaseFile(nsCOMPtr<mozIStorageService>& aStorage);

  






  nsresult InitSchema(bool* aDatabaseMigrated);

  


  nsresult CreateBookmarkRoots();

  


  nsresult InitFunctions();

  


  nsresult InitTempTriggers();

  


  nsresult MigrateV13Up();
  nsresult MigrateV15Up();
  nsresult MigrateV17Up();
  nsresult MigrateV18Up();
  nsresult MigrateV19Up();
  nsresult MigrateV20Up();
  nsresult MigrateV21Up();
  nsresult MigrateV22Up();
  nsresult MigrateV23Up();
  nsresult MigrateV24Up();
  nsresult MigrateV25Up();
  nsresult MigrateV26Up();
  nsresult MigrateV27Up();
  nsresult MigrateV28Up();
  nsresult MigrateV30Up();

  nsresult UpdateBookmarkRootTitles();

  friend class DatabaseShutdown;

private:
  ~Database();

  


  static already_AddRefed<Database> GetSingleton();

  static Database* gDatabase;

  nsCOMPtr<mozIStorageConnection> mMainConn;

  mutable StatementCache mMainThreadStatements;
  mutable AsyncStatementCache mMainThreadAsyncStatements;
  mutable StatementCache mAsyncThreadStatements;

  int32_t mDBPageSize;
  uint16_t mDatabaseStatus;
  bool mClosed;

  



  already_AddRefed<nsIAsyncShutdownClient> GetShutdownPhase();

  






  nsRefPtr<DatabaseShutdown> mConnectionShutdown;
};

} 
} 

#endif 
