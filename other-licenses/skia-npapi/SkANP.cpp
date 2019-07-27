

























#include "SkANP.h"

SkRect* SkANP::SetRect(SkRect* dst, const ANPRectF& src) {
    dst->set(SkFloatToScalar(src.left),
             SkFloatToScalar(src.top),
             SkFloatToScalar(src.right),
             SkFloatToScalar(src.bottom));
    return dst;
}

SkIRect* SkANP::SetRect(SkIRect* dst, const ANPRectI& src) {
    dst->set(src.left, src.top, src.right, src.bottom);
    return dst;
}

ANPRectI* SkANP::SetRect(ANPRectI* dst, const SkIRect& src) {
    dst->left = src.fLeft;
    dst->top = src.fTop;
    dst->right = src.fRight;
    dst->bottom = src.fBottom;
    return dst;
}

ANPRectF* SkANP::SetRect(ANPRectF* dst, const SkRect& src) {
    dst->left = SkScalarToFloat(src.fLeft);
    dst->top = SkScalarToFloat(src.fTop);
    dst->right = SkScalarToFloat(src.fRight);
    dst->bottom = SkScalarToFloat(src.fBottom);
    return dst;
}

SkBitmap* SkANP::SetBitmap(SkBitmap* dst, const ANPBitmap& src) {
    SkColorType colorType = kUnknown_SkColorType;

    switch (src.format) {
        case kRGBA_8888_ANPBitmapFormat:
            
            
            colorType = kN32_SkColorType;
            break;
        case kRGB_565_ANPBitmapFormat:
            colorType = kRGB_565_SkColorType;
            break;
        default:
            break;
    }

    SkImageInfo info = SkImageInfo::Make(src.width, src.height, colorType, kPremul_SkAlphaType);
    dst->setInfo(info, src.rowBytes);
    dst->setPixels(src.baseAddr);
    return dst;
}

bool SkANP::SetBitmap(ANPBitmap* dst, const SkBitmap& src) {
    if (!(dst->baseAddr = src.getPixels())) {
        SkDebugf("SkANP::SetBitmap - getPixels() returned null\n");
        return false;
    }

    switch (src.colorType()) {
        case SkColorType::kRGBA_8888_SkColorType:
            dst->format = kRGBA_8888_ANPBitmapFormat;
            break;
        case SkColorType::kRGB_565_SkColorType:
            dst->format = kRGB_565_ANPBitmapFormat;
            break;
        default:
            SkDebugf("SkANP::SetBitmap - unsupported src.colorType %d\n", src.colorType());
            return false;
    }

    dst->width    = src.width();
    dst->height   = src.height();
    dst->rowBytes = src.rowBytes();
    return true;
}

void SkANP::InitEvent(ANPEvent* event, ANPEventType et) {
    event->inSize = sizeof(ANPEvent);
    event->eventType = et;
}
