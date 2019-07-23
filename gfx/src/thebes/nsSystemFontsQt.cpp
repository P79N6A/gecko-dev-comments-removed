






































#include <stdlib.h>

#include "nsIDeviceContext.h"
#include "nsIRenderingContext.h"
#include "prlink.h"

#include <fontconfig/fontconfig.h>
#include "nsSystemFontsQt.h"
#include "gfxPlatformQt.h"
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QAction>
#include <QPushButton>

nsSystemFontsQt::nsSystemFontsQt()
  : mDefaultFontName(NS_LITERAL_STRING("sans-serif"))
  , mButtonFontName(NS_LITERAL_STRING("sans-serif"))
  , mFieldFontName(NS_LITERAL_STRING("sans-serif"))
  , mMenuFontName(NS_LITERAL_STRING("sans-serif"))
{
   
   QLabel *label = new QLabel();
   if (label) {
       GetSystemFontInfo(label->font(), &mDefaultFontName, &mDefaultFontStyle);
       delete label;
   }

   QLineEdit *entry = new QLineEdit();
   if (entry) {
      GetSystemFontInfo(entry->font(), &mFieldFontName, &mFieldFontStyle);
      delete entry;
   }

   QMenu *menu = new QMenu();
   QAction *action = new QAction(menu);
   if (action) {
      GetSystemFontInfo(action->font(), &mMenuFontName, &mMenuFontStyle);
      delete action;
   }

   QPushButton *button = new QPushButton();
   if (button) {
      GetSystemFontInfo(button->font(), &mButtonFontName, &mButtonFontStyle);
      delete button;
   }
}

nsSystemFontsQt::~nsSystemFontsQt()
{
}

nsresult
nsSystemFontsQt::GetSystemFontInfo(const QFont &aFont, nsString *aFontName,
                                     gfxFontStyle *aFontStyle) const
{
    aFontStyle->style       = FONT_STYLE_NORMAL;
    aFontStyle->systemFont  = PR_TRUE;
    NS_NAMED_LITERAL_STRING(quote, "\"");
    nsString family((PRUnichar*)aFont.family().data());
    *aFontName = quote + family + quote;
    aFontStyle->weight = aFont.weight();
    aFontStyle->size = aFont.pointSizeF();
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

