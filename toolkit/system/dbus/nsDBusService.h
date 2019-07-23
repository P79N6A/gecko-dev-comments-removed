








































#ifndef NSDBUSSERVICE_H_
#define NSDBUSSERVICE_H_

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>

#include "nsITimer.h"
#include "nsCOMPtr.h"

class DBusClient {
public:
  virtual void RegisterWithConnection(DBusConnection* connection) = 0;
  virtual void UnregisterWithConnection(DBusConnection* connection) = 0;
  virtual PRBool HandleMessage(DBusMessage* msg) = 0;
};

#define NS_DBUS_IID \
  { 0xba4f79b7, 0x0d4c, 0x4b3a, { 0x8a, 0x85, 0x6f, 0xb3, 0x0d, 0xce, 0xf5, 0x11 } }




















class nsDBusService : public nsISupports
{
public:
  nsDBusService();
  virtual ~nsDBusService();

  NS_DECL_ISUPPORTS
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_DBUS_IID)

  static already_AddRefed<nsDBusService> Get();
  
  nsresult AddClient(DBusClient* client);
  void RemoveClient(DBusClient* client);

  DBusPendingCall* SendWithReply(DBusClient* client, DBusMessage* message);

  
  PRBool HandleMessage(DBusMessage* message);
  void DoTimerCallback(nsITimer* aTimer);

private:
  nsresult CreateConnection();
  void DropConnection();
  void HandleDBusDisconnect();

  static nsDBusService* gSingleton;
  
  DBusConnection*    mConnection;
  nsCOMPtr<nsITimer> mReconnectTimer;
  DBusClient*        mSingleClient;
};

#endif 
