

























#ifndef HB_OT_SHAPE_PRIVATE_HH
#define HB_OT_SHAPE_PRIVATE_HH

#include "hb-private.hh"

#include "hb-ot-map-private.hh"
#include "hb-ot-layout-private.hh"



struct hb_ot_shape_plan_t
{
  hb_segment_properties_t props;
  const struct hb_ot_complex_shaper_t *shaper;
  hb_ot_map_t map;
  const void *data;
  hb_mask_t rtlm_mask, frac_mask, numr_mask, dnom_mask;
  hb_mask_t kern_mask;
  unsigned int has_frac : 1;
  unsigned int has_kern : 1;
  unsigned int has_mark : 1;

  inline void collect_lookups (hb_tag_t table_tag, hb_set_t *lookups) const
  {
    unsigned int table_index;
    switch (table_tag) {
      case HB_OT_TAG_GSUB: table_index = 0; break;
      case HB_OT_TAG_GPOS: table_index = 1; break;
      default: return;
    }
    map.collect_lookups (table_index, lookups);
  }
  inline void substitute (hb_font_t *font, hb_buffer_t *buffer) const { map.substitute (this, font, buffer); }
  inline void position (hb_font_t *font, hb_buffer_t *buffer) const { map.position (this, font, buffer); }

  void finish (void) { map.finish (); }
};

struct hb_ot_shape_planner_t
{
  
  hb_face_t *face;
  hb_segment_properties_t props;
  const struct hb_ot_complex_shaper_t *shaper;
  hb_ot_map_builder_t map;

  hb_ot_shape_planner_t (const hb_shape_plan_t *master_plan) :
			 face (master_plan->face_unsafe),
			 props (master_plan->props),
			 shaper (NULL),
			 map (face, &props) {}
  ~hb_ot_shape_planner_t (void) { map.finish (); }

  inline void compile (hb_ot_shape_plan_t &plan)
  {
    plan.props = props;
    plan.shaper = shaper;
    map.compile (plan.map);

    plan.rtlm_mask = plan.map.get_1_mask (HB_TAG ('r','t','l','m'));
    plan.frac_mask = plan.map.get_1_mask (HB_TAG ('f','r','a','c'));
    plan.numr_mask = plan.map.get_1_mask (HB_TAG ('n','u','m','r'));
    plan.dnom_mask = plan.map.get_1_mask (HB_TAG ('d','n','o','m'));

    plan.kern_mask = plan.map.get_mask (HB_DIRECTION_IS_HORIZONTAL (plan.props.direction) ?
					HB_TAG ('k','e','r','n') : HB_TAG ('v','k','r','n'));

    plan.has_frac = plan.frac_mask || (plan.numr_mask && plan.dnom_mask);
    plan.has_kern = !!plan.kern_mask;
    plan.has_mark = !!plan.map.get_1_mask (HB_TAG ('m','a','r','k'));
  }

  private:
  NO_COPY (hb_ot_shape_planner_t);
};


#endif 
