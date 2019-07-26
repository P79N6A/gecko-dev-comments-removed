






#include "SkGradientShaderPriv.h"
#include "SkLinearGradient.h"
#include "SkRadialGradient.h"
#include "SkTwoPointRadialGradient.h"
#include "SkTwoPointConicalGradient.h"
#include "SkSweepGradient.h"

SkGradientShaderBase::SkGradientShaderBase(const Descriptor& desc) {
    SkASSERT(desc.fCount > 1);

    fCacheAlpha = 256;  

    fMapper = desc.fMapper;
    SkSafeRef(fMapper);
    fGradFlags = SkToU8(desc.fFlags);

    SkASSERT((unsigned)desc.fTileMode < SkShader::kTileModeCount);
    SkASSERT(SkShader::kTileModeCount == SK_ARRAY_COUNT(gTileProcs));
    fTileMode = desc.fTileMode;
    fTileProc = gTileProcs[desc.fTileMode];

    fCache16 = fCache16Storage = NULL;
    fCache32 = NULL;
    fCache32PixelRef = NULL;

    










    fColorCount = desc.fCount;
    
    bool dummyFirst = false;
    bool dummyLast = false;
    if (desc.fPos) {
        dummyFirst = desc.fPos[0] != 0;
        dummyLast = desc.fPos[desc.fCount - 1] != SK_Scalar1;
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
            *origColors++ = desc.fColors[0];
        }
        memcpy(origColors, desc.fColors, desc.fCount * sizeof(SkColor));
        if (dummyLast) {
            origColors += desc.fCount;
            *origColors = desc.fColors[desc.fCount - 1];
        }
    }

    fRecs = (Rec*)(fOrigColors + fColorCount);
    if (fColorCount > 2) {
        Rec* recs = fRecs;
        recs->fPos = 0;
        
        recs += 1;
        if (desc.fPos) {
            






            SkFixed prev = 0;
            int startIndex = dummyFirst ? 0 : 1;
            int count = desc.fCount + dummyLast;
            for (int i = startIndex; i < count; i++) {
                
                SkFixed curr;
                if (i == desc.fCount) {  
                    curr = SK_Fixed1;
                } else {
                    curr = SkScalarToFixed(desc.fPos[i]);
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
            SkFixed dp = SK_Fixed1 / (desc.fCount - 1);
            SkFixed p = dp;
            SkFixed scale = (desc.fCount - 1) << 8;  
            for (int i = 1; i < desc.fCount; i++) {
                recs->fPos   = p;
                recs->fScale = scale;
                recs += 1;
                p += dp;
            }
        }
    }
    this->initCommon();
}

static uint32_t pack_mode_flags(SkShader::TileMode mode, uint32_t flags) {
    SkASSERT(0 == (flags >> 28));
    SkASSERT(0 == ((uint32_t)mode >> 4));
    return (flags << 4) | mode;
}

static SkShader::TileMode unpack_mode(uint32_t packed) {
    return (SkShader::TileMode)(packed & 0xF);
}

static uint32_t unpack_flags(uint32_t packed) {
    return packed >> 4;
}

SkGradientShaderBase::SkGradientShaderBase(SkReadBuffer& buffer) : INHERITED(buffer) {
    fCacheAlpha = 256;

    fMapper = buffer.readUnitMapper();

    fCache16 = fCache16Storage = NULL;
    fCache32 = NULL;
    fCache32PixelRef = NULL;

    int colorCount = fColorCount = buffer.getArrayCount();
    if (colorCount > kColorStorageCount) {
        size_t allocSize = (sizeof(SkColor) + sizeof(SkPMColor) + sizeof(Rec)) * colorCount;
        if (buffer.validateAvailable(allocSize)) {
            fOrigColors = reinterpret_cast<SkColor*>(sk_malloc_throw(allocSize));
        } else {
            fOrigColors =  NULL;
            colorCount = fColorCount = 0;
        }
    } else {
        fOrigColors = fStorage;
    }
    buffer.readColorArray(fOrigColors, colorCount);

    {
        uint32_t packed = buffer.readUInt();
        fGradFlags = SkToU8(unpack_flags(packed));
        fTileMode = unpack_mode(packed);
    }
    fTileProc = gTileProcs[fTileMode];
    fRecs = (Rec*)(fOrigColors + colorCount);
    if (colorCount > 2) {
        Rec* recs = fRecs;
        recs[0].fPos = 0;
        for (int i = 1; i < colorCount; i++) {
            recs[i].fPos = buffer.readInt();
            recs[i].fScale = buffer.readUInt();
        }
    }
    buffer.readMatrix(&fPtsToUnit);
    this->initCommon();
}

SkGradientShaderBase::~SkGradientShaderBase() {
    if (fCache16Storage) {
        sk_free(fCache16Storage);
    }
    SkSafeUnref(fCache32PixelRef);
    if (fOrigColors != fStorage) {
        sk_free(fOrigColors);
    }
    SkSafeUnref(fMapper);
}

void SkGradientShaderBase::initCommon() {
    fFlags = 0;
    unsigned colorAlpha = 0xFF;
    for (int i = 0; i < fColorCount; i++) {
        colorAlpha &= SkColorGetA(fOrigColors[i]);
    }
    fColorsAreOpaque = colorAlpha == 0xFF;
}

void SkGradientShaderBase::flatten(SkWriteBuffer& buffer) const {
    this->INHERITED::flatten(buffer);
    buffer.writeFlattenable(fMapper);
    buffer.writeColorArray(fOrigColors, fColorCount);
    buffer.writeUInt(pack_mode_flags(fTileMode, fGradFlags));
    if (fColorCount > 2) {
        Rec* recs = fRecs;
        for (int i = 1; i < fColorCount; i++) {
            buffer.writeInt(recs[i].fPos);
            buffer.writeUInt(recs[i].fScale);
        }
    }
    buffer.writeMatrix(fPtsToUnit);
}

bool SkGradientShaderBase::isOpaque() const {
    return fColorsAreOpaque;
}

bool SkGradientShaderBase::setContext(const SkBitmap& device,
                                 const SkPaint& paint,
                                 const SkMatrix& matrix) {
    if (!this->INHERITED::setContext(device, paint, matrix)) {
        return false;
    }

    const SkMatrix& inverse = this->getTotalInverse();

    if (!fDstToIndex.setConcat(fPtsToUnit, inverse)) {
        
        this->INHERITED::endContext();
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

void SkGradientShaderBase::setCacheAlpha(U8CPU alpha) const {
    
    
    
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





void SkGradientShaderBase::Build16bitCache(uint16_t cache[], SkColor c0, SkColor c1,
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

















typedef uint32_t SkUFixed;

void SkGradientShaderBase::Build32bitCache(SkPMColor cache[], SkColor c0, SkColor c1,
                                      int count, U8CPU paintAlpha, uint32_t gradFlags) {
    SkASSERT(count > 1);

    
    uint32_t a0 = SkMulDiv255Round(SkColorGetA(c0), paintAlpha);
    uint32_t a1 = SkMulDiv255Round(SkColorGetA(c1), paintAlpha);


    const bool interpInPremul = SkToBool(gradFlags &
                           SkGradientShader::kInterpolateColorsInPremul_Flag);

    uint32_t r0 = SkColorGetR(c0);
    uint32_t g0 = SkColorGetG(c0);
    uint32_t b0 = SkColorGetB(c0);

    uint32_t r1 = SkColorGetR(c1);
    uint32_t g1 = SkColorGetG(c1);
    uint32_t b1 = SkColorGetB(c1);

    if (interpInPremul) {
        r0 = SkMulDiv255Round(r0, a0);
        g0 = SkMulDiv255Round(g0, a0);
        b0 = SkMulDiv255Round(b0, a0);

        r1 = SkMulDiv255Round(r1, a1);
        g1 = SkMulDiv255Round(g1, a1);
        b1 = SkMulDiv255Round(b1, a1);
    }

    SkFixed da = SkIntToFixed(a1 - a0) / (count - 1);
    SkFixed dr = SkIntToFixed(r1 - r0) / (count - 1);
    SkFixed dg = SkIntToFixed(g1 - g0) / (count - 1);
    SkFixed db = SkIntToFixed(b1 - b0) / (count - 1);

    





    SkUFixed a = SkIntToFixed(a0) + 0x2000;
    SkUFixed r = SkIntToFixed(r0) + 0x2000;
    SkUFixed g = SkIntToFixed(g0) + 0x2000;
    SkUFixed b = SkIntToFixed(b0) + 0x2000;

    










    if (0xFF == a0 && 0 == da) {
        do {
            cache[kCache32Count*0] = SkPackARGB32(0xFF, (r + 0     ) >> 16,
                                                        (g + 0     ) >> 16,
                                                        (b + 0     ) >> 16);
            cache[kCache32Count*1] = SkPackARGB32(0xFF, (r + 0x8000) >> 16,
                                                        (g + 0x8000) >> 16,
                                                        (b + 0x8000) >> 16);
            cache[kCache32Count*2] = SkPackARGB32(0xFF, (r + 0xC000) >> 16,
                                                        (g + 0xC000) >> 16,
                                                        (b + 0xC000) >> 16);
            cache[kCache32Count*3] = SkPackARGB32(0xFF, (r + 0x4000) >> 16,
                                                        (g + 0x4000) >> 16,
                                                        (b + 0x4000) >> 16);
            cache += 1;
            r += dr;
            g += dg;
            b += db;
        } while (--count != 0);
    } else if (interpInPremul) {
        do {
            cache[kCache32Count*0] = SkPackARGB32((a + 0     ) >> 16,
                                                  (r + 0     ) >> 16,
                                                  (g + 0     ) >> 16,
                                                  (b + 0     ) >> 16);
            cache[kCache32Count*1] = SkPackARGB32((a + 0x8000) >> 16,
                                                  (r + 0x8000) >> 16,
                                                  (g + 0x8000) >> 16,
                                                  (b + 0x8000) >> 16);
            cache[kCache32Count*2] = SkPackARGB32((a + 0xC000) >> 16,
                                                  (r + 0xC000) >> 16,
                                                  (g + 0xC000) >> 16,
                                                  (b + 0xC000) >> 16);
            cache[kCache32Count*3] = SkPackARGB32((a + 0x4000) >> 16,
                                                  (r + 0x4000) >> 16,
                                                  (g + 0x4000) >> 16,
                                                  (b + 0x4000) >> 16);
            cache += 1;
            a += da;
            r += dr;
            g += dg;
            b += db;
        } while (--count != 0);
    } else {    
        do {
            cache[kCache32Count*0] = SkPremultiplyARGBInline((a + 0     ) >> 16,
                                                             (r + 0     ) >> 16,
                                                             (g + 0     ) >> 16,
                                                             (b + 0     ) >> 16);
            cache[kCache32Count*1] = SkPremultiplyARGBInline((a + 0x8000) >> 16,
                                                             (r + 0x8000) >> 16,
                                                             (g + 0x8000) >> 16,
                                                             (b + 0x8000) >> 16);
            cache[kCache32Count*2] = SkPremultiplyARGBInline((a + 0xC000) >> 16,
                                                             (r + 0xC000) >> 16,
                                                             (g + 0xC000) >> 16,
                                                             (b + 0xC000) >> 16);
            cache[kCache32Count*3] = SkPremultiplyARGBInline((a + 0x4000) >> 16,
                                                             (r + 0x4000) >> 16,
                                                             (g + 0x4000) >> 16,
                                                             (b + 0x4000) >> 16);
            cache += 1;
            a += da;
            r += dr;
            g += dg;
            b += db;
        } while (--count != 0);
    }
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

const uint16_t* SkGradientShaderBase::getCache16() const {
    if (fCache16 == NULL) {
        
        const int entryCount = kCache16Count * 2;
        const size_t allocSize = sizeof(uint16_t) * entryCount;

        if (fCache16Storage == NULL) { 
            fCache16Storage = (uint16_t*)sk_malloc_throw(allocSize);
        }
        fCache16 = fCache16Storage;
        if (fColorCount == 2) {
            Build16bitCache(fCache16, fOrigColors[0], fOrigColors[1],
                            kCache16Count);
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
        }

        if (fMapper) {
            fCache16Storage = (uint16_t*)sk_malloc_throw(allocSize);
            uint16_t* linear = fCache16;         
            uint16_t* mapped = fCache16Storage;  
            SkUnitMapper* map = fMapper;
            for (int i = 0; i < kCache16Count; i++) {
                int index = map->mapUnit16(bitsTo16(i, kCache16Bits)) >> kCache16Shift;
                mapped[i] = linear[index];
                mapped[i + kCache16Count] = linear[index + kCache16Count];
            }
            sk_free(fCache16);
            fCache16 = fCache16Storage;
        }
    }
    return fCache16;
}

const SkPMColor* SkGradientShaderBase::getCache32() const {
    if (fCache32 == NULL) {
        SkImageInfo info;
        info.fWidth = kCache32Count;
        info.fHeight = 4;   
        info.fAlphaType = kPremul_SkAlphaType;
        info.fColorType = kPMColor_SkColorType;

        if (NULL == fCache32PixelRef) {
            fCache32PixelRef = SkMallocPixelRef::NewAllocate(info, 0, NULL);
        }
        fCache32 = (SkPMColor*)fCache32PixelRef->getAddr();
        if (fColorCount == 2) {
            Build32bitCache(fCache32, fOrigColors[0], fOrigColors[1],
                            kCache32Count, fCacheAlpha, fGradFlags);
        } else {
            Rec* rec = fRecs;
            int prevIndex = 0;
            for (int i = 1; i < fColorCount; i++) {
                int nextIndex = SkFixedToFFFF(rec[i].fPos) >> kCache32Shift;
                SkASSERT(nextIndex < kCache32Count);

                if (nextIndex > prevIndex)
                    Build32bitCache(fCache32 + prevIndex, fOrigColors[i-1],
                                    fOrigColors[i], nextIndex - prevIndex + 1,
                                    fCacheAlpha, fGradFlags);
                prevIndex = nextIndex;
            }
        }

        if (fMapper) {
            SkMallocPixelRef* newPR = SkMallocPixelRef::NewAllocate(info, 0, NULL);
            SkPMColor* linear = fCache32;           
            SkPMColor* mapped = (SkPMColor*)newPR->getAddr();    
            SkUnitMapper* map = fMapper;
            for (int i = 0; i < kCache32Count; i++) {
                int index = map->mapUnit16((i << 8) | i) >> 8;
                mapped[i + kCache32Count*0] = linear[index + kCache32Count*0];
                mapped[i + kCache32Count*1] = linear[index + kCache32Count*1];
                mapped[i + kCache32Count*2] = linear[index + kCache32Count*2];
                mapped[i + kCache32Count*3] = linear[index + kCache32Count*3];
            }
            fCache32PixelRef->unref();
            fCache32PixelRef = newPR;
            fCache32 = (SkPMColor*)newPR->getAddr();
        }
    }
    return fCache32;
}









void SkGradientShaderBase::getGradientTableBitmap(SkBitmap* bitmap) const {
    
    
    this->setCacheAlpha(0xFF);

    
    if (fMapper) {
        
        (void)this->getCache32();
        bitmap->setConfig(SkBitmap::kARGB_8888_Config, kCache32Count, 1);
        bitmap->setPixelRef(fCache32PixelRef);
        return;
    }

    
    int count = 1 + fColorCount + 1;
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
    *buffer++ = fGradFlags;
    SkASSERT(buffer - storage.get() == count);

    

    SK_DECLARE_STATIC_MUTEX(gMutex);
    static SkBitmapCache* gCache;
    
    static const int MAX_NUM_CACHED_GRADIENT_BITMAPS = 32;
    SkAutoMutexAcquire ama(gMutex);

    if (NULL == gCache) {
        gCache = SkNEW_ARGS(SkBitmapCache, (MAX_NUM_CACHED_GRADIENT_BITMAPS));
    }
    size_t size = count * sizeof(int32_t);

    if (!gCache->find(storage.get(), size, bitmap)) {
        
        (void)this->getCache32();
        bitmap->setConfig(SkBitmap::kARGB_8888_Config, kCache32Count, 1);
        bitmap->setPixelRef(fCache32PixelRef);

        gCache->add(storage.get(), size, *bitmap);
    }
}

void SkGradientShaderBase::commonAsAGradient(GradientInfo* info) const {
    if (info) {
        if (info->fColorCount >= fColorCount) {
            if (info->fColors) {
                memcpy(info->fColors, fOrigColors, fColorCount * sizeof(SkColor));
            }
            if (info->fColorOffsets) {
                if (fColorCount == 2) {
                    info->fColorOffsets[0] = 0;
                    info->fColorOffsets[1] = SK_Scalar1;
                } else if (fColorCount > 2) {
                    for (int i = 0; i < fColorCount; ++i) {
                        info->fColorOffsets[i] = SkFixedToScalar(fRecs[i].fPos);
                    }
                }
            }
        }
        info->fColorCount = fColorCount;
        info->fTileMode = fTileMode;
        info->fGradientFlags = fGradFlags;
    }
}

#ifdef SK_DEVELOPER
void SkGradientShaderBase::toString(SkString* str) const {

    str->appendf("%d colors: ", fColorCount);

    for (int i = 0; i < fColorCount; ++i) {
        str->appendHex(fOrigColors[i]);
        if (i < fColorCount-1) {
            str->append(", ");
        }
    }

    if (fColorCount > 2) {
        str->append(" points: (");
        for (int i = 0; i < fColorCount; ++i) {
            str->appendScalar(SkFixedToScalar(fRecs[i].fPos));
            if (i < fColorCount-1) {
                str->append(", ");
            }
        }
        str->append(")");
    }

    static const char* gTileModeName[SkShader::kTileModeCount] = {
        "clamp", "repeat", "mirror"
    };

    str->append(" ");
    str->append(gTileModeName[fTileMode]);

    

    this->INHERITED::toString(str);
}
#endif




#include "SkEmptyShader.h"


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

static void desc_init(SkGradientShaderBase::Descriptor* desc,
                      const SkColor colors[],
                      const SkScalar pos[], int colorCount,
                      SkShader::TileMode mode,
                      SkUnitMapper* mapper, uint32_t flags) {
    desc->fColors   = colors;
    desc->fPos      = pos;
    desc->fCount    = colorCount;
    desc->fTileMode = mode;
    desc->fMapper   = mapper;
    desc->fFlags    = flags;
}

SkShader* SkGradientShader::CreateLinear(const SkPoint pts[2],
                                         const SkColor colors[],
                                         const SkScalar pos[], int colorCount,
                                         SkShader::TileMode mode,
                                         SkUnitMapper* mapper,
                                         uint32_t flags) {
    if (NULL == pts || NULL == colors || colorCount < 1) {
        return NULL;
    }
    EXPAND_1_COLOR(colorCount);

    SkGradientShaderBase::Descriptor desc;
    desc_init(&desc, colors, pos, colorCount, mode, mapper, flags);
    return SkNEW_ARGS(SkLinearGradient, (pts, desc));
}

SkShader* SkGradientShader::CreateRadial(const SkPoint& center, SkScalar radius,
                                         const SkColor colors[],
                                         const SkScalar pos[], int colorCount,
                                         SkShader::TileMode mode,
                                         SkUnitMapper* mapper,
                                         uint32_t flags) {
    if (radius <= 0 || NULL == colors || colorCount < 1) {
        return NULL;
    }
    EXPAND_1_COLOR(colorCount);

    SkGradientShaderBase::Descriptor desc;
    desc_init(&desc, colors, pos, colorCount, mode, mapper, flags);
    return SkNEW_ARGS(SkRadialGradient, (center, radius, desc));
}

SkShader* SkGradientShader::CreateTwoPointRadial(const SkPoint& start,
                                                 SkScalar startRadius,
                                                 const SkPoint& end,
                                                 SkScalar endRadius,
                                                 const SkColor colors[],
                                                 const SkScalar pos[],
                                                 int colorCount,
                                                 SkShader::TileMode mode,
                                                 SkUnitMapper* mapper,
                                                 uint32_t flags) {
    if (startRadius < 0 || endRadius < 0 || NULL == colors || colorCount < 1) {
        return NULL;
    }
    EXPAND_1_COLOR(colorCount);

    SkGradientShaderBase::Descriptor desc;
    desc_init(&desc, colors, pos, colorCount, mode, mapper, flags);
    return SkNEW_ARGS(SkTwoPointRadialGradient,
                      (start, startRadius, end, endRadius, desc));
}

SkShader* SkGradientShader::CreateTwoPointConical(const SkPoint& start,
                                                  SkScalar startRadius,
                                                  const SkPoint& end,
                                                  SkScalar endRadius,
                                                  const SkColor colors[],
                                                  const SkScalar pos[],
                                                  int colorCount,
                                                  SkShader::TileMode mode,
                                                  SkUnitMapper* mapper,
                                                  uint32_t flags) {
    if (startRadius < 0 || endRadius < 0 || NULL == colors || colorCount < 1) {
        return NULL;
    }
    if (start == end && startRadius == endRadius) {
        return SkNEW(SkEmptyShader);
    }
    EXPAND_1_COLOR(colorCount);

    SkGradientShaderBase::Descriptor desc;
    desc_init(&desc, colors, pos, colorCount, mode, mapper, flags);
    return SkNEW_ARGS(SkTwoPointConicalGradient,
                      (start, startRadius, end, endRadius, desc));
}

SkShader* SkGradientShader::CreateSweep(SkScalar cx, SkScalar cy,
                                        const SkColor colors[],
                                        const SkScalar pos[],
                                        int colorCount, SkUnitMapper* mapper,
                                        uint32_t flags) {
    if (NULL == colors || colorCount < 1) {
        return NULL;
    }
    EXPAND_1_COLOR(colorCount);

    SkGradientShaderBase::Descriptor desc;
    desc_init(&desc, colors, pos, colorCount, SkShader::kClamp_TileMode, mapper, flags);
    return SkNEW_ARGS(SkSweepGradient, (cx, cy, desc));
}

SK_DEFINE_FLATTENABLE_REGISTRAR_GROUP_START(SkGradientShader)
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(SkLinearGradient)
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(SkRadialGradient)
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(SkSweepGradient)
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(SkTwoPointRadialGradient)
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(SkTwoPointConicalGradient)
SK_DEFINE_FLATTENABLE_REGISTRAR_GROUP_END



#if SK_SUPPORT_GPU

#include "effects/GrTextureStripAtlas.h"
#include "GrTBackendEffectFactory.h"
#include "SkGr.h"

GrGLGradientEffect::GrGLGradientEffect(const GrBackendEffectFactory& factory)
    : INHERITED(factory)
    , fCachedYCoord(SK_ScalarMax) {
}

GrGLGradientEffect::~GrGLGradientEffect() { }

void GrGLGradientEffect::emitUniforms(GrGLShaderBuilder* builder, EffectKey key) {

    if (GrGradientEffect::kTwo_ColorType == ColorTypeFromKey(key)) { 
        fColorStartUni = builder->addUniform(GrGLShaderBuilder::kFragment_Visibility,
                                             kVec4f_GrSLType, "GradientStartColor");
        fColorEndUni = builder->addUniform(GrGLShaderBuilder::kFragment_Visibility,
                                           kVec4f_GrSLType, "GradientEndColor");

    } else if (GrGradientEffect::kThree_ColorType == ColorTypeFromKey(key)){ 
        fColorStartUni = builder->addUniform(GrGLShaderBuilder::kFragment_Visibility,
                                             kVec4f_GrSLType, "GradientStartColor");
        fColorMidUni = builder->addUniform(GrGLShaderBuilder::kFragment_Visibility,
                                           kVec4f_GrSLType, "GradientMidColor");
        fColorEndUni = builder->addUniform(GrGLShaderBuilder::kFragment_Visibility,
                                             kVec4f_GrSLType, "GradientEndColor");

    } else { 
        fFSYUni = builder->addUniform(GrGLShaderBuilder::kFragment_Visibility,
                                      kFloat_GrSLType, "GradientYCoordFS");
    }
}

static inline void set_color_uni(const GrGLUniformManager& uman,
                                 const GrGLUniformManager::UniformHandle uni,
                                 const SkColor* color) {
       uman.set4f(uni,
                  SkColorGetR(*color) / 255.f,
                  SkColorGetG(*color) / 255.f,
                  SkColorGetB(*color) / 255.f,
                  SkColorGetA(*color) / 255.f);
}

static inline void set_mul_color_uni(const GrGLUniformManager& uman,
                                     const GrGLUniformManager::UniformHandle uni,
                                     const SkColor* color){
       float a = SkColorGetA(*color) / 255.f;
       float aDiv255 = a / 255.f;
       uman.set4f(uni,
                  SkColorGetR(*color) * aDiv255,
                  SkColorGetG(*color) * aDiv255,
                  SkColorGetB(*color) * aDiv255,
                  a);
}

void GrGLGradientEffect::setData(const GrGLUniformManager& uman,
                                 const GrDrawEffect& drawEffect) {

    const GrGradientEffect& e = drawEffect.castEffect<GrGradientEffect>();


    if (GrGradientEffect::kTwo_ColorType == e.getColorType()){

        if (GrGradientEffect::kBeforeInterp_PremulType == e.getPremulType()) {
            set_mul_color_uni(uman, fColorStartUni, e.getColors(0));
            set_mul_color_uni(uman, fColorEndUni,   e.getColors(1));
        } else {
            set_color_uni(uman, fColorStartUni, e.getColors(0));
            set_color_uni(uman, fColorEndUni,   e.getColors(1));
        }

    } else if (GrGradientEffect::kThree_ColorType == e.getColorType()){

        if (GrGradientEffect::kBeforeInterp_PremulType == e.getPremulType()) {
            set_mul_color_uni(uman, fColorStartUni, e.getColors(0));
            set_mul_color_uni(uman, fColorMidUni,   e.getColors(1));
            set_mul_color_uni(uman, fColorEndUni,   e.getColors(2));
        } else {
            set_color_uni(uman, fColorStartUni, e.getColors(0));
            set_color_uni(uman, fColorMidUni,   e.getColors(1));
            set_color_uni(uman, fColorEndUni,   e.getColors(2));
        }
    } else {

        SkScalar yCoord = e.getYCoord();
        if (yCoord != fCachedYCoord) {
            uman.set1f(fFSYUni, yCoord);
            fCachedYCoord = yCoord;
        }
    }
}


GrGLEffect::EffectKey GrGLGradientEffect::GenBaseGradientKey(const GrDrawEffect& drawEffect) {
    const GrGradientEffect& e = drawEffect.castEffect<GrGradientEffect>();

    EffectKey key = 0;

    if (GrGradientEffect::kTwo_ColorType == e.getColorType()) {
        key |= kTwoColorKey;
    } else if (GrGradientEffect::kThree_ColorType == e.getColorType()){
        key |= kThreeColorKey;
    }

    if (GrGradientEffect::kBeforeInterp_PremulType == e.getPremulType()) {
        key |= kPremulBeforeInterpKey;
    }

    return key;
}

void GrGLGradientEffect::emitColor(GrGLShaderBuilder* builder,
                                   const char* gradientTValue,
                                   EffectKey key,
                                   const char* outputColor,
                                   const char* inputColor,
                                   const TextureSamplerArray& samplers) {
    if (GrGradientEffect::kTwo_ColorType == ColorTypeFromKey(key)){
        builder->fsCodeAppendf("\tvec4 colorTemp = mix(%s, %s, clamp(%s, 0.0, 1.0));\n",
                               builder->getUniformVariable(fColorStartUni).c_str(),
                               builder->getUniformVariable(fColorEndUni).c_str(),
                               gradientTValue);
        
        
        
        
        
        if (GrGradientEffect::kAfterInterp_PremulType == PremulTypeFromKey(key)) {
            builder->fsCodeAppend("\tcolorTemp.rgb *= colorTemp.a;\n");
        }

        builder->fsCodeAppendf("\t%s = %s;\n", outputColor,
                               (GrGLSLExpr4(inputColor) * GrGLSLExpr4("colorTemp")).c_str());
    } else if (GrGradientEffect::kThree_ColorType == ColorTypeFromKey(key)){
        builder->fsCodeAppendf("\tfloat oneMinus2t = 1.0 - (2.0 * (%s));\n",
                               gradientTValue);
        builder->fsCodeAppendf("\tvec4 colorTemp = clamp(oneMinus2t, 0.0, 1.0) * %s;\n",
                               builder->getUniformVariable(fColorStartUni).c_str());
        if (kTegra3_GrGLRenderer == builder->ctxInfo().renderer()) {
            
            
            builder->fsCodeAppend("\tfloat minAbs = abs(oneMinus2t);\n");
            builder->fsCodeAppend("\tminAbs = minAbs > 1.0 ? 1.0 : minAbs;\n");
            builder->fsCodeAppendf("\tcolorTemp += (1.0 - minAbs) * %s;\n",
                                   builder->getUniformVariable(fColorMidUni).c_str());
        } else {
            builder->fsCodeAppendf("\tcolorTemp += (1.0 - min(abs(oneMinus2t), 1.0)) * %s;\n",
                                   builder->getUniformVariable(fColorMidUni).c_str());
        }
        builder->fsCodeAppendf("\tcolorTemp += clamp(-oneMinus2t, 0.0, 1.0) * %s;\n",
                               builder->getUniformVariable(fColorEndUni).c_str());
        if (GrGradientEffect::kAfterInterp_PremulType == PremulTypeFromKey(key)) {
            builder->fsCodeAppend("\tcolorTemp.rgb *= colorTemp.a;\n");
        }

        builder->fsCodeAppendf("\t%s = %s;\n", outputColor,
                               (GrGLSLExpr4(inputColor) * GrGLSLExpr4("colorTemp")).c_str());
    } else {
        builder->fsCodeAppendf("\tvec2 coord = vec2(%s, %s);\n",
                               gradientTValue,
                               builder->getUniformVariable(fFSYUni).c_str());
        builder->fsCodeAppendf("\t%s = ", outputColor);
        builder->fsAppendTextureLookupAndModulate(inputColor,
                                                  samplers[0],
                                                  "coord");
        builder->fsCodeAppend(";\n");
    }
}



GrGradientEffect::GrGradientEffect(GrContext* ctx,
                                   const SkGradientShaderBase& shader,
                                   const SkMatrix& matrix,
                                   SkShader::TileMode tileMode) {

    fIsOpaque = shader.isOpaque();

    SkShader::GradientInfo info;
    SkScalar pos[3] = {0};

    info.fColorCount = 3;
    info.fColors = &fColors[0];
    info.fColorOffsets = &pos[0];
    shader.asAGradient(&info);

    
    bool foundSpecialCase = false;
    if (SkShader::kClamp_TileMode == info.fTileMode) {
        if (2 == info.fColorCount) {
            fRow = -1; 
            fColorType = kTwo_ColorType;
            foundSpecialCase = true;
        } else if (3 == info.fColorCount &&
                   (SkScalarAbs(pos[1] - SK_ScalarHalf) < SK_Scalar1 / 1000)) { 
            fRow = -1; 
            fColorType = kThree_ColorType;
            foundSpecialCase = true;
        }
    }
    if (foundSpecialCase) {
        if (SkGradientShader::kInterpolateColorsInPremul_Flag & info.fGradientFlags) {
            fPremulType = kBeforeInterp_PremulType;
        } else {
            fPremulType = kAfterInterp_PremulType;
        }
        fCoordTransform.reset(kCoordSet, matrix);
    } else {
        
        fPremulType = kBeforeInterp_PremulType;
        SkBitmap bitmap;
        shader.getGradientTableBitmap(&bitmap);
        fColorType = kTexture_ColorType;

        GrTextureStripAtlas::Desc desc;
        desc.fWidth  = bitmap.width();
        desc.fHeight = 32;
        desc.fRowHeight = bitmap.height();
        desc.fContext = ctx;
        desc.fConfig = SkBitmapConfig2GrPixelConfig(bitmap.config());
        fAtlas = GrTextureStripAtlas::GetAtlas(desc);
        SkASSERT(NULL != fAtlas);

        
        GrTextureParams params;
        params.setFilterMode(GrTextureParams::kBilerp_FilterMode);
        params.setTileModeX(tileMode);

        fRow = fAtlas->lockRow(bitmap);
        if (-1 != fRow) {
            fYCoord = fAtlas->getYOffset(fRow) + SK_ScalarHalf *
            fAtlas->getVerticalScaleFactor();
            fCoordTransform.reset(kCoordSet, matrix, fAtlas->getTexture());
            fTextureAccess.reset(fAtlas->getTexture(), params);
        } else {
            GrTexture* texture = GrLockAndRefCachedBitmapTexture(ctx, bitmap, &params);
            fCoordTransform.reset(kCoordSet, matrix, texture);
            fTextureAccess.reset(texture, params);
            fYCoord = SK_ScalarHalf;

            
            
            
            GrUnlockAndUnrefCachedBitmapTexture(texture);
        }
        this->addTextureAccess(&fTextureAccess);
    }
    this->addCoordTransform(&fCoordTransform);
}

GrGradientEffect::~GrGradientEffect() {
    if (this->useAtlas()) {
        fAtlas->unlockRow(fRow);
    }
}

bool GrGradientEffect::onIsEqual(const GrEffect& effect) const {
    const GrGradientEffect& s = CastEffect<GrGradientEffect>(effect);

    if (this->fColorType == s.getColorType()){

        if (kTwo_ColorType == fColorType) {
            if (*this->getColors(0) != *s.getColors(0) ||
                *this->getColors(1) != *s.getColors(1)) {
                return false;
            }
        } else if (kThree_ColorType == fColorType) {
            if (*this->getColors(0) != *s.getColors(0) ||
                *this->getColors(1) != *s.getColors(1) ||
                *this->getColors(2) != *s.getColors(2)) {
                return false;
            }
        } else {
            if (fYCoord != s.getYCoord()) {
                return false;
            }
        }

        return fTextureAccess.getTexture() == s.fTextureAccess.getTexture()  &&
            fTextureAccess.getParams().getTileModeX() ==
                s.fTextureAccess.getParams().getTileModeX() &&
            this->useAtlas() == s.useAtlas() &&
            fCoordTransform.getMatrix().cheapEqualTo(s.fCoordTransform.getMatrix());
    }

    return false;
}

void GrGradientEffect::getConstantColorComponents(GrColor* color, uint32_t* validFlags) const {
    if (fIsOpaque && (kA_GrColorComponentFlag & *validFlags) && 0xff == GrColorUnpackA(*color)) {
        *validFlags = kA_GrColorComponentFlag;
    } else {
        *validFlags = 0;
    }
}

int GrGradientEffect::RandomGradientParams(SkRandom* random,
                                           SkColor colors[],
                                           SkScalar** stops,
                                           SkShader::TileMode* tm) {
    int outColors = random->nextRangeU(1, kMaxRandomGradientColors);

    
    if (outColors == 1 || (outColors >= 2 && random->nextBool())) {
        *stops = NULL;
    }

    SkScalar stop = 0.f;
    for (int i = 0; i < outColors; ++i) {
        colors[i] = random->nextU();
        if (NULL != *stops) {
            (*stops)[i] = stop;
            stop = i < outColors - 1 ? stop + random->nextUScalar1() * (1.f - stop) : 1.f;
        }
    }
    *tm = static_cast<SkShader::TileMode>(random->nextULessThan(SkShader::kTileModeCount));

    return outColors;
}

#endif
