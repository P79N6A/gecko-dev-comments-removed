





































#include "nsLookAndFeel.h"

nsLookAndFeel::nsLookAndFeel()
    : nsXPLookAndFeel()
{
}

nsLookAndFeel::~nsLookAndFeel()
{
}

nsresult
nsLookAndFeel::NativeGetColor(const nsColorID aID, nscolor &aColor)
{
    nsresult rv = NS_OK;

#define BASE_ACTIVE_COLOR     NS_RGB(0xaa,0xaa,0xaa)
#define BASE_NORMAL_COLOR     NS_RGB(0xff,0xff,0xff)
#define BASE_SELECTED_COLOR   NS_RGB(0xaa,0xaa,0xaa)
#define BG_ACTIVE_COLOR       NS_RGB(0xff,0xff,0xff)
#define BG_INSENSITIVE_COLOR  NS_RGB(0xaa,0xaa,0xaa)
#define BG_NORMAL_COLOR       NS_RGB(0xff,0xff,0xff)
#define BG_PRELIGHT_COLOR     NS_RGB(0xee,0xee,0xee)
#define BG_SELECTED_COLOR     NS_RGB(0x99,0x99,0x99)
#define DARK_NORMAL_COLOR     NS_RGB(0x88,0x88,0x88)
#define FG_INSENSITIVE_COLOR  NS_RGB(0x44,0x44,0x44)
#define FG_NORMAL_COLOR       NS_RGB(0x00,0x00,0x00)
#define FG_PRELIGHT_COLOR     NS_RGB(0x77,0x77,0x77)
#define FG_SELECTED_COLOR     NS_RGB(0xaa,0xaa,0xaa)
#define LIGHT_NORMAL_COLOR    NS_RGB(0xaa,0xaa,0xaa)
#define TEXT_ACTIVE_COLOR     NS_RGB(0x99,0x99,0x99)
#define TEXT_NORMAL_COLOR     NS_RGB(0x00,0x00,0x00)
#define TEXT_SELECTED_COLOR   NS_RGB(0x00,0x00,0x00)

    
    

    switch (aID) {
        
        
        
    case eColor_WindowBackground:
        aColor = BASE_NORMAL_COLOR;
        break;
    case eColor_WindowForeground:
        aColor = TEXT_NORMAL_COLOR;
        break;
    case eColor_WidgetBackground:
        aColor = BG_NORMAL_COLOR;
        break;
    case eColor_WidgetForeground:
        aColor = FG_NORMAL_COLOR;
        break;
    case eColor_WidgetSelectBackground:
        aColor = BG_SELECTED_COLOR;
        break;
    case eColor_WidgetSelectForeground:
        aColor = FG_SELECTED_COLOR;
        break;
    case eColor_Widget3DHighlight:
        aColor = NS_RGB(0xa0,0xa0,0xa0);
        break;
    case eColor_Widget3DShadow:
        aColor = NS_RGB(0x40,0x40,0x40);
        break;
    case eColor_TextBackground:
        
        aColor = BASE_NORMAL_COLOR;
        break;
    case eColor_TextForeground:
        
        aColor = TEXT_NORMAL_COLOR;
        break;
    case eColor_TextSelectBackground:
    case eColor_IMESelectedRawTextBackground:
    case eColor_IMESelectedConvertedTextBackground:
        
        aColor = BASE_SELECTED_COLOR;
        break;
    case eColor_TextSelectForeground:
    case eColor_IMESelectedRawTextForeground:
    case eColor_IMESelectedConvertedTextForeground:
        
        aColor = TEXT_SELECTED_COLOR;
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
        
        aColor = BG_NORMAL_COLOR;
        break;
    case eColor_activecaption:
        
        aColor = BG_NORMAL_COLOR;
        break;
    case eColor_appworkspace:
        
        aColor = BG_NORMAL_COLOR;
        break;
    case eColor_background:
        
        aColor = BG_NORMAL_COLOR;
        break;
    case eColor_captiontext:
        
        aColor = FG_NORMAL_COLOR;
        break;
    case eColor_graytext:
        
        aColor = FG_INSENSITIVE_COLOR;
        break;
    case eColor_highlight:
        
        aColor = BASE_SELECTED_COLOR;
        break;
    case eColor_highlighttext:
        
        aColor = TEXT_SELECTED_COLOR;
        break;
    case eColor_inactiveborder:
        
        aColor = BG_NORMAL_COLOR;
        break;
    case eColor_inactivecaption:
        
        aColor = BG_INSENSITIVE_COLOR;
        break;
    case eColor_inactivecaptiontext:
        
        aColor = FG_INSENSITIVE_COLOR;
        break;
    case eColor_infobackground:
        
        aColor = BG_NORMAL_COLOR;
        break;
    case eColor_infotext:
        
        aColor = TEXT_NORMAL_COLOR;
        break;
    case eColor_menu:
        
        aColor = BG_NORMAL_COLOR;
        break;
    case eColor_menutext:
        
        aColor = TEXT_NORMAL_COLOR;
        break;
    case eColor_scrollbar:
        
        aColor = BG_ACTIVE_COLOR;
        break;

    case eColor_threedface:
    case eColor_buttonface:
        
        aColor = BG_NORMAL_COLOR;
        break;

    case eColor_buttontext:
        
        aColor = TEXT_NORMAL_COLOR;
        break;

    case eColor_buttonhighlight:
        
    case eColor_threedhighlight:
        
        aColor = LIGHT_NORMAL_COLOR;
        break;

    case eColor_threedlightshadow:
        
        aColor = BG_NORMAL_COLOR;
        break;

    case eColor_buttonshadow:
        
    case eColor_threedshadow:
        
        aColor = DARK_NORMAL_COLOR;
        break;

    case eColor_threeddarkshadow:
        
        aColor = NS_RGB(0,0,0);
        break;

    case eColor_window:
    case eColor_windowframe:
        aColor = BG_NORMAL_COLOR;
        break;

    case eColor_windowtext:
        aColor = FG_NORMAL_COLOR;
        break;

    case eColor__moz_eventreerow:
    case eColor__moz_field:
        aColor = BASE_NORMAL_COLOR;
        break;
    case eColor__moz_fieldtext:
        aColor = TEXT_NORMAL_COLOR;
        break;
    case eColor__moz_dialog:
        aColor = BG_NORMAL_COLOR;
        break;
    case eColor__moz_dialogtext:
        aColor = FG_NORMAL_COLOR;
        break;
    case eColor__moz_dragtargetzone:
        aColor = BG_SELECTED_COLOR;
        break;
    case eColor__moz_buttondefault:
        
        aColor = NS_RGB(0,0,0);
        break;
    case eColor__moz_buttonhoverface:
        aColor = BG_PRELIGHT_COLOR;
        break;
    case eColor__moz_buttonhovertext:
        aColor = FG_PRELIGHT_COLOR;
        break;
    case eColor__moz_cellhighlight:
    case eColor__moz_html_cellhighlight:
        aColor = BASE_ACTIVE_COLOR;
        break;
    case eColor__moz_cellhighlighttext:
    case eColor__moz_html_cellhighlighttext:
        aColor = TEXT_ACTIVE_COLOR;
        break;
    case eColor__moz_menuhover:
        aColor = BG_PRELIGHT_COLOR;
        break;
    case eColor__moz_menuhovertext:
        aColor = FG_PRELIGHT_COLOR;
        break;
    case eColor__moz_oddtreerow:
        aColor = NS_TRANSPARENT;
        break;
    case eColor__moz_nativehyperlinktext:
        aColor = NS_SAME_AS_FOREGROUND_COLOR;
        break;
    case eColor__moz_comboboxtext:
        aColor = TEXT_NORMAL_COLOR;
        break;
    case eColor__moz_combobox:
        aColor = BG_NORMAL_COLOR;
        break;
    case eColor__moz_menubartext:
        aColor = TEXT_NORMAL_COLOR;
        break;
    case eColor__moz_menubarhovertext:
        aColor = FG_PRELIGHT_COLOR;
        break;
    default:
        
        aColor = 0;
        rv = NS_ERROR_FAILURE;
        break;
    }

    return rv;
}


NS_IMETHODIMP
nsLookAndFeel::GetMetric(const nsMetricID aID, PRInt32 &aMetric)
{
    nsresult rv = nsXPLookAndFeel::GetMetric(aID, aMetric);
    if (NS_SUCCEEDED(rv))
        return rv;

    rv = NS_OK;

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

        case eMetric_WindowsDefaultTheme:
        case eMetric_TouchEnabled:
        case eMetric_MaemoClassic:
        case eMetric_WindowsThemeIdentifier:
            aMetric = 0;
            rv = NS_ERROR_NOT_IMPLEMENTED;
            break;

        case eMetric_SpellCheckerUnderlineStyle:
            aMetric = NS_UNDERLINE_STYLE_WAVY;
            break;

        default:
            aMetric = 0;
            rv = NS_ERROR_FAILURE;
    }

    return rv;
}

NS_IMETHODIMP
nsLookAndFeel::GetMetric(const nsMetricFloatID aID,
                         float &aMetric)
{
    nsresult rv = nsXPLookAndFeel::GetMetric(aID, aMetric);
    if (NS_SUCCEEDED(rv))
        return rv;
    rv = NS_OK;

    switch (aID) {
        case eMetricFloat_IMEUnderlineRelativeSize:
            aMetric = 1.0f;
            break;

        case eMetricFloat_SpellCheckerUnderlineRelativeSize:
            aMetric = 1.0f;
            break;

        default:
            aMetric = -1.0;
            rv = NS_ERROR_FAILURE;
            break;
    }
    return rv;
}
