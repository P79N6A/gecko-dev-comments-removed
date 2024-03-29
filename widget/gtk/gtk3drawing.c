









#include <gtk/gtk.h>
#include <gdk/gdkprivate.h>
#include <string.h>
#include "gtkdrawing.h"
#include "mozilla/Assertions.h"
#include "prinrval.h"

#include <math.h>

static GtkWidget* gProtoWindow;
static GtkWidget* gProtoLayout;
static GtkWidget* gButtonWidget;
static GtkWidget* gToggleButtonWidget;
static GtkWidget* gButtonArrowWidget;
static GtkWidget* gCheckboxWidget;
static GtkWidget* gRadiobuttonWidget;
static GtkWidget* gHorizScrollbarWidget;
static GtkWidget* gVertScrollbarWidget;
static GtkWidget* gSpinWidget;
static GtkWidget* gHScaleWidget;
static GtkWidget* gVScaleWidget;
static GtkWidget* gEntryWidget;
static GtkWidget* gComboBoxWidget;
static GtkWidget* gComboBoxButtonWidget;
static GtkWidget* gComboBoxArrowWidget;
static GtkWidget* gComboBoxSeparatorWidget;
static GtkWidget* gComboBoxEntryWidget;
static GtkWidget* gComboBoxEntryTextareaWidget;
static GtkWidget* gComboBoxEntryButtonWidget;
static GtkWidget* gComboBoxEntryArrowWidget;
static GtkWidget* gHandleBoxWidget;
static GtkWidget* gToolbarWidget;
static GtkWidget* gFrameWidget;
static GtkWidget* gStatusbarWidget;
static GtkWidget* gProgressWidget;
static GtkWidget* gTabWidget;
static GtkWidget* gTooltipWidget;
static GtkWidget* gMenuBarWidget;
static GtkWidget* gMenuBarItemWidget;
static GtkWidget* gMenuPopupWidget;
static GtkWidget* gMenuItemWidget;
static GtkWidget* gImageMenuItemWidget;
static GtkWidget* gCheckMenuItemWidget;
static GtkWidget* gTreeViewWidget;
static GtkTreeViewColumn* gMiddleTreeViewColumn;
static GtkWidget* gTreeHeaderCellWidget;
static GtkWidget* gTreeHeaderSortArrowWidget;
static GtkWidget* gExpanderWidget;
static GtkWidget* gToolbarSeparatorWidget;
static GtkWidget* gMenuSeparatorWidget;
static GtkWidget* gHPanedWidget;
static GtkWidget* gVPanedWidget;
static GtkWidget* gScrolledWindowWidget;

static style_prop_t style_prop_func;
static gboolean have_arrow_scaling;
static gboolean checkbox_check_state;
static gboolean notebook_has_tab_gap;
static gboolean is_initialized;

#define ARROW_UP      0
#define ARROW_DOWN    G_PI
#define ARROW_RIGHT   G_PI_2
#define ARROW_LEFT    (G_PI+G_PI_2)

#if !GTK_CHECK_VERSION(3,14,0)
#define GTK_STATE_FLAG_CHECKED (1 << 11)
#endif

static GtkStateFlags
GetStateFlagsFromGtkWidgetState(GtkWidgetState* state)
{
    GtkStateFlags stateFlags = GTK_STATE_FLAG_NORMAL;

    if (state->disabled)
        stateFlags = GTK_STATE_FLAG_INSENSITIVE;
    else {    
        if (state->depressed || state->active)
            stateFlags |= GTK_STATE_FLAG_ACTIVE;
        if (state->inHover)
            stateFlags |= GTK_STATE_FLAG_PRELIGHT;
        if (state->focused)
            stateFlags |= GTK_STATE_FLAG_FOCUSED;
    }
  
    return stateFlags;
}




static void
moz_gtk_set_widget_name(GtkWidget* widget)
{
    gtk_widget_set_name(widget, "MozillaGtkWidget");
}

gint
moz_gtk_enable_style_props(style_prop_t styleGetProp)
{
    style_prop_func = styleGetProp;
    return MOZ_GTK_SUCCESS;
}

static gint
ensure_window_widget()
{
    if (!gProtoWindow) {
        gProtoWindow = gtk_window_new(GTK_WINDOW_POPUP);
        gtk_widget_realize(gProtoWindow);
        moz_gtk_set_widget_name(gProtoWindow);
    }
    return MOZ_GTK_SUCCESS;
}

static gint
setup_widget_prototype(GtkWidget* widget)
{
    ensure_window_widget();
    if (!gProtoLayout) {
        gProtoLayout = gtk_fixed_new();
        gtk_container_add(GTK_CONTAINER(gProtoWindow), gProtoLayout);
    }

    gtk_container_add(GTK_CONTAINER(gProtoLayout), widget);
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
ensure_hpaned_widget()
{
    if (!gHPanedWidget) {
        gHPanedWidget = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
        setup_widget_prototype(gHPanedWidget);
    }
    return MOZ_GTK_SUCCESS;
}

static gint
ensure_vpaned_widget()
{
    if (!gVPanedWidget) {
        gVPanedWidget = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
        setup_widget_prototype(gVPanedWidget);
    }
    return MOZ_GTK_SUCCESS;
}

static gint
ensure_toggle_button_widget()
{
    if (!gToggleButtonWidget) {
        gToggleButtonWidget = gtk_toggle_button_new();
        setup_widget_prototype(gToggleButtonWidget);
  }
  return MOZ_GTK_SUCCESS;
}

static gint
ensure_button_arrow_widget()
{
    if (!gButtonArrowWidget) {
        ensure_toggle_button_widget();

        gButtonArrowWidget = gtk_arrow_new(GTK_ARROW_DOWN, GTK_SHADOW_OUT);
        gtk_container_add(GTK_CONTAINER(gToggleButtonWidget), gButtonArrowWidget);
        gtk_widget_realize(gButtonArrowWidget);
        gtk_widget_show(gButtonArrowWidget);
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
ensure_radiobutton_widget()
{
    if (!gRadiobuttonWidget) {
        gRadiobuttonWidget = gtk_radio_button_new_with_label(NULL, "M");
        setup_widget_prototype(gRadiobuttonWidget);
    }
    return MOZ_GTK_SUCCESS;
}

static gint
ensure_scrollbar_widget()
{
    if (!gVertScrollbarWidget) {
        gVertScrollbarWidget = gtk_scrollbar_new(GTK_ORIENTATION_VERTICAL, NULL);
        setup_widget_prototype(gVertScrollbarWidget);
    }
    if (!gHorizScrollbarWidget) {
        gHorizScrollbarWidget = gtk_scrollbar_new(GTK_ORIENTATION_HORIZONTAL, NULL);
        setup_widget_prototype(gHorizScrollbarWidget);
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
    gHScaleWidget = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, NULL);
    setup_widget_prototype(gHScaleWidget);
  }
  if (!gVScaleWidget) {
    gVScaleWidget = gtk_scale_new(GTK_ORIENTATION_VERTICAL, NULL);
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
    }
    return MOZ_GTK_SUCCESS;
}











static void
moz_gtk_get_combo_box_inner_button(GtkWidget *widget, gpointer client_data)
{
    if (GTK_IS_TOGGLE_BUTTON(widget)) {
        gComboBoxButtonWidget = widget;
        g_object_add_weak_pointer(G_OBJECT(widget),
                                  (gpointer) &gComboBoxButtonWidget);
        gtk_widget_realize(widget);
    }
}

static void
moz_gtk_get_combo_box_button_inner_widgets(GtkWidget *widget,
                                           gpointer client_data)
{
    if (GTK_IS_SEPARATOR(widget)) {
        gComboBoxSeparatorWidget = widget;
        g_object_add_weak_pointer(G_OBJECT(widget),
                                  (gpointer) &gComboBoxSeparatorWidget);
    } else if (GTK_IS_ARROW(widget)) {
        gComboBoxArrowWidget = widget;
        g_object_add_weak_pointer(G_OBJECT(widget),
                                  (gpointer) &gComboBoxArrowWidget);
    } else
        return;
    gtk_widget_realize(widget);
}

static gint
ensure_combo_box_widgets()
{
    GtkWidget* buttonChild;

    if (gComboBoxButtonWidget && gComboBoxArrowWidget)
        return MOZ_GTK_SUCCESS;

    
    if (!gComboBoxWidget) {
        gComboBoxWidget = gtk_combo_box_new();
        setup_widget_prototype(gComboBoxWidget);
    }

    
    gtk_container_forall(GTK_CONTAINER(gComboBoxWidget),
                         moz_gtk_get_combo_box_inner_button,
                         NULL);

    if (gComboBoxButtonWidget) {
        
        buttonChild = gtk_bin_get_child(GTK_BIN(gComboBoxButtonWidget));
        if (GTK_IS_BOX(buttonChild)) {
            



            gtk_container_forall(GTK_CONTAINER(buttonChild),
                                 moz_gtk_get_combo_box_button_inner_widgets,
                                 NULL);
        } else if(GTK_IS_ARROW(buttonChild)) {
            

            gComboBoxArrowWidget = buttonChild;
            g_object_add_weak_pointer(G_OBJECT(buttonChild), (gpointer)
                                      &gComboBoxArrowWidget);
            gtk_widget_realize(gComboBoxArrowWidget);
        }
    } else {
        


        ensure_toggle_button_widget();
        gComboBoxButtonWidget = gToggleButtonWidget;
    }

    if (!gComboBoxArrowWidget) {
        


        ensure_button_arrow_widget();
        gComboBoxArrowWidget = gButtonArrowWidget;
    }

    



    return MOZ_GTK_SUCCESS;
}











static void
moz_gtk_get_combo_box_entry_inner_widgets(GtkWidget *widget,
                                          gpointer client_data)
{
    if (GTK_IS_TOGGLE_BUTTON(widget)) {
        gComboBoxEntryButtonWidget = widget;
        g_object_add_weak_pointer(G_OBJECT(widget),
                                  (gpointer) &gComboBoxEntryButtonWidget);
    } else if (GTK_IS_ENTRY(widget)) {
        gComboBoxEntryTextareaWidget = widget;
        g_object_add_weak_pointer(G_OBJECT(widget),
                                  (gpointer) &gComboBoxEntryTextareaWidget);
    } else
        return;
    gtk_widget_realize(widget);
}

static void
moz_gtk_get_combo_box_entry_arrow(GtkWidget *widget, gpointer client_data)
{
    if (GTK_IS_ARROW(widget)) {
        gComboBoxEntryArrowWidget = widget;
        g_object_add_weak_pointer(G_OBJECT(widget),
                                  (gpointer) &gComboBoxEntryArrowWidget);
        gtk_widget_realize(widget);
    }
}

static gint
ensure_combo_box_entry_widgets()
{
    GtkWidget* buttonChild;

    if (gComboBoxEntryTextareaWidget &&
            gComboBoxEntryButtonWidget &&
            gComboBoxEntryArrowWidget)
        return MOZ_GTK_SUCCESS;

    
    if (!gComboBoxEntryWidget) {
        gComboBoxEntryWidget = gtk_combo_box_new_with_entry();
        setup_widget_prototype(gComboBoxEntryWidget);
    }

    
    gtk_container_forall(GTK_CONTAINER(gComboBoxEntryWidget),
                         moz_gtk_get_combo_box_entry_inner_widgets,
                         NULL);

    if (!gComboBoxEntryTextareaWidget) {
        ensure_entry_widget();
        gComboBoxEntryTextareaWidget = gEntryWidget;
    }

    if (gComboBoxEntryButtonWidget) {
        
        buttonChild = gtk_bin_get_child(GTK_BIN(gComboBoxEntryButtonWidget));
        if (GTK_IS_BOX(buttonChild)) {
           



            gtk_container_forall(GTK_CONTAINER(buttonChild),
                                 moz_gtk_get_combo_box_entry_arrow,
                                 NULL);
        } else if(GTK_IS_ARROW(buttonChild)) {
            

            gComboBoxEntryArrowWidget = buttonChild;
            g_object_add_weak_pointer(G_OBJECT(buttonChild), (gpointer)
                                      &gComboBoxEntryArrowWidget);
            gtk_widget_realize(gComboBoxEntryArrowWidget);
        }
    } else {
        


        ensure_toggle_button_widget();
        gComboBoxEntryButtonWidget = gToggleButtonWidget;
    }

    if (!gComboBoxEntryArrowWidget) {
        


        ensure_button_arrow_widget();
        gComboBoxEntryArrowWidget = gButtonArrowWidget;
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
ensure_toolbar_widget()
{
    if (!gToolbarWidget) {
        ensure_handlebox_widget();
        gToolbarWidget = gtk_toolbar_new();
        gtk_container_add(GTK_CONTAINER(gHandleBoxWidget), gToolbarWidget);
        gtk_widget_realize(gToolbarWidget);
    }
    return MOZ_GTK_SUCCESS;
}

static gint
ensure_toolbar_separator_widget()
{
    if (!gToolbarSeparatorWidget) {
        ensure_toolbar_widget();
        gToolbarSeparatorWidget = GTK_WIDGET(gtk_separator_tool_item_new());
        setup_widget_prototype(gToolbarSeparatorWidget);
    }
    return MOZ_GTK_SUCCESS;
}

static gint
ensure_tooltip_widget()
{
    if (!gTooltipWidget) {
        gTooltipWidget = gtk_window_new(GTK_WINDOW_POPUP);
        gtk_widget_realize(gTooltipWidget);
        moz_gtk_set_widget_name(gTooltipWidget);
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
ensure_statusbar_widget()
{
    if (!gStatusbarWidget) {
      gStatusbarWidget = gtk_statusbar_new();
      setup_widget_prototype(gStatusbarWidget);
    }
    return MOZ_GTK_SUCCESS;
}

static gint
ensure_frame_widget()
{
    if (!gFrameWidget) {
        ensure_statusbar_widget();
        gFrameWidget = gtk_frame_new(NULL);
        gtk_container_add(GTK_CONTAINER(gStatusbarWidget), gFrameWidget);
        gtk_widget_realize(gFrameWidget);
    }
    return MOZ_GTK_SUCCESS;
}

static gint
ensure_menu_bar_widget()
{
    if (!gMenuBarWidget) {
        gMenuBarWidget = gtk_menu_bar_new();
        setup_widget_prototype(gMenuBarWidget);
    }
    return MOZ_GTK_SUCCESS;
}

static gint
ensure_menu_bar_item_widget()
{
    if (!gMenuBarItemWidget) {
        ensure_menu_bar_widget();
        gMenuBarItemWidget = gtk_menu_item_new();
        gtk_menu_shell_append(GTK_MENU_SHELL(gMenuBarWidget),
                              gMenuBarItemWidget);
        gtk_widget_realize(gMenuBarItemWidget);
    }
    return MOZ_GTK_SUCCESS;
}

static gint
ensure_menu_popup_widget()
{
    if (!gMenuPopupWidget) {
        ensure_window_widget();
        gMenuPopupWidget = gtk_menu_new();
        gtk_menu_attach_to_widget(GTK_MENU(gMenuPopupWidget), gProtoWindow,
                                  NULL);
        gtk_widget_realize(gMenuPopupWidget);
    }
    return MOZ_GTK_SUCCESS;
}

static gint
ensure_menu_item_widget()
{
    if (!gMenuItemWidget) {
        ensure_menu_popup_widget();
        gMenuItemWidget = gtk_menu_item_new_with_label("M");
        gtk_menu_shell_append(GTK_MENU_SHELL(gMenuPopupWidget),
                              gMenuItemWidget);
        gtk_widget_realize(gMenuItemWidget);
    }
    return MOZ_GTK_SUCCESS;
}

static gint
ensure_image_menu_item_widget()
{
    if (!gImageMenuItemWidget) {
        ensure_menu_popup_widget();
        gImageMenuItemWidget = gtk_image_menu_item_new();
        gtk_menu_shell_append(GTK_MENU_SHELL(gMenuPopupWidget),
                              gImageMenuItemWidget);
        gtk_widget_realize(gImageMenuItemWidget);
    }
    return MOZ_GTK_SUCCESS;
}

static gint
ensure_menu_separator_widget()
{
    if (!gMenuSeparatorWidget) {
        ensure_menu_popup_widget();
        gMenuSeparatorWidget = gtk_separator_menu_item_new();
        gtk_menu_shell_append(GTK_MENU_SHELL(gMenuPopupWidget),
                              gMenuSeparatorWidget);
        gtk_widget_realize(gMenuSeparatorWidget);
    }
    return MOZ_GTK_SUCCESS;
}

static gint
ensure_check_menu_item_widget()
{
    if (!gCheckMenuItemWidget) {
        ensure_menu_popup_widget();
        gCheckMenuItemWidget = gtk_check_menu_item_new_with_label("M");
        gtk_menu_shell_append(GTK_MENU_SHELL(gMenuPopupWidget),
                              gCheckMenuItemWidget);
        gtk_widget_realize(gCheckMenuItemWidget);
    }
    return MOZ_GTK_SUCCESS;
}

static gint
ensure_tree_view_widget()
{
    if (!gTreeViewWidget) {
        gTreeViewWidget = gtk_tree_view_new();
        setup_widget_prototype(gTreeViewWidget);
    }
    return MOZ_GTK_SUCCESS;
}

static gint
ensure_tree_header_cell_widget()
{
    if(!gTreeHeaderCellWidget) {
        












        GtkTreeViewColumn* firstTreeViewColumn;
        GtkTreeViewColumn* lastTreeViewColumn;

        ensure_tree_view_widget();

        
        firstTreeViewColumn = gtk_tree_view_column_new();
        gtk_tree_view_column_set_title(firstTreeViewColumn, "M");
        gtk_tree_view_append_column(GTK_TREE_VIEW(gTreeViewWidget), firstTreeViewColumn);

        gMiddleTreeViewColumn = gtk_tree_view_column_new();
        gtk_tree_view_column_set_title(gMiddleTreeViewColumn, "M");
        gtk_tree_view_append_column(GTK_TREE_VIEW(gTreeViewWidget),
                                    gMiddleTreeViewColumn);

        lastTreeViewColumn = gtk_tree_view_column_new();
        gtk_tree_view_column_set_title(lastTreeViewColumn, "M");
        gtk_tree_view_append_column(GTK_TREE_VIEW(gTreeViewWidget), lastTreeViewColumn);

        
        gTreeHeaderCellWidget = gtk_tree_view_column_get_button(gMiddleTreeViewColumn);
        
        gTreeHeaderSortArrowWidget = gtk_button_new();
    }
    return MOZ_GTK_SUCCESS;
}

static gint
ensure_expander_widget()
{
    if (!gExpanderWidget) {
        gExpanderWidget = gtk_expander_new("M");
        setup_widget_prototype(gExpanderWidget);
    }
    return MOZ_GTK_SUCCESS;
}

static gint
ensure_scrolled_window_widget()
{
    if (!gScrolledWindowWidget) {
        gScrolledWindowWidget = gtk_scrolled_window_new(NULL, NULL);
        setup_widget_prototype(gScrolledWindowWidget);
    }
    return MOZ_GTK_SUCCESS;
}

gint
moz_gtk_init()
{
    GtkWidgetClass *entry_class;

    if (is_initialized)
        return MOZ_GTK_SUCCESS;

    is_initialized = TRUE;
    have_arrow_scaling = (gtk_major_version > 2 ||
                          (gtk_major_version == 2 && gtk_minor_version >= 12));
    if (gtk_major_version > 3 ||
       (gtk_major_version == 3 && gtk_minor_version >= 14))
        checkbox_check_state = GTK_STATE_FLAG_CHECKED;
    else
        checkbox_check_state = GTK_STATE_FLAG_ACTIVE;

    if(!gtk_check_version(3, 12, 0)) {
        ensure_tab_widget();
        gtk_widget_style_get(gTabWidget, "has-tab-gap", &notebook_has_tab_gap, NULL);
    }
    else {
        notebook_has_tab_gap = TRUE;
    }

    


    entry_class = g_type_class_ref(GTK_TYPE_ENTRY);

    return MOZ_GTK_SUCCESS;
}

gint
moz_gtk_checkbox_get_metrics(gint* indicator_size, gint* indicator_spacing)
{
    ensure_checkbox_widget();

    gtk_widget_style_get (gCheckboxWidget,
                          "indicator_size", indicator_size,
                          "indicator_spacing", indicator_spacing,
                          NULL);

    return MOZ_GTK_SUCCESS;
}

gint
moz_gtk_radio_get_metrics(gint* indicator_size, gint* indicator_spacing)
{
    ensure_radiobutton_widget();

    gtk_widget_style_get (gRadiobuttonWidget,
                          "indicator_size", indicator_size,
                          "indicator_spacing", indicator_spacing,
                          NULL);

    return MOZ_GTK_SUCCESS;
}

gint
moz_gtk_get_focus_outline_size(gint* focus_h_width, gint* focus_v_width)
{
    GtkBorder border;
    GtkBorder padding;
    GtkStyleContext *style;

    ensure_entry_widget();
    style = gtk_widget_get_style_context(gEntryWidget);

    gtk_style_context_get_border(style, 0, &border);
    gtk_style_context_get_padding(style, 0, &padding);
    *focus_h_width = border.left + padding.left;
    *focus_v_width = border.top + padding.top;
    return MOZ_GTK_SUCCESS;
}

gint
moz_gtk_menuitem_get_horizontal_padding(gint* horizontal_padding)
{
    ensure_menu_item_widget();

    gtk_widget_style_get (gMenuItemWidget,
                          "horizontal-padding", horizontal_padding,
                          NULL);

    return MOZ_GTK_SUCCESS;
}

gint
moz_gtk_checkmenuitem_get_horizontal_padding(gint* horizontal_padding)
{
    ensure_check_menu_item_widget();

    gtk_widget_style_get (gCheckMenuItemWidget,
                          "horizontal-padding", horizontal_padding,
                          NULL);

    return MOZ_GTK_SUCCESS;
}

gint
moz_gtk_button_get_default_overflow(gint* border_top, gint* border_left,
                                    gint* border_bottom, gint* border_right)
{
    GtkBorder* default_outside_border;

    ensure_button_widget();
    gtk_widget_style_get(gButtonWidget,
                         "default-outside-border", &default_outside_border,
                         NULL);

    if (default_outside_border) {
        *border_top = default_outside_border->top;
        *border_left = default_outside_border->left;
        *border_bottom = default_outside_border->bottom;
        *border_right = default_outside_border->right;
        gtk_border_free(default_outside_border);
    } else {
        *border_top = *border_left = *border_bottom = *border_right = 0;
    }
    return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_button_get_default_border(gint* border_top, gint* border_left,
                                  gint* border_bottom, gint* border_right)
{
    GtkBorder* default_border;

    ensure_button_widget();
    gtk_widget_style_get(gButtonWidget,
                         "default-border", &default_border,
                         NULL);

    if (default_border) {
        *border_top = default_border->top;
        *border_left = default_border->left;
        *border_bottom = default_border->bottom;
        *border_right = default_border->right;
        gtk_border_free(default_border);
    } else {
        
        *border_top = *border_left = *border_bottom = *border_right = 1;
    }
    return MOZ_GTK_SUCCESS;
}

gint
moz_gtk_splitter_get_metrics(gint orientation, gint* size)
{
    if (orientation == GTK_ORIENTATION_HORIZONTAL) {
        ensure_hpaned_widget();
        gtk_widget_style_get(gHPanedWidget, "handle_size", size, NULL);
    } else {
        ensure_vpaned_widget();
        gtk_widget_style_get(gVPanedWidget, "handle_size", size, NULL);
    }
    return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_button_paint(cairo_t *cr, GdkRectangle* rect,
                     GtkWidgetState* state,
                     GtkReliefStyle relief, GtkWidget* widget,
                     GtkTextDirection direction)
{
    GtkStateFlags state_flags = GetStateFlagsFromGtkWidgetState(state);
    GtkStyleContext* style = gtk_widget_get_style_context(widget);    
    gint x = rect->x, y=rect->y, width=rect->width, height=rect->height;

    gtk_widget_set_direction(widget, direction);
 
    gtk_style_context_save(style);
    gtk_style_context_set_state(style, state_flags);

    if (state->isDefault && relief == GTK_RELIEF_NORMAL) {
        
        gint default_top, default_left, default_bottom, default_right;
        moz_gtk_button_get_default_overflow(&default_top, &default_left,
                                            &default_bottom, &default_right);
        x -= default_left;
        y -= default_top;
        width += default_left + default_right;
        height += default_top + default_bottom;
        gtk_render_background(style, cr, x, y, width, height);
        gtk_render_frame(style, cr, x, y, width, height);
        moz_gtk_button_get_default_border(&default_top, &default_left,
                                          &default_bottom, &default_right);
        x += default_left;
        y += default_top;
        width -= (default_left + default_right);
        height -= (default_top + default_bottom);
    }
 
    if (relief != GTK_RELIEF_NONE || state->depressed ||
        (state_flags & GTK_STATE_FLAG_PRELIGHT)) {
        


        gtk_render_background(style, cr, x, y, width, height);
        gtk_render_frame(style, cr, x, y, width, height);
    }

    if (state->focused) {
        GtkBorder border;
        gtk_style_context_get_border(style, state_flags, &border);
        x += border.left;
        y += border.top;
        width -= (border.left + border.right);
        height -= (border.top + border.bottom);
        gtk_render_focus(style, cr, x, y, width, height);
    }
    gtk_style_context_restore(style);
    return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_toggle_paint(cairo_t *cr, GdkRectangle* rect,
                     GtkWidgetState* state,
                     gboolean selected, gboolean inconsistent,
                     gboolean isradio, GtkTextDirection direction)
{
    GtkStateFlags state_flags = GetStateFlagsFromGtkWidgetState(state);
    gint indicator_size, indicator_spacing;
    gint x, y, width, height;
    gint focus_x, focus_y, focus_width, focus_height;
    GtkWidget *w;
    GtkStyleContext *style;

    if (isradio) {
        moz_gtk_radio_get_metrics(&indicator_size, &indicator_spacing);
        w = gRadiobuttonWidget;
    } else {
        moz_gtk_checkbox_get_metrics(&indicator_size, &indicator_spacing);
        w = gCheckboxWidget;
    }

    
    
    MOZ_ASSERT(rect->width >= indicator_size,
               "GetMinimumWidgetSize was ignored");

    
    x = rect->x + (rect->width - indicator_size) / 2;
    y = rect->y + (rect->height - indicator_size) / 2;
    width = indicator_size;
    height = indicator_size;

    focus_x = x - indicator_spacing;
    focus_y = y - indicator_spacing;
    focus_width = width + 2 * indicator_spacing;
    focus_height = height + 2 * indicator_spacing;
  
    style = gtk_widget_get_style_context(w);

    gtk_widget_set_sensitive(w, !state->disabled);
    gtk_widget_set_direction(w, direction);
    gtk_style_context_save(style);

    if (selected)
        state_flags |= checkbox_check_state;

    if (inconsistent)
        state_flags |= GTK_STATE_FLAG_INCONSISTENT;

    gtk_style_context_set_state(style, state_flags);

    if (isradio) {
        gtk_style_context_add_class(style, GTK_STYLE_CLASS_RADIO);
        gtk_render_option(style, cr, x, y, width, height);
        if (state->focused) {
            gtk_render_focus(style, cr, focus_x, focus_y,
                            focus_width, focus_height);
        }
    }
    else {
       


        
        gtk_style_context_add_class(style, GTK_STYLE_CLASS_CHECK);
        gtk_toggle_button_set_inconsistent(GTK_TOGGLE_BUTTON(gCheckboxWidget), inconsistent);
        gtk_render_check(style, cr, x, y, width, height);        
        if (state->focused) {
            gtk_render_focus(style, cr, 
                             focus_x, focus_y, focus_width, focus_height);
        }
    }
    gtk_style_context_restore(style);

    return MOZ_GTK_SUCCESS;
}

static gint
calculate_button_inner_rect(GtkWidget* button, GdkRectangle* rect,
                            GdkRectangle* inner_rect,
                            GtkTextDirection direction,
                            gboolean ignore_focus)
{
    GtkStyleContext* style;
    GtkBorder border;
    GtkBorder padding = {0, 0, 0, 0};

    style = gtk_widget_get_style_context(button);

    
    gtk_style_context_get_border(style, 0, &border);
    if (!ignore_focus)
        gtk_style_context_get_padding(style, 0, &padding);

    inner_rect->x = rect->x + border.left + padding.left;
    inner_rect->y = rect->y + padding.top + border.top;
    inner_rect->width = MAX(1, rect->width - padding.left -
       padding.right - border.left * 2);
    inner_rect->height = MAX(1, rect->height - padding.top -
       padding.bottom - border.top * 2);

    return MOZ_GTK_SUCCESS;
}


static gint
calculate_arrow_rect(GtkWidget* arrow, GdkRectangle* rect,
                     GdkRectangle* arrow_rect, GtkTextDirection direction)
{
    
    gfloat arrow_scaling = 0.7;
    gfloat xalign, xpad;
    gint extent;
    gint mxpad, mypad;
    gfloat mxalign, myalign;
    GtkMisc* misc = GTK_MISC(arrow);

    if (have_arrow_scaling)
        gtk_widget_style_get(arrow, "arrow_scaling", &arrow_scaling, NULL);

    gtk_misc_get_padding(misc, &mxpad, &mypad); 
    extent = MIN((rect->width - mxpad * 2),
                 (rect->height - mypad * 2)) * arrow_scaling;

    gtk_misc_get_alignment(misc, &mxalign, &myalign);
    
    xalign = direction == GTK_TEXT_DIR_LTR ? mxalign : 1.0 - mxalign;
    xpad = mxpad + (rect->width - extent) * xalign;

    arrow_rect->x = direction == GTK_TEXT_DIR_LTR ?
                        floor(rect->x + xpad) : ceil(rect->x + xpad);
    arrow_rect->y = floor(rect->y + mypad +
                          ((rect->height - extent) * myalign));

    arrow_rect->width = arrow_rect->height = extent;

    return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_scrollbar_button_paint(cairo_t *cr, GdkRectangle* rect,
                               GtkWidgetState* state,
                               GtkScrollbarButtonFlags flags,
                               GtkTextDirection direction)
{
    GtkStateFlags state_flags = GetStateFlagsFromGtkWidgetState(state);
    GtkStateType saved_state;
    GdkRectangle arrow_rect;
    gdouble arrow_angle;
    GtkStyleContext* style;
    GtkWidget *scrollbar;
    gint arrow_displacement_x, arrow_displacement_y;
    const char* detail = (flags & MOZ_GTK_STEPPER_VERTICAL) ?
                           "vscrollbar" : "hscrollbar";

    ensure_scrollbar_widget();

    if (flags & MOZ_GTK_STEPPER_VERTICAL)
        scrollbar = gVertScrollbarWidget;
    else
        scrollbar = gHorizScrollbarWidget;

    gtk_widget_set_direction(scrollbar, direction);

    if (flags & MOZ_GTK_STEPPER_VERTICAL) {
        arrow_angle = (flags & MOZ_GTK_STEPPER_DOWN) ? ARROW_DOWN : ARROW_UP;        
    } else {
        arrow_angle = (flags & MOZ_GTK_STEPPER_DOWN) ? ARROW_RIGHT : ARROW_LEFT;        
    }

    style = gtk_widget_get_style_context(scrollbar);
  
    gtk_style_context_save(style);
    gtk_style_context_add_class(style, GTK_STYLE_CLASS_SCROLLBAR);  
    gtk_style_context_set_state(style, state_flags);
  
    gtk_render_background(style, cr, rect->x, rect->y, rect->width, rect->height);
    gtk_render_frame(style, cr, rect->x, rect->y, rect->width, rect->height);

    arrow_rect.width = rect->width / 2;
    arrow_rect.height = rect->height / 2;
    arrow_rect.x = rect->x + (rect->width - arrow_rect.width) / 2;
    arrow_rect.y = rect->y + (rect->height - arrow_rect.height) / 2;

    if (state_flags & GTK_STATE_FLAG_ACTIVE) {
        gtk_widget_style_get(scrollbar,
                             "arrow-displacement-x", &arrow_displacement_x,
                             "arrow-displacement-y", &arrow_displacement_y,
                             NULL);

        arrow_rect.x += arrow_displacement_x;
        arrow_rect.y += arrow_displacement_y;
    }
  
    gtk_render_arrow(style, cr, arrow_angle,
                     arrow_rect.x,
                     arrow_rect.y, 
                     arrow_rect.width);
  
    gtk_style_context_restore(style);

    return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_scrollbar_trough_paint(GtkThemeWidgetType widget,
                               cairo_t *cr, GdkRectangle* rect,
                               GtkWidgetState* state,
                               GtkScrollbarTrackFlags flags,
                               GtkTextDirection direction)
{
    GtkStyleContext* style;
    GtkScrollbar *scrollbar;

    ensure_scrollbar_widget();

    if (widget ==  MOZ_GTK_SCROLLBAR_TRACK_HORIZONTAL)
        scrollbar = GTK_SCROLLBAR(gHorizScrollbarWidget);
    else
        scrollbar = GTK_SCROLLBAR(gVertScrollbarWidget);

    gtk_widget_set_direction(GTK_WIDGET(scrollbar), direction);
    
    if (flags & MOZ_GTK_TRACK_OPAQUE) {
        style = gtk_widget_get_style_context(GTK_WIDGET(gProtoWindow));
        gtk_render_background(style, cr, rect->x, rect->y, rect->width, rect->height);
    }

    style = gtk_widget_get_style_context(GTK_WIDGET(scrollbar));
    gtk_style_context_save(style);
    gtk_style_context_add_class(style, GTK_STYLE_CLASS_TROUGH);

    gtk_render_background(style, cr, rect->x, rect->y, rect->width, rect->height);
    gtk_render_frame(style, cr, rect->x, rect->y, rect->width, rect->height);

    if (state->focused) {
        gtk_render_focus(style, cr,
                         rect->x, rect->y, rect->width, rect->height);
    }
    gtk_style_context_restore(style);
    return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_scrollbar_thumb_paint(GtkThemeWidgetType widget,
                              cairo_t *cr, GdkRectangle* rect,
                              GtkWidgetState* state,
                              GtkTextDirection direction)
{
    GtkStateFlags state_flags = GetStateFlagsFromGtkWidgetState(state);
    GtkStyleContext* style;
    GtkScrollbar *scrollbar;
    GtkAdjustment *adj;
    GtkBorder margin;

    ensure_scrollbar_widget();

    if (widget == MOZ_GTK_SCROLLBAR_THUMB_HORIZONTAL)
        scrollbar = GTK_SCROLLBAR(gHorizScrollbarWidget);
    else
        scrollbar = GTK_SCROLLBAR(gVertScrollbarWidget);

    gtk_widget_set_direction(GTK_WIDGET(scrollbar), direction);

    style = gtk_widget_get_style_context(GTK_WIDGET(scrollbar));
    gtk_style_context_save(style);

    gtk_style_context_add_class(style, GTK_STYLE_CLASS_SLIDER);
    gtk_style_context_set_state(style, state_flags);

    gtk_style_context_get_margin (style, state_flags, &margin);

    gtk_render_slider(style, cr,
                      rect->x + margin.left,
                      rect->y + margin.top,
                      rect->width - margin.left - margin.right,
                      rect->height - margin.top - margin.bottom,
                     (widget == MOZ_GTK_SCROLLBAR_THUMB_HORIZONTAL) ?
                     GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL);

    gtk_style_context_restore(style);

    return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_spin_paint(cairo_t *cr, GdkRectangle* rect,
                   GtkTextDirection direction)
{
    GtkStyleContext* style;

    ensure_spin_widget();
    gtk_widget_set_direction(gSpinWidget, direction);
    style = gtk_widget_get_style_context(gSpinWidget);
    gtk_style_context_save(style);
    gtk_style_context_add_class(style, GTK_STYLE_CLASS_SPINBUTTON);
    gtk_render_background(style, cr, rect->x, rect->y, rect->width, rect->height);
    gtk_render_frame(style, cr, rect->x, rect->y, rect->width, rect->height);
    gtk_style_context_restore(style);

    return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_spin_updown_paint(cairo_t *cr, GdkRectangle* rect,
                          gboolean isDown, GtkWidgetState* state,
                          GtkTextDirection direction)
{
    GdkRectangle arrow_rect;
    GtkStyleContext* style;

    ensure_spin_widget();
    style = gtk_widget_get_style_context(gSpinWidget);
    gtk_style_context_save(style);
    gtk_style_context_add_class(style, GTK_STYLE_CLASS_SPINBUTTON);
    gtk_style_context_set_state(style, GetStateFlagsFromGtkWidgetState(state));
    gtk_widget_set_direction(gSpinWidget, direction);

    gtk_render_background(style, cr, rect->x, rect->y, rect->width, rect->height);
    gtk_render_frame(style, cr, rect->x, rect->y, rect->width, rect->height);


    
    arrow_rect.width = 6;
    arrow_rect.height = 6;
    arrow_rect.x = rect->x + (rect->width - arrow_rect.width) / 2;
    arrow_rect.y = rect->y + (rect->height - arrow_rect.height) / 2;
    arrow_rect.y += isDown ? -1 : 1;

    gtk_render_arrow(style, cr, 
                    isDown ? ARROW_DOWN : ARROW_UP,
                    arrow_rect.x, arrow_rect.y,
                    arrow_rect.width);
    gtk_style_context_restore(style);
    return MOZ_GTK_SUCCESS;
}



static gint
moz_gtk_scale_paint(cairo_t *cr, GdkRectangle* rect,
                    GtkWidgetState* state,
                    GtkOrientation flags, GtkTextDirection direction)
{
  GtkStateFlags state_flags = GetStateFlagsFromGtkWidgetState(state);
  gint x = 0, y = 0;
  GtkStyleContext* style;
  GtkWidget* widget;
  GtkBorder margin;

  ensure_scale_widget();
  widget = ((flags == GTK_ORIENTATION_HORIZONTAL) ? gHScaleWidget : gVScaleWidget);
  gtk_widget_set_direction(widget, direction);

  style = gtk_widget_get_style_context(widget);
  gtk_style_context_save(style);
  gtk_style_context_add_class(style, GTK_STYLE_CLASS_SCALE);
  gtk_style_context_get_margin(style, state_flags, &margin); 

  if (flags == GTK_ORIENTATION_HORIZONTAL) {
    x = margin.left;
    y++;
  }
  else {
    x++;
    y = margin.top;
  }

  gtk_render_frame(style, cr, rect->x + x, rect->y + y,
                   rect->width - 2*x, rect->height - 2*y);

  if (state->focused)
    gtk_render_focus(style, cr, 
                    rect->x, rect->y, rect->width, rect->height);
  gtk_style_context_restore(style);
  return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_scale_thumb_paint(cairo_t *cr, GdkRectangle* rect,
                          GtkWidgetState* state,
                          GtkOrientation flags, GtkTextDirection direction)
{
  GtkStateFlags state_flags = GetStateFlagsFromGtkWidgetState(state);
  GtkStyleContext* style;
  GtkWidget* widget;
  gint thumb_width, thumb_height, x, y;

  ensure_scale_widget();
  widget = ((flags == GTK_ORIENTATION_HORIZONTAL) ? gHScaleWidget : gVScaleWidget);
  gtk_widget_set_direction(widget, direction);

  style = gtk_widget_get_style_context(widget);
  gtk_style_context_add_class(style, GTK_STYLE_CLASS_SLIDER);
  gtk_style_context_save(style);
  gtk_style_context_set_state(style, state_flags);
  

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

  gtk_render_slider(style, cr, x, y, thumb_width, thumb_height, flags);
  gtk_style_context_restore(style);
  return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_gripper_paint(cairo_t *cr, GdkRectangle* rect,
                      GtkWidgetState* state,
                      GtkTextDirection direction)
{
    GtkStyleContext* style;

    ensure_handlebox_widget();
    gtk_widget_set_direction(gHandleBoxWidget, direction);

    style = gtk_widget_get_style_context(gHandleBoxWidget);
    gtk_style_context_save(style);
    gtk_style_context_add_class(style, GTK_STYLE_CLASS_GRIP);
    gtk_style_context_set_state(style, GetStateFlagsFromGtkWidgetState(state));

    gtk_render_background(style, cr, rect->x, rect->y, rect->width, rect->height);
    gtk_render_frame(style, cr, rect->x, rect->y, rect->width, rect->height);
    gtk_style_context_restore(style);

    return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_hpaned_paint(cairo_t *cr, GdkRectangle* rect,
                     GtkWidgetState* state)
{
    GtkStyleContext* style;
    
    ensure_hpaned_widget();
    style = gtk_widget_get_style_context(gHPanedWidget);
    gtk_style_context_save(style);
    gtk_style_context_add_class(style, GTK_STYLE_CLASS_PANE_SEPARATOR);
    gtk_style_context_set_state(style, GetStateFlagsFromGtkWidgetState(state));
    gtk_render_handle(style, cr,
                      rect->x, rect->y, rect->width, rect->height);
    gtk_style_context_restore(style);

    return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_vpaned_paint(cairo_t *cr, GdkRectangle* rect,
                     GtkWidgetState* state)
{
    GtkStyleContext* style;

    ensure_vpaned_widget();
    style = gtk_widget_get_style_context(gVPanedWidget);
    gtk_style_context_save(style);
    gtk_style_context_add_class(style, GTK_STYLE_CLASS_PANE_SEPARATOR);
    gtk_style_context_set_state(style, GetStateFlagsFromGtkWidgetState(state));
    gtk_render_handle(style, cr,
                      rect->x, rect->y, rect->width, rect->height);                     
    gtk_style_context_restore(style);

    return MOZ_GTK_SUCCESS;
}


static gint
moz_gtk_entry_paint(cairo_t *cr, GdkRectangle* rect,
                    GtkWidgetState* state,
                    GtkWidget* widget, GtkTextDirection direction)
{
    gint x = rect->x, y = rect->y, width = rect->width, height = rect->height;
    GtkStyleContext* style;
    int draw_focus_outline_only = state->depressed; 

    gtk_widget_set_direction(widget, direction);

    style = gtk_widget_get_style_context(widget);

    if (draw_focus_outline_only) {
        
        gint h, v;
        moz_gtk_get_focus_outline_size(&h, &v);
        rect->x -= h;
        rect->width += 2 * h;
        rect->y -= v;
        rect->height += 2 * v;
        width = rect->width;
        height = rect->height;
    }

    




    
    gtk_widget_set_sensitive(widget, !state->disabled);

    gtk_style_context_save(style);
    gtk_style_context_add_class(style, GTK_STYLE_CLASS_ENTRY);
  
    



    if (state->focused && !state->disabled) {
        

        gtk_style_context_set_state(style, GTK_STATE_FLAG_FOCUSED);
    }

    if (state->disabled) {
        gtk_style_context_set_state(style, GTK_STATE_FLAG_INSENSITIVE);
    }

    if (!draw_focus_outline_only) {
        gtk_render_background(style, cr, x, y, width, height);
    }
    gtk_render_frame(style, cr, x, y, width, height);

    gtk_style_context_restore(style);

    return MOZ_GTK_SUCCESS;
}

static gint 
moz_gtk_treeview_paint(cairo_t *cr, GdkRectangle* rect,
                       GtkWidgetState* state,
                       GtkTextDirection direction)
{
    gint xthickness, ythickness;
    GtkStyleContext *style;
    GtkStyleContext *style_tree;
    GtkStateFlags state_flags;
    GtkBorder border;

    ensure_tree_view_widget();
    ensure_scrolled_window_widget();

    gtk_widget_set_direction(gTreeViewWidget, direction);
    gtk_widget_set_direction(gScrolledWindowWidget, direction);

    

    state_flags = state->disabled ? GTK_STATE_FLAG_INSENSITIVE : GTK_STATE_FLAG_NORMAL;

    style = gtk_widget_get_style_context(gScrolledWindowWidget);
    gtk_style_context_save(style);
    gtk_style_context_add_class(style, GTK_STYLE_CLASS_FRAME);    
    gtk_style_context_get_border(style, state_flags, &border);
    xthickness = border.left;
    ythickness = border.top;    

    style_tree = gtk_widget_get_style_context(gTreeViewWidget);
    gtk_style_context_save(style_tree);
    gtk_style_context_add_class(style_tree, GTK_STYLE_CLASS_VIEW);
    
    gtk_render_background(style_tree, cr,
                          rect->x + xthickness, rect->y + ythickness,
                          rect->width - 2 * xthickness,
                          rect->height - 2 * ythickness);
    gtk_render_frame(style, cr, 
                     rect->x, rect->y, rect->width, rect->height); 
    gtk_style_context_restore(style);
    gtk_style_context_restore(style_tree);
    return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_tree_header_cell_paint(cairo_t *cr, GdkRectangle* rect,
                               GtkWidgetState* state,
                               gboolean isSorted, GtkTextDirection direction)
{
    moz_gtk_button_paint(cr, rect, state, GTK_RELIEF_NORMAL,
                         gTreeHeaderCellWidget, direction);
    return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_tree_header_sort_arrow_paint(cairo_t *cr, GdkRectangle* rect,
                                     GtkWidgetState* state, GtkArrowType arrow_type,
                                     GtkTextDirection direction)
{
    GdkRectangle arrow_rect;
    gdouble arrow_angle;
    GtkStyleContext* style;

    ensure_tree_header_cell_widget();
    gtk_widget_set_direction(gTreeHeaderSortArrowWidget, direction);

    
    arrow_rect.width = 11;
    arrow_rect.height = 11;
    arrow_rect.x = rect->x + (rect->width - arrow_rect.width) / 2;
    arrow_rect.y = rect->y + (rect->height - arrow_rect.height) / 2;
    style = gtk_widget_get_style_context(gTreeHeaderSortArrowWidget);
    gtk_style_context_save(style);
    gtk_style_context_set_state(style, GetStateFlagsFromGtkWidgetState(state));

    switch (arrow_type) {
    case GTK_ARROW_LEFT:
        arrow_angle = ARROW_LEFT;
        break;
    case GTK_ARROW_RIGHT:
        arrow_angle = ARROW_RIGHT;
        break;
    case GTK_ARROW_DOWN:
        arrow_angle = ARROW_DOWN;
        break;
    default:
        arrow_angle = ARROW_UP;
        break;
    }
    if (arrow_type != GTK_ARROW_NONE)
        gtk_render_arrow(style, cr, arrow_angle,
                         arrow_rect.x, arrow_rect.y,
                         arrow_rect.width);
    gtk_style_context_restore(style);
    return MOZ_GTK_SUCCESS;
}



static gint
moz_gtk_treeview_expander_paint(cairo_t *cr, GdkRectangle* rect,
                                GtkWidgetState* state,
                                GtkExpanderStyle expander_state,
                                GtkTextDirection direction)
{
    GtkStyleContext *style;
    GtkStateFlags    state_flags;

    ensure_tree_view_widget();
    gtk_widget_set_direction(gTreeViewWidget, direction);

    style = gtk_widget_get_style_context(gTreeViewWidget);
    gtk_style_context_save(style);
    gtk_style_context_add_class(style, GTK_STYLE_CLASS_EXPANDER);

    

    state_flags = state->disabled ? GTK_STATE_FLAG_INSENSITIVE : GTK_STATE_FLAG_NORMAL;

    


    if (expander_state == GTK_EXPANDER_EXPANDED)
        state_flags |= GTK_STATE_FLAG_ACTIVE;
    else
        state_flags &= ~(GTK_STATE_FLAG_ACTIVE);

    gtk_style_context_set_state(style, state_flags);

    gtk_render_expander(style, cr,
                        rect->x,
                        rect->y,
                        rect->width,
                        rect->height);

    gtk_style_context_restore(style);
    return MOZ_GTK_SUCCESS;
}



static gint
moz_gtk_combo_box_paint(cairo_t *cr, GdkRectangle* rect,
                        GtkWidgetState* state,
                        gboolean ishtml, GtkTextDirection direction)
{
    GdkRectangle arrow_rect, real_arrow_rect;
    gint arrow_size, separator_width;
    gboolean wide_separators;
    GtkStyleContext* style;
    GtkRequisition arrow_req;

    ensure_combo_box_widgets();

    

    moz_gtk_button_paint(cr, rect, state, GTK_RELIEF_NORMAL,
                         gComboBoxButtonWidget, direction);

    calculate_button_inner_rect(gComboBoxButtonWidget,
                                rect, &arrow_rect, direction, ishtml);
    

    gtk_widget_get_preferred_size(gComboBoxArrowWidget, NULL, &arrow_req);
    if (direction == GTK_TEXT_DIR_LTR)
        arrow_rect.x += arrow_rect.width - arrow_req.width;
    arrow_rect.width = arrow_req.width;

    calculate_arrow_rect(gComboBoxArrowWidget,
                         &arrow_rect, &real_arrow_rect, direction);

    style = gtk_widget_get_style_context(gComboBoxArrowWidget);
    gtk_render_arrow(style, cr, ARROW_DOWN,
                     real_arrow_rect.x, real_arrow_rect.y,
                     real_arrow_rect.width);

    
    if (!gComboBoxSeparatorWidget)
        return MOZ_GTK_SUCCESS;
    style = gtk_widget_get_style_context(gComboBoxSeparatorWidget);
    gtk_widget_style_get(gComboBoxSeparatorWidget,
                         "wide-separators", &wide_separators,
                         "separator-width", &separator_width,
                         NULL);

    if (wide_separators) {
        if (direction == GTK_TEXT_DIR_LTR)
            arrow_rect.x -= separator_width;
        else
            arrow_rect.x += arrow_rect.width;
        
        gtk_render_frame(style, cr, arrow_rect.x, arrow_rect.y, separator_width, arrow_rect.height);
    } else {
        if (direction == GTK_TEXT_DIR_LTR) {
            GtkBorder padding;
            GtkStateFlags state_flags = GetStateFlagsFromGtkWidgetState(state);
            gtk_style_context_get_padding(style, state_flags, &padding);
            arrow_rect.x -= padding.left;
        }
        else
            arrow_rect.x += arrow_rect.width;
        
        gtk_render_line(style, cr, 
                        arrow_rect.x, arrow_rect.y, 
                        arrow_rect.x, arrow_rect.y + arrow_rect.height);
    }
    return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_arrow_paint(cairo_t *cr, GdkRectangle* rect,
                    GtkWidgetState* state,
                    GtkArrowType arrow_type, GtkTextDirection direction)
{
    GtkStyleContext* style;
    GtkStateFlags state_flags = GetStateFlagsFromGtkWidgetState(state);
    GdkRectangle arrow_rect;
    gdouble arrow_angle;

    ensure_button_arrow_widget();
    style = gtk_widget_get_style_context(gButtonArrowWidget);
    gtk_style_context_save(style);
    gtk_style_context_set_state(style, state_flags);
    gtk_widget_set_direction(gButtonArrowWidget, direction);

    calculate_arrow_rect(gButtonArrowWidget, rect, &arrow_rect,
                         direction);

    if (direction == GTK_TEXT_DIR_RTL) {
        arrow_type = (arrow_type == GTK_ARROW_LEFT) ?
                         GTK_ARROW_RIGHT : GTK_ARROW_LEFT;
    }
    switch (arrow_type) {
    case GTK_ARROW_LEFT:
        arrow_angle = ARROW_LEFT;
        break;
    case GTK_ARROW_RIGHT:
        arrow_angle = ARROW_RIGHT;
        break;
    case GTK_ARROW_DOWN:
        arrow_angle = ARROW_DOWN;
        break;
    default:
        arrow_angle = ARROW_UP;
        break;
    }
    if (arrow_type != GTK_ARROW_NONE)
        gtk_render_arrow(style, cr, arrow_angle,
                         arrow_rect.x, arrow_rect.y, arrow_rect.width);                    
    gtk_style_context_restore(style);
    return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_combo_box_entry_button_paint(cairo_t *cr, GdkRectangle* rect,
                                     GtkWidgetState* state,
                                     gboolean input_focus,
                                     GtkTextDirection direction)
{
    gint x_displacement, y_displacement;
    GdkRectangle arrow_rect, real_arrow_rect;
    GtkStateFlags state_flags = GetStateFlagsFromGtkWidgetState(state);
    GtkStyleContext* style;

    ensure_combo_box_entry_widgets();

    moz_gtk_button_paint(cr, rect, state, GTK_RELIEF_NORMAL,
                         gComboBoxEntryButtonWidget, direction);

    calculate_button_inner_rect(gComboBoxEntryButtonWidget,
                                rect, &arrow_rect, direction, FALSE);
    if (state_flags & GTK_STATE_FLAG_ACTIVE) {
        gtk_widget_style_get(gComboBoxEntryButtonWidget,
                             "child-displacement-x", &x_displacement,
                             "child-displacement-y", &y_displacement,
                             NULL);
        arrow_rect.x += x_displacement;
        arrow_rect.y += y_displacement;
    }

    calculate_arrow_rect(gComboBoxEntryArrowWidget,
                         &arrow_rect, &real_arrow_rect, direction);

    style = gtk_widget_get_style_context(gComboBoxEntryArrowWidget);

    gtk_render_arrow(style, cr, ARROW_DOWN,
                    real_arrow_rect.x, real_arrow_rect.y,
                    real_arrow_rect.width);

    return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_container_paint(cairo_t *cr, GdkRectangle* rect,
                        GtkWidgetState* state, 
                        gboolean isradio, GtkTextDirection direction)
{
    GtkStateFlags state_flags = GetStateFlagsFromGtkWidgetState(state);
    GtkStyleContext* style;
    GtkWidget *widget;

    if (isradio) {
        ensure_radiobutton_widget();
        widget = gRadiobuttonWidget;
    } else {
        ensure_checkbox_widget();
        widget = gCheckboxWidget;
    }
    gtk_widget_set_direction(widget, direction);

    style = gtk_widget_get_style_context(widget);
    gtk_style_context_save(style);
    gtk_style_context_set_state(style, state_flags);
  
    
    if (state_flags & GTK_STATE_FLAG_PRELIGHT) {
        gtk_render_background(style, cr,
                              rect->x, rect->y, rect->width, rect->height);
    }
  
    gtk_style_context_restore(style);
  
    return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_toggle_label_paint(cairo_t *cr, GdkRectangle* rect,
                           GtkWidgetState* state, 
                           gboolean isradio, GtkTextDirection direction)
{
    GtkStyleContext *style;
    GtkWidget *widget;

    if (!state->focused)
        return MOZ_GTK_SUCCESS;

    if (isradio) {
        ensure_radiobutton_widget();
        widget = gRadiobuttonWidget;
    } else {
        ensure_checkbox_widget();
        widget = gCheckboxWidget;
    }
    style = gtk_widget_get_style_context(widget);
    gtk_style_context_save(style);
    if (isradio) {
      gtk_style_context_add_class(style, GTK_STYLE_CLASS_RADIO);
    } else {
      gtk_style_context_add_class(style, GTK_STYLE_CLASS_CHECK);
    }
    gtk_widget_set_direction(widget, direction);

    gtk_style_context_set_state(style, GetStateFlagsFromGtkWidgetState(state));
    gtk_render_focus(style, cr,
                    rect->x, rect->y, rect->width, rect->height);
    gtk_style_context_restore(style);

    return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_toolbar_paint(cairo_t *cr, GdkRectangle* rect,
                      GtkTextDirection direction)
{
    GtkStyleContext* style;

    ensure_toolbar_widget();
    gtk_widget_set_direction(gToolbarWidget, direction);

    style = gtk_widget_get_style_context(gToolbarWidget);
    gtk_style_context_save(style);
    gtk_style_context_add_class(style, GTK_STYLE_CLASS_TOOLBAR);

    gtk_render_background(style, cr, rect->x, rect->y, rect->width, rect->height);
    gtk_render_frame(style, cr, rect->x, rect->y, rect->width, rect->height);
    gtk_style_context_restore(style);

    return MOZ_GTK_SUCCESS;
}



static gint
moz_gtk_toolbar_separator_paint(cairo_t *cr, GdkRectangle* rect,
                                GtkTextDirection direction)
{
    GtkStyleContext* style;
    gint     separator_width;
    gint     paint_width;
    gboolean wide_separators;
    
    
    const double start_fraction = 0.2;
    const double end_fraction = 0.8;

    ensure_toolbar_separator_widget();
    gtk_widget_set_direction(gToolbarSeparatorWidget, direction);

    style = gtk_widget_get_style_context(gToolbarSeparatorWidget);

    gtk_widget_style_get(gToolbarWidget,
                         "wide-separators", &wide_separators,
                         "separator-width", &separator_width,
                         NULL);

    if (wide_separators) {
        if (separator_width > rect->width)
            separator_width = rect->width;
        
        gtk_render_frame(style, cr,
                          rect->x + (rect->width - separator_width) / 2,
                          rect->y + rect->height * start_fraction,
                          separator_width,
                          rect->height * (end_fraction - start_fraction));
    } else {
        GtkBorder padding;
        gtk_style_context_get_padding(style, 0, &padding);
    
        paint_width = padding.left;
        if (paint_width > rect->width)
            paint_width = rect->width;
        
        gtk_render_line(style, cr, 
                        rect->x + (rect->width - paint_width) / 2,
                        rect->y + rect->height * start_fraction,
                        rect->x + (rect->width - paint_width) / 2,
                        rect->y + rect->height * end_fraction);
    }

    return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_tooltip_paint(cairo_t *cr, GdkRectangle* rect,
                      GtkTextDirection direction)
{
    GtkStyleContext* style;

    ensure_tooltip_widget();
    gtk_widget_set_direction(gTooltipWidget, direction);

    style = gtk_widget_get_style_context(gTooltipWidget);
    gtk_style_context_save(style);
    gtk_style_context_add_class(style, GTK_STYLE_CLASS_TOOLTIP);
    gtk_render_background(style, cr, rect->x, rect->y, rect->width, rect->height);
    gtk_render_frame(style, cr, rect->x, rect->y, rect->width, rect->height);
    gtk_style_context_restore(style);
    return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_resizer_paint(cairo_t *cr, GdkRectangle* rect,
                      GtkWidgetState* state,
                      GtkTextDirection direction)
{
    GtkStyleContext* style;

    ensure_frame_widget();
    gtk_widget_set_direction(gStatusbarWidget, GTK_TEXT_DIR_LTR);

    style = gtk_widget_get_style_context(gStatusbarWidget);
    gtk_style_context_save(style);
    gtk_style_context_add_class(style, GTK_STYLE_CLASS_GRIP);
    gtk_style_context_set_state(style, GetStateFlagsFromGtkWidgetState(state));

    
    
    cairo_save(cr);
    if (direction == GTK_TEXT_DIR_RTL) {
      cairo_matrix_t mat;
      cairo_matrix_init_translate(&mat, 2 * rect->x + rect->width, 0);
      cairo_matrix_scale(&mat, -1, 1);
      cairo_transform(cr, &mat);
    }

    gtk_render_handle(style, cr, rect->x, rect->y, rect->width, rect->height);
    cairo_restore(cr);
    gtk_style_context_restore(style);

    return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_frame_paint(cairo_t *cr, GdkRectangle* rect,
                    GtkTextDirection direction)
{
    GtkStyleContext* style;

    ensure_frame_widget();
    gtk_widget_set_direction(gFrameWidget, direction);
    style = gtk_widget_get_style_context(gFrameWidget);
    gtk_style_context_save(style);
    gtk_style_context_add_class(style, GTK_STYLE_CLASS_FRAME);

    gtk_render_frame(style, cr, rect->x, rect->y, rect->width, rect->height);
    gtk_style_context_restore(style);
    return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_progressbar_paint(cairo_t *cr, GdkRectangle* rect,
                          GtkTextDirection direction)
{
    GtkStyleContext* style;

    ensure_progress_widget();
    gtk_widget_set_direction(gProgressWidget, direction);

    style = gtk_widget_get_style_context(gProgressWidget);
    gtk_style_context_save(style);
    gtk_style_context_add_class(style, GTK_STYLE_CLASS_TROUGH);
    
    gtk_render_background(style, cr, rect->x, rect->y, rect->width, rect->height);
    gtk_render_frame(style, cr, rect->x, rect->y, rect->width, rect->height);
    gtk_style_context_restore(style);

    return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_progress_chunk_paint(cairo_t *cr, GdkRectangle* rect,
                             GtkTextDirection direction,
                             GtkThemeWidgetType widget)
{
    GtkStyleContext* style;

    ensure_progress_widget();
    gtk_widget_set_direction(gProgressWidget, direction);

    style = gtk_widget_get_style_context(gProgressWidget);
    gtk_style_context_save(style);
    gtk_style_context_add_class(style, GTK_STYLE_CLASS_PROGRESSBAR);

    if (widget == MOZ_GTK_PROGRESS_CHUNK_INDETERMINATE ||
        widget == MOZ_GTK_PROGRESS_CHUNK_VERTICAL_INDETERMINATE) {
      



      gboolean vertical = (widget == MOZ_GTK_PROGRESS_CHUNK_VERTICAL_INDETERMINATE);

      
      const gint progressSize = vertical ? rect->height : rect->width;

      

      const gint barSize = MAX(1, progressSize / 5);

      
      const gint travel = 2 * (progressSize - barSize);

      


      static const guint period = 1600;
      const gint t = PR_IntervalToMilliseconds(PR_IntervalNow()) % period;
      const gint dx = travel * t / period;

      if (vertical) {
        rect->y += (dx < travel / 2) ? dx : travel - dx;
        rect->height = barSize;
      } else {
        rect->x += (dx < travel / 2) ? dx : travel - dx;
        rect->width = barSize;
      }
    }
  
    gtk_render_background(style, cr, rect->x, rect->y, rect->width, rect->height);
    
    
    if (gtk_check_version(3, 13, 7))
      gtk_render_activity(style, cr, rect->x, rect->y, rect->width, rect->height);
    gtk_style_context_restore(style);

    return MOZ_GTK_SUCCESS;
}

gint
moz_gtk_get_tab_thickness(void)
{
    GtkBorder border;
    GtkStyleContext * style;

    ensure_tab_widget();
    if (!notebook_has_tab_gap)
      return 0; 

    style = gtk_widget_get_style_context(gTabWidget);
    gtk_style_context_add_class(style, GTK_STYLE_CLASS_NOTEBOOK);
    gtk_style_context_get_border(style, 0, &border);

    if (border.top < 2)
        return 2; 

    return border.top;
}

static void
moz_gtk_tab_prepare_style_context(GtkStyleContext *style,
                                  GtkTabFlags flags)
{  
  gtk_style_context_set_state(style, ((flags & MOZ_GTK_TAB_SELECTED) == 0) ? 
                                        GTK_STATE_FLAG_NORMAL : 
                                        GTK_STATE_FLAG_ACTIVE);
  gtk_style_context_add_region(style, GTK_STYLE_REGION_TAB, 
                                      (flags & MOZ_GTK_TAB_FIRST) ? 
                                        GTK_REGION_FIRST : 0);
  gtk_style_context_add_class(style, (flags & MOZ_GTK_TAB_BOTTOM) ? 
                                        GTK_STYLE_CLASS_BOTTOM : 
                                        GTK_STYLE_CLASS_TOP);
}


static gint
moz_gtk_tab_paint(cairo_t *cr, GdkRectangle* rect,
                  GtkWidgetState* state,
                  GtkTabFlags flags, GtkTextDirection direction)
{
    




    GtkStyleContext* style;
    GdkRectangle tabRect;
    GdkRectangle focusRect;
    GdkRectangle backRect;
    int initial_gap = 0;

    ensure_tab_widget();
    gtk_widget_set_direction(gTabWidget, direction);

    style = gtk_widget_get_style_context(gTabWidget);
    gtk_style_context_save(style);
    moz_gtk_tab_prepare_style_context(style, flags);

    tabRect = *rect;

    if (flags & MOZ_GTK_TAB_FIRST) {
        gtk_widget_style_get (gTabWidget, "initial-gap", &initial_gap, NULL);
        tabRect.width -= initial_gap;

        if (direction != GTK_TEXT_DIR_RTL) {
            tabRect.x += initial_gap;
        }
    }

    focusRect = backRect = tabRect;

    if (notebook_has_tab_gap) {
        if ((flags & MOZ_GTK_TAB_SELECTED) == 0) {
            
            gtk_render_extension(style, cr,
                                 tabRect.x, tabRect.y, tabRect.width, tabRect.height,
                                (flags & MOZ_GTK_TAB_BOTTOM) ?
                                    GTK_POS_TOP : GTK_POS_BOTTOM );
        } else {
            



































            gint gap_loffset, gap_roffset, gap_voffset, gap_height;

            
            gap_height = moz_gtk_get_tab_thickness();

            
            gap_voffset = flags & MOZ_GTK_TAB_MARGIN_MASK;
            if (gap_voffset > gap_height)
                gap_voffset = gap_height;

            
            gap_loffset = gap_roffset = 20; 
            if (flags & MOZ_GTK_TAB_FIRST) {
                if (direction == GTK_TEXT_DIR_RTL)
                    gap_roffset = initial_gap;
                else
                    gap_loffset = initial_gap;
            }

            if (flags & MOZ_GTK_TAB_BOTTOM) {
                
                focusRect.y += gap_voffset;
                focusRect.height -= gap_voffset;

                gtk_render_extension(style, cr,
                                     tabRect.x, tabRect.y + gap_voffset, tabRect.width,
                                     tabRect.height - gap_voffset, GTK_POS_TOP);

                gtk_style_context_remove_region(style, GTK_STYLE_REGION_TAB);

                backRect.y += (gap_voffset - gap_height);
                backRect.height = gap_height;

                

                gtk_render_background(style, cr, backRect.x, backRect.y,
                                     backRect.width, backRect.height);
                cairo_save(cr);
                cairo_rectangle(cr, backRect.x, backRect.y, backRect.width, backRect.height);
                cairo_clip(cr);

                gtk_render_frame_gap(style, cr,
                                     tabRect.x - gap_loffset,
                                     tabRect.y + gap_voffset - 3 * gap_height,
                                     tabRect.width + gap_loffset + gap_roffset,
                                     3 * gap_height, GTK_POS_BOTTOM,
                                     gap_loffset, gap_loffset + tabRect.width);
                cairo_restore(cr);
            } else {
                
                focusRect.height -= gap_voffset;
                gtk_render_extension(style, cr,
                                     tabRect.x, tabRect.y, tabRect.width,
                                     tabRect.height - gap_voffset, GTK_POS_BOTTOM);

                gtk_style_context_remove_region(style, GTK_STYLE_REGION_TAB);

                backRect.y += (tabRect.height - gap_voffset);
                backRect.height = gap_height;

                

                gtk_render_background(style, cr, backRect.x, backRect.y,
                                      backRect.width, backRect.height);

                cairo_save(cr);
                cairo_rectangle(cr, backRect.x, backRect.y, backRect.width, backRect.height);
                cairo_clip(cr);

                gtk_render_frame_gap(style, cr,
                                     tabRect.x - gap_loffset,
                                     tabRect.y + tabRect.height - gap_voffset,
                                     tabRect.width + gap_loffset + gap_roffset,
                                     3 * gap_height, GTK_POS_TOP,
                                     gap_loffset, gap_loffset + tabRect.width);
                cairo_restore(cr);
            }
        }
    } else {
        gtk_render_background(style, cr, tabRect.x, tabRect.y, tabRect.width, tabRect.height);
        gtk_render_frame(style, cr, tabRect.x, tabRect.y, tabRect.width, tabRect.height);
    }

    gtk_style_context_restore(style);

    if (state->focused) {
      
      GtkBorder padding;

      gtk_style_context_save(style);
      moz_gtk_tab_prepare_style_context(style, flags);

      gtk_style_context_get_padding(style, GetStateFlagsFromGtkWidgetState(state), &padding);

      focusRect.x += padding.left;
      focusRect.width -= (padding.left + padding.right);
      focusRect.y += padding.top;
      focusRect.height -= (padding.top + padding.bottom);

      gtk_render_focus(style, cr,
                      focusRect.x, focusRect.y, focusRect.width, focusRect.height);

      gtk_style_context_restore(style);
    }


    return MOZ_GTK_SUCCESS;
}


static gint
moz_gtk_tabpanels_paint(cairo_t *cr, GdkRectangle* rect,
                        GtkTextDirection direction)
{
    GtkStyleContext* style;

    ensure_tab_widget();
    gtk_widget_set_direction(gTabWidget, direction);

    style = gtk_widget_get_style_context(gTabWidget);
    gtk_style_context_save(style);

    gtk_render_background(style, cr, rect->x, rect->y, 
                          rect->width, rect->height);
    









    
    cairo_save(cr);
    cairo_rectangle(cr, rect->x, rect->y,
                    rect->x + rect->width / 2,
                    rect->y + rect->height);
    cairo_clip(cr);
    gtk_render_frame_gap(style, cr,
                         rect->x, rect->y,
                         rect->width, rect->height,
                         GTK_POS_TOP, rect->width - 1, rect->width);
    cairo_restore(cr);
    
    
    cairo_save(cr);
    cairo_rectangle(cr, rect->x + rect->width / 2, rect->y,
                    rect->x + rect->width,
                    rect->y + rect->height);
    cairo_clip(cr);
    gtk_render_frame_gap(style, cr,
                         rect->x, rect->y,
                         rect->width, rect->height,
                         GTK_POS_TOP, 0, 1);
    cairo_restore(cr);

    gtk_style_context_restore(style);
    return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_tab_scroll_arrow_paint(cairo_t *cr, GdkRectangle* rect,
                               GtkWidgetState* state,
                               GtkArrowType arrow_type,
                               GtkTextDirection direction)
{
    GtkStateFlags state_flags = GetStateFlagsFromGtkWidgetState(state);
    GtkStyleContext* style;
    gdouble arrow_angle;
    gint arrow_size = MIN(rect->width, rect->height);
    gint x = rect->x + (rect->width - arrow_size) / 2;
    gint y = rect->y + (rect->height - arrow_size) / 2;

    ensure_tab_widget();

    style = gtk_widget_get_style_context(gTabWidget);
    gtk_style_context_save(style);
    if (direction == GTK_TEXT_DIR_RTL) {
        arrow_type = (arrow_type == GTK_ARROW_LEFT) ?
                         GTK_ARROW_RIGHT : GTK_ARROW_LEFT;
    }    
    switch (arrow_type) {
    case GTK_ARROW_LEFT:
        arrow_angle = ARROW_LEFT;
        break;
    case GTK_ARROW_RIGHT:
        arrow_angle = ARROW_RIGHT;
        break;
    case GTK_ARROW_DOWN:
        arrow_angle = ARROW_DOWN;
        break;
    default:
        arrow_angle = ARROW_UP;
        break;      
    }
    if (arrow_type != GTK_ARROW_NONE)  {        
        gtk_style_context_add_class(style, GTK_STYLE_CLASS_NOTEBOOK); 
        gtk_style_context_set_state(style, state_flags);
        gtk_render_arrow(style, cr, arrow_angle,
                         x, y, arrow_size);
    }
    gtk_style_context_restore(style);
    return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_menu_bar_paint(cairo_t *cr, GdkRectangle* rect,
                       GtkTextDirection direction)
{
    GtkStyleContext* style;

    ensure_menu_bar_widget();
    gtk_widget_set_direction(gMenuBarWidget, direction);

    style = gtk_widget_get_style_context(gMenuBarWidget);
    gtk_style_context_save(style);
    gtk_style_context_add_class(style, GTK_STYLE_CLASS_MENUBAR);
    gtk_render_background(style, cr, rect->x, rect->y, rect->width, rect->height);
    gtk_render_frame(style, cr, rect->x, rect->y, rect->width, rect->height);
    gtk_style_context_restore(style);
    
    return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_menu_popup_paint(cairo_t *cr, GdkRectangle* rect,
                         GtkTextDirection direction)
{
    GtkStyleContext* style;

    ensure_menu_popup_widget();
    gtk_widget_set_direction(gMenuPopupWidget, direction);

    style = gtk_widget_get_style_context(gMenuPopupWidget);
    gtk_style_context_save(style);
    gtk_style_context_add_class(style, GTK_STYLE_CLASS_MENU);

    gtk_render_background(style, cr, rect->x, rect->y, rect->width, rect->height);
    gtk_render_frame(style, cr, rect->x, rect->y, rect->width, rect->height);
    gtk_style_context_restore(style);

    return MOZ_GTK_SUCCESS;
}


static gint
moz_gtk_menu_separator_paint(cairo_t *cr, GdkRectangle* rect,
                             GtkTextDirection direction)
{
    GtkStyleContext* style;
    gboolean wide_separators;
    gint separator_height;
    gint paint_height;
    guint border_width;
    gint x, y, w, h;
    GtkBorder padding;

    ensure_menu_separator_widget();
    gtk_widget_set_direction(gMenuSeparatorWidget, direction);

    border_width = gtk_container_get_border_width(GTK_CONTAINER(gMenuSeparatorWidget));

    style = gtk_widget_get_style_context(gMenuSeparatorWidget);
    gtk_style_context_get_padding(style, 0, &padding);

    x = rect->x + border_width;
    y = rect->y + border_width;
    w = rect->width - border_width * 2;
    h = rect->height - border_width * 2;

    gtk_style_context_save(style);
    gtk_style_context_add_class(style, GTK_STYLE_CLASS_SEPARATOR);

    gtk_widget_style_get(gMenuSeparatorWidget,
                         "wide-separators",    &wide_separators,
                         "separator-height",   &separator_height,
                         NULL);

    if (wide_separators) {
      gtk_render_frame(style, cr,
                       x + padding.left,
                       y + padding.top,
                       w - padding.left - padding.right,
                       separator_height);
    } else {
      gtk_render_line(style, cr,
                      x + padding.left,
                      y + padding.top,
                      x + w - padding.right - 1,
                      y + padding.top);
    }

    gtk_style_context_restore(style);

    return MOZ_GTK_SUCCESS;
}


static gint
moz_gtk_menu_item_paint(cairo_t *cr, GdkRectangle* rect,
                        GtkWidgetState* state,
                        gint flags, GtkTextDirection direction)
{
    GtkStyleContext* style;
    GtkWidget* item_widget;
    guint border_width;
    gint x, y, w, h;

    if (state->inHover && !state->disabled) {   
        if (flags & MOZ_TOPLEVEL_MENU_ITEM) {
            ensure_menu_bar_item_widget();
            item_widget = gMenuBarItemWidget;
        } else {
            ensure_menu_item_widget();
            item_widget = gMenuItemWidget;
        }
        style = gtk_widget_get_style_context(item_widget);
        gtk_style_context_save(style);

        if (flags & MOZ_TOPLEVEL_MENU_ITEM) {
            gtk_style_context_add_class(style, GTK_STYLE_CLASS_MENUBAR);
        }

        gtk_widget_set_direction(item_widget, direction);
        gtk_style_context_add_class(style, GTK_STYLE_CLASS_MENUITEM);
        gtk_style_context_set_state(style, GetStateFlagsFromGtkWidgetState(state));

        border_width = gtk_container_get_border_width(GTK_CONTAINER(item_widget));

        x = rect->x + border_width;
        y = rect->y + border_width;
        w = rect->width - border_width * 2;
        h = rect->height - border_width * 2;

        gtk_render_background(style, cr, x, y, w, h);
        gtk_render_frame(style, cr, x, y, w, h);
        gtk_style_context_restore(style);
    }

    return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_menu_arrow_paint(cairo_t *cr, GdkRectangle* rect,
                         GtkWidgetState* state,
                         GtkTextDirection direction)
{
    GtkStyleContext* style;
    GtkStateFlags state_flags = GetStateFlagsFromGtkWidgetState(state);

    ensure_menu_item_widget();
    gtk_widget_set_direction(gMenuItemWidget, direction);

    style = gtk_widget_get_style_context(gMenuItemWidget);
    gtk_style_context_save(style);
    gtk_style_context_add_class(style, GTK_STYLE_CLASS_MENUITEM);
    gtk_style_context_set_state(style, state_flags);
    gtk_render_arrow(style, cr,
                    (direction == GTK_TEXT_DIR_LTR) ? ARROW_RIGHT : ARROW_LEFT,
                    rect->x, rect->y, rect->width);
    gtk_style_context_restore(style);

    return MOZ_GTK_SUCCESS;
}


static gint
moz_gtk_check_menu_item_paint(cairo_t *cr, GdkRectangle* rect,
                              GtkWidgetState* state,
                              gboolean checked, gboolean isradio,
                              GtkTextDirection direction)
{
    GtkStateFlags state_flags = GetStateFlagsFromGtkWidgetState(state);
    GtkStyleContext* style;
    GtkBorder padding;
    gint offset;
    gint indicator_size, horizontal_padding;
    gint x, y;

    moz_gtk_menu_item_paint(cr, rect, state, FALSE, direction);

    ensure_check_menu_item_widget();
    gtk_widget_set_direction(gCheckMenuItemWidget, direction);

    gtk_widget_style_get (gCheckMenuItemWidget,
                          "indicator-size", &indicator_size,
                          "horizontal-padding", &horizontal_padding,
                          NULL);

    style = gtk_widget_get_style_context(gCheckMenuItemWidget);
    gtk_style_context_save(style);
    if (isradio) {
      gtk_style_context_add_class(style, GTK_STYLE_CLASS_RADIO);
    } else {
      gtk_style_context_add_class(style, GTK_STYLE_CLASS_CHECK);
    }

    if (checked) {
      state_flags |= checkbox_check_state;
    }
    
    gtk_style_context_set_state(style, state_flags);
    gtk_style_context_get_padding(style, state_flags, &padding);

    offset = gtk_container_get_border_width(GTK_CONTAINER(gCheckMenuItemWidget)) +
                                            padding.left + 2;

    if (direction == GTK_TEXT_DIR_RTL) {
        x = rect->width - indicator_size - offset - horizontal_padding;
    }
    else {
        x = rect->x + offset + horizontal_padding;
    }
    y = rect->y + (rect->height - indicator_size) / 2;

    if (isradio) {
      gtk_render_option(style, cr, x, y, indicator_size, indicator_size);
    } else {
      gtk_render_check(style, cr, x, y, indicator_size, indicator_size);
    }
    gtk_style_context_restore(style);

    return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_window_paint(cairo_t *cr, GdkRectangle* rect,
                     GtkTextDirection direction)
{
    GtkStyleContext* style;

    ensure_window_widget();
    gtk_widget_set_direction(gProtoWindow, direction);

    style = gtk_widget_get_style_context(gProtoWindow);	
    gtk_style_context_save(style);
    gtk_style_context_add_class(style, GTK_STYLE_CLASS_BACKGROUND);
    gtk_render_background(style, cr, rect->x, rect->y, rect->width, rect->height);
    gtk_style_context_restore(style);

    return MOZ_GTK_SUCCESS;
}

static void
moz_gtk_add_style_border(GtkStyleContext* style,
                         gint* left, gint* top, gint* right, gint* bottom)
{
    GtkBorder border;

    gtk_style_context_get_border(style, 0, &border);

    *left += border.left;
    *right += border.right;
    *top += border.top;
    *bottom += border.bottom;
}

static void
moz_gtk_add_style_padding(GtkStyleContext* style,
                          gint* left, gint* top, gint* right, gint* bottom)
{
    GtkBorder padding;

    gtk_style_context_get_padding(style, 0, &padding);

    *left += padding.left;
    *right += padding.right;
    *top += padding.top;
    *bottom += padding.bottom;
}

gint
moz_gtk_get_widget_border(GtkThemeWidgetType widget, gint* left, gint* top,
                          gint* right, gint* bottom, GtkTextDirection direction,
                          gboolean inhtml)
{
    GtkWidget* w;
    GtkStyleContext* style;
    *left = *top = *right = *bottom = 0;

    switch (widget) {
    case MOZ_GTK_BUTTON:
        {
            ensure_button_widget();
            style = gtk_widget_get_style_context(gButtonWidget);

            *left = *top = *right = *bottom = gtk_container_get_border_width(GTK_CONTAINER(gButtonWidget));

            

            if (!inhtml) {
                moz_gtk_add_style_padding(style, left, top, right, bottom);
            }

            moz_gtk_add_style_border(style, left, top, right, bottom);
            return MOZ_GTK_SUCCESS;
        }
    case MOZ_GTK_ENTRY:
        {
            ensure_entry_widget();
            style = gtk_widget_get_style_context(gEntryWidget);
            moz_gtk_add_style_border(style, left, top, right, bottom);

            return MOZ_GTK_SUCCESS;
        }
    case MOZ_GTK_TREEVIEW:
        {
            ensure_scrolled_window_widget();
            style = gtk_widget_get_style_context(gScrolledWindowWidget);
            gtk_style_context_save(style);
            gtk_style_context_add_class(style, GTK_STYLE_CLASS_FRAME);
            moz_gtk_add_style_border(style, left, top, right, bottom);
            gtk_style_context_restore(style);
            return MOZ_GTK_SUCCESS;
        }
    case MOZ_GTK_TREE_HEADER_CELL:
        {
            




            GtkStyleContext *style;

            ensure_tree_header_cell_widget();
            *left = *top = *right = *bottom = gtk_container_get_border_width(GTK_CONTAINER(gTreeHeaderCellWidget));

            style = gtk_widget_get_style_context(gTreeHeaderCellWidget);

            moz_gtk_add_style_border(style, left, top, right, bottom);
            moz_gtk_add_style_padding(style, left, top, right, bottom);
            return MOZ_GTK_SUCCESS;
        }
    case MOZ_GTK_TREE_HEADER_SORTARROW:
        ensure_tree_header_cell_widget();
        w = gTreeHeaderSortArrowWidget;
        break;
    case MOZ_GTK_DROPDOWN_ENTRY:
        ensure_combo_box_entry_widgets();
        w = gComboBoxEntryTextareaWidget;
        break;
    case MOZ_GTK_DROPDOWN_ARROW:
        ensure_combo_box_entry_widgets();
        w = gComboBoxEntryButtonWidget;
        break;
    case MOZ_GTK_DROPDOWN:
        {
            


            gboolean wide_separators;
            gint separator_width;
            GtkRequisition arrow_req;
            GtkBorder border;

            ensure_combo_box_widgets();

            *left = *top = *right = *bottom = 
                gtk_container_get_border_width(GTK_CONTAINER(gComboBoxButtonWidget));

            style = gtk_widget_get_style_context(gComboBoxButtonWidget);

            if (!inhtml) {
                moz_gtk_add_style_padding(style, left, top, right, bottom);
            }
          
            moz_gtk_add_style_border(style, left, top, right, bottom);

            
            separator_width = 0;
            if (gComboBoxSeparatorWidget) {
                gtk_widget_style_get(gComboBoxSeparatorWidget,
                                     "wide-separators", &wide_separators,
                                     "separator-width", &separator_width,
                                     NULL);

                if (!wide_separators) {
                    style = gtk_widget_get_style_context(gComboBoxSeparatorWidget);
                    gtk_style_context_get_border(style, 0, &border);
                    separator_width = border.left;
                }
            }

            gtk_widget_get_preferred_size(gComboBoxArrowWidget, NULL, &arrow_req);

            if (direction == GTK_TEXT_DIR_RTL)
                *left += separator_width + arrow_req.width;
            else
                *right += separator_width + arrow_req.width;

            return MOZ_GTK_SUCCESS;
        }
    case MOZ_GTK_TABPANELS:
        ensure_tab_widget();
        w = gTabWidget;
        break;
    case MOZ_GTK_PROGRESSBAR:
        ensure_progress_widget();
        w = gProgressWidget;
        break;
    case MOZ_GTK_SPINBUTTON_ENTRY:
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
        {
            if (widget == MOZ_GTK_CHECKBUTTON_CONTAINER) {
                ensure_checkbox_widget();
                w = gCheckboxWidget;
            } else {
                ensure_radiobutton_widget();
                w = gRadiobuttonWidget;
            }
            style = gtk_widget_get_style_context(w);

            *left = *top = *right = *bottom = gtk_container_get_border_width(GTK_CONTAINER(w));
            moz_gtk_add_style_border(style,
                                     left, top, right, bottom);
            moz_gtk_add_style_padding(style,
                                      left, top, right, bottom);
            return MOZ_GTK_SUCCESS;
        }
    case MOZ_GTK_MENUPOPUP:
        ensure_menu_popup_widget();
        w = gMenuPopupWidget;
        break;
    case MOZ_GTK_MENUITEM:
    case MOZ_GTK_CHECKMENUITEM:
    case MOZ_GTK_RADIOMENUITEM:
        {
            if (widget == MOZ_GTK_MENUITEM) {
                ensure_menu_item_widget();
                ensure_menu_bar_item_widget();
                w = gMenuItemWidget;
            }
            else {
                ensure_check_menu_item_widget();
                w = gCheckMenuItemWidget;
            }

            *left = *top = *right = *bottom = gtk_container_get_border_width(GTK_CONTAINER(w));
            moz_gtk_add_style_padding(gtk_widget_get_style_context(w),
                                      left, top, right, bottom);
            return MOZ_GTK_SUCCESS;
        }
    
    case MOZ_GTK_CHECKBUTTON_LABEL:
    case MOZ_GTK_RADIOBUTTON_LABEL:
    case MOZ_GTK_SPLITTER_HORIZONTAL:
    case MOZ_GTK_SPLITTER_VERTICAL:
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
    case MOZ_GTK_PROGRESS_CHUNK:
    case MOZ_GTK_PROGRESS_CHUNK_INDETERMINATE:
    case MOZ_GTK_PROGRESS_CHUNK_VERTICAL_INDETERMINATE:
    case MOZ_GTK_TREEVIEW_EXPANDER:
    case MOZ_GTK_TOOLBAR_SEPARATOR:
    case MOZ_GTK_MENUSEPARATOR:
    
    case MOZ_GTK_SPINBUTTON:
    case MOZ_GTK_TOOLTIP:
    case MOZ_GTK_WINDOW:
    case MOZ_GTK_RESIZER:
    case MOZ_GTK_MENUARROW:
    case MOZ_GTK_TOOLBARBUTTON_ARROW:
    case MOZ_GTK_TOOLBAR:
    case MOZ_GTK_MENUBAR:
    case MOZ_GTK_TAB_SCROLLARROW:
        return MOZ_GTK_SUCCESS;
    default:
        g_warning("Unsupported widget type: %d", widget);
        return MOZ_GTK_UNKNOWN_WIDGET;
    }
    
    if (w) {
      moz_gtk_add_style_border(gtk_widget_get_style_context(w), 
                               left, top, right, bottom);
    }
    return MOZ_GTK_SUCCESS;
}

gint
moz_gtk_get_tab_border(gint* left, gint* top, gint* right, gint* bottom, 
                       GtkTextDirection direction, GtkTabFlags flags)
{
    GtkStyleContext* style;    
    int tab_curvature;

    ensure_tab_widget();

    style = gtk_widget_get_style_context(gTabWidget);
    gtk_style_context_save(style);
    moz_gtk_tab_prepare_style_context(style, flags);

    *left = *top = *right = *bottom = 0;
    moz_gtk_add_style_padding(style, left, top, right, bottom);

    gtk_widget_style_get (gTabWidget, "tab-curvature", &tab_curvature, NULL);
    *left += tab_curvature;
    *right += tab_curvature;

    if (flags & MOZ_GTK_TAB_FIRST) {
      int initial_gap;
      gtk_widget_style_get (gTabWidget, "initial-gap", &initial_gap, NULL);
      if (direction == GTK_TEXT_DIR_RTL)
        *right += initial_gap;
      else
        *left += initial_gap;
    }

    gtk_style_context_restore(style);

    return MOZ_GTK_SUCCESS;
}

gint
moz_gtk_get_combo_box_entry_button_size(gint* width, gint* height)
{
    




    GtkRequisition requisition;
    ensure_combo_box_entry_widgets();

    gtk_widget_get_preferred_size(gComboBoxEntryButtonWidget, NULL, &requisition);
    *width = requisition.width;
    *height = requisition.height;

    return MOZ_GTK_SUCCESS;
}

gint
moz_gtk_get_tab_scroll_arrow_size(gint* width, gint* height)
{
    gint arrow_size;

    ensure_tab_widget();
    gtk_widget_style_get(gTabWidget,
                         "scroll-arrow-hlength", &arrow_size,
                         NULL);

    *height = *width = arrow_size;

    return MOZ_GTK_SUCCESS;
}

gint
moz_gtk_get_arrow_size(gint* width, gint* height)
{
    GtkRequisition requisition;
    ensure_button_arrow_widget();

    gtk_widget_get_preferred_size(gButtonArrowWidget, NULL, &requisition);
    *width = requisition.width;
    *height = requisition.height;

    return MOZ_GTK_SUCCESS;
}

gint
moz_gtk_get_toolbar_separator_width(gint* size)
{
    gboolean wide_separators;
    gint separator_width;
    GtkStyleContext* style;
    GtkBorder border;

    ensure_toolbar_widget();
    style = gtk_widget_get_style_context(gToolbarWidget);

    gtk_widget_style_get(gToolbarWidget,
                         "space-size", size,
                         "wide-separators",  &wide_separators,
                         "separator-width", &separator_width,
                         NULL);
    
    gtk_style_context_get_border(style, 0, &border);
    *size = MAX(*size, (wide_separators ? separator_width : border.left));
    return MOZ_GTK_SUCCESS;
}

gint
moz_gtk_get_expander_size(gint* size)
{
    ensure_expander_widget();
    gtk_widget_style_get(gExpanderWidget,
                         "expander-size", size,
                         NULL);

    return MOZ_GTK_SUCCESS;
}

gint
moz_gtk_get_treeview_expander_size(gint* size)
{
    ensure_tree_view_widget();
    gtk_widget_style_get(gTreeViewWidget,
                         "expander-size", size,
                         NULL);

    return MOZ_GTK_SUCCESS;
}


gint
moz_gtk_get_menu_separator_height(gint *size)
{
    gboolean  wide_separators;
    gint      separator_height;
    GtkBorder padding;
    GtkStyleContext* style;
    guint border_width;

    ensure_menu_separator_widget();

    border_width = gtk_container_get_border_width(GTK_CONTAINER(gMenuSeparatorWidget));

    style = gtk_widget_get_style_context(gMenuSeparatorWidget);
    gtk_style_context_get_padding(style, 0, &padding);

    gtk_style_context_save(style);
    gtk_style_context_add_class(style, GTK_STYLE_CLASS_SEPARATOR);

    gtk_widget_style_get(gMenuSeparatorWidget,
                          "wide-separators",  &wide_separators,
                          "separator-height", &separator_height,
                          NULL);

    gtk_style_context_restore(style);

    *size = padding.top + padding.bottom + border_width*2;
    *size += (wide_separators) ? separator_height : 1;

    return MOZ_GTK_SUCCESS;
}

gint
moz_gtk_get_scalethumb_metrics(GtkOrientation orient, gint* thumb_length, gint* thumb_height)
{
  GtkWidget* widget;

  ensure_scale_widget();
  widget = ((orient == GTK_ORIENTATION_HORIZONTAL) ? gHScaleWidget : gVScaleWidget);

  gtk_widget_style_get (widget,
                        "slider_length", thumb_length,
                        "slider_width", thumb_height,
                        NULL);

  return MOZ_GTK_SUCCESS;
}

gint
moz_gtk_get_scrollbar_metrics(MozGtkScrollbarMetrics *metrics)
{
    ensure_scrollbar_widget();

    gtk_widget_style_get (gHorizScrollbarWidget,
                          "slider_width", &metrics->slider_width,
                          "trough_border", &metrics->trough_border,
                          "stepper_size", &metrics->stepper_size,
                          "stepper_spacing", &metrics->stepper_spacing,
                          NULL);

    metrics->min_slider_size = 
        gtk_range_get_min_slider_size(GTK_RANGE(gHorizScrollbarWidget));

    return MOZ_GTK_SUCCESS;
}

gboolean
moz_gtk_images_in_menus()
{
    gboolean result;
    GtkSettings* settings;

    ensure_image_menu_item_widget();
    settings = gtk_widget_get_settings(gImageMenuItemWidget);

    g_object_get(settings, "gtk-menu-images", &result, NULL);
    return result;
}

gboolean
moz_gtk_images_in_buttons()
{
    gboolean result;
    GtkSettings* settings;

    ensure_button_widget();
    settings = gtk_widget_get_settings(gButtonWidget);

    g_object_get(settings, "gtk-button-images", &result, NULL);
    return result;
}


gint
moz_gtk_widget_paint(GtkThemeWidgetType widget, cairo_t *cr,
                     GdkRectangle* rect,
                     GtkWidgetState* state, gint flags,
                     GtkTextDirection direction)
{
    

    cairo_new_path(cr);

    switch (widget) {
    case MOZ_GTK_BUTTON:
        if (state->depressed) {
            ensure_toggle_button_widget();
            return moz_gtk_button_paint(cr, rect, state,
                                        (GtkReliefStyle) flags,
                                        gToggleButtonWidget, direction);
        }
        ensure_button_widget();
        return moz_gtk_button_paint(cr, rect, state,
                                    (GtkReliefStyle) flags, gButtonWidget,
                                    direction);
        break;
    case MOZ_GTK_CHECKBUTTON:
    case MOZ_GTK_RADIOBUTTON:
        return moz_gtk_toggle_paint(cr, rect, state,
                                    !!(flags & MOZ_GTK_WIDGET_CHECKED),
                                    !!(flags & MOZ_GTK_WIDGET_INCONSISTENT),
                                    (widget == MOZ_GTK_RADIOBUTTON),
                                    direction);
        break;
    case MOZ_GTK_SCROLLBAR_BUTTON:
        return moz_gtk_scrollbar_button_paint(cr, rect, state,
                                              (GtkScrollbarButtonFlags) flags,
                                              direction);
        break;
    case MOZ_GTK_SCROLLBAR_TRACK_HORIZONTAL:
    case MOZ_GTK_SCROLLBAR_TRACK_VERTICAL:
        return moz_gtk_scrollbar_trough_paint(widget, cr, rect,
                                              state,
                                              (GtkScrollbarTrackFlags) flags,
                                              direction);
        break;
    case MOZ_GTK_SCROLLBAR_THUMB_HORIZONTAL:
    case MOZ_GTK_SCROLLBAR_THUMB_VERTICAL:
        return moz_gtk_scrollbar_thumb_paint(widget, cr, rect,
                                             state, direction);
        break;
    case MOZ_GTK_SCALE_HORIZONTAL:
    case MOZ_GTK_SCALE_VERTICAL:
        return moz_gtk_scale_paint(cr, rect, state,
                                   (GtkOrientation) flags, direction);
        break;
    case MOZ_GTK_SCALE_THUMB_HORIZONTAL:
    case MOZ_GTK_SCALE_THUMB_VERTICAL:
        return moz_gtk_scale_thumb_paint(cr, rect, state,
                                         (GtkOrientation) flags, direction);
        break;
    case MOZ_GTK_SPINBUTTON:
        return moz_gtk_spin_paint(cr, rect, direction);
        break;
    case MOZ_GTK_SPINBUTTON_UP:
    case MOZ_GTK_SPINBUTTON_DOWN:
        return moz_gtk_spin_updown_paint(cr, rect,
                                         (widget == MOZ_GTK_SPINBUTTON_DOWN),
                                         state, direction);
        break;
    case MOZ_GTK_SPINBUTTON_ENTRY:
        ensure_spin_widget();
        return moz_gtk_entry_paint(cr, rect, state,
                                   gSpinWidget, direction);
        break;
    case MOZ_GTK_GRIPPER:
        return moz_gtk_gripper_paint(cr, rect, state,
                                     direction);
        break;
    case MOZ_GTK_TREEVIEW:
        return moz_gtk_treeview_paint(cr, rect, state,
                                      direction);
        break;
    case MOZ_GTK_TREE_HEADER_CELL:
        return moz_gtk_tree_header_cell_paint(cr, rect, state,
                                              flags, direction);
        break;
    case MOZ_GTK_TREE_HEADER_SORTARROW:
        return moz_gtk_tree_header_sort_arrow_paint(cr, rect, 
                                                    state,
                                                    (GtkArrowType) flags,
                                                    direction);
        break;
    case MOZ_GTK_TREEVIEW_EXPANDER:
        return moz_gtk_treeview_expander_paint(cr, rect, state,
                                               (GtkExpanderStyle) flags, direction);
        break;
    case MOZ_GTK_ENTRY:
        ensure_entry_widget();
        return moz_gtk_entry_paint(cr, rect, state,
                                   gEntryWidget, direction);
        break;
    case MOZ_GTK_DROPDOWN:
        return moz_gtk_combo_box_paint(cr, rect, state,
                                       (gboolean) flags, direction);
        break;
    case MOZ_GTK_DROPDOWN_ARROW:
        return moz_gtk_combo_box_entry_button_paint(cr, rect,
                                                    state, flags, direction);
        break;
    case MOZ_GTK_DROPDOWN_ENTRY:
        ensure_combo_box_entry_widgets();
        return moz_gtk_entry_paint(cr, rect, state,
                                   gComboBoxEntryTextareaWidget, direction);
        break;
    case MOZ_GTK_CHECKBUTTON_CONTAINER:
    case MOZ_GTK_RADIOBUTTON_CONTAINER:
        return moz_gtk_container_paint(cr, rect, state,
                                       (widget == MOZ_GTK_RADIOBUTTON_CONTAINER),
                                       direction);
        break;
    case MOZ_GTK_CHECKBUTTON_LABEL:
    case MOZ_GTK_RADIOBUTTON_LABEL:
        return moz_gtk_toggle_label_paint(cr, rect, state,
                                          (widget == MOZ_GTK_RADIOBUTTON_LABEL),
                                          direction);
        break;
    case MOZ_GTK_TOOLBAR:
        return moz_gtk_toolbar_paint(cr, rect, direction);
        break;
    case MOZ_GTK_TOOLBAR_SEPARATOR:
        return moz_gtk_toolbar_separator_paint(cr, rect,
                                               direction);
        break;
    case MOZ_GTK_TOOLTIP:
        return moz_gtk_tooltip_paint(cr, rect, direction);
        break;
    case MOZ_GTK_FRAME:
        return moz_gtk_frame_paint(cr, rect, direction);
        break;
    case MOZ_GTK_RESIZER:
        return moz_gtk_resizer_paint(cr, rect, state,
                                     direction);
        break;
    case MOZ_GTK_PROGRESSBAR:
        return moz_gtk_progressbar_paint(cr, rect, direction);
        break;
    case MOZ_GTK_PROGRESS_CHUNK:
    case MOZ_GTK_PROGRESS_CHUNK_INDETERMINATE:
    case MOZ_GTK_PROGRESS_CHUNK_VERTICAL_INDETERMINATE:
        return moz_gtk_progress_chunk_paint(cr, rect,
                                            direction, widget);
        break;
    case MOZ_GTK_TAB:
        return moz_gtk_tab_paint(cr, rect, state,
                                 (GtkTabFlags) flags, direction);
        break;
    case MOZ_GTK_TABPANELS:
        return moz_gtk_tabpanels_paint(cr, rect, direction);
        break;
    case MOZ_GTK_TAB_SCROLLARROW:
        return moz_gtk_tab_scroll_arrow_paint(cr, rect, state,
                                              (GtkArrowType) flags, direction);
        break;
    case MOZ_GTK_MENUBAR:
        return moz_gtk_menu_bar_paint(cr, rect, direction);
        break;
    case MOZ_GTK_MENUPOPUP:
        return moz_gtk_menu_popup_paint(cr, rect, direction);
        break;
    case MOZ_GTK_MENUSEPARATOR:
        return moz_gtk_menu_separator_paint(cr, rect,
                                            direction);
        break;
    case MOZ_GTK_MENUITEM:
        return moz_gtk_menu_item_paint(cr, rect, state, flags,
                                       direction);
        break;
    case MOZ_GTK_MENUARROW:
        return moz_gtk_menu_arrow_paint(cr, rect, state,
                                        direction);
        break;
    case MOZ_GTK_TOOLBARBUTTON_ARROW:
        return moz_gtk_arrow_paint(cr, rect, state,
                                   (GtkArrowType) flags, direction);
        break;
    case MOZ_GTK_CHECKMENUITEM:
    case MOZ_GTK_RADIOMENUITEM:
        return moz_gtk_check_menu_item_paint(cr, rect, state,
                                             (gboolean) flags,
                                             (widget == MOZ_GTK_RADIOMENUITEM),
                                             direction);
        break;
    case MOZ_GTK_SPLITTER_HORIZONTAL:
        return moz_gtk_vpaned_paint(cr, rect, state);
        break;
    case MOZ_GTK_SPLITTER_VERTICAL:
        return moz_gtk_hpaned_paint(cr, rect, state);
        break;
    case MOZ_GTK_WINDOW:
        return moz_gtk_window_paint(cr, rect, direction);
        break;
    default:
        g_warning("Unknown widget type: %d", widget);
    }

    return MOZ_GTK_UNKNOWN_WIDGET;
}

GtkWidget* moz_gtk_get_scrollbar_widget(void)
{
    MOZ_ASSERT(is_initialized, "Forgot to call moz_gtk_init()");
    ensure_scrollbar_widget();
    return gHorizScrollbarWidget;
}

gboolean moz_gtk_has_scrollbar_buttons(void)
{
    gboolean backward, forward, secondary_backward, secondary_forward;
    MOZ_ASSERT(is_initialized, "Forgot to call moz_gtk_init()");
    ensure_scrollbar_widget();
    gtk_widget_style_get (gHorizScrollbarWidget,
                          "has-backward-stepper", &backward,
                          "has-forward-stepper", &forward,
                          "has-secondary-backward-stepper", &secondary_backward,
                          "has-secondary-forward-stepper", &secondary_forward,
                          NULL);
    return backward | forward | secondary_forward | secondary_forward;
}

gint
moz_gtk_shutdown()
{
    GtkWidgetClass *entry_class;

    if (gTooltipWidget)
        gtk_widget_destroy(gTooltipWidget);
    
    if (gProtoWindow)
        gtk_widget_destroy(gProtoWindow);

    
    if (gTreeHeaderSortArrowWidget)
        gtk_widget_destroy(gTreeHeaderSortArrowWidget);

    gProtoWindow = NULL;
    gProtoLayout = NULL;
    gButtonWidget = NULL;
    gToggleButtonWidget = NULL;
    gButtonArrowWidget = NULL;
    gCheckboxWidget = NULL;
    gRadiobuttonWidget = NULL;
    gHorizScrollbarWidget = NULL;
    gVertScrollbarWidget = NULL;
    gSpinWidget = NULL;
    gHScaleWidget = NULL;
    gVScaleWidget = NULL;
    gEntryWidget = NULL;
    gComboBoxWidget = NULL;
    gComboBoxButtonWidget = NULL;
    gComboBoxSeparatorWidget = NULL;
    gComboBoxArrowWidget = NULL;
    gComboBoxEntryWidget = NULL;
    gComboBoxEntryButtonWidget = NULL;
    gComboBoxEntryArrowWidget = NULL;
    gComboBoxEntryTextareaWidget = NULL;
    gHandleBoxWidget = NULL;
    gToolbarWidget = NULL;
    gStatusbarWidget = NULL;
    gFrameWidget = NULL;
    gProgressWidget = NULL;
    gTabWidget = NULL;
    gTooltipWidget = NULL;
    gMenuBarWidget = NULL;
    gMenuBarItemWidget = NULL;
    gMenuPopupWidget = NULL;
    gMenuItemWidget = NULL;
    gImageMenuItemWidget = NULL;
    gCheckMenuItemWidget = NULL;
    gTreeViewWidget = NULL;
    gMiddleTreeViewColumn = NULL;
    gTreeHeaderCellWidget = NULL;
    gTreeHeaderSortArrowWidget = NULL;
    gExpanderWidget = NULL;
    gToolbarSeparatorWidget = NULL;
    gMenuSeparatorWidget = NULL;
    gHPanedWidget = NULL;
    gVPanedWidget = NULL;
    gScrolledWindowWidget = NULL;

    entry_class = g_type_class_peek(GTK_TYPE_ENTRY);
    g_type_class_unref(entry_class);

    is_initialized = FALSE;

    return MOZ_GTK_SUCCESS;
}
