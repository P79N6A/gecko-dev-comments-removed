










































#include <gtk/gtk.h>
#include <gdk/gdkprivate.h>
#include <string.h>
#include "gtkdrawing.h"

static GtkWidget* gButtonWidget;
static GtkWidget* gProtoWindow;
static GtkWidget* gCheckboxWidget;
static GtkWidget* gScrollbarWidget;
static GtkWidget* gSpinWidget;
static GtkWidget* gHScaleWidget;
static GtkWidget* gVScaleWidget;
static GtkWidget* gEntryWidget;
static GtkWidget* gArrowWidget;
static GtkWidget* gDropdownButtonWidget;
static GtkWidget* gHandleBoxWidget;
static GtkWidget* gFrameWidget;
static GtkWidget* gProgressWidget;
static GtkWidget* gTabWidget;
static GtkTooltips* gTooltipWidget;

static style_prop_t style_prop_func;

typedef struct GtkThemeEnginePrivate {
  GtkThemeEngine engine;
  void* library;
  char* name;
} GtkThemeEnginePrivate;

static const char* sDisabledEngines[] = {
  "xeno",   
  NULL
};

gint
moz_gtk_enable_style_props(style_prop_t styleGetProp)
{
  style_prop_func = styleGetProp;
  return MOZ_GTK_SUCCESS;
}

static gint
setup_widget_prototype(GtkWidget* widget)
{
  static GtkWidget* protoLayout;

  if (!gProtoWindow) {
    gProtoWindow = gtk_window_new(GTK_WINDOW_POPUP);
    protoLayout = gtk_fixed_new();
    gtk_container_add(GTK_CONTAINER(gProtoWindow), protoLayout);
  }

  gtk_container_add(GTK_CONTAINER(protoLayout), widget);
  gtk_widget_set_rc_style(widget);
  gtk_widget_realize(widget);
  return MOZ_GTK_SUCCESS;
}

static gint
ensure_button_widget()
{
  if (!gButtonWidget) {
    gButtonWidget = gtk_button_new_with_label("M");
    setup_widget_prototype(gButtonWidget);
  }
  return MOZ_GTK_SUCCESS;
}

static gint
ensure_checkbox_widget()
{
  if (!gCheckboxWidget) {
    gCheckboxWidget = gtk_check_button_new_with_label("M");
    setup_widget_prototype(gCheckboxWidget);
  }
  return MOZ_GTK_SUCCESS;
}

static gint
ensure_scrollbar_widget()
{
  if (!gScrollbarWidget) {
    gScrollbarWidget = gtk_vscrollbar_new(NULL);
    setup_widget_prototype(gScrollbarWidget);
  }
  return MOZ_GTK_SUCCESS;
}

static gint
ensure_spin_widget()
{
  if (!gSpinWidget) {
    gSpinWidget = gtk_spin_button_new(NULL, 1, 0);
    setup_widget_prototype(gSpinWidget);
  }
  return MOZ_GTK_SUCCESS;
}

static gint
ensure_scale_widget()
{
  if (!gHScaleWidget) {
    gHScaleWidget = gtk_hscale_new(NULL);
    setup_widget_prototype(gHScaleWidget);
  }
  if (!gVScaleWidget) {
    gVScaleWidget = gtk_vscale_new(NULL);
    setup_widget_prototype(gVScaleWidget);
  }
  return MOZ_GTK_SUCCESS;
}

static gint
ensure_entry_widget()
{
  if (!gEntryWidget) {
    gEntryWidget = gtk_entry_new();
    setup_widget_prototype(gEntryWidget);
#ifdef _AIX
    





    if (GTK_EDITABLE(gEntryWidget)->ic &&
        GTK_EDITABLE(gEntryWidget)->ic_attr->style & GDK_IM_STATUS_NOTHING) {
      gdk_ic_destroy(GTK_EDITABLE(gEntryWidget)->ic);
      GTK_EDITABLE(gEntryWidget)->ic = NULL;
    }
#endif
  }
  return MOZ_GTK_SUCCESS;
}

static gint
ensure_arrow_widget()
{
  if (!gArrowWidget) {
    gDropdownButtonWidget = gtk_button_new();
    setup_widget_prototype(gDropdownButtonWidget);
    gArrowWidget = gtk_arrow_new(GTK_ARROW_DOWN, GTK_SHADOW_OUT);
    gtk_container_add(GTK_CONTAINER(gDropdownButtonWidget), gArrowWidget);
    gtk_widget_set_rc_style(gArrowWidget);
    gtk_widget_realize(gArrowWidget);
  }
  return MOZ_GTK_SUCCESS;
}

static gint
ensure_handlebox_widget()
{
  if (!gHandleBoxWidget) {
    gHandleBoxWidget = gtk_handle_box_new();
    setup_widget_prototype(gHandleBoxWidget);
  }
  return MOZ_GTK_SUCCESS;
}

static gint
ensure_tooltip_widget()
{
  if (!gTooltipWidget) {
    gTooltipWidget = gtk_tooltips_new();
    gtk_tooltips_force_window(gTooltipWidget);
    gtk_widget_set_rc_style(gTooltipWidget->tip_window);
    gtk_widget_realize(gTooltipWidget->tip_window);
  }
  return MOZ_GTK_SUCCESS;
}

static gint
ensure_tab_widget()
{
  if (!gTabWidget) {
    gTabWidget = gtk_notebook_new();
    setup_widget_prototype(gTabWidget);
  }
  return MOZ_GTK_SUCCESS;
}

static gint
ensure_progress_widget()
{
  if (!gProgressWidget) {
    gProgressWidget = gtk_progress_bar_new();
    setup_widget_prototype(gProgressWidget);
  }
  return MOZ_GTK_SUCCESS;
}

static gint
ensure_frame_widget()
{
  if (!gFrameWidget) {
    gFrameWidget = gtk_frame_new(NULL);
    setup_widget_prototype(gFrameWidget);
  }
  return MOZ_GTK_SUCCESS;
}

static GtkStateType
ConvertGtkState(GtkWidgetState* state)
{
  if (state->disabled)
    return GTK_STATE_INSENSITIVE;
  else if (state->inHover)
    return (state->active ? GTK_STATE_ACTIVE : GTK_STATE_PRELIGHT);
  else
    return GTK_STATE_NORMAL;
}

static gint
TSOffsetStyleGCArray(GdkGC** gcs, gint xorigin, gint yorigin)
{
  int i;
  
  for (i = 0; i < 5; ++i)
    gdk_gc_set_ts_origin(gcs[i], xorigin, yorigin);
  return MOZ_GTK_SUCCESS;
}

static gint
TSOffsetStyleGCs(GtkStyle* style, gint xorigin, gint yorigin)
{
  TSOffsetStyleGCArray(style->fg_gc, xorigin, yorigin);
  TSOffsetStyleGCArray(style->bg_gc, xorigin, yorigin);
  TSOffsetStyleGCArray(style->light_gc, xorigin, yorigin);
  TSOffsetStyleGCArray(style->dark_gc, xorigin, yorigin);
  TSOffsetStyleGCArray(style->mid_gc, xorigin, yorigin);
  TSOffsetStyleGCArray(style->text_gc, xorigin, yorigin);
  TSOffsetStyleGCArray(style->base_gc, xorigin, yorigin);
  gdk_gc_set_ts_origin(style->black_gc, xorigin, yorigin);
  gdk_gc_set_ts_origin(style->white_gc, xorigin, yorigin);
  return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_button_paint(GdkDrawable* drawable, GdkRectangle* rect,
                     GdkRectangle* cliprect, GtkWidgetState* state,
                     GtkReliefStyle relief, GtkWidget* widget)
{
  GtkShadowType shadow_type;
  GtkStyle* style = widget->style;
  gint default_spacing = 7; 
  GtkStateType button_state = ConvertGtkState(state);
  gint x = rect->x, y=rect->y, width=rect->width, height=rect->height;

  if (((GdkWindowPrivate*)drawable)->mapped) {
    gdk_window_set_back_pixmap(drawable, NULL, TRUE);
    gdk_window_clear_area(drawable, cliprect->x, cliprect->y, cliprect->width,
                          cliprect->height);
  }

  gtk_widget_set_state(widget, button_state);
  if (state->isDefault) {
    TSOffsetStyleGCs(style, x, y);
    gtk_paint_box(style, drawable, GTK_STATE_NORMAL, GTK_SHADOW_IN, cliprect,
                  widget, "buttondefault", x, y, width, height);
  }

  if (state->canDefault) {
    x += style->klass->xthickness;
    y += style->klass->ythickness;
    width -= 2 * x + default_spacing;
    height -= 2 * y + default_spacing;
    x += (1 + default_spacing) / 2;
    y += (1 + default_spacing) / 2;
  }
       
  if (state->focused) {
    x += 1;
    y += 1;
    width -= 2;
    height -= 2;
  }

  shadow_type = (state->active && state->inHover && !state->disabled) ? GTK_SHADOW_IN : GTK_SHADOW_OUT;

  if (relief != GTK_RELIEF_NONE || (button_state != GTK_STATE_NORMAL &&
                                    button_state != GTK_STATE_INSENSITIVE)) {
    TSOffsetStyleGCs(style, x, y);
    gtk_paint_box(style, drawable, button_state, shadow_type, cliprect,
                  widget, "button", x, y, width, height);
  }

  if (state->focused) {
    x -= 1;
    y -= 1;
    width += 2;
    height += 2;
    
    TSOffsetStyleGCs(style, x, y);
    gtk_paint_focus(style, drawable, cliprect, widget, "button", x, y,
                    width - 1, height - 1);
  }

  return MOZ_GTK_SUCCESS;
}

gint
moz_gtk_init()
{
  
  GtkThemeEngine* gtkThemeEng;
  ensure_button_widget();
  gtkThemeEng = gButtonWidget->style->engine;
  if (gtkThemeEng) {
    const char* curEngine = ((GtkThemeEnginePrivate*) gtkThemeEng)->name;
    const char* eng;
    int i = 0;
    while ((eng = sDisabledEngines[i++])) {
      if (!strcmp(eng, curEngine)) {
        g_warning("gtkdrawing found unsafe theme engine: %s\n", eng);
        return MOZ_GTK_UNSAFE_THEME;
      }
    }
  }

  return MOZ_GTK_SUCCESS;
}

gint
moz_gtk_checkbox_get_metrics(gint* indicator_size, gint* indicator_spacing)
{
  GtkCheckButtonClass* klass;

  ensure_checkbox_widget();
  klass = GTK_CHECK_BUTTON_CLASS(GTK_OBJECT(gCheckboxWidget)->klass);

  if (style_prop_func) {
    



    GtkStyle* style = gCheckboxWidget->style;

    *indicator_size = style_prop_func(style,
                                      "GtkCheckButton::indicator_size",
                                      klass->indicator_size);
    *indicator_spacing = style_prop_func(style,
                                         "GtkCheckButton::indicator_spacing",
                                         klass->indicator_spacing);
  } else {
    *indicator_size = klass->indicator_size;
    *indicator_spacing = klass->indicator_spacing;
  }

  return MOZ_GTK_SUCCESS;
}

gint
moz_gtk_radio_get_metrics(gint* indicator_size, gint* indicator_spacing)
{
  return moz_gtk_checkbox_get_metrics(indicator_size, indicator_spacing);
}

gint
moz_gtk_checkbox_get_focus(gboolean* interior_focus,
                           gint* focus_width, gint* focus_pad)
{
  
  *interior_focus = FALSE;
  *focus_width = 1;
  *focus_pad = 0;
  return MOZ_GTK_SUCCESS;
}

gint
moz_gtk_radio_get_focus(gboolean* interior_focus,
                        gint* focus_width, gint* focus_pad)
{
  return moz_gtk_checkbox_get_focus(interior_focus, focus_width, focus_pad);
}

static gint
moz_gtk_toggle_paint(GdkDrawable* drawable, GdkRectangle* rect,
                     GdkRectangle* cliprect, GtkWidgetState* state,
                     gboolean selected, gboolean isradio)
{
  GtkStateType state_type;
  GtkShadowType shadow_type;
  gint indicator_size, indicator_spacing;
  gint x, y, width, height;
  GtkStyle* style;

  moz_gtk_checkbox_get_metrics(&indicator_size, &indicator_spacing);
  style = gCheckboxWidget->style;

  
  x = rect->x + indicator_spacing;
  y = rect->y + (rect->height - indicator_size) / 2;
  width = indicator_size;
  height = indicator_size;
  
  if (selected) {
    state_type = GTK_STATE_ACTIVE;
    shadow_type = GTK_SHADOW_IN;
  }
  else {
    shadow_type = GTK_SHADOW_OUT;
    state_type = ConvertGtkState(state);
  }
  
  TSOffsetStyleGCs(style, x, y);

  
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gCheckboxWidget), selected);

  if (isradio)
    gtk_paint_option(style, drawable, state_type, shadow_type, cliprect,
                     gCheckboxWidget, "radiobutton", x, y, width, height);
  else
    gtk_paint_check(style, drawable, state_type, shadow_type, cliprect, 
                     gCheckboxWidget, "checkbutton", x, y, width, height);

  return MOZ_GTK_SUCCESS;
}

static gint
calculate_arrow_dimensions(GdkRectangle* rect, GdkRectangle* arrow_rect)
{
  GtkMisc* misc = GTK_MISC(gArrowWidget);

  gint extent = MIN(rect->width - misc->xpad * 2, rect->height - misc->ypad * 2);
  arrow_rect->x = ((rect->x + misc->xpad) * (1.0 - misc->xalign) +
                   (rect->x + rect->width - extent - misc->xpad) * misc->xalign);
  arrow_rect->y = ((rect->y + misc->ypad) * (1.0 - misc->yalign) +
                   (rect->y + rect->height - extent - misc->ypad) * misc->yalign);

  arrow_rect->width = arrow_rect->height = extent;
  return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_scrollbar_button_paint(GdkDrawable* drawable, GdkRectangle* rect,
                               GdkRectangle* cliprect, GtkWidgetState* state,
                               GtkArrowType type)
{
  GtkStateType state_type = ConvertGtkState(state);
  GtkShadowType shadow_type = (state->active && !state->disabled) ? GTK_SHADOW_IN : GTK_SHADOW_OUT;
  GdkRectangle arrow_rect;
  GtkStyle* style;

  ensure_scrollbar_widget();
  style = gScrollbarWidget->style;

  ensure_arrow_widget();
  calculate_arrow_dimensions(rect, &arrow_rect);
  TSOffsetStyleGCs(style, arrow_rect.x, arrow_rect.y);
  gtk_paint_arrow(style, drawable, state_type, shadow_type, cliprect,
                  gScrollbarWidget, (type < 2) ? "vscrollbar" : "hscrollbar", 
                  type, TRUE, arrow_rect.x, arrow_rect.y, arrow_rect.width,
                  arrow_rect.height);
  return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_scrollbar_trough_paint(GdkDrawable* drawable, GdkRectangle* rect,
                               GdkRectangle* cliprect, GtkWidgetState* state)
{
  GtkStyle* style;

  ensure_scrollbar_widget();
  style = gScrollbarWidget->style;

  TSOffsetStyleGCs(style, rect->x, rect->y);
  gtk_style_apply_default_background(style, drawable, TRUE, GTK_STATE_ACTIVE,
                                     cliprect, rect->x, rect->y,
                                     rect->width, rect->height);

  gtk_paint_box(style, drawable, GTK_STATE_ACTIVE, GTK_SHADOW_IN, cliprect,
                gScrollbarWidget, "trough", rect->x, rect->y,  rect->width,
                rect->height);

  if (state->focused)
    gtk_paint_focus(style, drawable, cliprect, gScrollbarWidget, "trough",
                    rect->x, rect->y, rect->width, rect->height);

  return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_scrollbar_thumb_paint(GdkDrawable* drawable, GdkRectangle* rect,
                              GdkRectangle* cliprect, GtkWidgetState* state)
{
  GtkStateType state_type = (state->inHover || state->active) ? GTK_STATE_PRELIGHT : GTK_STATE_NORMAL;
  GtkStyle* style;

  ensure_scrollbar_widget();
  style = gScrollbarWidget->style;

  TSOffsetStyleGCs(style, rect->x, rect->y);
  gtk_paint_box(style, drawable, state_type, GTK_SHADOW_OUT, cliprect,
                gScrollbarWidget, "slider", rect->x, rect->y, rect->width, 
                rect->height);

  return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_spin_paint(GdkDrawable* drawable, GdkRectangle* rect, gboolean isDown,
                   GtkWidgetState* state)
{
    GdkRectangle arrow_rect;
    GtkStateType state_type = ConvertGtkState(state);
    GtkShadowType shadow_type = state_type == GTK_STATE_ACTIVE ? GTK_SHADOW_IN : GTK_SHADOW_OUT;
    GtkStyle* style;

    ensure_spin_widget();
    style = gSpinWidget->style;

    TSOffsetStyleGCs(style, rect->x, rect->y);
    gtk_paint_box(style, drawable, state_type, shadow_type, NULL, gSpinWidget,
                  isDown ? "spinbutton_down" : "spinbutton_up",
                  rect->x, rect->y, rect->width, rect->height);

    
    arrow_rect.width = 6;
    arrow_rect.height = 6;
    arrow_rect.x = rect->x + (rect->width - arrow_rect.width) / 2;
    arrow_rect.y = rect->y + (rect->height - arrow_rect.height) / 2;
    arrow_rect.y += isDown ? -1 : 1;

    gtk_paint_arrow(style, drawable, state_type, shadow_type, NULL,
                    gSpinWidget, "spinbutton",
                    isDown ? GTK_ARROW_DOWN : GTK_ARROW_UP, TRUE,
                    arrow_rect.x, arrow_rect.y,
                    arrow_rect.width, arrow_rect.height);

    return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_scale_paint(GdkDrawable* drawable, GdkRectangle* rect,
                    GdkRectangle* cliprect, GtkWidgetState* state,
                    GtkOrientation flags)
{
  gint x = 0, y = 0;
  GtkStateType state_type = ConvertGtkState(state);
  GtkStyle* style;
  GtkWidget* widget;

  ensure_scale_widget();
  widget = ((flags == GTK_ORIENTATION_HORIZONTAL) ? gHScaleWidget : gVScaleWidget);
  style = widget->style;

  if (flags == GTK_ORIENTATION_HORIZONTAL) {
    x = style->klass->xthickness;
    y++;
  }
  else {
    x++;
    y = style->klass->ythickness;
  }

  TSOffsetStyleGCs(style, rect->x, rect->y);
  gtk_style_apply_default_background(style, drawable, TRUE, GTK_STATE_NORMAL,
                                     cliprect, rect->x, rect->y,
                                     rect->width, rect->height);

  gtk_paint_box(style, drawable, GTK_STATE_ACTIVE, GTK_SHADOW_IN, cliprect,
                widget, "trough", rect->x + x, rect->y + y,
                rect->width - 2*x, rect->height - 2*y);

  if (state->focused)
    gtk_paint_focus(style, drawable, cliprect, widget, "trough",
                    rect->x, rect->y, rect->width, rect->height);

  return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_scale_thumb_paint(GdkDrawable* drawable, GdkRectangle* rect,
                          GdkRectangle* cliprect, GtkWidgetState* state,
                          GtkOrientation flags)
{
  GtkStateType state_type = ConvertGtkState(state);
  GtkStyle* style;
  GtkWidget* widget;
  gint thumb_width, thumb_height, x, y;

  ensure_scale_widget();
  widget = ((flags == GTK_ORIENTATION_HORIZONTAL) ? gHScaleWidget : gVScaleWidget);
  style = widget->style;

  
  if (flags == GTK_ORIENTATION_HORIZONTAL) {
    moz_gtk_get_scalethumb_metrics(GTK_ORIENTATION_HORIZONTAL, &thumb_width, &thumb_height);
    x = rect->x;
    y = rect->y + (rect->height - thumb_height) / 2;
  }
  else {
    moz_gtk_get_scalethumb_metrics(GTK_ORIENTATION_VERTICAL, &thumb_height, &thumb_width);
    x = rect->x + (rect->width - thumb_width) / 2;
    y = rect->y;
  }

  TSOffsetStyleGCs(style, rect->x, rect->y);
  gtk_paint_slider(style, drawable, state_type, GTK_SHADOW_OUT, cliprect,
                   widget, (flags == GTK_ORIENTATION_HORIZONTAL) ? "hscale" : "vscale",
                   rect->x, rect->y, thumb_width, thumb_height, flags);

  return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_gripper_paint(GdkDrawable* drawable, GdkRectangle* rect,
                      GdkRectangle* cliprect, GtkWidgetState* state)
{
  GtkStateType state_type = ConvertGtkState(state);
  GtkShadowType shadow_type;
  GtkStyle* style;

  ensure_handlebox_widget();
  style = gHandleBoxWidget->style;
  shadow_type = GTK_HANDLE_BOX(gHandleBoxWidget)->shadow_type;

  TSOffsetStyleGCs(style, rect->x, rect->y);
  gtk_paint_box(style, drawable, state_type, shadow_type, cliprect,
                gHandleBoxWidget, "handlebox_bin", rect->x, rect->y,
                rect->width, rect->height);

  return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_entry_paint(GdkDrawable* drawable, GdkRectangle* rect,
                    GdkRectangle* cliprect, GtkWidgetState* state)
{
  gint x = rect->x, y = rect->y, width = rect->width, height = rect->height;
  GtkStyle* style;

  ensure_entry_widget();
  style = gEntryWidget->style;

  








  TSOffsetStyleGCs(style, x, y);
  gtk_paint_shadow(style, drawable, GTK_STATE_NORMAL, GTK_SHADOW_IN, cliprect,
                   gEntryWidget, "entry", x, y, width, height);
  

  if (state->focused) {
      x += 1;
      y += 1;
      width -= 2;
      height -= 2;
      TSOffsetStyleGCs(style, x, y);
      gtk_paint_shadow(style, drawable, GTK_STATE_NORMAL, GTK_SHADOW_IN,
                       cliprect, gEntryWidget, "entry", x, y, width, height);

      TSOffsetStyleGCs(style, rect->x, rect->y);
      gtk_paint_focus(style, drawable,  cliprect, gEntryWidget, "entry",
                      rect->x, rect->y, rect->width - 1, rect->height - 1);
  }

  x = style->klass->xthickness;
  y = style->klass->ythickness;

  TSOffsetStyleGCs(style, rect->x + x, rect->y + y);
  gtk_paint_flat_box(style, drawable, GTK_STATE_NORMAL, GTK_SHADOW_NONE,
                     cliprect, gEntryWidget, "entry_bg",  rect->x + x,
                     rect->y + y, rect->width - 2*x, rect->height - 2*y);

  return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_dropdown_arrow_paint(GdkDrawable* drawable, GdkRectangle* rect,
                             GdkRectangle* cliprect, GtkWidgetState* state)
{
  GdkRectangle arrow_rect, real_arrow_rect;
  GtkStateType state_type = ConvertGtkState(state);
  GtkShadowType shadow_type = (state->active && !state->disabled) ? GTK_SHADOW_IN : GTK_SHADOW_OUT;
  GtkStyle* style;

  ensure_arrow_widget();
  moz_gtk_button_paint(drawable, rect, cliprect, state, GTK_RELIEF_NORMAL,
                       gDropdownButtonWidget);

  
  style = gDropdownButtonWidget->style;
  arrow_rect.x = rect->x + 1 + gDropdownButtonWidget->style->klass->xthickness;
  arrow_rect.y = rect->y + 1 + gDropdownButtonWidget->style->klass->ythickness;
  arrow_rect.width = MAX(1, rect->width - (arrow_rect.x - rect->x) * 2);
  arrow_rect.height = MAX(1, rect->height - (arrow_rect.y - rect->y) * 2);

  calculate_arrow_dimensions(&arrow_rect, &real_arrow_rect);
  style = gArrowWidget->style;
  TSOffsetStyleGCs(style, real_arrow_rect.x, real_arrow_rect.y);
  gtk_paint_arrow(style, drawable, state_type, shadow_type, cliprect,
                  gScrollbarWidget, "arrow",  GTK_ARROW_DOWN, TRUE,
                  real_arrow_rect.x, real_arrow_rect.y,
                  real_arrow_rect.width, real_arrow_rect.height);

  return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_container_paint(GdkDrawable* drawable, GdkRectangle* rect,
                        GdkRectangle* cliprect, GtkWidgetState* state, 
                        gboolean isradio)
{
  GtkStateType state_type = ConvertGtkState(state);
  GtkStyle* style;

  ensure_checkbox_widget();
  style = gCheckboxWidget->style;

  if (state_type != GTK_STATE_NORMAL && state_type != GTK_STATE_PRELIGHT)
    state_type = GTK_STATE_NORMAL;
  
  TSOffsetStyleGCs(style, rect->x, rect->y);

  if (state_type != GTK_STATE_NORMAL) 
    gtk_paint_flat_box(style, drawable, state_type, GTK_SHADOW_ETCHED_OUT,
                       cliprect, gCheckboxWidget,
                       isradio ? "radiobutton" : "checkbutton",
                       rect->x, rect->y, rect->width, rect->height);

  if (state->focused)
    gtk_paint_focus(style, drawable, cliprect, gCheckboxWidget, "checkbutton",
                    rect->x, rect->y, rect->width - 1, rect->height - 1);

  return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_toolbar_paint(GdkDrawable* drawable, GdkRectangle* rect,
                      GdkRectangle* cliprect)
{
  GtkStyle* style;

  ensure_handlebox_widget();
  style = gHandleBoxWidget->style;

  TSOffsetStyleGCs(style, rect->x, rect->y);
  if (style->bg_pixmap[GTK_STATE_NORMAL])
    gtk_style_apply_default_background(style, drawable, TRUE, GTK_STATE_NORMAL,
                                       cliprect, rect->x, rect->y,
                                       rect->width, rect->height);
  else
    gtk_paint_box(style, drawable, GTK_STATE_NORMAL, GTK_SHADOW_OUT, cliprect,
                  gHandleBoxWidget, "dockitem_bin",
                  rect->x, rect->y, rect->width, rect->height);

  return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_tooltip_paint(GdkDrawable* drawable, GdkRectangle* rect,
                      GdkRectangle* cliprect)
{
  GtkStyle* style;

  ensure_tooltip_widget();
  style = gTooltipWidget->tip_window->style;

  TSOffsetStyleGCs(style, rect->x, rect->y);
  gtk_paint_flat_box(style, drawable, GTK_STATE_NORMAL, GTK_SHADOW_OUT,
                     cliprect, gTooltipWidget->tip_window, "tooltip", rect->x,
                     rect->y, rect->width, rect->height);

  return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_frame_paint(GdkDrawable* drawable, GdkRectangle* rect,
                    GdkRectangle* cliprect)
{
  GtkStyle* style = gProtoWindow->style;

  TSOffsetStyleGCs(style, rect->x, rect->y);
  gtk_paint_flat_box(style, drawable, GTK_STATE_NORMAL, GTK_SHADOW_NONE, NULL,
                     gProtoWindow, "base", rect->x, rect->y, rect->width,
                     rect->height);

  ensure_frame_widget();
  style = gFrameWidget->style;

  TSOffsetStyleGCs(style, rect->x, rect->y);
  gtk_paint_shadow(style, drawable, GTK_STATE_NORMAL, GTK_SHADOW_IN, cliprect,
                   gFrameWidget, "frame", rect->x, rect->y, rect->width,
                   rect->height);

  return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_progressbar_paint(GdkDrawable* drawable, GdkRectangle* rect,
                          GdkRectangle* cliprect)
{
  GtkStyle* style;

  ensure_progress_widget();
  style = gProgressWidget->style;

  TSOffsetStyleGCs(style, rect->x, rect->y);
  gtk_paint_box(style, drawable, GTK_STATE_NORMAL, GTK_SHADOW_IN,
                cliprect, gProgressWidget, "trough", rect->x, rect->y,
                rect->width, rect->height);

  return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_progress_chunk_paint(GdkDrawable* drawable, GdkRectangle* rect,
                             GdkRectangle* cliprect)
{
  GtkStyle* style;

  ensure_progress_widget();
  style = gProgressWidget->style;

  TSOffsetStyleGCs(style, rect->x, rect->y);
  gtk_paint_box(style, drawable, GTK_STATE_PRELIGHT, GTK_SHADOW_OUT,
                cliprect, gProgressWidget, "bar", rect->x, rect->y,
                rect->width, rect->height);

  return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_tab_paint(GdkDrawable* drawable, GdkRectangle* rect,
                  GdkRectangle* cliprect, gint flags)
{
  




























  GtkStyle* style;
  ensure_tab_widget();

  if (!(flags & MOZ_GTK_TAB_FIRST)) {
    rect->x -= 2;
    rect->width += 2;
  }

  style = gTabWidget->style;
  TSOffsetStyleGCs(style, rect->x, rect->y);
  gtk_paint_extension(style, drawable,
                      ((flags & MOZ_GTK_TAB_SELECTED) ? GTK_STATE_NORMAL : GTK_STATE_ACTIVE),
                      GTK_SHADOW_OUT, cliprect, gTabWidget, "tab", rect->x,
                      rect->y, rect->width, rect->height, GTK_POS_BOTTOM);

  if (flags & MOZ_GTK_TAB_BEFORE_SELECTED) {
    gboolean before_selected = ((flags & MOZ_GTK_TAB_BEFORE_SELECTED) != 0);
    cliprect->y -= 2;
    cliprect->height += 2;
    TSOffsetStyleGCs(style, rect->x + rect->width - 2, rect->y - (2 * before_selected));
    gtk_paint_extension(style, drawable, GTK_STATE_NORMAL, GTK_SHADOW_OUT,
                        cliprect, gTabWidget, "tab", rect->x + rect->width - 2,
                        rect->y - (2 * before_selected), rect->width,
                        rect->height + (2 * before_selected), GTK_POS_BOTTOM);
  }

  return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_tabpanels_paint(GdkDrawable* drawable, GdkRectangle* rect,
                        GdkRectangle* cliprect)
{
  GtkStyle* style;

  ensure_tab_widget();
  style = gTabWidget->style;

  TSOffsetStyleGCs(style, rect->x, rect->y);
  gtk_paint_box(style, drawable, GTK_STATE_NORMAL, GTK_SHADOW_OUT,
                cliprect, gTabWidget, "notebook", rect->x, rect->y,
                rect->width, rect->height);

  return MOZ_GTK_SUCCESS;
}

gint
moz_gtk_get_widget_border(GtkThemeWidgetType widget, gint* xthickness,
                          gint* ythickness)
{
  GtkWidget* w;
  switch (widget) {
  case MOZ_GTK_BUTTON:
    ensure_button_widget();
    w = gButtonWidget;
    break;
  case MOZ_GTK_TOOLBAR:
    ensure_handlebox_widget();
    w = gHandleBoxWidget;
    break;
  case MOZ_GTK_ENTRY:
    ensure_entry_widget();
    w = gEntryWidget;
    break;
  case MOZ_GTK_DROPDOWN_ARROW:
    ensure_arrow_widget();
    w = gDropdownButtonWidget;
    break;
  case MOZ_GTK_TABPANELS:
    ensure_tab_widget();
    w = gTabWidget;
    break;
  case MOZ_GTK_PROGRESSBAR:
    ensure_progress_widget();
    w = gProgressWidget;
    break;
  case MOZ_GTK_SPINBUTTON_UP:
  case MOZ_GTK_SPINBUTTON_DOWN:
    ensure_spin_widget();
    w = gSpinWidget;
    break;
  case MOZ_GTK_SCALE_HORIZONTAL:
    ensure_scale_widget();
    w = gHScaleWidget;
    break;
  case MOZ_GTK_SCALE_VERTICAL:
    ensure_scale_widget();
    w = gVScaleWidget;
    break;
  case MOZ_GTK_FRAME:
    ensure_frame_widget();
    w = gFrameWidget;
    break;
  case MOZ_GTK_CHECKBUTTON_CONTAINER:
  case MOZ_GTK_RADIOBUTTON_CONTAINER:
    ensure_checkbox_widget();
    *xthickness = *ythickness =
      GTK_CONTAINER(gCheckboxWidget)->border_width + 1;

    return MOZ_GTK_SUCCESS;

  case MOZ_GTK_CHECKBUTTON_LABEL:
  case MOZ_GTK_RADIOBUTTON_LABEL:
    *xthickness = *ythickness = 0;
    return MOZ_GTK_SUCCESS;

  case MOZ_GTK_CHECKBUTTON:
  case MOZ_GTK_RADIOBUTTON:
  case MOZ_GTK_SCROLLBAR_BUTTON:
  case MOZ_GTK_SCROLLBAR_TRACK_HORIZONTAL:
  case MOZ_GTK_SCROLLBAR_TRACK_VERTICAL:
  case MOZ_GTK_SCROLLBAR_THUMB_HORIZONTAL:
  case MOZ_GTK_SCROLLBAR_THUMB_VERTICAL:
  case MOZ_GTK_SCALE_THUMB_HORIZONTAL:
  case MOZ_GTK_SCALE_THUMB_VERTICAL:
  case MOZ_GTK_GRIPPER:
  case MOZ_GTK_TOOLTIP:
  case MOZ_GTK_PROGRESS_CHUNK:
  case MOZ_GTK_TAB:
    
    *xthickness = 0;
    *ythickness = 0;
    return MOZ_GTK_SUCCESS;
  default:
    g_warning("Unsupported widget type: %d", widget);
    return MOZ_GTK_UNKNOWN_WIDGET;
  }

  *xthickness = w->style->klass->xthickness;
  *ythickness = w->style->klass->ythickness;

  return MOZ_GTK_SUCCESS;
}

gint
moz_gtk_get_dropdown_arrow_size(gint* width, gint* height)
{
  ensure_arrow_widget();

  





  *width = 2 * (1 + gDropdownButtonWidget->style->klass->xthickness);
  *width += 11 + GTK_MISC(gArrowWidget)->xpad * 2;

  *height = 2 * (1 + gDropdownButtonWidget->style->klass->ythickness);
  *height += 11 + GTK_MISC(gArrowWidget)->ypad * 2;

  return MOZ_GTK_SUCCESS;
}

gint
moz_gtk_get_scalethumb_metrics(GtkOrientation orient, gint* thumb_length, gint* thumb_height)
{
  GtkRangeClass* rangeklass;
  GtkScaleClass* scaleklass;
  GtkStyle* style;
  GtkWidget* widget;

  ensure_scale_widget();
  widget = ((orient == GTK_ORIENTATION_HORIZONTAL) ? gHScaleWidget : gVScaleWidget);

  rangeklass = GTK_RANGE_CLASS(GTK_OBJECT(widget)->klass);
  scaleklass = GTK_SCALE_CLASS(GTK_OBJECT(widget)->klass);
  style = widget->style;

  if (style_prop_func) {
    



    *thumb_length = style_prop_func(style, "GtkRange::slider_length",
                                    scaleklass->slider_length);
    *thumb_height = style_prop_func(style, "GtkRange::slider_width",
                                    rangeklass->slider_width);
  } else {
    



    *thumb_length = scaleklass->slider_length;
    *thumb_height = rangeklass->slider_width;
  }

  return MOZ_GTK_SUCCESS;
}

gint
moz_gtk_get_scrollbar_metrics(MozGtkScrollbarMetrics *metrics)
{
  GtkRangeClass* klass;
  GtkStyle* style;

  ensure_scrollbar_widget();
  klass = GTK_RANGE_CLASS(GTK_OBJECT(gScrollbarWidget)->klass);
  style = gScrollbarWidget->style;

  if (style_prop_func) {
    



    metrics->slider_width = style_prop_func(style, "GtkRange::slider_width",
                                            klass->slider_width);

    metrics->trough_border = style_prop_func(style, "GtkRange::trough_border",
                                             style->klass->xthickness);

    metrics->stepper_size = style_prop_func(style, "GtkRange::stepper_size",
                                            klass->stepper_size);

    metrics->stepper_spacing = style_prop_func(style,
                                               "GtkRange::stepper_spacing",
                                               klass->stepper_slider_spacing);
  } else {
    



    metrics->slider_width = klass->slider_width;
    metrics->trough_border = style->klass->xthickness;
    metrics->stepper_size = klass->stepper_size;
    metrics->stepper_spacing = klass->stepper_slider_spacing;
  }

  metrics->min_slider_size = klass->min_slider_size;

  return MOZ_GTK_SUCCESS;
}

gint
moz_gtk_widget_paint(GtkThemeWidgetType widget, GdkDrawable* drawable,
                     GdkRectangle* rect, GdkRectangle* cliprect,
                     GtkWidgetState* state, gint flags)
{
  switch (widget) {
  case MOZ_GTK_BUTTON:
    ensure_button_widget();
    return moz_gtk_button_paint(drawable, rect, cliprect, state,
                                (GtkReliefStyle) flags, gButtonWidget);
    break;
  case MOZ_GTK_CHECKBUTTON:
  case MOZ_GTK_RADIOBUTTON:
    return moz_gtk_toggle_paint(drawable, rect, cliprect, state,
                                (gboolean) flags,
                                (widget == MOZ_GTK_RADIOBUTTON));
    break;
  case MOZ_GTK_SCROLLBAR_BUTTON:
    return moz_gtk_scrollbar_button_paint(drawable, rect, cliprect,
                                          state, (GtkArrowType) flags);
    break;
  case MOZ_GTK_SCROLLBAR_TRACK_HORIZONTAL:
  case MOZ_GTK_SCROLLBAR_TRACK_VERTICAL:
    return moz_gtk_scrollbar_trough_paint(drawable, rect, cliprect, state);
    break;
  case MOZ_GTK_SCROLLBAR_THUMB_HORIZONTAL:
  case MOZ_GTK_SCROLLBAR_THUMB_VERTICAL:
    return moz_gtk_scrollbar_thumb_paint(drawable, rect, cliprect, state);
    break;
  case MOZ_GTK_SCALE_HORIZONTAL:
  case MOZ_GTK_SCALE_VERTICAL:
    return moz_gtk_scale_paint(drawable, rect, cliprect, state, (GtkOrientation) flags);
    break;
  case MOZ_GTK_SCALE_THUMB_HORIZONTAL:
  case MOZ_GTK_SCALE_THUMB_VERTICAL:
    return moz_gtk_scale_thumb_paint(drawable, rect, cliprect, state, (GtkOrientation) flags);
    break;
  case MOZ_GTK_SPINBUTTON_UP:
  case MOZ_GTK_SPINBUTTON_DOWN:
    return moz_gtk_spin_paint(drawable, rect,
                              (widget == MOZ_GTK_SPINBUTTON_DOWN), state);
    break;
  case MOZ_GTK_GRIPPER:
    return moz_gtk_gripper_paint(drawable, rect, cliprect, state);
    break;
  case MOZ_GTK_ENTRY:
    return moz_gtk_entry_paint(drawable, rect, cliprect, state);
    break;
  case MOZ_GTK_DROPDOWN_ARROW:
    return moz_gtk_dropdown_arrow_paint(drawable, rect, cliprect, state);
    break;
  case MOZ_GTK_CHECKBUTTON_CONTAINER:
  case MOZ_GTK_RADIOBUTTON_CONTAINER:
    return moz_gtk_container_paint(drawable, rect, cliprect, state,
                                   (widget == MOZ_GTK_RADIOBUTTON_CONTAINER));
    break;
  case MOZ_GTK_TOOLBAR:
    return moz_gtk_toolbar_paint(drawable, rect, cliprect);
    break;
  case MOZ_GTK_TOOLTIP:
    return moz_gtk_tooltip_paint(drawable, rect, cliprect);
    break;
  case MOZ_GTK_FRAME:
    return moz_gtk_frame_paint(drawable, rect, cliprect);
    break;
  case MOZ_GTK_PROGRESSBAR:
    return moz_gtk_progressbar_paint(drawable, rect, cliprect);
    break;
  case MOZ_GTK_PROGRESS_CHUNK:
    return moz_gtk_progress_chunk_paint(drawable, rect, cliprect);
    break;
  case MOZ_GTK_TAB:
    return moz_gtk_tab_paint(drawable, rect, cliprect, flags);
    break;
  case MOZ_GTK_TABPANELS:
    return moz_gtk_tabpanels_paint(drawable, rect, cliprect);
    break;
  case MOZ_GTK_CHECKBUTTON_LABEL:
  case MOZ_GTK_RADIOBUTTON_LABEL:
    

    return MOZ_GTK_SUCCESS;
  default:
    g_warning("Unknown widget type: %d", widget);
  }

  return MOZ_GTK_UNKNOWN_WIDGET;
}

gint
moz_gtk_shutdown()
{
  if (gTooltipWidget)
    gtk_object_unref(GTK_OBJECT(gTooltipWidget));
  
  if (gProtoWindow)
    gtk_widget_destroy(gProtoWindow);

  gButtonWidget = NULL;
  gProtoWindow = NULL;
  gCheckboxWidget = NULL;
  gScrollbarWidget = NULL;
  gEntryWidget = NULL;
  gArrowWidget = NULL;
  gDropdownButtonWidget = NULL;
  gHandleBoxWidget = NULL;
  gFrameWidget = NULL;
  gProgressWidget = NULL;
  gTabWidget = NULL;
  gTooltipWidget = NULL;

  return MOZ_GTK_SUCCESS;
}
