







































#ifndef mozilla_layers_ShadowLayerUtilsD3D10_h
#define mozilla_layers_ShadowLayerUtilsD3D10_h

#define MOZ_HAVE_PLATFORM_SPECIFIC_LAYER_BUFFERS

struct ID3D10Device;
struct ID3D10Texture2D;

namespace mozilla {
namespace layers {

class SurfaceDescriptorD3D10;





bool
GetDescriptor(ID3D10Texture2D* aTexture, SurfaceDescriptorD3D10* aDescr);

already_AddRefed<ID3D10Texture2D>
OpenForeign(ID3D10Device* aDevice, const SurfaceDescriptorD3D10& aDescr);

} 
} 

#endif  
