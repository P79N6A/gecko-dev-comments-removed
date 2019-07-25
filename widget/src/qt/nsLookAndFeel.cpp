







































#include <QPalette>
#include <QApplication>
#include <QStyle>

#include "nsLookAndFeel.h"
#include "nsStyleConsts.h"

#include <qglobal.h>

#undef NS_LOOKANDFEEL_DEBUG
#ifdef NS_LOOKANDFEEL_DEBUG
#include <QDebug>
#endif

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

  switch (aID) {
    case eColor_WindowBackground:
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Window));
      break;

    case eColor_WindowForeground:
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Window));
      break;

    case eColor_WidgetBackground:
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Window));
      break;

    case eColor_WidgetForeground:
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::WindowText));
      break;

    case eColor_WidgetSelectBackground:
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Window));
      break;

    case eColor_WidgetSelectForeground:
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::WindowText));
      break;

    case eColor_Widget3DHighlight:
      aColor = NS_RGB(0xa0,0xa0,0xa0);
      break;

    case eColor_Widget3DShadow:
      aColor = NS_RGB(0x40,0x40,0x40);
      break;

    case eColor_TextBackground:
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Window));
      break;

    case eColor_TextForeground:
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::WindowText));
      break;

    case eColor_TextSelectBackground:
    case eColor_IMESelectedRawTextBackground:
    case eColor_IMESelectedConvertedTextBackground:
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Highlight));
      break;

    case eColor_TextSelectForeground:
    case eColor_IMESelectedRawTextForeground:
    case eColor_IMESelectedConvertedTextForeground:
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::HighlightedText));
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
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Window));
      break;

    case eColor_activecaption:
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Window));
      break;

    case eColor_appworkspace:
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Window));
      break;

    case eColor_background:
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Window));
      break;

    case eColor_captiontext:
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Text));
      break;

    case eColor_graytext:
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Disabled, QPalette::Text));
      break;

    case eColor_highlight:
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Highlight));
      break;

    case eColor_highlighttext:
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::HighlightedText));
      break;

    case eColor_inactiveborder:
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Disabled, QPalette::Window));
      break;

    case eColor_inactivecaption:
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Disabled, QPalette::Window));
      break;

    case eColor_inactivecaptiontext:
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Disabled, QPalette::Text));
      break;

    case eColor_infobackground:
#if (QT_VERSION >= QT_VERSION_CHECK(4, 4, 0))
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::ToolTipBase));
#else
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Base));
#endif
      break;

    case eColor_infotext:
#if (QT_VERSION >= QT_VERSION_CHECK(4, 4, 0))
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::ToolTipText));
#else
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Text));
#endif
      break;

    case eColor_menu:
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Window));
      break;

    case eColor_menutext:
    case eColor__moz_menubartext:
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Text));
      break;

    case eColor_scrollbar:
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Mid));
      break;

    case eColor_threedface:
    case eColor_buttonface:
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Button));
      break;

    case eColor_buttonhighlight:
    case eColor_threedhighlight:
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Dark));
      break;

    case eColor_buttontext:
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::ButtonText));
      break;

    case eColor_buttonshadow:
    case eColor_threedshadow:
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Dark));
      break;

    case eColor_threeddarkshadow:
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Shadow));
      break;

    case eColor_threedlightshadow:
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Light));
      break;

    case eColor_window:
    case eColor_windowframe:
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Window));
      break;

    case eColor_windowtext:
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Text));
      break;

     
     

    case eColor__moz_buttondefault:
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Button));
      break;

    case eColor__moz_field:
    case eColor__moz_combobox:
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Base));
      break;

    case eColor__moz_fieldtext:
    case eColor__moz_comboboxtext:
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Text));
      break;

    case eColor__moz_dialog:
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Window));
      break;

    case eColor__moz_dialogtext:
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::WindowText));
      break;

    case eColor__moz_dragtargetzone:
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Window));
      break;

    case eColor__moz_buttonhovertext:
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::ButtonText));
      break;

    case eColor__moz_menuhovertext:
    case eColor__moz_menubarhovertext:
      aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Text));
      break;

    default:
      aColor = 0;
      res    = NS_ERROR_FAILURE;
      break;
  }
  return res;
}

#ifdef NS_LOOKANDFEEL_DEBUG
static const char *metricToString[] = {
    "eMetric_CaretBlinkTime",
    "eMetric_CaretWidth",
    "eMetric_ShowCaretDuringSelection",
    "eMetric_SelectTextfieldsOnKeyFocus",
    "eMetric_SubmenuDelay",
    "eMetric_MenusCanOverlapOSBar",
    "eMetric_SkipNavigatingDisabledMenuItem",
    "eMetric_DragThresholdX",
    "eMetric_DragThresholdY",
    "eMetric_UseAccessibilityTheme",
    "eMetric_ScrollArrowStyle",
    "eMetric_ScrollSliderStyle",
    "eMetric_ScrollButtonLeftMouseButtonAction",
    "eMetric_ScrollButtonMiddleMouseButtonAction",
    "eMetric_ScrollButtonRightMouseButtonAction",
    "eMetric_TreeOpenDelay",
    "eMetric_TreeCloseDelay",
    "eMetric_TreeLazyScrollDelay",
    "eMetric_TreeScrollDelay",
    "eMetric_TreeScrollLinesMax",
    "eMetric_TabFocusModel",
    "eMetric_WindowsDefaultTheme",
    "eMetric_AlertNotificationOrigin",
    "eMetric_ScrollToClick",
    "eMetric_IMERawInputUnderlineStyle",
    "eMetric_IMESelectedRawTextUnderlineStyle",
    "eMetric_IMEConvertedTextUnderlineStyle",
    "eMetric_IMESelectedConvertedTextUnderline",
    "eMetric_ImagesInMenus"
    };
#endif

NS_IMETHODIMP nsLookAndFeel::GetMetric(const nsMetricID aID,PRInt32 &aMetric)
{
#ifdef NS_LOOKANDFEEL_DEBUG
  qDebug("nsLookAndFeel::GetMetric aID = %s", metricToString[aID]);
#endif

  nsresult res = nsXPLookAndFeel::GetMetric(aID, aMetric);
  if (NS_SUCCEEDED(res))
      return res;

  res = NS_OK;

  switch (aID) {
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

    case eMetric_ScrollArrowStyle:
      aMetric = eMetric_ScrollArrowStyleSingle;
      break;

    case eMetric_ScrollSliderStyle:
      aMetric = eMetric_ScrollThumbStyleProportional;
      break;

    case eMetric_TouchEnabled:
#ifdef MOZ_PLATFORM_MAEMO
      
      aMetric = 1;
#else
      aMetric = 0;
      res = NS_ERROR_NOT_IMPLEMENTED;
#endif
      break;

    case eMetric_WindowsDefaultTheme:
    case eMetric_MaemoClassic:
      aMetric = 0;
      res = NS_ERROR_NOT_IMPLEMENTED;
      break;

    case eMetric_SpellCheckerUnderlineStyle:
      aMetric = NS_STYLE_TEXT_DECORATION_STYLE_WAVY;
      break;

    default:
      aMetric = 0;
      res = NS_ERROR_FAILURE;
  }
  return res;
}

#ifdef NS_LOOKANDFEEL_DEBUG
static const char *floatMetricToString[] = {
    "eMetricFloat_IMEUnderlineRelativeSize"
};
#endif

NS_IMETHODIMP nsLookAndFeel::GetMetric(const nsMetricFloatID aID,
                                       float &aMetric)
{
#ifdef NS_LOOKANDFEEL_DEBUG
  qDebug("nsLookAndFeel::GetMetric aID = %s", floatMetricToString[aID]);
#endif

  nsresult res = nsXPLookAndFeel::GetMetric(aID, aMetric);
  if (NS_SUCCEEDED(res))
      return res;
  res = NS_OK;

  switch (aID) {
    case eMetricFloat_IMEUnderlineRelativeSize:
      aMetric = 1.0f;
      break;

    case eMetricFloat_SpellCheckerUnderlineRelativeSize:
      aMetric = 1.0f;
      break;

    default:
      aMetric = -1.0;
      res = NS_ERROR_FAILURE;
      break;
  }
  return res;
}
