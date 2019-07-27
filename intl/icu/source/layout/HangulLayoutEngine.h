






#ifndef __HANGULAYOUTENGINE_H
#define __HANGULAYOUTENGINE_H

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











class HangulOpenTypeLayoutEngine : public OpenTypeLayoutEngine
{
public:
    

















    HangulOpenTypeLayoutEngine(const LEFontInstance *fontInstance, le_int32 scriptCode, le_int32 languageCode,
                               le_int32 typoFlags, const LEReferenceTo<GlyphSubstitutionTableHeader> &gsubTable, LEErrorCode &success);

    













    HangulOpenTypeLayoutEngine(const LEFontInstance *fontInstance, le_int32 scriptCode, le_int32 languageCode,
			      le_int32 typoFlags, LEErrorCode &success);

    




   virtual ~HangulOpenTypeLayoutEngine();

    




    virtual UClassID getDynamicClassID() const;

    




    static UClassID getStaticClassID();

protected:

    




















    virtual le_int32 characterProcessing(const LEUnicode chars[], le_int32 offset, le_int32 count, le_int32 max, le_bool rightToLeft,
            LEUnicode *&outChars, LEGlyphStorage &glyphStorage, LEErrorCode &success);
};

U_NAMESPACE_END
#endif

