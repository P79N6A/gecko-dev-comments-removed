





#ifndef mozilla_storage_Connection_h
#define mozilla_storage_Connection_h

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "mozilla/Mutex.h"
#include "nsProxyRelease.h"
#include "nsThreadUtils.h"
#include "nsIInterfaceRequestor.h"

#include "nsDataHashtable.h"
#include "mozIStorageProgressHandler.h"
#include "SQLiteMutex.h"
#include "mozIStorageConnection.h"
#include "mozStorageService.h"
#include "mozIStorageAsyncConnection.h"
#include "mozIStorageCompletionCallback.h"

#include "nsIMutableArray.h"
#include "mozilla/Attributes.h"

#include "sqlite3.h"

struct PRLock;
class nsIFile;
class nsIFileURL;
class nsIEventTarget;
class nsIThread;

namespace mozilla {
namespace storage {

class Connection final : public mozIStorageConnection
                       , public nsIInterfaceRequestor
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_MOZISTORAGEASYNCCONNECTION
  NS_DECL_MOZISTORAGECONNECTION
  NS_DECL_NSIINTERFACEREQUESTOR

  


  struct FunctionInfo {
    enum FunctionType {
      SIMPLE,
      AGGREGATE
    };

    nsCOMPtr<nsISupports> function;
    FunctionType type;
    int32_t numArgs;
  };

  











  Connection(Service *aService, int aFlags, bool aAsyncOnly);

  


  nsresult initialize();

  






  nsresult initialize(nsIFile *aDatabaseFile);

  






  nsresult initialize(nsIFileURL *aFileURL);

  








  int32_t getSqliteRuntimeStatus(int32_t aStatusOption,
                                 int32_t* aMaxValue=nullptr);
  







  void setCommitHook(int (*aCallbackFn)(void *) , void *aData=nullptr) {
    MOZ_ASSERT(mDBConn, "A connection must exist at this point");
    ::sqlite3_commit_hook(mDBConn, aCallbackFn, aData);
  };

  


  bool getAutocommit() {
    return mDBConn && static_cast<bool>(::sqlite3_get_autocommit(mDBConn));
  };

  






  nsIEventTarget *getAsyncExecutionTarget();

  









  Mutex sharedAsyncExecutionMutex;

  





  SQLiteMutex sharedDBMutex;

  



  const nsCOMPtr<nsIThread> threadOpenedOn;

  


  nsresult internalClose(sqlite3 *aDBConn);

  


  nsCString getFilename();

  










  int prepareStatement(sqlite3* aNativeConnection,
                       const nsCString &aSQL, sqlite3_stmt **_stmt);

  









  int stepStatement(sqlite3* aNativeConnection, sqlite3_stmt* aStatement);

  




  nsresult beginTransactionInternal(sqlite3 *aNativeConnection,
                                    int32_t aTransactionType=TRANSACTION_DEFERRED);
  nsresult commitTransactionInternal(sqlite3 *aNativeConnection);
  nsresult rollbackTransactionInternal(sqlite3 *aNativeConnection);

  bool connectionReady();

  


  bool isClosing();

  




  bool isClosed();

  nsresult initializeClone(Connection *aClone, bool aReadOnly);

private:
  ~Connection();
  nsresult initializeInternal(nsIFile *aDatabaseFile);

  





  nsresult setClosedState();

  








  int executeSql(sqlite3 *aNativeConnection, const char *aSqlString);

  






  enum DatabaseElementType {
    INDEX,
    TABLE
  };

  








  nsresult databaseElementExists(enum DatabaseElementType aElementType,
                                 const nsACString& aElementName,
                                 bool *_exists);

  bool findFunctionByInstance(nsISupports *aInstance);

  static int sProgressHelper(void *aArg);
  
  
  
  int progressHandler();

  sqlite3 *mDBConn;
  nsCOMPtr<nsIFileURL> mFileURL;
  nsCOMPtr<nsIFile> mDatabaseFile;

  




  nsCOMPtr<nsIThread> mAsyncExecutionThread;

  












  bool mAsyncExecutionThreadShuttingDown;

  






  bool mConnectionClosed;

  



  bool mTransactionInProgress;

  



  nsDataHashtable<nsCStringHashKey, FunctionInfo> mFunctions;

  



  nsCOMPtr<mozIStorageProgressHandler> mProgressHandler;

  


  const int mFlags;

  
  
  
  nsRefPtr<Service> mStorageService;

  



  const bool mAsyncOnly;
};






class CallbackComplete final : public nsRunnable
{
public:
  





  CallbackComplete(nsresult aStatus,
                   nsISupports* aValue,
                   already_AddRefed<mozIStorageCompletionCallback> aCallback)
    : mStatus(aStatus)
    , mValue(aValue)
    , mCallback(aCallback)
  {
  }

  NS_IMETHOD Run() {
    MOZ_ASSERT(NS_IsMainThread());
    nsresult rv = mCallback->Complete(mStatus, mValue);

    
    mValue = nullptr;
    mCallback = nullptr;
    return rv;
  }

private:
  nsresult mStatus;
  nsCOMPtr<nsISupports> mValue;
  
  
  
  nsRefPtr<mozIStorageCompletionCallback> mCallback;
};

} 
} 

#endif 
