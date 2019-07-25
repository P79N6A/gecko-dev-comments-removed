





































#define CAIRO_VERSION_H 1

#include "cairoint.h"


#undef CAIRO_VERSION_H
#include "cairo-features.h"




















int
cairo_version (void)
{
    return CAIRO_VERSION;
}












const char*
cairo_version_string (void)
{
    return CAIRO_VERSION_STRING;
}
slim_hidden_def (cairo_version_string);
