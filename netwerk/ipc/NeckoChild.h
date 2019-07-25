






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
  virtual PHttpChannelChild* AllocPHttpChannel(PBrowserChild* iframeEmbedding);
  virtual bool DeallocPHttpChannel(PHttpChannelChild*);
  virtual PCookieServiceChild* AllocPCookieService();
  virtual bool DeallocPCookieService(PCookieServiceChild*);
  virtual PWyciwygChannelChild* AllocPWyciwygChannel();
  virtual bool DeallocPWyciwygChannel(PWyciwygChannelChild*);
  virtual PFTPChannelChild* AllocPFTPChannel();
  virtual bool DeallocPFTPChannel(PFTPChannelChild*);
  virtual PWebSocketChild* AllocPWebSocket(PBrowserChild*);
  virtual bool DeallocPWebSocket(PWebSocketChild*);
};





extern PNeckoChild *gNeckoChild;

} 
} 

#endif 
