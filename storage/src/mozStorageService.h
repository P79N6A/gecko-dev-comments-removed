





#ifndef MOZSTORAGESERVICE_H
#define MOZSTORAGESERVICE_H

#include "nsCOMPtr.h"
#include "nsICollation.h"
#include "nsIFile.h"
#include "nsIObserver.h"
#include "nsTArray.h"
#include "mozilla/Mutex.h"

#include "mozIStorageService.h"

class nsIMemoryReporter;
class nsIXPConnect;
struct sqlite3_vfs;

namespace mozilla {
namespace storage {

class Connection;
class Service : public mozIStorageService
              , public nsIObserver
{
public:
  


  nsresult initialize();

  












  int localeCompareStrings(const nsAString &aStr1,
                           const nsAString &aStr2,
                           int32_t aComparisonStrength);

  static Service *getSingleton();

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_MOZISTORAGESERVICE
  NS_DECL_NSIOBSERVER

  



  static already_AddRefed<nsIXPConnect> getXPConnect();

  


  static int32_t getSynchronousPref();

  




  static int32_t getDefaultPageSize()
  {
    return sDefaultPageSize;
  }

  



  static bool pageSizeIsValid(int32_t aPageSize)
  {
    return aPageSize == 512 || aPageSize == 1024 || aPageSize == 2048 ||
           aPageSize == 4096 || aPageSize == 8192 || aPageSize == 16384 ||
           aPageSize == 32768 || aPageSize == 65536;
  }

  








  void registerConnection(Connection *aConnection);

  







  void unregisterConnection(Connection *aConnection);

  











  void getConnections(nsTArray<nsRefPtr<Connection> >& aConnections);

private:
  Service();
  virtual ~Service();

  




  Mutex mMutex;
  
  sqlite3_vfs *mSqliteVFS;

  


  Mutex mRegistrationMutex;

  



  nsTArray<nsRefPtr<Connection> > mConnections;

  


  void shutdown();

  






  nsICollation *getLocaleCollation();

  






  nsCOMPtr<nsICollation> mLocaleCollation;

  nsCOMPtr<nsIFile> mProfileStorageFile;

  nsCOMPtr<nsIMemoryReporter> mStorageSQLiteUniReporter;
  nsCOMPtr<nsIMemoryReporter> mStorageSQLiteMultiReporter;

  static Service *gService;

  static nsIXPConnect *sXPConnect;

  static int32_t sSynchronousPref;
  static int32_t sDefaultPageSize;
};

} 
} 

#endif 
