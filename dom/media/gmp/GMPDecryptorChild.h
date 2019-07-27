




#ifndef GMPDecryptorChild_h_
#define GMPDecryptorChild_h_

#include "mozilla/gmp/PGMPDecryptorChild.h"
#include "gmp-decryption.h"
#include "mozilla/gmp/GMPTypes.h"
#include "GMPEncryptedBufferDataImpl.h"
#include <string>

namespace mozilla {
namespace gmp {

class GMPContentChild;

class GMPDecryptorChild : public GMPDecryptorCallback
                        , public GMPDecryptorHost
                        , public PGMPDecryptorChild
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(GMPDecryptorChild);

  explicit GMPDecryptorChild(GMPContentChild* aPlugin,
                             const nsTArray<uint8_t>& aPluginVoucher,
                             const nsTArray<uint8_t>& aSandboxVoucher);

  void Init(GMPDecryptor* aSession);

  
  virtual void SetSessionId(uint32_t aCreateSessionToken,
                            const char* aSessionId,
                            uint32_t aSessionIdLength) override;
  virtual void ResolveLoadSessionPromise(uint32_t aPromiseId,
                                         bool aSuccess) override;
  virtual void ResolvePromise(uint32_t aPromiseId) override;

  virtual void RejectPromise(uint32_t aPromiseId,
                             GMPDOMException aException,
                             const char* aMessage,
                             uint32_t aMessageLength) override;

  virtual void SessionMessage(const char* aSessionId,
                              uint32_t aSessionIdLength,
                              GMPSessionMessageType aMessageType,
                              const uint8_t* aMessage,
                              uint32_t aMessageLength) override;

  virtual void ExpirationChange(const char* aSessionId,
                                 uint32_t aSessionIdLength,
                                 GMPTimestamp aExpiryTime) override;

   virtual void SessionClosed(const char* aSessionId,
                             uint32_t aSessionIdLength) override;

  virtual void SessionError(const char* aSessionId,
                            uint32_t aSessionIdLength,
                            GMPDOMException aException,
                            uint32_t aSystemCode,
                            const char* aMessage,
                            uint32_t aMessageLength) override;

  virtual void KeyStatusChanged(const char* aSessionId,
                                uint32_t aSessionIdLength,
                                const uint8_t* aKeyId,
                                uint32_t aKeyIdLength,
                                GMPMediaKeyStatus aStatus) override;

  virtual void SetCapabilities(uint64_t aCaps) override;

  virtual void Decrypted(GMPBuffer* aBuffer, GMPErr aResult) override;

  
  virtual void GetSandboxVoucher(const uint8_t** aVoucher,
                                 uint32_t* aVoucherLength) override;

  virtual void GetPluginVoucher(const uint8_t** aVoucher,
                                uint32_t* aVoucherLength) override;
private:
  ~GMPDecryptorChild();

  
  virtual bool RecvInit() override;

  virtual bool RecvCreateSession(const uint32_t& aCreateSessionToken,
                                 const uint32_t& aPromiseId,
                                 const nsCString& aInitDataType,
                                 InfallibleTArray<uint8_t>&& aInitData,
                                 const GMPSessionType& aSessionType) override;

  virtual bool RecvLoadSession(const uint32_t& aPromiseId,
                               const nsCString& aSessionId) override;

  virtual bool RecvUpdateSession(const uint32_t& aPromiseId,
                                 const nsCString& aSessionId,
                                 InfallibleTArray<uint8_t>&& aResponse) override;

  virtual bool RecvCloseSession(const uint32_t& aPromiseId,
                                const nsCString& aSessionId) override;

  virtual bool RecvRemoveSession(const uint32_t& aPromiseId,
                                 const nsCString& aSessionId) override;

  virtual bool RecvDecrypt(const uint32_t& aId,
                           InfallibleTArray<uint8_t>&& aBuffer,
                           const GMPDecryptionData& aMetadata) override;

  
  virtual bool RecvSetServerCertificate(const uint32_t& aPromiseId,
                                        InfallibleTArray<uint8_t>&& aServerCert) override;

  virtual bool RecvDecryptingComplete() override;

  template <typename MethodType, typename... ParamType>
  void CallMethod(MethodType, ParamType&&...);

  template<typename MethodType, typename... ParamType>
  void CallOnGMPThread(MethodType, ParamType&&...);

  
  
  GMPDecryptor* mSession;
  GMPContentChild* mPlugin;

  
  const nsTArray<uint8_t>& mPluginVoucher;
  const nsTArray<uint8_t>& mSandboxVoucher;
};

} 
} 

#endif 
