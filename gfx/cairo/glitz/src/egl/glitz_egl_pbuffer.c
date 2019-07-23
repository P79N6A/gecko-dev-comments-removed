
























#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include "glitz_eglint.h"

EGLSurface
glitz_egl_pbuffer_create (glitz_egl_screen_info_t *screen_info,
			  EGLConfig               egl_config,
			  int                     width,
			  int                     height)
{
    if (egl_config) {
	int attributes[9];

	attributes[0] = EGL_WIDTH;
	attributes[1] = width;

	attributes[2] = EGL_HEIGHT;
	attributes[3] = height;

#if 0
	attributes[4] = EGL_LARGEST_PBUFFER;
	attributes[5] = 0;

	attributes[6] = EGL_PRESERVED_CONTENTS;
	attributes[7] = 1;
	attributes[8] = 0;
#endif

	return
	    eglCreatePbufferSurface (screen_info->display_info->egl_display,
				     egl_config, attributes);
    } else
	return (EGLSurface) 0;
}

void
glitz_egl_pbuffer_destroy (glitz_egl_screen_info_t *screen_info,
			   EGLSurface              egl_pbuffer)
{
    eglDestroySurface (screen_info->display_info->egl_display,
		       egl_pbuffer);
}
