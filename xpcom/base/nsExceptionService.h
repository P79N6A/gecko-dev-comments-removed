





































#ifndef nsExceptionService_h__
#define nsExceptionService_h__

#include "nsVoidArray.h"
#include "nsIException.h"
#include "nsIExceptionService.h"
#include "nsIObserverService.h"
#include "nsHashtable.h"
#include "nsIObserver.h"

class nsExceptionManager;


class nsExceptionService : public nsIExceptionService, public nsIObserver
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

  

  static PRLock* lock;

  static PRUintn tlsIndex;
  static void PR_CALLBACK ThreadDestruct( void *data );
#ifdef NS_DEBUG
  static PRInt32 totalInstances;
#endif

private:
  ~nsExceptionService();
};


#endif 
