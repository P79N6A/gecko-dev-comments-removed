






#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "WakeLockListener.h"

#ifdef MOZ_ENABLE_DBUS

#define FREEDESKTOP_SCREENSAVER_TARGET    "org.freedesktop.ScreenSaver"
#define FREEDESKTOP_SCREENSAVER_OBJECT    "/ScreenSaver"
#define FREEDESKTOP_SCREENSAVER_INTERFACE "org.freedesktop.ScreenSaver"

#define SESSION_MANAGER_TARGET            "org.gnome.SessionManager"
#define SESSION_MANAGER_OBJECT            "/org/gnome/SessionManager"
#define SESSION_MANAGER_INTERFACE         "org.gnome.SessionManager"

#define DBUS_TIMEOUT                      (-1)

using namespace mozilla;

NS_IMPL_ISUPPORTS(WakeLockListener, nsIDOMMozWakeLockListener)

WakeLockListener* WakeLockListener::sSingleton = nullptr;


enum DesktopEnvironment {
  FreeDesktop,
  GNOME,
  Unsupported,
};

class WakeLockTopic
{
public:
  WakeLockTopic(const nsAString& aTopic, DBusConnection* aConnection)
    : mTopic(NS_ConvertUTF16toUTF8(aTopic))
    , mConnection(aConnection)
    , mDesktopEnvironment(FreeDesktop)
    , mInhibitRequest(0)
    , mShouldInhibit(false)
    , mWaitingForReply(false)
  {
  }

  nsresult InhibitScreensaver(void);
  nsresult UninhibitScreensaver(void);

private:
  bool SendInhibit();
  bool SendUninhibit();

  bool SendFreeDesktopInhibitMessage();
  bool SendGNOMEInhibitMessage();
  bool SendMessage(DBusMessage* aMessage);

  static void ReceiveInhibitReply(DBusPendingCall* aPending, void* aUserData);
  void InhibitFailed();
  void InhibitSucceeded(uint32_t aInhibitRequest);

  nsAutoCString mTopic;
  DBusConnection* mConnection;

  DesktopEnvironment mDesktopEnvironment;

  uint32_t mInhibitRequest;

  bool mShouldInhibit;
  bool mWaitingForReply;
};


bool
WakeLockTopic::SendMessage(DBusMessage* aMessage)
{
  
  DBusPendingCall* reply;
  dbus_connection_send_with_reply(mConnection, aMessage, &reply,
                                  DBUS_TIMEOUT);
  dbus_message_unref(aMessage);

  if (!reply) {
    return false;
  }

  dbus_pending_call_set_notify(reply, &ReceiveInhibitReply, this, NULL);
  dbus_pending_call_unref(reply);

  return true;
}

bool
WakeLockTopic::SendFreeDesktopInhibitMessage()
{
  DBusMessage* message =
    dbus_message_new_method_call(FREEDESKTOP_SCREENSAVER_TARGET,
                                 FREEDESKTOP_SCREENSAVER_OBJECT,
                                 FREEDESKTOP_SCREENSAVER_INTERFACE,
                                 "Inhibit");

  if (!message) {
    return false;
  }

  const char* app = g_get_prgname();
  const char* topic = mTopic.get();
  dbus_message_append_args(message,
                           DBUS_TYPE_STRING, &app,
                           DBUS_TYPE_STRING, &topic,
                           DBUS_TYPE_INVALID);

  return SendMessage(message);
}

bool
WakeLockTopic::SendGNOMEInhibitMessage()
{
  DBusMessage* message =
    dbus_message_new_method_call(SESSION_MANAGER_TARGET,
                                 SESSION_MANAGER_OBJECT,
                                 SESSION_MANAGER_INTERFACE,
                                 "Inhibit");

  if (!message) {
    return false;
  }

  static const uint32_t xid = 0;
  static const uint32_t flags = (1 << 3); 
  const char* app = g_get_prgname();
  const char* topic = mTopic.get();
  dbus_message_append_args(message,
                           DBUS_TYPE_STRING, &app,
                           DBUS_TYPE_UINT32, &xid,
                           DBUS_TYPE_STRING, &topic,
                           DBUS_TYPE_UINT32, &flags,
                           DBUS_TYPE_INVALID);

  return SendMessage(message);
}


bool
WakeLockTopic::SendInhibit()
{
  bool sendOk = false;

  switch (mDesktopEnvironment)
  {
  case FreeDesktop:
    sendOk = SendFreeDesktopInhibitMessage();
    break;
  case GNOME:
    sendOk = SendGNOMEInhibitMessage();
    break;
  case Unsupported:
    return false;
  }

  if (sendOk) {
    mWaitingForReply = true;
  }

  return sendOk;
}

bool
WakeLockTopic::SendUninhibit()
{
  DBusMessage* message = nullptr;

  if (mDesktopEnvironment == FreeDesktop) {
    message =
      dbus_message_new_method_call(FREEDESKTOP_SCREENSAVER_TARGET,
                                   FREEDESKTOP_SCREENSAVER_OBJECT,
                                   FREEDESKTOP_SCREENSAVER_INTERFACE,
                                   "UnInhibit");
  } else if (mDesktopEnvironment == GNOME) {
    message =
      dbus_message_new_method_call(SESSION_MANAGER_TARGET,
                                   SESSION_MANAGER_OBJECT,
                                   SESSION_MANAGER_INTERFACE,
                                   "Uninhibit");
  }

  if (!message) {
    return false;
  }

  dbus_message_append_args(message,
                           DBUS_TYPE_UINT32, &mInhibitRequest,
                           DBUS_TYPE_INVALID);

  dbus_connection_send(mConnection, message, nullptr);
  dbus_connection_flush(mConnection);
  dbus_message_unref(message);

  mInhibitRequest = 0;

  return true;
}

nsresult
WakeLockTopic::InhibitScreensaver()
{
  NS_ASSERTION(!mShouldInhibit, "Screensaver is already inhibited");
  mShouldInhibit = true;

  if (mWaitingForReply) {
    
    
    
    return NS_OK;
  }

  return SendInhibit() ? NS_OK : NS_ERROR_FAILURE;
}

nsresult
WakeLockTopic::UninhibitScreensaver()
{
  if (!mShouldInhibit) {
    
    return NS_OK;
  }

  mShouldInhibit = false;

  if (mWaitingForReply) {
    
    
    
    return NS_OK;
  }

  return SendUninhibit() ? NS_OK : NS_ERROR_FAILURE;
}

void
WakeLockTopic::InhibitFailed()
{
  mWaitingForReply = false;

  if (mDesktopEnvironment == FreeDesktop) {
    mDesktopEnvironment = GNOME;
  } else {
    NS_ASSERTION(mDesktopEnvironment == GNOME, "Unknown desktop environment");
    mDesktopEnvironment = Unsupported;
    mShouldInhibit = false;
  }

  if (!mShouldInhibit) {
    
    
    return;
  }

  SendInhibit();
}

void
WakeLockTopic::InhibitSucceeded(uint32_t aInhibitRequest)
{
  mWaitingForReply = false;
  mInhibitRequest = aInhibitRequest;

  if (!mShouldInhibit) {
    
    
    SendUninhibit();
  }
}

 void
WakeLockTopic::ReceiveInhibitReply(DBusPendingCall* pending, void* user_data)
{
  if (!WakeLockListener::GetSingleton(false)) {
    
    
    return;
  }

  WakeLockTopic* self = static_cast<WakeLockTopic*>(user_data);

  DBusMessage* msg = dbus_pending_call_steal_reply(pending);
  if (!msg) {
    return;
  }

  if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_METHOD_RETURN) {
    uint32_t inhibitRequest;

    if (dbus_message_get_args(msg, nullptr, DBUS_TYPE_UINT32,
                              &inhibitRequest, DBUS_TYPE_INVALID)) {
      self->InhibitSucceeded(inhibitRequest);
    }
  } else {
    self->InhibitFailed();
  }

  dbus_message_unref(msg);
}


WakeLockListener::WakeLockListener()
  : mConnection(dbus_bus_get(DBUS_BUS_SESSION, nullptr))
{
  if (mConnection) {
    dbus_connection_set_exit_on_disconnect(mConnection, false);
    dbus_connection_setup_with_g_main(mConnection, nullptr);
  }
}

WakeLockListener::~WakeLockListener()
{
  if (mConnection) {
    dbus_connection_unref(mConnection);
  }
}

 WakeLockListener*
WakeLockListener::GetSingleton(bool aCreate)
{
  if (!sSingleton && aCreate) {
    sSingleton = new WakeLockListener();
    sSingleton->AddRef();
  }

  return sSingleton;
}

 void
WakeLockListener::Shutdown()
{
  sSingleton->Release();
  sSingleton = nullptr;
}

nsresult
WakeLockListener::Callback(const nsAString& topic, const nsAString& state)
{
  if (!mConnection) {
    return NS_ERROR_FAILURE;
  }

  WakeLockTopic* topicLock = mTopics.Get(topic);
  if (!topicLock) {
    topicLock = new WakeLockTopic(topic, mConnection);
    mTopics.Put(topic, topicLock);
  }

  
  bool shouldLock = state.EqualsLiteral("locked-foreground");

  return shouldLock ?
    topicLock->InhibitScreensaver() :
    topicLock->UninhibitScreensaver();
}

#endif
