






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
  virtual PTCPSocketChild* AllocPTCPSocketChild(const nsString& aHost,
                                                const uint16_t& aPort,
                                                const bool& useSSL,
                                                const nsString& aBinaryType,
                                                PBrowserChild* aBrowser);
  virtual bool DeallocPTCPSocketChild(PTCPSocketChild*);
  virtual PRemoteOpenFileChild* AllocPRemoteOpenFileChild(const URIParams&,
                                                          PBrowserChild*);
  virtual bool DeallocPRemoteOpenFileChild(PRemoteOpenFileChild*);
};





extern PNeckoChild *gNeckoChild;

} 
} 

#endif 
