
























#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include "glitz_eglint.h"

static glitz_egl_surface_t *
_glitz_egl_create_surface (glitz_egl_screen_info_t *screen_info,
			   glitz_egl_context_t     *context,
			   glitz_drawable_format_t *format,
			   EGLSurface              egl_surface,
			   int                     width,
			   int                     height)
{
    glitz_egl_surface_t *surface;

    surface = (glitz_egl_surface_t *) malloc (sizeof (glitz_egl_surface_t));
    if (surface == NULL)
	return NULL;

    surface->screen_info = screen_info;
    surface->context = context;
    surface->egl_surface = egl_surface;
    surface->width = width;
    surface->height = height;

    _glitz_drawable_init (&surface->base,
			  &screen_info->formats[format->id],
			  &context->backend,
			  width, height);

    if (!context->initialized) {
	glitz_egl_push_current (surface, NULL, GLITZ_CONTEXT_CURRENT);
	glitz_egl_pop_current (surface);
    }

    if (width > context->backend.max_viewport_dims[0] ||
	height > context->backend.max_viewport_dims[1]) {
	free (surface);
	return NULL;
    }

    screen_info->drawables++;

    return surface;
}

glitz_bool_t
_glitz_egl_drawable_update_size (glitz_egl_surface_t *drawable,
				 int                  width,
				 int                  height)
{
    if (drawable->egl_surface)
    {
	glitz_egl_pbuffer_destroy (drawable->screen_info,
				   drawable->egl_surface);
	drawable->egl_surface =
	    glitz_egl_pbuffer_create (drawable->screen_info,
				      drawable->context->egl_config,
				      (int) width, (int) height);
	if (!drawable->egl_surface)
	    return 0;
    }

    drawable->width  = width;
    drawable->height = height;

    return 1;
}

static glitz_drawable_t *
_glitz_egl_create_pbuffer_surface (glitz_egl_screen_info_t *screen_info,
				   glitz_drawable_format_t *format,
				   unsigned int            width,
				   unsigned int            height)
{
    glitz_egl_surface_t *surface;
    glitz_egl_context_t *context;
    EGLSurface          egl_pbuffer;

    context = glitz_egl_context_get (screen_info, format);
    if (!context)
	return NULL;

    egl_pbuffer = glitz_egl_pbuffer_create (screen_info, context->egl_config,
					    (int) width, (int) height);
    if (!egl_pbuffer)
	return NULL;

    surface = _glitz_egl_create_surface (screen_info, context, format,
					 egl_pbuffer,
					 width, height);
    if (!surface)
    {
	glitz_egl_pbuffer_destroy (screen_info, egl_pbuffer);
	return NULL;
    }

    return &surface->base;
}

glitz_drawable_t *
glitz_egl_create_pbuffer (void                    *abstract_templ,
			  glitz_drawable_format_t *format,
			  unsigned int            width,
			  unsigned int            height)
{
    glitz_egl_surface_t *templ = (glitz_egl_surface_t *) abstract_templ;

    return _glitz_egl_create_pbuffer_surface (templ->screen_info, format,
					      width, height);
}

glitz_drawable_t *
glitz_egl_create_surface (EGLDisplay              egl_display,
			  EGLScreenMESA           egl_screen,
			  glitz_drawable_format_t *format,
			  EGLSurface              egl_surface,
			  unsigned int            width,
			  unsigned int            height)
{
    glitz_egl_surface_t         *surface;
    glitz_egl_screen_info_t     *screen_info;
    glitz_egl_context_t         *context;
    glitz_int_drawable_format_t *iformat;

    screen_info = glitz_egl_screen_info_get (egl_display, egl_screen);
    if (!screen_info)
	return NULL;

    if (format->id >= screen_info->n_formats)
	return NULL;

    iformat = &screen_info->formats[format->id];
    if (!(iformat->types & GLITZ_DRAWABLE_TYPE_WINDOW_MASK))
	return NULL;

    context = glitz_egl_context_get (screen_info, format);
    if (!context)
	return NULL;

    surface = _glitz_egl_create_surface (screen_info, context, format,
					 egl_surface, width, height);
    if (!surface)
	return NULL;

    return &surface->base;
}
slim_hidden_def(glitz_egl_create_surface);

glitz_drawable_t *
glitz_egl_create_pbuffer_surface (EGLDisplay              display,
				  EGLScreenMESA           screen,
				  glitz_drawable_format_t *format,
				  unsigned int            width,
				  unsigned int            height)
{
    glitz_egl_screen_info_t     *screen_info;
    glitz_int_drawable_format_t *iformat;

    screen_info = glitz_egl_screen_info_get (display, screen);
    if (!screen_info)
	return NULL;

    if (format->id >= screen_info->n_formats)
	return NULL;

    iformat = &screen_info->formats[format->id];
    if (!(iformat->types & GLITZ_DRAWABLE_TYPE_PBUFFER_MASK))
	return NULL;

    return _glitz_egl_create_pbuffer_surface (screen_info, format,
					      width, height);
}
slim_hidden_def(glitz_egl_create_pbuffer_surface);

void
glitz_egl_destroy (void *abstract_drawable)
{
    EGLint value;
    glitz_egl_surface_t *surface = (glitz_egl_surface_t *) abstract_drawable;

    surface->screen_info->drawables--;
    if (surface->screen_info->drawables == 0) {
	



	glitz_egl_push_current (abstract_drawable, NULL,
				GLITZ_CONTEXT_CURRENT);
	glitz_program_map_fini (surface->base.backend->gl,
				&surface->screen_info->program_map);
        glitz_program_map_init (&surface->screen_info->program_map);
	glitz_egl_pop_current (abstract_drawable);
    }

    if (eglGetCurrentSurface (0) == surface->egl_surface)
	eglMakeCurrent (surface->screen_info->display_info->egl_display,
			EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

    eglQuerySurface (surface->screen_info->display_info->egl_display,
		     surface->egl_surface,
		     EGL_SURFACE_TYPE, &value);
    if (value == EGL_PBUFFER_BIT)
	glitz_egl_pbuffer_destroy (surface->screen_info, surface->egl_surface);

    free (surface);
}

glitz_bool_t
glitz_egl_swap_buffers (void *abstract_drawable)
{
    glitz_egl_surface_t *surface = (glitz_egl_surface_t *) abstract_drawable;

    eglSwapBuffers (surface->screen_info->display_info->egl_display,
		    surface->egl_surface);

    return 1;
}
