



























#ifndef HB_OT_LAYOUT_GSUBGPOS_PRIVATE_HH
#define HB_OT_LAYOUT_GSUBGPOS_PRIVATE_HH

#include "hb-buffer-private.hh"
#include "hb-ot-layout-gdef-table.hh"



#define lig_id() var2.u8[2] /* unique ligature id */
#define lig_comp() var2.u8[3] /* component number in the ligature (0 = base) */

static inline uint8_t allocate_lig_id (hb_buffer_t *buffer) {
  uint8_t lig_id = buffer->next_serial ();
  if (unlikely (!lig_id)) lig_id = buffer->next_serial (); 
  return lig_id;
}



#ifndef HB_DEBUG_APPLY
#define HB_DEBUG_APPLY (HB_DEBUG+0)
#endif

#define TRACE_APPLY() \
	hb_auto_trace_t<HB_DEBUG_APPLY> trace (&c->debug_depth, "APPLY", this, NULL, HB_FUNC);



struct hb_apply_context_t
{
  unsigned int debug_depth;
  hb_font_t *font;
  hb_face_t *face;
  hb_buffer_t *buffer;
  hb_direction_t direction;
  hb_mask_t lookup_mask;
  unsigned int context_length;
  unsigned int nesting_level_left;
  unsigned int lookup_props;
  unsigned int property; 


  hb_apply_context_t (hb_font_t *font_,
		      hb_face_t *face_,
		      hb_buffer_t *buffer_,
		      hb_mask_t lookup_mask_,
		      const Lookup &l,
		      unsigned int context_length_ = NO_CONTEXT,
		      unsigned int nesting_level_left_ = MAX_NESTING_LEVEL) :
			font (font_), face (face_), buffer (buffer_),
			direction (buffer_->props.direction),
			lookup_mask (lookup_mask_),
			context_length (context_length_),
			nesting_level_left (nesting_level_left_),
			lookup_props (l.get_props ()),
			property (0) {}

  hb_apply_context_t (const hb_apply_context_t &c, const Lookup &l) {
    *this = c;
    nesting_level_left--;
    lookup_props = l.get_props ();
  }

  struct mark_skipping_forward_iterator_t
  {
    inline mark_skipping_forward_iterator_t (hb_apply_context_t *c_,
					     unsigned int start_index_,
					     unsigned int num_items_)
    {
      c = c_;
      idx = start_index_;
      num_items = num_items_;
      end = MIN (c->buffer->len, c->buffer->idx + c->context_length);
    }
    inline bool has_no_chance (void) const
    {
      return unlikely (num_items && idx + num_items >= end);
    }
    inline bool next (unsigned int *property_out,
		      unsigned int lookup_props)
    {
      assert (num_items > 0);
      do
      {
	if (has_no_chance ())
	  return false;
	idx++;
      } while (_hb_ot_layout_skip_mark (c->face, &c->buffer->info[idx], lookup_props, property_out));
      num_items--;
      return true;
    }
    inline bool next (unsigned int *property_out = NULL)
    {
      return next (property_out, c->lookup_props);
    }

    unsigned int idx;
    private:
    hb_apply_context_t *c;
    unsigned int num_items;
    unsigned int end;
  };

  struct mark_skipping_backward_iterator_t
  {
    inline mark_skipping_backward_iterator_t (hb_apply_context_t *c_,
					      unsigned int start_index_,
					      unsigned int num_items_)
    {
      c = c_;
      idx = start_index_;
      num_items = num_items_;
    }
    inline bool has_no_chance (void) const
    {
      return unlikely (idx < num_items);
    }
    inline bool prev (unsigned int *property_out,
		      unsigned int lookup_props)
    {
      assert (num_items > 0);
      do
      {
	if (has_no_chance ())
	  return false;
	idx--;
      } while (_hb_ot_layout_skip_mark (c->face, &c->buffer->out_info[idx], lookup_props, property_out));
      num_items--;
      return true;
    }
    inline bool prev (unsigned int *property_out = NULL)
    {
      return prev (property_out, c->lookup_props);
    }

    unsigned int idx;
    private:
    hb_apply_context_t *c;
    unsigned int num_items;
  };

  inline bool should_mark_skip_current_glyph (void) const
  {
    return _hb_ot_layout_skip_mark (face, &buffer->info[buffer->idx], lookup_props, NULL);
  }



  inline void replace_glyph (hb_codepoint_t glyph_index) const
  {
    clear_property ();
    buffer->replace_glyph (glyph_index);
  }
  inline void replace_glyphs_be16 (unsigned int num_in,
				   unsigned int num_out,
				   const uint16_t *glyph_data_be) const
  {
    clear_property ();
    buffer->replace_glyphs_be16 (num_in, num_out, glyph_data_be);
  }

  inline void guess_glyph_class (unsigned int klass)
  {
    
    buffer->info[buffer->idx].props_cache() = klass;
  }

  private:
  inline void clear_property (void) const
  {
    
    buffer->info[buffer->idx].props_cache() = 0;
  }
};



typedef bool (*match_func_t) (hb_codepoint_t glyph_id, const USHORT &value, const void *data);
typedef bool (*apply_lookup_func_t) (hb_apply_context_t *c, unsigned int lookup_index);

struct ContextFuncs
{
  match_func_t match;
  apply_lookup_func_t apply;
};


static inline bool match_glyph (hb_codepoint_t glyph_id, const USHORT &value, const void *data HB_UNUSED)
{
  return glyph_id == value;
}

static inline bool match_class (hb_codepoint_t glyph_id, const USHORT &value, const void *data)
{
  const ClassDef &class_def = *reinterpret_cast<const ClassDef *>(data);
  return class_def.get_class (glyph_id) == value;
}

static inline bool match_coverage (hb_codepoint_t glyph_id, const USHORT &value, const void *data)
{
  const OffsetTo<Coverage> &coverage = (const OffsetTo<Coverage>&)value;
  return (data+coverage) (glyph_id) != NOT_COVERED;
}


static inline bool match_input (hb_apply_context_t *c,
				unsigned int count, 
				const USHORT input[], 
				match_func_t match_func,
				const void *match_data,
				unsigned int *context_length_out)
{
  hb_apply_context_t::mark_skipping_forward_iterator_t skippy_iter (c, c->buffer->idx, count - 1);
  if (skippy_iter.has_no_chance ())
    return false;

  for (unsigned int i = 1; i < count; i++)
  {
    if (!skippy_iter.next ())
      return false;

    if (likely (!match_func (c->buffer->info[skippy_iter.idx].codepoint, input[i - 1], match_data)))
      return false;
  }

  *context_length_out = skippy_iter.idx - c->buffer->idx + 1;

  return true;
}

static inline bool match_backtrack (hb_apply_context_t *c,
				    unsigned int count,
				    const USHORT backtrack[],
				    match_func_t match_func,
				    const void *match_data)
{
  hb_apply_context_t::mark_skipping_backward_iterator_t skippy_iter (c, c->buffer->backtrack_len (), count);
  if (skippy_iter.has_no_chance ())
    return false;

  for (unsigned int i = 0; i < count; i++)
  {
    if (!skippy_iter.prev ())
      return false;

    if (likely (!match_func (c->buffer->out_info[skippy_iter.idx].codepoint, backtrack[i], match_data)))
      return false;
  }

  return true;
}

static inline bool match_lookahead (hb_apply_context_t *c,
				    unsigned int count,
				    const USHORT lookahead[],
				    match_func_t match_func,
				    const void *match_data,
				    unsigned int offset)
{
  hb_apply_context_t::mark_skipping_forward_iterator_t skippy_iter (c, c->buffer->idx + offset - 1, count);
  if (skippy_iter.has_no_chance ())
    return false;

  for (unsigned int i = 0; i < count; i++)
  {
    if (!skippy_iter.next ())
      return false;

    if (likely (!match_func (c->buffer->info[skippy_iter.idx].codepoint, lookahead[i], match_data)))
      return false;
  }

  return true;
}



struct LookupRecord
{
  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return c->check_struct (this);
  }

  USHORT	sequenceIndex;		

  USHORT	lookupListIndex;	

  public:
  DEFINE_SIZE_STATIC (4);
};



static inline bool apply_lookup (hb_apply_context_t *c,
				 unsigned int count, 
				 unsigned int lookupCount,
				 const LookupRecord lookupRecord[], 
				 apply_lookup_func_t apply_func)
{
  unsigned int end = MIN (c->buffer->len, c->buffer->idx + c->context_length);
  if (unlikely (count == 0 || c->buffer->idx + count > end))
    return false;

  


  



  for (unsigned int i = 0; i < count; )
  {
    if (unlikely (c->buffer->idx == end))
      return true;
    while (c->should_mark_skip_current_glyph ())
    {
      
      c->buffer->next_glyph ();
      if (unlikely (c->buffer->idx == end))
	return true;
    }

    if (lookupCount && i == lookupRecord->sequenceIndex)
    {
      unsigned int old_pos = c->buffer->idx;

      
      bool done = apply_func (c, lookupRecord->lookupListIndex);

      lookupRecord++;
      lookupCount--;
      
      i += c->buffer->idx - old_pos;
      if (unlikely (c->buffer->idx == end))
	return true;

      if (!done)
	goto not_applied;
    }
    else
    {
    not_applied:
      
      c->buffer->next_glyph ();
      i++;
    }
  }

  return true;
}





struct ContextLookupContext
{
  ContextFuncs funcs;
  const void *match_data;
};

static inline bool context_lookup (hb_apply_context_t *c,
				   unsigned int inputCount, 
				   const USHORT input[], 
				   unsigned int lookupCount,
				   const LookupRecord lookupRecord[],
				   ContextLookupContext &lookup_context)
{
  hb_apply_context_t new_context = *c;
  return match_input (c,
		      inputCount, input,
		      lookup_context.funcs.match, lookup_context.match_data,
		      &new_context.context_length)
      && apply_lookup (&new_context,
		       inputCount,
		       lookupCount, lookupRecord,
		       lookup_context.funcs.apply);
}

struct Rule
{
  friend struct RuleSet;

  private:
  inline bool apply (hb_apply_context_t *c, ContextLookupContext &lookup_context) const
  {
    TRACE_APPLY ();
    const LookupRecord *lookupRecord = &StructAtOffset<LookupRecord> (input, input[0].static_size * (inputCount ? inputCount - 1 : 0));
    return context_lookup (c,
			   inputCount, input,
			   lookupCount, lookupRecord,
			   lookup_context);
  }

  public:
  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return inputCount.sanitize (c)
	&& lookupCount.sanitize (c)
	&& c->check_range (input,
				 input[0].static_size * inputCount
				 + lookupRecordX[0].static_size * lookupCount);
  }

  private:
  USHORT	inputCount;		


  USHORT	lookupCount;		
  USHORT	input[VAR];		

  LookupRecord	lookupRecordX[VAR];	

  public:
  DEFINE_SIZE_ARRAY2 (4, input, lookupRecordX);
};

struct RuleSet
{
  inline bool apply (hb_apply_context_t *c, ContextLookupContext &lookup_context) const
  {
    TRACE_APPLY ();
    unsigned int num_rules = rule.len;
    for (unsigned int i = 0; i < num_rules; i++)
    {
      if ((this+rule[i]).apply (c, lookup_context))
        return true;
    }

    return false;
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return rule.sanitize (c, this);
  }

  private:
  OffsetArrayOf<Rule>
		rule;			

  public:
  DEFINE_SIZE_ARRAY (2, rule);
};


struct ContextFormat1
{
  friend struct Context;

  private:
  inline bool apply (hb_apply_context_t *c, apply_lookup_func_t apply_func) const
  {
    TRACE_APPLY ();
    unsigned int index = (this+coverage) (c->buffer->info[c->buffer->idx].codepoint);
    if (likely (index == NOT_COVERED))
      return false;

    const RuleSet &rule_set = this+ruleSet[index];
    struct ContextLookupContext lookup_context = {
      {match_glyph, apply_func},
      NULL
    };
    return rule_set.apply (c, lookup_context);
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return coverage.sanitize (c, this)
	&& ruleSet.sanitize (c, this);
  }

  private:
  USHORT	format;			
  OffsetTo<Coverage>
		coverage;		

  OffsetArrayOf<RuleSet>
		ruleSet;		

  public:
  DEFINE_SIZE_ARRAY (6, ruleSet);
};


struct ContextFormat2
{
  friend struct Context;

  private:
  inline bool apply (hb_apply_context_t *c, apply_lookup_func_t apply_func) const
  {
    TRACE_APPLY ();
    unsigned int index = (this+coverage) (c->buffer->info[c->buffer->idx].codepoint);
    if (likely (index == NOT_COVERED))
      return false;

    const ClassDef &class_def = this+classDef;
    index = class_def (c->buffer->info[c->buffer->idx].codepoint);
    const RuleSet &rule_set = this+ruleSet[index];
    struct ContextLookupContext lookup_context = {
      {match_class, apply_func},
      &class_def
    };
    return rule_set.apply (c, lookup_context);
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return coverage.sanitize (c, this)
        && classDef.sanitize (c, this)
	&& ruleSet.sanitize (c, this);
  }

  private:
  USHORT	format;			
  OffsetTo<Coverage>
		coverage;		

  OffsetTo<ClassDef>
		classDef;		

  OffsetArrayOf<RuleSet>
		ruleSet;		

  public:
  DEFINE_SIZE_ARRAY (8, ruleSet);
};


struct ContextFormat3
{
  friend struct Context;

  private:
  inline bool apply (hb_apply_context_t *c, apply_lookup_func_t apply_func) const
  {
    TRACE_APPLY ();
    unsigned int index = (this+coverage[0]) (c->buffer->info[c->buffer->idx].codepoint);
    if (likely (index == NOT_COVERED))
      return false;

    const LookupRecord *lookupRecord = &StructAtOffset<LookupRecord> (coverage, coverage[0].static_size * glyphCount);
    struct ContextLookupContext lookup_context = {
      {match_coverage, apply_func},
      this
    };
    return context_lookup (c,
			   glyphCount, (const USHORT *) (coverage + 1),
			   lookupCount, lookupRecord,
			   lookup_context);
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    if (!c->check_struct (this)) return false;
    unsigned int count = glyphCount;
    if (!c->check_array (coverage, coverage[0].static_size, count)) return false;
    for (unsigned int i = 0; i < count; i++)
      if (!coverage[i].sanitize (c, this)) return false;
    LookupRecord *lookupRecord = &StructAtOffset<LookupRecord> (coverage, coverage[0].static_size * count);
    return c->check_array (lookupRecord, lookupRecord[0].static_size, lookupCount);
  }

  private:
  USHORT	format;			
  USHORT	glyphCount;		

  USHORT	lookupCount;		
  OffsetTo<Coverage>
		coverage[VAR];		

  LookupRecord	lookupRecordX[VAR];	

  public:
  DEFINE_SIZE_ARRAY2 (6, coverage, lookupRecordX);
};

struct Context
{
  protected:
  inline bool apply (hb_apply_context_t *c, apply_lookup_func_t apply_func) const
  {
    TRACE_APPLY ();
    switch (u.format) {
    case 1: return u.format1.apply (c, apply_func);
    case 2: return u.format2.apply (c, apply_func);
    case 3: return u.format3.apply (c, apply_func);
    default:return false;
    }
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    if (!u.format.sanitize (c)) return false;
    switch (u.format) {
    case 1: return u.format1.sanitize (c);
    case 2: return u.format2.sanitize (c);
    case 3: return u.format3.sanitize (c);
    default:return true;
    }
  }

  private:
  union {
  USHORT		format;		
  ContextFormat1	format1;
  ContextFormat2	format2;
  ContextFormat3	format3;
  } u;
};




struct ChainContextLookupContext
{
  ContextFuncs funcs;
  const void *match_data[3];
};

static inline bool chain_context_lookup (hb_apply_context_t *c,
					 unsigned int backtrackCount,
					 const USHORT backtrack[],
					 unsigned int inputCount, 
					 const USHORT input[], 
					 unsigned int lookaheadCount,
					 const USHORT lookahead[],
					 unsigned int lookupCount,
					 const LookupRecord lookupRecord[],
					 ChainContextLookupContext &lookup_context)
{
  
  if (unlikely (c->buffer->backtrack_len () < backtrackCount ||
		c->buffer->idx + inputCount + lookaheadCount > c->buffer->len ||
		inputCount + lookaheadCount > c->context_length))
    return false;

  hb_apply_context_t new_context = *c;
  return match_backtrack (c,
			  backtrackCount, backtrack,
			  lookup_context.funcs.match, lookup_context.match_data[0])
      && match_input (c,
		      inputCount, input,
		      lookup_context.funcs.match, lookup_context.match_data[1],
		      &new_context.context_length)
      && match_lookahead (c,
			  lookaheadCount, lookahead,
			  lookup_context.funcs.match, lookup_context.match_data[2],
			  new_context.context_length)
      && apply_lookup (&new_context,
		       inputCount,
		       lookupCount, lookupRecord,
		       lookup_context.funcs.apply);
}

struct ChainRule
{
  friend struct ChainRuleSet;

  private:
  inline bool apply (hb_apply_context_t *c, ChainContextLookupContext &lookup_context) const
  {
    TRACE_APPLY ();
    const HeadlessArrayOf<USHORT> &input = StructAfter<HeadlessArrayOf<USHORT> > (backtrack);
    const ArrayOf<USHORT> &lookahead = StructAfter<ArrayOf<USHORT> > (input);
    const ArrayOf<LookupRecord> &lookup = StructAfter<ArrayOf<LookupRecord> > (lookahead);
    return chain_context_lookup (c,
				 backtrack.len, backtrack.array,
				 input.len, input.array,
				 lookahead.len, lookahead.array,
				 lookup.len, lookup.array,
				 lookup_context);
  }

  public:
  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    if (!backtrack.sanitize (c)) return false;
    HeadlessArrayOf<USHORT> &input = StructAfter<HeadlessArrayOf<USHORT> > (backtrack);
    if (!input.sanitize (c)) return false;
    ArrayOf<USHORT> &lookahead = StructAfter<ArrayOf<USHORT> > (input);
    if (!lookahead.sanitize (c)) return false;
    ArrayOf<LookupRecord> &lookup = StructAfter<ArrayOf<LookupRecord> > (lookahead);
    return lookup.sanitize (c);
  }

  private:
  ArrayOf<USHORT>
		backtrack;		


  HeadlessArrayOf<USHORT>
		inputX;			

  ArrayOf<USHORT>
		lookaheadX;		

  ArrayOf<LookupRecord>
		lookupX;		

  public:
  DEFINE_SIZE_MIN (8);
};

struct ChainRuleSet
{
  inline bool apply (hb_apply_context_t *c, ChainContextLookupContext &lookup_context) const
  {
    TRACE_APPLY ();
    unsigned int num_rules = rule.len;
    for (unsigned int i = 0; i < num_rules; i++)
    {
      if ((this+rule[i]).apply (c, lookup_context))
        return true;
    }

    return false;
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return rule.sanitize (c, this);
  }

  private:
  OffsetArrayOf<ChainRule>
		rule;			

  public:
  DEFINE_SIZE_ARRAY (2, rule);
};

struct ChainContextFormat1
{
  friend struct ChainContext;

  private:
  inline bool apply (hb_apply_context_t *c, apply_lookup_func_t apply_func) const
  {
    TRACE_APPLY ();
    unsigned int index = (this+coverage) (c->buffer->info[c->buffer->idx].codepoint);
    if (likely (index == NOT_COVERED))
      return false;

    const ChainRuleSet &rule_set = this+ruleSet[index];
    struct ChainContextLookupContext lookup_context = {
      {match_glyph, apply_func},
      {NULL, NULL, NULL}
    };
    return rule_set.apply (c, lookup_context);
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return coverage.sanitize (c, this)
	&& ruleSet.sanitize (c, this);
  }

  private:
  USHORT	format;			
  OffsetTo<Coverage>
		coverage;		

  OffsetArrayOf<ChainRuleSet>
		ruleSet;		

  public:
  DEFINE_SIZE_ARRAY (6, ruleSet);
};

struct ChainContextFormat2
{
  friend struct ChainContext;

  private:
  inline bool apply (hb_apply_context_t *c, apply_lookup_func_t apply_func) const
  {
    TRACE_APPLY ();
    unsigned int index = (this+coverage) (c->buffer->info[c->buffer->idx].codepoint);
    if (likely (index == NOT_COVERED))
      return false;

    const ClassDef &backtrack_class_def = this+backtrackClassDef;
    const ClassDef &input_class_def = this+inputClassDef;
    const ClassDef &lookahead_class_def = this+lookaheadClassDef;

    index = input_class_def (c->buffer->info[c->buffer->idx].codepoint);
    const ChainRuleSet &rule_set = this+ruleSet[index];
    struct ChainContextLookupContext lookup_context = {
      {match_class, apply_func},
      {&backtrack_class_def,
       &input_class_def,
       &lookahead_class_def}
    };
    return rule_set.apply (c, lookup_context);
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return coverage.sanitize (c, this)
	&& backtrackClassDef.sanitize (c, this)
	&& inputClassDef.sanitize (c, this)
	&& lookaheadClassDef.sanitize (c, this)
	&& ruleSet.sanitize (c, this);
  }

  private:
  USHORT	format;			
  OffsetTo<Coverage>
		coverage;		

  OffsetTo<ClassDef>
		backtrackClassDef;	


  OffsetTo<ClassDef>
		inputClassDef;		


  OffsetTo<ClassDef>
		lookaheadClassDef;	


  OffsetArrayOf<ChainRuleSet>
		ruleSet;		

  public:
  DEFINE_SIZE_ARRAY (12, ruleSet);
};

struct ChainContextFormat3
{
  friend struct ChainContext;

  private:

  inline bool apply (hb_apply_context_t *c, apply_lookup_func_t apply_func) const
  {
    TRACE_APPLY ();
    const OffsetArrayOf<Coverage> &input = StructAfter<OffsetArrayOf<Coverage> > (backtrack);

    unsigned int index = (this+input[0]) (c->buffer->info[c->buffer->idx].codepoint);
    if (likely (index == NOT_COVERED))
      return false;

    const OffsetArrayOf<Coverage> &lookahead = StructAfter<OffsetArrayOf<Coverage> > (input);
    const ArrayOf<LookupRecord> &lookup = StructAfter<ArrayOf<LookupRecord> > (lookahead);
    struct ChainContextLookupContext lookup_context = {
      {match_coverage, apply_func},
      {this, this, this}
    };
    return chain_context_lookup (c,
				 backtrack.len, (const USHORT *) backtrack.array,
				 input.len, (const USHORT *) input.array + 1,
				 lookahead.len, (const USHORT *) lookahead.array,
				 lookup.len, lookup.array,
				 lookup_context);
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    if (!backtrack.sanitize (c, this)) return false;
    OffsetArrayOf<Coverage> &input = StructAfter<OffsetArrayOf<Coverage> > (backtrack);
    if (!input.sanitize (c, this)) return false;
    OffsetArrayOf<Coverage> &lookahead = StructAfter<OffsetArrayOf<Coverage> > (input);
    if (!lookahead.sanitize (c, this)) return false;
    ArrayOf<LookupRecord> &lookup = StructAfter<ArrayOf<LookupRecord> > (lookahead);
    return lookup.sanitize (c);
  }

  private:
  USHORT	format;			
  OffsetArrayOf<Coverage>
		backtrack;		


  OffsetArrayOf<Coverage>
		inputX		;	


  OffsetArrayOf<Coverage>
		lookaheadX;		


  ArrayOf<LookupRecord>
		lookupX;		

  public:
  DEFINE_SIZE_MIN (10);
};

struct ChainContext
{
  protected:
  inline bool apply (hb_apply_context_t *c, apply_lookup_func_t apply_func) const
  {
    TRACE_APPLY ();
    switch (u.format) {
    case 1: return u.format1.apply (c, apply_func);
    case 2: return u.format2.apply (c, apply_func);
    case 3: return u.format3.apply (c, apply_func);
    default:return false;
    }
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    if (!u.format.sanitize (c)) return false;
    switch (u.format) {
    case 1: return u.format1.sanitize (c);
    case 2: return u.format2.sanitize (c);
    case 3: return u.format3.sanitize (c);
    default:return true;
    }
  }

  private:
  union {
  USHORT		format;	
  ChainContextFormat1	format1;
  ChainContextFormat2	format2;
  ChainContextFormat3	format3;
  } u;
};


struct ExtensionFormat1
{
  friend struct Extension;

  protected:
  inline unsigned int get_type (void) const { return extensionLookupType; }
  inline unsigned int get_offset (void) const { return extensionOffset; }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return c->check_struct (this);
  }

  private:
  USHORT	format;			
  USHORT	extensionLookupType;	


  ULONG		extensionOffset;	

  public:
  DEFINE_SIZE_STATIC (8);
};

struct Extension
{
  inline unsigned int get_type (void) const
  {
    switch (u.format) {
    case 1: return u.format1.get_type ();
    default:return 0;
    }
  }
  inline unsigned int get_offset (void) const
  {
    switch (u.format) {
    case 1: return u.format1.get_offset ();
    default:return 0;
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
  ExtensionFormat1	format1;
  } u;
};






struct GSUBGPOS
{
  static const hb_tag_t GSUBTag	= HB_OT_TAG_GSUB;
  static const hb_tag_t GPOSTag	= HB_OT_TAG_GPOS;

  inline unsigned int get_script_count (void) const
  { return (this+scriptList).len; }
  inline const Tag& get_script_tag (unsigned int i) const
  { return (this+scriptList).get_tag (i); }
  inline unsigned int get_script_tags (unsigned int start_offset,
				       unsigned int *script_count ,
				       hb_tag_t     *script_tags ) const
  { return (this+scriptList).get_tags (start_offset, script_count, script_tags); }
  inline const Script& get_script (unsigned int i) const
  { return (this+scriptList)[i]; }
  inline bool find_script_index (hb_tag_t tag, unsigned int *index) const
  { return (this+scriptList).find_index (tag, index); }

  inline unsigned int get_feature_count (void) const
  { return (this+featureList).len; }
  inline const Tag& get_feature_tag (unsigned int i) const
  { return (this+featureList).get_tag (i); }
  inline unsigned int get_feature_tags (unsigned int start_offset,
					unsigned int *feature_count ,
					hb_tag_t     *feature_tags ) const
  { return (this+featureList).get_tags (start_offset, feature_count, feature_tags); }
  inline const Feature& get_feature (unsigned int i) const
  { return (this+featureList)[i]; }
  inline bool find_feature_index (hb_tag_t tag, unsigned int *index) const
  { return (this+featureList).find_index (tag, index); }

  inline unsigned int get_lookup_count (void) const
  { return (this+lookupList).len; }
  inline const Lookup& get_lookup (unsigned int i) const
  { return (this+lookupList)[i]; }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return version.sanitize (c) && likely (version.major == 1)
	&& scriptList.sanitize (c, this)
	&& featureList.sanitize (c, this)
	&& lookupList.sanitize (c, this);
  }

  protected:
  FixedVersion	version;	

  OffsetTo<ScriptList>
		scriptList;  	
  OffsetTo<FeatureList>
		featureList; 	
  OffsetTo<LookupList>
		lookupList; 	
  public:
  DEFINE_SIZE_STATIC (10);
};



#endif 
