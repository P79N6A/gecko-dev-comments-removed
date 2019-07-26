






#include "mozilla/net/PNeckoParent.h"
#include "mozilla/net/NeckoCommon.h"

#ifndef mozilla_net_NeckoParent_h
#define mozilla_net_NeckoParent_h

namespace mozilla {
namespace net {


class NeckoParent :
  public PNeckoParent
{
public:
  NeckoParent();
  virtual ~NeckoParent();

protected:
  virtual PHttpChannelParent* AllocPHttpChannel(PBrowserParent*,
                                                const SerializedLoadContext&);
  virtual bool DeallocPHttpChannel(PHttpChannelParent*);
  virtual PCookieServiceParent* AllocPCookieService();
  virtual bool DeallocPCookieService(PCookieServiceParent*);
  virtual PWyciwygChannelParent* AllocPWyciwygChannel();
  virtual bool DeallocPWyciwygChannel(PWyciwygChannelParent*);
  virtual PFTPChannelParent* AllocPFTPChannel();
  virtual bool DeallocPFTPChannel(PFTPChannelParent*);
  virtual PWebSocketParent* AllocPWebSocket(PBrowserParent* browser);
  virtual bool DeallocPWebSocket(PWebSocketParent*);
  virtual PTCPSocketParent* AllocPTCPSocket(const nsString& aHost,
                                            const uint16_t& aPort,
                                            const bool& useSSL,
                                            const nsString& aBinaryType,
                                            PBrowserParent* aBrowser);
  virtual PRemoteOpenFileParent* AllocPRemoteOpenFile(
                                            const URIParams& fileuri,
                                            PBrowserParent* browser);
  virtual bool DeallocPRemoteOpenFile(PRemoteOpenFileParent* actor);

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
