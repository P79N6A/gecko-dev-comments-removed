















#ifndef GMP_DECRYPTION_h_
#define GMP_DECRYPTION_h_

#include "gmp-platform.h"

class GMPEncryptedBufferData {
public:
  
  virtual const uint8_t* KeyId() const = 0;

  
  virtual uint32_t KeyIdSize() const = 0;

  
  virtual const uint8_t* IV() const = 0;

  
  virtual uint32_t IVSize() const = 0;

  
  virtual uint32_t NumSubsamples() const = 0;

  virtual const uint32_t* ClearBytes() const = 0;

  virtual const uint32_t* CipherBytes() const = 0;
};



enum GMPDOMException {
  kGMPNoModificationAllowedError = 7,
  kGMPNotFoundError = 8,
  kGMPNotSupportedError = 9,
  kGMPInvalidStateError = 11,
  kGMPSyntaxError = 12,
  kGMPInvalidModificationError = 13,
  kGMPInvalidAccessError = 15,
  kGMPSecurityError = 18,
  kGMPAbortError = 20,
  kGMPQuotaExceededError = 22,
  kGMPTimeoutError = 23
};


typedef int64_t GMPTimestamp;

class GMPDecryptorCallback {
public:
  
  
  
  
  virtual void OnResolveNewSessionPromise(uint32_t aPromiseId,
                                          const char* aSessionId,
                                          uint32_t aSessionIdLength) = 0;

  
  virtual void OnResolvePromise(uint32_t aPromiseId) = 0;

  
  
  
  virtual void OnRejectPromise(uint32_t aPromiseId,
                               GMPDOMException aException,
                               const char* aMessage,
                               uint32_t aMessageLength) = 0;

  
  
  
  virtual void OnSessionMessage(const char* aSessionId,
                                uint32_t aSessionIdLength,
                                const uint8_t* aMessage,
                                uint32_t aMessageLength,
                                const char* aDestinationURL,
                                uint32_t aDestinationURLLength) = 0;

  
   virtual void OnExpirationChange(const char* aSessionId,
                                   uint32_t aSessionIdLength,
                                   GMPTimestamp aExpiryTime) = 0;

  
  
  
  virtual void OnSessionClosed(const char* aSessionId,
                               uint32_t aSessionIdLength) = 0;

  
  
  
  
  virtual void OnSessionError(const char* aSessionId,
                              uint32_t aSessionIdLength,
                              GMPDOMException aException,
                              uint32_t aSystemCode,
                              const char* aMessage,
                              uint32_t aMessageLength) = 0;

  virtual void OnKeyIdUsable(const char* aSessionId,
                             uint32_t aSessionIdLength,
                             const uint8_t* aKeyId,
                             uint32_t aKeyIdLength) = 0;

  
  
  virtual void OnKeyIdNotUsable(const char* aSessionId,
                                uint32_t aSessionIdLength,
                                const uint8_t* aKeyId,
                                uint32_t aKeyIdLength) = 0;

};


class GMPDecryptorHost {
public:

  
  
  
  
  
  
  
  virtual void GetNodeId(const char** aOutNodeId,
                         uint32_t* aOutNodeIdLength) = 0;

  virtual void GetSandboxVoucher(const uint8_t** aVoucher,
                                 uint8_t* aVoucherLength) = 0;

  virtual void GetPluginVoucher(const uint8_t** aVoucher,
                                uint8_t* aVoucherLength) = 0;
};

enum GMPSessionType {
  kGMPTemporySession = 0,
  kGMPPersistentSession = 1
};






class GMPDecryptor {
public:

  
  
  virtual void Init(GMPDecryptorCallback* aCallback) = 0;

  
  
  
  
  
  virtual void CreateSession(uint32_t aPromiseId,
                             const char* aInitDataType,
                             uint32_t aInitDataTypeSize,
                             const uint8_t* aInitData,
                             uint32_t aInitDataSize,
                             GMPSessionType aSessionType) = 0;

  
  virtual void LoadSession(uint32_t aPromiseId,
                           const char* aSessionId,
                           uint32_t aSessionIdLength) = 0;

  
  virtual void UpdateSession(uint32_t aPromiseId,
                             const char* aSessionId,
                             uint32_t aSessionIdLength,
                             const uint8_t* aResponse,
                             uint32_t aResponseSize) = 0;

  
  virtual void CloseSession(uint32_t aPromiseId,
                            const char* aSessionId,
                            uint32_t aSessionIdLength) = 0;

  
  virtual void RemoveSession(uint32_t aPromiseId,
                             const char* aSessionId,
                             uint32_t aSessionIdLength) = 0;

  
  virtual void SetServerCertificate(uint32_t aPromiseId,
                                    const uint8_t* aServerCert,
                                    uint32_t aServerCertSize) = 0;
};

#endif 
