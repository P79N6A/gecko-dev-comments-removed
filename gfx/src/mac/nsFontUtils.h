





































#ifndef nsFontUtils_h_
#define nsFontUtils_h_

#include <TextEdit.h>

#include "nsIFontMetrics.h"
#include "nsIDeviceContext.h"
#include "gfxCore.h"

class NS_GFX nsFontUtils
{
public:

  static void   GetNativeTextStyle(nsIFontMetrics& inMetrics, const nsIDeviceContext& inDevContext, TextStyle &outStyle);
  static PRBool DisplayVerySmallFonts();

protected:

	static PRBool	sDisplayVerySmallFonts;
  
};



#endif 
