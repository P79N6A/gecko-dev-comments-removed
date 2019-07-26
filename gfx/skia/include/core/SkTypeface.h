








#ifndef SkTypeface_DEFINED
#define SkTypeface_DEFINED

#include "SkAdvancedTypefaceMetrics.h"
#include "SkWeakRefCnt.h"

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

    

    bool isFixedWidth() const { return fIsFixedWidth; }

    


    SkFontID uniqueID() const { return fUniqueID; }

    



    static SkFontID UniqueID(const SkTypeface* face);

    


    static bool Equal(const SkTypeface* facea, const SkTypeface* faceb);

    








    static SkTypeface* CreateFromName(const char familyName[], Style style);

    









    static SkTypeface* CreateFromTypeface(const SkTypeface* family, Style s);

    


    static SkTypeface* CreateFromFile(const char path[]);

    



    static SkTypeface* CreateFromStream(SkStream* stream);

    


    void serialize(SkWStream*) const;

    




    static SkTypeface* Deserialize(SkStream*);

    









    SkAdvancedTypefaceMetrics* getAdvancedTypefaceMetrics(
            SkAdvancedTypefaceMetrics::PerGlyphInfo perGlyphInfo,
            const uint32_t* glyphIDs = NULL,
            uint32_t glyphIDsCount = 0) const;

    
    

    
    int countTables() const;

    




    int getTableTags(SkFontTableTag tags[]) const;

    

    size_t getTableSize(SkFontTableTag) const;

    



















    size_t getTableData(SkFontTableTag tag, size_t offset, size_t length,
                        void* data) const;

    



    int getUnitsPerEm() const;

protected:
    

    SkTypeface(Style style, SkFontID uniqueID, bool isFixedWidth = false);
    virtual ~SkTypeface();

private:
    SkFontID    fUniqueID;
    Style       fStyle;
    bool        fIsFixedWidth;

    typedef SkWeakRefCnt INHERITED;
};

#endif
