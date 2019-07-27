








#ifndef SkTypeface_DEFINED
#define SkTypeface_DEFINED

#include "SkAdvancedTypefaceMetrics.h"
#include "SkWeakRefCnt.h"

class SkDescriptor;
class SkFontDescriptor;
class SkScalerContext;
struct SkScalerContextRec;
class SkStream;
class SkAdvancedTypefaceMetrics;
class SkWStream;

typedef uint32_t SkFontID;

typedef uint32_t SkFontTableTag;










class SK_API SkTypeface : public SkWeakRefCnt {
public:
    SK_DECLARE_INST_COUNT(SkTypeface)

    

    enum Style {
        kNormal = 0,
        kBold   = 0x01,
        kItalic = 0x02,

        
        kBoldItalic = 0x03
    };

    

    Style style() const { return fStyle; }

    

    bool isBold() const { return (fStyle & kBold) != 0; }

    

    bool isItalic() const { return (fStyle & kItalic) != 0; }

    


    bool isFixedPitch() const { return fIsFixedPitch; }

    


    SkFontID uniqueID() const { return fUniqueID; }

    



    static SkFontID UniqueID(const SkTypeface* face);

    


    static bool Equal(const SkTypeface* facea, const SkTypeface* faceb);

    



    static SkTypeface* RefDefault(Style style = SkTypeface::kNormal);

    








    static SkTypeface* CreateFromName(const char familyName[], Style style);

    









    static SkTypeface* CreateFromTypeface(const SkTypeface* family, Style s);

    


    static SkTypeface* CreateFromFile(const char path[]);

    



    static SkTypeface* CreateFromStream(SkStream* stream);

    


    void serialize(SkWStream*) const;

    




    static SkTypeface* Deserialize(SkStream*);

    enum Encoding {
        kUTF8_Encoding,
        kUTF16_Encoding,
        kUTF32_Encoding
    };

    















    int charsToGlyphs(const void* chars, Encoding encoding, uint16_t glyphs[],
                      int glyphCount) const;

    


    int countGlyphs() const;

    
    

    
    int countTables() const;

    




    int getTableTags(SkFontTableTag tags[]) const;

    

    size_t getTableSize(SkFontTableTag) const;

    



















    size_t getTableData(SkFontTableTag tag, size_t offset, size_t length,
                        void* data) const;

    



    int getUnitsPerEm() const;

    



















    bool getKerningPairAdjustments(const uint16_t glyphs[], int count,
                                   int32_t adjustments[]) const;

    struct LocalizedString {
        SkString fString;
        SkString fLanguage;
    };
    class LocalizedStrings : ::SkNoncopyable {
    public:
        virtual ~LocalizedStrings() { }
        virtual bool next(LocalizedString* localizedString) = 0;
        void unref() { SkDELETE(this); }
    };
    




    LocalizedStrings* createFamilyNameIterator() const;

    




    void getFamilyName(SkString* name) const;

    





    SkStream* openStream(int* ttcIndex) const;

    




    SkScalerContext* createScalerContext(const SkDescriptor*,
                                         bool allowFailure = false) const;

    
    void filterRec(SkScalerContextRec* rec) const {
        this->onFilterRec(rec);
    }
    
    void getFontDescriptor(SkFontDescriptor* desc, bool* isLocal) const {
        this->onGetFontDescriptor(desc, isLocal);
    }

protected:
    

    SkTypeface(Style style, SkFontID uniqueID, bool isFixedPitch = false);
    virtual ~SkTypeface();

    
    void setIsFixedPitch(bool isFixedPitch) { fIsFixedPitch = isFixedPitch; }

    friend class SkScalerContext;
    static SkTypeface* GetDefaultTypeface(Style style = SkTypeface::kNormal);

    virtual SkScalerContext* onCreateScalerContext(const SkDescriptor*) const = 0;
    virtual void onFilterRec(SkScalerContextRec*) const = 0;
    virtual SkAdvancedTypefaceMetrics* onGetAdvancedTypefaceMetrics(
                        SkAdvancedTypefaceMetrics::PerGlyphInfo perGlyphInfo,
                        const uint32_t* glyphIDs,
                        uint32_t glyphIDsCount) const = 0;

    virtual SkStream* onOpenStream(int* ttcIndex) const = 0;
    virtual void onGetFontDescriptor(SkFontDescriptor*, bool* isLocal) const = 0;

    virtual int onCharsToGlyphs(const void* chars, Encoding, uint16_t glyphs[],
                                int glyphCount) const = 0;
    virtual int onCountGlyphs() const = 0;

    virtual int onGetUPEM() const = 0;
    virtual bool onGetKerningPairAdjustments(const uint16_t glyphs[], int count,
                                             int32_t adjustments[]) const;

    virtual LocalizedStrings* onCreateFamilyNameIterator() const = 0;

    virtual int onGetTableTags(SkFontTableTag tags[]) const = 0;
    virtual size_t onGetTableData(SkFontTableTag, size_t offset,
                                  size_t length, void* data) const = 0;

private:
    friend class SkGTypeface;
    friend class SkPDFFont;
    friend class SkPDFCIDFont;

    









    SkAdvancedTypefaceMetrics* getAdvancedTypefaceMetrics(
                          SkAdvancedTypefaceMetrics::PerGlyphInfo perGlyphInfo,
                          const uint32_t* glyphIDs = NULL,
                          uint32_t glyphIDsCount = 0) const;

private:
    static SkTypeface* CreateDefault(int style);  
    static void        DeleteDefault(SkTypeface*);

    SkFontID    fUniqueID;
    Style       fStyle;
    bool        fIsFixedPitch;

    friend class SkPaint;
    friend class SkGlyphCache;  
    
    friend class SkFontHost;

    typedef SkWeakRefCnt INHERITED;
};

#endif
