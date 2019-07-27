






#ifndef SkScaledBitmapSampler_DEFINED
#define SkScaledBitmapSampler_DEFINED

#include "SkTypes.h"
#include "SkColor.h"
#include "SkImageDecoder.h"

class SkBitmap;

class SkScaledBitmapSampler {
public:
    SkScaledBitmapSampler(int origWidth, int origHeight, int cellSize);

    int scaledWidth() const { return fScaledWidth; }
    int scaledHeight() const { return fScaledHeight; }

    int srcY0() const { return fY0; }
    int srcDX() const { return fDX; }
    int srcDY() const { return fDY; }

    enum SrcConfig {
        kGray,  
        kIndex, 
        kRGB,   
        kRGBX,  
        kRGBA,  
        kRGB_565 
    };

    struct Options {
        bool fDither;
        bool fPremultiplyAlpha;
        bool fSkipZeros;
        explicit Options(const SkImageDecoder &dec)
            : fDither(dec.getDitherImage())
            , fPremultiplyAlpha(!dec.getRequireUnpremultipliedColors())
            , fSkipZeros(dec.getSkipWritingZeroes())
            { }
    };

    
    
    
    bool begin(SkBitmap* dst, SrcConfig sc, const SkImageDecoder& decoder,
               const SkPMColor* = NULL);
    bool begin(SkBitmap* dst, SrcConfig sc, const Options& opts,
               const SkPMColor* = NULL);
    
    
    bool next(const uint8_t* SK_RESTRICT src);

    
    
    
    
    bool sampleInterlaced(const uint8_t* SK_RESTRICT src, int srcY);

    typedef bool (*RowProc)(void* SK_RESTRICT dstRow,
                            const uint8_t* SK_RESTRICT src,
                            int width, int deltaSrc, int y,
                            const SkPMColor[]);

private:
    int fScaledWidth;
    int fScaledHeight;

    int fX0;    
    int fY0;    
    int fDX;    
    int fDY;    

#ifdef SK_DEBUG
    
    
    enum SampleMode {
        kUninitialized_SampleMode,
        kConsecutive_SampleMode,
        kInterlaced_SampleMode,
    };

    SampleMode fSampleMode;
#endif

    
    char*   fDstRow; 
    size_t  fDstRowBytes;
    int     fCurrY; 
    int     fSrcPixelSize;  
    RowProc fRowProc;

    
    const SkPMColor* fCTable;

#ifdef SK_DEBUG
    
    friend class RowProcTester;
#endif
};

#endif
