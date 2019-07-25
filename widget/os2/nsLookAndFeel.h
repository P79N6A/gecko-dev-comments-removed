




#ifndef __nsLookAndFeel
#define __nsLookAndFeel
#include "nsXPLookAndFeel.h"

class nsLookAndFeel: public nsXPLookAndFeel {
public:
  nsLookAndFeel();
  virtual ~nsLookAndFeel();

  virtual nsresult NativeGetColor(ColorID aID, nscolor &aResult);
  virtual nsresult GetIntImpl(IntID aID, int32_t &aResult);
  virtual nsresult GetFloatImpl(FloatID aID, float &aResult);
  virtual bool GetFontImpl(FontID aID, nsString& aFontName,
                           gfxFontStyle& aFontStyle);
};

#endif
