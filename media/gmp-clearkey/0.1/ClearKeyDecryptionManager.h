



#ifndef __ClearKeyDecryptor_h__
#define __ClearKeyDecryptor_h__

#include <map>
#include <string>
#include <vector>

#include "ClearKeySession.h"
#include "ClearKeyUtils.h"
#include "gmp-api/gmp-decryption.h"
#include "ScopedNSSTypes.h"

class ClearKeyDecryptor;

class ClearKeyDecryptionManager MOZ_FINAL : public GMPDecryptor
{
public:
  ClearKeyDecryptionManager();
  ~ClearKeyDecryptionManager();

  virtual void Init(GMPDecryptorCallback* aCallback) MOZ_OVERRIDE;

  virtual void CreateSession(uint32_t aPromiseId,
                             const char* aInitDataType,
                             uint32_t aInitDataTypeSize,
                             const uint8_t* aInitData,
                             uint32_t aInitDataSize,
                             GMPSessionType aSessionType) MOZ_OVERRIDE;

  virtual void LoadSession(uint32_t aPromiseId,
                           const char* aSessionId,
                           uint32_t aSessionIdLength) MOZ_OVERRIDE;

  virtual void UpdateSession(uint32_t aPromiseId,
                             const char* aSessionId,
                             uint32_t aSessionIdLength,
                             const uint8_t* aResponse,
                             uint32_t aResponseSize) MOZ_OVERRIDE;

  virtual void CloseSession(uint32_t aPromiseId,
                            const char* aSessionId,
                            uint32_t aSessionIdLength) MOZ_OVERRIDE;

  virtual void RemoveSession(uint32_t aPromiseId,
                             const char* aSessionId,
                             uint32_t aSessionIdLength) MOZ_OVERRIDE;

  virtual void SetServerCertificate(uint32_t aPromiseId,
                                    const uint8_t* aServerCert,
                                    uint32_t aServerCertSize) MOZ_OVERRIDE;

  virtual void Decrypt(GMPBuffer* aBuffer,
                       GMPEncryptedBufferMetadata* aMetadata) MOZ_OVERRIDE;

  virtual void DecryptingComplete() MOZ_OVERRIDE;

private:
  GMPDecryptorCallback* mCallback;

  std::map<KeyId, ClearKeyDecryptor*> mDecryptors;
  std::map<std::string, ClearKeySession*> mSessions;
};

#endif 
