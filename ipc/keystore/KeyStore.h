





#ifndef mozilla_ipc_KeyStore_h
#define mozilla_ipc_KeyStore_h 1

#include <sys/socket.h>
#include <sys/un.h>
#include "cert.h"
#include "mozilla/ipc/ListenSocket.h"
#include "mozilla/ipc/StreamSocket.h"
#include "mozilla/ipc/UnixSocketConnector.h"
#include "nsNSSShutDown.h"

namespace mozilla {
namespace ipc {

enum ResponseCode {
  SUCCESS           =  1,
  LOCKED            =  2,
  UNINITIALIZED     =  3,
  SYSTEM_ERROR      =  4,
  PROTOCOL_ERROR    =  5,
  PERMISSION_DENIED =  6,
  KEY_NOT_FOUND     =  7,
  VALUE_CORRUPTED   =  8,
  UNDEFINED_ACTION  =  9,
  WRONG_PASSWORD_0  = 10,
  WRONG_PASSWORD_1  = 11,
  WRONG_PASSWORD_2  = 12,
  WRONG_PASSWORD_3  = 13, 
  NO_RESPONSE
};

void FormatCaData(const uint8_t *aCaData, int aCaDataLength,
                  const char *aName, const uint8_t **aFormatData,
                  size_t *aFormatDataLength);

ResponseCode getCertificate(const char *aCertName, const uint8_t **aCertData,
                            size_t *aCertDataLength);
ResponseCode getPrivateKey(const char *aKeyName, const uint8_t **aKeyData,
                           size_t *aKeyDataLength);
ResponseCode getPublicKey(const char *aKeyName, const uint8_t **aKeyData,
                          size_t *aKeyDataLength);
ResponseCode signData(const char *aKeyName, const uint8_t *data, size_t length,
                      uint8_t **out, size_t *outLength);

bool checkPermission(uid_t uid);

static const int MAX_PARAM = 2;
static const int KEY_SIZE = ((NAME_MAX - 15) / 2);
static const int VALUE_SIZE = 32768;
static const int PASSWORD_SIZE = VALUE_SIZE;

static const int CA_LINE_SIZE = 64;

struct ProtocolCommand {
  int8_t  command;
  int     paramNum;
};

static const struct ProtocolCommand commands[] = {
  {'g', 1}, 
  { 0,  0}
};

struct ProtocolParam{
  uint    length;
  int8_t  data[VALUE_SIZE];
};

typedef enum {
  STATE_IDLE,
  STATE_READ_PARAM_LEN,
  STATE_READ_PARAM_DATA,
  STATE_PROCESSING
} ProtocolHandlerState;

class KeyStoreConnector : public mozilla::ipc::UnixSocketConnector
{
public:
  KeyStoreConnector()
  {}

  virtual ~KeyStoreConnector()
  {}

  virtual int Create();
  virtual bool CreateAddr(bool aIsServer,
                          socklen_t& aAddrSize,
                          sockaddr_any& aAddr,
                          const char* aAddress);
  virtual bool SetUp(int aFd);
  virtual bool SetUpListenSocket(int aFd);
  virtual void GetSocketAddr(const sockaddr_any& aAddr,
                             nsAString& aAddrStr);
};

class KeyStore MOZ_FINAL : public nsNSSShutDownObject
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(KeyStore)

  KeyStore();

  void Shutdown();

protected:
  virtual void virtualDestroyNSSReference() {}

private:
  enum SocketType {
    LISTEN_SOCKET,
    STREAM_SOCKET
  };

  class ListenSocket MOZ_FINAL : public mozilla::ipc::ListenSocket
  {
  public:
    ListenSocket(KeyStore* aKeyStore);
    ListenSocket();

    
    

    void OnConnectSuccess() MOZ_OVERRIDE;
    void OnConnectError() MOZ_OVERRIDE;
    void OnDisconnect() MOZ_OVERRIDE;

  private:
    KeyStore* mKeyStore;
  };

  class StreamSocket MOZ_FINAL : public mozilla::ipc::StreamSocket
  {
  public:
    StreamSocket(KeyStore* aKeyStore);
    ~StreamSocket();

    
    

    void OnConnectSuccess() MOZ_OVERRIDE;
    void OnConnectError() MOZ_OVERRIDE;
    void OnDisconnect() MOZ_OVERRIDE;

    void ReceiveSocketData(nsAutoPtr<UnixSocketRawData>& aMessage) MOZ_OVERRIDE;

    
    

    ConnectionOrientedSocketIO* GetIO() MOZ_OVERRIDE;

  private:
    KeyStore* mKeyStore;
  };

  ~KeyStore();

  void ReceiveSocketData(nsAutoPtr<UnixSocketRawData>& aMessage);

  void OnConnectSuccess(enum SocketType aSocketType);
  void OnConnectError(enum SocketType aSocketType);
  void OnDisconnect(enum SocketType aSocketType);

  struct {
    ProtocolHandlerState          state;
    uint8_t                       command;
    struct ProtocolParam          param[MAX_PARAM];
    int                           paramCount;
    const struct ProtocolCommand  *commandPattern;
  } mHandlerInfo;
  void ResetHandlerInfo();
  void Listen();

  bool CheckSize(UnixSocketRawData *aMessage, size_t aExpectSize);
  ResponseCode ReadCommand(UnixSocketRawData *aMessage);
  ResponseCode ReadLength(UnixSocketRawData *aMessage);
  ResponseCode ReadData(UnixSocketRawData *aMessage);
  void SendResponse(ResponseCode response);
  void SendData(const uint8_t *data, int length);

  bool mShutdown;

  nsRefPtr<ListenSocket> mListenSocket;
  nsRefPtr<StreamSocket> mStreamSocket;
};

} 
} 

#endif
