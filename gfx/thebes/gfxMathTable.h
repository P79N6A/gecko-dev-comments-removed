



#ifndef GFX_MATH_TABLE_H
#define GFX_MATH_TABLE_H

#include "gfxFont.h"

struct Coverage;
struct GlyphAssembly;
struct MATHTableHeader;
struct MathConstants;
struct MathGlyphConstruction;
struct MathGlyphInfo;
struct MathVariants;





class gfxMathTable
{
public:
    






    explicit gfxMathTable(hb_blob_t* aMathTable);

    


    ~gfxMathTable();

    


    int32_t GetMathConstant(gfxFontEntry::MathConstant aConstant);

    



    bool
    GetMathItalicsCorrection(uint32_t aGlyphID, int16_t* aItalicCorrection);

    







    uint32_t GetMathVariantsSize(uint32_t aGlyphID, bool aVertical,
                                 uint16_t aSize);

    













    bool GetMathVariantsParts(uint32_t aGlyphID, bool aVertical,
                              uint32_t aGlyphs[4]);

protected:
    friend class gfxFontEntry;
    
    
    bool HasValidHeaders();

private:
    
    hb_blob_t*    mMathTable;

    
    
    
    
    
    
    const MathGlyphConstruction* mGlyphConstruction;
    uint32_t mGlyphID;
    bool     mVertical;
    void     SelectGlyphConstruction(uint32_t aGlyphID, bool aVertical);

    
    
    
    
    
    
    const MATHTableHeader* GetMATHTableHeader();
    const MathConstants*   GetMathConstants();
    const MathGlyphInfo*   GetMathGlyphInfo();
    const MathVariants*    GetMathVariants();
    const GlyphAssembly*   GetGlyphAssembly(uint32_t aGlyphID, bool aVertical);

    
    
    bool ValidStructure(const char* aStructStart, uint16_t aStructSize);
    bool ValidOffset(const char* aOffsetStart, uint16_t aOffset);

    
    
    int32_t GetCoverageIndex(const Coverage* aCoverage, uint32_t aGlyph);
};

#endif
