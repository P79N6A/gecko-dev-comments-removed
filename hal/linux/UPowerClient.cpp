




































#include <mozilla/Hal.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <mozilla/dom/battery/Constants.h>
#include "nsAutoRef.h"








NS_SPECIALIZE_TEMPLATE
class nsAutoRefTraits<DBusGProxy> : public nsPointerRefTraits<DBusGProxy>
{
public:
  static void Release(DBusGProxy* ptr) { g_object_unref(ptr); }
};

NS_SPECIALIZE_TEMPLATE
class nsAutoRefTraits<GHashTable> : public nsPointerRefTraits<GHashTable>
{
public:
  static void Release(GHashTable* ptr) { g_hash_table_unref(ptr); }
};

using namespace mozilla::dom::battery;

namespace mozilla {
namespace hal_impl {






class UPowerClient
{
public:
  static UPowerClient* GetInstance();

  void BeginListening();
  void StopListening();

  double GetLevel();
  bool   IsCharging();

  ~UPowerClient();

private:
  UPowerClient();

  enum States {
    eState_Unknown = 0,
    eState_Charging,
    eState_Discharging,
    eState_Empty,
    eState_FullyCharged,
    eState_PendingCharge,
    eState_PendingDischarge
  };

  



  void UpdateTrackedDevice();

  



  GHashTable* GetDeviceProperties(const gchar* aDevice);

  



  void UpdateSavedInfo(GHashTable* aHashTable);

  


  static void DeviceChanged(DBusGProxy* aProxy, const gchar* aObjectPath,
                            UPowerClient* aListener);

  


  static DBusHandlerResult ConnectionSignalFilter(DBusConnection* aConnection,
                                                  DBusMessage* aMessage,
                                                  void* aData);

  
  DBusGConnection* mDBusConnection;

  
  DBusGProxy* mUPowerProxy;

  
  gchar* mTrackedDevice;

  double mLevel;
  bool mCharging;

  static UPowerClient* sInstance;

  static const guint sDeviceTypeBattery = 2;
};







void
EnableBatteryNotifications()
{
  UPowerClient::GetInstance()->BeginListening();
}

void
DisableBatteryNotifications()
{
  UPowerClient::GetInstance()->StopListening();
}

void
GetCurrentBatteryInformation(hal::BatteryInformation* aBatteryInfo)
{
  UPowerClient* upowerClient = UPowerClient::GetInstance();

  aBatteryInfo->level() = upowerClient->GetLevel();
  aBatteryInfo->charging() = upowerClient->IsCharging();
  aBatteryInfo->remainingTime() = kUnknownRemainingTime;
}





UPowerClient* UPowerClient::sInstance = nsnull;

 UPowerClient*
UPowerClient::GetInstance()
{
  if (!sInstance) {
    sInstance = new UPowerClient();
  }

  return sInstance;
}

UPowerClient::UPowerClient()
  : mDBusConnection(nsnull)
  , mUPowerProxy(nsnull)
  , mTrackedDevice(nsnull)
  , mLevel(kDefaultLevel)
  , mCharging(kDefaultCharging)
{
}

UPowerClient::~UPowerClient()
{
  NS_ASSERTION(!mDBusConnection && !mUPowerProxy && !mTrackedDevice,
               "The observers have not been correctly removed! "
               "(StopListening should have been called)");
}

void
UPowerClient::BeginListening()
{
  GError* error = nsnull;
  mDBusConnection = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);

  if (!mDBusConnection) {
    g_printerr("Failed to open connection to bus: %s\n", error->message);
    g_error_free(error);
    return;
  }

  DBusConnection* dbusConnection =
    dbus_g_connection_get_connection(mDBusConnection);

  
  dbus_connection_set_exit_on_disconnect(dbusConnection, false);

  
  
  dbus_connection_add_filter(dbusConnection, ConnectionSignalFilter, this,
                             nsnull);

  mUPowerProxy = dbus_g_proxy_new_for_name(mDBusConnection,
                                           "org.freedesktop.UPower",
                                           "/org/freedesktop/UPower",
                                           "org.freedesktop.UPower");

  UpdateTrackedDevice();

  





  dbus_g_proxy_add_signal(mUPowerProxy, "DeviceChanged", G_TYPE_STRING,
                          G_TYPE_INVALID);
  dbus_g_proxy_connect_signal(mUPowerProxy, "DeviceChanged",
                              G_CALLBACK (DeviceChanged), this, nsnull);
}

void
UPowerClient::StopListening()
{
  
  if (!mDBusConnection) {
    return;
  }

  dbus_connection_remove_filter(
      dbus_g_connection_get_connection(mDBusConnection),
      ConnectionSignalFilter, this);

  dbus_g_proxy_disconnect_signal(mUPowerProxy, "DeviceChanged",
                                 G_CALLBACK (DeviceChanged), this);

  g_free(mTrackedDevice);
  mTrackedDevice = nsnull;

  g_object_unref(mUPowerProxy);
  mUPowerProxy = nsnull;

  dbus_g_connection_unref(mDBusConnection);
  mDBusConnection = nsnull;

  
  mLevel = kDefaultLevel;
  mCharging = kDefaultCharging;
}

void
UPowerClient::UpdateTrackedDevice()
{
  GType typeGPtrArray = dbus_g_type_get_collection("GPtrArray",
                                                   DBUS_TYPE_G_OBJECT_PATH);
  GPtrArray* devices = nsnull;
  GError* error = nsnull;

  
  if (!dbus_g_proxy_call(mUPowerProxy, "EnumerateDevices", &error, G_TYPE_INVALID,
                         typeGPtrArray, &devices, G_TYPE_INVALID)) {
    g_printerr ("Error: %s\n", error->message);

    mTrackedDevice = nsnull;
    g_error_free(error);
    return;
  }

  



  for (guint i=0; i<devices->len; ++i) {
    gchar* devicePath = static_cast<gchar*>(g_ptr_array_index(devices, i));
    nsAutoRef<GHashTable> hashTable(GetDeviceProperties(devicePath));

    if (g_value_get_uint(static_cast<const GValue*>(g_hash_table_lookup(hashTable, "Type"))) == sDeviceTypeBattery) {
      UpdateSavedInfo(hashTable);
      mTrackedDevice = devicePath;
      break;
    }

    g_free(devicePath);
  }

#if GLIB_MAJOR_VERSION >= 2 && GLIB_MINOR_VERSION >= 22
    g_ptr_array_unref(devices);
#else
    g_ptr_array_free(devices, true);
#endif
}

 void
UPowerClient::DeviceChanged(DBusGProxy* aProxy, const gchar* aObjectPath, UPowerClient* aListener)
{
#if GLIB_MAJOR_VERSION >= 2 && GLIB_MINOR_VERSION >= 16
  if (g_strcmp0(aObjectPath, aListener->mTrackedDevice)) {
#else
  if (g_ascii_strcasecmp(aObjectPath, aListener->mTrackedDevice)) {
#endif
    return;
  }

  nsAutoRef<GHashTable> hashTable(aListener->GetDeviceProperties(aObjectPath));
  aListener->UpdateSavedInfo(hashTable);

  hal::NotifyBatteryChange(hal::BatteryInformation(aListener->mLevel,
                                                   aListener->mCharging,
                                                   kUnknownRemainingTime));
}

 DBusHandlerResult
UPowerClient::ConnectionSignalFilter(DBusConnection* aConnection,
                                     DBusMessage* aMessage, void* aData)
{
  if (dbus_message_is_signal(aMessage, DBUS_INTERFACE_LOCAL, "Disconnected")) {
    static_cast<UPowerClient*>(aData)->StopListening();
    
    
  }

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

GHashTable*
UPowerClient::GetDeviceProperties(const gchar* aDevice)
{
  nsAutoRef<DBusGProxy> proxy(dbus_g_proxy_new_for_name(mDBusConnection,
                                                        "org.freedesktop.UPower",
                                                        aDevice,
                                                        "org.freedesktop.DBus.Properties"));

  GError* error = nsnull;
  GHashTable* hashTable = nsnull;
  GType typeGHashTable = dbus_g_type_get_map("GHashTable", G_TYPE_STRING,
                                            G_TYPE_VALUE);
  if (!dbus_g_proxy_call(proxy, "GetAll", &error, G_TYPE_STRING,
                         "org.freedesktop.UPower.Device", G_TYPE_INVALID,
                         typeGHashTable, &hashTable, G_TYPE_INVALID)) {
    g_printerr("Error: %s\n", error->message);
    g_error_free(error);
    return nsnull;
  }

  return hashTable;
}

void
UPowerClient::UpdateSavedInfo(GHashTable* aHashTable)
{
  mLevel = g_value_get_double(static_cast<const GValue*>(g_hash_table_lookup(aHashTable, "Percentage")))*0.01;

  switch (g_value_get_uint(static_cast<const GValue*>(g_hash_table_lookup(aHashTable, "State")))) {
    case eState_Unknown:
      mCharging = kDefaultCharging;
      break;
    case eState_Charging:
    case eState_FullyCharged:
    case eState_PendingCharge:
      mCharging = true;
      break;
    case eState_Discharging:
    case eState_Empty:
    case eState_PendingDischarge:
      mCharging = false;
      break;
  }
}

double
UPowerClient::GetLevel()
{
  return mLevel;
}

bool
UPowerClient::IsCharging()
{
  return mCharging;
}

} 
} 
