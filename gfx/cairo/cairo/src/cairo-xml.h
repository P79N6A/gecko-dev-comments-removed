


































#ifndef CAIRO_XML_H
#define CAIRO_XML_H

#include "cairo.h"

#if CAIRO_HAS_XML_SURFACE

CAIRO_BEGIN_DECLS

typedef struct _cairo_xml cairo_xml_t;

cairo_public cairo_xml_t *
cairo_xml_create (const char *filename);

cairo_public cairo_xml_t *
cairo_xml_create_for_stream (cairo_write_func_t	 write_func,
			     void		*closure);

cairo_public void
cairo_xml_destroy (cairo_xml_t *context);

cairo_public cairo_surface_t *
cairo_xml_surface_create (cairo_xml_t *xml,
			  cairo_content_t content,
			  double width, double height);

cairo_public cairo_status_t
cairo_xml_for_recording_surface (cairo_xml_t *context,
				 cairo_surface_t *surface);

CAIRO_END_DECLS

#else  
# error Cairo was not compiled with support for the XML backend
#endif 

#endif 
