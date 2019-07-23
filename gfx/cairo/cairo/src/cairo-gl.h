
































#ifndef CAIRO_GL_H
#define CAIRO_GL_H

#include "cairo.h"

#if CAIRO_HAS_GL_SURFACE

CAIRO_BEGIN_DECLS

typedef struct _cairo_gl_context cairo_gl_context_t;

cairo_public cairo_gl_context_t *
cairo_gl_context_reference (cairo_gl_context_t *context);

cairo_public void
cairo_gl_context_destroy (cairo_gl_context_t *context);

cairo_public cairo_surface_t *
cairo_gl_surface_create (cairo_gl_context_t *ctx,
			 cairo_content_t content,
			 int width, int height);

cairo_public void
cairo_gl_surface_set_size (cairo_surface_t *surface, int width, int height);

cairo_public int
cairo_gl_surface_get_width (cairo_surface_t *abstract_surface);

cairo_public int
cairo_gl_surface_get_height (cairo_surface_t *abstract_surface);

cairo_public void
cairo_gl_surface_swapbuffers (cairo_surface_t *surface);

cairo_public cairo_status_t
cairo_gl_surface_glfinish (cairo_surface_t *surface);

#if CAIRO_HAS_GLX_FUNCTIONS
#include <GL/glx.h>

cairo_public cairo_gl_context_t *
cairo_glx_context_create (Display *dpy, GLXContext gl_ctx);

cairo_public cairo_surface_t *
cairo_gl_surface_create_for_window (cairo_gl_context_t *ctx,
				    Window win,
				    int width, int height);
#endif

#if CAIRO_HAS_EAGLE_FUNCTIONS
#include <eagle.h>

cairo_public cairo_gl_context_t *
cairo_eagle_context_create (EGLDisplay display, EGLContext context);

cairo_public cairo_surface_t *
cairo_gl_surface_create_for_eagle (cairo_gl_context_t *ctx,
				   EGLSurface surface,
				   int width, int height);
#endif

CAIRO_END_DECLS

#else  
# error Cairo was not compiled with support for the GL backend
#endif 

#endif 
