









#ifndef GrTypes_DEFINED
#define GrTypes_DEFINED

#include "SkTypes.h"
#include "GrConfig.h"







#define GR_MAKE_BITFIELD_OPS(X) \
    inline X operator | (X a, X b) { \
        return (X) (+a | +b); \
    } \
    \
    inline X operator & (X a, X b) { \
        return (X) (+a & +b); \
    } \
    template <typename T> \
    inline X operator & (T a, X b) { \
        return (X) (+a & +b); \
    } \
    template <typename T> \
    inline X operator & (X a, T b) { \
        return (X) (+a & +b); \
    } \

#define GR_DECL_BITFIELD_OPS_FRIENDS(X) \
    friend X operator | (X a, X b); \
    \
    friend X operator & (X a, X b); \
    \
    template <typename T> \
    friend X operator & (T a, X b); \
    \
    template <typename T> \
    friend X operator & (X a, T b); \
////////////////////////////////////////////////////////////////////////////////






#define GrALIGN4(n)     SkAlign4(n)
#define GrIsALIGN4(n)   SkIsAlign4(n)

template <typename T> const T& GrMin(const T& a, const T& b) {
    return (a < b) ? a : b;
}

template <typename T> const T& GrMax(const T& a, const T& b) {
    return (b < a) ? a : b;
}


#define GR_CT_MAX(a, b) (((b) < (a)) ? (a) : (b))
#define GR_CT_MIN(a, b) (((b) < (a)) ? (b) : (a))




static inline int32_t GrIDivRoundUp(int x, int y) {
    GrAssert(y > 0);
    return (x + (y-1)) / y;
}
static inline uint32_t GrUIDivRoundUp(uint32_t x, uint32_t y) {
    return (x + (y-1)) / y;
}
static inline size_t GrSizeDivRoundUp(size_t x, size_t y) {
    return (x + (y-1)) / y;
}


#define GR_CT_DIV_ROUND_UP(X, Y) (((X) + ((Y)-1)) / (Y))




static inline uint32_t GrUIAlignUp(uint32_t x, uint32_t alignment) {
    return GrUIDivRoundUp(x, alignment) * alignment;
}
static inline size_t GrSizeAlignUp(size_t x, size_t alignment) {
    return GrSizeDivRoundUp(x, alignment) * alignment;
}


#define GR_CT_ALIGN_UP(X, A) (GR_CT_DIV_ROUND_UP((X),(A)) * (A))




static inline uint32_t GrUIAlignUpPad(uint32_t x, uint32_t alignment) {
    return (alignment - x % alignment) % alignment;
}
static inline size_t GrSizeAlignUpPad(size_t x, size_t alignment) {
    return (alignment - x % alignment) % alignment;
}




static inline uint32_t GrUIAlignDown(uint32_t x, uint32_t alignment) {
    return (x / alignment) * alignment;
}
static inline size_t GrSizeAlignDown(size_t x, uint32_t alignment) {
    return (x / alignment) * alignment;
}




#define GR_ARRAY_COUNT(array)  SK_ARRAY_COUNT(array)


extern void* GrMalloc(size_t bytes);


extern void GrFree(void* ptr);

static inline void Gr_bzero(void* dst, size_t size) {
    memset(dst, 0, size);
}






extern int Gr_clz(uint32_t n);




static inline bool GrIsPow2(unsigned n) {
    return n && 0 == (n & (n - 1));
}




static inline uint32_t GrNextPow2(uint32_t n) {
    return n ? (1 << (32 - Gr_clz(n - 1))) : 1;
}

static inline int GrNextPow2(int n) {
    GrAssert(n >= 0); 
    return n ? (1 << (32 - Gr_clz(n - 1))) : 1;
}






typedef int32_t GrFixed;

#if GR_DEBUG

static inline int16_t GrToS16(intptr_t x) {
    GrAssert((int16_t)x == x);
    return (int16_t)x;
}

#else

#define GrToS16(x)  x

#endif







enum GrEngine {
    kOpenGL_Shaders_GrEngine,
    kOpenGL_Fixed_GrEngine,
};





typedef intptr_t GrPlatform3DContext;







typedef int GrVertexLayout;




enum GrPrimitiveType {
    kTriangles_GrPrimitiveType,
    kTriangleStrip_GrPrimitiveType,
    kTriangleFan_GrPrimitiveType,
    kPoints_GrPrimitiveType,
    kLines_GrPrimitiveType,     
    kLineStrip_GrPrimitiveType  
};

static inline bool GrIsPrimTypeLines(GrPrimitiveType type) {
    return kLines_GrPrimitiveType == type || kLineStrip_GrPrimitiveType == type;
}

static inline bool GrIsPrimTypeTris(GrPrimitiveType type) {
    return kTriangles_GrPrimitiveType == type     ||
           kTriangleStrip_GrPrimitiveType == type ||
           kTriangleFan_GrPrimitiveType == type;
}




enum GrBlendCoeff {
    kInvalid_GrBlendCoeff = -1,

    kZero_GrBlendCoeff,    
    kOne_GrBlendCoeff,     
    kSC_GrBlendCoeff,      
    kISC_GrBlendCoeff,     
    kDC_GrBlendCoeff,      
    kIDC_GrBlendCoeff,     
    kSA_GrBlendCoeff,      
    kISA_GrBlendCoeff,     
    kDA_GrBlendCoeff,      
    kIDA_GrBlendCoeff,     
    kConstC_GrBlendCoeff,  
    kIConstC_GrBlendCoeff, 
    kConstA_GrBlendCoeff,  
    kIConstA_GrBlendCoeff, 

    kPublicGrBlendCoeffCount
};





enum GrMaskFormat {
    kA8_GrMaskFormat,    
    kA565_GrMaskFormat,  
    kA888_GrMaskFormat,  

    kCount_GrMaskFormats 
};




static inline int GrMaskFormatBytesPerPixel(GrMaskFormat format) {
    GrAssert((unsigned)format <= 2);
    
    
    
    return 1 << (int)format;
}




enum GrPixelConfig {
    kUnknown_GrPixelConfig,
    kAlpha_8_GrPixelConfig,
    kIndex_8_GrPixelConfig,
    kRGB_565_GrPixelConfig,
    


    kRGBA_4444_GrPixelConfig,
    


    kRGBA_8888_GrPixelConfig,
    


    kBGRA_8888_GrPixelConfig,

    kGrPixelConfigCount
};


#ifndef SK_CPU_LENDIAN
    #error "Skia gpu currently assumes little endian"
#endif
#if 24 == SK_A32_SHIFT && 16 == SK_R32_SHIFT && \
     8 == SK_G32_SHIFT &&  0 == SK_B32_SHIFT
    static const GrPixelConfig kSkia8888_GrPixelConfig = kBGRA_8888_GrPixelConfig;
#elif 24 == SK_A32_SHIFT && 16 == SK_B32_SHIFT && \
       8 == SK_G32_SHIFT &&  0 == SK_R32_SHIFT
    static const GrPixelConfig kSkia8888_GrPixelConfig = kRGBA_8888_GrPixelConfig;
#else
    #error "SK_*32_SHIFT values must correspond to GL_BGRA or GL_RGBA format."
#endif


static const GrPixelConfig kSkia8888_PM_GrPixelConfig = kSkia8888_GrPixelConfig;



static inline bool GrPixelConfigIsRGBA8888(GrPixelConfig config) {
    switch (config) {
        case kRGBA_8888_GrPixelConfig:
            return true;
        default:
            return false;
    }
}



static inline bool GrPixelConfigIsBGRA8888(GrPixelConfig config) {
    switch (config) {
        case kBGRA_8888_GrPixelConfig:
            return true;
        default:
            return false;
    }
}


static inline bool GrPixelConfigIs32Bit(GrPixelConfig config) {
    switch (config) {
        case kRGBA_8888_GrPixelConfig:
        case kBGRA_8888_GrPixelConfig:
            return true;
        default:
            return false;
    }
}



static inline GrPixelConfig GrPixelConfigSwapRAndB(GrPixelConfig config) {
    switch (config) {
        case kBGRA_8888_GrPixelConfig:
            return kRGBA_8888_GrPixelConfig;
        case kRGBA_8888_GrPixelConfig:
            return kBGRA_8888_GrPixelConfig;
        default:
            return kUnknown_GrPixelConfig;
    }
}

static inline size_t GrBytesPerPixel(GrPixelConfig config) {
    switch (config) {
        case kAlpha_8_GrPixelConfig:
        case kIndex_8_GrPixelConfig:
            return 1;
        case kRGB_565_GrPixelConfig:
        case kRGBA_4444_GrPixelConfig:
            return 2;
        case kRGBA_8888_GrPixelConfig:
        case kBGRA_8888_GrPixelConfig:
            return 4;
        default:
            return 0;
    }
}

static inline bool GrPixelConfigIsOpaque(GrPixelConfig config) {
    switch (config) {
        case kRGB_565_GrPixelConfig:
            return true;
        default:
            return false;
    }
}

static inline bool GrPixelConfigIsAlphaOnly(GrPixelConfig config) {
    switch (config) {
        case kAlpha_8_GrPixelConfig:
            return true;
        default:
            return false;
    }
}




enum GrTextureFlags {
    kNone_GrTextureFlags            = 0x0,
    



    kRenderTarget_GrTextureFlagBit  = 0x1,
    





    kNoStencil_GrTextureFlagBit     = 0x2,
    


    kDynamicUpdate_GrTextureFlagBit = 0x4,

    kDummy_GrTextureFlagBit,
    kLastPublic_GrTextureFlagBit = kDummy_GrTextureFlagBit-1,
};

GR_MAKE_BITFIELD_OPS(GrTextureFlags)

enum {
   


    kGrColorTableSize = 256 * 4 
};





struct GrTextureDesc {
    GrTextureDesc()
    : fFlags(kNone_GrTextureFlags)
    , fWidth(0)
    , fHeight(0)
    , fConfig(kUnknown_GrPixelConfig)
    , fSampleCnt(0) {
    }

    GrTextureFlags         fFlags;  
    int                    fWidth;  
    int                    fHeight; 

    



    GrPixelConfig          fConfig;

    






    int                    fSampleCnt;
};






struct GrCacheData {
    


    static const uint64_t kScratch_CacheID = 0xBBBBBBBB;

    



    static const uint8_t kScratch_ResourceDomain = 0;


    
    
    
    GrCacheData(uint64_t key)
    : fClientCacheID(key)
    , fResourceDomain(kScratch_ResourceDomain) {
    }

    





    uint64_t               fClientCacheID;

    



    uint8_t                fResourceDomain;
};




enum GrClipType {
    kRect_ClipType,
    kPath_ClipType
};





enum GrPathCmd {
    kMove_PathCmd,      
                        
                        
    kLine_PathCmd,      
                        
    kQuadratic_PathCmd, 
                        
    kCubic_PathCmd,     
                        
    kClose_PathCmd,     
                        
                        
                        
    kEnd_PathCmd        
                        
                        
};




static int inline NumPathCmdPoints(GrPathCmd cmd) {
    static const int gNumPoints[] = {
        1, 2, 3, 4, 0, 0
    };
    return gNumPoints[cmd];
}




enum GrPathFill {
    kWinding_GrPathFill,
    kEvenOdd_GrPathFill,
    kInverseWinding_GrPathFill,
    kInverseEvenOdd_GrPathFill,
    kHairLine_GrPathFill,

    kGrPathFillCount
};

static inline GrPathFill GrNonInvertedFill(GrPathFill fill) {
    static const GrPathFill gNonInvertedFills[] = {
        kWinding_GrPathFill, 
        kEvenOdd_GrPathFill, 
        kWinding_GrPathFill, 
        kEvenOdd_GrPathFill, 
        kHairLine_GrPathFill,
    };
    GR_STATIC_ASSERT(0 == kWinding_GrPathFill);
    GR_STATIC_ASSERT(1 == kEvenOdd_GrPathFill);
    GR_STATIC_ASSERT(2 == kInverseWinding_GrPathFill);
    GR_STATIC_ASSERT(3 == kInverseEvenOdd_GrPathFill);
    GR_STATIC_ASSERT(4 == kHairLine_GrPathFill);
    GR_STATIC_ASSERT(5 == kGrPathFillCount);
    return gNonInvertedFills[fill];
}

static inline bool GrIsFillInverted(GrPathFill fill) {
    static const bool gIsFillInverted[] = {
        false, 
        false, 
        true,  
        true,  
        false, 
    };
    GR_STATIC_ASSERT(0 == kWinding_GrPathFill);
    GR_STATIC_ASSERT(1 == kEvenOdd_GrPathFill);
    GR_STATIC_ASSERT(2 == kInverseWinding_GrPathFill);
    GR_STATIC_ASSERT(3 == kInverseEvenOdd_GrPathFill);
    GR_STATIC_ASSERT(4 == kHairLine_GrPathFill);
    GR_STATIC_ASSERT(5 == kGrPathFillCount);
    return gIsFillInverted[fill];
}




typedef intptr_t GrPlatform3DObject;






















enum GrPlatformTextureFlags {
    


    kNone_GrPlatformTextureFlag              = kNone_GrTextureFlags,
    






    kRenderTarget_GrPlatformTextureFlag      = kRenderTarget_GrTextureFlagBit,
};
GR_MAKE_BITFIELD_OPS(GrPlatformTextureFlags)

struct GrPlatformTextureDesc {
    GrPlatformTextureDesc() { memset(this, 0, sizeof(*this)); }
    GrPlatformTextureFlags          fFlags;
    int                             fWidth;         
    int                             fHeight;        
    GrPixelConfig                   fConfig;        
    



    int                             fSampleCnt;
    



    GrPlatform3DObject              fTextureHandle;
};













struct GrPlatformRenderTargetDesc {
    GrPlatformRenderTargetDesc() { memset(this, 0, sizeof(*this)); }
    int                             fWidth;         
    int                             fHeight;        
    GrPixelConfig                   fConfig;        
    



    int                             fSampleCnt;
    


    int                             fStencilBits;
    



    GrPlatform3DObject              fRenderTargetHandle;
};





#include "GrInstanceCounter.h"

#endif
