





#ifndef nsOfflineObserver_h__
#define nsOfflineObserver_h__

#include "nsIObserver.h"

namespace mozilla {
namespace net {










class DisconnectableParent
{
public:
  
  
  virtual nsresult OfflineNotification(nsISupports *aSubject);

  
  virtual uint32_t GetAppId() = 0;

  
  
  virtual void     OfflineDisconnect() { }
};





class OfflineObserver
  : public nsIObserver
{
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIOBSERVER
public:
  

  OfflineObserver(DisconnectableParent * parent);
  
  
  
  void RemoveObserver();
private:

  
  
  
  void RegisterOfflineObserver();
  void RemoveOfflineObserver();
  void RegisterOfflineObserverMainThread();
  void RemoveOfflineObserverMainThread();
private:
  virtual ~OfflineObserver() { }
  DisconnectableParent * mParent;
};

} 
} 

#endif 
