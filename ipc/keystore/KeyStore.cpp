





#include <fcntl.h>
#include <limits.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>

#undef CHROMIUM_LOG
#if defined(MOZ_WIDGET_GONK)
#include <android/log.h>
#define CHROMIUM_LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "Gonk", args)
#else
#define CHROMIUM_LOG(args...)  printf(args);
#endif

#include "KeyStore.h"
#include "jsfriendapi.h"
#include "MainThreadUtils.h" 

#include "plbase64.h"
#include "certdb.h"
#include "ScopedNSSTypes.h"

using namespace mozilla::ipc;
#if ANDROID_VERSION >= 18

#include <android/log.h>
#include <binder/BinderService.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <security/keystore/include/keystore/IKeystoreService.h>
#include <security/keystore/include/keystore/keystore.h>

using namespace android;

namespace android {

class BpKeystoreService : public BpInterface<IKeystoreService>
{
public:
  BpKeystoreService(const sp<IBinder>& impl)
    : BpInterface<IKeystoreService>(impl)
  {
  }

  virtual int32_t get(const String16& name, uint8_t** item, size_t* itemLength) {return 0;}
  virtual int32_t test() {return 0;}
  virtual int32_t insert(const String16& name, const uint8_t* item, size_t itemLength, int uid, int32_t flags) {return 0;}
  virtual int32_t del(const String16& name, int uid) {return 0;}
  virtual int32_t exist(const String16& name, int uid) {return 0;}
  virtual int32_t saw(const String16& name, int uid, Vector<String16>* matches) {return 0;}
  virtual int32_t reset() {return 0;}
  virtual int32_t password(const String16& password) {return 0;}
  virtual int32_t lock() {return 0;}
  virtual int32_t unlock(const String16& password) {return 0;}
  virtual int32_t zero() {return 0;}
  virtual int32_t import(const String16& name, const uint8_t* data, size_t length, int uid, int32_t flags) {return 0;}
  virtual int32_t sign(const String16& name, const uint8_t* data, size_t length, uint8_t** out, size_t* outLength) {return 0;}
  virtual int32_t verify(const String16& name, const uint8_t* data, size_t dataLength, const uint8_t* signature, size_t signatureLength) {return 0;}
  virtual int32_t get_pubkey(const String16& name, uint8_t** pubkey, size_t* pubkeyLength) {return 0;}
  virtual int32_t del_key(const String16& name, int uid) {return 0;}
  virtual int32_t grant(const String16& name, int32_t granteeUid) {return 0;}
  virtual int32_t ungrant(const String16& name, int32_t granteeUid) {return 0;}
  virtual int64_t getmtime(const String16& name) {return 0;}
  virtual int32_t duplicate(const String16& srcKey, int32_t srcUid, const String16& destKey, int32_t destUid) {return 0;}
  virtual int32_t clear_uid(int64_t uid) {return 0;}
#if ANDROID_VERSION >= 21
  virtual int32_t generate(const String16& name, int32_t uid, int32_t keyType, int32_t keySize, int32_t flags, Vector<sp<KeystoreArg> >* args) {return 0;}
  virtual int32_t is_hardware_backed(const String16& keyType) {return 0;}
  virtual int32_t reset_uid(int32_t uid) {return 0;}
  virtual int32_t sync_uid(int32_t sourceUid, int32_t targetUid) {return 0;}
  virtual int32_t password_uid(const String16& password, int32_t uid) {return 0;}
#elif ANDROID_VERSION == 18
  virtual int32_t generate(const String16& name, int uid, int32_t flags) {return 0;}
  virtual int32_t is_hardware_backed() {return 0;}
#else
  virtual int32_t generate(const String16& name, int32_t uid, int32_t keyType, int32_t keySize, int32_t flags, Vector<sp<KeystoreArg> >* args) {return 0;}
  virtual int32_t is_hardware_backed(const String16& keyType) {return 0;}
#endif
};

IMPLEMENT_META_INTERFACE(KeystoreService, "android.security.keystore");


status_t BnKeystoreService::onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
  switch(code) {
    case TEST: {
      CHECK_INTERFACE(IKeystoreService, data, reply);
      reply->writeNoException();
      reply->writeInt32(test());
      return NO_ERROR;
    } break;
    case GET: {
      CHECK_INTERFACE(IKeystoreService, data, reply);
      String16 name = data.readString16();
      String8 tmp(name);
      uint8_t* data = NULL;
      size_t dataLength = 0;
      int32_t ret = get(name, &data, &dataLength);

      reply->writeNoException();
      if (ret == 1) {
        reply->writeInt32(dataLength);
        void* buf = reply->writeInplace(dataLength);
        memcpy(buf, data, dataLength);
        free(data);
      } else {
        reply->writeInt32(-1);
      }
      return NO_ERROR;
    } break;
    default:
      return NO_ERROR;
  }
}


class KeyStoreService: public BnKeystoreService
{
public:
  int32_t test() {
    uid_t callingUid = IPCThreadState::self()->getCallingUid();
    if (!mozilla::ipc::checkPermission(callingUid)) {
      return ::PERMISSION_DENIED;
    }

    return ::NO_ERROR;
  }

  int32_t get(const String16& name, uint8_t** item, size_t* itemLength) {
    uid_t callingUid = IPCThreadState::self()->getCallingUid();
    if (!mozilla::ipc::checkPermission(callingUid)) {
      return ::PERMISSION_DENIED;
    }

    String8 certName(name);
    return mozilla::ipc::getCertificate(certName.string(), (const uint8_t **)item, (int *)itemLength);
  }

  int32_t insert(const String16& name, const uint8_t* item, size_t itemLength, int uid, int32_t flags) {return ::UNDEFINED_ACTION;}
  int32_t del(const String16& name, int uid) {return ::UNDEFINED_ACTION;}
  int32_t exist(const String16& name, int uid) {return ::UNDEFINED_ACTION;}
  int32_t saw(const String16& name, int uid, Vector<String16>* matches) {return ::UNDEFINED_ACTION;}
  int32_t reset() {return ::UNDEFINED_ACTION;}
  int32_t password(const String16& password) {return ::UNDEFINED_ACTION;}
  int32_t lock() {return ::UNDEFINED_ACTION;}
  int32_t unlock(const String16& password) {return ::UNDEFINED_ACTION;}
  int32_t zero() {return ::UNDEFINED_ACTION;}
  int32_t import(const String16& name, const uint8_t* data, size_t length, int uid, int32_t flags) {return ::UNDEFINED_ACTION;}
  int32_t sign(const String16& name, const uint8_t* data, size_t length, uint8_t** out, size_t* outLength) {return ::UNDEFINED_ACTION;}
  int32_t verify(const String16& name, const uint8_t* data, size_t dataLength, const uint8_t* signature, size_t signatureLength) {return ::UNDEFINED_ACTION;}
  int32_t get_pubkey(const String16& name, uint8_t** pubkey, size_t* pubkeyLength) {return ::UNDEFINED_ACTION;}
  int32_t del_key(const String16& name, int uid) {return ::UNDEFINED_ACTION;}
  int32_t grant(const String16& name, int32_t granteeUid) {return ::UNDEFINED_ACTION;}
  int32_t ungrant(const String16& name, int32_t granteeUid) {return ::UNDEFINED_ACTION;}
  int64_t getmtime(const String16& name) {return ::UNDEFINED_ACTION;}
  int32_t duplicate(const String16& srcKey, int32_t srcUid, const String16& destKey, int32_t destUid) {return ::UNDEFINED_ACTION;}
  int32_t clear_uid(int64_t uid) {return ::UNDEFINED_ACTION;}
#if ANDROID_VERSION >= 21
  virtual int32_t generate(const String16& name, int32_t uid, int32_t keyType, int32_t keySize, int32_t flags, Vector<sp<KeystoreArg> >* args) {return ::UNDEFINED_ACTION;}
  virtual int32_t is_hardware_backed(const String16& keyType) {return ::UNDEFINED_ACTION;}
  virtual int32_t reset_uid(int32_t uid) {return ::UNDEFINED_ACTION;;}
  virtual int32_t sync_uid(int32_t sourceUid, int32_t targetUid) {return ::UNDEFINED_ACTION;}
  virtual int32_t password_uid(const String16& password, int32_t uid) {return ::UNDEFINED_ACTION;}
#elif ANDROID_VERSION == 18
  virtual int32_t generate(const String16& name, int uid, int32_t flags) {return ::UNDEFINED_ACTION;}
  virtual int32_t is_hardware_backed() {return ::UNDEFINED_ACTION;}
#else
  virtual int32_t generate(const String16& name, int32_t uid, int32_t keyType, int32_t keySize, int32_t flags, Vector<sp<KeystoreArg> >* args) {return ::UNDEFINED_ACTION;}
  virtual int32_t is_hardware_backed(const String16& keyType) {return ::UNDEFINED_ACTION;}
#endif
};

} 

void startKeyStoreService()
{
  android::sp<android::IServiceManager> sm = android::defaultServiceManager();
  android::sp<android::KeyStoreService> keyStoreService = new android::KeyStoreService();
  sm->addService(String16("android.security.keystore"), keyStoreService);
}
#else
void startKeyStoreService() { return; }
#endif

static const char *CA_BEGIN = "-----BEGIN ",
                  *CA_END   = "-----END ",
                  *CA_TAILER = "-----\n";

namespace mozilla {
namespace ipc {

static const char* KEYSTORE_SOCKET_PATH = "/dev/socket/keystore";
static const char* KEYSTORE_ALLOWED_USERS[] = {
  "root",
  "wifi",
  NULL
};
static const char* KEYSTORE_ALLOWED_PREFIXES[] = {
  "WIFI_SERVERCERT_",
  "WIFI_USERCERT_",
  "WIFI_USERKEY_",
  NULL
};


void
FormatCaData(const uint8_t *aCaData, int aCaDataLength,
             const char *aName, const uint8_t **aFormatData,
             int *aFormatDataLength)
{
  int bufSize = strlen(CA_BEGIN) + strlen(CA_END) + strlen(CA_TAILER) * 2 +
                strlen(aName) * 2 + aCaDataLength + aCaDataLength/CA_LINE_SIZE + 2;
  char *buf = (char *)malloc(bufSize);

  *aFormatDataLength = bufSize;
  *aFormatData = (const uint8_t *)buf;

  char *ptr = buf;
  int len;

  
  len = snprintf(ptr, bufSize, "%s%s%s", CA_BEGIN, aName, CA_TAILER);
  ptr += len;
  bufSize -= len;

  
  int copySize;
  while (aCaDataLength > 0) {
    copySize = (aCaDataLength > CA_LINE_SIZE) ? CA_LINE_SIZE : aCaDataLength;

    memcpy(ptr, aCaData, copySize);
    ptr += copySize;
    aCaData += copySize;
    aCaDataLength -= copySize;
    bufSize -= copySize;

    *ptr = '\n';
    ptr++;
    bufSize--;
  }

  
  snprintf(ptr, bufSize, "%s%s%s", CA_END, aName, CA_TAILER);
}

ResponseCode
getCertificate(const char *aCertName, const uint8_t **aCertData, int *aCertDataLength)
{
  
  if (!aCertName) {
    return KEY_NOT_FOUND;
  }

  const char **prefix = KEYSTORE_ALLOWED_PREFIXES;
  for (; *prefix; prefix++ ) {
    if (!strncmp(*prefix, aCertName, strlen(*prefix))) {
      break;
    }
  }
  if (!(*prefix)) {
    return KEY_NOT_FOUND;
  }

  
  ScopedCERTCertificate cert(CERT_FindCertByNickname(CERT_GetDefaultCertDB(),
                                                     aCertName));

  if (!cert) {
    return KEY_NOT_FOUND;
  }

  char *certDER = PL_Base64Encode((const char *)cert->derCert.data,
                                  cert->derCert.len, nullptr);
  if (!certDER) {
    return SYSTEM_ERROR;
  }

  FormatCaData((const uint8_t *)certDER, strlen(certDER), "CERTIFICATE",
               aCertData, aCertDataLength);
  PL_strfree(certDER);

  return SUCCESS;
}

bool
checkPermission(uid_t uid)
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

  return ::checkPermission(userCred.uid);
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





KeyStore::ListenSocket::ListenSocket(KeyStore* aKeyStore)
: mKeyStore(aKeyStore)
{
  MOZ_ASSERT(mKeyStore);

  MOZ_COUNT_CTOR(KeyStore::ListenSocket);
}

void
KeyStore::ListenSocket::OnConnectSuccess()
{
  mKeyStore->OnConnectSuccess(LISTEN_SOCKET);

  MOZ_COUNT_DTOR(KeyStore::ListenSocket);
}

void
KeyStore::ListenSocket::OnConnectError()
{
  mKeyStore->OnConnectError(LISTEN_SOCKET);
}

void
KeyStore::ListenSocket::OnDisconnect()
{
  mKeyStore->OnDisconnect(LISTEN_SOCKET);
}





KeyStore::StreamSocket::StreamSocket(KeyStore* aKeyStore)
: mKeyStore(aKeyStore)
{
  MOZ_ASSERT(mKeyStore);

  MOZ_COUNT_CTOR(KeyStore::StreamSocket);
}

KeyStore::StreamSocket::~StreamSocket()
{
  MOZ_COUNT_DTOR(KeyStore::StreamSocket);
}

void
KeyStore::StreamSocket::OnConnectSuccess()
{
  mKeyStore->OnConnectSuccess(STREAM_SOCKET);
}

void
KeyStore::StreamSocket::OnConnectError()
{
  mKeyStore->OnConnectError(STREAM_SOCKET);
}

void
KeyStore::StreamSocket::OnDisconnect()
{
  mKeyStore->OnDisconnect(STREAM_SOCKET);
}

void
KeyStore::StreamSocket::ReceiveSocketData(
  nsAutoPtr<UnixSocketRawData>& aMessage)
{
  mKeyStore->ReceiveSocketData(aMessage);
}

ConnectionOrientedSocketIO*
KeyStore::StreamSocket::GetIO()
{
  return PrepareAccept(new KeyStoreConnector());
}





KeyStore::KeyStore()
: mShutdown(false)
{
  MOZ_COUNT_CTOR(KeyStore);
  ::startKeyStoreService();
  Listen();
}

KeyStore::~KeyStore()
{
  MOZ_COUNT_DTOR(KeyStore);

  MOZ_ASSERT(!mListenSocket);
  MOZ_ASSERT(!mStreamSocket);
}

void
KeyStore::Shutdown()
{
  
  mShutdown = true;

  if (mStreamSocket) {
    mStreamSocket->Close();
    mStreamSocket = nullptr;
  }
  if (mListenSocket) {
    mListenSocket->Close();
    mListenSocket = nullptr;
  }
}

void
KeyStore::Listen()
{
  
  if (mStreamSocket) {
    mStreamSocket->Close();
  } else {
    mStreamSocket = new StreamSocket(this);
  }

  if (!mListenSocket) {
    
    mListenSocket = new ListenSocket(this);
    mListenSocket->Listen(new KeyStoreConnector(), mStreamSocket);
  } else {
    
    mListenSocket->Listen(mStreamSocket);
  }

  ResetHandlerInfo();
}

void
KeyStore::ResetHandlerInfo()
{
  mHandlerInfo.state = STATE_IDLE;
  mHandlerInfo.command = 0;
  mHandlerInfo.paramCount = 0;
  mHandlerInfo.commandPattern = nullptr;
  for (int i = 0; i < MAX_PARAM; i++) {
    mHandlerInfo.param[i].length = 0;
    memset(mHandlerInfo.param[i].data, 0, VALUE_SIZE);
  }
}

bool
KeyStore::CheckSize(UnixSocketRawData *aMessage, size_t aExpectSize)
{
  return (aMessage->GetSize() >= aExpectSize);
}

ResponseCode
KeyStore::ReadCommand(UnixSocketRawData *aMessage)
{
  if (mHandlerInfo.state != STATE_IDLE) {
    NS_WARNING("Wrong state in ReadCommand()!");
    return SYSTEM_ERROR;
  }

  if (!CheckSize(aMessage, 1)) {
    NS_WARNING("Data size error in ReadCommand()!");
    return PROTOCOL_ERROR;
  }

  mHandlerInfo.command = *aMessage->GetData();
  aMessage->Consume(1);

  
  const struct ProtocolCommand *command = commands;
  while (command->command && command->command != mHandlerInfo.command) {
    command++;
  }

  if (!command->command) {
    NS_WARNING("Unsupported command!");
    return PROTOCOL_ERROR;
  }

  
  mHandlerInfo.commandPattern = command;
  if (command->paramNum) {
    
    mHandlerInfo.state = STATE_READ_PARAM_LEN;
  } else {
    mHandlerInfo.state = STATE_PROCESSING;
  }

  return SUCCESS;
}

ResponseCode
KeyStore::ReadLength(UnixSocketRawData *aMessage)
{
  if (mHandlerInfo.state != STATE_READ_PARAM_LEN) {
    NS_WARNING("Wrong state in ReadLength()!");
    return SYSTEM_ERROR;
  }

  if (!CheckSize(aMessage, 2)) {
    NS_WARNING("Data size error in ReadLength()!");
    return PROTOCOL_ERROR;
  }

  
  
  unsigned short dataLength;
  memcpy(&dataLength, aMessage->GetData(), 2);
  aMessage->Consume(2);
  mHandlerInfo.param[mHandlerInfo.paramCount].length = ntohs(dataLength);

  mHandlerInfo.state = STATE_READ_PARAM_DATA;

  return SUCCESS;
}

ResponseCode
KeyStore::ReadData(UnixSocketRawData *aMessage)
{
  if (mHandlerInfo.state != STATE_READ_PARAM_DATA) {
    NS_WARNING("Wrong state in ReadData()!");
    return SYSTEM_ERROR;
  }

  if (!CheckSize(aMessage, mHandlerInfo.param[mHandlerInfo.paramCount].length)) {
    NS_WARNING("Data size error in ReadData()!");
    return PROTOCOL_ERROR;
  }

  
  memcpy(mHandlerInfo.param[mHandlerInfo.paramCount].data,
         aMessage->GetData(),
         mHandlerInfo.param[mHandlerInfo.paramCount].length);
  aMessage->Consume(mHandlerInfo.param[mHandlerInfo.paramCount].length);
  mHandlerInfo.paramCount++;

  if (mHandlerInfo.paramCount == mHandlerInfo.commandPattern->paramNum) {
    mHandlerInfo.state = STATE_PROCESSING;
  } else {
    mHandlerInfo.state = STATE_READ_PARAM_LEN;
  }

  return SUCCESS;
}


void
KeyStore::SendResponse(ResponseCode aResponse)
{
  MOZ_ASSERT(mStreamSocket);

  if (aResponse == NO_RESPONSE)
    return;

  uint8_t response = (uint8_t)aResponse;
  UnixSocketRawData* data = new UnixSocketRawData((const void *)&response, 1);
  mStreamSocket->SendSocketData(data);
}


void
KeyStore::SendData(const uint8_t *aData, int aLength)
{
  MOZ_ASSERT(mStreamSocket);

  unsigned short dataLength = htons(aLength);

  UnixSocketRawData* length = new UnixSocketRawData((const void *)&dataLength, 2);
  mStreamSocket->SendSocketData(length);

  UnixSocketRawData* data = new UnixSocketRawData((const void *)aData, aLength);
  mStreamSocket->SendSocketData(data);
}

void
KeyStore::ReceiveSocketData(nsAutoPtr<UnixSocketRawData>& aMessage)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  ResponseCode result = SUCCESS;
  while (aMessage->GetSize() ||
         mHandlerInfo.state == STATE_PROCESSING) {
    switch (mHandlerInfo.state) {
      case STATE_IDLE:
        result = ReadCommand(aMessage);
        break;
      case STATE_READ_PARAM_LEN:
        result = ReadLength(aMessage);
        break;
      case STATE_READ_PARAM_DATA:
        result = ReadData(aMessage);
        break;
      case STATE_PROCESSING:
        if (mHandlerInfo.command == 'g') {
          
          const uint8_t *certData;
          int certDataLength;
          const char *certName = (const char *)mHandlerInfo.param[0].data;

          result = getCertificate(certName, &certData, &certDataLength);
          if (result != SUCCESS) {
            break;
          }

          SendResponse(SUCCESS);
          SendData(certData, certDataLength);

          free((void *)certData);
        }

        ResetHandlerInfo();
        break;
    }

    if (result != SUCCESS) {
      SendResponse(result);
      ResetHandlerInfo();
      return;
    }
  }
}

void
KeyStore::OnConnectSuccess(SocketType aSocketType)
{
  if (aSocketType == STREAM_SOCKET) {
    mShutdown = false;
  }
}

void
KeyStore::OnConnectError(SocketType aSocketType)
{
  if (mShutdown) {
    return;
  }

  if (aSocketType == STREAM_SOCKET) {
    
    Listen();
  }
}

void
KeyStore::OnDisconnect(SocketType aSocketType)
{
  if (mShutdown) {
    return;
  }

  switch (aSocketType) {
    case LISTEN_SOCKET:
      
      mListenSocket = nullptr;
      Listen();
      break;
    case STREAM_SOCKET:
      
      Listen();
      break;
  }
}

} 
} 
