






#ifndef mozilla_net_NeckoChild_h
#define mozilla_net_NeckoChild_h

#include "mozilla/net/PNeckoChild.h"
#include "mozilla/net/NeckoCommon.h"

namespace mozilla {
namespace net {


class NeckoChild :
  public PNeckoChild
{
public:
  NeckoChild();
  virtual ~NeckoChild();

  static void InitNeckoChild();
  static void DestroyNeckoChild();

protected:
  virtual PHttpChannelChild*
    AllocPHttpChannelChild(PBrowserChild*, const SerializedLoadContext&,
                           const HttpChannelCreationArgs& aOpenArgs);
  virtual bool DeallocPHttpChannelChild(PHttpChannelChild*);
  virtual PCookieServiceChild* AllocPCookieServiceChild();
  virtual bool DeallocPCookieServiceChild(PCookieServiceChild*);
  virtual PWyciwygChannelChild* AllocPWyciwygChannelChild();
  virtual bool DeallocPWyciwygChannelChild(PWyciwygChannelChild*);
  virtual PFTPChannelChild*
    AllocPFTPChannelChild(PBrowserChild* aBrowser,
                          const SerializedLoadContext& aSerialized,
                          const FTPChannelCreationArgs& aOpenArgs);
  virtual bool DeallocPFTPChannelChild(PFTPChannelChild*);
  virtual PWebSocketChild* AllocPWebSocketChild(PBrowserChild*, const SerializedLoadContext&);
  virtual bool DeallocPWebSocketChild(PWebSocketChild*);
  virtual PTCPSocketChild* AllocPTCPSocketChild();
  virtual bool DeallocPTCPSocketChild(PTCPSocketChild*);
  virtual PTCPServerSocketChild* AllocPTCPServerSocketChild(const uint16_t& aLocalPort,
                                                       const uint16_t& aBacklog,
                                                       const nsString& aBinaryType);
  virtual bool DeallocPTCPServerSocketChild(PTCPServerSocketChild*);
  virtual PRemoteOpenFileChild* AllocPRemoteOpenFileChild(const URIParams&, const URIParams&);
  virtual bool DeallocPRemoteOpenFileChild(PRemoteOpenFileChild*);
};





extern PNeckoChild *gNeckoChild;

} 
} 

#endif 
