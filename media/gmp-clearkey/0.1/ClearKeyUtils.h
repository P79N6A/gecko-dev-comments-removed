















#ifndef __ClearKeyUtils_h__
#define __ClearKeyUtils_h__

#include <stdint.h>
#include <string>
#include <vector>
#include <assert.h>
#include "gmp-api/gmp-decryption.h"

#define CLEARKEY_KEY_LEN ((size_t)16)

#if 0
void CK_Log(const char* aFmt, ...);
#define CK_LOGE(...) CK_Log(__VA_ARGS__)
#define CK_LOGD(...) CK_Log(__VA_ARGS__)
#define CK_LOGW(...) CK_Log(__VA_ARGS__)
#else
#define CK_LOGE(...)
#define CK_LOGD(...)
#define CK_LOGW(...)
#endif

struct GMPPlatformAPI;
extern GMPPlatformAPI* GetPlatform();

typedef std::vector<uint8_t> KeyId;
typedef std::vector<uint8_t> Key;

struct KeyIdPair
{
  KeyId mKeyId;
  Key mKey;
};

class ClearKeyUtils
{
public:
  static void DecryptAES(const std::vector<uint8_t>& aKey,
                         std::vector<uint8_t>& aData, std::vector<uint8_t>& aIV);

  static void ParseInitData(const uint8_t* aInitData, uint32_t aInitDataSize,
                            std::vector<Key>& aOutKeys);

  static void MakeKeyRequest(const std::vector<KeyId>& aKeyIds,
                             std::string& aOutRequest,
                             GMPSessionType aSessionType);

  static bool ParseJWK(const uint8_t* aKeyData, uint32_t aKeyDataSize,
                       std::vector<KeyIdPair>& aOutKeys,
                       GMPSessionType aSessionType);
  static const char* SessionTypeToString(GMPSessionType aSessionType);

  static bool IsValidSessionId(const char* aBuff, uint32_t aLength);
};

template<class Container, class Element>
inline bool
Contains(const Container& aContainer, const Element& aElement)
{
  return aContainer.find(aElement) != aContainer.end();
}

class AutoLock {
public:
  explicit AutoLock(GMPMutex* aMutex)
    : mMutex(aMutex)
  {
    assert(aMutex);
    if (mMutex) {
      mMutex->Acquire();
    }
  }
  ~AutoLock() {
    if (mMutex) {
      mMutex->Release();
    }
  }
private:
  GMPMutex* mMutex;
};

GMPMutex* GMPCreateMutex();

#endif 
