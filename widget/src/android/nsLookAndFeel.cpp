






































#include "mozilla/dom/ContentChild.h"
#include "nsStyleConsts.h"
#include "nsXULAppAPI.h"
#include "nsLookAndFeel.h"

using namespace mozilla;
using mozilla::dom::ContentChild;

PRBool nsLookAndFeel::mInitialized = PR_FALSE;
AndroidSystemColors nsLookAndFeel::mSystemColors;

nsLookAndFeel::nsLookAndFeel()
    : nsXPLookAndFeel()
{
}

nsLookAndFeel::~nsLookAndFeel()
{
}

#define BG_PRELIGHT_COLOR      NS_RGB(0xee,0xee,0xee)
#define FG_PRELIGHT_COLOR      NS_RGB(0x77,0x77,0x77)
#define BLACK_COLOR            NS_RGB(0x00,0x00,0x00)
#define DARK_GRAY_COLOR        NS_RGB(0x40,0x40,0x40)
#define GRAY_COLOR             NS_RGB(0x80,0x80,0x80)
#define LIGHT_GRAY_COLOR       NS_RGB(0xa0,0xa0,0xa0)
#define RED_COLOR              NS_RGB(0xff,0x00,0x00)

nsresult
nsLookAndFeel::GetSystemColors()
{
    if (mInitialized)
        return NS_OK;

    if (!AndroidBridge::Bridge())
        return NS_ERROR_FAILURE;

    AndroidBridge::Bridge()->GetSystemColors(&mSystemColors);

    mInitialized = PR_TRUE;

    return NS_OK;
}

nsresult
nsLookAndFeel::CallRemoteGetSystemColors()
{
    
    InfallibleTArray<PRUint32> colors;
    PRUint32 colorsCount = sizeof(AndroidSystemColors) / sizeof(nscolor);

    if (!ContentChild::GetSingleton()->SendGetSystemColors(colorsCount, &colors))
        return NS_ERROR_FAILURE;

    NS_ASSERTION(colors.Length() == colorsCount, "System colors array is incomplete");
    if (colors.Length() == 0)
        return NS_ERROR_FAILURE;

    if (colors.Length() < colorsCount)
        colorsCount = colors.Length();

    
    
    memcpy(&mSystemColors, colors.Elements(), sizeof(nscolor) * colorsCount);

    mInitialized = PR_TRUE;

    return NS_OK;
}

nsresult
nsLookAndFeel::NativeGetColor(const nsColorID aID, nscolor &aColor)
{
    nsresult rv = NS_OK;

    if (!mInitialized) {
        if (XRE_GetProcessType() == GeckoProcessType_Default)
            rv = GetSystemColors();
        else
            rv = CallRemoteGetSystemColors();
        NS_ENSURE_SUCCESS(rv, rv);
    }

    
    

    switch (aID) {
        
        
        
    case eColor_WindowBackground:
        aColor = mSystemColors.colorBackground;
        break;
    case eColor_WindowForeground:
        aColor = mSystemColors.textColorPrimary;
        break;
    case eColor_WidgetBackground:
        aColor = mSystemColors.colorBackground;
        break;
    case eColor_WidgetForeground:
        aColor = mSystemColors.colorForeground;
        break;
    case eColor_WidgetSelectBackground:
        aColor = mSystemColors.textColorHighlight;
        break;
    case eColor_WidgetSelectForeground:
        aColor = mSystemColors.textColorPrimaryInverse;
        break;
    case eColor_Widget3DHighlight:
        aColor = LIGHT_GRAY_COLOR;
        break;
    case eColor_Widget3DShadow:
        aColor = DARK_GRAY_COLOR;
        break;
    case eColor_TextBackground:
        
        aColor = mSystemColors.colorBackground;
        break;
    case eColor_TextForeground:
        
        aColor = mSystemColors.textColorPrimary;
        break;
    case eColor_TextSelectBackground:
    case eColor_IMESelectedRawTextBackground:
    case eColor_IMESelectedConvertedTextBackground:
        
        aColor = mSystemColors.textColorHighlight;
        break;
    case eColor_TextSelectForeground:
    case eColor_IMESelectedRawTextForeground:
    case eColor_IMESelectedConvertedTextForeground:
        
        aColor = mSystemColors.textColorPrimaryInverse;
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
      aColor = RED_COLOR;
      break;

        
    case eColor_activeborder:
        
        aColor = mSystemColors.colorBackground;
        break;
    case eColor_activecaption:
        
        aColor = mSystemColors.colorBackground;
        break;
    case eColor_appworkspace:
        
        aColor = mSystemColors.colorBackground;
        break;
    case eColor_background:
        
        aColor = mSystemColors.colorBackground;
        break;
    case eColor_captiontext:
        
        aColor = mSystemColors.colorForeground;
        break;
    case eColor_graytext:
        
        aColor = mSystemColors.textColorTertiary;
        break;
    case eColor_highlight:
        
        aColor = mSystemColors.textColorHighlight;
        break;
    case eColor_highlighttext:
        
        aColor = mSystemColors.textColorPrimaryInverse;
        break;
    case eColor_inactiveborder:
        
        aColor = mSystemColors.colorBackground;
        break;
    case eColor_inactivecaption:
        
        aColor = mSystemColors.colorBackground;
        break;
    case eColor_inactivecaptiontext:
        
        aColor = mSystemColors.textColorTertiary;
        break;
    case eColor_infobackground:
        
        aColor = mSystemColors.colorBackground;
        break;
    case eColor_infotext:
        
        aColor = mSystemColors.colorForeground;
        break;
    case eColor_menu:
        
        aColor = mSystemColors.colorBackground;
        break;
    case eColor_menutext:
        
        aColor = mSystemColors.colorForeground;
        break;
    case eColor_scrollbar:
        
        aColor = mSystemColors.colorBackground;
        break;

    case eColor_threedface:
    case eColor_buttonface:
        
        aColor = mSystemColors.colorBackground;
        break;

    case eColor_buttontext:
        
        aColor = mSystemColors.colorForeground;
        break;

    case eColor_buttonhighlight:
        
    case eColor_threedhighlight:
        
        aColor = LIGHT_GRAY_COLOR;
        break;

    case eColor_threedlightshadow:
        
        aColor = mSystemColors.colorBackground;
        break;

    case eColor_buttonshadow:
        
    case eColor_threedshadow:
        
        aColor = GRAY_COLOR;
        break;

    case eColor_threeddarkshadow:
        
        aColor = BLACK_COLOR;
        break;

    case eColor_window:
    case eColor_windowframe:
        aColor = mSystemColors.colorBackground;
        break;

    case eColor_windowtext:
        aColor = mSystemColors.textColorPrimary;
        break;

    case eColor__moz_eventreerow:
    case eColor__moz_field:
        aColor = mSystemColors.colorBackground;
        break;
    case eColor__moz_fieldtext:
        aColor = mSystemColors.textColorPrimary;
        break;
    case eColor__moz_dialog:
        aColor = mSystemColors.colorBackground;
        break;
    case eColor__moz_dialogtext:
        aColor = mSystemColors.colorForeground;
        break;
    case eColor__moz_dragtargetzone:
        aColor = mSystemColors.textColorHighlight;
        break;
    case eColor__moz_buttondefault:
        
        aColor = BLACK_COLOR;
        break;
    case eColor__moz_buttonhoverface:
        aColor = BG_PRELIGHT_COLOR;
        break;
    case eColor__moz_buttonhovertext:
        aColor = FG_PRELIGHT_COLOR;
        break;
    case eColor__moz_cellhighlight:
    case eColor__moz_html_cellhighlight:
        aColor = mSystemColors.textColorHighlight;
        break;
    case eColor__moz_cellhighlighttext:
    case eColor__moz_html_cellhighlighttext:
        aColor = mSystemColors.textColorPrimaryInverse;
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
        aColor = mSystemColors.colorForeground;
        break;
    case eColor__moz_combobox:
        aColor = mSystemColors.colorBackground;
        break;
    case eColor__moz_menubartext:
        aColor = mSystemColors.colorForeground;
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
            aMetric = NS_STYLE_TEXT_DECORATION_STYLE_WAVY;
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
