
























#ifdef HAVE_CONFIG_H
#  include "../../config.h"
#endif

#include "glitz_aglint.h"

AGLPbuffer
glitz_agl_pbuffer_create (glitz_agl_thread_info_t *thread_info,
			  int                     width,
			  int                     height)
{
    AGLPbuffer pbuffer;
    glitz_gl_enum_t target;

    if (!POWER_OF_TWO (width) || !POWER_OF_TWO (height)) {
	if (thread_info->agl_feature_mask &
	    GLITZ_AGL_FEATURE_TEXTURE_RECTANGLE_MASK)
	    target = GLITZ_GL_TEXTURE_RECTANGLE;
	else
	    return (AGLPbuffer) 0;
    } else
	target = GLITZ_GL_TEXTURE_2D;

    aglCreatePBuffer (width, height, target, GLITZ_GL_RGBA, 0, &pbuffer);

    return pbuffer;
}

void
glitz_agl_pbuffer_destroy (AGLPbuffer pbuffer)
{
    aglDestroyPBuffer (pbuffer);
}
