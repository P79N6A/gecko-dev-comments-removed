

























#ifndef HB_OT_SHAPE_COMPLEX_ARABIC_FALLBACK_HH
#define HB_OT_SHAPE_COMPLEX_ARABIC_FALLBACK_HH

#include "hb-private.hh"

#include "hb-ot-shape-private.hh"
#include "hb-ot-layout-gsub-table.hh"


static const hb_tag_t arabic_fallback_features[] =
{
  HB_TAG('i','n','i','t'),
  HB_TAG('m','e','d','i'),
  HB_TAG('f','i','n','a'),
  HB_TAG('i','s','o','l'),
  HB_TAG('r','l','i','g'),
};


enum {
  FALLBACK_INIT,
  FALLBACK_MEDI,
  FALLBACK_FINA,
  FALLBACK_ISOL,
  FALLBACK_RLIG,
  ARABIC_NUM_FALLBACK_FEATURES
};

static OT::SubstLookup *
arabic_fallback_synthesize_lookup_single (const hb_ot_shape_plan_t *plan HB_UNUSED,
					  hb_font_t *font,
					  unsigned int feature_index)
{
  OT::GlyphID glyphs[SHAPING_TABLE_LAST - SHAPING_TABLE_FIRST + 1];
  OT::GlyphID substitutes[SHAPING_TABLE_LAST - SHAPING_TABLE_FIRST + 1];
  unsigned int num_glyphs = 0;

  
  for (hb_codepoint_t u = SHAPING_TABLE_FIRST; u < SHAPING_TABLE_LAST + 1; u++)
  {
    hb_codepoint_t s = shaping_table[u - SHAPING_TABLE_FIRST][feature_index];
    hb_codepoint_t u_glyph, s_glyph;

    if (!s ||
	!hb_font_get_glyph (font, u, 0, &u_glyph) ||
	!hb_font_get_glyph (font, s, 0, &s_glyph) ||
	u_glyph == s_glyph ||
	u_glyph > 0xFFFF || s_glyph > 0xFFFF)
      continue;

    glyphs[num_glyphs].set (u_glyph);
    substitutes[num_glyphs].set (s_glyph);

    num_glyphs++;
  }

  

  hb_bubble_sort (&glyphs[0], num_glyphs, OT::GlyphID::cmp, &substitutes[0]);

  OT::Supplier<OT::GlyphID> glyphs_supplier      (glyphs, num_glyphs);
  OT::Supplier<OT::GlyphID> substitutes_supplier (substitutes, num_glyphs);

  
  char buf[(SHAPING_TABLE_LAST - SHAPING_TABLE_FIRST + 1) * 4 + 128];
  OT::hb_serialize_context_t c (buf, sizeof (buf));
  OT::SubstLookup *lookup = c.start_serialize<OT::SubstLookup> ();
  bool ret = lookup->serialize_single (&c,
				       OT::LookupFlag::IgnoreMarks,
				       glyphs_supplier,
				       substitutes_supplier,
				       num_glyphs);
  c.end_serialize ();
  

  return ret ? c.copy<OT::SubstLookup> () : NULL;
}

static OT::SubstLookup *
arabic_fallback_synthesize_lookup_ligature (const hb_ot_shape_plan_t *plan HB_UNUSED,
					    hb_font_t *font)
{
  OT::GlyphID first_glyphs[ARRAY_LENGTH_CONST (ligature_table)];
  unsigned int first_glyphs_indirection[ARRAY_LENGTH_CONST (ligature_table)];
  unsigned int ligature_per_first_glyph_count_list[ARRAY_LENGTH_CONST (first_glyphs)];
  unsigned int num_first_glyphs = 0;

  
  OT::GlyphID ligature_list[ARRAY_LENGTH_CONST (first_glyphs) * ARRAY_LENGTH_CONST(ligature_table[0].ligatures)];
  unsigned int component_count_list[ARRAY_LENGTH_CONST (ligature_list)];
  OT::GlyphID component_list[ARRAY_LENGTH_CONST (ligature_list) * 1];
  unsigned int num_ligatures = 0;

  

  
  for (unsigned int first_glyph_idx = 0; first_glyph_idx < ARRAY_LENGTH (first_glyphs); first_glyph_idx++)
  {
    hb_codepoint_t first_u = ligature_table[first_glyph_idx].first;
    hb_codepoint_t first_glyph;
    if (!hb_font_get_glyph (font, first_u, 0, &first_glyph))
      continue;
    first_glyphs[num_first_glyphs].set (first_glyph);
    ligature_per_first_glyph_count_list[num_first_glyphs] = 0;
    first_glyphs_indirection[num_first_glyphs] = first_glyph_idx;
    num_first_glyphs++;
  }
  hb_bubble_sort (&first_glyphs[0], num_first_glyphs, OT::GlyphID::cmp, &first_glyphs_indirection[0]);

  
  for (unsigned int i = 0; i < num_first_glyphs; i++)
  {
    unsigned int first_glyph_idx = first_glyphs_indirection[i];

    for (unsigned int second_glyph_idx = 0; second_glyph_idx < ARRAY_LENGTH (ligature_table[0].ligatures); second_glyph_idx++)
    {
      hb_codepoint_t second_u   = ligature_table[first_glyph_idx].ligatures[second_glyph_idx].second;
      hb_codepoint_t ligature_u = ligature_table[first_glyph_idx].ligatures[second_glyph_idx].ligature;
      hb_codepoint_t second_glyph, ligature_glyph;
      if (!second_u ||
	  !hb_font_get_glyph (font, second_u,   0, &second_glyph) ||
	  !hb_font_get_glyph (font, ligature_u, 0, &ligature_glyph))
	continue;

      ligature_per_first_glyph_count_list[i]++;

      ligature_list[num_ligatures].set (ligature_glyph);
      component_count_list[num_ligatures] = 2;
      component_list[num_ligatures].set (second_glyph);
      num_ligatures++;
    }
  }

  OT::Supplier<OT::GlyphID>   first_glyphs_supplier                      (first_glyphs, num_first_glyphs);
  OT::Supplier<unsigned int > ligature_per_first_glyph_count_supplier    (ligature_per_first_glyph_count_list, num_first_glyphs);
  OT::Supplier<OT::GlyphID>   ligatures_supplier                         (ligature_list, num_ligatures);
  OT::Supplier<unsigned int > component_count_supplier                   (component_count_list, num_ligatures);
  OT::Supplier<OT::GlyphID>   component_supplier                         (component_list, num_ligatures);

  
  char buf[ARRAY_LENGTH_CONST (ligature_list) * 16 + 128];
  OT::hb_serialize_context_t c (buf, sizeof (buf));
  OT::SubstLookup *lookup = c.start_serialize<OT::SubstLookup> ();
  bool ret = lookup->serialize_ligature (&c,
					 OT::LookupFlag::IgnoreMarks,
					 first_glyphs_supplier,
					 ligature_per_first_glyph_count_supplier,
					 num_first_glyphs,
					 ligatures_supplier,
					 component_count_supplier,
					 component_supplier);

  c.end_serialize ();
  

  return ret ? c.copy<OT::SubstLookup> () : NULL;
}

static OT::SubstLookup *
arabic_fallback_synthesize_lookup (const hb_ot_shape_plan_t *plan,
				   hb_font_t *font,
				   unsigned int feature_index)
{
  if (feature_index < 4)
    return arabic_fallback_synthesize_lookup_single (plan, font, feature_index);
  else
    return arabic_fallback_synthesize_lookup_ligature (plan, font);
}

struct arabic_fallback_plan_t
{
  ASSERT_POD ();

  hb_mask_t mask_array[ARABIC_NUM_FALLBACK_FEATURES];
  OT::SubstLookup *lookup_array[ARABIC_NUM_FALLBACK_FEATURES];
  hb_set_digest_t digest_array[ARABIC_NUM_FALLBACK_FEATURES];
};

static const arabic_fallback_plan_t arabic_fallback_plan_nil = {};

static arabic_fallback_plan_t *
arabic_fallback_plan_create (const hb_ot_shape_plan_t *plan,
			     hb_font_t *font)
{
  arabic_fallback_plan_t *fallback_plan = (arabic_fallback_plan_t *) calloc (1, sizeof (arabic_fallback_plan_t));
  if (unlikely (!fallback_plan))
    return const_cast<arabic_fallback_plan_t *> (&arabic_fallback_plan_nil);

  for (unsigned int i = 0; i < ARABIC_NUM_FALLBACK_FEATURES; i++) {
    fallback_plan->mask_array[i] = plan->map.get_1_mask (arabic_fallback_features[i]);
    if (fallback_plan->mask_array[i]) {
      fallback_plan->lookup_array[i] = arabic_fallback_synthesize_lookup (plan, font, i);
      if (fallback_plan->lookup_array[i])
	fallback_plan->lookup_array[i]->add_coverage (&fallback_plan->digest_array[i]);
    }
  }

  return fallback_plan;
}

static void
arabic_fallback_plan_destroy (arabic_fallback_plan_t *fallback_plan)
{
  if (!fallback_plan || fallback_plan == &arabic_fallback_plan_nil)
    return;

  for (unsigned int i = 0; i < ARABIC_NUM_FALLBACK_FEATURES; i++)
    if (fallback_plan->lookup_array[i])
      free (fallback_plan->lookup_array[i]);

  free (fallback_plan);
}

static void
arabic_fallback_plan_shape (arabic_fallback_plan_t *fallback_plan,
			    hb_font_t *font,
			    hb_buffer_t *buffer)
{
  for (unsigned int i = 0; i < ARABIC_NUM_FALLBACK_FEATURES; i++)
    if (fallback_plan->lookup_array[i]) {
      OT::hb_apply_context_t c (font, buffer, fallback_plan->mask_array[i]);
      fallback_plan->lookup_array[i]->apply_string (&c, &fallback_plan->digest_array[i]);
    }
}


#endif 
