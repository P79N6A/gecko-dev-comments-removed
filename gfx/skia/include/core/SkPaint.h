








#ifndef SkPaint_DEFINED
#define SkPaint_DEFINED

#include "SkColor.h"
#include "SkXfermode.h"

class SkAutoGlyphCache;
class SkColorFilter;
class SkDescriptor;
class SkFlattenableReadBuffer;
class SkFlattenableWriteBuffer;
struct SkGlyph;
struct SkRect;
class SkGlyphCache;
class SkMaskFilter;
class SkMatrix;
class SkPath;
class SkPathEffect;
class SkRasterizer;
class SkShader;
class SkDrawLooper;
class SkTypeface;

typedef const SkGlyph& (*SkDrawCacheProc)(SkGlyphCache*, const char**,
                                           SkFixed x, SkFixed y);

typedef const SkGlyph& (*SkMeasureCacheProc)(SkGlyphCache*, const char**);






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

    void flatten(SkFlattenableWriteBuffer&) const;
    void unflatten(SkFlattenableReadBuffer&);

    

    void reset();

    









    enum Hinting {
        kNo_Hinting            = 0,
        kSlight_Hinting        = 1,
        kNormal_Hinting        = 2,     
        kFull_Hinting          = 3,
    };

    Hinting getHinting() const {
        return static_cast<Hinting>(fHinting);
    }

    void setHinting(Hinting hintingLevel);

    

    enum Flags {
        kAntiAlias_Flag       = 0x01,   
        kFilterBitmap_Flag    = 0x02,   
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

        
        kForceAAText_Flag     = 0x1000,

        
        

        kAllFlags = 0x1FFF
    };

    


    uint32_t getFlags() const { return fFlags; }

    


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

    bool isFilterBitmap() const {
        return SkToBool(this->getFlags() & kFilterBitmap_Flag);
    }

    void setFilterBitmap(bool filterBitmap);

    








    enum Style {
        kFill_Style,            
        kStroke_Style,          
        kStrokeAndFill_Style,   

        kStyleCount,
    };

    




    Style getStyle() const { return (Style)fStyle; }

    




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

    




    Cap getStrokeCap() const { return (Cap)fCapType; }

    



    void setStrokeCap(Cap cap);

    



    Join getStrokeJoin() const { return (Join)fJoinType; }

    



    void setStrokeJoin(Join join);

    








    bool getFillPath(const SkPath& src, SkPath* dst) const;

    



    bool canComputeFastBounds() const {
        
        return (reinterpret_cast<uintptr_t>(this->getMaskFilter()) |
                reinterpret_cast<uintptr_t>(this->getLooper()) |
                reinterpret_cast<uintptr_t>(this->getRasterizer()) |
                reinterpret_cast<uintptr_t>(this->getPathEffect())) == 0;
    }

    




















    const SkRect& computeFastBounds(const SkRect& orig, SkRect* storage) const {
        return this->getStyle() == kFill_Style ? orig :
                    this->computeStrokeFastBounds(orig, storage);
    }

    




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

    



    SkDrawLooper* getLooper() const { return fLooper; }

    










    SkDrawLooper* setLooper(SkDrawLooper* looper);

    enum Align {
        kLeft_Align,
        kCenter_Align,
        kRight_Align,

        kAlignCount
    };

    


    Align   getTextAlign() const { return (Align)fTextAlign; }

    


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
        kGlyphID_TextEncoding   
    };

    TextEncoding getTextEncoding() const { return (TextEncoding)fTextEncoding; }

    void setTextEncoding(TextEncoding encoding);

    struct FontMetrics {
        SkScalar    fTop;       
        SkScalar    fAscent;    
        SkScalar    fDescent;   
        SkScalar    fBottom;    
        SkScalar    fLeading;   
        SkScalar    fAvgCharWidth;  
        SkScalar    fXMin;      
        SkScalar    fXMax;      
        SkScalar    fXHeight;   
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

#ifdef ANDROID
    const SkGlyph& getUnicharMetrics(SkUnichar);
    const void* findImage(const SkGlyph&);

    uint32_t getGenerationID() const;
#endif

    
    
    bool nothingToDraw() const;

private:
    SkTypeface*     fTypeface;
    SkScalar        fTextSize;
    SkScalar        fTextScaleX;
    SkScalar        fTextSkewX;

    SkPathEffect*   fPathEffect;
    SkShader*       fShader;
    SkXfermode*     fXfermode;
    SkMaskFilter*   fMaskFilter;
    SkColorFilter*  fColorFilter;
    SkRasterizer*   fRasterizer;
    SkDrawLooper*   fLooper;

    SkColor         fColor;
    SkScalar        fWidth;
    SkScalar        fMiterLimit;
    unsigned        fFlags : 13;
    unsigned        fTextAlign : 2;
    unsigned        fCapType : 2;
    unsigned        fJoinType : 2;
    unsigned        fStyle : 2;
    unsigned        fTextEncoding : 2;  
    unsigned        fHinting : 2;
#ifdef ANDROID
    uint32_t        fGenerationID;
#endif

    SkDrawCacheProc    getDrawCacheProc() const;
    SkMeasureCacheProc getMeasureCacheProc(TextBufferDirection dir,
                                           bool needFullMetrics) const;

    SkScalar measure_text(SkGlyphCache*, const char* text, size_t length,
                          int* count, SkRect* bounds) const;

    SkGlyphCache*   detachCache(const SkMatrix*) const;

    void descriptorProc(const SkMatrix* deviceMatrix,
                        void (*proc)(const SkDescriptor*, void*),
                        void* context, bool ignoreGamma = false) const;

    const SkRect& computeStrokeFastBounds(const SkRect& orig,
                                          SkRect* storage) const;

    enum {
        kCanonicalTextSizeForPaths = 64
    };
    friend class SkAutoGlyphCache;
    friend class SkCanvas;
    friend class SkDraw;
    friend class SkPDFDevice;
    friend class SkTextToPathIter;
};



#include "SkPathEffect.h"








class SkStrokePathEffect : public SkPathEffect {
public:
    SkStrokePathEffect(const SkPaint&);
    SkStrokePathEffect(SkScalar width, SkPaint::Style, SkPaint::Join,
                       SkPaint::Cap, SkScalar miterLimit = -1);

    
    virtual bool filterPath(SkPath* dst, const SkPath& src, SkScalar* width);

    
    virtual void flatten(SkFlattenableWriteBuffer&);
    virtual Factory getFactory();

    static SkFlattenable* CreateProc(SkFlattenableReadBuffer&);

private:
    SkScalar    fWidth, fMiter;
    uint8_t     fStyle, fJoin, fCap;

    SkStrokePathEffect(SkFlattenableReadBuffer&);

    typedef SkPathEffect INHERITED;

    
    SkStrokePathEffect(const SkStrokePathEffect&);
    SkStrokePathEffect& operator=(const SkStrokePathEffect&);
};

#endif

