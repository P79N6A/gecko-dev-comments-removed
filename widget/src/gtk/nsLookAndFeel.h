




































#ifndef __nsLookAndFeel
#define __nsLookAndFeel
#include "nsXPLookAndFeel.h"
#include "nsCOMPtr.h"
#include <gtk/gtk.h>

class nsLookAndFeel: public nsXPLookAndFeel {
public:
  nsLookAndFeel();
  virtual ~nsLookAndFeel();
  
  nsresult NativeGetColor(const nsColorID aID, nscolor &aColor);
  NS_IMETHOD GetMetric(const nsMetricID aID, PRInt32 & aMetric);
  NS_IMETHOD GetMetric(const nsMetricFloatID aID, float & aMetric);

protected:
  GtkStyle *mStyle;
  GtkWidget *mWidget;

  
  
  static PRBool sColorsInitialized;
  static nscolor sInfoBackground;
  static nscolor sInfoText;
  static nscolor sMenuBackground;
  static nscolor sMenuText;
  static nscolor sButtonBackground;
  static nscolor sButtonText;
  static nscolor sButtonOuterLightBorder;
  static nscolor sButtonInnerDarkBorder;

  static void InitColors();
};

#endif
