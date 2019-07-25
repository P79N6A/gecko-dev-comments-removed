




































#include "Layers.h"
#include "RenderTrace.h"


#ifdef MOZ_RENDERTRACE


namespace mozilla {
namespace layers {

static int colorId = 0;


const char* colors[] = {
    "00", "01", "02", "03", "04", "05", "06", "07", "08", "09",
    "10", "11", "12", "13", "14", "15", "16", "17", "18", "19"
    };

const char* layerName[] = {
    "CANVAS", "COLOR", "CONTAINER", "IMAGE", "READBACK", "SHADOW", "THEBES"
    };

static gfx3DMatrix GetRootTransform(Layer *aLayer) {
  if (aLayer->GetParent() != NULL)
    return GetRootTransform(aLayer->GetParent()) * aLayer->GetTransform();
  return aLayer->GetTransform();
}

void RenderTraceLayers(Layer *aLayer, const char *aColor, gfx3DMatrix aRootTransform, bool aReset) {
  if (!aLayer)
    return;

  gfx3DMatrix trans = aRootTransform * aLayer->GetTransform();
  nsIntRect clipRect = aLayer->GetEffectiveVisibleRegion().GetBounds();
  clipRect.MoveBy((int)trans.ProjectTo2D()[3][0], (int)trans.ProjectTo2D()[3][1]);

  printf_stderr("%s RENDERTRACE %u rect #%s%s %i %i %i %i\n",
    layerName[aLayer->GetType()], (int)PR_IntervalNow(),
    colors[colorId%19], aColor,
    clipRect.x, clipRect.y, clipRect.width, clipRect.height);

  colorId++;

  for (Layer* child = aLayer->GetFirstChild();
        child; child = child->GetNextSibling()) {
    RenderTraceLayers(child, aColor, aRootTransform, false);
  }

  if (aReset) colorId = 0;
}

void RenderTraceInvalidateStart(Layer *aLayer, const char *aColor, nsIntRect aRect) {
  gfx3DMatrix trans = GetRootTransform(aLayer);
  aRect.MoveBy((int)trans.ProjectTo2D()[3][0], (int)trans.ProjectTo2D()[3][1]);

  printf_stderr("%s RENDERTRACE %u fillrect #%s%s %i %i %i %i\n",
    layerName[aLayer->GetType()], (int)PR_IntervalNow(),
    "FF", aColor,
    aRect.x, aRect.y, aRect.width, aRect.height);
}
void RenderTraceInvalidateEnd(Layer *aLayer, const char *aColor) {
  
  RenderTraceInvalidateStart(aLayer, aColor, nsIntRect());
}

}
}

#endif

