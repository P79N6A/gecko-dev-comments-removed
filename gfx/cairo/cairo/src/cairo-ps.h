



































#ifndef CAIRO_PS_H
#define CAIRO_PS_H

#include <cairo.h>

#if CAIRO_HAS_PS_SURFACE

#include <stdio.h>

CAIRO_BEGIN_DECLS



cairo_public cairo_surface_t *
cairo_ps_surface_create (const char		*filename,
			 double			 width_in_points,
			 double			 height_in_points);

cairo_public cairo_surface_t *
cairo_ps_surface_create_for_stream (cairo_write_func_t	write_func,
				    void	       *closure,
				    double		width_in_points,
				    double		height_in_points);

cairo_public void
cairo_ps_surface_set_size (cairo_surface_t	*surface,
			   double		 width_in_points,
			   double		 height_in_points);

cairo_public void
cairo_ps_surface_dsc_comment (cairo_surface_t	*surface,
			      const char	*comment);

cairo_public void
cairo_ps_surface_dsc_begin_setup (cairo_surface_t *surface);

cairo_public void
cairo_ps_surface_dsc_begin_page_setup (cairo_surface_t *surface);

CAIRO_END_DECLS

#else  
# error Cairo was not compiled with support for the ps backend
#endif 

#endif 
