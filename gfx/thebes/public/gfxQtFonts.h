





































#ifndef GFX_QTFONTS_H
#define GFX_QTFONTS_H

#include "cairo.h"
#include "gfxTypes.h"
#include "gfxFont.h"

#include "nsDataHashtable.h"
#include "nsClassHashtable.h"


class gfxQtFont : public gfxFont {
public:
     gfxQtFont (const nsAString& aName,
                const gfxFontStyle *aFontStyle);
     virtual ~gfxQtFont ();

    static void Shutdown();
};

class THEBES_API gfxQtFontGroup : public gfxFontGroup {
public:
    gfxQtFontGroup (const nsAString& families,
                    const gfxFontStyle *aStyle);
    virtual ~gfxQtFontGroup ();

    virtual gfxFontGroup *Copy(const gfxFontStyle *aStyle);

    
    virtual gfxTextRun *MakeTextRun(const PRUnichar *aString, PRUint32 aLength,
                                    const Parameters *aParams, PRUint32 aFlags);
    virtual gfxTextRun *MakeTextRun(const PRUint8 *aString, PRUint32 aLength,
                                    const Parameters *aParams, PRUint32 aFlags);
};

#endif 

