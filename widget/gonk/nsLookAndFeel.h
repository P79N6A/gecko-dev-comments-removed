





































#ifndef __nsLookAndFeel
#define __nsLookAndFeel

#include "nsXPLookAndFeel.h"

class nsLookAndFeel : public nsXPLookAndFeel
{
public:
    nsLookAndFeel();
    virtual ~nsLookAndFeel();

    virtual bool GetFontImpl(FontID aID, nsString& aName, gfxFontStyle& aStyle);

protected:
    virtual nsresult NativeGetColor(ColorID aID, nscolor &aColor);
};

#endif
