







#ifndef SkBitmapTransformer_DEFINED
#define SkBitmapTransformer_DEFINED

#include "SkBitmap.h"




















class SkBitmapTransformer {
public:
    enum PixelFormat {
        
        
        kARGB_8888_Premul_PixelFormat,

        
        kLast_PixelFormat = kARGB_8888_Premul_PixelFormat,
    };

    






    SkBitmapTransformer(const SkBitmap& bitmap, PixelFormat pixelFormat) :
        fBitmap(bitmap), fPixelFormat(pixelFormat) {}

    







    bool isValid(bool logReason=false) const;

    



    size_t bytesNeededPerRow() const {
        
        return fBitmap.width() * 4;
    }

    





    size_t bytesNeededTotal() const {
        return this->bytesNeededPerRow() * fBitmap.height();
    }

    













    bool copyBitmapToPixelBuffer (void *dstBuffer, size_t dstBufferSize) const;

private:
    const SkBitmap& fBitmap;
    const PixelFormat fPixelFormat;
};

#endif
