

































CAIRO_MUTEX_DECLARE (_cairo_pattern_solid_pattern_cache_lock);
CAIRO_MUTEX_DECLARE (_cairo_pattern_solid_surface_cache_lock);

CAIRO_MUTEX_DECLARE (_cairo_font_face_mutex);
CAIRO_MUTEX_DECLARE (_cairo_scaled_font_map_mutex);

#if CAIRO_HAS_FT_FONT
CAIRO_MUTEX_DECLARE (_cairo_ft_unscaled_font_map_mutex);
#endif

#if CAIRO_HAS_XLIB_SURFACE
CAIRO_MUTEX_DECLARE (_cairo_xlib_display_mutex);
#endif



#undef   CAIRO_MUTEX_DECLARE
