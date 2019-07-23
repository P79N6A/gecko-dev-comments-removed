
























#ifndef GLITZ_AGL_H_INCLUDED
#define GLITZ_AGL_H_INCLUDED

#include <glitz.h>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#include <Carbon/Carbon.h>




void
glitz_agl_init (void);

void
glitz_agl_fini (void);




glitz_drawable_format_t *
glitz_agl_find_window_format (unsigned long                 mask,
			      const glitz_drawable_format_t *templ,
			      int                           count);

glitz_drawable_format_t *
glitz_agl_find_pbuffer_format (unsigned long                 mask,
			       const glitz_drawable_format_t *templ,
			       int                           count);



glitz_drawable_t *
glitz_agl_create_drawable_for_window (glitz_drawable_format_t *format,
				      WindowRef               window,
				      unsigned int            width,
				      unsigned int            height);

glitz_drawable_t *
glitz_agl_create_pbuffer_drawable (glitz_drawable_format_t *format,
				   unsigned int            width,
				   unsigned int            height);


#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
