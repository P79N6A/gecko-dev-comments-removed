






#include "SkMask.h"




static int32_t safeMul32(int32_t a, int32_t b) {
    int64_t size = sk_64_mul(a, b);
    if (size > 0 && sk_64_isS32(size)) {
        return sk_64_asS32(size);
    }
    return 0;
}

size_t SkMask::computeImageSize() const {
    return safeMul32(fBounds.height(), fRowBytes);
}

size_t SkMask::computeTotalImageSize() const {
    size_t size = this->computeImageSize();
    if (fFormat == SkMask::k3D_Format) {
        size = safeMul32(SkToS32(size), 3);
    }
    return size;
}




uint8_t* SkMask::AllocImage(size_t size) {
    return (uint8_t*)sk_malloc_throw(SkAlign4(size));
}




void SkMask::FreeImage(void* image) {
    sk_free(image);
}



static const int gMaskFormatToShift[] = {
    ~0, 
    0,  
    0,  
    2,  
    1,  
    2   
};

static int maskFormatToShift(SkMask::Format format) {
    SkASSERT((unsigned)format < SK_ARRAY_COUNT(gMaskFormatToShift));
    SkASSERT(SkMask::kBW_Format != format);
    return gMaskFormatToShift[format];
}

void* SkMask::getAddr(int x, int y) const {
    SkASSERT(kBW_Format != fFormat);
    SkASSERT(fBounds.contains(x, y));
    SkASSERT(fImage);

    char* addr = (char*)fImage;
    addr += (y - fBounds.fTop) * fRowBytes;
    addr += (x - fBounds.fLeft) << maskFormatToShift(fFormat);
    return addr;
}
