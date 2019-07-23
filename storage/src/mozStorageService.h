








































#ifndef _MOZSTORAGESERVICE_H_
#define _MOZSTORAGESERVICE_H_

#include "nsCOMPtr.h"
#include "nsIFile.h"
#include "nsIObserver.h"
#include "prlock.h"

#include "mozIStorageService.h"

class nsIXPConnect;

namespace mozilla {
namespace storage {

class Service : public mozIStorageService
              , public nsIObserver
{
public:
  


  nsresult initialize();

  static Service *getSingleton();

  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGESERVICE
  NS_DECL_NSIOBSERVER

  



  static already_AddRefed<nsIXPConnect> getXPConnect();

private:
  virtual ~Service();

  



  PRLock *mLock;

  


  void shutdown();

  nsCOMPtr<nsIFile> mProfileStorageFile;

  static Service *gService;

  static nsIXPConnect *sXPConnect;
};

} 
} 

#endif 
