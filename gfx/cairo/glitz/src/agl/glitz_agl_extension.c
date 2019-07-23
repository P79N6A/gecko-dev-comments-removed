
























#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include "glitz_aglint.h"

static glitz_extension_map agl_extensions[] = {
    { 0.0, "GL_APPLE_pixel_buffer", GLITZ_AGL_FEATURE_PBUFFER_MASK },
    { 0.0, "GL_ARB_multisample", GLITZ_AGL_FEATURE_MULTISAMPLE_MASK },
    { 0.0, "GL_ARB_texture_rectangle",
      GLITZ_AGL_FEATURE_TEXTURE_RECTANGLE_MASK },
    { 0.0, "GL_EXT_texture_rectangle",
      GLITZ_AGL_FEATURE_TEXTURE_RECTANGLE_MASK },
    { 0.0, "GL_NV_texture_rectangle",
      GLITZ_AGL_FEATURE_TEXTURE_RECTANGLE_MASK },
    { 0.0, NULL, 0 }
};

glitz_status_t
glitz_agl_query_extensions (glitz_agl_thread_info_t *thread_info)
{
    GLint attrib[] = {
	AGL_RGBA,
	AGL_NO_RECOVERY,
	AGL_NONE
    };
    AGLPixelFormat pf;
    AGLContext context;

    thread_info->agl_feature_mask = 0;

    pf = aglChoosePixelFormat (NULL, 0, attrib);
    context = aglCreateContext (pf, NULL);

    if (context) {
	const char *gl_extensions_string;

	aglSetCurrentContext (context);

	gl_extensions_string = (const char *) glGetString (GL_EXTENSIONS);

	thread_info->agl_feature_mask =
	    glitz_extensions_query (0.0,
				    gl_extensions_string,
				    agl_extensions);

	if (thread_info->agl_feature_mask & GLITZ_AGL_FEATURE_MULTISAMPLE_MASK)
	{
	    const char *vendor;

	    vendor = glGetString (GL_VENDOR);

	    if (vendor) {

		
		if (!strncmp ("NVIDIA", vendor, 6))
		    thread_info->agl_feature_mask |=
			GLITZ_AGL_FEATURE_PBUFFER_MULTISAMPLE_MASK;
	    }
	}

	aglSetCurrentContext (NULL);
	aglDestroyContext (context);
    }

    aglDestroyPixelFormat (pf);

    return GLITZ_STATUS_SUCCESS;
}
