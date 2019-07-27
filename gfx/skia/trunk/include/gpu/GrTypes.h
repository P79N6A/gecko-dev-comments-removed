









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

#ifdef SK_SUPPORT_LEGACY_GRTYPES





#define GrALIGN4(n)     SkAlign4(n)
#define GrIsALIGN4(n)   SkIsAlign4(n)

template <typename T> const T& GrMin(const T& a, const T& b) {
    return (a < b) ? a : b;
}

template <typename T> const T& GrMax(const T& a, const T& b) {
    return (b < a) ? a : b;
}




#define GR_ARRAY_COUNT(array)  SK_ARRAY_COUNT(array)




typedef int32_t GrFixed;

#ifdef SK_DEBUG

static inline int16_t GrToS16(intptr_t x) {
    SkASSERT((int16_t)x == x);
    return (int16_t)x;
}

#else

#define GrToS16(x)  x

#endif

#endif


#define GR_CT_MAX(a, b) (((b) < (a)) ? (a) : (b))
#define GR_CT_MIN(a, b) (((b) < (a)) ? (b) : (a))




static inline int32_t GrIDivRoundUp(int x, int y) {
    SkASSERT(y > 0);
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






static inline uint32_t GrNextPow2(uint32_t n) {
    return n ? (1 << (32 - SkCLZ(n - 1))) : 1;
}

static inline int GrNextPow2(int n) {
    SkASSERT(n >= 0); 
    return n ? (1 << (32 - SkCLZ(n - 1))) : 1;
}






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
    kARGB_GrMaskFormat,  

    kLast_GrMaskFormat = kARGB_GrMaskFormat
};
static const int kMaskFormatCount = kLast_GrMaskFormat + 1;




static inline int GrMaskFormatBytesPerPixel(GrMaskFormat format) {
    SkASSERT((unsigned)format <= 3);
    
    
    
    
    static const int sBytesPerPixel[] = { 1, 2, 4, 4 };
    SK_COMPILE_ASSERT(SK_ARRAY_COUNT(sBytesPerPixel) == kMaskFormatCount, array_size_mismatch);

    return sBytesPerPixel[(int) format];
}




enum GrPixelConfig {
    kUnknown_GrPixelConfig,
    kAlpha_8_GrPixelConfig,
    kIndex_8_GrPixelConfig,
    kRGB_565_GrPixelConfig,
    


    kRGBA_4444_GrPixelConfig,
    


    kRGBA_8888_GrPixelConfig,
    


    kBGRA_8888_GrPixelConfig,
    


    kETC1_GrPixelConfig,
    


    kLATC_GrPixelConfig,
    



    kR11_EAC_GrPixelConfig,

    









    kASTC_12x12_GrPixelConfig,

    


    kRGBA_float_GrPixelConfig,
    kLast_GrPixelConfig = kRGBA_float_GrPixelConfig
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



static inline bool GrPixelConfigIsCompressed(GrPixelConfig config) {
    switch (config) {
        case kETC1_GrPixelConfig:
        case kLATC_GrPixelConfig:
        case kR11_EAC_GrPixelConfig:
        case kASTC_12x12_GrPixelConfig:
            return true;
        default:
            return false;
    }
}


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
        case kRGBA_float_GrPixelConfig:
            return 16;
        default:
            return 0;
    }
}

static inline size_t GrUnpackAlignment(GrPixelConfig config) {
    switch (config) {
        case kAlpha_8_GrPixelConfig:
        case kIndex_8_GrPixelConfig:
            return 1;
        case kRGB_565_GrPixelConfig:
        case kRGBA_4444_GrPixelConfig:
            return 2;
        case kRGBA_8888_GrPixelConfig:
        case kBGRA_8888_GrPixelConfig:
        case kRGBA_float_GrPixelConfig:
            return 4;
        default:
            return 0;
    }
}

static inline bool GrPixelConfigIsOpaque(GrPixelConfig config) {
    switch (config) {
        case kETC1_GrPixelConfig:
        case kRGB_565_GrPixelConfig:
            return true;
        default:
            return false;
    }
}

static inline bool GrPixelConfigIsAlphaOnly(GrPixelConfig config) {
    switch (config) {
        case kR11_EAC_GrPixelConfig:
        case kLATC_GrPixelConfig:
        case kASTC_12x12_GrPixelConfig:
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
    



    kCheckAllocation_GrTextureFlagBit  = 0x8,

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
        SkASSERT(kInvalid_Domain != domain);
        this->reset(domain, key);
    }

    void reset(Domain domain, const Key& key) {
        fDomain = domain;
        memcpy(&fKey, &key, sizeof(Key));
    }

    
    bool isValid() const { return kInvalid_Domain != fDomain; }

    const Key& getKey() const { SkASSERT(this->isValid()); return fKey; }
    Domain getDomain() const { SkASSERT(this->isValid()); return fDomain; }

    
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





enum GrGLBackendState {
    kRenderTarget_GrGLBackendState     = 1 << 0,
    kTextureBinding_GrGLBackendState   = 1 << 1,
    
    kView_GrGLBackendState             = 1 << 2,
    kBlend_GrGLBackendState            = 1 << 3,
    kAA_GrGLBackendState               = 1 << 4,
    kVertex_GrGLBackendState           = 1 << 5,
    kStencil_GrGLBackendState          = 1 << 6,
    kPixelStore_GrGLBackendState       = 1 << 7,
    kProgram_GrGLBackendState          = 1 << 8,
    kFixedFunction_GrGLBackendState    = 1 << 9,
    kMisc_GrGLBackendState             = 1 << 10,
    kPathRendering_GrGLBackendState    = 1 << 11,
    kALL_GrGLBackendState              = 0xffff
};




static inline size_t GrCompressedFormatDataSize(GrPixelConfig config,
                                                int width, int height) {
    SkASSERT(GrPixelConfigIsCompressed(config));

    switch (config) {
        case kR11_EAC_GrPixelConfig:
        case kLATC_GrPixelConfig:
        case kETC1_GrPixelConfig:
            SkASSERT((width & 3) == 0);
            SkASSERT((height & 3) == 0);
            return (width >> 2) * (height >> 2) * 8;

        case kASTC_12x12_GrPixelConfig:
            SkASSERT((width % 12) == 0);
            SkASSERT((height % 12) == 0);
            return (width / 12) * (height / 12) * 16;

        default:
            SkFAIL("Unknown compressed pixel config");
            return 4 * width * height;
    }
}




static const uint32_t kAll_GrBackendState = 0xffffffff;



#if GR_ALWAYS_ALLOCATE_ON_HEAP
    #define GrAutoMallocBaseType SkAutoMalloc
#else
    #define GrAutoMallocBaseType SkAutoSMalloc<S>
#endif

template <size_t S> class GrAutoMalloc : public GrAutoMallocBaseType {
public:
    GrAutoMalloc() : INHERITED() {}
    explicit GrAutoMalloc(size_t size) : INHERITED(size) {}
    virtual ~GrAutoMalloc() {}
private:
    typedef GrAutoMallocBaseType INHERITED;
};

#undef GrAutoMallocBaseType
#endif
