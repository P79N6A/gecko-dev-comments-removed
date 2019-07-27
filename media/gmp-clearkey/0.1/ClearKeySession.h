



#ifndef __ClearKeySession_h__
#define __ClearKeySession_h__

#include "ClearKeyUtils.h"

class GMPBuffer;
class GMPDecryptorCallback;
class GMPDecryptorHost;
class GMPEncryptedBufferMetadata;






class ClearKeySession
{
public:
  ClearKeySession(const std::string& aSessionId,
                  GMPDecryptorHost* aHost, GMPDecryptorCallback *aCallback);

  ~ClearKeySession();

  const std::vector<KeyId>& GetKeyIds() { return mKeyIds; }

  void Init(uint32_t aPromiseId,
            const uint8_t* aInitData, uint32_t aInitDataSize);
private:
  std::string mSessionId;
  std::vector<KeyId> mKeyIds;

  GMPDecryptorCallback* mCallback;
  GMPDecryptorHost* mHost;
};

#endif 
