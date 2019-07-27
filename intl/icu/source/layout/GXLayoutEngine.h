






#ifndef __GXLAYOUTENGINE_H
#define __GXLAYOUTENGINE_H

#include "LETypes.h"
#include "LayoutEngine.h"

#include "MorphTables.h"

U_NAMESPACE_BEGIN

class LEFontInstance;
class LEGlyphStorage;









class GXLayoutEngine : public LayoutEngine
{
public:
    



















    GXLayoutEngine(const LEFontInstance *fontInstance, le_int32 scriptCode, le_int32 languageCode, const LEReferenceTo<MorphTableHeader> &morphTable, LEErrorCode &success);

    




    virtual ~GXLayoutEngine();

    




    virtual UClassID getDynamicClassID() const;

    




    static UClassID getStaticClassID();

protected:

    




    LEReferenceTo<MorphTableHeader> fMorphTable;

    



















    virtual le_int32 computeGlyphs(const LEUnicode chars[], le_int32 offset, le_int32 count, le_int32 max, le_bool rightToLeft,
        LEGlyphStorage &glyphStorage, LEErrorCode &success);

    











    virtual void adjustGlyphPositions(const LEUnicode chars[], le_int32 offset, le_int32 count, le_bool reverse,
                                      LEGlyphStorage &glyphStorage, LEErrorCode &success);

};

U_NAMESPACE_END
#endif

