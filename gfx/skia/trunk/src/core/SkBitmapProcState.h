








#ifndef SkBitmapProcState_DEFINED
#define SkBitmapProcState_DEFINED

#include "SkBitmap.h"
#include "SkBitmapFilter.h"
#include "SkMatrix.h"
#include "SkPaint.h"
#include "SkScaledImageCache.h"

#define FractionalInt_IS_64BIT

#ifdef FractionalInt_IS_64BIT
    typedef SkFixed48    SkFractionalInt;
    #define SkScalarToFractionalInt(x)  SkScalarToFixed48(x)
    #define SkFractionalIntToFixed(x)   SkFixed48ToFixed(x)
    #define SkFixedToFractionalInt(x)   SkFixedToFixed48(x)
    #define SkFractionalIntToInt(x)     SkFixed48ToInt(x)
#else
    typedef SkFixed    SkFractionalInt;
    #define SkScalarToFractionalInt(x)  SkScalarToFixed(x)
    #define SkFractionalIntToFixed(x)   (x)
    #define SkFixedToFractionalInt(x)   (x)
    #define SkFractionalIntToInt(x)     ((x) >> 16)
#endif

class SkPaint;

struct SkBitmapProcState {

    SkBitmapProcState(): fScaledCacheID(NULL), fBitmapFilter(NULL) {}
    ~SkBitmapProcState();

    typedef void (*ShaderProc32)(const SkBitmapProcState&, int x, int y,
                                 SkPMColor[], int count);

    typedef void (*ShaderProc16)(const SkBitmapProcState&, int x, int y,
                                 uint16_t[], int count);

    typedef void (*MatrixProc)(const SkBitmapProcState&,
                               uint32_t bitmapXY[],
                               int count,
                               int x, int y);

    typedef void (*SampleProc32)(const SkBitmapProcState&,
                                 const uint32_t[],
                                 int count,
                                 SkPMColor colors[]);

    typedef void (*SampleProc16)(const SkBitmapProcState&,
                                 const uint32_t[],
                                 int count,
                                 uint16_t colors[]);

    typedef U16CPU (*FixedTileProc)(SkFixed);   
    typedef U16CPU (*FixedTileLowBitsProc)(SkFixed, int);   
    typedef U16CPU (*IntTileProc)(int value, int count);   

    const SkBitmap*     fBitmap;            
    SkMatrix            fInvMatrix;         
    SkMatrix::MapXYProc fInvProc;           

    SkFractionalInt     fInvSxFractionalInt;
    SkFractionalInt     fInvKyFractionalInt;

    FixedTileProc       fTileProcX;         
    FixedTileProc       fTileProcY;         
    FixedTileLowBitsProc fTileLowBitsProcX; 
    FixedTileLowBitsProc fTileLowBitsProcY; 
    IntTileProc         fIntTileProcY;      
    SkFixed             fFilterOneX;
    SkFixed             fFilterOneY;

    SkPMColor           fPaintPMColor;      
    SkFixed             fInvSx;             
    SkFixed             fInvKy;             
    uint16_t            fAlphaScale;        
    uint8_t             fInvType;           
    uint8_t             fTileModeX;         
    uint8_t             fTileModeY;         
    uint8_t             fFilterLevel;       

    












    void platformProcs();

    








    int maxCountForBufferSize(size_t bufferSize) const;

    
    
    ShaderProc32 getShaderProc32() const { return fShaderProc32; }
    ShaderProc16 getShaderProc16() const { return fShaderProc16; }

    SkBitmapFilter* getBitmapFilter() const { return fBitmapFilter; }

#ifdef SK_DEBUG
    MatrixProc getMatrixProc() const;
#else
    MatrixProc getMatrixProc() const { return fMatrixProc; }
#endif
    SampleProc32 getSampleProc32() const { return fSampleProc32; }
    SampleProc16 getSampleProc16() const { return fSampleProc16; }

private:
    friend class SkBitmapProcShader;

    ShaderProc32        fShaderProc32;      
    ShaderProc16        fShaderProc16;      
    
    MatrixProc          fMatrixProc;        
    SampleProc32        fSampleProc32;      
    SampleProc16        fSampleProc16;      

    SkBitmap            fOrigBitmap;        
    SkBitmap            fScaledBitmap;      

    SkScaledImageCache::ID* fScaledCacheID;

    MatrixProc chooseMatrixProc(bool trivial_matrix);
    bool chooseProcs(const SkMatrix& inv, const SkPaint&);
    ShaderProc32 chooseShaderProc32();

    
    
    bool possiblyScaleImage();

    
    
    bool lockBaseBitmap();

    SkBitmapFilter* fBitmapFilter;

    
    
    bool setBitmapFilterProcs();

    
    bool setupForTranslate();

#ifdef SK_DEBUG
    static void DebugMatrixProc(const SkBitmapProcState&,
                                uint32_t[], int count, int x, int y);
#endif
};





#ifdef SK_CPU_BENDIAN
    #define PACK_TWO_SHORTS(pri, sec) ((pri) << 16 | (sec))
    #define UNPACK_PRIMARY_SHORT(packed)    ((uint32_t)(packed) >> 16)
    #define UNPACK_SECONDARY_SHORT(packed)  ((packed) & 0xFFFF)
#else
    #define PACK_TWO_SHORTS(pri, sec) ((pri) | ((sec) << 16))
    #define UNPACK_PRIMARY_SHORT(packed)    ((packed) & 0xFFFF)
    #define UNPACK_SECONDARY_SHORT(packed)  ((uint32_t)(packed) >> 16)
#endif

#ifdef SK_DEBUG
    static inline uint32_t pack_two_shorts(U16CPU pri, U16CPU sec) {
        SkASSERT((uint16_t)pri == pri);
        SkASSERT((uint16_t)sec == sec);
        return PACK_TWO_SHORTS(pri, sec);
    }
#else
    #define pack_two_shorts(pri, sec)   PACK_TWO_SHORTS(pri, sec)
#endif



void S32_opaque_D32_filter_DX(const SkBitmapProcState& s, const uint32_t xy[],
                              int count, SkPMColor colors[]);
void S32_alpha_D32_filter_DX(const SkBitmapProcState& s, const uint32_t xy[],
                             int count, SkPMColor colors[]);
void S32_opaque_D32_filter_DXDY(const SkBitmapProcState& s,
                                const uint32_t xy[], int count, SkPMColor colors[]);
void S32_alpha_D32_filter_DXDY(const SkBitmapProcState& s,
                               const uint32_t xy[], int count, SkPMColor colors[]);
void ClampX_ClampY_filter_scale(const SkBitmapProcState& s, uint32_t xy[],
                                int count, int x, int y);
void ClampX_ClampY_nofilter_scale(const SkBitmapProcState& s, uint32_t xy[],
                                  int count, int x, int y);
void ClampX_ClampY_filter_affine(const SkBitmapProcState& s,
                                 uint32_t xy[], int count, int x, int y);
void ClampX_ClampY_nofilter_affine(const SkBitmapProcState& s,
                                   uint32_t xy[], int count, int x, int y);
void S32_D16_filter_DX(const SkBitmapProcState& s,
                       const uint32_t* xy, int count, uint16_t* colors);

void highQualityFilter32(const SkBitmapProcState &s, int x, int y,
                         SkPMColor *SK_RESTRICT colors, int count);
void highQualityFilter16(const SkBitmapProcState &s, int x, int y,
                         uint16_t *SK_RESTRICT colors, int count);


#endif
