





#ifndef NSNETWORKMANAGERLISTENER_H_
#define NSNETWORKMANAGERLISTENER_H_

#include "nsINetworkLinkService.h"
#include "nsDBusService.h"

class nsNetworkManagerListener : public nsINetworkLinkService,
                                 public DBusClient {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSINETWORKLINKSERVICE

  nsNetworkManagerListener();

  nsresult Init();

  virtual void RegisterWithConnection(DBusConnection* connection) override;
  virtual void UnregisterWithConnection(DBusConnection* connection) override;
  virtual bool HandleMessage(DBusMessage* msg) override;

  




  void UpdateNetworkStatus(DBusMessage* message);

protected:
  virtual ~nsNetworkManagerListener();

private:
  void NotifyNetworkStatusObservers();

  nsCOMPtr<nsDBusService> mDBUS;
  bool                    mLinkUp;
  bool                    mNetworkManagerActive;
  bool                    mOK;
};

#endif 
