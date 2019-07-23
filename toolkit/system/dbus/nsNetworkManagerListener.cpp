







































 
#include "nsNetworkManagerListener.h"

#include "nsNetCID.h"
#include "nsServiceManagerUtils.h"
#include "nsIObserverService.h"
#include "nsStringAPI.h"



#define NM_DBUS_SERVICE                 "org.freedesktop.NetworkManager"
#define NM_DBUS_PATH                            "/org/freedesktop/NetworkManager"
#define NM_DBUS_INTERFACE                       "org.freedesktop.NetworkManager"
#define NM_DBUS_SIGNAL_STATE_CHANGE             "StateChange"
typedef enum NMState
{
        NM_STATE_UNKNOWN = 0,
        NM_STATE_ASLEEP,
        NM_STATE_CONNECTING,
        NM_STATE_CONNECTED,
        NM_STATE_DISCONNECTED
} NMState;

nsNetworkManagerListener::nsNetworkManagerListener() :
    mLinkUp(PR_TRUE), mNetworkManagerActive(PR_FALSE),
    mOK(PR_TRUE), mManageIOService(PR_TRUE)
{
}

nsNetworkManagerListener::~nsNetworkManagerListener() {
  if (mDBUS) {
    mDBUS->RemoveClient(this);
  }
}

NS_IMPL_ISUPPORTS1(nsNetworkManagerListener, nsINetworkLinkService)

nsresult
nsNetworkManagerListener::GetIsLinkUp(PRBool* aIsUp) {
  *aIsUp = mLinkUp;
  return NS_OK;
}

nsresult
nsNetworkManagerListener::GetLinkStatusKnown(PRBool* aKnown) {
  *aKnown = mNetworkManagerActive;
  return NS_OK;
}

nsresult
nsNetworkManagerListener::Init() {
  mDBUS = nsDBusService::Get();
  if (!mDBUS)
    return NS_ERROR_OUT_OF_MEMORY;
  nsresult rv = mDBUS->AddClient(this);
  if (NS_FAILED(rv)) {
    mDBUS = nsnull;
    return rv;
  }
  if (!mOK)
    return NS_ERROR_FAILURE;
  return NS_OK;
}

static void
NetworkStatusNotify(DBusPendingCall *pending, void* user_data) {
  DBusMessage* msg = dbus_pending_call_steal_reply(pending);
  if (!msg)
    return;
  if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_METHOD_RETURN) {
    static_cast<nsNetworkManagerListener*>(user_data)->UpdateNetworkStatus(msg);
  }
  dbus_message_unref(msg);
}

void
nsNetworkManagerListener::RegisterWithConnection(DBusConnection* connection) {
  DBusError error;
  dbus_error_init(&error);
  
  dbus_bus_add_match(connection,
                     "type='signal',"
                     "interface='" NM_DBUS_INTERFACE "',"
                     "sender='" NM_DBUS_SERVICE "',"
                     "path='" NM_DBUS_PATH "'", &error);
  mOK = !dbus_error_is_set(&error);
  dbus_error_free(&error);
  if (!mOK)
    return;
  
  DBusMessage* msg =
    dbus_message_new_method_call(NM_DBUS_SERVICE, NM_DBUS_PATH,
                                 NM_DBUS_INTERFACE, "state");
  if (!msg) {
    mOK = PR_FALSE;
    return;
  }
  
  DBusPendingCall* reply = mDBUS->SendWithReply(this, msg);
  if (!reply) {
    mOK = PR_FALSE;
    return;
  }

  dbus_pending_call_set_notify(reply, NetworkStatusNotify, this, NULL);
  dbus_pending_call_unref(reply);
}

void
nsNetworkManagerListener::NotifyNetworkStatusObservers() {
  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1");
  if (!observerService)
    return;
    
  const PRUnichar* status;
  if (mNetworkManagerActive) {
    status = mLinkUp ? NS_LITERAL_STRING(NS_NETWORK_LINK_DATA_UP).get()
                     : NS_LITERAL_STRING(NS_NETWORK_LINK_DATA_DOWN).get();
  } else {
    status = NS_LITERAL_STRING(NS_NETWORK_LINK_DATA_UNKNOWN).get();
  }

  observerService->NotifyObservers(static_cast<nsISupports*>(this),
                                   NS_NETWORK_LINK_TOPIC, status);
}

void
nsNetworkManagerListener::UnregisterWithConnection(DBusConnection* connection) {
  mNetworkManagerActive = PR_FALSE;
  NotifyNetworkStatusObservers();
}

PRBool
nsNetworkManagerListener::HandleMessage(DBusMessage* message) {
  if (dbus_message_is_signal(message, NM_DBUS_INTERFACE,
                             NM_DBUS_SIGNAL_STATE_CHANGE)) {
    UpdateNetworkStatus(message);
    return PR_TRUE;
  }
  return PR_FALSE;
}

void
nsNetworkManagerListener::UpdateNetworkStatus(DBusMessage* msg) {
  PRInt32 result;
  if (!dbus_message_get_args(msg, NULL, DBUS_TYPE_UINT32, &result,
                             DBUS_TYPE_INVALID))
    return;
  
  mNetworkManagerActive = PR_TRUE;
  
  PRBool wasUp = mLinkUp;
  mLinkUp = result == NM_STATE_CONNECTED;
  if (wasUp == mLinkUp)
    return;

  NotifyNetworkStatusObservers();
}
