




#ifndef GFX_LAYERSCOPE_H
#define GFX_LAYERSCOPE_H

#include <stdint.h>

struct nsIntSize;

namespace mozilla {

namespace gl { class GLContext; }

namespace layers {

struct EffectChain;

class LayerScope {
public:
    static void CreateServerSocket();
    static void DestroyServerSocket();
    static void BeginFrame(gl::GLContext* aGLContext, int64_t aFrameStamp);
    static void EndFrame(gl::GLContext* aGLContext);
    static void SendEffectChain(gl::GLContext* aGLContext,
                                const EffectChain& aEffectChain,
                                int aWidth, int aHeight);
};

} 
} 

#endif 
