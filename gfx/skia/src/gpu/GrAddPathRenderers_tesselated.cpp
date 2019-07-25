








#include "GrTesselatedPathRenderer.h"


void GrPathRenderer::AddPathRenderers(GrContext*,
                                      GrPathRendererChain::UsageFlags flags,
                                      GrPathRendererChain* chain) {
    chain->addPathRenderer(new GrTesselatedPathRenderer())->unref();
}
