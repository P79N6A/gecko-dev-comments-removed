









































#ifndef MOZSTORAGESERVICE_H
#define MOZSTORAGESERVICE_H

#include "nsCOMPtr.h"
#include "nsICollation.h"
#include "nsIFile.h"
#include "nsIObserver.h"
#include "mozilla/Mutex.h"

#include "mozIStorageService.h"
#include "mozIStorageServiceQuotaManagement.h"

class nsIXPConnect;
struct sqlite3_vfs;

namespace mozilla {
namespace storage {

class Service : public mozIStorageService
              , public nsIObserver
              , public mozIStorageServiceQuotaManagement
{
public:
  


  nsresult initialize();

  












  int localeCompareStrings(const nsAString &aStr1,
                           const nsAString &aStr2,
                           PRInt32 aComparisonStrength);

  static Service *getSingleton();

  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGESERVICE
  NS_DECL_NSIOBSERVER
  NS_DECL_MOZISTORAGESERVICEQUOTAMANAGEMENT

  



  static already_AddRefed<nsIXPConnect> getXPConnect();

  


  static PRInt32 getSynchronousPref();

private:
  Service();
  virtual ~Service();

  




  Mutex mMutex;
  
  sqlite3_vfs *mSqliteVFS;

  


  void shutdown();

  






  nsICollation *getLocaleCollation();

  






  nsCOMPtr<nsICollation> mLocaleCollation;

  nsCOMPtr<nsIFile> mProfileStorageFile;

  static Service *gService;

  static nsIXPConnect *sXPConnect;

  static PRInt32 sSynchronousPref;
};

} 
} 

#endif 
