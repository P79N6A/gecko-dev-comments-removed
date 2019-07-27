



#include "ClearKeySession.h"
#include "ClearKeyUtils.h"

#include "gmp-api/gmp-decryption.h"
#include "mozilla/Endian.h"
#include "pk11pub.h"

using namespace mozilla;

ClearKeySession::ClearKeySession(const std::string& aSessionId,
                                 GMPDecryptorHost* aHost,
                                 GMPDecryptorCallback* aCallback)
  : mSessionId(aSessionId)
  , mHost(aHost)
  , mCallback(aCallback)
{
  CK_LOGD("ClearKeySession ctor %p", this);
}

ClearKeySession::~ClearKeySession()
{
  CK_LOGD("ClearKeySession dtor %p", this);
}

void
ClearKeySession::Init(uint32_t aPromiseId,
                      const uint8_t* aInitData, uint32_t aInitDataSize)
{
  CK_LOGD("ClearKeySession::Init");

  ClearKeyUtils::ParseInitData(aInitData, aInitDataSize, mKeyIds);
  if (!mKeyIds.size()) {
    const char message[] = "Couldn't parse cenc key init data";
    mCallback->RejectPromise(aPromiseId, kGMPAbortError, message, strlen(message));
    return;
  }

  mCallback->ResolveNewSessionPromise(aPromiseId,
                                      mSessionId.data(), mSessionId.length());
}
