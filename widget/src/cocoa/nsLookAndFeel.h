




































#ifndef nsLookAndFeel_h_
#define nsLookAndFeel_h_
#include "nsXPLookAndFeel.h"

class nsLookAndFeel: public nsXPLookAndFeel {
public:
  nsLookAndFeel();
  virtual ~nsLookAndFeel();

  nsresult NativeGetColor(const nsColorID aID, nscolor &aColor);
  NS_IMETHOD GetMetric(const nsMetricID aID, PRInt32 & aMetric);
  NS_IMETHOD GetMetric(const nsMetricFloatID aID, float & aMetric);

protected:

typedef enum {
  
  
  eColorOffset_mac_accentlightesthighlight,
  eColorOffset_mac_accentregularhighlight,
  eColorOffset_mac_accentface,
  eColorOffset_mac_accentlightshadow,
  eColorOffset_mac_accentregularshadow,
  eColorOffset_mac_accentdarkshadow,
  eColorOffset_mac_accentdarkestshadow
} nsMacAccentColorOffset;

  NS_IMETHOD GetMacBrushColor(const PRInt32 aBrushType, nscolor & aColor, const nscolor & aDefaultColor);
  NS_IMETHOD GetMacTextColor(const PRInt32 aTextType, nscolor & aColor, const nscolor & aDefaultColor);
  NS_IMETHOD GetMacAccentColor(const nsMacAccentColorOffset aAccent, nscolor & aColor, const nscolor & aDefaultColor);

  PRUnichar GetPasswordCharacter() {
    
    return 0x2022;
  }
};

#endif 
