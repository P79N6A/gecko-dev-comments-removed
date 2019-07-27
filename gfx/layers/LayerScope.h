





#ifndef GFX_LAYERSCOPE_H
#define GFX_LAYERSCOPE_H

#include <stdint.h>
#include <mozilla/UniquePtr.h>

namespace mozilla {

namespace gl { class GLContext; }

namespace layers {

namespace layerscope { class Packet; }

struct EffectChain;
class LayerComposite;

class LayerScope {
public:
    static void DrawBegin();
    static void SetRenderOffset(float aX, float aY);
    static void SetLayerTransform(const gfx::Matrix4x4& aMatrix);
    static void SetLayerRects(size_t aRects, const gfx::Rect* aLayerRects);
    static void DrawEnd(gl::GLContext* aGLContext,
                        const EffectChain& aEffectChain,
                        int aWidth,
                        int aHeight);

    static void SendLayer(LayerComposite* aLayer,
                          int aWidth,
                          int aHeight);
    static void SendLayerDump(UniquePtr<layerscope::Packet> aPacket);
    static bool CheckSendable();
    static void CleanLayer();
    static void SetHWComposed();

    static void ContentChanged(TextureHost *host);
private:
    static void Init();
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
