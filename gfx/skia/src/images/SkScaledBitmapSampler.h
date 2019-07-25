






#ifndef SkScaledBitmapSampler_DEFINED
#define SkScaledBitmapSampler_DEFINED

#include "SkTypes.h"
#include "SkColor.h"

class SkBitmap;

class SkScaledBitmapSampler {
public:
    SkScaledBitmapSampler(int origWidth, int origHeight, int cellSize);
    
    int scaledWidth() const { return fScaledWidth; }
    int scaledHeight() const { return fScaledHeight; }
    
    int srcY0() const { return fY0; }
    int srcDY() const { return fDY; }

    enum SrcConfig {
        kGray,  
        kIndex, 
        kRGB,   
        kRGBX,  
        kRGBA,  
        kRGB_565 
    };

    
    
    
    bool begin(SkBitmap* dst, SrcConfig sc, bool doDither,
               const SkPMColor* = NULL);
    
    
    bool next(const uint8_t* SK_RESTRICT src);

private:
    int fScaledWidth;
    int fScaledHeight;

    int fX0;    
    int fY0;    
    int fDX;    
    int fDY;    

    typedef bool (*RowProc)(void* SK_RESTRICT dstRow,
                            const uint8_t* SK_RESTRICT src,
                            int width, int deltaSrc, int y,
                            const SkPMColor[]);

    
    char*   fDstRow; 
    int     fDstRowBytes;
    int     fCurrY; 
    int     fSrcPixelSize;  
    RowProc fRowProc;

    
    const SkPMColor* fCTable;
};

#endif
