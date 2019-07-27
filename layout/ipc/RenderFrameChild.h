






#ifndef mozilla_dom_RenderFrameChild_h
#define mozilla_dom_RenderFrameChild_h

#include "mozilla/layout/PRenderFrameChild.h"

namespace mozilla {
namespace layout {

class RenderFrameChild : public PRenderFrameChild
{
public:
  RenderFrameChild() : mWasDestroyed(false) {}
  virtual ~RenderFrameChild() {}

  void ActorDestroy(ActorDestroyReason why) override;

  void Destroy();

private:
  bool mWasDestroyed;
};

} 
} 

#endif  
