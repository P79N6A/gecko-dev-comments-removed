






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

SkFixed clamp_tileproc(SkFixed x);
SkFixed repeat_tileproc(SkFixed x);
SkFixed mirror_tileproc(SkFixed x);



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
        kCache16Count = (1 << kCache16Bits),
        kCache16Shift   = 16 - kCache16Bits,
        kSqrt16Shift    = 8 - kCache16Bits,

        
        
        kCache32Bits    = 8,
        kCache32Count   = (1 << kCache32Bits),
        kCache32Shift   = 16 - kCache32Bits,
        kSqrt32Shift    = 8 - kCache32Bits,

        
        
        kDitherStride32 = kCache32Count,
        kDitherStride16 = kCache16Count,

        kCache32ClampLower = -1,
        kCache32ClampUpper = kCache32Count * 4
    };


protected:
    SkGradientShaderBase(SkFlattenableReadBuffer& );
    virtual void flatten(SkFlattenableWriteBuffer&) const SK_OVERRIDE;
    SK_DEVELOPER_TO_STRING()

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

#include "gl/GrGLEffect.h"
#include "gl/GrGLEffectMatrix.h"

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
    const SkMatrix& getMatrix() const { return fMatrix;}

    virtual void getConstantColorComponents(GrColor* color, uint32_t* validFlags) const SK_OVERRIDE;

protected:

    






    static const int kMaxRandomGradientColors = 4;
    static int RandomGradientParams(SkMWCRandom* r,
                                    SkColor colors[kMaxRandomGradientColors],
                                    SkScalar** stops,
                                    SkShader::TileMode* tm);

    virtual bool onIsEqual(const GrEffect& effect) const SK_OVERRIDE;

private:

    GrTextureAccess fTextureAccess;
    SkScalar fYCoord;
    GrTextureStripAtlas* fAtlas;
    int fRow;
    SkMatrix fMatrix;
    bool fIsOpaque;

    typedef GrEffect INHERITED;

};




class GrGLGradientEffect : public GrGLEffect {
public:
    GrGLGradientEffect(const GrBackendEffectFactory& factory);
    virtual ~GrGLGradientEffect();

    virtual void setData(const GrGLUniformManager&, const GrDrawEffect&) SK_OVERRIDE;

protected:
    



    enum {
        kMatrixKeyBitCnt = GrGLEffectMatrix::kKeyBits,
        kMatrixKeyMask = (1 << kMatrixKeyBitCnt) - 1,
    };

    



    static EffectKey GenMatrixKey(const GrDrawEffect&);

    







    void setupMatrix(GrGLShaderBuilder* builder,
                     EffectKey key,
                     const char** fsCoordName,
                     const char** vsVaryingName = NULL,
                     GrSLType* vsVaryingType = NULL);

    
    
    void emitYCoordUniform(GrGLShaderBuilder* builder);

    
    
    
    void emitColorLookup(GrGLShaderBuilder* builder,
                         const char* gradientTValue,
                         const char* outputColor,
                         const char* inputColor,
                         const GrGLShaderBuilder::TextureSampler&);

private:
    static const GrEffect::CoordsType kCoordsType = GrEffect::kLocal_CoordsType;

    SkScalar fCachedYCoord;
    GrGLUniformManager::UniformHandle fFSYUni;
    GrGLEffectMatrix fEffectMatrix;

    typedef GrGLEffect INHERITED;
};

#endif

#endif
