


































#ifndef CAIRO_DEBUG_H
#define CAIRO_DEBUG_H

#include <cairo-features.h>
#include <stdio.h>

CAIRO_BEGIN_DECLS

struct _cairo_path_fixed;
struct _cairo_traps;
struct _cairo_trapezoid;
struct _cairo_clip;

cairo_public void
cairo_debug_reset_static_data (void);

cairo_public void
cairo_debug_dump_clip (struct _cairo_clip *clip,
                       FILE *fp);
cairo_public void
cairo_debug_dump_path (struct _cairo_path_fixed *path,
                       FILE *fp);

cairo_public void
cairo_debug_dump_traps (struct _cairo_traps *traps,
                        FILE *fp);

cairo_public void
cairo_debug_dump_trapezoid_array (struct _cairo_trapezoid *traps,
                                  int num_traps,
                                  FILE *fp);

CAIRO_END_DECLS

#endif 
