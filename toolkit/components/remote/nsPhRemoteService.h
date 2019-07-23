





































#ifndef __nsPhRemoteService_h__
#define __nsPhRemoteService_h__

#include "nsIRemoteService.h"
#include "nsIObserver.h"
#include "nsString.h"

class nsIWeakReference;

class nsPhRemoteService : public nsIRemoteService,
                          public nsIObserver
{
public:
  
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREMOTESERVICE
  NS_DECL_NSIOBSERVER

  nsPhRemoteService() { mIsInitialized = PR_FALSE; }

private:
  ~nsPhRemoteService() { }

  void HandleCommandsFor(nsIWidget *aWidget,
                         nsIWeakReference* aWindow);
  PRBool mIsInitialized;
  nsCString mAppName;

};

#endif 
