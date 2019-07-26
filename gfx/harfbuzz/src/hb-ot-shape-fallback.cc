

























#include "hb-ot-shape-fallback-private.hh"

static unsigned int
recategorize_combining_class (hb_codepoint_t u,
			      unsigned int klass)
{
  if (klass >= 200)
    return klass;

  
  if ((u & ~0xFF) == 0x0E00)
  {
    if (unlikely (klass == 0))
    {
      switch (u)
      {
        case 0x0E31:
        case 0x0E34:
        case 0x0E35:
        case 0x0E36:
        case 0x0E37:
        case 0x0E47:
        case 0x0E4C:
        case 0x0E4D:
        case 0x0E4E:
	  klass = HB_UNICODE_COMBINING_CLASS_ABOVE_RIGHT;
	  break;

        case 0x0EB1:
        case 0x0EB4:
        case 0x0EB5:
        case 0x0EB6:
        case 0x0EB7:
        case 0x0EBB:
        case 0x0ECC:
        case 0x0ECD:
	  klass = HB_UNICODE_COMBINING_CLASS_ABOVE;
	  break;

        case 0x0EBC:
	  klass = HB_UNICODE_COMBINING_CLASS_BELOW;
	  break;
      }
    } else {
      
      if (u == 0x0E3A)
	klass = HB_UNICODE_COMBINING_CLASS_BELOW_RIGHT;
    }
  }

  switch (klass)
  {

    

    case HB_MODIFIED_COMBINING_CLASS_CCC10: 
    case HB_MODIFIED_COMBINING_CLASS_CCC11: 
    case HB_MODIFIED_COMBINING_CLASS_CCC12: 
    case HB_MODIFIED_COMBINING_CLASS_CCC13: 
    case HB_MODIFIED_COMBINING_CLASS_CCC14: 
    case HB_MODIFIED_COMBINING_CLASS_CCC15: 
    case HB_MODIFIED_COMBINING_CLASS_CCC16: 
    case HB_MODIFIED_COMBINING_CLASS_CCC17: 
    case HB_MODIFIED_COMBINING_CLASS_CCC18: 
    case HB_MODIFIED_COMBINING_CLASS_CCC20: 
    case HB_MODIFIED_COMBINING_CLASS_CCC22: 
      return HB_UNICODE_COMBINING_CLASS_BELOW;

    case HB_MODIFIED_COMBINING_CLASS_CCC23: 
      return HB_UNICODE_COMBINING_CLASS_ATTACHED_ABOVE;

    case HB_MODIFIED_COMBINING_CLASS_CCC24: 
      return HB_UNICODE_COMBINING_CLASS_ABOVE_RIGHT;

    case HB_MODIFIED_COMBINING_CLASS_CCC25: 
    case HB_MODIFIED_COMBINING_CLASS_CCC19: 
      return HB_UNICODE_COMBINING_CLASS_ABOVE_LEFT;

    case HB_MODIFIED_COMBINING_CLASS_CCC26: 
      return HB_UNICODE_COMBINING_CLASS_ABOVE;

    case HB_MODIFIED_COMBINING_CLASS_CCC21: 
      break;


    

    case HB_MODIFIED_COMBINING_CLASS_CCC27: 
    case HB_MODIFIED_COMBINING_CLASS_CCC28: 
    case HB_MODIFIED_COMBINING_CLASS_CCC30: 
    case HB_MODIFIED_COMBINING_CLASS_CCC31: 
    case HB_MODIFIED_COMBINING_CLASS_CCC33: 
    case HB_MODIFIED_COMBINING_CLASS_CCC34: 
    case HB_MODIFIED_COMBINING_CLASS_CCC35: 
    case HB_MODIFIED_COMBINING_CLASS_CCC36: 
      return HB_UNICODE_COMBINING_CLASS_ABOVE;

    case HB_MODIFIED_COMBINING_CLASS_CCC29: 
    case HB_MODIFIED_COMBINING_CLASS_CCC32: 
      return HB_UNICODE_COMBINING_CLASS_BELOW;


    

    case HB_MODIFIED_COMBINING_CLASS_CCC103: 
      return HB_UNICODE_COMBINING_CLASS_BELOW_RIGHT;

    case HB_MODIFIED_COMBINING_CLASS_CCC107: 
      return HB_UNICODE_COMBINING_CLASS_ABOVE_RIGHT;


    

    case HB_MODIFIED_COMBINING_CLASS_CCC118: 
      return HB_UNICODE_COMBINING_CLASS_BELOW;

    case HB_MODIFIED_COMBINING_CLASS_CCC122: 
      return HB_UNICODE_COMBINING_CLASS_ABOVE;


    

    case HB_MODIFIED_COMBINING_CLASS_CCC129: 
      return HB_UNICODE_COMBINING_CLASS_BELOW;

    case HB_MODIFIED_COMBINING_CLASS_CCC130: 
      return HB_UNICODE_COMBINING_CLASS_ABOVE;

    case HB_MODIFIED_COMBINING_CLASS_CCC132: 
      return HB_UNICODE_COMBINING_CLASS_BELOW;

  }

  return klass;
}

void
_hb_ot_shape_fallback_position_recategorize_marks (const hb_ot_shape_plan_t *plan HB_UNUSED,
						   hb_font_t *font HB_UNUSED,
						   hb_buffer_t  *buffer)
{
  unsigned int count = buffer->len;
  for (unsigned int i = 0; i < count; i++)
    if (_hb_glyph_info_get_general_category (&buffer->info[i]) == HB_UNICODE_GENERAL_CATEGORY_NON_SPACING_MARK) {
      unsigned int combining_class = _hb_glyph_info_get_modified_combining_class (&buffer->info[i]);
      combining_class = recategorize_combining_class (buffer->info[i].codepoint, combining_class);
      _hb_glyph_info_set_modified_combining_class (&buffer->info[i], combining_class);
    }
}


static void
zero_mark_advances (hb_buffer_t *buffer,
		    unsigned int start,
		    unsigned int end)
{
  for (unsigned int i = start; i < end; i++)
    if (_hb_glyph_info_get_general_category (&buffer->info[i]) == HB_UNICODE_GENERAL_CATEGORY_NON_SPACING_MARK)
    {
      buffer->pos[i].x_advance = 0;
      buffer->pos[i].y_advance = 0;
    }
}

static inline void
position_mark (const hb_ot_shape_plan_t *plan,
	       hb_font_t *font,
	       hb_buffer_t  *buffer,
	       hb_glyph_extents_t &base_extents,
	       unsigned int i,
	       unsigned int combining_class)
{
  hb_glyph_extents_t mark_extents;
  if (!font->get_glyph_extents (buffer->info[i].codepoint,
				&mark_extents))
    return;

  hb_position_t y_gap = font->y_scale / 16;

  hb_glyph_position_t &pos = buffer->pos[i];
  pos.x_offset = pos.y_offset = 0;


  

  
  switch (combining_class)
  {
    case HB_UNICODE_COMBINING_CLASS_DOUBLE_BELOW:
    case HB_UNICODE_COMBINING_CLASS_DOUBLE_ABOVE:
      if (buffer->props.direction == HB_DIRECTION_LTR) {
	pos.x_offset += base_extents.x_bearing - mark_extents.width / 2 - mark_extents.x_bearing;
        break;
      } else if (buffer->props.direction == HB_DIRECTION_RTL) {
	pos.x_offset += base_extents.x_bearing + base_extents.width - mark_extents.width / 2 - mark_extents.x_bearing;
        break;
      }
      

    default:
    case HB_UNICODE_COMBINING_CLASS_ATTACHED_BELOW:
    case HB_UNICODE_COMBINING_CLASS_ATTACHED_ABOVE:
    case HB_UNICODE_COMBINING_CLASS_BELOW:
    case HB_UNICODE_COMBINING_CLASS_ABOVE:
      
      pos.x_offset += base_extents.x_bearing + (base_extents.width - mark_extents.width) / 2 - mark_extents.x_bearing;
      break;

    case HB_UNICODE_COMBINING_CLASS_ATTACHED_BELOW_LEFT:
    case HB_UNICODE_COMBINING_CLASS_BELOW_LEFT:
    case HB_UNICODE_COMBINING_CLASS_ABOVE_LEFT:
      
      pos.x_offset += base_extents.x_bearing - mark_extents.x_bearing;
      break;

    case HB_UNICODE_COMBINING_CLASS_ATTACHED_ABOVE_RIGHT:
    case HB_UNICODE_COMBINING_CLASS_BELOW_RIGHT:
    case HB_UNICODE_COMBINING_CLASS_ABOVE_RIGHT:
      
      pos.x_offset += base_extents.x_bearing + base_extents.width - mark_extents.width - mark_extents.x_bearing;
      break;
  }

  
  switch (combining_class)
  {
    case HB_UNICODE_COMBINING_CLASS_DOUBLE_BELOW:
    case HB_UNICODE_COMBINING_CLASS_BELOW_LEFT:
    case HB_UNICODE_COMBINING_CLASS_BELOW:
    case HB_UNICODE_COMBINING_CLASS_BELOW_RIGHT:
      
      base_extents.height -= y_gap;

    case HB_UNICODE_COMBINING_CLASS_ATTACHED_BELOW_LEFT:
    case HB_UNICODE_COMBINING_CLASS_ATTACHED_BELOW:
      pos.y_offset += base_extents.y_bearing + base_extents.height - mark_extents.y_bearing;
      base_extents.height += mark_extents.height;
      break;

    case HB_UNICODE_COMBINING_CLASS_DOUBLE_ABOVE:
    case HB_UNICODE_COMBINING_CLASS_ABOVE_LEFT:
    case HB_UNICODE_COMBINING_CLASS_ABOVE:
    case HB_UNICODE_COMBINING_CLASS_ABOVE_RIGHT:
      
      base_extents.y_bearing += y_gap;
      base_extents.height -= y_gap;

    case HB_UNICODE_COMBINING_CLASS_ATTACHED_ABOVE:
    case HB_UNICODE_COMBINING_CLASS_ATTACHED_ABOVE_RIGHT:
      pos.y_offset += base_extents.y_bearing - (mark_extents.y_bearing + mark_extents.height);
      base_extents.y_bearing -= mark_extents.height;
      base_extents.height += mark_extents.height;
      break;
  }
}

static inline void
position_around_base (const hb_ot_shape_plan_t *plan,
		      hb_font_t *font,
		      hb_buffer_t  *buffer,
		      unsigned int base,
		      unsigned int end)
{
  hb_direction_t horiz_dir = HB_DIRECTION_INVALID;
  hb_glyph_extents_t base_extents;
  if (!font->get_glyph_extents (buffer->info[base].codepoint,
				&base_extents))
  {
    
    zero_mark_advances (buffer, base + 1, end);
    return;
  }
  base_extents.x_bearing += buffer->pos[base].x_offset;
  base_extents.y_bearing += buffer->pos[base].y_offset;

  unsigned int lig_id = get_lig_id (buffer->info[base]);
  unsigned int num_lig_components = get_lig_num_comps (buffer->info[base]);

  hb_position_t x_offset = 0, y_offset = 0;
  if (HB_DIRECTION_IS_FORWARD (buffer->props.direction)) {
    x_offset -= buffer->pos[base].x_advance;
    y_offset -= buffer->pos[base].y_advance;
  }

  hb_glyph_extents_t component_extents = base_extents;
  unsigned int last_lig_component = (unsigned int) -1;
  unsigned int last_combining_class = 255;
  hb_glyph_extents_t cluster_extents = base_extents; 
  for (unsigned int i = base + 1; i < end; i++)
    if (_hb_glyph_info_get_modified_combining_class (&buffer->info[i]))
    {
      if (num_lig_components > 1) {
	unsigned int this_lig_id = get_lig_id (buffer->info[i]);
	unsigned int this_lig_component = get_lig_comp (buffer->info[i]) - 1;
	
	if (!lig_id || lig_id != this_lig_id || this_lig_component >= num_lig_components)
	  this_lig_component = num_lig_components - 1;
	if (last_lig_component != this_lig_component)
	{
	  last_lig_component = this_lig_component;
	  last_combining_class = 255;
	  component_extents = base_extents;
	  if (unlikely (horiz_dir == HB_DIRECTION_INVALID)) {
	    if (HB_DIRECTION_IS_HORIZONTAL (plan->props.direction))
	      horiz_dir = plan->props.direction;
	    else
	      horiz_dir = hb_script_get_horizontal_direction (plan->props.script);
	  }
	  if (horiz_dir == HB_DIRECTION_LTR)
	    component_extents.x_bearing += (this_lig_component * component_extents.width) / num_lig_components;
	  else
	    component_extents.x_bearing += ((num_lig_components - 1 - this_lig_component) * component_extents.width) / num_lig_components;
	  component_extents.width /= num_lig_components;
	}
      }

      unsigned int this_combining_class = _hb_glyph_info_get_modified_combining_class (&buffer->info[i]);
      if (last_combining_class != this_combining_class)
      {
	last_combining_class = this_combining_class;
        cluster_extents = component_extents;
      }

      position_mark (plan, font, buffer, cluster_extents, i, this_combining_class);

      buffer->pos[i].x_advance = 0;
      buffer->pos[i].y_advance = 0;
      buffer->pos[i].x_offset += x_offset;
      buffer->pos[i].y_offset += y_offset;

    } else {
      if (HB_DIRECTION_IS_FORWARD (buffer->props.direction)) {
	x_offset -= buffer->pos[i].x_advance;
	y_offset -= buffer->pos[i].y_advance;
      } else {
	x_offset += buffer->pos[i].x_advance;
	y_offset += buffer->pos[i].y_advance;
      }
    }
}

static inline void
position_cluster (const hb_ot_shape_plan_t *plan,
		  hb_font_t *font,
		  hb_buffer_t  *buffer,
		  unsigned int start,
		  unsigned int end)
{
  if (end - start < 2)
    return;

  
  for (unsigned int i = start; i < end; i++)
    if (!HB_UNICODE_GENERAL_CATEGORY_IS_MARK (_hb_glyph_info_get_general_category (&buffer->info[i])))
    {
      
      unsigned int j;
      for (j = i + 1; j < end; j++)
	if (!HB_UNICODE_GENERAL_CATEGORY_IS_MARK (_hb_glyph_info_get_general_category (&buffer->info[j])))
	  break;

      position_around_base (plan, font, buffer, i, j);

      i = j - 1;
    }
}

void
_hb_ot_shape_fallback_position (const hb_ot_shape_plan_t *plan,
				hb_font_t *font,
				hb_buffer_t  *buffer)
{
  unsigned int start = 0;
  unsigned int last_cluster = buffer->info[0].cluster;
  unsigned int count = buffer->len;
  for (unsigned int i = 1; i < count; i++)
    if (buffer->info[i].cluster != last_cluster) {
      position_cluster (plan, font, buffer, start, i);
      start = i;
      last_cluster = buffer->info[i].cluster;
    }
  position_cluster (plan, font, buffer, start, count);
}
