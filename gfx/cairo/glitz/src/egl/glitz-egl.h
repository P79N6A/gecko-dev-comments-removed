
























#ifndef GLITZ_EGL_H_INCLUDED
#define GLITZ_EGL_H_INCLUDED

#include <GL/gl.h>
#include <GLES/egl.h>

#include <glitz.h>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif



void
glitz_egl_init (const char *gl_library);

void
glitz_egl_fini (void);




glitz_drawable_format_t *
glitz_egl_find_window_config (EGLDisplay                    egl_display,
			      EGLScreenMESA                 egl_screen,
			      unsigned long                 mask,
			      const glitz_drawable_format_t *templ,
			      int                           count);

glitz_drawable_format_t *
glitz_egl_find_pbuffer_config (EGLDisplay                    egl_display,
			       EGLScreenMESA                 egl_screen,
			       unsigned long                 mask,
			       const glitz_drawable_format_t *templ,
			       int                           count);




glitz_drawable_t *
glitz_egl_create_surface (EGLDisplay              egl_display,
			  EGLScreenMESA           egl_screen,
			  glitz_drawable_format_t *format,
			  EGLSurface              egl_surface,
			  unsigned int            width,
			  unsigned int            height);

glitz_drawable_t *
glitz_egl_create_pbuffer_surface (EGLDisplay               egl_display,
				  EGLScreenMESA           egl_screen,
				  glitz_drawable_format_t *format,
				  unsigned int            width,
				  unsigned int            height);


#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
