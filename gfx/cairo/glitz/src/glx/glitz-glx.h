
























#ifndef GLITZ_GLX_H_INCLUDED
#define GLITZ_GLX_H_INCLUDED

#include <glitz.h>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>



void
glitz_glx_init (const char *gl_library);

void
glitz_glx_fini (void);




glitz_drawable_format_t *
glitz_glx_find_window_format (Display                       *display,
			      int                           screen,
			      unsigned long                 mask,
			      const glitz_drawable_format_t *templ,
			      int                           count);

glitz_drawable_format_t *
glitz_glx_find_pbuffer_format (Display                       *display,
			       int                           screen,
			       unsigned long                 mask,
			       const glitz_drawable_format_t *templ,
			       int                           count);

glitz_drawable_format_t *
glitz_glx_find_drawable_format_for_visual (Display  *display,
					   int       screen,
					   VisualID  visual_id);

XVisualInfo *
glitz_glx_get_visual_info_from_format (Display                 *display,
				       int                     screen,
				       glitz_drawable_format_t *format);




glitz_drawable_t *
glitz_glx_create_drawable_for_window (Display                 *display,
				      int                     screen,
				      glitz_drawable_format_t *format,
				      Window                  window,
				      unsigned int            width,
				      unsigned int            height);

glitz_drawable_t *
glitz_glx_create_pbuffer_drawable (Display                 *display,
				   int                     screen,
				   glitz_drawable_format_t *format,
				   unsigned int            width,
				   unsigned int            height);


#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
