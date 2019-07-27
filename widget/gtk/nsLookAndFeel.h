






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
    virtual char16_t GetPasswordCharacterImpl();
    virtual bool GetEchoPasswordImpl();

protected:
#if (MOZ_WIDGET_GTK == 2)
    struct _GtkStyle *mStyle;
#else
    struct _GtkStyleContext *mBackgroundStyle;
    struct _GtkStyleContext *mButtonStyle;
#endif

    
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

    
    nscolor sInfoBackground;
    nscolor sInfoText;
    nscolor sMenuBackground;
    nscolor sMenuBarText;
    nscolor sMenuBarHoverText;
    nscolor sMenuText;
    nscolor sMenuTextInactive;
    nscolor sMenuHover;
    nscolor sMenuHoverText;
    nscolor sButtonText;
    nscolor sButtonHoverText;
    nscolor sButtonBackground;
    nscolor sFrameOuterLightBorder;
    nscolor sFrameInnerDarkBorder;
    nscolor sOddCellBackground;
    nscolor sNativeHyperLinkText;
    nscolor sComboBoxText;
    nscolor sComboBoxBackground;
    nscolor sMozFieldText;
    nscolor sMozFieldBackground;
    nscolor sMozWindowText;
    nscolor sMozWindowBackground;
    nscolor sTextSelectedText;
    nscolor sTextSelectedBackground;
    nscolor sMozScrollbar;
    char16_t sInvisibleCharacter;
    float   sCaretRatio;
    bool    sMenuSupportsDrag;

    void Init();
};

#endif
