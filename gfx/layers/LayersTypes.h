




#ifndef GFX_LAYERSTYPES_H
#define GFX_LAYERSTYPES_H

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

enum BufferMode {
  BUFFER_NONE,
  BUFFER_BUFFERED
};

}
}

#endif 
