





































#ifndef __MOZ_DRAWINGAREA_H__
#define __MOZ_DRAWINGAREA_H__

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include "mozcontainer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MOZ_DRAWINGAREA_TYPE            (moz_drawingarea_get_type())
#define MOZ_DRAWINGAREA(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MOZ_DRAWINGAREA_TYPE, MozDrawingarea))
#define MOZ_DRAWINGAREA_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MOZ_DRAWINGAREA_TYPE, MozDrawingareaClass))
#define IS_MOZ_DRAWINGAREA(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MOZ_DRAWINGAREA_TYPE))
#define IS_MOZ_DRAWINGAREA_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MOZ_DRAWINGAREA_TYPE))
#define MOZ_DRAWINGAREA_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MOZ_DRAWINGAREA_TYPE, MozDrawingareaClass))

#if (GTK_CHECK_VERSION(2, 12, 0) || \
    (GTK_CHECK_VERSION(2, 10, 0) && defined(MOZ_PLATFORM_HILDON)))
#define HAVE_GTK_MOTION_HINTS
#endif

typedef struct _MozDrawingarea      MozDrawingarea;
typedef struct _MozDrawingareaClass MozDrawingareaClass;

struct _MozDrawingarea
{
    GObject         parent_instance;
    

    GdkWindow      *clip_window;
    GdkWindow      *inner_window;
};

struct _MozDrawingareaClass
{
    GObjectClass parent_class;
};

GType           moz_drawingarea_get_type       (void);
MozDrawingarea *moz_drawingarea_new            (MozDrawingarea *parent,
                                                MozContainer *widget_parent,
                                                GdkVisual *visual);
void            moz_drawingarea_reparent       (MozDrawingarea *drawingarea,
                                                GdkWindow *aNewParent);
void            moz_drawingarea_move           (MozDrawingarea *drawingarea,
                                                gint x, gint y);
void            moz_drawingarea_resize         (MozDrawingarea *drawingarea,
                                                gint width, gint height);
void            moz_drawingarea_move_resize    (MozDrawingarea *drawingarea,
                                                gint x, gint y,
                                                gint width, gint height);
void            moz_drawingarea_set_visibility (MozDrawingarea *drawingarea,
                                                gboolean visibility);
void            moz_drawingarea_scroll         (MozDrawingarea *drawingarea,
                                                gint x, gint y);

#ifdef __cplusplus
}
#endif 

#endif
