






#include "mozilla/net/PNeckoParent.h"
#include "mozilla/net/NeckoCommon.h"

#ifndef mozilla_net_NeckoParent_h
#define mozilla_net_NeckoParent_h

namespace mozilla {
namespace net {


enum PBOverrideStatus {
  kPBOverride_Unset = 0,
  kPBOverride_Private,
  kPBOverride_NotPrivate
};


class NeckoParent :
  public PNeckoParent
{
public:
  NeckoParent();
  virtual ~NeckoParent();

  MOZ_WARN_UNUSED_RESULT
  static const char *
  GetValidatedAppInfo(const SerializedLoadContext& aSerialized,
                      PBrowserParent* aBrowser,
                      uint32_t* aAppId,
                      bool* aInBrowserElement);

  







  MOZ_WARN_UNUSED_RESULT
  static const char*
  CreateChannelLoadContext(PBrowserParent* aBrowser,
                           const SerializedLoadContext& aSerialized,
                           nsCOMPtr<nsILoadContext> &aResult);

protected:
  virtual PHttpChannelParent* AllocPHttpChannel(PBrowserParent*,
                                                const SerializedLoadContext&);
  virtual bool DeallocPHttpChannel(PHttpChannelParent*);
  virtual PCookieServiceParent* AllocPCookieService();
  virtual bool DeallocPCookieService(PCookieServiceParent*);
  virtual PWyciwygChannelParent* AllocPWyciwygChannel();
  virtual bool DeallocPWyciwygChannel(PWyciwygChannelParent*);
  virtual PFTPChannelParent* AllocPFTPChannel(PBrowserParent* aBrowser,
                                              const SerializedLoadContext& aSerialized);
  virtual bool DeallocPFTPChannel(PFTPChannelParent*);
  virtual PWebSocketParent* AllocPWebSocket(PBrowserParent* browser,
                                            const SerializedLoadContext& aSerialized);
  virtual bool DeallocPWebSocket(PWebSocketParent*);
  virtual PTCPSocketParent* AllocPTCPSocket(const nsString& aHost,
                                            const uint16_t& aPort,
                                            const bool& useSSL,
                                            const nsString& aBinaryType,
                                            PBrowserParent* aBrowser);

  virtual PRemoteOpenFileParent* AllocPRemoteOpenFile(const URIParams& aFileURI,
                                                      PBrowserParent* aBrowser)
                                                      MOZ_OVERRIDE;
  virtual bool RecvPRemoteOpenFileConstructor(PRemoteOpenFileParent* aActor,
                                              const URIParams& aFileURI,
                                              PBrowserParent* aBrowser)
                                              MOZ_OVERRIDE;
  virtual bool DeallocPRemoteOpenFile(PRemoteOpenFileParent* aActor)
                                      MOZ_OVERRIDE;

  virtual bool RecvPTCPSocketConstructor(PTCPSocketParent*,
                                         const nsString& aHost,
                                         const uint16_t& aPort,
                                         const bool& useSSL,
                                         const nsString& aBinaryType,
                                         PBrowserParent* aBrowser);
  virtual bool DeallocPTCPSocket(PTCPSocketParent*);
  virtual bool RecvHTMLDNSPrefetch(const nsString& hostname,
                                   const uint16_t& flags);
  virtual bool RecvCancelHTMLDNSPrefetch(const nsString& hostname,
                                         const uint16_t& flags,
                                         const nsresult& reason);
private:
  nsCString mCoreAppsBasePath;
  nsCString mWebAppsBasePath;
};

} 
} 

#endif 
