





#include "WifiUtils.h"
#include <dlfcn.h>
#include <errno.h>
#include <cutils/properties.h>
#include "prinit.h"
#include "js/CharacterEncoding.h"

using namespace mozilla::dom;

#define BUFFER_SIZE        4096
#define COMMAND_SIZE       256



static void* sWifiLib;
static PRCallOnceType sInitWifiLib;

static PRStatus
InitWifiLib()
{
  sWifiLib = dlopen("/system/lib/libhardware_legacy.so", RTLD_LAZY);
  
  return PR_SUCCESS;
}

static void*
GetSharedLibrary()
{
  PR_CallOnce(&sInitWifiLib, InitWifiLib);
  return sWifiLib;
}

static bool
GetWifiP2pSupported()
{
  char propP2pSupported[PROPERTY_VALUE_MAX];
  property_get("ro.moz.wifi.p2p_supported", propP2pSupported, "0");
  return (0 == strcmp(propP2pSupported, "1"));
}

static int
hex2num(char c)
{
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  return -1;
}

static int
hex2byte(const char* hex)
{
  int a, b;
  a = hex2num(*hex++);
  if (a < 0)
    return -1;
  b = hex2num(*hex++);
  if (b < 0)
    return -1;
  return (a << 4) | b;
}




static uint32_t
convertToBytes(char* buf, uint32_t maxlen, const char* str)
{
  const char *pos = str;
  uint32_t len = 0;
  int val;

  while (*pos) {
    if (len == maxlen)
      break;
    switch (*pos) {
    case '\\':
      pos++;
      switch (*pos) {
      case '\\':
        buf[len++] = '\\';
        pos++;
        break;
      case '"':
        buf[len++] = '"';
        pos++;
        break;
      case 'n':
        buf[len++] = '\n';
        pos++;
        break;
      case 'r':
        buf[len++] = '\r';
        pos++;
        break;
      case 't':
        buf[len++] = '\t';
        pos++;
        break;
      case 'e':
        buf[len++] = '\e';
        pos++;
        break;
      case 'x':
        pos++;
        val = hex2byte(pos);
        if (val < 0) {
          val = hex2num(*pos);
          if (val < 0)
            break;
          buf[len++] = val;
          pos++;
        } else {
          buf[len++] = val;
          pos += 2;
        }
        break;
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
        val = *pos++ - '0';
        if (*pos >= '0' && *pos <= '7')
          val = val * 8 + (*pos++ - '0');
        if (*pos >= '0' && *pos <= '7')
          val = val * 8 + (*pos++ - '0');
        buf[len++] = val;
        break;
      default:
        break;
      }
      break;
    default:
      buf[len++] = *pos++;
      break;
    }
  }
  return len;
}





static const uint32_t REPLACE_UTF8 = 0xFFFD;

static void
LossyConvertUTF8toUTF16(const char* aInput, uint32_t aLength, nsAString& aOut)
{
  JS::UTF8Chars src(aInput, aLength);

  char16_t dst[aLength]; 

  
  
  size_t srclen = src.length();
  uint32_t j = 0;
  for (uint32_t i = 0; i < srclen; i++, j++) {
    uint32_t v = uint32_t(src[i]);
    if (v == uint32_t('\0') && i < srclen - 1) {
      
      
      
      j--;
      continue;
    }
    if (!(v & 0x80)) {
      
      dst[j] = char16_t(v);
    } else {
      
      uint32_t n = 1;
      while (v & (0x80 >> n))
        n++;

  #define INVALID(report, arg, n2)                          \
      do {                                                  \
        n = n2;                                             \
        goto invalidMultiByteCodeUnit;                      \
      } while (0)

      
      if (n < 2 || n > 4)
        INVALID(ReportInvalidCharacter, i, 1);

      
      if (i + n > srclen)
        INVALID(ReportBufferTooSmall,  0, 1);

      
      
      if ((v == 0xE0 && ((uint8_t)src[i + 1] & 0xE0) != 0xA0) ||  
        (v == 0xED && ((uint8_t)src[i + 1] & 0xE0) != 0x80) ||  
        (v == 0xF0 && ((uint8_t)src[i + 1] & 0xF0) == 0x80) ||  
        (v == 0xF4 && ((uint8_t)src[i + 1] & 0xF0) != 0x80))    
      {
        INVALID(ReportInvalidCharacter, i, 1);
      }

      
      for (uint32_t m = 1; m < n; m++)
        if ((src[i + m] & 0xC0) != 0x80)
          INVALID(ReportInvalidCharacter, i, m);

      
      v = JS::Utf8ToOneUcs4Char((uint8_t *)&src[i], n);
      if (v < 0x10000) {
        
        dst[j] = char16_t(v);
      } else {
        v -= 0x10000;
        if (v <= 0xFFFFF) {
          
          dst[j] = char16_t((v >> 10) + 0xD800);
          j++;
          dst[j] = char16_t((v & 0x3FF) + 0xDC00);
        } else {
          
          INVALID(ReportTooBigCharacter, v, 1);
        }
      }

    invalidMultiByteCodeUnit:
      
      
      
      i += n - 1;
    }
  }

  dst[j] = 0;
  aOut = dst;
}


#define CHECK_HWLIB(ret)                                                      \
  void* hwLib = GetSharedLibrary();                                           \
  if (!hwLib) {                                                               \
    NS_WARNING("No /system/lib/libhardware_legacy.so");                       \
    return ret;                                                               \
  }

#define DEFAULT_IMPL(name, ret, args...) \
  DEFINE_DLFUNC(name, ret, args...)      \
  ret do_##name(args) {                  \
    USE_DLFUNC(name)                     \
    return name(args);                   \
  }


class ICSWpaSupplicantImpl : public WpaSupplicantImpl
{
public:
  DEFAULT_IMPL(wifi_load_driver, int32_t, )
  DEFAULT_IMPL(wifi_unload_driver, int32_t, )

  DEFINE_DLFUNC(wifi_wait_for_event, int32_t, char*, size_t)
  int32_t do_wifi_wait_for_event(const char *iface, char *buf, size_t len) {
    USE_DLFUNC(wifi_wait_for_event)
    return wifi_wait_for_event(buf, len);
  }

  DEFINE_DLFUNC(wifi_command, int32_t, const char*, char*, size_t*)
  int32_t do_wifi_command(const char* iface, const char* cmd, char* buf, size_t* len) {
    USE_DLFUNC(wifi_command)
    return wifi_command(cmd, buf, len);
  }

  DEFINE_DLFUNC(wifi_start_supplicant, int32_t, )
  int32_t do_wifi_start_supplicant(int32_t) {
    USE_DLFUNC(wifi_start_supplicant)
    return wifi_start_supplicant();
  }

  DEFINE_DLFUNC(wifi_stop_supplicant, int32_t)
  int32_t do_wifi_stop_supplicant(int32_t) {
    USE_DLFUNC(wifi_stop_supplicant)
    return wifi_stop_supplicant();
  }

  DEFINE_DLFUNC(wifi_connect_to_supplicant, int32_t, )
  int32_t do_wifi_connect_to_supplicant(const char* iface) {
    USE_DLFUNC(wifi_connect_to_supplicant)
    return wifi_connect_to_supplicant();
  }

  DEFINE_DLFUNC(wifi_close_supplicant_connection, void, )
  void do_wifi_close_supplicant_connection(const char* iface) {
    USE_DLFUNC(wifi_close_supplicant_connection)
    return wifi_close_supplicant_connection();
  }
};



class JBWpaSupplicantImpl : public ICSWpaSupplicantImpl
{
public:
  DEFINE_DLFUNC(wifi_wait_for_event, int32_t, const char*, char*, size_t)
  int32_t do_wifi_wait_for_event(const char* iface, char* buf, size_t len) {
    USE_DLFUNC(wifi_wait_for_event)
    return wifi_wait_for_event(iface, buf, len);
  }

  DEFINE_DLFUNC(wifi_command, int32_t, const char*, const char*, char*, size_t*)
  int32_t do_wifi_command(const char* iface, const char* cmd, char* buf, size_t* len) {
    USE_DLFUNC(wifi_command)
    return wifi_command(iface, cmd, buf, len);
  }

  DEFINE_DLFUNC(wifi_start_supplicant, int32_t, int32_t)
  int32_t do_wifi_start_supplicant(int32_t arg) {
    USE_DLFUNC(wifi_start_supplicant)
    return wifi_start_supplicant(arg);
  }

  DEFINE_DLFUNC(wifi_stop_supplicant, int32_t, int32_t)
  int32_t do_wifi_stop_supplicant(int32_t arg) {
    USE_DLFUNC(wifi_stop_supplicant)
    return wifi_stop_supplicant(arg);
  }

  DEFINE_DLFUNC(wifi_connect_to_supplicant, int32_t, const char*)
  int32_t do_wifi_connect_to_supplicant(const char* iface) {
    USE_DLFUNC(wifi_connect_to_supplicant)
    return wifi_connect_to_supplicant(iface);
  }

  DEFINE_DLFUNC(wifi_close_supplicant_connection, void, const char*)
  void do_wifi_close_supplicant_connection(const char* iface) {
    USE_DLFUNC(wifi_close_supplicant_connection)
    wifi_close_supplicant_connection(iface);
  }
};



class KKWpaSupplicantImpl : public ICSWpaSupplicantImpl
{
public:
  DEFINE_DLFUNC(wifi_start_supplicant, int32_t, int32_t)
  int32_t do_wifi_start_supplicant(int32_t arg) {
    USE_DLFUNC(wifi_start_supplicant)
    return wifi_start_supplicant(arg);
  }

  DEFINE_DLFUNC(wifi_stop_supplicant, int32_t, int32_t)
  int32_t do_wifi_stop_supplicant(int32_t arg) {
    USE_DLFUNC(wifi_stop_supplicant)
    return wifi_stop_supplicant(arg);
  }

  DEFINE_DLFUNC(wifi_command, int32_t, const char*, char*, size_t*)
  int32_t do_wifi_command(const char* iface, const char* cmd, char* buf, size_t* len) {
    char command[COMMAND_SIZE];
    if (!strcmp(iface, "p2p0")) {
      
      PR_snprintf(command, COMMAND_SIZE, "%s", cmd);
    }
    else {
      PR_snprintf(command, COMMAND_SIZE, "IFNAME=%s %s", iface, cmd);
    }
    USE_DLFUNC(wifi_command)
    return wifi_command(command, buf, len);
  }
};


WpaSupplicant::WpaSupplicant()
{
  char propVersion[PROPERTY_VALUE_MAX];
  property_get("ro.build.version.sdk", propVersion, "0");
  mSdkVersion = strtol(propVersion, nullptr, 10);

  if (mSdkVersion < 16) {
    mImpl = new ICSWpaSupplicantImpl();
  } else if (mSdkVersion < 19) {
    mImpl = new JBWpaSupplicantImpl();
  } else {
    mImpl = new KKWpaSupplicantImpl();
  }
  mWifiHotspotUtils = new WifiHotspotUtils();
};

void WpaSupplicant::WaitForEvent(nsAString& aEvent, const nsCString& aInterface)
{
  CHECK_HWLIB()

  char buffer[BUFFER_SIZE];
  int32_t ret = mImpl->do_wifi_wait_for_event(aInterface.get(), buffer, BUFFER_SIZE);
  CheckBuffer(buffer, ret, aEvent);
}

#define GET_CHAR(prop) NS_ConvertUTF16toUTF8(aOptions.prop).get()




uint32_t WpaSupplicant::MakeMask(uint32_t len) {
  uint32_t mask = 0;
  for (uint32_t i = 0; i < len; ++i) {
    mask |= (0x80000000 >> i);
  }
  return ntohl(mask);
}

bool WpaSupplicant::ExecuteCommand(CommandOptions aOptions,
                                   WifiResultOptions& aResult,
                                   const nsCString& aInterface)
{
  CHECK_HWLIB(false)

  if (!mWifiHotspotUtils->GetSharedLibrary()) {
    return false;
  }

  
  aResult.mId = aOptions.mId;

  if (aOptions.mCmd.EqualsLiteral("command")) {
    size_t len = BUFFER_SIZE - 1;
    char buffer[BUFFER_SIZE];
    NS_ConvertUTF16toUTF8 request(aOptions.mRequest);
    aResult.mStatus = mImpl->do_wifi_command(aInterface.get(), request.get(), buffer, &len);
    nsString value;
    if (aResult.mStatus == 0) {
      if (buffer[len - 1] == '\n') { 
        len--;
      }
      buffer[len] = '\0';
      CheckBuffer(buffer, len, value);
    }
    aResult.mReply = value;
  } else if (aOptions.mCmd.EqualsLiteral("close_supplicant_connection")) {
    mImpl->do_wifi_close_supplicant_connection(aInterface.get());
  } else if (aOptions.mCmd.EqualsLiteral("load_driver")) {
    aResult.mStatus = mImpl->do_wifi_load_driver();
  } else if (aOptions.mCmd.EqualsLiteral("unload_driver")) {
    aResult.mStatus = mImpl->do_wifi_unload_driver();
  } else if (aOptions.mCmd.EqualsLiteral("start_supplicant")) {
    aResult.mStatus = mImpl->do_wifi_start_supplicant(GetWifiP2pSupported() ? 1 : 0);
  } else if (aOptions.mCmd.EqualsLiteral("stop_supplicant")) {
    aResult.mStatus = mImpl->do_wifi_stop_supplicant(0);
  } else if (aOptions.mCmd.EqualsLiteral("connect_to_supplicant")) {
    aResult.mStatus = mImpl->do_wifi_connect_to_supplicant(aInterface.get());
  } else if (aOptions.mCmd.EqualsLiteral("hostapd_command")) {
    size_t len = BUFFER_SIZE - 1;
    char buffer[BUFFER_SIZE];
    NS_ConvertUTF16toUTF8 request(aOptions.mRequest);
    aResult.mStatus = mWifiHotspotUtils->do_wifi_hostapd_command(request.get(),
                                                                 buffer,
                                                                 &len);
    nsString value;
    if (aResult.mStatus == 0) {
      if (buffer[len - 1] == '\n') { 
        len--;
      }
      buffer[len] = '\0';
      CheckBuffer(buffer, len, value);
    }
    aResult.mReply = value;
  } else if (aOptions.mCmd.EqualsLiteral("hostapd_get_stations")) {
    aResult.mStatus = mWifiHotspotUtils->do_wifi_hostapd_get_stations();
  } else if (aOptions.mCmd.EqualsLiteral("connect_to_hostapd")) {
    aResult.mStatus = mWifiHotspotUtils->do_wifi_connect_to_hostapd();
  } else if (aOptions.mCmd.EqualsLiteral("close_hostapd_connection")) {
    aResult.mStatus = mWifiHotspotUtils->do_wifi_close_hostapd_connection();
  } else if (aOptions.mCmd.EqualsLiteral("hostapd_command")) {
    size_t len = BUFFER_SIZE - 1;
    char buffer[BUFFER_SIZE];
    NS_ConvertUTF16toUTF8 request(aOptions.mRequest);
    aResult.mStatus = mWifiHotspotUtils->do_wifi_hostapd_command(request.get(),
                                                                 buffer,
                                                                 &len);
    nsString value;
    if (aResult.mStatus == 0) {
      if (buffer[len - 1] == '\n') { 
        len--;
      }
      buffer[len] = '\0';
      CheckBuffer(buffer, len, value);
    }
    aResult.mReply = value;
  } else if (aOptions.mCmd.EqualsLiteral("hostapd_get_stations")) {
    aResult.mStatus = mWifiHotspotUtils->do_wifi_hostapd_get_stations();
  } else if (aOptions.mCmd.EqualsLiteral("connect_to_hostapd")) {
    aResult.mStatus = mWifiHotspotUtils->do_wifi_connect_to_hostapd();
  } else if (aOptions.mCmd.EqualsLiteral("close_hostapd_connection")) {
    aResult.mStatus = mWifiHotspotUtils->do_wifi_close_hostapd_connection();

  } else {
    NS_WARNING("WpaSupplicant::ExecuteCommand : Unknown command");
    printf_stderr("WpaSupplicant::ExecuteCommand : Unknown command: %s",
      NS_ConvertUTF16toUTF8(aOptions.mCmd).get());
    return false;
  }

  return true;
}


void
WpaSupplicant::CheckBuffer(char* buffer, int32_t length,
                           nsAString& aEvent)
{
  if (length <= 0 || length >= (BUFFER_SIZE - 1)) {
    NS_WARNING("WpaSupplicant::CheckBuffer: Invalid buffer length");
    return;
  }

  if (mSdkVersion < 18) {
    buffer[length] = 0;
    LossyConvertUTF8toUTF16(buffer, length, aEvent);
    return;
  }

  
  
  
  
  char bytesBuffer[BUFFER_SIZE];
  uint32_t bytes = convertToBytes(bytesBuffer, length, buffer);
  if (bytes <= 0 || bytes >= BUFFER_SIZE) {
    NS_WARNING("WpaSupplicant::CheckBuffer: Invalid bytesbuffer length");
    return;
  }
  bytesBuffer[bytes] = 0;
  LossyConvertUTF8toUTF16(bytesBuffer, bytes, aEvent);
}
