




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
                           gfxFontStyle& aFontStyle,
                           float aDevPixPerCSSPixel);
  virtual char16_t GetPasswordCharacterImpl()
  {
    
    return 0x2022;
  }

  static bool UseOverlayScrollbars();

  virtual nsTArray<LookAndFeelInt> GetIntCacheImpl();
  virtual void SetIntCacheImpl(const nsTArray<LookAndFeelInt>& lookAndFeelIntCache);

  virtual void RefreshImpl();
protected:

  
  static const int kThemeScrollBarArrowsBoth = 2;
  static const int kThemeScrollBarArrowsUpperLeft = 3;

  static bool SystemWantsOverlayScrollbars();
  static bool AllowOverlayScrollbarsOverlap();

private:
  int32_t mUseOverlayScrollbars;
  bool mUseOverlayScrollbarsCached;

  int32_t mAllowOverlayScrollbarsOverlap;
  bool mAllowOverlayScrollbarsOverlapCached;
};

#endif 
