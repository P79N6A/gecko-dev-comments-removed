












#include "layout/LETypes.h"

#include "unicode/uscript.h"

#include "FontMap.h"

#include "ScriptCompositeFontInstance.h"

const char ScriptCompositeFontInstance::fgClassID=0;

ScriptCompositeFontInstance::ScriptCompositeFontInstance(FontMap *fontMap)
    : fFontMap(fontMap)
{
    
}

ScriptCompositeFontInstance::~ScriptCompositeFontInstance()
{
    delete fFontMap;
    fFontMap = NULL;
}

void ScriptCompositeFontInstance::getGlyphAdvance(LEGlyphID glyph, LEPoint &advance) const
{
    LEErrorCode status = LE_NO_ERROR;
    le_int32 script = LE_GET_SUB_FONT(glyph);
    const LEFontInstance *font = fFontMap->getScriptFont(script, status);

    advance.fX = 0;
    advance.fY = 0;

    if (LE_SUCCESS(status)) {
        font->getGlyphAdvance(LE_GET_GLYPH(glyph), advance);
    }
}

le_bool ScriptCompositeFontInstance::getGlyphPoint(LEGlyphID glyph, le_int32 pointNumber, LEPoint &point) const
{
    LEErrorCode status = LE_NO_ERROR;
    le_int32 script = LE_GET_SUB_FONT(glyph);
    const LEFontInstance *font = fFontMap->getScriptFont(script, status);

    if (LE_SUCCESS(status)) {
        return font->getGlyphPoint(LE_GET_GLYPH(glyph), pointNumber, point);
    }

    return FALSE;
}

const LEFontInstance *ScriptCompositeFontInstance::getSubFont(const LEUnicode chars[], le_int32 *offset, le_int32 limit, le_int32 script, LEErrorCode &success) const
{
    if (LE_FAILURE(success)) {
        return NULL;
    }

    if (chars == NULL || *offset < 0 || limit < 0 || *offset >= limit || script < 0 || script >= scriptCodeCount) {
        success = LE_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

    const LEFontInstance *result = fFontMap->getScriptFont(script, success);

    if (LE_FAILURE(success)) {
        return NULL;
    }

    *offset = limit;
    return result;
}




LEGlyphID ScriptCompositeFontInstance::mapCharToGlyph(LEUnicode32 ch) const
{
    UErrorCode  error  = U_ZERO_ERROR;
    LEErrorCode status = LE_NO_ERROR;
    le_int32 script = uscript_getScript(ch, &error);
    const LEFontInstance *scriptFont = fFontMap->getScriptFont(script, status);
    LEGlyphID subFont = LE_SET_SUB_FONT(0, script);

    if (LE_FAILURE(status)) {
        return subFont;
    }

    LEGlyphID glyph = scriptFont->mapCharToGlyph(ch);

    return LE_SET_GLYPH(subFont, glyph);
}

