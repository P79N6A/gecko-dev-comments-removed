






#ifndef IPC_FencerUtils_h
#define IPC_FencerUtils_h

#include "ipc/IPCMessageUtils.h"
#include "nsRefPtr.h"             

namespace mozilla {
namespace layers {

class FenceHandle {
public:
  class FdObj {
    NS_INLINE_DECL_THREADSAFE_REFCOUNTING(FdObj)
    friend class FenceHandle;
  public:
    FdObj()
      : mFd(-1) {}
    explicit FdObj(int aFd)
      : mFd(aFd) {}
    int GetAndResetFd()
    {
      int fd = mFd;
      mFd = -1;
      return fd;
    }

  private:
    virtual ~FdObj() {
      if (mFd != -1) {
        close(mFd);
      }
    }

    int mFd;
  };

  FenceHandle();

  explicit FenceHandle(FdObj* aFdObj);

  bool operator==(const FenceHandle& aOther) const {
    return mFence.get() == aOther.mFence.get();
  }

  bool IsValid() const
  {
    return (mFence->mFd != -1);
  }

  void Merge(const FenceHandle& aFenceHandle);

  void TransferToAnotherFenceHandle(FenceHandle& aFenceHandle);

  already_AddRefed<FdObj> GetAndResetFdObj();

  already_AddRefed<FdObj> GetDupFdObj();

private:
  nsRefPtr<FdObj> mFence;
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
