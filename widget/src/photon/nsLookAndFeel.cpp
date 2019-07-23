




































#include "nsLookAndFeel.h"
#include <Pt.h>
#include "nsFont.h"

#define PH_TO_NS_RGB(ns) (ns & 0xff) << 16 | (ns & 0xff00) | ((ns >> 16) & 0xff) 

nsLookAndFeel::nsLookAndFeel() : nsXPLookAndFeel()
{
}

nsLookAndFeel::~nsLookAndFeel()
{
}

nsresult nsLookAndFeel::NativeGetColor(const nsColorID aID, nscolor &aColor)
{
  nsresult res = NS_OK;

  







  switch (aID) 
  {
	  case eColor_WindowBackground:
		aColor = PH_TO_NS_RGB(Pg_LGREY);
		break;
	  case eColor_WindowForeground:
		aColor = PH_TO_NS_RGB(Pg_DGREY);
		break;
	  case eColor_WidgetBackground:
		aColor = PH_TO_NS_RGB(Pg_LGREY);
		break;
	  case eColor_WidgetForeground:
		aColor = PH_TO_NS_RGB(Pg_DGREY);
		break;
	  case eColor_WidgetSelectBackground:
		aColor = PH_TO_NS_RGB(Pg_DGREY);
		break;
	  case eColor_WidgetSelectForeground:
		aColor = PH_TO_NS_RGB(Pg_DGREY);
		break;
	  case eColor_Widget3DHighlight:
		aColor = PH_TO_NS_RGB(Pg_WHITE);
		break;
	  case eColor_Widget3DShadow:
		aColor = PH_TO_NS_RGB(Pg_DGREY);
		break;
	  case eColor_TextBackground:
		aColor = PH_TO_NS_RGB(Pg_WHITE);
		break;
	  case eColor_TextForeground: 
		aColor = PH_TO_NS_RGB(Pg_BLACK);
		break;
    case eColor_TextSelectBackground:
    case eColor_IMESelectedRawTextBackground:
    case eColor_IMESelectedConvertedTextBackground:
      aColor = PH_TO_NS_RGB(Pg_BLACK);
      break;
    case eColor_TextSelectForeground:
    case eColor_IMESelectedRawTextForeground:
    case eColor_IMESelectedConvertedTextForeground:
      aColor = PH_TO_NS_RGB(Pg_WHITE);
      break;
    case eColor_IMERawInputBackground:
    case eColor_IMEConvertedTextBackground:
      aColor = NS_TRANSPARENT;
      break;
    case eColor_IMERawInputForeground:
    case eColor_IMEConvertedTextForeground:
      aColor = NS_SAME_AS_FOREGROUND_COLOR;
      break;
    case eColor_IMERawInputUnderline:
    case eColor_IMEConvertedTextUnderline:
      aColor = NS_SAME_AS_FOREGROUND_COLOR;
      break;
    case eColor_IMESelectedRawTextUnderline:
    case eColor_IMESelectedConvertedTextUnderline:
      aColor = NS_TRANSPARENT;
      break;
    case eColor_SpellCheckerUnderline:
      aColor = NS_RGB(0xff, 0, 0);
      break;

		
	  case eColor_activeborder:
		aColor = PH_TO_NS_RGB(Pg_BLACK);
		break;
	  case eColor_activecaption:
		aColor = PH_TO_NS_RGB(Pg_YELLOW);
		break;
	  case eColor_appworkspace:
		aColor = PH_TO_NS_RGB(Pg_LGREY);
		break;
	  case eColor_background:
		aColor = PH_TO_NS_RGB(Pg_LGREY);
		break;
	  case eColor_captiontext:
		aColor = PH_TO_NS_RGB(Pg_BLACK);
		break;
	  case eColor_graytext:
		aColor = PH_TO_NS_RGB(Pg_DGREY);
		break;
	  case eColor_highlight:
	  case eColor__moz_html_cellhighlight:
	  case eColor__moz_menuhover:
		aColor = PH_TO_NS_RGB(0x9ba9c9); 
		break;
	  case eColor_highlighttext:
	  case eColor__moz_html_cellhighlighttext:
	  case eColor__moz_menuhovertext:
		aColor = PH_TO_NS_RGB(Pg_BLACK);
		break;
	  case eColor_inactiveborder:
		aColor = PH_TO_NS_RGB(Pg_DGREY);
		break;
	  case eColor_inactivecaption:
		aColor = PH_TO_NS_RGB(Pg_GREY);
		break;
	  case eColor_inactivecaptiontext:
		aColor = PH_TO_NS_RGB(Pg_DGREY);
		break;
	  case eColor_infobackground:
		aColor = PH_TO_NS_RGB(Pg_BALLOONCOLOR); 
		break;
	  case eColor_infotext:
		aColor = PH_TO_NS_RGB(Pg_BLACK);
		break;
	  case eColor_menu:
		aColor = PH_TO_NS_RGB(Pg_LGREY);
		break;
	  case eColor_menutext:
		aColor = PH_TO_NS_RGB(Pg_BLACK);
		break;
	  case eColor_scrollbar:
		aColor = PH_TO_NS_RGB(Pg_LGREY);
		break;
	  case eColor_threedface:
	  case eColor_buttonface:
	  case eColor__moz_buttonhoverface:
		aColor = PH_TO_NS_RGB(Pg_LGREY);
		break;

	  case eColor_buttonhighlight:
	  case eColor_threedhighlight:
		aColor = PH_TO_NS_RGB(Pg_WHITE);
		break;

	  case eColor_buttontext:
	  case eColor__moz_buttonhovertext:
		aColor = PH_TO_NS_RGB(Pg_BLACK);
		break;

	  case eColor_buttonshadow:
	  case eColor_threedshadow: 
		aColor = PH_TO_NS_RGB(Pg_DGREY);
		break;

	  case eColor_threeddarkshadow:
		aColor = PH_TO_NS_RGB(Pg_DGREY);
		break;

	  case eColor_threedlightshadow:
		aColor = PH_TO_NS_RGB(Pg_LGREY);
		break;

	  case eColor_window:
		aColor = PH_TO_NS_RGB(Pg_WHITE);
		break;

	  case eColor_windowframe:
		aColor = PH_TO_NS_RGB(Pg_LGREY);
		break;

	  case eColor_windowtext:
		aColor = PH_TO_NS_RGB(Pg_BLACK);
		break;

	  case eColor__moz_eventreerow:
	  case eColor__moz_oddtreerow:
	  case eColor__moz_field:
		aColor = PH_TO_NS_RGB(Pg_WHITE);
		break;

	case eColor__moz_fieldtext:
	  aColor = PH_TO_NS_RGB(Pg_BLACK);
	  break;

	case eColor__moz_dialog:
	case eColor__moz_cellhighlight:
	  aColor = PH_TO_NS_RGB(Pg_LGREY);
	  break;

	case eColor__moz_dialogtext:
	case eColor__moz_cellhighlighttext:
	  aColor = PH_TO_NS_RGB(Pg_BLACK);
	  break;

	case eColor__moz_dragtargetzone:
	  aColor = PH_TO_NS_RGB(Pg_LGREY);
	  break;

	case eColor__moz_buttondefault:
	  aColor = PH_TO_NS_RGB(Pg_DGREY);
	  break;

  	default:
    aColor = PH_TO_NS_RGB(Pg_WHITE);
    break;
  }

  return res;
}
  
NS_IMETHODIMP nsLookAndFeel::GetMetric(const nsMetricID aID, PRInt32 & aMetric)
{


  nsresult res = nsXPLookAndFeel::GetMetric(aID, aMetric);
  if (NS_SUCCEEDED(res))
      return res;

  res = NS_OK;

  







  switch (aID) 
  {
  case eMetric_WindowTitleHeight:
    aMetric = 0;
    break;
  case eMetric_WindowBorderWidth:
    aMetric = 1;
    break;
  case eMetric_WindowBorderHeight:
    aMetric = 1;
    break;
  case eMetric_Widget3DBorder:
    aMetric = 2;
    break;
  case eMetric_TextFieldHeight:
  	aMetric = 20;
    break;
  case eMetric_TextFieldBorder:
    aMetric = 1;
    break;
  case eMetric_TextVerticalInsidePadding:
    aMetric = 0;
    break;
  case eMetric_TextShouldUseVerticalInsidePadding:
    aMetric = 0;
    break;
  case eMetric_TextHorizontalInsideMinimumPadding:
    aMetric = 0;
    break;
  case eMetric_TextShouldUseHorizontalInsideMinimumPadding:
    aMetric = 0;
    break;
  case eMetric_ButtonHorizontalInsidePaddingNavQuirks:
  	aMetric = 10;
    break;
  case eMetric_ButtonHorizontalInsidePaddingOffsetNavQuirks:
  	aMetric = 8;
    break;
  case eMetric_CheckboxSize:
    aMetric = 10;
    break;
  case eMetric_RadioboxSize:
    aMetric = 10;
    break;
  case eMetric_ListShouldUseHorizontalInsideMinimumPadding:
    aMetric = 0;
    break;
  case eMetric_ListHorizontalInsideMinimumPadding:
    aMetric = 0;
    break;
  case eMetric_ListShouldUseVerticalInsidePadding:
    aMetric = 0;
    break;
  case eMetric_ListVerticalInsidePadding:
    aMetric = 0;
    break;
  case eMetric_CaretBlinkTime:
    aMetric = 500;
    break;
  case eMetric_CaretWidth:
    aMetric = 1;
    break;
	case eMetric_ShowCaretDuringSelection:
		aMetric = 0;
		break;
	case eMetric_SelectTextfieldsOnKeyFocus:
		
		
		aMetric = 1;
		break;
  case eMetric_SubmenuDelay:
    aMetric = 200;
    break;
	case eMetric_MenusCanOverlapOSBar:
		
		aMetric = 1;
		break;
  case eMetric_TreeOpenDelay:
    aMetric = 1000;
    break;
  case eMetric_TreeCloseDelay:
    aMetric = 1000;
    break;
  case eMetric_TreeLazyScrollDelay:
    aMetric = 150;
    break;
  case eMetric_TreeScrollDelay:
    aMetric = 100;
    break;
  case eMetric_TreeScrollLinesMax:
    aMetric = 3;
    break;
  case eMetric_DWMCompositor:
  case eMetric_WindowsClassic:
  case eMetric_WindowsDefaultTheme:
  case eMetric_TouchEnabled:
    aMetric = 0;
    res = NS_ERROR_NOT_IMPLEMENTED;
    break;
  case eMetric_MacGraphiteTheme:
  case eMetric_MaemoClassic:
    aMetric = 0;
    res = NS_ERROR_NOT_IMPLEMENTED;
    break;
  case eMetric_IMERawInputUnderlineStyle:
  case eMetric_IMEConvertedTextUnderlineStyle:
    aMetric = NS_UNDERLINE_STYLE_SOLID;
    break;
  case eMetric_IMESelectedRawTextUnderlineStyle:
  case eMetric_IMESelectedConvertedTextUnderline:
    aMetric = NS_UNDERLINE_STYLE_NONE;
    break;
  case eMetric_SpellCheckerUnderlineStyle:
    aMetric = NS_UNDERLINE_STYLE_WAVY;
    break;

  default:
    aMetric = 0;
    res     = NS_ERROR_FAILURE;
  }

  return res;
}

NS_IMETHODIMP nsLookAndFeel::GetMetric(const nsMetricFloatID aID, float & aMetric)
{

  nsresult res = nsXPLookAndFeel::GetMetric(aID, aMetric);
  if (NS_SUCCEEDED(res))
      return res;

  res = NS_OK;

  switch (aID) {
  case eMetricFloat_TextFieldVerticalInsidePadding:
    aMetric = 0.25f;
    break;
  case eMetricFloat_TextFieldHorizontalInsidePadding:
    aMetric = 0.95f; 
    break;
  case eMetricFloat_TextAreaVerticalInsidePadding:
    aMetric = 0.40f;    
    break;
  case eMetricFloat_TextAreaHorizontalInsidePadding:
    aMetric = 0.40f; 
    break;
  case eMetricFloat_ListVerticalInsidePadding:
    aMetric = 0.10f;
    break;
  case eMetricFloat_ListHorizontalInsidePadding:
    aMetric = 0.40f;
    break;
  case eMetricFloat_ButtonVerticalInsidePadding:
    aMetric = 0.25f;
    break;
  case eMetricFloat_ButtonHorizontalInsidePadding:
    aMetric = 0.25f;
    break;
  case eMetricFloat_IMEUnderlineRelativeSize:
    aMetric = 1.0f;
    break;
  case eMetricFloat_SpellCheckerUnderlineRelativeSize:
    aMetric = 1.0f;
    break;
  default:
    aMetric = -1.0;
    res = NS_ERROR_FAILURE;
  }
 
  return res;
}
