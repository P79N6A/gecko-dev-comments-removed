






#ifndef MOZILLA_LAYERS_COMPOSITABLETRANSACTIONPARENT_H
#define MOZILLA_LAYERS_COMPOSITABLETRANSACTIONPARENT_H

#include <vector>                       
#include "mozilla/Attributes.h"         
#include "mozilla/layers/ISurfaceAllocator.h"  
#include "mozilla/layers/LayersMessages.h"  

namespace mozilla {
namespace layers {

class CompositableHost;

typedef std::vector<mozilla::layers::EditReply> EditReplyVector;





class CompositableParentManager : public ISurfaceAllocator
{
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
