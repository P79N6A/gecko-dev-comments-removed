

























#ifndef HB_OT_LAYOUT_GPOS_PRIVATE_HH
#define HB_OT_LAYOUT_GPOS_PRIVATE_HH

#include "hb-ot-layout-gsubgpos-private.hh"


#define HB_OT_LAYOUT_GPOS_NO_LAST ((unsigned int) -1)



typedef USHORT Value;

typedef Value ValueRecord[VAR];

struct ValueFormat : USHORT
{
  enum
  {
    xPlacement	= 0x0001,	
    yPlacement	= 0x0002,	
    xAdvance	= 0x0004,	
    yAdvance	= 0x0008,	
    xPlaDevice	= 0x0010,	
    yPlaDevice	= 0x0020,	
    xAdvDevice	= 0x0040,	
    yAdvDevice	= 0x0080,	
    ignored	= 0x0F00,	
    reserved	= 0xF000,	

    devices	= 0x00F0	
  };


#if 0
  SHORT		xPlacement;		

  SHORT		yPlacement;		

  SHORT		xAdvance;		


  SHORT		yAdvance;		


  Offset	xPlaDevice;		


  Offset	yPlaDevice;		


  Offset	xAdvDevice;		


  Offset	yAdvDevice;		


#endif

  inline unsigned int get_len () const
  { return _hb_popcount32 ((unsigned int) *this); }
  inline unsigned int get_size () const
  { return get_len () * Value::static_size; }

  void apply_value (hb_ot_layout_context_t       *layout,
		    const void                   *base,
		    const Value                  *values,
		    hb_internal_glyph_position_t &glyph_pos) const
  {
    unsigned int x_ppem, y_ppem;
    unsigned int format = *this;

    if (!format) return;

    
    if (format & xPlacement) glyph_pos.x_offset  += layout->scale_x (get_short (values++));
    if (format & yPlacement) glyph_pos.y_offset  += layout->scale_y (get_short (values++));
    if (format & xAdvance)   glyph_pos.x_advance += layout->scale_x (get_short (values++));
    if (format & yAdvance)   glyph_pos.y_advance += layout->scale_y (get_short (values++));

    if (!has_device ()) return;

    x_ppem = layout->font->x_ppem;
    y_ppem = layout->font->y_ppem;

    if (!x_ppem && !y_ppem) return;

    
    if (format & xPlaDevice) {
      if (x_ppem) glyph_pos.x_offset  += (base + get_device (values++)).get_x_delta (layout); else values++;
    }
    if (format & yPlaDevice) {
      if (y_ppem) glyph_pos.y_offset  += (base + get_device (values++)).get_y_delta (layout); else values++;
    }
    if (format & xAdvDevice) {
      if (x_ppem) glyph_pos.x_advance += (base + get_device (values++)).get_x_delta (layout); else values++;
    }
    if (format & yAdvDevice) {
      if (y_ppem) glyph_pos.y_advance += (base + get_device (values++)).get_y_delta (layout); else values++;
    }
  }

  private:
  inline bool sanitize_value_devices (hb_sanitize_context_t *c, void *base, Value *values) {
    unsigned int format = *this;

    if (format & xPlacement) values++;
    if (format & yPlacement) values++;
    if (format & xAdvance)   values++;
    if (format & yAdvance)   values++;

    if ((format & xPlaDevice) && !get_device (values++).sanitize (c, base)) return false;
    if ((format & yPlaDevice) && !get_device (values++).sanitize (c, base)) return false;
    if ((format & xAdvDevice) && !get_device (values++).sanitize (c, base)) return false;
    if ((format & yAdvDevice) && !get_device (values++).sanitize (c, base)) return false;

    return true;
  }

  static inline OffsetTo<Device>& get_device (Value* value)
  { return *CastP<OffsetTo<Device> > (value); }
  static inline const OffsetTo<Device>& get_device (const Value* value)
  { return *CastP<OffsetTo<Device> > (value); }

  static inline const SHORT& get_short (const Value* value)
  { return *CastP<SHORT> (value); }

  public:

  inline bool has_device () const {
    unsigned int format = *this;
    return (format & devices) != 0;
  }

  inline bool sanitize_value (hb_sanitize_context_t *c, void *base, Value *values) {
    TRACE_SANITIZE ();
    return c->check_range (values, get_size ())
	&& (!has_device () || sanitize_value_devices (c, base, values));
  }

  inline bool sanitize_values (hb_sanitize_context_t *c, void *base, Value *values, unsigned int count) {
    TRACE_SANITIZE ();
    unsigned int len = get_len ();

    if (!c->check_array (values, get_size (), count)) return false;

    if (!has_device ()) return true;

    for (unsigned int i = 0; i < count; i++) {
      if (!sanitize_value_devices (c, base, values))
        return false;
      values += len;
    }

    return true;
  }

  
  inline bool sanitize_values_stride_unsafe (hb_sanitize_context_t *c, void *base, Value *values, unsigned int count, unsigned int stride) {
    TRACE_SANITIZE ();

    if (!has_device ()) return true;

    for (unsigned int i = 0; i < count; i++) {
      if (!sanitize_value_devices (c, base, values))
        return false;
      values += stride;
    }

    return true;
  }
};


struct AnchorFormat1
{
  friend struct Anchor;

  private:
  inline void get_anchor (hb_ot_layout_context_t *layout, hb_codepoint_t glyph_id HB_UNUSED,
			  hb_position_t *x, hb_position_t *y) const
  {
      *x = layout->scale_x (xCoordinate);
      *y = layout->scale_y (yCoordinate);
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return c->check_struct (this);
  }

  private:
  USHORT	format;			
  SHORT		xCoordinate;		
  SHORT		yCoordinate;		
  public:
  DEFINE_SIZE_STATIC (6);
};

struct AnchorFormat2
{
  friend struct Anchor;

  private:
  inline void get_anchor (hb_ot_layout_context_t *layout, hb_codepoint_t glyph_id,
			  hb_position_t *x, hb_position_t *y) const
  {
      unsigned int x_ppem = layout->font->x_ppem;
      unsigned int y_ppem = layout->font->y_ppem;
      hb_position_t cx, cy;
      hb_bool_t ret;

      if (x_ppem || y_ppem)
	ret = hb_font_get_contour_point (layout->font, layout->face, anchorPoint, glyph_id, &cx, &cy);
      *x = x_ppem && ret ? cx : layout->scale_x (xCoordinate);
      *y = y_ppem && ret ? cy : layout->scale_y (yCoordinate);
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return c->check_struct (this);
  }

  private:
  USHORT	format;			
  SHORT		xCoordinate;		
  SHORT		yCoordinate;		
  USHORT	anchorPoint;		
  public:
  DEFINE_SIZE_STATIC (8);
};

struct AnchorFormat3
{
  friend struct Anchor;

  private:
  inline void get_anchor (hb_ot_layout_context_t *layout, hb_codepoint_t glyph_id HB_UNUSED,
			  hb_position_t *x, hb_position_t *y) const
  {
      *x = layout->scale_x (xCoordinate);
      *y = layout->scale_y (yCoordinate);

      
      if (layout->font->x_ppem)
	*x += (this+xDeviceTable).get_x_delta (layout);
      if (layout->font->y_ppem)
	*y += (this+yDeviceTable).get_x_delta (layout);
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return c->check_struct (this)
	&& xDeviceTable.sanitize (c, this)
	&& yDeviceTable.sanitize (c, this);
  }

  private:
  USHORT	format;			
  SHORT		xCoordinate;		
  SHORT		yCoordinate;		
  OffsetTo<Device>
		xDeviceTable;		


  OffsetTo<Device>
		yDeviceTable;		


  public:
  DEFINE_SIZE_STATIC (10);
};

struct Anchor
{
  inline void get_anchor (hb_ot_layout_context_t *layout, hb_codepoint_t glyph_id,
			  hb_position_t *x, hb_position_t *y) const
  {
    *x = *y = 0;
    switch (u.format) {
    case 1: u.format1.get_anchor (layout, glyph_id, x, y); return;
    case 2: u.format2.get_anchor (layout, glyph_id, x, y); return;
    case 3: u.format3.get_anchor (layout, glyph_id, x, y); return;
    default:						    return;
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
  AnchorFormat1		format1;
  AnchorFormat2		format2;
  AnchorFormat3		format3;
  } u;
  public:
  DEFINE_SIZE_UNION (2, format);
};


struct AnchorMatrix
{
  inline const Anchor& get_anchor (unsigned int row, unsigned int col, unsigned int cols) const {
    if (unlikely (row >= rows || col >= cols)) return Null(Anchor);
    return this+matrix[row * cols + col];
  }

  inline bool sanitize (hb_sanitize_context_t *c, unsigned int cols) {
    TRACE_SANITIZE ();
    if (!c->check_struct (this)) return false;
    if (unlikely (rows > 0 && cols >= ((unsigned int) -1) / rows)) return false;
    unsigned int count = rows * cols;
    if (!c->check_array (matrix, matrix[0].static_size, count)) return false;
    for (unsigned int i = 0; i < count; i++)
      if (!matrix[i].sanitize (c, this)) return false;
    return true;
  }

  USHORT	rows;			
  private:
  OffsetTo<Anchor>
		matrix[VAR];		

  public:
  DEFINE_SIZE_ARRAY (2, matrix);
};


struct MarkRecord
{
  friend struct MarkArray;

  inline bool sanitize (hb_sanitize_context_t *c, void *base) {
    TRACE_SANITIZE ();
    return c->check_struct (this)
	&& markAnchor.sanitize (c, base);
  }

  private:
  USHORT	klass;			
  OffsetTo<Anchor>
		markAnchor;		

  public:
  DEFINE_SIZE_STATIC (4);
};

struct MarkArray : ArrayOf<MarkRecord>	
{
  inline bool apply (hb_apply_context_t *c,
		     unsigned int mark_index, unsigned int glyph_index,
		     const AnchorMatrix &anchors, unsigned int class_count,
		     unsigned int glyph_pos) const
  {
    TRACE_APPLY ();
    const MarkRecord &record = ArrayOf<MarkRecord>::operator[](mark_index);
    unsigned int mark_class = record.klass;

    const Anchor& mark_anchor = this + record.markAnchor;
    const Anchor& glyph_anchor = anchors.get_anchor (glyph_index, mark_class, class_count);

    hb_position_t mark_x, mark_y, base_x, base_y;

    mark_anchor.get_anchor (c->layout, c->buffer->info[c->buffer->i].codepoint, &mark_x, &mark_y);
    glyph_anchor.get_anchor (c->layout, c->buffer->info[glyph_pos].codepoint, &base_x, &base_y);

    hb_internal_glyph_position_t &o = c->buffer->pos[c->buffer->i];
    o.x_advance = 0;
    o.y_advance = 0;
    o.x_offset  = base_x - mark_x;
    o.y_offset  = base_y - mark_y;
    o.back      = c->buffer->i - glyph_pos;

    c->buffer->i++;
    return true;
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return ArrayOf<MarkRecord>::sanitize (c, this);
  }
};




struct SinglePosFormat1
{
  friend struct SinglePos;

  private:
  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    unsigned int index = (this+coverage) (c->buffer->info[c->buffer->i].codepoint);
    if (likely (index == NOT_COVERED))
      return false;

    valueFormat.apply_value (c->layout, this, values, c->buffer->pos[c->buffer->i]);

    c->buffer->i++;
    return true;
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return c->check_struct (this)
	&& coverage.sanitize (c, this)
	&& valueFormat.sanitize_value (c, this, values);
  }

  private:
  USHORT	format;			
  OffsetTo<Coverage>
		coverage;		

  ValueFormat	valueFormat;		

  ValueRecord	values;			


  public:
  DEFINE_SIZE_ARRAY (6, values);
};

struct SinglePosFormat2
{
  friend struct SinglePos;

  private:
  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    unsigned int index = (this+coverage) (c->buffer->info[c->buffer->i].codepoint);
    if (likely (index == NOT_COVERED))
      return false;

    if (likely (index >= valueCount))
      return false;

    valueFormat.apply_value (c->layout, this,
			     &values[index * valueFormat.get_len ()],
			     c->buffer->pos[c->buffer->i]);

    c->buffer->i++;
    return true;
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return c->check_struct (this)
	&& coverage.sanitize (c, this)
	&& valueFormat.sanitize_values (c, this, values, valueCount);
  }

  private:
  USHORT	format;			
  OffsetTo<Coverage>
		coverage;		

  ValueFormat	valueFormat;		

  USHORT	valueCount;		
  ValueRecord	values;			

  public:
  DEFINE_SIZE_ARRAY (8, values);
};

struct SinglePos
{
  friend struct PosLookupSubTable;

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
  SinglePosFormat1	format1;
  SinglePosFormat2	format2;
  } u;
};


struct PairValueRecord
{
  friend struct PairSet;

  private:
  GlyphID	secondGlyph;		


  ValueRecord	values;			

  public:
  DEFINE_SIZE_ARRAY (2, values);
};

struct PairSet
{
  friend struct PairPosFormat1;

  inline bool apply (hb_apply_context_t *c,
		     const ValueFormat *valueFormats,
		     unsigned int pos) const
  {
    TRACE_APPLY ();
    unsigned int len1 = valueFormats[0].get_len ();
    unsigned int len2 = valueFormats[1].get_len ();
    unsigned int record_size = USHORT::static_size * (1 + len1 + len2);

    unsigned int count = len;
    const PairValueRecord *record = CastP<PairValueRecord> (array);
    for (unsigned int i = 0; i < count; i++)
    {
      if (c->buffer->info[pos].codepoint == record->secondGlyph)
      {
	valueFormats[0].apply_value (c->layout, this, &record->values[0], c->buffer->pos[c->buffer->i]);
	valueFormats[1].apply_value (c->layout, this, &record->values[len1], c->buffer->pos[pos]);
	if (len2)
	  pos++;
	c->buffer->i = pos;
	return true;
      }
      record = &StructAtOffset<PairValueRecord> (record, record_size);
    }

    return false;
  }

  struct sanitize_closure_t {
    void *base;
    ValueFormat *valueFormats;
    unsigned int len1; 
    unsigned int stride; 
  };

  inline bool sanitize (hb_sanitize_context_t *c, const sanitize_closure_t *closure) {
    TRACE_SANITIZE ();
    if (!(c->check_struct (this)
       && c->check_array (array, USHORT::static_size * closure->stride, len))) return false;

    unsigned int count = len;
    PairValueRecord *record = CastP<PairValueRecord> (array);
    return closure->valueFormats[0].sanitize_values_stride_unsafe (c, closure->base, &record->values[0], count, closure->stride)
	&& closure->valueFormats[1].sanitize_values_stride_unsafe (c, closure->base, &record->values[closure->len1], count, closure->stride);
  }

  private:
  USHORT	len;			
  USHORT	array[VAR];		

  public:
  DEFINE_SIZE_ARRAY (2, array);
};

struct PairPosFormat1
{
  friend struct PairPos;

  private:
  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    unsigned int end = MIN (c->buffer->len, c->buffer->i + c->context_length);
    if (unlikely (c->buffer->i + 2 > end))
      return false;

    unsigned int index = (this+coverage) (c->buffer->info[c->buffer->i].codepoint);
    if (likely (index == NOT_COVERED))
      return false;

    unsigned int j = c->buffer->i + 1;
    while (_hb_ot_layout_skip_mark (c->layout->face, &c->buffer->info[j], c->lookup_flag, NULL))
    {
      if (unlikely (j == end))
	return false;
      j++;
    }

    return (this+pairSet[index]).apply (c, &valueFormat1, j);
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();

    unsigned int len1 = valueFormat1.get_len ();
    unsigned int len2 = valueFormat2.get_len ();
    PairSet::sanitize_closure_t closure = {
      this,
      &valueFormat1,
      len1,
      1 + len1 + len2
    };

    return c->check_struct (this)
	&& coverage.sanitize (c, this)
	&& pairSet.sanitize (c, this, &closure);
  }

  private:
  USHORT	format;			
  OffsetTo<Coverage>
		coverage;		

  ValueFormat	valueFormat1;		


  ValueFormat	valueFormat2;		


  OffsetArrayOf<PairSet>
		pairSet;		

  public:
  DEFINE_SIZE_ARRAY (10, pairSet);
};

struct PairPosFormat2
{
  friend struct PairPos;

  private:
  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    unsigned int end = MIN (c->buffer->len, c->buffer->i + c->context_length);
    if (unlikely (c->buffer->i + 2 > end))
      return false;

    unsigned int index = (this+coverage) (c->buffer->info[c->buffer->i].codepoint);
    if (likely (index == NOT_COVERED))
      return false;

    unsigned int j = c->buffer->i + 1;
    while (_hb_ot_layout_skip_mark (c->layout->face, &c->buffer->info[j], c->lookup_flag, NULL))
    {
      if (unlikely (j == end))
	return false;
      j++;
    }

    unsigned int len1 = valueFormat1.get_len ();
    unsigned int len2 = valueFormat2.get_len ();
    unsigned int record_len = len1 + len2;

    unsigned int klass1 = (this+classDef1) (c->buffer->info[c->buffer->i].codepoint);
    unsigned int klass2 = (this+classDef2) (c->buffer->info[j].codepoint);
    if (unlikely (klass1 >= class1Count || klass2 >= class2Count))
      return false;

    const Value *v = &values[record_len * (klass1 * class2Count + klass2)];
    valueFormat1.apply_value (c->layout, this, v, c->buffer->pos[c->buffer->i]);
    valueFormat2.apply_value (c->layout, this, v + len1, c->buffer->pos[j]);

    if (len2)
      j++;
    c->buffer->i = j;

    return true;
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    if (!(c->check_struct (this)
       && coverage.sanitize (c, this)
       && classDef1.sanitize (c, this)
       && classDef2.sanitize (c, this))) return false;

    unsigned int len1 = valueFormat1.get_len ();
    unsigned int len2 = valueFormat2.get_len ();
    unsigned int stride = len1 + len2;
    unsigned int record_size = valueFormat1.get_size () + valueFormat2.get_size ();
    unsigned int count = (unsigned int) class1Count * (unsigned int) class2Count;
    return c->check_array (values, record_size, count) &&
	   valueFormat1.sanitize_values_stride_unsafe (c, this, &values[0], count, stride) &&
	   valueFormat2.sanitize_values_stride_unsafe (c, this, &values[len1], count, stride);
  }

  private:
  USHORT	format;			
  OffsetTo<Coverage>
		coverage;		

  ValueFormat	valueFormat1;		


  ValueFormat	valueFormat2;		


  OffsetTo<ClassDef>
		classDef1;		


  OffsetTo<ClassDef>
		classDef2;		


  USHORT	class1Count;		

  USHORT	class2Count;		

  ValueRecord	values;			


  public:
  DEFINE_SIZE_ARRAY (16, values);
};

struct PairPos
{
  friend struct PosLookupSubTable;

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
  PairPosFormat1	format1;
  PairPosFormat2	format2;
  } u;
};


struct EntryExitRecord
{
  friend struct CursivePosFormat1;

  inline bool sanitize (hb_sanitize_context_t *c, void *base) {
    TRACE_SANITIZE ();
    return entryAnchor.sanitize (c, base)
	&& exitAnchor.sanitize (c, base);
  }

  private:
  OffsetTo<Anchor>
		entryAnchor;		


  OffsetTo<Anchor>
		exitAnchor;		


  public:
  DEFINE_SIZE_STATIC (4);
};

struct CursivePosFormat1
{
  friend struct CursivePos;

  private:
  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    




















































































































    struct hb_ot_layout_context_t::info_t::gpos_t *gpi = &c->layout->info.gpos;
    hb_codepoint_t last_pos = gpi->last;
    gpi->last = HB_OT_LAYOUT_GPOS_NO_LAST;

    
    if (c->property == HB_OT_LAYOUT_GLYPH_CLASS_MARK)
      return false;

    unsigned int index = (this+coverage) (c->buffer->info[c->buffer->i].codepoint);
    if (likely (index == NOT_COVERED))
      return false;

    const EntryExitRecord &record = entryExitRecord[index];

    if (last_pos == HB_OT_LAYOUT_GPOS_NO_LAST || !record.entryAnchor)
      goto end;

    hb_position_t entry_x, entry_y;
    (this+record.entryAnchor).get_anchor (c->layout, c->buffer->info[c->buffer->i].codepoint, &entry_x, &entry_y);

    

    if (c->buffer->direction == HB_DIRECTION_RTL)
    {
      
      c->buffer->pos[c->buffer->i].x_advance = entry_x - gpi->anchor_x;
    }
    else
    {
      
      c->buffer->pos[last_pos].x_advance = gpi->anchor_x - entry_x;
    }

    if  (c->lookup_flag & LookupFlag::RightToLeft)
    {
      c->buffer->pos[last_pos].cursive_chain = last_pos - c->buffer->i;
      c->buffer->pos[last_pos].y_offset = entry_y - gpi->anchor_y;
    }
    else
    {
      c->buffer->pos[c->buffer->i].cursive_chain = c->buffer->i - last_pos;
      c->buffer->pos[c->buffer->i].y_offset = gpi->anchor_y - entry_y;
    }

  end:
    if (record.exitAnchor)
    {
      gpi->last = c->buffer->i;
      (this+record.exitAnchor).get_anchor (c->layout, c->buffer->info[c->buffer->i].codepoint, &gpi->anchor_x, &gpi->anchor_y);
    }

    c->buffer->i++;
    return true;
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return coverage.sanitize (c, this)
	&& entryExitRecord.sanitize (c, this);
  }

  private:
  USHORT	format;			
  OffsetTo<Coverage>
		coverage;		

  ArrayOf<EntryExitRecord>
		entryExitRecord;	

  public:
  DEFINE_SIZE_ARRAY (6, entryExitRecord);
};

struct CursivePos
{
  friend struct PosLookupSubTable;

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
  CursivePosFormat1	format1;
  } u;
};


typedef AnchorMatrix BaseArray;		




struct MarkBasePosFormat1
{
  friend struct MarkBasePos;

  private:
  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    unsigned int mark_index = (this+markCoverage) (c->buffer->info[c->buffer->i].codepoint);
    if (likely (mark_index == NOT_COVERED))
      return false;

    
    unsigned int property;
    unsigned int j = c->buffer->i;
    do
    {
      if (unlikely (!j))
	return false;
      j--;
    } while (_hb_ot_layout_skip_mark (c->layout->face, &c->buffer->info[j], LookupFlag::IgnoreMarks, &property));

    
    if (false && !(property & HB_OT_LAYOUT_GLYPH_CLASS_BASE_GLYPH))
      return false;

    unsigned int base_index = (this+baseCoverage) (c->buffer->info[j].codepoint);
    if (base_index == NOT_COVERED)
      return false;

    return (this+markArray).apply (c, mark_index, base_index, this+baseArray, classCount, j);
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return c->check_struct (this)
        && markCoverage.sanitize (c, this)
	&& baseCoverage.sanitize (c, this)
	&& markArray.sanitize (c, this)
	&& baseArray.sanitize (c, this, (unsigned int) classCount);
  }

  private:
  USHORT	format;			
  OffsetTo<Coverage>
		markCoverage;		

  OffsetTo<Coverage>
		baseCoverage;		

  USHORT	classCount;		
  OffsetTo<MarkArray>
		markArray;		

  OffsetTo<BaseArray>
		baseArray;		

  public:
  DEFINE_SIZE_STATIC (12);
};

struct MarkBasePos
{
  friend struct PosLookupSubTable;

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
  MarkBasePosFormat1	format1;
  } u;
};


typedef AnchorMatrix LigatureAttach;	




typedef OffsetListOf<LigatureAttach> LigatureArray;
					



struct MarkLigPosFormat1
{
  friend struct MarkLigPos;

  private:
  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    unsigned int mark_index = (this+markCoverage) (c->buffer->info[c->buffer->i].codepoint);
    if (likely (mark_index == NOT_COVERED))
      return false;

    
    unsigned int property;
    unsigned int j = c->buffer->i;
    do
    {
      if (unlikely (!j))
	return false;
      j--;
    } while (_hb_ot_layout_skip_mark (c->layout->face, &c->buffer->info[j], LookupFlag::IgnoreMarks, &property));

    
    if (false && !(property & HB_OT_LAYOUT_GLYPH_CLASS_LIGATURE))
      return false;

    unsigned int lig_index = (this+ligatureCoverage) (c->buffer->info[j].codepoint);
    if (lig_index == NOT_COVERED)
      return false;

    const LigatureArray& lig_array = this+ligatureArray;
    const LigatureAttach& lig_attach = lig_array[lig_index];

    
    unsigned int comp_count = lig_attach.rows;
    if (unlikely (!comp_count))
      return false;
    unsigned int comp_index;
    



    if (c->buffer->info[j].lig_id && c->buffer->info[j].lig_id == c->buffer->info[c->buffer->i].lig_id && c->buffer->info[c->buffer->i].component)
    {
      comp_index = c->buffer->info[c->buffer->i].component - 1;
      if (comp_index >= comp_count)
	comp_index = comp_count - 1;
    }
    else
      comp_index = comp_count - 1;

    return (this+markArray).apply (c, mark_index, comp_index, lig_attach, classCount, j);
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return c->check_struct (this)
        && markCoverage.sanitize (c, this)
	&& ligatureCoverage.sanitize (c, this)
	&& markArray.sanitize (c, this)
	&& ligatureArray.sanitize (c, this, (unsigned int) classCount);
  }

  private:
  USHORT	format;			
  OffsetTo<Coverage>
		markCoverage;		

  OffsetTo<Coverage>
		ligatureCoverage;	


  USHORT	classCount;		
  OffsetTo<MarkArray>
		markArray;		

  OffsetTo<LigatureArray>
		ligatureArray;		

  public:
  DEFINE_SIZE_STATIC (12);
};

struct MarkLigPos
{
  friend struct PosLookupSubTable;

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
  MarkLigPosFormat1	format1;
  } u;
};


typedef AnchorMatrix Mark2Array;	




struct MarkMarkPosFormat1
{
  friend struct MarkMarkPos;

  private:
  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    unsigned int mark1_index = (this+mark1Coverage) (c->buffer->info[c->buffer->i].codepoint);
    if (likely (mark1_index == NOT_COVERED))
      return false;

    
    unsigned int property;
    unsigned int j = c->buffer->i;
    do
    {
      if (unlikely (!j))
	return false;
      j--;
    } while (_hb_ot_layout_skip_mark (c->layout->face, &c->buffer->info[j], c->lookup_flag, &property));

    if (!(property & HB_OT_LAYOUT_GLYPH_CLASS_MARK))
      return false;

    


    if ((c->buffer->info[j].component != c->buffer->info[c->buffer->i].component) ||
	(c->buffer->info[j].component && c->buffer->info[j].lig_id != c->buffer->info[c->buffer->i].lig_id))
      return false;

    unsigned int mark2_index = (this+mark2Coverage) (c->buffer->info[j].codepoint);
    if (mark2_index == NOT_COVERED)
      return false;

    return (this+mark1Array).apply (c, mark1_index, mark2_index, this+mark2Array, classCount, j);
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return c->check_struct (this)
	&& mark1Coverage.sanitize (c, this)
	&& mark2Coverage.sanitize (c, this)
	&& mark1Array.sanitize (c, this)
	&& mark2Array.sanitize (c, this, (unsigned int) classCount);
  }

  private:
  USHORT	format;			
  OffsetTo<Coverage>
		mark1Coverage;		


  OffsetTo<Coverage>
		mark2Coverage;		


  USHORT	classCount;		
  OffsetTo<MarkArray>
		mark1Array;		

  OffsetTo<Mark2Array>
		mark2Array;		

  public:
  DEFINE_SIZE_STATIC (12);
};

struct MarkMarkPos
{
  friend struct PosLookupSubTable;

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
  MarkMarkPosFormat1	format1;
  } u;
};


static inline bool position_lookup (hb_apply_context_t *c, unsigned int lookup_index);

struct ContextPos : Context
{
  friend struct PosLookupSubTable;

  private:
  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    return Context::apply (c, position_lookup);
  }
};

struct ChainContextPos : ChainContext
{
  friend struct PosLookupSubTable;

  private:
  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    return ChainContext::apply (c, position_lookup);
  }
};


struct ExtensionPos : Extension
{
  friend struct PosLookupSubTable;

  private:
  inline const struct PosLookupSubTable& get_subtable (void) const
  {
    unsigned int offset = get_offset ();
    if (unlikely (!offset)) return Null(PosLookupSubTable);
    return StructAtOffset<PosLookupSubTable> (this, offset);
  }

  inline bool apply (hb_apply_context_t *c) const;

  inline bool sanitize (hb_sanitize_context_t *c);
};








struct PosLookupSubTable
{
  friend struct PosLookup;

  enum {
    Single		= 1,
    Pair		= 2,
    Cursive		= 3,
    MarkBase		= 4,
    MarkLig		= 5,
    MarkMark		= 6,
    Context		= 7,
    ChainContext	= 8,
    Extension		= 9
  };

  inline bool apply (hb_apply_context_t *c, unsigned int lookup_type) const
  {
    TRACE_APPLY ();
    switch (lookup_type) {
    case Single:		return u.single.apply (c);
    case Pair:			return u.pair.apply (c);
    case Cursive:		return u.cursive.apply (c);
    case MarkBase:		return u.markBase.apply (c);
    case MarkLig:		return u.markLig.apply (c);
    case MarkMark:		return u.markMark.apply (c);
    case Context:		return u.c.apply (c);
    case ChainContext:		return u.chainContext.apply (c);
    case Extension:		return u.extension.apply (c);
    default:return false;
    }
  }

  inline bool sanitize (hb_sanitize_context_t *c, unsigned int lookup_type) {
    TRACE_SANITIZE ();
    switch (lookup_type) {
    case Single:		return u.single.sanitize (c);
    case Pair:			return u.pair.sanitize (c);
    case Cursive:		return u.cursive.sanitize (c);
    case MarkBase:		return u.markBase.sanitize (c);
    case MarkLig:		return u.markLig.sanitize (c);
    case MarkMark:		return u.markMark.sanitize (c);
    case Context:		return u.c.sanitize (c);
    case ChainContext:		return u.chainContext.sanitize (c);
    case Extension:		return u.extension.sanitize (c);
    default:return true;
    }
  }

  private:
  union {
  USHORT		sub_format;
  SinglePos		single;
  PairPos		pair;
  CursivePos		cursive;
  MarkBasePos		markBase;
  MarkLigPos		markLig;
  MarkMarkPos		markMark;
  ContextPos		c;
  ChainContextPos	chainContext;
  ExtensionPos		extension;
  } u;
  public:
  DEFINE_SIZE_UNION (2, sub_format);
};


struct PosLookup : Lookup
{
  inline const PosLookupSubTable& get_subtable (unsigned int i) const
  { return this+CastR<OffsetArrayOf<PosLookupSubTable> > (subTable)[i]; }

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
    c->lookup_flag = get_flag ();

    if (!_hb_ot_layout_check_glyph_property (c->layout->face, &c->buffer->info[c->buffer->i], c->lookup_flag, &c->property))
      return false;

    for (unsigned int i = 0; i < get_subtable_count (); i++)
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

    layout->info.gpos.last = HB_OT_LAYOUT_GPOS_NO_LAST; 

    buffer->i = 0;
    while (buffer->i < buffer->len)
    {
      bool done;
      if (buffer->info[buffer->i].mask & mask)
      {
	  done = apply_once (layout, buffer, mask, NO_CONTEXT, MAX_NESTING_LEVEL);
	  ret |= done;
      }
      else
      {
          done = false;
	  

	  layout->info.gpos.last = HB_OT_LAYOUT_GPOS_NO_LAST;
      }

      if (!done)
	buffer->i++;
    }

    return ret;
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    if (unlikely (!Lookup::sanitize (c))) return false;
    OffsetArrayOf<PosLookupSubTable> &list = CastR<OffsetArrayOf<PosLookupSubTable> > (subTable);
    return list.sanitize (c, this, get_type ());
  }
};

typedef OffsetListOf<PosLookup> PosLookupList;





struct GPOS : GSUBGPOS
{
  static const hb_tag_t Tag	= HB_OT_TAG_GPOS;

  inline const PosLookup& get_lookup (unsigned int i) const
  { return CastR<PosLookup> (GSUBGPOS::get_lookup (i)); }

  inline bool position_lookup (hb_ot_layout_context_t *layout,
			       hb_buffer_t  *buffer,
			       unsigned int  lookup_index,
			       hb_mask_t     mask) const
  { return get_lookup (lookup_index).apply_string (layout, buffer, mask); }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    if (unlikely (!GSUBGPOS::sanitize (c))) return false;
    OffsetTo<PosLookupList> &list = CastR<OffsetTo<PosLookupList> > (lookupList);
    return list.sanitize (c, this);
  }
  public:
  DEFINE_SIZE_STATIC (10);
};




inline bool ExtensionPos::apply (hb_apply_context_t *c) const
{
  TRACE_APPLY ();
  return get_subtable ().apply (c, get_type ());
}

inline bool ExtensionPos::sanitize (hb_sanitize_context_t *c)
{
  TRACE_SANITIZE ();
  if (unlikely (!Extension::sanitize (c))) return false;
  unsigned int offset = get_offset ();
  if (unlikely (!offset)) return true;
  return StructAtOffset<PosLookupSubTable> (this, offset).sanitize (c, get_type ());
}

static inline bool position_lookup (hb_apply_context_t *c, unsigned int lookup_index)
{
  const GPOS &gpos = *(c->layout->face->ot_layout->gpos);
  const PosLookup &l = gpos.get_lookup (lookup_index);

  if (unlikely (c->nesting_level_left == 0))
    return false;

  if (unlikely (c->context_length < 1))
    return false;

  return l.apply_once (c->layout, c->buffer, c->lookup_mask, c->context_length, c->nesting_level_left - 1);
}


#endif 
