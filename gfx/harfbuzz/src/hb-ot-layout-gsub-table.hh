



























#ifndef HB_OT_LAYOUT_GSUB_TABLE_HH
#define HB_OT_LAYOUT_GSUB_TABLE_HH

#include "hb-ot-layout-gsubgpos-private.hh"



struct SingleSubstFormat1
{
  friend struct SingleSubst;

  private:

  inline void closure (hb_closure_context_t *c) const
  {
    TRACE_CLOSURE ();
    Coverage::Iter iter;
    for (iter.init (this+coverage); iter.more (); iter.next ()) {
      hb_codepoint_t glyph_id = iter.get_glyph ();
      if (c->glyphs->has (glyph_id))
	c->glyphs->add ((glyph_id + deltaGlyphID) & 0xFFFF);
    }
  }

  inline const Coverage &get_coverage (void) const
  {
    return this+coverage;
  }

  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    hb_codepoint_t glyph_id = c->buffer->cur().codepoint;
    unsigned int index = (this+coverage) (glyph_id);
    if (likely (index == NOT_COVERED)) return TRACE_RETURN (false);

    

    glyph_id = (glyph_id + deltaGlyphID) & 0xFFFF;
    c->replace_glyph (glyph_id);

    return TRACE_RETURN (true);
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return TRACE_RETURN (coverage.sanitize (c, this) && deltaGlyphID.sanitize (c));
  }

  protected:
  USHORT	format;			
  OffsetTo<Coverage>
		coverage;		

  SHORT		deltaGlyphID;		

  public:
  DEFINE_SIZE_STATIC (6);
};

struct SingleSubstFormat2
{
  friend struct SingleSubst;

  private:

  inline void closure (hb_closure_context_t *c) const
  {
    TRACE_CLOSURE ();
    Coverage::Iter iter;
    for (iter.init (this+coverage); iter.more (); iter.next ()) {
      if (c->glyphs->has (iter.get_glyph ()))
	c->glyphs->add (substitute[iter.get_coverage ()]);
    }
  }

  inline const Coverage &get_coverage (void) const
  {
    return this+coverage;
  }

  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    hb_codepoint_t glyph_id = c->buffer->cur().codepoint;
    unsigned int index = (this+coverage) (glyph_id);
    if (likely (index == NOT_COVERED)) return TRACE_RETURN (false);

    if (unlikely (index >= substitute.len)) return TRACE_RETURN (false);

    glyph_id = substitute[index];
    c->replace_glyph (glyph_id);

    return TRACE_RETURN (true);
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return TRACE_RETURN (coverage.sanitize (c, this) && substitute.sanitize (c));
  }

  protected:
  USHORT	format;			
  OffsetTo<Coverage>
		coverage;		

  ArrayOf<GlyphID>
		substitute;		

  public:
  DEFINE_SIZE_ARRAY (6, substitute);
};

struct SingleSubst
{
  friend struct SubstLookupSubTable;

  private:

  inline void closure (hb_closure_context_t *c) const
  {
    TRACE_CLOSURE ();
    switch (u.format) {
    case 1: u.format1.closure (c); break;
    case 2: u.format2.closure (c); break;
    default:                       break;
    }
  }

  inline const Coverage &get_coverage (void) const
  {
    switch (u.format) {
    case 1: return u.format1.get_coverage ();
    case 2: return u.format2.get_coverage ();
    default:return Null(Coverage);
    }
  }

  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    switch (u.format) {
    case 1: return TRACE_RETURN (u.format1.apply (c));
    case 2: return TRACE_RETURN (u.format2.apply (c));
    default:return TRACE_RETURN (false);
    }
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    if (!u.format.sanitize (c)) return TRACE_RETURN (false);
    switch (u.format) {
    case 1: return TRACE_RETURN (u.format1.sanitize (c));
    case 2: return TRACE_RETURN (u.format2.sanitize (c));
    default:return TRACE_RETURN (true);
    }
  }

  protected:
  union {
  USHORT		format;		
  SingleSubstFormat1	format1;
  SingleSubstFormat2	format2;
  } u;
};


struct Sequence
{
  friend struct MultipleSubstFormat1;

  private:

  inline void closure (hb_closure_context_t *c) const
  {
    TRACE_CLOSURE ();
    unsigned int count = substitute.len;
    for (unsigned int i = 0; i < count; i++)
      c->glyphs->add (substitute[i]);
  }

  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    if (unlikely (!substitute.len)) return TRACE_RETURN (false);

    unsigned int klass = c->property & HB_OT_LAYOUT_GLYPH_CLASS_LIGATURE ? HB_OT_LAYOUT_GLYPH_CLASS_BASE_GLYPH : 0;
    unsigned int count = substitute.len;
    for (unsigned int i = 0; i < count; i++) {
      set_lig_props_for_component (c->buffer->cur(), i);
      c->output_glyph (substitute.array[i], klass);
    }
    c->buffer->skip_glyph ();

    return TRACE_RETURN (true);
  }

  public:
  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return TRACE_RETURN (substitute.sanitize (c));
  }

  protected:
  ArrayOf<GlyphID>
		substitute;		
  public:
  DEFINE_SIZE_ARRAY (2, substitute);
};

struct MultipleSubstFormat1
{
  friend struct MultipleSubst;

  private:

  inline void closure (hb_closure_context_t *c) const
  {
    TRACE_CLOSURE ();
    Coverage::Iter iter;
    for (iter.init (this+coverage); iter.more (); iter.next ()) {
      if (c->glyphs->has (iter.get_glyph ()))
	(this+sequence[iter.get_coverage ()]).closure (c);
    }
  }

  inline const Coverage &get_coverage (void) const
  {
    return this+coverage;
  }

  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();

    unsigned int index = (this+coverage) (c->buffer->cur().codepoint);
    if (likely (index == NOT_COVERED)) return TRACE_RETURN (false);

    return TRACE_RETURN ((this+sequence[index]).apply (c));
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return TRACE_RETURN (coverage.sanitize (c, this) && sequence.sanitize (c, this));
  }

  protected:
  USHORT	format;			
  OffsetTo<Coverage>
		coverage;		

  OffsetArrayOf<Sequence>
		sequence;		

  public:
  DEFINE_SIZE_ARRAY (6, sequence);
};

struct MultipleSubst
{
  friend struct SubstLookupSubTable;

  private:

  inline void closure (hb_closure_context_t *c) const
  {
    TRACE_CLOSURE ();
    switch (u.format) {
    case 1: u.format1.closure (c); break;
    default:                       break;
    }
  }

  inline const Coverage &get_coverage (void) const
  {
    switch (u.format) {
    case 1: return u.format1.get_coverage ();
    default:return Null(Coverage);
    }
  }

  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    switch (u.format) {
    case 1: return TRACE_RETURN (u.format1.apply (c));
    default:return TRACE_RETURN (false);
    }
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    if (!u.format.sanitize (c)) return TRACE_RETURN (false);
    switch (u.format) {
    case 1: return TRACE_RETURN (u.format1.sanitize (c));
    default:return TRACE_RETURN (true);
    }
  }

  protected:
  union {
  USHORT		format;		
  MultipleSubstFormat1	format1;
  } u;
};


typedef ArrayOf<GlyphID> AlternateSet;	


struct AlternateSubstFormat1
{
  friend struct AlternateSubst;

  private:

  inline void closure (hb_closure_context_t *c) const
  {
    TRACE_CLOSURE ();
    Coverage::Iter iter;
    for (iter.init (this+coverage); iter.more (); iter.next ()) {
      if (c->glyphs->has (iter.get_glyph ())) {
	const AlternateSet &alt_set = this+alternateSet[iter.get_coverage ()];
	unsigned int count = alt_set.len;
	for (unsigned int i = 0; i < count; i++)
	  c->glyphs->add (alt_set[i]);
      }
    }
  }

  inline const Coverage &get_coverage (void) const
  {
    return this+coverage;
  }

  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    hb_codepoint_t glyph_id = c->buffer->cur().codepoint;

    unsigned int index = (this+coverage) (glyph_id);
    if (likely (index == NOT_COVERED)) return TRACE_RETURN (false);

    const AlternateSet &alt_set = this+alternateSet[index];

    if (unlikely (!alt_set.len)) return TRACE_RETURN (false);

    hb_mask_t glyph_mask = c->buffer->cur().mask;
    hb_mask_t lookup_mask = c->lookup_mask;

    
    unsigned int shift = _hb_ctz (lookup_mask);
    unsigned int alt_index = ((lookup_mask & glyph_mask) >> shift);

    if (unlikely (alt_index > alt_set.len || alt_index == 0)) return TRACE_RETURN (false);

    glyph_id = alt_set[alt_index - 1];

    c->replace_glyph (glyph_id);

    return TRACE_RETURN (true);
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return TRACE_RETURN (coverage.sanitize (c, this) && alternateSet.sanitize (c, this));
  }

  protected:
  USHORT	format;			
  OffsetTo<Coverage>
		coverage;		

  OffsetArrayOf<AlternateSet>
		alternateSet;		

  public:
  DEFINE_SIZE_ARRAY (6, alternateSet);
};

struct AlternateSubst
{
  friend struct SubstLookupSubTable;

  private:

  inline void closure (hb_closure_context_t *c) const
  {
    TRACE_CLOSURE ();
    switch (u.format) {
    case 1: u.format1.closure (c); break;
    default:                       break;
    }
  }

  inline const Coverage &get_coverage (void) const
  {
    switch (u.format) {
    case 1: return u.format1.get_coverage ();
    default:return Null(Coverage);
    }
  }

  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    switch (u.format) {
    case 1: return TRACE_RETURN (u.format1.apply (c));
    default:return TRACE_RETURN (false);
    }
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    if (!u.format.sanitize (c)) return TRACE_RETURN (false);
    switch (u.format) {
    case 1: return TRACE_RETURN (u.format1.sanitize (c));
    default:return TRACE_RETURN (true);
    }
  }

  protected:
  union {
  USHORT		format;		
  AlternateSubstFormat1	format1;
  } u;
};


struct Ligature
{
  friend struct LigatureSet;

  private:

  inline void closure (hb_closure_context_t *c) const
  {
    TRACE_CLOSURE ();
    unsigned int count = component.len;
    for (unsigned int i = 1; i < count; i++)
      if (!c->glyphs->has (component[i]))
        return;
    c->glyphs->add (ligGlyph);
  }

  inline bool would_apply (hb_would_apply_context_t *c) const
  {
    if (c->len != component.len)
      return false;

    for (unsigned int i = 1; i < c->len; i++)
      if (likely (c->glyphs[i] != component[i]))
	return false;

    return true;
  }

  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    unsigned int count = component.len;
    if (unlikely (count < 1)) return TRACE_RETURN (false);

    hb_apply_context_t::mark_skipping_forward_iterator_t skippy_iter (c, c->buffer->idx, count - 1);
    if (skippy_iter.has_no_chance ()) return TRACE_RETURN (false);

    











































    bool is_mark_ligature = !!(c->property & HB_OT_LAYOUT_GLYPH_CLASS_MARK);

    unsigned int total_component_count = 0;
    total_component_count += get_lig_num_comps (c->buffer->cur());

    unsigned int first_lig_id = get_lig_id (c->buffer->cur());
    unsigned int first_lig_comp = get_lig_comp (c->buffer->cur());

    for (unsigned int i = 1; i < count; i++)
    {
      unsigned int property;

      if (!skippy_iter.next (&property)) return TRACE_RETURN (false);

      if (likely (c->buffer->info[skippy_iter.idx].codepoint != component[i])) return TRACE_RETURN (false);

      unsigned int this_lig_id = get_lig_id (c->buffer->info[skippy_iter.idx]);
      unsigned int this_lig_comp = get_lig_comp (c->buffer->info[skippy_iter.idx]);

      if (first_lig_id && first_lig_comp) {
        


        if (first_lig_id != this_lig_id || first_lig_comp != this_lig_comp)
	  return TRACE_RETURN (false);
      } else {
        


        if (this_lig_id && this_lig_comp && (this_lig_id != first_lig_id))
	  return TRACE_RETURN (false);
      }

      is_mark_ligature = is_mark_ligature && (property & HB_OT_LAYOUT_GLYPH_CLASS_MARK);
      total_component_count += get_lig_num_comps (c->buffer->info[skippy_iter.idx]);
    }

    
    c->buffer->merge_clusters (c->buffer->idx, skippy_iter.idx + 1);

    unsigned int klass = is_mark_ligature ? 0 : HB_OT_LAYOUT_GLYPH_CLASS_LIGATURE;
    unsigned int lig_id = is_mark_ligature ? 0 : allocate_lig_id (c->buffer);
    unsigned int last_lig_id = get_lig_id (c->buffer->cur());
    unsigned int last_num_components = get_lig_num_comps (c->buffer->cur());
    unsigned int components_so_far = last_num_components;

    if (!is_mark_ligature)
      set_lig_props_for_ligature (c->buffer->cur(), lig_id, total_component_count);
    c->replace_glyph (ligGlyph, klass);

    for (unsigned int i = 1; i < count; i++)
    {
      while (c->should_mark_skip_current_glyph ())
      {
	if (!is_mark_ligature) {
	  unsigned int new_lig_comp = components_so_far - last_num_components +
				      MIN (MAX (get_lig_comp (c->buffer->cur()), 1u), last_num_components);
	  set_lig_props_for_mark (c->buffer->cur(), lig_id, new_lig_comp);
	}
	c->buffer->next_glyph ();
      }

      last_lig_id = get_lig_id (c->buffer->cur());
      last_num_components = get_lig_num_comps (c->buffer->cur());
      components_so_far += last_num_components;

      
      c->buffer->idx++;
    }

    if (!is_mark_ligature && last_lig_id) {
      
      for (unsigned int i = c->buffer->idx; i < c->buffer->len; i++) {
	if (last_lig_id == get_lig_id (c->buffer->info[i])) {
	  unsigned int new_lig_comp = components_so_far - last_num_components +
				      MIN (MAX (get_lig_comp (c->buffer->info[i]), 1u), last_num_components);
	  set_lig_props_for_mark (c->buffer->info[i], lig_id, new_lig_comp);
	} else
	  break;
      }
    }

    return TRACE_RETURN (true);
  }

  public:
  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return TRACE_RETURN (ligGlyph.sanitize (c) && component.sanitize (c));
  }

  protected:
  GlyphID	ligGlyph;		
  HeadlessArrayOf<GlyphID>
		component;		


  public:
  DEFINE_SIZE_ARRAY (4, component);
};

struct LigatureSet
{
  friend struct LigatureSubstFormat1;

  private:

  inline void closure (hb_closure_context_t *c) const
  {
    TRACE_CLOSURE ();
    unsigned int num_ligs = ligature.len;
    for (unsigned int i = 0; i < num_ligs; i++)
      (this+ligature[i]).closure (c);
  }

  inline bool would_apply (hb_would_apply_context_t *c) const
  {
    unsigned int num_ligs = ligature.len;
    for (unsigned int i = 0; i < num_ligs; i++)
    {
      const Ligature &lig = this+ligature[i];
      if (lig.would_apply (c))
        return true;
    }
    return false;
  }

  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    unsigned int num_ligs = ligature.len;
    for (unsigned int i = 0; i < num_ligs; i++)
    {
      const Ligature &lig = this+ligature[i];
      if (lig.apply (c)) return TRACE_RETURN (true);
    }

    return TRACE_RETURN (false);
  }

  public:
  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return TRACE_RETURN (ligature.sanitize (c, this));
  }

  protected:
  OffsetArrayOf<Ligature>
		ligature;		

  public:
  DEFINE_SIZE_ARRAY (2, ligature);
};

struct LigatureSubstFormat1
{
  friend struct LigatureSubst;

  private:

  inline void closure (hb_closure_context_t *c) const
  {
    TRACE_CLOSURE ();
    Coverage::Iter iter;
    for (iter.init (this+coverage); iter.more (); iter.next ()) {
      if (c->glyphs->has (iter.get_glyph ()))
	(this+ligatureSet[iter.get_coverage ()]).closure (c);
    }
  }

  inline const Coverage &get_coverage (void) const
  {
    return this+coverage;
  }

  inline bool would_apply (hb_would_apply_context_t *c) const
  {
    return (this+ligatureSet[(this+coverage) (c->glyphs[0])]).would_apply (c);
  }

  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    hb_codepoint_t glyph_id = c->buffer->cur().codepoint;

    unsigned int index = (this+coverage) (glyph_id);
    if (likely (index == NOT_COVERED)) return TRACE_RETURN (false);

    const LigatureSet &lig_set = this+ligatureSet[index];
    return TRACE_RETURN (lig_set.apply (c));
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return TRACE_RETURN (coverage.sanitize (c, this) && ligatureSet.sanitize (c, this));
  }

  protected:
  USHORT	format;			
  OffsetTo<Coverage>
		coverage;		

  OffsetArrayOf<LigatureSet>
		ligatureSet;		

  public:
  DEFINE_SIZE_ARRAY (6, ligatureSet);
};

struct LigatureSubst
{
  friend struct SubstLookupSubTable;

  private:

  inline void closure (hb_closure_context_t *c) const
  {
    TRACE_CLOSURE ();
    switch (u.format) {
    case 1: u.format1.closure (c); break;
    default:                       break;
    }
  }

  inline const Coverage &get_coverage (void) const
  {
    switch (u.format) {
    case 1: return u.format1.get_coverage ();
    default:return Null(Coverage);
    }
  }

  inline bool would_apply (hb_would_apply_context_t *c) const
  {
    switch (u.format) {
    case 1: return u.format1.would_apply (c);
    default:return false;
    }
  }

  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    switch (u.format) {
    case 1: return TRACE_RETURN (u.format1.apply (c));
    default:return TRACE_RETURN (false);
    }
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    if (!u.format.sanitize (c)) return TRACE_RETURN (false);
    switch (u.format) {
    case 1: return TRACE_RETURN (u.format1.sanitize (c));
    default:return TRACE_RETURN (true);
    }
  }

  protected:
  union {
  USHORT		format;		
  LigatureSubstFormat1	format1;
  } u;
};


static inline bool substitute_lookup (hb_apply_context_t *c, unsigned int lookup_index);
static inline void closure_lookup (hb_closure_context_t *c, unsigned int lookup_index);

struct ContextSubst : Context
{
  friend struct SubstLookupSubTable;

  private:

  inline void closure (hb_closure_context_t *c) const
  {
    TRACE_CLOSURE ();
    return Context::closure (c, closure_lookup);
  }

  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    return TRACE_RETURN (Context::apply (c, substitute_lookup));
  }
};

struct ChainContextSubst : ChainContext
{
  friend struct SubstLookupSubTable;

  private:

  inline void closure (hb_closure_context_t *c) const
  {
    TRACE_CLOSURE ();
    return ChainContext::closure (c, closure_lookup);
  }

  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    return TRACE_RETURN (ChainContext::apply (c, substitute_lookup));
  }
};


struct ExtensionSubst : Extension
{
  friend struct SubstLookupSubTable;
  friend struct SubstLookup;

  private:
  inline const struct SubstLookupSubTable& get_subtable (void) const
  {
    unsigned int offset = get_offset ();
    if (unlikely (!offset)) return Null(SubstLookupSubTable);
    return StructAtOffset<SubstLookupSubTable> (this, offset);
  }

  inline void closure (hb_closure_context_t *c) const;

  inline const Coverage &get_coverage (void) const;

  inline bool would_apply (hb_would_apply_context_t *c) const;

  inline bool apply (hb_apply_context_t *c) const;

  inline bool sanitize (hb_sanitize_context_t *c);

  inline bool is_reverse (void) const;
};


struct ReverseChainSingleSubstFormat1
{
  friend struct ReverseChainSingleSubst;

  private:

  inline void closure (hb_closure_context_t *c) const
  {
    TRACE_CLOSURE ();
    const OffsetArrayOf<Coverage> &lookahead = StructAfter<OffsetArrayOf<Coverage> > (backtrack);

    unsigned int count;

    count = backtrack.len;
    for (unsigned int i = 0; i < count; i++)
      if (!(this+backtrack[i]).intersects (c->glyphs))
        return;

    count = lookahead.len;
    for (unsigned int i = 0; i < count; i++)
      if (!(this+lookahead[i]).intersects (c->glyphs))
        return;

    const ArrayOf<GlyphID> &substitute = StructAfter<ArrayOf<GlyphID> > (lookahead);
    Coverage::Iter iter;
    for (iter.init (this+coverage); iter.more (); iter.next ()) {
      if (c->glyphs->has (iter.get_glyph ()))
	c->glyphs->add (substitute[iter.get_coverage ()]);
    }
  }

  inline const Coverage &get_coverage (void) const
  {
    return this+coverage;
  }

  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    if (unlikely (c->nesting_level_left != MAX_NESTING_LEVEL))
      return TRACE_RETURN (false); 

    unsigned int index = (this+coverage) (c->buffer->cur().codepoint);
    if (likely (index == NOT_COVERED)) return TRACE_RETURN (false);

    const OffsetArrayOf<Coverage> &lookahead = StructAfter<OffsetArrayOf<Coverage> > (backtrack);
    const ArrayOf<GlyphID> &substitute = StructAfter<ArrayOf<GlyphID> > (lookahead);

    if (match_backtrack (c,
			 backtrack.len, (USHORT *) backtrack.array,
			 match_coverage, this) &&
        match_lookahead (c,
			 lookahead.len, (USHORT *) lookahead.array,
			 match_coverage, this,
			 1))
    {
      c->replace_glyph_inplace (substitute[index]);
      c->buffer->idx--; 
      return TRACE_RETURN (true);
    }

    return TRACE_RETURN (false);
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    if (!(coverage.sanitize (c, this) && backtrack.sanitize (c, this)))
      return TRACE_RETURN (false);
    OffsetArrayOf<Coverage> &lookahead = StructAfter<OffsetArrayOf<Coverage> > (backtrack);
    if (!lookahead.sanitize (c, this))
      return TRACE_RETURN (false);
    ArrayOf<GlyphID> &substitute = StructAfter<ArrayOf<GlyphID> > (lookahead);
    return TRACE_RETURN (substitute.sanitize (c));
  }

  protected:
  USHORT	format;			
  OffsetTo<Coverage>
		coverage;		

  OffsetArrayOf<Coverage>
		backtrack;		


  OffsetArrayOf<Coverage>
		lookaheadX;		


  ArrayOf<GlyphID>
		substituteX;		

  public:
  DEFINE_SIZE_MIN (10);
};

struct ReverseChainSingleSubst
{
  friend struct SubstLookupSubTable;

  private:

  inline void closure (hb_closure_context_t *c) const
  {
    TRACE_CLOSURE ();
    switch (u.format) {
    case 1: u.format1.closure (c); break;
    default:                       break;
    }
  }

  inline const Coverage &get_coverage (void) const
  {
    switch (u.format) {
    case 1: return u.format1.get_coverage ();
    default:return Null(Coverage);
    }
  }

  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    switch (u.format) {
    case 1: return TRACE_RETURN (u.format1.apply (c));
    default:return TRACE_RETURN (false);
    }
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    if (!u.format.sanitize (c)) return TRACE_RETURN (false);
    switch (u.format) {
    case 1: return TRACE_RETURN (u.format1.sanitize (c));
    default:return TRACE_RETURN (true);
    }
  }

  protected:
  union {
  USHORT				format;		
  ReverseChainSingleSubstFormat1	format1;
  } u;
};







struct SubstLookupSubTable
{
  friend struct SubstLookup;

  enum Type {
    Single		= 1,
    Multiple		= 2,
    Alternate		= 3,
    Ligature		= 4,
    Context		= 5,
    ChainContext	= 6,
    Extension		= 7,
    ReverseChainSingle	= 8
  };

  inline void closure (hb_closure_context_t *c,
		       unsigned int    lookup_type) const
  {
    TRACE_CLOSURE ();
    switch (lookup_type) {
    case Single:		u.single.closure (c); break;
    case Multiple:		u.multiple.closure (c); break;
    case Alternate:		u.alternate.closure (c); break;
    case Ligature:		u.ligature.closure (c); break;
    case Context:		u.context.closure (c); break;
    case ChainContext:		u.chainContext.closure (c); break;
    case Extension:		u.extension.closure (c); break;
    case ReverseChainSingle:	u.reverseChainContextSingle.closure (c); break;
    default:                    break;
    }
  }

  inline const Coverage &get_coverage (unsigned int lookup_type) const
  {
    switch (lookup_type) {
    case Single:		return u.single.get_coverage ();
    case Multiple:		return u.multiple.get_coverage ();
    case Alternate:		return u.alternate.get_coverage ();
    case Ligature:		return u.ligature.get_coverage ();
    case Context:		return u.context.get_coverage ();
    case ChainContext:		return u.chainContext.get_coverage ();
    case Extension:		return u.extension.get_coverage ();
    case ReverseChainSingle:	return u.reverseChainContextSingle.get_coverage ();
    default:			return Null(Coverage);
    }
  }

  inline bool would_apply (hb_would_apply_context_t *c,
			   unsigned int lookup_type) const
  {
    TRACE_WOULD_APPLY ();
    if (get_coverage (lookup_type).get_coverage (c->glyphs[0]) == NOT_COVERED) return false;
    if (c->len == 1) {
      switch (lookup_type) {
      case Single:
      case Multiple:
      case Alternate:
      case ReverseChainSingle:
        return true;
      }
    }

    

    switch (lookup_type) {
    case Ligature:		return u.ligature.would_apply (c);
    case Context:		return u.context.would_apply (c);
    case ChainContext:		return u.chainContext.would_apply (c);
    case Extension:		return u.extension.would_apply (c);
    default:			return false;
    }
  }

  inline bool apply (hb_apply_context_t *c, unsigned int lookup_type) const
  {
    TRACE_APPLY ();
    switch (lookup_type) {
    case Single:		return TRACE_RETURN (u.single.apply (c));
    case Multiple:		return TRACE_RETURN (u.multiple.apply (c));
    case Alternate:		return TRACE_RETURN (u.alternate.apply (c));
    case Ligature:		return TRACE_RETURN (u.ligature.apply (c));
    case Context:		return TRACE_RETURN (u.context.apply (c));
    case ChainContext:		return TRACE_RETURN (u.chainContext.apply (c));
    case Extension:		return TRACE_RETURN (u.extension.apply (c));
    case ReverseChainSingle:	return TRACE_RETURN (u.reverseChainContextSingle.apply (c));
    default:			return TRACE_RETURN (false);
    }
  }

  inline bool sanitize (hb_sanitize_context_t *c, unsigned int lookup_type) {
    TRACE_SANITIZE ();
    if (!u.header.sub_format.sanitize (c))
      return TRACE_RETURN (false);
    switch (lookup_type) {
    case Single:		return TRACE_RETURN (u.single.sanitize (c));
    case Multiple:		return TRACE_RETURN (u.multiple.sanitize (c));
    case Alternate:		return TRACE_RETURN (u.alternate.sanitize (c));
    case Ligature:		return TRACE_RETURN (u.ligature.sanitize (c));
    case Context:		return TRACE_RETURN (u.context.sanitize (c));
    case ChainContext:		return TRACE_RETURN (u.chainContext.sanitize (c));
    case Extension:		return TRACE_RETURN (u.extension.sanitize (c));
    case ReverseChainSingle:	return TRACE_RETURN (u.reverseChainContextSingle.sanitize (c));
    default:			return TRACE_RETURN (true);
    }
  }

  protected:
  union {
  struct {
    USHORT			sub_format;
  } header;
  SingleSubst			single;
  MultipleSubst			multiple;
  AlternateSubst		alternate;
  LigatureSubst			ligature;
  ContextSubst			context;
  ChainContextSubst		chainContext;
  ExtensionSubst		extension;
  ReverseChainSingleSubst	reverseChainContextSingle;
  } u;
  public:
  DEFINE_SIZE_UNION (2, header.sub_format);
};


struct SubstLookup : Lookup
{
  inline const SubstLookupSubTable& get_subtable (unsigned int i) const
  { return this+CastR<OffsetArrayOf<SubstLookupSubTable> > (subTable)[i]; }

  inline static bool lookup_type_is_reverse (unsigned int lookup_type)
  { return lookup_type == SubstLookupSubTable::ReverseChainSingle; }

  inline bool is_reverse (void) const
  {
    unsigned int type = get_type ();
    if (unlikely (type == SubstLookupSubTable::Extension))
      return CastR<ExtensionSubst> (get_subtable(0)).is_reverse ();
    return lookup_type_is_reverse (type);
  }

  inline void closure (hb_closure_context_t *c) const
  {
    unsigned int lookup_type = get_type ();
    unsigned int count = get_subtable_count ();
    for (unsigned int i = 0; i < count; i++)
      get_subtable (i).closure (c, lookup_type);
  }

  template <typename set_t>
  inline void add_coverage (set_t *glyphs) const
  {
    const Coverage *last = NULL;
    unsigned int count = get_subtable_count ();
    for (unsigned int i = 0; i < count; i++) {
      const Coverage *c = &get_subtable (i).get_coverage (get_type ());
      if (c != last) {
        c->add_coverage (glyphs);
        last = c;
      }
    }
  }

  inline bool would_apply (hb_would_apply_context_t *c) const
  {
    if (unlikely (!c->len)) return false;
    if (!c->digest.may_have (c->glyphs[0])) return false;
    unsigned int lookup_type = get_type ();
    unsigned int count = get_subtable_count ();
    for (unsigned int i = 0; i < count; i++)
      if (get_subtable (i).would_apply (c, lookup_type))
	return true;
    return false;
  }

  inline bool apply_once (hb_apply_context_t *c) const
  {
    unsigned int lookup_type = get_type ();

    if (!c->check_glyph_property (&c->buffer->cur(), c->lookup_props, &c->property))
      return false;

    unsigned int count = get_subtable_count ();
    for (unsigned int i = 0; i < count; i++)
      if (get_subtable (i).apply (c, lookup_type))
	return true;

    return false;
  }

  inline bool apply_string (hb_apply_context_t *c) const
  {
    bool ret = false;

    if (unlikely (!c->buffer->len))
      return false;

    c->set_lookup (*this);

    if (likely (!is_reverse ()))
    {
	
	c->buffer->clear_output ();
	c->buffer->idx = 0;

	while (c->buffer->idx < c->buffer->len)
	{
	  if ((c->buffer->cur().mask & c->lookup_mask) &&
	      c->digest.may_have (c->buffer->cur().codepoint) &&
	      apply_once (c))
	    ret = true;
	  else
	    c->buffer->next_glyph ();
	}
	if (ret)
	  c->buffer->swap_buffers ();
    }
    else
    {
	
	c->buffer->idx = c->buffer->len - 1;
	do
	{
	  if ((c->buffer->cur().mask & c->lookup_mask) &&
	      c->digest.may_have (c->buffer->cur().codepoint) &&
	      apply_once (c))
	    ret = true;
	  else
	    c->buffer->idx--;

	}
	while ((int) c->buffer->idx >= 0);
    }

    return ret;
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    if (unlikely (!Lookup::sanitize (c))) return TRACE_RETURN (false);
    OffsetArrayOf<SubstLookupSubTable> &list = CastR<OffsetArrayOf<SubstLookupSubTable> > (subTable);
    if (unlikely (!list.sanitize (c, this, get_type ()))) return TRACE_RETURN (false);

    if (unlikely (get_type () == SubstLookupSubTable::Extension))
    {
      




      unsigned int type = get_subtable (0).u.extension.get_type ();
      unsigned int count = get_subtable_count ();
      for (unsigned int i = 1; i < count; i++)
        if (get_subtable (i).u.extension.get_type () != type)
	  return TRACE_RETURN (false);
    }
    return TRACE_RETURN (true);
  }
};

typedef OffsetListOf<SubstLookup> SubstLookupList;





struct GSUB : GSUBGPOS
{
  static const hb_tag_t Tag	= HB_OT_TAG_GSUB;

  inline const SubstLookup& get_lookup (unsigned int i) const
  { return CastR<SubstLookup> (GSUBGPOS::get_lookup (i)); }

  template <typename set_t>
  inline void add_coverage (set_t *glyphs, unsigned int lookup_index) const
  { get_lookup (lookup_index).add_coverage (glyphs); }

  inline bool would_substitute_lookup (hb_would_apply_context_t *c, unsigned int lookup_index) const
  { return get_lookup (lookup_index).would_apply (c); }

  inline bool substitute_lookup (hb_apply_context_t *c, unsigned int lookup_index) const
  { return get_lookup (lookup_index).apply_string (c); }

  static inline void substitute_start (hb_font_t *font, hb_buffer_t *buffer);
  static inline void substitute_finish (hb_font_t *font, hb_buffer_t *buffer);

  inline void closure_lookup (hb_closure_context_t *c,
			      unsigned int          lookup_index) const
  { return get_lookup (lookup_index).closure (c); }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    if (unlikely (!GSUBGPOS::sanitize (c))) return TRACE_RETURN (false);
    OffsetTo<SubstLookupList> &list = CastR<OffsetTo<SubstLookupList> > (lookupList);
    return TRACE_RETURN (list.sanitize (c, this));
  }
  public:
  DEFINE_SIZE_STATIC (10);
};


void
GSUB::substitute_start (hb_font_t *font, hb_buffer_t *buffer)
{
  HB_BUFFER_ALLOCATE_VAR (buffer, glyph_props);
  HB_BUFFER_ALLOCATE_VAR (buffer, lig_props);
  HB_BUFFER_ALLOCATE_VAR (buffer, syllable);

  const GDEF &gdef = *hb_ot_layout_from_face (font->face)->gdef;
  unsigned int count = buffer->len;
  for (unsigned int i = 0; i < count; i++) {
    buffer->info[i].lig_props() = buffer->info[i].syllable() = 0;
    buffer->info[i].glyph_props() = gdef.get_glyph_props (buffer->info[i].codepoint);
  }
}

void
GSUB::substitute_finish (hb_font_t *font HB_UNUSED, hb_buffer_t *buffer HB_UNUSED)
{
}




inline void ExtensionSubst::closure (hb_closure_context_t *c) const
{
  get_subtable ().closure (c, get_type ());
}

inline const Coverage & ExtensionSubst::get_coverage (void) const
{
  return get_subtable ().get_coverage (get_type ());
}

inline bool ExtensionSubst::would_apply (hb_would_apply_context_t *c) const
{
  return get_subtable ().would_apply (c, get_type ());
}

inline bool ExtensionSubst::apply (hb_apply_context_t *c) const
{
  TRACE_APPLY ();
  return TRACE_RETURN (get_subtable ().apply (c, get_type ()));
}

inline bool ExtensionSubst::sanitize (hb_sanitize_context_t *c)
{
  TRACE_SANITIZE ();
  if (unlikely (!Extension::sanitize (c))) return TRACE_RETURN (false);
  unsigned int offset = get_offset ();
  if (unlikely (!offset)) return TRACE_RETURN (true);
  return TRACE_RETURN (StructAtOffset<SubstLookupSubTable> (this, offset).sanitize (c, get_type ()));
}

inline bool ExtensionSubst::is_reverse (void) const
{
  unsigned int type = get_type ();
  if (unlikely (type == SubstLookupSubTable::Extension))
    return CastR<ExtensionSubst> (get_subtable()).is_reverse ();
  return SubstLookup::lookup_type_is_reverse (type);
}

static inline void closure_lookup (hb_closure_context_t *c, unsigned int lookup_index)
{
  const GSUB &gsub = *(hb_ot_layout_from_face (c->face)->gsub);
  const SubstLookup &l = gsub.get_lookup (lookup_index);

  if (unlikely (c->nesting_level_left == 0))
    return;

  c->nesting_level_left--;
  l.closure (c);
  c->nesting_level_left++;
}

static inline bool substitute_lookup (hb_apply_context_t *c, unsigned int lookup_index)
{
  const GSUB &gsub = *(hb_ot_layout_from_face (c->face)->gsub);
  const SubstLookup &l = gsub.get_lookup (lookup_index);

  if (unlikely (c->nesting_level_left == 0))
    return false;

  hb_apply_context_t new_c (*c);
  new_c.nesting_level_left--;
  new_c.set_lookup (l);
  return l.apply_once (&new_c);
}



#endif 
