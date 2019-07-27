



#ifndef _WEBRTC_GLOBAL_CHILD_H_
#define _WEBRTC_GLOBAL_CHILD_H_

#include "mozilla/dom/PWebrtcGlobalChild.h"

namespace mozilla {
namespace dom {

class WebrtcGlobalChild :
  public PWebrtcGlobalChild
{
  friend class ContentChild;

  bool mShutdown;

  MOZ_IMPLICIT WebrtcGlobalChild();
  virtual void ActorDestroy(ActorDestroyReason aWhy) override;

  virtual bool RecvGetStatsRequest(const int& aRequestId,
                                   const nsString& aPcIdFilter) override;
  virtual bool RecvGetLogRequest(const int& aReqestId,
                                 const nsCString& aPattern) override;
  virtual bool RecvSetAecLogging(const bool& aEnable) override;
  virtual bool RecvSetDebugMode(const int& aLevel) override;

public:
  virtual ~WebrtcGlobalChild();
  static WebrtcGlobalChild* Create();
};

} 
} 

#endif  
