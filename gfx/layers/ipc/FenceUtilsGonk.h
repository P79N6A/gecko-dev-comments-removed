






#ifndef mozilla_layers_FenceUtilsGonk_h
#define mozilla_layers_FenceUtilsGonk_h

#include <unistd.h>
#include <ui/Fence.h>

#include "ipc/IPCMessageUtils.h"

namespace mozilla {
namespace layers {

struct FenceHandle {
  typedef android::Fence Fence;

  FenceHandle();

  explicit FenceHandle(const android::sp<Fence>& aFence);

  bool operator==(const FenceHandle& aOther) const {
    return mFence.get() == aOther.mFence.get();
  }

  bool IsValid() const
  {
    return mFence.get() && mFence->isValid();
  }

  void Merge(const FenceHandle& aFenceHandle);

  android::sp<Fence> mFence;
};

} 
} 

namespace IPC {

template <>
struct ParamTraits<mozilla::layers::FenceHandle> {
  typedef mozilla::layers::FenceHandle paramType;

  static void Write(Message* aMsg, const paramType& aParam);
  static bool Read(const Message* aMsg, void** aIter, paramType* aResult);
};

} 

#endif  
