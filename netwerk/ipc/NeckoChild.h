







































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
  virtual PHttpChannelChild* AllocPHttpChannel(PIFrameEmbeddingChild* iframeEmbedding);
  virtual bool DeallocPHttpChannel(PHttpChannelChild*);
  virtual PCookieServiceChild* AllocPCookieService();
  virtual bool DeallocPCookieService(PCookieServiceChild*);
};





extern PNeckoChild *gNeckoChild;

} 
} 

#endif 
