




































#include "nsLookAndFeel.h"
#include "nsWidget.h"

nsLookAndFeel::nsLookAndFeel() : nsXPLookAndFeel()
{
    PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("nsLookAndFeel::nsLookAndFeel()\n"));
}

nsLookAndFeel::~nsLookAndFeel()
{
    PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("nsLookAndFeel::~nsLookAndFeel()\n"));
}

nsresult nsLookAndFeel::NativeGetColor(const nsColorID aID, nscolor &aColor)
{
    PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("nsLookAndFeel::NativeGetColor()\n"));
    nsresult res = NS_OK;

    aColor = 0;
    switch (aID) {
    case eColor_WindowBackground:
        aColor = NS_RGB(0xcc, 0xcc, 0xcc);
        break;
    case eColor_WindowForeground:
        aColor = NS_RGB(0x00, 0x00, 0x00);
        break;
    case eColor_WidgetBackground:
        aColor = NS_RGB(0xcc, 0xcc, 0xcc);
        break;
    case eColor_WidgetForeground:
        aColor = NS_RGB(0x00, 0x00, 0x00);
        break;
    case eColor_WidgetSelectBackground:
        aColor = NS_RGB(0x00, 0x00, 0x9c);
        break;
    case eColor_WidgetSelectForeground:
        aColor = NS_RGB(0xff, 0xff, 0xff);
        break;
    case eColor_Widget3DHighlight:
        aColor = NS_RGB(0x99,0x99,0x99);
        break;
    case eColor_Widget3DShadow:
        aColor = NS_RGB(0x40,0x40,0x40);
        break;
    case eColor_TextBackground:
        aColor = NS_RGB(0xff, 0xff, 0xff);
        break;
    case eColor_TextForeground:
        aColor = NS_RGB(0x00, 0x00, 0x00);
        break;
    case eColor_TextSelectBackground:
    case eColor_IMESelectedRawTextBackground:
    case eColor_IMESelectedConvertedTextBackground:
        aColor = NS_RGB(0x00, 0x00, 0x9c);
        break;
    case eColor_TextSelectForeground:
    case eColor_IMESelectedRawTextForeground:
    case eColor_IMESelectedConvertedTextForeground:
        aColor = NS_RGB(0xff, 0xff, 0xff);
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
        aColor = NS_RGB(0x00, 0x00, 0x00);
        break;
    case eColor_activecaption:
        aColor = NS_RGB(0xcc, 0xcc, 0xcc);
        break;
    case eColor_appworkspace:
        aColor = NS_RGB(0xcc, 0xcc, 0xcc);
        break;
    case eColor_background:
        aColor = NS_RGB(0xcc, 0xcc, 0xcc);
        break;

    case eColor_captiontext:
        aColor = NS_RGB(0x00, 0x00, 0x00);
        break;
    case eColor_graytext:
        aColor = NS_RGB(0x80, 0x80, 0x80);
        break;
    case eColor_highlight:
    case eColor__moz_menuhover:
        aColor = NS_RGB(0x00, 0x00, 0x9c);
        break;
    case eColor_highlighttext:
    case eColor__moz_menuhovertext:
        aColor = NS_RGB(0xff, 0xff, 0xff);
        break;
    case eColor_inactiveborder:
        aColor = NS_RGB(0xcc, 0xcc, 0xcc);
        break;
    case eColor_inactivecaption:
        aColor = NS_RGB(0x80, 0x80, 0x80);
        break;
    case eColor_inactivecaptiontext:
        aColor = NS_RGB(0xff, 0xff, 0xff);
        break;
    case eColor_infobackground:
        aColor = NS_RGB(0xcc, 0xcc, 0xcc);
        break;
    case eColor_infotext:
        aColor = NS_RGB(0x00, 0x00, 0x00);
        break;
    case eColor_menu:
        aColor = NS_RGB(0xcc, 0xcc, 0xcc);
        break;
    case eColor_menutext:
        aColor = NS_RGB(0x00, 0x00, 0x00);
        break;
    case eColor_scrollbar:
        aColor = NS_RGB(0xcc, 0xcc, 0xcc);
        break;

    case eColor_threedface:
          
    case eColor_buttonface:
    case eColor__moz_buttonhoverface:
          
    case eColor_threedlightshadow:
          
        aColor = NS_RGB(0xcc, 0xcc, 0xcc);
        break;

    case eColor_buttonhighlight:
          
    case eColor_threedhighlight:
          
        aColor = NS_RGB(0xff, 0xff, 0xff);
        break;

    case eColor_buttontext:
    case eColor__moz_buttonhovertext:
        aColor = NS_RGB(0x00, 0x00, 0x00);
        break;

    case eColor_buttonshadow:
          
    case eColor_threedshadow:
          
        aColor = NS_RGB(0x66, 0x66, 0x66);
        break;

    case eColor_threeddarkshadow:
          
        aColor = NS_RGB(0x00, 0x00, 0x00);
        break;

    case eColor_window:
    case eColor_windowframe:
        aColor = NS_RGB(0xcc, 0xcc, 0xcc);
        break;

    case eColor_windowtext:
        aColor = NS_RGB(0x00, 0x00, 0x00);
        break;

    case eColor__moz_field:
        aColor = NS_RGB(0xff, 0xff, 0xff);
        break;
    case eColor__moz_fieldtext:
        aColor = NS_RGB(0x00, 0x00, 0x00);
        break;
    case eColor__moz_dialog:
    case eColor__moz_cellhighlight:
        aColor = NS_RGB(0xcc, 0xcc, 0xcc);
        break;
    case eColor__moz_dialogtext:
    case eColor__moz_cellhighlighttext:
        aColor = NS_RGB(0x00, 0x00, 0x00);
        break;

    case eColor__moz_buttondefault:
          
        aColor = NS_RGB(0x00, 0x00, 0x00);
        break;

    default:
        
        aColor = 0;
        res    = NS_ERROR_FAILURE;
        break;
    }
    return NS_OK;
}

NS_IMETHODIMP nsLookAndFeel::GetMetric(const nsMetricID aID, PRInt32 & aMetric)
{
    PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("nsLookAndFeel::GetMetric()\n"));
    
    nsresult res = nsXPLookAndFeel::GetMetric(aID, aMetric);
    if (NS_SUCCEEDED(res))
        return res;
    res = NS_OK;

    switch (aID) { 
    case eMetric_WindowTitleHeight:
        aMetric = 20;
        break;
    case eMetric_WindowBorderWidth:
        aMetric = 1;
        break;
    case eMetric_WindowBorderHeight:
        aMetric = 1;
        break;
    case eMetric_Widget3DBorder:
        aMetric = 1;
        break;
    case eMetric_TextFieldHeight:
        aMetric = 24;
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
    case eMetric_TextHorizontalInsideMinimumPadding:
        aMetric = 3;
        break;
    case eMetric_TextVerticalInsidePadding:
        aMetric = 0;
        break;
    case eMetric_TextShouldUseVerticalInsidePadding:
        aMetric = 0;
        break;
    case eMetric_TextShouldUseHorizontalInsideMinimumPadding:
        aMetric = 1;
        break;
    case eMetric_ListShouldUseHorizontalInsideMinimumPadding:
        aMetric = 0;
        break;
    case eMetric_ListHorizontalInsideMinimumPadding:
        aMetric = 3;
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
    default:
        aMetric = 0;
        res = NS_ERROR_FAILURE;
    }
    return res;
}

NS_IMETHODIMP nsLookAndFeel::GetMetric(const nsMetricFloatID aID, float & aMetric)
{
    PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("nsLookAndFeel::GetMetric()\n"));

    
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
