














#include "NetworkUtils.h"

#include <android/log.h>
#include <cutils/properties.h>
#include <limits>
#include "mozilla/dom/network/NetUtils.h"

#include <sys/types.h>  
#include <sys/socket.h> 
#include <netdb.h>
#include <arpa/inet.h>  

#define _DEBUG 0

#define WARN(args...)   __android_log_print(ANDROID_LOG_WARN,  "NetworkUtils", ## args)
#define ERROR(args...)  __android_log_print(ANDROID_LOG_ERROR,  "NetworkUtils", ## args)

#if _DEBUG
#define DEBUG(args...)  __android_log_print(ANDROID_LOG_DEBUG, "NetworkUtils" , ## args)
#else
#define DEBUG(args...)
#endif

using namespace mozilla::dom;
using namespace mozilla::ipc;

static const char* PERSIST_SYS_USB_CONFIG_PROPERTY = "persist.sys.usb.config";
static const char* SYS_USB_CONFIG_PROPERTY         = "sys.usb.config";
static const char* SYS_USB_STATE_PROPERTY          = "sys.usb.state";

static const char* USB_FUNCTION_RNDIS  = "rndis";
static const char* USB_FUNCTION_ADB    = "adb";


static const char* DUMMY_COMMAND = "tether status";


static const uint32_t USB_FUNCTION_RETRY_TIMES = 20;

static const uint32_t USB_FUNCTION_RETRY_INTERVAL = 100;


static const uint32_t NETD_COMMAND_PROCEEDING   = 100;

static const uint32_t NETD_COMMAND_OKAY         = 200;


static const uint32_t NETD_COMMAND_FAIL         = 400;

static const uint32_t NETD_COMMAND_ERROR        = 500;

static const uint32_t NETD_COMMAND_UNSOLICITED  = 600;


static const uint32_t NETD_COMMAND_INTERFACE_CHANGE     = 600;
static const uint32_t NETD_COMMAND_BANDWIDTH_CONTROLLER = 601;

static const char* INTERFACE_DELIMIT = ",";
static const char* USB_CONFIG_DELIMIT = ",";
static const char* NETD_MESSAGE_DELIMIT = " ";

static const uint32_t BUF_SIZE = 1024;

static uint32_t SDK_VERSION;

struct IFProperties {
  char gateway[PROPERTY_VALUE_MAX];
  char dns1[PROPERTY_VALUE_MAX];
  char dns2[PROPERTY_VALUE_MAX];
};

struct CurrentCommand {
  CommandChain* chain;
  CommandCallback callback;
  char command[MAX_COMMAND_SIZE];
};

typedef Tuple3<NetdCommand*, CommandChain*, CommandCallback> QueueData;

#define GET_CURRENT_NETD_COMMAND   (gCommandQueue.IsEmpty() ? nullptr : gCommandQueue[0].a)
#define GET_CURRENT_CHAIN          (gCommandQueue.IsEmpty() ? nullptr : gCommandQueue[0].b)
#define GET_CURRENT_CALLBACK       (gCommandQueue.IsEmpty() ? nullptr : gCommandQueue[0].c)
#define GET_CURRENT_COMMAND        (gCommandQueue.IsEmpty() ? nullptr : gCommandQueue[0].a->mData)

static NetworkUtils* gNetworkUtils;
static nsTArray<QueueData> gCommandQueue;
static CurrentCommand gCurrentCommand;
static bool gPending = false;
static nsTArray<nsCString> gReason;

CommandFunc NetworkUtils::sWifiEnableChain[] = {
  NetworkUtils::wifiFirmwareReload,
  NetworkUtils::startAccessPointDriver,
  NetworkUtils::setAccessPoint,
  NetworkUtils::startSoftAP,
  NetworkUtils::setInterfaceUp,
  NetworkUtils::tetherInterface,
  NetworkUtils::setIpForwardingEnabled,
  NetworkUtils::tetheringStatus,
  NetworkUtils::startTethering,
  NetworkUtils::setDnsForwarders,
  NetworkUtils::enableNat,
  NetworkUtils::wifiTetheringSuccess
};

CommandFunc NetworkUtils::sWifiDisableChain[] = {
  NetworkUtils::stopSoftAP,
  NetworkUtils::stopAccessPointDriver,
  NetworkUtils::wifiFirmwareReload,
  NetworkUtils::untetherInterface,
  NetworkUtils::preTetherInterfaceList,
  NetworkUtils::postTetherInterfaceList,
  NetworkUtils::disableNat,
  NetworkUtils::setIpForwardingEnabled,
  NetworkUtils::stopTethering,
  NetworkUtils::wifiTetheringSuccess
};

CommandFunc NetworkUtils::sWifiFailChain[] = {
  NetworkUtils::stopSoftAP,
  NetworkUtils::setIpForwardingEnabled,
  NetworkUtils::stopTethering
};

CommandFunc NetworkUtils::sWifiOperationModeChain[] = {
  NetworkUtils::wifiFirmwareReload,
  NetworkUtils::wifiOperationModeSuccess
};

CommandFunc NetworkUtils::sUSBEnableChain[] = {
  NetworkUtils::setInterfaceUp,
  NetworkUtils::enableNat,
  NetworkUtils::setIpForwardingEnabled,
  NetworkUtils::tetherInterface,
  NetworkUtils::tetheringStatus,
  NetworkUtils::startTethering,
  NetworkUtils::setDnsForwarders,
  NetworkUtils::usbTetheringSuccess
};

CommandFunc NetworkUtils::sUSBDisableChain[] = {
  NetworkUtils::untetherInterface,
  NetworkUtils::preTetherInterfaceList,
  NetworkUtils::postTetherInterfaceList,
  NetworkUtils::disableNat,
  NetworkUtils::setIpForwardingEnabled,
  NetworkUtils::stopTethering,
  NetworkUtils::usbTetheringSuccess
};

CommandFunc NetworkUtils::sUSBFailChain[] = {
  NetworkUtils::stopSoftAP,
  NetworkUtils::setIpForwardingEnabled,
  NetworkUtils::stopTethering
};

CommandFunc NetworkUtils::sUpdateUpStreamChain[] = {
  NetworkUtils::cleanUpStream,
  NetworkUtils::createUpStream,
  NetworkUtils::updateUpStreamSuccess
};

CommandFunc NetworkUtils::sStartDhcpServerChain[] = {
  NetworkUtils::setInterfaceUp,
  NetworkUtils::startTethering,
  NetworkUtils::setDhcpServerSuccess
};

CommandFunc NetworkUtils::sStopDhcpServerChain[] = {
  NetworkUtils::stopTethering,
  NetworkUtils::setDhcpServerSuccess
};

CommandFunc NetworkUtils::sNetworkInterfaceStatsChain[] = {
  NetworkUtils::getRxBytes,
  NetworkUtils::getTxBytes,
  NetworkUtils::networkInterfaceStatsSuccess
};

CommandFunc NetworkUtils::sNetworkInterfaceEnableAlarmChain[] = {
  NetworkUtils::enableAlarm,
  NetworkUtils::setQuota,
  NetworkUtils::setAlarm,
  NetworkUtils::networkInterfaceAlarmSuccess
};

CommandFunc NetworkUtils::sNetworkInterfaceDisableAlarmChain[] = {
  NetworkUtils::removeQuota,
  NetworkUtils::disableAlarm,
  NetworkUtils::networkInterfaceAlarmSuccess
};

CommandFunc NetworkUtils::sNetworkInterfaceSetAlarmChain[] = {
  NetworkUtils::setAlarm,
  NetworkUtils::networkInterfaceAlarmSuccess
};

CommandFunc NetworkUtils::sSetDnsChain[] = {
  NetworkUtils::setDefaultInterface,
  NetworkUtils::setInterfaceDns
};




static uint32_t makeMask(const uint32_t prefixLength)
{
  uint32_t mask = 0;
  for (uint32_t i = 0; i < prefixLength; ++i) {
    mask |= (0x80000000 >> i);
  }
  return ntohl(mask);
}





static char* getNetworkAddr(const uint32_t ip, const uint32_t prefix)
{
  uint32_t mask = 0, subnet = 0;

  mask = ~mask << (32 - prefix);
  mask = htonl(mask);
  subnet = ip & mask;

  struct in_addr addr;
  addr.s_addr = subnet;

  return inet_ntoa(addr);
}




static void split(char* str, const char* sep, nsTArray<nsCString>& result)
{
  char *s = strtok(str, sep);
  while (s != nullptr) {
    result.AppendElement(s);
    s = strtok(nullptr, sep);
  }
}

static void split(char* str, const char* sep, nsTArray<nsString>& result)
{
  char *s = strtok(str, sep);
  while (s != nullptr) {
    result.AppendElement(NS_ConvertUTF8toUTF16(s));
    s = strtok(nullptr, sep);
  }
}




static void join(nsTArray<nsCString>& array, const char* sep, const uint32_t maxlen, char* result)
{
#define CHECK_LENGTH(len, add, max)  len += add;          \
                                     if (len > max - 1)   \
                                       return;            \

  uint32_t len = 0;
  uint32_t seplen = strlen(sep);

  if (array.Length() > 0) {
    CHECK_LENGTH(len, strlen(array[0].get()), maxlen)
    strcpy(result, array[0].get());

    for (uint32_t i = 1; i < array.Length(); i++) {
      CHECK_LENGTH(len, seplen, maxlen)
      strcat(result, sep);

      CHECK_LENGTH(len, strlen(array[i].get()), maxlen)
      strcat(result, array[i].get());
    }
  }

#undef CHECK_LEN
}




static void getIFProperties(const char* ifname, IFProperties& prop)
{
  char key[PROPERTY_KEY_MAX];
  snprintf(key, PROPERTY_KEY_MAX - 1, "net.%s.gw", ifname);
  property_get(key, prop.gateway, "");
  snprintf(key, PROPERTY_KEY_MAX - 1, "net.%s.dns1", ifname);
  property_get(key, prop.dns1, "");
  snprintf(key, PROPERTY_KEY_MAX - 1, "net.%s.dns2", ifname);
  property_get(key, prop.dns2, "");
}

static int getIpType(const char *aIp) {
  struct addrinfo hint, *ip_info = NULL;

  memset(&hint, 0, sizeof(hint));
  hint.ai_family = AF_UNSPEC;
  hint.ai_flags = AI_NUMERICHOST;

  if (getaddrinfo(aIp, NULL, &hint, &ip_info)) {
    return AF_UNSPEC;
  }

  int type = ip_info->ai_family;
  freeaddrinfo(ip_info);

  return type;
}





static uint32_t selectGateway(nsTArray<nsString>& gateways, int addrFamily)
{
  uint32_t length = gateways.Length();

  for (uint32_t i = 0; i < length; i++) {
    NS_ConvertUTF16toUTF8 autoGateway(gateways[i]);
    if ((getIpType(autoGateway.get()) == AF_INET && addrFamily == AF_INET) ||
        (getIpType(autoGateway.get()) == AF_INET6 && addrFamily == AF_INET6)) {
      return i;
    }
  }
  return length; 
}

static void postMessage(NetworkResultOptions& aResult)
{
  MOZ_ASSERT(gNetworkUtils);
  MOZ_ASSERT(gNetworkUtils->getMessageCallback());

  if (*(gNetworkUtils->getMessageCallback()))
    (*(gNetworkUtils->getMessageCallback()))(aResult);
}

static void postMessage(NetworkParams& aOptions, NetworkResultOptions& aResult)
{
  MOZ_ASSERT(gNetworkUtils);
  MOZ_ASSERT(gNetworkUtils->getMessageCallback());

  aResult.mId = aOptions.mId;

  if (*(gNetworkUtils->getMessageCallback()))
    (*(gNetworkUtils->getMessageCallback()))(aResult);
}

void NetworkUtils::next(CommandChain* aChain, bool aError, NetworkResultOptions& aResult)
{
  if (aError) {
    ErrorCallback onError = aChain->getErrorCallback();
    if(onError) {
      aResult.mError = true;
      (*onError)(aChain->getParams(), aResult);
    }
    delete aChain;
    return;
  }
  CommandFunc f = aChain->getNextCommand();
  if (!f) {
    delete aChain;
    return;
  }

  (*f)(aChain, next, aResult);
}




void NetworkUtils::nextNetdCommand()
{
  if (gCommandQueue.IsEmpty() || gPending) {
    return;
  }

  gCurrentCommand.chain = GET_CURRENT_CHAIN;
  gCurrentCommand.callback = GET_CURRENT_CALLBACK;
  snprintf(gCurrentCommand.command, MAX_COMMAND_SIZE - 1, "%s", GET_CURRENT_COMMAND);

  DEBUG("Sending \'%s\' command to netd.", gCurrentCommand.command);
  SendNetdCommand(GET_CURRENT_NETD_COMMAND);

  gCommandQueue.RemoveElementAt(0);
  gPending = true;
}









void NetworkUtils::doCommand(const char* aCommand, CommandChain* aChain, CommandCallback aCallback)
{
  DEBUG("Preparing to send \'%s\' command...", aCommand);

  NetdCommand* netdCommand = new NetdCommand();

  
  if (SDK_VERSION >= 16) {
    snprintf((char*)netdCommand->mData, MAX_COMMAND_SIZE - 1, "0 %s", aCommand);
  } else {
    snprintf((char*)netdCommand->mData, MAX_COMMAND_SIZE - 1, "%s", aCommand);
  }
  netdCommand->mSize = strlen((char*)netdCommand->mData) + 1;

  gCommandQueue.AppendElement(QueueData(netdCommand, aChain, aCallback));

  nextNetdCommand();
}




#define GET_CHAR(prop) NS_ConvertUTF16toUTF8(aChain->getParams().prop).get()
#define GET_FIELD(prop) aChain->getParams().prop

void NetworkUtils::wifiFirmwareReload(CommandChain* aChain,
                                      CommandCallback aCallback,
                                      NetworkResultOptions& aResult)
{
  char command[MAX_COMMAND_SIZE];
  snprintf(command, MAX_COMMAND_SIZE - 1, "softap fwreload %s %s", GET_CHAR(mIfname), GET_CHAR(mMode));

  doCommand(command, aChain, aCallback);
}

void NetworkUtils::startAccessPointDriver(CommandChain* aChain,
                                          CommandCallback aCallback,
                                          NetworkResultOptions& aResult)
{
  
  if (SDK_VERSION >= 16) {
    aResult.mResultCode = 0;
    aResult.mResultReason = NS_ConvertUTF8toUTF16("");
    aCallback(aChain, false, aResult);
    return;
  }

  char command[MAX_COMMAND_SIZE];
  snprintf(command, MAX_COMMAND_SIZE - 1, "softap start %s", GET_CHAR(mIfname));

  doCommand(command, aChain, aCallback);
}

void NetworkUtils::stopAccessPointDriver(CommandChain* aChain,
                                         CommandCallback aCallback,
                                         NetworkResultOptions& aResult)
{
  
  if (SDK_VERSION >= 16) {
    aResult.mResultCode = 0;
    aResult.mResultReason = NS_ConvertUTF8toUTF16("");
    aCallback(aChain, false, aResult);
    return;
  }

  char command[MAX_COMMAND_SIZE];
  snprintf(command, MAX_COMMAND_SIZE - 1, "softap stop %s", GET_CHAR(mIfname));

  doCommand(command, aChain, aCallback);
}




























void NetworkUtils::setAccessPoint(CommandChain* aChain,
                                  CommandCallback aCallback,
                                  NetworkResultOptions& aResult)
{
  char command[MAX_COMMAND_SIZE];
  if (SDK_VERSION >= 19) {
    snprintf(command, MAX_COMMAND_SIZE - 1, "softap set %s \"%s\" broadcast 6 %s \"%s\"",
                     GET_CHAR(mIfname),
                     GET_CHAR(mSsid),
                     GET_CHAR(mSecurity),
                     GET_CHAR(mKey));
  } else if (SDK_VERSION >= 16) {
    snprintf(command, MAX_COMMAND_SIZE - 1, "softap set %s \"%s\" %s \"%s\"",
                     GET_CHAR(mIfname),
                     GET_CHAR(mSsid),
                     GET_CHAR(mSecurity),
                     GET_CHAR(mKey));
  } else {
    snprintf(command, MAX_COMMAND_SIZE - 1, "softap set %s %s \"%s\" %s \"%s\" 6 0 8",
                     GET_CHAR(mIfname),
                     GET_CHAR(mWifictrlinterfacename),
                     GET_CHAR(mSsid),
                     GET_CHAR(mSecurity),
                     GET_CHAR(mKey));
  }

  doCommand(command, aChain, aCallback);
}

void NetworkUtils::cleanUpStream(CommandChain* aChain,
                                 CommandCallback aCallback,
                                 NetworkResultOptions& aResult)
{
  char command[MAX_COMMAND_SIZE];
  snprintf(command, MAX_COMMAND_SIZE - 1, "nat disable %s %s 0", GET_CHAR(mPreInternalIfname), GET_CHAR(mPreExternalIfname));

  doCommand(command, aChain, aCallback);
}

void NetworkUtils::createUpStream(CommandChain* aChain,
                                  CommandCallback aCallback,
                                  NetworkResultOptions& aResult)
{
  char command[MAX_COMMAND_SIZE];
  snprintf(command, MAX_COMMAND_SIZE - 1, "nat enable %s %s 0", GET_CHAR(mCurInternalIfname), GET_CHAR(mCurExternalIfname));

  doCommand(command, aChain, aCallback);
}

void NetworkUtils::startSoftAP(CommandChain* aChain,
                               CommandCallback aCallback,
                               NetworkResultOptions& aResult)
{
  const char* command= "softap startap";
  doCommand(command, aChain, aCallback);
}

void NetworkUtils::stopSoftAP(CommandChain* aChain,
                              CommandCallback aCallback,
                              NetworkResultOptions& aResult)
{
  const char* command= "softap stopap";
  doCommand(command, aChain, aCallback);
}

void NetworkUtils::getRxBytes(CommandChain* aChain,
                              CommandCallback aCallback,
                              NetworkResultOptions& aResult)
{
  char command[MAX_COMMAND_SIZE];
  snprintf(command, MAX_COMMAND_SIZE - 1, "interface readrxcounter %s", GET_CHAR(mIfname));

  doCommand(command, aChain, aCallback);
}

void NetworkUtils::getTxBytes(CommandChain* aChain,
                              CommandCallback aCallback,
                              NetworkResultOptions& aResult)
{
  NetworkParams& options = aChain->getParams();
  options.mRxBytes = atof(NS_ConvertUTF16toUTF8(aResult.mResultReason).get());

  char command[MAX_COMMAND_SIZE];
  snprintf(command, MAX_COMMAND_SIZE - 1, "interface readtxcounter %s", GET_CHAR(mIfname));

  doCommand(command, aChain, aCallback);
}

void NetworkUtils::enableAlarm(CommandChain* aChain,
                               CommandCallback aCallback,
                               NetworkResultOptions& aResult)
{
  const char* command= "bandwidth enable";
  doCommand(command, aChain, aCallback);
}

void NetworkUtils::disableAlarm(CommandChain* aChain,
                                CommandCallback aCallback,
                                NetworkResultOptions& aResult)
{
  const char* command= "bandwidth disable";
  doCommand(command, aChain, aCallback);
}

void NetworkUtils::setQuota(CommandChain* aChain,
                            CommandCallback aCallback,
                            NetworkResultOptions& aResult)
{
  char command[MAX_COMMAND_SIZE];
  snprintf(command, MAX_COMMAND_SIZE - 1, "bandwidth setiquota %s %lld", GET_CHAR(mIfname), LLONG_MAX);

  doCommand(command, aChain, aCallback);
}

void NetworkUtils::removeQuota(CommandChain* aChain,
                               CommandCallback aCallback,
                               NetworkResultOptions& aResult)
{
  char command[MAX_COMMAND_SIZE];
  snprintf(command, MAX_COMMAND_SIZE - 1, "bandwidth removeiquota %s", GET_CHAR(mIfname));

  doCommand(command, aChain, aCallback);
}

void NetworkUtils::setAlarm(CommandChain* aChain,
                            CommandCallback aCallback,
                            NetworkResultOptions& aResult)
{
  char command[MAX_COMMAND_SIZE];
  snprintf(command, MAX_COMMAND_SIZE - 1, "bandwidth setinterfacealert %s %ld", GET_CHAR(mIfname), GET_FIELD(mThreshold));

  doCommand(command, aChain, aCallback);
}

void NetworkUtils::setInterfaceUp(CommandChain* aChain,
                                  CommandCallback aCallback,
                                  NetworkResultOptions& aResult)
{
  char command[MAX_COMMAND_SIZE];
  if (SDK_VERSION >= 16) {
    snprintf(command, MAX_COMMAND_SIZE - 1, "interface setcfg %s %s %s %s",
                     GET_CHAR(mIfname),
                     GET_CHAR(mIp),
                     GET_CHAR(mPrefix),
                     GET_CHAR(mLink));
  } else {
    snprintf(command, MAX_COMMAND_SIZE - 1, "interface setcfg %s %s %s [%s]",
                     GET_CHAR(mIfname),
                     GET_CHAR(mIp),
                     GET_CHAR(mPrefix),
                     GET_CHAR(mLink));
  }
  doCommand(command, aChain, aCallback);
}

void NetworkUtils::tetherInterface(CommandChain* aChain,
                                   CommandCallback aCallback,
                                   NetworkResultOptions& aResult)
{
  char command[MAX_COMMAND_SIZE];
  snprintf(command, MAX_COMMAND_SIZE - 1, "tether interface add %s", GET_CHAR(mIfname));

  doCommand(command, aChain, aCallback);
}

void NetworkUtils::preTetherInterfaceList(CommandChain* aChain,
                                          CommandCallback aCallback,
                                          NetworkResultOptions& aResult)
{
  char command[MAX_COMMAND_SIZE];
  if (SDK_VERSION >= 16) {
    snprintf(command, MAX_COMMAND_SIZE - 1, "tether interface list");
  } else {
    snprintf(command, MAX_COMMAND_SIZE - 1, "tether interface list 0");
  }

  doCommand(command, aChain, aCallback);
}

void NetworkUtils::postTetherInterfaceList(CommandChain* aChain,
                                           CommandCallback aCallback,
                                           NetworkResultOptions& aResult)
{
  
  char command[MAX_COMMAND_SIZE];
  snprintf(command, MAX_COMMAND_SIZE - 1, "%s", DUMMY_COMMAND);

  char buf[BUF_SIZE];
  const char* reason = NS_ConvertUTF16toUTF8(aResult.mResultReason).get();
  memcpy(buf, reason, strlen(reason));
  split(buf, INTERFACE_DELIMIT, GET_FIELD(mInterfaceList));

  doCommand(command, aChain, aCallback);
}

void NetworkUtils::setIpForwardingEnabled(CommandChain* aChain,
                                          CommandCallback aCallback,
                                          NetworkResultOptions& aResult)
{
  char command[MAX_COMMAND_SIZE];

  if (GET_FIELD(mEnable)) {
    snprintf(command, MAX_COMMAND_SIZE - 1, "ipfwd enable");
  } else {
    
    
    if (GET_FIELD(mInterfaceList).Length() > 1) {
      snprintf(command, MAX_COMMAND_SIZE - 1, "%s", DUMMY_COMMAND);
    } else {
      snprintf(command, MAX_COMMAND_SIZE - 1, "ipfwd disable");
    }
  }

  doCommand(command, aChain, aCallback);
}

void NetworkUtils::tetheringStatus(CommandChain* aChain,
                                   CommandCallback aCallback,
                                   NetworkResultOptions& aResult)
{
  const char* command= "tether status";
  doCommand(command, aChain, aCallback);
}

void NetworkUtils::stopTethering(CommandChain* aChain,
                                 CommandCallback aCallback,
                                 NetworkResultOptions& aResult)
{
  char command[MAX_COMMAND_SIZE];

  
  
  if (GET_FIELD(mInterfaceList).Length() > 1) {
    snprintf(command, MAX_COMMAND_SIZE - 1, "%s", DUMMY_COMMAND);
  } else {
    snprintf(command, MAX_COMMAND_SIZE - 1, "tether stop");
  }

  doCommand(command, aChain, aCallback);
}

void NetworkUtils::startTethering(CommandChain* aChain,
                                  CommandCallback aCallback,
                                  NetworkResultOptions& aResult)
{
  char command[MAX_COMMAND_SIZE];

  
  
  if (aResult.mResultReason.Find("started") != kNotFound) {
    snprintf(command, MAX_COMMAND_SIZE - 1, "%s", DUMMY_COMMAND);
  } else {
    snprintf(command, MAX_COMMAND_SIZE - 1, "tether start %s %s", GET_CHAR(mWifiStartIp), GET_CHAR(mWifiEndIp));

    
    
    
    if (!GET_FIELD(mUsbStartIp).IsEmpty() && !GET_FIELD(mUsbEndIp).IsEmpty()) {
      snprintf(command, MAX_COMMAND_SIZE - 1, "%s %s %s", command, GET_CHAR(mUsbStartIp), GET_CHAR(mUsbEndIp));
    }
  }

  doCommand(command, aChain, aCallback);
}

void NetworkUtils::untetherInterface(CommandChain* aChain,
                                     CommandCallback aCallback,
                                     NetworkResultOptions& aResult)
{
  char command[MAX_COMMAND_SIZE];
  snprintf(command, MAX_COMMAND_SIZE - 1, "tether interface remove %s", GET_CHAR(mIfname));

  doCommand(command, aChain, aCallback);
}

void NetworkUtils::setDnsForwarders(CommandChain* aChain,
                                    CommandCallback aCallback,
                                    NetworkResultOptions& aResult)
{
  char command[MAX_COMMAND_SIZE];
  snprintf(command, MAX_COMMAND_SIZE - 1, "tether dns set %s %s", GET_CHAR(mDns1), GET_CHAR(mDns2));

  doCommand(command, aChain, aCallback);
}

void NetworkUtils::enableNat(CommandChain* aChain,
                             CommandCallback aCallback,
                             NetworkResultOptions& aResult)
{
  char command[MAX_COMMAND_SIZE];

  if (!GET_FIELD(mIp).IsEmpty() && !GET_FIELD(mPrefix).IsEmpty()) {
    uint32_t prefix = atoi(GET_CHAR(mPrefix));
    uint32_t ip = inet_addr(GET_CHAR(mIp));
    char* networkAddr = getNetworkAddr(ip, prefix);

    
    snprintf(command, MAX_COMMAND_SIZE - 1, "nat enable %s %s 1 %s/%s",
      GET_CHAR(mInternalIfname), GET_CHAR(mExternalIfname), networkAddr,
      GET_CHAR(mPrefix));
  } else {
    snprintf(command, MAX_COMMAND_SIZE - 1, "nat enable %s %s 0",
      GET_CHAR(mInternalIfname), GET_CHAR(mExternalIfname));
  }

  doCommand(command, aChain, aCallback);
}

void NetworkUtils::disableNat(CommandChain* aChain,
                              CommandCallback aCallback,
                              NetworkResultOptions& aResult)
{
  char command[MAX_COMMAND_SIZE];

  if (!GET_FIELD(mIp).IsEmpty() && !GET_FIELD(mPrefix).IsEmpty()) {
    uint32_t prefix = atoi(GET_CHAR(mPrefix));
    uint32_t ip = inet_addr(GET_CHAR(mIp));
    char* networkAddr = getNetworkAddr(ip, prefix);

    snprintf(command, MAX_COMMAND_SIZE - 1, "nat disable %s %s 1 %s/%s",
      GET_CHAR(mInternalIfname), GET_CHAR(mExternalIfname), networkAddr,
      GET_CHAR(mPrefix));
  } else {
    snprintf(command, MAX_COMMAND_SIZE - 1, "nat disable %s %s 0",
      GET_CHAR(mInternalIfname), GET_CHAR(mExternalIfname));
  }

  doCommand(command, aChain, aCallback);
}

void NetworkUtils::setDefaultInterface(CommandChain* aChain,
                                       CommandCallback aCallback,
                                       NetworkResultOptions& aResult)
{
  char command[MAX_COMMAND_SIZE];
  snprintf(command, MAX_COMMAND_SIZE - 1, "resolver setdefaultif %s", GET_CHAR(mIfname));

  doCommand(command, aChain, aCallback);
}

void NetworkUtils::setInterfaceDns(CommandChain* aChain,
                                   CommandCallback aCallback,
                                   NetworkResultOptions& aResult)
{
  char command[MAX_COMMAND_SIZE];
  int written = snprintf(command, sizeof command, "resolver setifdns %s %s",
                         GET_CHAR(mIfname), GET_CHAR(mDomain));

  nsTArray<nsString>& dnses = GET_FIELD(mDnses);
  uint32_t length = dnses.Length();

  for (uint32_t i = 0; i < length; i++) {
    NS_ConvertUTF16toUTF8 autoDns(dnses[i]);

    int ret = snprintf(command + written, sizeof(command) - written, " %s", autoDns.get());
    if (ret <= 1) {
      command[written] = '\0';
      continue;
    }

    if ((ret + written) >= sizeof(command)) {
      command[written] = '\0';
      break;
    }

    written += ret;
  }

  doCommand(command, aChain, aCallback);
}

#undef GET_CHAR
#undef GET_FIELD




#define ASSIGN_FIELD(prop)  aResult.prop = aChain->getParams().prop;
#define ASSIGN_FIELD_VALUE(prop, value)  aResult.prop = value;

#define RUN_CHAIN(param, cmds, err)                                \
  uint32_t size = sizeof(cmds) / sizeof(CommandFunc);              \
  CommandChain* chain = new CommandChain(param, cmds, size, err);  \
  NetworkResultOptions result;                                     \
  next(chain, false, result);

void NetworkUtils::wifiTetheringFail(NetworkParams& aOptions, NetworkResultOptions& aResult)
{
  
  postMessage(aOptions, aResult);

  
  
  ASSIGN_FIELD_VALUE(mEnable, false)
  RUN_CHAIN(aOptions, sWifiFailChain, nullptr)
}

void NetworkUtils::wifiTetheringSuccess(CommandChain* aChain,
                                        CommandCallback aCallback,
                                        NetworkResultOptions& aResult)
{
  ASSIGN_FIELD(mEnable)
  postMessage(aChain->getParams(), aResult);
}

void NetworkUtils::usbTetheringFail(NetworkParams& aOptions,
                                    NetworkResultOptions& aResult)
{
  
  postMessage(aOptions, aResult);
  
  
  
  {
    aOptions.mEnable = false;
    RUN_CHAIN(aOptions, sUSBFailChain, nullptr)
  }

  
  {
    NetworkParams options;
    options.mEnable = false;
    options.mReport = false;
    gNetworkUtils->enableUsbRndis(options);
  }
}

void NetworkUtils::usbTetheringSuccess(CommandChain* aChain,
                                       CommandCallback aCallback,
                                       NetworkResultOptions& aResult)
{
  ASSIGN_FIELD(mEnable)
  postMessage(aChain->getParams(), aResult);
}

void NetworkUtils::networkInterfaceStatsFail(NetworkParams& aOptions, NetworkResultOptions& aResult)
{
  postMessage(aOptions, aResult);
}

void NetworkUtils::networkInterfaceStatsSuccess(CommandChain* aChain,
                                                CommandCallback aCallback,
                                                NetworkResultOptions& aResult)
{
  ASSIGN_FIELD(mRxBytes)
  ASSIGN_FIELD_VALUE(mTxBytes, atof(NS_ConvertUTF16toUTF8(aResult.mResultReason).get()))
  postMessage(aChain->getParams(), aResult);
}

void NetworkUtils::networkInterfaceAlarmFail(NetworkParams& aOptions, NetworkResultOptions& aResult)
{
  postMessage(aOptions, aResult);
}

void NetworkUtils::networkInterfaceAlarmSuccess(CommandChain* aChain,
                                                CommandCallback aCallback,
                                                NetworkResultOptions& aResult)
{
  
  
  postMessage(aChain->getParams(), aResult);
}

void NetworkUtils::updateUpStreamFail(NetworkParams& aOptions, NetworkResultOptions& aResult)
{
  postMessage(aOptions, aResult);
}

void NetworkUtils::updateUpStreamSuccess(CommandChain* aChain,
                                         CommandCallback aCallback,
                                         NetworkResultOptions& aResult)
{
  ASSIGN_FIELD(mCurExternalIfname)
  ASSIGN_FIELD(mCurInternalIfname)
  postMessage(aChain->getParams(), aResult);
}

void NetworkUtils::setDhcpServerFail(NetworkParams& aOptions, NetworkResultOptions& aResult)
{
  aResult.mSuccess = false;
  postMessage(aOptions, aResult);
}

void NetworkUtils::setDhcpServerSuccess(CommandChain* aChain, CommandCallback aCallback, NetworkResultOptions& aResult)
{
  aResult.mSuccess = true;
  postMessage(aChain->getParams(), aResult);
}

void NetworkUtils::wifiOperationModeFail(NetworkParams& aOptions, NetworkResultOptions& aResult)
{
  postMessage(aOptions, aResult);
}

void NetworkUtils::wifiOperationModeSuccess(CommandChain* aChain,
                                            CommandCallback aCallback,
                                            NetworkResultOptions& aResult)
{
  postMessage(aChain->getParams(), aResult);
}

void NetworkUtils::setDnsFail(NetworkParams& aOptions, NetworkResultOptions& aResult)
{
  postMessage(aOptions, aResult);
}

#undef ASSIGN_FIELD
#undef ASSIGN_FIELD_VALUE

NetworkUtils::NetworkUtils(MessageCallback aCallback)
 : mMessageCallback(aCallback)
{
  mNetUtils = new NetUtils();

  char value[PROPERTY_VALUE_MAX];
  property_get("ro.build.version.sdk", value, nullptr);
  SDK_VERSION = atoi(value);

  gNetworkUtils = this;
}

NetworkUtils::~NetworkUtils()
{
}

#define GET_CHAR(prop) NS_ConvertUTF16toUTF8(aOptions.prop).get()
#define GET_FIELD(prop) aOptions.prop

void NetworkUtils::ExecuteCommand(NetworkParams aOptions)
{
  bool ret = true;

  if (aOptions.mCmd.EqualsLiteral("removeNetworkRoute")) {
    removeNetworkRoute(aOptions);
  } else if (aOptions.mCmd.EqualsLiteral("setDNS")) {
    setDNS(aOptions);
  } else if (aOptions.mCmd.EqualsLiteral("setDefaultRouteAndDNS")) {
    setDefaultRouteAndDNS(aOptions);
  } else if (aOptions.mCmd.EqualsLiteral("removeDefaultRoute")) {
    removeDefaultRoute(aOptions);
  } else if (aOptions.mCmd.EqualsLiteral("addHostRoute")) {
    addHostRoute(aOptions);
  } else if (aOptions.mCmd.EqualsLiteral("removeHostRoute")) {
    removeHostRoute(aOptions);
  } else if (aOptions.mCmd.EqualsLiteral("removeHostRoutes")) {
    removeHostRoutes(aOptions);
  } else if (aOptions.mCmd.EqualsLiteral("addSecondaryRoute")) {
    addSecondaryRoute(aOptions);
  } else if (aOptions.mCmd.EqualsLiteral("removeSecondaryRoute")) {
    removeSecondaryRoute(aOptions);
  } else if (aOptions.mCmd.EqualsLiteral("getNetworkInterfaceStats")) {
    getNetworkInterfaceStats(aOptions);
  } else if (aOptions.mCmd.EqualsLiteral("setNetworkInterfaceAlarm")) {
    setNetworkInterfaceAlarm(aOptions);
  } else if (aOptions.mCmd.EqualsLiteral("enableNetworkInterfaceAlarm")) {
    enableNetworkInterfaceAlarm(aOptions);
  } else if (aOptions.mCmd.EqualsLiteral("disableNetworkInterfaceAlarm")) {
    disableNetworkInterfaceAlarm(aOptions);
  } else if (aOptions.mCmd.EqualsLiteral("setWifiOperationMode")) {
    setWifiOperationMode(aOptions);
  } else if (aOptions.mCmd.EqualsLiteral("setDhcpServer")) {
    setDhcpServer(aOptions);
  } else if (aOptions.mCmd.EqualsLiteral("setWifiTethering")) {
    setWifiTethering(aOptions);
  } else if (aOptions.mCmd.EqualsLiteral("setUSBTethering")) {
    setUSBTethering(aOptions);
  } else if (aOptions.mCmd.EqualsLiteral("enableUsbRndis")) {
    enableUsbRndis(aOptions);
  } else if (aOptions.mCmd.EqualsLiteral("updateUpStream")) {
    updateUpStream(aOptions);
  } else {
    WARN("unknon message");
    return;
  }

  if (!aOptions.mIsAsync) {
    NetworkResultOptions result;
    result.mRet = ret;
    postMessage(aOptions, result);
  }
}




void NetworkUtils::onNetdMessage(NetdCommand* aCommand)
{
  char* data = (char*)aCommand->mData;

  
  char* result = strtok(data, NETD_MESSAGE_DELIMIT);

  if (!result) {
    nextNetdCommand();
    return;
  }
  uint32_t code = atoi(result);

  if (!isBroadcastMessage(code) && SDK_VERSION >= 16) {
    strtok(nullptr, NETD_MESSAGE_DELIMIT);
  }

  char* reason = strtok(nullptr, "\0");

  if (isBroadcastMessage(code)) {
    DEBUG("Receiving broadcast message from netd.");
    DEBUG("          ==> Code: %d  Reason: %s", code, reason);
    sendBroadcastMessage(code, reason);
    nextNetdCommand();
    return;
  }

   
  DEBUG("Receiving \"%s\" command response from netd.", gCurrentCommand.command);
  DEBUG("          ==> Code: %d  Reason: %s", code, reason);

  gReason.AppendElement(nsCString(reason));

  
  
  if (isProceeding(code)) {
    return;
  }

  if (isComplete(code)) {
    gPending = false;
  }

  if (gCurrentCommand.callback) {
    char buf[BUF_SIZE];
    join(gReason, INTERFACE_DELIMIT, BUF_SIZE, buf);

    NetworkResultOptions result;
    result.mResultCode = code;
    result.mResultReason = NS_ConvertUTF8toUTF16(buf);
    (*gCurrentCommand.callback)(gCurrentCommand.chain, isError(code), result);
    gReason.Clear();
  }

  
  if (isComplete(code)) {
    nextNetdCommand();
  }
}




bool NetworkUtils::setDhcpServer(NetworkParams& aOptions)
{
  if (aOptions.mEnabled) {
    aOptions.mWifiStartIp = aOptions.mStartIp;
    aOptions.mWifiEndIp = aOptions.mEndIp;
    aOptions.mIp = aOptions.mServerIp;
    aOptions.mPrefix = aOptions.mMaskLength;
    aOptions.mLink = NS_ConvertUTF8toUTF16("up");

    RUN_CHAIN(aOptions, sStartDhcpServerChain, setDhcpServerFail)
  } else {
    RUN_CHAIN(aOptions, sStopDhcpServerChain, setDhcpServerFail)
  }
  return true;
}




bool NetworkUtils::setDNS(NetworkParams& aOptions)
{
  uint32_t length = aOptions.mDnses.Length();

  if (length > 0) {
    for (uint32_t i = 0; i < length; i++) {
      NS_ConvertUTF16toUTF8 autoDns(aOptions.mDnses[i]);

      char dns_prop_key[PROPERTY_VALUE_MAX];
      snprintf(dns_prop_key, sizeof dns_prop_key, "net.dns%d", i+1);
      property_set(dns_prop_key, autoDns.get());
    }
  } else {
    
    IFProperties interfaceProperties;
    getIFProperties(GET_CHAR(mIfname), interfaceProperties);

    property_set("net.dns1", interfaceProperties.dns1);
    property_set("net.dns2", interfaceProperties.dns2);
  }

  
  char dnschange[PROPERTY_VALUE_MAX];
  property_get("net.dnschange", dnschange, "0");

  char num[PROPERTY_VALUE_MAX];
  snprintf(num, PROPERTY_VALUE_MAX - 1, "%d", atoi(dnschange) + 1);
  property_set("net.dnschange", num);

  
  if (SDK_VERSION >= 18) {
    RUN_CHAIN(aOptions, sSetDnsChain, setDnsFail)
  }

  return true;
}




bool NetworkUtils::setDefaultRouteAndDNS(NetworkParams& aOptions)
{
  NS_ConvertUTF16toUTF8 autoIfname(aOptions.mIfname);

  if (!aOptions.mOldIfname.IsEmpty()) {
    
    mNetUtils->do_ifc_remove_default_route(GET_CHAR(mOldIfname));
    
    mNetUtils->do_ifc_remove_route(GET_CHAR(mOldIfname), "::", 0, NULL);
  }

  uint32_t length = aOptions.mGateways.Length();
  if (length > 0) {
    for (uint32_t i = 0; i < length; i++) {
      NS_ConvertUTF16toUTF8 autoGateway(aOptions.mGateways[i]);

      int type = getIpType(autoGateway.get());
      if (type != AF_INET && type != AF_INET6) {
        continue;
      }

      if (type == AF_INET6) {
        mNetUtils->do_ifc_add_route(autoIfname.get(), "::", 0, autoGateway.get());
      } else { 
        mNetUtils->do_ifc_set_default_route(autoIfname.get(), inet_addr(autoGateway.get()));
      }
    }
  } else {
    
    char key[PROPERTY_KEY_MAX];
    char gateway[PROPERTY_KEY_MAX];

    snprintf(key, sizeof key - 1, "net.%s.gw", autoIfname.get());
    property_get(key, gateway, "");

    int type = getIpType(gateway);
    if (type != AF_INET && type != AF_INET6) {
      return false;
    }

    if (type == AF_INET6) {
      mNetUtils->do_ifc_add_route(autoIfname.get(), "::", 0, gateway);
    } else { 
      mNetUtils->do_ifc_set_default_route(autoIfname.get(), inet_addr(gateway));
    }
  }

  setDNS(aOptions);
  return true;
}




bool NetworkUtils::removeDefaultRoute(NetworkParams& aOptions)
{
  uint32_t length = aOptions.mGateways.Length();
  for (uint32_t i = 0; i < length; i++) {
    NS_ConvertUTF16toUTF8 autoGateway(aOptions.mGateways[i]);

    int type = getIpType(autoGateway.get());
    if (type != AF_INET && type != AF_INET6) {
      return false;
    }

    mNetUtils->do_ifc_remove_route(GET_CHAR(mIfname),
                                   type == AF_INET ? "0.0.0.0" : "::",
                                   0, autoGateway.get());
  }

  return true;
}




bool NetworkUtils::addHostRoute(NetworkParams& aOptions)
{
  NS_ConvertUTF16toUTF8 autoIfname(aOptions.mIfname);
  int type, prefix;

  uint32_t length = aOptions.mHostnames.Length();
  for (uint32_t i = 0; i < length; i++) {
    NS_ConvertUTF16toUTF8 autoHostname(aOptions.mHostnames[i]);

    type = getIpType(autoHostname.get());
    if (type != AF_INET && type != AF_INET6) {
      continue;
    }

    uint32_t index = selectGateway(aOptions.mGateways, type);
    if (index >= aOptions.mGateways.Length()) {
      continue;
    }

    NS_ConvertUTF16toUTF8 autoGateway(aOptions.mGateways[index]);
    prefix = type == AF_INET ? 32 : 128;
    mNetUtils->do_ifc_add_route(autoIfname.get(), autoHostname.get(), prefix,
                                autoGateway.get());
  }
  return true;
}




bool NetworkUtils::removeHostRoute(NetworkParams& aOptions)
{
  NS_ConvertUTF16toUTF8 autoIfname(aOptions.mIfname);
  int type, prefix;

  uint32_t length = aOptions.mHostnames.Length();
  for (uint32_t i = 0; i < length; i++) {
    NS_ConvertUTF16toUTF8 autoHostname(aOptions.mHostnames[i]);

    type = getIpType(autoHostname.get());
    if (type != AF_INET && type != AF_INET6) {
      continue;
    }

    uint32_t index = selectGateway(aOptions.mGateways, type);
    if (index >= aOptions.mGateways.Length()) {
      continue;
    }

    NS_ConvertUTF16toUTF8 autoGateway(aOptions.mGateways[index]);
    prefix = type == AF_INET ? 32 : 128;
    mNetUtils->do_ifc_remove_route(autoIfname.get(), autoHostname.get(), prefix,
                                   autoGateway.get());
  }
  return true;
}




bool NetworkUtils::removeHostRoutes(NetworkParams& aOptions)
{
  mNetUtils->do_ifc_remove_host_routes(GET_CHAR(mIfname));
  return true;
}

bool NetworkUtils::removeNetworkRoute(NetworkParams& aOptions)
{
  NS_ConvertUTF16toUTF8 autoIfname(aOptions.mIfname);
  NS_ConvertUTF16toUTF8 autoIp(aOptions.mIp);

  int type = getIpType(autoIp.get());
  if (type != AF_INET && type != AF_INET6) {
    return false;
  }

  uint32_t prefixLength = GET_FIELD(mPrefixLength);

  if (type == AF_INET6) {
    
    struct in6_addr in6;
    if (inet_pton(AF_INET6, autoIp.get(), &in6) != 1) {
      return false;
    }

    uint32_t p, i, p1, mask;
    p = prefixLength;
    i = 0;
    while (i < 4) {
      p1 = p > 32 ? 32 : p;
      p -= p1;
      mask = p1 ? ~0x0 << (32 - p1) : 0;
      in6.s6_addr32[i++] &= htonl(mask);
    }

    char subnetStr[INET6_ADDRSTRLEN];
    if (!inet_ntop(AF_INET6, &in6, subnetStr, sizeof subnetStr)) {
      return false;
    }

    
    mNetUtils->do_ifc_remove_route(autoIfname.get(), "::", 0, NULL);

    
    mNetUtils->do_ifc_remove_route(autoIfname.get(), subnetStr, prefixLength, NULL);
    return true;
  }

  
  uint32_t ip = inet_addr(autoIp.get());
  uint32_t netmask = makeMask(prefixLength);
  uint32_t subnet = ip & netmask;
  const char* gateway = "0.0.0.0";
  struct in_addr addr;
  addr.s_addr = subnet;
  const char* dst = inet_ntoa(addr);

  mNetUtils->do_ifc_remove_default_route(autoIfname.get());
  mNetUtils->do_ifc_remove_route(autoIfname.get(), dst, prefixLength, gateway);
  return true;
}

bool NetworkUtils::addSecondaryRoute(NetworkParams& aOptions)
{
  char command[MAX_COMMAND_SIZE];
  snprintf(command, MAX_COMMAND_SIZE - 1,
           "interface route add %s secondary %s %s %s",
           GET_CHAR(mIfname),
           GET_CHAR(mIp),
           GET_CHAR(mPrefix),
           GET_CHAR(mGateway));

  doCommand(command, nullptr, nullptr);
  return true;
}

bool NetworkUtils::removeSecondaryRoute(NetworkParams& aOptions)
{
  char command[MAX_COMMAND_SIZE];
  snprintf(command, MAX_COMMAND_SIZE - 1,
           "interface route remove %s secondary %s %s %s",
           GET_CHAR(mIfname),
           GET_CHAR(mIp),
           GET_CHAR(mPrefix),
           GET_CHAR(mGateway));

  doCommand(command, nullptr, nullptr);
  return true;
}

bool NetworkUtils::getNetworkInterfaceStats(NetworkParams& aOptions)
{
  DEBUG("getNetworkInterfaceStats: %s", GET_CHAR(mIfname));
  aOptions.mRxBytes = -1;
  aOptions.mTxBytes = -1;

  RUN_CHAIN(aOptions, sNetworkInterfaceStatsChain, networkInterfaceStatsFail);
  return  true;
}

bool NetworkUtils::setNetworkInterfaceAlarm(NetworkParams& aOptions)
{
  DEBUG("setNetworkInterfaceAlarms: %s", GET_CHAR(mIfname));
  RUN_CHAIN(aOptions, sNetworkInterfaceSetAlarmChain, networkInterfaceAlarmFail);
  return true;
}

bool NetworkUtils::enableNetworkInterfaceAlarm(NetworkParams& aOptions)
{
  DEBUG("enableNetworkInterfaceAlarm: %s", GET_CHAR(mIfname));
  RUN_CHAIN(aOptions, sNetworkInterfaceEnableAlarmChain, networkInterfaceAlarmFail);
  return true;
}

bool NetworkUtils::disableNetworkInterfaceAlarm(NetworkParams& aOptions)
{
  DEBUG("disableNetworkInterfaceAlarms: %s", GET_CHAR(mIfname));
  RUN_CHAIN(aOptions, sNetworkInterfaceDisableAlarmChain, networkInterfaceAlarmFail);
  return true;
}




bool NetworkUtils::setWifiOperationMode(NetworkParams& aOptions)
{
  DEBUG("setWifiOperationMode: %s %s", GET_CHAR(mIfname), GET_CHAR(mMode));
  RUN_CHAIN(aOptions, sWifiOperationModeChain, wifiOperationModeFail);
  return true;
}




bool NetworkUtils::setWifiTethering(NetworkParams& aOptions)
{
  bool enable = aOptions.mEnable;
  IFProperties interfaceProperties;
  getIFProperties(GET_CHAR(mExternalIfname), interfaceProperties);

  if (strcmp(interfaceProperties.dns1, "")) {
    aOptions.mDns1 = NS_ConvertUTF8toUTF16(interfaceProperties.dns1);
  }
  if (strcmp(interfaceProperties.dns2, "")) {
    aOptions.mDns2 = NS_ConvertUTF8toUTF16(interfaceProperties.dns2);
  }
  dumpParams(aOptions, "WIFI");

  if (enable) {
    DEBUG("Starting Wifi Tethering on %s <-> %s",
           GET_CHAR(mInternalIfname), GET_CHAR(mExternalIfname));
    RUN_CHAIN(aOptions, sWifiEnableChain, wifiTetheringFail)
  } else {
    DEBUG("Stopping Wifi Tethering on %s <-> %s",
           GET_CHAR(mInternalIfname), GET_CHAR(mExternalIfname));
    RUN_CHAIN(aOptions, sWifiDisableChain, wifiTetheringFail)
  }
  return true;
}

bool NetworkUtils::setUSBTethering(NetworkParams& aOptions)
{
  bool enable = aOptions.mEnable;
  IFProperties interfaceProperties;
  getIFProperties(GET_CHAR(mExternalIfname), interfaceProperties);

  if (strcmp(interfaceProperties.dns1, "")) {
    aOptions.mDns1 = NS_ConvertUTF8toUTF16(interfaceProperties.dns1);
  }
  if (strcmp(interfaceProperties.dns2, "")) {
    aOptions.mDns2 = NS_ConvertUTF8toUTF16(interfaceProperties.dns2);
  }
  dumpParams(aOptions, "USB");

  if (enable) {
    DEBUG("Starting USB Tethering on %s <-> %s",
           GET_CHAR(mInternalIfname), GET_CHAR(mExternalIfname));
    RUN_CHAIN(aOptions, sUSBEnableChain, usbTetheringFail)
  } else {
    DEBUG("Stopping USB Tethering on %s <-> %s",
           GET_CHAR(mInternalIfname), GET_CHAR(mExternalIfname));
    RUN_CHAIN(aOptions, sUSBDisableChain, usbTetheringFail)
  }
  return true;
}

void NetworkUtils::checkUsbRndisState(NetworkParams& aOptions)
{
  static uint32_t retry = 0;

  char currentState[PROPERTY_VALUE_MAX];
  property_get(SYS_USB_STATE_PROPERTY, currentState, nullptr);

  nsTArray<nsCString> stateFuncs;
  split(currentState, USB_CONFIG_DELIMIT, stateFuncs);
  bool rndisPresent = stateFuncs.Contains(nsCString(USB_FUNCTION_RNDIS));

  if (aOptions.mEnable == rndisPresent) {
    NetworkResultOptions result;
    result.mEnable = aOptions.mEnable;
    result.mResult = true;
    postMessage(aOptions, result);
    retry = 0;
    return;
  }
  if (retry < USB_FUNCTION_RETRY_TIMES) {
    retry++;
    usleep(USB_FUNCTION_RETRY_INTERVAL * 1000);
    checkUsbRndisState(aOptions);
    return;
  }

  NetworkResultOptions result;
  result.mResult = false;
  postMessage(aOptions, result);
  retry = 0;
}




bool NetworkUtils::enableUsbRndis(NetworkParams& aOptions)
{
  bool report = aOptions.mReport;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  char currentConfig[PROPERTY_VALUE_MAX];
  property_get(SYS_USB_CONFIG_PROPERTY, currentConfig, nullptr);

  nsTArray<nsCString> configFuncs;
  split(currentConfig, USB_CONFIG_DELIMIT, configFuncs);

  char persistConfig[PROPERTY_VALUE_MAX];
  property_get(PERSIST_SYS_USB_CONFIG_PROPERTY, persistConfig, nullptr);

  nsTArray<nsCString> persistFuncs;
  split(persistConfig, USB_CONFIG_DELIMIT, persistFuncs);

  if (aOptions.mEnable) {
    configFuncs.Clear();
    configFuncs.AppendElement(nsCString(USB_FUNCTION_RNDIS));
    if (persistFuncs.Contains(nsCString(USB_FUNCTION_ADB))) {
      configFuncs.AppendElement(nsCString(USB_FUNCTION_ADB));
    }
  } else {
    
    
    
    configFuncs = persistFuncs;
  }

  char newConfig[PROPERTY_VALUE_MAX] = "";
  property_get(SYS_USB_CONFIG_PROPERTY, currentConfig, nullptr);
  join(configFuncs, USB_CONFIG_DELIMIT, PROPERTY_VALUE_MAX, newConfig);
  if (strcmp(currentConfig, newConfig)) {
    property_set(SYS_USB_CONFIG_PROPERTY, newConfig);
  }

  
  if (report) {
    usleep(USB_FUNCTION_RETRY_INTERVAL * 1000);
    checkUsbRndisState(aOptions);
  }
  return true;
}




bool NetworkUtils::updateUpStream(NetworkParams& aOptions)
{
  RUN_CHAIN(aOptions, sUpdateUpStreamChain, updateUpStreamFail)
  return true;
}

void NetworkUtils::sendBroadcastMessage(uint32_t code, char* reason)
{
  NetworkResultOptions result;
  switch(code) {
    case NETD_COMMAND_INTERFACE_CHANGE:
      result.mTopic = NS_ConvertUTF8toUTF16("netd-interface-change");
      break;
    case NETD_COMMAND_BANDWIDTH_CONTROLLER:
      result.mTopic = NS_ConvertUTF8toUTF16("netd-bandwidth-control");
      break;
    default:
      return;
  }

  result.mBroadcast = true;
  result.mReason = NS_ConvertUTF8toUTF16(reason);
  postMessage(result);
}

inline uint32_t NetworkUtils::netdResponseType(uint32_t code)
{
  return (code / 100) * 100;
}

inline bool NetworkUtils::isBroadcastMessage(uint32_t code)
{
  uint32_t type = netdResponseType(code);
  return type == NETD_COMMAND_UNSOLICITED;
}

inline bool NetworkUtils::isError(uint32_t code)
{
  uint32_t type = netdResponseType(code);
  return type != NETD_COMMAND_PROCEEDING && type != NETD_COMMAND_OKAY;
}

inline bool NetworkUtils::isComplete(uint32_t code)
{
  uint32_t type = netdResponseType(code);
  return type != NETD_COMMAND_PROCEEDING;
}

inline bool NetworkUtils::isProceeding(uint32_t code)
{
  uint32_t type = netdResponseType(code);
  return type == NETD_COMMAND_PROCEEDING;
}

void NetworkUtils::dumpParams(NetworkParams& aOptions, const char* aType)
{
#ifdef _DEBUG
  DEBUG("Dump params:");
  DEBUG("     ifname: %s", GET_CHAR(mIfname));
  DEBUG("     ip: %s", GET_CHAR(mIp));
  DEBUG("     link: %s", GET_CHAR(mLink));
  DEBUG("     prefix: %s", GET_CHAR(mPrefix));
  DEBUG("     wifiStartIp: %s", GET_CHAR(mWifiStartIp));
  DEBUG("     wifiEndIp: %s", GET_CHAR(mWifiEndIp));
  DEBUG("     usbStartIp: %s", GET_CHAR(mUsbStartIp));
  DEBUG("     usbEndIp: %s", GET_CHAR(mUsbEndIp));
  DEBUG("     dnsserver1: %s", GET_CHAR(mDns1));
  DEBUG("     dnsserver2: %s", GET_CHAR(mDns2));
  DEBUG("     internalIfname: %s", GET_CHAR(mInternalIfname));
  DEBUG("     externalIfname: %s", GET_CHAR(mExternalIfname));
  if (!strcmp(aType, "WIFI")) {
    DEBUG("     wifictrlinterfacename: %s", GET_CHAR(mWifictrlinterfacename));
    DEBUG("     ssid: %s", GET_CHAR(mSsid));
    DEBUG("     security: %s", GET_CHAR(mSecurity));
    DEBUG("     key: %s", GET_CHAR(mKey));
  }
#endif
}

#undef GET_CHAR
#undef GET_FIELD
