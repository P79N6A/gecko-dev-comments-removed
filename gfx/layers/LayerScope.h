




#ifndef GFX_LAYERSCOPE_H
#define GFX_LAYERSCOPE_H

#include <stdint.h>
#include <mozilla/UniquePtr.h>

struct nsIntSize;

namespace mozilla {

namespace gl { class GLContext; }

namespace layers {

namespace layerscope { class Packet; }

struct EffectChain;
class LayerComposite;

class LayerScope {
public:
    static void Init();
    static void DeInit();
    static void SendEffectChain(gl::GLContext* aGLContext,
                                const EffectChain& aEffectChain,
                                int aWidth,
                                int aHeight);
    static void SendLayer(LayerComposite* aLayer,
                          int aWidth,
                          int aHeight);
    static void SendLayerDump(UniquePtr<layerscope::Packet> aPacket);
    static bool CheckSendable();
    static void CleanLayer();
};


class LayerScopeAutoFrame {
public:
    explicit LayerScopeAutoFrame(int64_t aFrameStamp);
    ~LayerScopeAutoFrame();

private:
    static void BeginFrame(int64_t aFrameStamp);
    static void EndFrame();
};

} 
} 

#endif 
