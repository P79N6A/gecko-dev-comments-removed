














































#ifndef _GTK_DRAWING_H_
#define _GTK_DRAWING_H_

#include <gdk/gdk.h>
#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
  guint8 active;
  guint8 focused;
  guint8 inHover;
  guint8 disabled;
  guint8 isDefault;
  guint8 canDefault;
  

  guint8 depressed;
  gint32 curpos; 
  gint32 maxpos;
} GtkWidgetState;

typedef struct {
  gint slider_width;
  gint trough_border;
  gint stepper_size;
  gint stepper_spacing;
  gint min_slider_size;
} MozGtkScrollbarMetrics;

typedef enum {
  MOZ_GTK_STEPPER_DOWN        = 1 << 0,
  MOZ_GTK_STEPPER_BOTTOM      = 1 << 1,
  MOZ_GTK_STEPPER_VERTICAL    = 1 << 2
} GtkScrollbarButtonFlags;


typedef enum {
  
  MOZ_GTK_TAB_MARGIN_MASK     = 0xFF,
  
  MOZ_GTK_TAB_BOTTOM          = 1 << 8,
  
  MOZ_GTK_TAB_FIRST           = 1 << 9,
  
  MOZ_GTK_TAB_SELECTED        = 1 << 10
} GtkTabFlags;


typedef enum {
  
  MOZ_TOPLEVEL_MENU_ITEM      = 1 << 0
} GtkMenuItemFlags;


typedef gint (*style_prop_t)(GtkStyle*, const gchar*, gint);


#define MOZ_GTK_SUCCESS 0
#define MOZ_GTK_UNKNOWN_WIDGET -1
#define MOZ_GTK_UNSAFE_THEME -2


#define MOZ_GTK_WIDGET_CHECKED 1
#define MOZ_GTK_WIDGET_INCONSISTENT (1 << 1)


typedef enum {
  
  MOZ_GTK_BUTTON,
  
  MOZ_GTK_CHECKBUTTON,
  
  MOZ_GTK_RADIOBUTTON,
  



  MOZ_GTK_SCROLLBAR_BUTTON,
  
  MOZ_GTK_SCROLLBAR_TRACK_HORIZONTAL,
  MOZ_GTK_SCROLLBAR_TRACK_VERTICAL,
  
  MOZ_GTK_SCROLLBAR_THUMB_HORIZONTAL,
  MOZ_GTK_SCROLLBAR_THUMB_VERTICAL,
  
  MOZ_GTK_SCALE_HORIZONTAL,
  MOZ_GTK_SCALE_VERTICAL,
  
  MOZ_GTK_SCALE_THUMB_HORIZONTAL,
  MOZ_GTK_SCALE_THUMB_VERTICAL,
  
  MOZ_GTK_SPINBUTTON,
  MOZ_GTK_SPINBUTTON_UP,
  MOZ_GTK_SPINBUTTON_DOWN,
  MOZ_GTK_SPINBUTTON_ENTRY,
  
  MOZ_GTK_GRIPPER,
  
  MOZ_GTK_ENTRY,
  
  MOZ_GTK_ENTRY_CARET,
  
  MOZ_GTK_DROPDOWN,
  
  MOZ_GTK_DROPDOWN_ARROW,
  
  MOZ_GTK_DROPDOWN_ENTRY,
  
  MOZ_GTK_CHECKBUTTON_CONTAINER,
  
  MOZ_GTK_RADIOBUTTON_CONTAINER,
  
  MOZ_GTK_CHECKBUTTON_LABEL,
  
  MOZ_GTK_RADIOBUTTON_LABEL,
  
  MOZ_GTK_TOOLBAR,
  
  MOZ_GTK_TOOLBAR_SEPARATOR,
  
  MOZ_GTK_TOOLTIP,
  
  MOZ_GTK_FRAME,
  
  MOZ_GTK_RESIZER,
  
  MOZ_GTK_PROGRESSBAR,
  
  MOZ_GTK_PROGRESS_CHUNK,
  
  MOZ_GTK_TAB,
  
  MOZ_GTK_TABPANELS,
  
  MOZ_GTK_TAB_SCROLLARROW,
  
  MOZ_GTK_TREEVIEW,
  
  MOZ_GTK_TREE_HEADER_CELL,
  
  MOZ_GTK_TREE_HEADER_SORTARROW,
  
  MOZ_GTK_TREEVIEW_EXPANDER,
  
  MOZ_GTK_EXPANDER,
  
  MOZ_GTK_MENUBAR,
  
  MOZ_GTK_MENUPOPUP,
  
  MOZ_GTK_MENUARROW,
  
  MOZ_GTK_TOOLBARBUTTON_ARROW,
  
  MOZ_GTK_MENUITEM,
  MOZ_GTK_CHECKMENUITEM,
  MOZ_GTK_RADIOMENUITEM,
  MOZ_GTK_MENUSEPARATOR,
  
  MOZ_GTK_SPLITTER_HORIZONTAL,
  
  MOZ_GTK_SPLITTER_VERTICAL,
  
  MOZ_GTK_WINDOW
} GtkThemeWidgetType;









gint moz_gtk_init();








gint moz_gtk_enable_style_props(style_prop_t styleGetProp);







gint moz_gtk_shutdown();












gint
moz_gtk_widget_paint(GtkThemeWidgetType widget, GdkDrawable* drawable,
                     GdkRectangle* rect, GdkRectangle* cliprect,
                     GtkWidgetState* state, gint flags,
                     GtkTextDirection direction);













gint moz_gtk_get_widget_border(GtkThemeWidgetType widget, gint* left, gint* top, 
                               gint* right, gint* bottom, GtkTextDirection direction,
                               gboolean inhtml);









gint
moz_gtk_checkbox_get_metrics(gint* indicator_size, gint* indicator_spacing);









gint
moz_gtk_radio_get_metrics(gint* indicator_size, gint* indicator_spacing);








gint
moz_gtk_button_get_inner_border(GtkWidget* widget, GtkBorder* inner_border);










gint
moz_gtk_widget_get_focus(GtkWidget* widget, gboolean* interior_focus,
                         gint* focus_width, gint* focus_pad);












gint
moz_gtk_button_get_default_overflow(gint* border_top, gint* border_left,
                                    gint* border_bottom, gint* border_right);









gint
moz_gtk_get_scalethumb_metrics(GtkOrientation orient, gint* thumb_length, gint* thumb_height);







gint
moz_gtk_get_scrollbar_metrics(MozGtkScrollbarMetrics* metrics);








gint moz_gtk_get_combo_box_entry_button_size(gint* width, gint* height);








gint moz_gtk_get_tab_scroll_arrow_size(gint* width, gint* height);








gint moz_gtk_get_downarrow_size(gint* width, gint* height);







gint moz_gtk_get_toolbar_separator_width(gint* size);







gint moz_gtk_get_expander_size(gint* size);







gint moz_gtk_get_treeview_expander_size(gint* size);







gint moz_gtk_get_menu_separator_height(gint* size);








gint moz_gtk_splitter_get_metrics(gint orientation, gint* size);





GtkWidget* moz_gtk_get_scrollbar_widget(void);




gint moz_gtk_get_tab_thickness(void);





gboolean moz_gtk_images_in_menus(void);





gboolean moz_gtk_images_in_buttons(void);

#ifdef __cplusplus
}
#endif 

#endif
