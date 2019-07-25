



























#ifndef HB_OT_LAYOUT_GDEF_PRIVATE_HH
#define HB_OT_LAYOUT_GDEF_PRIVATE_HH

#include "hb-ot-layout-common-private.hh"

#include "hb-font-private.h"

HB_BEGIN_DECLS






typedef ArrayOf<USHORT> AttachPoint;	


struct AttachList
{
  inline unsigned int get_attach_points (hb_codepoint_t glyph_id,
					 unsigned int start_offset,
					 unsigned int *point_count ,
					 unsigned int *point_array ) const
  {
    unsigned int index = (this+coverage) (glyph_id);
    if (index == NOT_COVERED)
    {
      if (point_count)
	*point_count = 0;
      return 0;
    }

    const AttachPoint &points = this+attachPoint[index];

    if (point_count) {
      const USHORT *array = points.sub_array (start_offset, point_count);
      unsigned int count = *point_count;
      for (unsigned int i = 0; i < count; i++)
	point_array[i] = array[i];
    }

    return points.len;
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return coverage.sanitize (c, this)
	&& attachPoint.sanitize (c, this);
  }

  private:
  OffsetTo<Coverage>
		coverage;		

  OffsetArrayOf<AttachPoint>
		attachPoint;		

  public:
  DEFINE_SIZE_ARRAY (4, attachPoint);
};





struct CaretValueFormat1
{
  friend struct CaretValue;

  private:
  inline int get_caret_value (hb_ot_layout_context_t *c, hb_direction_t direction, hb_codepoint_t glyph_id HB_UNUSED) const
  {
    return HB_DIRECTION_IS_HORIZONTAL (direction) ? c->scale_x (coordinate) : c->scale_y (coordinate);
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return c->check_struct (this);
  }

  private:
  USHORT	caretValueFormat;	
  SHORT		coordinate;		
  public:
  DEFINE_SIZE_STATIC (4);
};

struct CaretValueFormat2
{
  friend struct CaretValue;

  private:
  inline int get_caret_value (hb_ot_layout_context_t *c, hb_direction_t direction, hb_codepoint_t glyph_id) const
  {
    hb_position_t x, y;
    if (hb_font_get_contour_point (c->font, c->face, caretValuePoint, glyph_id, &x, &y))
      return HB_DIRECTION_IS_HORIZONTAL (direction) ? x : y;
    else
      return 0;
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return c->check_struct (this);
  }

  private:
  USHORT	caretValueFormat;	
  USHORT	caretValuePoint;	
  public:
  DEFINE_SIZE_STATIC (4);
};

struct CaretValueFormat3
{
  friend struct CaretValue;

  inline int get_caret_value (hb_ot_layout_context_t *c, hb_direction_t direction, hb_codepoint_t glyph_id) const
  {
    return HB_DIRECTION_IS_HORIZONTAL (direction) ?
           c->scale_x (coordinate) + (this+deviceTable).get_x_delta (c) :
           c->scale_y (coordinate) + (this+deviceTable).get_y_delta (c);
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return c->check_struct (this)
	&& deviceTable.sanitize (c, this);
  }

  private:
  USHORT	caretValueFormat;	
  SHORT		coordinate;		
  OffsetTo<Device>
		deviceTable;		


  public:
  DEFINE_SIZE_STATIC (6);
};

struct CaretValue
{
  inline int get_caret_value (hb_ot_layout_context_t *c, hb_direction_t direction, hb_codepoint_t glyph_id) const
  {
    switch (u.format) {
    case 1: return u.format1.get_caret_value (c, direction, glyph_id);
    case 2: return u.format2.get_caret_value (c, direction, glyph_id);
    case 3: return u.format3.get_caret_value (c, direction, glyph_id);
    default:return 0;
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
  CaretValueFormat1	format1;
  CaretValueFormat2	format2;
  CaretValueFormat3	format3;
  } u;
  public:
  DEFINE_SIZE_UNION (2, format);
};

struct LigGlyph
{
  inline unsigned int get_lig_carets (hb_ot_layout_context_t *c,
				      hb_direction_t direction,
				      hb_codepoint_t glyph_id,
				      unsigned int start_offset,
				      unsigned int *caret_count ,
				      int *caret_array ) const
  {
    if (caret_count) {
      const OffsetTo<CaretValue> *array = carets.sub_array (start_offset, caret_count);
      unsigned int count = *caret_count;
      for (unsigned int i = 0; i < count; i++)
	caret_array[i] = (this+array[i]).get_caret_value (c, direction, glyph_id);
    }

    return carets.len;
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return carets.sanitize (c, this);
  }

  private:
  OffsetArrayOf<CaretValue>
		carets;			


  public:
  DEFINE_SIZE_ARRAY (2, carets);
};

struct LigCaretList
{
  inline unsigned int get_lig_carets (hb_ot_layout_context_t *c,
				      hb_direction_t direction,
				      hb_codepoint_t glyph_id,
				      unsigned int start_offset,
				      unsigned int *caret_count ,
				      int *caret_array ) const
  {
    unsigned int index = (this+coverage) (glyph_id);
    if (index == NOT_COVERED)
    {
      if (caret_count)
	*caret_count = 0;
      return 0;
    }
    const LigGlyph &lig_glyph = this+ligGlyph[index];
    return lig_glyph.get_lig_carets (c, direction, glyph_id, start_offset, caret_count, caret_array);
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return coverage.sanitize (c, this)
	&& ligGlyph.sanitize (c, this);
  }

  private:
  OffsetTo<Coverage>
		coverage;		

  OffsetArrayOf<LigGlyph>
		ligGlyph;		

  public:
  DEFINE_SIZE_ARRAY (4, ligGlyph);
};


struct MarkGlyphSetsFormat1
{
  inline bool covers (unsigned int set_index, hb_codepoint_t glyph_id) const
  { return (this+coverage[set_index]).get_coverage (glyph_id) != NOT_COVERED; }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return coverage.sanitize (c, this);
  }

  private:
  USHORT	format;			
  LongOffsetArrayOf<Coverage>
		coverage;		

  public:
  DEFINE_SIZE_ARRAY (4, coverage);
};

struct MarkGlyphSets
{
  inline bool covers (unsigned int set_index, hb_codepoint_t glyph_id) const
  {
    switch (u.format) {
    case 1: return u.format1.covers (set_index, glyph_id);
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
  MarkGlyphSetsFormat1	format1;
  } u;
  public:
  DEFINE_SIZE_UNION (2, format);
};






struct GDEF
{
  static const hb_tag_t Tag	= HB_OT_TAG_GDEF;

  enum {
    UnclassifiedGlyph	= 0,
    BaseGlyph		= 1,
    LigatureGlyph	= 2,
    MarkGlyph		= 3,
    ComponentGlyph	= 4
  };

  inline bool has_glyph_classes (void) const { return glyphClassDef != 0; }
  inline unsigned int get_glyph_class (hb_codepoint_t glyph) const
  { return (this+glyphClassDef).get_class (glyph); }

  inline bool has_mark_attachment_types (void) const { return markAttachClassDef != 0; }
  inline unsigned int get_mark_attachment_type (hb_codepoint_t glyph) const
  { return (this+markAttachClassDef).get_class (glyph); }

  inline bool has_attach_points (void) const { return attachList != 0; }
  inline unsigned int get_attach_points (hb_codepoint_t glyph_id,
					 unsigned int start_offset,
					 unsigned int *point_count ,
					 unsigned int *point_array ) const
  { return (this+attachList).get_attach_points (glyph_id, start_offset, point_count, point_array); }

  inline bool has_lig_carets (void) const { return ligCaretList != 0; }
  inline unsigned int get_lig_carets (hb_ot_layout_context_t *c,
				      hb_direction_t direction,
				      hb_codepoint_t glyph_id,
				      unsigned int start_offset,
				      unsigned int *caret_count ,
				      int *caret_array ) const
  { return (this+ligCaretList).get_lig_carets (c, direction, glyph_id, start_offset, caret_count, caret_array); }

  inline bool has_mark_sets (void) const { return version >= 0x00010002 && markGlyphSetsDef[0] != 0; }
  inline bool mark_set_covers (unsigned int set_index, hb_codepoint_t glyph_id) const
  { return version >= 0x00010002 && (this+markGlyphSetsDef[0]).covers (set_index, glyph_id); }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return version.sanitize (c) && likely (version.major == 1)
	&& glyphClassDef.sanitize (c, this)
	&& attachList.sanitize (c, this)
	&& ligCaretList.sanitize (c, this)
	&& markAttachClassDef.sanitize (c, this)
	&& (version < 0x00010002 || markGlyphSetsDef[0].sanitize (c, this));
  }


  


  inline unsigned int get_glyph_props (hb_codepoint_t glyph) const
  {
    unsigned int klass = get_glyph_class (glyph);

    switch (klass) {
    default:
    case UnclassifiedGlyph:	return HB_OT_LAYOUT_GLYPH_CLASS_UNCLASSIFIED;
    case BaseGlyph:		return HB_OT_LAYOUT_GLYPH_CLASS_BASE_GLYPH;
    case LigatureGlyph:		return HB_OT_LAYOUT_GLYPH_CLASS_LIGATURE;
    case ComponentGlyph:	return HB_OT_LAYOUT_GLYPH_CLASS_COMPONENT;
    case MarkGlyph:
	  klass = get_mark_attachment_type (glyph);
	  return HB_OT_LAYOUT_GLYPH_CLASS_MARK | (klass << 8);
    }
  }


  private:
  FixedVersion	version;		

  OffsetTo<ClassDef>
		glyphClassDef;		


  OffsetTo<AttachList>
		attachList;		


  OffsetTo<LigCaretList>
		ligCaretList;		


  OffsetTo<ClassDef>
		markAttachClassDef;	


  OffsetTo<MarkGlyphSets>
		markGlyphSetsDef[VAR];	



  public:
  DEFINE_SIZE_ARRAY (12, markGlyphSetsDef);
};


HB_END_DECLS

#endif 
