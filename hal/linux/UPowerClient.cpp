




#include <mozilla/Hal.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <mozilla/dom/battery/Constants.h>
#include "nsAutoRef.h"








template <>
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
  double GetRemainingTime();

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

  



  void UpdateTrackedDeviceSync();

  



  GHashTable* GetDevicePropertiesSync(DBusGProxy* aProxy);
  void GetDevicePropertiesAsync(DBusGProxy* aProxy);
  static void GetDevicePropertiesCallback(DBusGProxy* aProxy,
                                          DBusGProxyCall* aCall,
                                          void* aData);

  



  void UpdateSavedInfo(GHashTable* aHashTable);

  


  static void DeviceChanged(DBusGProxy* aProxy, const gchar* aObjectPath,
                            UPowerClient* aListener);

  


  static DBusHandlerResult ConnectionSignalFilter(DBusConnection* aConnection,
                                                  DBusMessage* aMessage,
                                                  void* aData);

  
  DBusGConnection* mDBusConnection;

  
  DBusGProxy* mUPowerProxy;

  
  gchar* mTrackedDevice;

  
  DBusGProxy* mTrackedDeviceProxy;

  double mLevel;
  bool mCharging;
  double mRemainingTime;

  static UPowerClient* sInstance;

  static const guint sDeviceTypeBattery = 2;
  static const guint64 kUPowerUnknownRemainingTime = 0;
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
  aBatteryInfo->remainingTime() = upowerClient->GetRemainingTime();
}





UPowerClient* UPowerClient::sInstance = nullptr;

 UPowerClient*
UPowerClient::GetInstance()
{
  if (!sInstance) {
    sInstance = new UPowerClient();
  }

  return sInstance;
}

UPowerClient::UPowerClient()
  : mDBusConnection(nullptr)
  , mUPowerProxy(nullptr)
  , mTrackedDevice(nullptr)
  , mTrackedDeviceProxy(nullptr)
  , mLevel(kDefaultLevel)
  , mCharging(kDefaultCharging)
  , mRemainingTime(kDefaultRemainingTime)
{
}

UPowerClient::~UPowerClient()
{
  NS_ASSERTION(!mDBusConnection && !mUPowerProxy && !mTrackedDevice && !mTrackedDeviceProxy,
               "The observers have not been correctly removed! "
               "(StopListening should have been called)");
}

void
UPowerClient::BeginListening()
{
  GError* error = nullptr;
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
                             nullptr);

  mUPowerProxy = dbus_g_proxy_new_for_name(mDBusConnection,
                                           "org.freedesktop.UPower",
                                           "/org/freedesktop/UPower",
                                           "org.freedesktop.UPower");

  UpdateTrackedDeviceSync();

  





  dbus_g_proxy_add_signal(mUPowerProxy, "DeviceChanged", G_TYPE_STRING,
                          G_TYPE_INVALID);
  dbus_g_proxy_connect_signal(mUPowerProxy, "DeviceChanged",
                              G_CALLBACK (DeviceChanged), this, nullptr);
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
  mTrackedDevice = nullptr;

  if (mTrackedDeviceProxy) {
    g_object_unref(mTrackedDeviceProxy);
    mTrackedDeviceProxy = nullptr;
  }

  g_object_unref(mUPowerProxy);
  mUPowerProxy = nullptr;

  dbus_g_connection_unref(mDBusConnection);
  mDBusConnection = nullptr;

  
  mLevel = kDefaultLevel;
  mCharging = kDefaultCharging;
  mRemainingTime = kDefaultRemainingTime;
}

void
UPowerClient::UpdateTrackedDeviceSync()
{
  GType typeGPtrArray = dbus_g_type_get_collection("GPtrArray",
                                                   DBUS_TYPE_G_OBJECT_PATH);
  GPtrArray* devices = nullptr;
  GError* error = nullptr;

  
  g_free(mTrackedDevice);
  mTrackedDevice = nullptr;

  
  if (mTrackedDeviceProxy) {
    g_object_unref(mTrackedDeviceProxy);
    mTrackedDeviceProxy = nullptr;
  }

  
  if (!dbus_g_proxy_call(mUPowerProxy, "EnumerateDevices", &error, G_TYPE_INVALID,
                         typeGPtrArray, &devices, G_TYPE_INVALID)) {
    g_printerr ("Error: %s\n", error->message);
    g_error_free(error);
    return;
  }

  



  for (guint i=0; i<devices->len; ++i) {
    gchar* devicePath = static_cast<gchar*>(g_ptr_array_index(devices, i));

    DBusGProxy* proxy = dbus_g_proxy_new_from_proxy(mUPowerProxy,
                                                    "org.freedesktop.DBus.Properties",
                                                    devicePath);

    nsAutoRef<GHashTable> hashTable(GetDevicePropertiesSync(proxy));

    if (g_value_get_uint(static_cast<const GValue*>(g_hash_table_lookup(hashTable, "Type"))) == sDeviceTypeBattery) {
      UpdateSavedInfo(hashTable);
      mTrackedDevice = devicePath;
      mTrackedDeviceProxy = proxy;
      break;
    }

    g_object_unref(proxy);
    g_free(devicePath);
  }

  g_ptr_array_free(devices, true);
}

 void
UPowerClient::DeviceChanged(DBusGProxy* aProxy, const gchar* aObjectPath, UPowerClient* aListener)
{
  if (!aListener->mTrackedDevice) {
    return;
  }

#if GLIB_MAJOR_VERSION >= 2 && GLIB_MINOR_VERSION >= 16
  if (g_strcmp0(aObjectPath, aListener->mTrackedDevice)) {
#else
  if (g_ascii_strcasecmp(aObjectPath, aListener->mTrackedDevice)) {
#endif
    return;
  }

  aListener->GetDevicePropertiesAsync(aListener->mTrackedDeviceProxy);
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
UPowerClient::GetDevicePropertiesSync(DBusGProxy* aProxy)
{
  GError* error = nullptr;
  GHashTable* hashTable = nullptr;
  GType typeGHashTable = dbus_g_type_get_map("GHashTable", G_TYPE_STRING,
                                            G_TYPE_VALUE);
  if (!dbus_g_proxy_call(aProxy, "GetAll", &error, G_TYPE_STRING,
                         "org.freedesktop.UPower.Device", G_TYPE_INVALID,
                         typeGHashTable, &hashTable, G_TYPE_INVALID)) {
    g_printerr("Error: %s\n", error->message);
    g_error_free(error);
    return nullptr;
  }

  return hashTable;
}

 void
UPowerClient::GetDevicePropertiesCallback(DBusGProxy* aProxy,
                                          DBusGProxyCall* aCall, void* aData)
{
  GError* error = nullptr;
  GHashTable* hashTable = nullptr;
  GType typeGHashTable = dbus_g_type_get_map("GHashTable", G_TYPE_STRING,
                                             G_TYPE_VALUE);
  if (!dbus_g_proxy_end_call(aProxy, aCall, &error, typeGHashTable,
                             &hashTable, G_TYPE_INVALID)) {
    g_printerr("Error: %s\n", error->message);
    g_error_free(error);
  } else {
    sInstance->UpdateSavedInfo(hashTable);
    hal::NotifyBatteryChange(hal::BatteryInformation(sInstance->mLevel,
                                                     sInstance->mCharging,
                                                     sInstance->mRemainingTime));
    g_hash_table_unref(hashTable);
  }
}

void
UPowerClient::GetDevicePropertiesAsync(DBusGProxy* aProxy)
{
  dbus_g_proxy_begin_call(aProxy, "GetAll", GetDevicePropertiesCallback, nullptr,
                          nullptr, G_TYPE_STRING,
                          "org.freedesktop.UPower.Device", G_TYPE_INVALID);
}

void
UPowerClient::UpdateSavedInfo(GHashTable* aHashTable)
{
  bool isFull = false;

  
















  switch (g_value_get_uint(static_cast<const GValue*>(g_hash_table_lookup(aHashTable, "State")))) {
    case eState_Unknown:
      mCharging = kDefaultCharging;
      break;
    case eState_FullyCharged:
      isFull = true;
    case eState_Charging:
    case eState_PendingCharge:
      mCharging = true;
      break;
    case eState_Discharging:
    case eState_Empty:
    case eState_PendingDischarge:
      mCharging = false;
      break;
  }

  




  if (isFull) {
    mLevel = 1.0;
  } else {
    mLevel = g_value_get_double(static_cast<const GValue*>(g_hash_table_lookup(aHashTable, "Percentage")))*0.01;
  }

  if (isFull) {
    mRemainingTime = 0;
  } else {
    mRemainingTime = mCharging ? g_value_get_int64(static_cast<const GValue*>(g_hash_table_lookup(aHashTable, "TimeToFull")))
                               : g_value_get_int64(static_cast<const GValue*>(g_hash_table_lookup(aHashTable, "TimeToEmpty")));

    if (mRemainingTime == kUPowerUnknownRemainingTime) {
      mRemainingTime = kUnknownRemainingTime;
    }
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

double
UPowerClient::GetRemainingTime()
{
  return mRemainingTime;
}

} 
} 
