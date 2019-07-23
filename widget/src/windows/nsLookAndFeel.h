




































#ifndef __nsLookAndFeel
#define __nsLookAndFeel
#include "nsXPLookAndFeel.h"

#include "nsCOMPtr.h"

class nsLookAndFeel: public nsXPLookAndFeel {
public:
  nsLookAndFeel();
  virtual ~nsLookAndFeel();

  nsresult NativeGetColor(const nsColorID aID, nscolor &aColor);
  nsresult GetColorFromTheme(const PRUnichar* aClassList,
                             void* aTheme,
                             PRInt32 aPart,
                             PRInt32 aState,
                             PRInt32 aPropId,
                             nscolor &aColor);
  NS_IMETHOD GetMetric(const nsMetricID aID, PRInt32 & aMetric);
  NS_IMETHOD GetMetric(const nsMetricFloatID aID, float & aMetric);
  virtual PRUnichar GetPasswordCharacter();

#ifdef NS_DEBUG
  
  
  
  
  NS_IMETHOD GetNavSize(const nsMetricNavWidgetID aWidgetID,
                        const nsMetricNavFontID   aFontID, 
                        const PRInt32             aFontSize, 
                        nsSize &aSize);
#endif
};

#endif
