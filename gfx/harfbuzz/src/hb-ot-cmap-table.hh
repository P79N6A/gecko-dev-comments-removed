

























#ifndef HB_OT_CMAP_TABLE_HH
#define HB_OT_CMAP_TABLE_HH

#include "hb-open-type-private.hh"


namespace OT {






#define HB_OT_TAG_cmap HB_TAG('c','m','a','p')


struct CmapSubtableFormat0
{
  inline bool get_glyph (hb_codepoint_t codepoint, hb_codepoint_t *glyph) const
  {
    hb_codepoint_t gid = codepoint < 256 ? glyphIdArray[codepoint] : 0;
    if (!gid)
      return false;
    *glyph = gid;
    return true;
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE (this);
    return TRACE_RETURN (c->check_struct (this));
  }

  protected:
  USHORT	format;		
  USHORT	lengthZ;	
  USHORT	languageZ;	
  BYTE		glyphIdArray[256];

  public:
  DEFINE_SIZE_STATIC (6 + 256);
};

struct CmapSubtableFormat4
{
  inline bool get_glyph (hb_codepoint_t codepoint, hb_codepoint_t *glyph) const
  {
    unsigned int segCount;
    const USHORT *endCount;
    const USHORT *startCount;
    const USHORT *idDelta;
    const USHORT *idRangeOffset;
    const USHORT *glyphIdArray;
    unsigned int glyphIdArrayLength;

    segCount = this->segCountX2 / 2;
    endCount = this->values;
    startCount = endCount + segCount + 1;
    idDelta = startCount + segCount;
    idRangeOffset = idDelta + segCount;
    glyphIdArray = idRangeOffset + segCount;
    glyphIdArrayLength = (this->length - 16 - 8 * segCount) / 2;

    
    int min = 0, max = (int) segCount - 1;
    unsigned int i;
    while (min <= max)
    {
      int mid = (min + max) / 2;
      if (codepoint < startCount[mid])
        max = mid - 1;
      else if (codepoint > endCount[mid])
        min = mid + 1;
      else
      {
	i = mid;
	goto found;
      }
    }
    return false;

  found:
    hb_codepoint_t gid;
    unsigned int rangeOffset = idRangeOffset[i];
    if (rangeOffset == 0)
      gid = codepoint + idDelta[i];
    else
    {
      
      unsigned int index = rangeOffset / 2 + (codepoint - startCount[i]) + i - segCount;
      if (unlikely (index >= glyphIdArrayLength))
	return false;
      gid = glyphIdArray[index];
      if (unlikely (!gid))
	return false;
      gid += idDelta[i];
    }

    *glyph = gid & 0xFFFFu;
    return true;
  }

  inline bool sanitize (hb_sanitize_context_t *c)
  {
    TRACE_SANITIZE (this);
    if (unlikely (!c->check_struct (this)))
      return TRACE_RETURN (false);

    if (unlikely (!c->check_range (this, length)))
    {
      


      uint16_t new_length = (uint16_t) MIN ((uintptr_t) 65535,
					    (uintptr_t) (c->end -
							 (char *) this));
      if (!c->try_set (&length, new_length))
	return TRACE_RETURN (false);
    }

    return TRACE_RETURN (16 + 4 * (unsigned int) segCountX2 <= length);
  }

  protected:
  USHORT	format;		
  USHORT	length;		

  USHORT	languageZ;	
  USHORT	segCountX2;	
  USHORT	searchRangeZ;	
  USHORT	entrySelectorZ;	
  USHORT	rangeShiftZ;	

  USHORT	values[VAR];
#if 0
  USHORT	endCount[segCount];	

  USHORT	reservedPad;		
  USHORT	startCount[segCount];	
  SHORT		idDelta[segCount];	
  USHORT	idRangeOffset[segCount];
  USHORT	glyphIdArray[VAR];	
#endif

  public:
  DEFINE_SIZE_ARRAY (14, values);
};

struct CmapSubtableLongGroup
{
  friend struct CmapSubtableFormat12;
  friend struct CmapSubtableFormat13;

  int cmp (hb_codepoint_t codepoint) const
  {
    if (codepoint < startCharCode) return -1;
    if (codepoint > endCharCode)   return +1;
    return 0;
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE (this);
    return TRACE_RETURN (c->check_struct (this));
  }

  private:
  ULONG		startCharCode;	
  ULONG		endCharCode;	
  ULONG		glyphID;	

  public:
  DEFINE_SIZE_STATIC (12);
};

template <typename UINT>
struct CmapSubtableTrimmed
{
  inline bool get_glyph (hb_codepoint_t codepoint, hb_codepoint_t *glyph) const
  {
    
    hb_codepoint_t gid = glyphIdArray[codepoint - startCharCode];
    if (!gid)
      return false;
    *glyph = gid;
    return true;
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE (this);
    return TRACE_RETURN (c->check_struct (this) && glyphIdArray.sanitize (c));
  }

  protected:
  UINT		formatReserved;	
  UINT		lengthZ;	
  UINT		languageZ;	
  UINT		startCharCode;	
  ArrayOf<GlyphID, UINT>
		glyphIdArray;	

  public:
  DEFINE_SIZE_ARRAY (5 * sizeof (UINT), glyphIdArray);
};

struct CmapSubtableFormat6  : CmapSubtableTrimmed<USHORT> {};
struct CmapSubtableFormat10 : CmapSubtableTrimmed<ULONG > {};

template <typename T>
struct CmapSubtableLongSegmented
{
  inline bool get_glyph (hb_codepoint_t codepoint, hb_codepoint_t *glyph) const
  {
    int i = groups.bsearch (codepoint);
    if (i == -1)
      return false;
    *glyph = T::group_get_glyph (groups[i], codepoint);
    return true;
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE (this);
    return TRACE_RETURN (c->check_struct (this) && groups.sanitize (c));
  }

  protected:
  USHORT	format;		
  USHORT	reservedZ;	
  ULONG		lengthZ;	
  ULONG		languageZ;	
  SortedArrayOf<CmapSubtableLongGroup, ULONG>
		groups;		
  public:
  DEFINE_SIZE_ARRAY (16, groups);
};

struct CmapSubtableFormat12 : CmapSubtableLongSegmented<CmapSubtableFormat12>
{
  static inline hb_codepoint_t group_get_glyph (const CmapSubtableLongGroup &group,
						hb_codepoint_t u)
  { return group.glyphID + (u - group.startCharCode); }
};

struct CmapSubtableFormat13 : CmapSubtableLongSegmented<CmapSubtableFormat13>
{
  static inline hb_codepoint_t group_get_glyph (const CmapSubtableLongGroup &group,
						hb_codepoint_t u HB_UNUSED)
  { return group.glyphID; }
};

typedef enum
{
  GLYPH_VARIANT_NOT_FOUND = 0,
  GLYPH_VARIANT_FOUND = 1,
  GLYPH_VARIANT_USE_DEFAULT = 2
} glyph_variant_t;

struct UnicodeValueRange
{
  inline int cmp (const hb_codepoint_t &codepoint) const
  {
    if (codepoint < startUnicodeValue) return -1;
    if (codepoint > startUnicodeValue + additionalCount) return +1;
    return 0;
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE (this);
    return TRACE_RETURN (c->check_struct (this));
  }

  UINT24	startUnicodeValue;	
  BYTE		additionalCount;	

  public:
  DEFINE_SIZE_STATIC (4);
};

typedef SortedArrayOf<UnicodeValueRange, ULONG> DefaultUVS;

struct UVSMapping
{
  inline int cmp (const hb_codepoint_t &codepoint) const
  {
    return unicodeValue.cmp (codepoint);
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE (this);
    return TRACE_RETURN (c->check_struct (this));
  }

  UINT24	unicodeValue;	
  GlyphID	glyphID;	
  public:
  DEFINE_SIZE_STATIC (5);
};

typedef SortedArrayOf<UVSMapping, ULONG> NonDefaultUVS;

struct VariationSelectorRecord
{
  inline glyph_variant_t get_glyph (hb_codepoint_t codepoint,
				    hb_codepoint_t *glyph,
				    const void *base) const
  {
    int i;
    const DefaultUVS &defaults = base+defaultUVS;
    i = defaults.bsearch (codepoint);
    if (i != -1)
      return GLYPH_VARIANT_USE_DEFAULT;
    const NonDefaultUVS &nonDefaults = base+nonDefaultUVS;
    i = nonDefaults.bsearch (codepoint);
    if (i != -1)
    {
      *glyph = nonDefaults[i].glyphID;
       return GLYPH_VARIANT_FOUND;
    }
    return GLYPH_VARIANT_NOT_FOUND;
  }

  inline int cmp (const hb_codepoint_t &variation_selector) const
  {
    return varSelector.cmp (variation_selector);
  }

  inline bool sanitize (hb_sanitize_context_t *c, void *base) {
    TRACE_SANITIZE (this);
    return TRACE_RETURN (c->check_struct (this) &&
			 defaultUVS.sanitize (c, base) &&
			 nonDefaultUVS.sanitize (c, base));
  }

  UINT24	varSelector;	
  OffsetTo<DefaultUVS, ULONG>
		defaultUVS;	
  OffsetTo<NonDefaultUVS, ULONG>
		nonDefaultUVS;	
  public:
  DEFINE_SIZE_STATIC (11);
};

struct CmapSubtableFormat14
{
  inline glyph_variant_t get_glyph_variant (hb_codepoint_t codepoint,
					    hb_codepoint_t variation_selector,
					    hb_codepoint_t *glyph) const
  {
    return record[record.bsearch(variation_selector)].get_glyph (codepoint, glyph, this);
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE (this);
    return TRACE_RETURN (c->check_struct (this) &&
			 record.sanitize (c, this));
  }

  protected:
  USHORT	format;		
  ULONG		lengthZ;	
  SortedArrayOf<VariationSelectorRecord, ULONG>
		record;		

  public:
  DEFINE_SIZE_ARRAY (10, record);
};

struct CmapSubtable
{
  

  inline bool get_glyph (hb_codepoint_t codepoint,
			 hb_codepoint_t *glyph) const
  {
    switch (u.format) {
    case  0: return u.format0 .get_glyph(codepoint, glyph);
    case  4: return u.format4 .get_glyph(codepoint, glyph);
    case  6: return u.format6 .get_glyph(codepoint, glyph);
    case 10: return u.format10.get_glyph(codepoint, glyph);
    case 12: return u.format12.get_glyph(codepoint, glyph);
    case 13: return u.format13.get_glyph(codepoint, glyph);
    case 14:
    default: return false;
    }
  }

  inline glyph_variant_t get_glyph_variant (hb_codepoint_t codepoint,
					    hb_codepoint_t variation_selector,
					    hb_codepoint_t *glyph) const
  {
    switch (u.format) {
    case 14: return u.format14.get_glyph_variant(codepoint, variation_selector, glyph);
    default: return GLYPH_VARIANT_NOT_FOUND;
    }
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE (this);
    if (!u.format.sanitize (c)) return TRACE_RETURN (false);
    switch (u.format) {
    case  0: return TRACE_RETURN (u.format0 .sanitize (c));
    case  4: return TRACE_RETURN (u.format4 .sanitize (c));
    case  6: return TRACE_RETURN (u.format6 .sanitize (c));
    case 10: return TRACE_RETURN (u.format10.sanitize (c));
    case 12: return TRACE_RETURN (u.format12.sanitize (c));
    case 13: return TRACE_RETURN (u.format13.sanitize (c));
    case 14: return TRACE_RETURN (u.format14.sanitize (c));
    default:return TRACE_RETURN (true);
    }
  }

  protected:
  union {
  USHORT		format;		
  CmapSubtableFormat0	format0;
  CmapSubtableFormat4	format4;
  CmapSubtableFormat6	format6;
  CmapSubtableFormat10	format10;
  CmapSubtableFormat12	format12;
  CmapSubtableFormat13	format13;
  CmapSubtableFormat14	format14;
  } u;
  public:
  DEFINE_SIZE_UNION (2, format);
};


struct EncodingRecord
{
  inline int cmp (const EncodingRecord &other) const
  {
    int ret;
    ret = platformID.cmp (other.platformID);
    if (ret) return ret;
    ret = encodingID.cmp (other.encodingID);
    if (ret) return ret;
    return 0;
  }

  inline bool sanitize (hb_sanitize_context_t *c, void *base) {
    TRACE_SANITIZE (this);
    return TRACE_RETURN (c->check_struct (this) &&
			 subtable.sanitize (c, base));
  }

  USHORT	platformID;	
  USHORT	encodingID;	
  OffsetTo<CmapSubtable, ULONG>
		subtable;	
  public:
  DEFINE_SIZE_STATIC (8);
};

struct cmap
{
  static const hb_tag_t tableTag	= HB_OT_TAG_cmap;

  inline const CmapSubtable *find_subtable (unsigned int platform_id,
					    unsigned int encoding_id) const
  {
    EncodingRecord key;
    key.platformID.set (platform_id);
    key.encodingID.set (encoding_id);

    


    int result = encodingRecord.lsearch (key);
    if (result == -1 || !encodingRecord[result].subtable)
      return NULL;

    return &(this+encodingRecord[result].subtable);
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE (this);
    return TRACE_RETURN (c->check_struct (this) &&
			 likely (version == 0) &&
			 encodingRecord.sanitize (c, this));
  }

  USHORT		version;	
  SortedArrayOf<EncodingRecord>
			encodingRecord;	
  public:
  DEFINE_SIZE_ARRAY (4, encodingRecord);
};


} 


#endif 
