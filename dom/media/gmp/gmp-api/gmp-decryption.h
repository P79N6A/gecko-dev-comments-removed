















#ifndef GMP_DECRYPTION_h_
#define GMP_DECRYPTION_h_

#include "gmp-platform.h"

class GMPStringList {
public:
  virtual const uint32_t Size() const = 0;

  virtual void StringAt(uint32_t aIndex,
                        const char** aOutString, uint32_t* aOutLength) const = 0;

  virtual ~GMPStringList() { }
};

class GMPEncryptedBufferMetadata {
public:
  
  virtual const uint8_t* KeyId() const = 0;

  
  virtual uint32_t KeyIdSize() const = 0;

  
  virtual const uint8_t* IV() const = 0;

  
  virtual uint32_t IVSize() const = 0;

  
  virtual uint32_t NumSubsamples() const = 0;

  virtual const uint16_t* ClearBytes() const = 0;

  virtual const uint32_t* CipherBytes() const = 0;

  virtual ~GMPEncryptedBufferMetadata() {}

  
  
  virtual const GMPStringList* SessionIds() const = 0;
};

class GMPBuffer {
public:
  virtual uint32_t Id() const = 0;
  virtual uint8_t* Data() = 0;
  virtual uint32_t Size() const = 0;
  virtual void Resize(uint32_t aSize) = 0;
  virtual ~GMPBuffer() {}
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

enum GMPSessionMessageType {
  kGMPLicenseRequest = 0,
  kGMPLicenseRenewal = 1,
  kGMPLicenseRelease = 2,
  kGMPIndividualizationRequest = 3,
  kGMPMessageInvalid = 4 
};

enum GMPMediaKeyStatus {
  kGMPUsable = 0,
  kGMPExpired = 1,
  kGMPOutputDownscaled = 2,
  kGMPOutputNotAllowed = 3,
  kGMPInternalError = 4,
  kGMPUnknown = 5,
  kGMPMediaKeyStatusInvalid = 6 
};


typedef int64_t GMPTimestamp;















#define GMP_EME_CAP_DECRYPT_AUDIO (uint64_t(1) << 0)
#define GMP_EME_CAP_DECRYPT_VIDEO (uint64_t(1) << 1)



#define GMP_EME_CAP_DECRYPT_AND_DECODE_AUDIO (uint64_t(1) << 2)
#define GMP_EME_CAP_DECRYPT_AND_DECODE_VIDEO (uint64_t(1) << 3)


class GMPDecryptorCallback {
public:

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual void SetSessionId(uint32_t aCreateSessionToken,
                            const char* aSessionId,
                            uint32_t aSessionIdLength) = 0;

  
  
  
  
  virtual void ResolveLoadSessionPromise(uint32_t aPromiseId,
                                         bool aSuccess) = 0;

  
  virtual void ResolvePromise(uint32_t aPromiseId) = 0;

  
  
  
  virtual void RejectPromise(uint32_t aPromiseId,
                             GMPDOMException aException,
                             const char* aMessage,
                             uint32_t aMessageLength) = 0;

  
  
  
  virtual void SessionMessage(const char* aSessionId,
                              uint32_t aSessionIdLength,
                              GMPSessionMessageType aMessageType,
                              const uint8_t* aMessage,
                              uint32_t aMessageLength) = 0;

  
   virtual void ExpirationChange(const char* aSessionId,
                                 uint32_t aSessionIdLength,
                                 GMPTimestamp aExpiryTime) = 0;

  
  
  
  virtual void SessionClosed(const char* aSessionId,
                             uint32_t aSessionIdLength) = 0;

  
  
  
  
  virtual void SessionError(const char* aSessionId,
                            uint32_t aSessionIdLength,
                            GMPDOMException aException,
                            uint32_t aSystemCode,
                            const char* aMessage,
                            uint32_t aMessageLength) = 0;

  
  
  
  virtual void KeyStatusChanged(const char* aSessionId,
                                uint32_t aSessionIdLength,
                                const uint8_t* aKeyId,
                                uint32_t aKeyIdLength,
                                GMPMediaKeyStatus aStatus) = 0;

  
  
  
  
  
  
  virtual void SetCapabilities(uint64_t aCaps) = 0;

  
  virtual void Decrypted(GMPBuffer* aBuffer, GMPErr aResult) = 0;

  virtual ~GMPDecryptorCallback() {}
};


class GMPDecryptorHost {
public:
  virtual void GetSandboxVoucher(const uint8_t** aVoucher,
                                 uint32_t* aVoucherLength) = 0;

  virtual void GetPluginVoucher(const uint8_t** aVoucher,
                                uint32_t* aVoucherLength) = 0;

  virtual ~GMPDecryptorHost() {}
};

enum GMPSessionType {
  kGMPTemporySession = 0,
  kGMPPersistentSession = 1,
  kGMPSessionInvalid = 2 
};

#define GMP_API_DECRYPTOR "eme-decrypt-v7"






class GMPDecryptor {
public:

  
  
  
  
  
  
  
  
  
  virtual void Init(GMPDecryptorCallback* aCallback) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual void CreateSession(uint32_t aCreateSessionToken,
                             uint32_t aPromiseId,
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

  
  
  
  
  
  
  
  
  virtual void Decrypt(GMPBuffer* aBuffer,
                       GMPEncryptedBufferMetadata* aMetadata) = 0;

  
  
  virtual void DecryptingComplete() = 0;

  virtual ~GMPDecryptor() {}
};

#endif 
