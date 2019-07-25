




































#ifndef mozilla_places_Database_h_
#define mozilla_places_Database_h_

#include "nsThreadUtils.h"
#include "nsWeakReference.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIObserver.h"
#include "mozilla/storage.h"
#include "mozilla/storage/StatementCache.h"



#define DATABASE_SCHEMA_VERSION 12


#define TOPIC_PLACES_INIT_COMPLETE "places-init-complete"

#define TOPIC_DATABASE_LOCKED "places-database-locked"



#define TOPIC_PROFILE_CHANGE_TEARDOWN "profile-change-teardown"




#define TOPIC_PROFILE_BEFORE_CHANGE "profile-before-change"




#define TOPIC_PLACES_SHUTDOWN "places-shutdown"



#define TOPIC_PLACES_WILL_CLOSE_CONNECTION "places-will-close-connection"

#define TOPIC_PLACES_CONNECTION_CLOSED "places-connection-closed"

namespace mozilla {
namespace places {

enum JournalMode {
  
  JOURNAL_DELETE = 0
  
  
, JOURNAL_TRUNCATE
  
, JOURNAL_MEMORY
  
, JOURNAL_WAL
};

class Database : public nsIObserver
               , public nsSupportsWeakReference
{
  typedef mozilla::storage::StatementCache<mozIStorageStatement> StatementCache;
  typedef mozilla::storage::StatementCache<mozIStorageAsyncStatement> AsyncStatementCache;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  Database();

  



  nsresult Init();

  



  void Shutdown();

  




  static already_AddRefed<Database> GetDatabase()
  {
    return GetSingleton();
  }

  




  PRUint16 GetDatabaseStatus() const
  {
    return mDatabaseStatus;
  }

  




  mozIStorageConnection* MainConn() const
  {
    return mMainConn;
  }

  






  void DispatchToAsyncThread(nsIRunnable* aEvent) const
  {
    if (mShuttingDown) {
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

  








  already_AddRefed<mozIStorageStatement>
  GetStatement(const nsACString& aQuery) const
  {
    if (mShuttingDown) {
      return nsnull;
    }
    if (NS_IsMainThread()) {
      return mMainThreadStatements.GetCachedStatement(aQuery);
    }
    return mAsyncThreadStatements.GetCachedStatement(aQuery);
  }

  








  template<int N>
  already_AddRefed<mozIStorageAsyncStatement>
  GetAsyncStatement(const char (&aQuery)[N]) const
  {
    nsDependentCString query(aQuery, N - 1);
    return GetAsyncStatement(query);
  }

  








  already_AddRefed<mozIStorageAsyncStatement>
  GetAsyncStatement(const nsACString& aQuery) const
  {
    if (mShuttingDown) {
      return nsnull;
    }
    MOZ_ASSERT(NS_IsMainThread());
    return mMainThreadAsyncStatements.GetCachedStatement(aQuery);
  }

protected:
  









  nsresult InitDatabaseFile(nsCOMPtr<mozIStorageService>& aStorage,
                            bool* aNewDatabaseCreated);

  






  nsresult BackupAndReplaceDatabaseFile(nsCOMPtr<mozIStorageService>& aStorage);

  






  nsresult InitSchema(bool* aDatabaseMigrated);

  


  nsresult InitFunctions();

  

  
  nsresult InitTempTriggers();

  


  nsresult MigrateV7Up();
  nsresult MigrateV8Up();
  nsresult MigrateV9Up();
  nsresult MigrateV10Up();
  nsresult MigrateV11Up();
  nsresult CheckAndUpdateGUIDs();

private:
  ~Database();

  




  static Database* GetSingleton();

  static Database* gDatabase;

  nsCOMPtr<mozIStorageConnection> mMainConn;

  mutable StatementCache mMainThreadStatements;
  mutable AsyncStatementCache mMainThreadAsyncStatements;
  mutable StatementCache mAsyncThreadStatements;

  PRInt32 mDBPageSize;
  enum JournalMode mCurrentJournalMode;
  PRUint16 mDatabaseStatus;
  bool mShuttingDown;
};

} 
} 

#endif 
