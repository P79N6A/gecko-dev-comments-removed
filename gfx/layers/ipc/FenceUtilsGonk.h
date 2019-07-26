






#ifndef mozilla_layers_FenceUtilsGonk_h
#define mozilla_layers_FenceUtilsGonk_h

#include <unistd.h>
#include <ui/Fence.h>

#include "ipc/IPCMessageUtils.h"
#include "mozilla/layers/AsyncTransactionTracker.h" 

namespace mozilla {
namespace layers {

struct FenceHandle {
  typedef android::Fence Fence;

  FenceHandle()
  { }
  FenceHandle(const android::sp<Fence>& aFence);

  bool operator==(const FenceHandle& aOther) const {
    return mFence.get() == aOther.mFence.get();
  }

  bool IsValid() const
  {
    return mFence.get() && mFence->isValid();
  }

  android::sp<Fence> mFence;
};


class FenceDeliveryTracker : public AsyncTransactionTracker {
public:
  FenceDeliveryTracker(const android::sp<android::Fence>& aFence)
    : mFence(aFence)
  {
    MOZ_COUNT_CTOR(FenceDeliveryTracker);
  }

  ~FenceDeliveryTracker()
  {
    MOZ_COUNT_DTOR(FenceDeliveryTracker);
  }

  virtual void Complete() MOZ_OVERRIDE
  {
    mFence = nullptr;
  }

  virtual void Cancel() MOZ_OVERRIDE
  {
    mFence = nullptr;
  }

private:
  android::sp<android::Fence> mFence;
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
