








































#ifndef __nsQtRemoteService_h__
#define __nsQtRemoteService_h__

#include "nsIRemoteService.h"

#include "nsIObserver.h"

#include "nsString.h"
#include "nsInterfaceHashtable.h"

class nsIDOMWindow;
class nsIWeakReference;
class nsIWidget;

class nsQtRemoteService : public nsIRemoteService,
                          public nsIObserver
{
public:
  
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREMOTESERVICE
  NS_DECL_NSIOBSERVER

  nsQtRemoteService() { };

private:
  ~nsQtRemoteService() { };
};

#endif 
