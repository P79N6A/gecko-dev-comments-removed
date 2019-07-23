


































#include "cairoint.h"






















void
cairo_debug_reset_static_data (void)
{
#if CAIRO_HAS_XLIB_SURFACE
    _cairo_xlib_screen_reset_static_data ();
#endif

    _cairo_font_reset_static_data ();

#if CAIRO_HAS_FT_FONT
    _cairo_ft_font_reset_static_data ();
#endif
}
