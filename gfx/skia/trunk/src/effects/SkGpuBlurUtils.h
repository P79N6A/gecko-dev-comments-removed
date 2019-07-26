






#ifndef SkGpuBlurUtils_DEFINED
#define SkGpuBlurUtils_DEFINED

#if SK_SUPPORT_GPU
class GrTexture;
class GrContext;
#endif

struct SkRect;

namespace SkGpuBlurUtils {

#if SK_SUPPORT_GPU
  













    GrTexture* GaussianBlur(GrContext* context,
                            GrTexture* srcTexture,
                            bool canClobberSrc,
                            const SkRect& rect,
                            bool cropToRect,
                            float sigmaX,
                            float sigmaY);
#endif

};

#endif
