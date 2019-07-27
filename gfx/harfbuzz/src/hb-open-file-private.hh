



























#ifndef HB_OPEN_FILE_PRIVATE_HH
#define HB_OPEN_FILE_PRIVATE_HH

#include "hb-open-type-private.hh"


namespace OT {













struct OpenTypeFontFile;
struct OffsetTable;
struct TTCHeader;


typedef struct TableRecord
{
  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE (this);
    return TRACE_RETURN (c->check_struct (this));
  }

  Tag		tag;		
  CheckSum	checkSum;	
  ULONG		offset;		

  ULONG		length;		
  public:
  DEFINE_SIZE_STATIC (16);
} OpenTypeTable;

typedef struct OffsetTable
{
  friend struct OpenTypeFontFile;

  inline unsigned int get_table_count (void) const
  { return numTables; }
  inline const TableRecord& get_table (unsigned int i) const
  {
    if (unlikely (i >= numTables)) return Null(TableRecord);
    return tables[i];
  }
  inline bool find_table_index (hb_tag_t tag, unsigned int *table_index) const
  {
    Tag t;
    t.set (tag);
    unsigned int count = numTables;
    for (unsigned int i = 0; i < count; i++)
    {
      if (t == tables[i].tag)
      {
        if (table_index) *table_index = i;
        return true;
      }
    }
    if (table_index) *table_index = Index::NOT_FOUND_INDEX;
    return false;
  }
  inline const TableRecord& get_table_by_tag (hb_tag_t tag) const
  {
    unsigned int table_index;
    find_table_index (tag, &table_index);
    return get_table (table_index);
  }

  public:
  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE (this);
    return TRACE_RETURN (c->check_struct (this) && c->check_array (tables, TableRecord::static_size, numTables));
  }

  protected:
  Tag		sfnt_version;	
  USHORT	numTables;	
  USHORT	searchRangeZ;	
  USHORT	entrySelectorZ;	
  USHORT	rangeShiftZ;	
  TableRecord	tables[VAR];	
  public:
  DEFINE_SIZE_ARRAY (12, tables);
} OpenTypeFontFace;






struct TTCHeaderVersion1
{
  friend struct TTCHeader;

  inline unsigned int get_face_count (void) const { return table.len; }
  inline const OpenTypeFontFace& get_face (unsigned int i) const { return this+table[i]; }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE (this);
    return TRACE_RETURN (table.sanitize (c, this));
  }

  protected:
  Tag		ttcTag;		
  FixedVersion	version;	

  ArrayOf<OffsetTo<OffsetTable, ULONG>, ULONG>
		table;		

  public:
  DEFINE_SIZE_ARRAY (12, table);
};

struct TTCHeader
{
  friend struct OpenTypeFontFile;

  private:

  inline unsigned int get_face_count (void) const
  {
    switch (u.header.version.major) {
    case 2: 
    case 1: return u.version1.get_face_count ();
    default:return 0;
    }
  }
  inline const OpenTypeFontFace& get_face (unsigned int i) const
  {
    switch (u.header.version.major) {
    case 2: 
    case 1: return u.version1.get_face (i);
    default:return Null(OpenTypeFontFace);
    }
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE (this);
    if (unlikely (!u.header.version.sanitize (c))) return TRACE_RETURN (false);
    switch (u.header.version.major) {
    case 2: 
    case 1: return TRACE_RETURN (u.version1.sanitize (c));
    default:return TRACE_RETURN (true);
    }
  }

  protected:
  union {
  struct {
  Tag		ttcTag;		
  FixedVersion	version;	

  }			header;
  TTCHeaderVersion1	version1;
  } u;
};






struct OpenTypeFontFile
{
  static const hb_tag_t CFFTag		= HB_TAG ('O','T','T','O'); 
  static const hb_tag_t TrueTypeTag	= HB_TAG ( 0 , 1 , 0 , 0 ); 
  static const hb_tag_t TTCTag		= HB_TAG ('t','t','c','f'); 
  static const hb_tag_t TrueTag		= HB_TAG ('t','r','u','e'); 
  static const hb_tag_t Typ1Tag		= HB_TAG ('t','y','p','1'); 

  inline hb_tag_t get_tag (void) const { return u.tag; }

  inline unsigned int get_face_count (void) const
  {
    switch (u.tag) {
    case CFFTag:	
    case TrueTag:
    case Typ1Tag:
    case TrueTypeTag:	return 1;
    case TTCTag:	return u.ttcHeader.get_face_count ();
    default:		return 0;
    }
  }
  inline const OpenTypeFontFace& get_face (unsigned int i) const
  {
    switch (u.tag) {
    


    case CFFTag:	
    case TrueTag:
    case Typ1Tag:
    case TrueTypeTag:	return u.fontFace;
    case TTCTag:	return u.ttcHeader.get_face (i);
    default:		return Null(OpenTypeFontFace);
    }
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE (this);
    if (unlikely (!u.tag.sanitize (c))) return TRACE_RETURN (false);
    switch (u.tag) {
    case CFFTag:	
    case TrueTag:
    case Typ1Tag:
    case TrueTypeTag:	return TRACE_RETURN (u.fontFace.sanitize (c));
    case TTCTag:	return TRACE_RETURN (u.ttcHeader.sanitize (c));
    default:		return TRACE_RETURN (true);
    }
  }

  protected:
  union {
  Tag			tag;		
  OpenTypeFontFace	fontFace;
  TTCHeader		ttcHeader;
  } u;
  public:
  DEFINE_SIZE_UNION (4, tag);
};


} 


#endif 
