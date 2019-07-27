






#ifndef __ARABICLAYOUTENGINE_H
#define __ARABICLAYOUTENGINE_H

#include "LETypes.h"
#include "LEFontInstance.h"
#include "LEGlyphFilter.h"
#include "LayoutEngine.h"
#include "OpenTypeLayoutEngine.h"

#include "GlyphSubstitutionTables.h"
#include "GlyphDefinitionTables.h"
#include "GlyphPositioningTables.h"

U_NAMESPACE_BEGIN









class ArabicOpenTypeLayoutEngine : public OpenTypeLayoutEngine
{
public:
    

















    ArabicOpenTypeLayoutEngine(const LEFontInstance *fontInstance, le_int32 scriptCode, le_int32 languageCode,
                            le_int32 typoFlags, const LEReferenceTo<GlyphSubstitutionTableHeader> &gsubTable, LEErrorCode &success);

    













    ArabicOpenTypeLayoutEngine(const LEFontInstance *fontInstance, le_int32 scriptCode, le_int32 languageCode,
			       le_int32 typoFlags, LEErrorCode &success);

    




    virtual ~ArabicOpenTypeLayoutEngine();

    




    virtual UClassID getDynamicClassID() const;

    




    static UClassID getStaticClassID();

protected:

    




















    virtual le_int32 characterProcessing(const LEUnicode chars[], le_int32 offset, le_int32 count, le_int32 max, le_bool rightToLeft,
            LEUnicode *&outChars, LEGlyphStorage &glyphStorage, LEErrorCode &success);

    
















    virtual void adjustGlyphPositions(const LEUnicode chars[], le_int32 offset, le_int32 count, le_bool reverse, LEGlyphStorage &glyphStorage, LEErrorCode &success);

    

};













class UnicodeArabicOpenTypeLayoutEngine : public ArabicOpenTypeLayoutEngine
{
public:
    













    UnicodeArabicOpenTypeLayoutEngine(const LEFontInstance *fontInstance, le_int32 scriptCode, le_int32 languageCode,
		le_int32 typoFlags, LEErrorCode &success);

    




    virtual ~UnicodeArabicOpenTypeLayoutEngine();

protected:

    

















    virtual le_int32 glyphPostProcessing(LEGlyphStorage &tempGlyphStorage, LEGlyphStorage &glyphStorage, LEErrorCode &success);

    















    virtual void mapCharsToGlyphs(const LEUnicode chars[], le_int32 offset, le_int32 count, le_bool reverse, le_bool mirror,
        LEGlyphStorage &glyphStorage, LEErrorCode &success);

    













    virtual void adjustGlyphPositions(const LEUnicode chars[], le_int32 offset, le_int32 count, le_bool reverse, LEGlyphStorage &glyphStorage, LEErrorCode &success);
};

U_NAMESPACE_END
#endif

