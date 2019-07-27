






#ifndef __INDICLAYOUTENGINE_H
#define __INDICLAYOUTENGINE_H

#include "LETypes.h"
#include "LEFontInstance.h"
#include "LEGlyphFilter.h"
#include "LayoutEngine.h"
#include "OpenTypeLayoutEngine.h"

#include "GlyphSubstitutionTables.h"
#include "GlyphDefinitionTables.h"
#include "GlyphPositioningTables.h"

U_NAMESPACE_BEGIN

class MPreFixups;
class LEGlyphStorage;












class IndicOpenTypeLayoutEngine : public OpenTypeLayoutEngine
{
public:
    

















    IndicOpenTypeLayoutEngine(const LEFontInstance *fontInstance, le_int32 scriptCode, le_int32 languageCode,
                            le_int32 typoFlags, le_bool version2, const LEReferenceTo<GlyphSubstitutionTableHeader> &gsubTable, LEErrorCode &success);

    













    IndicOpenTypeLayoutEngine(const LEFontInstance *fontInstance, le_int32 scriptCode, le_int32 languageCode,
			      le_int32 typoFlags, LEErrorCode &success);

    




   virtual ~IndicOpenTypeLayoutEngine();

    




    virtual UClassID getDynamicClassID() const;

    




    static UClassID getStaticClassID();

protected:

    





















    virtual le_int32 characterProcessing(const LEUnicode chars[], le_int32 offset, le_int32 count, le_int32 max, le_bool rightToLeft,
            LEUnicode *&outChars, LEGlyphStorage &glyphStorage, LEErrorCode &success);

    



























    virtual le_int32 glyphProcessing(const LEUnicode chars[], le_int32 offset, le_int32 count, le_int32 max, le_bool rightToLeft,
            LEGlyphStorage &glyphStorage, LEErrorCode &success);

    le_bool fVersion2;

private:

    MPreFixups *fMPreFixups;

};

U_NAMESPACE_END
#endif

