






































#include <stdlib.h>

#include "nsIDeviceContext.h"
#include "nsIRenderingContext.h"
#include "prlink.h"

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#ifdef MOZ_PANGO
#include <pango/pango.h>
#include <pango/pango-fontmap.h>
#endif

#include <fontconfig/fontconfig.h>
#include "nsSystemFontsGTK2.h"
#include "gfxPlatformGtk.h"


#if defined(MOZ_PANGO) && !defined(THEBES_USE_PANGO_CAIRO)
static gboolean
(* PTR_pango_font_description_get_size_is_absolute)(PangoFontDescription*)
    = nsnull;

static void InitPangoLib()
{
    static PRBool initialized = PR_FALSE;
    if (initialized)
        return;
    initialized = PR_TRUE;

    PRLibrary *pangoLib = nsnull;
    PTR_pango_font_description_get_size_is_absolute =
        (gboolean (*)(PangoFontDescription*))
        PR_FindFunctionSymbolAndLibrary("pango_font_description_get_size_is_absolute",
                                        &pangoLib);
    if (pangoLib)
        PR_UnloadLibrary(pangoLib);
}

static void
ShutdownPangoLib()
{
}

static gboolean
MOZ_pango_font_description_get_size_is_absolute(PangoFontDescription *desc)
{
    if (PTR_pango_font_description_get_size_is_absolute) {
        return PTR_pango_font_description_get_size_is_absolute(desc);
    }

    
    return PR_FALSE;
}
#else
static inline void InitPangoLib()
{
}

static inline void ShutdownPangoLib()
{
}

#ifdef MOZ_PANGO
static inline gboolean
MOZ_pango_font_description_get_size_is_absolute(PangoFontDescription *desc)
{
    pango_font_description_get_size_is_absolute(desc);
}
#endif
#endif

nsSystemFontsGTK2::nsSystemFontsGTK2()
  : mDefaultFontName(NS_LITERAL_STRING("sans-serif"))
  , mButtonFontName(NS_LITERAL_STRING("sans-serif"))
  , mFieldFontName(NS_LITERAL_STRING("sans-serif"))
  , mMenuFontName(NS_LITERAL_STRING("sans-serif"))
{
    InitPangoLib();

    




    
    GtkWidget *label = gtk_label_new("M");
    GtkWidget *parent = gtk_fixed_new();
    GtkWidget *window = gtk_window_new(GTK_WINDOW_POPUP);

    gtk_container_add(GTK_CONTAINER(parent), label);
    gtk_container_add(GTK_CONTAINER(window), parent);

    gtk_widget_ensure_style(label);

    GetSystemFontInfo(label, &mDefaultFontName, &mDefaultFontStyle);

    gtk_widget_destroy(window);  

    
    GtkWidget *entry = gtk_entry_new();
    parent = gtk_fixed_new();
    window = gtk_window_new(GTK_WINDOW_POPUP);

    gtk_container_add(GTK_CONTAINER(parent), entry);
    gtk_container_add(GTK_CONTAINER(window), parent);
    gtk_widget_ensure_style(entry);

    GetSystemFontInfo(entry, &mFieldFontName, &mFieldFontStyle);

    gtk_widget_destroy(window);  

    
    GtkWidget *accel_label = gtk_accel_label_new("M");
    GtkWidget *menuitem = gtk_menu_item_new();
    GtkWidget *menu = gtk_menu_new();
    g_object_ref_sink(GTK_OBJECT(menu));

    gtk_container_add(GTK_CONTAINER(menuitem), accel_label);
    gtk_menu_shell_append((GtkMenuShell *)GTK_MENU(menu), menuitem);

    gtk_widget_ensure_style(accel_label);

    GetSystemFontInfo(accel_label, &mMenuFontName, &mMenuFontStyle);

    g_object_unref(menu);

    
    parent = gtk_fixed_new();
    GtkWidget *button = gtk_button_new();
    label = gtk_label_new("M");
    window = gtk_window_new(GTK_WINDOW_POPUP);
          
    gtk_container_add(GTK_CONTAINER(button), label);
    gtk_container_add(GTK_CONTAINER(parent), button);
    gtk_container_add(GTK_CONTAINER(window), parent);

    gtk_widget_ensure_style(label);

    GetSystemFontInfo(label, &mButtonFontName, &mButtonFontStyle);

    gtk_widget_destroy(window);  
}

nsSystemFontsGTK2::~nsSystemFontsGTK2()
{
    ShutdownPangoLib();
}

nsresult
nsSystemFontsGTK2::GetSystemFontInfo(GtkWidget *aWidget, nsString *aFontName,
                                     gfxFontStyle *aFontStyle) const
{
#ifdef MOZ_PANGO
    GtkSettings *settings = gtk_widget_get_settings(aWidget);

    aFontStyle->style       = FONT_STYLE_NORMAL;

    gchar *fontname;
    g_object_get(settings, "gtk-font-name", &fontname, NULL);

    PangoFontDescription *desc;
    desc = pango_font_description_from_string(fontname);

    aFontStyle->systemFont = PR_TRUE;

    g_free(fontname);

    NS_NAMED_LITERAL_STRING(quote, "\"");
    NS_ConvertUTF8toUTF16 family(pango_font_description_get_family(desc));
    *aFontName = quote + family + quote;

    aFontStyle->weight = pango_font_description_get_weight(desc);

    
    aFontStyle->stretch = NS_FONT_STRETCH_NORMAL;

    float size = float(pango_font_description_get_size(desc)) / PANGO_SCALE;

    

    if (!MOZ_pango_font_description_get_size_is_absolute(desc)) {
        
        size *= float(gfxPlatformGtk::GetPlatformDPI()) / POINTS_PER_INCH_FLOAT;
    }

    

    aFontStyle->size = size;
  
    pango_font_description_free(desc);

#else
    
    aFontStyle->style       = FONT_STYLE_NORMAL;
    aFontStyle->systemFont = PR_TRUE;

    NS_NAMED_LITERAL_STRING(fontname, "\"Sans\"");
    *aFontName = fontname;
    aFontStyle->weight = 400;
    aFontStyle->size = 40/3;

#endif

    return NS_OK;
}

nsresult
nsSystemFontsGTK2::GetSystemFont(nsSystemFontID anID, nsString *aFontName,
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
