

























#ifndef HB_LANGUAGE_H
#define HB_LANGUAGE_H

#include "hb-common.h"

HB_BEGIN_DECLS


typedef const void *hb_language_t;

hb_language_t
hb_language_from_string (const char *str);

const char *
hb_language_to_string (hb_language_t language);


HB_END_DECLS

#endif 
