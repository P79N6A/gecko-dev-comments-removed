

























#include "hb-ot-shape-complex-private.hh"








enum thai_consonant_type_t
{
  NC,
  AC,
  RC,
  DC,
  NOT_CONSONANT,
  NUM_CONSONANT_TYPES = NOT_CONSONANT
};

static thai_consonant_type_t
get_consonant_type (hb_codepoint_t u)
{
  if (u == 0x0E1B || u == 0x0E1D || u == 0x0E1F)
    return AC;
  if (u == 0x0E0D || u == 0x0E10)
    return RC;
  if (u == 0x0E0E || u == 0x0E0F)
    return DC;
  if (hb_in_range<hb_codepoint_t> (u, 0x0E01, 0x0E2E))
    return NC;
  return NOT_CONSONANT;
}


enum thai_mark_type_t
{
  AV,
  BV,
  T,
  NOT_MARK,
  NUM_MARK_TYPES = NOT_MARK
};

static thai_mark_type_t
get_mark_type (hb_codepoint_t u)
{
  if (u == 0x0E31 || hb_in_range<hb_codepoint_t> (u, 0x0E34, 0x0E37) ||
      u == 0x0E47 || hb_in_range<hb_codepoint_t> (u, 0x0E4D, 0x0E4E))
    return AV;
  if (hb_in_range<hb_codepoint_t> (u, 0x0E38, 0x0E3A))
    return BV;
  if (hb_in_range<hb_codepoint_t> (u, 0x0E48, 0x0E4C))
    return T;
  return NOT_MARK;
}


enum thai_action_t
{
  NOP,
  SD,  
  SL,  
  SDL, 
  RD   
};

static hb_codepoint_t
thai_pua_shape (hb_codepoint_t u, thai_action_t action, hb_font_t *font)
{
  struct thai_pua_mapping_t {
    hb_codepoint_t u;
    hb_codepoint_t win_pua;
    hb_codepoint_t mac_pua;
  } const *pua_mappings = NULL;
  static const thai_pua_mapping_t SD_mappings[] = {
    {0x0E48, 0xF70A, 0xF88B}, 
    {0x0E49, 0xF70B, 0xF88E}, 
    {0x0E4A, 0xF70C, 0xF891}, 
    {0x0E4B, 0xF70D, 0xF894}, 
    {0x0E4C, 0xF70E, 0xF897}, 
    {0x0E38, 0xF718, 0xF89B}, 
    {0x0E39, 0xF719, 0xF89C}, 
    {0x0E3A, 0xF71A, 0xF89D}, 
    {0x0000, 0x0000, 0x0000}
  };
  static const thai_pua_mapping_t SDL_mappings[] = {
    {0x0E48, 0xF705, 0xF88C}, 
    {0x0E49, 0xF706, 0xF88F}, 
    {0x0E4A, 0xF707, 0xF892}, 
    {0x0E4B, 0xF708, 0xF895}, 
    {0x0E4C, 0xF709, 0xF898}, 
    {0x0000, 0x0000, 0x0000}
  };
  static const thai_pua_mapping_t SL_mappings[] = {
    {0x0E48, 0xF713, 0xF88A}, 
    {0x0E49, 0xF714, 0xF88D}, 
    {0x0E4A, 0xF715, 0xF890}, 
    {0x0E4B, 0xF716, 0xF893}, 
    {0x0E4C, 0xF717, 0xF896}, 
    {0x0E31, 0xF710, 0xF884}, 
    {0x0E34, 0xF701, 0xF885}, 
    {0x0E35, 0xF702, 0xF886}, 
    {0x0E36, 0xF703, 0xF887}, 
    {0x0E37, 0xF704, 0xF888}, 
    {0x0E47, 0xF712, 0xF889}, 
    {0x0E4D, 0xF711, 0xF899}, 
    {0x0000, 0x0000, 0x0000}
  };
  static const thai_pua_mapping_t RD_mappings[] = {
    {0x0E0D, 0xF70F, 0xF89A}, 
    {0x0E10, 0xF700, 0xF89E}, 
    {0x0000, 0x0000, 0x0000}
  };

  switch (action) {
    default: assert (false); 
    case NOP: return u;
    case SD:  pua_mappings = SD_mappings; break;
    case SDL: pua_mappings = SDL_mappings; break;
    case SL:  pua_mappings = SL_mappings; break;
    case RD:  pua_mappings = RD_mappings; break;
  }
  for (; pua_mappings->u; pua_mappings++)
    if (pua_mappings->u == u)
    {
      hb_codepoint_t glyph;
      if (hb_font_get_glyph (font, pua_mappings->win_pua, 0, &glyph))
	return pua_mappings->win_pua;
      if (hb_font_get_glyph (font, pua_mappings->mac_pua, 0, &glyph))
	return pua_mappings->mac_pua;
      break;
    }
  return u;
}


static enum thai_above_state_t
{     
  T0, 
  T1, 
  T2, 
  T3, 
  NUM_ABOVE_STATES
} thai_above_start_state[NUM_CONSONANT_TYPES + 1] =
{
  T0, 
  T1, 
  T0, 
  T0, 
  T3, 
};

static const struct thai_above_state_machine_edge_t {
  thai_action_t action;
  thai_above_state_t next_state;
} thai_above_state_machine[NUM_ABOVE_STATES][NUM_MARK_TYPES] =
{                
 {{NOP,T3}, {NOP,T0}, {SD, T3}},
 {{SL, T2}, {NOP,T1}, {SDL,T2}},
 {{NOP,T3}, {NOP,T2}, {SL, T3}},
 {{NOP,T3}, {NOP,T3}, {NOP,T3}},
};


static enum thai_below_state_t
{
  B0, 
  B1, 
  B2, 
  NUM_BELOW_STATES
} thai_below_start_state[NUM_CONSONANT_TYPES + 1] =
{
  B0, 
  B0, 
  B1, 
  B2, 
  B2, 
};

static const struct thai_below_state_machine_edge_t {
  thai_action_t action;
  thai_below_state_t next_state;
} thai_below_state_machine[NUM_BELOW_STATES][NUM_MARK_TYPES] =
{                
 {{NOP,B0}, {NOP,B2}, {NOP, B0}},
 {{NOP,B1}, {RD, B2}, {NOP, B1}},
 {{NOP,B2}, {SD, B2}, {NOP, B2}},
};


static void
do_thai_pua_shaping (const hb_ot_shape_plan_t *plan HB_UNUSED,
		     hb_buffer_t              *buffer,
		     hb_font_t                *font)
{
  thai_above_state_t above_state = thai_above_start_state[NOT_CONSONANT];
  thai_below_state_t below_state = thai_below_start_state[NOT_CONSONANT];
  unsigned int base = 0;

  hb_glyph_info_t *info = buffer->info;
  unsigned int count = buffer->len;
  for (unsigned int i = 0; i < count; i++)
  {
    thai_mark_type_t mt = get_mark_type (info[i].codepoint);

    if (mt == NOT_MARK) {
      thai_consonant_type_t ct = get_consonant_type (info[i].codepoint);
      above_state = thai_above_start_state[ct];
      below_state = thai_below_start_state[ct];
      base = i;
      continue;
    }

    const thai_above_state_machine_edge_t &above_edge = thai_above_state_machine[above_state][mt];
    const thai_below_state_machine_edge_t &below_edge = thai_below_state_machine[below_state][mt];
    above_state = above_edge.next_state;
    below_state = below_edge.next_state;

    
    thai_action_t action = above_edge.action != NOP ? above_edge.action : below_edge.action;

    if (action == RD)
      info[base].codepoint = thai_pua_shape (info[base].codepoint, action, font);
    else
      info[i].codepoint = thai_pua_shape (info[i].codepoint, action, font);
  }
}


static void
preprocess_text_thai (const hb_ot_shape_plan_t *plan,
		      hb_buffer_t              *buffer,
		      hb_font_t                *font)
{
  








  























  














  

#define IS_SARA_AM(x) (((x) & ~0x0080) == 0x0E33)
#define NIKHAHIT_FROM_SARA_AM(x) ((x) - 0xE33 + 0xE4D)
#define SARA_AA_FROM_SARA_AM(x) ((x) - 1)
#define IS_TONE_MARK(x) (hb_in_ranges<hb_codepoint_t> ((x) & ~0x0080, 0x0E34, 0x0E37, 0x0E47, 0x0E4E, 0x0E31, 0x0E31))

  buffer->clear_output ();
  unsigned int count = buffer->len;
  for (buffer->idx = 0; buffer->idx < count;)
  {
    hb_codepoint_t u = buffer->cur().codepoint;
    if (likely (!IS_SARA_AM (u))) {
      buffer->next_glyph ();
      continue;
    }

    
    hb_codepoint_t decomposed[2] = {hb_codepoint_t (NIKHAHIT_FROM_SARA_AM (u)),
				    hb_codepoint_t (SARA_AA_FROM_SARA_AM (u))};
    buffer->replace_glyphs (1, 2, decomposed);
    if (unlikely (buffer->in_error))
      return;

    
    unsigned int end = buffer->out_len;
    unsigned int start = end - 2;
    while (start > 0 && IS_TONE_MARK (buffer->out_info[start - 1].codepoint))
      start--;

    if (start + 2 < end)
    {
      
      buffer->merge_out_clusters (start, end);
      hb_glyph_info_t t = buffer->out_info[end - 2];
      memmove (buffer->out_info + start + 1,
	       buffer->out_info + start,
	       sizeof (buffer->out_info[0]) * (end - start - 2));
      buffer->out_info[start] = t;
    }
    else
    {
      

      if (start)
	buffer->merge_out_clusters (start - 1, end);
    }
  }
  buffer->swap_buffers ();

  
  if (plan->props.script == HB_SCRIPT_THAI && !plan->map.found_script[0])
    do_thai_pua_shaping (plan, buffer, font);
}

const hb_ot_complex_shaper_t _hb_ot_complex_shaper_thai =
{
  "thai",
  NULL, 
  NULL, 
  NULL, 
  NULL, 
  preprocess_text_thai,
  NULL, 
  NULL, 
  NULL, 
  NULL, 
  HB_OT_SHAPE_ZERO_WIDTH_MARKS_BY_UNICODE,
  false,
};
