






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
  virtual ~nsNetworkManagerListener();

  nsresult Init();

  virtual void RegisterWithConnection(DBusConnection* connection);
  virtual void UnregisterWithConnection(DBusConnection* connection);
  virtual bool HandleMessage(DBusMessage* msg);

  




  void UpdateNetworkStatus(DBusMessage* message);

private:
  void NotifyNetworkStatusObservers();

  nsCOMPtr<nsDBusService> mDBUS;
  bool                    mLinkUp;
  bool                    mNetworkManagerActive;
  bool                    mOK;
  bool                    mManageIOService;
};

#endif 
