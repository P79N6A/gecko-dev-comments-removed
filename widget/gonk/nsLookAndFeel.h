















#ifndef __nsLookAndFeel
#define __nsLookAndFeel

#include "nsXPLookAndFeel.h"

class nsLookAndFeel : public nsXPLookAndFeel
{
public:
    nsLookAndFeel();
    virtual ~nsLookAndFeel();

    virtual bool GetFontImpl(FontID aID, nsString& aName, gfxFontStyle& aStyle,
                             float aDevPixPerCSSPixel);
    virtual nsresult GetIntImpl(IntID aID, int32_t &aResult);
    virtual nsresult GetFloatImpl(FloatID aID, float &aResult);
    virtual bool GetEchoPasswordImpl();
    virtual uint32_t GetPasswordMaskDelayImpl();
    virtual PRUnichar GetPasswordCharacterImpl();

protected:
    virtual nsresult NativeGetColor(ColorID aID, nscolor &aColor);
};

#endif
