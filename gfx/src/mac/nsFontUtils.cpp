






































#include "nsIPref.h"
#include "nsIServiceManager.h"

#include "nsFont.h"
#include "nsDeviceContextMac.h"

#include "nsFontUtils.h"

PRBool nsFontUtils::sDisplayVerySmallFonts = true;


PRBool
nsFontUtils::DisplayVerySmallFonts()
{
	static PRBool sInitialized = PR_FALSE;
	if (sInitialized)
		return sDisplayVerySmallFonts;

	sInitialized = PR_TRUE;

  nsresult rv;
  nsCOMPtr<nsIPref> prefs = do_GetService(NS_PREF_CONTRACTID, &rv);
  if (NS_SUCCEEDED(rv) && prefs) {
    PRBool boolVal;
    if (NS_SUCCEEDED(prefs->GetBoolPref("browser.display_very_small_fonts", &boolVal))) {
      sDisplayVerySmallFonts = boolVal;
    }
  }

	return sDisplayVerySmallFonts;
}





void
nsFontUtils::GetNativeTextStyle(nsIFontMetrics& inMetrics,
		const nsIDeviceContext& inDevContext, TextStyle &outStyle)
{
	const nsFont *aFont = &inMetrics.Font();

	nsFontHandle	fontNum;
	inMetrics.GetFontHandle(fontNum);
	
	float  dev2app;
	dev2app = inDevContext.DevUnitsToAppUnits();
	short		textSize = float(aFont->size) / dev2app;
	textSize = PR_MAX(1, textSize);

	if (textSize < 9 && !nsFontUtils::DisplayVerySmallFonts())
		textSize = 9;
	
	Style textFace = normal;
	switch (aFont->style)
	{
		case NS_FONT_STYLE_NORMAL: 								break;
		case NS_FONT_STYLE_ITALIC: 		textFace |= italic;		break;
		case NS_FONT_STYLE_OBLIQUE: 	textFace |= italic;		break;	
	}

	PRInt32 offset = aFont->weight % 100;
	PRInt32 baseWeight = aFont->weight / 100;
	NS_ASSERTION((offset < 10) || (offset > 90), "Invalid bolder or lighter value");
	if (offset == 0) {
		if (aFont->weight >= NS_FONT_WEIGHT_BOLD)
			textFace |= bold;
	} else {
		if (offset < 10)
			textFace |= bold;
	}

	RGBColor	black = {0};

	outStyle.tsFont = (short) NS_PTR_TO_INT32(fontNum);
	outStyle.tsFace = textFace;
	outStyle.tsSize = textSize;
	outStyle.tsColor = black;
}
	
