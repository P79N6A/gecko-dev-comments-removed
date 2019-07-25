



























#ifndef HB_OT_LAYOUT_GSUBGPOS_PRIVATE_HH
#define HB_OT_LAYOUT_GSUBGPOS_PRIVATE_HH

#include "hb-buffer-private.hh"
#include "hb-ot-layout-gdef-table.hh"
#include "hb-set-private.hh"



#ifndef HB_DEBUG_CLOSURE
#define HB_DEBUG_CLOSURE (HB_DEBUG+0)
#endif

#define TRACE_CLOSURE() \
	hb_auto_trace_t<HB_DEBUG_CLOSURE> trace (&c->debug_depth, "CLOSURE", this, HB_FUNC, "");


struct hb_closure_context_t
{
  hb_face_t *face;
  hb_set_t *glyphs;
  unsigned int nesting_level_left;
  unsigned int debug_depth;


  hb_closure_context_t (hb_face_t *face_,
			hb_set_t *glyphs_,
		        unsigned int nesting_level_left_ = MAX_NESTING_LEVEL) :
			  face (face_),
			  glyphs (glyphs_),
			  nesting_level_left (nesting_level_left_),
			  debug_depth (0) {}
};




#ifndef HB_DEBUG_WOULD_APPLY
#define HB_DEBUG_WOULD_APPLY (HB_DEBUG+0)
#endif

#define TRACE_WOULD_APPLY() \
	hb_auto_trace_t<HB_DEBUG_WOULD_APPLY> trace (&c->debug_depth, "WOULD_APPLY", this, HB_FUNC, "%d glyphs", c->len);


struct hb_would_apply_context_t
{
  hb_face_t *face;
  const hb_codepoint_t *glyphs;
  unsigned int len;
  const hb_set_digest_t digest;
  unsigned int debug_depth;

  hb_would_apply_context_t (hb_face_t *face_,
			    const hb_codepoint_t *glyphs_,
			    unsigned int len_,
			    const hb_set_digest_t *digest_
			    ) :
			      face (face_),
			      glyphs (glyphs_),
			      len (len_),
			      digest (*digest_),
			      debug_depth (0) {};
};


#ifndef HB_DEBUG_APPLY
#define HB_DEBUG_APPLY (HB_DEBUG+0)
#endif

#define TRACE_APPLY() \
	hb_auto_trace_t<HB_DEBUG_APPLY> trace (&c->debug_depth, "APPLY", this, HB_FUNC, "idx %d codepoint %u", c->buffer->idx, c->buffer->cur().codepoint);


struct hb_apply_context_t
{
  hb_font_t *font;
  hb_face_t *face;
  hb_buffer_t *buffer;
  hb_direction_t direction;
  hb_mask_t lookup_mask;
  unsigned int nesting_level_left;
  unsigned int lookup_props;
  unsigned int property; 
  unsigned int debug_depth;
  const GDEF &gdef;
  bool has_glyph_classes;
  const hb_set_digest_t digest;


  hb_apply_context_t (hb_font_t *font_,
		      hb_buffer_t *buffer_,
		      hb_mask_t lookup_mask_,
		      const hb_set_digest_t *digest_) :
			font (font_), face (font->face), buffer (buffer_),
			direction (buffer_->props.direction),
			lookup_mask (lookup_mask_),
			nesting_level_left (MAX_NESTING_LEVEL),
			lookup_props (0), property (0), debug_depth (0),
			gdef (*hb_ot_layout_from_face (face)->gdef),
			has_glyph_classes (gdef.has_glyph_classes ()),
			digest (*digest_) {}

  void set_lookup (const Lookup &l) {
    lookup_props = l.get_props ();
  }

  struct mark_skipping_forward_iterator_t
  {
    inline mark_skipping_forward_iterator_t (hb_apply_context_t *c_,
					     unsigned int start_index_,
					     unsigned int num_items_,
					     bool context_match = false)
    {
      c = c_;
      idx = start_index_;
      num_items = num_items_;
      mask = context_match ? -1 : c->lookup_mask;
      syllable = context_match ? 0 : c->buffer->cur().syllable ();
      end = c->buffer->len;
    }
    inline bool has_no_chance (void) const
    {
      return unlikely (num_items && idx + num_items >= end);
    }
    inline void reject (void)
    {
      num_items++;
    }
    inline bool next (unsigned int *property_out,
		      unsigned int  lookup_props)
    {
      assert (num_items > 0);
      do
      {
	if (has_no_chance ())
	  return false;
	idx++;
      } while (c->should_skip_mark (&c->buffer->info[idx], lookup_props, property_out));
      num_items--;
      return (c->buffer->info[idx].mask & mask) && (!syllable || syllable == c->buffer->info[idx].syllable ());
    }
    inline bool next (unsigned int *property_out = NULL)
    {
      return next (property_out, c->lookup_props);
    }

    unsigned int idx;
    protected:
    hb_apply_context_t *c;
    unsigned int num_items;
    hb_mask_t mask;
    uint8_t syllable;
    unsigned int end;
  };

  struct mark_skipping_backward_iterator_t
  {
    inline mark_skipping_backward_iterator_t (hb_apply_context_t *c_,
					      unsigned int start_index_,
					      unsigned int num_items_,
					      hb_mask_t mask_ = 0,
					      bool match_syllable_ = true)
    {
      c = c_;
      idx = start_index_;
      num_items = num_items_;
      mask = mask_ ? mask_ : c->lookup_mask;
      syllable = match_syllable_ ? c->buffer->cur().syllable () : 0;
    }
    inline bool has_no_chance (void) const
    {
      return unlikely (idx < num_items);
    }
    inline void reject (void)
    {
      num_items++;
    }
    inline bool prev (unsigned int *property_out,
		      unsigned int  lookup_props)
    {
      assert (num_items > 0);
      do
      {
	if (has_no_chance ())
	  return false;
	idx--;
      } while (c->should_skip_mark (&c->buffer->out_info[idx], lookup_props, property_out));
      num_items--;
      return (c->buffer->out_info[idx].mask & mask) && (!syllable || syllable == c->buffer->out_info[idx].syllable ());
    }
    inline bool prev (unsigned int *property_out = NULL)
    {
      return prev (property_out, c->lookup_props);
    }

    unsigned int idx;
    protected:
    hb_apply_context_t *c;
    unsigned int num_items;
    hb_mask_t mask;
    uint8_t syllable;
  };

  inline bool
  match_properties_mark (hb_codepoint_t  glyph,
			 unsigned int    glyph_props,
			 unsigned int    lookup_props) const
  {
    


    if (lookup_props & LookupFlag::UseMarkFilteringSet)
      return gdef.mark_set_covers (lookup_props >> 16, glyph);

    



    if (lookup_props & LookupFlag::MarkAttachmentType)
      return (lookup_props & LookupFlag::MarkAttachmentType) == (glyph_props & LookupFlag::MarkAttachmentType);

    return true;
  }

  inline bool
  match_properties (hb_codepoint_t  glyph,
		    unsigned int    glyph_props,
		    unsigned int    lookup_props) const
  {
    


    if (glyph_props & lookup_props & LookupFlag::IgnoreFlags)
      return false;

    if (unlikely (glyph_props & HB_OT_LAYOUT_GLYPH_CLASS_MARK))
      return match_properties_mark (glyph, glyph_props, lookup_props);

    return true;
  }

  inline bool
  check_glyph_property (hb_glyph_info_t *info,
			unsigned int  lookup_props,
			unsigned int *property_out) const
  {
    unsigned int property;

    property = info->glyph_props();
    *property_out = property;

    return match_properties (info->codepoint, property, lookup_props);
  }

  inline bool
  should_skip_mark (hb_glyph_info_t *info,
		   unsigned int  lookup_props,
		   unsigned int *property_out) const
  {
    unsigned int property;

    property = info->glyph_props();
    if (property_out)
      *property_out = property;

    
    if (unlikely (property & HB_OT_LAYOUT_GLYPH_CLASS_MARK))
      return !match_properties (info->codepoint, property, lookup_props);

    
    return false;
  }


  inline bool should_mark_skip_current_glyph (void) const
  {
    return should_skip_mark (&buffer->cur(), lookup_props, NULL);
  }

  inline void set_class (hb_codepoint_t glyph_index, unsigned int class_guess) const
  {
    if (likely (has_glyph_classes))
      buffer->cur().glyph_props() = gdef.get_glyph_props (glyph_index);
    else if (class_guess)
      buffer->cur().glyph_props() = class_guess;
  }

  inline void output_glyph (hb_codepoint_t glyph_index,
			    unsigned int class_guess = 0) const
  {
    set_class (glyph_index, class_guess);
    buffer->output_glyph (glyph_index);
  }
  inline void replace_glyph (hb_codepoint_t glyph_index,
			     unsigned int class_guess = 0) const
  {
    set_class (glyph_index, class_guess);
    buffer->replace_glyph (glyph_index);
  }
  inline void replace_glyph_inplace (hb_codepoint_t glyph_index,
				     unsigned int class_guess = 0) const
  {
    set_class (glyph_index, class_guess);
    buffer->cur().codepoint = glyph_index;
  }
};



typedef bool (*intersects_func_t) (hb_set_t *glyphs, const USHORT &value, const void *data);
typedef bool (*match_func_t) (hb_codepoint_t glyph_id, const USHORT &value, const void *data);
typedef void (*closure_lookup_func_t) (hb_closure_context_t *c, unsigned int lookup_index);
typedef bool (*apply_lookup_func_t) (hb_apply_context_t *c, unsigned int lookup_index);

struct ContextClosureFuncs
{
  intersects_func_t intersects;
  closure_lookup_func_t closure;
};
struct ContextApplyFuncs
{
  match_func_t match;
  apply_lookup_func_t apply;
};

static inline bool intersects_glyph (hb_set_t *glyphs, const USHORT &value, const void *data HB_UNUSED)
{
  return glyphs->has (value);
}
static inline bool intersects_class (hb_set_t *glyphs, const USHORT &value, const void *data)
{
  const ClassDef &class_def = *reinterpret_cast<const ClassDef *>(data);
  return class_def.intersects_class (glyphs, value);
}
static inline bool intersects_coverage (hb_set_t *glyphs, const USHORT &value, const void *data)
{
  const OffsetTo<Coverage> &coverage = (const OffsetTo<Coverage>&)value;
  return (data+coverage).intersects (glyphs);
}

static inline bool intersects_array (hb_closure_context_t *c,
				     unsigned int count,
				     const USHORT values[],
				     intersects_func_t intersects_func,
				     const void *intersects_data)
{
  for (unsigned int i = 0; i < count; i++)
    if (likely (!intersects_func (c->glyphs, values[i], intersects_data)))
      return false;
  return true;
}


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
  return (data+coverage).get_coverage (glyph_id) != NOT_COVERED;
}


static inline bool would_match_input (hb_would_apply_context_t *c,
				      unsigned int count, 
				      const USHORT input[], 
				      match_func_t match_func,
				      const void *match_data)
{
  if (count != c->len)
    return false;

  for (unsigned int i = 1; i < count; i++)
    if (likely (!match_func (c->glyphs[i], input[i - 1], match_data)))
      return false;

  return true;
}
static inline bool match_input (hb_apply_context_t *c,
				unsigned int count, 
				const USHORT input[], 
				match_func_t match_func,
				const void *match_data,
				unsigned int *end_offset = NULL)
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

  if (end_offset)
    *end_offset = skippy_iter.idx - c->buffer->idx + 1;

  return true;
}

static inline bool match_backtrack (hb_apply_context_t *c,
				    unsigned int count,
				    const USHORT backtrack[],
				    match_func_t match_func,
				    const void *match_data)
{
  hb_apply_context_t::mark_skipping_backward_iterator_t skippy_iter (c, c->buffer->backtrack_len (), count, true);
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
  hb_apply_context_t::mark_skipping_forward_iterator_t skippy_iter (c, c->buffer->idx + offset - 1, count, true);
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
    return TRACE_RETURN (c->check_struct (this));
  }

  USHORT	sequenceIndex;		

  USHORT	lookupListIndex;	

  public:
  DEFINE_SIZE_STATIC (4);
};


static inline void closure_lookup (hb_closure_context_t *c,
				   unsigned int lookupCount,
				   const LookupRecord lookupRecord[], 
				   closure_lookup_func_t closure_func)
{
  for (unsigned int i = 0; i < lookupCount; i++)
    closure_func (c, lookupRecord->lookupListIndex);
}

static inline bool apply_lookup (hb_apply_context_t *c,
				 unsigned int count, 
				 unsigned int lookupCount,
				 const LookupRecord lookupRecord[], 
				 apply_lookup_func_t apply_func)
{
  unsigned int end = c->buffer->len;
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





struct ContextClosureLookupContext
{
  ContextClosureFuncs funcs;
  const void *intersects_data;
};

struct ContextApplyLookupContext
{
  ContextApplyFuncs funcs;
  const void *match_data;
};

static inline void context_closure_lookup (hb_closure_context_t *c,
					   unsigned int inputCount, 
					   const USHORT input[], 
					   unsigned int lookupCount,
					   const LookupRecord lookupRecord[],
					   ContextClosureLookupContext &lookup_context)
{
  if (intersects_array (c,
			inputCount ? inputCount - 1 : 0, input,
			lookup_context.funcs.intersects, lookup_context.intersects_data))
    closure_lookup (c,
		    lookupCount, lookupRecord,
		    lookup_context.funcs.closure);
}


static inline bool context_would_apply_lookup (hb_would_apply_context_t *c,
					       unsigned int inputCount, 
					       const USHORT input[], 
					       unsigned int lookupCount,
					       const LookupRecord lookupRecord[],
					       ContextApplyLookupContext &lookup_context)
{
  return would_match_input (c,
			    inputCount, input,
			    lookup_context.funcs.match, lookup_context.match_data);
}
static inline bool context_apply_lookup (hb_apply_context_t *c,
					 unsigned int inputCount, 
					 const USHORT input[], 
					 unsigned int lookupCount,
					 const LookupRecord lookupRecord[],
					 ContextApplyLookupContext &lookup_context)
{
  return match_input (c,
		      inputCount, input,
		      lookup_context.funcs.match, lookup_context.match_data)
      && apply_lookup (c,
		       inputCount,
		       lookupCount, lookupRecord,
		       lookup_context.funcs.apply);
}

struct Rule
{
  friend struct RuleSet;

  private:

  inline void closure (hb_closure_context_t *c, ContextClosureLookupContext &lookup_context) const
  {
    TRACE_CLOSURE ();
    const LookupRecord *lookupRecord = &StructAtOffset<LookupRecord> (input, input[0].static_size * (inputCount ? inputCount - 1 : 0));
    context_closure_lookup (c,
			    inputCount, input,
			    lookupCount, lookupRecord,
			    lookup_context);
  }

  inline bool would_apply (hb_would_apply_context_t *c, ContextApplyLookupContext &lookup_context) const
  {
    TRACE_WOULD_APPLY ();
    const LookupRecord *lookupRecord = &StructAtOffset<LookupRecord> (input, input[0].static_size * (inputCount ? inputCount - 1 : 0));
    return TRACE_RETURN (context_would_apply_lookup (c, inputCount, input, lookupCount, lookupRecord, lookup_context));
  }

  inline bool apply (hb_apply_context_t *c, ContextApplyLookupContext &lookup_context) const
  {
    TRACE_APPLY ();
    const LookupRecord *lookupRecord = &StructAtOffset<LookupRecord> (input, input[0].static_size * (inputCount ? inputCount - 1 : 0));
    return TRACE_RETURN (context_apply_lookup (c, inputCount, input, lookupCount, lookupRecord, lookup_context));
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

  protected:
  USHORT	inputCount;		


  USHORT	lookupCount;		
  USHORT	input[VAR];		

  LookupRecord	lookupRecordX[VAR];	

  public:
  DEFINE_SIZE_ARRAY2 (4, input, lookupRecordX);
};

struct RuleSet
{
  inline void closure (hb_closure_context_t *c, ContextClosureLookupContext &lookup_context) const
  {
    TRACE_CLOSURE ();
    unsigned int num_rules = rule.len;
    for (unsigned int i = 0; i < num_rules; i++)
      (this+rule[i]).closure (c, lookup_context);
  }

  inline bool would_apply (hb_would_apply_context_t *c, ContextApplyLookupContext &lookup_context) const
  {
    TRACE_WOULD_APPLY ();
    unsigned int num_rules = rule.len;
    for (unsigned int i = 0; i < num_rules; i++)
    {
      if ((this+rule[i]).would_apply (c, lookup_context))
        return TRACE_RETURN (true);
    }
    return TRACE_RETURN (false);
  }

  inline bool apply (hb_apply_context_t *c, ContextApplyLookupContext &lookup_context) const
  {
    TRACE_APPLY ();
    unsigned int num_rules = rule.len;
    for (unsigned int i = 0; i < num_rules; i++)
    {
      if ((this+rule[i]).apply (c, lookup_context))
        return TRACE_RETURN (true);
    }
    return TRACE_RETURN (false);
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return TRACE_RETURN (rule.sanitize (c, this));
  }

  protected:
  OffsetArrayOf<Rule>
		rule;			

  public:
  DEFINE_SIZE_ARRAY (2, rule);
};


struct ContextFormat1
{
  friend struct Context;

  private:

  inline void closure (hb_closure_context_t *c, closure_lookup_func_t closure_func) const
  {
    TRACE_CLOSURE ();

    const Coverage &cov = (this+coverage);

    struct ContextClosureLookupContext lookup_context = {
      {intersects_glyph, closure_func},
      NULL
    };

    unsigned int count = ruleSet.len;
    for (unsigned int i = 0; i < count; i++)
      if (cov.intersects_coverage (c->glyphs, i)) {
	const RuleSet &rule_set = this+ruleSet[i];
	rule_set.closure (c, lookup_context);
      }
  }

  inline bool would_apply (hb_would_apply_context_t *c) const
  {
    TRACE_WOULD_APPLY ();

    const RuleSet &rule_set = this+ruleSet[(this+coverage) (c->glyphs[0])];
    struct ContextApplyLookupContext lookup_context = {
      {match_glyph, NULL},
      NULL
    };
    return TRACE_RETURN (rule_set.would_apply (c, lookup_context));
  }

  inline bool apply (hb_apply_context_t *c, apply_lookup_func_t apply_func) const
  {
    TRACE_APPLY ();
    unsigned int index = (this+coverage) (c->buffer->cur().codepoint);
    if (likely (index == NOT_COVERED))
      return TRACE_RETURN (false);

    const RuleSet &rule_set = this+ruleSet[index];
    struct ContextApplyLookupContext lookup_context = {
      {match_glyph, apply_func},
      NULL
    };
    return TRACE_RETURN (rule_set.apply (c, lookup_context));
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return TRACE_RETURN (coverage.sanitize (c, this) && ruleSet.sanitize (c, this));
  }

  protected:
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

  inline void closure (hb_closure_context_t *c, closure_lookup_func_t closure_func) const
  {
    TRACE_CLOSURE ();
    if (!(this+coverage).intersects (c->glyphs))
      return;

    const ClassDef &class_def = this+classDef;

    struct ContextClosureLookupContext lookup_context = {
      {intersects_class, closure_func},
      NULL
    };

    unsigned int count = ruleSet.len;
    for (unsigned int i = 0; i < count; i++)
      if (class_def.intersects_class (c->glyphs, i)) {
	const RuleSet &rule_set = this+ruleSet[i];
	rule_set.closure (c, lookup_context);
      }
  }

  inline bool would_apply (hb_would_apply_context_t *c) const
  {
    TRACE_WOULD_APPLY ();

    const ClassDef &class_def = this+classDef;
    unsigned int index = class_def (c->glyphs[0]);
    const RuleSet &rule_set = this+ruleSet[index];
    struct ContextApplyLookupContext lookup_context = {
      {match_class, NULL},
      &class_def
    };
    return TRACE_RETURN (rule_set.would_apply (c, lookup_context));
  }

  inline bool apply (hb_apply_context_t *c, apply_lookup_func_t apply_func) const
  {
    TRACE_APPLY ();
    unsigned int index = (this+coverage) (c->buffer->cur().codepoint);
    if (likely (index == NOT_COVERED)) return TRACE_RETURN (false);

    const ClassDef &class_def = this+classDef;
    index = class_def (c->buffer->cur().codepoint);
    const RuleSet &rule_set = this+ruleSet[index];
    struct ContextApplyLookupContext lookup_context = {
      {match_class, apply_func},
      &class_def
    };
    return TRACE_RETURN (rule_set.apply (c, lookup_context));
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return TRACE_RETURN (coverage.sanitize (c, this) && classDef.sanitize (c, this) && ruleSet.sanitize (c, this));
  }

  protected:
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

  inline void closure (hb_closure_context_t *c, closure_lookup_func_t closure_func) const
  {
    TRACE_CLOSURE ();
    if (!(this+coverage[0]).intersects (c->glyphs))
      return;

    const LookupRecord *lookupRecord = &StructAtOffset<LookupRecord> (coverage, coverage[0].static_size * glyphCount);
    struct ContextClosureLookupContext lookup_context = {
      {intersects_coverage, closure_func},
      this
    };
    context_closure_lookup (c,
			    glyphCount, (const USHORT *) (coverage + 1),
			    lookupCount, lookupRecord,
			    lookup_context);
  }

  inline bool would_apply (hb_would_apply_context_t *c) const
  {
    TRACE_WOULD_APPLY ();

    const LookupRecord *lookupRecord = &StructAtOffset<LookupRecord> (coverage, coverage[0].static_size * glyphCount);
    struct ContextApplyLookupContext lookup_context = {
      {match_coverage, NULL},
      this
    };
    return TRACE_RETURN (context_would_apply_lookup (c, glyphCount, (const USHORT *) (coverage + 1), lookupCount, lookupRecord, lookup_context));
  }

  inline bool apply (hb_apply_context_t *c, apply_lookup_func_t apply_func) const
  {
    TRACE_APPLY ();
    unsigned int index = (this+coverage[0]) (c->buffer->cur().codepoint);
    if (likely (index == NOT_COVERED)) return TRACE_RETURN (false);

    const LookupRecord *lookupRecord = &StructAtOffset<LookupRecord> (coverage, coverage[0].static_size * glyphCount);
    struct ContextApplyLookupContext lookup_context = {
      {match_coverage, apply_func},
      this
    };
    return TRACE_RETURN (context_apply_lookup (c, glyphCount, (const USHORT *) (coverage + 1), lookupCount, lookupRecord, lookup_context));
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    if (!c->check_struct (this)) return TRACE_RETURN (false);
    unsigned int count = glyphCount;
    if (!c->check_array (coverage, coverage[0].static_size, count)) return TRACE_RETURN (false);
    for (unsigned int i = 0; i < count; i++)
      if (!coverage[i].sanitize (c, this)) return TRACE_RETURN (false);
    LookupRecord *lookupRecord = &StructAtOffset<LookupRecord> (coverage, coverage[0].static_size * count);
    return TRACE_RETURN (c->check_array (lookupRecord, lookupRecord[0].static_size, lookupCount));
  }

  protected:
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

  inline void closure (hb_closure_context_t *c, closure_lookup_func_t closure_func) const
  {
    TRACE_CLOSURE ();
    switch (u.format) {
    case 1: u.format1.closure (c, closure_func); break;
    case 2: u.format2.closure (c, closure_func); break;
    case 3: u.format3.closure (c, closure_func); break;
    default:                                     break;
    }
  }

  inline const Coverage &get_coverage (void) const
  {
    switch (u.format) {
    case 1: return this + u.format1.coverage;
    case 2: return this + u.format2.coverage;
    case 3: return this + u.format3.coverage[0];
    default:return Null(Coverage);
    }
  }

  inline bool would_apply (hb_would_apply_context_t *c) const
  {
    switch (u.format) {
    case 1: return u.format1.would_apply (c);
    case 2: return u.format2.would_apply (c);
    case 3: return u.format3.would_apply (c);
    default:return false;
    }
  }

  inline bool apply (hb_apply_context_t *c, apply_lookup_func_t apply_func) const
  {
    TRACE_APPLY ();
    switch (u.format) {
    case 1: return TRACE_RETURN (u.format1.apply (c, apply_func));
    case 2: return TRACE_RETURN (u.format2.apply (c, apply_func));
    case 3: return TRACE_RETURN (u.format3.apply (c, apply_func));
    default:return TRACE_RETURN (false);
    }
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    if (!u.format.sanitize (c)) return TRACE_RETURN (false);
    switch (u.format) {
    case 1: return TRACE_RETURN (u.format1.sanitize (c));
    case 2: return TRACE_RETURN (u.format2.sanitize (c));
    case 3: return TRACE_RETURN (u.format3.sanitize (c));
    default:return TRACE_RETURN (true);
    }
  }

  protected:
  union {
  USHORT		format;		
  ContextFormat1	format1;
  ContextFormat2	format2;
  ContextFormat3	format3;
  } u;
};




struct ChainContextClosureLookupContext
{
  ContextClosureFuncs funcs;
  const void *intersects_data[3];
};

struct ChainContextApplyLookupContext
{
  ContextApplyFuncs funcs;
  const void *match_data[3];
};

static inline void chain_context_closure_lookup (hb_closure_context_t *c,
						 unsigned int backtrackCount,
						 const USHORT backtrack[],
						 unsigned int inputCount, 
						 const USHORT input[], 
						 unsigned int lookaheadCount,
						 const USHORT lookahead[],
						 unsigned int lookupCount,
						 const LookupRecord lookupRecord[],
						 ChainContextClosureLookupContext &lookup_context)
{
  if (intersects_array (c,
			backtrackCount, backtrack,
			lookup_context.funcs.intersects, lookup_context.intersects_data[0])
   && intersects_array (c,
			inputCount ? inputCount - 1 : 0, input,
			lookup_context.funcs.intersects, lookup_context.intersects_data[1])
  && intersects_array (c,
		       lookaheadCount, lookahead,
		       lookup_context.funcs.intersects, lookup_context.intersects_data[2]))
    closure_lookup (c,
		    lookupCount, lookupRecord,
		    lookup_context.funcs.closure);
}

static inline bool chain_context_would_apply_lookup (hb_would_apply_context_t *c,
						     unsigned int backtrackCount,
						     const USHORT backtrack[],
						     unsigned int inputCount, 
						     const USHORT input[], 
						     unsigned int lookaheadCount,
						     const USHORT lookahead[],
						     unsigned int lookupCount,
						     const LookupRecord lookupRecord[],
						     ChainContextApplyLookupContext &lookup_context)
{
  return !backtrackCount
      && !lookaheadCount
      && would_match_input (c,
			    inputCount, input,
			    lookup_context.funcs.match, lookup_context.match_data[1]);
}

static inline bool chain_context_apply_lookup (hb_apply_context_t *c,
					       unsigned int backtrackCount,
					       const USHORT backtrack[],
					       unsigned int inputCount, 
					       const USHORT input[], 
					       unsigned int lookaheadCount,
					       const USHORT lookahead[],
					       unsigned int lookupCount,
					       const LookupRecord lookupRecord[],
					       ChainContextApplyLookupContext &lookup_context)
{
  unsigned int lookahead_offset;
  return match_input (c,
		      inputCount, input,
		      lookup_context.funcs.match, lookup_context.match_data[1],
		      &lookahead_offset)
      && match_backtrack (c,
			  backtrackCount, backtrack,
			  lookup_context.funcs.match, lookup_context.match_data[0])
      && match_lookahead (c,
			  lookaheadCount, lookahead,
			  lookup_context.funcs.match, lookup_context.match_data[2],
			  lookahead_offset)
      && apply_lookup (c,
		       inputCount,
		       lookupCount, lookupRecord,
		       lookup_context.funcs.apply);
}

struct ChainRule
{
  friend struct ChainRuleSet;

  private:

  inline void closure (hb_closure_context_t *c, ChainContextClosureLookupContext &lookup_context) const
  {
    TRACE_CLOSURE ();
    const HeadlessArrayOf<USHORT> &input = StructAfter<HeadlessArrayOf<USHORT> > (backtrack);
    const ArrayOf<USHORT> &lookahead = StructAfter<ArrayOf<USHORT> > (input);
    const ArrayOf<LookupRecord> &lookup = StructAfter<ArrayOf<LookupRecord> > (lookahead);
    chain_context_closure_lookup (c,
				  backtrack.len, backtrack.array,
				  input.len, input.array,
				  lookahead.len, lookahead.array,
				  lookup.len, lookup.array,
				  lookup_context);
  }

  inline bool would_apply (hb_would_apply_context_t *c, ChainContextApplyLookupContext &lookup_context) const
  {
    TRACE_WOULD_APPLY ();
    const HeadlessArrayOf<USHORT> &input = StructAfter<HeadlessArrayOf<USHORT> > (backtrack);
    const ArrayOf<USHORT> &lookahead = StructAfter<ArrayOf<USHORT> > (input);
    const ArrayOf<LookupRecord> &lookup = StructAfter<ArrayOf<LookupRecord> > (lookahead);
    return TRACE_RETURN (chain_context_would_apply_lookup (c,
							   backtrack.len, backtrack.array,
							   input.len, input.array,
							   lookahead.len, lookahead.array, lookup.len,
							   lookup.array, lookup_context));
  }

  inline bool apply (hb_apply_context_t *c, ChainContextApplyLookupContext &lookup_context) const
  {
    TRACE_APPLY ();
    const HeadlessArrayOf<USHORT> &input = StructAfter<HeadlessArrayOf<USHORT> > (backtrack);
    const ArrayOf<USHORT> &lookahead = StructAfter<ArrayOf<USHORT> > (input);
    const ArrayOf<LookupRecord> &lookup = StructAfter<ArrayOf<LookupRecord> > (lookahead);
    return TRACE_RETURN (chain_context_apply_lookup (c,
						     backtrack.len, backtrack.array,
						     input.len, input.array,
						     lookahead.len, lookahead.array, lookup.len,
						     lookup.array, lookup_context));
  }

  public:
  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    if (!backtrack.sanitize (c)) return TRACE_RETURN (false);
    HeadlessArrayOf<USHORT> &input = StructAfter<HeadlessArrayOf<USHORT> > (backtrack);
    if (!input.sanitize (c)) return TRACE_RETURN (false);
    ArrayOf<USHORT> &lookahead = StructAfter<ArrayOf<USHORT> > (input);
    if (!lookahead.sanitize (c)) return TRACE_RETURN (false);
    ArrayOf<LookupRecord> &lookup = StructAfter<ArrayOf<LookupRecord> > (lookahead);
    return TRACE_RETURN (lookup.sanitize (c));
  }

  protected:
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
  inline void closure (hb_closure_context_t *c, ChainContextClosureLookupContext &lookup_context) const
  {
    TRACE_CLOSURE ();
    unsigned int num_rules = rule.len;
    for (unsigned int i = 0; i < num_rules; i++)
      (this+rule[i]).closure (c, lookup_context);
  }

  inline bool would_apply (hb_would_apply_context_t *c, ChainContextApplyLookupContext &lookup_context) const
  {
    TRACE_WOULD_APPLY ();
    unsigned int num_rules = rule.len;
    for (unsigned int i = 0; i < num_rules; i++)
      if ((this+rule[i]).would_apply (c, lookup_context))
        return TRACE_RETURN (true);

    return TRACE_RETURN (false);
  }

  inline bool apply (hb_apply_context_t *c, ChainContextApplyLookupContext &lookup_context) const
  {
    TRACE_APPLY ();
    unsigned int num_rules = rule.len;
    for (unsigned int i = 0; i < num_rules; i++)
      if ((this+rule[i]).apply (c, lookup_context))
        return TRACE_RETURN (true);

    return TRACE_RETURN (false);
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return TRACE_RETURN (rule.sanitize (c, this));
  }

  protected:
  OffsetArrayOf<ChainRule>
		rule;			

  public:
  DEFINE_SIZE_ARRAY (2, rule);
};

struct ChainContextFormat1
{
  friend struct ChainContext;

  private:

  inline void closure (hb_closure_context_t *c, closure_lookup_func_t closure_func) const
  {
    TRACE_CLOSURE ();
    const Coverage &cov = (this+coverage);

    struct ChainContextClosureLookupContext lookup_context = {
      {intersects_glyph, closure_func},
      {NULL, NULL, NULL}
    };

    unsigned int count = ruleSet.len;
    for (unsigned int i = 0; i < count; i++)
      if (cov.intersects_coverage (c->glyphs, i)) {
	const ChainRuleSet &rule_set = this+ruleSet[i];
	rule_set.closure (c, lookup_context);
      }
  }

  inline bool would_apply (hb_would_apply_context_t *c) const
  {
    TRACE_WOULD_APPLY ();

    const ChainRuleSet &rule_set = this+ruleSet[(this+coverage) (c->glyphs[0])];
    struct ChainContextApplyLookupContext lookup_context = {
      {match_glyph, NULL},
      {NULL, NULL, NULL}
    };
    return TRACE_RETURN (rule_set.would_apply (c, lookup_context));
  }

  inline bool apply (hb_apply_context_t *c, apply_lookup_func_t apply_func) const
  {
    TRACE_APPLY ();
    unsigned int index = (this+coverage) (c->buffer->cur().codepoint);
    if (likely (index == NOT_COVERED)) return TRACE_RETURN (false);

    const ChainRuleSet &rule_set = this+ruleSet[index];
    struct ChainContextApplyLookupContext lookup_context = {
      {match_glyph, apply_func},
      {NULL, NULL, NULL}
    };
    return TRACE_RETURN (rule_set.apply (c, lookup_context));
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return TRACE_RETURN (coverage.sanitize (c, this) && ruleSet.sanitize (c, this));
  }

  protected:
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

  inline void closure (hb_closure_context_t *c, closure_lookup_func_t closure_func) const
  {
    TRACE_CLOSURE ();
    if (!(this+coverage).intersects (c->glyphs))
      return;

    const ClassDef &backtrack_class_def = this+backtrackClassDef;
    const ClassDef &input_class_def = this+inputClassDef;
    const ClassDef &lookahead_class_def = this+lookaheadClassDef;

    struct ChainContextClosureLookupContext lookup_context = {
      {intersects_class, closure_func},
      {&backtrack_class_def,
       &input_class_def,
       &lookahead_class_def}
    };

    unsigned int count = ruleSet.len;
    for (unsigned int i = 0; i < count; i++)
      if (input_class_def.intersects_class (c->glyphs, i)) {
	const ChainRuleSet &rule_set = this+ruleSet[i];
	rule_set.closure (c, lookup_context);
      }
  }

  inline bool would_apply (hb_would_apply_context_t *c) const
  {
    TRACE_WOULD_APPLY ();

    const ClassDef &input_class_def = this+inputClassDef;

    unsigned int index = input_class_def (c->glyphs[0]);
    const ChainRuleSet &rule_set = this+ruleSet[index];
    struct ChainContextApplyLookupContext lookup_context = {
      {match_class, NULL},
      {NULL, &input_class_def, NULL}
    };
    return TRACE_RETURN (rule_set.would_apply (c, lookup_context));
  }

  inline bool apply (hb_apply_context_t *c, apply_lookup_func_t apply_func) const
  {
    TRACE_APPLY ();
    unsigned int index = (this+coverage) (c->buffer->cur().codepoint);
    if (likely (index == NOT_COVERED)) return TRACE_RETURN (false);

    const ClassDef &backtrack_class_def = this+backtrackClassDef;
    const ClassDef &input_class_def = this+inputClassDef;
    const ClassDef &lookahead_class_def = this+lookaheadClassDef;

    index = input_class_def (c->buffer->cur().codepoint);
    const ChainRuleSet &rule_set = this+ruleSet[index];
    struct ChainContextApplyLookupContext lookup_context = {
      {match_class, apply_func},
      {&backtrack_class_def,
       &input_class_def,
       &lookahead_class_def}
    };
    return TRACE_RETURN (rule_set.apply (c, lookup_context));
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return TRACE_RETURN (coverage.sanitize (c, this) && backtrackClassDef.sanitize (c, this) &&
			 inputClassDef.sanitize (c, this) && lookaheadClassDef.sanitize (c, this) &&
			 ruleSet.sanitize (c, this));
  }

  protected:
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

  inline void closure (hb_closure_context_t *c, closure_lookup_func_t closure_func) const
  {
    TRACE_CLOSURE ();
    const OffsetArrayOf<Coverage> &input = StructAfter<OffsetArrayOf<Coverage> > (backtrack);

    if (!(this+input[0]).intersects (c->glyphs))
      return;

    const OffsetArrayOf<Coverage> &lookahead = StructAfter<OffsetArrayOf<Coverage> > (input);
    const ArrayOf<LookupRecord> &lookup = StructAfter<ArrayOf<LookupRecord> > (lookahead);
    struct ChainContextClosureLookupContext lookup_context = {
      {intersects_coverage, closure_func},
      {this, this, this}
    };
    chain_context_closure_lookup (c,
				  backtrack.len, (const USHORT *) backtrack.array,
				  input.len, (const USHORT *) input.array + 1,
				  lookahead.len, (const USHORT *) lookahead.array,
				  lookup.len, lookup.array,
				  lookup_context);
  }

  inline const Coverage &get_coverage (void) const
  {
    const OffsetArrayOf<Coverage> &input = StructAfter<OffsetArrayOf<Coverage> > (backtrack);
    return this+input[0];
  }

  inline bool would_apply (hb_would_apply_context_t *c) const
  {
    TRACE_WOULD_APPLY ();

    const OffsetArrayOf<Coverage> &input = StructAfter<OffsetArrayOf<Coverage> > (backtrack);
    const OffsetArrayOf<Coverage> &lookahead = StructAfter<OffsetArrayOf<Coverage> > (input);
    const ArrayOf<LookupRecord> &lookup = StructAfter<ArrayOf<LookupRecord> > (lookahead);
    struct ChainContextApplyLookupContext lookup_context = {
      {match_coverage, NULL},
      {this, this, this}
    };
    return TRACE_RETURN (chain_context_would_apply_lookup (c,
							   backtrack.len, (const USHORT *) backtrack.array,
							   input.len, (const USHORT *) input.array + 1,
							   lookahead.len, (const USHORT *) lookahead.array,
							   lookup.len, lookup.array, lookup_context));
  }

  inline bool apply (hb_apply_context_t *c, apply_lookup_func_t apply_func) const
  {
    TRACE_APPLY ();
    const OffsetArrayOf<Coverage> &input = StructAfter<OffsetArrayOf<Coverage> > (backtrack);

    unsigned int index = (this+input[0]) (c->buffer->cur().codepoint);
    if (likely (index == NOT_COVERED)) return TRACE_RETURN (false);

    const OffsetArrayOf<Coverage> &lookahead = StructAfter<OffsetArrayOf<Coverage> > (input);
    const ArrayOf<LookupRecord> &lookup = StructAfter<ArrayOf<LookupRecord> > (lookahead);
    struct ChainContextApplyLookupContext lookup_context = {
      {match_coverage, apply_func},
      {this, this, this}
    };
    return TRACE_RETURN (chain_context_apply_lookup (c,
						     backtrack.len, (const USHORT *) backtrack.array,
						     input.len, (const USHORT *) input.array + 1,
						     lookahead.len, (const USHORT *) lookahead.array,
						     lookup.len, lookup.array, lookup_context));
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    if (!backtrack.sanitize (c, this)) return TRACE_RETURN (false);
    OffsetArrayOf<Coverage> &input = StructAfter<OffsetArrayOf<Coverage> > (backtrack);
    if (!input.sanitize (c, this)) return TRACE_RETURN (false);
    OffsetArrayOf<Coverage> &lookahead = StructAfter<OffsetArrayOf<Coverage> > (input);
    if (!lookahead.sanitize (c, this)) return TRACE_RETURN (false);
    ArrayOf<LookupRecord> &lookup = StructAfter<ArrayOf<LookupRecord> > (lookahead);
    return TRACE_RETURN (lookup.sanitize (c));
  }

  protected:
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

  inline void closure (hb_closure_context_t *c, closure_lookup_func_t closure_func) const
  {
    TRACE_CLOSURE ();
    switch (u.format) {
    case 1: u.format1.closure (c, closure_func); break;
    case 2: u.format2.closure (c, closure_func); break;
    case 3: u.format3.closure (c, closure_func); break;
    default:                                     break;
    }
  }

  inline const Coverage &get_coverage (void) const
  {
    switch (u.format) {
    case 1: return this + u.format1.coverage;
    case 2: return this + u.format2.coverage;
    case 3: return u.format3.get_coverage ();
    default:return Null(Coverage);
    }
  }

  inline bool would_apply (hb_would_apply_context_t *c) const
  {
    switch (u.format) {
    case 1: return u.format1.would_apply (c);
    case 2: return u.format2.would_apply (c);
    case 3: return u.format3.would_apply (c);
    default:return false;
    }
  }

  inline bool apply (hb_apply_context_t *c, apply_lookup_func_t apply_func) const
  {
    TRACE_APPLY ();
    switch (u.format) {
    case 1: return TRACE_RETURN (u.format1.apply (c, apply_func));
    case 2: return TRACE_RETURN (u.format2.apply (c, apply_func));
    case 3: return TRACE_RETURN (u.format3.apply (c, apply_func));
    default:return TRACE_RETURN (false);
    }
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    if (!u.format.sanitize (c)) return TRACE_RETURN (false);
    switch (u.format) {
    case 1: return TRACE_RETURN (u.format1.sanitize (c));
    case 2: return TRACE_RETURN (u.format2.sanitize (c));
    case 3: return TRACE_RETURN (u.format3.sanitize (c));
    default:return TRACE_RETURN (true);
    }
  }

  protected:
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
    return TRACE_RETURN (c->check_struct (this));
  }

  protected:
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
    if (!u.format.sanitize (c)) return TRACE_RETURN (false);
    switch (u.format) {
    case 1: return TRACE_RETURN (u.format1.sanitize (c));
    default:return TRACE_RETURN (true);
    }
  }

  protected:
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
    return TRACE_RETURN (version.sanitize (c) && likely (version.major == 1) &&
			 scriptList.sanitize (c, this) &&
			 featureList.sanitize (c, this) &&
			 lookupList.sanitize (c, this));
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
