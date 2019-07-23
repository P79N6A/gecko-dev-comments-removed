




































#ifndef GFX_WINDOWSDWRITEFONTS_H
#define GFX_WINDOWSDWRITEFONTS_H

#include <dwrite.h>

#include "gfxFont.h"
#include "gfxUserFontSet.h"
#include "cairo-win32.h"




class gfxDWriteFont : public gfxFont 
{
public:
    gfxDWriteFont(gfxFontEntry *aFontEntry,
                  const gfxFontStyle *aFontStyle,
                  PRBool aNeedsBold = PR_FALSE);
    ~gfxDWriteFont();

    virtual nsString GetUniqueName();

    virtual const gfxFont::Metrics& GetMetrics();

    virtual PRUint32 GetSpaceGlyph();

    virtual PRBool SetupCairoFont(gfxContext *aContext);

    virtual PRBool IsValid() { return mFontFace != NULL; }
protected:
    friend class gfxDWriteFontGroup;

    void ComputeMetrics();

    cairo_font_face_t *CairoFontFace();

    cairo_scaled_font_t *CairoScaledFont();

    nsRefPtr<IDWriteFontFace> mFontFace;
    cairo_font_face_t *mCairoFontFace;
    cairo_scaled_font_t *mCairoScaledFont;

    gfxFont::Metrics *mMetrics;
    PRBool mNeedsOblique;
};




class gfxDWriteFontGroup : public gfxFontGroup 
{
public:
    gfxDWriteFontGroup(const nsAString& aFamilies, 
                              const gfxFontStyle *aStyle,
                              gfxUserFontSet *aUserFontSet);
    virtual ~gfxDWriteFontGroup();
    
    gfxFontGroup *Copy(const gfxFontStyle *aStyle);

    






    virtual gfxDWriteFont *GetFontAt(PRInt32 i);

    virtual gfxTextRun *MakeTextRun(const PRUnichar *aString, PRUint32 aLength,
                                    const Parameters *aParams, PRUint32 aFlags);
    virtual gfxTextRun *MakeTextRun(const PRUint8 *aString, PRUint32 aLength,
                                    const Parameters *aParams, PRUint32 aFlags);
};

#endif
