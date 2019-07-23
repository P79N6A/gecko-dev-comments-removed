
























#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include "glitz_glxint.h"

static glitz_extension_map glx_extensions[] = {
    { 0.0, "GLX_EXT_visual_rating", GLITZ_GLX_FEATURE_VISUAL_RATING_MASK },
    { 1.3, "GLX_SGIX_fbconfig", GLITZ_GLX_FEATURE_FBCONFIG_MASK },
    { 1.3, "GLX_SGIX_pbuffer", GLITZ_GLX_FEATURE_PBUFFER_MASK },
    { 0.0, "GLX_SGI_make_current_read",
      GLITZ_GLX_FEATURE_MAKE_CURRENT_READ_MASK },
    { 0.0, "GLX_ARB_multisample", GLITZ_GLX_FEATURE_MULTISAMPLE_MASK },
    { 0.0, NULL, 0 }
};


static glitz_extension_map glx_client_extensions[] = {
    { 0.0, "GLX_MESA_copy_sub_buffer", GLITZ_GLX_FEATURE_COPY_SUB_BUFFER_MASK },
    { 0.0, NULL, 0 }
};

void
glitz_glx_query_extensions (glitz_glx_screen_info_t *screen_info,
			    glitz_gl_float_t        glx_version)
{
    const char *glx_extensions_string;
    const char *glx_client_extensions_string;
    const char *vendor;

    glx_extensions_string =
	glXQueryExtensionsString (screen_info->display_info->display,
				  screen_info->screen);

    glx_client_extensions_string =
	glXGetClientString (screen_info->display_info->display,
			    GLX_EXTENSIONS);

    vendor = glXGetClientString (screen_info->display_info->display,
				 GLX_VENDOR);

    if (vendor)
    {
	if (glx_version < 1.3f)
	{
	    
	    if (!strncmp ("ATI", vendor, 3))
		screen_info->glx_version = glx_version = 1.3f;
	}
    }

    screen_info->glx_feature_mask =
	glitz_extensions_query (glx_version,
				glx_extensions_string,
				glx_extensions);

    screen_info->glx_feature_mask |=
	glitz_extensions_query (glx_version,
				glx_client_extensions_string,
				glx_client_extensions);

    if (vendor)
    {
	if (screen_info->glx_feature_mask & GLITZ_GLX_FEATURE_MULTISAMPLE_MASK)
	{
	    
	    if (!strncmp ("NVIDIA", vendor, 6))
		screen_info->glx_feature_mask |=
		    GLITZ_GLX_FEATURE_PBUFFER_MULTISAMPLE_MASK;
	}
    }
}
