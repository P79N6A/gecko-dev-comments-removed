




















#ifndef mozilla_net_AlternateServices_h
#define mozilla_net_AlternateServices_h

#include "nsRefPtrHashtable.h"
#include "nsString.h"
#include "nsIInterfaceRequestor.h"
#include "nsISpeculativeConnect.h"

class nsProxyInfo;

namespace mozilla { namespace net {

class nsHttpConnectionInfo;

class AltSvcMapping
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(AltSvcMapping)
  friend class AltSvcCache;

private: 
  AltSvcMapping(const nsACString &originScheme,
                const nsACString &originHost,
                int32_t originPort,
                const nsACString &username,
                bool privateBrowsing,
                uint32_t expiresAt,
                const nsACString &alternateHost,
                int32_t alternatePort,
                const nsACString &npnToken);

public:
  static void ProcessHeader(const nsCString &buf, const nsCString &originScheme,
                            const nsCString &originHost, int32_t originPort,
                            const nsACString &username, bool privateBrowsing,
                            nsIInterfaceRequestor *callbacks, nsProxyInfo *proxyInfo,
                            uint32_t caps);

  const nsCString &AlternateHost() const { return mAlternateHost; }
  const nsCString &OriginHost() const { return mOriginHost; }
  const nsCString &HashKey() const { return mHashKey; }
  uint32_t AlternatePort() const { return mAlternatePort; }
  bool Validated() { return mValidated; }
  void SetValidated(bool val) { mValidated = val; }
  bool IsRunning() { return mRunning; }
  void SetRunning(bool val) { mRunning = val; }
  int32_t GetExpiresAt() { return mExpiresAt; }
  void SetExpiresAt(int32_t val) { mExpiresAt = val; }
  void SetExpired();
  bool RouteEquals(AltSvcMapping *map);

  void GetConnectionInfo(nsHttpConnectionInfo **outCI, nsProxyInfo *pi);
  int32_t TTL();

private:
  virtual ~AltSvcMapping() {};
  static void MakeHashKey(nsCString &outKey,
                          const nsACString &originScheme,
                          const nsACString &originHost,
                          int32_t originPort,
                          bool privateBrowsing);

  nsCString mHashKey;

  nsCString mAlternateHost;
  int32_t mAlternatePort;

  nsCString mOriginHost;
  int32_t mOriginPort;

  nsCString mUsername;
  bool mPrivate;

  uint32_t mExpiresAt;

  bool mValidated;
  bool mRunning;
  bool mHttps;

  nsCString mNPNToken;
};

class AltSvcOverride : public nsIInterfaceRequestor
                     , public nsISpeculativeConnectionOverrider
{
public:
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSISPECULATIVECONNECTIONOVERRIDER
    NS_DECL_NSIINTERFACEREQUESTOR

    explicit AltSvcOverride(nsIInterfaceRequestor *aRequestor)
      : mCallbacks(aRequestor) {}

private:
    virtual ~AltSvcOverride() {}
    nsCOMPtr<nsIInterfaceRequestor> mCallbacks;
};

class AltSvcCache
{
public:
  void UpdateAltServiceMapping(AltSvcMapping *map, nsProxyInfo *pi,
                               nsIInterfaceRequestor *, uint32_t caps); 
  AltSvcMapping *GetAltServiceMapping(const nsACString &scheme,
                                      const nsACString &host,
                                      int32_t port, bool pb);
  void ClearAltServiceMappings();
  void ClearHostMapping(const nsACString &host, int32_t port);
  void ClearHostMapping(nsHttpConnectionInfo *ci);

private:
  nsRefPtrHashtable<nsCStringHashKey, AltSvcMapping> mHash;
};

}} 

#endif 
