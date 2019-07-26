






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
  virtual PHttpChannelChild* AllocPHttpChannel(PBrowserChild*,
                                              const SerializedLoadContext&);
  virtual bool DeallocPHttpChannel(PHttpChannelChild*);
  virtual PCookieServiceChild* AllocPCookieService();
  virtual bool DeallocPCookieService(PCookieServiceChild*);
  virtual PWyciwygChannelChild* AllocPWyciwygChannel();
  virtual bool DeallocPWyciwygChannel(PWyciwygChannelChild*);
  virtual PFTPChannelChild* AllocPFTPChannel(PBrowserChild* aBrowser,
                                             const SerializedLoadContext& aSerialized);
  virtual bool DeallocPFTPChannel(PFTPChannelChild*);
  virtual PWebSocketChild* AllocPWebSocket(PBrowserChild*, const SerializedLoadContext&);
  virtual bool DeallocPWebSocket(PWebSocketChild*);
  virtual PTCPSocketChild* AllocPTCPSocket(const nsString& aHost,
                                           const uint16_t& aPort,
                                           const bool& useSSL,
                                           const nsString& aBinaryType,
                                           PBrowserChild* aBrowser);
  virtual bool DeallocPTCPSocket(PTCPSocketChild*);
  virtual PRemoteOpenFileChild* AllocPRemoteOpenFile(const URIParams&,
                                                     PBrowserChild*);
  virtual bool DeallocPRemoteOpenFile(PRemoteOpenFileChild*);
};





extern PNeckoChild *gNeckoChild;

} 
} 

#endif 
