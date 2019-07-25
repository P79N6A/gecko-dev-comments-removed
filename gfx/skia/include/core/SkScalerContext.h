








#ifndef SkScalerContext_DEFINED
#define SkScalerContext_DEFINED

#include "SkMask.h"
#include "SkMatrix.h"
#include "SkPaint.h"
#include "SkPath.h"
#include "SkPoint.h"

class SkDescriptor;
class SkMaskFilter;
class SkPathEffect;
class SkRasterizer;


#define MASK_FORMAT_UNKNOWN         (0xFF)
#define MASK_FORMAT_JUST_ADVANCE    MASK_FORMAT_UNKNOWN

#define kMaxGlyphWidth (1<<13)

struct SkGlyph {
    void*       fImage;
    SkPath*     fPath;
    SkFixed     fAdvanceX, fAdvanceY;

    uint32_t    fID;
    uint16_t    fWidth, fHeight;
    int16_t     fTop, fLeft;

    uint8_t     fMaskFormat;
    int8_t      fRsbDelta, fLsbDelta;  

    void init(uint32_t id) {
        fID             = id;
        fImage          = NULL;
        fPath           = NULL;
        fMaskFormat     = MASK_FORMAT_UNKNOWN;
    }

    


    static unsigned ComputeRowBytes(unsigned width, SkMask::Format format) {
        unsigned rb = width;
        if (SkMask::kBW_Format == format) {
            rb = (rb + 7) >> 3;
		} else if (SkMask::kARGB32_Format == format ||
                   SkMask::kLCD32_Format == format)
        {
			rb <<= 2;
		} else if (SkMask::kLCD16_Format == format) {
			rb = SkAlign4(rb << 1);
        } else {
            rb = SkAlign4(rb);
        }
        return rb;
    }

    unsigned rowBytes() const {
        return ComputeRowBytes(fWidth, (SkMask::Format)fMaskFormat);
    }

    bool isJustAdvance() const {
        return MASK_FORMAT_JUST_ADVANCE == fMaskFormat;
    }

    bool isFullMetrics() const {
        return MASK_FORMAT_JUST_ADVANCE != fMaskFormat;
    }

    uint16_t getGlyphID() const {
        return ID2Code(fID);
    }

    unsigned getGlyphID(unsigned baseGlyphCount) const {
        unsigned code = ID2Code(fID);
        SkASSERT(code >= baseGlyphCount);
        return code - baseGlyphCount;
    }

    unsigned getSubX() const {
        return ID2SubX(fID);
    }

    SkFixed getSubXFixed() const {
        return SubToFixed(ID2SubX(fID));
    }

    SkFixed getSubYFixed() const {
        return SubToFixed(ID2SubY(fID));
    }

    size_t computeImageSize() const;

    



    void zeroMetrics();

    enum {
        kSubBits = 2,
        kSubMask = ((1 << kSubBits) - 1),
        kSubShift = 24, 
        kCodeMask = ((1 << kSubShift) - 1),
        
        kSubShiftX = kSubBits,
        kSubShiftY = 0
    };

    static unsigned ID2Code(uint32_t id) {
        return id & kCodeMask;
    }

    static unsigned ID2SubX(uint32_t id) {
        return id >> (kSubShift + kSubShiftX);
    }

    static unsigned ID2SubY(uint32_t id) {
        return (id >> (kSubShift + kSubShiftY)) & kSubMask;
    }

    static unsigned FixedToSub(SkFixed n) {
        return (n >> (16 - kSubBits)) & kSubMask;
    }

    static SkFixed SubToFixed(unsigned sub) {
        SkASSERT(sub <= kSubMask);
        return sub << (16 - kSubBits);
    }

    static uint32_t MakeID(unsigned code) {
        return code;
    }

    static uint32_t MakeID(unsigned code, SkFixed x, SkFixed y) {
        SkASSERT(code <= kCodeMask);
        x = FixedToSub(x);
        y = FixedToSub(y);
        return (x << (kSubShift + kSubShiftX)) |
               (y << (kSubShift + kSubShiftY)) |
               code;
    }

    void toMask(SkMask* mask) const;
};

class SkScalerContext {
public:
    enum Flags {
        kFrameAndFill_Flag        = 0x0001,
        kDevKernText_Flag         = 0x0002,
        kEmbeddedBitmapText_Flag  = 0x0004,
        kEmbolden_Flag            = 0x0008,
        kSubpixelPositioning_Flag = 0x0010,
        kAutohinting_Flag         = 0x0020,
        kVertical_Flag            = 0x0040,

        
        
        kHinting_Shift            = 7, 
        kHintingBit1_Flag         = 0x0080,
        kHintingBit2_Flag         = 0x0100,

        
        kLCD_Vertical_Flag        = 0x0200,    
        kLCD_BGROrder_Flag        = 0x0400,    

        
        kLuminance_Shift          = 11, 
        kLuminance_Bits           = 3  
    };
    
    
    enum {
        kHinting_Mask   = kHintingBit1_Flag | kHintingBit2_Flag,
        kLuminance_Max  = (1 << kLuminance_Bits) - 1,
        kLuminance_Mask = kLuminance_Max << kLuminance_Shift
    };

    struct Rec {
        uint32_t    fOrigFontID;
        uint32_t    fFontID;
        SkScalar    fTextSize, fPreScaleX, fPreSkewX;
        SkScalar    fPost2x2[2][2];
        SkScalar    fFrameWidth, fMiterLimit;
        uint8_t     fMaskFormat;
        uint8_t     fStrokeJoin;
        uint16_t    fFlags;
        
        
        
        

        void    getMatrixFrom2x2(SkMatrix*) const;
        void    getLocalMatrix(SkMatrix*) const;
        void    getSingleMatrix(SkMatrix*) const;

        SkPaint::Hinting getHinting() const {
            unsigned hint = (fFlags & kHinting_Mask) >> kHinting_Shift;
            return static_cast<SkPaint::Hinting>(hint);
        }

        void setHinting(SkPaint::Hinting hinting) {
            fFlags = (fFlags & ~kHinting_Mask) | (hinting << kHinting_Shift);
        }

        unsigned getLuminanceBits() const {
            return (fFlags & kLuminance_Mask) >> kLuminance_Shift;
        }
        
        void setLuminanceBits(unsigned lum) {
            SkASSERT(lum <= kLuminance_Max);
            fFlags = (fFlags & ~kLuminance_Mask) | (lum << kLuminance_Shift);
        }

        U8CPU getLuminanceByte() const {
            SkASSERT(3 == kLuminance_Bits);
            unsigned lum = this->getLuminanceBits();
            lum |= (lum << kLuminance_Bits);
            lum |= (lum << kLuminance_Bits*2);
            return lum >> (4*kLuminance_Bits - 8);
        }

        SkMask::Format getFormat() const {
            return static_cast<SkMask::Format>(fMaskFormat);
        }
    };

    SkScalerContext(const SkDescriptor* desc);
    virtual ~SkScalerContext();

    SkMask::Format getMaskFormat() const {
        return (SkMask::Format)fRec.fMaskFormat;
    }

    bool isSubpixel() const {
        return SkToBool(fRec.fFlags & kSubpixelPositioning_Flag);
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
    void        getFontMetrics(SkPaint::FontMetrics* mX,
                               SkPaint::FontMetrics* mY);

    static inline void MakeRec(const SkPaint&, const SkMatrix*, Rec* rec);
    static SkScalerContext* Create(const SkDescriptor*);

protected:
    Rec         fRec;
    unsigned    fBaseGlyphCount;

    virtual unsigned generateGlyphCount() = 0;
    virtual uint16_t generateCharToGlyph(SkUnichar) = 0;
    virtual void generateAdvance(SkGlyph*) = 0;
    virtual void generateMetrics(SkGlyph*) = 0;
    virtual void generateImage(const SkGlyph&) = 0;
    virtual void generatePath(const SkGlyph&, SkPath*) = 0;
    virtual void generateFontMetrics(SkPaint::FontMetrics* mX,
                                     SkPaint::FontMetrics* mY) = 0;
    
    virtual SkUnichar generateGlyphToChar(uint16_t);

    void forceGenerateImageFromPath() { fGenerateImageFromPath = true; }

private:
    SkPathEffect*   fPathEffect;
    SkMaskFilter*   fMaskFilter;
    SkRasterizer*   fRasterizer;
    SkScalar        fDevFrameWidth;

    
    
    bool fGenerateImageFromPath;

    void internalGetPath(const SkGlyph& glyph, SkPath* fillPath,
                         SkPath* devPath, SkMatrix* fillToDevMatrix);

    
    SkScalerContext* getNextContext();

    
    
    SkScalerContext* getGlyphContext(const SkGlyph& glyph);

    
    SkScalerContext* fNextContext;
};

#define kRec_SkDescriptorTag            SkSetFourByteTag('s', 'r', 'e', 'c')
#define kPathEffect_SkDescriptorTag     SkSetFourByteTag('p', 't', 'h', 'e')
#define kMaskFilter_SkDescriptorTag     SkSetFourByteTag('m', 's', 'k', 'f')
#define kRasterizer_SkDescriptorTag     SkSetFourByteTag('r', 'a', 's', 't')



enum SkAxisAlignment {
    kNone_SkAxisAlignment,
    kX_SkAxisAlignment,
    kY_SkAxisAlignment
};







SkAxisAlignment SkComputeAxisAlignmentForHText(const SkMatrix& matrix);

#endif

