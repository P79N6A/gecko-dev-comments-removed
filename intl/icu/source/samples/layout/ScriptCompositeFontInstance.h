






#ifndef __SCRIPTCOMPOSITEFONTINSTANCE_H
#define __SCRIPTCOMPOSITEFONTINSTANCE_H

#include "layout/LETypes.h"
#include "layout/LEFontInstance.h"

#include "FontMap.h"



class ScriptCompositeFontInstance : public LEFontInstance
{
public:

    ScriptCompositeFontInstance(FontMap *fontMap);

    virtual ~ScriptCompositeFontInstance();

      
















































    virtual const LEFontInstance *getSubFont(const LEUnicode chars[], le_int32 *offset, le_int32 limit, le_int32 script, LEErrorCode &success) const;

    







    virtual LEGlyphID mapCharToGlyph(LEUnicode32 ch) const;

    virtual const void *getFontTable(LETag tableTag) const;

    virtual le_int32 getUnitsPerEM() const;

    virtual le_int32 getAscent() const;

    virtual le_int32 getDescent() const;

    virtual le_int32 getLeading() const;

    virtual void getGlyphAdvance(LEGlyphID glyph, LEPoint &advance) const;

    virtual le_bool getGlyphPoint(LEGlyphID glyph, le_int32 pointNumber, LEPoint &point) const;

    float getXPixelsPerEm() const;

    float getYPixelsPerEm() const;

    float getScaleFactorX() const;

    float getScaleFactorY() const;

    


    virtual inline UClassID getDynamicClassID() const { return getStaticClassID(); }

    


    static inline UClassID getStaticClassID() { return (UClassID)&fgClassID; }

protected:
    FontMap *fFontMap;

private:

    



    static const char fgClassID;
};

inline const void *ScriptCompositeFontInstance::getFontTable(LETag ) const
{
    return NULL;
}



inline le_int32 ScriptCompositeFontInstance::getUnitsPerEM() const
{
    return 1;
}

inline le_int32 ScriptCompositeFontInstance::getAscent() const
{
    return fFontMap->getAscent();
}

inline le_int32 ScriptCompositeFontInstance::getDescent() const
{
    return fFontMap->getDescent();
}

inline le_int32 ScriptCompositeFontInstance::getLeading() const
{
    return fFontMap->getLeading();
}

inline float ScriptCompositeFontInstance::getXPixelsPerEm() const
{
    return fFontMap->getPointSize();
}

inline float ScriptCompositeFontInstance::getYPixelsPerEm() const
{
    return fFontMap->getPointSize();
}



inline float ScriptCompositeFontInstance::getScaleFactorX() const
{
    return 1.0;
}



inline float ScriptCompositeFontInstance::getScaleFactorY() const
{
    return 1.0;
}


#endif
