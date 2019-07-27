









#ifndef SkPaint_DEFINED
#define SkPaint_DEFINED

#include "SkColor.h"
#include "SkDrawLooper.h"
#include "SkMatrix.h"
#include "SkXfermode.h"
#ifdef SK_BUILD_FOR_ANDROID
#include "SkPaintOptionsAndroid.h"
#endif

class SkAnnotation;
class SkAutoGlyphCache;
class SkColorFilter;
class SkDescriptor;
struct SkDeviceProperties;
class SkReadBuffer;
class SkWriteBuffer;
struct SkGlyph;
struct SkRect;
class SkGlyphCache;
class SkImageFilter;
class SkMaskFilter;
class SkPath;
class SkPathEffect;
struct SkPoint;
class SkRasterizer;
class SkShader;
class SkTypeface;

typedef const SkGlyph& (*SkDrawCacheProc)(SkGlyphCache*, const char**,
                                           SkFixed x, SkFixed y);

typedef const SkGlyph& (*SkMeasureCacheProc)(SkGlyphCache*, const char**);

#define kBicubicFilterBitmap_Flag kHighQualityFilterBitmap_Flag







class SK_API SkPaint {
public:
    SkPaint();
    SkPaint(const SkPaint& paint);
    ~SkPaint();

    SkPaint& operator=(const SkPaint&);

    SK_API friend bool operator==(const SkPaint& a, const SkPaint& b);
    friend bool operator!=(const SkPaint& a, const SkPaint& b) {
        return !(a == b);
    }

    void flatten(SkWriteBuffer&) const;
    void unflatten(SkReadBuffer&);

    

    void reset();

    









    enum Hinting {
        kNo_Hinting            = 0,
        kSlight_Hinting        = 1,
        kNormal_Hinting        = 2,     
        kFull_Hinting          = 3
    };

    Hinting getHinting() const {
        return static_cast<Hinting>(fBitfields.fHinting);
    }

    void setHinting(Hinting hintingLevel);

    

    enum Flags {
        kAntiAlias_Flag       = 0x01,   
        kDither_Flag          = 0x04,   
        kUnderlineText_Flag   = 0x08,   
        kStrikeThruText_Flag  = 0x10,   
        kFakeBoldText_Flag    = 0x20,   
        kLinearText_Flag      = 0x40,   
        kSubpixelText_Flag    = 0x80,   
        kDevKernText_Flag     = 0x100,  
        kLCDRenderText_Flag   = 0x200,  
        kEmbeddedBitmapText_Flag = 0x400, 
        kAutoHinting_Flag     = 0x800,  
        kVerticalText_Flag    = 0x1000,
        kGenA8FromLCD_Flag    = 0x2000, 
        kDistanceFieldTextTEMP_Flag = 0x4000, 
                                              
        
        

        kAllFlags = 0xFFFF
    };

    


    uint32_t getFlags() const { return fBitfields.fFlags; }

    


    void setFlags(uint32_t flags);

    


    bool isAntiAlias() const {
        return SkToBool(this->getFlags() & kAntiAlias_Flag);
    }

    


    void setAntiAlias(bool aa);

    


    bool isDither() const {
        return SkToBool(this->getFlags() & kDither_Flag);
    }

    


    void setDither(bool dither);

    


    bool isLinearText() const {
        return SkToBool(this->getFlags() & kLinearText_Flag);
    }

    



    void setLinearText(bool linearText);

    


    bool isSubpixelText() const {
        return SkToBool(this->getFlags() & kSubpixelText_Flag);
    }

    




    void setSubpixelText(bool subpixelText);

    bool isLCDRenderText() const {
        return SkToBool(this->getFlags() & kLCDRenderText_Flag);
    }

    





    void setLCDRenderText(bool lcdText);

    bool isEmbeddedBitmapText() const {
        return SkToBool(this->getFlags() & kEmbeddedBitmapText_Flag);
    }

    



    void setEmbeddedBitmapText(bool useEmbeddedBitmapText);

    bool isAutohinted() const {
        return SkToBool(this->getFlags() & kAutoHinting_Flag);
    }

    




    void setAutohinted(bool useAutohinter);

    bool isVerticalText() const {
        return SkToBool(this->getFlags() & kVerticalText_Flag);
    }

    







    void setVerticalText(bool);

    


    bool isUnderlineText() const {
        return SkToBool(this->getFlags() & kUnderlineText_Flag);
    }

    



    void setUnderlineText(bool underlineText);

    


    bool isStrikeThruText() const {
        return SkToBool(this->getFlags() & kStrikeThruText_Flag);
    }

    



    void setStrikeThruText(bool strikeThruText);

    


    bool isFakeBoldText() const {
        return SkToBool(this->getFlags() & kFakeBoldText_Flag);
    }

    



    void setFakeBoldText(bool fakeBoldText);

    


    bool isDevKernText() const {
        return SkToBool(this->getFlags() & kDevKernText_Flag);
    }

    



    void setDevKernText(bool devKernText);

    


    bool isDistanceFieldTextTEMP() const {
        return SkToBool(this->getFlags() & kDistanceFieldTextTEMP_Flag);
    }

    



    void setDistanceFieldTextTEMP(bool distanceFieldText);

    enum FilterLevel {
        kNone_FilterLevel,
        kLow_FilterLevel,
        kMedium_FilterLevel,
        kHigh_FilterLevel
    };

    



    FilterLevel getFilterLevel() const {
      return (FilterLevel)fBitfields.fFilterLevel;
    }

    



    void setFilterLevel(FilterLevel);

    



    SK_ATTR_DEPRECATED("use setFilterLevel")
    void setFilterBitmap(bool doFilter) {
        this->setFilterLevel(doFilter ? kLow_FilterLevel : kNone_FilterLevel);
    }

    


    SK_ATTR_DEPRECATED("use getFilterLevel")
    bool isFilterBitmap() const {
        return kNone_FilterLevel != this->getFilterLevel();
    }

    








    enum Style {
        kFill_Style,            
        kStroke_Style,          
        kStrokeAndFill_Style,   
    };
    enum {
        kStyleCount = kStrokeAndFill_Style + 1
    };

    




    Style getStyle() const { return (Style)fBitfields.fStyle; }

    




    void setStyle(Style style);

    





    SkColor getColor() const { return fColor; }

    




    void setColor(SkColor color);

    


    uint8_t getAlpha() const { return SkToU8(SkColorGetA(fColor)); }

    



    void setAlpha(U8CPU a);

    






    void setARGB(U8CPU a, U8CPU r, U8CPU g, U8CPU b);

    






    SkScalar getStrokeWidth() const { return fWidth; }

    





    void setStrokeWidth(SkScalar width);

    




    SkScalar getStrokeMiter() const { return fMiterLimit; }

    





    void setStrokeMiter(SkScalar miter);

    



    enum Cap {
        kButt_Cap,      
        kRound_Cap,     
        kSquare_Cap,    

        kCapCount,
        kDefault_Cap = kButt_Cap
    };

    


    enum Join {
        kMiter_Join,    
        kRound_Join,    
        kBevel_Join,    

        kJoinCount,
        kDefault_Join = kMiter_Join
    };

    




    Cap getStrokeCap() const { return (Cap)fBitfields.fCapType; }

    



    void setStrokeCap(Cap cap);

    



    Join getStrokeJoin() const { return (Join)fBitfields.fJoinType; }

    



    void setStrokeJoin(Join join);

    











    bool getFillPath(const SkPath& src, SkPath* dst,
                     const SkRect* cullRect = NULL) const;

    




    SkShader* getShader() const { return fShader; }

    




















    SkShader* setShader(SkShader* shader);

    



    SkColorFilter* getColorFilter() const { return fColorFilter; }

    






    SkColorFilter* setColorFilter(SkColorFilter* filter);

    




    SkXfermode* getXfermode() const { return fXfermode; }

    









    SkXfermode* setXfermode(SkXfermode* xfermode);

    



    SkXfermode* setXfermodeMode(SkXfermode::Mode);

    




    SkPathEffect* getPathEffect() const { return fPathEffect; }

    









    SkPathEffect* setPathEffect(SkPathEffect* effect);

    




    SkMaskFilter* getMaskFilter() const { return fMaskFilter; }

    









    SkMaskFilter* setMaskFilter(SkMaskFilter* maskfilter);

    

    





    SkTypeface* getTypeface() const { return fTypeface; }

    









    SkTypeface* setTypeface(SkTypeface* typeface);

    




    SkRasterizer* getRasterizer() const { return fRasterizer; }

    










    SkRasterizer* setRasterizer(SkRasterizer* rasterizer);

    SkImageFilter* getImageFilter() const { return fImageFilter; }
    SkImageFilter* setImageFilter(SkImageFilter*);

    SkAnnotation* getAnnotation() const { return fAnnotation; }
    SkAnnotation* setAnnotation(SkAnnotation*);

    



    SK_ATTR_DEPRECATED("use getAnnotation and check for non-null")
    bool isNoDrawAnnotation() const { return this->getAnnotation() != NULL; }

    



    SkDrawLooper* getLooper() const { return fLooper; }

    










    SkDrawLooper* setLooper(SkDrawLooper* looper);

    enum Align {
        kLeft_Align,
        kCenter_Align,
        kRight_Align,
    };
    enum {
        kAlignCount = 3
    };

    


    Align   getTextAlign() const { return (Align)fBitfields.fTextAlign; }

    


    void    setTextAlign(Align align);

    


    SkScalar getTextSize() const { return fTextSize; }

    


    void setTextSize(SkScalar textSize);

    



    SkScalar getTextScaleX() const { return fTextScaleX; }

    





    void setTextScaleX(SkScalar scaleX);

    



    SkScalar getTextSkewX() const { return fTextSkewX; }

    



    void setTextSkewX(SkScalar skewX);

    


    enum TextEncoding {
        kUTF8_TextEncoding,     
        kUTF16_TextEncoding,    
        kUTF32_TextEncoding,    
        kGlyphID_TextEncoding   
    };

    TextEncoding getTextEncoding() const {
      return (TextEncoding)fBitfields.fTextEncoding;
    }

    void setTextEncoding(TextEncoding encoding);

    struct FontMetrics {
        


        enum FontMetricsFlags {
            kUnderlineThinknessIsValid_Flag = 1 << 0,
            kUnderlinePositionIsValid_Flag = 1 << 1,
        };

        uint32_t    fFlags;       
        SkScalar    fTop;       
        SkScalar    fAscent;    
        SkScalar    fDescent;   
        SkScalar    fBottom;    
        SkScalar    fLeading;   
        SkScalar    fAvgCharWidth;  
        SkScalar    fMaxCharWidth;  
        SkScalar    fXMin;      
        SkScalar    fXMax;      
        SkScalar    fXHeight;   
        SkScalar    fCapHeight;  
        SkScalar    fUnderlineThickness; 

        





        SkScalar    fUnderlinePosition; 

        



        bool hasUnderlineThickness(SkScalar* thickness) const {
            if (SkToBool(fFlags & kUnderlineThinknessIsValid_Flag)) {
                *thickness = fUnderlineThickness;
                return true;
            }
            return false;
        }

        



        bool hasUnderlinePosition(SkScalar* position) const {
            if (SkToBool(fFlags & kUnderlinePositionIsValid_Flag)) {
                *position = fUnderlinePosition;
                return true;
            }
            return false;
        }

    };

    










    SkScalar getFontMetrics(FontMetrics* metrics, SkScalar scale = 0) const;

    


    SkScalar getFontSpacing() const { return this->getFontMetrics(NULL, 0); }

    



    int textToGlyphs(const void* text, size_t byteLength,
                     uint16_t glyphs[]) const;

    






    bool containsText(const void* text, size_t byteLength) const;

    



    void glyphsToUnichars(const uint16_t glyphs[], int count,
                          SkUnichar text[]) const;

    




    int countText(const void* text, size_t byteLength) const {
        return this->textToGlyphs(text, byteLength, NULL);
    }

    











    SkScalar measureText(const void* text, size_t length,
                         SkRect* bounds, SkScalar scale = 0) const;

    







    SkScalar measureText(const void* text, size_t length) const {
        return this->measureText(text, length, NULL, 0);
    }

    

    enum TextBufferDirection {
        


        kForward_TextBufferDirection,
        


        kBackward_TextBufferDirection
    };

    














    size_t  breakText(const void* text, size_t length, SkScalar maxWidth,
                      SkScalar* measuredWidth = NULL,
                      TextBufferDirection tbd = kForward_TextBufferDirection)
                      const;

    











    int getTextWidths(const void* text, size_t byteLength, SkScalar widths[],
                      SkRect bounds[] = NULL) const;

    



    void getTextPath(const void* text, size_t length, SkScalar x, SkScalar y,
                     SkPath* path) const;

    void getPosTextPath(const void* text, size_t length,
                        const SkPoint pos[], SkPath* path) const;

#ifdef SK_BUILD_FOR_ANDROID
    uint32_t getGenerationID() const;
    void setGenerationID(uint32_t generationID);

    

    unsigned getBaseGlyphCount(SkUnichar text) const;

    const SkPaintOptionsAndroid& getPaintOptionsAndroid() const {
        return fPaintOptionsAndroid;
    }
    void setPaintOptionsAndroid(const SkPaintOptionsAndroid& options);
#endif

    
    
    bool nothingToDraw() const;

    
    

    



    bool canComputeFastBounds() const {
        if (this->getLooper()) {
            return this->getLooper()->canComputeFastBounds(*this);
        }
        return !this->getRasterizer();
    }

    




















    const SkRect& computeFastBounds(const SkRect& orig, SkRect* storage) const {
        SkPaint::Style style = this->getStyle();
        
        if (kFill_Style == style) {
            uintptr_t effects = reinterpret_cast<uintptr_t>(this->getLooper());
            effects |= reinterpret_cast<uintptr_t>(this->getMaskFilter());
            effects |= reinterpret_cast<uintptr_t>(this->getPathEffect());
            effects |= reinterpret_cast<uintptr_t>(this->getImageFilter());
            if (!effects) {
                return orig;
            }
        }

        return this->doComputeFastBounds(orig, storage, style);
    }

    const SkRect& computeFastStrokeBounds(const SkRect& orig,
                                          SkRect* storage) const {
        return this->doComputeFastBounds(orig, storage, kStroke_Style);
    }

    
    
    const SkRect& doComputeFastBounds(const SkRect& orig, SkRect* storage,
                                      Style) const;

    


    static SkMatrix* SetTextMatrix(SkMatrix* matrix, SkScalar size,
                                   SkScalar scaleX, SkScalar skewX) {
        matrix->setScale(size * scaleX, size);
        if (skewX) {
            matrix->postSkew(skewX, 0);
        }
        return matrix;
    }

    SkMatrix* setTextMatrix(SkMatrix* matrix) const {
        return SetTextMatrix(matrix, fTextSize, fTextScaleX, fTextSkewX);
    }

    SK_TO_STRING_NONVIRT()

    struct FlatteningTraits {
        static void Flatten(SkWriteBuffer& buffer, const SkPaint& paint);
        static void Unflatten(SkReadBuffer& buffer, SkPaint* paint);
    };

private:
    SkTypeface*     fTypeface;
    SkPathEffect*   fPathEffect;
    SkShader*       fShader;
    SkXfermode*     fXfermode;
    SkMaskFilter*   fMaskFilter;
    SkColorFilter*  fColorFilter;
    SkRasterizer*   fRasterizer;
    SkDrawLooper*   fLooper;
    SkImageFilter*  fImageFilter;
    SkAnnotation*   fAnnotation;

    SkScalar        fTextSize;
    SkScalar        fTextScaleX;
    SkScalar        fTextSkewX;
    SkColor         fColor;
    SkScalar        fWidth;
    SkScalar        fMiterLimit;
    union {
        struct {
            
            unsigned        fFlags : 16;
            unsigned        fTextAlign : 2;
            unsigned        fCapType : 2;
            unsigned        fJoinType : 2;
            unsigned        fStyle : 2;
            unsigned        fTextEncoding : 2;  
            unsigned        fHinting : 2;
            unsigned        fFilterLevel : 2;
            
        } fBitfields;
        uint32_t fBitfieldsUInt;
    };
    uint32_t fDirtyBits;

    SkDrawCacheProc    getDrawCacheProc() const;
    SkMeasureCacheProc getMeasureCacheProc(TextBufferDirection dir,
                                           bool needFullMetrics) const;

    SkScalar measure_text(SkGlyphCache*, const char* text, size_t length,
                          int* count, SkRect* bounds) const;

    SkGlyphCache* detachCache(const SkDeviceProperties* deviceProperties, const SkMatrix*,
                              bool ignoreGamma) const;

    void descriptorProc(const SkDeviceProperties* deviceProperties, const SkMatrix* deviceMatrix,
                        void (*proc)(SkTypeface*, const SkDescriptor*, void*),
                        void* context, bool ignoreGamma = false) const;

    static void Term();

    enum {
        













        kCanonicalTextSizeForPaths  = 64,

        




        kMaxSizeForGlyphCache       = 256,
    };

    static bool TooBigToUseCache(const SkMatrix& ctm, const SkMatrix& textM);

    
    
    
    SkScalar setupForAsPaths();

    static SkScalar MaxCacheSize2() {
        static const SkScalar kMaxSize = SkIntToScalar(kMaxSizeForGlyphCache);
        static const SkScalar kMag2Max = kMaxSize * kMaxSize;
        return kMag2Max;
    }

    friend class SkAutoGlyphCache;
    friend class SkAutoGlyphCacheNoGamma;
    friend class SkCanvas;
    friend class SkDraw;
    friend class SkGraphics; 
    friend class SkPDFDevice;
    friend class GrBitmapTextContext;
    friend class GrDistanceFieldTextContext;
    friend class GrStencilAndCoverTextContext;
    friend class SkTextToPathIter;
    friend class SkCanonicalizePaint;

#ifdef SK_BUILD_FOR_ANDROID
    SkPaintOptionsAndroid fPaintOptionsAndroid;

    
    
    uint32_t        fGenerationID;
#endif
};

#endif
