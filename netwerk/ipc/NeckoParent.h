







































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
  virtual PHttpChannelParent* AllocPHttpChannel(PBrowserParent* browser);
  virtual bool DeallocPHttpChannel(PHttpChannelParent*);
  virtual PCookieServiceParent* AllocPCookieService();
  virtual bool DeallocPCookieService(PCookieServiceParent*);
  virtual PWyciwygChannelParent* AllocPWyciwygChannel();
  virtual bool DeallocPWyciwygChannel(PWyciwygChannelParent*);
  virtual PFTPChannelParent* AllocPFTPChannel();
  virtual bool DeallocPFTPChannel(PFTPChannelParent*);
  virtual PWebSocketParent* AllocPWebSocket(PBrowserParent* browser);
  virtual bool DeallocPWebSocket(PWebSocketParent*);
  virtual bool RecvHTMLDNSPrefetch(const nsString& hostname,
                                   const PRUint16& flags);
  virtual bool RecvCancelHTMLDNSPrefetch(const nsString& hostname,
                                         const PRUint16& flags,
                                         const nsresult& reason);

};

} 
} 

#endif 
