







#include "SkAdvancedTypefaceMetrics.h"
#include "SkBase64.h"
#include "SkColorPriv.h"
#include "SkData.h"
#include "SkDescriptor.h"
#include "SkFontDescriptor.h"
#include "SkFontHost.h"
#include "SkGlyph.h"
#include "SkHRESULT.h"
#include "SkMaskGamma.h"
#include "SkMatrix22.h"
#include "SkOTTable_maxp.h"
#include "SkOTTable_name.h"
#include "SkOTUtils.h"
#include "SkPath.h"
#include "SkSFNTHeader.h"
#include "SkStream.h"
#include "SkString.h"
#include "SkTemplates.h"
#include "SkThread.h"
#include "SkTypeface_win.h"
#include "SkTypefaceCache.h"
#include "SkUtils.h"

#include "SkTypes.h"
#include <tchar.h>
#include <usp10.h>
#include <objbase.h>

static void (*gEnsureLOGFONTAccessibleProc)(const LOGFONT&);

void SkTypeface_SetEnsureLOGFONTAccessibleProc(void (*proc)(const LOGFONT&)) {
    gEnsureLOGFONTAccessibleProc = proc;
}

static void call_ensure_accessible(const LOGFONT& lf) {
    if (gEnsureLOGFONTAccessibleProc) {
        gEnsureLOGFONTAccessibleProc(lf);
    }
}




typedef uint32_t SkGdiRGB;






static bool isLCD(const SkScalerContext::Rec& rec) {
    return SkMask::kLCD16_Format == rec.fMaskFormat ||
           SkMask::kLCD32_Format == rec.fMaskFormat;
}

static bool bothZero(SkScalar a, SkScalar b) {
    return 0 == a && 0 == b;
}


static bool isAxisAligned(const SkScalerContext::Rec& rec) {
    return 0 == rec.fPreSkewX &&
           (bothZero(rec.fPost2x2[0][1], rec.fPost2x2[1][0]) ||
            bothZero(rec.fPost2x2[0][0], rec.fPost2x2[1][1]));
}

static bool needToRenderWithSkia(const SkScalerContext::Rec& rec) {
#ifdef SK_ENFORCE_ROTATED_TEXT_AA_ON_WINDOWS
    
    
    
    
    
    if (SkMask::kA8_Format == rec.fMaskFormat && !isAxisAligned(rec)) {
        return true;
    }
#endif
    return rec.getHinting() == SkPaint::kNo_Hinting || rec.getHinting() == SkPaint::kSlight_Hinting;
}

using namespace skia_advanced_typeface_metrics_utils;

static void tchar_to_skstring(const TCHAR t[], SkString* s) {
#ifdef UNICODE
    size_t sSize = WideCharToMultiByte(CP_UTF8, 0, t, -1, NULL, 0, NULL, NULL);
    s->resize(sSize);
    WideCharToMultiByte(CP_UTF8, 0, t, -1, s->writable_str(), sSize, NULL, NULL);
#else
    s->set(t);
#endif
}

static void dcfontname_to_skstring(HDC deviceContext, const LOGFONT& lf, SkString* familyName) {
    int fontNameLen; 
    if (0 == (fontNameLen = GetTextFace(deviceContext, 0, NULL))) {
        call_ensure_accessible(lf);
        if (0 == (fontNameLen = GetTextFace(deviceContext, 0, NULL))) {
            fontNameLen = 0;
        }
    }

    SkAutoSTArray<LF_FULLFACESIZE, TCHAR> fontName(fontNameLen+1);
    if (0 == GetTextFace(deviceContext, fontNameLen, fontName.get())) {
        call_ensure_accessible(lf);
        if (0 == GetTextFace(deviceContext, fontNameLen, fontName.get())) {
            fontName[0] = 0;
        }
    }

    tchar_to_skstring(fontName.get(), familyName);
}

static void make_canonical(LOGFONT* lf) {
    lf->lfHeight = -64;
    lf->lfQuality = CLEARTYPE_QUALITY;
    lf->lfCharSet = DEFAULT_CHARSET;

}

static SkTypeface::Style get_style(const LOGFONT& lf) {
    unsigned style = 0;
    if (lf.lfWeight >= FW_BOLD) {
        style |= SkTypeface::kBold;
    }
    if (lf.lfItalic) {
        style |= SkTypeface::kItalic;
    }
    return static_cast<SkTypeface::Style>(style);
}

static void setStyle(LOGFONT* lf, SkTypeface::Style style) {
    lf->lfWeight = (style & SkTypeface::kBold) != 0 ? FW_BOLD : FW_NORMAL ;
    lf->lfItalic = ((style & SkTypeface::kItalic) != 0);
}

static inline FIXED SkFixedToFIXED(SkFixed x) {
    return *(FIXED*)(&x);
}
static inline SkFixed SkFIXEDToFixed(FIXED x) {
    return *(SkFixed*)(&x);
}

static inline FIXED SkScalarToFIXED(SkScalar x) {
    return SkFixedToFIXED(SkScalarToFixed(x));
}

static inline SkScalar SkFIXEDToScalar(FIXED x) {
    return SkFixedToScalar(SkFIXEDToFixed(x));
}

static unsigned calculateGlyphCount(HDC hdc, const LOGFONT& lf) {
    TEXTMETRIC textMetric;
    if (0 == GetTextMetrics(hdc, &textMetric)) {
        textMetric.tmPitchAndFamily = TMPF_VECTOR;
        call_ensure_accessible(lf);
        GetTextMetrics(hdc, &textMetric);
    }

    if (!(textMetric.tmPitchAndFamily & TMPF_VECTOR)) {
        return textMetric.tmLastChar;
    }

    
    uint16_t glyphs;
    if (GDI_ERROR != GetFontData(hdc, SkOTTableMaximumProfile::TAG, 4, &glyphs, sizeof(glyphs))) {
        return SkEndian_SwapBE16(glyphs);
    }

    
    static const MAT2 mat2 = {{0, 1}, {0, 0}, {0, 0}, {0, 1}};
    int32_t max = SK_MaxU16 + 1;
    int32_t min = 0;
    GLYPHMETRICS gm;
    while (min < max) {
        int32_t mid = min + ((max - min) / 2);
        if (GetGlyphOutlineW(hdc, mid, GGO_METRICS | GGO_GLYPH_INDEX, &gm, 0,
                             NULL, &mat2) == GDI_ERROR) {
            max = mid;
        } else {
            min = mid + 1;
        }
    }
    SkASSERT(min == max);
    return min;
}

static unsigned calculateUPEM(HDC hdc, const LOGFONT& lf) {
    TEXTMETRIC textMetric;
    if (0 == GetTextMetrics(hdc, &textMetric)) {
        textMetric.tmPitchAndFamily = TMPF_VECTOR;
        call_ensure_accessible(lf);
        GetTextMetrics(hdc, &textMetric);
    }

    if (!(textMetric.tmPitchAndFamily & TMPF_VECTOR)) {
        return textMetric.tmMaxCharWidth;
    }

    OUTLINETEXTMETRIC otm;
    unsigned int otmRet = GetOutlineTextMetrics(hdc, sizeof(otm), &otm);
    if (0 == otmRet) {
        call_ensure_accessible(lf);
        otmRet = GetOutlineTextMetrics(hdc, sizeof(otm), &otm);
    }

    return (0 == otmRet) ? 0 : otm.otmEMSquare;
}

class LogFontTypeface : public SkTypeface {
public:
    LogFontTypeface(SkTypeface::Style style, SkFontID fontID, const LOGFONT& lf, bool serializeAsStream = false) :
        SkTypeface(style, fontID, false), fLogFont(lf), fSerializeAsStream(serializeAsStream) {

        
        HFONT font = CreateFontIndirect(&lf);

        HDC deviceContext = ::CreateCompatibleDC(NULL);
        HFONT savefont = (HFONT)SelectObject(deviceContext, font);

        TEXTMETRIC textMetric;
        if (0 == GetTextMetrics(deviceContext, &textMetric)) {
            call_ensure_accessible(lf);
            if (0 == GetTextMetrics(deviceContext, &textMetric)) {
                textMetric.tmPitchAndFamily = TMPF_TRUETYPE;
            }
        }
        if (deviceContext) {
            ::SelectObject(deviceContext, savefont);
            ::DeleteDC(deviceContext);
        }
        if (font) {
            ::DeleteObject(font);
        }

        
        this->setIsFixedPitch((textMetric.tmPitchAndFamily & TMPF_FIXED_PITCH) == 0);

        
        
        fCanBeLCD = !((textMetric.tmPitchAndFamily & TMPF_VECTOR) &&
                      (textMetric.tmPitchAndFamily & TMPF_DEVICE));
    }

    LOGFONT fLogFont;
    bool fSerializeAsStream;
    bool fCanBeLCD;

    static LogFontTypeface* Create(const LOGFONT& lf) {
        SkTypeface::Style style = get_style(lf);
        SkFontID fontID = SkTypefaceCache::NewFontID();
        return new LogFontTypeface(style, fontID, lf);
    }

    static void EnsureAccessible(const SkTypeface* face) {
        call_ensure_accessible(static_cast<const LogFontTypeface*>(face)->fLogFont);
    }

protected:
    virtual SkStream* onOpenStream(int* ttcIndex) const SK_OVERRIDE;
    virtual SkScalerContext* onCreateScalerContext(const SkDescriptor*) const SK_OVERRIDE;
    virtual void onFilterRec(SkScalerContextRec*) const SK_OVERRIDE;
    virtual SkAdvancedTypefaceMetrics* onGetAdvancedTypefaceMetrics(
                                SkAdvancedTypefaceMetrics::PerGlyphInfo,
                                const uint32_t*, uint32_t) const SK_OVERRIDE;
    virtual void onGetFontDescriptor(SkFontDescriptor*, bool*) const SK_OVERRIDE;
    virtual int onCharsToGlyphs(const void* chars, Encoding encoding,
                                uint16_t glyphs[], int glyphCount) const SK_OVERRIDE;
    virtual int onCountGlyphs() const SK_OVERRIDE;
    virtual int onGetUPEM() const SK_OVERRIDE;
    virtual SkTypeface::LocalizedStrings* onCreateFamilyNameIterator() const SK_OVERRIDE;
    virtual int onGetTableTags(SkFontTableTag tags[]) const SK_OVERRIDE;
    virtual size_t onGetTableData(SkFontTableTag, size_t offset,
                                  size_t length, void* data) const SK_OVERRIDE;
};

class FontMemResourceTypeface : public LogFontTypeface {
public:
    


    static FontMemResourceTypeface* Create(const LOGFONT& lf, HANDLE fontMemResource) {
        SkTypeface::Style style = get_style(lf);
        SkFontID fontID = SkTypefaceCache::NewFontID();
        return new FontMemResourceTypeface(style, fontID, lf, fontMemResource);
    }

protected:
    virtual void weak_dispose() const SK_OVERRIDE {
        RemoveFontMemResourceEx(fFontMemResource);
        
        INHERITED::weak_dispose();
    }

private:
    


    FontMemResourceTypeface(SkTypeface::Style style, SkFontID fontID, const LOGFONT& lf, HANDLE fontMemResource) :
        LogFontTypeface(style, fontID, lf, true), fFontMemResource(fontMemResource) {
    }

    HANDLE fFontMemResource;

    typedef LogFontTypeface INHERITED;
};

static const LOGFONT& get_default_font() {
    static LOGFONT gDefaultFont;
    return gDefaultFont;
}

static bool FindByLogFont(SkTypeface* face, SkTypeface::Style requestedStyle, void* ctx) {
    LogFontTypeface* lface = static_cast<LogFontTypeface*>(face);
    const LOGFONT* lf = reinterpret_cast<const LOGFONT*>(ctx);

    return lface &&
           get_style(lface->fLogFont) == requestedStyle &&
           !memcmp(&lface->fLogFont, lf, sizeof(LOGFONT));
}





SkTypeface* SkCreateTypefaceFromLOGFONT(const LOGFONT& origLF) {
    LOGFONT lf = origLF;
    make_canonical(&lf);
    SkTypeface* face = SkTypefaceCache::FindByProcAndRef(FindByLogFont, &lf);
    if (NULL == face) {
        face = LogFontTypeface::Create(lf);
        SkTypefaceCache::Add(face, get_style(lf));
    }
    return face;
}




SkTypeface* SkCreateFontMemResourceTypefaceFromLOGFONT(const LOGFONT& origLF, HANDLE fontMemResource) {
    LOGFONT lf = origLF;
    make_canonical(&lf);
    FontMemResourceTypeface* face = FontMemResourceTypeface::Create(lf, fontMemResource);
    SkTypefaceCache::Add(face, get_style(lf), false);
    return face;
}




void SkLOGFONTFromTypeface(const SkTypeface* face, LOGFONT* lf) {
    if (NULL == face) {
        *lf = get_default_font();
    } else {
        *lf = static_cast<const LogFontTypeface*>(face)->fLogFont;
    }
}







static void populate_glyph_to_unicode(HDC fontHdc, const unsigned glyphCount,
                                      SkTDArray<SkUnichar>* glyphToUnicode) {
    DWORD glyphSetBufferSize = GetFontUnicodeRanges(fontHdc, NULL);
    if (!glyphSetBufferSize) {
        return;
    }

    SkAutoTDeleteArray<BYTE> glyphSetBuffer(new BYTE[glyphSetBufferSize]);
    GLYPHSET* glyphSet =
        reinterpret_cast<LPGLYPHSET>(glyphSetBuffer.get());
    if (GetFontUnicodeRanges(fontHdc, glyphSet) != glyphSetBufferSize) {
        return;
    }

    glyphToUnicode->setCount(glyphCount);
    memset(glyphToUnicode->begin(), 0, glyphCount * sizeof(SkUnichar));
    for (DWORD i = 0; i < glyphSet->cRanges; ++i) {
        
        
        
        
        int count = glyphSet->ranges[i].cGlyphs;
        SkAutoTArray<WCHAR> chars(count + 1);
        chars[count] = 0;  
        SkAutoTArray<WORD> glyph(count);
        for (USHORT j = 0; j < count; ++j) {
            chars[j] = glyphSet->ranges[i].wcLow + j;
        }
        GetGlyphIndicesW(fontHdc, chars.get(), count, glyph.get(),
                         GGI_MARK_NONEXISTING_GLYPHS);
        
        
        
        
        
        
        
        for (USHORT j = 0; j < count; ++j) {
            if (glyph[j] != 0xffff && glyph[j] < glyphCount &&
                (*glyphToUnicode)[glyph[j]] == 0) {
                (*glyphToUnicode)[glyph[j]] = chars[j];
            }
        }
    }
}



static int alignTo32(int n) {
    return (n + 31) & ~31;
}

struct MyBitmapInfo : public BITMAPINFO {
    RGBQUAD fMoreSpaceForColors[1];
};

class HDCOffscreen {
public:
    HDCOffscreen() {
        fFont = 0;
        fDC = 0;
        fBM = 0;
        fBits = NULL;
        fWidth = fHeight = 0;
        fIsBW = false;
    }

    ~HDCOffscreen() {
        if (fDC) {
            DeleteDC(fDC);
        }
        if (fBM) {
            DeleteObject(fBM);
        }
    }

    void init(HFONT font, const XFORM& xform) {
        fFont = font;
        fXform = xform;
    }

    const void* draw(const SkGlyph&, bool isBW, size_t* srcRBPtr);

private:
    HDC     fDC;
    HBITMAP fBM;
    HFONT   fFont;
    XFORM   fXform;
    void*   fBits;  
    int     fWidth;
    int     fHeight;
    bool    fIsBW;
};

const void* HDCOffscreen::draw(const SkGlyph& glyph, bool isBW,
                               size_t* srcRBPtr) {
    
    
    if (0 == fDC) {
        fDC = CreateCompatibleDC(0);
        if (0 == fDC) {
            return NULL;
        }
        SetGraphicsMode(fDC, GM_ADVANCED);
        SetBkMode(fDC, TRANSPARENT);
        SetTextAlign(fDC, TA_LEFT | TA_BASELINE);
        SelectObject(fDC, fFont);

        COLORREF color = 0x00FFFFFF;
        SkDEBUGCODE(COLORREF prev =) SetTextColor(fDC, color);
        SkASSERT(prev != CLR_INVALID);
    }

    if (fBM && (fIsBW != isBW || fWidth < glyph.fWidth || fHeight < glyph.fHeight)) {
        DeleteObject(fBM);
        fBM = 0;
    }
    fIsBW = isBW;

    fWidth = SkMax32(fWidth, glyph.fWidth);
    fHeight = SkMax32(fHeight, glyph.fHeight);

    int biWidth = isBW ? alignTo32(fWidth) : fWidth;

    if (0 == fBM) {
        MyBitmapInfo info;
        sk_bzero(&info, sizeof(info));
        if (isBW) {
            RGBQUAD blackQuad = { 0, 0, 0, 0 };
            RGBQUAD whiteQuad = { 0xFF, 0xFF, 0xFF, 0 };
            info.bmiColors[0] = blackQuad;
            info.bmiColors[1] = whiteQuad;
        }
        info.bmiHeader.biSize = sizeof(info.bmiHeader);
        info.bmiHeader.biWidth = biWidth;
        info.bmiHeader.biHeight = fHeight;
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biBitCount = isBW ? 1 : 32;
        info.bmiHeader.biCompression = BI_RGB;
        if (isBW) {
            info.bmiHeader.biClrUsed = 2;
        }
        fBM = CreateDIBSection(fDC, &info, DIB_RGB_COLORS, &fBits, 0, 0);
        if (0 == fBM) {
            return NULL;
        }
        SelectObject(fDC, fBM);
    }

    
    size_t srcRB = isBW ? (biWidth >> 3) : (fWidth << 2);
    size_t size = fHeight * srcRB;
    memset(fBits, 0, size);

    XFORM xform = fXform;
    xform.eDx = (float)-glyph.fLeft;
    xform.eDy = (float)-glyph.fTop;
    SetWorldTransform(fDC, &xform);

    uint16_t glyphID = glyph.getGlyphID();
    BOOL ret = ExtTextOutW(fDC, 0, 0, ETO_GLYPH_INDEX, NULL, reinterpret_cast<LPCWSTR>(&glyphID), 1, NULL);
    GdiFlush();
    if (0 == ret) {
        return NULL;
    }
    *srcRBPtr = srcRB;
    
    return (const char*)fBits + (fHeight - glyph.fHeight) * srcRB;
}


#define BUFFERSIZE (1 << 13)

class SkScalerContext_GDI : public SkScalerContext {
public:
    SkScalerContext_GDI(SkTypeface*, const SkDescriptor* desc);
    virtual ~SkScalerContext_GDI();

    
    
    bool isValid() const;

protected:
    virtual unsigned generateGlyphCount() SK_OVERRIDE;
    virtual uint16_t generateCharToGlyph(SkUnichar uni) SK_OVERRIDE;
    virtual void generateAdvance(SkGlyph* glyph) SK_OVERRIDE;
    virtual void generateMetrics(SkGlyph* glyph) SK_OVERRIDE;
    virtual void generateImage(const SkGlyph& glyph) SK_OVERRIDE;
    virtual void generatePath(const SkGlyph& glyph, SkPath* path) SK_OVERRIDE;
    virtual void generateFontMetrics(SkPaint::FontMetrics*) SK_OVERRIDE;

private:
    DWORD getGDIGlyphPath(const SkGlyph& glyph, UINT flags,
                          SkAutoSTMalloc<BUFFERSIZE, uint8_t>* glyphbuf);

    HDCOffscreen fOffscreen;
    


    MAT2         fGsA;
    
    MAT2         fMat22;
    
    MAT2         fHighResMat22;
    HDC          fDDC;
    HFONT        fSavefont;
    HFONT        fFont;
    SCRIPT_CACHE fSC;
    int          fGlyphCount;

    
    SkMatrix     fHiResMatrix;
    


    SkMatrix     fG_inv;
    enum Type {
        kTrueType_Type, kBitmap_Type, kLine_Type
    } fType;
    TEXTMETRIC fTM;
};

static FIXED float2FIXED(float x) {
    return SkFixedToFIXED(SkFloatToFixed(x));
}

static BYTE compute_quality(const SkScalerContext::Rec& rec) {
    switch (rec.fMaskFormat) {
        case SkMask::kBW_Format:
            return NONANTIALIASED_QUALITY;
        case SkMask::kLCD16_Format:
        case SkMask::kLCD32_Format:
            return CLEARTYPE_QUALITY;
        default:
            if (rec.fFlags & SkScalerContext::kGenA8FromLCD_Flag) {
                return CLEARTYPE_QUALITY;
            } else {
                return ANTIALIASED_QUALITY;
            }
    }
}

SkScalerContext_GDI::SkScalerContext_GDI(SkTypeface* rawTypeface,
                                                 const SkDescriptor* desc)
        : SkScalerContext(rawTypeface, desc)
        , fDDC(0)
        , fSavefont(0)
        , fFont(0)
        , fSC(0)
        , fGlyphCount(-1)
{
    LogFontTypeface* typeface = reinterpret_cast<LogFontTypeface*>(rawTypeface);

    fDDC = ::CreateCompatibleDC(NULL);
    if (!fDDC) {
        return;
    }
    SetGraphicsMode(fDDC, GM_ADVANCED);
    SetBkMode(fDDC, TRANSPARENT);

    SkPoint h = SkPoint::Make(SK_Scalar1, 0);
    
    SkMatrix A;
    fRec.getSingleMatrix(&A);
    A.mapPoints(&h, 1);

    
    SkMatrix G;
    SkComputeGivensRotation(h, &G);

    
    SkMatrix GA(G);
    GA.preConcat(A);

    
    
    
    SkScalar realTextSize = SkScalarAbs(GA.get(SkMatrix::kMScaleY));
    SkScalar gdiTextSize = SkScalarRoundToScalar(realTextSize);
    if (gdiTextSize == 0) {
        gdiTextSize = SK_Scalar1;
    }

    
    
    SkScalar scale = (fRec.getHinting() == SkPaint::kNo_Hinting ||
                      fRec.getHinting() == SkPaint::kSlight_Hinting)
                   ? SkScalarInvert(gdiTextSize)
                   : SkScalarInvert(realTextSize);

    
    
    SkMatrix sA(A);
    sA.preScale(scale, scale); 

    
    
    SkMatrix GsA(GA);
    GsA.preScale(scale, scale); 

    fGsA.eM11 = SkScalarToFIXED(GsA.get(SkMatrix::kMScaleX));
    fGsA.eM12 = SkScalarToFIXED(-GsA.get(SkMatrix::kMSkewY)); 
    fGsA.eM21 = SkScalarToFIXED(-GsA.get(SkMatrix::kMSkewX));
    fGsA.eM22 = SkScalarToFIXED(GsA.get(SkMatrix::kMScaleY));

    
    fG_inv.setAll(G.get(SkMatrix::kMScaleX), -G.get(SkMatrix::kMSkewX), G.get(SkMatrix::kMTransX),
                  -G.get(SkMatrix::kMSkewY), G.get(SkMatrix::kMScaleY), G.get(SkMatrix::kMTransY),
                  G.get(SkMatrix::kMPersp0), G.get(SkMatrix::kMPersp1), G.get(SkMatrix::kMPersp2));

    LOGFONT lf = typeface->fLogFont;
    lf.lfHeight = -SkScalarTruncToInt(gdiTextSize);
    lf.lfQuality = compute_quality(fRec);
    fFont = CreateFontIndirect(&lf);
    if (!fFont) {
        return;
    }

    fSavefont = (HFONT)SelectObject(fDDC, fFont);

    if (0 == GetTextMetrics(fDDC, &fTM)) {
        call_ensure_accessible(lf);
        if (0 == GetTextMetrics(fDDC, &fTM)) {
            fTM.tmPitchAndFamily = TMPF_TRUETYPE;
        }
    }

    XFORM xform;
    if (fTM.tmPitchAndFamily & TMPF_VECTOR) {
        
        

        
        
        
        
        
        if (fTM.tmPitchAndFamily & (TMPF_TRUETYPE | TMPF_DEVICE)) {
            
            fType = SkScalerContext_GDI::kTrueType_Type;
        } else {
            
            fType = SkScalerContext_GDI::kLine_Type;
        }

        
        
        xform.eM11 = SkScalarToFloat(sA.get(SkMatrix::kMScaleX));
        xform.eM12 = SkScalarToFloat(sA.get(SkMatrix::kMSkewY));
        xform.eM21 = SkScalarToFloat(sA.get(SkMatrix::kMSkewX));
        xform.eM22 = SkScalarToFloat(sA.get(SkMatrix::kMScaleY));
        xform.eDx = 0;
        xform.eDy = 0;

        
        fMat22.eM11 = float2FIXED(xform.eM11);
        fMat22.eM12 = float2FIXED(-xform.eM12);
        fMat22.eM21 = float2FIXED(-xform.eM21);
        fMat22.eM22 = float2FIXED(xform.eM22);

        if (needToRenderWithSkia(fRec)) {
            this->forceGenerateImageFromPath();
        }

        
        if (this->isSubpixel()) {
            OUTLINETEXTMETRIC otm;
            UINT success = GetOutlineTextMetrics(fDDC, sizeof(otm), &otm);
            if (0 == success) {
                call_ensure_accessible(lf);
                success = GetOutlineTextMetrics(fDDC, sizeof(otm), &otm);
            }
            if (0 != success) {
                SkScalar upem = SkIntToScalar(otm.otmEMSquare);

                SkScalar gdiTextSizeToEMScale = upem / gdiTextSize;
                fHighResMat22.eM11 = float2FIXED(gdiTextSizeToEMScale);
                fHighResMat22.eM12 = float2FIXED(0);
                fHighResMat22.eM21 = float2FIXED(0);
                fHighResMat22.eM22 = float2FIXED(gdiTextSizeToEMScale);

                SkScalar removeEMScale = SkScalarInvert(upem);
                fHiResMatrix = A;
                fHiResMatrix.preScale(removeEMScale, removeEMScale);
            }
        }

    } else {
        
        fType = SkScalerContext_GDI::kBitmap_Type;

        xform.eM11 = 1.0f;
        xform.eM12 = 0.0f;
        xform.eM21 = 0.0f;
        xform.eM22 = 1.0f;
        xform.eDx = 0.0f;
        xform.eDy = 0.0f;

        
        
        fMat22.eM11 = SkScalarToFIXED(fRec.fPost2x2[0][0]);
        fMat22.eM12 = SkScalarToFIXED(-fRec.fPost2x2[1][0]);
        fMat22.eM21 = SkScalarToFIXED(-fRec.fPost2x2[0][1]);
        fMat22.eM22 = SkScalarToFIXED(fRec.fPost2x2[1][1]);
    }

    fOffscreen.init(fFont, xform);
}

SkScalerContext_GDI::~SkScalerContext_GDI() {
    if (fDDC) {
        ::SelectObject(fDDC, fSavefont);
        ::DeleteDC(fDDC);
    }
    if (fFont) {
        ::DeleteObject(fFont);
    }
    if (fSC) {
        ::ScriptFreeCache(&fSC);
    }
}

bool SkScalerContext_GDI::isValid() const {
    return fDDC && fFont;
}

unsigned SkScalerContext_GDI::generateGlyphCount() {
    if (fGlyphCount < 0) {
        fGlyphCount = calculateGlyphCount(
                          fDDC, static_cast<const LogFontTypeface*>(this->getTypeface())->fLogFont);
    }
    return fGlyphCount;
}

uint16_t SkScalerContext_GDI::generateCharToGlyph(SkUnichar utf32) {
    uint16_t index = 0;
    WCHAR utf16[2];
    
    if (SkUTF16_FromUnichar(utf32, (uint16_t*)utf16) == 1) {
        

        












        DWORD result = GetGlyphIndicesW(fDDC, utf16, 1, &index, GGI_MARK_NONEXISTING_GLYPHS);
        if (result == GDI_ERROR
            || 0xFFFF == index
            || (0x1F == index &&
               (fType == SkScalerContext_GDI::kBitmap_Type ||
                fType == SkScalerContext_GDI::kLine_Type)
               )
           )
        {
            index = 0;
        }
    } else {
        
        static const int numWCHAR = 2;
        static const int maxItems = 2;
        
        SCRIPT_CONTROL sc = { 0 };
        
        
        SCRIPT_ITEM si[maxItems + 1];
        int numItems;
        HRZM(ScriptItemize(utf16, numWCHAR, maxItems, &sc, NULL, si, &numItems),
             "Could not itemize character.");

        
        static const int maxGlyphs = 2;
        SCRIPT_VISATTR vsa[maxGlyphs];
        WORD outGlyphs[maxGlyphs];
        WORD logClust[numWCHAR];
        int numGlyphs;
        HRZM(ScriptShape(fDDC, &fSC, utf16, numWCHAR, maxGlyphs, &si[0].a,
                         outGlyphs, logClust, vsa, &numGlyphs),
             "Could not shape character.");
        if (1 == numGlyphs) {
            index = outGlyphs[0];
        }
    }
    return index;
}

void SkScalerContext_GDI::generateAdvance(SkGlyph* glyph) {
    this->generateMetrics(glyph);
}

void SkScalerContext_GDI::generateMetrics(SkGlyph* glyph) {
    SkASSERT(fDDC);

    if (fType == SkScalerContext_GDI::kBitmap_Type || fType == SkScalerContext_GDI::kLine_Type) {
        SIZE size;
        WORD glyphs = glyph->getGlyphID(0);
        if (0 == GetTextExtentPointI(fDDC, &glyphs, 1, &size)) {
            glyph->fWidth = SkToS16(fTM.tmMaxCharWidth);
        } else {
            glyph->fWidth = SkToS16(size.cx);
        }
        glyph->fHeight = SkToS16(size.cy);

        glyph->fTop = SkToS16(-fTM.tmAscent);
        
        
        glyph->fLeft = SkToS16(0);
        glyph->fAdvanceX = SkIntToFixed(glyph->fWidth);
        glyph->fAdvanceY = 0;

        
        if (fType == SkScalerContext_GDI::kLine_Type) {
            SkRect bounds = SkRect::MakeXYWH(glyph->fLeft, glyph->fTop,
                                             glyph->fWidth, glyph->fHeight);
            SkMatrix m;
            m.setAll(SkFIXEDToScalar(fMat22.eM11), -SkFIXEDToScalar(fMat22.eM21), 0,
                     -SkFIXEDToScalar(fMat22.eM12), SkFIXEDToScalar(fMat22.eM22), 0,
                     0,  0, SkScalarToPersp(SK_Scalar1));
            m.mapRect(&bounds);
            bounds.roundOut();
            glyph->fLeft = SkScalarTruncToInt(bounds.fLeft);
            glyph->fTop = SkScalarTruncToInt(bounds.fTop);
            glyph->fWidth = SkScalarTruncToInt(bounds.width());
            glyph->fHeight = SkScalarTruncToInt(bounds.height());
        }

        
        glyph->fAdvanceY = SkFixedMul(-SkFIXEDToFixed(fMat22.eM12), glyph->fAdvanceX);
        glyph->fAdvanceX = SkFixedMul(SkFIXEDToFixed(fMat22.eM11), glyph->fAdvanceX);

        return;
    }

    UINT glyphId = glyph->getGlyphID(0);

    GLYPHMETRICS gm;
    sk_bzero(&gm, sizeof(gm));

    DWORD status = GetGlyphOutlineW(fDDC, glyphId, GGO_METRICS | GGO_GLYPH_INDEX, &gm, 0, NULL, &fMat22);
    if (GDI_ERROR == status) {
        LogFontTypeface::EnsureAccessible(this->getTypeface());
        status = GetGlyphOutlineW(fDDC, glyphId, GGO_METRICS | GGO_GLYPH_INDEX, &gm, 0, NULL, &fMat22);
        if (GDI_ERROR == status) {
            glyph->zeroMetrics();
            return;
        }
    }

    bool empty = false;
    
    
    
    if (1 == gm.gmBlackBoxX && 1 == gm.gmBlackBoxY) {
        
        DWORD bufferSize = GetGlyphOutlineW(fDDC, glyphId, GGO_NATIVE | GGO_GLYPH_INDEX, &gm, 0, NULL, &fMat22);
        empty = (0 == bufferSize);
    }

    glyph->fTop = SkToS16(-gm.gmptGlyphOrigin.y);
    glyph->fLeft = SkToS16(gm.gmptGlyphOrigin.x);
    if (empty) {
        glyph->fWidth = 0;
        glyph->fHeight = 0;
    } else {
        
        
        
        
        glyph->fWidth = gm.gmBlackBoxX + 4;
        glyph->fHeight = gm.gmBlackBoxY + 4;
        glyph->fTop -= 2;
        glyph->fLeft -= 2;
    }
    glyph->fAdvanceX = SkIntToFixed(gm.gmCellIncX);
    glyph->fAdvanceY = SkIntToFixed(gm.gmCellIncY);
    glyph->fRsbDelta = 0;
    glyph->fLsbDelta = 0;

    if (this->isSubpixel()) {
        sk_bzero(&gm, sizeof(gm));
        status = GetGlyphOutlineW(fDDC, glyphId, GGO_METRICS | GGO_GLYPH_INDEX, &gm, 0, NULL, &fHighResMat22);
        if (GDI_ERROR != status) {
            SkPoint advance;
            fHiResMatrix.mapXY(SkIntToScalar(gm.gmCellIncX), SkIntToScalar(gm.gmCellIncY), &advance);
            glyph->fAdvanceX = SkScalarToFixed(advance.fX);
            glyph->fAdvanceY = SkScalarToFixed(advance.fY);
        }
    } else if (!isAxisAligned(this->fRec)) {
        status = GetGlyphOutlineW(fDDC, glyphId, GGO_METRICS | GGO_GLYPH_INDEX, &gm, 0, NULL, &fGsA);
        if (GDI_ERROR != status) {
            SkPoint advance;
            fG_inv.mapXY(SkIntToScalar(gm.gmCellIncX), SkIntToScalar(gm.gmCellIncY), &advance);
            glyph->fAdvanceX = SkScalarToFixed(advance.fX);
            glyph->fAdvanceY = SkScalarToFixed(advance.fY);
        }
    }
}

static const MAT2 gMat2Identity = {{0, 1}, {0, 0}, {0, 0}, {0, 1}};
void SkScalerContext_GDI::generateFontMetrics(SkPaint::FontMetrics* metrics) {
    if (NULL == metrics) {
        return;
    }
    sk_bzero(metrics, sizeof(*metrics));

    SkASSERT(fDDC);

#ifndef SK_GDI_ALWAYS_USE_TEXTMETRICS_FOR_FONT_METRICS
    if (fType == SkScalerContext_GDI::kBitmap_Type || fType == SkScalerContext_GDI::kLine_Type) {
#endif
        metrics->fTop = SkIntToScalar(-fTM.tmAscent);
        metrics->fAscent = SkIntToScalar(-fTM.tmAscent);
        metrics->fDescent = SkIntToScalar(fTM.tmDescent);
        metrics->fBottom = SkIntToScalar(fTM.tmDescent);
        metrics->fLeading = SkIntToScalar(fTM.tmExternalLeading);
        metrics->fAvgCharWidth = SkIntToScalar(fTM.tmAveCharWidth);
        metrics->fMaxCharWidth = SkIntToScalar(fTM.tmMaxCharWidth);
        metrics->fXMin = 0;
        metrics->fXMax = metrics->fMaxCharWidth;
        
#ifndef SK_GDI_ALWAYS_USE_TEXTMETRICS_FOR_FONT_METRICS
        return;
    }
#endif

    OUTLINETEXTMETRIC otm;

    uint32_t ret = GetOutlineTextMetrics(fDDC, sizeof(otm), &otm);
    if (0 == ret) {
        LogFontTypeface::EnsureAccessible(this->getTypeface());
        ret = GetOutlineTextMetrics(fDDC, sizeof(otm), &otm);
    }
    if (0 == ret) {
        return;
    }

#ifndef SK_GDI_ALWAYS_USE_TEXTMETRICS_FOR_FONT_METRICS
    metrics->fTop = SkIntToScalar(-otm.otmrcFontBox.top);
    metrics->fAscent = SkIntToScalar(-otm.otmAscent);
    metrics->fDescent = SkIntToScalar(-otm.otmDescent);
    metrics->fBottom = SkIntToScalar(-otm.otmrcFontBox.bottom);
    metrics->fLeading = SkIntToScalar(otm.otmLineGap);
    metrics->fAvgCharWidth = SkIntToScalar(otm.otmTextMetrics.tmAveCharWidth);
    metrics->fMaxCharWidth = SkIntToScalar(otm.otmTextMetrics.tmMaxCharWidth);
    metrics->fXMin = SkIntToScalar(otm.otmrcFontBox.left);
    metrics->fXMax = SkIntToScalar(otm.otmrcFontBox.right);
#endif
    metrics->fUnderlineThickness = SkIntToScalar(otm.otmsUnderscoreSize);
    metrics->fUnderlinePosition = -SkIntToScalar(otm.otmsUnderscorePosition);

    metrics->fFlags |= SkPaint::FontMetrics::kUnderlineThinknessIsValid_Flag;
    metrics->fFlags |= SkPaint::FontMetrics::kUnderlinePositionIsValid_Flag;

    metrics->fXHeight = SkIntToScalar(otm.otmsXHeight);
    GLYPHMETRICS gm;
    sk_bzero(&gm, sizeof(gm));
    DWORD len = GetGlyphOutlineW(fDDC, 'x', GGO_METRICS, &gm, 0, 0, &gMat2Identity);
    if (len != GDI_ERROR && gm.gmBlackBoxY > 0) {
        metrics->fXHeight = SkIntToScalar(gm.gmBlackBoxY);
    }
}



#define SK_SHOW_TEXT_BLIT_COVERAGE 0

static void build_power_table(uint8_t table[], float ee) {
    for (int i = 0; i < 256; i++) {
        float x = i / 255.f;
        x = sk_float_pow(x, ee);
        int xx = SkScalarRoundToInt(x * 255);
        table[i] = SkToU8(xx);
    }
}












static const uint8_t* getInverseGammaTableGDI() {
    
    

    
    
    
    
    static volatile bool gInited;
    static uint8_t gTableGdi[256];
    if (gInited) {
        
        
    } else {
        build_power_table(gTableGdi, 2.3f);
        
        
        gInited = true;
    }
    return gTableGdi;
}








static const uint8_t* getInverseGammaTableClearType() {
    
    

    
    
    
    
    static volatile bool gInited;
    static uint8_t gTableClearType[256];
    if (gInited) {
        
        
    } else {
        UINT level = 0;
        if (!SystemParametersInfo(SPI_GETFONTSMOOTHINGCONTRAST, 0, &level, 0) || !level) {
            
            level = 1400;
        }
        build_power_table(gTableClearType, level / 1000.0f);
        
        
        gInited = true;
    }
    return gTableClearType;
}

#include "SkColorPriv.h"


template<bool APPLY_PREBLEND>
static inline uint8_t rgb_to_a8(SkGdiRGB rgb, const uint8_t* table8) {
    U8CPU r = (rgb >> 16) & 0xFF;
    U8CPU g = (rgb >>  8) & 0xFF;
    U8CPU b = (rgb >>  0) & 0xFF;
    return sk_apply_lut_if<APPLY_PREBLEND>(SkComputeLuminance(r, g, b), table8);
}

template<bool APPLY_PREBLEND>
static inline uint16_t rgb_to_lcd16(SkGdiRGB rgb, const uint8_t* tableR,
                                                  const uint8_t* tableG,
                                                  const uint8_t* tableB) {
    U8CPU r = sk_apply_lut_if<APPLY_PREBLEND>((rgb >> 16) & 0xFF, tableR);
    U8CPU g = sk_apply_lut_if<APPLY_PREBLEND>((rgb >>  8) & 0xFF, tableG);
    U8CPU b = sk_apply_lut_if<APPLY_PREBLEND>((rgb >>  0) & 0xFF, tableB);
#if SK_SHOW_TEXT_BLIT_COVERAGE
    r = SkMax32(r, 10); g = SkMax32(g, 10); b = SkMax32(b, 10);
#endif
    return SkPack888ToRGB16(r, g, b);
}

template<bool APPLY_PREBLEND>
static inline SkPMColor rgb_to_lcd32(SkGdiRGB rgb, const uint8_t* tableR,
                                                   const uint8_t* tableG,
                                                   const uint8_t* tableB) {
    U8CPU r = sk_apply_lut_if<APPLY_PREBLEND>((rgb >> 16) & 0xFF, tableR);
    U8CPU g = sk_apply_lut_if<APPLY_PREBLEND>((rgb >>  8) & 0xFF, tableG);
    U8CPU b = sk_apply_lut_if<APPLY_PREBLEND>((rgb >>  0) & 0xFF, tableB);
#if SK_SHOW_TEXT_BLIT_COVERAGE
    r = SkMax32(r, 10); g = SkMax32(g, 10); b = SkMax32(b, 10);
#endif
    return SkPackARGB32(0xFF, r, g, b);
}






static int is_not_black_or_white(SkGdiRGB c) {
    
    
    
    return (c + (c & 1)) & 0x00FFFFFF;
}

static bool is_rgb_really_bw(const SkGdiRGB* src, int width, int height, size_t srcRB) {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (is_not_black_or_white(src[x])) {
                return false;
            }
        }
        src = SkTAddOffset<const SkGdiRGB>(src, srcRB);
    }
    return true;
}



static void rgb_to_bw(const SkGdiRGB* SK_RESTRICT src, size_t srcRB,
                      const SkGlyph& glyph) {
    const int width = glyph.fWidth;
    const size_t dstRB = (width + 7) >> 3;
    uint8_t* SK_RESTRICT dst = (uint8_t*)((char*)glyph.fImage + (glyph.fHeight - 1) * dstRB);

    int byteCount = width >> 3;
    int bitCount = width & 7;

    
    
    srcRB -= byteCount * 8 * sizeof(SkGdiRGB);

    for (int y = 0; y < glyph.fHeight; ++y) {
        if (byteCount > 0) {
            for (int i = 0; i < byteCount; ++i) {
                unsigned byte = 0;
                byte |= src[0] & (1 << 7);
                byte |= src[1] & (1 << 6);
                byte |= src[2] & (1 << 5);
                byte |= src[3] & (1 << 4);
                byte |= src[4] & (1 << 3);
                byte |= src[5] & (1 << 2);
                byte |= src[6] & (1 << 1);
                byte |= src[7] & (1 << 0);
                dst[i] = byte;
                src += 8;
            }
        }
        if (bitCount > 0) {
            unsigned byte = 0;
            unsigned mask = 0x80;
            for (int i = 0; i < bitCount; i++) {
                byte |= src[i] & mask;
                mask >>= 1;
            }
            dst[byteCount] = byte;
        }
        src = SkTAddOffset<const SkGdiRGB>(src, srcRB);
        dst -= dstRB;
    }
#if SK_SHOW_TEXT_BLIT_COVERAGE
    if (glyph.fWidth > 0 && glyph.fHeight > 0) {
        uint8_t* first = (uint8_t*)glyph.fImage;
        uint8_t* last = (uint8_t*)((char*)glyph.fImage + glyph.fHeight * dstRB - 1);
        *first |= 1 << 7;
        *last |= bitCount == 0 ? 1 : 1 << (8 - bitCount);
    }
#endif
}

template<bool APPLY_PREBLEND>
static void rgb_to_a8(const SkGdiRGB* SK_RESTRICT src, size_t srcRB,
                      const SkGlyph& glyph, const uint8_t* table8) {
    const size_t dstRB = glyph.rowBytes();
    const int width = glyph.fWidth;
    uint8_t* SK_RESTRICT dst = (uint8_t*)((char*)glyph.fImage + (glyph.fHeight - 1) * dstRB);

    for (int y = 0; y < glyph.fHeight; y++) {
        for (int i = 0; i < width; i++) {
            dst[i] = rgb_to_a8<APPLY_PREBLEND>(src[i], table8);
#if SK_SHOW_TEXT_BLIT_COVERAGE
            dst[i] = SkMax32(dst[i], 10);
#endif
        }
        src = SkTAddOffset<const SkGdiRGB>(src, srcRB);
        dst -= dstRB;
    }
}

template<bool APPLY_PREBLEND>
static void rgb_to_lcd16(const SkGdiRGB* SK_RESTRICT src, size_t srcRB, const SkGlyph& glyph,
                         const uint8_t* tableR, const uint8_t* tableG, const uint8_t* tableB) {
    const size_t dstRB = glyph.rowBytes();
    const int width = glyph.fWidth;
    uint16_t* SK_RESTRICT dst = (uint16_t*)((char*)glyph.fImage + (glyph.fHeight - 1) * dstRB);

    for (int y = 0; y < glyph.fHeight; y++) {
        for (int i = 0; i < width; i++) {
            dst[i] = rgb_to_lcd16<APPLY_PREBLEND>(src[i], tableR, tableG, tableB);
        }
        src = SkTAddOffset<const SkGdiRGB>(src, srcRB);
        dst = (uint16_t*)((char*)dst - dstRB);
    }
}

template<bool APPLY_PREBLEND>
static void rgb_to_lcd32(const SkGdiRGB* SK_RESTRICT src, size_t srcRB, const SkGlyph& glyph,
                         const uint8_t* tableR, const uint8_t* tableG, const uint8_t* tableB) {
    const size_t dstRB = glyph.rowBytes();
    const int width = glyph.fWidth;
    uint32_t* SK_RESTRICT dst = (uint32_t*)((char*)glyph.fImage + (glyph.fHeight - 1) * dstRB);

    for (int y = 0; y < glyph.fHeight; y++) {
        for (int i = 0; i < width; i++) {
            dst[i] = rgb_to_lcd32<APPLY_PREBLEND>(src[i], tableR, tableG, tableB);
        }
        src = SkTAddOffset<const SkGdiRGB>(src, srcRB);
        dst = (uint32_t*)((char*)dst - dstRB);
    }
}

static inline unsigned clamp255(unsigned x) {
    SkASSERT(x <= 256);
    return x - (x >> 8);
}

void SkScalerContext_GDI::generateImage(const SkGlyph& glyph) {
    SkASSERT(fDDC);

    const bool isBW = SkMask::kBW_Format == fRec.fMaskFormat;
    const bool isAA = !isLCD(fRec);

    size_t srcRB;
    const void* bits = fOffscreen.draw(glyph, isBW, &srcRB);
    if (NULL == bits) {
        LogFontTypeface::EnsureAccessible(this->getTypeface());
        bits = fOffscreen.draw(glyph, isBW, &srcRB);
        if (NULL == bits) {
            sk_bzero(glyph.fImage, glyph.computeImageSize());
            return;
        }
    }

    if (!isBW) {
        const uint8_t* table;
        
        
        if (isAA && !(fRec.fFlags & SkScalerContext::kGenA8FromLCD_Flag)) {
            table = getInverseGammaTableGDI();
        } else {
            table = getInverseGammaTableClearType();
        }
        
        
        
        
        
        SkGdiRGB* addr = (SkGdiRGB*)bits;
        for (int y = 0; y < glyph.fHeight; ++y) {
            for (int x = 0; x < glyph.fWidth; ++x) {
                int r = (addr[x] >> 16) & 0xFF;
                int g = (addr[x] >>  8) & 0xFF;
                int b = (addr[x] >>  0) & 0xFF;
                addr[x] = (table[r] << 16) | (table[g] << 8) | table[b];
            }
            addr = SkTAddOffset<SkGdiRGB>(addr, srcRB);
        }
    }

    int width = glyph.fWidth;
    size_t dstRB = glyph.rowBytes();
    if (isBW) {
        const uint8_t* src = (const uint8_t*)bits;
        uint8_t* dst = (uint8_t*)((char*)glyph.fImage + (glyph.fHeight - 1) * dstRB);
        for (int y = 0; y < glyph.fHeight; y++) {
            memcpy(dst, src, dstRB);
            src += srcRB;
            dst -= dstRB;
        }
#if SK_SHOW_TEXT_BLIT_COVERAGE
            if (glyph.fWidth > 0 && glyph.fHeight > 0) {
                int bitCount = width & 7;
                uint8_t* first = (uint8_t*)glyph.fImage;
                uint8_t* last = (uint8_t*)((char*)glyph.fImage + glyph.fHeight * dstRB - 1);
                *first |= 1 << 7;
                *last |= bitCount == 0 ? 1 : 1 << (8 - bitCount);
            }
#endif
    } else if (isAA) {
        
        
        const SkGdiRGB* src = (const SkGdiRGB*)bits;
        if (fPreBlend.isApplicable()) {
            rgb_to_a8<true>(src, srcRB, glyph, fPreBlend.fG);
        } else {
            rgb_to_a8<false>(src, srcRB, glyph, fPreBlend.fG);
        }
    } else {    
        const SkGdiRGB* src = (const SkGdiRGB*)bits;
        if (is_rgb_really_bw(src, width, glyph.fHeight, srcRB)) {
            rgb_to_bw(src, srcRB, glyph);
            ((SkGlyph*)&glyph)->fMaskFormat = SkMask::kBW_Format;
        } else {
            if (SkMask::kLCD16_Format == glyph.fMaskFormat) {
                if (fPreBlend.isApplicable()) {
                    rgb_to_lcd16<true>(src, srcRB, glyph,
                                       fPreBlend.fR, fPreBlend.fG, fPreBlend.fB);
                } else {
                    rgb_to_lcd16<false>(src, srcRB, glyph,
                                        fPreBlend.fR, fPreBlend.fG, fPreBlend.fB);
                }
            } else {
                SkASSERT(SkMask::kLCD32_Format == glyph.fMaskFormat);
                if (fPreBlend.isApplicable()) {
                    rgb_to_lcd32<true>(src, srcRB, glyph,
                                       fPreBlend.fR, fPreBlend.fG, fPreBlend.fB);
                } else {
                    rgb_to_lcd32<false>(src, srcRB, glyph,
                                        fPreBlend.fR, fPreBlend.fG, fPreBlend.fB);
                }
            }
        }
    }
}

class GDIGlyphbufferPointIter {
public:
    GDIGlyphbufferPointIter(const uint8_t* glyphbuf, DWORD total_size)
        : fHeaderIter(glyphbuf, total_size), fCurveIter(), fPointIter()
    { }

    POINTFX const * next() {
nextHeader:
        if (!fCurveIter.isSet()) {
            const TTPOLYGONHEADER* header = fHeaderIter.next();
            if (NULL == header) {
                return NULL;
            }
            fCurveIter.set(header);
            const TTPOLYCURVE* curve = fCurveIter.next();
            if (NULL == curve) {
                return NULL;
            }
            fPointIter.set(curve);
            return &header->pfxStart;
        }

        const POINTFX* nextPoint = fPointIter.next();
        if (NULL == nextPoint) {
            const TTPOLYCURVE* curve = fCurveIter.next();
            if (NULL == curve) {
                fCurveIter.set();
                goto nextHeader;
            } else {
                fPointIter.set(curve);
            }
            nextPoint = fPointIter.next();
        }
        return nextPoint;
    }

    WORD currentCurveType() {
        return fPointIter.fCurveType;
    }

private:
    
    class GDIPolygonHeaderIter {
    public:
        GDIPolygonHeaderIter(const uint8_t* glyphbuf, DWORD total_size)
            : fCurPolygon(reinterpret_cast<const TTPOLYGONHEADER*>(glyphbuf))
            , fEndPolygon(SkTAddOffset<const TTPOLYGONHEADER>(glyphbuf, total_size))
        { }

        const TTPOLYGONHEADER* next() {
            if (fCurPolygon >= fEndPolygon) {
                return NULL;
            }
            const TTPOLYGONHEADER* thisPolygon = fCurPolygon;
            fCurPolygon = SkTAddOffset<const TTPOLYGONHEADER>(fCurPolygon, fCurPolygon->cb);
            return thisPolygon;
        }
    private:
        const TTPOLYGONHEADER* fCurPolygon;
        const TTPOLYGONHEADER* fEndPolygon;
    };

    
    class GDIPolygonCurveIter {
    public:
        GDIPolygonCurveIter() : fCurCurve(NULL), fEndCurve(NULL) { }

        GDIPolygonCurveIter(const TTPOLYGONHEADER* curPolygon)
            : fCurCurve(SkTAddOffset<const TTPOLYCURVE>(curPolygon, sizeof(TTPOLYGONHEADER)))
            , fEndCurve(SkTAddOffset<const TTPOLYCURVE>(curPolygon, curPolygon->cb))
        { }

        bool isSet() { return fCurCurve != NULL; }

        void set(const TTPOLYGONHEADER* curPolygon) {
            fCurCurve = SkTAddOffset<const TTPOLYCURVE>(curPolygon, sizeof(TTPOLYGONHEADER));
            fEndCurve = SkTAddOffset<const TTPOLYCURVE>(curPolygon, curPolygon->cb);
        }
        void set() {
            fCurCurve = NULL;
            fEndCurve = NULL;
        }

        const TTPOLYCURVE* next() {
            if (fCurCurve >= fEndCurve) {
                return NULL;
            }
            const TTPOLYCURVE* thisCurve = fCurCurve;
            fCurCurve = SkTAddOffset<const TTPOLYCURVE>(fCurCurve, size_of_TTPOLYCURVE(*fCurCurve));
            return thisCurve;
        }
    private:
        size_t size_of_TTPOLYCURVE(const TTPOLYCURVE& curve) {
            return 2*sizeof(WORD) + curve.cpfx*sizeof(POINTFX);
        }
        const TTPOLYCURVE* fCurCurve;
        const TTPOLYCURVE* fEndCurve;
    };

    
    class GDIPolygonCurvePointIter {
    public:
        GDIPolygonCurvePointIter() : fCurveType(0), fCurPoint(NULL), fEndPoint(NULL) { }

        GDIPolygonCurvePointIter(const TTPOLYCURVE* curPolygon)
            : fCurveType(curPolygon->wType)
            , fCurPoint(&curPolygon->apfx[0])
            , fEndPoint(&curPolygon->apfx[curPolygon->cpfx])
        { }

        bool isSet() { return fCurPoint != NULL; }

        void set(const TTPOLYCURVE* curPolygon) {
            fCurveType = curPolygon->wType;
            fCurPoint = &curPolygon->apfx[0];
            fEndPoint = &curPolygon->apfx[curPolygon->cpfx];
        }
        void set() {
            fCurPoint = NULL;
            fEndPoint = NULL;
        }

        const POINTFX* next() {
            if (fCurPoint >= fEndPoint) {
                return NULL;
            }
            const POINTFX* thisPoint = fCurPoint;
            ++fCurPoint;
            return thisPoint;
        }

        WORD fCurveType;
    private:
        const POINTFX* fCurPoint;
        const POINTFX* fEndPoint;
    };

    GDIPolygonHeaderIter fHeaderIter;
    GDIPolygonCurveIter fCurveIter;
    GDIPolygonCurvePointIter fPointIter;
};

static void sk_path_from_gdi_path(SkPath* path, const uint8_t* glyphbuf, DWORD total_size) {
    const uint8_t* cur_glyph = glyphbuf;
    const uint8_t* end_glyph = glyphbuf + total_size;

    while (cur_glyph < end_glyph) {
        const TTPOLYGONHEADER* th = (TTPOLYGONHEADER*)cur_glyph;

        const uint8_t* end_poly = cur_glyph + th->cb;
        const uint8_t* cur_poly = cur_glyph + sizeof(TTPOLYGONHEADER);

        path->moveTo(SkFixedToScalar( SkFIXEDToFixed(th->pfxStart.x)),
                     SkFixedToScalar(-SkFIXEDToFixed(th->pfxStart.y)));

        while (cur_poly < end_poly) {
            const TTPOLYCURVE* pc = (const TTPOLYCURVE*)cur_poly;

            if (pc->wType == TT_PRIM_LINE) {
                for (uint16_t i = 0; i < pc->cpfx; i++) {
                    path->lineTo(SkFixedToScalar( SkFIXEDToFixed(pc->apfx[i].x)),
                                 SkFixedToScalar(-SkFIXEDToFixed(pc->apfx[i].y)));
                }
            }

            if (pc->wType == TT_PRIM_QSPLINE) {
                for (uint16_t u = 0; u < pc->cpfx - 1; u++) { 
                    POINTFX pnt_b = pc->apfx[u];    
                    POINTFX pnt_c = pc->apfx[u+1];

                    if (u < pc->cpfx - 2) {          
                        pnt_c.x = SkFixedToFIXED(SkFixedAve(SkFIXEDToFixed(pnt_b.x),
                                                            SkFIXEDToFixed(pnt_c.x)));
                        pnt_c.y = SkFixedToFIXED(SkFixedAve(SkFIXEDToFixed(pnt_b.y),
                                                            SkFIXEDToFixed(pnt_c.y)));
                    }

                    path->quadTo(SkFixedToScalar( SkFIXEDToFixed(pnt_b.x)),
                                 SkFixedToScalar(-SkFIXEDToFixed(pnt_b.y)),
                                 SkFixedToScalar( SkFIXEDToFixed(pnt_c.x)),
                                 SkFixedToScalar(-SkFIXEDToFixed(pnt_c.y)));
                }
            }
            
            cur_poly += sizeof(WORD) * 2 + sizeof(POINTFX) * pc->cpfx;
        }
        cur_glyph += th->cb;
        path->close();
    }
}

#define move_next_expected_hinted_point(iter, pElem) do {\
    pElem = iter.next(); \
    if (NULL == pElem) return false; \
} while(0)




static bool sk_path_from_gdi_paths(SkPath* path, const uint8_t* glyphbuf, DWORD total_size,
                                   GDIGlyphbufferPointIter hintedYs) {
    const uint8_t* cur_glyph = glyphbuf;
    const uint8_t* end_glyph = glyphbuf + total_size;

    POINTFX const * hintedPoint;

    while (cur_glyph < end_glyph) {
        const TTPOLYGONHEADER* th = (TTPOLYGONHEADER*)cur_glyph;

        const uint8_t* end_poly = cur_glyph + th->cb;
        const uint8_t* cur_poly = cur_glyph + sizeof(TTPOLYGONHEADER);

        move_next_expected_hinted_point(hintedYs, hintedPoint);
        path->moveTo(SkFixedToScalar( SkFIXEDToFixed(th->pfxStart.x)),
                     SkFixedToScalar(-SkFIXEDToFixed(hintedPoint->y)));

        while (cur_poly < end_poly) {
            const TTPOLYCURVE* pc = (const TTPOLYCURVE*)cur_poly;

            if (pc->wType == TT_PRIM_LINE) {
                for (uint16_t i = 0; i < pc->cpfx; i++) {
                    move_next_expected_hinted_point(hintedYs, hintedPoint);
                    path->lineTo(SkFixedToScalar( SkFIXEDToFixed(pc->apfx[i].x)),
                                 SkFixedToScalar(-SkFIXEDToFixed(hintedPoint->y)));
                }
            }

            if (pc->wType == TT_PRIM_QSPLINE) {
                POINTFX currentPoint = pc->apfx[0];
                move_next_expected_hinted_point(hintedYs, hintedPoint);
                
                if (hintedYs.currentCurveType() == TT_PRIM_QSPLINE) {
                    currentPoint.y = hintedPoint->y;
                }
                for (uint16_t u = 0; u < pc->cpfx - 1; u++) { 
                    POINTFX pnt_b = currentPoint;
                    POINTFX pnt_c = pc->apfx[u+1];
                    move_next_expected_hinted_point(hintedYs, hintedPoint);
                    
                    if (hintedYs.currentCurveType() == TT_PRIM_QSPLINE) {
                        pnt_c.y = hintedPoint->y;
                    }
                    currentPoint.x = pnt_c.x;
                    currentPoint.y = pnt_c.y;

                    if (u < pc->cpfx - 2) {          
                        pnt_c.x = SkFixedToFIXED(SkFixedAve(SkFIXEDToFixed(pnt_b.x),
                                                            SkFIXEDToFixed(pnt_c.x)));
                        pnt_c.y = SkFixedToFIXED(SkFixedAve(SkFIXEDToFixed(pnt_b.y),
                                                            SkFIXEDToFixed(pnt_c.y)));
                    }

                    path->quadTo(SkFixedToScalar( SkFIXEDToFixed(pnt_b.x)),
                                 SkFixedToScalar(-SkFIXEDToFixed(pnt_b.y)),
                                 SkFixedToScalar( SkFIXEDToFixed(pnt_c.x)),
                                 SkFixedToScalar(-SkFIXEDToFixed(pnt_c.y)));
                }
            }
            
            cur_poly += sizeof(WORD) * 2 + sizeof(POINTFX) * pc->cpfx;
        }
        cur_glyph += th->cb;
        path->close();
    }
    return true;
}

DWORD SkScalerContext_GDI::getGDIGlyphPath(const SkGlyph& glyph, UINT flags,
                                               SkAutoSTMalloc<BUFFERSIZE, uint8_t>* glyphbuf)
{
    GLYPHMETRICS gm;

    DWORD total_size = GetGlyphOutlineW(fDDC, glyph.fID, flags, &gm, BUFFERSIZE, glyphbuf->get(), &fMat22);
    
    
    if (GDI_ERROR == total_size || total_size > BUFFERSIZE) {
        
        
        
        total_size = GetGlyphOutlineW(fDDC, glyph.fID, flags, &gm, 0, NULL, &fMat22);
        if (GDI_ERROR == total_size) {
            LogFontTypeface::EnsureAccessible(this->getTypeface());
            total_size = GetGlyphOutlineW(fDDC, glyph.fID, flags, &gm, 0, NULL, &fMat22);
            if (GDI_ERROR == total_size) {
                
                
                return 0;
            }
        }

        glyphbuf->reset(total_size);

        DWORD ret = GetGlyphOutlineW(fDDC, glyph.fID, flags, &gm, total_size, glyphbuf->get(), &fMat22);
        if (GDI_ERROR == ret) {
            LogFontTypeface::EnsureAccessible(this->getTypeface());
            ret = GetGlyphOutlineW(fDDC, glyph.fID, flags, &gm, total_size, glyphbuf->get(), &fMat22);
            if (GDI_ERROR == ret) {
                SkASSERT(false);
                return 0;
            }
        }
    }
    return total_size;
}

void SkScalerContext_GDI::generatePath(const SkGlyph& glyph, SkPath* path) {
    SkASSERT(&glyph && path);
    SkASSERT(fDDC);

    path->reset();

    
    
    
    
    
    
    

    
    UINT format = GGO_NATIVE | GGO_GLYPH_INDEX;
    if (fRec.getHinting() == SkPaint::kNo_Hinting || fRec.getHinting() == SkPaint::kSlight_Hinting){
        format |= GGO_UNHINTED;
    }
    SkAutoSTMalloc<BUFFERSIZE, uint8_t> glyphbuf(BUFFERSIZE);
    DWORD total_size = getGDIGlyphPath(glyph, format, &glyphbuf);
    if (0 == total_size) {
        return;
    }

    if (fRec.getHinting() != SkPaint::kSlight_Hinting) {
        sk_path_from_gdi_path(path, glyphbuf, total_size);
    } else {
        
        UINT format = GGO_NATIVE | GGO_GLYPH_INDEX;

        SkAutoSTMalloc<BUFFERSIZE, uint8_t> hintedGlyphbuf(BUFFERSIZE);
        DWORD hinted_total_size = getGDIGlyphPath(glyph, format, &hintedGlyphbuf);
        if (0 == hinted_total_size) {
            return;
        }

        if (!sk_path_from_gdi_paths(path, glyphbuf, total_size,
                                    GDIGlyphbufferPointIter(hintedGlyphbuf, hinted_total_size)))
        {
            path->reset();
            sk_path_from_gdi_path(path, glyphbuf, total_size);
        }
    }
}

static void logfont_for_name(const char* familyName, LOGFONT* lf) {
    sk_bzero(lf, sizeof(LOGFONT));
#ifdef UNICODE
    
    size_t str_len = ::MultiByteToWideChar(CP_UTF8, 0, familyName,
                                            -1, NULL, 0);
    
    
    wchar_t *wideFamilyName = new wchar_t[str_len];
    
    ::MultiByteToWideChar(CP_UTF8, 0, familyName, -1,
                            wideFamilyName, str_len);
    ::wcsncpy(lf->lfFaceName, wideFamilyName, LF_FACESIZE - 1);
    delete [] wideFamilyName;
    lf->lfFaceName[LF_FACESIZE-1] = L'\0';
#else
    ::strncpy(lf->lfFaceName, familyName, LF_FACESIZE - 1);
    lf->lfFaceName[LF_FACESIZE - 1] = '\0';
#endif
}

void LogFontTypeface::onGetFontDescriptor(SkFontDescriptor* desc,
                                          bool* isLocalStream) const {
    
    HFONT font = CreateFontIndirect(&fLogFont);

    HDC deviceContext = ::CreateCompatibleDC(NULL);
    HFONT savefont = (HFONT)SelectObject(deviceContext, font);

    SkString familyName;
    dcfontname_to_skstring(deviceContext, fLogFont, &familyName);

    if (deviceContext) {
        ::SelectObject(deviceContext, savefont);
        ::DeleteDC(deviceContext);
    }
    if (font) {
        ::DeleteObject(font);
    }

    desc->setFamilyName(familyName.c_str());
    *isLocalStream = this->fSerializeAsStream;
}

static bool getWidthAdvance(HDC hdc, int gId, int16_t* advance) {
    
    static const MAT2 mat2 = {SkScalarToFIXED(1), SkScalarToFIXED(0),
                        SkScalarToFIXED(0), SkScalarToFIXED(1)};
    int flags = GGO_METRICS | GGO_GLYPH_INDEX;
    GLYPHMETRICS gm;
    if (GDI_ERROR == GetGlyphOutline(hdc, gId, flags, &gm, 0, NULL, &mat2)) {
        return false;
    }
    SkASSERT(advance);
    *advance = gm.gmCellIncX;
    return true;
}

SkAdvancedTypefaceMetrics* LogFontTypeface::onGetAdvancedTypefaceMetrics(
        SkAdvancedTypefaceMetrics::PerGlyphInfo perGlyphInfo,
        const uint32_t* glyphIDs,
        uint32_t glyphIDsCount) const {
    LOGFONT lf = fLogFont;
    SkAdvancedTypefaceMetrics* info = NULL;

    HDC hdc = CreateCompatibleDC(NULL);
    HFONT font = CreateFontIndirect(&lf);
    HFONT savefont = (HFONT)SelectObject(hdc, font);
    HFONT designFont = NULL;

    const char stem_chars[] = {'i', 'I', '!', '1'};
    int16_t min_width;
    unsigned glyphCount;

    
    
    OUTLINETEXTMETRIC otm;
    unsigned int otmRet = GetOutlineTextMetrics(hdc, sizeof(otm), &otm);
    if (0 == otmRet) {
        call_ensure_accessible(lf);
        otmRet = GetOutlineTextMetrics(hdc, sizeof(otm), &otm);
    }
    if (!otmRet || !GetTextFace(hdc, LF_FACESIZE, lf.lfFaceName)) {
        goto Error;
    }
    lf.lfHeight = -SkToS32(otm.otmEMSquare);
    designFont = CreateFontIndirect(&lf);
    SelectObject(hdc, designFont);
    if (!GetOutlineTextMetrics(hdc, sizeof(otm), &otm)) {
        goto Error;
    }
    glyphCount = calculateGlyphCount(hdc, fLogFont);

    info = new SkAdvancedTypefaceMetrics;
    info->fEmSize = otm.otmEMSquare;
    info->fLastGlyphID = SkToU16(glyphCount - 1);
    info->fStyle = 0;
    tchar_to_skstring(lf.lfFaceName, &info->fFontName);
    info->fFlags = SkAdvancedTypefaceMetrics::kEmpty_FontFlag;
    
    
    
    if (otm.otmfsType & 0x1) {
        info->fFlags = SkTBitOr<SkAdvancedTypefaceMetrics::FontFlags>(
                info->fFlags,
                SkAdvancedTypefaceMetrics::kNotEmbeddable_FontFlag);
    }

    if (perGlyphInfo & SkAdvancedTypefaceMetrics::kToUnicode_PerGlyphInfo) {
        populate_glyph_to_unicode(hdc, glyphCount, &(info->fGlyphToUnicode));
    }

    if (glyphCount > 0 &&
        (otm.otmTextMetrics.tmPitchAndFamily & TMPF_TRUETYPE)) {
        info->fType = SkAdvancedTypefaceMetrics::kTrueType_Font;
    } else {
        info->fType = SkAdvancedTypefaceMetrics::kOther_Font;
        info->fItalicAngle = 0;
        info->fAscent = 0;
        info->fDescent = 0;
        info->fStemV = 0;
        info->fCapHeight = 0;
        info->fBBox = SkIRect::MakeEmpty();
        goto ReturnInfo;
    }

    
    if (!(otm.otmTextMetrics.tmPitchAndFamily & TMPF_FIXED_PITCH)) {
        info->fStyle |= SkAdvancedTypefaceMetrics::kFixedPitch_Style;
    }
    if (otm.otmTextMetrics.tmItalic) {
        info->fStyle |= SkAdvancedTypefaceMetrics::kItalic_Style;
    }
    if (otm.otmTextMetrics.tmPitchAndFamily & FF_ROMAN) {
        info->fStyle |= SkAdvancedTypefaceMetrics::kSerif_Style;
    } else if (otm.otmTextMetrics.tmPitchAndFamily & FF_SCRIPT) {
            info->fStyle |= SkAdvancedTypefaceMetrics::kScript_Style;
    }

    
    
    info->fItalicAngle = otm.otmItalicAngle / 10;
    info->fAscent = SkToS16(otm.otmTextMetrics.tmAscent);
    info->fDescent = SkToS16(-otm.otmTextMetrics.tmDescent);
    
    
    
    info->fCapHeight = otm.otmsCapEmHeight;
    info->fBBox =
        SkIRect::MakeLTRB(otm.otmrcFontBox.left, otm.otmrcFontBox.top,
                          otm.otmrcFontBox.right, otm.otmrcFontBox.bottom);

    
    
    min_width = SHRT_MAX;
    info->fStemV = 0;
    for (size_t i = 0; i < SK_ARRAY_COUNT(stem_chars); i++) {
        ABC abcWidths;
        if (GetCharABCWidths(hdc, stem_chars[i], stem_chars[i], &abcWidths)) {
            int16_t width = abcWidths.abcB;
            if (width > 0 && width < min_width) {
                min_width = width;
                info->fStemV = min_width;
            }
        }
    }

    if (perGlyphInfo & SkAdvancedTypefaceMetrics::kHAdvance_PerGlyphInfo) {
        if (info->fStyle & SkAdvancedTypefaceMetrics::kFixedPitch_Style) {
            appendRange(&info->fGlyphWidths, 0);
            info->fGlyphWidths->fAdvance.append(1, &min_width);
            finishRange(info->fGlyphWidths.get(), 0,
                        SkAdvancedTypefaceMetrics::WidthRange::kDefault);
        } else {
            info->fGlyphWidths.reset(
                getAdvanceData(hdc,
                               glyphCount,
                               glyphIDs,
                               glyphIDsCount,
                               &getWidthAdvance));
        }
    }

Error:
ReturnInfo:
    SelectObject(hdc, savefont);
    DeleteObject(designFont);
    DeleteObject(font);
    DeleteDC(hdc);

    return info;
}


#define BASE64_GUID_ID "XXXXXXXXXXXXXXXXXXXXXXXX"

#define BASE64_GUID_ID_LEN SK_ARRAY_COUNT(BASE64_GUID_ID)

SK_COMPILE_ASSERT(BASE64_GUID_ID_LEN < LF_FACESIZE, GUID_longer_than_facesize);








static const char postscript_safe_base64_encode[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789-_=";







static void format_guid_b64(const GUID& guid, char* buffer, size_t bufferSize) {
    SkASSERT(bufferSize >= BASE64_GUID_ID_LEN);
    size_t written = SkBase64::Encode(&guid, sizeof(guid), buffer, postscript_safe_base64_encode);
    SkASSERT(written < LF_FACESIZE);
    buffer[written] = '\0';
}







static HRESULT create_unique_font_name(char* buffer, size_t bufferSize) {
    GUID guid = {};
    if (FAILED(CoCreateGuid(&guid))) {
        return E_UNEXPECTED;
    }
    format_guid_b64(guid, buffer, bufferSize);

    return S_OK;
}





static HANDLE activate_font(SkData* fontData) {
    DWORD numFonts = 0;
    
    HANDLE fontHandle = AddFontMemResourceEx(const_cast<void*>(fontData->data()),
                                             static_cast<DWORD>(fontData->size()),
                                             0,
                                             &numFonts);

    if (fontHandle != NULL && numFonts < 1) {
        RemoveFontMemResourceEx(fontHandle);
        return NULL;
    }

    return fontHandle;
}

static SkTypeface* create_from_stream(SkStream* stream) {
    
    
    char familyName[BASE64_GUID_ID_LEN];
    const int familyNameSize = SK_ARRAY_COUNT(familyName);
    if (FAILED(create_unique_font_name(familyName, familyNameSize))) {
        return NULL;
    }

    
    SkAutoTUnref<SkData> rewrittenFontData(SkOTUtils::RenameFont(stream, familyName, familyNameSize-1));
    if (NULL == rewrittenFontData.get()) {
        return NULL;
    }

    
    HANDLE fontReference = activate_font(rewrittenFontData.get());
    if (NULL == fontReference) {
        return NULL;
    }

    
    LOGFONT lf;
    logfont_for_name(familyName, &lf);

    return SkCreateFontMemResourceTypefaceFromLOGFONT(lf, fontReference);
}

SkStream* LogFontTypeface::onOpenStream(int* ttcIndex) const {
    *ttcIndex = 0;

    const DWORD kTTCTag =
        SkEndian_SwapBE32(SkSetFourByteTag('t', 't', 'c', 'f'));
    LOGFONT lf = fLogFont;

    HDC hdc = ::CreateCompatibleDC(NULL);
    HFONT font = CreateFontIndirect(&lf);
    HFONT savefont = (HFONT)SelectObject(hdc, font);

    SkMemoryStream* stream = NULL;
    DWORD tables[2] = {kTTCTag, 0};
    for (int i = 0; i < SK_ARRAY_COUNT(tables); i++) {
        DWORD bufferSize = GetFontData(hdc, tables[i], 0, NULL, 0);
        if (bufferSize == GDI_ERROR) {
            call_ensure_accessible(lf);
            bufferSize = GetFontData(hdc, tables[i], 0, NULL, 0);
        }
        if (bufferSize != GDI_ERROR) {
            stream = new SkMemoryStream(bufferSize);
            if (GetFontData(hdc, tables[i], 0, (void*)stream->getMemoryBase(), bufferSize)) {
                break;
            } else {
                delete stream;
                stream = NULL;
            }
        }
    }

    SelectObject(hdc, savefont);
    DeleteObject(font);
    DeleteDC(hdc);

    return stream;
}

static void bmpCharsToGlyphs(HDC hdc, const WCHAR* bmpChars, int count, uint16_t* glyphs,
                             bool Ox1FHack)
{
    DWORD result = GetGlyphIndicesW(hdc, bmpChars, count, glyphs, GGI_MARK_NONEXISTING_GLYPHS);
    if (GDI_ERROR == result) {
        for (int i = 0; i < count; ++i) {
            glyphs[i] = 0;
        }
        return;
    }

    if (Ox1FHack) {
        for (int i = 0; i < count; ++i) {
            if (0xFFFF == glyphs[i] || 0x1F == glyphs[i]) {
                glyphs[i] = 0;
            }
        }
    } else {
        for (int i = 0; i < count; ++i) {
            if (0xFFFF == glyphs[i]){
                glyphs[i] = 0;
            }
        }
    }
}

static uint16_t nonBmpCharToGlyph(HDC hdc, SCRIPT_CACHE* scriptCache, const WCHAR utf16[2]) {
    uint16_t index = 0;
    
    static const int numWCHAR = 2;
    static const int maxItems = 2;
    
    SCRIPT_CONTROL scriptControl = { 0 };
    
    
    SCRIPT_ITEM si[maxItems + 1];
    int numItems;
    HRZM(ScriptItemize(utf16, numWCHAR, maxItems, &scriptControl, NULL, si, &numItems),
         "Could not itemize character.");

    
    static const int maxGlyphs = 2;
    SCRIPT_VISATTR vsa[maxGlyphs];
    WORD outGlyphs[maxGlyphs];
    WORD logClust[numWCHAR];
    int numGlyphs;
    HRZM(ScriptShape(hdc, scriptCache, utf16, numWCHAR, maxGlyphs, &si[0].a,
                     outGlyphs, logClust, vsa, &numGlyphs),
         "Could not shape character.");
    if (1 == numGlyphs) {
        index = outGlyphs[0];
    }
    return index;
}

class SkAutoHDC {
public:
    SkAutoHDC(const LOGFONT& lf)
        : fHdc(::CreateCompatibleDC(NULL))
        , fFont(::CreateFontIndirect(&lf))
        , fSavefont((HFONT)SelectObject(fHdc, fFont))
    { }
    ~SkAutoHDC() {
        SelectObject(fHdc, fSavefont);
        DeleteObject(fFont);
        DeleteDC(fHdc);
    }
    operator HDC() { return fHdc; }
private:
    HDC fHdc;
    HFONT fFont;
    HFONT fSavefont;
};
#define SkAutoHDC(...) SK_REQUIRE_LOCAL_VAR(SkAutoHDC)

int LogFontTypeface::onCharsToGlyphs(const void* chars, Encoding encoding,
                                     uint16_t userGlyphs[], int glyphCount) const
{
    SkAutoHDC hdc(fLogFont);

    TEXTMETRIC tm;
    if (0 == GetTextMetrics(hdc, &tm)) {
        call_ensure_accessible(fLogFont);
        if (0 == GetTextMetrics(hdc, &tm)) {
            tm.tmPitchAndFamily = TMPF_TRUETYPE;
        }
    }
    bool Ox1FHack = !(tm.tmPitchAndFamily & TMPF_VECTOR) ;

    SkAutoSTMalloc<256, uint16_t> scratchGlyphs;
    uint16_t* glyphs;
    if (userGlyphs != NULL) {
        glyphs = userGlyphs;
    } else {
        glyphs = scratchGlyphs.reset(glyphCount);
    }

    SCRIPT_CACHE sc = 0;
    switch (encoding) {
    case SkTypeface::kUTF8_Encoding: {
        static const int scratchCount = 256;
        WCHAR scratch[scratchCount];
        int glyphIndex = 0;
        const char* currentUtf8 = reinterpret_cast<const char*>(chars);
        SkUnichar currentChar;
        if (glyphCount) {
            currentChar = SkUTF8_NextUnichar(&currentUtf8);
        }
        while (glyphIndex < glyphCount) {
            
            int glyphsLeft = SkTMin(glyphCount - glyphIndex, scratchCount);
            int runLength = 0;
            while (runLength < glyphsLeft && currentChar <= 0xFFFF) {
                scratch[runLength] = static_cast<WCHAR>(currentChar);
                ++runLength;
                if (runLength < glyphsLeft) {
                    currentChar = SkUTF8_NextUnichar(&currentUtf8);
                }
            }
            if (runLength) {
                bmpCharsToGlyphs(hdc, scratch, runLength, &glyphs[glyphIndex], Ox1FHack);
                glyphIndex += runLength;
            }

            
            while (glyphIndex < glyphCount && currentChar > 0xFFFF) {
                SkUTF16_FromUnichar(currentChar, reinterpret_cast<uint16_t*>(scratch));
                glyphs[glyphIndex] = nonBmpCharToGlyph(hdc, &sc, scratch);
                ++glyphIndex;
                if (glyphIndex < glyphCount) {
                    currentChar = SkUTF8_NextUnichar(&currentUtf8);
                }
            }
        }
        break;
    }
    case SkTypeface::kUTF16_Encoding: {
        int glyphIndex = 0;
        const WCHAR* currentUtf16 = reinterpret_cast<const WCHAR*>(chars);
        while (glyphIndex < glyphCount) {
            
            int glyphsLeft = glyphCount - glyphIndex;
            int runLength = 0;
            while (runLength < glyphsLeft && !SkUTF16_IsHighSurrogate(currentUtf16[runLength])) {
                ++runLength;
            }
            if (runLength) {
                bmpCharsToGlyphs(hdc, currentUtf16, runLength, &glyphs[glyphIndex], Ox1FHack);
                glyphIndex += runLength;
                currentUtf16 += runLength;
            }

            
            while (glyphIndex < glyphCount && SkUTF16_IsHighSurrogate(*currentUtf16)) {
                glyphs[glyphIndex] = nonBmpCharToGlyph(hdc, &sc, currentUtf16);
                ++glyphIndex;
                currentUtf16 += 2;
            }
        }
        break;
    }
    case SkTypeface::kUTF32_Encoding: {
        static const int scratchCount = 256;
        WCHAR scratch[scratchCount];
        int glyphIndex = 0;
        const uint32_t* utf32 = reinterpret_cast<const uint32_t*>(chars);
        while (glyphIndex < glyphCount) {
            
            int glyphsLeft = SkTMin(glyphCount - glyphIndex, scratchCount);
            int runLength = 0;
            while (runLength < glyphsLeft && utf32[glyphIndex + runLength] <= 0xFFFF) {
                scratch[runLength] = static_cast<WCHAR>(utf32[glyphIndex + runLength]);
                ++runLength;
            }
            if (runLength) {
                bmpCharsToGlyphs(hdc, scratch, runLength, &glyphs[glyphIndex], Ox1FHack);
                glyphIndex += runLength;
            }

            
            while (glyphIndex < glyphCount && utf32[glyphIndex] > 0xFFFF) {
                SkUTF16_FromUnichar(utf32[glyphIndex], reinterpret_cast<uint16_t*>(scratch));
                glyphs[glyphIndex] = nonBmpCharToGlyph(hdc, &sc, scratch);
                ++glyphIndex;
            }
        }
        break;
    }
    default:
        SK_CRASH();
    }

    if (sc) {
        ::ScriptFreeCache(&sc);
    }

    for (int i = 0; i < glyphCount; ++i) {
        if (0 == glyphs[i]) {
            return i;
        }
    }
    return glyphCount;
}

int LogFontTypeface::onCountGlyphs() const {
    HDC hdc = ::CreateCompatibleDC(NULL);
    HFONT font = CreateFontIndirect(&fLogFont);
    HFONT savefont = (HFONT)SelectObject(hdc, font);

    unsigned int glyphCount = calculateGlyphCount(hdc, fLogFont);

    SelectObject(hdc, savefont);
    DeleteObject(font);
    DeleteDC(hdc);

    return glyphCount;
}

int LogFontTypeface::onGetUPEM() const {
    HDC hdc = ::CreateCompatibleDC(NULL);
    HFONT font = CreateFontIndirect(&fLogFont);
    HFONT savefont = (HFONT)SelectObject(hdc, font);

    unsigned int upem = calculateUPEM(hdc, fLogFont);

    SelectObject(hdc, savefont);
    DeleteObject(font);
    DeleteDC(hdc);

    return upem;
}

SkTypeface::LocalizedStrings* LogFontTypeface::onCreateFamilyNameIterator() const {
    SkTypeface::LocalizedStrings* nameIter =
        SkOTUtils::LocalizedStrings_NameTable::CreateForFamilyNames(*this);
    if (NULL == nameIter) {
        SkString familyName;
        this->getFamilyName(&familyName);
        SkString language("und"); 
        nameIter = new SkOTUtils::LocalizedStrings_SingleName(familyName, language);
    }
    return nameIter;
}

int LogFontTypeface::onGetTableTags(SkFontTableTag tags[]) const {
    SkSFNTHeader header;
    if (sizeof(header) != this->onGetTableData(0, 0, sizeof(header), &header)) {
        return 0;
    }

    int numTables = SkEndian_SwapBE16(header.numTables);

    if (tags) {
        size_t size = numTables * sizeof(SkSFNTHeader::TableDirectoryEntry);
        SkAutoSTMalloc<0x20, SkSFNTHeader::TableDirectoryEntry> dir(numTables);
        if (size != this->onGetTableData(0, sizeof(header), size, dir.get())) {
            return 0;
        }

        for (int i = 0; i < numTables; ++i) {
            tags[i] = SkEndian_SwapBE32(dir[i].tag);
        }
    }
    return numTables;
}

size_t LogFontTypeface::onGetTableData(SkFontTableTag tag, size_t offset,
                                       size_t length, void* data) const
{
    LOGFONT lf = fLogFont;

    HDC hdc = ::CreateCompatibleDC(NULL);
    HFONT font = CreateFontIndirect(&lf);
    HFONT savefont = (HFONT)SelectObject(hdc, font);

    tag = SkEndian_SwapBE32(tag);
    if (NULL == data) {
        length = 0;
    }
    DWORD bufferSize = GetFontData(hdc, tag, (DWORD) offset, data, (DWORD) length);
    if (bufferSize == GDI_ERROR) {
        call_ensure_accessible(lf);
        bufferSize = GetFontData(hdc, tag, (DWORD) offset, data, (DWORD) length);
    }

    SelectObject(hdc, savefont);
    DeleteObject(font);
    DeleteDC(hdc);

    return bufferSize == GDI_ERROR ? 0 : bufferSize;
}

SkScalerContext* LogFontTypeface::onCreateScalerContext(const SkDescriptor* desc) const {
    SkScalerContext_GDI* ctx = SkNEW_ARGS(SkScalerContext_GDI,
                                                (const_cast<LogFontTypeface*>(this), desc));
    if (!ctx->isValid()) {
        SkDELETE(ctx);
        ctx = NULL;
    }
    return ctx;
}

void LogFontTypeface::onFilterRec(SkScalerContextRec* rec) const {
    if (rec->fFlags & SkScalerContext::kLCD_BGROrder_Flag ||
        rec->fFlags & SkScalerContext::kLCD_Vertical_Flag)
    {
        rec->fMaskFormat = SkMask::kA8_Format;
        rec->fFlags |= SkScalerContext::kGenA8FromLCD_Flag;
    }

    unsigned flagsWeDontSupport = SkScalerContext::kVertical_Flag |
                                  SkScalerContext::kDevKernText_Flag |
                                  SkScalerContext::kForceAutohinting_Flag |
                                  SkScalerContext::kEmbeddedBitmapText_Flag |
                                  SkScalerContext::kEmbolden_Flag |
                                  SkScalerContext::kLCD_BGROrder_Flag |
                                  SkScalerContext::kLCD_Vertical_Flag;
    rec->fFlags &= ~flagsWeDontSupport;

    SkPaint::Hinting h = rec->getHinting();
    switch (h) {
        case SkPaint::kNo_Hinting:
            break;
        case SkPaint::kSlight_Hinting:
            
            
            
                h = SkPaint::kNo_Hinting;
            
            break;
        case SkPaint::kNormal_Hinting:
        case SkPaint::kFull_Hinting:
            
            
            
            h = SkPaint::kNormal_Hinting;
            break;
        default:
            SkDEBUGFAIL("unknown hinting");
    }
    
    rec->setHinting(h);


#if 0
    
    if (isLCD(*rec) && !isAxisAligned(*rec)) {
        rec->fMaskFormat = SkMask::kA8_Format;
    }
#endif

    if (!fCanBeLCD && isLCD(*rec)) {
        rec->fMaskFormat = SkMask::kA8_Format;
        rec->fFlags &= ~SkScalerContext::kGenA8FromLCD_Flag;
    }
}



#include "SkFontMgr.h"
#include "SkDataTable.h"

static bool valid_logfont_for_enum(const LOGFONT& lf) {
    
    return
        
        lf.lfFaceName[0] && lf.lfFaceName[0] != '@'

        
        
        && ANSI_CHARSET == lf.lfCharSet
    ;
}





static int CALLBACK enum_family_proc(const LOGFONT* lf, const TEXTMETRIC*,
                                     DWORD fontType, LPARAM builderParam) {
    if (valid_logfont_for_enum(*lf)) {
        SkTDArray<ENUMLOGFONTEX>* array = (SkTDArray<ENUMLOGFONTEX>*)builderParam;
        *array->append() = *(ENUMLOGFONTEX*)lf;
    }
    return 1; 
}

static SkFontStyle compute_fontstyle(const LOGFONT& lf) {
    return SkFontStyle(lf.lfWeight, SkFontStyle::kNormal_Width,
                       lf.lfItalic ? SkFontStyle::kItalic_Slant
                                   : SkFontStyle::kUpright_Slant);
}

class SkFontStyleSetGDI : public SkFontStyleSet {
public:
    SkFontStyleSetGDI(const TCHAR familyName[]) {
        LOGFONT lf;
        sk_bzero(&lf, sizeof(lf));
        lf.lfCharSet = DEFAULT_CHARSET;
        _tcscpy_s(lf.lfFaceName, familyName);

        HDC hdc = ::CreateCompatibleDC(NULL);
        ::EnumFontFamiliesEx(hdc, &lf, enum_family_proc, (LPARAM)&fArray, 0);
        ::DeleteDC(hdc);
    }

    virtual int count() SK_OVERRIDE {
        return fArray.count();
    }

    virtual void getStyle(int index, SkFontStyle* fs, SkString* styleName) SK_OVERRIDE {
        if (fs) {
            *fs = compute_fontstyle(fArray[index].elfLogFont);
        }
        if (styleName) {
            const ENUMLOGFONTEX& ref = fArray[index];
            
            
            
            
            
            
            SkASSERT(sizeof(TCHAR) == sizeof(ref.elfStyle[0]));
            tchar_to_skstring((const TCHAR*)ref.elfStyle, styleName);
        }
    }

    virtual SkTypeface* createTypeface(int index) SK_OVERRIDE {
        return SkCreateTypefaceFromLOGFONT(fArray[index].elfLogFont);
    }

    virtual SkTypeface* matchStyle(const SkFontStyle& pattern) SK_OVERRIDE {
        
        return SkCreateTypefaceFromLOGFONT(fArray[0].elfLogFont);
    }

private:
    SkTDArray<ENUMLOGFONTEX> fArray;
};

class SkFontMgrGDI : public SkFontMgr {
public:
    SkFontMgrGDI() {
        LOGFONT lf;
        sk_bzero(&lf, sizeof(lf));
        lf.lfCharSet = DEFAULT_CHARSET;

        HDC hdc = ::CreateCompatibleDC(NULL);
        ::EnumFontFamiliesEx(hdc, &lf, enum_family_proc, (LPARAM)&fLogFontArray, 0);
        ::DeleteDC(hdc);
    }

protected:
    virtual int onCountFamilies() const SK_OVERRIDE {
        return fLogFontArray.count();
    }

    virtual void onGetFamilyName(int index, SkString* familyName) const SK_OVERRIDE {
        SkASSERT((unsigned)index < (unsigned)fLogFontArray.count());
        tchar_to_skstring(fLogFontArray[index].elfLogFont.lfFaceName, familyName);
    }

    virtual SkFontStyleSet* onCreateStyleSet(int index) const SK_OVERRIDE {
        SkASSERT((unsigned)index < (unsigned)fLogFontArray.count());
        return SkNEW_ARGS(SkFontStyleSetGDI, (fLogFontArray[index].elfLogFont.lfFaceName));
    }

    virtual SkFontStyleSet* onMatchFamily(const char familyName[]) const SK_OVERRIDE {
        if (NULL == familyName) {
            familyName = "";    
        }
        LOGFONT lf;
        logfont_for_name(familyName, &lf);
        return SkNEW_ARGS(SkFontStyleSetGDI, (lf.lfFaceName));
    }

    virtual SkTypeface* onMatchFamilyStyle(const char familyName[],
                                           const SkFontStyle& fontstyle) const SK_OVERRIDE {
        
        SkAutoTUnref<SkFontStyleSet> sset(this->matchFamily(familyName));
        return sset->matchStyle(fontstyle);
    }

    virtual SkTypeface* onMatchFaceStyle(const SkTypeface* familyMember,
                                         const SkFontStyle& fontstyle) const SK_OVERRIDE {
        
        SkString familyName;
        ((LogFontTypeface*)familyMember)->getFamilyName(&familyName);
        return this->matchFamilyStyle(familyName.c_str(), fontstyle);
    }

    virtual SkTypeface* onCreateFromStream(SkStream* stream, int ttcIndex) const SK_OVERRIDE {
        return create_from_stream(stream);
    }

    virtual SkTypeface* onCreateFromData(SkData* data, int ttcIndex) const SK_OVERRIDE {
        
        SkAutoTUnref<SkStream> stream(SkNEW_ARGS(SkMemoryStream, (data)));
        return this->createFromStream(stream);
    }

    virtual SkTypeface* onCreateFromFile(const char path[], int ttcIndex) const SK_OVERRIDE {
        
        SkAutoTUnref<SkStream> stream(SkStream::NewFromFile(path));
        return this->createFromStream(stream);
    }

    virtual SkTypeface* onLegacyCreateTypeface(const char familyName[],
                                               unsigned styleBits) const SK_OVERRIDE {
        LOGFONT lf;
        if (NULL == familyName) {
            lf = get_default_font();
        } else {
            logfont_for_name(familyName, &lf);
        }
        setStyle(&lf, (SkTypeface::Style)styleBits);
        return SkCreateTypefaceFromLOGFONT(lf);
    }

private:
    SkTDArray<ENUMLOGFONTEX> fLogFontArray;
};



SkFontMgr* SkFontMgr_New_GDI() {
    return SkNEW(SkFontMgrGDI);
}
