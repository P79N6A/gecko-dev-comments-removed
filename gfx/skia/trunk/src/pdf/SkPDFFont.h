








#ifndef SkPDFFont_DEFINED
#define SkPDFFont_DEFINED

#include "SkAdvancedTypefaceMetrics.h"
#include "SkBitSet.h"
#include "SkPDFTypes.h"
#include "SkTDArray.h"
#include "SkThread.h"
#include "SkTypeface.h"

class SkPaint;
class SkPDFCatalog;
class SkPDFFont;

class SkPDFGlyphSet : SkNoncopyable {
public:
    SkPDFGlyphSet();

    void set(const uint16_t* glyphIDs, int numGlyphs);
    bool has(uint16_t glyphID) const;
    void merge(const SkPDFGlyphSet& usage);
    void exportTo(SkTDArray<uint32_t>* glyphIDs) const;

private:
    SkBitSet fBitSet;
};

class SkPDFGlyphSetMap : SkNoncopyable {
public:
    struct FontGlyphSetPair {
        FontGlyphSetPair(SkPDFFont* font, SkPDFGlyphSet* glyphSet);

        SkPDFFont* fFont;
        SkPDFGlyphSet* fGlyphSet;
    };

    SkPDFGlyphSetMap();
    ~SkPDFGlyphSetMap();

    class F2BIter {
    public:
        explicit F2BIter(const SkPDFGlyphSetMap& map);
        const FontGlyphSetPair* next() const;
        void reset(const SkPDFGlyphSetMap& map);

    private:
        const SkTDArray<FontGlyphSetPair>* fMap;
        mutable int fIndex;
    };

    void merge(const SkPDFGlyphSetMap& usage);
    void reset();

    void noteGlyphUsage(SkPDFFont* font, const uint16_t* glyphIDs,
                        int numGlyphs);

private:
    SkPDFGlyphSet* getGlyphSetForFont(SkPDFFont* font);

    SkTDArray<FontGlyphSetPair> fMap;
};









class SkPDFFont : public SkPDFDict {
    SK_DECLARE_INST_COUNT(SkPDFFont)
public:
    virtual ~SkPDFFont();

    virtual void getResources(const SkTSet<SkPDFObject*>& knownResourceObjects,
                              SkTSet<SkPDFObject*>* newResourceObjects);

    


    SkTypeface* typeface();

    


    virtual SkAdvancedTypefaceMetrics::FontType getType();

    

    virtual bool multiByteGlyphs() const = 0;

    

    bool canEmbed() const;

    

    bool canSubset() const;

    

    bool hasGlyph(uint16_t glyphID);

    







    int glyphsToPDFFontEncoding(uint16_t* glyphIDs, int numGlyphs);

    







    static SkPDFFont* GetFontResource(SkTypeface* typeface, uint16_t glyphID);

    





    virtual SkPDFFont* getFontSubset(const SkPDFGlyphSet* usage);

protected:
    
    SkPDFFont(const SkAdvancedTypefaceMetrics* fontInfo, SkTypeface* typeface,
              SkPDFDict* relatedFontDescriptor);

    
    const SkAdvancedTypefaceMetrics* fontInfo();
    void setFontInfo(const SkAdvancedTypefaceMetrics* info);
    uint16_t firstGlyphID() const;
    uint16_t lastGlyphID() const;
    void setLastGlyphID(uint16_t glyphID);

    
    void addResource(SkPDFObject* object);

    
    SkPDFDict* getFontDescriptor();
    void setFontDescriptor(SkPDFDict* descriptor);

    
    bool addCommonFontDescriptorEntries(int16_t defaultWidth);

    


    void adjustGlyphRangeForSingleByteEncoding(int16_t glyphID);

    
    
    void populateToUnicodeTable(const SkPDFGlyphSet* subset);

    
    static SkPDFFont* Create(const SkAdvancedTypefaceMetrics* fontInfo,
                             SkTypeface* typeface, uint16_t glyphID,
                             SkPDFDict* relatedFontDescriptor);

    static bool Find(uint32_t fontID, uint16_t glyphID, int* index);

private:
    class FontRec {
    public:
        SkPDFFont* fFont;
        uint32_t fFontID;
        uint16_t fGlyphID;

        
        bool operator==(const FontRec& b) const;
        FontRec(SkPDFFont* font, uint32_t fontID, uint16_t fGlyphID);
    };

    SkAutoTUnref<SkTypeface> fTypeface;

    
    
    uint16_t fFirstGlyphID;
    uint16_t fLastGlyphID;
    SkAutoTUnref<const SkAdvancedTypefaceMetrics> fFontInfo;
    SkTDArray<SkPDFObject*> fResources;
    SkAutoTUnref<SkPDFDict> fDescriptor;

    SkAdvancedTypefaceMetrics::FontType fFontType;

    
    static SkTDArray<FontRec>& CanonicalFonts();
    static SkBaseMutex& CanonicalFontsMutex();
    typedef SkPDFDict INHERITED;
};

#endif
