







































#ifndef _MOZSTORAGECONNECTION_H_
#define _MOZSTORAGECONNECTION_H_

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "mozilla/Mutex.h"

#include "nsString.h"
#include "nsInterfaceHashtable.h"
#include "mozIStorageProgressHandler.h"
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

  






  already_AddRefed<nsIEventTarget> getAsyncExecutionTarget();

  




  Mutex sharedAsyncExecutionMutex;

  


  nsresult internalClose();

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

  


  Mutex mAsyncExecutionMutex;

  




  nsCOMPtr<nsIThread> mAsyncExecutionThread;
  





  bool mAsyncExecutionThreadShuttingDown;

  PRLock *mTransactionMutex;
  PRBool mTransactionInProgress;

  PRLock *mFunctionsMutex;
  nsInterfaceHashtable<nsCStringHashKey, nsISupports> mFunctions;

  PRLock *mProgressHandlerMutex;
  nsCOMPtr<mozIStorageProgressHandler> mProgressHandler;

  



  nsCOMPtr<nsIThread> mOpenedThread;

  
  
  
  nsRefPtr<Service> mStorageService;
};

} 
} 

#endif 
