





































#include "nsLookAndFeel.h"
#include <gtk/gtkinvisible.h>

#define GDK_COLOR_TO_NS_RGB(c) \
    ((nscolor) NS_RGB(c.red>>8, c.green>>8, c.blue>>8))

nscolor nsLookAndFeel::sInfoText = 0;
nscolor nsLookAndFeel::sInfoBackground = 0;
nscolor nsLookAndFeel::sMenuText = 0;
nscolor nsLookAndFeel::sMenuBackground = 0;
nscolor nsLookAndFeel::sButtonBackground = 0;
nscolor nsLookAndFeel::sButtonText = 0;
nscolor nsLookAndFeel::sButtonOuterLightBorder = 0;
nscolor nsLookAndFeel::sButtonInnerDarkBorder = 0;
PRBool nsLookAndFeel::sColorsInitialized = PR_FALSE;






nsLookAndFeel::nsLookAndFeel() : nsXPLookAndFeel()
{
  mWidget = gtk_invisible_new();
  gtk_widget_ensure_style(mWidget);
  mStyle = gtk_widget_get_style(mWidget);

  if (!sColorsInitialized)
    InitColors();
}

nsLookAndFeel::~nsLookAndFeel()
{
  
  gtk_widget_unref(mWidget);
}

nsresult nsLookAndFeel::NativeGetColor(const nsColorID aID, nscolor& aColor)
{
  nsresult res = NS_OK;
  aColor = 0; 

  switch (aID) {
    
    
  case eColor_WindowBackground:
    aColor = GDK_COLOR_TO_NS_RGB(mStyle->bg[GTK_STATE_NORMAL]);
    break;
  case eColor_WindowForeground:
    aColor = GDK_COLOR_TO_NS_RGB(mStyle->fg[GTK_STATE_NORMAL]);
    break;
  case eColor_WidgetBackground:
    aColor = GDK_COLOR_TO_NS_RGB(mStyle->bg[GTK_STATE_NORMAL]);
    break;
  case eColor_WidgetForeground:
    aColor = GDK_COLOR_TO_NS_RGB(mStyle->fg[GTK_STATE_NORMAL]);
    break;
  case eColor_WidgetSelectBackground:
    aColor = GDK_COLOR_TO_NS_RGB(mStyle->bg[GTK_STATE_SELECTED]);
    break;
  case eColor_WidgetSelectForeground:
    aColor = GDK_COLOR_TO_NS_RGB(mStyle->fg[GTK_STATE_SELECTED]);
    break;
  case eColor_Widget3DHighlight:
    aColor = NS_RGB(0xa0,0xa0,0xa0);
    break;
  case eColor_Widget3DShadow:
    aColor = NS_RGB(0x40,0x40,0x40);
    break;
  case eColor_TextBackground:
    aColor = GDK_COLOR_TO_NS_RGB(mStyle->bg[GTK_STATE_NORMAL]);
    break;
  case eColor_TextForeground: 
    aColor = GDK_COLOR_TO_NS_RGB(mStyle->fg[GTK_STATE_NORMAL]);
    break;
  case eColor_TextSelectBackground:
  case eColor_IMESelectedRawTextBackground:
  case eColor_IMESelectedConvertedTextBackground:
    aColor = GDK_COLOR_TO_NS_RGB(mStyle->bg[GTK_STATE_SELECTED]);
    break;
  case eColor_TextSelectForeground:
  case eColor_IMESelectedRawTextForeground:
  case eColor_IMESelectedConvertedTextForeground:
    aColor = GDK_COLOR_TO_NS_RGB(mStyle->fg[GTK_STATE_SELECTED]);
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

    
  case eColor_activeborder:
      
    aColor = GDK_COLOR_TO_NS_RGB(mStyle->bg[GTK_STATE_NORMAL]);
    break;
  case eColor_activecaption:
      
    aColor = GDK_COLOR_TO_NS_RGB(mStyle->bg[GTK_STATE_NORMAL]);
    break;
  case eColor_appworkspace:
      
    aColor = GDK_COLOR_TO_NS_RGB(mStyle->bg[GTK_STATE_NORMAL]);
    break;
  case eColor_background:
      
    aColor = GDK_COLOR_TO_NS_RGB(mStyle->bg[GTK_STATE_NORMAL]);
    break;
  case eColor_captiontext:
      
    aColor = GDK_COLOR_TO_NS_RGB(mStyle->fg[GTK_STATE_NORMAL]);
    break;
  case eColor_graytext:
      
    aColor = GDK_COLOR_TO_NS_RGB(mStyle->fg[GTK_STATE_INSENSITIVE]);
    
    break;
  case eColor_highlight:
  case eColor__moz_menuhover:
      
    aColor = GDK_COLOR_TO_NS_RGB(mStyle->bg[GTK_STATE_SELECTED]);
    break;
  case eColor_highlighttext:
  case eColor__moz_menuhovertext:
      
    aColor = GDK_COLOR_TO_NS_RGB(mStyle->fg[GTK_STATE_SELECTED]);
    break;
  case eColor_inactiveborder:
      
    aColor = GDK_COLOR_TO_NS_RGB(mStyle->bg[GTK_STATE_NORMAL]);
    break;
  case eColor_inactivecaption:
      
    aColor = GDK_COLOR_TO_NS_RGB(mStyle->bg[GTK_STATE_INSENSITIVE]);
    break;
  case eColor_inactivecaptiontext:
      
    aColor = GDK_COLOR_TO_NS_RGB(mStyle->fg[GTK_STATE_INSENSITIVE]);
    break;
  case eColor_infobackground:
      
    aColor = sInfoBackground;
    break;
  case eColor_infotext:
      
    aColor = sInfoText;
    break;
  case eColor_menu:
      
    aColor = sMenuBackground;
    break;
  case eColor_menutext:
      
    aColor = sMenuText;
    break;
  case eColor_scrollbar:
      
    aColor = GDK_COLOR_TO_NS_RGB(mStyle->bg[GTK_STATE_ACTIVE]);
    break;

  case eColor_threedface:
  case eColor_buttonface:
  case eColor__moz_buttonhoverface:
      
    aColor = sButtonBackground;
    break;

  case eColor_buttontext:
  case eColor__moz_buttonhovertext:
      
    aColor = sButtonText;
    break;

  case eColor_buttonhighlight:
      
  case eColor_threedhighlight:
      
    aColor = sButtonOuterLightBorder;
    break;

  case eColor_threedlightshadow:
      
    aColor = sButtonBackground; 
    break;

  case eColor_buttonshadow:
      
  case eColor_threedshadow:
      
    aColor = sButtonInnerDarkBorder;
    break;

  case eColor_threeddarkshadow:
      
    aColor = GDK_COLOR_TO_NS_RGB(mStyle->black);
    break;

  case eColor_window:
  case eColor_windowframe:
    aColor = GDK_COLOR_TO_NS_RGB(mStyle->bg[GTK_STATE_NORMAL]);
    break;

  case eColor_windowtext:
    aColor = GDK_COLOR_TO_NS_RGB(mStyle->fg[GTK_STATE_NORMAL]);
    break;

  
  

  case eColor__moz_field:
    aColor = GDK_COLOR_TO_NS_RGB(mStyle->base[GTK_STATE_NORMAL]);
    break;
  case eColor__moz_fieldtext:
    aColor = GDK_COLOR_TO_NS_RGB(mStyle->fg[GTK_STATE_NORMAL]);
    break;
  case eColor__moz_dialog:
  case eColor__moz_cellhighlight:
    aColor = GDK_COLOR_TO_NS_RGB(mStyle->bg[GTK_STATE_NORMAL]);
    break;
  case eColor__moz_dialogtext:
  case eColor__moz_cellhighlighttext:
    aColor = GDK_COLOR_TO_NS_RGB(mStyle->fg[GTK_STATE_NORMAL]);
    break;
  case eColor__moz_dragtargetzone:
    aColor = GDK_COLOR_TO_NS_RGB(mStyle->bg[GTK_STATE_SELECTED]);
    break; 
  case eColor__moz_buttondefault:
      
    aColor = GDK_COLOR_TO_NS_RGB(mStyle->black);
    break;
  default:
    
    aColor = 0;
    res    = NS_ERROR_FAILURE;
    break;
  }

  

  return res;
}

NS_IMETHODIMP nsLookAndFeel::GetMetric(const nsMetricID aID, PRInt32 & aMetric)
{
  nsresult res = NS_OK;

  res = nsXPLookAndFeel::GetMetric(aID, aMetric);
  if (NS_SUCCEEDED(res))
    return res;
  res = NS_OK;

  switch (aID) {
  case eMetric_WindowTitleHeight:
    aMetric = 0;
    break;
  case eMetric_WindowBorderWidth:
    
    
    break;
  case eMetric_WindowBorderHeight:
    
    
    break;
  case eMetric_Widget3DBorder:
    
    
    break;
  case eMetric_TextFieldHeight:
    {
      GtkRequisition req;
      GtkWidget *text = gtk_entry_new();
      
      gtk_widget_ref(text);
      gtk_object_sink(GTK_OBJECT(text));
      gtk_widget_size_request(text,&req);
      aMetric = req.height;
      gtk_widget_destroy(text);
      gtk_widget_unref(text);
    }
    break;
  case eMetric_TextFieldBorder:
    aMetric = 2;
    break;
  case eMetric_TextVerticalInsidePadding:
    aMetric = 0;
    break;
  case eMetric_TextShouldUseVerticalInsidePadding:
    aMetric = 0;
    break;
  case eMetric_TextHorizontalInsideMinimumPadding:
    aMetric = 15;
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
    aMetric = 15;
    break;
  case eMetric_RadioboxSize:
    aMetric = 15;
    break;
  case eMetric_ListShouldUseHorizontalInsideMinimumPadding:
    aMetric = 15;
    break;
  case eMetric_ListHorizontalInsideMinimumPadding:
    aMetric = 15;
    break;
  case eMetric_ListShouldUseVerticalInsidePadding:
    aMetric = 1;
    break;
  case eMetric_ListVerticalInsidePadding:
    aMetric = 1;
    break;
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
  case eMetric_SkipNavigatingDisabledMenuItem:
    aMetric = 1;
    break;
  case eMetric_DragFullWindow:
    aMetric = 1;
    break;
  case eMetric_DragThresholdX:
  case eMetric_DragThresholdY:
    
    aMetric = 3;
    break;
  case eMetric_ScrollArrowStyle:
    aMetric = eMetric_ScrollArrowStyleSingle;
    break;
  case eMetric_ScrollSliderStyle:
    aMetric = eMetric_ScrollThumbStyleProportional;
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

  default:
    aMetric = 0;
    res     = NS_ERROR_FAILURE;
  }

  return res;
}

NS_IMETHODIMP nsLookAndFeel::GetMetric(const nsMetricFloatID aID, float & aMetric)
{
  nsresult res = NS_OK;
  res = nsXPLookAndFeel::GetMetric(aID, aMetric);
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
  default:
    aMetric = -1.0;
    res = NS_ERROR_FAILURE;
  }
  return res;
}

void
nsLookAndFeel::InitColors()
{
  sColorsInitialized = PR_TRUE;
  GtkStyle *style;
  
  
  GtkTooltips *tooltips = gtk_tooltips_new();
  gtk_tooltips_force_window(tooltips);
  GtkWidget *tip_window = tooltips->tip_window;
  gtk_widget_set_rc_style(tip_window);

  style = gtk_widget_get_style(tip_window);
  sInfoBackground = GDK_COLOR_TO_NS_RGB(style->bg[GTK_STATE_NORMAL]);
  sInfoText = GDK_COLOR_TO_NS_RGB(style->fg[GTK_STATE_NORMAL]);

  gtk_object_unref(GTK_OBJECT(tooltips));


  
  GtkWidget *accel_label = gtk_accel_label_new("M");
  GtkWidget *menuitem = gtk_menu_item_new();
  GtkWidget *menu = gtk_menu_new();

  gtk_container_add(GTK_CONTAINER(menuitem), accel_label);
  gtk_menu_append(GTK_MENU(menu), menuitem);

  gtk_widget_set_rc_style(accel_label);
  gtk_widget_set_rc_style(menu);
  gtk_widget_realize(menu);
  gtk_widget_realize(accel_label);

  style = gtk_widget_get_style(accel_label);
  sMenuText = GDK_COLOR_TO_NS_RGB(style->fg[GTK_STATE_NORMAL]);

  style = gtk_widget_get_style(menu);
  sMenuBackground = GDK_COLOR_TO_NS_RGB(style->bg[GTK_STATE_NORMAL]);

  gtk_widget_unref(menu);


  
  GtkWidget *parent = gtk_fixed_new();
  GtkWidget *button = gtk_button_new();
  GtkWidget *label = gtk_label_new("M");
  GtkWidget *window = gtk_window_new(GTK_WINDOW_POPUP);
  
  gtk_container_add(GTK_CONTAINER(button), label);
  gtk_container_add(GTK_CONTAINER(parent), button);
  gtk_container_add(GTK_CONTAINER(window), parent);

  gtk_widget_set_rc_style(button);
  gtk_widget_set_rc_style(label);

  gtk_widget_realize(button);
  gtk_widget_realize(label);

  style = gtk_widget_get_style(label);
  sButtonText = GDK_COLOR_TO_NS_RGB(style->fg[GTK_STATE_NORMAL]);

  style = gtk_widget_get_style(button);
  sButtonBackground = GDK_COLOR_TO_NS_RGB(style->bg[GTK_STATE_NORMAL]);
  sButtonOuterLightBorder =
    GDK_COLOR_TO_NS_RGB(style->light[GTK_STATE_NORMAL]);
  sButtonInnerDarkBorder =
    GDK_COLOR_TO_NS_RGB(style->dark[GTK_STATE_NORMAL]);

  gtk_widget_destroy(window);

}
