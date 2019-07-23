
























#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include "glitz_glxint.h"

GLXPbuffer
glitz_glx_pbuffer_create (glitz_glx_screen_info_t *screen_info,
			  GLXFBConfig             fbconfig,
			  int                     width,
			  int                     height)
{
    Display *dpy = screen_info->display_info->display;

    if (fbconfig) {
	int attributes[9];

	attributes[0] = GLX_PBUFFER_WIDTH;
	attributes[1] = width;

	attributes[2] = GLX_PBUFFER_HEIGHT;
	attributes[3] = height;

	attributes[4] = GLX_LARGEST_PBUFFER;
	attributes[5] = 0;

	attributes[6] = GLX_PRESERVED_CONTENTS;
	attributes[7] = 1;
	attributes[8] = 0;

	return screen_info->glx.create_pbuffer (dpy, fbconfig, attributes);
    } else
	return (GLXPbuffer) 0;
}

void
glitz_glx_pbuffer_destroy (glitz_glx_screen_info_t *screen_info,
			   GLXPbuffer              pbuffer)
{
    screen_info->glx.destroy_pbuffer (screen_info->display_info->display,
				      pbuffer);
}
