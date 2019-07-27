





#ifndef CDMCallbackProxy_h_
#define CDMCallbackProxy_h_

#include "mozilla/CDMProxy.h"
#include "gmp-decryption.h"
#include "GMPDecryptorProxy.h"

namespace mozilla {



class CDMCallbackProxy : public GMPDecryptorProxyCallback {
public:
  virtual void ResolveNewSessionPromise(uint32_t aPromiseId,
                                        const nsCString& aSessionId) MOZ_OVERRIDE;

  virtual void ResolveLoadSessionPromise(uint32_t aPromiseId,
                                         bool aSuccess) MOZ_OVERRIDE;

  virtual void ResolvePromise(uint32_t aPromiseId) MOZ_OVERRIDE;

  virtual void RejectPromise(uint32_t aPromiseId,
                             nsresult aException,
                             const nsCString& aSessionId) MOZ_OVERRIDE;

  virtual void SessionMessage(const nsCString& aSessionId,
                              const nsTArray<uint8_t>& aMessage,
                              const nsCString& aDestinationURL) MOZ_OVERRIDE;

  virtual void ExpirationChange(const nsCString& aSessionId,
                                GMPTimestamp aExpiryTime) MOZ_OVERRIDE;

  virtual void SessionClosed(const nsCString& aSessionId) MOZ_OVERRIDE;

  virtual void SessionError(const nsCString& aSessionId,
                            nsresult aException,
                            uint32_t aSystemCode,
                            const nsCString& aMessage) MOZ_OVERRIDE;

  virtual void KeyIdUsable(const nsCString& aSessionId,
                           const nsTArray<uint8_t>& aKeyId) MOZ_OVERRIDE;

  virtual void KeyIdNotUsable(const nsCString& aSessionId,
                              const nsTArray<uint8_t>& aKeyId) MOZ_OVERRIDE;

  virtual void SetCaps(uint64_t aCaps) MOZ_OVERRIDE;

  virtual void Decrypted(uint32_t aId,
                         GMPErr aResult,
                         const nsTArray<uint8_t>& aDecryptedData) MOZ_OVERRIDE;

  virtual void Terminated() MOZ_OVERRIDE;

  ~CDMCallbackProxy() {}

private:
  friend class CDMProxy;
  explicit CDMCallbackProxy(CDMProxy* aProxy);

  
  CDMProxy* mProxy;
};

} 

#endif 
