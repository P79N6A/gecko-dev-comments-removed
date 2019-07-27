






#ifndef SkScalerContext_DEFINED
#define SkScalerContext_DEFINED

#include "SkMask.h"
#include "SkMaskGamma.h"
#include "SkMatrix.h"
#include "SkPaint.h"
#include "SkTypeface.h"

#ifdef SK_BUILD_FOR_ANDROID
    #include "SkPaintOptionsAndroid.h"
#endif

struct SkGlyph;
class SkDescriptor;
class SkMaskFilter;
class SkPathEffect;
class SkRasterizer;





struct SkScalerContextRec {
    uint32_t    fOrigFontID;
    uint32_t    fFontID;
    SkScalar    fTextSize, fPreScaleX, fPreSkewX;
    SkScalar    fPost2x2[2][2];
    SkScalar    fFrameWidth, fMiterLimit;

    
    uint32_t    fLumBits;
    uint8_t     fDeviceGamma; 
    uint8_t     fPaintGamma;  
    uint8_t     fContrast;    
    uint8_t     fReservedAlign;

    SkScalar getDeviceGamma() const {
        return SkIntToScalar(fDeviceGamma) / (1 << 6);
    }
    void setDeviceGamma(SkScalar dg) {
        SkASSERT(0 <= dg && dg < SkIntToScalar(4));
        fDeviceGamma = SkScalarFloorToInt(dg * (1 << 6));
    }

    SkScalar getPaintGamma() const {
        return SkIntToScalar(fPaintGamma) / (1 << 6);
    }
    void setPaintGamma(SkScalar pg) {
        SkASSERT(0 <= pg && pg < SkIntToScalar(4));
        fPaintGamma = SkScalarFloorToInt(pg * (1 << 6));
    }

    SkScalar getContrast() const {
        return SkIntToScalar(fContrast) / ((1 << 8) - 1);
    }
    void setContrast(SkScalar c) {
        SkASSERT(0 <= c && c <= SK_Scalar1);
        fContrast = SkScalarRoundToInt(c * ((1 << 8) - 1));
    }

    



    void ignorePreBlend() {
        setLuminanceColor(SK_ColorTRANSPARENT);
        setPaintGamma(SK_Scalar1);
        setDeviceGamma(SK_Scalar1);
        setContrast(0);
    }

    uint8_t     fMaskFormat;
    uint8_t     fStrokeJoin;
    uint16_t    fFlags;
    
    
    
    

    void    getMatrixFrom2x2(SkMatrix*) const;
    void    getLocalMatrix(SkMatrix*) const;
    void    getSingleMatrix(SkMatrix*) const;

    inline SkPaint::Hinting getHinting() const;
    inline void setHinting(SkPaint::Hinting);

    SkMask::Format getFormat() const {
        return static_cast<SkMask::Format>(fMaskFormat);
    }

    SkColor getLuminanceColor() const {
        return fLumBits;
    }

    void setLuminanceColor(SkColor c) {
        fLumBits = c;
    }
};






typedef SkTMaskGamma<3, 3, 3> SkMaskGamma;

class SkScalerContext {
public:
    typedef SkScalerContextRec Rec;

    enum Flags {
        kFrameAndFill_Flag        = 0x0001,
        kDevKernText_Flag         = 0x0002,
        kEmbeddedBitmapText_Flag  = 0x0004,
        kEmbolden_Flag            = 0x0008,
        kSubpixelPositioning_Flag = 0x0010,
        kForceAutohinting_Flag    = 0x0020,  
        kVertical_Flag            = 0x0040,

        
        
        kHinting_Shift            = 7, 
        kHintingBit1_Flag         = 0x0080,
        kHintingBit2_Flag         = 0x0100,

        
        
        kLCD_Vertical_Flag        = 0x0200,    
        kLCD_BGROrder_Flag        = 0x0400,    

        
        
        kGenA8FromLCD_Flag        = 0x0800, 
    };

    
    enum {
        kHinting_Mask   = kHintingBit1_Flag | kHintingBit2_Flag,
    };


    SkScalerContext(SkTypeface*, const SkDescriptor*);
    virtual ~SkScalerContext();

    SkTypeface* getTypeface() const { return fTypeface.get(); }

    SkMask::Format getMaskFormat() const {
        return (SkMask::Format)fRec.fMaskFormat;
    }

    bool isSubpixel() const {
        return SkToBool(fRec.fFlags & kSubpixelPositioning_Flag);
    }

    bool isVertical() const {
        return SkToBool(fRec.fFlags & kVertical_Flag);
    }

    
    void setBaseGlyphCount(unsigned baseGlyphCount) {
        fBaseGlyphCount = baseGlyphCount;
    }

    




    uint16_t charToGlyphID(SkUnichar uni);

    


    SkUnichar glyphIDToChar(uint16_t glyphID);

    unsigned    getGlyphCount() { return this->generateGlyphCount(); }
    void        getAdvance(SkGlyph*);
    void        getMetrics(SkGlyph*);
    void        getImage(const SkGlyph&);
    void        getPath(const SkGlyph&, SkPath*);
    void        getFontMetrics(SkPaint::FontMetrics*);

    

    static size_t GetGammaLUTSize(SkScalar contrast, SkScalar paintGamma, SkScalar deviceGamma,
                                  int* width, int* height);

    


    static void   GetGammaLUTData(SkScalar contrast, SkScalar paintGamma, SkScalar deviceGamma,
                                  void* data);

#ifdef SK_BUILD_FOR_ANDROID
    unsigned getBaseGlyphCount(SkUnichar charCode);

    
    
    SkFontID findTypefaceIdForChar(SkUnichar uni);
#endif

    static void MakeRec(const SkPaint&, const SkDeviceProperties* deviceProperties,
                        const SkMatrix*, Rec* rec);
    static inline void PostMakeRec(const SkPaint&, Rec*);

    static SkMaskGamma::PreBlend GetMaskPreBlend(const Rec& rec);

protected:
    Rec         fRec;
    unsigned    fBaseGlyphCount;

    


    virtual void generateAdvance(SkGlyph* glyph) = 0;

    




    virtual void generateMetrics(SkGlyph* glyph) = 0;

    








    virtual void generateImage(const SkGlyph& glyph) = 0;

    






    virtual void generatePath(const SkGlyph& glyph, SkPath* path) = 0;

    
    virtual void generateFontMetrics(SkPaint::FontMetrics*) = 0;

    
    virtual unsigned generateGlyphCount() = 0;

    


    virtual uint16_t generateCharToGlyph(SkUnichar unichar) = 0;

    



    virtual SkUnichar generateGlyphToChar(uint16_t glyphId);

    void forceGenerateImageFromPath() { fGenerateImageFromPath = true; }

private:
    
    SkAutoTUnref<SkTypeface> fTypeface;

#ifdef SK_BUILD_FOR_ANDROID
    SkPaintOptionsAndroid fPaintOptionsAndroid;
#endif

    
    SkPathEffect*   fPathEffect;
    SkMaskFilter*   fMaskFilter;
    SkRasterizer*   fRasterizer;

    
    
    bool fGenerateImageFromPath;

    void internalGetPath(const SkGlyph& glyph, SkPath* fillPath,
                         SkPath* devPath, SkMatrix* fillToDevMatrix);

    
    
    SkScalerContext* allocNextContext() const;

    
    SkScalerContext* getNextContext();

    
    
    SkScalerContext* getGlyphContext(const SkGlyph& glyph);

    
    
    
    SkScalerContext* getContextFromChar(SkUnichar uni, uint16_t* glyphID);

    
    SkScalerContext* fNextContext;

    
protected:
    
    const SkMaskGamma::PreBlend fPreBlend;
private:
    
    
    const SkMaskGamma::PreBlend fPreBlendForFilter;
};

#define kRec_SkDescriptorTag            SkSetFourByteTag('s', 'r', 'e', 'c')
#define kPathEffect_SkDescriptorTag     SkSetFourByteTag('p', 't', 'h', 'e')
#define kMaskFilter_SkDescriptorTag     SkSetFourByteTag('m', 's', 'k', 'f')
#define kRasterizer_SkDescriptorTag     SkSetFourByteTag('r', 'a', 's', 't')
#ifdef SK_BUILD_FOR_ANDROID
#define kAndroidOpts_SkDescriptorTag    SkSetFourByteTag('a', 'n', 'd', 'r')
#endif



enum SkAxisAlignment {
    kNone_SkAxisAlignment,
    kX_SkAxisAlignment,
    kY_SkAxisAlignment
};







SkAxisAlignment SkComputeAxisAlignmentForHText(const SkMatrix& matrix);



SkPaint::Hinting SkScalerContextRec::getHinting() const {
    unsigned hint = (fFlags & SkScalerContext::kHinting_Mask) >>
                                            SkScalerContext::kHinting_Shift;
    return static_cast<SkPaint::Hinting>(hint);
}

void SkScalerContextRec::setHinting(SkPaint::Hinting hinting) {
    fFlags = (fFlags & ~SkScalerContext::kHinting_Mask) |
                                (hinting << SkScalerContext::kHinting_Shift);
}


#endif
