


































#ifndef CAIRO_SCRIPT_H
#define CAIRO_SCRIPT_H

#include "cairo.h"

#if CAIRO_HAS_SCRIPT_SURFACE

CAIRO_BEGIN_DECLS

typedef struct _cairo_script_context cairo_script_context_t;

typedef enum {
    CAIRO_SCRIPT_MODE_BINARY,
    CAIRO_SCRIPT_MODE_ASCII
} cairo_script_mode_t;

cairo_public cairo_script_context_t *
cairo_script_context_create (const char *filename);

cairo_public cairo_script_context_t *
cairo_script_context_create_for_stream (cairo_write_func_t	 write_func,
					void			*closure);

cairo_public void
cairo_script_context_write_comment (cairo_script_context_t *context,
				    const char *comment,
				    int len);

cairo_public void
cairo_script_context_set_mode (cairo_script_context_t *context,
			       cairo_script_mode_t mode);

cairo_public cairo_script_mode_t
cairo_script_context_get_mode (cairo_script_context_t *context);

cairo_public void
cairo_script_context_destroy (cairo_script_context_t *context);

cairo_public cairo_surface_t *
cairo_script_surface_create (cairo_script_context_t *context,
			     cairo_content_t content,
			     double width,
			     double height);

cairo_public cairo_surface_t *
cairo_script_surface_create_for_target (cairo_script_context_t *context,
					cairo_surface_t *target);

cairo_public cairo_status_t
cairo_script_from_recording_surface (cairo_script_context_t *context,
				     cairo_surface_t        *recording_surface);

CAIRO_END_DECLS

#else  
# error Cairo was not compiled with support for the CairoScript backend
#endif 

#endif 
