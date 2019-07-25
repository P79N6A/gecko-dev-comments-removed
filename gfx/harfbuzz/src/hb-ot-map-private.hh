



























#ifndef HB_OT_MAP_PRIVATE_HH
#define HB_OT_MAP_PRIVATE_HH

#include "hb-buffer-private.hh"

#include "hb-ot-layout.h"

HB_BEGIN_DECLS


#define MAX_FEATURES 100 /* FIXME */
#define MAX_LOOKUPS 1000 /* FIXME */



#define FIRST_PRIORITY   100
#define EARLY_PRIORITY   300
#define DEFAULT_PRIORITY 500
#define LATE_PRIORITY    700
#define LAST_PRIORITY    900

static const hb_tag_t table_tags[2] = {HB_OT_TAG_GSUB, HB_OT_TAG_GPOS};

struct hb_ot_map_t {

  private:

  struct feature_info_t {
    hb_tag_t tag;
    unsigned int seq; 
    unsigned int max_value;
    bool global; 
    unsigned short default_value; 
    unsigned short priority; 

    static int cmp (const feature_info_t *a, const feature_info_t *b)
    { return (a->tag != b->tag) ?  (a->tag < b->tag ? -1 : 1) : (a->seq < b->seq ? -1 : 1); }
  };

  struct feature_map_t {
    hb_tag_t tag; 
    unsigned int index[2]; 
    unsigned short shift;
    unsigned short priority;
    hb_mask_t mask;
    hb_mask_t _1_mask; 

    static int cmp (const feature_map_t *a, const feature_map_t *b)
    { return a->tag < b->tag ? -1 : a->tag > b->tag ? 1 : 0; }
  };

  struct lookup_map_t {
    unsigned short index;
    unsigned short priority;
    hb_mask_t mask;

    static int cmp (const lookup_map_t *a, const lookup_map_t *b)
    {
      unsigned int a_key = (a->priority << 16) + a->index;
      unsigned int b_key = (b->priority << 16) + b->index;
      return a_key < b_key ? -1 : a_key > b_key ? 1 : 0;
    }
  };

  HB_INTERNAL void add_lookups (hb_face_t    *face,
				unsigned int  table_index,
				unsigned int  feature_index,
				unsigned short priority,
				hb_mask_t     mask);


  public:

  hb_ot_map_t (void) : feature_count (0) {}

  void add_feature (hb_tag_t tag, unsigned int value, unsigned short priority, bool global)
  {
    feature_info_t *info = &feature_infos[feature_count++];
    info->tag = tag;
    info->seq = feature_count;
    info->max_value = value;
    info->global = global;
    info->default_value = global ? value : 0;
    info->priority = priority;
  }

  inline void add_bool_feature (hb_tag_t tag, unsigned short priority, bool global = true)
  { add_feature (tag, 1, priority, global); }

  HB_INTERNAL void compile (hb_face_t *face,
			    const hb_segment_properties_t *props);

  inline hb_mask_t get_global_mask (void) const { return global_mask; }

  inline hb_mask_t get_mask (hb_tag_t tag, unsigned short *shift = NULL) const {
    const feature_map_t *map = (const feature_map_t *) bsearch (&tag, feature_maps, feature_count, sizeof (feature_maps[0]), (hb_compare_func_t) feature_map_t::cmp);
    if (shift) *shift = map ? map->shift : 0;
    return map ? map->mask : 0;
  }

  inline hb_mask_t get_1_mask (hb_tag_t tag) const {
    const feature_map_t *map = (const feature_map_t *) bsearch (&tag, feature_maps, feature_count, sizeof (feature_maps[0]), (hb_compare_func_t) feature_map_t::cmp);
    return map ? map->_1_mask : 0;
  }

  inline void substitute (hb_face_t *face, hb_buffer_t *buffer) const {
    for (unsigned int i = 0; i < lookup_count[0]; i++)
      hb_ot_layout_substitute_lookup (face, buffer, lookup_maps[0][i].index, lookup_maps[0][i].mask);
  }

  inline void position (hb_font_t *font, hb_face_t *face, hb_buffer_t *buffer) const {
    for (unsigned int i = 0; i < lookup_count[1]; i++)
      hb_ot_layout_position_lookup (font, face, buffer, lookup_maps[1][i].index, lookup_maps[1][i].mask);
  }

  private:

  hb_mask_t global_mask;

  unsigned int feature_count;
  feature_info_t feature_infos[MAX_FEATURES]; 
  feature_map_t feature_maps[MAX_FEATURES];

  lookup_map_t lookup_maps[2][MAX_LOOKUPS]; 
  unsigned int lookup_count[2];
};



HB_END_DECLS

#endif 
