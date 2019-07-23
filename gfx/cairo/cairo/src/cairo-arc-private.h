


































#ifndef CAIRO_ARC_PRIVATE_H
#define CAIRO_ARC_PRIVATE_H

#include "cairoint.h"

cairo_private void
_cairo_arc_path (cairo_t *cr,
		 double	  xc,
		 double	  yc,
		 double	  radius,
		 double	  angle1,
		 double	  angle2);

cairo_private void
_cairo_arc_path_negative (cairo_t *cr,
			  double   xc,
			  double   yc,
			  double   radius,
			  double   angle1,
			  double   angle2);

#endif 
