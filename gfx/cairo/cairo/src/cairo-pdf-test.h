



































#ifndef CAIRO_PDF_TEST_H
#define CAIRO_PDF_TEST_H

#include <cairo.h>

#if CAIRO_HAS_PDF_SURFACE

#include <cairo-pdf.h>

CAIRO_BEGIN_DECLS

cairo_public void
_cairo_pdf_test_force_fallbacks (void);

CAIRO_END_DECLS

#endif 
#endif 
