






#include "KeyStoreConnector.h"
#include <fcntl.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/un.h>
#include "nsThreadUtils.h" 

#ifdef MOZ_WIDGET_GONK
#include <android/log.h>
#define KEYSTORE_LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "Gonk", args)
#else
#define KEYSTORE_LOG(args...)  printf(args);
#endif

namespace mozilla {
namespace ipc {

static const char KEYSTORE_SOCKET_PATH[] = "/dev/socket/keystore";

KeyStoreConnector::KeyStoreConnector(const char** const aAllowedUsers)
  : mAllowedUsers(aAllowedUsers)
{ }

KeyStoreConnector::~KeyStoreConnector()
{ }

nsresult
KeyStoreConnector::CreateSocket(int& aFd) const
{
  unlink(KEYSTORE_SOCKET_PATH);

  aFd = socket(AF_LOCAL, SOCK_STREAM, 0);
  if (aFd < 0) {
    KEYSTORE_LOG("Could not open KeyStore socket!");
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

nsresult
KeyStoreConnector::SetSocketFlags(int aFd) const
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
KeyStoreConnector::CheckPermission(int aFd) const
{
  struct ucred userCred;
  socklen_t len = sizeof(userCred);

  if (getsockopt(aFd, SOL_SOCKET, SO_PEERCRED, &userCred, &len)) {
    return NS_ERROR_FAILURE;
  }

  const struct passwd* userInfo = getpwuid(userCred.uid);
  if (!userInfo) {
    return NS_ERROR_FAILURE;
  }

  if (!mAllowedUsers) {
    return NS_ERROR_FAILURE;
  }

  for (const char** user = mAllowedUsers; *user; ++user) {
    if (!strcmp(*user, userInfo->pw_name)) {
      return NS_OK;
    }
  }

  return NS_ERROR_FAILURE;
}

nsresult
KeyStoreConnector::CreateAddress(struct sockaddr& aAddress,
                                 socklen_t& aAddressLength) const
{
  struct sockaddr_un* address =
    reinterpret_cast<struct sockaddr_un*>(&aAddress);

  size_t namesiz = strlen(KEYSTORE_SOCKET_PATH) + 1; 

  if (namesiz > sizeof(address->sun_path)) {
      KEYSTORE_LOG("Address too long for socket struct!");
      return NS_ERROR_FAILURE;
  }

  address->sun_family = AF_UNIX;
  memcpy(address->sun_path, KEYSTORE_SOCKET_PATH, namesiz);

  aAddressLength = offsetof(struct sockaddr_un, sun_path) + namesiz;

  return NS_OK;
}



nsresult
KeyStoreConnector::ConvertAddressToString(const struct sockaddr& aAddress,
                                          socklen_t aAddressLength,
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
KeyStoreConnector::CreateListenSocket(struct sockaddr* aAddress,
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

  
  
  
  
  
  chmod(KEYSTORE_SOCKET_PATH, S_IRUSR|S_IWUSR|
                              S_IRGRP|S_IWGRP|
                              S_IROTH|S_IWOTH);

  aListenFd = fd.forget();

  return NS_OK;
}

nsresult
KeyStoreConnector::AcceptStreamSocket(int aListenFd,
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
  rv = CheckPermission(fd);
  if (NS_FAILED(rv)) {
    return rv;
  }

  aStreamFd = fd.forget();

  return NS_OK;
}

nsresult
KeyStoreConnector::CreateStreamSocket(struct sockaddr* aAddress,
                                      socklen_t* aAddressLength,
                                      int& aStreamFd)
{
  MOZ_CRASH("|KeyStoreConnector| does not support creating stream sockets.");
  return NS_ERROR_FAILURE;
}



static const char* KEYSTORE_ALLOWED_USERS[] = {
  "root",
  "wifi",
  NULL
};

static bool checkPermission(uid_t uid)
{
  struct passwd *userInfo = getpwuid(uid);
  for (const char **user = KEYSTORE_ALLOWED_USERS; *user; user++ ) {
    if (!strcmp(*user, userInfo->pw_name)) {
      return true;
    }
  }

  return false;
}

int
KeyStoreConnector::Create()
{
  MOZ_ASSERT(!NS_IsMainThread());

  int fd;

  unlink(KEYSTORE_SOCKET_PATH);

  fd = socket(AF_LOCAL, SOCK_STREAM, 0);

  if (fd < 0) {
    NS_WARNING("Could not open keystore socket!");
    return -1;
  }

  return fd;
}

bool
KeyStoreConnector::CreateAddr(bool aIsServer,
                              socklen_t& aAddrSize,
                              sockaddr_any& aAddr,
                              const char* aAddress)
{
  
  MOZ_ASSERT(aIsServer);

  aAddr.un.sun_family = AF_LOCAL;
  if(strlen(KEYSTORE_SOCKET_PATH) > sizeof(aAddr.un.sun_path)) {
      NS_WARNING("Address too long for socket struct!");
      return false;
  }
  strcpy((char*)&aAddr.un.sun_path, KEYSTORE_SOCKET_PATH);
  aAddrSize = strlen(KEYSTORE_SOCKET_PATH) + offsetof(struct sockaddr_un, sun_path) + 1;

  return true;
}

bool
KeyStoreConnector::SetUp(int aFd)
{
  
  struct ucred userCred;
  socklen_t len = sizeof(struct ucred);

  if (getsockopt(aFd, SOL_SOCKET, SO_PEERCRED, &userCred, &len)) {
    return false;
  }

  return checkPermission(userCred.uid);
}

bool
KeyStoreConnector::SetUpListenSocket(int aFd)
{
  
  chmod(KEYSTORE_SOCKET_PATH, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);

  return true;
}

void
KeyStoreConnector::GetSocketAddr(const sockaddr_any& aAddr,
                                 nsAString& aAddrStr)
{
  
  MOZ_CRASH("This should never be called!");
}

}
}
