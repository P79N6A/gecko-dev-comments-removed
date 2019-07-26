






















#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/socket.h>
#ifdef MOZ_B2G_BT_BLUEZ
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/sco.h>
#endif
#include "BluetoothUnixSocketConnector.h"
#include "nsThreadUtils.h"

using namespace mozilla::ipc;
USING_BLUETOOTH_NAMESPACE

static const int RFCOMM_SO_SNDBUF = 70 * 1024;  
static const int L2CAP_SO_SNDBUF = 400 * 1024;  
static const int L2CAP_SO_RCVBUF = 400 * 1024;  
static const int L2CAP_MAX_MTU = 65000;

#ifdef MOZ_B2G_BT_BLUEZ
static
int get_bdaddr(const char *str, bdaddr_t *ba)
{
  char *d = ((char*)ba) + 5, *endp;
  for (int i = 0; i < 6; i++) {
    *d-- = strtol(str, &endp, 16);
    MOZ_ASSERT(!(*endp != ':' && i != 5));
    str = endp + 1;
  }
  return 0;
}

static
void get_bdaddr_as_string(const bdaddr_t *ba, char *str) {
    const uint8_t *b = (const uint8_t *)ba;
    sprintf(str, "%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X",
            b[5], b[4], b[3], b[2], b[1], b[0]);
}

#endif

BluetoothUnixSocketConnector::BluetoothUnixSocketConnector(
  BluetoothSocketType aType,
  int aChannel,
  bool aAuth,
  bool aEncrypt) : mType(aType)
                 , mChannel(aChannel)
                 , mAuth(aAuth)
                 , mEncrypt(aEncrypt)
{
}

bool
BluetoothUnixSocketConnector::SetUp(int aFd)
{
#ifdef MOZ_B2G_BT_BLUEZ
  int lm = 0;
  int sndbuf, rcvbuf;

  
  switch (mType) {
  case BluetoothSocketType::RFCOMM:
    lm |= mAuth ? RFCOMM_LM_AUTH : 0;
    lm |= mEncrypt ? RFCOMM_LM_ENCRYPT : 0;
    break;
  case BluetoothSocketType::L2CAP:
  case BluetoothSocketType::EL2CAP:
    lm |= mAuth ? L2CAP_LM_AUTH : 0;
    lm |= mEncrypt ? L2CAP_LM_ENCRYPT : 0;
    break;
  case BluetoothSocketType::SCO:
    break;
  default:
    MOZ_CRASH("Unknown socket type!");
  }

  if (lm) {
    if (mType == BluetoothSocketType::RFCOMM) {
      if (setsockopt(aFd, SOL_RFCOMM, RFCOMM_LM, &lm, sizeof(lm))) {
        BT_WARNING("setsockopt(RFCOMM_LM) failed, throwing");
        return false;
      }
    } else if (mType == BluetoothSocketType::L2CAP ||
               mType == BluetoothSocketType::EL2CAP) {
      if (setsockopt(aFd, SOL_L2CAP, L2CAP_LM, &lm, sizeof(lm))) {
        BT_WARNING("setsockopt(L2CAP_LM) failed, throwing");
        return false;
      }
    }
  }

  if (mType == BluetoothSocketType::RFCOMM) {
    sndbuf = RFCOMM_SO_SNDBUF;
    if (setsockopt(aFd, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf))) {
      BT_WARNING("setsockopt(SO_SNDBUF) failed, throwing");
      return false;
    }
  }

  
  if (mType == BluetoothSocketType::L2CAP ||
      mType == BluetoothSocketType::EL2CAP) {
    struct l2cap_options opts;
    socklen_t optlen = sizeof(opts);
    int err;
    err = getsockopt(aFd, SOL_L2CAP, L2CAP_OPTIONS, &opts, &optlen);
    if (!err) {
      
      opts.omtu = opts.imtu = L2CAP_MAX_MTU;

      
      if (mType == BluetoothSocketType::EL2CAP) {
        opts.flush_to = 0xffff; 
        opts.mode = L2CAP_MODE_ERTM;
        opts.fcs = 1;
        opts.txwin_size = 64;
        opts.max_tx = 10;
      }

      err = setsockopt(aFd, SOL_L2CAP, L2CAP_OPTIONS, &opts, optlen);
    }

    
    if (mType == BluetoothSocketType::EL2CAP) {
      sndbuf = L2CAP_SO_SNDBUF;
      if (setsockopt(aFd, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf))) {
        BT_WARNING("setsockopt(SO_SNDBUF) failed, throwing");
        return false;
      }

      rcvbuf = L2CAP_SO_RCVBUF;
      if (setsockopt(aFd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf))) {
        BT_WARNING("setsockopt(SO_RCVBUF) failed, throwing");
        return false;
      }
    }
  }
#endif
  return true;
}

bool
BluetoothUnixSocketConnector::SetUpListenSocket(int aFd)
{
  
  return true;
}

int
BluetoothUnixSocketConnector::Create()
{
  MOZ_ASSERT(!NS_IsMainThread());
  int fd = -1;

#ifdef MOZ_B2G_BT_BLUEZ
  switch (mType) {
  case BluetoothSocketType::RFCOMM:
    fd = socket(PF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
    break;
  case BluetoothSocketType::SCO:
    fd = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_SCO);
    break;
  case BluetoothSocketType::L2CAP:
    fd = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    break;
  case BluetoothSocketType::EL2CAP:
    fd = socket(PF_BLUETOOTH, SOCK_STREAM, BTPROTO_L2CAP);
    break;
  default:
    MOZ_CRASH();
  }

  if (fd < 0) {
    BT_WARNING("Could not open bluetooth socket!");
    return -1;
  }

  if (!SetUp(fd)) {
    BT_WARNING("Could not set up socket!");
    return -1;
  }
#endif
  return fd;
}

bool
BluetoothUnixSocketConnector::CreateAddr(bool aIsServer,
                                         socklen_t& aAddrSize,
                                         sockaddr_any& aAddr,
                                         const char* aAddress)
{
#ifdef MOZ_B2G_BT_BLUEZ
  
  bdaddr_t bd_address_obj = {{0, 0, 0, 0, 0, 0}};

  if (!aIsServer && aAddress && strlen(aAddress) > 0) {
    if (get_bdaddr(aAddress, &bd_address_obj)) {
      BT_WARNING("Can't get bluetooth address!");
      return false;
    }
  }

  
  memset(&aAddr, 0, sizeof(aAddr));

  switch (mType) {
  case BluetoothSocketType::RFCOMM:
    struct sockaddr_rc addr_rc;
    aAddrSize = sizeof(addr_rc);
    aAddr.rc.rc_family = AF_BLUETOOTH;
    aAddr.rc.rc_channel = mChannel;
    memcpy(&aAddr.rc.rc_bdaddr, &bd_address_obj, sizeof(bd_address_obj));
    break;
  case BluetoothSocketType::L2CAP:
  case BluetoothSocketType::EL2CAP:
    struct sockaddr_l2 addr_l2;
    aAddrSize = sizeof(addr_l2);
    aAddr.l2.l2_family = AF_BLUETOOTH;
    aAddr.l2.l2_psm = mChannel;
    memcpy(&aAddr.l2.l2_bdaddr, &bd_address_obj, sizeof(bdaddr_t));
    break;
  case BluetoothSocketType::SCO:
    struct sockaddr_sco addr_sco;
    aAddrSize = sizeof(addr_sco);
    aAddr.sco.sco_family = AF_BLUETOOTH;
    memcpy(&aAddr.sco.sco_bdaddr, &bd_address_obj, sizeof(bd_address_obj));
    break;
  default:
    BT_WARNING("Socket type unknown!");
    return false;
  }
#endif
  return true;
}

void
BluetoothUnixSocketConnector::GetSocketAddr(const sockaddr_any& aAddr,
                                            nsAString& aAddrStr)
{
#ifdef MOZ_B2G_BT_BLUEZ
  char addr[18];
  switch (mType) {
  case BluetoothSocketType::RFCOMM:
    get_bdaddr_as_string((bdaddr_t*)(&aAddr.rc.rc_bdaddr), addr);
    break;
  case BluetoothSocketType::SCO:
    get_bdaddr_as_string((bdaddr_t*)(&aAddr.sco.sco_bdaddr), addr);
    break;
  case BluetoothSocketType::L2CAP:
  case BluetoothSocketType::EL2CAP:
    get_bdaddr_as_string((bdaddr_t*)(&aAddr.l2.l2_bdaddr), addr);
    break;
  default:
    MOZ_CRASH("Socket should be either RFCOMM or SCO!");
  }
  aAddrStr.AssignASCII(addr);
#endif
}
