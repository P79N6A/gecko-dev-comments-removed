




#ifndef GFX_LAYERSBACKEND_H
#define GFX_LAYERSBACKEND_H

namespace mozilla {
namespace layers {
enum LayersBackend {
  LAYERS_NONE = 0,
  LAYERS_BASIC,
  LAYERS_OPENGL,
  LAYERS_D3D9,
  LAYERS_D3D10,
  LAYERS_LAST
};
}
}

#endif 
