

























#ifndef HB_CORETEXT_H
#define HB_CORETEXT_H

#include "hb.h"

#include <TargetConditionals.h>
#if TARGET_OS_IPHONE
#  include <CoreText/CoreText.h>
#  include <CoreGraphics/CoreGraphics.h>
#else
#  include <ApplicationServices/ApplicationServices.h>
#endif

HB_BEGIN_DECLS


#define HB_CORETEXT_TAG_MORT HB_TAG('m','o','r','t')
#define HB_CORETEXT_TAG_MORX HB_TAG('m','o','r','x')


hb_face_t *
hb_coretext_face_create (CGFontRef cg_font);


CGFontRef
hb_coretext_face_get_cg_font (hb_face_t *face);

CTFontRef
hb_coretext_font_get_ct_font (hb_font_t *font);


HB_END_DECLS

#endif 
