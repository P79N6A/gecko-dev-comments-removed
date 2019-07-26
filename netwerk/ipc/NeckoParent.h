






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
  static const char *
  GetValidatedAppInfo(const SerializedLoadContext& aSerialized,
                      PContentParent* aBrowser,
                      uint32_t* aAppId,
                      bool* aInBrowserElement);

  






  MOZ_WARN_UNUSED_RESULT
  static const char*
  CreateChannelLoadContext(PBrowserParent* aBrowser,
                           PContentParent* aContent,
                           const SerializedLoadContext& aSerialized,
                           nsCOMPtr<nsILoadContext> &aResult);

  virtual void
  CloneManagees(ProtocolBase* aSource,
              mozilla::ipc::ProtocolCloneContext* aCtx) MOZ_OVERRIDE;
  virtual PCookieServiceParent* AllocPCookieServiceParent() MOZ_OVERRIDE;
  virtual bool
  RecvPCookieServiceConstructor(PCookieServiceParent* aActor) MOZ_OVERRIDE
  {
    return PNeckoParent::RecvPCookieServiceConstructor(aActor);
  }

protected:
  virtual PHttpChannelParent*
    AllocPHttpChannelParent(PBrowserParent*, const SerializedLoadContext&,
                            const HttpChannelCreationArgs& aOpenArgs);
  virtual bool
    RecvPHttpChannelConstructor(
                      PHttpChannelParent* aActor,
                      PBrowserParent* aBrowser,
                      const SerializedLoadContext& aSerialized,
                      const HttpChannelCreationArgs& aOpenArgs);
  virtual bool DeallocPHttpChannelParent(PHttpChannelParent*);
  virtual bool DeallocPCookieServiceParent(PCookieServiceParent*);
  virtual PWyciwygChannelParent* AllocPWyciwygChannelParent();
  virtual bool DeallocPWyciwygChannelParent(PWyciwygChannelParent*);
  virtual PFTPChannelParent*
    AllocPFTPChannelParent(PBrowserParent* aBrowser,
                           const SerializedLoadContext& aSerialized,
                           const FTPChannelCreationArgs& aOpenArgs);
  virtual bool
    RecvPFTPChannelConstructor(
                      PFTPChannelParent* aActor,
                      PBrowserParent* aBrowser,
                      const SerializedLoadContext& aSerialized,
                      const FTPChannelCreationArgs& aOpenArgs);
  virtual bool DeallocPFTPChannelParent(PFTPChannelParent*);
  virtual PWebSocketParent* AllocPWebSocketParent(PBrowserParent* browser,
                                                  const SerializedLoadContext& aSerialized);
  virtual bool DeallocPWebSocketParent(PWebSocketParent*);
  virtual PTCPSocketParent* AllocPTCPSocketParent();

  virtual PRemoteOpenFileParent* AllocPRemoteOpenFileParent(const URIParams& aFileURI,
                                                            const OptionalURIParams& aAppURI)
                                                            MOZ_OVERRIDE;
  virtual bool RecvPRemoteOpenFileConstructor(PRemoteOpenFileParent* aActor,
                                              const URIParams& aFileURI,
                                              const OptionalURIParams& aAppURI)
                                              MOZ_OVERRIDE;
  virtual bool DeallocPRemoteOpenFileParent(PRemoteOpenFileParent* aActor)
                                            MOZ_OVERRIDE;

  virtual bool DeallocPTCPSocketParent(PTCPSocketParent*);
  virtual PTCPServerSocketParent* AllocPTCPServerSocketParent(const uint16_t& aLocalPort,
                                                        const uint16_t& aBacklog,
                                                        const nsString& aBinaryType);
  virtual bool RecvPTCPServerSocketConstructor(PTCPServerSocketParent*,
                                               const uint16_t& aLocalPort,
                                               const uint16_t& aBacklog,
                                               const nsString& aBinaryType);
  virtual bool DeallocPTCPServerSocketParent(PTCPServerSocketParent*);
  virtual bool RecvHTMLDNSPrefetch(const nsString& hostname,
                                   const uint16_t& flags);
  virtual bool RecvCancelHTMLDNSPrefetch(const nsString& hostname,
                                         const uint16_t& flags,
                                         const nsresult& reason);

  virtual mozilla::ipc::IProtocol*
  CloneProtocol(Channel* aChannel,
                mozilla::ipc::ProtocolCloneContext* aCtx) MOZ_OVERRIDE;
  virtual PRtspControllerParent* AllocPRtspControllerParent();
  virtual bool DeallocPRtspControllerParent(PRtspControllerParent*);

private:
  nsCString mCoreAppsBasePath;
  nsCString mWebAppsBasePath;
};

} 
} 

#endif 
