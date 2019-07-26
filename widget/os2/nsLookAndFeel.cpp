




#define INCL_WIN
#define INCL_WINWINDOWMGR
#define INCL_WINSHELLDATA
#define INCL_DOSNLS
#define INCL_DOSERRORS
#include <os2.h>
#include <stdlib.h>

#include "nsLookAndFeel.h"
#include "nsFont.h"
#include "nsSize.h"
#include "nsStyleConsts.h"

static bool bIsDBCS;
static bool bIsDBCSSet = false;

nsLookAndFeel::nsLookAndFeel() : nsXPLookAndFeel()
{
}

nsLookAndFeel::~nsLookAndFeel()
{
}

nsresult
nsLookAndFeel::NativeGetColor(ColorID aID, nscolor &aColor)
{
  nsresult res = NS_OK;

  int idx;
  switch (aID) {
    case eColorID_WindowBackground:
        idx = SYSCLR_WINDOW;
        break;
    case eColorID_WindowForeground:
        idx = SYSCLR_WINDOWTEXT;
        break;
    case eColorID_WidgetBackground:
        idx = SYSCLR_BUTTONMIDDLE;
        break;
    case eColorID_WidgetForeground:
        idx = SYSCLR_WINDOWTEXT;
        break;
    case eColorID_WidgetSelectBackground:
        idx = SYSCLR_HILITEBACKGROUND;
        break;
    case eColorID_WidgetSelectForeground:
        idx = SYSCLR_HILITEFOREGROUND;
        break;
    case eColorID_Widget3DHighlight:
        idx = SYSCLR_BUTTONLIGHT;
        break;
    case eColorID_Widget3DShadow:
        idx = SYSCLR_BUTTONDARK;
        break;
    case eColorID_TextBackground:
        idx = SYSCLR_WINDOW;
        break;
    case eColorID_TextForeground:
        idx = SYSCLR_WINDOWTEXT;
        break;
    case eColorID_TextSelectBackground:
    case eColorID_IMESelectedRawTextBackground:
    case eColorID_IMESelectedConvertedTextBackground:
        idx = SYSCLR_HILITEBACKGROUND;
        break;
    case eColorID_TextSelectForeground:
    case eColorID_IMESelectedRawTextForeground:
    case eColorID_IMESelectedConvertedTextForeground:
        idx = SYSCLR_HILITEFOREGROUND;
        break;
    case eColorID_IMERawInputBackground:
    case eColorID_IMEConvertedTextBackground:
        aColor = NS_TRANSPARENT;
        return NS_OK;
    case eColorID_IMERawInputForeground:
    case eColorID_IMEConvertedTextForeground:
        aColor = NS_SAME_AS_FOREGROUND_COLOR;
        return NS_OK;
    case eColorID_IMERawInputUnderline:
    case eColorID_IMEConvertedTextUnderline:
        aColor = NS_SAME_AS_FOREGROUND_COLOR;
        return NS_OK;
    case eColorID_IMESelectedRawTextUnderline:
    case eColorID_IMESelectedConvertedTextUnderline:
        aColor = NS_TRANSPARENT;
        return NS_OK;
    case eColorID_SpellCheckerUnderline:
        aColor = NS_RGB(0xff, 0, 0);
        return NS_OK;

    
    case eColorID_activeborder:
      idx = SYSCLR_ACTIVEBORDER;
      break;
    case eColorID_activecaption:
      idx = SYSCLR_ACTIVETITLETEXT;
      break;
    case eColorID_appworkspace:
      idx = SYSCLR_APPWORKSPACE;
      break;
    case eColorID_background:
      idx = SYSCLR_BACKGROUND;
      break;
    case eColorID_buttonface:
    case eColorID__moz_buttonhoverface:
      idx = SYSCLR_BUTTONMIDDLE;
      break;
    case eColorID_buttonhighlight:
      idx = SYSCLR_BUTTONLIGHT;
      break;
    case eColorID_buttonshadow:
      idx = SYSCLR_BUTTONDARK;
      break;
    case eColorID_buttontext:
    case eColorID__moz_buttonhovertext:
      idx = SYSCLR_MENUTEXT;
      break;
    case eColorID_captiontext:
      idx = SYSCLR_WINDOWTEXT;
      break;
    case eColorID_graytext:
      idx = SYSCLR_MENUDISABLEDTEXT;
      break;
    case eColorID_highlight:
    case eColorID__moz_html_cellhighlight:
      idx = SYSCLR_HILITEBACKGROUND;
      break;
    case eColorID_highlighttext:
    case eColorID__moz_html_cellhighlighttext:
      idx = SYSCLR_HILITEFOREGROUND;
      break;
    case eColorID_inactiveborder:
      idx = SYSCLR_INACTIVEBORDER;
      break;
    case eColorID_inactivecaption:
      idx = SYSCLR_INACTIVETITLE;
      break;
    case eColorID_inactivecaptiontext:
      idx = SYSCLR_INACTIVETITLETEXT;
      break;
    case eColorID_infobackground:
      aColor = NS_RGB( 255, 255, 228);
      return res;
    case eColorID_infotext:
      idx = SYSCLR_WINDOWTEXT;
      break;
    case eColorID_menu:
      idx = SYSCLR_MENU;
      break;
    case eColorID_menutext:
    case eColorID__moz_menubartext:
      idx = SYSCLR_MENUTEXT;
      break;
    case eColorID_scrollbar:
      idx = SYSCLR_SCROLLBAR;
      break;
    case eColorID_threeddarkshadow:
      idx = SYSCLR_BUTTONDARK;
      break;
    case eColorID_threedface:
      idx = SYSCLR_BUTTONMIDDLE;
      break;
    case eColorID_threedhighlight:
      idx = SYSCLR_BUTTONLIGHT;
      break;
    case eColorID_threedlightshadow:
      idx = SYSCLR_BUTTONMIDDLE;
      break;
    case eColorID_threedshadow:
      idx = SYSCLR_BUTTONDARK;
      break;
    case eColorID_window:
      idx = SYSCLR_WINDOW;
      break;
    case eColorID_windowframe:
      idx = SYSCLR_WINDOWFRAME;
      break;
    case eColorID_windowtext:
      idx = SYSCLR_WINDOWTEXT;
      break;
    case eColorID__moz_eventreerow:
    case eColorID__moz_oddtreerow:
    case eColorID__moz_field:
    case eColorID__moz_combobox:
      idx = SYSCLR_ENTRYFIELD;
      break;
    case eColorID__moz_fieldtext:
    case eColorID__moz_comboboxtext:
      idx = SYSCLR_WINDOWTEXT;
      break;
    case eColorID__moz_dialog:
    case eColorID__moz_cellhighlight:
      idx = SYSCLR_DIALOGBACKGROUND;
      break;
    case eColorID__moz_dialogtext:
    case eColorID__moz_cellhighlighttext:
      idx = SYSCLR_WINDOWTEXT;
      break;
    case eColorID__moz_buttondefault:
      idx = SYSCLR_BUTTONDEFAULT;
      break;
    case eColorID__moz_menuhover:
      if (WinQuerySysColor(HWND_DESKTOP, SYSCLR_MENUHILITEBGND, 0) ==
          WinQuerySysColor(HWND_DESKTOP, SYSCLR_MENU, 0)) {
        
        
        
        aColor = NS_RGB( 132, 130, 132);
        return res;
      } else {
        idx = SYSCLR_MENUHILITEBGND;
      }
      break;
    case eColorID__moz_menuhovertext:
    case eColorID__moz_menubarhovertext:
      if (WinQuerySysColor(HWND_DESKTOP, SYSCLR_MENUHILITEBGND, 0) ==
          WinQuerySysColor(HWND_DESKTOP, SYSCLR_MENU, 0)) {
        
        aColor = NS_RGB( 255, 255, 255);
        return res;
      } else {
        idx = SYSCLR_MENUHILITE;
      }
      break;
    case eColorID__moz_nativehyperlinktext:
      aColor = NS_RGB( 0, 0, 255);
      return res;
    default:
      idx = SYSCLR_WINDOW;
      break;
  }

  long lColor = WinQuerySysColor( HWND_DESKTOP, idx, 0);

  int iRed = (lColor & RGB_RED) >> 16;
  int iGreen = (lColor & RGB_GREEN) >> 8;
  int iBlue = (lColor & RGB_BLUE);

  aColor = NS_RGB( iRed, iGreen, iBlue);

  return res;
}

nsresult
nsLookAndFeel::GetIntImpl(IntID aID, int32_t &aResult)
{
  nsresult res = nsXPLookAndFeel::GetIntImpl(aID, aResult);
  if (NS_SUCCEEDED(res))
      return res;
  res = NS_OK;

  switch (aID) {
    case eIntID_CaretBlinkTime:
        aResult = WinQuerySysValue( HWND_DESKTOP, SV_CURSORRATE);
        break;
    case eIntID_CaretWidth:
        aResult = 1;
        break;
    case eIntID_ShowCaretDuringSelection:
        aResult = 0;
        break;
    case eIntID_SelectTextfieldsOnKeyFocus:
        
        
        aResult = 0;
        break;
    case eIntID_SubmenuDelay:
        aResult = 300;
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
    case eIntID_TreeOpenDelay:
        aResult = 1000;
        break;
    case eIntID_TreeCloseDelay:
        aResult = 0;
        break;
    case eIntID_TreeLazyScrollDelay:
        aResult = 150;
        break;
    case eIntID_TreeScrollDelay:
        aResult = 100;
        break;
    case eIntID_TreeScrollLinesMax:
        aResult = 3;
        break;
    case eIntID_DWMCompositor:
    case eIntID_WindowsClassic:
    case eIntID_WindowsDefaultTheme:
    case eIntID_TouchEnabled:
    case eIntID_WindowsThemeIdentifier:
        aResult = 0;
        res = NS_ERROR_NOT_IMPLEMENTED;
        break;
    case eIntID_MacGraphiteTheme:
    case eIntID_MacLionTheme:
    case eIntID_MaemoClassic:
        aResult = 0;
        res = NS_ERROR_NOT_IMPLEMENTED;
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
        res = NS_ERROR_FAILURE;
  }
  return res;
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


static bool
IsDBCS()
{
  if (!bIsDBCSSet) {
    APIRET rc;
    COUNTRYCODE ctrycodeInfo = {0};
    CHAR        achDBCSInfo[12] = {0};  
    ctrycodeInfo.country  = 0;          
    ctrycodeInfo.codepage = 0;          

    rc = DosQueryDBCSEnv(sizeof(achDBCSInfo), &ctrycodeInfo, achDBCSInfo);
    if (rc == NO_ERROR) {
      
      
      bIsDBCS = (achDBCSInfo[0] != 0 || achDBCSInfo[1] != 0 ||
                 achDBCSInfo[2] != 0 || achDBCSInfo[3] != 0);
    } else {
      bIsDBCS = false;
    }
    bIsDBCSSet = true;
  }
  return bIsDBCS;
}


static void
QueryFontFromINI(char* fontType, char* fontName, ULONG ulLength)
{
  ULONG ulMaxNameL = ulLength;

  
  
  if (PrfQueryProfileData(HINI_USER,
                          (PCSZ)"PM_SystemFonts",
                          (PCSZ)fontType,
                          fontName, &ulMaxNameL)) {
    
    fontName[ulMaxNameL] = '\0';
  } else {
    
    
    if (IsDBCS()) {
      strcpy(fontName, "9.WarpSans Combined");
    } else {
      strcpy(fontName, "9.WarpSans");
    }
  }
}















bool
nsLookAndFeel::GetFont(FontID aID, nsString& aFontName,
                       gfxFontStyle& aFontStyle)
{
  char szFontNameSize[MAXNAMEL];

  switch (aID)
    {
    case eFont_Icon:
      QueryFontFromINI("IconText", szFontNameSize, MAXNAMEL);
      break;

    case eFont_Menu:
      QueryFontFromINI("Menus", szFontNameSize, MAXNAMEL);
      break;

    case eFont_Caption:
    case eFont_MessageBox:
    case eFont_SmallCaption:
    case eFont_StatusBar:
    case eFont_Tooltips:
    case eFont_Widget:

    case eFont_Window:      
    case eFont_Document:
    case eFont_Workspace:
    case eFont_Desktop:
    case eFont_Info:
    case eFont_Dialog:
    case eFont_Button:
    case eFont_PullDownMenu:
    case eFont_List:
    case eFont_Field:
      QueryFontFromINI("WindowText", szFontNameSize, MAXNAMEL);
      break;

    default:
      NS_WARNING("None of the listed font types, using WarpSans");
      if (!IsDBCS()) {
        strcpy(szFontNameSize, "9.WarpSans");
      } else {
        strcpy(szFontNameSize, "9.WarpSans Combined");
      }
    }

  char *szFacename = strchr(szFontNameSize, '.');
  if (!szFacename || (*(szFacename++) == '\0'))
    return false;

  
  aFontStyle.size = atof(szFontNameSize);

  
  
  HPS ps = WinGetScreenPS(HWND_DESKTOP);
  HDC dc = GpiQueryDevice(ps);
  
  LONG vertScreenRes = 120; 
  DevQueryCaps(dc, CAPS_VERTICAL_FONT_RES, 1, &vertScreenRes);
  WinReleasePS(ps);

  
  aFontStyle.size *= vertScreenRes / 72.0;

  NS_ConvertUTF8toUTF16 fontFace(szFacename);
  int pos = 0;

  
  aFontStyle.systemFont = true;

  
  
  NS_NAMED_LITERAL_CSTRING(spcBold, " Bold");
  if ((pos = fontFace.Find(spcBold.get(), false, 0, -1)) > -1) {
    aFontStyle.weight = NS_FONT_WEIGHT_BOLD;
    
    fontFace.Cut(pos, spcBold.Length());
  } else {
    aFontStyle.weight = NS_FONT_WEIGHT_NORMAL;
  }

  
  aFontStyle.stretch = NS_FONT_STRETCH_NORMAL;

  
  NS_NAMED_LITERAL_CSTRING(spcItalic, " Italic");
  NS_NAMED_LITERAL_CSTRING(spcOblique, " Oblique");
  NS_NAMED_LITERAL_CSTRING(spcObli, " Obli");
  if ((pos = fontFace.Find(spcItalic.get(), false, 0, -1)) > -1) {
    aFontStyle.style = NS_FONT_STYLE_ITALIC;
    fontFace.Cut(pos, spcItalic.Length());
  } else if ((pos = fontFace.Find(spcOblique.get(), false, 0, -1)) > -1) {
    
    
    aFontStyle.style = NS_FONT_STYLE_OBLIQUE;
    fontFace.Cut(pos, spcOblique.Length());
  } else if ((pos = fontFace.Find(spcObli.get(), false, 0, -1)) > -1) {
    
    
    aFontStyle.style = NS_FONT_STYLE_OBLIQUE;
    
    
    
    fontFace.Cut(pos, fontFace.Length());
  } else {
    aFontStyle.style = NS_FONT_STYLE_NORMAL;
  }

  
  
  
  if ((pos = fontFace.Find(".", false, 0, -1)) > -1) {
    fontFace.Cut(pos, fontFace.Length());
  }

  
  NS_NAMED_LITERAL_STRING(quote, "\"");
  aFontName = quote + fontFace + quote;

  return true;
}
