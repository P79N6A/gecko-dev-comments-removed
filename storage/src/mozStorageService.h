









































#ifndef _MOZSTORAGESERVICE_H_
#define _MOZSTORAGESERVICE_H_

#include "nsCOMPtr.h"
#include "nsICollation.h"
#include "nsIFile.h"
#include "nsIObserver.h"
#include "mozilla/Mutex.h"

#include "mozIStorageService.h"

class nsIXPConnect;

namespace mozilla {
namespace storage {

class Service : public mozIStorageService
              , public nsIObserver
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

  



  static already_AddRefed<nsIXPConnect> getXPConnect();

private:
  Service();
  virtual ~Service();

  




  Mutex mMutex;

  


  void shutdown();

  






  nsICollation *getLocaleCollation();

  






  nsCOMPtr<nsICollation> mLocaleCollation;

  nsCOMPtr<nsIFile> mProfileStorageFile;

  static Service *gService;

  static nsIXPConnect *sXPConnect;
};

} 
} 

#endif 
