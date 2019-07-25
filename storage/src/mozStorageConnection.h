







































#ifndef MOZSTORAGECONNECTION_H
#define MOZSTORAGECONNECTION_H

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "mozilla/Mutex.h"

#include "nsString.h"
#include "nsInterfaceHashtable.h"
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

namespace mozilla {
namespace storage {

class Connection : public mozIStorageConnection
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGECONNECTION

  Connection(Service *aService);

  







  nsresult initialize(nsIFile *aDatabaseFile);

  
  sqlite3 *GetNativeConnection() { return mDBConn; }

  






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

  




  nsCOMPtr<nsIThread> mAsyncExecutionThread;
  





  bool mAsyncExecutionThreadShuttingDown;

  



  PRBool mTransactionInProgress;

  



  nsInterfaceHashtable<nsCStringHashKey, nsISupports> mFunctions;

  



  nsCOMPtr<mozIStorageProgressHandler> mProgressHandler;

  
  
  
  nsRefPtr<Service> mStorageService;
};

} 
} 

#endif 
