




























#ifndef HB_BUFFER_PRIVATE_HH
#define HB_BUFFER_PRIVATE_HH

#include "hb-private.hh"
#include "hb-buffer.h"
#include "hb-object-private.hh"
#include "hb-unicode-private.hh"



ASSERT_STATIC (sizeof (hb_glyph_info_t) == 20);
ASSERT_STATIC (sizeof (hb_glyph_info_t) == sizeof (hb_glyph_position_t));






typedef struct hb_segment_properties_t {
    hb_direction_t      direction;
    hb_script_t         script;
    hb_language_t       language;
    ASSERT_POD ();
} hb_segment_properties_t;

#define _HB_BUFFER_PROPS_DEFAULT { HB_DIRECTION_INVALID, HB_SCRIPT_INVALID, HB_LANGUAGE_INVALID }

static inline hb_bool_t
hb_segment_properties_equal (const hb_segment_properties_t *a,
			     const hb_segment_properties_t *b)
{
  return a->direction == b->direction &&
	 a->script    == b->script    &&
	 a->language  == b->language;
}


#if 0
static inline unsigned int
hb_segment_properties_hash (const hb_segment_properties_t *p)
{
  
  return (unsigned int) p->direction +
	 (unsigned int) p->script +
	 (intptr_t) (p->language);
}
#endif







struct hb_buffer_t {
  hb_object_header_t header;
  ASSERT_POD ();

  

  hb_unicode_funcs_t *unicode; 
  hb_segment_properties_t props; 

  

  bool in_error; 
  bool have_output; 
  bool have_positions; 

  unsigned int idx; 
  unsigned int len; 
  unsigned int out_len; 

  unsigned int allocated; 
  hb_glyph_info_t     *info;
  hb_glyph_info_t     *out_info;
  hb_glyph_position_t *pos;

  inline hb_glyph_info_t &cur (unsigned int i = 0) { return info[idx + i]; }
  inline hb_glyph_info_t cur (unsigned int i = 0) const { return info[idx + i]; }

  inline hb_glyph_position_t &cur_pos (unsigned int i = 0) { return pos[idx + i]; }
  inline hb_glyph_position_t cur_pos (unsigned int i = 0) const { return pos[idx + i]; }

  inline hb_glyph_info_t &prev (void) { return out_info[out_len - 1]; }
  inline hb_glyph_info_t prev (void) const { return info[out_len - 1]; }

  unsigned int serial;
  uint8_t allocated_var_bytes[8];
  const char *allocated_var_owner[8];


  

  HB_INTERNAL void reset (void);

  inline unsigned int backtrack_len (void) const
  { return have_output? out_len : idx; }
  inline unsigned int next_serial (void) { return serial++; }

  HB_INTERNAL void allocate_var (unsigned int byte_i, unsigned int count, const char *owner);
  HB_INTERNAL void deallocate_var (unsigned int byte_i, unsigned int count, const char *owner);
  HB_INTERNAL void deallocate_var_all (void);

  HB_INTERNAL void add (hb_codepoint_t  codepoint,
			hb_mask_t       mask,
			unsigned int    cluster);

  HB_INTERNAL void reverse_range (unsigned int start, unsigned int end);
  HB_INTERNAL void reverse (void);
  HB_INTERNAL void reverse_clusters (void);
  HB_INTERNAL void guess_properties (void);

  HB_INTERNAL void swap_buffers (void);
  HB_INTERNAL void clear_output (void);
  HB_INTERNAL void clear_positions (void);

  HB_INTERNAL void replace_glyphs (unsigned int num_in,
				   unsigned int num_out,
				   const hb_codepoint_t *glyph_data);

  HB_INTERNAL void replace_glyph (hb_codepoint_t glyph_index);
  
  HB_INTERNAL void output_glyph (hb_codepoint_t glyph_index);
  
  HB_INTERNAL void copy_glyph (void);
  

  HB_INTERNAL void next_glyph (void);
  
  inline void skip_glyph (void) { idx++; }

  inline void reset_masks (hb_mask_t mask)
  {
    for (unsigned int j = 0; j < len; j++)
      info[j].mask = mask;
  }
  inline void add_masks (hb_mask_t mask)
  {
    for (unsigned int j = 0; j < len; j++)
      info[j].mask |= mask;
  }
  HB_INTERNAL void set_masks (hb_mask_t value,
			      hb_mask_t mask,
			      unsigned int cluster_start,
			      unsigned int cluster_end);

  HB_INTERNAL void merge_clusters (unsigned int start,
				   unsigned int end);
  HB_INTERNAL void merge_out_clusters (unsigned int start,
				       unsigned int end);

  
  HB_INTERNAL bool enlarge (unsigned int size);

  inline bool ensure (unsigned int size)
  { return likely (size < allocated) ? true : enlarge (size); }

  HB_INTERNAL bool make_room_for (unsigned int num_in, unsigned int num_out);

  HB_INTERNAL void *get_scratch_buffer (unsigned int *size);
};


#define HB_BUFFER_XALLOCATE_VAR(b, func, var, owner) \
  b->func (offsetof (hb_glyph_info_t, var) - offsetof(hb_glyph_info_t, var1), \
	   sizeof (b->info[0].var), owner)
#define HB_BUFFER_ALLOCATE_VAR(b, var) \
	HB_BUFFER_XALLOCATE_VAR (b, allocate_var, var (), #var)
#define HB_BUFFER_DEALLOCATE_VAR(b, var) \
	HB_BUFFER_XALLOCATE_VAR (b, deallocate_var, var (), #var)


#endif 
