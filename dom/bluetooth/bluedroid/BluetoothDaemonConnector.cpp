






#include "BluetoothDaemonConnector.h"
#include <fcntl.h>
#include "nsISupportsImpl.h"

BEGIN_BLUETOOTH_NAMESPACE

BluetoothDaemonConnector::BluetoothDaemonConnector(
  const nsACString& aSocketName)
  : mSocketName(aSocketName)
{
  MOZ_COUNT_CTOR_INHERITED(BluetoothDaemonConnector, UnixSocketConnector);
}

BluetoothDaemonConnector::~BluetoothDaemonConnector()
{
  MOZ_COUNT_DTOR_INHERITED(BluetoothDaemonConnector, UnixSocketConnector);
}

nsresult
BluetoothDaemonConnector::CreateSocket(int& aFd) const
{
  aFd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
  if (aFd < 0) {
    BT_WARNING("Could not open Bluetooth daemon socket!");
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

nsresult
BluetoothDaemonConnector::SetSocketFlags(int aFd) const
{
  static const int sReuseAddress = 1;

  
  int flags = TEMP_FAILURE_RETRY(fcntl(aFd, F_GETFD));
  if (flags < 0) {
    return NS_ERROR_FAILURE;
  }
  flags |= FD_CLOEXEC;
  int res = TEMP_FAILURE_RETRY(fcntl(aFd, F_SETFD, flags));
  if (res < 0) {
    return NS_ERROR_FAILURE;
  }

  
  flags = TEMP_FAILURE_RETRY(fcntl(aFd, F_GETFL));
  if (flags < 0) {
    return NS_ERROR_FAILURE;
  }
  flags |= O_NONBLOCK;
  res = TEMP_FAILURE_RETRY(fcntl(aFd, F_SETFL, flags));
  if (res < 0) {
    return NS_ERROR_FAILURE;
  }

  
  res = setsockopt(aFd, SOL_SOCKET, SO_REUSEADDR, &sReuseAddress,
                   sizeof(sReuseAddress));
  if (res < 0) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

nsresult
BluetoothDaemonConnector::CreateAddress(struct sockaddr& aAddress,
                                        socklen_t& aAddressLength) const
{
  static const size_t sNameOffset = 1;

  struct sockaddr_un* address =
    reinterpret_cast<struct sockaddr_un*>(&aAddress);

  size_t namesiz = mSocketName.Length() + 1; 

  if (NS_WARN_IF((sNameOffset + namesiz) > sizeof(address->sun_path))) {
    return NS_ERROR_FAILURE;
  }

  address->sun_family = AF_UNIX;
  memset(address->sun_path, '\0', sNameOffset); 
  memcpy(address->sun_path + sNameOffset, mSocketName.get(), namesiz);

  aAddressLength =
    offsetof(struct sockaddr_un, sun_path) + sNameOffset + namesiz;

  return NS_OK;
}



nsresult
BluetoothDaemonConnector::ConvertAddressToString(
  const struct sockaddr& aAddress, socklen_t aAddressLength,
  nsACString& aAddressString)
{
  MOZ_ASSERT(aAddress.sa_family == AF_UNIX);

  const struct sockaddr_un* un =
    reinterpret_cast<const struct sockaddr_un*>(&aAddress);

  size_t len = aAddressLength - offsetof(struct sockaddr_un, sun_path);

  aAddressString.Assign(un->sun_path, len);

  return NS_OK;
}

nsresult
BluetoothDaemonConnector::CreateListenSocket(struct sockaddr* aAddress,
                                             socklen_t* aAddressLength,
                                             int& aListenFd)
{
  ScopedClose fd;

  nsresult rv = CreateSocket(fd.rwget());
  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = SetSocketFlags(fd);
  if (NS_FAILED(rv)) {
    return rv;
  }
  if (aAddress && aAddressLength) {
    rv = CreateAddress(*aAddress, *aAddressLength);
    if (NS_FAILED(rv)) {
      return rv;
    }
  }

  aListenFd = fd.forget();

  return NS_OK;
}

nsresult
BluetoothDaemonConnector::AcceptStreamSocket(int aListenFd,
                                             struct sockaddr* aAddress,
                                             socklen_t* aAddressLength,
                                             int& aStreamFd)
{
  ScopedClose fd(
    TEMP_FAILURE_RETRY(accept(aListenFd, aAddress, aAddressLength)));
  if (fd < 0) {
    NS_WARNING("Cannot accept file descriptor!");
    return NS_ERROR_FAILURE;
  }
  nsresult rv = SetSocketFlags(fd);
  if (NS_FAILED(rv)) {
    return rv;
  }

  aStreamFd = fd.forget();

  return NS_OK;
}

nsresult
BluetoothDaemonConnector::CreateStreamSocket(struct sockaddr* aAddress,
                                             socklen_t* aAddressLength,
                                             int& aStreamFd)
{
  MOZ_CRASH("|BluetoothDaemonConnector| does not support "
            "creating stream sockets.");
  return NS_ERROR_ABORT;
}

END_BLUETOOTH_NAMESPACE
