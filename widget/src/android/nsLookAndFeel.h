





































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
    virtual PRBool GetEchoPasswordImpl();

protected:
    static PRBool mInitializedSystemColors;
    static mozilla::AndroidSystemColors mSystemColors;
    static PRBool mInitializedShowPassword;
    static PRBool mShowPassword;

    nsresult GetSystemColors();
    nsresult CallRemoteGetSystemColors();
};

#endif
