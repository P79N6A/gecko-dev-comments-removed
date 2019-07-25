







































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

#include "sqlite3.h"

struct PRLock;
class nsIFile;
class nsIEventTarget;
class nsIThread;
class nsIMemoryReporter;

namespace mozilla {
namespace storage {

class Connection : public mozIStorageConnection
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
    PRInt32 numArgs;
  };

  






  Connection(Service *aService, int aFlags);

  










  nsresult initialize(nsIFile *aDatabaseFile,
                      const char* aVFSName = NULL);

  
  sqlite3 *GetNativeConnection() { return mDBConn; }
  operator sqlite3 *() const { return mDBConn; }

  






  nsIEventTarget *getAsyncExecutionTarget();

  





  Mutex sharedAsyncExecutionMutex;

  





  SQLiteMutex sharedDBMutex;

  



  const nsCOMPtr<nsIThread> threadOpenedOn;

  


  nsresult internalClose();

  


  nsCString getFilename();

private:
  ~Connection();

  





  nsresult setClosedState();

  






  enum DatabaseElementType {
    INDEX,
    TABLE
  };

  








  nsresult databaseElementExists(enum DatabaseElementType aElementType,
                                 const nsACString& aElementName,
                                 PRBool *_exists);

  bool findFunctionByInstance(nsISupports *aInstance);

  static int sProgressHelper(void *aArg);
  
  
  
  int progressHandler();

  sqlite3 *mDBConn;
  nsCOMPtr<nsIFile> mDatabaseFile;

  nsTArray<nsCOMPtr<nsIMemoryReporter> > mMemoryReporters;

  




  nsCOMPtr<nsIThread> mAsyncExecutionThread;
  





  bool mAsyncExecutionThreadShuttingDown;

  



  PRBool mTransactionInProgress;

  



  nsDataHashtable<nsCStringHashKey, FunctionInfo> mFunctions;

  



  nsCOMPtr<mozIStorageProgressHandler> mProgressHandler;

  


  const int mFlags;

  
  
  
  nsRefPtr<Service> mStorageService;
};

} 
} 

#endif 
