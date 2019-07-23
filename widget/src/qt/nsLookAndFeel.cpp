






































#include "nsLookAndFeel.h"

#include <qpalette.h>
#include <qapplication.h>

#define QCOLOR_TO_NS_RGB(c) \
    ((nscolor)NS_RGB(c.red(),c.green(),c.blue()))






nsLookAndFeel::nsLookAndFeel() : nsXPLookAndFeel()
{
}

nsLookAndFeel::~nsLookAndFeel()
{
}

nsresult nsLookAndFeel::NativeGetColor(const nsColorID aID,nscolor &aColor)
{
  nsresult res = NS_OK;

  if (!qApp)
    return NS_ERROR_FAILURE;

  QPalette palette = qApp->palette();
  QColorGroup normalGroup = palette.inactive();
  QColorGroup activeGroup = palette.active();
  QColorGroup disabledGroup = palette.disabled();

  switch (aID) {
    case eColor_WindowBackground:
      aColor = QCOLOR_TO_NS_RGB(normalGroup.background());
      break;

    case eColor_WindowForeground:
      aColor = QCOLOR_TO_NS_RGB(normalGroup.foreground());
      break;

    case eColor_WidgetBackground:
      aColor = QCOLOR_TO_NS_RGB(normalGroup.background());
      break;

    case eColor_WidgetForeground:
      aColor = QCOLOR_TO_NS_RGB(normalGroup.foreground());
      break;

    case eColor_WidgetSelectBackground:
      aColor = QCOLOR_TO_NS_RGB(activeGroup.background());
      break;

    case eColor_WidgetSelectForeground:
      aColor = QCOLOR_TO_NS_RGB(activeGroup.foreground());
      break;

    case eColor_Widget3DHighlight:
      aColor = NS_RGB(0xa0,0xa0,0xa0);
      break;

    case eColor_Widget3DShadow:
      aColor = NS_RGB(0x40,0x40,0x40);
      break;

    case eColor_TextBackground:
      aColor = QCOLOR_TO_NS_RGB(normalGroup.background());
      break;

    case eColor_TextForeground:
      aColor = QCOLOR_TO_NS_RGB(normalGroup.text());
      break;

    case eColor_TextSelectBackground:
    case eColor_IMESelectedRawTextBackground:
    case eColor_IMESelectedConvertedTextBackground:
      aColor = QCOLOR_TO_NS_RGB(activeGroup.highlight());
      break;

    case eColor_TextSelectForeground:
    case eColor_IMESelectedRawTextForeground:
    case eColor_IMESelectedConvertedTextForeground:
      aColor = QCOLOR_TO_NS_RGB(activeGroup.highlightedText());
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

    case eColor_activeborder:
      aColor = QCOLOR_TO_NS_RGB(normalGroup.background());
      break;

    case eColor_activecaption:
      aColor = QCOLOR_TO_NS_RGB(normalGroup.background());
      break;

    case eColor_appworkspace:
      aColor = QCOLOR_TO_NS_RGB(normalGroup.background());
      break;

    case eColor_background:
      aColor = QCOLOR_TO_NS_RGB(normalGroup.background());
      break;

    case eColor_captiontext:
      aColor = QCOLOR_TO_NS_RGB(normalGroup.text());
      break;

    case eColor_graytext:
      aColor = QCOLOR_TO_NS_RGB(disabledGroup.text());
      break;

    case eColor_highlight:
      aColor = QCOLOR_TO_NS_RGB(activeGroup.highlight());
      break;

    case eColor_highlighttext:
      aColor = QCOLOR_TO_NS_RGB(activeGroup.highlightedText());
      break;

    case eColor_inactiveborder:
      aColor = QCOLOR_TO_NS_RGB(normalGroup.background());
      break;

    case eColor_inactivecaption:
      aColor = QCOLOR_TO_NS_RGB(disabledGroup.background());
      break;

    case eColor_inactivecaptiontext:
      aColor = QCOLOR_TO_NS_RGB(disabledGroup.text());
      break;

    case eColor_infobackground:
      aColor = QCOLOR_TO_NS_RGB(normalGroup.background());
      break;

    case eColor_infotext:
      aColor = QCOLOR_TO_NS_RGB(normalGroup.text());
      break;

    case eColor_menu:
      aColor = QCOLOR_TO_NS_RGB(normalGroup.background());
      break;

    case eColor_menutext:
      aColor = QCOLOR_TO_NS_RGB(normalGroup.text());
      break;

    case eColor_scrollbar:
      aColor = QCOLOR_TO_NS_RGB(activeGroup.background());
      break;

    case eColor_threedface:
    case eColor_buttonface:
      aColor = QCOLOR_TO_NS_RGB(normalGroup.background());
      break;

    case eColor_buttonhighlight:
    case eColor_threedhighlight:
      aColor = QCOLOR_TO_NS_RGB(normalGroup.light());
      break;

    case eColor_buttontext:
      aColor = QCOLOR_TO_NS_RGB(normalGroup.text());
      break;

    case eColor_buttonshadow:
    case eColor_threedshadow:
      aColor = QCOLOR_TO_NS_RGB(normalGroup.shadow());
      break;

    case eColor_threeddarkshadow:
      aColor = QCOLOR_TO_NS_RGB(normalGroup.dark());
      break;

    case eColor_threedlightshadow:
      aColor = QCOLOR_TO_NS_RGB(normalGroup.light());
      break;

    case eColor_window:
    case eColor_windowframe:
      aColor = QCOLOR_TO_NS_RGB(normalGroup.background());
      break;

    case eColor_windowtext:
      aColor = QCOLOR_TO_NS_RGB(normalGroup.text());
      break;

     
     
     case eColor__moz_field:
       aColor = QCOLOR_TO_NS_RGB(normalGroup.base());
       break;

     case eColor__moz_fieldtext:
       aColor = QCOLOR_TO_NS_RGB(normalGroup.text());
       break;

     case eColor__moz_dialog:
       aColor = QCOLOR_TO_NS_RGB(normalGroup.background());
       break;

     case eColor__moz_dialogtext:
       aColor = QCOLOR_TO_NS_RGB(normalGroup.text());
       break;

     case eColor__moz_dragtargetzone:
       aColor = QCOLOR_TO_NS_RGB(activeGroup.background());
       break;

    default:
      aColor = 0;
      res    = NS_ERROR_FAILURE;
      break;
  }
  return res;
}

static const char *metricToString[] = {
    "eMetric_WindowTitleHeight",
    "eMetric_WindowBorderWidth",
    "eMetric_WindowBorderHeight",
    "eMetric_Widget3DBorder",
    "eMetric_TextFieldBorder"
    "eMetric_TextFieldHeight",
    "eMetric_TextVerticalInsidePadding",
    "eMetric_TextShouldUseVerticalInsidePadding",
    "eMetric_TextHorizontalInsideMinimumPadding",
    "eMetric_TextShouldUseHorizontalInsideMinimumPadding",
    "eMetric_ButtonHorizontalInsidePaddingNavQuirks",
    "eMetric_ButtonHorizontalInsidePaddingOffsetNavQuirks",
    "eMetric_CheckboxSize",
    "eMetric_RadioboxSize",

    "eMetric_ListShouldUseHorizontalInsideMinimumPadding",
    "eMetric_ListHorizontalInsideMinimumPadding",

    "eMetric_ListShouldUseVerticalInsidePadding",
    "eMetric_ListVerticalInsidePadding",

    "eMetric_CaretBlinkTime",
    "eMetric_CaretWidth",
    "eMetric_ShowCaretDuringSelection",
    "eMetric_SelectTextfieldsOnKeyFocus",
    "eMetric_SubmenuDelay",
    "eMetric_MenusCanOverlapOSBar",
    "eMetric_DragFullWindow",
    "eMetric_DragThresholdX",
    "eMetric_DragThresholdY",
    "eMetric_UseAccessibilityTheme",
    "eMetric_IsScreenReaderActive",

    "eMetric_ScrollArrowStyle",
    "eMetric_ScrollSliderStyle",

    "eMetric_TreeOpenDelay",
    "eMetric_TreeCloseDelay",
    "eMetric_TreeLazyScrollDelay",
    "eMetric_TreeScrollDelay",
    "eMetric_TreeScrollLinesMax"
    };


NS_IMETHODIMP nsLookAndFeel::GetMetric(const nsMetricID aID,PRInt32 &aMetric)
{

  nsresult res = nsXPLookAndFeel::GetMetric(aID, aMetric);
  if (NS_SUCCEEDED(res))
      return res;

  res = NS_OK;

  switch (aID) {
    case eMetric_WindowTitleHeight:
      aMetric = 0;
      break;

    case eMetric_WindowBorderWidth:
      
      
      break;

    case eMetric_WindowBorderHeight:
      
      
      break;

    case eMetric_Widget3DBorder:
      aMetric = 4;
      break;

    case eMetric_TextFieldHeight:
      aMetric = 15;
      break;

    case eMetric_TextFieldBorder:
      aMetric = 2;
      break;

    case eMetric_TextVerticalInsidePadding:
      aMetric = 0;
      break;

    case eMetric_TextShouldUseVerticalInsidePadding:
      aMetric = 0;
      break;

    case eMetric_TextHorizontalInsideMinimumPadding:
      aMetric = 15;
      break;

    case eMetric_TextShouldUseHorizontalInsideMinimumPadding:
      aMetric = 1;
      break;

    case eMetric_ButtonHorizontalInsidePaddingNavQuirks:
      aMetric = 10;
      break;

    case eMetric_ButtonHorizontalInsidePaddingOffsetNavQuirks:
      aMetric = 8;
      break;

    case eMetric_CheckboxSize:
      aMetric = 15;
      break;

    case eMetric_RadioboxSize:
      aMetric = 15;
      break;

    case eMetric_ListShouldUseHorizontalInsideMinimumPadding:
      aMetric = 15;
      break;

    case eMetric_ListHorizontalInsideMinimumPadding:
      aMetric = 15;
      break;

    case eMetric_ListShouldUseVerticalInsidePadding:
      aMetric = 1;
      break;

    case eMetric_ListVerticalInsidePadding:
      aMetric = 1;
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

    case eMetric_DragFullWindow:
      aMetric = 1;
      break;

    case eMetric_ScrollArrowStyle:
      aMetric = eMetric_ScrollArrowStyleSingle;
      break;

    case eMetric_ScrollSliderStyle:
      aMetric = eMetric_ScrollThumbStyleProportional;
      break;

    default:
      aMetric = 0;
      res = NS_ERROR_FAILURE;
  }
  return res;
}

NS_IMETHODIMP nsLookAndFeel::GetMetric(const nsMetricFloatID aID,
                                       float &aMetric)
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

    default:
      aMetric = -1.0;
      res = NS_ERROR_FAILURE;
  }
  return res;
}
