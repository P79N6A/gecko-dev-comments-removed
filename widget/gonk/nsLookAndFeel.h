





































#ifndef __nsLookAndFeel
#define __nsLookAndFeel

#include "nsXPLookAndFeel.h"

class nsLookAndFeel : public nsXPLookAndFeel
{
public:
    nsLookAndFeel();
    virtual ~nsLookAndFeel();

    virtual bool GetFontImpl(FontID aID, nsString& aName, gfxFontStyle& aStyle);
    virtual nsresult GetIntImpl(IntID aID, PRInt32 &aResult);

protected:
    virtual nsresult NativeGetColor(ColorID aID, nscolor &aColor);
};

#endif
