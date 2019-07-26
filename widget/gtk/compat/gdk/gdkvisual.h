



#ifndef GDKVISUAL_WRAPPER_H
#define GDKVISUAL_WRAPPER_H

#define gdk_visual_get_depth gdk_visual_get_depth_
#include_next <gdk/gdkvisual.h>
#undef gdk_visual_get_depth

static inline gint
gdk_visual_get_depth(GdkVisual *visual)
{
  return visual->depth;
}
#endif 
