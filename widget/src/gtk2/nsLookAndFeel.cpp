







































#include "nsLookAndFeel.h"
#include <gtk/gtk.h>

#include "gtkdrawing.h"
#include "nsStyleConsts.h"

#ifdef MOZ_PLATFORM_MAEMO
#include "nsIServiceManager.h"
#include "nsIPropertyBag2.h"
#include "nsLiteralString.h"
#endif

#define GDK_COLOR_TO_NS_RGB(c) \
    ((nscolor) NS_RGB(c.red>>8, c.green>>8, c.blue>>8))

nscolor   nsLookAndFeel::sInfoText = 0;
nscolor   nsLookAndFeel::sInfoBackground = 0;
nscolor   nsLookAndFeel::sMenuBarText = 0;
nscolor   nsLookAndFeel::sMenuBarHoverText = 0;
nscolor   nsLookAndFeel::sMenuText = 0;
nscolor   nsLookAndFeel::sMenuHover = 0;
nscolor   nsLookAndFeel::sMenuHoverText = 0;
nscolor   nsLookAndFeel::sMenuBackground = 0;
nscolor   nsLookAndFeel::sButtonBackground = 0;
nscolor   nsLookAndFeel::sButtonText = 0;
nscolor   nsLookAndFeel::sButtonOuterLightBorder = 0;
nscolor   nsLookAndFeel::sButtonInnerDarkBorder = 0;
nscolor   nsLookAndFeel::sOddCellBackground = 0;
nscolor   nsLookAndFeel::sNativeHyperLinkText = 0;
nscolor   nsLookAndFeel::sComboBoxText = 0;
nscolor   nsLookAndFeel::sComboBoxBackground = 0;
PRUnichar nsLookAndFeel::sInvisibleCharacter = PRUnichar('*');
float     nsLookAndFeel::sCaretRatio = 0;
PRBool    nsLookAndFeel::sMenuSupportsDrag = PR_FALSE;






nsLookAndFeel::nsLookAndFeel() : nsXPLookAndFeel()
{
    mStyle = nsnull;
    InitWidget();

    static PRBool sInitialized = PR_FALSE;

    if (!sInitialized) {
        sInitialized = PR_TRUE;
        InitLookAndFeel();
    }
}

nsLookAndFeel::~nsLookAndFeel()
{
    g_object_unref(mStyle);
}

nsresult
nsLookAndFeel::NativeGetColor(ColorID aID, nscolor& aColor)
{
    nsresult res = NS_OK;

    switch (aID) {
        
        
        
    case eColorID_WindowBackground:
        aColor = GDK_COLOR_TO_NS_RGB(mStyle->base[GTK_STATE_NORMAL]);
        break;
    case eColorID_WindowForeground:
        aColor = GDK_COLOR_TO_NS_RGB(mStyle->text[GTK_STATE_NORMAL]);
        break;
    case eColorID_WidgetBackground:
        aColor = GDK_COLOR_TO_NS_RGB(mStyle->bg[GTK_STATE_NORMAL]);
        break;
    case eColorID_WidgetForeground:
        aColor = GDK_COLOR_TO_NS_RGB(mStyle->fg[GTK_STATE_NORMAL]);
        break;
    case eColorID_WidgetSelectBackground:
        aColor = GDK_COLOR_TO_NS_RGB(mStyle->bg[GTK_STATE_SELECTED]);
        break;
    case eColorID_WidgetSelectForeground:
        aColor = GDK_COLOR_TO_NS_RGB(mStyle->fg[GTK_STATE_SELECTED]);
        break;
    case eColorID_Widget3DHighlight:
        aColor = NS_RGB(0xa0,0xa0,0xa0);
        break;
    case eColorID_Widget3DShadow:
        aColor = NS_RGB(0x40,0x40,0x40);
        break;
    case eColorID_TextBackground:
        
        aColor = GDK_COLOR_TO_NS_RGB(mStyle->base[GTK_STATE_NORMAL]);
        break;
    case eColorID_TextForeground: 
        
        aColor = GDK_COLOR_TO_NS_RGB(mStyle->text[GTK_STATE_NORMAL]);
        break;
    case eColorID_TextSelectBackground:
    case eColorID_IMESelectedRawTextBackground:
    case eColorID_IMESelectedConvertedTextBackground:
        
        aColor = GDK_COLOR_TO_NS_RGB(mStyle->base[GTK_STATE_SELECTED]);
        break;
    case eColorID_TextSelectForeground:
    case eColorID_IMESelectedRawTextForeground:
    case eColorID_IMESelectedConvertedTextForeground:
        
        aColor = GDK_COLOR_TO_NS_RGB(mStyle->text[GTK_STATE_SELECTED]);
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
        
        aColor = GDK_COLOR_TO_NS_RGB(mStyle->bg[GTK_STATE_NORMAL]);
        break;
    case eColorID_activecaption:
        
        aColor = GDK_COLOR_TO_NS_RGB(mStyle->bg[GTK_STATE_NORMAL]);
        break;
    case eColorID_appworkspace:
        
        aColor = GDK_COLOR_TO_NS_RGB(mStyle->bg[GTK_STATE_NORMAL]);
        break;
    case eColorID_background:
        
        aColor = GDK_COLOR_TO_NS_RGB(mStyle->bg[GTK_STATE_NORMAL]);
        break;
    case eColorID_captiontext:
        
        aColor = GDK_COLOR_TO_NS_RGB(mStyle->fg[GTK_STATE_NORMAL]);
        break;
    case eColorID_graytext:
        
        aColor = GDK_COLOR_TO_NS_RGB(mStyle->fg[GTK_STATE_INSENSITIVE]);
        break;
    case eColorID_highlight:
        
        aColor = GDK_COLOR_TO_NS_RGB(mStyle->base[GTK_STATE_SELECTED]);
        break;
    case eColorID_highlighttext:
        
        aColor = GDK_COLOR_TO_NS_RGB(mStyle->text[GTK_STATE_SELECTED]);
        break;
    case eColorID_inactiveborder:
        
        aColor = GDK_COLOR_TO_NS_RGB(mStyle->bg[GTK_STATE_NORMAL]);
        break;
    case eColorID_inactivecaption:
        
        aColor = GDK_COLOR_TO_NS_RGB(mStyle->bg[GTK_STATE_INSENSITIVE]);
        break;
    case eColorID_inactivecaptiontext:
        
        aColor = GDK_COLOR_TO_NS_RGB(mStyle->fg[GTK_STATE_INSENSITIVE]);
        break;
    case eColorID_infobackground:
        
        aColor = sInfoBackground;
        break;
    case eColorID_infotext:
        
        aColor = sInfoText;
        break;
    case eColorID_menu:
        
        aColor = sMenuBackground;
        break;
    case eColorID_menutext:
        
        aColor = sMenuText;
        break;
    case eColorID_scrollbar:
        
        aColor = GDK_COLOR_TO_NS_RGB(mStyle->bg[GTK_STATE_ACTIVE]);
        break;

    case eColorID_threedface:
    case eColorID_buttonface:
        
        aColor = sButtonBackground;
        break;

    case eColorID_buttontext:
        
        aColor = sButtonText;
        break;

    case eColorID_buttonhighlight:
        
    case eColorID_threedhighlight:
        
        aColor = sButtonOuterLightBorder;
        break;

    case eColorID_threedlightshadow:
        
        aColor = sButtonBackground; 
        break;

    case eColorID_buttonshadow:
        
    case eColorID_threedshadow:
        
        aColor = sButtonInnerDarkBorder;
        break;

    case eColorID_threeddarkshadow:
        
        aColor = GDK_COLOR_TO_NS_RGB(mStyle->black);
        break;

    case eColorID_window:
    case eColorID_windowframe:
        aColor = GDK_COLOR_TO_NS_RGB(mStyle->bg[GTK_STATE_NORMAL]);
        break;

    case eColorID_windowtext:
        aColor = GDK_COLOR_TO_NS_RGB(mStyle->fg[GTK_STATE_NORMAL]);
        break;

    case eColorID__moz_eventreerow:
    case eColorID__moz_field:
        aColor = GDK_COLOR_TO_NS_RGB(mStyle->base[GTK_STATE_NORMAL]);
        break;
    case eColorID__moz_fieldtext:
        aColor = GDK_COLOR_TO_NS_RGB(mStyle->text[GTK_STATE_NORMAL]);
        break;
    case eColorID__moz_dialog:
        aColor = GDK_COLOR_TO_NS_RGB(mStyle->bg[GTK_STATE_NORMAL]);
        break;
    case eColorID__moz_dialogtext:
        aColor = GDK_COLOR_TO_NS_RGB(mStyle->fg[GTK_STATE_NORMAL]);
        break;
    case eColorID__moz_dragtargetzone:
        aColor = GDK_COLOR_TO_NS_RGB(mStyle->bg[GTK_STATE_SELECTED]);
        break; 
    case eColorID__moz_buttondefault:
        
        aColor = GDK_COLOR_TO_NS_RGB(mStyle->black);
        break;
    case eColorID__moz_buttonhoverface:
        aColor = GDK_COLOR_TO_NS_RGB(mStyle->bg[GTK_STATE_PRELIGHT]);
        break;
    case eColorID__moz_buttonhovertext:
        aColor = GDK_COLOR_TO_NS_RGB(mStyle->fg[GTK_STATE_PRELIGHT]);
        break;
    case eColorID__moz_cellhighlight:
    case eColorID__moz_html_cellhighlight:
        aColor = GDK_COLOR_TO_NS_RGB(mStyle->base[GTK_STATE_ACTIVE]);
        break;
    case eColorID__moz_cellhighlighttext:
    case eColorID__moz_html_cellhighlighttext:
        aColor = GDK_COLOR_TO_NS_RGB(mStyle->text[GTK_STATE_ACTIVE]);
        break;
    case eColorID__moz_menuhover:
        aColor = sMenuHover;
        break;
    case eColorID__moz_menuhovertext:
        aColor = sMenuHoverText;
        break;
    case eColorID__moz_oddtreerow:
        aColor = sOddCellBackground;
        break;
    case eColorID__moz_nativehyperlinktext:
        aColor = sNativeHyperLinkText;
        break;
    case eColorID__moz_comboboxtext:
        aColor = sComboBoxText;
        break;
    case eColorID__moz_combobox:
        aColor = sComboBoxBackground;
        break;
    case eColorID__moz_menubartext:
        aColor = sMenuBarText;
        break;
    case eColorID__moz_menubarhovertext:
        aColor = sMenuBarHoverText;
        break;
    default:
        
        aColor = 0;
        res    = NS_ERROR_FAILURE;
        break;
    }

    return res;
}

static void darken_gdk_color(GdkColor *src, GdkColor *dest)
{
    gdouble red;
    gdouble green;
    gdouble blue;

    red = (gdouble) src->red / 65535.0;
    green = (gdouble) src->green / 65535.0;
    blue = (gdouble) src->blue / 65535.0;

    red *= 0.93;
    green *= 0.93;
    blue *= 0.93;

    dest->red = red * 65535.0;
    dest->green = green * 65535.0;
    dest->blue = blue * 65535.0;
}

static PRInt32 CheckWidgetStyle(GtkWidget* aWidget, const char* aStyle, PRInt32 aResult) {
    gboolean value = PR_FALSE;
    gtk_widget_style_get(aWidget, aStyle, &value, NULL);
    return value ? aResult : 0;
}

static PRInt32 ConvertGTKStepperStyleToMozillaScrollArrowStyle(GtkWidget* aWidget)
{
    if (!aWidget)
        return mozilla::LookAndFeel::eScrollArrowStyle_Single;
  
    return
        CheckWidgetStyle(aWidget, "has-backward-stepper",
                         mozilla::LookAndFeel::eScrollArrow_StartBackward) |
        CheckWidgetStyle(aWidget, "has-forward-stepper",
                         mozilla::LookAndFeel::eScrollArrow_EndForward) |
        CheckWidgetStyle(aWidget, "has-secondary-backward-stepper",
                         mozilla::LookAndFeel::eScrollArrow_EndBackward) |
        CheckWidgetStyle(aWidget, "has-secondary-forward-stepper",
                         mozilla::LookAndFeel::eScrollArrow_StartForward);
}

nsresult
nsLookAndFeel::GetIntImpl(IntID aID, PRInt32 &aResult)
{
    nsresult res = NS_OK;

    
    switch (aID) {
    case eIntID_ScrollButtonLeftMouseButtonAction:
        aResult = 0;
        return NS_OK;
    case eIntID_ScrollButtonMiddleMouseButtonAction:
        aResult = 1;
        return NS_OK;
    case eIntID_ScrollButtonRightMouseButtonAction:
        aResult = 2;
        return NS_OK;
    default:
        break;
    }

    res = nsXPLookAndFeel::GetIntImpl(aID, aResult);
    if (NS_SUCCEEDED(res))
        return res;
    res = NS_OK;

    switch (aID) {
    case eIntID_CaretBlinkTime:
        {
            GtkSettings *settings;
            gint blink_time;
            gboolean blink;

            settings = gtk_settings_get_default ();
            g_object_get (settings,
                          "gtk-cursor-blink-time", &blink_time,
                          "gtk-cursor-blink", &blink,
                          NULL);
 
            if (blink)
                aResult = (PRInt32) blink_time;
            else
                aResult = 0;
            break;
        }
    case eIntID_CaretWidth:
        aResult = 1;
        break;
    case eIntID_ShowCaretDuringSelection:
        aResult = 0;
        break;
    case eIntID_SelectTextfieldsOnKeyFocus:
        {
            GtkWidget *entry;
            GtkSettings *settings;
            gboolean select_on_focus;

            entry = gtk_entry_new();
            g_object_ref_sink(entry);
            settings = gtk_widget_get_settings(entry);
            g_object_get(settings, 
                         "gtk-entry-select-on-focus",
                         &select_on_focus,
                         NULL);
            
            if(select_on_focus)
                aResult = 1;
            else
                aResult = 0;

            gtk_widget_destroy(entry);
            g_object_unref(entry);
        }
        break;
    case eIntID_SubmenuDelay:
        {
            GtkSettings *settings;
            gint delay;

            settings = gtk_settings_get_default ();
            g_object_get (settings, "gtk-menu-popup-delay", &delay, NULL);
            aResult = (PRInt32) delay;
            break;
        }
    case eIntID_MenusCanOverlapOSBar:
        
        aResult = 1;
        break;
    case eIntID_SkipNavigatingDisabledMenuItem:
        aResult = 1;
        break;
    case eIntID_DragThresholdX:
    case eIntID_DragThresholdY:
        {
            GtkWidget* box = gtk_hbox_new(FALSE, 5);
            gint threshold = 0;
            g_object_get(gtk_widget_get_settings(box),
                         "gtk-dnd-drag-threshold", &threshold,
                         NULL);
            g_object_ref_sink(box);
            
            aResult = threshold;
        }
        break;
    case eIntID_ScrollArrowStyle:
        moz_gtk_init();
        aResult =
            ConvertGTKStepperStyleToMozillaScrollArrowStyle(moz_gtk_get_scrollbar_widget());
        break;
    case eIntID_ScrollSliderStyle:
        aResult = eScrollThumbStyle_Proportional;
        break;
    case eIntID_TreeOpenDelay:
        aResult = 1000;
        break;
    case eIntID_TreeCloseDelay:
        aResult = 1000;
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
    case eIntID_WindowsThemeIdentifier:
        aResult = 0;
        res = NS_ERROR_NOT_IMPLEMENTED;
        break;
    case eIntID_TouchEnabled:
#ifdef MOZ_PLATFORM_MAEMO
        
        aResult = 1;
#else
        aResult = 0;
        res = NS_ERROR_NOT_IMPLEMENTED;
#endif
        break;
    case eIntID_MaemoClassic:
#ifdef MOZ_PLATFORM_MAEMO
        {
            aResult = 0;
            nsCOMPtr<nsIPropertyBag2> infoService(do_GetService("@mozilla.org/system-info;1"));
            if (infoService) {
                nsCString deviceType;
                nsresult rv = infoService->GetPropertyAsACString(NS_LITERAL_STRING("device"),
                                                                 deviceType);
                if (NS_SUCCEEDED(rv)) {
                    if (deviceType.EqualsLiteral("Nokia N8xx"))
                        aResult = 1;
                }
            }
        }
#else
        aResult = 0;
        res = NS_ERROR_NOT_IMPLEMENTED;
#endif
        break;
    case eIntID_MacGraphiteTheme:
    case eIntID_MacLionTheme:
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
    case eIntID_ImagesInMenus:
        aResult = moz_gtk_images_in_menus();
        break;
    case eIntID_ImagesInButtons:
        aResult = moz_gtk_images_in_buttons();
        break;
    case eIntID_MenuBarDrag:
        aResult = sMenuSupportsDrag;
        break;
    default:
        aResult = 0;
        res     = NS_ERROR_FAILURE;
    }

    return res;
}

nsresult
nsLookAndFeel::GetFloatImpl(FloatID aID, float &aResult)
{
    nsresult res = NS_OK;
    res = nsXPLookAndFeel::GetFloatImpl(aID, aResult);
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
    case eFloatID_CaretAspectRatio:
        aResult = sCaretRatio;
        break;
    default:
        aResult = -1.0;
        res = NS_ERROR_FAILURE;
    }
    return res;
}

void
nsLookAndFeel::InitLookAndFeel()
{
    GtkStyle *style;

    
    style = gtk_rc_get_style_by_paths(gtk_settings_get_default(),
                                      "gtk-tooltips", "GtkWindow",
                                      GTK_TYPE_WINDOW);
    if (style) {
        sInfoBackground = GDK_COLOR_TO_NS_RGB(style->bg[GTK_STATE_NORMAL]);
        sInfoText = GDK_COLOR_TO_NS_RGB(style->fg[GTK_STATE_NORMAL]);
    }

    
    GtkWidget *accel_label = gtk_accel_label_new("M");
    GtkWidget *menuitem = gtk_menu_item_new();
    GtkWidget *menu = gtk_menu_new();

    g_object_ref_sink(menu);

    gtk_container_add(GTK_CONTAINER(menuitem), accel_label);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    gtk_widget_set_style(accel_label, NULL);
    gtk_widget_set_style(menu, NULL);
    gtk_widget_realize(menu);
    gtk_widget_realize(accel_label);

    style = gtk_widget_get_style(accel_label);
    if (style) {
        sMenuText = GDK_COLOR_TO_NS_RGB(style->fg[GTK_STATE_NORMAL]);
    }

    style = gtk_widget_get_style(menu);
    if (style) {
        sMenuBackground = GDK_COLOR_TO_NS_RGB(style->bg[GTK_STATE_NORMAL]);
    }
    
    style = gtk_widget_get_style(menuitem);
    if (style) {
        sMenuHover = GDK_COLOR_TO_NS_RGB(style->bg[GTK_STATE_PRELIGHT]);
        sMenuHoverText = GDK_COLOR_TO_NS_RGB(style->fg[GTK_STATE_PRELIGHT]);
    }

    g_object_unref(menu);


    
    GtkWidget *parent = gtk_fixed_new();
    GtkWidget *button = gtk_button_new();
    GtkWidget *label = gtk_label_new("M");
    GtkWidget *combobox = gtk_combo_box_new();
    GtkWidget *comboboxLabel = gtk_label_new("M");
    GtkWidget *window = gtk_window_new(GTK_WINDOW_POPUP);
    GtkWidget *treeView = gtk_tree_view_new();
    GtkWidget *linkButton = gtk_link_button_new("http://example.com/");
    GtkWidget *menuBar = gtk_menu_bar_new();
    GtkWidget *entry = gtk_entry_new();

    gtk_container_add(GTK_CONTAINER(button), label);
    gtk_container_add(GTK_CONTAINER(combobox), comboboxLabel);
    gtk_container_add(GTK_CONTAINER(parent), button);
    gtk_container_add(GTK_CONTAINER(parent), treeView);
    gtk_container_add(GTK_CONTAINER(parent), linkButton);
    gtk_container_add(GTK_CONTAINER(parent), combobox);
    gtk_container_add(GTK_CONTAINER(parent), menuBar);
    gtk_container_add(GTK_CONTAINER(window), parent);
    gtk_container_add(GTK_CONTAINER(parent), entry);

    gtk_widget_set_style(button, NULL);
    gtk_widget_set_style(label, NULL);
    gtk_widget_set_style(treeView, NULL);
    gtk_widget_set_style(linkButton, NULL);
    gtk_widget_set_style(combobox, NULL);
    gtk_widget_set_style(comboboxLabel, NULL);
    gtk_widget_set_style(menuBar, NULL);
    gtk_widget_set_style(entry, NULL);

    gtk_widget_realize(button);
    gtk_widget_realize(label);
    gtk_widget_realize(treeView);
    gtk_widget_realize(linkButton);
    gtk_widget_realize(combobox);
    gtk_widget_realize(comboboxLabel);
    gtk_widget_realize(menuBar);
    gtk_widget_realize(entry);

    style = gtk_widget_get_style(label);
    if (style) {
        sButtonText = GDK_COLOR_TO_NS_RGB(style->fg[GTK_STATE_NORMAL]);
    }

    style = gtk_widget_get_style(comboboxLabel);
    if (style) {
        sComboBoxText = GDK_COLOR_TO_NS_RGB(style->fg[GTK_STATE_NORMAL]);
    }
    style = gtk_widget_get_style(combobox);
    if (style) {
        sComboBoxBackground = GDK_COLOR_TO_NS_RGB(style->bg[GTK_STATE_NORMAL]);
    }

    style = gtk_widget_get_style(menuBar);
    if (style) {
        sMenuBarText = GDK_COLOR_TO_NS_RGB(style->fg[GTK_STATE_NORMAL]);
        sMenuBarHoverText = GDK_COLOR_TO_NS_RGB(style->fg[GTK_STATE_SELECTED]);
    }

    
    gboolean supports_menubar_drag = FALSE;
    GParamSpec *param_spec =
        gtk_widget_class_find_style_property(GTK_WIDGET_GET_CLASS(menuBar),
                                             "window-dragging");
    if (param_spec) {
        if (g_type_is_a(G_PARAM_SPEC_VALUE_TYPE(param_spec), G_TYPE_BOOLEAN)) {
            gtk_widget_style_get(menuBar,
                                 "window-dragging", &supports_menubar_drag,
                                 NULL);
        }
    }
    sMenuSupportsDrag = supports_menubar_drag;

    
    
    
    
    
    
    GdkColor colorValue;
    GdkColor *colorValuePtr = NULL;
    gtk_widget_style_get(treeView,
                         "odd-row-color", &colorValuePtr,
                         NULL);

    if (colorValuePtr) {
        colorValue = *colorValuePtr;
    } else {
        gtk_widget_style_get(treeView,
                             "even-row-color", &colorValuePtr,
                             NULL);
        if (colorValuePtr)
            darken_gdk_color(colorValuePtr, &colorValue);
        else
            darken_gdk_color(&treeView->style->base[GTK_STATE_NORMAL], &colorValue);
    }

    sOddCellBackground = GDK_COLOR_TO_NS_RGB(colorValue);
    if (colorValuePtr)
        gdk_color_free(colorValuePtr);

    style = gtk_widget_get_style(button);
    if (style) {
        sButtonBackground = GDK_COLOR_TO_NS_RGB(style->bg[GTK_STATE_NORMAL]);
        sButtonOuterLightBorder =
            GDK_COLOR_TO_NS_RGB(style->light[GTK_STATE_NORMAL]);
        sButtonInnerDarkBorder =
            GDK_COLOR_TO_NS_RGB(style->dark[GTK_STATE_NORMAL]);
    }

    colorValuePtr = NULL;
    gtk_widget_style_get(linkButton, "link-color", &colorValuePtr, NULL);
    if (colorValuePtr) {
        colorValue = *colorValuePtr; 
        sNativeHyperLinkText = GDK_COLOR_TO_NS_RGB(colorValue);
        gdk_color_free(colorValuePtr);
    } else {
        sNativeHyperLinkText = NS_RGB(0x00,0x00,0xEE);
    }

    
    guint value;
    g_object_get (entry, "invisible-char", &value, NULL);
    sInvisibleCharacter = PRUnichar(value);

    
    gtk_widget_style_get(entry,
                         "cursor-aspect-ratio", &sCaretRatio,
                         NULL);

    gtk_widget_destroy(window);
}


PRUnichar
nsLookAndFeel::GetPasswordCharacterImpl()
{
    return sInvisibleCharacter;
}

void
nsLookAndFeel::RefreshImpl()
{
    nsXPLookAndFeel::RefreshImpl();

    g_object_unref(mStyle);
    mStyle = nsnull;
 
    InitWidget();
    InitLookAndFeel();
}

PRBool
nsLookAndFeel::GetEchoPasswordImpl() {
#ifdef MOZ_PLATFORM_MAEMO
    return PR_TRUE;
#else
    return PR_FALSE;
#endif
}
