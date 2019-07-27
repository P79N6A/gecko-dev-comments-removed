






#include "LETypes.h"
#include "LayoutEngine.h"
#include "ThaiLayoutEngine.h"
#include "ScriptAndLanguageTags.h"
#include "LEGlyphStorage.h"

#include "KernTable.h"

#include "ThaiShaping.h"

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(ThaiLayoutEngine)

ThaiLayoutEngine::ThaiLayoutEngine(const LEFontInstance *fontInstance, le_int32 scriptCode, le_int32 languageCode, le_int32 typoFlags, LEErrorCode &success)
    : LayoutEngine(fontInstance, scriptCode, languageCode, typoFlags, success)
{
    fErrorChar = 0x25CC;

    
    if (! fontInstance->canDisplay(0x0E01)) {
        
        fGlyphSet = 3;
    } else if (fontInstance->canDisplay(0x0E64)) {
        
        fGlyphSet = 0;
    } else if (fontInstance->canDisplay(0xF701)) {
        
        fGlyphSet = 1;

        if (!fontInstance->canDisplay(fErrorChar)) {
            fErrorChar = 0xF71B;
        }
    } else if (fontInstance->canDisplay(0xF885)) {
        
        fGlyphSet = 2;
    } else {
        
        fGlyphSet = 3;
    }
}

ThaiLayoutEngine::~ThaiLayoutEngine()
{
    
}





le_int32 ThaiLayoutEngine::computeGlyphs(const LEUnicode chars[], le_int32 offset, le_int32 count, le_int32 max, le_bool , LEGlyphStorage &glyphStorage, LEErrorCode &success)
{
    if (LE_FAILURE(success)) {
        return 0;
    }

    if (chars == NULL || offset < 0 || count < 0 || max < 0 || offset >= max || offset + count > max) {
        success = LE_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    LEUnicode *outChars;
    le_int32 glyphCount;
    
    
    
    outChars = LE_NEW_ARRAY(LEUnicode, count * 2);

    if (outChars == NULL) {
        success = LE_MEMORY_ALLOCATION_ERROR;
        return 0;
    }

    glyphStorage.allocateGlyphArray(count * 2, FALSE, success);

    if (LE_FAILURE(success)) {
        LE_DELETE_ARRAY(outChars);
        success = LE_MEMORY_ALLOCATION_ERROR;
        return 0;
    }

    glyphCount = ThaiShaping::compose(chars, offset, count, fGlyphSet, fErrorChar, outChars, glyphStorage);
    mapCharsToGlyphs(outChars, 0, glyphCount, FALSE, FALSE, glyphStorage, success);

    LE_DELETE_ARRAY(outChars);

    glyphStorage.adoptGlyphCount(glyphCount);
    return glyphCount;
}


void ThaiLayoutEngine::adjustGlyphPositions(const LEUnicode chars[], le_int32 offset, le_int32 count, le_bool  ,
                                        LEGlyphStorage &glyphStorage, LEErrorCode &success)
{
    if (LE_FAILURE(success)) {
        return;
    }

    if (chars == NULL || offset < 0 || count < 0) {
        success = LE_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    if (fTypoFlags & LE_Kerning_FEATURE_FLAG) { 
      LETableReference kernTable(fFontInstance, LE_KERN_TABLE_TAG, success);
      KernTable kt(kernTable, success);
      kt.process(glyphStorage, success);
    }

    
    return;
}

U_NAMESPACE_END
