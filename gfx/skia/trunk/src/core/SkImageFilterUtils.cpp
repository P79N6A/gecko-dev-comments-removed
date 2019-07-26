






#include "SkMatrix.h"

#if SK_SUPPORT_GPU
#include "GrTexture.h"
#include "SkImageFilterUtils.h"
#include "SkBitmap.h"
#include "SkGrPixelRef.h"
#include "SkGr.h"

bool SkImageFilterUtils::WrapTexture(GrTexture* texture, int width, int height, SkBitmap* result) {
    SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);
    result->setConfig(info);
    result->setPixelRef(SkNEW_ARGS(SkGrPixelRef, (info, texture)))->unref();
    return true;
}

bool SkImageFilterUtils::GetInputResultGPU(const SkImageFilter* filter, SkImageFilter::Proxy* proxy,
                                           const SkBitmap& src, const SkMatrix& ctm,
                                           SkBitmap* result, SkIPoint* offset) {
    
    
    
    GrContext* context = src.getTexture()->getContext();
    GrContext::AutoWideOpenIdentityDraw awoid(context, NULL);
    if (!filter) {
        offset->fX = offset->fY = 0;
        *result = src;
        return true;
    } else if (filter->canFilterImageGPU()) {
        return filter->filterImageGPU(proxy, src, ctm, result, offset);
    } else {
        if (filter->filterImage(proxy, src, ctm, result, offset)) {
            if (!result->getTexture()) {
                SkImageInfo info;
                if (!result->asImageInfo(&info)) {
                    return false;
                }
                GrTexture* resultTex = GrLockAndRefCachedBitmapTexture(context, *result, NULL);
                result->setPixelRef(new SkGrPixelRef(info, resultTex))->unref();
                GrUnlockAndUnrefCachedBitmapTexture(resultTex);
            }
            return true;
        } else {
            return false;
        }
    }
}
#endif
