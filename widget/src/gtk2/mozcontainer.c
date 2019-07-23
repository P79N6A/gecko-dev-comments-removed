





































#include "mozcontainer.h"
#include <gtk/gtk.h>
#include <stdio.h>

#ifdef ACCESSIBILITY
#include <atk/atk.h>
#include "maiRedundantObjectFactory.h"
#endif 


static void moz_container_class_init          (MozContainerClass *klass);
static void moz_container_init                (MozContainer      *container);


static void moz_container_map                 (GtkWidget         *widget);
static void moz_container_unmap               (GtkWidget         *widget);
static void moz_container_realize             (GtkWidget         *widget);
static void moz_container_size_allocate       (GtkWidget         *widget,
                                               GtkAllocation     *allocation);


static void moz_container_remove      (GtkContainer      *container,
                                       GtkWidget         *child_widget);
static void moz_container_forall      (GtkContainer      *container,
                                       gboolean           include_internals,
                                       GtkCallback        callback,
                                       gpointer           callback_data);
static void moz_container_add         (GtkContainer      *container,
                                        GtkWidget        *widget);

typedef struct _MozContainerChild MozContainerChild;

struct _MozContainerChild {
    GtkWidget *widget;
    gint x;
    gint y;
};

static void moz_container_allocate_child (MozContainer      *container,
                                          MozContainerChild *child);
static MozContainerChild *
moz_container_get_child (MozContainer *container, GtkWidget *child);

static GtkContainerClass *parent_class = NULL;



GType
moz_container_get_type(void)
{
    static GType moz_container_type = 0;

    if (!moz_container_type) {
        static GTypeInfo moz_container_info = {
            sizeof(MozContainerClass), 
            NULL, 
            NULL, 
            (GClassInitFunc) moz_container_class_init, 
            NULL, 
            NULL, 
            sizeof(MozContainer), 
            0, 
            (GInstanceInitFunc) moz_container_init, 
            NULL, 
        };

        moz_container_type = g_type_register_static (GTK_TYPE_CONTAINER,
                                                     "MozContainer",
                                                     &moz_container_info, 0);
#ifdef ACCESSIBILITY
        

        atk_registry_set_factory_type(atk_get_default_registry(),
                                      moz_container_type,
                                      mai_redundant_object_factory_get_type());
#endif
    }

    return moz_container_type;
}

GtkWidget *
moz_container_new (void)
{
    MozContainer *container;

    container = g_object_new (MOZ_CONTAINER_TYPE, NULL);

    return GTK_WIDGET(container);
}

void
moz_container_put (MozContainer *container, GtkWidget *child_widget,
                   gint x, gint y)
{
    MozContainerChild *child;

    child = g_new (MozContainerChild, 1);

    child->widget = child_widget;
    child->x = x;
    child->y = y;

    


    container->children = g_list_append (container->children, child);

    

    gtk_widget_set_parent(child_widget, GTK_WIDGET(container));
}

void
moz_container_move (MozContainer *container, GtkWidget *child_widget,
                    gint x, gint y, gint width, gint height)
{
    MozContainerChild *child;
    GtkAllocation new_allocation;

    child = moz_container_get_child (container, child_widget);

    child->x = x;
    child->y = y;

    new_allocation.x = x;
    new_allocation.y = y;
    new_allocation.width = width;
    new_allocation.height = height;

    




    gtk_widget_size_allocate(child_widget, &new_allocation);
}



void
moz_container_class_init (MozContainerClass *klass)
{
    

    GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    parent_class = g_type_class_peek_parent (klass);

    widget_class->map = moz_container_map;
    widget_class->unmap = moz_container_unmap;
    widget_class->realize = moz_container_realize;
    widget_class->size_allocate = moz_container_size_allocate;

    container_class->remove = moz_container_remove;
    container_class->forall = moz_container_forall;
    container_class->add = moz_container_add;
}

void
moz_container_init (MozContainer *container)
{
    GTK_WIDGET_SET_FLAGS(container, GTK_CAN_FOCUS);
    container->container.resize_mode = GTK_RESIZE_IMMEDIATE;
    gtk_widget_set_redraw_on_allocate(GTK_WIDGET(container),
                                      FALSE);

    


    gtk_widget_set_colormap(GTK_WIDGET(container), gdk_rgb_get_colormap());
}

void
moz_container_map (GtkWidget *widget)
{
    MozContainer *container;
    GList *tmp_list;
    GtkWidget *tmp_child;

    g_return_if_fail (IS_MOZ_CONTAINER(widget));
    container = MOZ_CONTAINER (widget);

    GTK_WIDGET_SET_FLAGS (widget, GTK_MAPPED);

    tmp_list = container->children;
    while (tmp_list) {
        tmp_child = ((MozContainerChild *)tmp_list->data)->widget;
    
        if (GTK_WIDGET_VISIBLE(tmp_child)) {
            if (!GTK_WIDGET_MAPPED(tmp_child))
                gtk_widget_map(tmp_child);
        }
        tmp_list = tmp_list->next;
    }

    gdk_window_show (widget->window);
}

void
moz_container_unmap (GtkWidget *widget)
{
    g_return_if_fail (IS_MOZ_CONTAINER (widget));
  
    GTK_WIDGET_UNSET_FLAGS (widget, GTK_MAPPED);

    gdk_window_hide (widget->window);
}

void
moz_container_realize (GtkWidget *widget)
{
    GdkWindowAttr attributes;
    gint attributes_mask = 0;
    MozContainer *container;

    g_return_if_fail(IS_MOZ_CONTAINER(widget));

    container = MOZ_CONTAINER(widget);

    GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);

    

    attributes.event_mask = (gtk_widget_get_events (widget) |
                             GDK_EXPOSURE_MASK | GDK_STRUCTURE_MASK |
                             GDK_VISIBILITY_NOTIFY_MASK |
                             GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK |
                             GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
#ifdef HAVE_GTK_MOTION_HINTS
                             GDK_POINTER_MOTION_HINT_MASK |
#endif
                             GDK_POINTER_MOTION_MASK);
    attributes.x = widget->allocation.x;
    attributes.y = widget->allocation.y;
    attributes.width = widget->allocation.width;
    attributes.height = widget->allocation.height;
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.visual = gtk_widget_get_visual (widget);
    attributes.colormap = gtk_widget_get_colormap (widget);
    attributes.window_type = GDK_WINDOW_CHILD;

    attributes_mask |= GDK_WA_VISUAL | GDK_WA_COLORMAP |
        GDK_WA_X | GDK_WA_Y;

    widget->window = gdk_window_new (gtk_widget_get_parent_window (widget),
                                     &attributes, attributes_mask);
    
    gdk_window_set_user_data (widget->window, container);

    widget->style = gtk_style_attach (widget->style, widget->window);

    

    gdk_window_set_back_pixmap (widget->window, NULL, FALSE);
}

void
moz_container_size_allocate (GtkWidget     *widget,
                             GtkAllocation *allocation)
{
    MozContainer   *container;
    GList          *tmp_list;
    GtkAllocation   tmp_allocation;
    GtkRequisition  tmp_requisition;
    GtkWidget      *tmp_child;

    g_return_if_fail (IS_MOZ_CONTAINER (widget));

    






    
    container = MOZ_CONTAINER (widget);
    if (!container->children &&
        widget->allocation.x == allocation->x &&
        widget->allocation.y == allocation->y &&
        widget->allocation.width == allocation->width &&
        widget->allocation.height == allocation->height) {
        return;
    }

    widget->allocation = *allocation;

    tmp_list = container->children;

    while (tmp_list) {
        MozContainerChild *child = tmp_list->data;

        moz_container_allocate_child (container, child);

        tmp_list = tmp_list->next;
    }

    if (GTK_WIDGET_REALIZED (widget)) {
        gdk_window_move_resize(widget->window,
                               widget->allocation.x,
                               widget->allocation.y,
                               widget->allocation.width,
                               widget->allocation.height);
    }
}

void
moz_container_remove (GtkContainer *container, GtkWidget *child_widget)
{
    MozContainerChild *child;
    MozContainer *moz_container;
    GdkWindow* parent_window;

    g_return_if_fail (IS_MOZ_CONTAINER(container));
    g_return_if_fail (GTK_IS_WIDGET(child_widget));

    moz_container = MOZ_CONTAINER(container);

    child = moz_container_get_child (moz_container, child_widget);
    g_return_if_fail (child);

    









    parent_window = gtk_widget_get_parent_window(child_widget);
    if (parent_window)
        g_object_ref(parent_window);

    gtk_widget_unparent(child_widget);

    if (parent_window) {
        






        if (parent_window != GTK_WIDGET(container)->window)
            gtk_widget_set_parent_window(child_widget, parent_window);

        g_object_unref(parent_window);
    }

    moz_container->children = g_list_remove(moz_container->children, child);
    g_free(child);
}

void
moz_container_forall (GtkContainer *container, gboolean include_internals,
                      GtkCallback  callback, gpointer callback_data)
{
    MozContainer *moz_container;
    GList *tmp_list;
  
    g_return_if_fail (IS_MOZ_CONTAINER(container));
    g_return_if_fail (callback != NULL);

    moz_container = MOZ_CONTAINER(container);

    tmp_list = moz_container->children;
    while (tmp_list) {
        MozContainerChild *child;
        child = tmp_list->data;
        tmp_list = tmp_list->next;
        (* callback) (child->widget, callback_data);
    }
}

static void
moz_container_allocate_child (MozContainer *container,
                              MozContainerChild *child)
{
    GtkAllocation  allocation;
    GtkRequisition requisition;

    allocation.x = child->x;
    allocation.y = child->y;
    
    
    allocation.width = child->widget->allocation.width;
    allocation.height = child->widget->allocation.height;

    gtk_widget_size_allocate (child->widget, &allocation);
}

MozContainerChild *
moz_container_get_child (MozContainer *container, GtkWidget *child_widget)
{
    GList *tmp_list;

    tmp_list = container->children;
    while (tmp_list) {
        MozContainerChild *child;
    
        child = tmp_list->data;
        tmp_list = tmp_list->next;

        if (child->widget == child_widget)
            return child;
    }

    return NULL;
}

static void 
moz_container_add(GtkContainer *container, GtkWidget *widget)
{
    moz_container_put(MOZ_CONTAINER(container), widget, 0, 0);
}

