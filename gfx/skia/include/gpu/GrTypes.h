









#ifndef GrTypes_DEFINED
#define GrTypes_DEFINED

#include "SkTypes.h"
#include "GrConfig.h"
#include "SkMath.h"







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






static inline bool GrIsPow2(unsigned n) {
    return n && 0 == (n & (n - 1));
}




static inline uint32_t GrNextPow2(uint32_t n) {
    return n ? (1 << (32 - SkCLZ(n - 1))) : 1;
}

static inline int GrNextPow2(int n) {
    GrAssert(n >= 0); 
    return n ? (1 << (32 - SkCLZ(n - 1))) : 1;
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







enum GrBackend {
    kOpenGL_GrBackend,
};





typedef intptr_t GrBackendContext;






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

    kLast_GrPixelConfig = kBGRA_8888_GrPixelConfig
};
static const int kGrPixelConfigCnt = kLast_GrPixelConfig + 1;


#ifndef SK_CPU_LENDIAN
    #error "Skia gpu currently assumes little endian"
#endif
#if SK_PMCOLOR_BYTE_ORDER(B,G,R,A)
    static const GrPixelConfig kSkia8888_GrPixelConfig = kBGRA_8888_GrPixelConfig;
#elif SK_PMCOLOR_BYTE_ORDER(R,G,B,A)
    static const GrPixelConfig kSkia8888_GrPixelConfig = kRGBA_8888_GrPixelConfig;
#else
    #error "SK_*32_SHIFT values must correspond to GL_BGRA or GL_RGBA format."
#endif


static inline bool GrPixelConfigIs8888(GrPixelConfig config) {
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








enum GrSurfaceOrigin {
    kDefault_GrSurfaceOrigin,         
    kTopLeft_GrSurfaceOrigin,
    kBottomLeft_GrSurfaceOrigin,
};




struct GrTextureDesc {
    GrTextureDesc()
    : fFlags(kNone_GrTextureFlags)
    , fOrigin(kDefault_GrSurfaceOrigin)
    , fWidth(0)
    , fHeight(0)
    , fConfig(kUnknown_GrPixelConfig)
    , fSampleCnt(0) {
    }

    GrTextureFlags         fFlags;  
    GrSurfaceOrigin        fOrigin; 
    int                    fWidth;  
    int                    fHeight; 

    



    GrPixelConfig          fConfig;

    






    int                    fSampleCnt;
};







struct GrCacheID {
public:
    typedef uint8_t  Domain;

    struct Key {
        union {
            uint8_t  fData8[16];
            uint32_t fData32[4];
            uint64_t fData64[2];
        };
    };

    


    GrCacheID() { fDomain = kInvalid_Domain; }

    


    GrCacheID(Domain domain, const Key& key) {
        GrAssert(kInvalid_Domain != domain);
        this->reset(domain, key);
    }

    void reset(Domain domain, const Key& key) {
        fDomain = domain;
        memcpy(&fKey, &key, sizeof(Key));
    }

    
    bool isValid() const { return kInvalid_Domain != fDomain; }

    const Key& getKey() const { GrAssert(this->isValid()); return fKey; }
    Domain getDomain() const { GrAssert(this->isValid()); return fDomain; }

    
    static Domain GenerateDomain();

private:
    Key             fKey;
    Domain          fDomain;

    static const Domain kInvalid_Domain = 0;
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




typedef intptr_t GrBackendObject;






















enum GrBackendTextureFlags {
    


    kNone_GrBackendTextureFlag             = kNone_GrTextureFlags,
    






    kRenderTarget_GrBackendTextureFlag     = kRenderTarget_GrTextureFlagBit,
};
GR_MAKE_BITFIELD_OPS(GrBackendTextureFlags)

struct GrBackendTextureDesc {
    GrBackendTextureDesc() { memset(this, 0, sizeof(*this)); }
    GrBackendTextureFlags           fFlags;
    GrSurfaceOrigin                 fOrigin;
    int                             fWidth;         
    int                             fHeight;        
    GrPixelConfig                   fConfig;        
    



    int                             fSampleCnt;
    



    GrBackendObject                 fTextureHandle;
};













struct GrBackendRenderTargetDesc {
    GrBackendRenderTargetDesc() { memset(this, 0, sizeof(*this)); }
    int                             fWidth;         
    int                             fHeight;        
    GrPixelConfig                   fConfig;        
    GrSurfaceOrigin                 fOrigin;        
    



    int                             fSampleCnt;
    


    int                             fStencilBits;
    



    GrBackendObject                 fRenderTargetHandle;
};



#endif
