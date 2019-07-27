






#ifndef __THAILAYOUTENGINE_H
#define __THAILAYOUTENGINE_H

#include "LETypes.h"
#include "LEFontInstance.h"
#include "LayoutEngine.h"

#include "ThaiShaping.h"

U_NAMESPACE_BEGIN

class LEGlyphStorage;









class ThaiLayoutEngine : public LayoutEngine
{
public:
    














    ThaiLayoutEngine(const LEFontInstance *fontInstance, le_int32 scriptCode, le_int32 languageCode, le_int32 typoFlags, LEErrorCode &success);

    




    virtual ~ThaiLayoutEngine();

    




    virtual UClassID getDynamicClassID() const;

    




    static UClassID getStaticClassID();

protected:
    







    le_uint8 fGlyphSet;

    









    LEUnicode fErrorChar;

    





















    virtual le_int32 computeGlyphs(const LEUnicode chars[], le_int32 offset, le_int32 count, le_int32 max, le_bool rightToLeft,
        LEGlyphStorage &glyphStorage, LEErrorCode &success);

    



















    virtual void adjustGlyphPositions(const LEUnicode chars[], le_int32 offset, le_int32 count, le_bool reverse, LEGlyphStorage &glyphStorage, LEErrorCode &success);

};

U_NAMESPACE_END
#endif

