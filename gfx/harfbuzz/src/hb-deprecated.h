

























#ifndef HB_H_IN
#error "Include <hb.h> instead."
#endif

#ifndef HB_DEPRECATED_H
#define HB_DEPRECATED_H

#include "hb-common.h"
#include "hb-unicode.h"
#include "hb-font.h"

HB_BEGIN_DECLS

#ifndef HB_DISABLE_DEPRECATED

#define HB_SCRIPT_CANADIAN_ABORIGINAL		HB_SCRIPT_CANADIAN_SYLLABICS

#define HB_BUFFER_FLAGS_DEFAULT			HB_BUFFER_FLAG_DEFAULT
#define HB_BUFFER_SERIALIZE_FLAGS_DEFAULT	HB_BUFFER_SERIALIZE_FLAG_DEFAULT

#endif

HB_END_DECLS

#endif 
