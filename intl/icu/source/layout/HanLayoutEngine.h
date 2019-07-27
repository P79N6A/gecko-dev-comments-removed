






#ifndef __HANLAYOUTENGINE_H
#define __HANLAYOUTENGINE_H

#include "LETypes.h"
#include "LEFontInstance.h"
#include "LayoutEngine.h"
#include "OpenTypeLayoutEngine.h"

#include "GlyphSubstitutionTables.h"

U_NAMESPACE_BEGIN

class LEGlyphStorage;








class HanOpenTypeLayoutEngine : public OpenTypeLayoutEngine
{
public:
    

















    HanOpenTypeLayoutEngine(const LEFontInstance *fontInstance, le_int32 scriptCode, le_int32 languageCode,
                            le_int32 typoFlags, const LEReferenceTo<GlyphSubstitutionTableHeader> &gsubTablem, LEErrorCode &success);


    




    virtual ~HanOpenTypeLayoutEngine();

    




    virtual UClassID getDynamicClassID() const;

    




    static UClassID getStaticClassID();

protected:

    





















    virtual le_int32 characterProcessing(const LEUnicode chars[], le_int32 offset, le_int32 count, le_int32 max, le_bool rightToLeft,
            LEUnicode *&outChars, LEGlyphStorage &glyphStorage, LEErrorCode &success);

};

U_NAMESPACE_END
#endif
