








#include "GrAAHairLinePathRenderer.h"

void GrPathRenderer::AddPathRenderers(GrContext* ctx,
                                      GrPathRendererChain::UsageFlags flags,
                                      GrPathRendererChain* chain) {
    if (!(GrPathRendererChain::kNonAAOnly_UsageFlag & flags)) {
        if (GrPathRenderer* pr = GrAAHairLinePathRenderer::Create(ctx)) {
            chain->addPathRenderer(pr)->unref();
        }
    }
}
