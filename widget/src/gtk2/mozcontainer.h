





































#ifndef __MOZ_CONTAINER_H__
#define __MOZ_CONTAINER_H__

#include <gtk/gtkcontainer.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MOZ_CONTAINER_TYPE            (moz_container_get_type())
#define MOZ_CONTAINER(obj)            (GTK_CHECK_CAST ((obj), MOZ_CONTAINER_TYPE, MozContainer))
#define MOZ_CONTAINER_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), MOZ_CONTAINER_TYPE, MozContainerClass))
#define IS_MOZ_CONTAINER(obj)         (GTK_CHECK_TYPE ((obj), MOZ_CONTAINER_TYPE))
#define IS_MOZ_CONTAINER_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), MOZ_CONTAINER_TYPE))
#define MOZ_CONAINTER_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), MOZ_CONTAINER_TYPE, MozContainerClass))

typedef struct _MozContainer      MozContainer;
typedef struct _MozContainerClass MozContainerClass;

struct _MozContainer
{
    GtkContainer   container;
    GList         *children;
};

struct _MozContainerClass
{
    GtkContainerClass parent_class;
};

GtkType    moz_container_get_type (void);
GtkWidget *moz_container_new      (void);
void       moz_container_put      (MozContainer *container,
                                   GtkWidget    *child_widget,
                                   gint          x,
                                   gint          y);
void       moz_container_move          (MozContainer *container,
                                        GtkWidget    *child_widget,
                                        gint          x,
                                        gint          y,
                                        gint          width,
                                        gint          height);
void       moz_container_scroll_update (MozContainer *container,
                                        GtkWidget    *child_widget,
                                        gint          x,
                                        gint          y);

#ifdef __cplusplus
}
#endif 

#endif
