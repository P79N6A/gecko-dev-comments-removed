















#ifndef __nsLookAndFeel
#define __nsLookAndFeel

#include "nsXPLookAndFeel.h"

class nsLookAndFeel : public nsXPLookAndFeel
{
public:
    nsLookAndFeel();
    virtual ~nsLookAndFeel();

    virtual bool GetFontImpl(FontID aID, nsString& aName, gfxFontStyle& aStyle);
    virtual nsresult GetIntImpl(IntID aID, int32_t &aResult);
    virtual bool GetEchoPasswordImpl();

protected:
    virtual nsresult NativeGetColor(ColorID aID, nscolor &aColor);
};

#endif
