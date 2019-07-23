








































#include <Font.h>
#include <Menu.h>

#include "nsIDeviceContext.h"
#include "nsUnitConversion.h"
#include "nsSystemFontsBeOS.h"

#define DEFAULT_PIXEL_FONT_SIZE 16.0f

nsSystemFontsBeOS::nsSystemFontsBeOS()
  : mDefaultFontName(NS_LITERAL_STRING("sans-serif"))
  , mMenuFontName(NS_LITERAL_STRING("sans-serif"))
  , mCaptionFontName(NS_LITERAL_STRING("sans-serif"))
  , mDefaultFontStyle(FONT_STYLE_NORMAL, FONT_WEIGHT_NORMAL,
                 DEFAULT_PIXEL_FONT_SIZE, NS_LITERAL_CSTRING(""),
                 0.0f, PR_TRUE, PR_FALSE)
  , mMenuFontStyle(FONT_STYLE_NORMAL, FONT_WEIGHT_NORMAL,
               DEFAULT_PIXEL_FONT_SIZE, NS_LITERAL_CSTRING(""),
               0.0f, PR_TRUE, PR_FALSE)
  , mCaptionFontStyle(FONT_STYLE_NORMAL, FONT_WEIGHT_NORMAL,
               DEFAULT_PIXEL_FONT_SIZE, NS_LITERAL_CSTRING(""),
               0.0f, PR_TRUE, PR_FALSE)
{
  menu_info info;
  get_menu_info(&info);
  BFont menuFont;
  menuFont.SetFamilyAndStyle(info.f_family, info.f_style);
  menuFont.SetSize(info.font_size);

  GetSystemFontInfo(be_plain_font, &mDefaultFontName, &mDefaultFontStyle);
  GetSystemFontInfo(be_bold_font, &mCaptionFontName, &mCaptionFontStyle);
  GetSystemFontInfo(&menuFont, &mMenuFontName, &mMenuFontStyle);
}

nsresult nsSystemFontsBeOS::GetSystemFont(nsSystemFontID aID,
                                          nsString *aFontName,
                                          gfxFontStyle *aFontStyle) const
{
  switch (aID) {
    case eSystemFont_PullDownMenu: 
    case eSystemFont_Menu:
      *aFontName = mMenuFontName;
      *aFontStyle = mMenuFontStyle;
      return NS_OK;
    case eSystemFont_Caption:             
      *aFontName = mCaptionFontName;
      *aFontStyle = mCaptionFontStyle;
      return NS_OK;
    case eSystemFont_List:   
    case eSystemFont_Field:
    case eSystemFont_Icon : 
    case eSystemFont_MessageBox : 
    case eSystemFont_SmallCaption : 
    case eSystemFont_StatusBar : 
    case eSystemFont_Window:              
    case eSystemFont_Document: 
    case eSystemFont_Workspace: 
    case eSystemFont_Desktop: 
    case eSystemFont_Info: 
    case eSystemFont_Dialog: 
    case eSystemFont_Button: 
    case eSystemFont_Tooltips:            
    case eSystemFont_Widget: 
    default:
      *aFontName = mDefaultFontName;
      *aFontStyle = mDefaultFontStyle;
      return NS_OK;
  }
  NS_NOTREACHED("huh?");
}

nsresult 
nsSystemFontsBeOS::GetSystemFontInfo(const BFont *theFont, nsString *aFontName,
                                     gfxFontStyle *aFontStyle) const 
{ 
 
  aFontStyle->style       = FONT_STYLE_NORMAL; 
  aFontStyle->weight      = FONT_WEIGHT_NORMAL; 
  aFontStyle->decorations = FONT_DECORATION_NONE; 
  
  font_family family; 
  theFont->GetFamilyAndStyle(&family, NULL);

  uint16 face = theFont->Face();
  CopyUTF8toUTF16(family, *aFontName);
  aFontStyle->size = theFont->Size();

  if (face & B_ITALIC_FACE)
    aFontStyle->style = FONT_STYLE_ITALIC;

  if (face & B_BOLD_FACE)
    aFontStyle->weight = FONT_WEIGHT_BOLD;

  if (face & B_UNDERSCORE_FACE)
    aFontStyle->decorations |= FONT_DECORATION_UNDERLINE;

  if (face & B_STRIKEOUT_FACE)
    aFontStyle->decorations |= FONT_DECORATION_STRIKEOUT;

  aFontStyle->systemFont = PR_TRUE;

  return NS_OK;
}

