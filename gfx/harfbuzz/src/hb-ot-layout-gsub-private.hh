



























#ifndef HB_OT_LAYOUT_GSUB_PRIVATE_HH
#define HB_OT_LAYOUT_GSUB_PRIVATE_HH

#include "hb-ot-layout-gsubgpos-private.hh"

HB_BEGIN_DECLS


struct SingleSubstFormat1
{
  friend struct SingleSubst;

  private:

  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    hb_codepoint_t glyph_id = c->buffer->info[c->buffer->i].codepoint;
    unsigned int index = (this+coverage) (glyph_id);
    if (likely (index == NOT_COVERED))
      return false;

    glyph_id += deltaGlyphID;
    c->replace_glyph (glyph_id);

    return true;
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return coverage.sanitize (c, this)
	&& deltaGlyphID.sanitize (c);
  }

  private:
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

  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    hb_codepoint_t glyph_id = c->buffer->info[c->buffer->i].codepoint;
    unsigned int index = (this+coverage) (glyph_id);
    if (likely (index == NOT_COVERED))
      return false;

    if (unlikely (index >= substitute.len))
      return false;

    glyph_id = substitute[index];
    c->replace_glyph (glyph_id);

    return true;
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return coverage.sanitize (c, this)
	&& substitute.sanitize (c);
  }

  private:
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

  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    switch (u.format) {
    case 1: return u.format1.apply (c);
    case 2: return u.format2.apply (c);
    default:return false;
    }
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    if (!u.format.sanitize (c)) return false;
    switch (u.format) {
    case 1: return u.format1.sanitize (c);
    case 2: return u.format2.sanitize (c);
    default:return true;
    }
  }

  private:
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
  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    if (unlikely (!substitute.len))
      return false;

    if (c->property & HB_OT_LAYOUT_GLYPH_CLASS_LIGATURE)
      c->guess_glyph_class (HB_OT_LAYOUT_GLYPH_CLASS_BASE_GLYPH);
    c->replace_glyphs_be16 (1, substitute.len, (const uint16_t *) substitute.array);

    return true;
  }

  public:
  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return substitute.sanitize (c);
  }

  private:
  ArrayOf<GlyphID>
		substitute;		
  public:
  DEFINE_SIZE_ARRAY (2, substitute);
};

struct MultipleSubstFormat1
{
  friend struct MultipleSubst;

  private:

  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();

    unsigned int index = (this+coverage) (c->buffer->info[c->buffer->i].codepoint);
    if (likely (index == NOT_COVERED))
      return false;

    return (this+sequence[index]).apply (c);
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return coverage.sanitize (c, this)
	&& sequence.sanitize (c, this);
  }

  private:
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

  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    switch (u.format) {
    case 1: return u.format1.apply (c);
    default:return false;
    }
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    if (!u.format.sanitize (c)) return false;
    switch (u.format) {
    case 1: return u.format1.sanitize (c);
    default:return true;
    }
  }

  private:
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

  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    hb_codepoint_t glyph_id = c->buffer->info[c->buffer->i].codepoint;
    hb_mask_t glyph_mask = c->buffer->info[c->buffer->i].mask;
    hb_mask_t lookup_mask = c->lookup_mask;

    unsigned int index = (this+coverage) (glyph_id);
    if (likely (index == NOT_COVERED))
      return false;

    const AlternateSet &alt_set = this+alternateSet[index];

    if (unlikely (!alt_set.len))
      return false;

    
    unsigned int shift = _hb_ctz (lookup_mask);
    unsigned int alt_index = ((lookup_mask & glyph_mask) >> shift);

    if (unlikely (alt_index > alt_set.len || alt_index == 0))
      return false;

    glyph_id = alt_set[alt_index - 1];

    c->replace_glyph (glyph_id);

    return true;
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return coverage.sanitize (c, this)
	&& alternateSet.sanitize (c, this);
  }

  private:
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

  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    switch (u.format) {
    case 1: return u.format1.apply (c);
    default:return false;
    }
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    if (!u.format.sanitize (c)) return false;
    switch (u.format) {
    case 1: return u.format1.sanitize (c);
    default:return true;
    }
  }

  private:
  union {
  USHORT		format;		
  AlternateSubstFormat1	format1;
  } u;
};


struct Ligature
{
  friend struct LigatureSet;

  private:
  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    unsigned int i;
    unsigned int j = c->buffer->i;
    unsigned int count = component.len;
    unsigned int end = MIN (c->buffer->len, j + c->context_length);
    if (unlikely (j >= end))
      return false;

    bool first_was_mark = (c->property & HB_OT_LAYOUT_GLYPH_CLASS_MARK);
    bool found_non_mark = false;

    for (i = 1; i < count; i++)
    {
      unsigned int property;
      do
      {
	j++;
	if (unlikely (j == end))
	  return false;
      } while (_hb_ot_layout_skip_mark (c->layout->face, &c->buffer->info[j], c->lookup_props, &property));

      found_non_mark |= !(property & HB_OT_LAYOUT_GLYPH_CLASS_MARK);

      if (likely (c->buffer->info[j].codepoint != component[i]))
        return false;
    }

    if (first_was_mark && found_non_mark)
      c->guess_glyph_class (HB_OT_LAYOUT_GLYPH_CLASS_LIGATURE);

    
    unsigned int lig_id = allocate_lig_id (c->buffer);
    c->buffer->info[c->buffer->i].lig_comp() = 0;
    c->buffer->info[c->buffer->i].lig_id() = lig_id;

    if (j == c->buffer->i + i) 
    {
      c->replace_glyphs_be16 (i, 1, (const uint16_t *) &ligGlyph);
    }
    else
    {
      c->replace_glyph (ligGlyph);

      






      for (i = 1; i < count; i++)
      {
	while (_hb_ot_layout_skip_mark (c->layout->face, &c->buffer->info[c->buffer->i], c->lookup_props, NULL))
	{
	  c->buffer->info[c->buffer->i].lig_comp() = i;
	  c->buffer->info[c->buffer->i].lig_id() = lig_id;
	  c->replace_glyph (c->buffer->info[c->buffer->i].codepoint);
	}

	
	c->buffer->i++;
      }
    }

    return true;
  }

  inline uint16_t allocate_lig_id (hb_buffer_t *buffer) const {
    uint16_t lig_id = buffer->next_serial ();
    if (unlikely (!lig_id)) lig_id = buffer->next_serial (); 
    return lig_id;
  }

  public:
  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return ligGlyph.sanitize (c)
        && component.sanitize (c);
  }

  private:
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
  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    unsigned int num_ligs = ligature.len;
    for (unsigned int i = 0; i < num_ligs; i++)
    {
      const Ligature &lig = this+ligature[i];
      if (lig.apply (c))
        return true;
    }

    return false;
  }

  public:
  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return ligature.sanitize (c, this);
  }

  private:
  OffsetArrayOf<Ligature>
		ligature;		

  public:
  DEFINE_SIZE_ARRAY (2, ligature);
};

struct LigatureSubstFormat1
{
  friend struct LigatureSubst;

  private:
  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    hb_codepoint_t glyph_id = c->buffer->info[c->buffer->i].codepoint;

    unsigned int index = (this+coverage) (glyph_id);
    if (likely (index == NOT_COVERED))
      return false;

    const LigatureSet &lig_set = this+ligatureSet[index];
    return lig_set.apply (c);
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return coverage.sanitize (c, this)
	&& ligatureSet.sanitize (c, this);
  }

  private:
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
  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    switch (u.format) {
    case 1: return u.format1.apply (c);
    default:return false;
    }
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    if (!u.format.sanitize (c)) return false;
    switch (u.format) {
    case 1: return u.format1.sanitize (c);
    default:return true;
    }
  }

  private:
  union {
  USHORT		format;		
  LigatureSubstFormat1	format1;
  } u;
};


HB_BEGIN_DECLS
static inline bool substitute_lookup (hb_apply_context_t *c, unsigned int lookup_index);
HB_END_DECLS

struct ContextSubst : Context
{
  friend struct SubstLookupSubTable;

  private:
  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    return Context::apply (c, substitute_lookup);
  }
};

struct ChainContextSubst : ChainContext
{
  friend struct SubstLookupSubTable;

  private:
  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    return ChainContext::apply (c, substitute_lookup);
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

  inline bool apply (hb_apply_context_t *c) const;

  inline bool sanitize (hb_sanitize_context_t *c);

  inline bool is_reverse (void) const;
};


struct ReverseChainSingleSubstFormat1
{
  friend struct ReverseChainSingleSubst;

  private:
  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    if (unlikely (c->context_length != NO_CONTEXT))
      return false; 

    unsigned int index = (this+coverage) (c->buffer->info[c->buffer->i].codepoint);
    if (likely (index == NOT_COVERED))
      return false;

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
      c->buffer->info[c->buffer->i].codepoint = substitute[index];
      c->buffer->i--; 
      return true;
    }

    return false;
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    if (!(coverage.sanitize (c, this)
       && backtrack.sanitize (c, this)))
      return false;
    OffsetArrayOf<Coverage> &lookahead = StructAfter<OffsetArrayOf<Coverage> > (backtrack);
    if (!lookahead.sanitize (c, this))
      return false;
    ArrayOf<GlyphID> &substitute = StructAfter<ArrayOf<GlyphID> > (lookahead);
    return substitute.sanitize (c);
  }

  private:
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
  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    switch (u.format) {
    case 1: return u.format1.apply (c);
    default:return false;
    }
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    if (!u.format.sanitize (c)) return false;
    switch (u.format) {
    case 1: return u.format1.sanitize (c);
    default:return true;
    }
  }

  private:
  union {
  USHORT				format;		
  ReverseChainSingleSubstFormat1	format1;
  } u;
};







struct SubstLookupSubTable
{
  friend struct SubstLookup;

  enum {
    Single		= 1,
    Multiple		= 2,
    Alternate		= 3,
    Ligature		= 4,
    Context		= 5,
    ChainContext	= 6,
    Extension		= 7,
    ReverseChainSingle	= 8
  };

  inline bool apply (hb_apply_context_t *c, unsigned int lookup_type) const
  {
    TRACE_APPLY ();
    switch (lookup_type) {
    case Single:		return u.single.apply (c);
    case Multiple:		return u.multiple.apply (c);
    case Alternate:		return u.alternate.apply (c);
    case Ligature:		return u.ligature.apply (c);
    case Context:		return u.c.apply (c);
    case ChainContext:		return u.chainContext.apply (c);
    case Extension:		return u.extension.apply (c);
    case ReverseChainSingle:	return u.reverseChainContextSingle.apply (c);
    default:return false;
    }
  }

  inline bool sanitize (hb_sanitize_context_t *c, unsigned int lookup_type) {
    TRACE_SANITIZE ();
    switch (lookup_type) {
    case Single:		return u.single.sanitize (c);
    case Multiple:		return u.multiple.sanitize (c);
    case Alternate:		return u.alternate.sanitize (c);
    case Ligature:		return u.ligature.sanitize (c);
    case Context:		return u.c.sanitize (c);
    case ChainContext:		return u.chainContext.sanitize (c);
    case Extension:		return u.extension.sanitize (c);
    case ReverseChainSingle:	return u.reverseChainContextSingle.sanitize (c);
    default:return true;
    }
  }

  private:
  union {
  USHORT			sub_format;
  SingleSubst			single;
  MultipleSubst			multiple;
  AlternateSubst		alternate;
  LigatureSubst			ligature;
  ContextSubst			c;
  ChainContextSubst		chainContext;
  ExtensionSubst		extension;
  ReverseChainSingleSubst	reverseChainContextSingle;
  } u;
  public:
  DEFINE_SIZE_UNION (2, sub_format);
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


  inline bool apply_once (hb_ot_layout_context_t *layout,
			  hb_buffer_t *buffer,
			  hb_mask_t lookup_mask,
			  unsigned int context_length,
			  unsigned int nesting_level_left) const
  {
    unsigned int lookup_type = get_type ();
    hb_apply_context_t c[1] = {{0}};

    c->layout = layout;
    c->buffer = buffer;
    c->lookup_mask = lookup_mask;
    c->context_length = context_length;
    c->nesting_level_left = nesting_level_left;
    c->lookup_props = get_props ();

    if (!_hb_ot_layout_check_glyph_property (c->layout->face, &c->buffer->info[c->buffer->i], c->lookup_props, &c->property))
      return false;

    if (unlikely (lookup_type == SubstLookupSubTable::Extension))
    {
      




      unsigned int count = get_subtable_count ();
      unsigned int type = get_subtable(0).u.extension.get_type ();
      for (unsigned int i = 1; i < count; i++)
        if (get_subtable(i).u.extension.get_type () != type)
	  return false;
    }

    unsigned int count = get_subtable_count ();
    for (unsigned int i = 0; i < count; i++)
      if (get_subtable (i).apply (c, lookup_type))
	return true;

    return false;
  }

  inline bool apply_string (hb_ot_layout_context_t *layout,
			    hb_buffer_t *buffer,
			    hb_mask_t    mask) const
  {
    bool ret = false;

    if (unlikely (!buffer->len))
      return false;

    if (likely (!is_reverse ()))
    {
	
	buffer->clear_output ();
	buffer->i = 0;
	while (buffer->i < buffer->len)
	{
	  if ((buffer->info[buffer->i].mask & mask) &&
	      apply_once (layout, buffer, mask, NO_CONTEXT, MAX_NESTING_LEVEL))
	    ret = true;
	  else
	    buffer->next_glyph ();

	}
	if (ret)
	  buffer->swap ();
    }
    else
    {
	
	buffer->i = buffer->len - 1;
	do
	{
	  if ((buffer->info[buffer->i].mask & mask) &&
	      apply_once (layout, buffer, mask, NO_CONTEXT, MAX_NESTING_LEVEL))
	    ret = true;
	  else
	    buffer->i--;

	}
	while ((int) buffer->i >= 0);
    }

    return ret;
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    if (unlikely (!Lookup::sanitize (c))) return false;
    OffsetArrayOf<SubstLookupSubTable> &list = CastR<OffsetArrayOf<SubstLookupSubTable> > (subTable);
    return list.sanitize (c, this, get_type ());
  }
};

typedef OffsetListOf<SubstLookup> SubstLookupList;





struct GSUB : GSUBGPOS
{
  static const hb_tag_t Tag	= HB_OT_TAG_GSUB;

  inline const SubstLookup& get_lookup (unsigned int i) const
  { return CastR<SubstLookup> (GSUBGPOS::get_lookup (i)); }

  inline bool substitute_lookup (hb_ot_layout_context_t *layout,
				 hb_buffer_t  *buffer,
			         unsigned int  lookup_index,
				 hb_mask_t     mask) const
  { return get_lookup (lookup_index).apply_string (layout, buffer, mask); }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    if (unlikely (!GSUBGPOS::sanitize (c))) return false;
    OffsetTo<SubstLookupList> &list = CastR<OffsetTo<SubstLookupList> > (lookupList);
    return list.sanitize (c, this);
  }
  public:
  DEFINE_SIZE_STATIC (10);
};




inline bool ExtensionSubst::apply (hb_apply_context_t *c) const
{
  TRACE_APPLY ();
  return get_subtable ().apply (c, get_type ());
}

inline bool ExtensionSubst::sanitize (hb_sanitize_context_t *c)
{
  TRACE_SANITIZE ();
  if (unlikely (!Extension::sanitize (c))) return false;
  unsigned int offset = get_offset ();
  if (unlikely (!offset)) return true;
  return StructAtOffset<SubstLookupSubTable> (this, offset).sanitize (c, get_type ());
}

inline bool ExtensionSubst::is_reverse (void) const
{
  unsigned int type = get_type ();
  if (unlikely (type == SubstLookupSubTable::Extension))
    return CastR<ExtensionSubst> (get_subtable()).is_reverse ();
  return SubstLookup::lookup_type_is_reverse (type);
}

static inline bool substitute_lookup (hb_apply_context_t *c, unsigned int lookup_index)
{
  const GSUB &gsub = *(c->layout->face->ot_layout->gsub);
  const SubstLookup &l = gsub.get_lookup (lookup_index);

  if (unlikely (c->nesting_level_left == 0))
    return false;

  if (unlikely (c->context_length < 1))
    return false;

  return l.apply_once (c->layout, c->buffer, c->lookup_mask, c->context_length, c->nesting_level_left - 1);
}


HB_END_DECLS

#endif 
