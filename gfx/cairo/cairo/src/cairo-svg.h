






























#ifndef CAIRO_SVG_H
#define CAIRO_SVG_H

#include <cairo.h>

#if CAIRO_HAS_SVG_SURFACE

CAIRO_BEGIN_DECLS









typedef enum _cairo_svg_version {
    CAIRO_SVG_VERSION_1_1,
    CAIRO_SVG_VERSION_1_2
} cairo_svg_version_t;

cairo_public cairo_surface_t *
cairo_svg_surface_create (const char   *filename,
			  double	width_in_points,
			  double	height_in_points);

cairo_public cairo_surface_t *
cairo_svg_surface_create_for_stream (cairo_write_func_t	write_func,
				     void	       *closure,
				     double		width_in_points,
				     double		height_in_points);

cairo_public void
cairo_svg_surface_restrict_to_version (cairo_surface_t 		*surface,
				       cairo_svg_version_t  	 version);

cairo_public void
cairo_svg_get_versions (cairo_svg_version_t const	**versions,
                        int                      	 *num_versions);

cairo_public const char *
cairo_svg_version_to_string (cairo_svg_version_t version);

CAIRO_END_DECLS

#else  
# error Cairo was not compiled with support for the svg backend
#endif 

#endif 
