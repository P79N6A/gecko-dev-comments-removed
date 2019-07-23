


































#include "cairoint.h"






















void
cairo_debug_reset_static_data (void)
{
    _cairo_font_reset_static_data ();

#if CAIRO_HAS_FT_FONT
    _cairo_ft_font_reset_static_data ();
#endif

    _cairo_pattern_reset_static_data ();
}
