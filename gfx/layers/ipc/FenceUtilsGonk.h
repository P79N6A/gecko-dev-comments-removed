






#ifndef mozilla_layers_FenceUtilsGonk_h
#define mozilla_layers_FenceUtilsGonk_h

#include <unistd.h>
#include <ui/Fence.h>

#include "ipc/IPCMessageUtils.h"

namespace mozilla {
namespace layers {

struct FenceHandleFromChild;

struct FenceHandle {
  typedef android::Fence Fence;

  FenceHandle()
  { }
  FenceHandle(const android::sp<Fence>& aFence);

  FenceHandle(const FenceHandleFromChild& aFenceHandle);

  bool operator==(const FenceHandle& aOther) const {
    return mFence.get() == aOther.mFence.get();
  }

  bool IsValid() const
  {
    return mFence.get() && mFence->isValid();
  }

  android::sp<Fence> mFence;
};

struct FenceHandleFromChild {
  typedef android::Fence Fence;

  FenceHandleFromChild()
  { }
  FenceHandleFromChild(const android::sp<Fence>& aFence);

  FenceHandleFromChild(const FenceHandle& aFence) {
    mFence = aFence.mFence;
  }

  bool operator==(const FenceHandle& aOther) const {
    return mFence.get() == aOther.mFence.get();
  }

  bool operator==(const FenceHandleFromChild& aOther) const {
    return mFence.get() == aOther.mFence.get();
  }

  bool IsValid() const
  {
    return mFence.get() && mFence->isValid();
  }

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

template <>
struct ParamTraits<mozilla::layers::FenceHandleFromChild> {
  typedef mozilla::layers::FenceHandleFromChild paramType;

  static void Write(Message* aMsg, const paramType& aParam);
  static bool Read(const Message* aMsg, void** aIter, paramType* aResult);
};

} 

#endif  
