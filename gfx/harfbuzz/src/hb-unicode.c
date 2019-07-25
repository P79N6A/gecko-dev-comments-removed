

























#include "hb-private.h"

#include "hb-unicode-private.h"

HB_BEGIN_DECLS






static hb_codepoint_t hb_unicode_get_mirroring_nil (hb_codepoint_t unicode) { return unicode; }
static hb_category_t hb_unicode_get_general_category_nil (hb_codepoint_t unicode HB_UNUSED) { return HB_CATEGORY_OTHER_LETTER; }
static hb_script_t hb_unicode_get_script_nil (hb_codepoint_t unicode HB_UNUSED) { return HB_SCRIPT_UNKNOWN; }
static unsigned int hb_unicode_get_combining_class_nil (hb_codepoint_t unicode HB_UNUSED) { return 0; }
static unsigned int hb_unicode_get_eastasian_width_nil (hb_codepoint_t unicode HB_UNUSED) { return 1; }

hb_unicode_funcs_t _hb_unicode_funcs_nil = {
  HB_REFERENCE_COUNT_INVALID, 
  TRUE, 
  {
    hb_unicode_get_general_category_nil,
    hb_unicode_get_combining_class_nil,
    hb_unicode_get_mirroring_nil,
    hb_unicode_get_script_nil,
    hb_unicode_get_eastasian_width_nil
  }
};

hb_unicode_funcs_t *
hb_unicode_funcs_create (void)
{
  hb_unicode_funcs_t *ufuncs;

  if (!HB_OBJECT_DO_CREATE (hb_unicode_funcs_t, ufuncs))
    return &_hb_unicode_funcs_nil;

  ufuncs->v = _hb_unicode_funcs_nil.v;

  return ufuncs;
}

hb_unicode_funcs_t *
hb_unicode_funcs_reference (hb_unicode_funcs_t *ufuncs)
{
  HB_OBJECT_DO_REFERENCE (ufuncs);
}

unsigned int
hb_unicode_funcs_get_reference_count (hb_unicode_funcs_t *ufuncs)
{
  HB_OBJECT_DO_GET_REFERENCE_COUNT (ufuncs);
}

void
hb_unicode_funcs_destroy (hb_unicode_funcs_t *ufuncs)
{
  HB_OBJECT_DO_DESTROY (ufuncs);

  free (ufuncs);
}

hb_unicode_funcs_t *
hb_unicode_funcs_copy (hb_unicode_funcs_t *other_ufuncs)
{
  hb_unicode_funcs_t *ufuncs;

  if (!HB_OBJECT_DO_CREATE (hb_unicode_funcs_t, ufuncs))
    return &_hb_unicode_funcs_nil;

  ufuncs->v = other_ufuncs->v;

  return ufuncs;
}

void
hb_unicode_funcs_make_immutable (hb_unicode_funcs_t *ufuncs)
{
  if (HB_OBJECT_IS_INERT (ufuncs))
    return;

  ufuncs->immutable = TRUE;
}

hb_bool_t
hb_unicode_funcs_is_immutable (hb_unicode_funcs_t *ufuncs)
{
  return ufuncs->immutable;
}


void
hb_unicode_funcs_set_mirroring_func (hb_unicode_funcs_t *ufuncs,
				     hb_unicode_get_mirroring_func_t mirroring_func)
{
  if (ufuncs->immutable)
    return;

  ufuncs->v.get_mirroring = mirroring_func ? mirroring_func : hb_unicode_get_mirroring_nil;
}

void
hb_unicode_funcs_set_general_category_func (hb_unicode_funcs_t *ufuncs,
					    hb_unicode_get_general_category_func_t general_category_func)
{
  if (ufuncs->immutable)
    return;

  ufuncs->v.get_general_category = general_category_func ? general_category_func : hb_unicode_get_general_category_nil;
}

void
hb_unicode_funcs_set_script_func (hb_unicode_funcs_t *ufuncs,
				  hb_unicode_get_script_func_t script_func)
{
  if (ufuncs->immutable)
    return;

  ufuncs->v.get_script = script_func ? script_func : hb_unicode_get_script_nil;
}

void
hb_unicode_funcs_set_combining_class_func (hb_unicode_funcs_t *ufuncs,
					   hb_unicode_get_combining_class_func_t combining_class_func)
{
  if (ufuncs->immutable)
    return;

  ufuncs->v.get_combining_class = combining_class_func ? combining_class_func : hb_unicode_get_combining_class_nil;
}

void
hb_unicode_funcs_set_eastasian_width_func (hb_unicode_funcs_t *ufuncs,
					   hb_unicode_get_eastasian_width_func_t eastasian_width_func)
{
  if (ufuncs->immutable)
    return;

  ufuncs->v.get_eastasian_width = eastasian_width_func ? eastasian_width_func : hb_unicode_get_eastasian_width_nil;
}


hb_unicode_get_mirroring_func_t
hb_unicode_funcs_get_mirroring_func (hb_unicode_funcs_t *ufuncs)
{
  return ufuncs->v.get_mirroring;
}

hb_unicode_get_general_category_func_t
hb_unicode_funcs_get_general_category_func (hb_unicode_funcs_t *ufuncs)
{
  return ufuncs->v.get_general_category;
}

hb_unicode_get_script_func_t
hb_unicode_funcs_get_script_func (hb_unicode_funcs_t *ufuncs)
{
  return ufuncs->v.get_script;
}

hb_unicode_get_combining_class_func_t
hb_unicode_funcs_get_combining_class_func (hb_unicode_funcs_t *ufuncs)
{
  return ufuncs->v.get_combining_class;
}

hb_unicode_get_eastasian_width_func_t
hb_unicode_funcs_get_eastasian_width_func (hb_unicode_funcs_t *ufuncs)
{
  return ufuncs->v.get_eastasian_width;
}



hb_codepoint_t
hb_unicode_get_mirroring (hb_unicode_funcs_t *ufuncs,
			  hb_codepoint_t unicode)
{
  return ufuncs->v.get_mirroring (unicode);
}

hb_category_t
hb_unicode_get_general_category (hb_unicode_funcs_t *ufuncs,
				 hb_codepoint_t unicode)
{
  return ufuncs->v.get_general_category (unicode);
}

hb_script_t
hb_unicode_get_script (hb_unicode_funcs_t *ufuncs,
		       hb_codepoint_t unicode)
{
  return ufuncs->v.get_script (unicode);
}

unsigned int
hb_unicode_get_combining_class (hb_unicode_funcs_t *ufuncs,
				hb_codepoint_t unicode)
{
  return ufuncs->v.get_combining_class (unicode);
}

unsigned int
hb_unicode_get_eastasian_width (hb_unicode_funcs_t *ufuncs,
				hb_codepoint_t unicode)
{
  return ufuncs->v.get_eastasian_width (unicode);
}



#define LTR HB_DIRECTION_LTR
#define RTL HB_DIRECTION_RTL
const hb_direction_t horiz_dir[] =
{
  LTR,	
  LTR,	
  RTL,	
  LTR,	
  LTR,	
  LTR,	
  LTR,	
  LTR,	
  LTR,	
  LTR,	
  LTR,	
  LTR,	
  LTR,	
  LTR,	
  LTR,	
  LTR,	
  LTR,	
  LTR,	
  LTR,	
  RTL,	
  LTR,	
  LTR,	
  LTR,	
  LTR,	
  LTR,	
  LTR,	
  LTR,	
  LTR,	
  LTR,	
  LTR,	
  LTR,	
  LTR,	
  LTR,	
  LTR,	
  RTL,	
  LTR,	
  LTR,	
  RTL,	
  LTR,	
  LTR,	
  LTR,	
  LTR,	
  LTR,	
  LTR,	
  LTR,	
  LTR,	

  
  LTR,	
  RTL,	
  LTR,	
  LTR,	
  LTR,	
  LTR,	
  LTR,	
  LTR,	

  
  LTR,	
  LTR,	
  LTR,	
  LTR,	
  LTR,	
  LTR,	
  LTR,	

  
  LTR,	
  LTR,	
  LTR,	
  RTL,	
  LTR,	
  RTL,	

  
  LTR,	
  LTR,	
  LTR,	
  LTR,	
  LTR,	
  LTR,	
  LTR,	
  LTR,	
  LTR,	
  LTR,	
  LTR,	

  
  RTL,	
  LTR,	
  LTR,	
  RTL,	
  RTL,	
  RTL,	
  LTR,	
  LTR,	
  LTR,	
  LTR,	
  RTL,	
  RTL,	
  RTL,	
  LTR,	
  LTR,	

  
  LTR,	
  LTR,	
  RTL 	
};
#undef LTR
#undef RTL

hb_direction_t
_hb_script_get_horizontal_direction (hb_script_t script)
{
  if (unlikely ((unsigned int) script >= ARRAY_LENGTH (horiz_dir)))
    return HB_DIRECTION_LTR;

  return horiz_dir[script];
}


HB_END_DECLS
