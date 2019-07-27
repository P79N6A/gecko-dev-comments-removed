






#ifndef __LEFONTINSTANCE_H
#define __LEFONTINSTANCE_H

#include "LETypes.h"





U_NAMESPACE_BEGIN










class LECharMapper 
{
public:
    



    virtual ~LECharMapper();

    








    virtual LEUnicode32 mapChar(LEUnicode32 ch) const = 0;
};







class LEGlyphStorage;

























class U_LAYOUT_API LEFontInstance : public UObject
{
public:

    





    virtual ~LEFontInstance();

    


















































    virtual const LEFontInstance *getSubFont(const LEUnicode chars[], le_int32 *offset, le_int32 limit, le_int32 script, LEErrorCode &success) const;

    
    
    

    














    virtual const void* getFontTable(LETag tableTag, size_t &length) const = 0;

    















    virtual le_bool canDisplay(LEUnicode32 ch) const;

    







    virtual le_int32 getUnitsPerEM() const = 0;

    






















    virtual void mapCharsToGlyphs(const LEUnicode chars[], le_int32 offset, le_int32 count, le_bool reverse, const LECharMapper *mapper, le_bool filterZeroWidth, LEGlyphStorage &glyphStorage) const;

    














    virtual LEGlyphID mapCharToGlyph(LEUnicode32 ch, const LECharMapper *mapper, le_bool filterZeroWidth) const;

    













    virtual LEGlyphID mapCharToGlyph(LEUnicode32 ch, const LECharMapper *mapper) const;

    











    virtual LEGlyphID mapCharToGlyph(LEUnicode32 ch) const = 0;

    
    
    

    







    virtual void getGlyphAdvance(LEGlyphID glyph, LEPoint &advance) const = 0;

    











    virtual le_bool getGlyphPoint(LEGlyphID glyph, le_int32 pointNumber, LEPoint &point) const = 0;

    







    virtual float getXPixelsPerEm() const = 0;

    







    virtual float getYPixelsPerEm() const = 0;

    









    virtual float xUnitsToPoints(float xUnits) const;

    









    virtual float yUnitsToPoints(float yUnits) const;

    







    virtual void unitsToPoints(LEPoint &units, LEPoint &points) const;

    









    virtual float xPixelsToUnits(float xPixels) const;

    









    virtual float yPixelsToUnits(float yPixels) const;

    







    virtual void pixelsToUnits(LEPoint &pixels, LEPoint &units) const;

    










    virtual float getScaleFactorX() const = 0;

    









    virtual float getScaleFactorY() const = 0;

    














    virtual void transformFunits(float xFunits, float yFunits, LEPoint &pixels) const;

    









    static inline float fixedToFloat(le_int32 fixed);

    









    static inline le_int32 floatToFixed(float theFloat);

    
    
    
    
    

    







    virtual le_int32 getAscent() const = 0;

    







    virtual le_int32 getDescent() const = 0;

    







    virtual le_int32 getLeading() const = 0;

    









    virtual le_int32 getLineHeight() const;

    




    virtual UClassID getDynamicClassID() const;

    




    static UClassID getStaticClassID();

};

inline float LEFontInstance::fixedToFloat(le_int32 fixed)
{
    return (float) (fixed / 65536.0);
}

inline le_int32 LEFontInstance::floatToFixed(float theFloat)
{
    return (le_int32) (theFloat * 65536.0);
}

U_NAMESPACE_END
#endif
