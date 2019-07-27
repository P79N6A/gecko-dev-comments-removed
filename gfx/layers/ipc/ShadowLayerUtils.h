






#ifndef IPC_ShadowLayerUtils_h
#define IPC_ShadowLayerUtils_h

#include "ipc/IPCMessageUtils.h"
#include "GLContextTypes.h"
#include "SurfaceTypes.h"
#include "mozilla/WidgetUtils.h"

#if defined(MOZ_ENABLE_D3D10_LAYER)
# include "mozilla/layers/ShadowLayerUtilsD3D10.h"
#endif

#if defined(XP_MACOSX)
#define MOZ_HAVE_PLATFORM_SPECIFIC_LAYER_BUFFERS
#endif

#if defined(MOZ_X11)
# include "mozilla/layers/ShadowLayerUtilsX11.h"
#else
namespace mozilla { namespace layers {
struct SurfaceDescriptorX11 {
  bool operator==(const SurfaceDescriptorX11&) const { return false; }
};
} }
#endif

#if defined(MOZ_WIDGET_GONK)
# include "mozilla/layers/ShadowLayerUtilsGralloc.h"
#else
namespace mozilla { namespace layers {
struct MagicGrallocBufferHandle {
  bool operator==(const MagicGrallocBufferHandle&) const { return false; }
};

struct GrallocBufferRef {
  bool operator==(const GrallocBufferRef&) const { return false; }
};
} }
#endif

namespace IPC {

#if !defined(MOZ_HAVE_SURFACEDESCRIPTORX11)
template <>
struct ParamTraits<mozilla::layers::SurfaceDescriptorX11> {
  typedef mozilla::layers::SurfaceDescriptorX11 paramType;
  static void Write(Message*, const paramType&) {}
  static bool Read(const Message*, void**, paramType*) { return false; }
};
#endif  

#if !defined(MOZ_HAVE_SURFACEDESCRIPTORGRALLOC)
template <>
struct ParamTraits<mozilla::layers::MagicGrallocBufferHandle> {
  typedef mozilla::layers::MagicGrallocBufferHandle paramType;
  static void Write(Message*, const paramType&) {}
  static bool Read(const Message*, void**, paramType*) { return false; }
};

template <>
struct ParamTraits<mozilla::layers::GrallocBufferRef> {
  typedef mozilla::layers::GrallocBufferRef paramType;
  static void Write(Message*, const paramType&) {}
  static bool Read(const Message*, void**, paramType*) { return false; }
};
#endif  

template <>
struct ParamTraits<mozilla::ScreenRotation>
  : public ContiguousEnumSerializer<
             mozilla::ScreenRotation,
             mozilla::ROTATION_0,
             mozilla::ROTATION_COUNT>
{};

} 

#endif 
