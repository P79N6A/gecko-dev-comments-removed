








#ifndef SkTypeface_DEFINED
#define SkTypeface_DEFINED

#include "SkAdvancedTypefaceMetrics.h"
#include "SkRefCnt.h"

class SkStream;
class SkAdvancedTypefaceMetrics;
class SkWStream;

typedef uint32_t SkFontID;










class SK_API SkTypeface : public SkRefCnt {
public:
    

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

    








    static SkTypeface* CreateForChars(const void* data, size_t bytelength,
                                      Style s);

    









    static SkTypeface* CreateFromTypeface(const SkTypeface* family, Style s);

    


    static SkTypeface* CreateFromFile(const char path[]);

    



    static SkTypeface* CreateFromStream(SkStream* stream);

    


    void serialize(SkWStream*) const;

    




    static SkTypeface* Deserialize(SkStream*);

    









    SkAdvancedTypefaceMetrics* getAdvancedTypefaceMetrics(
            SkAdvancedTypefaceMetrics::PerGlyphInfo perGlyphInfo,
            const uint32_t* glyphIDs = NULL,
            uint32_t glyphIDsCount = 0) const;

protected:
    

    SkTypeface(Style style, SkFontID uniqueID, bool isFixedWidth = false);
    virtual ~SkTypeface();

private:
    SkFontID    fUniqueID;
    Style       fStyle;
    bool        fIsFixedWidth;

    typedef SkRefCnt INHERITED;
};

#endif
