

















#include "base/basictypes.h"
#include "BluetoothDBusService.h"
#include "BluetoothA2dpManager.h"
#include "BluetoothHfpManager.h"
#include "BluetoothHidManager.h"
#include "BluetoothOppManager.h"
#include "BluetoothReplyRunnable.h"
#include "BluetoothUnixSocketConnector.h"
#include "BluetoothUtils.h"
#include "BluetoothUuid.h"

#include <cstdio>
#include <dbus/dbus.h>

#include "nsAutoPtr.h"
#include "nsThreadUtils.h"
#include "nsDebug.h"
#include "nsDataHashtable.h"
#include "nsPrintfCString.h"
#include "mozilla/Atomics.h"
#include "mozilla/dom/bluetooth/BluetoothTypes.h"
#include "mozilla/Hal.h"
#include "mozilla/ipc/UnixSocket.h"
#include "mozilla/ipc/DBusThread.h"
#include "mozilla/ipc/DBusUtils.h"
#include "mozilla/ipc/RawDBusConnection.h"
#include "mozilla/Mutex.h"
#include "mozilla/NullPtr.h"
#include "mozilla/StaticMutex.h"
#include "mozilla/Util.h"
#if defined(MOZ_WIDGET_GONK)
#include "cutils/properties.h"
#endif











using namespace mozilla;
using namespace mozilla::ipc;
USING_BLUETOOTH_NAMESPACE

#define B2G_AGENT_CAPABILITIES "DisplayYesNo"
#define DBUS_MANAGER_IFACE BLUEZ_DBUS_BASE_IFC  ".Manager"
#define DBUS_ADAPTER_IFACE BLUEZ_DBUS_BASE_IFC  ".Adapter"
#define DBUS_DEVICE_IFACE  BLUEZ_DBUS_BASE_IFC  ".Device"
#define DBUS_AGENT_IFACE   BLUEZ_DBUS_BASE_IFC  ".Agent"
#define DBUS_SINK_IFACE    BLUEZ_DBUS_BASE_IFC  ".AudioSink"
#define DBUS_CTL_IFACE     BLUEZ_DBUS_BASE_IFC  ".Control"
#define DBUS_INPUT_IFACE   BLUEZ_DBUS_BASE_IFC  ".Input"
#define BLUEZ_DBUS_BASE_PATH      "/org/bluez"
#define BLUEZ_DBUS_BASE_IFC       "org.bluez"
#define BLUEZ_ERROR_IFC           "org.bluez.Error"

#define ERR_A2DP_IS_DISCONNECTED      "A2dpIsDisconnected"
#define ERR_AVRCP_IS_DISCONNECTED     "AvrcpIsDisconnected"
#define ERR_UNKNOWN_PROFILE           "UnknownProfileError"






#define TIMEOUT_FORCE_TO_DISABLE_BT 5

typedef struct {
  const char* name;
  int type;
} Properties;

static Properties sDeviceProperties[] = {
  {"Address", DBUS_TYPE_STRING},
  {"Name", DBUS_TYPE_STRING},
  {"Icon", DBUS_TYPE_STRING},
  {"Class", DBUS_TYPE_UINT32},
  {"UUIDs", DBUS_TYPE_ARRAY},
  {"Paired", DBUS_TYPE_BOOLEAN},
  {"Connected", DBUS_TYPE_BOOLEAN},
  {"Trusted", DBUS_TYPE_BOOLEAN},
  {"Blocked", DBUS_TYPE_BOOLEAN},
  {"Alias", DBUS_TYPE_STRING},
  {"Nodes", DBUS_TYPE_ARRAY},
  {"Adapter", DBUS_TYPE_OBJECT_PATH},
  {"LegacyPairing", DBUS_TYPE_BOOLEAN},
  {"RSSI", DBUS_TYPE_INT16},
  {"TX", DBUS_TYPE_UINT32},
  {"Type", DBUS_TYPE_STRING},
  {"Broadcaster", DBUS_TYPE_BOOLEAN},
  {"Services", DBUS_TYPE_ARRAY}
};

static Properties sAdapterProperties[] = {
  {"Address", DBUS_TYPE_STRING},
  {"Name", DBUS_TYPE_STRING},
  {"Class", DBUS_TYPE_UINT32},
  {"Powered", DBUS_TYPE_BOOLEAN},
  {"Discoverable", DBUS_TYPE_BOOLEAN},
  {"DiscoverableTimeout", DBUS_TYPE_UINT32},
  {"Pairable", DBUS_TYPE_BOOLEAN},
  {"PairableTimeout", DBUS_TYPE_UINT32},
  {"Discovering", DBUS_TYPE_BOOLEAN},
  {"Devices", DBUS_TYPE_ARRAY},
  {"UUIDs", DBUS_TYPE_ARRAY},
  {"Type", DBUS_TYPE_STRING}
};

static Properties sManagerProperties[] = {
  {"Adapters", DBUS_TYPE_ARRAY},
};

static Properties sSinkProperties[] = {
  {"State", DBUS_TYPE_STRING},
  {"Connected", DBUS_TYPE_BOOLEAN},
  {"Playing", DBUS_TYPE_BOOLEAN}
};

static Properties sControlProperties[] = {
  {"Connected", DBUS_TYPE_BOOLEAN}
};

static Properties sInputProperties[] = {
  {"Connected", DBUS_TYPE_BOOLEAN}
};

static const char* sBluetoothDBusIfaces[] =
{
  DBUS_MANAGER_IFACE,
  DBUS_ADAPTER_IFACE,
  DBUS_DEVICE_IFACE
};

static const char* sBluetoothDBusSignals[] =
{
  "type='signal',interface='org.freedesktop.DBus'",
  "type='signal',interface='org.bluez.Adapter'",
  "type='signal',interface='org.bluez.Manager'",
  "type='signal',interface='org.bluez.Device'",
  "type='signal',interface='org.bluez.Input'",
  "type='signal',interface='org.bluez.Network'",
  "type='signal',interface='org.bluez.NetworkServer'",
  "type='signal',interface='org.bluez.HealthDevice'",
  "type='signal',interface='org.bluez.AudioSink'",
  "type='signal',interface='org.bluez.Control'"
};






static nsRefPtr<RawDBusConnection> gThreadConnection;
static nsDataHashtable<nsStringHashKey, DBusMessage* > sPairingReqTable;
static nsTArray<uint32_t> sAuthorizedServiceClass;
static nsString sAdapterPath;
static Atomic<int32_t> sIsPairing(0);
static int sConnectedDeviceCount = 0;
static Monitor sStopBluetoothMonitor("BluetoothService.sStopBluetoothMonitor");

typedef void (*UnpackFunc)(DBusMessage*, DBusError*, BluetoothValue&, nsAString&);
typedef bool (*FilterFunc)(const BluetoothValue&);

static bool
GetConnectedDevicesFilter(const BluetoothValue& aValue)
{
  
  return true;
}

static bool
GetPairedDevicesFilter(const BluetoothValue& aValue)
{
  
  if (aValue.type() != BluetoothValue::TArrayOfBluetoothNamedValue) {
    NS_WARNING("Not a BluetoothNamedValue array!");
    return false;
  }

  const InfallibleTArray<BluetoothNamedValue>& deviceProperties =
    aValue.get_ArrayOfBluetoothNamedValue();
  uint32_t length = deviceProperties.Length();
  for (uint32_t p = 0; p < length; ++p) {
    if (deviceProperties[p].name().EqualsLiteral("Paired")) {
      return deviceProperties[p].value().get_bool();
    }
  }

  return false;
}

class DistributeBluetoothSignalTask : public nsRunnable
{
public:
  DistributeBluetoothSignalTask(const BluetoothSignal& aSignal)
    : mSignal(aSignal)
  {
  }

  nsresult Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    BluetoothService* bs = BluetoothService::Get();
    NS_ENSURE_TRUE(bs, NS_ERROR_FAILURE);
    bs->DistributeSignal(mSignal);

    return NS_OK;
  }

private:
  BluetoothSignal mSignal;
};

class ControlPropertyChangedHandler : public nsRunnable
{
public:
  ControlPropertyChangedHandler(const BluetoothSignal& aSignal)
    : mSignal(aSignal)
  {
  }

  nsresult Run()
  {
    MOZ_ASSERT(NS_IsMainThread());
    if (mSignal.value().type() != BluetoothValue::TArrayOfBluetoothNamedValue) {
       BT_WARNING("Wrong value type for ControlPropertyChangedHandler");
       return NS_ERROR_FAILURE;
    }

    InfallibleTArray<BluetoothNamedValue>& arr =
      mSignal.value().get_ArrayOfBluetoothNamedValue();
    MOZ_ASSERT(arr[0].name().EqualsLiteral("Connected"));
    MOZ_ASSERT(arr[0].value().type() == BluetoothValue::Tbool);
    bool connected = arr[0].value().get_bool();

    BluetoothA2dpManager* a2dp = BluetoothA2dpManager::Get();
    NS_ENSURE_TRUE(a2dp, NS_ERROR_FAILURE);
    a2dp->SetAvrcpConnected(connected);
    return NS_OK;
  }

private:
  BluetoothSignal mSignal;
};

class SinkPropertyChangedHandler : public nsRunnable
{
public:
  SinkPropertyChangedHandler(const BluetoothSignal& aSignal)
    : mSignal(aSignal)
  {
  }

  NS_IMETHOD
  Run()
  {
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(mSignal.name().EqualsLiteral("PropertyChanged"));
    MOZ_ASSERT(mSignal.value().type() ==
               BluetoothValue::TArrayOfBluetoothNamedValue);

    
    nsString address = GetAddressFromObjectPath(mSignal.path());
    mSignal.path() = address;

    BluetoothA2dpManager* a2dp = BluetoothA2dpManager::Get();
    NS_ENSURE_TRUE(a2dp, NS_ERROR_FAILURE);
    a2dp->HandleSinkPropertyChanged(mSignal);
    return NS_OK;
  }

private:
  BluetoothSignal mSignal;
};

class InputPropertyChangedHandler : public nsRunnable
{
public:
  InputPropertyChangedHandler(const BluetoothSignal& aSignal)
    : mSignal(aSignal)
  {
  }

  NS_IMETHOD
  Run()
  {
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(mSignal.name().EqualsLiteral("PropertyChanged"));
    MOZ_ASSERT(mSignal.value().type() == BluetoothValue::TArrayOfBluetoothNamedValue);

    
    nsString address = GetAddressFromObjectPath(mSignal.path());
    mSignal.path() = address;

    BluetoothHidManager* hid = BluetoothHidManager::Get();
    NS_ENSURE_TRUE(hid, NS_ERROR_FAILURE);
    hid->HandleInputPropertyChanged(mSignal);
    return NS_OK;
  }

private:
  BluetoothSignal mSignal;
};

static bool
IsDBusMessageError(DBusMessage* aMsg, DBusError* aErr, nsAString& aErrorStr)
{
  if (aErr && dbus_error_is_set(aErr)) {
    aErrorStr = NS_ConvertUTF8toUTF16(aErr->message);
    LOG_AND_FREE_DBUS_ERROR(aErr);
    return true;
  }

  DBusError err;
  dbus_error_init(&err);
  if (dbus_message_get_type(aMsg) == DBUS_MESSAGE_TYPE_ERROR) {
    const char* error_msg;
    if (!dbus_message_get_args(aMsg, &err, DBUS_TYPE_STRING,
                               &error_msg, DBUS_TYPE_INVALID) ||
        !error_msg) {
      if (dbus_error_is_set(&err)) {
        aErrorStr = NS_ConvertUTF8toUTF16(err.message);
        LOG_AND_FREE_DBUS_ERROR(&err);
        return true;
      } else {
        aErrorStr.AssignLiteral("Unknown Error");
        return true;
      }
    } else {
      aErrorStr = NS_ConvertUTF8toUTF16(error_msg);
      return true;
    }
  }
  return false;
}

static void
UnpackIntMessage(DBusMessage* aMsg, DBusError* aErr,
                 BluetoothValue& aValue, nsAString& aErrorStr)
{
  MOZ_ASSERT(aMsg);

  DBusError err;
  dbus_error_init(&err);
  if (!IsDBusMessageError(aMsg, aErr, aErrorStr)) {
    MOZ_ASSERT(dbus_message_get_type(aMsg) == DBUS_MESSAGE_TYPE_METHOD_RETURN,
               "Got dbus callback that's not a METHOD_RETURN!");
    int i;
    if (!dbus_message_get_args(aMsg, &err, DBUS_TYPE_INT32,
                               &i, DBUS_TYPE_INVALID)) {
      if (dbus_error_is_set(&err)) {
        aErrorStr = NS_ConvertUTF8toUTF16(err.message);
        LOG_AND_FREE_DBUS_ERROR(&err);
      }
    } else {
      aValue = (uint32_t)i;
    }
  }
}

static void
UnpackObjectPathMessage(DBusMessage* aMsg, DBusError* aErr,
                        BluetoothValue& aValue, nsAString& aErrorStr)
{
  DBusError err;
  dbus_error_init(&err);
  if (!IsDBusMessageError(aMsg, aErr, aErrorStr)) {
    MOZ_ASSERT(dbus_message_get_type(aMsg) == DBUS_MESSAGE_TYPE_METHOD_RETURN,
               "Got dbus callback that's not a METHOD_RETURN!");
    const char* object_path;
    if (!dbus_message_get_args(aMsg, &err, DBUS_TYPE_OBJECT_PATH,
                               &object_path, DBUS_TYPE_INVALID) ||
        !object_path) {
      if (dbus_error_is_set(&err)) {
        aErrorStr = NS_ConvertUTF8toUTF16(err.message);
        LOG_AND_FREE_DBUS_ERROR(&err);
      }
    } else {
      aValue = NS_ConvertUTF8toUTF16(object_path);
    }
  }
}

void
BluetoothDBusService::DisconnectAllAcls(const nsAString& aAdapterPath)
{
  MOZ_ASSERT(!NS_IsMainThread());

  DBusMessage* reply =
    dbus_func_args(gThreadConnection->GetConnection(),
                   NS_ConvertUTF16toUTF8(aAdapterPath).get(),
                   DBUS_ADAPTER_IFACE, "DisconnectAllConnections",
                   DBUS_TYPE_INVALID);

  if (reply) {
    dbus_message_unref(reply);
  }
}

class PrepareProfileManagersRunnable : public nsRunnable
{
public:
  nsresult Run()
  {
    BluetoothHfpManager* hfp = BluetoothHfpManager::Get();
    if (!hfp || !hfp->Listen()) {
      NS_WARNING("Failed to start listening for BluetoothHfpManager!");
      return NS_ERROR_FAILURE;
    }

    BluetoothOppManager* opp = BluetoothOppManager::Get();
    if (!opp || !opp->Listen()) {
      NS_WARNING("Failed to start listening for BluetoothOppManager!");
      return NS_ERROR_FAILURE;
    }

    BluetoothA2dpManager* a2dp = BluetoothA2dpManager::Get();
    NS_ENSURE_TRUE(a2dp, NS_ERROR_FAILURE);
    a2dp->ResetA2dp();
    a2dp->ResetAvrcp();

    return NS_OK;
  }
};

static void
RunDBusCallback(DBusMessage* aMsg, void* aBluetoothReplyRunnable,
                UnpackFunc aFunc)
{
#ifdef MOZ_WIDGET_GONK
  
  
  
  
  MOZ_ASSERT(!NS_IsMainThread());
#endif
  nsRefPtr<BluetoothReplyRunnable> replyRunnable =
    dont_AddRef(static_cast< BluetoothReplyRunnable* >(aBluetoothReplyRunnable));

  MOZ_ASSERT(replyRunnable, "Callback reply runnable is null!");

  nsAutoString replyError;
  BluetoothValue v;
  aFunc(aMsg, nullptr, v, replyError);
  DispatchBluetoothReply(replyRunnable, v, replyError);
}

static void
GetObjectPathCallback(DBusMessage* aMsg, void* aBluetoothReplyRunnable)
{
  if (sIsPairing) {
    RunDBusCallback(aMsg, aBluetoothReplyRunnable,
                    UnpackObjectPathMessage);
    sIsPairing--;
  }
}

static void
UnpackVoidMessage(DBusMessage* aMsg, DBusError* aErr, BluetoothValue& aValue,
                  nsAString& aErrorStr)
{
  DBusError err;
  dbus_error_init(&err);
  if (!IsDBusMessageError(aMsg, aErr, aErrorStr) &&
      dbus_message_get_type(aMsg) == DBUS_MESSAGE_TYPE_METHOD_RETURN &&
      !dbus_message_get_args(aMsg, &err, DBUS_TYPE_INVALID)) {
    if (dbus_error_is_set(&err)) {
      aErrorStr = NS_ConvertUTF8toUTF16(err.message);
      LOG_AND_FREE_DBUS_ERROR(&err);
    }
  }
  aValue = aErrorStr.IsEmpty();
}

static void
GetVoidCallback(DBusMessage* aMsg, void* aBluetoothReplyRunnable)
{
  RunDBusCallback(aMsg, aBluetoothReplyRunnable,
                  UnpackVoidMessage);
}

static void
GetIntCallback(DBusMessage* aMsg, void* aBluetoothReplyRunnable)
{
  RunDBusCallback(aMsg, aBluetoothReplyRunnable,
                  UnpackIntMessage);
}

#ifdef DEBUG
static void
CheckForError(DBusMessage* aMsg, void *aParam, const nsAString& aError)
{
  NS_ENSURE_TRUE_VOID(aMsg);

  BluetoothValue v;
  nsAutoString replyError;
  UnpackVoidMessage(aMsg, nullptr, v, replyError);
  if (!v.get_bool()) {
    BT_WARNING(NS_ConvertUTF16toUTF8(aError).get());
  }
}
#endif

static void
InputDisconnectCallback(DBusMessage* aMsg, void* aParam)
{
#ifdef DEBUG
  NS_NAMED_LITERAL_STRING(errorStr, "Failed to disconnect input device");
  CheckForError(aMsg, aParam, errorStr);
#endif
}

static void
SinkConnectCallback(DBusMessage* aMsg, void* aParam)
{
#ifdef DEBUG
  NS_NAMED_LITERAL_STRING(errorStr, "Failed to connect sink");
  CheckForError(aMsg, aParam, errorStr);
#endif
}

static void
SinkDisconnectCallback(DBusMessage* aMsg, void* aParam)
{
#ifdef DEBUG
  NS_NAMED_LITERAL_STRING(errorStr, "Failed to disconnect sink");
  CheckForError(aMsg, aParam, errorStr);
#endif
}

static bool
HasAudioService(uint32_t aCodValue)
{
  return ((aCodValue & 0x200000) == 0x200000);
}

static bool
ContainsIcon(const InfallibleTArray<BluetoothNamedValue>& aProperties)
{
  for (uint8_t i = 0; i < aProperties.Length(); i++) {
    if (aProperties[i].name().EqualsLiteral("Icon")) {
      return true;
    }
  }
  return false;
}

static bool
GetProperty(DBusMessageIter aIter, Properties* aPropertyTypes,
            int aPropertyTypeLen, int* aPropIndex,
            InfallibleTArray<BluetoothNamedValue>& aProperties)
{
  DBusMessageIter prop_val, array_val_iter;
  char* property = NULL;
  uint32_t array_type;
  int i, expectedType, receivedType;

  if (dbus_message_iter_get_arg_type(&aIter) != DBUS_TYPE_STRING) {
    return false;
  }

  dbus_message_iter_get_basic(&aIter, &property);

  if (!dbus_message_iter_next(&aIter) ||
      dbus_message_iter_get_arg_type(&aIter) != DBUS_TYPE_VARIANT) {
    return false;
  }

  for (i = 0; i < aPropertyTypeLen; i++) {
    if (!strncmp(property, aPropertyTypes[i].name, strlen(property))) {
      break;
    }
  }

  if (i == aPropertyTypeLen) {
    return false;
  }

  nsAutoString propertyName;
  propertyName.AssignASCII(aPropertyTypes[i].name);
  *aPropIndex = i;

  dbus_message_iter_recurse(&aIter, &prop_val);
  expectedType = aPropertyTypes[*aPropIndex].type;
  receivedType = dbus_message_iter_get_arg_type(&prop_val);

  




  bool convert = false;
  if (propertyName.EqualsLiteral("Connected") &&
      receivedType == DBUS_TYPE_ARRAY) {
    convert = true;
  }

  if ((receivedType != expectedType) && !convert) {
    NS_WARNING(nsPrintfCString("Iterator not type we expect! Property name: %s,
      Property Type Expected: %d, Property Type Received: %d",
      NS_ConvertUTF16toUTF8(propertyName).get(), expectedType, receivedType).get());
    return false;
  }

  BluetoothValue propertyValue;
  switch (receivedType) {
    case DBUS_TYPE_STRING:
    case DBUS_TYPE_OBJECT_PATH:
      const char* c;
      dbus_message_iter_get_basic(&prop_val, &c);
      propertyValue = NS_ConvertUTF8toUTF16(c);
      break;
    case DBUS_TYPE_UINT32:
    case DBUS_TYPE_INT16:
      uint32_t i;
      dbus_message_iter_get_basic(&prop_val, &i);
      propertyValue = i;
      break;
    case DBUS_TYPE_BOOLEAN:
      bool b;
      dbus_message_iter_get_basic(&prop_val, &b);
      propertyValue = b;
      break;
    case DBUS_TYPE_ARRAY:
      dbus_message_iter_recurse(&prop_val, &array_val_iter);
      array_type = dbus_message_iter_get_arg_type(&array_val_iter);
      if (array_type == DBUS_TYPE_OBJECT_PATH ||
          array_type == DBUS_TYPE_STRING) {
        InfallibleTArray<nsString> arr;
        do {
          const char* tmp;
          dbus_message_iter_get_basic(&array_val_iter, &tmp);
          nsAutoString s;
          s = NS_ConvertUTF8toUTF16(tmp);
          arr.AppendElement(s);
        } while (dbus_message_iter_next(&array_val_iter));
        propertyValue = arr;
      } else if (array_type == DBUS_TYPE_BYTE) {
        InfallibleTArray<uint8_t> arr;
        do {
          uint8_t tmp;
          dbus_message_iter_get_basic(&array_val_iter, &tmp);
          arr.AppendElement(tmp);
        } while (dbus_message_iter_next(&array_val_iter));
        propertyValue = arr;
      } else {
        
        
        propertyValue = InfallibleTArray<nsString>();
      }
      break;
    default:
      NS_NOTREACHED("Cannot find dbus message type!");
  }

  if (convert) {
    MOZ_ASSERT(propertyValue.type() == BluetoothValue::TArrayOfuint8_t);

    bool b = propertyValue.get_ArrayOfuint8_t()[0];
    propertyValue = BluetoothValue(b);
  }

  aProperties.AppendElement(BluetoothNamedValue(propertyName, propertyValue));
  return true;
}

static void
ParseProperties(DBusMessageIter* aIter,
                BluetoothValue& aValue,
                nsAString& aErrorStr,
                Properties* aPropertyTypes,
                const int aPropertyTypeLen)
{
  DBusMessageIter dict_entry, dict;
  int prop_index = -1;

  MOZ_ASSERT(dbus_message_iter_get_arg_type(aIter) == DBUS_TYPE_ARRAY,
             "Trying to parse a property from sth. that's not an array");

  dbus_message_iter_recurse(aIter, &dict);
  InfallibleTArray<BluetoothNamedValue> props;
  do {
    MOZ_ASSERT(dbus_message_iter_get_arg_type(&dict) == DBUS_TYPE_DICT_ENTRY,
               "Trying to parse a property from sth. that's not an dict!");
    dbus_message_iter_recurse(&dict, &dict_entry);

    if (!GetProperty(dict_entry, aPropertyTypes, aPropertyTypeLen, &prop_index,
                     props)) {
      aErrorStr.AssignLiteral("Can't Create Property!");
      NS_WARNING("Can't create property!");
      return;
    }
  } while (dbus_message_iter_next(&dict));

  aValue = props;
}

static bool
UnpackPropertiesMessage(DBusMessage* aMsg, DBusError* aErr,
                        BluetoothValue& aValue, const char* aIface)
{
  MOZ_ASSERT(aMsg);

  Properties* propertyTypes;
  int propertyTypesLength;

  nsAutoString errorStr;
  if (IsDBusMessageError(aMsg, aErr, errorStr) ||
      dbus_message_get_type(aMsg) != DBUS_MESSAGE_TYPE_METHOD_RETURN) {
    BT_WARNING("dbus message has an error.");
    return false;
  }

  DBusMessageIter iter;
  if (!dbus_message_iter_init(aMsg, &iter)) {
    BT_WARNING("Cannot create dbus message iter!");
    return false;
  }

  if (!strcmp(aIface, DBUS_DEVICE_IFACE)) {
    propertyTypes = sDeviceProperties;
    propertyTypesLength = ArrayLength(sDeviceProperties);
  } else if (!strcmp(aIface, DBUS_ADAPTER_IFACE)) {
    propertyTypes = sAdapterProperties;
    propertyTypesLength = ArrayLength(sAdapterProperties);
  } else if (!strcmp(aIface, DBUS_MANAGER_IFACE)) {
    propertyTypes = sManagerProperties;
    propertyTypesLength = ArrayLength(sManagerProperties);
  } else {
    return false;
  }

  ParseProperties(&iter, aValue, errorStr, propertyTypes,
                  propertyTypesLength);
  return true;
}

static void
ParsePropertyChange(DBusMessage* aMsg, BluetoothValue& aValue,
                    nsAString& aErrorStr, Properties* aPropertyTypes,
                    const int aPropertyTypeLen)
{
  DBusMessageIter iter;
  DBusError err;
  int prop_index = -1;
  InfallibleTArray<BluetoothNamedValue> props;

  dbus_error_init(&err);
  if (!dbus_message_iter_init(aMsg, &iter)) {
    NS_WARNING("Can't create iterator!");
    return;
  }

  if (!GetProperty(iter, aPropertyTypes, aPropertyTypeLen,
                   &prop_index, props)) {
    NS_WARNING("Can't get property!");
    aErrorStr.AssignLiteral("Can't get property!");
    return;
  }
  aValue = props;
}

class AppendDeviceNameReplyHandler: public DBusReplyHandler
{
public:
  AppendDeviceNameReplyHandler(const nsCString& aIface,
                               const nsString& aDevicePath,
                               const BluetoothSignal& aSignal)
    : mIface(aIface)
    , mDevicePath(aDevicePath)
    , mSignal(aSignal)
  {
    MOZ_ASSERT(!mIface.IsEmpty());
    MOZ_ASSERT(!mDevicePath.IsEmpty());
  }

  void Handle(DBusMessage* aReply) MOZ_OVERRIDE
  {
    MOZ_ASSERT(!NS_IsMainThread()); 

    if (!aReply || (dbus_message_get_type(aReply) == DBUS_MESSAGE_TYPE_ERROR)) {
      return;
    }

    

    DBusError err;
    dbus_error_init(&err);

    BluetoothValue deviceProperties;

    bool success = UnpackPropertiesMessage(aReply, &err, deviceProperties,
                                           mIface.get());
    if (!success) {
      BT_WARNING("Failed to get device properties");
      return;
    }

    

    InfallibleTArray<BluetoothNamedValue>& parameters =
      mSignal.value().get_ArrayOfBluetoothNamedValue();
    nsString address =
      GetAddressFromObjectPath(mDevicePath);
    parameters[0].name().AssignLiteral("address");
    parameters[0].value() = address;

    

    InfallibleTArray<BluetoothNamedValue>& properties =
      deviceProperties.get_ArrayOfBluetoothNamedValue();
    InfallibleTArray<BluetoothNamedValue>::size_type i;
    for (i = 0; i < properties.Length(); i++) {
      if (properties[i].name().EqualsLiteral("Name")) {
        properties[i].name().AssignLiteral("name");
        parameters.AppendElement(properties[i]);
        break;
      }
    }
    MOZ_ASSERT_IF(i == properties.Length(), "failed to get device name");

    nsRefPtr<DistributeBluetoothSignalTask> task =
      new DistributeBluetoothSignalTask(mSignal);
    NS_DispatchToMainThread(task);
  }

private:
  nsCString mIface;
  nsString mDevicePath;
  BluetoothSignal mSignal;
};

static void
AppendDeviceName(BluetoothSignal& aSignal)
{
  BluetoothValue v = aSignal.value();
  if (v.type() != BluetoothValue::TArrayOfBluetoothNamedValue ||
      v.get_ArrayOfBluetoothNamedValue().Length() == 0) {
    BT_WARNING("Invalid argument type for AppendDeviceNameRunnable");
    return;
  }
  const InfallibleTArray<BluetoothNamedValue>& arr =
    v.get_ArrayOfBluetoothNamedValue();

  
  if (!arr[0].name().EqualsLiteral("path") ||
       arr[0].value().type() != BluetoothValue::TnsString) {
    BT_WARNING("Invalid object path for AppendDeviceNameRunnable");
    return;
  }

  nsString devicePath = arr[0].value().get_nsString();

  nsRefPtr<RawDBusConnection> threadConnection = gThreadConnection;

  if (!threadConnection.get()) {
    BT_WARNING("%s: DBus connection has been closed.", __FUNCTION__);
    return;
  }

  nsRefPtr<AppendDeviceNameReplyHandler> handler =
    new AppendDeviceNameReplyHandler(nsCString(DBUS_DEVICE_IFACE),
                                     devicePath, aSignal);
  bool success = dbus_func_args_async(threadConnection->GetConnection(), 1000,
                                      AppendDeviceNameReplyHandler::Callback,
                                      handler.get(),
                                      NS_ConvertUTF16toUTF8(devicePath).get(),
                                      DBUS_DEVICE_IFACE, "GetProperties",
                                      DBUS_TYPE_INVALID);
  NS_ENSURE_TRUE_VOID(success);

  handler.forget();
}

static DBusHandlerResult
AgentEventFilter(DBusConnection *conn, DBusMessage *msg, void *data)
{
  if (dbus_message_get_type(msg) != DBUS_MESSAGE_TYPE_METHOD_CALL) {
    BT_WARNING("%s: agent handler not interested (not a method call).\n",
               __FUNCTION__);
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }

  DBusError err;
  dbus_error_init(&err);

  BT_LOG("%s: %s, %s", __FUNCTION__,
                       dbus_message_get_path(msg),
                       dbus_message_get_member(msg));

  nsString signalPath = NS_ConvertUTF8toUTF16(dbus_message_get_path(msg));
  nsString signalName = NS_ConvertUTF8toUTF16(dbus_message_get_member(msg));
  nsString errorStr;
  BluetoothValue v;
  InfallibleTArray<BluetoothNamedValue> parameters;
  bool isPairingReq = false;
  BluetoothSignal signal(signalName, signalPath, v);
  char *objectPath;

  
  
  
  
  if (dbus_message_is_method_call(msg, DBUS_AGENT_IFACE, "Cancel")) {
    
    

    
    DBusMessage *reply = dbus_message_new_method_return(msg);

    if (!reply) {
      errorStr.AssignLiteral("Memory can't be allocated for the message.");
      goto handle_error;
    }

    dbus_connection_send(conn, reply, NULL);
    dbus_message_unref(reply);
    v = parameters;
  } else if (dbus_message_is_method_call(msg, DBUS_AGENT_IFACE, "Authorize")) {
    
    
    const char *uuid;
    if (!dbus_message_get_args(msg, NULL,
                               DBUS_TYPE_OBJECT_PATH, &objectPath,
                               DBUS_TYPE_STRING, &uuid,
                               DBUS_TYPE_INVALID)) {
      errorStr.AssignLiteral("Invalid arguments for Authorize() method");
      goto handle_error;
    }

    NS_ConvertUTF8toUTF16 uuidStr(uuid);
    BluetoothServiceClass serviceClass =
      BluetoothUuidHelper::GetBluetoothServiceClass(uuidStr);
    if (serviceClass == BluetoothServiceClass::UNKNOWN) {
      errorStr.AssignLiteral("Failed to get service class");
      goto handle_error;
    }

    DBusMessage* reply;
    int i;
    int length = sAuthorizedServiceClass.Length();
    for (i = 0; i < length; i++) {
      if (serviceClass == sAuthorizedServiceClass[i]) {
        reply = dbus_message_new_method_return(msg);
        break;
      }
    }

    
    if (i == length) {
      BT_WARNING("Uuid is not authorized.");
      reply = dbus_message_new_error(msg, "org.bluez.Error.Rejected",
                                     "The uuid is not authorized");
    }

    if (!reply) {
      errorStr.AssignLiteral("Memory can't be allocated for the message.");
      goto handle_error;
    }

    dbus_connection_send(conn, reply, NULL);
    dbus_message_unref(reply);
    return DBUS_HANDLER_RESULT_HANDLED;
  } else if (dbus_message_is_method_call(msg, DBUS_AGENT_IFACE,
                                         "RequestConfirmation")) {
    
    
    uint32_t passkey;
    if (!dbus_message_get_args(msg, NULL,
                               DBUS_TYPE_OBJECT_PATH, &objectPath,
                               DBUS_TYPE_UINT32, &passkey,
                               DBUS_TYPE_INVALID)) {
      errorStr.AssignLiteral("Invalid arguments: RequestConfirmation()");
      goto handle_error;
    }

    parameters.AppendElement(
      BluetoothNamedValue(NS_LITERAL_STRING("path"),
                          NS_ConvertUTF8toUTF16(objectPath)));
    parameters.AppendElement(
      BluetoothNamedValue(NS_LITERAL_STRING("method"),
                          NS_LITERAL_STRING("confirmation")));
    parameters.AppendElement(
      BluetoothNamedValue(NS_LITERAL_STRING("passkey"), passkey));

    v = parameters;
    isPairingReq = true;
  } else if (dbus_message_is_method_call(msg, DBUS_AGENT_IFACE,
                                         "RequestPinCode")) {
    
    
    
    if (!dbus_message_get_args(msg, NULL,
                               DBUS_TYPE_OBJECT_PATH, &objectPath,
                               DBUS_TYPE_INVALID)) {
      errorStr.AssignLiteral("Invalid arguments for RequestPinCode() method");
      goto handle_error;
    }

    parameters.AppendElement(
      BluetoothNamedValue(NS_LITERAL_STRING("path"),
                          NS_ConvertUTF8toUTF16(objectPath)));
    parameters.AppendElement(
      BluetoothNamedValue(NS_LITERAL_STRING("method"),
                          NS_LITERAL_STRING("pincode")));

    v = parameters;
    isPairingReq = true;
  } else if (dbus_message_is_method_call(msg, DBUS_AGENT_IFACE,
                                         "RequestPasskey")) {
    
    
    
    if (!dbus_message_get_args(msg, NULL,
                               DBUS_TYPE_OBJECT_PATH, &objectPath,
                               DBUS_TYPE_INVALID)) {
      errorStr.AssignLiteral("Invalid arguments for RequestPasskey() method");
      goto handle_error;
    }

    parameters.AppendElement(BluetoothNamedValue(
                               NS_LITERAL_STRING("path"),
                               NS_ConvertUTF8toUTF16(objectPath)));
    parameters.AppendElement(BluetoothNamedValue(
                               NS_LITERAL_STRING("method"),
                               NS_LITERAL_STRING("passkey")));

    v = parameters;
    isPairingReq = true;
  } else if (dbus_message_is_method_call(msg, DBUS_AGENT_IFACE, "Release")) {
    
    
    
    
    DBusMessage *reply = dbus_message_new_method_return(msg);

    if (!reply) {
      errorStr.AssignLiteral("Memory can't be allocated for the message.");
      goto handle_error;
    }

    dbus_connection_send(conn, reply, NULL);
    dbus_message_unref(reply);

    
    return DBUS_HANDLER_RESULT_HANDLED;
  } else {
#ifdef DEBUG
    BT_WARNING("agent handler %s: Unhandled event. Ignore.", __FUNCTION__);
#endif
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }

  if (!errorStr.IsEmpty()) {
    BT_WARNING(NS_ConvertUTF16toUTF8(errorStr).get());
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }

  
  signal.value() = v;

  if (isPairingReq) {
    sPairingReqTable.Put(
      GetAddressFromObjectPath(NS_ConvertUTF8toUTF16(objectPath)), msg);

    
    
    dbus_message_ref(msg);

    AppendDeviceName(signal);
  } else {
    NS_DispatchToMainThread(new DistributeBluetoothSignalTask(signal));
  }

  return DBUS_HANDLER_RESULT_HANDLED;

handle_error:
  BT_WARNING(NS_ConvertUTF16toUTF8(errorStr).get());
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

class RegisterAgentReplyHandler : public DBusReplyHandler
{
public:
  RegisterAgentReplyHandler(const DBusObjectPathVTable* aAgentVTable)
    : mAgentVTable(aAgentVTable)
  {
    MOZ_ASSERT(aAgentVTable);
  }

  void Handle(DBusMessage* aReply)
  {
    MOZ_ASSERT(!NS_IsMainThread()); 

    if (!aReply || (dbus_message_get_type(aReply) == DBUS_MESSAGE_TYPE_ERROR)) {
      return;
    }

    nsRefPtr<RawDBusConnection> threadConnection = gThreadConnection;

    if (!threadConnection.get()) {
      BT_WARNING("%s: DBus connection has been closed.", __FUNCTION__);
      return;
    }

    
    
    
    if (!dbus_connection_register_object_path(threadConnection->GetConnection(),
                                              KEY_REMOTE_AGENT,
                                              mAgentVTable,
                                              NULL)) {
      BT_WARNING("%s: Can't register object path %s for remote device agent!",
                 __FUNCTION__, KEY_REMOTE_AGENT);
      return;
    }

    NS_DispatchToMainThread(new PrepareProfileManagersRunnable());
  }

private:
  const DBusObjectPathVTable* mAgentVTable;
};

class AddReservedServiceRecordsReplyHandler : public DBusReplyHandler
{
public:
  void Handle(DBusMessage* aReply)
  {
    static const DBusObjectPathVTable sAgentVTable = {
      NULL, AgentEventFilter, NULL, NULL, NULL, NULL
    };

    MOZ_ASSERT(!NS_IsMainThread()); 

    if (!aReply || (dbus_message_get_type(aReply) == DBUS_MESSAGE_TYPE_ERROR)) {
      return;
    }

    
    
    nsTArray<uint32_t> handles;

    ExtractHandles(aReply, handles);

    if(!RegisterAgent(&sAgentVTable)) {
      NS_WARNING("Failed to register agent");
    }
  }

private:
  void ExtractHandles(DBusMessage *aMessage, nsTArray<uint32_t>& aOutHandles)
  {
    DBusError error;
    int length;
    uint32_t* handles = nullptr;

    dbus_error_init(&error);

    bool success = dbus_message_get_args(aMessage, &error,
                                         DBUS_TYPE_ARRAY,
                                         DBUS_TYPE_UINT32,
                                         &handles, &length,
                                         DBUS_TYPE_INVALID);
    if (success != TRUE) {
      LOG_AND_FREE_DBUS_ERROR(&error);
      return;
    }

    if (!handles) {
      BT_WARNING("Null array in extract_handles");
      return;
    }

    for (int i = 0; i < length; ++i) {
      aOutHandles.AppendElement(handles[i]);
    }
  }

  bool RegisterAgent(const DBusObjectPathVTable* aAgentVTable)
  {
    const char* agentPath = KEY_LOCAL_AGENT;
    const char* capabilities = B2G_AGENT_CAPABILITIES;

    nsRefPtr<RawDBusConnection> threadConnection = gThreadConnection;

    if (!threadConnection.get()) {
      BT_WARNING("%s: DBus connection has been closed.", __FUNCTION__);
      return false;
    }

    
    
    
    
    
    
    if (!dbus_connection_register_object_path(threadConnection->GetConnection(),
                                              KEY_LOCAL_AGENT,
                                              aAgentVTable,
                                              NULL)) {
      BT_WARNING("%s: Can't register object path %s for agent!",
                 __FUNCTION__, KEY_LOCAL_AGENT);
      return false;
    }

    nsRefPtr<RegisterAgentReplyHandler> handler =
      new RegisterAgentReplyHandler(aAgentVTable);
    MOZ_ASSERT(handler.get());

    bool success = dbus_func_args_async(threadConnection->GetConnection(), -1,
                                        RegisterAgentReplyHandler::Callback,
                                        handler.get(),
                                        NS_ConvertUTF16toUTF8(sAdapterPath).get(),
                                        DBUS_ADAPTER_IFACE, "RegisterAgent",
                                        DBUS_TYPE_OBJECT_PATH, &agentPath,
                                        DBUS_TYPE_STRING, &capabilities,
                                        DBUS_TYPE_INVALID);
    NS_ENSURE_TRUE(success, false);

    handler.forget();

    return true;
  }
};

class PrepareAdapterRunnable : public nsRunnable
{
public:
  PrepareAdapterRunnable(const nsAString& aAdapterPath)
    : mAdapterPath(aAdapterPath)
  { }

  NS_IMETHOD Run()
  {
    static const dbus_uint32_t sServices[] = {
      BluetoothServiceClass::HANDSFREE_AG,
      BluetoothServiceClass::HEADSET_AG,
      BluetoothServiceClass::OBJECT_PUSH
    };

    MOZ_ASSERT(NS_IsMainThread());

    nsRefPtr<RawDBusConnection> threadConnection = gThreadConnection;

    if (!threadConnection.get()) {
      BT_WARNING("%s: DBus connection has been closed.", __FUNCTION__);
      return NS_ERROR_FAILURE;
    }

    sAdapterPath = mAdapterPath;
    sAuthorizedServiceClass.AppendElement(BluetoothServiceClass::A2DP);
    sAuthorizedServiceClass.AppendElement(BluetoothServiceClass::HID);

    nsRefPtr<DBusReplyHandler> handler =
      new AddReservedServiceRecordsReplyHandler();

    const dbus_uint32_t* services = sServices;

    bool success = dbus_func_args_async(threadConnection->GetConnection(), -1,
                                        DBusReplyHandler::Callback, handler.get(),
                                        NS_ConvertUTF16toUTF8(sAdapterPath).get(),
                                        DBUS_ADAPTER_IFACE, "AddReservedServiceRecords",
                                        DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32,
                                        &services, NS_ARRAY_LENGTH(sServices),
                                        DBUS_TYPE_INVALID);
    NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

    handler.forget();

    return NS_OK;
  }

private:
  nsString mAdapterPath;
};

class SendPlayStatusTask : public nsRunnable
{
public:
  SendPlayStatusTask()
  {
    MOZ_ASSERT(!NS_IsMainThread());
  }

  nsresult Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    BluetoothA2dpManager* a2dp = BluetoothA2dpManager::Get();
    NS_ENSURE_TRUE(a2dp, NS_ERROR_FAILURE);

    BluetoothService* bs = BluetoothService::Get();
    NS_ENSURE_TRUE(bs, NS_ERROR_FAILURE);

    bs->UpdatePlayStatus(a2dp->GetDuration(),
                         a2dp->GetPosition(),
                         a2dp->GetPlayStatus());
    return NS_OK;
  }
};



static DBusHandlerResult
EventFilter(DBusConnection* aConn, DBusMessage* aMsg, void* aData)
{
  MOZ_ASSERT(!NS_IsMainThread(), "Shouldn't be called from Main Thread!");

  if (dbus_message_get_type(aMsg) != DBUS_MESSAGE_TYPE_SIGNAL) {
    BT_WARNING("%s: event handler not interested in %s (not a signal).\n",
        __FUNCTION__, dbus_message_get_member(aMsg));
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }

  if (dbus_message_get_path(aMsg) == NULL) {
    BT_WARNING("DBusMessage %s has no bluetooth destination, ignoring\n",
               dbus_message_get_member(aMsg));
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }

  DBusError err;
  dbus_error_init(&err);

  nsAutoString signalPath;
  nsAutoString signalName;
  nsAutoString signalInterface;

  BT_LOG("%s: %s, %s, %s", __FUNCTION__,
                          dbus_message_get_interface(aMsg),
                          dbus_message_get_path(aMsg),
                          dbus_message_get_member(aMsg));

  signalInterface = NS_ConvertUTF8toUTF16(dbus_message_get_interface(aMsg));
  signalPath = NS_ConvertUTF8toUTF16(dbus_message_get_path(aMsg));
  signalName = NS_ConvertUTF8toUTF16(dbus_message_get_member(aMsg));
  nsString errorStr;
  BluetoothValue v;

  
  
  
  if (signalInterface.EqualsLiteral(DBUS_MANAGER_IFACE)) {
    signalPath.AssignLiteral(KEY_MANAGER);
  } else if (signalInterface.EqualsLiteral(DBUS_ADAPTER_IFACE)) {
    signalPath.AssignLiteral(KEY_ADAPTER);
  } else if (signalInterface.EqualsLiteral(DBUS_DEVICE_IFACE)){
    signalPath = GetAddressFromObjectPath(signalPath);
  }

  if (dbus_message_is_signal(aMsg, DBUS_ADAPTER_IFACE, "DeviceFound")) {
    DBusMessageIter iter;

    if (!dbus_message_iter_init(aMsg, &iter)) {
      NS_WARNING("Can't create iterator!");
      return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    const char* addr;
    dbus_message_iter_get_basic(&iter, &addr);

    if (!dbus_message_iter_next(&iter)) {
      errorStr.AssignLiteral("Unexpected message struct in msg DeviceFound");
    } else {
      ParseProperties(&iter,
                      v,
                      errorStr,
                      sDeviceProperties,
                      ArrayLength(sDeviceProperties));

      InfallibleTArray<BluetoothNamedValue>& properties =
        v.get_ArrayOfBluetoothNamedValue();

      
      
      
      
      
      nsAutoString address = NS_ConvertUTF8toUTF16(addr);
      properties.AppendElement(
        BluetoothNamedValue(NS_LITERAL_STRING("Address"), address));
      properties.AppendElement(
        BluetoothNamedValue(NS_LITERAL_STRING("Path"),
                            GetObjectPathFromAddress(signalPath, address)));

      if (!ContainsIcon(properties)) {
        for (uint8_t i = 0; i < properties.Length(); i++) {
          
          
          
          
          
          if (properties[i].name().EqualsLiteral("Class")) {
            if (HasAudioService(properties[i].value().get_uint32_t())) {
              v.get_ArrayOfBluetoothNamedValue().AppendElement(
                BluetoothNamedValue(NS_LITERAL_STRING("Icon"),
                                    NS_LITERAL_STRING("audio-card")));
            }
            break;
          }
        }
      }
    }
  } else if (dbus_message_is_signal(aMsg, DBUS_ADAPTER_IFACE,
                                    "DeviceDisappeared")) {
    const char* str;
    if (!dbus_message_get_args(aMsg, &err,
                               DBUS_TYPE_STRING, &str,
                               DBUS_TYPE_INVALID)) {
      LOG_AND_FREE_DBUS_ERROR_WITH_MSG(&err, aMsg);
      errorStr.AssignLiteral("Cannot parse device address!");
    } else {
      v = NS_ConvertUTF8toUTF16(str);
    }
  } else if (dbus_message_is_signal(aMsg, DBUS_ADAPTER_IFACE,
                                    "DeviceCreated")) {
    const char* str;
    if (!dbus_message_get_args(aMsg, &err,
                               DBUS_TYPE_OBJECT_PATH, &str,
                               DBUS_TYPE_INVALID)) {
      LOG_AND_FREE_DBUS_ERROR_WITH_MSG(&err, aMsg);
      errorStr.AssignLiteral("Cannot parse device path!");
    } else {
      v = NS_ConvertUTF8toUTF16(str);
    }
  } else if (dbus_message_is_signal(aMsg, DBUS_ADAPTER_IFACE,
                                    "DeviceRemoved")) {
    const char* str;
    if (!dbus_message_get_args(aMsg, &err,
                               DBUS_TYPE_OBJECT_PATH, &str,
                               DBUS_TYPE_INVALID)) {
      LOG_AND_FREE_DBUS_ERROR_WITH_MSG(&err, aMsg);
      errorStr.AssignLiteral("Cannot parse device path!");
    } else {
      v = NS_ConvertUTF8toUTF16(str);
    }
  } else if (dbus_message_is_signal(aMsg, DBUS_ADAPTER_IFACE,
                                    "PropertyChanged")) {
    ParsePropertyChange(aMsg,
                        v,
                        errorStr,
                        sAdapterProperties,
                        ArrayLength(sAdapterProperties));
  } else if (dbus_message_is_signal(aMsg, DBUS_DEVICE_IFACE,
                                    "PropertyChanged")) {
    ParsePropertyChange(aMsg,
                        v,
                        errorStr,
                        sDeviceProperties,
                        ArrayLength(sDeviceProperties));

    BluetoothNamedValue& property = v.get_ArrayOfBluetoothNamedValue()[0];
    if (property.name().EqualsLiteral("Paired")) {
      
      
      BluetoothValue newValue(v);
      ToLowerCase(newValue.get_ArrayOfBluetoothNamedValue()[0].name());
      BluetoothSignal signal(NS_LITERAL_STRING(PAIRED_STATUS_CHANGED_ID),
                             NS_LITERAL_STRING(KEY_LOCAL_AGENT),
                             newValue);
      NS_DispatchToMainThread(new DistributeBluetoothSignalTask(signal));

      
      bool status = property.value();
      InfallibleTArray<BluetoothNamedValue> parameters;
      parameters.AppendElement(
        BluetoothNamedValue(NS_LITERAL_STRING("address"), signalPath));
      parameters.AppendElement(
        BluetoothNamedValue(NS_LITERAL_STRING("status"), status));
      signal.path() = NS_LITERAL_STRING(KEY_ADAPTER);
      signal.value() = parameters;
      NS_DispatchToMainThread(new DistributeBluetoothSignalTask(signal));
    } else if (property.name().EqualsLiteral("Connected")) {
      MonitorAutoLock lock(sStopBluetoothMonitor);

      if (property.value().get_bool()) {
        ++sConnectedDeviceCount;
      } else {
        MOZ_ASSERT(sConnectedDeviceCount > 0);

        --sConnectedDeviceCount;
        if (sConnectedDeviceCount == 0) {
          lock.Notify();
        }
      }
    }
  } else if (dbus_message_is_signal(aMsg, DBUS_MANAGER_IFACE, "AdapterAdded")) {
    const char* str;
    if (!dbus_message_get_args(aMsg, &err,
                               DBUS_TYPE_OBJECT_PATH, &str,
                               DBUS_TYPE_INVALID)) {
      LOG_AND_FREE_DBUS_ERROR_WITH_MSG(&err, aMsg);
      errorStr.AssignLiteral("Cannot parse manager path!");
    } else {
      v = NS_ConvertUTF8toUTF16(str);
      NS_DispatchToMainThread(new PrepareAdapterRunnable(v.get_nsString()));
    }
  } else if (dbus_message_is_signal(aMsg, DBUS_MANAGER_IFACE,
                                    "PropertyChanged")) {
    ParsePropertyChange(aMsg,
                        v,
                        errorStr,
                        sManagerProperties,
                        ArrayLength(sManagerProperties));
  } else if (dbus_message_is_signal(aMsg, DBUS_SINK_IFACE,
                                    "PropertyChanged")) {
    ParsePropertyChange(aMsg,
                        v,
                        errorStr,
                        sSinkProperties,
                        ArrayLength(sSinkProperties));
  } else if (dbus_message_is_signal(aMsg, DBUS_CTL_IFACE, "GetPlayStatus")) {
    NS_DispatchToMainThread(new SendPlayStatusTask());
    return DBUS_HANDLER_RESULT_HANDLED;
  } else if (dbus_message_is_signal(aMsg, DBUS_CTL_IFACE, "PropertyChanged")) {
    ParsePropertyChange(aMsg,
                        v,
                        errorStr,
                        sControlProperties,
                        ArrayLength(sControlProperties));
  } else if (dbus_message_is_signal(aMsg, DBUS_INPUT_IFACE,
                                    "PropertyChanged")) {
    ParsePropertyChange(aMsg,
                        v,
                        errorStr,
                        sInputProperties,
                        ArrayLength(sInputProperties));
  } else {
    errorStr = NS_ConvertUTF8toUTF16(dbus_message_get_member(aMsg));
    errorStr.AppendLiteral(" Signal not handled!");
  }

  if (!errorStr.IsEmpty()) {
    NS_WARNING(NS_ConvertUTF16toUTF8(errorStr).get());
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }

  BluetoothSignal signal(signalName, signalPath, v);
  nsRefPtr<nsRunnable> task;
  if (signalInterface.EqualsLiteral(DBUS_SINK_IFACE)) {
    task = new SinkPropertyChangedHandler(signal);
  } else if (signalInterface.EqualsLiteral(DBUS_CTL_IFACE)) {
    task = new ControlPropertyChangedHandler(signal);
  } else if (signalInterface.EqualsLiteral(DBUS_INPUT_IFACE)) {
    task = new InputPropertyChangedHandler(signal);
  } else {
    task = new DistributeBluetoothSignalTask(signal);
  }

  NS_DispatchToMainThread(task);

  return DBUS_HANDLER_RESULT_HANDLED;
}

static bool
GetDefaultAdapterPath(BluetoothValue& aValue, nsString& aError)
{
  
  MOZ_ASSERT(!NS_IsMainThread());

  DBusError err;
  dbus_error_init(&err);

  DBusMessage* msg = dbus_func_args_timeout(gThreadConnection->GetConnection(),
                                            1000,
                                            &err,
                                            "/",
                                            DBUS_MANAGER_IFACE,
                                            "DefaultAdapter",
                                            DBUS_TYPE_INVALID);
  NS_ENSURE_TRUE(msg, false);

  UnpackObjectPathMessage(msg, &err, aValue, aError);

  dbus_message_unref(msg);

  return aError.IsEmpty();
}

bool
BluetoothDBusService::IsReady()
{
  if (!IsEnabled() || !mConnection || !gThreadConnection || IsToggling()) {
    NS_WARNING("Bluetooth service is not ready yet!");
    return false;
  }
  return true;
}

nsresult
BluetoothDBusService::StartInternal()
{
  
  MOZ_ASSERT(!NS_IsMainThread());

  if (!StartDBus()) {
    NS_WARNING("Cannot start DBus thread!");
    return NS_ERROR_FAILURE;
  }

  if (mConnection) {
    return NS_OK;
  }

  if (NS_FAILED(EstablishDBusConnection())) {
    NS_WARNING("Cannot start Main Thread DBus connection!");
    StopDBus();
    return NS_ERROR_FAILURE;
  }

  gThreadConnection = new RawDBusConnection();

  if (NS_FAILED(gThreadConnection->EstablishDBusConnection())) {
    NS_WARNING("Cannot start Sync Thread DBus connection!");
    StopDBus();
    return NS_ERROR_FAILURE;
  }

  DBusError err;
  dbus_error_init(&err);

  
  
  
  
  for (uint32_t i = 0; i < ArrayLength(sBluetoothDBusSignals); ++i) {
    dbus_bus_add_match(mConnection,
                       sBluetoothDBusSignals[i],
                       &err);
    if (dbus_error_is_set(&err)) {
      LOG_AND_FREE_DBUS_ERROR(&err);
    }
  }

  
  if (!dbus_connection_add_filter(mConnection, EventFilter,
                                  NULL, NULL)) {
    NS_WARNING("Cannot create DBus Event Filter for DBus Thread!");
    return NS_ERROR_FAILURE;
  }

  if (!sPairingReqTable.IsInitialized()) {
    sPairingReqTable.Init();
  }

  BluetoothValue v;
  nsAutoString replyError;
  if (!GetDefaultAdapterPath(v, replyError)) {
    
    
  } else {
    
    nsRefPtr<PrepareAdapterRunnable> b = new PrepareAdapterRunnable(v.get_nsString());
    if (NS_FAILED(NS_DispatchToMainThread(b))) {
      NS_WARNING("Failed to dispatch to main thread!");
    }
  }

  return NS_OK;
}

PLDHashOperator
UnrefDBusMessages(const nsAString& key, DBusMessage* value, void* arg)
{
  dbus_message_unref(value);

  return PL_DHASH_NEXT;
}

nsresult
BluetoothDBusService::StopInternal()
{
  
  MOZ_ASSERT(!NS_IsMainThread());

  {
    MonitorAutoLock lock(sStopBluetoothMonitor);
    if (sConnectedDeviceCount > 0) {
      lock.Wait(PR_SecondsToInterval(TIMEOUT_FORCE_TO_DISABLE_BT));
    }
  }

  if (!mConnection) {
    StopDBus();
    return NS_OK;
  }

  DBusError err;
  dbus_error_init(&err);
  for (uint32_t i = 0; i < ArrayLength(sBluetoothDBusSignals); ++i) {
    dbus_bus_remove_match(mConnection,
                          sBluetoothDBusSignals[i],
                          &err);
    if (dbus_error_is_set(&err)) {
      LOG_AND_FREE_DBUS_ERROR(&err);
    }
  }

  dbus_connection_remove_filter(mConnection, EventFilter, nullptr);

  if (!dbus_connection_unregister_object_path(gThreadConnection->GetConnection(),
                                              KEY_LOCAL_AGENT)) {
    BT_WARNING("%s: Can't unregister object path %s for agent!",
        __FUNCTION__, KEY_LOCAL_AGENT);
  }

  if (!dbus_connection_unregister_object_path(gThreadConnection->GetConnection(),
                                              KEY_REMOTE_AGENT)) {
    BT_WARNING("%s: Can't unregister object path %s for agent!",
        __FUNCTION__, KEY_REMOTE_AGENT);
  }

  mConnection = nullptr;
  gThreadConnection = nullptr;

  
  sPairingReqTable.EnumerateRead(UnrefDBusMessages, nullptr);
  sPairingReqTable.Clear();

  sIsPairing = 0;
  sConnectedDeviceCount = 0;

  sAuthorizedServiceClass.Clear();

  StopDBus();
  return NS_OK;
}

bool
BluetoothDBusService::IsEnabledInternal()
{
  return mEnabled;
}

class DefaultAdapterPathReplyHandler : public DBusReplyHandler
{
public:
  DefaultAdapterPathReplyHandler(BluetoothReplyRunnable* aRunnable)
    : mRunnable(aRunnable)
  {
    MOZ_ASSERT(mRunnable);
  }

  void Handle(DBusMessage* aReply) MOZ_OVERRIDE
  {
    MOZ_ASSERT(!NS_IsMainThread()); 

    if (!aReply || (dbus_message_get_type(aReply) == DBUS_MESSAGE_TYPE_ERROR)) {
      const char* errStr = "Timeout in DefaultAdapterPathReplyHandler";
      if (aReply) {
        errStr = dbus_message_get_error_name(aReply);
        if (!errStr) {
          errStr = "Bluetooth DBus Error";
        }
      }
      DispatchBluetoothReply(mRunnable, BluetoothValue(),
                             NS_ConvertUTF8toUTF16(errStr));
      return;
    }

    bool success;
    nsAutoString replyError;

    if (mAdapterPath.IsEmpty()) {
      success = HandleDefaultAdapterPathReply(aReply, replyError);
    } else {
      success = HandleGetPropertiesReply(aReply, replyError);
    }

    if (!success) {
      DispatchBluetoothReply(mRunnable, BluetoothValue(), replyError);
    }
  }

protected:
  bool HandleDefaultAdapterPathReply(DBusMessage* aReply,
                                     nsAString& aReplyError)
  {
    BluetoothValue value;
    DBusError error;
    dbus_error_init(&error);

    MOZ_ASSERT(!NS_IsMainThread()); 

    UnpackObjectPathMessage(aReply, &error, value, aReplyError);

    if (!aReplyError.IsEmpty()) {
      return false;
    }

    mAdapterPath = value.get_nsString();

    
    nsRefPtr<DefaultAdapterPathReplyHandler> handler = this;

    nsRefPtr<RawDBusConnection> threadConnection = gThreadConnection;

    if (!threadConnection.get()) {
      aReplyError = NS_LITERAL_STRING("DBus connection has been closed.");
      return false;
    }

    bool success = dbus_func_args_async(threadConnection->GetConnection(), 1000,
                                        DefaultAdapterPathReplyHandler::Callback,
                                        handler.get(),
                                        NS_ConvertUTF16toUTF8(mAdapterPath).get(),
                                        DBUS_ADAPTER_IFACE, "GetProperties",
                                        DBUS_TYPE_INVALID);
    if (!success) {
      aReplyError = NS_LITERAL_STRING("dbus_func_args_async failed");
      return false;
    }

    handler.forget();

    return true;
  }

  bool HandleGetPropertiesReply(DBusMessage* aReply,
                                nsAutoString& aReplyError)
  {
    BluetoothValue value;
    DBusError error;
    dbus_error_init(&error);

    MOZ_ASSERT(!NS_IsMainThread()); 

    bool success = UnpackPropertiesMessage(aReply, &error, value,
                                           DBUS_ADAPTER_IFACE);
    if (!success) {
      aReplyError = NS_ConvertUTF8toUTF16(error.message);
      return false;
    }

    
    value.get_ArrayOfBluetoothNamedValue().AppendElement(
      BluetoothNamedValue(NS_LITERAL_STRING("Path"), mAdapterPath));

    
    DispatchBluetoothReply(mRunnable, value, aReplyError);

    return true;
  }

private:
  nsRefPtr<BluetoothReplyRunnable> mRunnable;
  nsString mAdapterPath;
};

nsresult
BluetoothDBusService::GetDefaultAdapterPathInternal(
                                              BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!IsReady()) {
    NS_NAMED_LITERAL_STRING(errorStr, "Bluetooth service is not ready yet!");
    DispatchBluetoothReply(aRunnable, BluetoothValue(), errorStr);
    return NS_OK;
  }

  nsRefPtr<DefaultAdapterPathReplyHandler> handler =
    new DefaultAdapterPathReplyHandler(aRunnable);
  bool success = dbus_func_args_async(mConnection, 1000,
                                      DefaultAdapterPathReplyHandler::Callback,
                                      handler.get(),
                                      "/",
                                      DBUS_MANAGER_IFACE, "DefaultAdapter",
                                      DBUS_TYPE_INVALID);
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  handler.forget();

  return NS_OK;
}

static void
OnSendDiscoveryMessageReply(DBusMessage *aReply, void *aData)
{
  MOZ_ASSERT(!NS_IsMainThread());

  nsAutoString errorStr;

  if (!aReply) {
    errorStr.AssignLiteral("SendDiscovery failed");
  }

  nsRefPtr<BluetoothReplyRunnable> runnable =
    dont_AddRef<BluetoothReplyRunnable>(static_cast<BluetoothReplyRunnable*>(aData));

  DispatchBluetoothReply(runnable.get(), BluetoothValue(true), errorStr);
}

nsresult
BluetoothDBusService::SendDiscoveryMessage(const char* aMessageName,
                                           BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!IsReady()) {
    NS_NAMED_LITERAL_STRING(errorStr, "Bluetooth service is not ready yet!");
    DispatchBluetoothReply(aRunnable, BluetoothValue(), errorStr);
    return NS_OK;
  }

  nsRefPtr<BluetoothReplyRunnable> runnable(aRunnable);

  bool success = dbus_func_args_async(mConnection, -1,
                                      OnSendDiscoveryMessageReply,
                                      static_cast<void*>(aRunnable),
                                      NS_ConvertUTF16toUTF8(sAdapterPath).get(),
                                      DBUS_ADAPTER_IFACE, aMessageName,
                                      DBUS_TYPE_INVALID);
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  runnable.forget();

  return NS_OK;
}

nsresult
BluetoothDBusService::SendInputMessage(const nsAString& aDeviceAddress,
                                       const nsAString& aMessage,
                                       BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mConnection);
  MOZ_ASSERT(aMessage.EqualsLiteral("Connect") ||
             aMessage.EqualsLiteral("Disconnect"));

  NS_ENSURE_TRUE(IsReady(), NS_ERROR_FAILURE);

  DBusCallback callback;
  if (aMessage.EqualsLiteral("Connect")) {
    callback = GetVoidCallback;
  } else if (aMessage.EqualsLiteral("Disconnect")) {
    callback = InputDisconnectCallback;
  }

  nsRefPtr<BluetoothReplyRunnable> runnable(aRunnable);

  nsString objectPath = GetObjectPathFromAddress(sAdapterPath, aDeviceAddress);
  bool ret = dbus_func_args_async(mConnection,
                                  -1,
                                  callback,
                                  static_cast<void*>(runnable.get()),
                                  NS_ConvertUTF16toUTF8(objectPath).get(),
                                  DBUS_INPUT_IFACE,
                                  NS_ConvertUTF16toUTF8(aMessage).get(),
                                  DBUS_TYPE_INVALID);
  NS_ENSURE_TRUE(ret, NS_ERROR_FAILURE);

  runnable.forget();

  return NS_OK;
}

nsresult
BluetoothDBusService::SendSinkMessage(const nsAString& aDeviceAddress,
                                      const nsAString& aMessage)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mConnection);
  MOZ_ASSERT(IsEnabled());

  DBusCallback callback;
  if (aMessage.EqualsLiteral("Connect")) {
    callback = SinkConnectCallback;
  } else if (aMessage.EqualsLiteral("Disconnect")) {
    callback = SinkDisconnectCallback;
  } else {
    BT_WARNING("Unknown sink message");
    return NS_ERROR_FAILURE;
  }

  nsString objectPath = GetObjectPathFromAddress(sAdapterPath, aDeviceAddress);
  bool ret = dbus_func_args_async(mConnection,
                                  -1,
                                  callback,
                                  nullptr,
                                  NS_ConvertUTF16toUTF8(objectPath).get(),
                                  DBUS_SINK_IFACE,
                                  NS_ConvertUTF16toUTF8(aMessage).get(),
                                  DBUS_TYPE_INVALID);
  NS_ENSURE_TRUE(ret, NS_ERROR_FAILURE);

  return NS_OK;
}

nsresult
BluetoothDBusService::StopDiscoveryInternal(BluetoothReplyRunnable* aRunnable)
{
  return SendDiscoveryMessage("StopDiscovery", aRunnable);
}

nsresult
BluetoothDBusService::StartDiscoveryInternal(BluetoothReplyRunnable* aRunnable)
{
  return SendDiscoveryMessage("StartDiscovery", aRunnable);
}

class BluetoothArrayOfDevicePropertiesReplyHandler : public DBusReplyHandler
{
public:
  BluetoothArrayOfDevicePropertiesReplyHandler(
    const nsTArray<nsString>& aDeviceAddresses,
    const FilterFunc aFilterFunc, BluetoothReplyRunnable* aRunnable)
    : mDeviceAddresses(aDeviceAddresses)
    , mProcessedDeviceAddresses(0)
    , mFilterFunc(aFilterFunc)
    , mRunnable(aRunnable)
    , mValues(InfallibleTArray<BluetoothNamedValue>())
  {
    MOZ_ASSERT(mRunnable);
  }

  void Handle(DBusMessage* aReply) MOZ_OVERRIDE
  {
    MOZ_ASSERT(!NS_IsMainThread()); 
    MOZ_ASSERT(!mObjectPath.IsEmpty());
    MOZ_ASSERT(mProcessedDeviceAddresses < mDeviceAddresses.Length());

    const nsTArray<nsString>::index_type i = mProcessedDeviceAddresses++;

    if (!aReply ||
        (dbus_message_get_type(aReply) == DBUS_MESSAGE_TYPE_ERROR)) {
      BT_WARNING("Invalid DBus message");
      ProcessRemainingDeviceAddresses();
      return;
    }

    

    DBusError err;
    dbus_error_init(&err);

    BluetoothValue deviceProperties;

    bool success = UnpackPropertiesMessage(aReply, &err, deviceProperties,
                                           DBUS_DEVICE_IFACE);
    if (!success) {
      BT_WARNING("Failed to get device properties");
      ProcessRemainingDeviceAddresses();
      return;
    }

    InfallibleTArray<BluetoothNamedValue>& devicePropertiesArray =
      deviceProperties.get_ArrayOfBluetoothNamedValue();

    
    devicePropertiesArray.AppendElement(
      BluetoothNamedValue(NS_LITERAL_STRING("Path"), mObjectPath));

    
    
    
    
    
    if (!ContainsIcon(devicePropertiesArray)) {
      InfallibleTArray<BluetoothNamedValue>::size_type j;
      for (j = 0; j < devicePropertiesArray.Length(); ++j) {
        BluetoothNamedValue& deviceProperty = devicePropertiesArray[j];
        if (deviceProperty.name().EqualsLiteral("Class")) {
          if (HasAudioService(deviceProperty.value().get_uint32_t())) {
            devicePropertiesArray.AppendElement(
              BluetoothNamedValue(NS_LITERAL_STRING("Icon"),
                                  NS_LITERAL_STRING("audio-card")));
          }
          break;
        }
      }
    }

    if (mFilterFunc(deviceProperties)) {
      mValues.get_ArrayOfBluetoothNamedValue().AppendElement(
        BluetoothNamedValue(mDeviceAddresses[i], deviceProperties));
    }

    ProcessRemainingDeviceAddresses();
  }

  void ProcessRemainingDeviceAddresses()
  {
    if (mProcessedDeviceAddresses < mDeviceAddresses.Length()) {
      if (!SendNextGetProperties()) {
        DispatchBluetoothReply(mRunnable, BluetoothValue(),
                               NS_LITERAL_STRING(
                                 "SendNextGetProperties failed"));
      }
    } else {
      
      DispatchBluetoothReply(mRunnable, mValues, EmptyString());
    }
  }

protected:
  bool SendNextGetProperties()
  {
    MOZ_ASSERT(mProcessedDeviceAddresses < mDeviceAddresses.Length());

    
    mObjectPath = GetObjectPathFromAddress(sAdapterPath,
      mDeviceAddresses[mProcessedDeviceAddresses]);

    nsRefPtr<RawDBusConnection> threadConnection = gThreadConnection;

    if (!threadConnection.get()) {
      BT_WARNING("%s: DBus connection has been closed.", __FUNCTION__);
      return false;
    }

    nsRefPtr<BluetoothArrayOfDevicePropertiesReplyHandler> handler = this;

    bool success =
      dbus_func_args_async(threadConnection->GetConnection(), 1000,
        BluetoothArrayOfDevicePropertiesReplyHandler::Callback,
        handler.get(),
        NS_ConvertUTF16toUTF8(mObjectPath).get(),
        DBUS_DEVICE_IFACE, "GetProperties",
        DBUS_TYPE_INVALID);

    NS_ENSURE_TRUE(success, false);

    handler.forget();

    return true;
  }

private:
  nsString mObjectPath;
  const nsTArray<nsString> mDeviceAddresses;
  nsTArray<nsString>::size_type mProcessedDeviceAddresses;
  const FilterFunc mFilterFunc;
  nsRefPtr<BluetoothReplyRunnable> mRunnable;
  BluetoothValue mValues;
};

nsresult
BluetoothDBusService::GetConnectedDevicePropertiesInternal(uint16_t aProfileId,
                                              BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsAutoString errorStr;
  BluetoothValue values = InfallibleTArray<BluetoothNamedValue>();
  if (!IsReady()) {
    NS_NAMED_LITERAL_STRING(errorStr, "Bluetooth service is not ready yet!");
    DispatchBluetoothReply(aRunnable, BluetoothValue(), errorStr);
    return NS_OK;
  }

  nsTArray<nsString> deviceAddresses;
  BluetoothProfileManagerBase* profile;
  if (aProfileId == BluetoothServiceClass::HANDSFREE ||
      aProfileId == BluetoothServiceClass::HEADSET) {
    profile = BluetoothHfpManager::Get();
  } else if (aProfileId == BluetoothServiceClass::HID) {
    profile = BluetoothHidManager::Get();
  } else if (aProfileId == BluetoothServiceClass::OBJECT_PUSH) {
    profile = BluetoothOppManager::Get();
  } else {
    DispatchBluetoothReply(aRunnable, values,
                           NS_LITERAL_STRING(ERR_UNKNOWN_PROFILE));
    return NS_OK;
  }

  if (profile->IsConnected()) {
    nsString address;
    profile->GetAddress(address);
    deviceAddresses.AppendElement(address);
  }

  nsRefPtr<BluetoothReplyRunnable> runnable = aRunnable;
  nsRefPtr<BluetoothArrayOfDevicePropertiesReplyHandler> handler =
    new BluetoothArrayOfDevicePropertiesReplyHandler(deviceAddresses,
                                                     GetConnectedDevicesFilter,
                                                     runnable);
  handler->ProcessRemainingDeviceAddresses();

  return NS_OK;
}

nsresult
BluetoothDBusService::GetPairedDevicePropertiesInternal(
                                     const nsTArray<nsString>& aDeviceAddresses,
                                     BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!IsReady()) {
    NS_NAMED_LITERAL_STRING(errorStr, "Bluetooth service is not ready yet!");
    DispatchBluetoothReply(aRunnable, BluetoothValue(), errorStr);
    return NS_OK;
  }

  nsRefPtr<BluetoothReplyRunnable> runnable = aRunnable;
  nsRefPtr<BluetoothArrayOfDevicePropertiesReplyHandler> handler =
    new BluetoothArrayOfDevicePropertiesReplyHandler(aDeviceAddresses,
                                                     GetPairedDevicesFilter,
                                                     runnable);
  handler->ProcessRemainingDeviceAddresses();

  return NS_OK;
}

nsresult
BluetoothDBusService::SetProperty(BluetoothObjectType aType,
                                  const BluetoothNamedValue& aValue,
                                  BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!IsReady()) {
    NS_NAMED_LITERAL_STRING(errorStr, "Bluetooth service is not ready yet!");
    DispatchBluetoothReply(aRunnable, BluetoothValue(), errorStr);
    return NS_OK;
  }

  MOZ_ASSERT(aType < ArrayLength(sBluetoothDBusIfaces));
  const char* interface = sBluetoothDBusIfaces[aType];

  
  DBusMessage* msg = dbus_message_new_method_call(
                                      "org.bluez",
                                      NS_ConvertUTF16toUTF8(sAdapterPath).get(),
                                      interface,
                                      "SetProperty");

  if (!msg) {
    NS_WARNING("Could not allocate D-Bus message object!");
    return NS_ERROR_FAILURE;
  }

  nsCString intermediatePropName(NS_ConvertUTF16toUTF8(aValue.name()));
  const char* propName = intermediatePropName.get();
  if (!dbus_message_append_args(msg, DBUS_TYPE_STRING, &propName,
                                DBUS_TYPE_INVALID)) {
    NS_WARNING("Couldn't append arguments to dbus message!");
    return NS_ERROR_FAILURE;
  }

  int type;
  int tmp_int;
  void* val;
  nsCString str;
  if (aValue.value().type() == BluetoothValue::Tuint32_t) {
    tmp_int = aValue.value().get_uint32_t();
    val = &tmp_int;
    type = DBUS_TYPE_UINT32;
  } else if (aValue.value().type() == BluetoothValue::TnsString) {
    str = NS_ConvertUTF16toUTF8(aValue.value().get_nsString());
    const char* tempStr = str.get();
    val = &tempStr;
    type = DBUS_TYPE_STRING;
  } else if (aValue.value().type() == BluetoothValue::Tbool) {
    tmp_int = aValue.value().get_bool() ? 1 : 0;
    val = &(tmp_int);
    type = DBUS_TYPE_BOOLEAN;
  } else {
    NS_WARNING("Property type not handled!");
    dbus_message_unref(msg);
    return NS_ERROR_FAILURE;
  }

  DBusMessageIter value_iter, iter;
  dbus_message_iter_init_append(msg, &iter);
  char var_type[2] = {(char)type, '\0'};
  if (!dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT,
                                        var_type, &value_iter) ||
      !dbus_message_iter_append_basic(&value_iter, type, val) ||
      !dbus_message_iter_close_container(&iter, &value_iter)) {
    NS_WARNING("Could not append argument to method call!");
    dbus_message_unref(msg);
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<BluetoothReplyRunnable> runnable = aRunnable;

  
  if (!dbus_func_send_async(mConnection,
                            msg,
                            1000,
                            GetVoidCallback,
                            (void*)aRunnable)) {
    NS_WARNING("Could not start async function!");
    return NS_ERROR_FAILURE;
  }
  runnable.forget();
  return NS_OK;
}

bool
BluetoothDBusService::GetDevicePath(const nsAString& aAdapterPath,
                                    const nsAString& aDeviceAddress,
                                    nsAString& aDevicePath)
{
  aDevicePath = GetObjectPathFromAddress(aAdapterPath, aDeviceAddress);
  return true;
}


bool
BluetoothDBusService::RemoveReservedServicesInternal(
                                      const nsTArray<uint32_t>& aServiceHandles)
{
  MOZ_ASSERT(!NS_IsMainThread());

  int length = aServiceHandles.Length();
  if (length == 0) return false;

  const uint32_t* services = aServiceHandles.Elements();

  DBusMessage* reply =
    dbus_func_args(gThreadConnection->GetConnection(),
                   NS_ConvertUTF16toUTF8(sAdapterPath).get(),
                   DBUS_ADAPTER_IFACE, "RemoveReservedServiceRecords",
                   DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32,
                   &services, length, DBUS_TYPE_INVALID);

  if (!reply) return false;

  dbus_message_unref(reply);
  return true;
}

nsresult
BluetoothDBusService::CreatePairedDeviceInternal(
                                              const nsAString& aDeviceAddress,
                                              int aTimeout,
                                              BluetoothReplyRunnable* aRunnable)
{
  const char *capabilities = B2G_AGENT_CAPABILITIES;
  const char *deviceAgentPath = KEY_REMOTE_AGENT;

  nsCString tempDeviceAddress = NS_ConvertUTF16toUTF8(aDeviceAddress);
  const char *deviceAddress = tempDeviceAddress.get();

  











  sIsPairing++;

  nsRefPtr<BluetoothReplyRunnable> runnable = aRunnable;
  
  
  bool ret = dbus_func_args_async(mConnection,
                                  aTimeout,
                                  GetObjectPathCallback,
                                  (void*)runnable,
                                  NS_ConvertUTF16toUTF8(sAdapterPath).get(),
                                  DBUS_ADAPTER_IFACE,
                                  "CreatePairedDevice",
                                  DBUS_TYPE_STRING, &deviceAddress,
                                  DBUS_TYPE_OBJECT_PATH, &deviceAgentPath,
                                  DBUS_TYPE_STRING, &capabilities,
                                  DBUS_TYPE_INVALID);
  if (!ret) {
    NS_WARNING("Could not start async function!");
    return NS_ERROR_FAILURE;
  }

  runnable.forget();
  return NS_OK;
}

static void
OnRemoveDeviceReply(DBusMessage *aReply, void *aData)
{
  nsAutoString errorStr;

  if (!aReply) {
    errorStr.AssignLiteral("RemoveDevice failed");
  }

  nsRefPtr<BluetoothReplyRunnable> runnable =
    dont_AddRef<BluetoothReplyRunnable>(static_cast<BluetoothReplyRunnable*>(aData));

  DispatchBluetoothReply(runnable.get(), BluetoothValue(true), errorStr);
}

nsresult
BluetoothDBusService::RemoveDeviceInternal(const nsAString& aDeviceAddress,
                                           BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!IsReady()) {
    NS_NAMED_LITERAL_STRING(errorStr, "Bluetooth service is not ready yet!");
    DispatchBluetoothReply(aRunnable, BluetoothValue(), errorStr);
    return NS_OK;
  }

  nsCString deviceObjectPath =
    NS_ConvertUTF16toUTF8(GetObjectPathFromAddress(sAdapterPath,
                                                   aDeviceAddress));
  const char* cstrDeviceObjectPath = deviceObjectPath.get();

  nsRefPtr<BluetoothReplyRunnable> runnable(aRunnable);

  bool success = dbus_func_args_async(mConnection, -1,
                                      OnRemoveDeviceReply,
                                      static_cast<void*>(runnable.get()),
                                      NS_ConvertUTF16toUTF8(sAdapterPath).get(),
                                      DBUS_ADAPTER_IFACE, "RemoveDevice",
                                      DBUS_TYPE_OBJECT_PATH, &cstrDeviceObjectPath,
                                      DBUS_TYPE_INVALID);
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  runnable.forget();

  return NS_OK;
}

bool
BluetoothDBusService::SetPinCodeInternal(const nsAString& aDeviceAddress,
                                         const nsAString& aPinCode,
                                         BluetoothReplyRunnable* aRunnable)
{
  nsAutoString errorStr;
  BluetoothValue v = true;
  DBusMessage *msg;
  if (!sPairingReqTable.Get(aDeviceAddress, &msg)) {
    BT_WARNING("%s: Couldn't get original request message.", __FUNCTION__);
    errorStr.AssignLiteral("Couldn't get original request message.");
    DispatchBluetoothReply(aRunnable, v, errorStr);
    return false;
  }

  DBusMessage *reply = dbus_message_new_method_return(msg);

  if (!reply) {
    BT_WARNING("%s: Memory can't be allocated for the message.", __FUNCTION__);
    dbus_message_unref(msg);
    errorStr.AssignLiteral("Memory can't be allocated for the message.");
    DispatchBluetoothReply(aRunnable, v, errorStr);
    return false;
  }

  bool result;

  nsCString tempPinCode = NS_ConvertUTF16toUTF8(aPinCode);
  const char* pinCode = tempPinCode.get();

  if (!dbus_message_append_args(reply,
                                DBUS_TYPE_STRING, &pinCode,
                                DBUS_TYPE_INVALID)) {
    BT_WARNING("%s: Couldn't append arguments to dbus message.", __FUNCTION__);
    errorStr.AssignLiteral("Couldn't append arguments to dbus message.");
    result = false;
  } else {
    result = dbus_func_send(mConnection, nullptr, reply);
  }

  dbus_message_unref(msg);
  dbus_message_unref(reply);

  sPairingReqTable.Remove(aDeviceAddress);
  DispatchBluetoothReply(aRunnable, v, errorStr);
  return result;
}

bool
BluetoothDBusService::SetPasskeyInternal(const nsAString& aDeviceAddress,
                                         uint32_t aPasskey,
                                         BluetoothReplyRunnable* aRunnable)
{
  nsAutoString errorStr;
  BluetoothValue v = true;
  DBusMessage *msg;
  if (!sPairingReqTable.Get(aDeviceAddress, &msg)) {
    BT_WARNING("%s: Couldn't get original request message.", __FUNCTION__);
    errorStr.AssignLiteral("Couldn't get original request message.");
    DispatchBluetoothReply(aRunnable, v, errorStr);
    return false;
  }

  DBusMessage *reply = dbus_message_new_method_return(msg);

  if (!reply) {
    BT_WARNING("%s: Memory can't be allocated for the message.", __FUNCTION__);
    dbus_message_unref(msg);
    errorStr.AssignLiteral("Memory can't be allocated for the message.");
    DispatchBluetoothReply(aRunnable, v, errorStr);
    return false;
  }

  uint32_t passkey = aPasskey;
  bool result;

  if (!dbus_message_append_args(reply,
                                DBUS_TYPE_UINT32, &passkey,
                                DBUS_TYPE_INVALID)) {
    BT_WARNING("%s: Couldn't append arguments to dbus message.", __FUNCTION__);
    errorStr.AssignLiteral("Couldn't append arguments to dbus message.");
    result = false;
  } else {
    result = dbus_func_send(mConnection, nullptr, reply);
  }

  dbus_message_unref(msg);
  dbus_message_unref(reply);

  sPairingReqTable.Remove(aDeviceAddress);
  DispatchBluetoothReply(aRunnable, v, errorStr);
  return result;
}

bool
BluetoothDBusService::SetPairingConfirmationInternal(
                                              const nsAString& aDeviceAddress,
                                              bool aConfirm,
                                              BluetoothReplyRunnable* aRunnable)
{
  nsAutoString errorStr;
  BluetoothValue v = true;
  DBusMessage *msg;
  if (!sPairingReqTable.Get(aDeviceAddress, &msg)) {
    BT_WARNING("%s: Couldn't get original request message.", __FUNCTION__);
    errorStr.AssignLiteral("Couldn't get original request message.");
    DispatchBluetoothReply(aRunnable, v, errorStr);
    return false;
  }

  DBusMessage *reply;

  if (aConfirm) {
    reply = dbus_message_new_method_return(msg);
  } else {
    reply = dbus_message_new_error(msg, "org.bluez.Error.Rejected",
                                   "User rejected confirmation");
  }

  if (!reply) {
    BT_WARNING("%s: Memory can't be allocated for the message.", __FUNCTION__);
    dbus_message_unref(msg);
    errorStr.AssignLiteral("Memory can't be allocated for the message.");
    DispatchBluetoothReply(aRunnable, v, errorStr);
    return false;
  }

  bool result = dbus_func_send(mConnection, nullptr, reply);
  if (!result) {
    errorStr.AssignLiteral("Can't send message!");
  }
  dbus_message_unref(msg);
  dbus_message_unref(reply);

  sPairingReqTable.Remove(aDeviceAddress);
  DispatchBluetoothReply(aRunnable, v, errorStr);
  return result;
}

void
BluetoothDBusService::Connect(const nsAString& aDeviceAddress,
                              const uint16_t aProfileId,
                              BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (aProfileId == BluetoothServiceClass::HANDSFREE) {
    BluetoothHfpManager* hfp = BluetoothHfpManager::Get();
    hfp->Connect(aDeviceAddress, true, aRunnable);
  } else if (aProfileId == BluetoothServiceClass::HEADSET) {
    BluetoothHfpManager* hfp = BluetoothHfpManager::Get();
    hfp->Connect(aDeviceAddress, false, aRunnable);
  } else if (aProfileId == BluetoothServiceClass::HID) {
    BluetoothHidManager* hid = BluetoothHidManager::Get();
    hid->Connect(aDeviceAddress, aRunnable);
  } else if (aProfileId == BluetoothServiceClass::OBJECT_PUSH) {
    BluetoothOppManager* opp = BluetoothOppManager::Get();
    opp->Connect(aDeviceAddress, aRunnable);
  } else {
    DispatchBluetoothReply(aRunnable, BluetoothValue(),
                           NS_LITERAL_STRING(ERR_UNKNOWN_PROFILE));
  }
}

void
BluetoothDBusService::Disconnect(const uint16_t aProfileId,
                                 BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (aProfileId == BluetoothServiceClass::HANDSFREE ||
      aProfileId == BluetoothServiceClass::HEADSET) {
    BluetoothHfpManager* hfp = BluetoothHfpManager::Get();
    hfp->Disconnect();
  } else if (aProfileId == BluetoothServiceClass::HID) {
    BluetoothHidManager* hid = BluetoothHidManager::Get();
    hid->Disconnect();
  } else if (aProfileId == BluetoothServiceClass::OBJECT_PUSH) {
    BluetoothOppManager* opp = BluetoothOppManager::Get();
    opp->Disconnect();
  } else {
    BT_WARNING(ERR_UNKNOWN_PROFILE);
    return;
  }

  
  
  
  DispatchBluetoothReply(aRunnable, BluetoothValue(true), EmptyString());
}

bool
BluetoothDBusService::IsConnected(const uint16_t aProfileId)
{
  MOZ_ASSERT(NS_IsMainThread());

  BluetoothProfileManagerBase* profile;
  if (aProfileId == BluetoothServiceClass::HANDSFREE ||
      aProfileId == BluetoothServiceClass::HEADSET) {
    profile = BluetoothHfpManager::Get();
  } else if (aProfileId == BluetoothServiceClass::HID) {
    profile = BluetoothHidManager::Get();
  } else if (aProfileId == BluetoothServiceClass::OBJECT_PUSH) {
    profile = BluetoothOppManager::Get();
  } else {
    NS_WARNING(ERR_UNKNOWN_PROFILE);
    return false;
  }

  NS_ENSURE_TRUE(profile, false);
  return profile->IsConnected();
}

class ConnectBluetoothSocketRunnable : public nsRunnable
{
public:
  ConnectBluetoothSocketRunnable(BluetoothReplyRunnable* aRunnable,
                                 UnixSocketConsumer* aConsumer,
                                 const nsAString& aObjectPath,
                                 const nsAString& aServiceUUID,
                                 BluetoothSocketType aType,
                                 bool aAuth,
                                 bool aEncrypt,
                                 int aChannel)
    : mRunnable(dont_AddRef(aRunnable))
    , mConsumer(aConsumer)
    , mObjectPath(aObjectPath)
    , mServiceUUID(aServiceUUID)
    , mType(aType)
    , mAuth(aAuth)
    , mEncrypt(aEncrypt)
    , mChannel(aChannel)
  {
  }

  nsresult
  Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    nsString address = GetAddressFromObjectPath(mObjectPath);
    BluetoothUnixSocketConnector* c =
      new BluetoothUnixSocketConnector(mType, mChannel, mAuth, mEncrypt);
    if (!mConsumer->ConnectSocket(c, NS_ConvertUTF16toUTF8(address).get())) {
      NS_NAMED_LITERAL_STRING(errorStr, "SocketConnectionError");
      DispatchBluetoothReply(mRunnable, BluetoothValue(), errorStr);
      return NS_ERROR_FAILURE;
    }
    return NS_OK;
  }

private:
  nsRefPtr<BluetoothReplyRunnable> mRunnable;
  nsRefPtr<UnixSocketConsumer> mConsumer;
  nsString mObjectPath;
  nsString mServiceUUID;
  BluetoothSocketType mType;
  bool mAuth;
  bool mEncrypt;
  int mChannel;
};

class OnUpdateSdpRecordsRunnable : public nsRunnable
{
public:
  OnUpdateSdpRecordsRunnable(const nsAString& aObjectPath,
                             BluetoothProfileManagerBase* aManager)
    : mManager(aManager)
  {
    MOZ_ASSERT(!aObjectPath.IsEmpty());
    MOZ_ASSERT(aManager);

    mDeviceAddress = GetAddressFromObjectPath(aObjectPath);
  }

  nsresult
  Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    mManager->OnUpdateSdpRecords(mDeviceAddress);

    return NS_OK;
  }

private:
  nsString mDeviceAddress;
  BluetoothProfileManagerBase* mManager;
};

class OnGetServiceChannelRunnable : public nsRunnable
{
public:
  OnGetServiceChannelRunnable(const nsAString& aObjectPath,
                              const nsAString& aServiceUuid,
                              int aChannel,
                              BluetoothProfileManagerBase* aManager)
    : mServiceUuid(aServiceUuid),
      mChannel(aChannel),
      mManager(aManager)
  {
    MOZ_ASSERT(!aObjectPath.IsEmpty());
    MOZ_ASSERT(!aServiceUuid.IsEmpty());
    MOZ_ASSERT(aManager);

    mDeviceAddress = GetAddressFromObjectPath(aObjectPath);
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    mManager->OnGetServiceChannel(mDeviceAddress, mServiceUuid, mChannel);

    return NS_OK;
  }

private:
  nsString mDeviceAddress;
  nsString mServiceUuid;
  int mChannel;
  BluetoothProfileManagerBase* mManager;
};

class OnGetServiceChannelReplyHandler : public DBusReplyHandler
{
public:
  OnGetServiceChannelReplyHandler(const nsAString& aObjectPath,
                                  const nsAString& aServiceUUID,
                                  BluetoothProfileManagerBase* aBluetoothProfileManager)
  : mObjectPath(aObjectPath),
    mServiceUUID(aServiceUUID),
    mBluetoothProfileManager(aBluetoothProfileManager)
  {
    MOZ_ASSERT(mBluetoothProfileManager);
  }

  void Handle(DBusMessage* aReply)
  {
    MOZ_ASSERT(!NS_IsMainThread()); 

    
    
    
    

    int channel = -1;

    if (aReply && (dbus_message_get_type(aReply) != DBUS_MESSAGE_TYPE_ERROR)) {
      channel = dbus_returns_int32(aReply);
    }

    nsRefPtr<nsRunnable> r = new OnGetServiceChannelRunnable(mObjectPath,
                                                             mServiceUUID,
                                                             channel,
                                                             mBluetoothProfileManager);
    nsresult rv = NS_DispatchToMainThread(r);
    NS_ENSURE_SUCCESS_VOID(rv);
  }

private:
  nsString mObjectPath;
  nsString mServiceUUID;
  BluetoothProfileManagerBase* mBluetoothProfileManager;
};

nsresult
BluetoothDBusService::GetServiceChannel(const nsAString& aDeviceAddress,
                                        const nsAString& aServiceUUID,
                                        BluetoothProfileManagerBase* aManager)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsString objectPath(GetObjectPathFromAddress(sAdapterPath, aDeviceAddress));

#ifdef MOZ_WIDGET_GONK
  static const int sProtocolDescriptorList = 0x0004;

  
  
  nsCString serviceUUID = NS_ConvertUTF16toUTF8(aServiceUUID);
  const char* cstrServiceUUID = serviceUUID.get();

  nsRefPtr<OnGetServiceChannelReplyHandler> handler =
    new OnGetServiceChannelReplyHandler(objectPath, aServiceUUID, aManager);

  bool success = dbus_func_args_async(mConnection, -1,
                                      OnGetServiceChannelReplyHandler::Callback, handler,
                                      NS_ConvertUTF16toUTF8(objectPath).get(),
                                      DBUS_DEVICE_IFACE, "GetServiceAttributeValue",
                                      DBUS_TYPE_STRING, &cstrServiceUUID,
                                      DBUS_TYPE_UINT16, &sProtocolDescriptorList,
                                      DBUS_TYPE_INVALID);
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  handler.forget();
#else
  
  
  
  
  
  
  nsRefPtr<nsRunnable> r = new OnGetServiceChannelRunnable(objectPath,
                                                           aServiceUUID,
                                                           1,
                                                           aManager);
  NS_DispatchToMainThread(r);
#endif

  return NS_OK;
}

static void
DiscoverServicesCallback(DBusMessage* aMsg, void* aData)
{
  MOZ_ASSERT(!NS_IsMainThread());

  nsRefPtr<OnUpdateSdpRecordsRunnable> r(
    static_cast<OnUpdateSdpRecordsRunnable*>(aData));
  NS_DispatchToMainThread(r);
}

bool
BluetoothDBusService::UpdateSdpRecords(const nsAString& aDeviceAddress,
                                       BluetoothProfileManagerBase* aManager)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!aDeviceAddress.IsEmpty());
  MOZ_ASSERT(aManager);
  MOZ_ASSERT(mConnection);

  nsString objectPath(GetObjectPathFromAddress(sAdapterPath, aDeviceAddress));

  
  
  OnUpdateSdpRecordsRunnable* callbackRunnable =
    new OnUpdateSdpRecordsRunnable(objectPath, aManager);

  return dbus_func_args_async(mConnection,
                              -1,
                              DiscoverServicesCallback,
                              (void*)callbackRunnable,
                              NS_ConvertUTF16toUTF8(objectPath).get(),
                              DBUS_DEVICE_IFACE,
                              "DiscoverServices",
                              DBUS_TYPE_STRING, &EmptyCString(),
                              DBUS_TYPE_INVALID);
}

nsresult
BluetoothDBusService::GetScoSocket(const nsAString& aAddress,
                                   bool aAuth,
                                   bool aEncrypt,
                                   mozilla::ipc::UnixSocketConsumer* aConsumer)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!mConnection || !gThreadConnection) {
    NS_ERROR("Bluetooth service not started yet!");
    return NS_ERROR_FAILURE;
  }

  BluetoothUnixSocketConnector* c =
    new BluetoothUnixSocketConnector(BluetoothSocketType::SCO, -1,
                                     aAuth, aEncrypt);

  if (!aConsumer->ConnectSocket(c, NS_ConvertUTF16toUTF8(aAddress).get())) {
    nsAutoString replyError;
    replyError.AssignLiteral("SocketConnectionError");
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

void
BluetoothDBusService::SendFile(const nsAString& aDeviceAddress,
                               BlobParent* aBlobParent,
                               BlobChild* aBlobChild,
                               BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  
  
  
  BluetoothOppManager* opp = BluetoothOppManager::Get();
  NS_ENSURE_TRUE_VOID(opp);

  nsAutoString errorStr;
  if (!opp->SendFile(aDeviceAddress, aBlobParent)) {
    errorStr.AssignLiteral("Calling SendFile() failed");
  }

  DispatchBluetoothReply(aRunnable, BluetoothValue(true), errorStr);
}

void
BluetoothDBusService::StopSendingFile(const nsAString& aDeviceAddress,
                                      BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  
  
  
  BluetoothOppManager* opp = BluetoothOppManager::Get();
  NS_ENSURE_TRUE_VOID(opp);

  nsAutoString errorStr;
  if (!opp->StopSendingFile()) {
    errorStr.AssignLiteral("Calling StopSendingFile() failed");
  }

  DispatchBluetoothReply(aRunnable, BluetoothValue(true), errorStr);
}

void
BluetoothDBusService::ConfirmReceivingFile(const nsAString& aDeviceAddress,
                                           bool aConfirm,
                                           BluetoothReplyRunnable* aRunnable)
{
  NS_ASSERTION(NS_IsMainThread(), "Must be called from main thread!");

  
  
  
  
  BluetoothOppManager* opp = BluetoothOppManager::Get();
  NS_ENSURE_TRUE_VOID(opp);

  nsAutoString errorStr;
  if (!opp->ConfirmReceivingFile(aConfirm)) {
    errorStr.AssignLiteral("Calling ConfirmReceivingFile() failed");
  }

  DispatchBluetoothReply(aRunnable, BluetoothValue(true), errorStr);
}

void
BluetoothDBusService::ConnectSco(BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  BluetoothHfpManager* hfp = BluetoothHfpManager::Get();
  NS_ENSURE_TRUE_VOID(hfp);
  if(!hfp->ConnectSco(aRunnable)) {
    NS_NAMED_LITERAL_STRING(replyError,
      "SCO socket exists or HFP is not connected");
    DispatchBluetoothReply(aRunnable, BluetoothValue(), replyError);
  }
}

void
BluetoothDBusService::DisconnectSco(BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  BluetoothHfpManager* hfp = BluetoothHfpManager::Get();
  NS_ENSURE_TRUE_VOID(hfp);
  if (hfp->DisconnectSco()) {
    DispatchBluetoothReply(aRunnable,
                           BluetoothValue(true), NS_LITERAL_STRING(""));
    return;
  }

  NS_NAMED_LITERAL_STRING(replyError,
    "SCO socket doesn't exist or HFP is not connected");
  DispatchBluetoothReply(aRunnable, BluetoothValue(), replyError);
}

void
BluetoothDBusService::IsScoConnected(BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  BluetoothHfpManager* hfp = BluetoothHfpManager::Get();
  NS_ENSURE_TRUE_VOID(hfp);
  DispatchBluetoothReply(aRunnable,
                         hfp->IsScoConnected(), EmptyString());
}

void
BluetoothDBusService::SendMetaData(const nsAString& aTitle,
                                   const nsAString& aArtist,
                                   const nsAString& aAlbum,
                                   int64_t aMediaNumber,
                                   int64_t aTotalMediaCount,
                                   int64_t aDuration,
                                   BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!IsReady()) {
    NS_NAMED_LITERAL_STRING(errorStr, "Bluetooth service is not ready yet!");
    DispatchBluetoothReply(aRunnable, BluetoothValue(), errorStr);
    return;
  }

  BluetoothA2dpManager* a2dp = BluetoothA2dpManager::Get();
  NS_ENSURE_TRUE_VOID(a2dp);

  if (!a2dp->IsConnected()) {
    DispatchBluetoothReply(aRunnable, BluetoothValue(),
                           NS_LITERAL_STRING(ERR_A2DP_IS_DISCONNECTED));
    return;
  } else if (!a2dp->IsAvrcpConnected()) {
    DispatchBluetoothReply(aRunnable, BluetoothValue(),
                           NS_LITERAL_STRING(ERR_AVRCP_IS_DISCONNECTED));
    return;
  }

  nsAutoString address;
  a2dp->GetAddress(address);
  nsString objectPath =
    GetObjectPathFromAddress(sAdapterPath, address);

  nsCString tempTitle = NS_ConvertUTF16toUTF8(aTitle);
  nsCString tempArtist = NS_ConvertUTF16toUTF8(aArtist);
  nsCString tempAlbum = NS_ConvertUTF16toUTF8(aAlbum);

  nsCString tempMediaNumber = EmptyCString();
  nsCString tempTotalMediaCount = EmptyCString();
  nsCString tempDuration = EmptyCString();
  if (aMediaNumber >= 0) {
    tempMediaNumber.AppendInt(aMediaNumber);
  }
  if (aTotalMediaCount >= 0) {
    tempTotalMediaCount.AppendInt(aTotalMediaCount);
  }
  if (aDuration >= 0) {
    tempDuration.AppendInt(aDuration);
  }

  const char* title = tempTitle.get();
  const char* album = tempAlbum.get();
  const char* artist = tempArtist.get();
  const char* mediaNumber = tempMediaNumber.get();
  const char* totalMediaCount = tempTotalMediaCount.get();
  const char* duration = tempDuration.get();

  nsRefPtr<BluetoothReplyRunnable> runnable(aRunnable);

  bool ret = dbus_func_args_async(mConnection,
                                  -1,
                                  GetVoidCallback,
                                  (void*)runnable.get(),
                                  NS_ConvertUTF16toUTF8(objectPath).get(),
                                  DBUS_CTL_IFACE,
                                  "UpdateMetaData",
                                  DBUS_TYPE_STRING, &title,
                                  DBUS_TYPE_STRING, &artist,
                                  DBUS_TYPE_STRING, &album,
                                  DBUS_TYPE_STRING, &mediaNumber,
                                  DBUS_TYPE_STRING, &totalMediaCount,
                                  DBUS_TYPE_STRING, &duration,
                                  DBUS_TYPE_INVALID);
  NS_ENSURE_TRUE_VOID(ret);

  runnable.forget();

  nsAutoString prevTitle, prevAlbum;
  a2dp->GetTitle(prevTitle);
  a2dp->GetAlbum(prevAlbum);

  if (aMediaNumber != a2dp->GetMediaNumber() ||
      !aTitle.Equals(prevTitle) ||
      !aAlbum.Equals(prevAlbum)) {
    UpdateNotification(ControlEventId::EVENT_TRACK_CHANGED, aMediaNumber);
  }

  a2dp->UpdateMetaData(aTitle, aArtist, aAlbum,
                       aMediaNumber, aTotalMediaCount, aDuration);
}

static ControlPlayStatus
PlayStatusStringToControlPlayStatus(const nsAString& aPlayStatus)
{
  ControlPlayStatus playStatus = ControlPlayStatus::PLAYSTATUS_UNKNOWN;
  if (aPlayStatus.EqualsLiteral("STOPPED")) {
    playStatus = ControlPlayStatus::PLAYSTATUS_STOPPED;
  } else if (aPlayStatus.EqualsLiteral("PLAYING")) {
    playStatus = ControlPlayStatus::PLAYSTATUS_PLAYING;
  } else if (aPlayStatus.EqualsLiteral("PAUSED")) {
    playStatus = ControlPlayStatus::PLAYSTATUS_PAUSED;
  } else if (aPlayStatus.EqualsLiteral("FWD_SEEK")) {
    playStatus = ControlPlayStatus::PLAYSTATUS_FWD_SEEK;
  } else if (aPlayStatus.EqualsLiteral("REV_SEEK")) {
    playStatus = ControlPlayStatus::PLAYSTATUS_REV_SEEK;
  } else if (aPlayStatus.EqualsLiteral("ERROR")) {
    playStatus = ControlPlayStatus::PLAYSTATUS_ERROR;
  }

  return playStatus;
}

void
BluetoothDBusService::SendPlayStatus(int64_t aDuration,
                                     int64_t aPosition,
                                     const nsAString& aPlayStatus,
                                     BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!IsReady()) {
    NS_NAMED_LITERAL_STRING(errorStr, "Bluetooth service is not ready yet!");
    DispatchBluetoothReply(aRunnable, BluetoothValue(), errorStr);
    return;
  }

  ControlPlayStatus playStatus =
    PlayStatusStringToControlPlayStatus(aPlayStatus);
  if (playStatus == ControlPlayStatus::PLAYSTATUS_UNKNOWN) {
    DispatchBluetoothReply(aRunnable, BluetoothValue(),
                           NS_LITERAL_STRING("Invalid play status"));
    return;
  } else if (aDuration < 0) {
    DispatchBluetoothReply(aRunnable, BluetoothValue(),
                           NS_LITERAL_STRING("Invalid duration"));
    return;
  } else if (aPosition < 0) {
    DispatchBluetoothReply(aRunnable, BluetoothValue(),
                           NS_LITERAL_STRING("Invalid position"));
    return;
  }

  BluetoothA2dpManager* a2dp = BluetoothA2dpManager::Get();
  NS_ENSURE_TRUE_VOID(a2dp);

  if (!a2dp->IsConnected()) {
    DispatchBluetoothReply(aRunnable, BluetoothValue(),
                           NS_LITERAL_STRING(ERR_A2DP_IS_DISCONNECTED));
    return;
  } else if (!a2dp->IsAvrcpConnected()) {
    DispatchBluetoothReply(aRunnable, BluetoothValue(),
                           NS_LITERAL_STRING(ERR_AVRCP_IS_DISCONNECTED));
    return;
  }

  nsAutoString address;
  a2dp->GetAddress(address);
  nsString objectPath =
    GetObjectPathFromAddress(sAdapterPath, address);

  nsRefPtr<BluetoothReplyRunnable> runnable(aRunnable);

  uint32_t tempPlayStatus = playStatus;
  bool ret = dbus_func_args_async(mConnection,
                                  -1,
                                  GetVoidCallback,
                                  (void*)runnable.get(),
                                  NS_ConvertUTF16toUTF8(objectPath).get(),
                                  DBUS_CTL_IFACE,
                                  "UpdatePlayStatus",
                                  DBUS_TYPE_UINT32, &aDuration,
                                  DBUS_TYPE_UINT32, &aPosition,
                                  DBUS_TYPE_UINT32, &tempPlayStatus,
                                  DBUS_TYPE_INVALID);
  NS_ENSURE_TRUE_VOID(ret);

  runnable.forget();

  ControlEventId eventId = ControlEventId::EVENT_UNKNOWN;
  uint64_t data;
  if (aPosition != a2dp->GetPosition()) {
    eventId = ControlEventId::EVENT_PLAYBACK_POS_CHANGED;
    data = aPosition;
  } else if (playStatus != a2dp->GetPlayStatus()) {
    eventId = ControlEventId::EVENT_PLAYBACK_STATUS_CHANGED;
    data = tempPlayStatus;
  }

  if (eventId != ControlEventId::EVENT_UNKNOWN) {
    UpdateNotification(eventId, data);
  }

  a2dp->UpdatePlayStatus(aDuration, aPosition, playStatus);
}

static void
ControlCallback(DBusMessage* aMsg, void* aParam)
{
#ifdef DEBUG
  NS_NAMED_LITERAL_STRING(errorStr, "Failed to update playstatus");
  CheckForError(aMsg, aParam, errorStr);
#endif
}

void
BluetoothDBusService::UpdatePlayStatus(uint32_t aDuration,
                                       uint32_t aPosition,
                                       ControlPlayStatus aPlayStatus)
{
  MOZ_ASSERT(NS_IsMainThread());
  NS_ENSURE_TRUE_VOID(this->IsReady());

  BluetoothA2dpManager* a2dp = BluetoothA2dpManager::Get();
  NS_ENSURE_TRUE_VOID(a2dp);
  MOZ_ASSERT(a2dp->IsConnected());
  MOZ_ASSERT(a2dp->IsAvrcpConnected());

  nsAutoString address;
  a2dp->GetAddress(address);
  nsString objectPath =
    GetObjectPathFromAddress(sAdapterPath, address);

  uint32_t tempPlayStatus = aPlayStatus;
  bool ret = dbus_func_args_async(mConnection,
                                  -1,
                                  ControlCallback,
                                  nullptr,
                                  NS_ConvertUTF16toUTF8(objectPath).get(),
                                  DBUS_CTL_IFACE,
                                  "UpdatePlayStatus",
                                  DBUS_TYPE_UINT32, &aDuration,
                                  DBUS_TYPE_UINT32, &aPosition,
                                  DBUS_TYPE_UINT32, &tempPlayStatus,
                                  DBUS_TYPE_INVALID);
  NS_ENSURE_TRUE_VOID(ret);
}

void
BluetoothDBusService::UpdateNotification(ControlEventId aEventId,
                                         uint64_t aData)
{
  MOZ_ASSERT(NS_IsMainThread());
  NS_ENSURE_TRUE_VOID(this->IsReady());

  BluetoothA2dpManager* a2dp = BluetoothA2dpManager::Get();
  NS_ENSURE_TRUE_VOID(a2dp);
  MOZ_ASSERT(a2dp->IsConnected());
  MOZ_ASSERT(a2dp->IsAvrcpConnected());

  nsAutoString address;
  a2dp->GetAddress(address);
  nsString objectPath =
    GetObjectPathFromAddress(sAdapterPath, address);
  uint16_t eventId = aEventId;

  bool ret = dbus_func_args_async(mConnection,
                                  -1,
                                  ControlCallback,
                                  nullptr,
                                  NS_ConvertUTF16toUTF8(objectPath).get(),
                                  DBUS_CTL_IFACE,
                                  "UpdateNotification",
                                  DBUS_TYPE_UINT16, &eventId,
                                  DBUS_TYPE_UINT64, &aData,
                                  DBUS_TYPE_INVALID);
  NS_ENSURE_TRUE_VOID(ret);
}
