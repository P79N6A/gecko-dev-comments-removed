








#ifndef SkFontHost_DEFINED
#define SkFontHost_DEFINED

#include "SkScalerContext.h"
#include "SkTypeface.h"

class SkDescriptor;
class SkStream;
class SkWStream;

typedef uint32_t SkFontTableTag;






























class SK_API SkFontHost {
public:
    







    static SkTypeface* CreateTypeface(const SkTypeface* familyFace,
                                      const char familyName[],
                                      const void* data, size_t bytelength,
                                      SkTypeface::Style style);

    










    static SkTypeface* CreateTypefaceFromStream(SkStream*);

    



    static SkTypeface* CreateTypefaceFromFile(const char path[]);

    

    



    static bool ValidFontID(SkFontID uniqueID);

    



    static SkStream* OpenStream(SkFontID uniqueID);

    

























    static size_t GetFileName(SkFontID fontID, char path[], size_t length,
                              int32_t* index);

    

    


    static void Serialize(const SkTypeface*, SkWStream*);

    


    static SkTypeface* Deserialize(SkStream*);

    

    

    static SkScalerContext* CreateScalerContext(const SkDescriptor* desc);

    














    static SkFontID NextLogicalFont(SkFontID currFontID, SkFontID origFontID);

    

    









    static void FilterRec(SkScalerContext::Rec* rec);

    

    










    static SkAdvancedTypefaceMetrics* GetAdvancedTypefaceMetrics(
            SkFontID fontID,
            SkAdvancedTypefaceMetrics::PerGlyphInfo perGlyphInfo,
            const uint32_t* glyphIDs,
            uint32_t glyphIDsCount);

    

    static int CountTables(SkFontID);

    



    static int GetTableTags(SkFontID, SkFontTableTag[]);

    

    static size_t GetTableSize(SkFontID, SkFontTableTag);

    



















    static size_t GetTableData(SkFontID fontID, SkFontTableTag tag,
                               size_t offset, size_t length, void* data);

    

    





    static size_t ShouldPurgeFontCache(size_t sizeAllocatedSoFar);

    


    static int ComputeGammaFlag(const SkPaint& paint);

    


    static void GetGammaTables(const uint8_t* tables[2]);

    

    






    enum LCDOrientation {
        kHorizontal_LCDOrientation = 0,    
        kVertical_LCDOrientation   = 1,
    };

    static void SetSubpixelOrientation(LCDOrientation orientation);
    static LCDOrientation GetSubpixelOrientation();

    









    enum LCDOrder {
        kRGB_LCDOrder = 0,    
        kBGR_LCDOrder = 1,
        kNONE_LCDOrder = 2,
    };

    static void SetSubpixelOrder(LCDOrder order);
    static LCDOrder GetSubpixelOrder();

#ifdef ANDROID
    

    





    static uint32_t GetUnitsPerEm(SkFontID fontID);
#endif
};

#endif
