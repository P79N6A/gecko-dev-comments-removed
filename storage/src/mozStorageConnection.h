





#ifndef mozilla_storage_Connection_h
#define mozilla_storage_Connection_h

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "mozilla/Mutex.h"
#include "nsIInterfaceRequestor.h"

#include "nsDataHashtable.h"
#include "mozIStorageProgressHandler.h"
#include "SQLiteMutex.h"
#include "mozIStorageConnection.h"
#include "mozStorageService.h"

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

class Connection MOZ_FINAL : public mozIStorageConnection
                           , public nsIInterfaceRequestor
{
public:
  NS_DECL_ISUPPORTS
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

  






  Connection(Service *aService, int aFlags);

  


  nsresult initialize();

  






  nsresult initialize(nsIFile *aDatabaseFile);

  






  nsresult initialize(nsIFileURL *aFileURL);

  
  sqlite3 *GetNativeConnection() { return mDBConn; }
  operator sqlite3 *() const { return mDBConn; }

  






  nsIEventTarget *getAsyncExecutionTarget();

  





  Mutex sharedAsyncExecutionMutex;

  





  SQLiteMutex sharedDBMutex;

  



  const nsCOMPtr<nsIThread> threadOpenedOn;

  


  nsresult internalClose();

  


  nsCString getFilename();

  








  int prepareStatement(const nsCString &aSQL, sqlite3_stmt **_stmt);

  







  int stepStatement(sqlite3_stmt* aStatement);

  bool ConnectionReady() {
    return mDBConn != nullptr;
  }

  



  bool isAsyncClosing();

private:
  ~Connection();

  nsresult initializeInternal(nsIFile *aDatabaseFile);

  





  nsresult setClosedState();

  






  int executeSql(const char *aSqlString);

  






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

  



  bool mTransactionInProgress;

  



  nsDataHashtable<nsCStringHashKey, FunctionInfo> mFunctions;

  



  nsCOMPtr<mozIStorageProgressHandler> mProgressHandler;

  


  const int mFlags;

  
  
  
  nsRefPtr<Service> mStorageService;
};

} 
} 

#endif 
