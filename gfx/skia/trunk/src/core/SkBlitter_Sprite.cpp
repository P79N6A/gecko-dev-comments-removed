








#include "SkSpriteBlitter.h"

SkSpriteBlitter::SkSpriteBlitter(const SkBitmap& source)
        : fSource(&source) {
    fSource->lockPixels();
}

SkSpriteBlitter::~SkSpriteBlitter() {
    fSource->unlockPixels();
}

void SkSpriteBlitter::setup(const SkBitmap& device, int left, int top,
                            const SkPaint& paint) {
    fDevice = &device;
    fLeft = left;
    fTop = top;
    fPaint = &paint;
}

#ifdef SK_DEBUG
void SkSpriteBlitter::blitH(int x, int y, int width) {
    SkDEBUGFAIL("how did we get here?");
}

void SkSpriteBlitter::blitAntiH(int x, int y, const SkAlpha antialias[],
                                const int16_t runs[]) {
    SkDEBUGFAIL("how did we get here?");
}

void SkSpriteBlitter::blitV(int x, int y, int height, SkAlpha alpha) {
    SkDEBUGFAIL("how did we get here?");
}

void SkSpriteBlitter::blitMask(const SkMask&, const SkIRect& clip) {
    SkDEBUGFAIL("how did we get here?");
}
#endif





SkBlitter* SkBlitter::ChooseSprite( const SkBitmap& device,
                                    const SkPaint& paint,
                                    const SkBitmap& source,
                                    int left, int top,
                                    void* storage, size_t storageSize) {
    









    SkSpriteBlitter* blitter;

    switch (device.config()) {
        case SkBitmap::kRGB_565_Config:
            blitter = SkSpriteBlitter::ChooseD16(source, paint, storage,
                                                 storageSize);
            break;
        case SkBitmap::kARGB_8888_Config:
            blitter = SkSpriteBlitter::ChooseD32(source, paint, storage,
                                                 storageSize);
            break;
        default:
            blitter = NULL;
            break;
    }

    if (blitter) {
        blitter->setup(device, left, top, paint);
    }
    return blitter;
}
