




































#include "nsIDeviceContext.h"
#include "nsIRenderingContext.h"

#include "nsSystemFontsAndroid.h"

#include "gfxPlatform.h"

#define DEFAULT_FONT  "Droid Sans"

nsSystemFontsAndroid::nsSystemFontsAndroid()
  : mDefaultFontName(NS_LITERAL_STRING(DEFAULT_FONT))
  , mButtonFontName(NS_LITERAL_STRING(DEFAULT_FONT))
  , mFieldFontName(NS_LITERAL_STRING(DEFAULT_FONT))
  , mMenuFontName(NS_LITERAL_STRING(DEFAULT_FONT))
{
}

nsSystemFontsAndroid::~nsSystemFontsAndroid()
{
}

nsresult
nsSystemFontsAndroid::GetSystemFontInfo(const char *aClassName, nsString *aFontName,
                                        gfxFontStyle *aFontStyle) const
{
    aFontStyle->style = FONT_STYLE_NORMAL;
    aFontStyle->systemFont = PR_TRUE;
    *aFontName = NS_LITERAL_STRING("Droid Sans");
    aFontStyle->weight = 400;
    aFontStyle->stretch = NS_FONT_STRETCH_NORMAL;
    aFontStyle->size = 9.0 * 96.0f / 72.0f;
    return NS_OK;
}


nsresult
nsSystemFontsAndroid::GetSystemFont(nsSystemFontID anID, nsString *aFontName,
                                    gfxFontStyle *aFontStyle) const
{
    switch (anID) {
    case eSystemFont_Menu:         
    case eSystemFont_PullDownMenu: 
        *aFontName = mMenuFontName;
        *aFontStyle = mMenuFontStyle;
        break;

    case eSystemFont_Field:        
    case eSystemFont_List:         
        *aFontName = mFieldFontName;
        *aFontStyle = mFieldFontStyle;
        break;

    case eSystemFont_Button:       
        *aFontName = mButtonFontName;
        *aFontStyle = mButtonFontStyle;
        break;

    case eSystemFont_Caption:      
    case eSystemFont_Icon:         
    case eSystemFont_MessageBox:   
    case eSystemFont_SmallCaption: 
    case eSystemFont_StatusBar:    
    case eSystemFont_Window:       
    case eSystemFont_Document:     
    case eSystemFont_Workspace:    
    case eSystemFont_Desktop:      
    case eSystemFont_Info:         
    case eSystemFont_Dialog:       
    case eSystemFont_Tooltips:     
    case eSystemFont_Widget:       
        *aFontName = mDefaultFontName;
        *aFontStyle = mDefaultFontStyle;
        break;
    }

    return NS_OK;
}

