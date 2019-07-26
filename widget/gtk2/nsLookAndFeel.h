






#ifndef __nsLookAndFeel
#define __nsLookAndFeel

#include "nsXPLookAndFeel.h"
#include "nsCOMPtr.h"
#include "gfxFont.h"

struct _GtkStyle;

class nsLookAndFeel: public nsXPLookAndFeel {
public:
    nsLookAndFeel();
    virtual ~nsLookAndFeel();

    virtual nsresult NativeGetColor(ColorID aID, nscolor &aResult);
    virtual nsresult GetIntImpl(IntID aID, int32_t &aResult);
    virtual nsresult GetFloatImpl(FloatID aID, float &aResult);
    virtual bool GetFontImpl(FontID aID, nsString& aFontName,
                             gfxFontStyle& aFontStyle,
                             float aDevPixPerCSSPixel);

    virtual void RefreshImpl();
    virtual PRUnichar GetPasswordCharacterImpl();
    virtual bool GetEchoPasswordImpl();

protected:
    struct _GtkStyle *mStyle;

    
    bool mDefaultFontCached;
    bool mButtonFontCached;
    bool mFieldFontCached;
    bool mMenuFontCached;
    nsString mDefaultFontName;
    nsString mButtonFontName;
    nsString mFieldFontName;
    nsString mMenuFontName;
    gfxFontStyle mDefaultFontStyle;
    gfxFontStyle mButtonFontStyle;
    gfxFontStyle mFieldFontStyle;
    gfxFontStyle mMenuFontStyle;

    
    

    static nscolor sInfoBackground;
    static nscolor sInfoText;
    static nscolor sMenuBackground;
    static nscolor sMenuBarText;
    static nscolor sMenuBarHoverText;
    static nscolor sMenuText;
    static nscolor sMenuHover;
    static nscolor sMenuHoverText;
    static nscolor sButtonBackground;
    static nscolor sButtonText;
    static nscolor sButtonOuterLightBorder;
    static nscolor sButtonInnerDarkBorder;
    static nscolor sOddCellBackground;
    static nscolor sNativeHyperLinkText;
    static nscolor sComboBoxText;
    static nscolor sComboBoxBackground;
    static PRUnichar sInvisibleCharacter;
    static float   sCaretRatio;
    static bool    sMenuSupportsDrag;

    static void InitLookAndFeel();
    void InitWidget();
};

#endif
