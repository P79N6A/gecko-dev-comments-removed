





































#include "nsIDeviceContext.h"
#include "nsIRenderingContext.h"

#include "nsSystemFontsQt.h"
#include "gfxQtPlatform.h"
#include <QApplication>
#include <QFont>

nsSystemFontsQt::nsSystemFontsQt()
  : mDefaultFontName(NS_LITERAL_STRING("sans-serif"))
  , mButtonFontName(NS_LITERAL_STRING("sans-serif"))
  , mFieldFontName(NS_LITERAL_STRING("sans-serif"))
  , mMenuFontName(NS_LITERAL_STRING("sans-serif"))
{
   
   GetSystemFontInfo("Qlabel", &mDefaultFontName, &mDefaultFontStyle);

   GetSystemFontInfo("QlineEdit", &mFieldFontName, &mFieldFontStyle);

   GetSystemFontInfo("QAction", &mMenuFontName, &mMenuFontStyle);
  
   GetSystemFontInfo("QPushButton", &mButtonFontName, &mButtonFontStyle);
}

nsSystemFontsQt::~nsSystemFontsQt()
{
    
}

nsresult
nsSystemFontsQt::GetSystemFontInfo(const char *aClassName, nsString *aFontName,
                                   gfxFontStyle *aFontStyle) const
{
    QFont qFont = QApplication::font(aClassName);

    aFontStyle->style = FONT_STYLE_NORMAL;
    aFontStyle->systemFont = PR_TRUE;
    NS_NAMED_LITERAL_STRING(quote, "\"");
    nsString family((PRUnichar*)qFont.family().data());
    *aFontName = quote + family + quote;
    aFontStyle->weight = qFont.weight();
    
    aFontStyle->stretch = NS_FONT_STRETCH_NORMAL;
    aFontStyle->size = qFont.pointSizeF() * float(gfxQtPlatform::GetPlatformDPI()) / 72.0f;
    return NS_OK;
}


nsresult
nsSystemFontsQt::GetSystemFont(nsSystemFontID anID, nsString *aFontName,
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

