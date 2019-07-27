















#include "OpenTypeLayoutEngine.h"
#include "TibetanLayoutEngine.h"
#include "LEGlyphStorage.h"
#include "TibetanReordering.h"

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(TibetanOpenTypeLayoutEngine)

TibetanOpenTypeLayoutEngine::TibetanOpenTypeLayoutEngine(const LEFontInstance *fontInstance, le_int32 scriptCode, le_int32 languageCode,
                                                         le_int32 typoFlags, const LEReferenceTo<GlyphSubstitutionTableHeader> &gsubTable, LEErrorCode &success)
    : OpenTypeLayoutEngine(fontInstance, scriptCode, languageCode, typoFlags, gsubTable, success)
{
    fFeatureMap   = TibetanReordering::getFeatureMap(fFeatureMapCount);
    fFeatureOrder = TRUE;
}

TibetanOpenTypeLayoutEngine::TibetanOpenTypeLayoutEngine(const LEFontInstance *fontInstance, le_int32 scriptCode, le_int32 languageCode,
						     le_int32 typoFlags, LEErrorCode &success)
    : OpenTypeLayoutEngine(fontInstance, scriptCode, languageCode, typoFlags, success)
{
    fFeatureMap   = TibetanReordering::getFeatureMap(fFeatureMapCount);
    fFeatureOrder = TRUE;
}

TibetanOpenTypeLayoutEngine::~TibetanOpenTypeLayoutEngine()
{
    
}




le_int32 TibetanOpenTypeLayoutEngine::characterProcessing(const LEUnicode chars[], le_int32 offset, le_int32 count, le_int32 max, le_bool rightToLeft,
        LEUnicode *&outChars, LEGlyphStorage &glyphStorage, LEErrorCode &success)
{
    if (LE_FAILURE(success)) {
        return 0;
    }

    if (chars == NULL || offset < 0 || count < 0 || max < 0 || offset >= max || offset + count > max) {
        success = LE_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    le_int32 worstCase = count * 3;  

    outChars = LE_NEW_ARRAY(LEUnicode, worstCase);

    if (outChars == NULL) {
        success = LE_MEMORY_ALLOCATION_ERROR;
        return 0;
    }

    glyphStorage.allocateGlyphArray(worstCase, rightToLeft, success);
    glyphStorage.allocateAuxData(success);

    if (LE_FAILURE(success)) {
        LE_DELETE_ARRAY(outChars);
        return 0;
    }

    
    
    le_int32 outCharCount = TibetanReordering::reorder(&chars[offset], count, fScriptCode, outChars, glyphStorage);

    glyphStorage.adoptGlyphCount(outCharCount);
    return outCharCount;
}

U_NAMESPACE_END
