




#ifndef nsLookAndFeel_h_
#define nsLookAndFeel_h_
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
  virtual PRUnichar GetPasswordCharacterImpl()
  {
    
    return 0x2022;
  }

protected:

  
  static const int kThemeScrollBarArrowsBoth = 2;
  static const int kThemeScrollBarArrowsUpperLeft = 3;
};

#endif 
