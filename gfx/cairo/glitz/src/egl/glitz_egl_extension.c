
























#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include "glitz_eglint.h"

#if 0
static glitz_extension_map egl_extensions[] = {
  { 0.0, "EGL_SGIX_fbconfig", GLITZ_EGL_FEATURE_FBCONFIG_MASK },
  { 0.0, "EGL_SGIX_pbuffer", GLITZ_EGL_FEATURE_PBUFFER_MASK },
  { 0.0, "EGL_SGI_make_current_read",
    GLITZ_EGL_FEATURE_MAKE_CURRENT_READ_MASK },
  { 0.0, "EGL_ARB_multisample", GLITZ_EGL_FEATURE_MULTISAMPLE_MASK },
  { 0.0, NULL, 0 }
};
#endif

void
glitz_egl_query_extensions (glitz_egl_screen_info_t *screen_info,
                            glitz_gl_float_t        egl_version)
{
#if 0
  const char *egl_extensions_string;

  egl_extensions_string =
    eglQueryExtensionsString (screen_info->display_info->display,
                              screen_info->screen);
  
  screen_info->egl_feature_mask =
    glitz_extensions_query (egl_version,
                            egl_extensions_string,
                            egl_extensions);
  
  if (screen_info->egl_feature_mask & GLITZ_EGL_FEATURE_MULTISAMPLE_MASK) {
    const char *vendor;

    vendor = eglGetClientString (screen_info->display_info->display,
                                 EGL_VENDOR);
    
    if (vendor) {
      
      
      if (!strncmp ("NVIDIA", vendor, 6))
        screen_info->egl_feature_mask |=
          GLITZ_EGL_FEATURE_PBUFFER_MULTISAMPLE_MASK;
    }
  }
#endif
}
