






































#include "mozilla/dom/ContentChild.h"
#include "nsStyleConsts.h"
#include "nsXULAppAPI.h"
#include "nsLookAndFeel.h"

using namespace mozilla;
using mozilla::dom::ContentChild;

bool nsLookAndFeel::mInitializedSystemColors = false;
AndroidSystemColors nsLookAndFeel::mSystemColors;

bool nsLookAndFeel::mInitializedShowPassword = false;
bool nsLookAndFeel::mShowPassword = true;

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
    if (mInitializedSystemColors)
        return NS_OK;

    if (!AndroidBridge::Bridge())
        return NS_ERROR_FAILURE;

    AndroidBridge::Bridge()->GetSystemColors(&mSystemColors);

    mInitializedSystemColors = true;

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

    mInitializedSystemColors = true;

    return NS_OK;
}

nsresult
nsLookAndFeel::NativeGetColor(ColorID aID, nscolor &aColor)
{
    nsresult rv = NS_OK;

    if (!mInitializedSystemColors) {
        if (XRE_GetProcessType() == GeckoProcessType_Default)
            rv = GetSystemColors();
        else
            rv = CallRemoteGetSystemColors();
        NS_ENSURE_SUCCESS(rv, rv);
    }

    
    

    switch (aID) {
        
        
        
    case eColorID_WindowBackground:
        aColor = mSystemColors.colorBackground;
        break;
    case eColorID_WindowForeground:
        aColor = mSystemColors.textColorPrimary;
        break;
    case eColorID_WidgetBackground:
        aColor = mSystemColors.colorBackground;
        break;
    case eColorID_WidgetForeground:
        aColor = mSystemColors.colorForeground;
        break;
    case eColorID_WidgetSelectBackground:
        aColor = mSystemColors.textColorHighlight;
        break;
    case eColorID_WidgetSelectForeground:
        aColor = mSystemColors.textColorPrimaryInverse;
        break;
    case eColorID_Widget3DHighlight:
        aColor = LIGHT_GRAY_COLOR;
        break;
    case eColorID_Widget3DShadow:
        aColor = DARK_GRAY_COLOR;
        break;
    case eColorID_TextBackground:
        
        aColor = mSystemColors.colorBackground;
        break;
    case eColorID_TextForeground:
        
        aColor = mSystemColors.textColorPrimary;
        break;
    case eColorID_TextSelectBackground:
    case eColorID_IMESelectedRawTextBackground:
    case eColorID_IMESelectedConvertedTextBackground:
        
        aColor = mSystemColors.textColorHighlight;
        break;
    case eColorID_TextSelectForeground:
    case eColorID_IMESelectedRawTextForeground:
    case eColorID_IMESelectedConvertedTextForeground:
        
        aColor = mSystemColors.textColorPrimaryInverse;
        break;
    case eColorID_IMERawInputBackground:
    case eColorID_IMEConvertedTextBackground:
        aColor = NS_TRANSPARENT;
        break;
    case eColorID_IMERawInputForeground:
    case eColorID_IMEConvertedTextForeground:
        aColor = NS_SAME_AS_FOREGROUND_COLOR;
        break;
    case eColorID_IMERawInputUnderline:
    case eColorID_IMEConvertedTextUnderline:
        aColor = NS_SAME_AS_FOREGROUND_COLOR;
        break;
    case eColorID_IMESelectedRawTextUnderline:
    case eColorID_IMESelectedConvertedTextUnderline:
        aColor = NS_TRANSPARENT;
        break;
    case eColorID_SpellCheckerUnderline:
      aColor = RED_COLOR;
      break;

        
    case eColorID_activeborder:
        
        aColor = mSystemColors.colorBackground;
        break;
    case eColorID_activecaption:
        
        aColor = mSystemColors.colorBackground;
        break;
    case eColorID_appworkspace:
        
        aColor = mSystemColors.colorBackground;
        break;
    case eColorID_background:
        
        aColor = mSystemColors.colorBackground;
        break;
    case eColorID_captiontext:
        
        aColor = mSystemColors.colorForeground;
        break;
    case eColorID_graytext:
        
        aColor = mSystemColors.textColorTertiary;
        break;
    case eColorID_highlight:
        
        aColor = mSystemColors.textColorHighlight;
        break;
    case eColorID_highlighttext:
        
        aColor = mSystemColors.textColorPrimaryInverse;
        break;
    case eColorID_inactiveborder:
        
        aColor = mSystemColors.colorBackground;
        break;
    case eColorID_inactivecaption:
        
        aColor = mSystemColors.colorBackground;
        break;
    case eColorID_inactivecaptiontext:
        
        aColor = mSystemColors.textColorTertiary;
        break;
    case eColorID_infobackground:
        
        aColor = mSystemColors.colorBackground;
        break;
    case eColorID_infotext:
        
        aColor = mSystemColors.colorForeground;
        break;
    case eColorID_menu:
        
        aColor = mSystemColors.colorBackground;
        break;
    case eColorID_menutext:
        
        aColor = mSystemColors.colorForeground;
        break;
    case eColorID_scrollbar:
        
        aColor = mSystemColors.colorBackground;
        break;

    case eColorID_threedface:
    case eColorID_buttonface:
        
        aColor = mSystemColors.colorBackground;
        break;

    case eColorID_buttontext:
        
        aColor = mSystemColors.colorForeground;
        break;

    case eColorID_buttonhighlight:
        
    case eColorID_threedhighlight:
        
        aColor = LIGHT_GRAY_COLOR;
        break;

    case eColorID_threedlightshadow:
        
        aColor = mSystemColors.colorBackground;
        break;

    case eColorID_buttonshadow:
        
    case eColorID_threedshadow:
        
        aColor = GRAY_COLOR;
        break;

    case eColorID_threeddarkshadow:
        
        aColor = BLACK_COLOR;
        break;

    case eColorID_window:
    case eColorID_windowframe:
        aColor = mSystemColors.colorBackground;
        break;

    case eColorID_windowtext:
        aColor = mSystemColors.textColorPrimary;
        break;

    case eColorID__moz_eventreerow:
    case eColorID__moz_field:
        aColor = mSystemColors.colorBackground;
        break;
    case eColorID__moz_fieldtext:
        aColor = mSystemColors.textColorPrimary;
        break;
    case eColorID__moz_dialog:
        aColor = mSystemColors.colorBackground;
        break;
    case eColorID__moz_dialogtext:
        aColor = mSystemColors.colorForeground;
        break;
    case eColorID__moz_dragtargetzone:
        aColor = mSystemColors.textColorHighlight;
        break;
    case eColorID__moz_buttondefault:
        
        aColor = BLACK_COLOR;
        break;
    case eColorID__moz_buttonhoverface:
        aColor = BG_PRELIGHT_COLOR;
        break;
    case eColorID__moz_buttonhovertext:
        aColor = FG_PRELIGHT_COLOR;
        break;
    case eColorID__moz_cellhighlight:
    case eColorID__moz_html_cellhighlight:
        aColor = mSystemColors.textColorHighlight;
        break;
    case eColorID__moz_cellhighlighttext:
    case eColorID__moz_html_cellhighlighttext:
        aColor = mSystemColors.textColorPrimaryInverse;
        break;
    case eColorID__moz_menuhover:
        aColor = BG_PRELIGHT_COLOR;
        break;
    case eColorID__moz_menuhovertext:
        aColor = FG_PRELIGHT_COLOR;
        break;
    case eColorID__moz_oddtreerow:
        aColor = NS_TRANSPARENT;
        break;
    case eColorID__moz_nativehyperlinktext:
        aColor = NS_SAME_AS_FOREGROUND_COLOR;
        break;
    case eColorID__moz_comboboxtext:
        aColor = mSystemColors.colorForeground;
        break;
    case eColorID__moz_combobox:
        aColor = mSystemColors.colorBackground;
        break;
    case eColorID__moz_menubartext:
        aColor = mSystemColors.colorForeground;
        break;
    case eColorID__moz_menubarhovertext:
        aColor = FG_PRELIGHT_COLOR;
        break;
    default:
        
        aColor = 0;
        rv = NS_ERROR_FAILURE;
        break;
    }

    return rv;
}


nsresult
nsLookAndFeel::GetIntImpl(IntID aID, PRInt32 &aResult)
{
    nsresult rv = nsXPLookAndFeel::GetIntImpl(aID, aResult);
    if (NS_SUCCEEDED(rv))
        return rv;

    rv = NS_OK;

    switch (aID) {
        case eIntID_CaretBlinkTime:
            aResult = 500;
            break;

        case eIntID_CaretWidth:
            aResult = 1;
            break;

        case eIntID_ShowCaretDuringSelection:
            aResult = 0;
            break;

        case eIntID_SelectTextfieldsOnKeyFocus:
            
            
            aResult = 1;
            break;

        case eIntID_SubmenuDelay:
            aResult = 200;
            break;

        case eIntID_MenusCanOverlapOSBar:
            
            aResult = 1;
            break;

        case eIntID_ScrollArrowStyle:
            aResult = eScrollArrowStyle_Single;
            break;

        case eIntID_ScrollSliderStyle:
            aResult = eScrollThumbStyle_Proportional;
            break;

        case eIntID_WindowsDefaultTheme:
        case eIntID_TouchEnabled:
        case eIntID_MaemoClassic:
        case eIntID_WindowsThemeIdentifier:
            aResult = 0;
            rv = NS_ERROR_NOT_IMPLEMENTED;
            break;

        case eIntID_SpellCheckerUnderlineStyle:
            aResult = NS_STYLE_TEXT_DECORATION_STYLE_WAVY;
            break;

        default:
            aResult = 0;
            rv = NS_ERROR_FAILURE;
    }

    return rv;
}

nsresult
nsLookAndFeel::GetFloatImpl(FloatID aID, float &aResult)
{
    nsresult rv = nsXPLookAndFeel::GetFloatImpl(aID, aResult);
    if (NS_SUCCEEDED(rv))
        return rv;
    rv = NS_OK;

    switch (aID) {
        case eFloatID_IMEUnderlineRelativeSize:
            aResult = 1.0f;
            break;

        case eFloatID_SpellCheckerUnderlineRelativeSize:
            aResult = 1.0f;
            break;

        default:
            aResult = -1.0;
            rv = NS_ERROR_FAILURE;
            break;
    }
    return rv;
}


bool
nsLookAndFeel::GetEchoPasswordImpl()
{
    if (!mInitializedShowPassword) {
        if (XRE_GetProcessType() == GeckoProcessType_Default) {
            if (AndroidBridge::Bridge())
                mShowPassword = AndroidBridge::Bridge()->GetShowPasswordSetting();
            else
                NS_ASSERTION(AndroidBridge::Bridge() != nsnull, "AndroidBridge is not available!");
        } else {
            ContentChild::GetSingleton()->SendGetShowPasswordSetting(&mShowPassword);
        }
        mInitializedShowPassword = true;
    }
    return mShowPassword;
}
