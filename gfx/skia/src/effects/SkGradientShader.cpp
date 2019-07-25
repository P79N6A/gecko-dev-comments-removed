








#include "SkGradientShader.h"
#include "SkClampRange.h"
#include "SkColorPriv.h"
#include "SkMallocPixelRef.h"
#include "SkUnitMapper.h"
#include "SkUtils.h"
#include "SkTemplates.h"
#include "SkBitmapCache.h"

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



static SkFixed clamp_tileproc(SkFixed x) {
    return SkClampMax(x, 0xFFFF);
}



static SkFixed repeat_tileproc(SkFixed x) {
    return x & 0xFFFF;
}

static inline int repeat_bits(int x, const int bits) {
    return x & ((1 << bits) - 1);
}

static inline int repeat_8bits(int x) {
    return x & 0xFF;
}





#if defined(_MSC_VER) && (_MSC_VER >= 1600)
#pragma optimize("", off)
#endif

static inline SkFixed mirror_tileproc(SkFixed x) {
    int s = x << 15 >> 31;
    return (x ^ s) & 0xFFFF;
}

static inline int mirror_bits(int x, const int bits) {
#ifdef SK_CPU_HAS_CONDITIONAL_INSTR
    if (x & (1 << bits))
        x = ~x;
    return x & ((1 << bits) - 1);
#else
    int s = x << (31 - bits) >> 31;
    return (x ^ s) & ((1 << bits) - 1);
#endif
}

static inline int mirror_8bits(int x) {
#ifdef SK_CPU_HAS_CONDITIONAL_INSTR
    if (x & 256) {
        x = ~x;
    }
    return x & 255;
#else
    int s = x << 23 >> 31;
    return (x ^ s) & 0xFF;
#endif
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




class Gradient_Shader : public SkShader {
public:
    Gradient_Shader(const SkColor colors[], const SkScalar pos[],
                int colorCount, SkShader::TileMode mode, SkUnitMapper* mapper);
    virtual ~Gradient_Shader();

    
    virtual bool setContext(const SkBitmap&, const SkPaint&, const SkMatrix&) SK_OVERRIDE;
    virtual uint32_t getFlags() SK_OVERRIDE { return fFlags; }
    virtual bool isOpaque() const SK_OVERRIDE;

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
        kLerpRemainderMask32 = (1 << (16 - kCache32Bits)) - 1
    };


protected:
    Gradient_Shader(SkFlattenableReadBuffer& );
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

    void commonAsABitmap(SkBitmap*) const;
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

    static SkPMColor PremultiplyColor(SkColor c0, U8CPU alpha);
    static void Build16bitCache(uint16_t[], SkColor c0, SkColor c1, int count);
    static void Build32bitCache(SkPMColor[], SkColor c0, SkColor c1, int count,
                                U8CPU alpha);
    void setCacheAlpha(U8CPU alpha) const;
    void initCommon();

    typedef SkShader INHERITED;
};

Gradient_Shader::Gradient_Shader(const SkColor colors[], const SkScalar pos[],
             int colorCount, SkShader::TileMode mode, SkUnitMapper* mapper) {
    SkASSERT(colorCount > 1);

    fCacheAlpha = 256;  

    fMapper = mapper;
    SkSafeRef(mapper);

    SkASSERT((unsigned)mode < SkShader::kTileModeCount);
    SkASSERT(SkShader::kTileModeCount == SK_ARRAY_COUNT(gTileProcs));
    fTileMode = mode;
    fTileProc = gTileProcs[mode];

    fCache16 = fCache16Storage = NULL;
    fCache32 = NULL;
    fCache32PixelRef = NULL;

    










    fColorCount = colorCount;
    
    bool dummyFirst = false;
    bool dummyLast = false;
    if (pos) {
        dummyFirst = pos[0] != 0;
        dummyLast = pos[colorCount - 1] != SK_Scalar1;
        fColorCount += dummyFirst + dummyLast;
    }

    if (fColorCount > kColorStorageCount) {
        size_t size = sizeof(SkColor) + sizeof(Rec);
        fOrigColors = reinterpret_cast<SkColor*>(
                                        sk_malloc_throw(size * fColorCount));
    }
    else {
        fOrigColors = fStorage;
    }

    
    {
        SkColor* origColors = fOrigColors;
        if (dummyFirst) {
            *origColors++ = colors[0];
        }
        memcpy(origColors, colors, colorCount * sizeof(SkColor));
        if (dummyLast) {
            origColors += colorCount;
            *origColors = colors[colorCount - 1];
        }
    }

    fRecs = (Rec*)(fOrigColors + fColorCount);
    if (fColorCount > 2) {
        Rec* recs = fRecs;
        recs->fPos = 0;
        
        recs += 1;
        if (pos) {
            






            SkFixed prev = 0;
            int startIndex = dummyFirst ? 0 : 1;
            int count = colorCount + dummyLast;
            for (int i = startIndex; i < count; i++) {
                
                SkFixed curr;
                if (i == colorCount) {  
                    curr = SK_Fixed1;
                } else {
                    curr = SkScalarToFixed(pos[i]);
                }
                
                if (curr < 0) {
                    curr = 0;
                } else if (curr > SK_Fixed1) {
                    curr = SK_Fixed1;
                }
                recs->fPos = curr;
                if (curr > prev) {
                    recs->fScale = (1 << 24) / (curr - prev);
                } else {
                    recs->fScale = 0; 
                }
                
                prev = curr;
                recs += 1;
            }
        } else {    
            SkFixed dp = SK_Fixed1 / (colorCount - 1);
            SkFixed p = dp;
            SkFixed scale = (colorCount - 1) << 8;  
            for (int i = 1; i < colorCount; i++) {
                recs->fPos   = p;
                recs->fScale = scale;
                recs += 1;
                p += dp;
            }
        }
    }
    this->initCommon();
}

Gradient_Shader::Gradient_Shader(SkFlattenableReadBuffer& buffer) :
    INHERITED(buffer) {
    fCacheAlpha = 256;

    fMapper = static_cast<SkUnitMapper*>(buffer.readFlattenable());

    fCache16 = fCache16Storage = NULL;
    fCache32 = NULL;
    fCache32PixelRef = NULL;

    int colorCount = fColorCount = buffer.readU32();
    if (colorCount > kColorStorageCount) {
        size_t size = sizeof(SkColor) + sizeof(SkPMColor) + sizeof(Rec);
        fOrigColors = (SkColor*)sk_malloc_throw(size * colorCount);
    } else {
        fOrigColors = fStorage;
    }
    buffer.read(fOrigColors, colorCount * sizeof(SkColor));

    fTileMode = (TileMode)buffer.readU8();
    fTileProc = gTileProcs[fTileMode];
    fRecs = (Rec*)(fOrigColors + colorCount);
    if (colorCount > 2) {
        Rec* recs = fRecs;
        recs[0].fPos = 0;
        for (int i = 1; i < colorCount; i++) {
            recs[i].fPos = buffer.readS32();
            recs[i].fScale = buffer.readU32();
        }
    }
    buffer.readMatrix(&fPtsToUnit);
    this->initCommon();
}

Gradient_Shader::~Gradient_Shader() {
    if (fCache16Storage) {
        sk_free(fCache16Storage);
    }
    SkSafeUnref(fCache32PixelRef);
    if (fOrigColors != fStorage) {
        sk_free(fOrigColors);
    }
    SkSafeUnref(fMapper);
}

void Gradient_Shader::initCommon() {
    fFlags = 0;
    unsigned colorAlpha = 0xFF;
    for (int i = 0; i < fColorCount; i++) {
        colorAlpha &= SkColorGetA(fOrigColors[i]);
    }
    fColorsAreOpaque = colorAlpha == 0xFF;
}

void Gradient_Shader::flatten(SkFlattenableWriteBuffer& buffer) const {
    this->INHERITED::flatten(buffer);
    buffer.writeFlattenable(fMapper);
    buffer.write32(fColorCount);
    buffer.writeMul4(fOrigColors, fColorCount * sizeof(SkColor));
    buffer.write8(fTileMode);
    if (fColorCount > 2) {
        Rec* recs = fRecs;
        for (int i = 1; i < fColorCount; i++) {
            buffer.write32(recs[i].fPos);
            buffer.write32(recs[i].fScale);
        }
    }
    buffer.writeMatrix(fPtsToUnit);
}

bool Gradient_Shader::isOpaque() const {
    return fColorsAreOpaque;
}

bool Gradient_Shader::setContext(const SkBitmap& device,
                                 const SkPaint& paint,
                                 const SkMatrix& matrix) {
    if (!this->INHERITED::setContext(device, paint, matrix)) {
        return false;
    }

    const SkMatrix& inverse = this->getTotalInverse();

    if (!fDstToIndex.setConcat(fPtsToUnit, inverse)) {
        return false;
    }

    fDstToIndexProc = fDstToIndex.getMapXYProc();
    fDstToIndexClass = (uint8_t)SkShader::ComputeMatrixClass(fDstToIndex);

    
    unsigned paintAlpha = this->getPaintAlpha();

    fFlags = this->INHERITED::getFlags();
    if (fColorsAreOpaque && paintAlpha == 0xFF) {
        fFlags |= kOpaqueAlpha_Flag;
    }
    
    
    if (fColorsAreOpaque) {
        fFlags |= kHasSpan16_Flag;
    }

    this->setCacheAlpha(paintAlpha);
    return true;
}

void Gradient_Shader::setCacheAlpha(U8CPU alpha) const {
    
    
    
    if (fCacheAlpha != alpha) {
        fCache16 = NULL;            
        fCache32 = NULL;            
        fCacheAlpha = alpha;        
        
        if (fCache32PixelRef) {
            fCache32PixelRef->notifyPixelsChanged();
        }
    }
}

#define Fixed_To_Dot8(x)        (((x) + 0x80) >> 8)





void Gradient_Shader::Build16bitCache(uint16_t cache[], SkColor c0, SkColor c1,
                                      int count) {
    SkASSERT(count > 1);
    SkASSERT(SkColorGetA(c0) == 0xFF);
    SkASSERT(SkColorGetA(c1) == 0xFF);

    SkFixed r = SkColorGetR(c0);
    SkFixed g = SkColorGetG(c0);
    SkFixed b = SkColorGetB(c0);

    SkFixed dr = SkIntToFixed(SkColorGetR(c1) - r) / (count - 1);
    SkFixed dg = SkIntToFixed(SkColorGetG(c1) - g) / (count - 1);
    SkFixed db = SkIntToFixed(SkColorGetB(c1) - b) / (count - 1);

    r = SkIntToFixed(r) + 0x8000;
    g = SkIntToFixed(g) + 0x8000;
    b = SkIntToFixed(b) + 0x8000;

    do {
        unsigned rr = r >> 16;
        unsigned gg = g >> 16;
        unsigned bb = b >> 16;
        cache[0] = SkPackRGB16(SkR32ToR16(rr), SkG32ToG16(gg), SkB32ToB16(bb));
        cache[kCache16Count] = SkDitherPack888ToRGB16(rr, gg, bb);
        cache += 1;
        r += dr;
        g += dg;
        b += db;
    } while (--count != 0);
}





static inline U8CPU dither_fixed_to_8(SkFixed n) {
    n >>= 8;
    return ((n << 1) - ((n >> 8 << 8) | (n >> 8))) >> 8;
}





static inline U8CPU dither_ceil_fixed_to_8(SkFixed n) {
    n >>= 8;
    return ((n << 1) - (n | (n >> 8))) >> 8;
}

SkPMColor Gradient_Shader::PremultiplyColor(SkColor c0, U8CPU paintAlpha)
{
    SkFixed a = SkMulDiv255Round(SkColorGetA(c0), paintAlpha);
    SkFixed r = SkColorGetR(c0);
    SkFixed g = SkColorGetG(c0);
    SkFixed b = SkColorGetB(c0);

    a = SkIntToFixed(a) + 0x8000;
    r = SkIntToFixed(r) + 0x8000;
    g = SkIntToFixed(g) + 0x8000;
    b = SkIntToFixed(b) + 0x8000;

    return SkPremultiplyARGBInline(a >> 16, r >> 16, g >> 16, b >> 16);
}

void Gradient_Shader::Build32bitCache(SkPMColor cache[], SkColor c0, SkColor c1,
                                      int count, U8CPU paintAlpha) {
    SkASSERT(count > 1);

    
    SkFixed a = SkMulDiv255Round(SkColorGetA(c0), paintAlpha);
    SkFixed da;
    {
        int tmp = SkMulDiv255Round(SkColorGetA(c1), paintAlpha);
        da = SkIntToFixed(tmp - a) / (count - 1);
    }

    SkFixed r = SkColorGetR(c0);
    SkFixed g = SkColorGetG(c0);
    SkFixed b = SkColorGetB(c0);
    SkFixed dr = SkIntToFixed(SkColorGetR(c1) - r) / (count - 1);
    SkFixed dg = SkIntToFixed(SkColorGetG(c1) - g) / (count - 1);
    SkFixed db = SkIntToFixed(SkColorGetB(c1) - b) / (count - 1);

    a = SkIntToFixed(a) + 0x8000;
    r = SkIntToFixed(r) + 0x8000;
    g = SkIntToFixed(g) + 0x8000;
    b = SkIntToFixed(b) + 0x8000;

    do {
        cache[0] = SkPremultiplyARGBInline(a >> 16, r >> 16, g >> 16, b >> 16);
        cache[kCache32Count] =
            SkPremultiplyARGBInline(dither_ceil_fixed_to_8(a),
                                    dither_fixed_to_8(r),
                                    dither_fixed_to_8(g),
                                    dither_fixed_to_8(b));
        cache += 1;
        a += da;
        r += dr;
        g += dg;
        b += db;
    } while (--count != 0);
}

static inline int SkFixedToFFFF(SkFixed x) {
    SkASSERT((unsigned)x <= SK_Fixed1);
    return x - (x >> 16);
}

static inline U16CPU bitsTo16(unsigned x, const unsigned bits) {
    SkASSERT(x < (1U << bits));
    if (6 == bits) {
        return (x << 10) | (x << 4) | (x >> 2);
    }
    if (8 == bits) {
        return (x << 8) | x;
    }
    sk_throw();
    return 0;
}




static void complete_16bit_cache(uint16_t* cache, int stride) {
    cache[stride - 1] = cache[stride - 2];
    cache[2 * stride - 1] = cache[2 * stride - 2];
}

const uint16_t* Gradient_Shader::getCache16() const {
    if (fCache16 == NULL) {
        
        const int entryCount = kCache16Count * 2;
        const size_t allocSize = sizeof(uint16_t) * entryCount;

        if (fCache16Storage == NULL) { 
            fCache16Storage = (uint16_t*)sk_malloc_throw(allocSize);
        }
        fCache16 = fCache16Storage;
        if (fColorCount == 2) {
            Build16bitCache(fCache16, fOrigColors[0], fOrigColors[1],
                            kGradient16Length);
        } else {
            Rec* rec = fRecs;
            int prevIndex = 0;
            for (int i = 1; i < fColorCount; i++) {
                int nextIndex = SkFixedToFFFF(rec[i].fPos) >> kCache16Shift;
                SkASSERT(nextIndex < kCache16Count);

                if (nextIndex > prevIndex)
                    Build16bitCache(fCache16 + prevIndex, fOrigColors[i-1], fOrigColors[i], nextIndex - prevIndex + 1);
                prevIndex = nextIndex;
            }
            
            SkASSERT(prevIndex == kGradient16Length - 1);
        }

        if (fMapper) {
            fCache16Storage = (uint16_t*)sk_malloc_throw(allocSize);
            uint16_t* linear = fCache16;         
            uint16_t* mapped = fCache16Storage;  
            SkUnitMapper* map = fMapper;
            for (int i = 0; i < kGradient16Length; i++) {
                int index = map->mapUnit16(bitsTo16(i, kCache16Bits)) >> kCache16Shift;
                mapped[i] = linear[index];
                mapped[i + kCache16Count] = linear[index + kCache16Count];
            }
            sk_free(fCache16);
            fCache16 = fCache16Storage;
        }
        complete_16bit_cache(fCache16, kCache16Count);
    }
    return fCache16;
}




static void complete_32bit_cache(SkPMColor* cache, int stride) {
    cache[stride - 1] = cache[stride - 2];
    cache[2 * stride - 1] = cache[2 * stride - 2];
}

const SkPMColor* Gradient_Shader::getCache32() const {
    if (fCache32 == NULL) {
        
        const int entryCount = kCache32Count * 2 + 2;
        const size_t allocSize = sizeof(SkPMColor) * entryCount;

        if (NULL == fCache32PixelRef) {
            fCache32PixelRef = SkNEW_ARGS(SkMallocPixelRef,
                                          (NULL, allocSize, NULL));
        }
        fCache32 = (SkPMColor*)fCache32PixelRef->getAddr() + 1;
        if (fColorCount == 2) {
            Build32bitCache(fCache32, fOrigColors[0], fOrigColors[1],
                            kGradient32Length, fCacheAlpha);
        } else {
            Rec* rec = fRecs;
            int prevIndex = 0;
            for (int i = 1; i < fColorCount; i++) {
                int nextIndex = SkFixedToFFFF(rec[i].fPos) >> kCache32Shift;
                SkASSERT(nextIndex < kGradient32Length);

                if (nextIndex > prevIndex)
                    Build32bitCache(fCache32 + prevIndex, fOrigColors[i-1],
                                    fOrigColors[i],
                                    nextIndex - prevIndex + 1, fCacheAlpha);
                prevIndex = nextIndex;
            }
            SkASSERT(prevIndex == kGradient32Length - 1);
        }

        if (fMapper) {
            SkMallocPixelRef* newPR = SkNEW_ARGS(SkMallocPixelRef,
                                                 (NULL, allocSize, NULL));
            SkPMColor* linear = fCache32;           
            SkPMColor* mapped = (SkPMColor*)newPR->getAddr() + 1;    
            SkUnitMapper* map = fMapper;
            for (int i = 0; i < kGradient32Length; i++) {
                int index = map->mapUnit16((i << 8) | i) >> 8;
                mapped[i] = linear[index];
                mapped[i + kCache32Count] = linear[index + kCache32Count];
            }
            fCache32PixelRef->unref();
            fCache32PixelRef = newPR;
            fCache32 = (SkPMColor*)newPR->getAddr() + 1;
        }
        complete_32bit_cache(fCache32, kCache32Count);
    }
    
    fCache32[-1] = PremultiplyColor(fOrigColors[0], fCacheAlpha);
    fCache32[kCache32Count * 2] = PremultiplyColor(fOrigColors[fColorCount - 1], fCacheAlpha);
    return fCache32;
}









void Gradient_Shader::commonAsABitmap(SkBitmap* bitmap) const {
    
    
    this->setCacheAlpha(0xFF);

    
    if (fMapper) {
        
        (void)this->getCache32();
        bitmap->setConfig(SkBitmap::kARGB_8888_Config, kGradient32Length, 1);
        bitmap->setPixelRef(fCache32PixelRef);
        return;
    }

    
    int count = 1 + fColorCount;
    if (fColorCount > 2) {
        count += fColorCount - 1;    
    }

    SkAutoSTMalloc<16, int32_t> storage(count);
    int32_t* buffer = storage.get();

    *buffer++ = fColorCount;
    memcpy(buffer, fOrigColors, fColorCount * sizeof(SkColor));
    buffer += fColorCount;
    if (fColorCount > 2) {
        for (int i = 1; i < fColorCount; i++) {
            *buffer++ = fRecs[i].fPos;
        }
    }
    SkASSERT(buffer - storage.get() == count);

    

    SK_DECLARE_STATIC_MUTEX(gMutex);
    static SkBitmapCache* gCache;
    
    static const int MAX_NUM_CACHED_GRADIENT_BITMAPS = 32;
    SkAutoMutexAcquire ama(gMutex);

    if (NULL == gCache) {
        gCache = new SkBitmapCache(MAX_NUM_CACHED_GRADIENT_BITMAPS);
    }
    size_t size = count * sizeof(int32_t);

    if (!gCache->find(storage.get(), size, bitmap)) {
        
        (void)this->getCache32();
        
        
        bitmap->setConfig(SkBitmap::kARGB_8888_Config, kGradient32Length, 1);
        bitmap->setPixelRef(fCache32PixelRef);

        gCache->add(storage.get(), size, *bitmap);
    }
}

void Gradient_Shader::commonAsAGradient(GradientInfo* info) const {
    if (info) {
        if (info->fColorCount >= fColorCount) {
            if (info->fColors) {
                memcpy(info->fColors, fOrigColors,
                       fColorCount * sizeof(SkColor));
            }
            if (info->fColorOffsets) {
                if (fColorCount == 2) {
                    info->fColorOffsets[0] = 0;
                    info->fColorOffsets[1] = SK_Scalar1;
                } else if (fColorCount > 2) {
                    for (int i = 0; i < fColorCount; i++)
                        info->fColorOffsets[i] = SkFixedToScalar(fRecs[i].fPos);
                }
            }
        }
        info->fColorCount = fColorCount;
        info->fTileMode = fTileMode;
    }
}



static void pts_to_unit_matrix(const SkPoint pts[2], SkMatrix* matrix) {
    SkVector    vec = pts[1] - pts[0];
    SkScalar    mag = vec.length();
    SkScalar    inv = mag ? SkScalarInvert(mag) : 0;

    vec.scale(inv);
    matrix->setSinCos(-vec.fY, vec.fX, pts[0].fX, pts[0].fY);
    matrix->postTranslate(-pts[0].fX, -pts[0].fY);
    matrix->postScale(inv, inv);
}



class Linear_Gradient : public Gradient_Shader {
public:
    Linear_Gradient(const SkPoint pts[2],
                    const SkColor colors[], const SkScalar pos[], int colorCount,
                    SkShader::TileMode mode, SkUnitMapper* mapper)
        : Gradient_Shader(colors, pos, colorCount, mode, mapper),
          fStart(pts[0]),
          fEnd(pts[1])
    {
        pts_to_unit_matrix(pts, &fPtsToUnit);
    }

    virtual bool setContext(const SkBitmap&, const SkPaint&, const SkMatrix&) SK_OVERRIDE;
    virtual void shadeSpan(int x, int y, SkPMColor dstC[], int count) SK_OVERRIDE;
    virtual void shadeSpan16(int x, int y, uint16_t dstC[], int count) SK_OVERRIDE;
    virtual BitmapType asABitmap(SkBitmap*, SkMatrix*, TileMode*,
                             SkScalar* twoPointRadialParams) const SK_OVERRIDE;
    virtual GradientType asAGradient(GradientInfo* info) const SK_OVERRIDE;

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(Linear_Gradient)

protected:
    Linear_Gradient(SkFlattenableReadBuffer& buffer)
        : INHERITED(buffer),
          fStart(buffer.readPoint()),
          fEnd(buffer.readPoint()) {
    }
    virtual void flatten(SkFlattenableWriteBuffer& buffer)  const SK_OVERRIDE {
        this->INHERITED::flatten(buffer);
        buffer.writePoint(fStart);
        buffer.writePoint(fEnd);
    }

private:
    typedef Gradient_Shader INHERITED;
    const SkPoint fStart;
    const SkPoint fEnd;
};

bool Linear_Gradient::setContext(const SkBitmap& device, const SkPaint& paint,
                                 const SkMatrix& matrix) {
    if (!this->INHERITED::setContext(device, paint, matrix)) {
        return false;
    }

    unsigned mask = SkMatrix::kTranslate_Mask | SkMatrix::kScale_Mask;
    if ((fDstToIndex.getType() & ~mask) == 0) {
        fFlags |= SkShader::kConstInY32_Flag;
        if ((fFlags & SkShader::kHasSpan16_Flag) && !paint.isDither()) {
            
            
            
            fFlags |= SkShader::kConstInY16_Flag;
        }
    }
    return true;
}

#define NO_CHECK_ITER               \
    do {                            \
    unsigned fi = fx >> Gradient_Shader::kCache32Shift; \
    SkASSERT(fi <= 0xFF);           \
    fx += dx;                       \
    *dstC++ = cache[toggle + fi];   \
    toggle ^= Gradient_Shader::kDitherStride32; \
    } while (0)

namespace {

typedef void (*LinearShadeProc)(TileProc proc, SkFixed dx, SkFixed fx,
                                SkPMColor* dstC, const SkPMColor* cache,
                                int toggle, int count);




void shadeSpan_linear_vertical_lerp(TileProc proc, SkFixed dx, SkFixed fx,
                                    SkPMColor* SK_RESTRICT dstC,
                                    const SkPMColor* SK_RESTRICT cache,
                                    int toggle, int count) {
    if (proc == clamp_tileproc) {
        
        
        if (fx < 0) {
            sk_memset32(dstC, cache[-1], count);
            return;
        } else if (fx > 0xFFFF) {
            sk_memset32(dstC, cache[Gradient_Shader::kCache32Count * 2], count);
            return;
        }
    }

    
    
    
    unsigned fullIndex = proc(fx);
    unsigned fi = fullIndex >> (16 - Gradient_Shader::kCache32Bits);
    unsigned remainder = fullIndex & Gradient_Shader::kLerpRemainderMask32;
    SkPMColor lerp =
        SkFastFourByteInterp(
            cache[toggle + fi + 1],
            cache[toggle + fi], remainder);
    SkPMColor dlerp =
        SkFastFourByteInterp(
            cache[(toggle ^ Gradient_Shader::kDitherStride32) + fi + 1],
            cache[(toggle ^ Gradient_Shader::kDitherStride32) + fi], remainder);
    sk_memset32_dither(dstC, lerp, dlerp, count);
}

void shadeSpan_linear_clamp(TileProc proc, SkFixed dx, SkFixed fx,
                            SkPMColor* SK_RESTRICT dstC,
                            const SkPMColor* SK_RESTRICT cache,
                            int toggle, int count) {
    SkClampRange range;
    range.init(fx, dx, count, 0, Gradient_Shader::kGradient32Length);

    if ((count = range.fCount0) > 0) {
        
        sk_memset32(dstC, cache[-1], count);
        dstC += count;
    }
    if ((count = range.fCount1) > 0) {
        int unroll = count >> 3;
        fx = range.fFx1;
        for (int i = 0; i < unroll; i++) {
            NO_CHECK_ITER;  NO_CHECK_ITER;
            NO_CHECK_ITER;  NO_CHECK_ITER;
            NO_CHECK_ITER;  NO_CHECK_ITER;
            NO_CHECK_ITER;  NO_CHECK_ITER;
        }
        if ((count &= 7) > 0) {
            do {
                NO_CHECK_ITER;
            } while (--count != 0);
        }
    }
    if ((count = range.fCount2) > 0) {
        
        sk_memset32(dstC, cache[Gradient_Shader::kCache32Count * 2], count);
    }
}

void shadeSpan_linear_mirror(TileProc proc, SkFixed dx, SkFixed fx,
                             SkPMColor* SK_RESTRICT dstC,
                             const SkPMColor* SK_RESTRICT cache,
                             int toggle, int count) {
    do {
        unsigned fi = mirror_8bits(fx >> 8);
        SkASSERT(fi <= 0xFF);
        fx += dx;
        *dstC++ = cache[toggle + fi];
        toggle ^= Gradient_Shader::kDitherStride32;
    } while (--count != 0);
}

void shadeSpan_linear_repeat(TileProc proc, SkFixed dx, SkFixed fx,
        SkPMColor* SK_RESTRICT dstC,
        const SkPMColor* SK_RESTRICT cache,
        int toggle, int count) {
    do {
        unsigned fi = repeat_8bits(fx >> 8);
        SkASSERT(fi <= 0xFF);
        fx += dx;
        *dstC++ = cache[toggle + fi];
        toggle ^= Gradient_Shader::kDitherStride32;
    } while (--count != 0);
}

}

void Linear_Gradient::shadeSpan(int x, int y, SkPMColor* SK_RESTRICT dstC,
                                int count) {
    SkASSERT(count > 0);

    SkPoint             srcPt;
    SkMatrix::MapXYProc dstProc = fDstToIndexProc;
    TileProc            proc = fTileProc;
    const SkPMColor* SK_RESTRICT cache = this->getCache32();
#ifdef USE_DITHER_32BIT_GRADIENT
    int                 toggle = ((x ^ y) & 1) * kDitherStride32;
#else
    int toggle = 0;
#endif

    if (fDstToIndexClass != kPerspective_MatrixClass) {
        dstProc(fDstToIndex, SkIntToScalar(x) + SK_ScalarHalf,
                             SkIntToScalar(y) + SK_ScalarHalf, &srcPt);
        SkFixed dx, fx = SkScalarToFixed(srcPt.fX);

        if (fDstToIndexClass == kFixedStepInX_MatrixClass) {
            SkFixed dxStorage[1];
            (void)fDstToIndex.fixedStepInX(SkIntToScalar(y), dxStorage, NULL);
            dx = dxStorage[0];
        } else {
            SkASSERT(fDstToIndexClass == kLinear_MatrixClass);
            dx = SkScalarToFixed(fDstToIndex.getScaleX());
        }

        LinearShadeProc shadeProc = shadeSpan_linear_repeat;
        if (SkFixedNearlyZero(dx)) {
#ifdef SK_SIMPLE_TWOCOLOR_VERTICAL_GRADIENTS
            if (fColorCount > 2) {
                shadeProc = shadeSpan_linear_vertical_lerp;
            } else {
                shadeProc = shadeSpan_linear_vertical;
            }
#else
            shadeProc = shadeSpan_linear_vertical_lerp;
#endif
        } else if (proc == clamp_tileproc) {
            shadeProc = shadeSpan_linear_clamp;
        } else if (proc == mirror_tileproc) {
            shadeProc = shadeSpan_linear_mirror;
        } else {
            SkASSERT(proc == repeat_tileproc);
        }
        (*shadeProc)(proc, dx, fx, dstC, cache, toggle, count);
    } else {
        SkScalar    dstX = SkIntToScalar(x);
        SkScalar    dstY = SkIntToScalar(y);
        do {
            dstProc(fDstToIndex, dstX, dstY, &srcPt);
            unsigned fi = proc(SkScalarToFixed(srcPt.fX));
            SkASSERT(fi <= 0xFFFF);
            *dstC++ = cache[toggle + (fi >> kCache32Shift)];
            toggle ^= Gradient_Shader::kDitherStride32;
            dstX += SK_Scalar1;
        } while (--count != 0);
    }
}

SkShader::BitmapType Linear_Gradient::asABitmap(SkBitmap* bitmap,
                                                SkMatrix* matrix,
                                                TileMode xy[],
                                        SkScalar* twoPointRadialParams) const {
    if (bitmap) {
        this->commonAsABitmap(bitmap);
    }
    if (matrix) {
        matrix->setScale(SkIntToScalar(kGradient32Length), SK_Scalar1);
        matrix->preConcat(fPtsToUnit);
    }
    if (xy) {
        xy[0] = fTileMode;
        xy[1] = kClamp_TileMode;
    }
    return kDefault_BitmapType;
}

SkShader::GradientType Linear_Gradient::asAGradient(GradientInfo* info) const {
    if (info) {
        commonAsAGradient(info);
        info->fPoint[0] = fStart;
        info->fPoint[1] = fEnd;
    }
    return kLinear_GradientType;
}

static void dither_memset16(uint16_t dst[], uint16_t value, uint16_t other,
                            int count) {
    if (reinterpret_cast<uintptr_t>(dst) & 2) {
        *dst++ = value;
        count -= 1;
        SkTSwap(value, other);
    }

    sk_memset32((uint32_t*)dst, (value << 16) | other, count >> 1);

    if (count & 1) {
        dst[count - 1] = value;
    }
}

#define NO_CHECK_ITER_16                \
    do {                                \
    unsigned fi = fx >> Gradient_Shader::kCache16Shift;  \
    SkASSERT(fi < Gradient_Shader::kCache16Count);       \
    fx += dx;                           \
    *dstC++ = cache[toggle + fi];       \
    toggle ^= Gradient_Shader::kDitherStride16;            \
    } while (0)

namespace {

typedef void (*LinearShade16Proc)(TileProc proc, SkFixed dx, SkFixed fx,
                                  uint16_t* dstC, const uint16_t* cache,
                                  int toggle, int count);

void shadeSpan16_linear_vertical(TileProc proc, SkFixed dx, SkFixed fx,
                                 uint16_t* SK_RESTRICT dstC,
                                 const uint16_t* SK_RESTRICT cache,
                                 int toggle, int count) {
    
    unsigned fi = proc(fx) >> Gradient_Shader::kCache16Shift;
    SkASSERT(fi < Gradient_Shader::kCache16Count);
    dither_memset16(dstC, cache[toggle + fi],
        cache[(toggle ^ Gradient_Shader::kDitherStride16) + fi], count);

}

void shadeSpan16_linear_clamp(TileProc proc, SkFixed dx, SkFixed fx,
                              uint16_t* SK_RESTRICT dstC,
                              const uint16_t* SK_RESTRICT cache,
                              int toggle, int count) {
    SkClampRange range;
    range.init(fx, dx, count, 0, Gradient_Shader::kGradient16Length);

    if ((count = range.fCount0) > 0) {
        dither_memset16(dstC,
            cache[toggle + range.fV0],
            cache[(toggle ^ Gradient_Shader::kDitherStride16) + range.fV0],
            count);
        dstC += count;
    }
    if ((count = range.fCount1) > 0) {
        int unroll = count >> 3;
        fx = range.fFx1;
        for (int i = 0; i < unroll; i++) {
            NO_CHECK_ITER_16;  NO_CHECK_ITER_16;
            NO_CHECK_ITER_16;  NO_CHECK_ITER_16;
            NO_CHECK_ITER_16;  NO_CHECK_ITER_16;
            NO_CHECK_ITER_16;  NO_CHECK_ITER_16;
        }
        if ((count &= 7) > 0) {
            do {
                NO_CHECK_ITER_16;
            } while (--count != 0);
        }
    }
    if ((count = range.fCount2) > 0) {
        dither_memset16(dstC,
            cache[toggle + range.fV1],
            cache[(toggle ^ Gradient_Shader::kDitherStride16) + range.fV1],
            count);
    }
}

void shadeSpan16_linear_mirror(TileProc proc, SkFixed dx, SkFixed fx,
                               uint16_t* SK_RESTRICT dstC,
                               const uint16_t* SK_RESTRICT cache,
                               int toggle, int count) {
    do {
        unsigned fi = mirror_bits(fx >> Gradient_Shader::kCache16Shift,
                                        Gradient_Shader::kCache16Bits);
        SkASSERT(fi < Gradient_Shader::kCache16Count);
        fx += dx;
        *dstC++ = cache[toggle + fi];
        toggle ^= Gradient_Shader::kDitherStride16;
    } while (--count != 0);
}

void shadeSpan16_linear_repeat(TileProc proc, SkFixed dx, SkFixed fx,
                               uint16_t* SK_RESTRICT dstC,
                               const uint16_t* SK_RESTRICT cache,
                               int toggle, int count) {
    SkASSERT(proc == repeat_tileproc);
    do {
        unsigned fi = repeat_bits(fx >> Gradient_Shader::kCache16Shift,
                                  Gradient_Shader::kCache16Bits);
        SkASSERT(fi < Gradient_Shader::kCache16Count);
        fx += dx;
        *dstC++ = cache[toggle + fi];
        toggle ^= Gradient_Shader::kDitherStride16;
    } while (--count != 0);
}
}

void Linear_Gradient::shadeSpan16(int x, int y,
                                  uint16_t* SK_RESTRICT dstC, int count) {
    SkASSERT(count > 0);

    SkPoint             srcPt;
    SkMatrix::MapXYProc dstProc = fDstToIndexProc;
    TileProc            proc = fTileProc;
    const uint16_t* SK_RESTRICT cache = this->getCache16();
    int                 toggle = ((x ^ y) & 1) * kDitherStride16;

    if (fDstToIndexClass != kPerspective_MatrixClass) {
        dstProc(fDstToIndex, SkIntToScalar(x) + SK_ScalarHalf,
                             SkIntToScalar(y) + SK_ScalarHalf, &srcPt);
        SkFixed dx, fx = SkScalarToFixed(srcPt.fX);

        if (fDstToIndexClass == kFixedStepInX_MatrixClass) {
            SkFixed dxStorage[1];
            (void)fDstToIndex.fixedStepInX(SkIntToScalar(y), dxStorage, NULL);
            dx = dxStorage[0];
        } else {
            SkASSERT(fDstToIndexClass == kLinear_MatrixClass);
            dx = SkScalarToFixed(fDstToIndex.getScaleX());
        }

        LinearShade16Proc shadeProc = shadeSpan16_linear_repeat;
        if (SkFixedNearlyZero(dx)) {
            shadeProc = shadeSpan16_linear_vertical;
        } else if (proc == clamp_tileproc) {
            shadeProc = shadeSpan16_linear_clamp;
        } else if (proc == mirror_tileproc) {
            shadeProc = shadeSpan16_linear_mirror;
        } else {
            SkASSERT(proc == repeat_tileproc);
        }
        (*shadeProc)(proc, dx, fx, dstC, cache, toggle, count);
    } else {
        SkScalar    dstX = SkIntToScalar(x);
        SkScalar    dstY = SkIntToScalar(y);
        do {
            dstProc(fDstToIndex, dstX, dstY, &srcPt);
            unsigned fi = proc(SkScalarToFixed(srcPt.fX));
            SkASSERT(fi <= 0xFFFF);

            int index = fi >> kCache16Shift;
            *dstC++ = cache[toggle + index];
            toggle ^= Gradient_Shader::kDitherStride16;

            dstX += SK_Scalar1;
        } while (--count != 0);
    }
}



#define kSQRT_TABLE_BITS    11
#define kSQRT_TABLE_SIZE    (1 << kSQRT_TABLE_BITS)

#include "SkRadialGradient_Table.h"

#if defined(SK_BUILD_FOR_WIN32) && defined(SK_DEBUG)

#include <stdio.h>

void SkRadialGradient_BuildTable() {
    

    FILE* file = ::fopen("SkRadialGradient_Table.h", "w");
    SkASSERT(file);
    ::fprintf(file, "static const uint8_t gSqrt8Table[] = {\n");

    for (int i = 0; i < kSQRT_TABLE_SIZE; i++) {
        if ((i & 15) == 0) {
            ::fprintf(file, "\t");
        }

        uint8_t value = SkToU8(SkFixedSqrt(i * SK_Fixed1 / kSQRT_TABLE_SIZE) >> 8);

        ::fprintf(file, "0x%02X", value);
        if (i < kSQRT_TABLE_SIZE-1) {
            ::fprintf(file, ", ");
        }
        if ((i & 15) == 15) {
            ::fprintf(file, "\n");
        }
    }
    ::fprintf(file, "};\n");
    ::fclose(file);
}

#endif


static void rad_to_unit_matrix(const SkPoint& center, SkScalar radius,
                               SkMatrix* matrix) {
    SkScalar    inv = SkScalarInvert(radius);

    matrix->setTranslate(-center.fX, -center.fY);
    matrix->postScale(inv, inv);
}


namespace {

typedef void (* RadialShade16Proc)(SkScalar sfx, SkScalar sdx,
        SkScalar sfy, SkScalar sdy,
        uint16_t* dstC, const uint16_t* cache,
        int toggle, int count);

void shadeSpan16_radial_clamp(SkScalar sfx, SkScalar sdx,
        SkScalar sfy, SkScalar sdy,
        uint16_t* SK_RESTRICT dstC, const uint16_t* SK_RESTRICT cache,
        int toggle, int count) {
    const uint8_t* SK_RESTRICT sqrt_table = gSqrt8Table;

    






    SkFixed fx = SkScalarToFixed(sfx) >> 1;
    SkFixed dx = SkScalarToFixed(sdx) >> 1;
    SkFixed fy = SkScalarToFixed(sfy) >> 1;
    SkFixed dy = SkScalarToFixed(sdy) >> 1;
    
    
    if (dy == 0) {
        fy = SkPin32(fy, -0xFFFF >> 1, 0xFFFF >> 1);
        fy *= fy;
        do {
            unsigned xx = SkPin32(fx, -0xFFFF >> 1, 0xFFFF >> 1);
            unsigned fi = (xx * xx + fy) >> (14 + 16 - kSQRT_TABLE_BITS);
            fi = SkFastMin32(fi, 0xFFFF >> (16 - kSQRT_TABLE_BITS));
            fx += dx;
            *dstC++ = cache[toggle +
                            (sqrt_table[fi] >> Gradient_Shader::kSqrt16Shift)];
            toggle ^= Gradient_Shader::kDitherStride16;
        } while (--count != 0);
    } else {
        do {
            unsigned xx = SkPin32(fx, -0xFFFF >> 1, 0xFFFF >> 1);
            unsigned fi = SkPin32(fy, -0xFFFF >> 1, 0xFFFF >> 1);
            fi = (xx * xx + fi * fi) >> (14 + 16 - kSQRT_TABLE_BITS);
            fi = SkFastMin32(fi, 0xFFFF >> (16 - kSQRT_TABLE_BITS));
            fx += dx;
            fy += dy;
            *dstC++ = cache[toggle +
                            (sqrt_table[fi] >> Gradient_Shader::kSqrt16Shift)];
            toggle ^= Gradient_Shader::kDitherStride16;
        } while (--count != 0);
    }
}

void shadeSpan16_radial_mirror(SkScalar sfx, SkScalar sdx,
        SkScalar sfy, SkScalar sdy,
        uint16_t* SK_RESTRICT dstC, const uint16_t* SK_RESTRICT cache,
        int toggle, int count) {
    do {
#ifdef SK_SCALAR_IS_FLOAT
        float fdist = sk_float_sqrt(sfx*sfx + sfy*sfy);
        SkFixed dist = SkFloatToFixed(fdist);
#else
        SkFixed magnitudeSquared = SkFixedSquare(sfx) +
            SkFixedSquare(sfy);
        if (magnitudeSquared < 0) 
            magnitudeSquared = SK_FixedMax;
        SkFixed dist = SkFixedSqrt(magnitudeSquared);
#endif
        unsigned fi = mirror_tileproc(dist);
        SkASSERT(fi <= 0xFFFF);
        *dstC++ = cache[toggle + (fi >> Gradient_Shader::kCache16Shift)];
        toggle ^= Gradient_Shader::kDitherStride16;
        sfx += sdx;
        sfy += sdy;
    } while (--count != 0);
}

void shadeSpan16_radial_repeat(SkScalar sfx, SkScalar sdx,
        SkScalar sfy, SkScalar sdy,
        uint16_t* SK_RESTRICT dstC, const uint16_t* SK_RESTRICT cache,
        int toggle, int count) {
    SkFixed fx = SkScalarToFixed(sfx);
    SkFixed dx = SkScalarToFixed(sdx);
    SkFixed fy = SkScalarToFixed(sfy);
    SkFixed dy = SkScalarToFixed(sdy);
    do {
        SkFixed dist = SkFixedSqrt(SkFixedSquare(fx) + SkFixedSquare(fy));
        unsigned fi = repeat_tileproc(dist);
        SkASSERT(fi <= 0xFFFF);
        fx += dx;
        fy += dy;
        *dstC++ = cache[toggle + (fi >> Gradient_Shader::kCache16Shift)];
        toggle ^= Gradient_Shader::kDitherStride16;
    } while (--count != 0);
}

}

class Radial_Gradient : public Gradient_Shader {
public:
    Radial_Gradient(const SkPoint& center, SkScalar radius,
                    const SkColor colors[], const SkScalar pos[], int colorCount,
                    SkShader::TileMode mode, SkUnitMapper* mapper)
        : Gradient_Shader(colors, pos, colorCount, mode, mapper),
          fCenter(center),
          fRadius(radius)
    {
        
        SkASSERT(sizeof(gSqrt8Table) == kSQRT_TABLE_SIZE);

        rad_to_unit_matrix(center, radius, &fPtsToUnit);
    }

    virtual void shadeSpan(int x, int y, SkPMColor* dstC, int count)
        SK_OVERRIDE;
    virtual void shadeSpan16(int x, int y, uint16_t* dstCParam,
                             int count) SK_OVERRIDE {
        SkASSERT(count > 0);

        uint16_t* SK_RESTRICT dstC = dstCParam;

        SkPoint             srcPt;
        SkMatrix::MapXYProc dstProc = fDstToIndexProc;
        TileProc            proc = fTileProc;
        const uint16_t* SK_RESTRICT cache = this->getCache16();
        int                 toggle = ((x ^ y) & 1) * kDitherStride16;

        if (fDstToIndexClass != kPerspective_MatrixClass) {
            dstProc(fDstToIndex, SkIntToScalar(x) + SK_ScalarHalf,
                                 SkIntToScalar(y) + SK_ScalarHalf, &srcPt);

            SkScalar sdx = fDstToIndex.getScaleX();
            SkScalar sdy = fDstToIndex.getSkewY();

            if (fDstToIndexClass == kFixedStepInX_MatrixClass) {
                SkFixed storage[2];
                (void)fDstToIndex.fixedStepInX(SkIntToScalar(y),
                                               &storage[0], &storage[1]);
                sdx = SkFixedToScalar(storage[0]);
                sdy = SkFixedToScalar(storage[1]);
            } else {
                SkASSERT(fDstToIndexClass == kLinear_MatrixClass);
            }

            RadialShade16Proc shadeProc = shadeSpan16_radial_repeat;
            if (proc == clamp_tileproc) {
                shadeProc = shadeSpan16_radial_clamp;
            } else if (proc == mirror_tileproc) {
                shadeProc = shadeSpan16_radial_mirror;
            } else {
                SkASSERT(proc == repeat_tileproc);
            }
            (*shadeProc)(srcPt.fX, sdx, srcPt.fY, sdy, dstC,
                         cache, toggle, count);
        } else {    
            SkScalar dstX = SkIntToScalar(x);
            SkScalar dstY = SkIntToScalar(y);
            do {
                dstProc(fDstToIndex, dstX, dstY, &srcPt);
                unsigned fi = proc(SkScalarToFixed(srcPt.length()));
                SkASSERT(fi <= 0xFFFF);

                int index = fi >> (16 - kCache16Bits);
                *dstC++ = cache[toggle + index];
                toggle ^= kDitherStride16;

                dstX += SK_Scalar1;
            } while (--count != 0);
        }
    }

    virtual BitmapType asABitmap(SkBitmap* bitmap,
                                 SkMatrix* matrix,
                                 TileMode* xy,
                                 SkScalar* twoPointRadialParams)
            const SK_OVERRIDE {
        if (bitmap) {
            this->commonAsABitmap(bitmap);
        }
        if (matrix) {
            matrix->setScale(SkIntToScalar(kGradient32Length),
                             SkIntToScalar(kGradient32Length));
            matrix->preConcat(fPtsToUnit);
        }
        if (xy) {
            xy[0] = fTileMode;
            xy[1] = kClamp_TileMode;
        }
        return kRadial_BitmapType;
    }
    virtual GradientType asAGradient(GradientInfo* info) const SK_OVERRIDE {
        if (info) {
            commonAsAGradient(info);
            info->fPoint[0] = fCenter;
            info->fRadius[0] = fRadius;
        }
        return kRadial_GradientType;
    }

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(Radial_Gradient)

protected:
    Radial_Gradient(SkFlattenableReadBuffer& buffer)
        : INHERITED(buffer),
          fCenter(buffer.readPoint()),
          fRadius(buffer.readScalar()) {
    }
    virtual void flatten(SkFlattenableWriteBuffer& buffer) const SK_OVERRIDE {
        this->INHERITED::flatten(buffer);
        buffer.writePoint(fCenter);
        buffer.writeScalar(fRadius);
    }

private:
    typedef Gradient_Shader INHERITED;
    const SkPoint fCenter;
    const SkScalar fRadius;
};

namespace {

inline bool radial_completely_pinned(int fx, int dx, int fy, int dy) {
    
    
    bool xClamped = (fx >= SK_FixedHalf && dx >= 0) ||
                    (fx <= -SK_FixedHalf && dx <= 0);
    bool yClamped = (fy >= SK_FixedHalf && dy >= 0) ||
                    (fy <= -SK_FixedHalf && dy <= 0);

    return xClamped || yClamped;
}




inline bool no_need_for_radial_pin(int fx, int dx,
                                          int fy, int dy, int count) {
    SkASSERT(count > 0);
    if (SkAbs32(fx) > 0x7FFF || SkAbs32(fy) > 0x7FFF) {
        return false;
    }
    if (fx*fx + fy*fy > 0x7FFF*0x7FFF) {
        return false;
    }
    fx += (count - 1) * dx;
    fy += (count - 1) * dy;
    if (SkAbs32(fx) > 0x7FFF || SkAbs32(fy) > 0x7FFF) {
        return false;
    }
    return fx*fx + fy*fy <= 0x7FFF*0x7FFF;
}

#define UNPINNED_RADIAL_STEP \
    fi = (fx * fx + fy * fy) >> (14 + 16 - kSQRT_TABLE_BITS); \
    *dstC++ = cache[toggle + \
                    (sqrt_table[fi] >> Gradient_Shader::kSqrt32Shift)]; \
    toggle ^= Gradient_Shader::kDitherStride32; \
    fx += dx; \
    fy += dy;

typedef void (* RadialShadeProc)(SkScalar sfx, SkScalar sdx,
        SkScalar sfy, SkScalar sdy,
        SkPMColor* dstC, const SkPMColor* cache,
        int count, int toggle);


void shadeSpan_radial_clamp(SkScalar sfx, SkScalar sdx,
        SkScalar sfy, SkScalar sdy,
        SkPMColor* SK_RESTRICT dstC, const SkPMColor* SK_RESTRICT cache,
        int count, int toggle) {
    
    
    const uint8_t* SK_RESTRICT sqrt_table = gSqrt8Table;
    SkFixed fx = SkScalarToFixed(sfx) >> 1;
    SkFixed dx = SkScalarToFixed(sdx) >> 1;
    SkFixed fy = SkScalarToFixed(sfy) >> 1;
    SkFixed dy = SkScalarToFixed(sdy) >> 1;
    if ((count > 4) && radial_completely_pinned(fx, dx, fy, dy)) {
        unsigned fi = Gradient_Shader::kGradient32Length;
        sk_memset32_dither(dstC,
            cache[toggle + fi],
            cache[(toggle ^ Gradient_Shader::kDitherStride32) + fi],
            count);
    } else if ((count > 4) &&
               no_need_for_radial_pin(fx, dx, fy, dy, count)) {
        unsigned fi;
        
        while (count > 1) {
            UNPINNED_RADIAL_STEP;
            UNPINNED_RADIAL_STEP;
            count -= 2;
        }
        if (count) {
            UNPINNED_RADIAL_STEP;
        }
    }
    else  {
        
        if (dy == 0) {
            unsigned yy = SkPin32(fy, -0xFFFF >> 1, 0xFFFF >> 1);
            yy *= yy;
            do {
                unsigned xx = SkPin32(fx, -0xFFFF >> 1, 0xFFFF >> 1);
                unsigned fi = (xx * xx + yy) >> (14 + 16 - kSQRT_TABLE_BITS);
                fi = SkFastMin32(fi, 0xFFFF >> (16 - kSQRT_TABLE_BITS));
                *dstC++ = cache[toggle + (sqrt_table[fi] >>
                    Gradient_Shader::kSqrt32Shift)];
                toggle ^= Gradient_Shader::kDitherStride32;
                fx += dx;
            } while (--count != 0);
        } else {
            do {
                unsigned xx = SkPin32(fx, -0xFFFF >> 1, 0xFFFF >> 1);
                unsigned fi = SkPin32(fy, -0xFFFF >> 1, 0xFFFF >> 1);
                fi = (xx * xx + fi * fi) >> (14 + 16 - kSQRT_TABLE_BITS);
                fi = SkFastMin32(fi, 0xFFFF >> (16 - kSQRT_TABLE_BITS));
                *dstC++ = cache[toggle + (sqrt_table[fi] >>
                    Gradient_Shader::kSqrt32Shift)];
                toggle ^= Gradient_Shader::kDitherStride32;
                fx += dx;
                fy += dy;
            } while (--count != 0);
        }
    }
}




void shadeSpan_radial_mirror(SkScalar sfx, SkScalar sdx,
        SkScalar sfy, SkScalar sdy,
        SkPMColor* SK_RESTRICT dstC, const SkPMColor* SK_RESTRICT cache,
        int count, int toggle) {
    do {
#ifdef SK_SCALAR_IS_FLOAT
        float fdist = sk_float_sqrt(sfx*sfx + sfy*sfy);
        SkFixed dist = SkFloatToFixed(fdist);
#else
        SkFixed magnitudeSquared = SkFixedSquare(sfx) +
            SkFixedSquare(sfy);
        if (magnitudeSquared < 0) 
            magnitudeSquared = SK_FixedMax;
        SkFixed dist = SkFixedSqrt(magnitudeSquared);
#endif
        unsigned fi = mirror_tileproc(dist);
        SkASSERT(fi <= 0xFFFF);
        *dstC++ = cache[toggle + (fi >> Gradient_Shader::kCache32Shift)];
        toggle ^= Gradient_Shader::kDitherStride32;
        sfx += sdx;
        sfy += sdy;
    } while (--count != 0);
}

void shadeSpan_radial_repeat(SkScalar sfx, SkScalar sdx,
        SkScalar sfy, SkScalar sdy,
        SkPMColor* SK_RESTRICT dstC, const SkPMColor* SK_RESTRICT cache,
        int count, int toggle) {
    SkFixed fx = SkScalarToFixed(sfx);
    SkFixed dx = SkScalarToFixed(sdx);
    SkFixed fy = SkScalarToFixed(sfy);
    SkFixed dy = SkScalarToFixed(sdy);
    do {
        SkFixed magnitudeSquared = SkFixedSquare(fx) +
            SkFixedSquare(fy);
        if (magnitudeSquared < 0) 
            magnitudeSquared = SK_FixedMax;
        SkFixed dist = SkFixedSqrt(magnitudeSquared);
        unsigned fi = repeat_tileproc(dist);
        SkASSERT(fi <= 0xFFFF);
        *dstC++ = cache[toggle + (fi >> Gradient_Shader::kCache32Shift)];
        toggle ^= Gradient_Shader::kDitherStride32;
        fx += dx;
        fy += dy;
    } while (--count != 0);
}
}

void Radial_Gradient::shadeSpan(int x, int y,
                                SkPMColor* SK_RESTRICT dstC, int count) {
    SkASSERT(count > 0);

    SkPoint             srcPt;
    SkMatrix::MapXYProc dstProc = fDstToIndexProc;
    TileProc            proc = fTileProc;
    const SkPMColor* SK_RESTRICT cache = this->getCache32();
#ifdef USE_DITHER_32BIT_GRADIENT
    int toggle = ((x ^ y) & 1) * Gradient_Shader::kDitherStride32;
#else
    int toggle = 0;
#endif

    if (fDstToIndexClass != kPerspective_MatrixClass) {
        dstProc(fDstToIndex, SkIntToScalar(x) + SK_ScalarHalf,
                             SkIntToScalar(y) + SK_ScalarHalf, &srcPt);
        SkScalar sdx = fDstToIndex.getScaleX();
        SkScalar sdy = fDstToIndex.getSkewY();

        if (fDstToIndexClass == kFixedStepInX_MatrixClass) {
            SkFixed storage[2];
            (void)fDstToIndex.fixedStepInX(SkIntToScalar(y),
                                           &storage[0], &storage[1]);
            sdx = SkFixedToScalar(storage[0]);
            sdy = SkFixedToScalar(storage[1]);
        } else {
            SkASSERT(fDstToIndexClass == kLinear_MatrixClass);
        }

        RadialShadeProc shadeProc = shadeSpan_radial_repeat;
        if (proc == clamp_tileproc) {
            shadeProc = shadeSpan_radial_clamp;
        } else if (proc == mirror_tileproc) {
            shadeProc = shadeSpan_radial_mirror;
        } else {
            SkASSERT(proc == repeat_tileproc);
        }
        (*shadeProc)(srcPt.fX, sdx, srcPt.fY, sdy, dstC, cache, count, toggle);
    } else {    
        SkScalar dstX = SkIntToScalar(x);
        SkScalar dstY = SkIntToScalar(y);
        do {
            dstProc(fDstToIndex, dstX, dstY, &srcPt);
            unsigned fi = proc(SkScalarToFixed(srcPt.length()));
            SkASSERT(fi <= 0xFFFF);
            *dstC++ = cache[fi >> Gradient_Shader::kCache32Shift];
            dstX += SK_Scalar1;
        } while (--count != 0);
    }
}









































































namespace {

inline SkFixed two_point_radial(SkScalar b, SkScalar fx, SkScalar fy,
                                SkScalar sr2d2, SkScalar foura,
                                SkScalar oneOverTwoA, bool posRoot) {
    SkScalar c = SkScalarSquare(fx) + SkScalarSquare(fy) - sr2d2;
    if (0 == foura) {
        return SkScalarToFixed(SkScalarDiv(-c, b));
    }

    SkScalar discrim = SkScalarSquare(b) - SkScalarMul(foura, c);
    if (discrim < 0) {
        discrim = -discrim;
    }
    SkScalar rootDiscrim = SkScalarSqrt(discrim);
    SkScalar result;
    if (posRoot) {
        result = SkScalarMul(-b + rootDiscrim, oneOverTwoA);
    } else {
        result = SkScalarMul(-b - rootDiscrim, oneOverTwoA);
    }
    return SkScalarToFixed(result);
}

typedef void (* TwoPointRadialShadeProc)(SkScalar fx, SkScalar dx,
        SkScalar fy, SkScalar dy,
        SkScalar b, SkScalar db,
        SkScalar fSr2D2, SkScalar foura, SkScalar fOneOverTwoA, bool posRoot,
        SkPMColor* SK_RESTRICT dstC, const SkPMColor* SK_RESTRICT cache,
        int count);

void shadeSpan_twopoint_clamp(SkScalar fx, SkScalar dx,
        SkScalar fy, SkScalar dy,
        SkScalar b, SkScalar db,
        SkScalar fSr2D2, SkScalar foura, SkScalar fOneOverTwoA, bool posRoot,
        SkPMColor* SK_RESTRICT dstC, const SkPMColor* SK_RESTRICT cache,
        int count) {
    for (; count > 0; --count) {
        SkFixed t = two_point_radial(b, fx, fy, fSr2D2, foura,
                                     fOneOverTwoA, posRoot);

        if (t < 0) {
            *dstC++ = cache[-1];
        } else if (t > 0xFFFF) {
            *dstC++ = cache[Gradient_Shader::kCache32Count * 2];
        } else {
            SkASSERT(t <= 0xFFFF);
            *dstC++ = cache[t >> Gradient_Shader::kCache32Shift];
        }

        fx += dx;
        fy += dy;
        b += db;
    }
}
void shadeSpan_twopoint_mirror(SkScalar fx, SkScalar dx,
        SkScalar fy, SkScalar dy,
        SkScalar b, SkScalar db,
        SkScalar fSr2D2, SkScalar foura, SkScalar fOneOverTwoA, bool posRoot,
        SkPMColor* SK_RESTRICT dstC, const SkPMColor* SK_RESTRICT cache,
        int count) {
    for (; count > 0; --count) {
        SkFixed t = two_point_radial(b, fx, fy, fSr2D2, foura,
                                     fOneOverTwoA, posRoot);
        SkFixed index = mirror_tileproc(t);
        SkASSERT(index <= 0xFFFF);
        *dstC++ = cache[index >> Gradient_Shader::kCache32Shift];
        fx += dx;
        fy += dy;
        b += db;
    }
}

void shadeSpan_twopoint_repeat(SkScalar fx, SkScalar dx,
        SkScalar fy, SkScalar dy,
        SkScalar b, SkScalar db,
        SkScalar fSr2D2, SkScalar foura, SkScalar fOneOverTwoA, bool posRoot,
        SkPMColor* SK_RESTRICT dstC, const SkPMColor* SK_RESTRICT cache,
        int count) {
    for (; count > 0; --count) {
        SkFixed t = two_point_radial(b, fx, fy, fSr2D2, foura,
                                     fOneOverTwoA, posRoot);
        SkFixed index = repeat_tileproc(t);
        SkASSERT(index <= 0xFFFF);
        *dstC++ = cache[index >> Gradient_Shader::kCache32Shift];
        fx += dx;
        fy += dy;
        b += db;
    }
}



}

class Two_Point_Radial_Gradient : public Gradient_Shader {
public:
    Two_Point_Radial_Gradient(const SkPoint& start, SkScalar startRadius,
                              const SkPoint& end, SkScalar endRadius,
                              const SkColor colors[], const SkScalar pos[],
                              int colorCount, SkShader::TileMode mode,
                              SkUnitMapper* mapper)
            : Gradient_Shader(colors, pos, colorCount, mode, mapper),
              fCenter1(start),
              fCenter2(end),
              fRadius1(startRadius),
              fRadius2(endRadius) {
        init();
    }

    virtual BitmapType asABitmap(SkBitmap* bitmap,
                                 SkMatrix* matrix,
                                 TileMode* xy,
                                 SkScalar* twoPointRadialParams) const {
        if (bitmap) {
            this->commonAsABitmap(bitmap);
        }
        SkScalar diffL = 0; 
        if (matrix || twoPointRadialParams) {
            diffL = SkScalarSqrt(SkScalarSquare(fDiff.fX) +
                                 SkScalarSquare(fDiff.fY));
        }
        if (matrix) {
            if (diffL) {
                SkScalar invDiffL = SkScalarInvert(diffL);
                matrix->setSinCos(-SkScalarMul(invDiffL, fDiff.fY),
                                  SkScalarMul(invDiffL, fDiff.fX));
            } else {
                matrix->reset();
            }
            matrix->preConcat(fPtsToUnit);
        }
        if (xy) {
            xy[0] = fTileMode;
            xy[1] = kClamp_TileMode;
        }
        if (NULL != twoPointRadialParams) {
            twoPointRadialParams[0] = diffL;
            twoPointRadialParams[1] = fStartRadius;
            twoPointRadialParams[2] = fDiffRadius;
        }
        return kTwoPointRadial_BitmapType;
    }

    virtual GradientType asAGradient(GradientInfo* info) const SK_OVERRIDE {
        if (info) {
            commonAsAGradient(info);
            info->fPoint[0] = fCenter1;
            info->fPoint[1] = fCenter2;
            info->fRadius[0] = fRadius1;
            info->fRadius[1] = fRadius2;
        }
        return kRadial2_GradientType;
    }

    virtual void shadeSpan(int x, int y, SkPMColor* dstCParam,
                           int count) SK_OVERRIDE {
        SkASSERT(count > 0);

        SkPMColor* SK_RESTRICT dstC = dstCParam;

        
        
        
        
        if (fDiffRadius == 0 && fCenter1 == fCenter2) {
          sk_bzero(dstC, count * sizeof(*dstC));
          return;
        }
        SkMatrix::MapXYProc dstProc = fDstToIndexProc;
        TileProc            proc = fTileProc;
        const SkPMColor* SK_RESTRICT cache = this->getCache32();

        SkScalar foura = fA * 4;
        bool posRoot = fDiffRadius < 0;
        if (fDstToIndexClass != kPerspective_MatrixClass) {
            SkPoint srcPt;
            dstProc(fDstToIndex, SkIntToScalar(x) + SK_ScalarHalf,
                                 SkIntToScalar(y) + SK_ScalarHalf, &srcPt);
            SkScalar dx, fx = srcPt.fX;
            SkScalar dy, fy = srcPt.fY;

            if (fDstToIndexClass == kFixedStepInX_MatrixClass) {
                SkFixed fixedX, fixedY;
                (void)fDstToIndex.fixedStepInX(SkIntToScalar(y), &fixedX, &fixedY);
                dx = SkFixedToScalar(fixedX);
                dy = SkFixedToScalar(fixedY);
            } else {
                SkASSERT(fDstToIndexClass == kLinear_MatrixClass);
                dx = fDstToIndex.getScaleX();
                dy = fDstToIndex.getSkewY();
            }
            SkScalar b = (SkScalarMul(fDiff.fX, fx) +
                         SkScalarMul(fDiff.fY, fy) - fStartRadius) * 2;
            SkScalar db = (SkScalarMul(fDiff.fX, dx) +
                          SkScalarMul(fDiff.fY, dy)) * 2;

            TwoPointRadialShadeProc shadeProc = shadeSpan_twopoint_repeat;
            if (proc == clamp_tileproc) {
                shadeProc = shadeSpan_twopoint_clamp;
            } else if (proc == mirror_tileproc) {
                shadeProc = shadeSpan_twopoint_mirror;
            } else {
                SkASSERT(proc == repeat_tileproc);
            }
            (*shadeProc)(fx, dx, fy, dy, b, db,
                         fSr2D2, foura, fOneOverTwoA, posRoot,
                         dstC, cache, count);
        } else {    
            SkScalar dstX = SkIntToScalar(x);
            SkScalar dstY = SkIntToScalar(y);
            for (; count > 0; --count) {
                SkPoint             srcPt;
                dstProc(fDstToIndex, dstX, dstY, &srcPt);
                SkScalar fx = srcPt.fX;
                SkScalar fy = srcPt.fY;
                SkScalar b = (SkScalarMul(fDiff.fX, fx) +
                             SkScalarMul(fDiff.fY, fy) - fStartRadius) * 2;
                SkFixed t = two_point_radial(b, fx, fy, fSr2D2, foura,
                                             fOneOverTwoA, posRoot);
                SkFixed index = proc(t);
                SkASSERT(index <= 0xFFFF);
                *dstC++ = cache[index >> Gradient_Shader::kCache32Shift];
                dstX += SK_Scalar1;
            }
        }
    }

    virtual bool setContext(const SkBitmap& device,
                            const SkPaint& paint,
                            const SkMatrix& matrix) SK_OVERRIDE {
        if (!this->INHERITED::setContext(device, paint, matrix)) {
            return false;
        }

        
        fFlags &= ~kHasSpan16_Flag;
        return true;
    }

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(Two_Point_Radial_Gradient)

protected:
    Two_Point_Radial_Gradient(SkFlattenableReadBuffer& buffer)
            : INHERITED(buffer),
              fCenter1(buffer.readPoint()),
              fCenter2(buffer.readPoint()),
              fRadius1(buffer.readScalar()),
              fRadius2(buffer.readScalar()) {
        init();
    };

    virtual void flatten(SkFlattenableWriteBuffer& buffer) const SK_OVERRIDE {
        this->INHERITED::flatten(buffer);
        buffer.writePoint(fCenter1);
        buffer.writePoint(fCenter2);
        buffer.writeScalar(fRadius1);
        buffer.writeScalar(fRadius2);
    }

private:
    typedef Gradient_Shader INHERITED;
    const SkPoint fCenter1;
    const SkPoint fCenter2;
    const SkScalar fRadius1;
    const SkScalar fRadius2;
    SkPoint fDiff;
    SkScalar fStartRadius, fDiffRadius, fSr2D2, fA, fOneOverTwoA;

    void init() {
        fDiff = fCenter1 - fCenter2;
        fDiffRadius = fRadius2 - fRadius1;
        SkScalar inv = SkScalarInvert(fDiffRadius);
        fDiff.fX = SkScalarMul(fDiff.fX, inv);
        fDiff.fY = SkScalarMul(fDiff.fY, inv);
        fStartRadius = SkScalarMul(fRadius1, inv);
        fSr2D2 = SkScalarSquare(fStartRadius);
        fA = SkScalarSquare(fDiff.fX) + SkScalarSquare(fDiff.fY) - SK_Scalar1;
        fOneOverTwoA = fA ? SkScalarInvert(fA * 2) : 0;

        fPtsToUnit.setTranslate(-fCenter1.fX, -fCenter1.fY);
        fPtsToUnit.postScale(inv, inv);
    }
};



class Sweep_Gradient : public Gradient_Shader {
public:
    Sweep_Gradient(SkScalar cx, SkScalar cy, const SkColor colors[],
                   const SkScalar pos[], int count, SkUnitMapper* mapper)
    : Gradient_Shader(colors, pos, count, SkShader::kClamp_TileMode, mapper),
      fCenter(SkPoint::Make(cx, cy))
    {
        fPtsToUnit.setTranslate(-cx, -cy);
    }
    virtual void shadeSpan(int x, int y, SkPMColor dstC[], int count) SK_OVERRIDE;
    virtual void shadeSpan16(int x, int y, uint16_t dstC[], int count) SK_OVERRIDE;

    virtual BitmapType asABitmap(SkBitmap* bitmap,
                                 SkMatrix* matrix,
                                 TileMode* xy,
                                 SkScalar* twoPointRadialParams) const SK_OVERRIDE {
        if (bitmap) {
            this->commonAsABitmap(bitmap);
        }
        if (matrix) {
            *matrix = fPtsToUnit;
        }
        if (xy) {
            xy[0] = fTileMode;
            xy[1] = kClamp_TileMode;
        }
        return kSweep_BitmapType;
    }

    virtual GradientType asAGradient(GradientInfo* info) const SK_OVERRIDE {
        if (info) {
            commonAsAGradient(info);
            info->fPoint[0] = fCenter;
        }
        return kSweep_GradientType;
    }

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(Sweep_Gradient)

protected:
    Sweep_Gradient(SkFlattenableReadBuffer& buffer)
        : INHERITED(buffer),
          fCenter(buffer.readPoint()) {
    }
    virtual void flatten(SkFlattenableWriteBuffer& buffer) const SK_OVERRIDE {
        this->INHERITED::flatten(buffer);
        buffer.writePoint(fCenter);
    }

private:
    typedef Gradient_Shader INHERITED;
    const SkPoint fCenter;
};

#ifndef SK_SCALAR_IS_FLOAT 

#ifdef COMPUTE_SWEEP_TABLE
#define PI  3.14159265
static bool gSweepTableReady;
static uint8_t gSweepTable[65];




static const uint8_t* build_sweep_table() {
    if (!gSweepTableReady) {
        const int N = 65;
        const double DENOM = N - 1;

        for (int i = 0; i < N; i++)
        {
            double arg = i / DENOM;
            double v = atan(arg);
            int iv = (int)round(v * DENOM * 2 / PI);

            printf("%d, ", iv);
            gSweepTable[i] = iv;
        }
        gSweepTableReady = true;
    }
    return gSweepTable;
}
#else
static const uint8_t gSweepTable[] = {
    0, 1, 1, 2, 3, 3, 4, 4, 5, 6, 6, 7, 8, 8, 9, 9,
    10, 11, 11, 12, 12, 13, 13, 14, 15, 15, 16, 16, 17, 17, 18, 18,
    19, 19, 20, 20, 21, 21, 22, 22, 23, 23, 24, 24, 25, 25, 25, 26,
    26, 27, 27, 27, 28, 28, 29, 29, 29, 30, 30, 30, 31, 31, 31, 32,
    32
};
static const uint8_t* build_sweep_table() { return gSweepTable; }
#endif

#endif





#ifndef SK_SCALAR_IS_FLOAT

static unsigned div_64(int numer, int denom) {
    SkASSERT(numer <= denom);
    SkASSERT(numer > 0);
    SkASSERT(denom > 0);

    int nbits = SkCLZ(numer);
    int dbits = SkCLZ(denom);
    int bits = 6 - nbits + dbits;
    SkASSERT(bits <= 6);

    if (bits < 0) {  
        return 0;
    }

    denom <<= dbits - 1;
    numer <<= nbits - 1;

    unsigned result = 0;

    
    if ((numer -= denom) >= 0) {
        result = 1;
    } else {
        numer += denom;
    }

    
    if (bits > 0) {
        
        result <<= bits;
        switch (bits) {
        case 6:
            if ((numer = (numer << 1) - denom) >= 0)
                result |= 32;
            else
                numer += denom;
        case 5:
            if ((numer = (numer << 1) - denom) >= 0)
                result |= 16;
            else
                numer += denom;
        case 4:
            if ((numer = (numer << 1) - denom) >= 0)
                result |= 8;
            else
                numer += denom;
        case 3:
            if ((numer = (numer << 1) - denom) >= 0)
                result |= 4;
            else
                numer += denom;
        case 2:
            if ((numer = (numer << 1) - denom) >= 0)
                result |= 2;
            else
                numer += denom;
        case 1:
        default:    
            if ((numer = (numer << 1) - denom) >= 0)
                result |= 1;
            else
                numer += denom;
        }
    }
    return result;
}


static unsigned atan_0_90(SkFixed y, SkFixed x) {
#ifdef SK_DEBUG
    {
        static bool gOnce;
        if (!gOnce) {
            gOnce = true;
            SkASSERT(div_64(55, 55) == 64);
            SkASSERT(div_64(128, 256) == 32);
            SkASSERT(div_64(2326528, 4685824) == 31);
            SkASSERT(div_64(753664, 5210112) == 9);
            SkASSERT(div_64(229376, 4882432) == 3);
            SkASSERT(div_64(2, 64) == 2);
            SkASSERT(div_64(1, 64) == 1);
            
            SkASSERT(div_64(12345, 0x54321234) == 0);
        }
    }
#endif

    SkASSERT(y > 0 && x > 0);
    const uint8_t* table = build_sweep_table();

    unsigned result;
    bool swap = (x < y);
    if (swap) {
        
        
        SkTSwap<SkFixed>(x, y);
    }

    result = div_64(y, x);

#ifdef SK_DEBUG
    {
        unsigned result2 = SkDivBits(y, x, 6);
        SkASSERT(result2 == result ||
                 (result == 1 && result2 == 0));
    }
#endif

    SkASSERT(result < SK_ARRAY_COUNT(gSweepTable));
    result = table[result];

    if (swap) {
        
        result = 64 - result;
        
        result -= result >> 6;
    }

    SkASSERT(result <= 63);
    return result;
}
#endif


#ifdef SK_SCALAR_IS_FLOAT
static unsigned SkATan2_255(float y, float x) {
    
    static const float g255Over2PI = 40.584510488433314f;
    
    float result = sk_float_atan2(y, x);
    if (result < 0) {
        result += 2 * SK_ScalarPI;
    }
    SkASSERT(result >= 0);
    
    
    int ir = (int)(result * g255Over2PI);
    SkASSERT(ir >= 0 && ir <= 255);
    return ir;
}
#else
static unsigned SkATan2_255(SkFixed y, SkFixed x) {
    if (x == 0) {
        if (y == 0) {
            return 0;
        }
        return y < 0 ? 192 : 64;
    }
    if (y == 0) {
        return x < 0 ? 128 : 0;
    }

    












    int xsign = x >> 31;
    int ysign = y >> 31;
    int add = ((-xsign) ^ (ysign & 3)) << 6;

#ifdef SK_DEBUG
    if (0 == add)
        SkASSERT(x > 0 && y > 0);
    else if (64 == add)
        SkASSERT(x < 0 && y > 0);
    else if (128 == add)
        SkASSERT(x < 0 && y < 0);
    else if (192 == add)
        SkASSERT(x > 0 && y < 0);
    else
        SkDEBUGFAIL("bad value for add");
#endif

    


    x = (x ^ xsign) - xsign;
    y = (y ^ ysign) - ysign;
    if (add & 64) {             
        SkTSwap<SkFixed>(x, y);
    }

    unsigned result = add + atan_0_90(y, x);
    SkASSERT(result < 256);
    return result;
}
#endif

void Sweep_Gradient::shadeSpan(int x, int y, SkPMColor* SK_RESTRICT dstC,
                               int count) {
    SkMatrix::MapXYProc proc = fDstToIndexProc;
    const SkMatrix&     matrix = fDstToIndex;
    const SkPMColor* SK_RESTRICT cache = this->getCache32();
    SkPoint             srcPt;

    if (fDstToIndexClass != kPerspective_MatrixClass) {
        proc(matrix, SkIntToScalar(x) + SK_ScalarHalf,
                     SkIntToScalar(y) + SK_ScalarHalf, &srcPt);
        SkScalar dx, fx = srcPt.fX;
        SkScalar dy, fy = srcPt.fY;

        if (fDstToIndexClass == kFixedStepInX_MatrixClass) {
            SkFixed storage[2];
            (void)matrix.fixedStepInX(SkIntToScalar(y) + SK_ScalarHalf,
                                      &storage[0], &storage[1]);
            dx = SkFixedToScalar(storage[0]);
            dy = SkFixedToScalar(storage[1]);
        } else {
            SkASSERT(fDstToIndexClass == kLinear_MatrixClass);
            dx = matrix.getScaleX();
            dy = matrix.getSkewY();
        }

        for (; count > 0; --count) {
            *dstC++ = cache[SkATan2_255(fy, fx)];
            fx += dx;
            fy += dy;
        }
    } else {  
        for (int stop = x + count; x < stop; x++) {
            proc(matrix, SkIntToScalar(x) + SK_ScalarHalf,
                         SkIntToScalar(y) + SK_ScalarHalf, &srcPt);
            *dstC++ = cache[SkATan2_255(srcPt.fY, srcPt.fX)];
        }
    }
}

void Sweep_Gradient::shadeSpan16(int x, int y, uint16_t* SK_RESTRICT dstC,
                                 int count) {
    SkMatrix::MapXYProc proc = fDstToIndexProc;
    const SkMatrix&     matrix = fDstToIndex;
    const uint16_t* SK_RESTRICT cache = this->getCache16();
    int                 toggle = ((x ^ y) & 1) * kDitherStride16;
    SkPoint             srcPt;

    if (fDstToIndexClass != kPerspective_MatrixClass) {
        proc(matrix, SkIntToScalar(x) + SK_ScalarHalf,
                     SkIntToScalar(y) + SK_ScalarHalf, &srcPt);
        SkScalar dx, fx = srcPt.fX;
        SkScalar dy, fy = srcPt.fY;

        if (fDstToIndexClass == kFixedStepInX_MatrixClass) {
            SkFixed storage[2];
            (void)matrix.fixedStepInX(SkIntToScalar(y) + SK_ScalarHalf,
                                      &storage[0], &storage[1]);
            dx = SkFixedToScalar(storage[0]);
            dy = SkFixedToScalar(storage[1]);
        } else {
            SkASSERT(fDstToIndexClass == kLinear_MatrixClass);
            dx = matrix.getScaleX();
            dy = matrix.getSkewY();
        }

        for (; count > 0; --count) {
            int index = SkATan2_255(fy, fx) >> (8 - kCache16Bits);
            *dstC++ = cache[toggle + index];
            toggle ^= kDitherStride16;
            fx += dx;
            fy += dy;
        }
    } else {  
        for (int stop = x + count; x < stop; x++) {
            proc(matrix, SkIntToScalar(x) + SK_ScalarHalf,
                         SkIntToScalar(y) + SK_ScalarHalf, &srcPt);

            int index = SkATan2_255(srcPt.fY, srcPt.fX);
            index >>= (8 - kCache16Bits);
            *dstC++ = cache[toggle + index];
            toggle ^= kDitherStride16;
        }
    }
}





#define EXPAND_1_COLOR(count)               \
    SkColor tmp[2];                         \
    do {                                    \
        if (1 == count) {                   \
            tmp[0] = tmp[1] = colors[0];    \
            colors = tmp;                   \
            pos = NULL;                     \
            count = 2;                      \
        }                                   \
    } while (0)

SkShader* SkGradientShader::CreateLinear(const SkPoint pts[2],
                                         const SkColor colors[],
                                         const SkScalar pos[], int colorCount,
                                         SkShader::TileMode mode,
                                         SkUnitMapper* mapper) {
    if (NULL == pts || NULL == colors || colorCount < 1) {
        return NULL;
    }
    EXPAND_1_COLOR(colorCount);

    return SkNEW_ARGS(Linear_Gradient,
                      (pts, colors, pos, colorCount, mode, mapper));
}

SkShader* SkGradientShader::CreateRadial(const SkPoint& center, SkScalar radius,
                                         const SkColor colors[],
                                         const SkScalar pos[], int colorCount,
                                         SkShader::TileMode mode,
                                         SkUnitMapper* mapper) {
    if (radius <= 0 || NULL == colors || colorCount < 1) {
        return NULL;
    }
    EXPAND_1_COLOR(colorCount);

    return SkNEW_ARGS(Radial_Gradient,
                      (center, radius, colors, pos, colorCount, mode, mapper));
}

SkShader* SkGradientShader::CreateTwoPointRadial(const SkPoint& start,
                                                 SkScalar startRadius,
                                                 const SkPoint& end,
                                                 SkScalar endRadius,
                                                 const SkColor colors[],
                                                 const SkScalar pos[],
                                                 int colorCount,
                                                 SkShader::TileMode mode,
                                                 SkUnitMapper* mapper) {
    if (startRadius < 0 || endRadius < 0 || NULL == colors || colorCount < 1) {
        return NULL;
    }
    EXPAND_1_COLOR(colorCount);

    return SkNEW_ARGS(Two_Point_Radial_Gradient,
                      (start, startRadius, end, endRadius, colors, pos,
                       colorCount, mode, mapper));
}

SkShader* SkGradientShader::CreateSweep(SkScalar cx, SkScalar cy,
                                        const SkColor colors[],
                                        const SkScalar pos[],
                                        int count, SkUnitMapper* mapper) {
    if (NULL == colors || count < 1) {
        return NULL;
    }
    EXPAND_1_COLOR(count);

    return SkNEW_ARGS(Sweep_Gradient, (cx, cy, colors, pos, count, mapper));
}

SK_DEFINE_FLATTENABLE_REGISTRAR_GROUP_START(SkGradientShader)
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(Linear_Gradient)
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(Radial_Gradient)
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(Sweep_Gradient)
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(Two_Point_Radial_Gradient)
SK_DEFINE_FLATTENABLE_REGISTRAR_GROUP_END
