



























#ifndef HB_OT_MAP_PRIVATE_HH
#define HB_OT_MAP_PRIVATE_HH

#include "hb-buffer-private.hh"

#include "hb-ot-layout-private.hh"


static const hb_tag_t table_tags[2] = {HB_OT_TAG_GSUB, HB_OT_TAG_GPOS};

struct hb_ot_map_t
{
  friend struct hb_ot_map_builder_t;

  public:

  struct feature_map_t {
    hb_tag_t tag; 
    unsigned int index[2]; 
    unsigned int stage[2]; 
    unsigned int shift;
    hb_mask_t mask;
    hb_mask_t _1_mask; 

    static int cmp (const feature_map_t *a, const feature_map_t *b)
    { return a->tag < b->tag ? -1 : a->tag > b->tag ? 1 : 0; }
  };

  struct lookup_map_t {
    unsigned int index;
    hb_mask_t mask;

    static int cmp (const lookup_map_t *a, const lookup_map_t *b)
    { return a->index < b->index ? -1 : a->index > b->index ? 1 : 0; }
  };

  typedef void (*pause_func_t) (const struct hb_ot_shape_plan_t *plan, hb_font_t *font, hb_buffer_t *buffer);

  struct pause_map_t {
    unsigned int num_lookups; 
    pause_func_t callback;
  };


  hb_ot_map_t (void) { memset (this, 0, sizeof (*this)); }

  inline hb_mask_t get_global_mask (void) const { return global_mask; }

  inline hb_mask_t get_mask (hb_tag_t feature_tag, unsigned int *shift = NULL) const {
    const feature_map_t *map = features.bsearch (&feature_tag);
    if (shift) *shift = map ? map->shift : 0;
    return map ? map->mask : 0;
  }

  inline hb_mask_t get_1_mask (hb_tag_t feature_tag) const {
    const feature_map_t *map = features.bsearch (&feature_tag);
    return map ? map->_1_mask : 0;
  }

  inline unsigned int get_feature_index (unsigned int table_index, hb_tag_t feature_tag) const {
    const feature_map_t *map = features.bsearch (&feature_tag);
    return map ? map->index[table_index] : HB_OT_LAYOUT_NO_FEATURE_INDEX;
  }

  inline unsigned int get_feature_stage (unsigned int table_index, hb_tag_t feature_tag) const {
    const feature_map_t *map = features.bsearch (&feature_tag);
    return map ? map->stage[table_index] : (unsigned int) -1;
  }

  inline void get_stage_lookups (unsigned int table_index, unsigned int stage,
				 const struct lookup_map_t **plookups, unsigned int *lookup_count) const {
    if (unlikely (stage == (unsigned int) -1)) {
      *plookups = NULL;
      *lookup_count = 0;
      return;
    }
    assert (stage <= pauses[table_index].len);
    unsigned int start = stage ? pauses[table_index][stage - 1].num_lookups : 0;
    unsigned int end   = stage < pauses[table_index].len ? pauses[table_index][stage].num_lookups : lookups[table_index].len;
    *plookups = &lookups[table_index][start];
    *lookup_count = end - start;
  }

  inline hb_tag_t get_chosen_script (unsigned int table_index) const
  { return chosen_script[table_index]; }

  HB_INTERNAL void substitute_closure (const struct hb_ot_shape_plan_t *plan, hb_face_t *face, hb_set_t *glyphs) const;
  HB_INTERNAL void substitute (const struct hb_ot_shape_plan_t *plan, hb_font_t *font, hb_buffer_t *buffer) const;
  HB_INTERNAL void position (const struct hb_ot_shape_plan_t *plan, hb_font_t *font, hb_buffer_t *buffer) const;

  inline void finish (void) {
    features.finish ();
    lookups[0].finish ();
    lookups[1].finish ();
    pauses[0].finish ();
    pauses[1].finish ();
  }


  private:

  HB_INTERNAL void add_lookups (hb_face_t    *face,
				unsigned int  table_index,
				unsigned int  feature_index,
				hb_mask_t     mask);

  hb_mask_t global_mask;

  hb_tag_t chosen_script[2];
  hb_prealloced_array_t<feature_map_t, 8> features;
  hb_prealloced_array_t<lookup_map_t, 32> lookups[2]; 
  hb_prealloced_array_t<pause_map_t, 1> pauses[2]; 
};


struct hb_ot_map_builder_t
{
  public:

  hb_ot_map_builder_t (void) { memset (this, 0, sizeof (*this)); }

  HB_INTERNAL void add_feature (hb_tag_t tag, unsigned int value, bool global);

  inline void add_bool_feature (hb_tag_t tag, bool global = true)
  { add_feature (tag, 1, global); }

  inline void add_gsub_pause (hb_ot_map_t::pause_func_t pause_func)
  { add_pause (0, pause_func); }
  inline void add_gpos_pause (hb_ot_map_t::pause_func_t pause_func)
  { add_pause (1, pause_func); }

  HB_INTERNAL void compile (hb_face_t *face,
			    const hb_segment_properties_t *props,
			    struct hb_ot_map_t &m);

  inline void finish (void) {
    feature_infos.finish ();
    pauses[0].finish ();
    pauses[1].finish ();
  }

  private:

  struct feature_info_t {
    hb_tag_t tag;
    unsigned int seq; 
    unsigned int max_value;
    bool global; 
    unsigned int default_value; 
    unsigned int stage[2]; 

    static int cmp (const feature_info_t *a, const feature_info_t *b)
    { return (a->tag != b->tag) ?  (a->tag < b->tag ? -1 : 1) : (a->seq < b->seq ? -1 : 1); }
  };

  struct pause_info_t {
    unsigned int stage;
    hb_ot_map_t::pause_func_t callback;
  };

  HB_INTERNAL void add_pause (unsigned int table_index, hb_ot_map_t::pause_func_t pause_func);

  unsigned int current_stage[2]; 
  hb_prealloced_array_t<feature_info_t,16> feature_infos;
  hb_prealloced_array_t<pause_info_t, 1> pauses[2]; 
};



#endif 
