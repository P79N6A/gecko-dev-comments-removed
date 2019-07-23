



































#ifndef CAIRO_PDF_H
#define CAIRO_PDF_H

#include <cairo.h>

#if CAIRO_HAS_PDF_SURFACE

CAIRO_BEGIN_DECLS

cairo_public cairo_surface_t *
cairo_pdf_surface_create (const char		*filename,
			  double		 width_in_points,
			  double		 height_in_points);

cairo_public cairo_surface_t *
cairo_pdf_surface_create_for_stream (cairo_write_func_t	write_func,
				     void	       *closure,
				     double		width_in_points,
				     double		height_in_points);

cairo_public void
cairo_pdf_surface_set_size (cairo_surface_t	*surface,
			    double		 width_in_points,
			    double		 height_in_points);

CAIRO_END_DECLS

#else  
# error Cairo was not compiled with support for the pdf backend
#endif 

#endif 
