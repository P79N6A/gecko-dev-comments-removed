



























#ifndef HB_ICU_H
#define HB_ICU_H

#include "hb.h"

#include <unicode/uscript.h>

HB_BEGIN_DECLS


hb_script_t
hb_icu_script_to_script (UScriptCode script);

UScriptCode
hb_icu_script_from_script (hb_script_t script);


hb_unicode_funcs_t *
hb_icu_get_unicode_funcs (void);


HB_END_DECLS

#endif 
