



#ifndef __nsLookAndFeel
#define __nsLookAndFeel

#include "nsXPLookAndFeel.h"
#include "AndroidBridge.h"

class nsLookAndFeel: public nsXPLookAndFeel
{
public:
    nsLookAndFeel();
    virtual ~nsLookAndFeel();

    virtual nsresult NativeGetColor(ColorID aID, nscolor &aResult);
    virtual nsresult GetIntImpl(IntID aID, PRInt32 &aResult);
    virtual nsresult GetFloatImpl(FloatID aID, float &aResult);
    virtual bool GetFontImpl(FontID aID, nsString& aName, gfxFontStyle& aStyle);
    virtual bool GetEchoPasswordImpl();
    virtual PRUint32 GetPasswordMaskDelayImpl();

protected:
    static bool mInitializedSystemColors;
    static mozilla::AndroidSystemColors mSystemColors;
    static bool mInitializedShowPassword;
    static bool mShowPassword;

    nsresult GetSystemColors();
    nsresult CallRemoteGetSystemColors();
};

#endif
