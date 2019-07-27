





#ifndef __GXLAYOUTENGINE2_H
#define __GXLAYOUTENGINE2_H

#include "LETypes.h"
#include "LayoutEngine.h"

#include "MorphTables.h"

U_NAMESPACE_BEGIN

class LEFontInstance;
class LEGlyphStorage;









class GXLayoutEngine2 : public LayoutEngine
{
public:
    



















    GXLayoutEngine2(const LEFontInstance *fontInstance, le_int32 scriptCode, le_int32 languageCode, const LEReferenceTo<MorphTableHeader2> &morphTable, le_int32 typoFlags, LEErrorCode &success);

    




    virtual ~GXLayoutEngine2();

    




    virtual UClassID getDynamicClassID() const;

    




    static UClassID getStaticClassID();

protected:

    




    const LEReferenceTo<MorphTableHeader2> fMorphTable;

    



















    virtual le_int32 computeGlyphs(const LEUnicode chars[], le_int32 offset, le_int32 count, le_int32 max, le_bool rightToLeft,
        LEGlyphStorage &glyphStorage, LEErrorCode &success);

    











    virtual void adjustGlyphPositions(const LEUnicode chars[], le_int32 offset, le_int32 count, le_bool reverse,
                                      LEGlyphStorage &glyphStorage, LEErrorCode &success);

};

U_NAMESPACE_END
#endif

