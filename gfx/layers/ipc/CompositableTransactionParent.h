






#include "mozilla/layers/ISurfaceAllocator.h"
#include "mozilla/layers/LayerTransaction.h"

namespace mozilla {
namespace layers {

typedef std::vector<mozilla::layers::EditReply> EditReplyVector;





class CompositableParentManager : public ISurfaceAllocator
{
public:

protected:
  


  bool ReceiveCompositableUpdate(const CompositableOperation& aEdit,
                                 EditReplyVector& replyv);
  bool IsOnCompositorSide() const MOZ_OVERRIDE { return true; }
};




} 
} 
