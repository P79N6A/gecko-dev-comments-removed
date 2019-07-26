







#ifndef SkGradientShaderPriv_DEFINED
#define SkGradientShaderPriv_DEFINED

#include "SkGradientShader.h"
#include "SkClampRange.h"
#include "SkColorPriv.h"
#include "SkFlattenableBuffers.h"
#include "SkMallocPixelRef.h"
#include "SkUnitMapper.h"
#include "SkUtils.h"
#include "SkTemplates.h"
#include "SkBitmapCache.h"
#include "SkShader.h"

#ifndef SK_DISABLE_DITHER_32BIT_GRADIENT
    #define USE_DITHER_32BIT_GRADIENT
#endif

static void sk_memset32_dither(uint32_t dst[], uint32_t v0, uint32_t v1,
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



SkFixed clamp_tileproc(SkFixed x);
SkFixed mirror_tileproc(SkFixed x);
SkFixed repeat_tileproc(SkFixed x);



typedef SkFixed (*TileProc)(SkFixed);



static const TileProc gTileProcs[] = {
    clamp_tileproc,
    repeat_tileproc,
    mirror_tileproc
};



class SkGradientShaderBase : public SkShader {
public:
    SkGradientShaderBase(const SkColor colors[], const SkScalar pos[],
                int colorCount, SkShader::TileMode mode, SkUnitMapper* mapper);
    virtual ~SkGradientShaderBase();

    
    virtual bool setContext(const SkBitmap&, const SkPaint&, const SkMatrix&) SK_OVERRIDE;
    virtual uint32_t getFlags() SK_OVERRIDE { return fFlags; }
    virtual bool isOpaque() const SK_OVERRIDE;

    void getGradientTableBitmap(SkBitmap*) const;

    enum {
        
        
        kCache16Bits    = 8,
        kGradient16Length = (1 << kCache16Bits),
        
        
        
        
        
        kCache16Count   = kGradient16Length + 1,
        kCache16Shift   = 16 - kCache16Bits,
        kSqrt16Shift    = 8 - kCache16Bits,

        
        
        kCache32Bits    = 8,
        kGradient32Length = (1 << kCache32Bits),
        
        
        
        
        
        kCache32Count   = kGradient32Length + 1,
        kCache32Shift   = 16 - kCache32Bits,
        kSqrt32Shift    = 8 - kCache32Bits,

        
        
#ifdef USE_DITHER_32BIT_GRADIENT
        kDitherStride32 = kCache32Count,
#else
        kDitherStride32 = 0,
#endif
        kDitherStride16 = kCache16Count,
        kLerpRemainderMask32 = (1 << (16 - kCache32Bits)) - 1,

        kCache32ClampLower = -1,
        kCache32ClampUpper = kCache32Count * 2
    };


protected:
    SkGradientShaderBase(SkFlattenableReadBuffer& );
    virtual void flatten(SkFlattenableWriteBuffer&) const SK_OVERRIDE;

    SkUnitMapper* fMapper;
    SkMatrix    fPtsToUnit;     
    SkMatrix    fDstToIndex;
    SkMatrix::MapXYProc fDstToIndexProc;
    TileMode    fTileMode;
    TileProc    fTileProc;
    int         fColorCount;
    uint8_t     fDstToIndexClass;
    uint8_t     fFlags;

    struct Rec {
        SkFixed     fPos;   
        uint32_t    fScale; 
    };
    Rec*        fRecs;

    const uint16_t*     getCache16() const;
    const SkPMColor*    getCache32() const;

    void commonAsAGradient(GradientInfo*) const;

private:
    enum {
        kColorStorageCount = 4, 

        kStorageSize = kColorStorageCount * (sizeof(SkColor) + sizeof(Rec))
    };
    SkColor     fStorage[(kStorageSize + 3) >> 2];
    SkColor*    fOrigColors; 
    bool        fColorsAreOpaque;

    mutable uint16_t*   fCache16;   
    mutable SkPMColor*  fCache32;   

    mutable uint16_t*   fCache16Storage;    
    mutable SkMallocPixelRef* fCache32PixelRef;
    mutable unsigned    fCacheAlpha;        

    static void Build16bitCache(uint16_t[], SkColor c0, SkColor c1, int count);
    static void Build32bitCache(SkPMColor[], SkColor c0, SkColor c1, int count,
                                U8CPU alpha);
    void setCacheAlpha(U8CPU alpha) const;
    void initCommon();

    typedef SkShader INHERITED;
};



#if SK_SUPPORT_GPU

#include "gl/GrGLProgramStage.h"

class GrSamplerState;
class GrProgramStageFactory;
























 class GrTextureStripAtlas;


class GrGradientEffect : public GrCustomStage {
public:

    GrGradientEffect(GrContext* ctx, const SkGradientShaderBase& shader,
                     GrSamplerState* sampler);

    virtual ~GrGradientEffect();

    virtual int numTextures() const SK_OVERRIDE;
    virtual const GrTextureAccess& textureAccess(int index) const SK_OVERRIDE;

    bool useTexture() const { return fUseTexture; }
    bool useAtlas() const { return SkToBool(-1 != fRow); }
    GrScalar getYCoord() const { GrAssert(fUseTexture); return fYCoord; };

    virtual bool isEqual(const GrCustomStage& stage) const SK_OVERRIDE {
        const GrGradientEffect& s = static_cast<const GrGradientEffect&>(stage);
        return INHERITED::isEqual(stage) && this->useAtlas() == s.useAtlas() &&
               fYCoord == s.getYCoord();
    }

protected:

    






    static const int kMaxRandomGradientColors = 4;
    static int RandomGradientParams(SkRandom* r,
                                    SkColor colors[kMaxRandomGradientColors],
                                    SkScalar** stops,
                                    SkShader::TileMode* tm);

private:
    GrTextureAccess fTextureAccess;
    bool fUseTexture;
    GrScalar fYCoord;
    GrTextureStripAtlas* fAtlas;
    int fRow;

    typedef GrCustomStage INHERITED;

};




class GrGLGradientStage : public GrGLProgramStage {
public:

    GrGLGradientStage(const GrProgramStageFactory& factory);
    virtual ~GrGLGradientStage();

    virtual void setupVariables(GrGLShaderBuilder* builder) SK_OVERRIDE;
    virtual void setData(const GrGLUniformManager&,
                         const GrCustomStage&,
                         const GrRenderTarget*,
                         int stageNum) SK_OVERRIDE;

    
    
    void emitColorLookup(GrGLShaderBuilder* builder,
                         const char* gradientTValue,
                         const char* outputColor,
                         const char* inputColor,
                         const GrGLShaderBuilder::TextureSampler&);

private:

    GrScalar fCachedYCoord;
    GrGLUniformManager::UniformHandle fFSYUni;

    typedef GrGLProgramStage INHERITED;
};

#endif

#endif

