






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

protected:
  


  bool ReceiveCompositableUpdate(const CompositableOperation& aEdit,
                                 EditReplyVector& replyv);
  bool IsOnCompositorSide() const MOZ_OVERRIDE { return true; }

  



  virtual bool IsAsync() const { return false; }

  void ReturnTextureDataIfNecessary(CompositableHost* aCompositable,
                                    EditReplyVector& replyv,
                                    PCompositableParent* aParent);
  void ClearPrevFenceHandles();

protected:
  std::vector<FenceHandle> mPrevFenceHandles;
};

} 
} 

#endif
