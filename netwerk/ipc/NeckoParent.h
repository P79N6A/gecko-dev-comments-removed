






#include "mozilla/net/PNeckoParent.h"
#include "mozilla/net/NeckoCommon.h"
#include "mozilla/net/OfflineObserver.h"

#ifndef mozilla_net_NeckoParent_h
#define mozilla_net_NeckoParent_h

namespace mozilla {
namespace net {


enum PBOverrideStatus {
  kPBOverride_Unset = 0,
  kPBOverride_Private,
  kPBOverride_NotPrivate
};


class NeckoParent
  : public PNeckoParent
  , public DisconnectableParent
{
public:
  NeckoParent();
  virtual ~NeckoParent();

  MOZ_WARN_UNUSED_RESULT
  static const char *
  GetValidatedAppInfo(const SerializedLoadContext& aSerialized,
                      PContentParent* aBrowser,
                      uint32_t* aAppId,
                      bool* aInBrowserElement);

  






  MOZ_WARN_UNUSED_RESULT
  static const char*
  CreateChannelLoadContext(const PBrowserOrId& aBrowser,
                           PContentParent* aContent,
                           const SerializedLoadContext& aSerialized,
                           nsCOMPtr<nsILoadContext> &aResult);

  virtual void ActorDestroy(ActorDestroyReason aWhy) override;
  virtual nsresult OfflineNotification(nsISupports *) override;
  virtual uint32_t GetAppId() override { return NECKO_UNKNOWN_APP_ID; }
  virtual void
  CloneManagees(ProtocolBase* aSource,
              mozilla::ipc::ProtocolCloneContext* aCtx) override;
  virtual PCookieServiceParent* AllocPCookieServiceParent() override;
  virtual bool
  RecvPCookieServiceConstructor(PCookieServiceParent* aActor) override
  {
    return PNeckoParent::RecvPCookieServiceConstructor(aActor);
  }

  





  class NestedFrameAuthPrompt final : public nsIAuthPrompt2
  {
    ~NestedFrameAuthPrompt() {}

  public:
    NS_DECL_ISUPPORTS

    NestedFrameAuthPrompt(PNeckoParent* aParent, TabId aNestedFrameId);

    NS_IMETHOD PromptAuth(nsIChannel*, uint32_t, nsIAuthInformation*, bool*) override
    {
      return NS_ERROR_NOT_IMPLEMENTED;
    }

    NS_IMETHOD AsyncPromptAuth(nsIChannel* aChannel, nsIAuthPromptCallback* callback,
                               nsISupports*, uint32_t,
                               nsIAuthInformation* aInfo, nsICancelable**) override;

  protected:
    PNeckoParent* mNeckoParent;
    TabId mNestedFrameId;
  };

protected:
  virtual PHttpChannelParent*
    AllocPHttpChannelParent(const PBrowserOrId&, const SerializedLoadContext&,
                            const HttpChannelCreationArgs& aOpenArgs) override;
  virtual bool
    RecvPHttpChannelConstructor(
                      PHttpChannelParent* aActor,
                      const PBrowserOrId& aBrowser,
                      const SerializedLoadContext& aSerialized,
                      const HttpChannelCreationArgs& aOpenArgs) override;
  virtual bool DeallocPHttpChannelParent(PHttpChannelParent*) override;
  virtual bool DeallocPCookieServiceParent(PCookieServiceParent*) override;
  virtual PWyciwygChannelParent* AllocPWyciwygChannelParent() override;
  virtual bool DeallocPWyciwygChannelParent(PWyciwygChannelParent*) override;
  virtual PFTPChannelParent*
    AllocPFTPChannelParent(const PBrowserOrId& aBrowser,
                           const SerializedLoadContext& aSerialized,
                           const FTPChannelCreationArgs& aOpenArgs) override;
  virtual bool
    RecvPFTPChannelConstructor(
                      PFTPChannelParent* aActor,
                      const PBrowserOrId& aBrowser,
                      const SerializedLoadContext& aSerialized,
                      const FTPChannelCreationArgs& aOpenArgs) override;
  virtual bool DeallocPFTPChannelParent(PFTPChannelParent*) override;
  virtual PWebSocketParent*
    AllocPWebSocketParent(const PBrowserOrId& browser,
                          const SerializedLoadContext& aSerialized) override;
  virtual bool DeallocPWebSocketParent(PWebSocketParent*) override;
  virtual PTCPSocketParent* AllocPTCPSocketParent(const nsString& host,
                                                  const uint16_t& port) override;

  virtual PRemoteOpenFileParent*
    AllocPRemoteOpenFileParent(const SerializedLoadContext& aSerialized,
                               const URIParams& aFileURI,
                               const OptionalURIParams& aAppURI) override;
  virtual bool
    RecvPRemoteOpenFileConstructor(PRemoteOpenFileParent* aActor,
                                   const SerializedLoadContext& aSerialized,
                                   const URIParams& aFileURI,
                                   const OptionalURIParams& aAppURI)
                                   override;
  virtual bool DeallocPRemoteOpenFileParent(PRemoteOpenFileParent* aActor)
                                            override;

  virtual bool DeallocPTCPSocketParent(PTCPSocketParent*) override;
  virtual PTCPServerSocketParent*
    AllocPTCPServerSocketParent(const uint16_t& aLocalPort,
                                const uint16_t& aBacklog,
                                const nsString& aBinaryType) override;
  virtual bool RecvPTCPServerSocketConstructor(PTCPServerSocketParent*,
                                               const uint16_t& aLocalPort,
                                               const uint16_t& aBacklog,
                                               const nsString& aBinaryType) override;
  virtual bool DeallocPTCPServerSocketParent(PTCPServerSocketParent*) override;
  virtual PUDPSocketParent* AllocPUDPSocketParent(const Principal& aPrincipal,
                                                  const nsCString& aFilter) override;
  virtual bool RecvPUDPSocketConstructor(PUDPSocketParent*,
                                         const Principal& aPrincipal,
                                         const nsCString& aFilter) override;
  virtual bool DeallocPUDPSocketParent(PUDPSocketParent*) override;
  virtual PDNSRequestParent* AllocPDNSRequestParent(const nsCString& aHost,
                                                    const uint32_t& aFlags,
                                                    const nsCString& aNetworkInterface) override;
  virtual bool RecvPDNSRequestConstructor(PDNSRequestParent* actor,
                                          const nsCString& hostName,
                                          const uint32_t& flags,
                                          const nsCString& aNetworkInterface) override;
  virtual bool DeallocPDNSRequestParent(PDNSRequestParent*) override;
  virtual bool RecvSpeculativeConnect(const URIParams& aURI) override;
  virtual bool RecvHTMLDNSPrefetch(const nsString& hostname,
                                   const uint16_t& flags) override;
  virtual bool RecvCancelHTMLDNSPrefetch(const nsString& hostname,
                                         const uint16_t& flags,
                                         const nsresult& reason) override;

  virtual mozilla::ipc::IProtocol*
  CloneProtocol(Channel* aChannel,
                mozilla::ipc::ProtocolCloneContext* aCtx) override;

  virtual PDataChannelParent*
    AllocPDataChannelParent(const uint32_t& channelId) override;
  virtual bool DeallocPDataChannelParent(PDataChannelParent* parent) override;

  virtual bool RecvPDataChannelConstructor(PDataChannelParent* aActor,
                                           const uint32_t& channelId) override;

  virtual PRtspControllerParent* AllocPRtspControllerParent() override;
  virtual bool DeallocPRtspControllerParent(PRtspControllerParent*) override;

  virtual PRtspChannelParent*
    AllocPRtspChannelParent(const RtspChannelConnectArgs& aArgs)
                            override;
  virtual bool
    RecvPRtspChannelConstructor(PRtspChannelParent* aActor,
                                const RtspChannelConnectArgs& aArgs)
                                override;
  virtual bool DeallocPRtspChannelParent(PRtspChannelParent*) override;

  virtual PChannelDiverterParent*
  AllocPChannelDiverterParent(const ChannelDiverterArgs& channel) override;
  virtual bool
  RecvPChannelDiverterConstructor(PChannelDiverterParent* actor,
                                  const ChannelDiverterArgs& channel) override;
  virtual bool DeallocPChannelDiverterParent(PChannelDiverterParent* actor)
                                                                override;

  virtual bool RecvOnAuthAvailable(const uint64_t& aCallbackId,
                                   const nsString& aUser,
                                   const nsString& aPassword,
                                   const nsString& aDomain) override;
  virtual bool RecvOnAuthCancelled(const uint64_t& aCallbackId,
                                   const bool& aUserCancel) override;

private:
  nsCString mCoreAppsBasePath;
  nsCString mWebAppsBasePath;
  nsRefPtr<OfflineObserver> mObserver;
};

} 
} 

#endif 
