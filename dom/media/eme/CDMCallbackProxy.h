





#ifndef CDMCallbackProxy_h_
#define CDMCallbackProxy_h_

#include "mozilla/CDMProxy.h"
#include "gmp-decryption.h"
#include "GMPDecryptorProxy.h"

namespace mozilla {



class CDMCallbackProxy : public GMPDecryptorProxyCallback {
public:
  virtual void SetSessionId(uint32_t aCreateSessionToken,
                            const nsCString& aSessionId) override;

  virtual void ResolveLoadSessionPromise(uint32_t aPromiseId,
                                         bool aSuccess) override;

  virtual void ResolvePromise(uint32_t aPromiseId) override;

  virtual void RejectPromise(uint32_t aPromiseId,
                             nsresult aException,
                             const nsCString& aSessionId) override;

  virtual void SessionMessage(const nsCString& aSessionId,
                              GMPSessionMessageType aMessageType,
                              const nsTArray<uint8_t>& aMessage) override;

  virtual void ExpirationChange(const nsCString& aSessionId,
                                GMPTimestamp aExpiryTime) override;

  virtual void SessionClosed(const nsCString& aSessionId) override;

  virtual void SessionError(const nsCString& aSessionId,
                            nsresult aException,
                            uint32_t aSystemCode,
                            const nsCString& aMessage) override;

  virtual void KeyStatusChanged(const nsCString& aSessionId,
                                const nsTArray<uint8_t>& aKeyId,
                                GMPMediaKeyStatus aStatus) override;

  virtual void SetCaps(uint64_t aCaps) override;

  virtual void Decrypted(uint32_t aId,
                         GMPErr aResult,
                         const nsTArray<uint8_t>& aDecryptedData) override;

  virtual void Terminated() override;

  ~CDMCallbackProxy() {}

private:
  friend class CDMProxy;
  explicit CDMCallbackProxy(CDMProxy* aProxy);

  
  CDMProxy* mProxy;
};

} 

#endif 
