



























#ifndef HB_OT_LAYOUT_COMMON_PRIVATE_HH
#define HB_OT_LAYOUT_COMMON_PRIVATE_HH

#include "hb-ot-layout-private.hh"
#include "hb-open-type-private.hh"
#include "hb-set-private.hh"


namespace OT {


#define NOT_COVERED		((unsigned int) -1)
#define MAX_NESTING_LEVEL	8














template <typename Type>
struct Record
{
  inline int cmp (hb_tag_t a) const {
    return tag.cmp (a);
  }

  struct sanitize_closure_t {
    hb_tag_t tag;
    void *list_base;
  };
  inline bool sanitize (hb_sanitize_context_t *c, void *base) {
    TRACE_SANITIZE (this);
    const sanitize_closure_t closure = {tag, base};
    return TRACE_RETURN (c->check_struct (this) && offset.sanitize (c, base, &closure));
  }

  Tag		tag;		
  OffsetTo<Type>
		offset;		

  public:
  DEFINE_SIZE_STATIC (6);
};

template <typename Type>
struct RecordArrayOf : SortedArrayOf<Record<Type> > {
  inline const Tag& get_tag (unsigned int i) const
  {
    


    if (unlikely (i >= this->len)) return Null(Tag);
    return (*this)[i].tag;
  }
  inline unsigned int get_tags (unsigned int start_offset,
				unsigned int *record_count ,
				hb_tag_t     *record_tags ) const
  {
    if (record_count) {
      const Record<Type> *arr = this->sub_array (start_offset, record_count);
      unsigned int count = *record_count;
      for (unsigned int i = 0; i < count; i++)
	record_tags[i] = arr[i].tag;
    }
    return this->len;
  }
  inline bool find_index (hb_tag_t tag, unsigned int *index) const
  {
    int i = this->search (tag);
    if (i != -1) {
        if (index) *index = i;
        return true;
    } else {
      if (index) *index = Index::NOT_FOUND_INDEX;
      return false;
    }
  }
};

template <typename Type>
struct RecordListOf : RecordArrayOf<Type>
{
  inline const Type& operator [] (unsigned int i) const
  { return this+RecordArrayOf<Type>::operator [](i).offset; }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE (this);
    return TRACE_RETURN (RecordArrayOf<Type>::sanitize (c, this));
  }
};


struct RangeRecord
{
  inline int cmp (hb_codepoint_t g) const {
    hb_codepoint_t a = start, b = end;
    return g < a ? -1 : g <= b ? 0 : +1 ;
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE (this);
    return TRACE_RETURN (c->check_struct (this));
  }

  inline bool intersects (const hb_set_t *glyphs) const {
    return glyphs->intersects (start, end);
  }

  template <typename set_t>
  inline void add_coverage (set_t *glyphs) const {
    glyphs->add_range (start, end);
  }

  GlyphID	start;		
  GlyphID	end;		
  USHORT	value;		
  public:
  DEFINE_SIZE_STATIC (6);
};
DEFINE_NULL_DATA (RangeRecord, "\000\001");


struct IndexArray : ArrayOf<Index>
{
  inline unsigned int get_indexes (unsigned int start_offset,
				   unsigned int *_count ,
				   unsigned int *_indexes ) const
  {
    if (_count) {
      const USHORT *arr = this->sub_array (start_offset, _count);
      unsigned int count = *_count;
      for (unsigned int i = 0; i < count; i++)
	_indexes[i] = arr[i];
    }
    return this->len;
  }
};


struct Script;
struct LangSys;
struct Feature;


struct LangSys
{
  inline unsigned int get_feature_count (void) const
  { return featureIndex.len; }
  inline hb_tag_t get_feature_index (unsigned int i) const
  { return featureIndex[i]; }
  inline unsigned int get_feature_indexes (unsigned int start_offset,
					   unsigned int *feature_count ,
					   unsigned int *feature_indexes ) const
  { return featureIndex.get_indexes (start_offset, feature_count, feature_indexes); }

  inline bool has_required_feature (void) const { return reqFeatureIndex != 0xffff; }
  inline unsigned int get_required_feature_index (void) const
  {
    if (reqFeatureIndex == 0xffff)
      return Index::NOT_FOUND_INDEX;
   return reqFeatureIndex;;
  }

  inline bool sanitize (hb_sanitize_context_t *c,
			const Record<LangSys>::sanitize_closure_t * = NULL) {
    TRACE_SANITIZE (this);
    return TRACE_RETURN (c->check_struct (this) && featureIndex.sanitize (c));
  }

  Offset	lookupOrder;	

  USHORT	reqFeatureIndex;


  IndexArray	featureIndex;	
  public:
  DEFINE_SIZE_ARRAY (6, featureIndex);
};
DEFINE_NULL_DATA (LangSys, "\0\0\xFF\xFF");


struct Script
{
  inline unsigned int get_lang_sys_count (void) const
  { return langSys.len; }
  inline const Tag& get_lang_sys_tag (unsigned int i) const
  { return langSys.get_tag (i); }
  inline unsigned int get_lang_sys_tags (unsigned int start_offset,
					 unsigned int *lang_sys_count ,
					 hb_tag_t     *lang_sys_tags ) const
  { return langSys.get_tags (start_offset, lang_sys_count, lang_sys_tags); }
  inline const LangSys& get_lang_sys (unsigned int i) const
  {
    if (i == Index::NOT_FOUND_INDEX) return get_default_lang_sys ();
    return this+langSys[i].offset;
  }
  inline bool find_lang_sys_index (hb_tag_t tag, unsigned int *index) const
  { return langSys.find_index (tag, index); }

  inline bool has_default_lang_sys (void) const { return defaultLangSys != 0; }
  inline const LangSys& get_default_lang_sys (void) const { return this+defaultLangSys; }

  inline bool sanitize (hb_sanitize_context_t *c,
			const Record<Script>::sanitize_closure_t * = NULL) {
    TRACE_SANITIZE (this);
    return TRACE_RETURN (defaultLangSys.sanitize (c, this) && langSys.sanitize (c, this));
  }

  protected:
  OffsetTo<LangSys>
		defaultLangSys;	

  RecordArrayOf<LangSys>
		langSys;	

  public:
  DEFINE_SIZE_ARRAY (4, langSys);
};

typedef RecordListOf<Script> ScriptList;



struct FeatureParamsSize
{
  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE (this);
    if (unlikely (!c->check_struct (this))) return TRACE_RETURN (false);

    
















































    if (!designSize)
      return TRACE_RETURN (false);
    else if (subfamilyID == 0 &&
	     subfamilyNameID == 0 &&
	     rangeStart == 0 &&
	     rangeEnd == 0)
      return TRACE_RETURN (true);
    else if (designSize < rangeStart ||
	     designSize > rangeEnd ||
	     subfamilyNameID < 256 ||
	     subfamilyNameID > 32767)
      return TRACE_RETURN (false);
    else
      return TRACE_RETURN (true);
  }

  USHORT	designSize;	




  USHORT	subfamilyID;	









  USHORT	subfamilyNameID;













  USHORT	rangeStart;	


  USHORT	rangeEnd;	


  public:
  DEFINE_SIZE_STATIC (10);
};


struct FeatureParamsStylisticSet
{
  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE (this);
    

    return TRACE_RETURN (c->check_struct (this));
  }

  USHORT	minorVersion;	




  USHORT	uiNameID;	













  public:
  DEFINE_SIZE_STATIC (4);
};

struct FeatureParamsCharacterVariants
{
  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE (this);
    return TRACE_RETURN (c->check_struct (this) &&
			 characters.sanitize (c));
  }

  USHORT	format;			
  USHORT	featUILableNameID;	




  USHORT	featUITooltipTextNameID;





  USHORT	sampleTextNameID;	



  USHORT	numNamedParameters;	

  USHORT	firstParamUILabelNameID;




  ArrayOf<UINT24>
		characters;		



  public:
  DEFINE_SIZE_ARRAY (14, characters);
};

struct FeatureParams
{
  inline bool sanitize (hb_sanitize_context_t *c, hb_tag_t tag) {
    TRACE_SANITIZE (this);
    if (tag == HB_TAG ('s','i','z','e'))
      return TRACE_RETURN (u.size.sanitize (c));
    if ((tag & 0xFFFF0000) == HB_TAG ('s','s','\0','\0')) 
      return TRACE_RETURN (u.stylisticSet.sanitize (c));
    if ((tag & 0xFFFF0000) == HB_TAG ('c','v','\0','\0')) 
      return TRACE_RETURN (u.characterVariants.sanitize (c));
    return TRACE_RETURN (true);
  }

  inline const FeatureParamsSize& get_size_params (hb_tag_t tag) const
  {
    if (tag == HB_TAG ('s','i','z','e'))
      return u.size;
    return Null(FeatureParamsSize);
  }

  private:
  union {
  FeatureParamsSize			size;
  FeatureParamsStylisticSet		stylisticSet;
  FeatureParamsCharacterVariants	characterVariants;
  } u;
  DEFINE_SIZE_STATIC (17);
};

struct Feature
{
  inline unsigned int get_lookup_count (void) const
  { return lookupIndex.len; }
  inline hb_tag_t get_lookup_index (unsigned int i) const
  { return lookupIndex[i]; }
  inline unsigned int get_lookup_indexes (unsigned int start_index,
					  unsigned int *lookup_count ,
					  unsigned int *lookup_tags ) const
  { return lookupIndex.get_indexes (start_index, lookup_count, lookup_tags); }

  inline const FeatureParams &get_feature_params (void) const
  { return this+featureParams; }

  inline bool sanitize (hb_sanitize_context_t *c,
			const Record<Feature>::sanitize_closure_t *closure) {
    TRACE_SANITIZE (this);
    if (unlikely (!(c->check_struct (this) && lookupIndex.sanitize (c))))
      return TRACE_RETURN (false);

    










    Offset orig_offset = featureParams;
    if (unlikely (!featureParams.sanitize (c, this, closure ? closure->tag : HB_TAG_NONE)))
      return TRACE_RETURN (false);

    if (likely (!orig_offset))
      return TRACE_RETURN (true);

    if (featureParams == 0 && closure &&
	closure->tag == HB_TAG ('s','i','z','e') &&
	closure->list_base && closure->list_base < this)
    {
      unsigned int new_offset_int = (unsigned int) orig_offset -
				    ((char *) this - (char *) closure->list_base);

      Offset new_offset;
      
      new_offset.set (new_offset_int);
      if (new_offset == new_offset_int &&
	  featureParams.try_set (c, new_offset) &&
	  !featureParams.sanitize (c, this, closure ? closure->tag : HB_TAG_NONE))
	return TRACE_RETURN (false);
    }

    return TRACE_RETURN (true);
  }

  OffsetTo<FeatureParams>
		 featureParams;	



  IndexArray	 lookupIndex;	
  public:
  DEFINE_SIZE_ARRAY (4, lookupIndex);
};

typedef RecordListOf<Feature> FeatureList;


struct LookupFlag : USHORT
{
  enum Flags {
    RightToLeft		= 0x0001u,
    IgnoreBaseGlyphs	= 0x0002u,
    IgnoreLigatures	= 0x0004u,
    IgnoreMarks		= 0x0008u,
    IgnoreFlags		= 0x000Eu,
    UseMarkFilteringSet	= 0x0010u,
    Reserved		= 0x00E0u,
    MarkAttachmentType	= 0xFF00u
  };
  public:
  DEFINE_SIZE_STATIC (2);
};

struct Lookup
{
  inline unsigned int get_subtable_count (void) const { return subTable.len; }

  inline unsigned int get_type (void) const { return lookupType; }

  


  inline uint32_t get_props (void) const
  {
    unsigned int flag = lookupFlag;
    if (unlikely (flag & LookupFlag::UseMarkFilteringSet))
    {
      const USHORT &markFilteringSet = StructAfter<USHORT> (subTable);
      flag += (markFilteringSet << 16);
    }
    return flag;
  }

  inline bool serialize (hb_serialize_context_t *c,
			 unsigned int lookup_type,
			 uint32_t lookup_props,
			 unsigned int num_subtables)
  {
    TRACE_SERIALIZE (this);
    if (unlikely (!c->extend_min (*this))) return TRACE_RETURN (false);
    lookupType.set (lookup_type);
    lookupFlag.set (lookup_props & 0xFFFF);
    if (unlikely (!subTable.serialize (c, num_subtables))) return TRACE_RETURN (false);
    if (lookupFlag & LookupFlag::UseMarkFilteringSet)
    {
      USHORT &markFilteringSet = StructAfter<USHORT> (subTable);
      markFilteringSet.set (lookup_props >> 16);
    }
    return TRACE_RETURN (true);
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE (this);
    
    if (!(c->check_struct (this) && subTable.sanitize (c))) return TRACE_RETURN (false);
    if (lookupFlag & LookupFlag::UseMarkFilteringSet)
    {
      USHORT &markFilteringSet = StructAfter<USHORT> (subTable);
      if (!markFilteringSet.sanitize (c)) return TRACE_RETURN (false);
    }
    return TRACE_RETURN (true);
  }

  USHORT	lookupType;		
  USHORT	lookupFlag;		
  ArrayOf<Offset>
		subTable;		
  USHORT	markFilteringSetX[VAR];	


  public:
  DEFINE_SIZE_ARRAY2 (6, subTable, markFilteringSetX);
};

typedef OffsetListOf<Lookup> LookupList;






struct CoverageFormat1
{
  friend struct Coverage;

  private:
  inline unsigned int get_coverage (hb_codepoint_t glyph_id) const
  {
    int i = glyphArray.search (glyph_id);
    ASSERT_STATIC (((unsigned int) -1) == NOT_COVERED);
    return i;
  }

  inline bool serialize (hb_serialize_context_t *c,
			 Supplier<GlyphID> &glyphs,
			 unsigned int num_glyphs)
  {
    TRACE_SERIALIZE (this);
    if (unlikely (!c->extend_min (*this))) return TRACE_RETURN (false);
    glyphArray.len.set (num_glyphs);
    if (unlikely (!c->extend (glyphArray))) return TRACE_RETURN (false);
    for (unsigned int i = 0; i < num_glyphs; i++)
      glyphArray[i] = glyphs[i];
    glyphs.advance (num_glyphs);
    return TRACE_RETURN (true);
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE (this);
    return TRACE_RETURN (glyphArray.sanitize (c));
  }

  inline bool intersects_coverage (const hb_set_t *glyphs, unsigned int index) const {
    return glyphs->has (glyphArray[index]);
  }

  template <typename set_t>
  inline void add_coverage (set_t *glyphs) const {
    unsigned int count = glyphArray.len;
    for (unsigned int i = 0; i < count; i++)
      glyphs->add (glyphArray[i]);
  }

  public:
  
  struct Iter {
    inline void init (const struct CoverageFormat1 &c_) { c = &c_; i = 0; };
    inline bool more (void) { return i < c->glyphArray.len; }
    inline void next (void) { i++; }
    inline uint16_t get_glyph (void) { return c->glyphArray[i]; }
    inline uint16_t get_coverage (void) { return i; }

    private:
    const struct CoverageFormat1 *c;
    unsigned int i;
  };
  private:

  protected:
  USHORT	coverageFormat;	
  SortedArrayOf<GlyphID>
		glyphArray;	
  public:
  DEFINE_SIZE_ARRAY (4, glyphArray);
};

struct CoverageFormat2
{
  friend struct Coverage;

  private:
  inline unsigned int get_coverage (hb_codepoint_t glyph_id) const
  {
    int i = rangeRecord.search (glyph_id);
    if (i != -1) {
      const RangeRecord &range = rangeRecord[i];
      return (unsigned int) range.value + (glyph_id - range.start);
    }
    return NOT_COVERED;
  }

  inline bool serialize (hb_serialize_context_t *c,
			 Supplier<GlyphID> &glyphs,
			 unsigned int num_glyphs)
  {
    TRACE_SERIALIZE (this);
    if (unlikely (!c->extend_min (*this))) return TRACE_RETURN (false);

    if (unlikely (!num_glyphs)) return TRACE_RETURN (true);

    unsigned int num_ranges = 1;
    for (unsigned int i = 1; i < num_glyphs; i++)
      if (glyphs[i - 1] + 1 != glyphs[i])
        num_ranges++;
    rangeRecord.len.set (num_ranges);
    if (unlikely (!c->extend (rangeRecord))) return TRACE_RETURN (false);

    unsigned int range = 0;
    rangeRecord[range].start = glyphs[0];
    rangeRecord[range].value.set (0);
    for (unsigned int i = 1; i < num_glyphs; i++)
      if (glyphs[i - 1] + 1 != glyphs[i]) {
	range++;
	rangeRecord[range].start = glyphs[i];
	rangeRecord[range].value.set (i);
        rangeRecord[range].end = glyphs[i];
      } else {
        rangeRecord[range].end = glyphs[i];
      }
    glyphs.advance (num_glyphs);
    return TRACE_RETURN (true);
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE (this);
    return TRACE_RETURN (rangeRecord.sanitize (c));
  }

  inline bool intersects_coverage (const hb_set_t *glyphs, unsigned int index) const {
    unsigned int i;
    unsigned int count = rangeRecord.len;
    for (i = 0; i < count; i++) {
      const RangeRecord &range = rangeRecord[i];
      if (range.value <= index &&
	  index < (unsigned int) range.value + (range.end - range.start) &&
	  range.intersects (glyphs))
        return true;
      else if (index < range.value)
        return false;
    }
    return false;
  }

  template <typename set_t>
  inline void add_coverage (set_t *glyphs) const {
    unsigned int count = rangeRecord.len;
    for (unsigned int i = 0; i < count; i++)
      rangeRecord[i].add_coverage (glyphs);
  }

  public:
  
  struct Iter {
    inline void init (const CoverageFormat2 &c_) {
      c = &c_;
      coverage = 0;
      i = 0;
      j = c->rangeRecord.len ? c_.rangeRecord[0].start : 0;
    }
    inline bool more (void) { return i < c->rangeRecord.len; }
    inline void next (void) {
      coverage++;
      if (j == c->rangeRecord[i].end) {
        i++;
	if (more ())
	  j = c->rangeRecord[i].start;
	return;
      }
      j++;
    }
    inline uint16_t get_glyph (void) { return j; }
    inline uint16_t get_coverage (void) { return coverage; }

    private:
    const struct CoverageFormat2 *c;
    unsigned int i, j, coverage;
  };
  private:

  protected:
  USHORT	coverageFormat;	
  SortedArrayOf<RangeRecord>
		rangeRecord;	


  public:
  DEFINE_SIZE_ARRAY (4, rangeRecord);
};

struct Coverage
{
  inline unsigned int get_coverage (hb_codepoint_t glyph_id) const
  {
    switch (u.format) {
    case 1: return u.format1.get_coverage(glyph_id);
    case 2: return u.format2.get_coverage(glyph_id);
    default:return NOT_COVERED;
    }
  }

  inline bool serialize (hb_serialize_context_t *c,
			 Supplier<GlyphID> &glyphs,
			 unsigned int num_glyphs)
  {
    TRACE_SERIALIZE (this);
    if (unlikely (!c->extend_min (*this))) return TRACE_RETURN (false);
    unsigned int num_ranges = 1;
    for (unsigned int i = 1; i < num_glyphs; i++)
      if (glyphs[i - 1] + 1 != glyphs[i])
        num_ranges++;
    u.format.set (num_glyphs * 2 < num_ranges * 3 ? 1 : 2);
    switch (u.format) {
    case 1: return TRACE_RETURN (u.format1.serialize (c, glyphs, num_glyphs));
    case 2: return TRACE_RETURN (u.format2.serialize (c, glyphs, num_glyphs));
    default:return TRACE_RETURN (false);
    }
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE (this);
    if (!u.format.sanitize (c)) return TRACE_RETURN (false);
    switch (u.format) {
    case 1: return TRACE_RETURN (u.format1.sanitize (c));
    case 2: return TRACE_RETURN (u.format2.sanitize (c));
    default:return TRACE_RETURN (true);
    }
  }

  inline bool intersects (const hb_set_t *glyphs) const {
    
    Coverage::Iter iter;
    for (iter.init (*this); iter.more (); iter.next ()) {
      if (glyphs->has (iter.get_glyph ()))
        return true;
    }
    return false;
  }

  inline bool intersects_coverage (const hb_set_t *glyphs, unsigned int index) const {
    switch (u.format) {
    case 1: return u.format1.intersects_coverage (glyphs, index);
    case 2: return u.format2.intersects_coverage (glyphs, index);
    default:return false;
    }
  }

  template <typename set_t>
  inline void add_coverage (set_t *glyphs) const {
    switch (u.format) {
    case 1: u.format1.add_coverage (glyphs); break;
    case 2: u.format2.add_coverage (glyphs); break;
    default:                                 break;
    }
  }

  struct Iter {
    Iter (void) : format (0) {};
    inline void init (const Coverage &c_) {
      format = c_.u.format;
      switch (format) {
      case 1: return u.format1.init (c_.u.format1);
      case 2: return u.format2.init (c_.u.format2);
      default:return;
      }
    }
    inline bool more (void) {
      switch (format) {
      case 1: return u.format1.more ();
      case 2: return u.format2.more ();
      default:return true;
      }
    }
    inline void next (void) {
      switch (format) {
      case 1: u.format1.next (); break;
      case 2: u.format2.next (); break;
      default:                   break;
      }
    }
    inline uint16_t get_glyph (void) {
      switch (format) {
      case 1: return u.format1.get_glyph ();
      case 2: return u.format2.get_glyph ();
      default:return true;
      }
    }
    inline uint16_t get_coverage (void) {
      switch (format) {
      case 1: return u.format1.get_coverage ();
      case 2: return u.format2.get_coverage ();
      default:return true;
      }
    }

    private:
    unsigned int format;
    union {
    CoverageFormat1::Iter	format1;
    CoverageFormat2::Iter	format2;
    } u;
  };

  protected:
  union {
  USHORT		format;		
  CoverageFormat1	format1;
  CoverageFormat2	format2;
  } u;
  public:
  DEFINE_SIZE_UNION (2, format);
};






struct ClassDefFormat1
{
  friend struct ClassDef;

  private:
  inline unsigned int get_class (hb_codepoint_t glyph_id) const
  {
    if (unlikely ((unsigned int) (glyph_id - startGlyph) < classValue.len))
      return classValue[glyph_id - startGlyph];
    return 0;
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE (this);
    return TRACE_RETURN (c->check_struct (this) && classValue.sanitize (c));
  }

  template <typename set_t>
  inline void add_class (set_t *glyphs, unsigned int klass) const {
    unsigned int count = classValue.len;
    for (unsigned int i = 0; i < count; i++)
      if (classValue[i] == klass)
        glyphs->add (startGlyph + i);
  }

  inline bool intersects_class (const hb_set_t *glyphs, unsigned int klass) const {
    unsigned int count = classValue.len;
    for (unsigned int i = 0; i < count; i++)
      if (classValue[i] == klass && glyphs->has (startGlyph + i))
        return true;
    return false;
  }

  protected:
  USHORT	classFormat;		
  GlyphID	startGlyph;		
  ArrayOf<USHORT>
		classValue;		
  public:
  DEFINE_SIZE_ARRAY (6, classValue);
};

struct ClassDefFormat2
{
  friend struct ClassDef;

  private:
  inline unsigned int get_class (hb_codepoint_t glyph_id) const
  {
    int i = rangeRecord.search (glyph_id);
    if (i != -1)
      return rangeRecord[i].value;
    return 0;
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE (this);
    return TRACE_RETURN (rangeRecord.sanitize (c));
  }

  template <typename set_t>
  inline void add_class (set_t *glyphs, unsigned int klass) const {
    unsigned int count = rangeRecord.len;
    for (unsigned int i = 0; i < count; i++)
      if (rangeRecord[i].value == klass)
        rangeRecord[i].add_coverage (glyphs);
  }

  inline bool intersects_class (const hb_set_t *glyphs, unsigned int klass) const {
    unsigned int count = rangeRecord.len;
    for (unsigned int i = 0; i < count; i++)
      if (rangeRecord[i].value == klass && rangeRecord[i].intersects (glyphs))
        return true;
    return false;
  }

  protected:
  USHORT	classFormat;	
  SortedArrayOf<RangeRecord>
		rangeRecord;	

  public:
  DEFINE_SIZE_ARRAY (4, rangeRecord);
};

struct ClassDef
{
  inline unsigned int get_class (hb_codepoint_t glyph_id) const
  {
    switch (u.format) {
    case 1: return u.format1.get_class(glyph_id);
    case 2: return u.format2.get_class(glyph_id);
    default:return 0;
    }
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE (this);
    if (!u.format.sanitize (c)) return TRACE_RETURN (false);
    switch (u.format) {
    case 1: return TRACE_RETURN (u.format1.sanitize (c));
    case 2: return TRACE_RETURN (u.format2.sanitize (c));
    default:return TRACE_RETURN (true);
    }
  }

  inline void add_class (hb_set_t *glyphs, unsigned int klass) const {
    switch (u.format) {
    case 1: u.format1.add_class (glyphs, klass); return;
    case 2: u.format2.add_class (glyphs, klass); return;
    default:return;
    }
  }

  inline bool intersects_class (const hb_set_t *glyphs, unsigned int klass) const {
    switch (u.format) {
    case 1: return u.format1.intersects_class (glyphs, klass);
    case 2: return u.format2.intersects_class (glyphs, klass);
    default:return false;
    }
  }

  protected:
  union {
  USHORT		format;		
  ClassDefFormat1	format1;
  ClassDefFormat2	format2;
  } u;
  public:
  DEFINE_SIZE_UNION (2, format);
};






struct Device
{

  inline hb_position_t get_x_delta (hb_font_t *font) const
  { return get_delta (font->x_ppem, font->x_scale); }

  inline hb_position_t get_y_delta (hb_font_t *font) const
  { return get_delta (font->y_ppem, font->y_scale); }

  inline int get_delta (unsigned int ppem, int scale) const
  {
    if (!ppem) return 0;

    int pixels = get_delta_pixels (ppem);

    if (!pixels) return 0;

    return pixels * (int64_t) scale / ppem;
  }


  inline int get_delta_pixels (unsigned int ppem_size) const
  {
    unsigned int f = deltaFormat;
    if (unlikely (f < 1 || f > 3))
      return 0;

    if (ppem_size < startSize || ppem_size > endSize)
      return 0;

    unsigned int s = ppem_size - startSize;

    unsigned int byte = deltaValue[s >> (4 - f)];
    unsigned int bits = (byte >> (16 - (((s & ((1 << (4 - f)) - 1)) + 1) << f)));
    unsigned int mask = (0xFFFF >> (16 - (1 << f)));

    int delta = bits & mask;

    if ((unsigned int) delta >= ((mask + 1) >> 1))
      delta -= mask + 1;

    return delta;
  }

  inline unsigned int get_size (void) const
  {
    unsigned int f = deltaFormat;
    if (unlikely (f < 1 || f > 3 || startSize > endSize)) return 3 * USHORT::static_size;
    return USHORT::static_size * (4 + ((endSize - startSize) >> (4 - f)));
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE (this);
    return TRACE_RETURN (c->check_struct (this) && c->check_range (this, this->get_size ()));
  }

  protected:
  USHORT	startSize;		
  USHORT	endSize;		
  USHORT	deltaFormat;		




  USHORT	deltaValue[VAR];	
  public:
  DEFINE_SIZE_ARRAY (6, deltaValue);
};


} 


#endif 
