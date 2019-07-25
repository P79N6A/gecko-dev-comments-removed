




#ifndef nsExceptionService_h__
#define nsExceptionService_h__

#include "mozilla/Attributes.h"
#include "mozilla/Mutex.h"

#include "nsIException.h"
#include "nsIExceptionService.h"
#include "nsIObserverService.h"
#include "nsHashtable.h"
#include "nsIObserver.h"

class nsExceptionManager;


class nsExceptionService MOZ_FINAL : public nsIExceptionService, public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIEXCEPTIONSERVICE
  NS_DECL_NSIEXCEPTIONMANAGER
  NS_DECL_NSIOBSERVER

  nsExceptionService();

  
  nsresult DoGetExceptionFromProvider(nsresult errCode,
                                      nsIException *defaultException,
                                      nsIException **_richError);
  void Shutdown();


  
  static void AddThread(nsExceptionManager *);
  static void DropThread(nsExceptionManager *);
  static void DoDropThread(nsExceptionManager *thread);

  static void DropAllThreads();
  static nsExceptionManager *firstThread;

  nsSupportsHashtable mProviders;

  

  static mozilla::Mutex* sLock;

  static unsigned tlsIndex;
  static void ThreadDestruct( void *data );
#ifdef DEBUG
  static int32_t totalInstances;
#endif

private:
  ~nsExceptionService();
};


#endif 
