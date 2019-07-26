















#include "nsLookAndFeel.h"
#include "nsStyleConsts.h"
#include "gfxFont.h"

static const PRUnichar UNICODE_BULLET = 0x2022;

nsLookAndFeel::nsLookAndFeel()
    : nsXPLookAndFeel()
{
}

nsLookAndFeel::~nsLookAndFeel()
{
}

nsresult
nsLookAndFeel::NativeGetColor(ColorID aID, nscolor &aColor)
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
        
        
        
    case eColorID_WindowBackground:
        aColor = BASE_NORMAL_COLOR;
        break;
    case eColorID_WindowForeground:
        aColor = TEXT_NORMAL_COLOR;
        break;
    case eColorID_WidgetBackground:
        aColor = BG_NORMAL_COLOR;
        break;
    case eColorID_WidgetForeground:
        aColor = FG_NORMAL_COLOR;
        break;
    case eColorID_WidgetSelectBackground:
        aColor = BG_SELECTED_COLOR;
        break;
    case eColorID_WidgetSelectForeground:
        aColor = FG_SELECTED_COLOR;
        break;
    case eColorID_Widget3DHighlight:
        aColor = NS_RGB(0xa0,0xa0,0xa0);
        break;
    case eColorID_Widget3DShadow:
        aColor = NS_RGB(0x40,0x40,0x40);
        break;
    case eColorID_TextBackground:
        
        aColor = BASE_NORMAL_COLOR;
        break;
    case eColorID_TextForeground: 
        
        aColor = TEXT_NORMAL_COLOR;
        break;
    case eColorID_TextSelectBackground:
    case eColorID_IMESelectedRawTextBackground:
    case eColorID_IMESelectedConvertedTextBackground:
        
        aColor = BASE_SELECTED_COLOR;
        break;
    case eColorID_TextSelectForeground:
    case eColorID_IMESelectedRawTextForeground:
    case eColorID_IMESelectedConvertedTextForeground:
        
        aColor = TEXT_SELECTED_COLOR;
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
      aColor = NS_RGB(0xff, 0, 0);
      break;

        
    case eColorID_activeborder:
        
        aColor = BG_NORMAL_COLOR;
        break;
    case eColorID_activecaption:
        
        aColor = BG_NORMAL_COLOR;
        break;
    case eColorID_appworkspace:
        
        aColor = BG_NORMAL_COLOR;
        break;
    case eColorID_background:
        
        aColor = BG_NORMAL_COLOR;
        break;
    case eColorID_captiontext:
        
        aColor = FG_NORMAL_COLOR;
        break;
    case eColorID_graytext:
        
        aColor = FG_INSENSITIVE_COLOR;
        break;
    case eColorID_highlight:
        
        aColor = BASE_SELECTED_COLOR;
        break;
    case eColorID_highlighttext:
        
        aColor = TEXT_SELECTED_COLOR;
        break;
    case eColorID_inactiveborder:
        
        aColor = BG_NORMAL_COLOR;
        break;
    case eColorID_inactivecaption:
        
        aColor = BG_INSENSITIVE_COLOR;
        break;
    case eColorID_inactivecaptiontext:
        
        aColor = FG_INSENSITIVE_COLOR;
        break;
    case eColorID_infobackground:
        
        aColor = BG_NORMAL_COLOR;
        break;
    case eColorID_infotext:
        
        aColor = TEXT_NORMAL_COLOR;
        break;
    case eColorID_menu:
        
        aColor = BG_NORMAL_COLOR;
        break;
    case eColorID_menutext:
        
        aColor = TEXT_NORMAL_COLOR;
        break;
    case eColorID_scrollbar:
        
        aColor = BG_ACTIVE_COLOR;
        break;

    case eColorID_threedface:
    case eColorID_buttonface:
        
        aColor = BG_NORMAL_COLOR;
        break;

    case eColorID_buttontext:
        
        aColor = TEXT_NORMAL_COLOR;
        break;

    case eColorID_buttonhighlight:
        
    case eColorID_threedhighlight:
        
        aColor = LIGHT_NORMAL_COLOR;
        break;

    case eColorID_threedlightshadow:
        
        aColor = BG_NORMAL_COLOR;
        break;

    case eColorID_buttonshadow:
        
    case eColorID_threedshadow:
        
        aColor = DARK_NORMAL_COLOR;
        break;

    case eColorID_threeddarkshadow:
        
        aColor = NS_RGB(0,0,0);
        break;

    case eColorID_window:
    case eColorID_windowframe:
        aColor = BG_NORMAL_COLOR;
        break;

    case eColorID_windowtext:
        aColor = FG_NORMAL_COLOR;
        break;

    case eColorID__moz_eventreerow:
    case eColorID__moz_field:
        aColor = BASE_NORMAL_COLOR;
        break;
    case eColorID__moz_fieldtext:
        aColor = TEXT_NORMAL_COLOR;
        break;
    case eColorID__moz_dialog:
        aColor = BG_NORMAL_COLOR;
        break;
    case eColorID__moz_dialogtext:
        aColor = FG_NORMAL_COLOR;
        break;
    case eColorID__moz_dragtargetzone:
        aColor = BG_SELECTED_COLOR;
        break; 
    case eColorID__moz_buttondefault:
        
        aColor = NS_RGB(0,0,0);
        break;
    case eColorID__moz_buttonhoverface:
        aColor = BG_PRELIGHT_COLOR;
        break;
    case eColorID__moz_buttonhovertext:
        aColor = FG_PRELIGHT_COLOR;
        break;
    case eColorID__moz_cellhighlight:
    case eColorID__moz_html_cellhighlight:
        aColor = BASE_ACTIVE_COLOR;
        break;
    case eColorID__moz_cellhighlighttext:
    case eColorID__moz_html_cellhighlighttext:
        aColor = TEXT_ACTIVE_COLOR;
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
        aColor = TEXT_NORMAL_COLOR;
        break;
    case eColorID__moz_combobox:
        aColor = BG_NORMAL_COLOR;
        break;
    case eColorID__moz_menubartext:
        aColor = TEXT_NORMAL_COLOR;
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
nsLookAndFeel::GetIntImpl(IntID aID, int32_t &aResult)
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

        case eIntID_TooltipDelay:
            aResult = 500;
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

        case eIntID_TouchEnabled:
            aResult = 1;
            break;

        case eIntID_WindowsDefaultTheme:
        case eIntID_MaemoClassic:
        case eIntID_WindowsThemeIdentifier:
            aResult = 0;
            rv = NS_ERROR_NOT_IMPLEMENTED;
            break;

        case eIntID_SpellCheckerUnderlineStyle:
            aResult = NS_STYLE_TEXT_DECORATION_STYLE_WAVY;
            break;

        case eIntID_ScrollbarButtonAutoRepeatBehavior:
            aResult = 0;
            break;

        default:
            aResult = 0;
            rv = NS_ERROR_FAILURE;
    }

    return rv;
}


bool
nsLookAndFeel::GetFontImpl(FontID aID, nsString& aFontName,
                           gfxFontStyle& aFontStyle,
                           float aDevPixPerCSSPixel)
{
    aFontName.AssignLiteral("\"Droid Sans\"");
    aFontStyle.style = NS_FONT_STYLE_NORMAL;
    aFontStyle.weight = NS_FONT_WEIGHT_NORMAL;
    aFontStyle.stretch = NS_FONT_STRETCH_NORMAL;
    aFontStyle.size = 9.0 * 96.0f / 72.0f;
    aFontStyle.systemFont = true;
    return true;
}


bool
nsLookAndFeel::GetEchoPasswordImpl() {
    return true;
}


uint32_t
nsLookAndFeel::GetPasswordMaskDelayImpl()
{
    
    return 1500;
}


PRUnichar
nsLookAndFeel::GetPasswordCharacterImpl()
{
    return UNICODE_BULLET;
}
