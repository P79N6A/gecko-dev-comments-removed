






#ifndef IPC_FencerUtils_h
#define IPC_FencerUtils_h

#include "ipc/IPCMessageUtils.h"




#if MOZ_WIDGET_GONK && ANDROID_VERSION >= 17
# include "mozilla/layers/FenceUtilsGonk.h"
#else
namespace mozilla {
namespace layers {

struct FenceHandleFromChild;

struct FenceHandle {
  FenceHandle() {}
  explicit FenceHandle(const FenceHandleFromChild& aFenceHandle) {}
  bool operator==(const FenceHandle&) const { return false; }
  bool IsValid() const { return false; }
};

struct FenceHandleFromChild {
  FenceHandleFromChild() {}
  explicit FenceHandleFromChild(const FenceHandle& aFence) {}
  bool operator==(const FenceHandle&) const { return false; }
  bool operator==(const FenceHandleFromChild&) const { return false; }
  bool IsValid() const { return false; }
};

} 
} 
#endif 

namespace IPC {

#if MOZ_WIDGET_GONK && ANDROID_VERSION >= 17
#else
template <>
struct ParamTraits<mozilla::layers::FenceHandle> {
  typedef mozilla::layers::FenceHandle paramType;
  static void Write(Message*, const paramType&) {}
  static bool Read(const Message*, void**, paramType*) { return false; }
};

template <>
struct ParamTraits<mozilla::layers::FenceHandleFromChild> {
  typedef mozilla::layers::FenceHandleFromChild paramType;
  static void Write(Message*, const paramType&) {}
  static bool Read(const Message*, void**, paramType*) { return false; }
};
#endif 

} 

#endif 
