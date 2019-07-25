
































#ifndef CAIRO_FEATURES_H

#define CAIRO_MUTEX_DECLARE(mutex)
#endif

CAIRO_MUTEX_DECLARE (_cairo_pattern_solid_surface_cache_lock)

CAIRO_MUTEX_DECLARE (_cairo_image_solid_cache_mutex)

CAIRO_MUTEX_DECLARE (_cairo_error_mutex)
CAIRO_MUTEX_DECLARE (_cairo_toy_font_face_mutex)
CAIRO_MUTEX_DECLARE (_cairo_intern_string_mutex)
CAIRO_MUTEX_DECLARE (_cairo_scaled_font_map_mutex)
CAIRO_MUTEX_DECLARE (_cairo_scaled_glyph_page_cache_mutex)
CAIRO_MUTEX_DECLARE (_cairo_scaled_font_error_mutex)

#if CAIRO_HAS_FT_FONT
CAIRO_MUTEX_DECLARE (_cairo_ft_unscaled_font_map_mutex)
#endif

#if CAIRO_HAS_XLIB_SURFACE
CAIRO_MUTEX_DECLARE (_cairo_xlib_display_mutex)
#endif

#if CAIRO_HAS_XCB_SURFACE
CAIRO_MUTEX_DECLARE (_cairo_xcb_connections_mutex)
#endif

#if CAIRO_HAS_GL_SURFACE
CAIRO_MUTEX_DECLARE (_cairo_gl_context_mutex)
#endif

#if !defined (HAS_ATOMIC_OPS) || defined (ATOMIC_OP_NEEDS_MEMORY_BARRIER)
CAIRO_MUTEX_DECLARE (_cairo_atomic_mutex)
#endif

#if CAIRO_HAS_DRM_SURFACE
CAIRO_MUTEX_DECLARE (_cairo_drm_device_mutex)
#endif

#undef   CAIRO_MUTEX_DECLARE
