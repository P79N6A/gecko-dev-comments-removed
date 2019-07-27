

























#ifndef HB_H_IN
#error "Include <hb.h> instead."
#endif

#ifndef HB_VERSION_H
#define HB_VERSION_H

#include "hb-common.h"

HB_BEGIN_DECLS


#define HB_VERSION_MAJOR 0
#define HB_VERSION_MINOR 9
#define HB_VERSION_MICRO 34

#define HB_VERSION_STRING "0.9.34"

#define HB_VERSION_ATLEAST(major,minor,micro) \
	((major)*10000+(minor)*100+(micro) <= \
	 HB_VERSION_MAJOR*10000+HB_VERSION_MINOR*100+HB_VERSION_MICRO)


void
hb_version (unsigned int *major,
	    unsigned int *minor,
	    unsigned int *micro);

const char *
hb_version_string (void);

hb_bool_t
hb_version_atleast (unsigned int major,
		    unsigned int minor,
		    unsigned int micro);


HB_END_DECLS

#endif 
