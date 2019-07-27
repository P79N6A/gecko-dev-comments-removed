















#include <QGuiApplication>
#include <QFont>
#include <QScreen>
#include <QPalette>

#include "nsLookAndFeel.h"
#include "nsStyleConsts.h"
#include "gfxFont.h"
#include "gfxFontConstants.h"
#include "mozilla/gfx/2D.h"

static const char16_t UNICODE_BULLET = 0x2022;

#define QCOLOR_TO_NS_RGB(c)                     \
  ((nscolor)NS_RGB(c.red(),c.green(),c.blue()))

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

#define BG_PRELIGHT_COLOR     NS_RGB(0xee,0xee,0xee)
#define FG_PRELIGHT_COLOR     NS_RGB(0x77,0x77,0x77)
#define RED_COLOR             NS_RGB(0xff,0x00,0x00)

    QPalette palette = QGuiApplication::palette();

    switch (aID) {
        
        
        
    case eColorID_WindowBackground:
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Window));
        break;
    case eColorID_WindowForeground:
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::WindowText));
        break;
    case eColorID_WidgetBackground:
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Window));
        break;
    case eColorID_WidgetForeground:
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::WindowText));
        break;
    case eColorID_WidgetSelectBackground:
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Window));
        break;
    case eColorID_WidgetSelectForeground:
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::WindowText));
        break;
    case eColorID_Widget3DHighlight:
        aColor = NS_RGB(0xa0,0xa0,0xa0);
        break;
    case eColorID_Widget3DShadow:
        aColor = NS_RGB(0x40,0x40,0x40);
        break;
    case eColorID_TextBackground:
        
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Window));
        break;
    case eColorID_TextForeground:
        
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::WindowText));
        break;
    case eColorID_TextSelectBackground:
    case eColorID_IMESelectedRawTextBackground:
    case eColorID_IMESelectedConvertedTextBackground:
        
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Highlight));
        break;
    case eColorID_TextSelectForeground:
    case eColorID_IMESelectedRawTextForeground:
    case eColorID_IMESelectedConvertedTextForeground:
        
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::HighlightedText));
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
        
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Window));
        break;
    case eColorID_activecaption:
        
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Window));
        break;
    case eColorID_appworkspace:
        
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Window));
        break;
    case eColorID_background:
        
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Window));
        break;
    case eColorID_captiontext:
        
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Text));
        break;
    case eColorID_graytext:
        
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Disabled, QPalette::Text));
        break;
    case eColorID_highlight:
        
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Highlight));
        break;
    case eColorID_highlighttext:
        
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::HighlightedText));
        break;
    case eColorID_inactiveborder:
        
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Disabled, QPalette::Window));
        break;
    case eColorID_inactivecaption:
        
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Disabled, QPalette::Window));
        break;
    case eColorID_inactivecaptiontext:
        
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Disabled, QPalette::Text));
        break;
    case eColorID_infobackground:
        
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::ToolTipBase));
        break;
    case eColorID_infotext:
        
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::ToolTipText));
        break;
    case eColorID_menu:
        
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Window));
        break;
    case eColorID_menutext:
        
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Text));
        break;
    case eColorID_scrollbar:
        
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Mid));
        break;

    case eColorID_threedface:
    case eColorID_buttonface:
        
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Button));
        break;

    case eColorID_buttontext:
        
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::ButtonText));
        break;

    case eColorID_buttonhighlight:
        
    case eColorID_threedhighlight:
        
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Dark));
        break;

    case eColorID_threedlightshadow:
        
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Light));
        break;

    case eColorID_buttonshadow:
        
    case eColorID_threedshadow:
        
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Dark));
        break;

    case eColorID_threeddarkshadow:
        
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Shadow));
        break;

    case eColorID_window:
    case eColorID_windowframe:
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Window));
        break;

    case eColorID_windowtext:
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Text));
        break;

    case eColorID__moz_eventreerow:
    case eColorID__moz_field:
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Base));
        break;
    case eColorID__moz_fieldtext:
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Text));
        break;
    case eColorID__moz_dialog:
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Window));
        break;
    case eColorID__moz_dialogtext:
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::WindowText));
        break;
    case eColorID__moz_dragtargetzone:
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Window));
        break;
    case eColorID__moz_buttondefault:
        
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Button));
        break;
    case eColorID__moz_buttonhoverface:
        aColor = BG_PRELIGHT_COLOR;
        break;
    case eColorID__moz_buttonhovertext:
        aColor = FG_PRELIGHT_COLOR;
        break;
    case eColorID__moz_cellhighlight:
    case eColorID__moz_html_cellhighlight:
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Highlight));
        break;
    case eColorID__moz_cellhighlighttext:
    case eColorID__moz_html_cellhighlighttext:
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::HighlightedText));
        break;
    case eColorID__moz_menuhover:
        aColor = BG_PRELIGHT_COLOR;
        break;
    case eColorID__moz_menuhovertext:
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Text));
        break;
    case eColorID__moz_oddtreerow:
        aColor = NS_TRANSPARENT;
        break;
    case eColorID__moz_nativehyperlinktext:
        aColor = NS_SAME_AS_FOREGROUND_COLOR;
        break;
    case eColorID__moz_comboboxtext:
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Text));
        break;
    case eColorID__moz_combobox:
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Base));
        break;
    case eColorID__moz_menubartext:
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Text));
        break;
    case eColorID__moz_menubarhovertext:
        aColor = QCOLOR_TO_NS_RGB(palette.color(QPalette::Normal, QPalette::Text));
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
        case eIntID_WindowsThemeIdentifier:
        case eIntID_OperatingSystemVersionIdentifier:
            aResult = 0;
            rv = NS_ERROR_NOT_IMPLEMENTED;
            break;

        case eIntID_IMERawInputUnderlineStyle:
        case eIntID_IMEConvertedTextUnderlineStyle:
            aResult = NS_STYLE_TEXT_DECORATION_STYLE_SOLID;
            break;

        case eIntID_IMESelectedRawTextUnderlineStyle:
        case eIntID_IMESelectedConvertedTextUnderline:
            aResult = NS_STYLE_TEXT_DECORATION_STYLE_NONE;
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

nsresult
nsLookAndFeel::GetFloatImpl(FloatID aID, float &aResult)
{
  nsresult res = nsXPLookAndFeel::GetFloatImpl(aID, aResult);
  if (NS_SUCCEEDED(res))
    return res;
  res = NS_OK;

  switch (aID) {
    case eFloatID_IMEUnderlineRelativeSize:
        aResult = 1.0f;
        break;
    case eFloatID_SpellCheckerUnderlineRelativeSize:
        aResult = 1.0f;
        break;
    default:
        aResult = -1.0;
        res = NS_ERROR_FAILURE;
    }
  return res;
}


bool
nsLookAndFeel::GetFontImpl(FontID aID, nsString& aFontName,
                           gfxFontStyle& aFontStyle,
                           float aDevPixPerCSSPixel)
{
  QFont qFont = QGuiApplication::font();

  NS_NAMED_LITERAL_STRING(quote, "\"");
  nsString family((char16_t*)qFont.family().data());
  aFontName = quote + family + quote;

  aFontStyle.systemFont = true;
  aFontStyle.style = qFont.style();
  aFontStyle.weight = qFont.weight();
  aFontStyle.stretch = qFont.stretch();
  
  if (qFont.pixelSize() != -1) {
    aFontStyle.size = qFont.pixelSize();
  } else {
    aFontStyle.size = qFont.pointSizeF() * qApp->primaryScreen()->logicalDotsPerInch() / 72.0f;
  }

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


char16_t
nsLookAndFeel::GetPasswordCharacterImpl()
{
    return UNICODE_BULLET;
}
