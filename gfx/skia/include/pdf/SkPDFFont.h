








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

class SkPDFGlyphSet : public SkNoncopyable {
public:
    SkPDFGlyphSet();

    void set(const uint16_t* glyphIDs, int numGlyphs);
    bool has(uint16_t glyphID) const;
    void merge(const SkPDFGlyphSet& usage);
    void exportTo(SkTDArray<uint32_t>* glyphIDs) const;

private:
    SkBitSet fBitSet;
};

class SkPDFGlyphSetMap : public SkNoncopyable {
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
        FontGlyphSetPair* next() const;
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
public:
    SK_API virtual ~SkPDFFont();

    SK_API virtual void getResources(SkTDArray<SkPDFObject*>* resourceList);

    


    SK_API SkTypeface* typeface();

    


    SK_API virtual SkAdvancedTypefaceMetrics::FontType getType();

    

    SK_API virtual bool multiByteGlyphs() const = 0;

    

    SK_API bool hasGlyph(uint16_t glyphID);

    







    SK_API size_t glyphsToPDFFontEncoding(uint16_t* glyphIDs, size_t numGlyphs);

    







    SK_API static SkPDFFont* GetFontResource(SkTypeface* typeface,
                                             uint16_t glyphID);

    





    SK_API virtual SkPDFFont* getFontSubset(const SkPDFGlyphSet* usage);

protected:
    
    SkPDFFont(SkAdvancedTypefaceMetrics* fontInfo, SkTypeface* typeface,
              uint16_t glyphID, bool descendantFont);

    
    SkAdvancedTypefaceMetrics* fontInfo();
    void setFontInfo(SkAdvancedTypefaceMetrics* info);
    uint16_t firstGlyphID() const;
    uint16_t lastGlyphID() const;
    void setLastGlyphID(uint16_t glyphID);

    
    void addResource(SkPDFObject* object);

    
    SkPDFDict* getFontDescriptor();
    void setFontDescriptor(SkPDFDict* descriptor);

    
    bool addCommonFontDescriptorEntries(int16_t defaultWidth);

    


    void adjustGlyphRangeForSingleByteEncoding(int16_t glyphID);

    
    
    void populateToUnicodeTable(const SkPDFGlyphSet* subset);

    
    static SkPDFFont* Create(SkAdvancedTypefaceMetrics* fontInfo,
                             SkTypeface* typeface, uint16_t glyphID,
                             SkPDFDict* fontDescriptor);

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

    SkRefPtr<SkTypeface> fTypeface;

    
    
    uint16_t fFirstGlyphID;
    uint16_t fLastGlyphID;
    
    
    SkRefPtr<SkAdvancedTypefaceMetrics> fFontInfo;
    SkTDArray<SkPDFObject*> fResources;
    SkRefPtr<SkPDFDict> fDescriptor;

    SkAdvancedTypefaceMetrics::FontType fFontType;

    
    static SkTDArray<FontRec>& CanonicalFonts();
    static SkMutex& CanonicalFontsMutex();
};

#endif
