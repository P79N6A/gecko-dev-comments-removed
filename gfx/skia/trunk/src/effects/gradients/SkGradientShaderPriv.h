






#ifndef SkGradientShaderPriv_DEFINED
#define SkGradientShaderPriv_DEFINED

#include "SkGradientShader.h"
#include "SkClampRange.h"
#include "SkColorPriv.h"
#include "SkReadBuffer.h"
#include "SkWriteBuffer.h"
#include "SkMallocPixelRef.h"
#include "SkUtils.h"
#include "SkTemplates.h"
#include "SkBitmapCache.h"
#include "SkShader.h"
#include "SkOnce.h"

static inline void sk_memset32_dither(uint32_t dst[], uint32_t v0, uint32_t v1,
                               int count) {
    if (count > 0) {
        if (v0 == v1) {
            sk_memset32(dst, v0, count);
        } else {
            int pairs = count >> 1;
            for (int i = 0; i < pairs; i++) {
                *dst++ = v0;
                *dst++ = v1;
            }
            if (count & 1) {
                *dst = v0;
            }
        }
    }
}



static inline SkFixed clamp_tileproc(SkFixed x) {
    return SkClampMax(x, 0xFFFF);
}



static inline SkFixed repeat_tileproc(SkFixed x) {
    return x & 0xFFFF;
}





#if defined(_MSC_VER) && (_MSC_VER >= 1600)
#pragma optimize("", off)
#endif

static inline SkFixed mirror_tileproc(SkFixed x) {
    int s = x << 15 >> 31;
    return (x ^ s) & 0xFFFF;
}

#if defined(_MSC_VER) && (_MSC_VER >= 1600)
#pragma optimize("", on)
#endif



typedef SkFixed (*TileProc)(SkFixed);



static const TileProc gTileProcs[] = {
    clamp_tileproc,
    repeat_tileproc,
    mirror_tileproc
};



class SkGradientShaderBase : public SkShader {
public:
    struct Descriptor {
        Descriptor() {
            sk_bzero(this, sizeof(*this));
            fTileMode = SkShader::kClamp_TileMode;
        }

        const SkColor*      fColors;
        const SkScalar*     fPos;
        int                 fCount;
        SkShader::TileMode  fTileMode;
        uint32_t            fGradFlags;
    };

public:
    SkGradientShaderBase(const Descriptor& desc, const SkMatrix* localMatrix);
    virtual ~SkGradientShaderBase();

    
    class GradientShaderCache : public SkRefCnt {
    public:
        GradientShaderCache(U8CPU alpha, const SkGradientShaderBase& shader);
        ~GradientShaderCache();

        const uint16_t*     getCache16();
        const SkPMColor*    getCache32();

        SkMallocPixelRef* getCache32PixelRef() const { return fCache32PixelRef; }

        unsigned getAlpha() const { return fCacheAlpha; }

    private:
        
        uint16_t*   fCache16;
        SkPMColor*  fCache32;

        uint16_t*         fCache16Storage;    
        SkMallocPixelRef* fCache32PixelRef;
        const unsigned    fCacheAlpha;        
                                              
                                              

        const SkGradientShaderBase& fShader;

        
        bool    fCache16Inited, fCache32Inited;
        SkMutex fCache16Mutex, fCache32Mutex;

        static void initCache16(GradientShaderCache* cache);
        static void initCache32(GradientShaderCache* cache);

        static void Build16bitCache(uint16_t[], SkColor c0, SkColor c1, int count);
        static void Build32bitCache(SkPMColor[], SkColor c0, SkColor c1, int count,
                                    U8CPU alpha, uint32_t gradFlags);
    };

    class GradientShaderBaseContext : public SkShader::Context {
    public:
        GradientShaderBaseContext(const SkGradientShaderBase& shader, const ContextRec&);

        virtual uint32_t getFlags() const SK_OVERRIDE { return fFlags; }

    protected:
        SkMatrix    fDstToIndex;
        SkMatrix::MapXYProc fDstToIndexProc;
        uint8_t     fDstToIndexClass;
        uint8_t     fFlags;

        SkAutoTUnref<GradientShaderCache> fCache;

    private:
        typedef SkShader::Context INHERITED;
    };

    virtual bool isOpaque() const SK_OVERRIDE;

    void getGradientTableBitmap(SkBitmap*) const;

    enum {
        
        
        kCache16Bits    = 8,
        kCache16Count = (1 << kCache16Bits),
        kCache16Shift   = 16 - kCache16Bits,
        kSqrt16Shift    = 8 - kCache16Bits,

        
        
        kCache32Bits    = 8,
        kCache32Count   = (1 << kCache32Bits),
        kCache32Shift   = 16 - kCache32Bits,
        kSqrt32Shift    = 8 - kCache32Bits,

        
        
        kDitherStride32 = kCache32Count,
        kDitherStride16 = kCache16Count,
    };

    enum GpuColorType {
        kTwo_GpuColorType,
        kThree_GpuColorType, 
        kTexture_GpuColorType
    };

    
    
    
    GpuColorType getGpuColorType(SkColor colors[3]) const;

    uint32_t getGradFlags() const { return fGradFlags; }

protected:
    SkGradientShaderBase(SkReadBuffer& );
    virtual void flatten(SkWriteBuffer&) const SK_OVERRIDE;
    SK_TO_STRING_OVERRIDE()

    SkMatrix    fPtsToUnit;     
    TileMode    fTileMode;
    TileProc    fTileProc;
    int         fColorCount;
    uint8_t     fGradFlags;

    struct Rec {
        SkFixed     fPos;   
        uint32_t    fScale; 
    };
    Rec*        fRecs;

    void commonAsAGradient(GradientInfo*, bool flipGrad = false) const;

    






    static void FlipGradientColors(SkColor* colorDst, Rec* recDst,
                                   SkColor* colorSrc, Rec* recSrc,
                                   int count);

    
    
    
    void flipGradientColors();

private:
    enum {
        kColorStorageCount = 4, 

        kStorageSize = kColorStorageCount * (sizeof(SkColor) + sizeof(Rec))
    };
    SkColor     fStorage[(kStorageSize + 3) >> 2];
    SkColor*    fOrigColors; 
    bool        fColorsAreOpaque;

    GradientShaderCache* refCache(U8CPU alpha) const;
    mutable SkMutex                           fCacheMutex;
    mutable SkAutoTUnref<GradientShaderCache> fCache;

    void initCommon();

    typedef SkShader INHERITED;
};

static inline int init_dither_toggle(int x, int y) {
    x &= 1;
    y = (y & 1) << 1;
    return (x | y) * SkGradientShaderBase::kDitherStride32;
}

static inline int next_dither_toggle(int toggle) {
    return toggle ^ SkGradientShaderBase::kDitherStride32;
}

static inline int init_dither_toggle16(int x, int y) {
    return ((x ^ y) & 1) * SkGradientShaderBase::kDitherStride16;
}

static inline int next_dither_toggle16(int toggle) {
    return toggle ^ SkGradientShaderBase::kDitherStride16;
}



#if SK_SUPPORT_GPU

#include "GrCoordTransform.h"
#include "gl/GrGLEffect.h"

class GrEffectStage;
class GrBackendEffectFactory;
























 class GrTextureStripAtlas;


class GrGradientEffect : public GrEffect {
public:

    GrGradientEffect(GrContext* ctx,
                     const SkGradientShaderBase& shader,
                     const SkMatrix& matrix,
                     SkShader::TileMode tileMode);

    virtual ~GrGradientEffect();

    bool useAtlas() const { return SkToBool(-1 != fRow); }
    SkScalar getYCoord() const { return fYCoord; };

    virtual void getConstantColorComponents(GrColor* color, uint32_t* validFlags) const SK_OVERRIDE;

    SkGradientShaderBase::GpuColorType getColorType() const { return fColorType; }

    enum PremulType {
        kBeforeInterp_PremulType,
        kAfterInterp_PremulType,
    };

    PremulType getPremulType() const { return fPremulType; }

    const SkColor* getColors(int pos) const {
        SkASSERT(fColorType != SkGradientShaderBase::kTexture_GpuColorType);
        SkASSERT((pos-1) <= fColorType);
        return &fColors[pos];
    }

protected:

    






    static const int kMaxRandomGradientColors = 4;
    static int RandomGradientParams(SkRandom* r,
                                    SkColor colors[kMaxRandomGradientColors],
                                    SkScalar** stops,
                                    SkShader::TileMode* tm);

    virtual bool onIsEqual(const GrEffect& effect) const SK_OVERRIDE;

    const GrCoordTransform& getCoordTransform() const { return fCoordTransform; }

private:
    static const GrCoordSet kCoordSet = kLocal_GrCoordSet;

    GrCoordTransform fCoordTransform;
    GrTextureAccess fTextureAccess;
    SkScalar fYCoord;
    GrTextureStripAtlas* fAtlas;
    int fRow;
    bool fIsOpaque;
    SkGradientShaderBase::GpuColorType fColorType;
    SkColor fColors[3]; 
    PremulType fPremulType; 
                            
    typedef GrEffect INHERITED;

};




class GrGLGradientEffect : public GrGLEffect {
public:
    GrGLGradientEffect(const GrBackendEffectFactory& factory);
    virtual ~GrGLGradientEffect();

    virtual void setData(const GrGLUniformManager&, const GrDrawEffect&) SK_OVERRIDE;

protected:
    




    static uint32_t GenBaseGradientKey(const GrDrawEffect&);

    
    
    void emitUniforms(GrGLShaderBuilder* builder, uint32_t baseKey);


    
    
    
    void emitColor(GrGLShaderBuilder* builder,
                   const char* gradientTValue,
                   uint32_t baseKey,
                   const char* outputColor,
                   const char* inputColor,
                   const TextureSamplerArray& samplers);

private:
    enum {
        kPremulTypeKeyBitCnt = 1,
        kPremulTypeMask = 1,
        kPremulBeforeInterpKey = kPremulTypeMask,

        kTwoColorKey = 2 << kPremulTypeKeyBitCnt,
        kThreeColorKey = 3 << kPremulTypeKeyBitCnt,
        kColorKeyMask = kTwoColorKey | kThreeColorKey,
        kColorKeyBitCnt = 2,

        
        
        kBaseKeyBitCnt = (kPremulTypeKeyBitCnt + kColorKeyBitCnt)
    };
    GR_STATIC_ASSERT(kBaseKeyBitCnt <= 32);

    static SkGradientShaderBase::GpuColorType ColorTypeFromKey(uint32_t baseKey){
        if (kTwoColorKey == (baseKey & kColorKeyMask)) {
            return SkGradientShaderBase::kTwo_GpuColorType;
        } else if (kThreeColorKey == (baseKey & kColorKeyMask)) {
            return SkGradientShaderBase::kThree_GpuColorType;
        } else {return SkGradientShaderBase::kTexture_GpuColorType;}
    }

    static GrGradientEffect::PremulType PremulTypeFromKey(uint32_t baseKey){
        if (kPremulBeforeInterpKey == (baseKey & kPremulTypeMask)) {
            return GrGradientEffect::kBeforeInterp_PremulType;
        } else {
            return GrGradientEffect::kAfterInterp_PremulType;
        }
    }

    SkScalar fCachedYCoord;
    GrGLUniformManager::UniformHandle fFSYUni;
    GrGLUniformManager::UniformHandle fColorStartUni;
    GrGLUniformManager::UniformHandle fColorMidUni;
    GrGLUniformManager::UniformHandle fColorEndUni;

    typedef GrGLEffect INHERITED;
};

#endif

#endif
