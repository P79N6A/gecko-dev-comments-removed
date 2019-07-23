


































#include "cairoint.h"






















void
cairo_debug_reset_static_data (void)
{
    CAIRO_MUTEX_INITIALIZE ();

    _cairo_scaled_font_map_destroy ();

    _cairo_toy_font_face_reset_static_data ();

#if CAIRO_HAS_FT_FONT
    _cairo_ft_font_reset_static_data ();
#endif

    _cairo_intern_string_reset_static_data ();

    _cairo_scaled_font_reset_static_data ();

    _cairo_pattern_reset_static_data ();

    CAIRO_MUTEX_FINALIZE ();
}
