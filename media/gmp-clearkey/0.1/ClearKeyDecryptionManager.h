















#ifndef __ClearKeyDecryptionManager_h__
#define __ClearKeyDecryptionManager_h__

#include <map>

#include "ClearKeyUtils.h"
#include "RefCounted.h"

class ClearKeyDecryptor;

class ClearKeyDecryptionManager : public RefCounted
{
private:
  ClearKeyDecryptionManager();
  ~ClearKeyDecryptionManager();

  static ClearKeyDecryptionManager* sInstance;

public:
  static ClearKeyDecryptionManager* Get();

  bool HasSeenKeyId(const KeyId& aKeyId) const;
  bool HasKeyForKeyId(const KeyId& aKeyId) const;

  const Key& GetDecryptionKey(const KeyId& aKeyId);

  
  void InitKey(KeyId aKeyId, Key aKey);
  void ExpectKeyId(KeyId aKeyId);
  void ReleaseKeyId(KeyId aKeyId);

  GMPErr Decrypt(uint8_t* aBuffer, uint32_t aBufferSize,
                 const GMPEncryptedBufferMetadata* aMetadata);

  void Shutdown();

private:
  bool IsExpectingKeyForKeyId(const KeyId& aKeyId) const;

  std::map<KeyId, ClearKeyDecryptor*> mDecryptors;
};

#endif 
