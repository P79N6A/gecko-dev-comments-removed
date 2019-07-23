














































#ifndef _GTK_DRAWING_H_
#define _GTK_DRAWING_H_

#include <gdk/gdk.h>
#include <gtk/gtkstyle.h>

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
  
  MOZ_GTK_TAB_FIRST           = 1 << 0,
  
  MOZ_GTK_TAB_BEFORE_SELECTED = 1 << 1,
  
  MOZ_GTK_TAB_SELECTED        = 1 << 2
} GtkTabFlags;


typedef gint (*style_prop_t)(GtkStyle*, const gchar*, gint);


#define MOZ_GTK_SUCCESS 0
#define MOZ_GTK_UNKNOWN_WIDGET -1
#define MOZ_GTK_UNSAFE_THEME -2


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
  
  MOZ_GTK_SPINBUTTON_UP,
  MOZ_GTK_SPINBUTTON_DOWN,
  
  MOZ_GTK_GRIPPER,
  
  MOZ_GTK_ENTRY,
  
  MOZ_GTK_DROPDOWN,
  
  MOZ_GTK_DROPDOWN_ARROW,
  
  MOZ_GTK_CHECKBUTTON_CONTAINER,
  
  MOZ_GTK_RADIOBUTTON_CONTAINER,
  
  MOZ_GTK_CHECKBUTTON_LABEL,
  
  MOZ_GTK_RADIOBUTTON_LABEL,
  
  MOZ_GTK_TOOLBAR,
  
  MOZ_GTK_TOOLTIP,
  
  MOZ_GTK_FRAME,
  
  MOZ_GTK_PROGRESSBAR,
  
  MOZ_GTK_PROGRESS_CHUNK,
  
  MOZ_GTK_TAB,
  
  MOZ_GTK_TABPANELS,
  
  MOZ_GTK_MENUBAR,
  
  MOZ_GTK_MENUPOPUP,
  
  MOZ_GTK_MENUITEM,
  MOZ_GTK_CHECKMENUITEM,
  MOZ_GTK_RADIOMENUITEM,
  
  MOZ_GTK_WINDOW
} GtkThemeWidgetType;









gint moz_gtk_init();








gint moz_gtk_enable_style_props(style_prop_t styleGetProp);







gint moz_gtk_shutdown();











gint
moz_gtk_widget_paint(GtkThemeWidgetType widget, GdkDrawable* drawable,
                     GdkRectangle* rect, GdkRectangle* cliprect,
                     GtkWidgetState* state, gint flags);










gint moz_gtk_get_widget_border(GtkThemeWidgetType widget, gint* xthickness,
                               gint* ythickness);









gint
moz_gtk_checkbox_get_metrics(gint* indicator_size, gint* indicator_spacing);









gint
moz_gtk_radio_get_metrics(gint* indicator_size, gint* indicator_spacing);









gint
moz_gtk_button_get_focus(gboolean* interior_focus,
                         gint* focus_width, gint* focus_pad);
gint
moz_gtk_checkbox_get_focus(gboolean* interior_focus,
                           gint* focus_width, gint* focus_pad);
gint
moz_gtk_radio_get_focus(gboolean* interior_focus,
                        gint* focus_width, gint* focus_pad);









gint
moz_gtk_get_scalethumb_metrics(GtkOrientation orient, gint* thumb_length, gint* thumb_height);







gint
moz_gtk_get_scrollbar_metrics(MozGtkScrollbarMetrics* metrics);








gint moz_gtk_get_dropdown_arrow_size(gint* width, gint* height);

#ifdef __cplusplus
}
#endif 

#endif
