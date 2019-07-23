





































#include "nsLookAndFeel.h"
#include "nsFont.h"
#include "nsSize.h"

#include <InterfaceDefs.h>
#include <Menu.h>

nsLookAndFeel::nsLookAndFeel() : nsXPLookAndFeel()
{
}

nsLookAndFeel::~nsLookAndFeel()
{
}

nsresult nsLookAndFeel::NativeGetColor(const nsColorID aID, nscolor &aColor)
{
  nsresult res = NS_OK;
  rgb_color color;
  







  switch (aID) {
    case eColor_WindowBackground:
      aColor = NS_RGB(0xff, 0xff, 0xff); 
      break;
    case eColor_WindowForeground:
      aColor = NS_RGB(0x00, 0x00, 0x00);        
      break;
    case eColor_WidgetBackground:
      aColor = NS_RGB(0xdd, 0xdd, 0xdd);
      break;
    case eColor_WidgetForeground:
      aColor = NS_RGB(0x00, 0x00, 0x00);        
      break;
    case eColor_WidgetSelectBackground:
      {
        color = ui_color(B_MENU_SELECTION_BACKGROUND_COLOR);
        aColor = NS_RGB(color.red, color.green, color.blue);
      }
      break;
    case eColor_WidgetSelectForeground:
      aColor = NS_RGB(0x00, 0x00, 0x80);
      break;
    case eColor_Widget3DHighlight:
      aColor = NS_RGB(0xa0, 0xa0, 0xa0);
      break;
    case eColor_Widget3DShadow:
      aColor = NS_RGB(0x40, 0x40, 0x40);
      break;
    case eColor_TextBackground:
      aColor = NS_RGB(0xff, 0xff, 0xff);
      break;
    case eColor_TextForeground: 
      aColor = NS_RGB(0x00, 0x00, 0x00);
      break;
    case eColor_TextSelectBackground:
    case eColor_IMESelectedRawTextBackground:
    case eColor_IMESelectedConvertedTextBackground:
      {
        
        color = ui_color(B_MENU_SELECTION_BACKGROUND_COLOR);
        aColor = NS_RGB(color.red, color.green, color.blue);
      }
      break;
    case eColor_TextSelectForeground:
    case eColor_IMESelectedRawTextForeground:
    case eColor_IMESelectedConvertedTextForeground:
      {
        color = ui_color(B_MENU_SELECTED_ITEM_TEXT_COLOR);
        aColor = NS_RGB(color.red, color.green, color.blue);
      }
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
      aColor = NS_RGB(0x88, 0x88, 0x88);
      break;
    
    case eColor_activecaption:
      {
        color = ui_color(B_WINDOW_TAB_COLOR);
        aColor = NS_RGB(color.red, color.green, color.blue);
      }    
      break;
    
    case eColor_appworkspace:
      aColor = NS_RGB(0xd8, 0xd8, 0xd8);
      break;
    
    
    case eColor_background:
      {
        color = ui_color(B_DESKTOP_COLOR);
        aColor = NS_RGB(color.red, color.green, color.blue);
      }
      break;
    case eColor_buttonface:
    case eColor__moz_buttonhoverface:
      aColor = NS_RGB(0xdd, 0xdd, 0xdd);
      break;
    
    case eColor_buttonhighlight:
      aColor = NS_RGB(0xff, 0xff, 0xff);
      break;
    
    case eColor_buttonshadow:
      aColor = NS_RGB(0x77, 0x77, 0x77);
      break;
    case eColor_buttontext:
    case eColor__moz_buttonhovertext:
      aColor = NS_RGB(0x00, 0x00, 0x00);
      break;
    case eColor_captiontext:
      aColor = NS_RGB(0x00, 0x00, 0x00);
      break;
    case eColor_graytext:
      aColor = NS_RGB(0x77, 0x77, 0x77);
      break;
    case eColor_highlight:
    case eColor__moz_html_cellhighlight:
    case eColor__moz_menuhover:
      {
        
        
        color = ui_color(B_KEYBOARD_NAVIGATION_COLOR);
        aColor = NS_RGB(color.red, color.green, color.blue);
      }
      break;
    case eColor_highlighttext:
    case eColor__moz_html_cellhighlighttext:
    case eColor__moz_menuhovertext:
      {
        color = ui_color(B_MENU_SELECTED_ITEM_TEXT_COLOR);
        aColor = NS_RGB(color.red, color.green, color.blue);
      }
      break;
    case eColor_inactiveborder:
      aColor = NS_RGB(0x55, 0x55, 0x55);
      break;
    case eColor_inactivecaption:
      aColor = NS_RGB(0xdd, 0xdd, 0xdd);
      break;
    case eColor_inactivecaptiontext:
      aColor = NS_RGB(0x77, 0x77, 0x77);
      break;
    case eColor_infobackground:
      
      aColor = NS_RGB(0xff, 0xff, 0xd0);
      break;
    case eColor_infotext:
      aColor = NS_RGB(0x00, 0x00, 0x00);
      break;
    case eColor_menu:
      {
        color = ui_color(B_MENU_BACKGROUND_COLOR);
        aColor = NS_RGB(color.red, color.green, color.blue);
      }
      break;
    case eColor_menutext:
    case eColor__moz_menubartext:
      {
        color = ui_color(B_MENU_ITEM_TEXT_COLOR);
        aColor = NS_RGB(color.red, color.green, color.blue);
      }
      break;
    case eColor_scrollbar:
      aColor = NS_RGB(0xaa, 0xaa, 0xaa);
      break;
    case eColor_threeddarkshadow:
      aColor = NS_RGB(0x77, 0x77, 0x77);
      break;
    case eColor_threedface:
      aColor = NS_RGB(0xdd, 0xdd, 0xdd);
      break;
    case eColor_threedhighlight:
      aColor = NS_RGB(0xff, 0xff, 0xff);
      break;
    case eColor_threedlightshadow:
      aColor = NS_RGB(0xdd, 0xdd, 0xdd);
      break;
    case eColor_threedshadow:
      aColor = NS_RGB(0x99, 0x99, 0x99);
      break;
    case eColor_window:
      aColor = NS_RGB(0xff, 0xff, 0xff);
      break;
    case eColor_windowframe:
      aColor = NS_RGB(0xcc, 0xcc, 0xcc);
      break;
    case eColor_windowtext:
      aColor = NS_RGB(0x00, 0x00, 0x00);
      break;
    case eColor__moz_eventreerow:
    case eColor__moz_oddtreerow:
    case eColor__moz_field: 
    case eColor__moz_combobox:
      
      aColor = NS_RGB(0xff, 0xff, 0xff);
      break;  
    case eColor__moz_fieldtext:
    case eColor__moz_comboboxtext:
      aColor = NS_RGB(0x00, 0x00, 0x00);
      break;  
    case eColor__moz_dialog:
    case eColor__moz_cellhighlight:
      
      aColor = NS_RGB(0xdd, 0xdd, 0xdd);
      break;  
    case eColor__moz_dialogtext:
    case eColor__moz_cellhighlighttext:
      aColor = NS_RGB(0x00, 0x00, 0x00);
      break;  
    case eColor__moz_dragtargetzone:
      aColor = NS_RGB(0x63, 0x63, 0xCE);
      break;
    case eColor__moz_buttondefault:
      aColor = NS_RGB(0x77, 0x77, 0x77);
      break;
    case eColor_LAST_COLOR:
    default:
      aColor = NS_RGB(0xff, 0xff, 0xff);
      res    = NS_ERROR_FAILURE;
      break;
  }

  return res;
}
  
NS_IMETHODIMP nsLookAndFeel::GetMetric(const nsMetricID aID, PRInt32 & aMetric)
{
  nsresult res = nsXPLookAndFeel::GetMetric(aID, aMetric);
  if (NS_SUCCEEDED(res))
    return res;
  res = NS_OK;

  







  switch (aID) 
  {
    case eMetric_WindowTitleHeight:
      
      aMetric = 28;
      break;
    case eMetric_WindowBorderWidth:
      aMetric = 2;
      break;
    case eMetric_WindowBorderHeight:
      aMetric = 2;
      break;
    case eMetric_Widget3DBorder:
      aMetric = 5;
      break;
    case eMetric_TextFieldBorder:
      aMetric = 3;
      break;
    case eMetric_TextFieldHeight:
      aMetric = 24;
      break;
    case eMetric_TextVerticalInsidePadding:
      aMetric = 0;
      break;    
    case eMetric_TextShouldUseVerticalInsidePadding:
      aMetric = 0;
      break;
    case eMetric_TextHorizontalInsideMinimumPadding:
      aMetric = 3;
      break;
    case eMetric_TextShouldUseHorizontalInsideMinimumPadding:
      aMetric = 1;
      break;    
    case eMetric_ButtonHorizontalInsidePaddingNavQuirks:
      aMetric = 10;
      break;
    case eMetric_ButtonHorizontalInsidePaddingOffsetNavQuirks:
      aMetric = 8;
      break;
    case eMetric_CheckboxSize:
      aMetric = 12;
      break;
    case eMetric_RadioboxSize:
      aMetric = 12;
      break;

    case eMetric_ListShouldUseHorizontalInsideMinimumPadding:
      aMetric = 0;
      break;
    case eMetric_ListHorizontalInsideMinimumPadding:
      aMetric = 3;
      break;
      
    case eMetric_ListShouldUseVerticalInsidePadding:
      aMetric = 0;
      break;
    case eMetric_ListVerticalInsidePadding:
      aMetric = 0;
      break;
      
    case eMetric_CaretBlinkTime:
      aMetric = 500;
      break;
    case eMetric_CaretWidth:
      aMetric = 1;
      break;
    case eMetric_ShowCaretDuringSelection:
      aMetric = 1;
      break;
    case eMetric_SelectTextfieldsOnKeyFocus:
      
      
      aMetric = 0;
      break;
    case eMetric_SubmenuDelay:
      aMetric = 500;
      break;
    case eMetric_MenusCanOverlapOSBar: 
      aMetric = 0;
      break;
    case eMetric_ScrollArrowStyle:
      {
        aMetric = eMetric_ScrollArrowStyleBothAtEachEnd; 

        scroll_bar_info info;
        if(B_OK == get_scroll_bar_info(&info) && !info.double_arrows)
        {
          aMetric = eMetric_ScrollArrowStyleSingle;
        }
      }
      break;
    case eMetric_ScrollSliderStyle:
      {
        aMetric = eMetric_ScrollThumbStyleProportional; 

        scroll_bar_info info;
        if(B_OK == get_scroll_bar_info(&info) && !info.proportional)
        {
          aMetric = eMetric_ScrollThumbStyleNormal;
        }
      }
      break;
    case eMetric_TreeOpenDelay:
      aMetric = 1000;
      break;
    case eMetric_TreeCloseDelay:
      aMetric = 1000;
      break;
    case eMetric_TreeLazyScrollDelay:
      aMetric = 150;
      break;
    case eMetric_TreeScrollDelay:
      aMetric = 100;
      break;
    case eMetric_TreeScrollLinesMax:
      aMetric = 3;
      break;
    case eMetric_DWMCompositor:
    case eMetric_WindowsClassic:
    case eMetric_WindowsDefaultTheme:
    case eMetric_TouchEnabled:
      aMetric = 0;
      res = NS_ERROR_NOT_IMPLEMENTED;
      break;
    case eMetric_MacGraphiteTheme:
    case eMetric_MaemoClassic:
      aMetric = 0;
      res = NS_ERROR_NOT_IMPLEMENTED;
      break;
    case eMetric_IMERawInputUnderlineStyle:
    case eMetric_IMEConvertedTextUnderlineStyle:
      aMetric = NS_UNDERLINE_STYLE_SOLID;
      break;
    case eMetric_IMESelectedRawTextUnderlineStyle:
    case eMetric_IMESelectedConvertedTextUnderline:
      aMetric = NS_UNDERLINE_STYLE_NONE;
      break;
    case eMetric_SpellCheckerUnderlineStyle:
      aMetric = NS_UNDERLINE_STYLE_WAVY;
      break;
    default:
        aMetric = 0;
        res = NS_ERROR_FAILURE;
    }
  return res;
}

NS_IMETHODIMP nsLookAndFeel::GetMetric(const nsMetricFloatID aID, float & aMetric)
{
  nsresult res = nsXPLookAndFeel::GetMetric(aID, aMetric);
  if (NS_SUCCEEDED(res))
    return res;
  res = NS_OK;

  switch (aID) {
    case eMetricFloat_TextFieldVerticalInsidePadding:
        aMetric = 0.25f;
        break;
    case eMetricFloat_TextFieldHorizontalInsidePadding:
        aMetric = 0.95f;
        break;
    case eMetricFloat_TextAreaVerticalInsidePadding:
        aMetric = 0.40f;
        break;
    case eMetricFloat_TextAreaHorizontalInsidePadding:
        aMetric = 0.40f;
        break;
    case eMetricFloat_ListVerticalInsidePadding:
        aMetric = 0.10f;
        break;
    case eMetricFloat_ListHorizontalInsidePadding:
        aMetric = 0.40f;
        break;
    case eMetricFloat_ButtonVerticalInsidePadding:
        aMetric = 0.25f;
        break;
    case eMetricFloat_ButtonHorizontalInsidePadding:
        aMetric = 0.25f;
        break;
    case eMetricFloat_IMEUnderlineRelativeSize:
        aMetric = 1.0f;
        break;
    case eMetricFloat_SpellCheckerUnderlineRelativeSize:
        aMetric = 1.0f;
        break;
    default:
        aMetric = -1.0;
        res = NS_ERROR_FAILURE;
    }
  return res;
}
