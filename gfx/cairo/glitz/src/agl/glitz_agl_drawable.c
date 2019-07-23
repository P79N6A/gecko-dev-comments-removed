
























#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include "glitz_aglint.h"

static glitz_agl_drawable_t *
_glitz_agl_create_drawable (glitz_agl_thread_info_t *thread_info,
			    glitz_agl_context_t     *context,
			    glitz_drawable_format_t *format,
			    AGLDrawable             agl_drawable,
			    AGLPbuffer              agl_pbuffer,
			    unsigned int            width,
			    unsigned int            height)
{
    glitz_agl_drawable_t *drawable;

    drawable = (glitz_agl_drawable_t *) malloc (sizeof (glitz_agl_drawable_t));
    if (drawable == NULL)
	return NULL;

    drawable->thread_info = thread_info;
    drawable->context = context;
    drawable->drawable = agl_drawable;
    drawable->pbuffer = agl_pbuffer;
    drawable->width = width;
    drawable->height = height;

    _glitz_drawable_init (&drawable->base,
			  &thread_info->formats[format->id],
			  &context->backend,
			  width, height);

    if (!context->initialized) {
	glitz_agl_push_current (drawable, NULL, GLITZ_CONTEXT_CURRENT, NULL);
	glitz_agl_pop_current (drawable);
    }

    if (width > context->backend.max_viewport_dims[0] ||
	height > context->backend.max_viewport_dims[1]) {
	free (drawable);
	return NULL;
    }

    thread_info->drawables++;

    return drawable;
}

glitz_bool_t
glitz_agl_drawable_update_size (glitz_agl_drawable_t *drawable,
				int                  width,
				int                  height)
{
    if (drawable->pbuffer)
    {
	glitz_agl_pbuffer_destroy (drawable->pbuffer);
	drawable->pbuffer =
	    glitz_agl_pbuffer_create (drawable->thread_info,
				      (int) width, (int) height);
	if (!drawable->pbuffer)
	    return 0;
    }

    drawable->width  = width;
    drawable->height = height;

    return 1;
}

static glitz_drawable_t *
_glitz_agl_create_pbuffer_drawable (glitz_agl_thread_info_t *thread_info,
				    glitz_drawable_format_t *format,
				    unsigned int            width,
				    unsigned int            height)
{
    glitz_agl_drawable_t *drawable;
    glitz_agl_context_t *context;
    AGLPbuffer pbuffer;

    context = glitz_agl_context_get (thread_info, format);
    if (!context)
	return NULL;

    pbuffer = glitz_agl_pbuffer_create (thread_info,
					(int) width, (int) height);
    if (!pbuffer)
	return NULL;

    drawable = _glitz_agl_create_drawable (thread_info, context, format,
					   (AGLDrawable) 0, pbuffer,
					   width, height);
    if (!drawable) {
	glitz_agl_pbuffer_destroy (pbuffer);
	return NULL;
    }

    return &drawable->base;
}

glitz_drawable_t *
glitz_agl_create_pbuffer (void                    *abstract_templ,
			  glitz_drawable_format_t *format,
			  unsigned int            width,
			  unsigned int            height)
{
    glitz_agl_drawable_t *templ = (glitz_agl_drawable_t *) abstract_templ;

    return _glitz_agl_create_pbuffer_drawable (templ->thread_info, format,
					       width, height);
}

glitz_drawable_t *
glitz_agl_create_drawable_for_window (glitz_drawable_format_t *format,
				      WindowRef               window,
				      unsigned int            width,
				      unsigned int            height)
{
    glitz_agl_drawable_t *drawable;
    glitz_agl_thread_info_t *thread_info;
    glitz_agl_context_t *context;
    AGLDrawable agl_drawable;

    agl_drawable = (AGLDrawable) GetWindowPort (window);
    if (!agl_drawable)
	return NULL;

    thread_info = glitz_agl_thread_info_get ();
    if (!thread_info)
	return NULL;

    if (format->id >= thread_info->n_formats)
	return NULL;

    context = glitz_agl_context_get (thread_info, format);
    if (!context)
	return NULL;

    drawable = _glitz_agl_create_drawable (thread_info, context, format,
					   agl_drawable, (AGLPbuffer) 0,
					   width, height);
    if (!drawable)
	return NULL;

    return &drawable->base;
}
slim_hidden_def(glitz_agl_create_drawable_for_window);

glitz_drawable_t *
glitz_agl_create_pbuffer_drawable (glitz_drawable_format_t *format,
				   unsigned int            width,
				   unsigned int            height)
{
    glitz_agl_thread_info_t *thread_info;

    thread_info = glitz_agl_thread_info_get ();
    if (!thread_info)
	return NULL;

    if (format->id >= thread_info->n_formats)
	return NULL;

    return _glitz_agl_create_pbuffer_drawable (thread_info, format,
					       width, height);
}
slim_hidden_def(glitz_agl_create_pbuffer_drawable);

void
glitz_agl_destroy (void *abstract_drawable)
{
    glitz_agl_drawable_t *drawable = (glitz_agl_drawable_t *)
	abstract_drawable;

    drawable->thread_info->drawables--;
    if (drawable->thread_info->drawables == 0) {
	



	glitz_agl_push_current (abstract_drawable, NULL,
				GLITZ_CONTEXT_CURRENT, NULL);
	glitz_program_map_fini (drawable->base.backend->gl,
				&drawable->thread_info->program_map);
	glitz_program_map_init (&drawable->thread_info->program_map);
	glitz_agl_pop_current (abstract_drawable);
    }

    if (drawable->drawable || drawable->pbuffer) {
	AGLContext context = aglGetCurrentContext ();

	if (context == drawable->context->context) {
	    if (drawable->pbuffer) {
		AGLPbuffer pbuffer;
		GLint unused;

		aglGetPBuffer (context, &pbuffer, &unused, &unused, &unused);

		if (pbuffer == drawable->pbuffer)
		    aglSetCurrentContext (NULL);
	    } else {
		if (aglGetDrawable (context) == drawable->drawable)
		    aglSetCurrentContext (NULL);
	    }
	}

	if (drawable->pbuffer)
	    glitz_agl_pbuffer_destroy (drawable->pbuffer);
    }

    free (drawable);
}

glitz_bool_t
glitz_agl_swap_buffers (void *abstract_drawable)
{
    glitz_agl_drawable_t *drawable = (glitz_agl_drawable_t *)
	abstract_drawable;

    glitz_agl_push_current (abstract_drawable, NULL, GLITZ_DRAWABLE_CURRENT,
			    NULL);
    aglSwapBuffers (drawable->context->context);
    glitz_agl_pop_current (abstract_drawable);

    return 1;
}

glitz_bool_t
glitz_agl_copy_sub_buffer (void *abstract_drawable,
			   int  x,
			   int  y,
			   int  width,
			   int  height)
{
    return 0;
}
