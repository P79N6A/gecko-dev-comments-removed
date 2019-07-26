






#ifndef MOZILLA_LAYERS_COMPOSITABLETRANSACTIONPARENT_H
#define MOZILLA_LAYERS_COMPOSITABLETRANSACTIONPARENT_H

#include <vector>                       
#include "mozilla/Attributes.h"         
#include "mozilla/layers/AsyncTransactionTracker.h" 
#include "mozilla/layers/ISurfaceAllocator.h"  
#include "mozilla/layers/LayersMessages.h"  

namespace mozilla {
namespace layers {

class CompositableHost;
class PTextureChild;

typedef std::vector<mozilla::layers::EditReply> EditReplyVector;





class CompositableParentManager : public ISurfaceAllocator
                                , public AsyncTransactionTrackersHolder
{
public:
  virtual void SendFenceHandle(AsyncTransactionTracker* aTracker,
                               PTextureParent* aTexture,
                               const FenceHandle& aFence) = 0;

  virtual void SendAsyncMessage(const InfallibleTArray<AsyncParentMessageData>& aMessage) = 0;

  


  virtual base::ProcessId GetChildProcessId() = 0;

protected:
  


  bool ReceiveCompositableUpdate(const CompositableOperation& aEdit,
                                 EditReplyVector& replyv);
  bool IsOnCompositorSide() const MOZ_OVERRIDE { return true; }

  



  virtual bool IsAsync() const { return false; }

  virtual void ReplyRemoveTexture(const OpReplyRemoveTexture& aReply) {}

};

} 
} 

#endif
