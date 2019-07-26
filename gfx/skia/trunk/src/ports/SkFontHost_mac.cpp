







#include <vector>
#ifdef SK_BUILD_FOR_MAC
#import <ApplicationServices/ApplicationServices.h>
#endif

#ifdef SK_BUILD_FOR_IOS
#include <CoreText/CoreText.h>
#include <CoreText/CTFontManager.h>
#include <CoreGraphics/CoreGraphics.h>
#include <CoreFoundation/CoreFoundation.h>
#endif

#include "SkFontHost.h"
#include "SkCGUtils.h"
#include "SkColorPriv.h"
#include "SkDescriptor.h"
#include "SkEndian.h"
#include "SkFontDescriptor.h"
#include "SkFloatingPoint.h"
#include "SkGlyph.h"
#include "SkMaskGamma.h"
#include "SkSFNTHeader.h"
#include "SkOTTable_glyf.h"
#include "SkOTTable_head.h"
#include "SkOTTable_hhea.h"
#include "SkOTTable_loca.h"
#include "SkOTUtils.h"
#include "SkPaint.h"
#include "SkPath.h"
#include "SkString.h"
#include "SkStream.h"
#include "SkThread.h"
#include "SkTypeface_mac.h"
#include "SkUtils.h"
#include "SkTypefaceCache.h"
#include "SkFontMgr.h"
#include "SkUtils.h"



class SkScalerContext_Mac;



static CFArrayRef SkCTFontManagerCopyAvailableFontFamilyNames() {
#ifdef SK_BUILD_FOR_IOS
    return CFArrayCreate(NULL, NULL, 0, NULL);
#else
    return CTFontManagerCopyAvailableFontFamilyNames();
#endif
}




template <typename T> static void CFSafeRelease(const T* cfTypeRef) {
    if (cfTypeRef) {
        CFRelease(cfTypeRef);
    }
}



template <typename T> static void CFSafeRetain(const T* cfTypeRef) {
    if (cfTypeRef) {
        CFRetain(cfTypeRef);
    }
}


template<typename CFRef> class AutoCFRelease : private SkNoncopyable {
public:
    explicit AutoCFRelease(CFRef cfRef = NULL) : fCFRef(cfRef) { }
    ~AutoCFRelease() { CFSafeRelease(fCFRef); }

    void reset(CFRef that = NULL) {
        CFSafeRetain(that);
        CFSafeRelease(fCFRef);
        fCFRef = that;
    }

    AutoCFRelease& operator =(CFRef that) {
        reset(that);
        return *this;
    }

    operator CFRef() const { return fCFRef; }
    CFRef get() const { return fCFRef; }

    CFRef* operator&() { SkASSERT(fCFRef == NULL); return &fCFRef; }
private:
    CFRef fCFRef;
};

static CFStringRef make_CFString(const char str[]) {
    return CFStringCreateWithCString(NULL, str, kCFStringEncodingUTF8);
}

template<typename T> class AutoCGTable : SkNoncopyable {
public:
    AutoCGTable(CGFontRef font)
    
    : fCFData(CGFontCopyTableForTag(font, SkSetFourByteTag(T::TAG0, T::TAG1, T::TAG2, T::TAG3)))
    , fData(fCFData ? reinterpret_cast<const T*>(CFDataGetBytePtr(fCFData)) : NULL)
    { }

    const T* operator->() const { return fData; }

private:
    AutoCFRelease<CFDataRef> fCFData;
public:
    const T* fData;
};



static bool CGRectIsEmpty_inline(const CGRect& rect) {
    return rect.size.width <= 0 || rect.size.height <= 0;
}

static CGFloat CGRectGetMinX_inline(const CGRect& rect) {
    return rect.origin.x;
}

static CGFloat CGRectGetMaxX_inline(const CGRect& rect) {
    return rect.origin.x + rect.size.width;
}

static CGFloat CGRectGetMinY_inline(const CGRect& rect) {
    return rect.origin.y;
}

static CGFloat CGRectGetMaxY_inline(const CGRect& rect) {
    return rect.origin.y + rect.size.height;
}

static CGFloat CGRectGetWidth_inline(const CGRect& rect) {
    return rect.size.width;
}



static void sk_memset_rect32(uint32_t* ptr, uint32_t value,
                             int width, int height, size_t rowBytes) {
    SkASSERT(width);
    SkASSERT(width * sizeof(uint32_t) <= rowBytes);

    if (width >= 32) {
        while (height) {
            sk_memset32(ptr, value, width);
            ptr = (uint32_t*)((char*)ptr + rowBytes);
            height -= 1;
        }
        return;
    }

    rowBytes -= width * sizeof(uint32_t);

    if (width >= 8) {
        while (height) {
            int w = width;
            do {
                *ptr++ = value; *ptr++ = value;
                *ptr++ = value; *ptr++ = value;
                *ptr++ = value; *ptr++ = value;
                *ptr++ = value; *ptr++ = value;
                w -= 8;
            } while (w >= 8);
            while (--w >= 0) {
                *ptr++ = value;
            }
            ptr = (uint32_t*)((char*)ptr + rowBytes);
            height -= 1;
        }
    } else {
        while (height) {
            int w = width;
            do {
                *ptr++ = value;
            } while (--w > 0);
            ptr = (uint32_t*)((char*)ptr + rowBytes);
            height -= 1;
        }
    }
}

#include <sys/utsname.h>

typedef uint32_t CGRGBPixel;

static unsigned CGRGBPixel_getAlpha(CGRGBPixel pixel) {
    return pixel & 0xFF;
}









#if !defined(MAC_OS_X_VERSION_10_6) || (MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_6)
CG_EXTERN void CGContextSetAllowsFontSmoothing(CGContextRef context, bool value);
CG_EXTERN void CGContextSetAllowsFontSubpixelPositioning(CGContextRef context, bool value);
CG_EXTERN void CGContextSetShouldSubpixelPositionFonts(CGContextRef context, bool value);
CG_EXTERN void CGContextSetAllowsFontSubpixelQuantization(CGContextRef context, bool value);
CG_EXTERN void CGContextSetShouldSubpixelQuantizeFonts(CGContextRef context, bool value);
#endif

static const char FONT_DEFAULT_NAME[] = "Lucida Sans";


static int readVersion() {
    struct utsname info;
    if (uname(&info) != 0) {
        SkDebugf("uname failed\n");
        return 0;
    }
    if (strcmp(info.sysname, "Darwin") != 0) {
        SkDebugf("unexpected uname sysname %s\n", info.sysname);
        return 0;
    }
    char* dot = strchr(info.release, '.');
    if (!dot) {
        SkDebugf("expected dot in uname release %s\n", info.release);
        return 0;
    }
    int version = atoi(info.release);
    if (version == 0) {
        SkDebugf("could not parse uname release %s\n", info.release);
    }
    return version;
}

static int darwinVersion() {
    static int darwin_version = readVersion();
    return darwin_version;
}

static bool isSnowLeopard() {
    return darwinVersion() == 10;
}

static bool isLion() {
    return darwinVersion() == 11;
}

static bool isMountainLion() {
    return darwinVersion() == 12;
}

static bool isLCDFormat(unsigned format) {
    return SkMask::kLCD16_Format == format || SkMask::kLCD32_Format == format;
}

static CGFloat ScalarToCG(SkScalar scalar) {
    if (sizeof(CGFloat) == sizeof(float)) {
        return SkScalarToFloat(scalar);
    } else {
        SkASSERT(sizeof(CGFloat) == sizeof(double));
        return (CGFloat) SkScalarToDouble(scalar);
    }
}

static SkScalar CGToScalar(CGFloat cgFloat) {
    if (sizeof(CGFloat) == sizeof(float)) {
        return cgFloat;
    } else {
        SkASSERT(sizeof(CGFloat) == sizeof(double));
        return SkDoubleToScalar(cgFloat);
    }
}

static CGAffineTransform MatrixToCGAffineTransform(const SkMatrix& matrix,
                                                   SkScalar sx = SK_Scalar1,
                                                   SkScalar sy = SK_Scalar1) {
    return CGAffineTransformMake( ScalarToCG(matrix[SkMatrix::kMScaleX] * sx),
                                 -ScalarToCG(matrix[SkMatrix::kMSkewY]  * sy),
                                 -ScalarToCG(matrix[SkMatrix::kMSkewX]  * sx),
                                  ScalarToCG(matrix[SkMatrix::kMScaleY] * sy),
                                  ScalarToCG(matrix[SkMatrix::kMTransX] * sx),
                                  ScalarToCG(matrix[SkMatrix::kMTransY] * sy));
}



#define BITMAP_INFO_RGB (kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Host)
#define BITMAP_INFO_GRAY (kCGImageAlphaNone)






static bool supports_LCD() {
    static int gSupportsLCD = -1;
    if (gSupportsLCD >= 0) {
        return (bool) gSupportsLCD;
    }
    uint32_t rgb = 0;
    AutoCFRelease<CGColorSpaceRef> colorspace(CGColorSpaceCreateDeviceRGB());
    AutoCFRelease<CGContextRef> cgContext(CGBitmapContextCreate(&rgb, 1, 1, 8, 4,
                                                                colorspace, BITMAP_INFO_RGB));
    CGContextSelectFont(cgContext, "Helvetica", 16, kCGEncodingMacRoman);
    CGContextSetShouldSmoothFonts(cgContext, true);
    CGContextSetShouldAntialias(cgContext, true);
    CGContextSetTextDrawingMode(cgContext, kCGTextFill);
    CGContextSetGrayFillColor(cgContext, 1, 1);
    CGContextShowTextAtPoint(cgContext, -1, 0, "|", 1);
    uint32_t r = (rgb >> 16) & 0xFF;
    uint32_t g = (rgb >>  8) & 0xFF;
    uint32_t b = (rgb >>  0) & 0xFF;
    gSupportsLCD = (r != g || r != b);
    return (bool) gSupportsLCD;
}

class Offscreen {
public:
    Offscreen();

    CGRGBPixel* getCG(const SkScalerContext_Mac& context, const SkGlyph& glyph,
                      CGGlyph glyphID, size_t* rowBytesPtr,
                      bool generateA8FromLCD);

private:
    enum {
        kSize = 32 * 32 * sizeof(CGRGBPixel)
    };
    SkAutoSMalloc<kSize> fImageStorage;
    AutoCFRelease<CGColorSpaceRef> fRGBSpace;

    
    AutoCFRelease<CGContextRef> fCG;
    SkISize fSize;
    bool fDoAA;
    bool fDoLCD;

    static int RoundSize(int dimension) {
        return SkNextPow2(dimension);
    }
};

Offscreen::Offscreen() : fRGBSpace(NULL), fCG(NULL),
                         fDoAA(false), fDoLCD(false) {
    fSize.set(0, 0);
}



static SkTypeface::Style computeStyleBits(CTFontRef font, bool* isFixedPitch) {
    unsigned style = SkTypeface::kNormal;
    CTFontSymbolicTraits traits = CTFontGetSymbolicTraits(font);

    if (traits & kCTFontBoldTrait) {
        style |= SkTypeface::kBold;
    }
    if (traits & kCTFontItalicTrait) {
        style |= SkTypeface::kItalic;
    }
    if (isFixedPitch) {
        *isFixedPitch = (traits & kCTFontMonoSpaceTrait) != 0;
    }
    return (SkTypeface::Style)style;
}

static SkFontID CTFontRef_to_SkFontID(CTFontRef fontRef) {
    SkFontID id = 0;


#ifdef SK_BUILD_FOR_MAC
    ATSFontRef ats = CTFontGetPlatformFont(fontRef, NULL);
    id = (SkFontID)ats;
    if (id != 0) {
        id &= 0x3FFFFFFF; 
        return id;
    }
#endif
    
    
    AutoCFRelease<CGFontRef> cgFont(CTFontCopyGraphicsFont(fontRef, NULL));
    AutoCGTable<SkOTTableHead> headTable(cgFont);
    if (headTable.fData) {
        id = (SkFontID) headTable->checksumAdjustment;
        id = (id & 0x3FFFFFFF) | 0x40000000; 
    }
    
    if (id == 0) {
        id = (SkFontID) (uintptr_t) fontRef;
        id = (id & 0x3FFFFFFF) | 0x80000000; 
    }
    return id;
}

static SkFontStyle stylebits2fontstyle(SkTypeface::Style styleBits) {
    return SkFontStyle((styleBits & SkTypeface::kBold)
                           ? SkFontStyle::kBold_Weight
                           : SkFontStyle::kNormal_Weight,
                       SkFontStyle::kNormal_Width,
                       (styleBits & SkTypeface::kItalic)
                           ? SkFontStyle::kItalic_Slant
                           : SkFontStyle::kUpright_Slant);
}

#define WEIGHT_THRESHOLD    ((SkFontStyle::kNormal_Weight + SkFontStyle::kBold_Weight)/2)

static SkTypeface::Style fontstyle2stylebits(const SkFontStyle& fs) {
    unsigned style = 0;
    if (fs.width() >= WEIGHT_THRESHOLD) {
        style |= SkTypeface::kBold;
    }
    if (fs.isItalic()) {
        style |= SkTypeface::kItalic;
    }
    return (SkTypeface::Style)style;
}

class SkTypeface_Mac : public SkTypeface {
public:
    SkTypeface_Mac(SkTypeface::Style style, SkFontID fontID, bool isFixedPitch,
                   CTFontRef fontRef, const char name[])
        : SkTypeface(style, fontID, isFixedPitch)
        , fName(name)
        , fFontRef(fontRef) 
        , fFontStyle(stylebits2fontstyle(style))
    {
        SkASSERT(fontRef);
    }

    SkTypeface_Mac(const SkFontStyle& fs, SkFontID fontID, bool isFixedPitch,
                   CTFontRef fontRef, const char name[])
        : SkTypeface(fontstyle2stylebits(fs), fontID, isFixedPitch)
        , fName(name)
        , fFontRef(fontRef) 
        , fFontStyle(fs)
    {
        SkASSERT(fontRef);
    }

    SkString fName;
    AutoCFRelease<CTFontRef> fFontRef;
    SkFontStyle fFontStyle;

protected:
    friend class SkFontHost;    

    virtual int onGetUPEM() const SK_OVERRIDE;
    virtual SkStream* onOpenStream(int* ttcIndex) const SK_OVERRIDE;
    virtual SkTypeface::LocalizedStrings* onCreateFamilyNameIterator() const SK_OVERRIDE;
    virtual int onGetTableTags(SkFontTableTag tags[]) const SK_OVERRIDE;
    virtual size_t onGetTableData(SkFontTableTag, size_t offset,
                                  size_t length, void* data) const SK_OVERRIDE;
    virtual SkScalerContext* onCreateScalerContext(const SkDescriptor*) const SK_OVERRIDE;
    virtual void onFilterRec(SkScalerContextRec*) const SK_OVERRIDE;
    virtual void onGetFontDescriptor(SkFontDescriptor*, bool*) const SK_OVERRIDE;
    virtual SkAdvancedTypefaceMetrics* onGetAdvancedTypefaceMetrics(
                                SkAdvancedTypefaceMetrics::PerGlyphInfo,
                                const uint32_t*, uint32_t) const SK_OVERRIDE;
    virtual int onCharsToGlyphs(const void* chars, Encoding, uint16_t glyphs[],
                                int glyphCount) const SK_OVERRIDE;
    virtual int onCountGlyphs() const SK_OVERRIDE;

private:

    typedef SkTypeface INHERITED;
};

static SkTypeface* NewFromFontRef(CTFontRef fontRef, const char name[]) {
    SkASSERT(fontRef);
    bool isFixedPitch;
    SkTypeface::Style style = computeStyleBits(fontRef, &isFixedPitch);
    SkFontID fontID = CTFontRef_to_SkFontID(fontRef);

    return new SkTypeface_Mac(style, fontID, isFixedPitch, fontRef, name);
}

static SkTypeface* NewFromName(const char familyName[], SkTypeface::Style theStyle) {
    CTFontRef ctFont = NULL;

    CTFontSymbolicTraits ctFontTraits = 0;
    if (theStyle & SkTypeface::kBold) {
        ctFontTraits |= kCTFontBoldTrait;
    }
    if (theStyle & SkTypeface::kItalic) {
        ctFontTraits |= kCTFontItalicTrait;
    }

    
    AutoCFRelease<CFStringRef> cfFontName(make_CFString(familyName));

    AutoCFRelease<CFNumberRef> cfFontTraits(
            CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &ctFontTraits));

    AutoCFRelease<CFMutableDictionaryRef> cfAttributes(
            CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
                                      &kCFTypeDictionaryKeyCallBacks,
                                      &kCFTypeDictionaryValueCallBacks));

    AutoCFRelease<CFMutableDictionaryRef> cfTraits(
            CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
                                      &kCFTypeDictionaryKeyCallBacks,
                                      &kCFTypeDictionaryValueCallBacks));

    
    if (cfFontName != NULL && cfFontTraits != NULL && cfAttributes != NULL && cfTraits != NULL) {
        CFDictionaryAddValue(cfTraits, kCTFontSymbolicTrait, cfFontTraits);

        CFDictionaryAddValue(cfAttributes, kCTFontFamilyNameAttribute, cfFontName);
        CFDictionaryAddValue(cfAttributes, kCTFontTraitsAttribute, cfTraits);

        AutoCFRelease<CTFontDescriptorRef> ctFontDesc(
                CTFontDescriptorCreateWithAttributes(cfAttributes));

        if (ctFontDesc != NULL) {
            ctFont = CTFontCreateWithFontDescriptor(ctFontDesc, 0, NULL);
        }
    }

    return ctFont ? NewFromFontRef(ctFont, familyName) : NULL;
}

static SkTypeface* GetDefaultFace() {
    SK_DECLARE_STATIC_MUTEX(gMutex);
    SkAutoMutexAcquire ma(gMutex);

    static SkTypeface* gDefaultFace;

    if (NULL == gDefaultFace) {
        gDefaultFace = NewFromName(FONT_DEFAULT_NAME, SkTypeface::kNormal);
        SkTypefaceCache::Add(gDefaultFace, SkTypeface::kNormal);
    }
    return gDefaultFace;
}



extern CTFontRef SkTypeface_GetCTFontRef(const SkTypeface* face);
CTFontRef SkTypeface_GetCTFontRef(const SkTypeface* face) {
    const SkTypeface_Mac* macface = (const SkTypeface_Mac*)face;
    return macface ? macface->fFontRef.get() : NULL;
}




SkTypeface* SkCreateTypefaceFromCTFont(CTFontRef fontRef) {
    SkFontID fontID = CTFontRef_to_SkFontID(fontRef);
    SkTypeface* face = SkTypefaceCache::FindByID(fontID);
    if (face) {
        face->ref();
    } else {
        face = NewFromFontRef(fontRef, NULL);
        SkTypefaceCache::Add(face, face->style());
        
        
        
        CFRetain(fontRef);
    }
    SkASSERT(face->getRefCnt() > 1);
    return face;
}

struct NameStyleRec {
    const char*         fName;
    SkTypeface::Style   fStyle;
};

static bool FindByNameStyle(SkTypeface* face, SkTypeface::Style style,
                            void* ctx) {
    const SkTypeface_Mac* mface = reinterpret_cast<SkTypeface_Mac*>(face);
    const NameStyleRec* rec = reinterpret_cast<const NameStyleRec*>(ctx);

    return rec->fStyle == style && mface->fName.equals(rec->fName);
}

static const char* map_css_names(const char* name) {
    static const struct {
        const char* fFrom;  
        const char* fTo;    
    } gPairs[] = {
        { "sans-serif", "Helvetica" },
        { "serif",      "Times"     },
        { "monospace",  "Courier"   }
    };

    for (size_t i = 0; i < SK_ARRAY_COUNT(gPairs); i++) {
        if (strcmp(name, gPairs[i].fFrom) == 0) {
            return gPairs[i].fTo;
        }
    }
    return name;    
}

static SkTypeface* create_typeface(const SkTypeface* familyFace,
                                   const char familyName[],
                                   SkTypeface::Style style) {
    if (familyName) {
        familyName = map_css_names(familyName);
    }

    
    
    if (familyName == NULL && familyFace != NULL) {
        familyFace->ref();
        return const_cast<SkTypeface*>(familyFace);
    }

    if (!familyName || !*familyName) {
        familyName = FONT_DEFAULT_NAME;
    }

    NameStyleRec rec = { familyName, style };
    SkTypeface* face = SkTypefaceCache::FindByProcAndRef(FindByNameStyle, &rec);

    if (NULL == face) {
        face = NewFromName(familyName, style);
        if (face) {
            SkTypefaceCache::Add(face, style);
        } else {
            face = GetDefaultFace();
            face->ref();
        }
    }
    return face;
}




struct GlyphRect {
    int16_t fMinX;
    int16_t fMinY;
    int16_t fMaxX;
    int16_t fMaxY;
};

class SkScalerContext_Mac : public SkScalerContext {
public:
    SkScalerContext_Mac(SkTypeface_Mac*, const SkDescriptor*);

protected:
    unsigned generateGlyphCount(void) SK_OVERRIDE;
    uint16_t generateCharToGlyph(SkUnichar uni) SK_OVERRIDE;
    void generateAdvance(SkGlyph* glyph) SK_OVERRIDE;
    void generateMetrics(SkGlyph* glyph) SK_OVERRIDE;
    void generateImage(const SkGlyph& glyph) SK_OVERRIDE;
    void generatePath(const SkGlyph& glyph, SkPath* path) SK_OVERRIDE;
    void generateFontMetrics(SkPaint::FontMetrics* mX, SkPaint::FontMetrics* mY) SK_OVERRIDE;

private:
    static void CTPathElement(void *info, const CGPathElement *element);

    
    void getVerticalOffset(CGGlyph glyphID, SkPoint* offset) const;

    



    uint16_t getFBoundingBoxesGlyphOffset();

    















    bool generateBBoxes();

    




    SkMatrix fFUnitMatrix;

    Offscreen fOffscreen;
    AutoCFRelease<CTFontRef> fCTFont;

    






    AutoCFRelease<CTFontRef> fCTVerticalFont;

    AutoCFRelease<CGFontRef> fCGFont;
    SkAutoTMalloc<GlyphRect> fFBoundingBoxes;
    uint16_t fFBoundingBoxesGlyphOffset;
    uint16_t fGlyphCount;
    bool fGeneratedFBoundingBoxes;
    const bool fDoSubPosition;
    const bool fVertical;

    friend class Offscreen;

    typedef SkScalerContext INHERITED;
};

SkScalerContext_Mac::SkScalerContext_Mac(SkTypeface_Mac* typeface,
                                         const SkDescriptor* desc)
        : INHERITED(typeface, desc)
        , fFBoundingBoxes()
        , fFBoundingBoxesGlyphOffset(0)
        , fGeneratedFBoundingBoxes(false)
        , fDoSubPosition(SkToBool(fRec.fFlags & kSubpixelPositioning_Flag))
        , fVertical(SkToBool(fRec.fFlags & kVertical_Flag))

{
    CTFontRef ctFont = typeface->fFontRef.get();
    CFIndex numGlyphs = CTFontGetGlyphCount(ctFont);
    SkASSERT(numGlyphs >= 1 && numGlyphs <= 0xFFFF);
    fGlyphCount = SkToU16(numGlyphs);

    fRec.getSingleMatrix(&fFUnitMatrix);
    CGAffineTransform transform = MatrixToCGAffineTransform(fFUnitMatrix);

    AutoCFRelease<CTFontDescriptorRef> ctFontDesc;
    if (fVertical) {
        AutoCFRelease<CFMutableDictionaryRef> cfAttributes(CFDictionaryCreateMutable(
                kCFAllocatorDefault, 0,
                &kCFTypeDictionaryKeyCallBacks,
                &kCFTypeDictionaryValueCallBacks));
        if (cfAttributes) {
            CTFontOrientation ctOrientation = kCTFontVerticalOrientation;
            AutoCFRelease<CFNumberRef> cfVertical(CFNumberCreate(
                    kCFAllocatorDefault, kCFNumberSInt32Type, &ctOrientation));
            CFDictionaryAddValue(cfAttributes, kCTFontOrientationAttribute, cfVertical);
            ctFontDesc = CTFontDescriptorCreateWithAttributes(cfAttributes);
        }
    }
    
    fCTFont = CTFontCreateCopyWithAttributes(ctFont, 1, &transform, ctFontDesc);
    fCGFont = CTFontCopyGraphicsFont(fCTFont, NULL);
    if (fVertical) {
        CGAffineTransform rotateLeft = CGAffineTransformMake(0, -1, 1, 0, 0, 0);
        transform = CGAffineTransformConcat(rotateLeft, transform);
        fCTVerticalFont = CTFontCreateCopyWithAttributes(ctFont, 1, &transform, NULL);
    }

    SkScalar emPerFUnit = SkScalarInvert(SkIntToScalar(CGFontGetUnitsPerEm(fCGFont)));
    fFUnitMatrix.preScale(emPerFUnit, -emPerFUnit);
}

CGRGBPixel* Offscreen::getCG(const SkScalerContext_Mac& context, const SkGlyph& glyph,
                             CGGlyph glyphID, size_t* rowBytesPtr,
                             bool generateA8FromLCD) {
    if (!fRGBSpace) {
        
        
        
        fRGBSpace = CGColorSpaceCreateDeviceRGB();
    }

    
    bool doAA = false;
    bool doLCD = false;

    if (SkMask::kBW_Format != glyph.fMaskFormat) {
        doLCD = true;
        doAA = true;
    }

    
    if (!generateA8FromLCD && SkMask::kA8_Format == glyph.fMaskFormat) {
        doLCD = false;
        doAA = true;
    }

    size_t rowBytes = fSize.fWidth * sizeof(CGRGBPixel);
    if (!fCG || fSize.fWidth < glyph.fWidth || fSize.fHeight < glyph.fHeight) {
        if (fSize.fWidth < glyph.fWidth) {
            fSize.fWidth = RoundSize(glyph.fWidth);
        }
        if (fSize.fHeight < glyph.fHeight) {
            fSize.fHeight = RoundSize(glyph.fHeight);
        }

        rowBytes = fSize.fWidth * sizeof(CGRGBPixel);
        void* image = fImageStorage.reset(rowBytes * fSize.fHeight);
        fCG = CGBitmapContextCreate(image, fSize.fWidth, fSize.fHeight, 8,
                                    rowBytes, fRGBSpace, BITMAP_INFO_RGB);

        
        
        CGContextSetAllowsFontSubpixelQuantization(fCG, false);
        CGContextSetShouldSubpixelQuantizeFonts(fCG, false);

        CGContextSetTextDrawingMode(fCG, kCGTextFill);
        CGContextSetFont(fCG, context.fCGFont);
        CGContextSetFontSize(fCG, 1 );
        CGContextSetTextMatrix(fCG, CTFontGetMatrix(context.fCTFont));

        
        
        
        CGContextSetAllowsFontSubpixelPositioning(fCG, context.fDoSubPosition || context.fVertical);
        CGContextSetShouldSubpixelPositionFonts(fCG, context.fDoSubPosition || context.fVertical);

        
        
        CGContextSetGrayFillColor(fCG, 1.0f, 1.0f);

        
        fDoAA = !doAA;
        fDoLCD = !doLCD;
    }

    if (fDoAA != doAA) {
        CGContextSetShouldAntialias(fCG, doAA);
        fDoAA = doAA;
    }
    if (fDoLCD != doLCD) {
        CGContextSetShouldSmoothFonts(fCG, doLCD);
        fDoLCD = doLCD;
    }

    CGRGBPixel* image = (CGRGBPixel*)fImageStorage.get();
    
    image += (fSize.fHeight - glyph.fHeight) * fSize.fWidth;

    
    sk_memset_rect32(image, 0, glyph.fWidth, glyph.fHeight, rowBytes);

    float subX = 0;
    float subY = 0;
    if (context.fDoSubPosition) {
        subX = SkFixedToFloat(glyph.getSubXFixed());
        subY = SkFixedToFloat(glyph.getSubYFixed());
    }

    
    if (context.fVertical) {
        SkPoint offset;
        context.getVerticalOffset(glyphID, &offset);
        subX += offset.fX;
        subY += offset.fY;
    }

    CGContextShowGlyphsAtPoint(fCG, -glyph.fLeft + subX,
                               glyph.fTop + glyph.fHeight - subY,
                               &glyphID, 1);

    SkASSERT(rowBytesPtr);
    *rowBytesPtr = rowBytes;
    return image;
}

void SkScalerContext_Mac::getVerticalOffset(CGGlyph glyphID, SkPoint* offset) const {
    
    
    CGSize cgVertOffset;
    CTFontGetVerticalTranslationsForGlyphs(fCTFont, &glyphID, &cgVertOffset, 1);

    SkPoint skVertOffset = { CGToScalar(cgVertOffset.width), CGToScalar(cgVertOffset.height) };
    if (isSnowLeopard()) {
        
        fFUnitMatrix.mapPoints(&skVertOffset, 1);
    } else {
        
        skVertOffset.fY = -skVertOffset.fY;
    }

    *offset = skVertOffset;
}

uint16_t SkScalerContext_Mac::getFBoundingBoxesGlyphOffset() {
    if (fFBoundingBoxesGlyphOffset) {
        return fFBoundingBoxesGlyphOffset;
    }
    fFBoundingBoxesGlyphOffset = fGlyphCount; 
    AutoCGTable<SkOTTableHorizontalHeader> hheaTable(fCGFont);
    if (hheaTable.fData) {
        fFBoundingBoxesGlyphOffset = SkEndian_SwapBE16(hheaTable->numberOfHMetrics);
    }
    return fFBoundingBoxesGlyphOffset;
}

bool SkScalerContext_Mac::generateBBoxes() {
    if (fGeneratedFBoundingBoxes) {
        return NULL != fFBoundingBoxes.get();
    }
    fGeneratedFBoundingBoxes = true;

    AutoCGTable<SkOTTableHead> headTable(fCGFont);
    if (!headTable.fData) {
        return false;
    }

    AutoCGTable<SkOTTableIndexToLocation> locaTable(fCGFont);
    if (!locaTable.fData) {
        return false;
    }

    AutoCGTable<SkOTTableGlyph> glyfTable(fCGFont);
    if (!glyfTable.fData) {
        return false;
    }

    uint16_t entries = fGlyphCount - fFBoundingBoxesGlyphOffset;
    fFBoundingBoxes.reset(entries);

    SkOTTableHead::IndexToLocFormat locaFormat = headTable->indexToLocFormat;
    SkOTTableGlyph::Iterator glyphDataIter(*glyfTable.fData, *locaTable.fData, locaFormat);
    glyphDataIter.advance(fFBoundingBoxesGlyphOffset);
    for (uint16_t boundingBoxesIndex = 0; boundingBoxesIndex < entries; ++boundingBoxesIndex) {
        const SkOTTableGlyphData* glyphData = glyphDataIter.next();
        GlyphRect& rect = fFBoundingBoxes[boundingBoxesIndex];
        rect.fMinX = SkEndian_SwapBE16(glyphData->xMin);
        rect.fMinY = SkEndian_SwapBE16(glyphData->yMin);
        rect.fMaxX = SkEndian_SwapBE16(glyphData->xMax);
        rect.fMaxY = SkEndian_SwapBE16(glyphData->yMax);
    }

    return true;
}

unsigned SkScalerContext_Mac::generateGlyphCount(void) {
    return fGlyphCount;
}

uint16_t SkScalerContext_Mac::generateCharToGlyph(SkUnichar uni) {
    CGGlyph cgGlyph[2];
    UniChar theChar[2]; 

    
    size_t numUniChar = SkUTF16_FromUnichar(uni, theChar);
    SkASSERT(sizeof(CGGlyph) <= sizeof(uint16_t));

    
    
    
    CTFontGetGlyphsForCharacters(fCTFont, theChar, cgGlyph, numUniChar);
    return cgGlyph[0];
}

void SkScalerContext_Mac::generateAdvance(SkGlyph* glyph) {
    this->generateMetrics(glyph);
}

void SkScalerContext_Mac::generateMetrics(SkGlyph* glyph) {
    const CGGlyph cgGlyph = (CGGlyph) glyph->getGlyphID(fBaseGlyphCount);
    glyph->zeroMetrics();

    
    CGSize cgAdvance;
    if (fVertical) {
        CTFontGetAdvancesForGlyphs(fCTVerticalFont, kCTFontVerticalOrientation,
                                   &cgGlyph, &cgAdvance, 1);
    } else {
        CTFontGetAdvancesForGlyphs(fCTFont, kCTFontHorizontalOrientation,
                                   &cgGlyph, &cgAdvance, 1);
    }
    glyph->fAdvanceX =  SkFloatToFixed_Check(cgAdvance.width);
    glyph->fAdvanceY = -SkFloatToFixed_Check(cgAdvance.height);

    
    
    SkRect skBounds;

    
    
    
    
    
    
    
    
    
    
    
    
    

    
    

    
    
    
    
    if ((isLion() || isMountainLion()) &&
        (cgGlyph < fGlyphCount && cgGlyph >= getFBoundingBoxesGlyphOffset() && generateBBoxes()))
    {
        const GlyphRect& gRect = fFBoundingBoxes[cgGlyph - fFBoundingBoxesGlyphOffset];
        if (gRect.fMinX >= gRect.fMaxX || gRect.fMinY >= gRect.fMaxY) {
            return;
        }
        skBounds = SkRect::MakeLTRB(gRect.fMinX, gRect.fMinY, gRect.fMaxX, gRect.fMaxY);
        
        fFUnitMatrix.mapRect(&skBounds);

    } else {
        
        CGRect cgBounds;
        CTFontGetBoundingRectsForGlyphs(fCTFont, kCTFontHorizontalOrientation,
                                        &cgGlyph, &cgBounds, 1);

        
        
        
        
        
        if (0 == cgAdvance.width && 0 == cgAdvance.height) {
            AutoCFRelease<CGPathRef> path(CTFontCreatePathForGlyph(fCTFont, cgGlyph, NULL));
            if (NULL == path || CGPathIsEmpty(path)) {
                return;
            }
        }

        if (CGRectIsEmpty_inline(cgBounds)) {
            return;
        }

        
        skBounds = SkRect::MakeXYWH(cgBounds.origin.x, -cgBounds.origin.y - cgBounds.size.height,
                                    cgBounds.size.width, cgBounds.size.height);
    }

    if (fVertical) {
        
        
        SkPoint offset;
        getVerticalOffset(cgGlyph, &offset);
        skBounds.offset(offset);
    }

    
    
    if (fDoSubPosition) {
        skBounds.fRight += SkFixedToFloat(glyph->getSubXFixed());
        skBounds.fBottom += SkFixedToFloat(glyph->getSubYFixed());
    }

    SkIRect skIBounds;
    skBounds.roundOut(&skIBounds);
    
    
    
    
    skIBounds.outset(1, 1);
    glyph->fLeft = SkToS16(skIBounds.fLeft);
    glyph->fTop = SkToS16(skIBounds.fTop);
    glyph->fWidth = SkToU16(skIBounds.width());
    glyph->fHeight = SkToU16(skIBounds.height());

#ifdef HACK_COLORGLYPHS
    glyph->fMaskFormat = SkMask::kARGB32_Format;
#endif
}

#include "SkColorPriv.h"

static void build_power_table(uint8_t table[], float ee) {
    for (int i = 0; i < 256; i++) {
        float x = i / 255.f;
        x = sk_float_pow(x, ee);
        int xx = SkScalarRoundToInt(x * 255);
        table[i] = SkToU8(xx);
    }
}








static const uint8_t* getInverseGammaTableCoreGraphicSmoothing() {
    static bool gInited;
    static uint8_t gTableCoreGraphicsSmoothing[256];
    if (!gInited) {
        build_power_table(gTableCoreGraphicsSmoothing, 2.0f);
        gInited = true;
    }
    return gTableCoreGraphicsSmoothing;
}

static void cgpixels_to_bits(uint8_t dst[], const CGRGBPixel src[], int count) {
    while (count > 0) {
        uint8_t mask = 0;
        for (int i = 7; i >= 0; --i) {
            mask |= (CGRGBPixel_getAlpha(*src++) >> 7) << i;
            if (0 == --count) {
                break;
            }
        }
        *dst++ = mask;
    }
}

template<bool APPLY_PREBLEND>
static inline uint8_t rgb_to_a8(CGRGBPixel rgb, const uint8_t* table8) {
    U8CPU r = (rgb >> 16) & 0xFF;
    U8CPU g = (rgb >>  8) & 0xFF;
    U8CPU b = (rgb >>  0) & 0xFF;
    return sk_apply_lut_if<APPLY_PREBLEND>(SkComputeLuminance(r, g, b), table8);
}
template<bool APPLY_PREBLEND>
static void rgb_to_a8(const CGRGBPixel* SK_RESTRICT cgPixels, size_t cgRowBytes,
                      const SkGlyph& glyph, const uint8_t* table8) {
    const int width = glyph.fWidth;
    size_t dstRB = glyph.rowBytes();
    uint8_t* SK_RESTRICT dst = (uint8_t*)glyph.fImage;

    for (int y = 0; y < glyph.fHeight; y++) {
        for (int i = 0; i < width; ++i) {
            dst[i] = rgb_to_a8<APPLY_PREBLEND>(cgPixels[i], table8);
        }
        cgPixels = (CGRGBPixel*)((char*)cgPixels + cgRowBytes);
        dst += dstRB;
    }
}

template<bool APPLY_PREBLEND>
static inline uint16_t rgb_to_lcd16(CGRGBPixel rgb, const uint8_t* tableR,
                                                    const uint8_t* tableG,
                                                    const uint8_t* tableB) {
    U8CPU r = sk_apply_lut_if<APPLY_PREBLEND>((rgb >> 16) & 0xFF, tableR);
    U8CPU g = sk_apply_lut_if<APPLY_PREBLEND>((rgb >>  8) & 0xFF, tableG);
    U8CPU b = sk_apply_lut_if<APPLY_PREBLEND>((rgb >>  0) & 0xFF, tableB);
    return SkPack888ToRGB16(r, g, b);
}
template<bool APPLY_PREBLEND>
static void rgb_to_lcd16(const CGRGBPixel* SK_RESTRICT cgPixels, size_t cgRowBytes, const SkGlyph& glyph,
                         const uint8_t* tableR, const uint8_t* tableG, const uint8_t* tableB) {
    const int width = glyph.fWidth;
    size_t dstRB = glyph.rowBytes();
    uint16_t* SK_RESTRICT dst = (uint16_t*)glyph.fImage;

    for (int y = 0; y < glyph.fHeight; y++) {
        for (int i = 0; i < width; i++) {
            dst[i] = rgb_to_lcd16<APPLY_PREBLEND>(cgPixels[i], tableR, tableG, tableB);
        }
        cgPixels = (CGRGBPixel*)((char*)cgPixels + cgRowBytes);
        dst = (uint16_t*)((char*)dst + dstRB);
    }
}

template<bool APPLY_PREBLEND>
static inline uint32_t rgb_to_lcd32(CGRGBPixel rgb, const uint8_t* tableR,
                                                    const uint8_t* tableG,
                                                    const uint8_t* tableB) {
    U8CPU r = sk_apply_lut_if<APPLY_PREBLEND>((rgb >> 16) & 0xFF, tableR);
    U8CPU g = sk_apply_lut_if<APPLY_PREBLEND>((rgb >>  8) & 0xFF, tableG);
    U8CPU b = sk_apply_lut_if<APPLY_PREBLEND>((rgb >>  0) & 0xFF, tableB);
    return SkPackARGB32(0xFF, r, g, b);
}
template<bool APPLY_PREBLEND>
static void rgb_to_lcd32(const CGRGBPixel* SK_RESTRICT cgPixels, size_t cgRowBytes, const SkGlyph& glyph,
                         const uint8_t* tableR, const uint8_t* tableG, const uint8_t* tableB) {
    const int width = glyph.fWidth;
    size_t dstRB = glyph.rowBytes();
    uint32_t* SK_RESTRICT dst = (uint32_t*)glyph.fImage;
    for (int y = 0; y < glyph.fHeight; y++) {
        for (int i = 0; i < width; i++) {
            dst[i] = rgb_to_lcd32<APPLY_PREBLEND>(cgPixels[i], tableR, tableG, tableB);
        }
        cgPixels = (CGRGBPixel*)((char*)cgPixels + cgRowBytes);
        dst = (uint32_t*)((char*)dst + dstRB);
    }
}

#ifdef HACK_COLORGLYPHS

static SkPMColor cgpixels_to_pmcolor(CGRGBPixel rgb, const SkGlyph& glyph,
                                     int x, int y) {
    U8CPU r = (rgb >> 16) & 0xFF;
    U8CPU g = (rgb >>  8) & 0xFF;
    U8CPU b = (rgb >>  0) & 0xFF;
    unsigned a = SkComputeLuminance(r, g, b);

    
    r = x * 255 / glyph.fWidth;
    g = 0;
    b = (glyph.fHeight - y) * 255 / glyph.fHeight;
    return SkPreMultiplyARGB(a, r, g, b);    
}
#endif

template <typename T> T* SkTAddByteOffset(T* ptr, size_t byteOffset) {
    return (T*)((char*)ptr + byteOffset);
}

void SkScalerContext_Mac::generateImage(const SkGlyph& glyph) {
    CGGlyph cgGlyph = (CGGlyph) glyph.getGlyphID(fBaseGlyphCount);

    
    bool generateA8FromLCD = fRec.getHinting() != SkPaint::kNo_Hinting;

    
    size_t cgRowBytes;
    CGRGBPixel* cgPixels = fOffscreen.getCG(*this, glyph, cgGlyph, &cgRowBytes, generateA8FromLCD);
    if (cgPixels == NULL) {
        return;
    }

    
    
    

    
    const bool isLCD = isLCDFormat(glyph.fMaskFormat);
    if (isLCD || (glyph.fMaskFormat == SkMask::kA8_Format && supports_LCD() && generateA8FromLCD)) {
        const uint8_t* table = getInverseGammaTableCoreGraphicSmoothing();

        
        
        
        
        
        CGRGBPixel* addr = cgPixels;
        for (int y = 0; y < glyph.fHeight; ++y) {
            for (int x = 0; x < glyph.fWidth; ++x) {
                int r = (addr[x] >> 16) & 0xFF;
                int g = (addr[x] >>  8) & 0xFF;
                int b = (addr[x] >>  0) & 0xFF;
                addr[x] = (table[r] << 16) | (table[g] << 8) | table[b];
            }
            addr = SkTAddByteOffset(addr, cgRowBytes);
        }
    }

    
    switch (glyph.fMaskFormat) {
        case SkMask::kLCD32_Format: {
            if (fPreBlend.isApplicable()) {
                rgb_to_lcd32<true>(cgPixels, cgRowBytes, glyph,
                                   fPreBlend.fR, fPreBlend.fG, fPreBlend.fB);
            } else {
                rgb_to_lcd32<false>(cgPixels, cgRowBytes, glyph,
                                    fPreBlend.fR, fPreBlend.fG, fPreBlend.fB);
            }
        } break;
        case SkMask::kLCD16_Format: {
            if (fPreBlend.isApplicable()) {
                rgb_to_lcd16<true>(cgPixels, cgRowBytes, glyph,
                                   fPreBlend.fR, fPreBlend.fG, fPreBlend.fB);
            } else {
                rgb_to_lcd16<false>(cgPixels, cgRowBytes, glyph,
                                    fPreBlend.fR, fPreBlend.fG, fPreBlend.fB);
            }
        } break;
        case SkMask::kA8_Format: {
            if (fPreBlend.isApplicable()) {
                rgb_to_a8<true>(cgPixels, cgRowBytes, glyph, fPreBlend.fG);
            } else {
                rgb_to_a8<false>(cgPixels, cgRowBytes, glyph, fPreBlend.fG);
            }
        } break;
        case SkMask::kBW_Format: {
            const int width = glyph.fWidth;
            size_t dstRB = glyph.rowBytes();
            uint8_t* dst = (uint8_t*)glyph.fImage;
            for (int y = 0; y < glyph.fHeight; y++) {
                cgpixels_to_bits(dst, cgPixels, width);
                cgPixels = (CGRGBPixel*)((char*)cgPixels + cgRowBytes);
                dst += dstRB;
            }
        } break;
#ifdef HACK_COLORGLYPHS
        case SkMask::kARGB32_Format: {
            const int width = glyph.fWidth;
            size_t dstRB = glyph.rowBytes();
            SkPMColor* dst = (SkPMColor*)glyph.fImage;
            for (int y = 0; y < glyph.fHeight; y++) {
                for (int x = 0; x < width; ++x) {
                    dst[x] = cgpixels_to_pmcolor(cgPixels[x], glyph, x, y);
                }
                cgPixels = (CGRGBPixel*)((char*)cgPixels + cgRowBytes);
                dst = (SkPMColor*)((char*)dst + dstRB);
            }
        } break;
#endif
        default:
            SkDEBUGFAIL("unexpected mask format");
            break;
    }
}






#define kScaleForSubPixelPositionHinting (4.0f)

void SkScalerContext_Mac::generatePath(const SkGlyph& glyph, SkPath* path) {
    CTFontRef font = fCTFont;
    SkScalar scaleX = SK_Scalar1;
    SkScalar scaleY = SK_Scalar1;

    








    if (fRec.fFlags & SkScalerContext::kSubpixelPositioning_Flag) {
        SkMatrix m;
        fRec.getSingleMatrix(&m);

        
        scaleX = scaleY = kScaleForSubPixelPositionHinting;
        
        switch (SkComputeAxisAlignmentForHText(m)) {
            case kX_SkAxisAlignment:
                scaleY = SK_Scalar1; 
                break;
            case kY_SkAxisAlignment:
                scaleX = SK_Scalar1; 
                break;
            default:
                break;
        }

        CGAffineTransform xform = MatrixToCGAffineTransform(m, scaleX, scaleY);
        
        font = CTFontCreateCopyWithAttributes(fCTFont, 1, &xform, NULL);
    }

    CGGlyph cgGlyph = (CGGlyph)glyph.getGlyphID(fBaseGlyphCount);
    AutoCFRelease<CGPathRef> cgPath(CTFontCreatePathForGlyph(font, cgGlyph, NULL));

    path->reset();
    if (cgPath != NULL) {
        CGPathApply(cgPath, path, SkScalerContext_Mac::CTPathElement);
    }

    if (fDoSubPosition) {
        SkMatrix m;
        m.setScale(SkScalarInvert(scaleX), SkScalarInvert(scaleY));
        path->transform(m);
        
        CFSafeRelease(font);
    }
    if (fVertical) {
        SkPoint offset;
        getVerticalOffset(cgGlyph, &offset);
        path->offset(offset.fX, offset.fY);
    }
}

void SkScalerContext_Mac::generateFontMetrics(SkPaint::FontMetrics* mx,
                                              SkPaint::FontMetrics* my) {
    CGRect theBounds = CTFontGetBoundingBox(fCTFont);

    SkPaint::FontMetrics theMetrics;
    theMetrics.fTop          = CGToScalar(-CGRectGetMaxY_inline(theBounds));
    theMetrics.fAscent       = CGToScalar(-CTFontGetAscent(fCTFont));
    theMetrics.fDescent      = CGToScalar( CTFontGetDescent(fCTFont));
    theMetrics.fBottom       = CGToScalar(-CGRectGetMinY_inline(theBounds));
    theMetrics.fLeading      = CGToScalar( CTFontGetLeading(fCTFont));
    theMetrics.fAvgCharWidth = CGToScalar( CGRectGetWidth_inline(theBounds));
    theMetrics.fXMin         = CGToScalar( CGRectGetMinX_inline(theBounds));
    theMetrics.fXMax         = CGToScalar( CGRectGetMaxX_inline(theBounds));
    theMetrics.fXHeight      = CGToScalar( CTFontGetXHeight(fCTFont));

    if (mx != NULL) {
        *mx = theMetrics;
    }
    if (my != NULL) {
        *my = theMetrics;
    }
}

void SkScalerContext_Mac::CTPathElement(void *info, const CGPathElement *element) {
    SkPath* skPath = (SkPath*)info;

    
    switch (element->type) {
        case kCGPathElementMoveToPoint:
            skPath->moveTo(element->points[0].x, -element->points[0].y);
            break;

        case kCGPathElementAddLineToPoint:
            skPath->lineTo(element->points[0].x, -element->points[0].y);
            break;

        case kCGPathElementAddQuadCurveToPoint:
            skPath->quadTo(element->points[0].x, -element->points[0].y,
                           element->points[1].x, -element->points[1].y);
            break;

        case kCGPathElementAddCurveToPoint:
            skPath->cubicTo(element->points[0].x, -element->points[0].y,
                            element->points[1].x, -element->points[1].y,
                            element->points[2].x, -element->points[2].y);
            break;

        case kCGPathElementCloseSubpath:
            skPath->close();
            break;

        default:
            SkDEBUGFAIL("Unknown path element!");
            break;
        }
}






static SkTypeface* create_from_dataProvider(CGDataProviderRef provider) {
    AutoCFRelease<CGFontRef> cg(CGFontCreateWithDataProvider(provider));
    if (NULL == cg) {
        return NULL;
    }
    CTFontRef ct = CTFontCreateWithGraphicsFont(cg, 0, NULL, NULL);
    return cg ? SkCreateTypefaceFromCTFont(ct) : NULL;
}




static void populate_glyph_to_unicode_slow(CTFontRef ctFont, CFIndex glyphCount,
                                           SkTDArray<SkUnichar>* glyphToUnicode) {
    glyphToUnicode->setCount(SkToInt(glyphCount));
    SkUnichar* out = glyphToUnicode->begin();
    sk_bzero(out, glyphCount * sizeof(SkUnichar));
    UniChar unichar = 0;
    while (glyphCount > 0) {
        CGGlyph glyph;
        if (CTFontGetGlyphsForCharacters(ctFont, &unichar, &glyph, 1)) {
            out[glyph] = unichar;
            --glyphCount;
        }
        if (++unichar == 0) {
            break;
        }
    }
}




static void populate_glyph_to_unicode(CTFontRef ctFont, CFIndex glyphCount,
                                      SkTDArray<SkUnichar>* glyphToUnicode) {
    AutoCFRelease<CFCharacterSetRef> charSet(CTFontCopyCharacterSet(ctFont));
    if (!charSet) {
        populate_glyph_to_unicode_slow(ctFont, glyphCount, glyphToUnicode);
        return;
    }

    AutoCFRelease<CFDataRef> bitmap(CFCharacterSetCreateBitmapRepresentation(kCFAllocatorDefault,
                                                                             charSet));
    if (!bitmap) {
        return;
    }
    CFIndex length = CFDataGetLength(bitmap);
    if (!length) {
        return;
    }
    if (length > 8192) {
        
        
        
        
        length = 8192;
    }
    const UInt8* bits = CFDataGetBytePtr(bitmap);
    glyphToUnicode->setCount(SkToInt(glyphCount));
    SkUnichar* out = glyphToUnicode->begin();
    sk_bzero(out, glyphCount * sizeof(SkUnichar));
    for (int i = 0; i < length; i++) {
        int mask = bits[i];
        if (!mask) {
            continue;
        }
        for (int j = 0; j < 8; j++) {
            CGGlyph glyph;
            UniChar unichar = static_cast<UniChar>((i << 3) + j);
            if (mask & (1 << j) && CTFontGetGlyphsForCharacters(ctFont, &unichar, &glyph, 1)) {
                out[glyph] = unichar;
            }
        }
    }
}

static bool getWidthAdvance(CTFontRef ctFont, int gId, int16_t* data) {
    CGSize advance;
    advance.width = 0;
    CGGlyph glyph = gId;
    CTFontGetAdvancesForGlyphs(ctFont, kCTFontHorizontalOrientation, &glyph, &advance, 1);
    *data = sk_float_round2int(advance.width);
    return true;
}


static void CFStringToSkString(CFStringRef src, SkString* dst) {
    
    
    CFIndex length = CFStringGetMaximumSizeForEncoding(CFStringGetLength(src),
                                                       kCFStringEncodingUTF8) + 1;
    dst->resize(length);
    CFStringGetCString(src, dst->writable_str(), length, kCFStringEncodingUTF8);
    
    dst->resize(strlen(dst->c_str()));
}

SkAdvancedTypefaceMetrics* SkTypeface_Mac::onGetAdvancedTypefaceMetrics(
        SkAdvancedTypefaceMetrics::PerGlyphInfo perGlyphInfo,
        const uint32_t* glyphIDs,
        uint32_t glyphIDsCount) const {

    CTFontRef originalCTFont = fFontRef.get();
    AutoCFRelease<CTFontRef> ctFont(CTFontCreateCopyWithAttributes(
            originalCTFont, CTFontGetUnitsPerEm(originalCTFont), NULL, NULL));
    SkAdvancedTypefaceMetrics* info = new SkAdvancedTypefaceMetrics;

    {
        AutoCFRelease<CFStringRef> fontName(CTFontCopyPostScriptName(ctFont));
        CFStringToSkString(fontName, &info->fFontName);
    }

    info->fMultiMaster = false;
    CFIndex glyphCount = CTFontGetGlyphCount(ctFont);
    info->fLastGlyphID = SkToU16(glyphCount - 1);
    info->fEmSize = CTFontGetUnitsPerEm(ctFont);

    if (perGlyphInfo & SkAdvancedTypefaceMetrics::kToUnicode_PerGlyphInfo) {
        populate_glyph_to_unicode(ctFont, glyphCount, &info->fGlyphToUnicode);
    }

    info->fStyle = 0;

    
    
    
    
    if (!this->getTableSize('glyf') || !this->getTableSize('loca')) {
        info->fType = SkAdvancedTypefaceMetrics::kOther_Font;
        info->fItalicAngle = 0;
        info->fAscent = 0;
        info->fDescent = 0;
        info->fStemV = 0;
        info->fCapHeight = 0;
        info->fBBox = SkIRect::MakeEmpty();
        return info;
    }

    info->fType = SkAdvancedTypefaceMetrics::kTrueType_Font;
    CTFontSymbolicTraits symbolicTraits = CTFontGetSymbolicTraits(ctFont);
    if (symbolicTraits & kCTFontMonoSpaceTrait) {
        info->fStyle |= SkAdvancedTypefaceMetrics::kFixedPitch_Style;
    }
    if (symbolicTraits & kCTFontItalicTrait) {
        info->fStyle |= SkAdvancedTypefaceMetrics::kItalic_Style;
    }
    CTFontStylisticClass stylisticClass = symbolicTraits & kCTFontClassMaskTrait;
    if (stylisticClass >= kCTFontOldStyleSerifsClass && stylisticClass <= kCTFontSlabSerifsClass) {
        info->fStyle |= SkAdvancedTypefaceMetrics::kSerif_Style;
    } else if (stylisticClass & kCTFontScriptsClass) {
        info->fStyle |= SkAdvancedTypefaceMetrics::kScript_Style;
    }
    info->fItalicAngle = (int16_t) CTFontGetSlantAngle(ctFont);
    info->fAscent = (int16_t) CTFontGetAscent(ctFont);
    info->fDescent = (int16_t) CTFontGetDescent(ctFont);
    info->fCapHeight = (int16_t) CTFontGetCapHeight(ctFont);
    CGRect bbox = CTFontGetBoundingBox(ctFont);

    SkRect r;
    r.set( CGToScalar(CGRectGetMinX_inline(bbox)),   
           CGToScalar(CGRectGetMaxY_inline(bbox)),   
           CGToScalar(CGRectGetMaxX_inline(bbox)),   
           CGToScalar(CGRectGetMinY_inline(bbox)));  

    r.roundOut(&(info->fBBox));

    
    
    int16_t min_width = SHRT_MAX;
    info->fStemV = 0;
    static const UniChar stem_chars[] = {'i', 'I', '!', '1'};
    const size_t count = sizeof(stem_chars) / sizeof(stem_chars[0]);
    CGGlyph glyphs[count];
    CGRect boundingRects[count];
    if (CTFontGetGlyphsForCharacters(ctFont, stem_chars, glyphs, count)) {
        CTFontGetBoundingRectsForGlyphs(ctFont, kCTFontHorizontalOrientation,
                                        glyphs, boundingRects, count);
        for (size_t i = 0; i < count; i++) {
            int16_t width = (int16_t) boundingRects[i].size.width;
            if (width > 0 && width < min_width) {
                min_width = width;
                info->fStemV = min_width;
            }
        }
    }

    if (false) { 
        
        info->fType = SkAdvancedTypefaceMetrics::kNotEmbeddable_Font;
    } else if (perGlyphInfo & SkAdvancedTypefaceMetrics::kHAdvance_PerGlyphInfo) {
        if (info->fStyle & SkAdvancedTypefaceMetrics::kFixedPitch_Style) {
            skia_advanced_typeface_metrics_utils::appendRange(&info->fGlyphWidths, 0);
            info->fGlyphWidths->fAdvance.append(1, &min_width);
            skia_advanced_typeface_metrics_utils::finishRange(info->fGlyphWidths.get(), 0,
                        SkAdvancedTypefaceMetrics::WidthRange::kDefault);
        } else {
            info->fGlyphWidths.reset(
                skia_advanced_typeface_metrics_utils::getAdvanceData(ctFont.get(),
                               SkToInt(glyphCount),
                               glyphIDs,
                               glyphIDsCount,
                               &getWidthAdvance));
        }
    }
    return info;
}



static SK_SFNT_ULONG get_font_type_tag(const SkTypeface_Mac* typeface) {
    CTFontRef ctFont = typeface->fFontRef.get();
    AutoCFRelease<CFNumberRef> fontFormatRef(
            static_cast<CFNumberRef>(CTFontCopyAttribute(ctFont, kCTFontFormatAttribute)));
    if (!fontFormatRef) {
        return 0;
    }

    SInt32 fontFormatValue;
    if (!CFNumberGetValue(fontFormatRef, kCFNumberSInt32Type, &fontFormatValue)) {
        return 0;
    }

    switch (fontFormatValue) {
        case kCTFontFormatOpenTypePostScript:
            return SkSFNTHeader::fontType_OpenTypeCFF::TAG;
        case kCTFontFormatOpenTypeTrueType:
            return SkSFNTHeader::fontType_WindowsTrueType::TAG;
        case kCTFontFormatTrueType:
            return SkSFNTHeader::fontType_MacTrueType::TAG;
        case kCTFontFormatPostScript:
            return SkSFNTHeader::fontType_PostScript::TAG;
        case kCTFontFormatBitmap:
            return SkSFNTHeader::fontType_MacTrueType::TAG;
        case kCTFontFormatUnrecognized:
        default:
            
            
            
            return SkSFNTHeader::fontType_WindowsTrueType::TAG;
    }
}

SkStream* SkTypeface_Mac::onOpenStream(int* ttcIndex) const {
    SK_SFNT_ULONG fontType = get_font_type_tag(this);
    if (0 == fontType) {
        return NULL;
    }

    
    int numTables = this->countTables();
    SkTDArray<SkFontTableTag> tableTags;
    tableTags.setCount(numTables);
    this->getTableTags(tableTags.begin());

    
    SkTDArray<size_t> tableSizes;
    size_t totalSize = sizeof(SkSFNTHeader) + sizeof(SkSFNTHeader::TableDirectoryEntry) * numTables;
    for (int tableIndex = 0; tableIndex < numTables; ++tableIndex) {
        size_t tableSize = this->getTableSize(tableTags[tableIndex]);
        totalSize += (tableSize + 3) & ~3;
        *tableSizes.append() = tableSize;
    }

    
    SkMemoryStream* stream = new SkMemoryStream(totalSize);
    char* dataStart = (char*)stream->getMemoryBase();
    sk_bzero(dataStart, totalSize);
    char* dataPtr = dataStart;

    
    uint16_t entrySelector = 0;
    uint16_t searchRange = 1;
    while (searchRange < numTables >> 1) {
        entrySelector++;
        searchRange <<= 1;
    }
    searchRange <<= 4;
    uint16_t rangeShift = (numTables << 4) - searchRange;

    
    SkSFNTHeader* header = (SkSFNTHeader*)dataPtr;
    header->fontType = fontType;
    header->numTables = SkEndian_SwapBE16(numTables);
    header->searchRange = SkEndian_SwapBE16(searchRange);
    header->entrySelector = SkEndian_SwapBE16(entrySelector);
    header->rangeShift = SkEndian_SwapBE16(rangeShift);
    dataPtr += sizeof(SkSFNTHeader);

    
    SkSFNTHeader::TableDirectoryEntry* entry = (SkSFNTHeader::TableDirectoryEntry*)dataPtr;
    dataPtr += sizeof(SkSFNTHeader::TableDirectoryEntry) * numTables;
    for (int tableIndex = 0; tableIndex < numTables; ++tableIndex) {
        size_t tableSize = tableSizes[tableIndex];
        this->getTableData(tableTags[tableIndex], 0, tableSize, dataPtr);
        entry->tag = SkEndian_SwapBE32(tableTags[tableIndex]);
        entry->checksum = SkEndian_SwapBE32(SkOTUtils::CalcTableChecksum((SK_OT_ULONG*)dataPtr,
                                                                         tableSize));
        entry->offset = SkEndian_SwapBE32(SkToU32(dataPtr - dataStart));
        entry->logicalLength = SkEndian_SwapBE32(SkToU32(tableSize));

        dataPtr += (tableSize + 3) & ~3;
        ++entry;
    }

    return stream;
}




int SkTypeface_Mac::onGetUPEM() const {
    AutoCFRelease<CGFontRef> cgFont(CTFontCopyGraphicsFont(fFontRef, NULL));
    return CGFontGetUnitsPerEm(cgFont);
}

SkTypeface::LocalizedStrings* SkTypeface_Mac::onCreateFamilyNameIterator() const {
    SkTypeface::LocalizedStrings* nameIter =
        SkOTUtils::LocalizedStrings_NameTable::CreateForFamilyNames(*this);
    if (NULL == nameIter) {
        AutoCFRelease<CFStringRef> cfLanguage;
        AutoCFRelease<CFStringRef> cfFamilyName(
            CTFontCopyLocalizedName(fFontRef, kCTFontFamilyNameKey, &cfLanguage));

        SkString skLanguage;
        SkString skFamilyName;
        if (cfLanguage.get()) {
            CFStringToSkString(cfLanguage.get(), &skLanguage);
        } else {
            skLanguage = "und"; 
        }
        if (cfFamilyName.get()) {
            CFStringToSkString(cfFamilyName.get(), &skFamilyName);
        }

        nameIter = new SkOTUtils::LocalizedStrings_SingleName(skFamilyName, skLanguage);
    }
    return nameIter;
}




static CFDataRef copyTableFromFont(CTFontRef ctFont, SkFontTableTag tag) {
    CFDataRef data = CTFontCopyTable(ctFont, (CTFontTableTag) tag,
                                     kCTFontTableOptionNoOptions);
    if (NULL == data) {
        AutoCFRelease<CGFontRef> cgFont(CTFontCopyGraphicsFont(ctFont, NULL));
        data = CGFontCopyTableForTag(cgFont, tag);
    }
    return data;
}

int SkTypeface_Mac::onGetTableTags(SkFontTableTag tags[]) const {
    AutoCFRelease<CFArrayRef> cfArray(CTFontCopyAvailableTables(fFontRef,
                                                kCTFontTableOptionNoOptions));
    if (NULL == cfArray) {
        return 0;
    }
    int count = SkToInt(CFArrayGetCount(cfArray));
    if (tags) {
        for (int i = 0; i < count; ++i) {
            uintptr_t fontTag = reinterpret_cast<uintptr_t>(CFArrayGetValueAtIndex(cfArray, i));
            tags[i] = static_cast<SkFontTableTag>(fontTag);
        }
    }
    return count;
}

size_t SkTypeface_Mac::onGetTableData(SkFontTableTag tag, size_t offset,
                                      size_t length, void* dstData) const {
    AutoCFRelease<CFDataRef> srcData(copyTableFromFont(fFontRef, tag));
    if (NULL == srcData) {
        return 0;
    }

    size_t srcSize = CFDataGetLength(srcData);
    if (offset >= srcSize) {
        return 0;
    }
    if (length > srcSize - offset) {
        length = srcSize - offset;
    }
    if (dstData) {
        memcpy(dstData, CFDataGetBytePtr(srcData) + offset, length);
    }
    return length;
}

SkScalerContext* SkTypeface_Mac::onCreateScalerContext(const SkDescriptor* desc) const {
    return new SkScalerContext_Mac(const_cast<SkTypeface_Mac*>(this), desc);
}

void SkTypeface_Mac::onFilterRec(SkScalerContextRec* rec) const {
    if (rec->fFlags & SkScalerContext::kLCD_BGROrder_Flag ||
        rec->fFlags & SkScalerContext::kLCD_Vertical_Flag)
    {
        rec->fMaskFormat = SkMask::kA8_Format;
        
        
        
        
        rec->setHinting(SkPaint::kNormal_Hinting);
    }

    unsigned flagsWeDontSupport = SkScalerContext::kDevKernText_Flag  |
                                  SkScalerContext::kForceAutohinting_Flag  |
                                  SkScalerContext::kLCD_BGROrder_Flag |
                                  SkScalerContext::kLCD_Vertical_Flag;

    rec->fFlags &= ~flagsWeDontSupport;

    bool lcdSupport = supports_LCD();

    
    
    
    
    SkPaint::Hinting hinting = rec->getHinting();
    if (SkPaint::kSlight_Hinting == hinting || !lcdSupport) {
        hinting = SkPaint::kNo_Hinting;
    } else if (SkPaint::kFull_Hinting == hinting) {
        hinting = SkPaint::kNormal_Hinting;
    }
    rec->setHinting(hinting);

    
    
    
    

    
    
    
    
    

    
    
    
    
    
    
    

    if (isLCDFormat(rec->fMaskFormat)) {
        if (lcdSupport) {
            
            rec->fMaskFormat = SkMask::kLCD16_Format;
            rec->setHinting(SkPaint::kNormal_Hinting);
        } else {
            rec->fMaskFormat = SkMask::kA8_Format;
        }
    }

    
    
    if (SkMask::kA8_Format == rec->fMaskFormat && SkPaint::kNo_Hinting == hinting) {
#ifndef SK_GAMMA_APPLY_TO_A8
        rec->ignorePreBlend();
#endif
    } else {
        
        rec->setContrast(0);
    }
}


static const char* get_str(CFStringRef ref, SkString* str) {
    CFStringToSkString(ref, str);
    CFSafeRelease(ref);
    return str->c_str();
}

void SkTypeface_Mac::onGetFontDescriptor(SkFontDescriptor* desc,
                                         bool* isLocalStream) const {
    SkString tmpStr;

    desc->setFamilyName(get_str(CTFontCopyFamilyName(fFontRef), &tmpStr));
    desc->setFullName(get_str(CTFontCopyFullName(fFontRef), &tmpStr));
    desc->setPostscriptName(get_str(CTFontCopyPostScriptName(fFontRef), &tmpStr));
    
    *isLocalStream = false;
}

int SkTypeface_Mac::onCharsToGlyphs(const void* chars, Encoding encoding,
                                    uint16_t glyphs[], int glyphCount) const
{
    
    
    

    SkAutoSTMalloc<1024, UniChar> charStorage;
    const UniChar* src; 
    int srcCount;
    switch (encoding) {
        case kUTF8_Encoding: {
            const char* utf8 = reinterpret_cast<const char*>(chars);
            UniChar* utf16 = charStorage.reset(2 * glyphCount);
            src = utf16;
            for (int i = 0; i < glyphCount; ++i) {
                SkUnichar uni = SkUTF8_NextUnichar(&utf8);
                utf16 += SkUTF16_FromUnichar(uni, utf16);
            }
            srcCount = SkToInt(utf16 - src);
            break;
        }
        case kUTF16_Encoding: {
            src = reinterpret_cast<const UniChar*>(chars);
            int extra = 0;
            for (int i = 0; i < glyphCount; ++i) {
                if (SkUTF16_IsHighSurrogate(src[i + extra])) {
                    ++extra;
                }
            }
            srcCount = glyphCount + extra;
            break;
        }
        case kUTF32_Encoding: {
            const SkUnichar* utf32 = reinterpret_cast<const SkUnichar*>(chars);
            UniChar* utf16 = charStorage.reset(2 * glyphCount);
            src = utf16;
            for (int i = 0; i < glyphCount; ++i) {
                utf16 += SkUTF16_FromUnichar(utf32[i], utf16);
            }
            srcCount = SkToInt(utf16 - src);
            break;
        }
    }

    
    
    SkAutoSTMalloc<1024, uint16_t> glyphStorage;
    uint16_t* macGlyphs = glyphs;
    if (NULL == macGlyphs || srcCount > glyphCount) {
        macGlyphs = glyphStorage.reset(srcCount);
    }

    bool allEncoded = CTFontGetGlyphsForCharacters(fFontRef, src, macGlyphs, srcCount);

    
    
    
    
    uint16_t* compactedGlyphs = glyphs;
    if (NULL == compactedGlyphs) {
        compactedGlyphs = macGlyphs;
    }
    if (srcCount > glyphCount) {
        int extra = 0;
        for (int i = 0; i < glyphCount; ++i) {
            if (SkUTF16_IsHighSurrogate(src[i + extra])) {
                ++extra;
            }
            compactedGlyphs[i] = macGlyphs[i + extra];
        }
    }

    if (allEncoded) {
        return glyphCount;
    }

    
    for (int i = 0; i < glyphCount; ++i) {
        if (0 == compactedGlyphs[i]) {
            return i;
        }
    }
    
    return glyphCount;
}

int SkTypeface_Mac::onCountGlyphs() const {
    return SkToInt(CTFontGetGlyphCount(fFontRef));
}



#if 1

static bool find_desc_str(CTFontDescriptorRef desc, CFStringRef name, SkString* value) {
    AutoCFRelease<CFStringRef> ref((CFStringRef)CTFontDescriptorCopyAttribute(desc, name));
    if (NULL == ref.get()) {
        return false;
    }
    CFStringToSkString(ref, value);
    return true;
}

static bool find_dict_float(CFDictionaryRef dict, CFStringRef name, float* value) {
    CFNumberRef num;
    return CFDictionaryGetValueIfPresent(dict, name, (const void**)&num)
    && CFNumberIsFloatType(num)
    && CFNumberGetValue(num, kCFNumberFloatType, value);
}

#include "SkFontMgr.h"

static int unit_weight_to_fontstyle(float unit) {
    float value;
    if (unit < 0) {
        value = 100 + (1 + unit) * 300;
    } else {
        value = 400 + unit * 500;
    }
    return sk_float_round2int(value);
}

static int unit_width_to_fontstyle(float unit) {
    float value;
    if (unit < 0) {
        value = 1 + (1 + unit) * 4;
    } else {
        value = 5 + unit * 4;
    }
    return sk_float_round2int(value);
}

static inline int sqr(int value) {
    SkASSERT(SkAbs32(value) < 0x7FFF);  
    return value * value;
}


static int compute_metric(const SkFontStyle& a, const SkFontStyle& b) {
    return sqr(a.weight() - b.weight()) +
           sqr((a.width() - b.width()) * 100) +
           sqr((a.isItalic() != b.isItalic()) * 900);
}

static SkFontStyle desc2fontstyle(CTFontDescriptorRef desc) {
    AutoCFRelease<CFDictionaryRef> dict(
        (CFDictionaryRef)CTFontDescriptorCopyAttribute(desc,
                                                       kCTFontTraitsAttribute));
    if (NULL == dict.get()) {
        return SkFontStyle();
    }

    float weight, width, slant;
    if (!find_dict_float(dict, kCTFontWeightTrait, &weight)) {
        weight = 0;
    }
    if (!find_dict_float(dict, kCTFontWidthTrait, &width)) {
        width = 0;
    }
    if (!find_dict_float(dict, kCTFontSlantTrait, &slant)) {
        slant = 0;
    }

    return SkFontStyle(unit_weight_to_fontstyle(weight),
                       unit_width_to_fontstyle(width),
                       slant ? SkFontStyle::kItalic_Slant
                       : SkFontStyle::kUpright_Slant);
}

struct NameFontStyleRec {
    SkString    fFamilyName;
    SkFontStyle fFontStyle;
};

static bool nameFontStyleProc(SkTypeface* face, SkTypeface::Style,
                              void* ctx) {
    SkTypeface_Mac* macFace = (SkTypeface_Mac*)face;
    const NameFontStyleRec* rec = (const NameFontStyleRec*)ctx;

    return macFace->fFontStyle == rec->fFontStyle &&
           macFace->fName == rec->fFamilyName;
}

static SkTypeface* createFromDesc(CFStringRef cfFamilyName,
                                  CTFontDescriptorRef desc) {
    NameFontStyleRec rec;
    CFStringToSkString(cfFamilyName, &rec.fFamilyName);
    rec.fFontStyle = desc2fontstyle(desc);

    SkTypeface* face = SkTypefaceCache::FindByProcAndRef(nameFontStyleProc,
                                                         &rec);
    if (face) {
        return face;
    }

    AutoCFRelease<CFDictionaryRef> fontFamilyNameDictionary(
        CFDictionaryCreate(kCFAllocatorDefault,
                           (const void**)&kCTFontFamilyNameAttribute, (const void**)&cfFamilyName,
                           1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
    AutoCFRelease<CTFontDescriptorRef> fontDescriptor(
        CTFontDescriptorCreateWithAttributes(fontFamilyNameDictionary));
    AutoCFRelease<CTFontRef> ctNamed(CTFontCreateWithFontDescriptor(fontDescriptor, 0, NULL));
    CTFontRef ctFont = CTFontCreateCopyWithAttributes(ctNamed, 1, NULL, desc);
    if (NULL == ctFont) {
        return NULL;
    }

    SkString str;
    CFStringToSkString(cfFamilyName, &str);

    bool isFixedPitch;
    (void)computeStyleBits(ctFont, &isFixedPitch);
    SkFontID fontID = CTFontRef_to_SkFontID(ctFont);

    face = SkNEW_ARGS(SkTypeface_Mac, (rec.fFontStyle, fontID, isFixedPitch,
                                       ctFont, str.c_str()));
    SkTypefaceCache::Add(face, face->style());
    return face;
}

class SkFontStyleSet_Mac : public SkFontStyleSet {
public:
    SkFontStyleSet_Mac(CFStringRef familyName, CTFontDescriptorRef desc)
        : fArray(CTFontDescriptorCreateMatchingFontDescriptors(desc, NULL))
        , fFamilyName(familyName)
        , fCount(0) {
        CFRetain(familyName);
        if (NULL == fArray) {
            fArray = CFArrayCreate(NULL, NULL, 0, NULL);
        }
        fCount = SkToInt(CFArrayGetCount(fArray));
    }

    virtual ~SkFontStyleSet_Mac() {
        CFRelease(fArray);
        CFRelease(fFamilyName);
    }

    virtual int count() SK_OVERRIDE {
        return fCount;
    }

    virtual void getStyle(int index, SkFontStyle* style,
                          SkString* name) SK_OVERRIDE {
        SkASSERT((unsigned)index < (unsigned)fCount);
        CTFontDescriptorRef desc = (CTFontDescriptorRef)CFArrayGetValueAtIndex(fArray, index);
        if (style) {
            *style = desc2fontstyle(desc);
        }
        if (name) {
            if (!find_desc_str(desc, kCTFontStyleNameAttribute, name)) {
                name->reset();
            }
        }
    }

    virtual SkTypeface* createTypeface(int index) SK_OVERRIDE {
        SkASSERT((unsigned)index < (unsigned)CFArrayGetCount(fArray));
        CTFontDescriptorRef desc = (CTFontDescriptorRef)CFArrayGetValueAtIndex(fArray, index);

        return createFromDesc(fFamilyName, desc);
    }

    virtual SkTypeface* matchStyle(const SkFontStyle& pattern) SK_OVERRIDE {
        if (0 == fCount) {
            return NULL;
        }
        return createFromDesc(fFamilyName, findMatchingDesc(pattern));
    }

private:
    CFArrayRef  fArray;
    CFStringRef fFamilyName;
    int         fCount;

    CTFontDescriptorRef findMatchingDesc(const SkFontStyle& pattern) const {
        int bestMetric = SK_MaxS32;
        CTFontDescriptorRef bestDesc = NULL;

        for (int i = 0; i < fCount; ++i) {
            CTFontDescriptorRef desc = (CTFontDescriptorRef)CFArrayGetValueAtIndex(fArray, i);
            int metric = compute_metric(pattern, desc2fontstyle(desc));
            if (0 == metric) {
                return desc;
            }
            if (metric < bestMetric) {
                bestMetric = metric;
                bestDesc = desc;
            }
        }
        SkASSERT(bestDesc);
        return bestDesc;
    }
};

class SkFontMgr_Mac : public SkFontMgr {
    CFArrayRef  fNames;
    int         fCount;

    CFStringRef stringAt(int index) const {
        SkASSERT((unsigned)index < (unsigned)fCount);
        return (CFStringRef)CFArrayGetValueAtIndex(fNames, index);
    }

    static SkFontStyleSet* CreateSet(CFStringRef cfFamilyName) {
        AutoCFRelease<CFMutableDictionaryRef> cfAttr(
                 CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
                                           &kCFTypeDictionaryKeyCallBacks,
                                           &kCFTypeDictionaryValueCallBacks));

        CFDictionaryAddValue(cfAttr, kCTFontFamilyNameAttribute, cfFamilyName);

        AutoCFRelease<CTFontDescriptorRef> desc(
                                CTFontDescriptorCreateWithAttributes(cfAttr));
        return SkNEW_ARGS(SkFontStyleSet_Mac, (cfFamilyName, desc));
    }

public:
    SkFontMgr_Mac()
        : fNames(SkCTFontManagerCopyAvailableFontFamilyNames())
        , fCount(fNames ? SkToInt(CFArrayGetCount(fNames)) : 0) {}

    virtual ~SkFontMgr_Mac() {
        CFSafeRelease(fNames);
    }

protected:
    virtual int onCountFamilies() const SK_OVERRIDE {
        return fCount;
    }

    virtual void onGetFamilyName(int index, SkString* familyName) const SK_OVERRIDE {
        if ((unsigned)index < (unsigned)fCount) {
            CFStringToSkString(this->stringAt(index), familyName);
        } else {
            familyName->reset();
        }
    }

    virtual SkFontStyleSet* onCreateStyleSet(int index) const SK_OVERRIDE {
        if ((unsigned)index >= (unsigned)fCount) {
            return NULL;
        }
        return CreateSet(this->stringAt(index));
    }

    virtual SkFontStyleSet* onMatchFamily(const char familyName[]) const SK_OVERRIDE {
        AutoCFRelease<CFStringRef> cfName(make_CFString(familyName));
        return CreateSet(cfName);
    }

    virtual SkTypeface* onMatchFamilyStyle(const char familyName[],
                                           const SkFontStyle&) const SK_OVERRIDE {
        return NULL;
    }

    virtual SkTypeface* onMatchFaceStyle(const SkTypeface* familyMember,
                                         const SkFontStyle&) const SK_OVERRIDE {
        return NULL;
    }

    virtual SkTypeface* onCreateFromData(SkData* data,
                                         int ttcIndex) const SK_OVERRIDE {
        AutoCFRelease<CGDataProviderRef> pr(SkCreateDataProviderFromData(data));
        if (NULL == pr) {
            return NULL;
        }
        return create_from_dataProvider(pr);
    }

    virtual SkTypeface* onCreateFromStream(SkStream* stream,
                                           int ttcIndex) const SK_OVERRIDE {
        AutoCFRelease<CGDataProviderRef> pr(SkCreateDataProviderFromStream(stream));
        if (NULL == pr) {
            return NULL;
        }
        return create_from_dataProvider(pr);
    }

    virtual SkTypeface* onCreateFromFile(const char path[],
                                         int ttcIndex) const SK_OVERRIDE {
        AutoCFRelease<CGDataProviderRef> pr(CGDataProviderCreateWithFilename(path));
        if (NULL == pr) {
            return NULL;
        }
        return create_from_dataProvider(pr);
    }

    virtual SkTypeface* onLegacyCreateTypeface(const char familyName[],
                                               unsigned styleBits) const SK_OVERRIDE {
        return create_typeface(NULL, familyName, (SkTypeface::Style)styleBits);
    }
};



SkFontMgr* SkFontMgr::Factory() {
    return SkNEW(SkFontMgr_Mac);
}
#endif

SkTypeface* SkFontHost::CreateTypeface(const SkTypeface* familyFace,
                                       const char famillyName[],
                                       SkTypeface::Style style)
{
    SkDEBUGFAIL("SkFontHost::FindTypeface unimplemented");
    return NULL;
}

SkTypeface* SkFontHost::CreateTypefaceFromStream(SkStream*)
{
    SkDEBUGFAIL("SkFontHost::CreateTypeface unimplemented");
    return NULL;
}

SkTypeface* SkFontHost::CreateTypefaceFromFile(char const*)
{
    SkDEBUGFAIL("SkFontHost::CreateTypefaceFromFile unimplemented");
    return NULL;
}

