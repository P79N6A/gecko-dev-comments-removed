



























#ifndef HB_OPEN_TYPE_PRIVATE_HH
#define HB_OPEN_TYPE_PRIVATE_HH

#include "hb-private.hh"

#include "hb-blob.h"


namespace OT {







template<typename Type, typename TObject>
inline const Type& CastR(const TObject &X)
{ return reinterpret_cast<const Type&> (X); }
template<typename Type, typename TObject>
inline Type& CastR(TObject &X)
{ return reinterpret_cast<Type&> (X); }


template<typename Type, typename TObject>
inline const Type* CastP(const TObject *X)
{ return reinterpret_cast<const Type*> (X); }
template<typename Type, typename TObject>
inline Type* CastP(TObject *X)
{ return reinterpret_cast<Type*> (X); }



template<typename Type>
inline const Type& StructAtOffset(const void *P, unsigned int offset)
{ return * reinterpret_cast<const Type*> ((const char *) P + offset); }
template<typename Type>
inline Type& StructAtOffset(void *P, unsigned int offset)
{ return * reinterpret_cast<Type*> ((char *) P + offset); }



template<typename Type, typename TObject>
inline const Type& StructAfter(const TObject &X)
{ return StructAtOffset<Type>(&X, X.get_size()); }
template<typename Type, typename TObject>
inline Type& StructAfter(TObject &X)
{ return StructAtOffset<Type>(&X, X.get_size()); }








#define _DEFINE_INSTANCE_ASSERTION1(_line, _assertion) \
  inline void _instance_assertion_on_line_##_line (void) const \
  { \
    ASSERT_STATIC (_assertion); \
    ASSERT_INSTANCE_POD (*this); /* Make sure it's POD. */ \
  }
# define _DEFINE_INSTANCE_ASSERTION0(_line, _assertion) _DEFINE_INSTANCE_ASSERTION1 (_line, _assertion)
# define DEFINE_INSTANCE_ASSERTION(_assertion) _DEFINE_INSTANCE_ASSERTION0 (__LINE__, _assertion)


#define _DEFINE_COMPILES_ASSERTION1(_line, _code) \
  inline void _compiles_assertion_on_line_##_line (void) const \
  { _code; }
# define _DEFINE_COMPILES_ASSERTION0(_line, _code) _DEFINE_COMPILES_ASSERTION1 (_line, _code)
# define DEFINE_COMPILES_ASSERTION(_code) _DEFINE_COMPILES_ASSERTION0 (__LINE__, _code)


#define DEFINE_SIZE_STATIC(size) \
  DEFINE_INSTANCE_ASSERTION (sizeof (*this) == (size)); \
  static const unsigned int static_size = (size); \
  static const unsigned int min_size = (size)


#define VAR 1

#define DEFINE_SIZE_UNION(size, _member) \
  DEFINE_INSTANCE_ASSERTION (this->u._member.static_size == (size)); \
  static const unsigned int min_size = (size)

#define DEFINE_SIZE_MIN(size) \
  DEFINE_INSTANCE_ASSERTION (sizeof (*this) >= (size)); \
  static const unsigned int min_size = (size)

#define DEFINE_SIZE_ARRAY(size, array) \
  DEFINE_INSTANCE_ASSERTION (sizeof (*this) == (size) + sizeof (array[0])); \
  DEFINE_COMPILES_ASSERTION ((void) array[0].static_size) \
  static const unsigned int min_size = (size)

#define DEFINE_SIZE_ARRAY2(size, array1, array2) \
  DEFINE_INSTANCE_ASSERTION (sizeof (*this) == (size) + sizeof (this->array1[0]) + sizeof (this->array2[0])); \
  DEFINE_COMPILES_ASSERTION ((void) array1[0].static_size; (void) array2[0].static_size) \
  static const unsigned int min_size = (size)









static const void *_NullPool[64 / sizeof (void *)];


template <typename Type>
static inline const Type& Null (void) {
  ASSERT_STATIC (Type::min_size <= sizeof (_NullPool));
  return *CastP<Type> (_NullPool);
}


#define DEFINE_NULL_DATA(Type, data) \
static const char _Null##Type[Type::min_size + 1] = data; /* +1 is for nul-termination in data */ \
template <> \
inline const Type& Null<Type> (void) { \
  return *CastP<Type> (_Null##Type); \
} /* The following line really exists such that we end in a place needing semicolon */ \
ASSERT_STATIC (Type::min_size + 1 <= sizeof (_Null##Type))


#define Null(Type) Null<Type>()







#ifndef HB_DEBUG_SANITIZE
#define HB_DEBUG_SANITIZE (HB_DEBUG+0)
#endif


#define TRACE_SANITIZE() \
	hb_auto_trace_t<HB_DEBUG_SANITIZE> trace (&c->debug_depth, "SANITIZE", this, HB_FUNC, "");


struct hb_sanitize_context_t
{
  inline void init (hb_blob_t *b)
  {
    this->blob = hb_blob_reference (b);
    this->writable = false;
  }

  inline void start_processing (void)
  {
    this->start = hb_blob_get_data (this->blob, NULL);
    this->end = this->start + hb_blob_get_length (this->blob);
    this->edit_count = 0;
    this->debug_depth = 0;

    DEBUG_MSG_LEVEL (SANITIZE, this->blob, 0, +1,
		     "start [%p..%p] (%lu bytes)",
		     this->start, this->end,
		     (unsigned long) (this->end - this->start));
  }

  inline void end_processing (void)
  {
    DEBUG_MSG_LEVEL (SANITIZE, this->blob, 0, -1,
		     "end [%p..%p] %u edit requests",
		     this->start, this->end, this->edit_count);

    hb_blob_destroy (this->blob);
    this->blob = NULL;
    this->start = this->end = NULL;
  }

  inline bool check_range (const void *base, unsigned int len) const
  {
    const char *p = (const char *) base;

    hb_auto_trace_t<HB_DEBUG_SANITIZE> trace (&this->debug_depth, "SANITIZE", this->blob, NULL,
					      "check_range [%p..%p] (%d bytes) in [%p..%p]",
					      p, p + len, len,
					      this->start, this->end);

    return TRACE_RETURN (likely (this->start <= p && p <= this->end && (unsigned int) (this->end - p) >= len));
  }

  inline bool check_array (const void *base, unsigned int record_size, unsigned int len) const
  {
    const char *p = (const char *) base;
    bool overflows = _hb_unsigned_int_mul_overflows (len, record_size);

    hb_auto_trace_t<HB_DEBUG_SANITIZE> trace (&this->debug_depth, "SANITIZE", this->blob, NULL,
					      "check_array [%p..%p] (%d*%d=%ld bytes) in [%p..%p]",
					      p, p + (record_size * len), record_size, len, (unsigned long) record_size * len,
					      this->start, this->end);

    return TRACE_RETURN (likely (!overflows && this->check_range (base, record_size * len)));
  }

  template <typename Type>
  inline bool check_struct (const Type *obj) const
  {
    return likely (this->check_range (obj, obj->min_size));
  }

  inline bool may_edit (const void *base HB_UNUSED, unsigned int len HB_UNUSED)
  {
    const char *p = (const char *) base;
    this->edit_count++;

    hb_auto_trace_t<HB_DEBUG_SANITIZE> trace (&this->debug_depth, "SANITIZE", this->blob, NULL,
					      "may_edit(%u) [%p..%p] (%d bytes) in [%p..%p] -> %s",
					      this->edit_count,
					      p, p + len, len,
					      this->start, this->end);

    return TRACE_RETURN (this->writable);
  }

  mutable unsigned int debug_depth;
  const char *start, *end;
  bool writable;
  unsigned int edit_count;
  hb_blob_t *blob;
};




template <typename Type>
struct Sanitizer
{
  static hb_blob_t *sanitize (hb_blob_t *blob) {
    hb_sanitize_context_t c[1] = {{0}};
    bool sane;

    

    c->init (blob);

  retry:
    DEBUG_MSG_FUNC (SANITIZE, blob, "start");

    c->start_processing ();

    if (unlikely (!c->start)) {
      c->end_processing ();
      return blob;
    }

    Type *t = CastP<Type> (const_cast<char *> (c->start));

    sane = t->sanitize (c);
    if (sane) {
      if (c->edit_count) {
	DEBUG_MSG_FUNC (SANITIZE, blob, "passed first round with %d edits; going for second round", c->edit_count);

        
        c->edit_count = 0;
	sane = t->sanitize (c);
	if (c->edit_count) {
	  DEBUG_MSG_FUNC (SANITIZE, blob, "requested %d edits in second round; FAILLING", c->edit_count);
	  sane = false;
	}
      }
    } else {
      unsigned int edit_count = c->edit_count;
      if (edit_count && !c->writable) {
        c->start = hb_blob_get_data_writable (blob, NULL);
	c->end = c->start + hb_blob_get_length (blob);

	if (c->start) {
	  c->writable = true;
	  
	  DEBUG_MSG_FUNC (SANITIZE, blob, "retry");
	  goto retry;
	}
      }
    }

    c->end_processing ();

    DEBUG_MSG_FUNC (SANITIZE, blob, sane ? "PASSED" : "FAILED");
    if (sane)
      return blob;
    else {
      hb_blob_destroy (blob);
      return hb_blob_get_empty ();
    }
  }

  static const Type* lock_instance (hb_blob_t *blob) {
    hb_blob_make_immutable (blob);
    const char *base = hb_blob_get_data (blob, NULL);
    return unlikely (!base) ? &Null(Type) : CastP<Type> (base);
  }
};







#ifndef HB_DEBUG_SERIALIZE
#define HB_DEBUG_SERIALIZE (HB_DEBUG+0)
#endif


#define TRACE_SERIALIZE() \
	hb_auto_trace_t<HB_DEBUG_SERIALIZE> trace (&c->debug_depth, "SERIALIZE", c, HB_FUNC, "");


struct hb_serialize_context_t
{
  inline hb_serialize_context_t (void *start, unsigned int size)
  {
    this->start = (char *) start;
    this->end = this->start + size;

    this->ran_out_of_room = false;
    this->head = this->start;
    this->debug_depth = 0;
  }

  template <typename Type>
  inline Type *start_serialize (void)
  {
    DEBUG_MSG_LEVEL (SERIALIZE, this->start, 0, +1,
		     "start [%p..%p] (%lu bytes)",
		     this->start, this->end,
		     (unsigned long) (this->end - this->start));

    return start_embed<Type> ();
  }

  inline void end_serialize (void)
  {
    DEBUG_MSG_LEVEL (SERIALIZE, this->start, 0, -1,
		     "end [%p..%p] serialized %d bytes; %s",
		     this->start, this->end,
		     (int) (this->head - this->start),
		     this->ran_out_of_room ? "RAN OUT OF ROOM" : "did not ran out of room");

  }

  template <typename Type>
  inline Type *copy (void)
  {
    assert (!this->ran_out_of_room);
    unsigned int len = this->head - this->start;
    void *p = malloc (len);
    if (p)
      memcpy (p, this->start, len);
    return reinterpret_cast<Type *> (p);
  }

  template <typename Type>
  inline Type *allocate_size (unsigned int size)
  {
    if (unlikely (this->ran_out_of_room || this->end - this->head < size)) {
      this->ran_out_of_room = true;
      return NULL;
    }
    memset (this->head, 0, size);
    char *ret = this->head;
    this->head += size;
    return reinterpret_cast<Type *> (ret);
  }

  template <typename Type>
  inline Type *allocate_min (void)
  {
    return this->allocate_size<Type> (Type::min_size);
  }

  template <typename Type>
  inline Type *start_embed (void)
  {
    Type *ret = reinterpret_cast<Type *> (this->head);
    return ret;
  }

  template <typename Type>
  inline Type *embed (const Type &obj)
  {
    unsigned int size = obj.get_size ();
    Type *ret = this->allocate_size<Type> (size);
    if (unlikely (!ret)) return NULL;
    memcpy (ret, obj, size);
    return ret;
  }

  template <typename Type>
  inline Type *extend_min (Type &obj)
  {
    unsigned int size = obj.min_size;
    assert (this->start <= (char *) &obj && (char *) &obj <= this->head && (char *) &obj + size >= this->head);
    if (unlikely (!this->allocate_size<Type> (((char *) &obj) + size - this->head))) return NULL;
    return reinterpret_cast<Type *> (&obj);
  }

  template <typename Type>
  inline Type *extend (Type &obj)
  {
    unsigned int size = obj.get_size ();
    assert (this->start < (char *) &obj && (char *) &obj <= this->head && (char *) &obj + size >= this->head);
    if (unlikely (!this->allocate_size<Type> (((char *) &obj) + size - this->head))) return NULL;
    return reinterpret_cast<Type *> (&obj);
  }

  inline void truncate (void *head)
  {
    assert (this->start < head && head <= this->head);
    this->head = (char *) head;
  }

  unsigned int debug_depth;
  char *start, *end, *head;
  bool ran_out_of_room;
};

template <typename Type>
struct Supplier
{
  inline Supplier (const Type *array, unsigned int len_)
  {
    head = array;
    len = len_;
  }
  inline const Type operator [] (unsigned int i) const
  {
    if (unlikely (i >= len)) return Type ();
    return head[i];
  }

  inline void advance (unsigned int count)
  {
    if (unlikely (count > len))
      count = len;
    len -= count;
    head += count;
  }

  private:
  inline Supplier (const Supplier<Type> &); 
  inline Supplier<Type>& operator= (const Supplier<Type> &); 

  unsigned int len;
  const Type *head;
};


















template <typename Type, int Bytes> struct BEInt;

template <typename Type>
struct BEInt<Type, 2>
{
  public:
  inline void set (Type i) { hb_be_uint16_put (v,i); }
  inline operator Type (void) const { return hb_be_uint16_get (v); }
  inline bool operator == (const BEInt<Type, 2>& o) const { return hb_be_uint16_eq (v, o.v); }
  inline bool operator != (const BEInt<Type, 2>& o) const { return !(*this == o); }
  private: uint8_t v[2];
};
template <typename Type>
struct BEInt<Type, 4>
{
  public:
  inline void set (Type i) { hb_be_uint32_put (v,i); }
  inline operator Type (void) const { return hb_be_uint32_get (v); }
  inline bool operator == (const BEInt<Type, 4>& o) const { return hb_be_uint32_eq (v, o.v); }
  inline bool operator != (const BEInt<Type, 4>& o) const { return !(*this == o); }
  private: uint8_t v[4];
};


template <typename Type>
struct IntType
{
  inline void set (Type i) { v.set (i); }
  inline operator Type(void) const { return v; }
  inline bool operator == (const IntType<Type> &o) const { return v == o.v; }
  inline bool operator != (const IntType<Type> &o) const { return v != o.v; }
  static inline int cmp (const IntType<Type> *a, const IntType<Type> *b) { return b->cmp (*a); }
  inline int cmp (IntType<Type> va) const { Type a = va; Type b = v; return a < b ? -1 : a == b ? 0 : +1; }
  inline int cmp (Type a) const { Type b = v; return a < b ? -1 : a == b ? 0 : +1; }
  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return TRACE_RETURN (likely (c->check_struct (this)));
  }
  protected:
  BEInt<Type, sizeof (Type)> v;
  public:
  DEFINE_SIZE_STATIC (sizeof (Type));
};

typedef IntType<uint16_t> USHORT;	
typedef IntType<int16_t>  SHORT;	
typedef IntType<uint32_t> ULONG;	
typedef IntType<int32_t>  LONG;		


typedef SHORT FWORD;


typedef USHORT UFWORD;



struct LONGDATETIME
{
  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return TRACE_RETURN (likely (c->check_struct (this)));
  }
  private:
  LONG major;
  ULONG minor;
  public:
  DEFINE_SIZE_STATIC (8);
};



struct Tag : ULONG
{
  
  inline operator const char* (void) const { return reinterpret_cast<const char *> (&this->v); }
  inline operator char* (void) { return reinterpret_cast<char *> (&this->v); }
  public:
  DEFINE_SIZE_STATIC (4);
};
DEFINE_NULL_DATA (Tag, "    ");


typedef USHORT GlyphID;


struct Index : USHORT {
  static const unsigned int NOT_FOUND_INDEX = 0xFFFF;
};
DEFINE_NULL_DATA (Index, "\xff\xff");


typedef USHORT Offset;


typedef ULONG LongOffset;



struct CheckSum : ULONG
{
  static uint32_t CalcTableChecksum (ULONG *Table, uint32_t Length)
  {
    uint32_t Sum = 0L;
    ULONG *EndPtr = Table+((Length+3) & ~3) / ULONG::static_size;

    while (Table < EndPtr)
      Sum += *Table++;
    return Sum;
  }
  public:
  DEFINE_SIZE_STATIC (4);
};






struct FixedVersion
{
  inline uint32_t to_int (void) const { return (major << 16) + minor; }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return TRACE_RETURN (c->check_struct (this));
  }

  USHORT major;
  USHORT minor;
  public:
  DEFINE_SIZE_STATIC (4);
};








template <typename OffsetType, typename Type>
struct GenericOffsetTo : OffsetType
{
  inline const Type& operator () (const void *base) const
  {
    unsigned int offset = *this;
    if (unlikely (!offset)) return Null(Type);
    return StructAtOffset<Type> (base, offset);
  }
  inline Type& operator () (void *base)
  {
    unsigned int offset = *this;
    return StructAtOffset<Type> (base, offset);
  }

  inline Type& serialize (hb_serialize_context_t *c, void *base)
  {
    Type *t = c->start_embed<Type> ();
    this->set ((char *) t - (char *) base); 
    return *t;
  }

  inline bool sanitize (hb_sanitize_context_t *c, void *base) {
    TRACE_SANITIZE ();
    if (unlikely (!c->check_struct (this))) return TRACE_RETURN (false);
    unsigned int offset = *this;
    if (unlikely (!offset)) return TRACE_RETURN (true);
    Type &obj = StructAtOffset<Type> (base, offset);
    return TRACE_RETURN (likely (obj.sanitize (c)) || neuter (c));
  }
  template <typename T>
  inline bool sanitize (hb_sanitize_context_t *c, void *base, T user_data) {
    TRACE_SANITIZE ();
    if (unlikely (!c->check_struct (this))) return TRACE_RETURN (false);
    unsigned int offset = *this;
    if (unlikely (!offset)) return TRACE_RETURN (true);
    Type &obj = StructAtOffset<Type> (base, offset);
    return TRACE_RETURN (likely (obj.sanitize (c, user_data)) || neuter (c));
  }

  private:
  
  inline bool neuter (hb_sanitize_context_t *c) {
    if (c->may_edit (this, this->static_size)) {
      this->set (0); 
      return true;
    }
    return false;
  }
};
template <typename Base, typename OffsetType, typename Type>
inline const Type& operator + (const Base &base, const GenericOffsetTo<OffsetType, Type> &offset) { return offset (base); }
template <typename Base, typename OffsetType, typename Type>
inline Type& operator + (Base &base, GenericOffsetTo<OffsetType, Type> &offset) { return offset (base); }

template <typename Type>
struct OffsetTo : GenericOffsetTo<Offset, Type> {};

template <typename Type>
struct LongOffsetTo : GenericOffsetTo<LongOffset, Type> {};






template <typename LenType, typename Type>
struct GenericArrayOf
{
  const Type *sub_array (unsigned int start_offset, unsigned int *pcount ) const
  {
    unsigned int count = len;
    if (unlikely (start_offset > count))
      count = 0;
    else
      count -= start_offset;
    count = MIN (count, *pcount);
    *pcount = count;
    return array + start_offset;
  }

  inline const Type& operator [] (unsigned int i) const
  {
    if (unlikely (i >= len)) return Null(Type);
    return array[i];
  }
  inline Type& operator [] (unsigned int i)
  {
    return array[i];
  }
  inline unsigned int get_size (void) const
  { return len.static_size + len * Type::static_size; }

  inline bool serialize (hb_serialize_context_t *c,
			 unsigned int items_len)
  {
    TRACE_SERIALIZE ();
    if (unlikely (!c->extend_min (*this))) return TRACE_RETURN (false);
    len.set (items_len); 
    if (unlikely (!c->extend (*this))) return TRACE_RETURN (false);
    return TRACE_RETURN (true);
  }

  inline bool serialize (hb_serialize_context_t *c,
			 Supplier<Type> &items,
			 unsigned int items_len)
  {
    TRACE_SERIALIZE ();
    if (unlikely (!serialize (c, items_len))) return TRACE_RETURN (false);
    for (unsigned int i = 0; i < items_len; i++)
      array[i] = items[i];
    items.advance (items_len);
    return TRACE_RETURN (true);
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    if (unlikely (!sanitize_shallow (c))) return TRACE_RETURN (false);

    






    (void) (false && array[0].sanitize (c));

    return TRACE_RETURN (true);
  }
  inline bool sanitize (hb_sanitize_context_t *c, void *base) {
    TRACE_SANITIZE ();
    if (unlikely (!sanitize_shallow (c))) return TRACE_RETURN (false);
    unsigned int count = len;
    for (unsigned int i = 0; i < count; i++)
      if (unlikely (!array[i].sanitize (c, base)))
        return TRACE_RETURN (false);
    return TRACE_RETURN (true);
  }
  template <typename T>
  inline bool sanitize (hb_sanitize_context_t *c, void *base, T user_data) {
    TRACE_SANITIZE ();
    if (unlikely (!sanitize_shallow (c))) return TRACE_RETURN (false);
    unsigned int count = len;
    for (unsigned int i = 0; i < count; i++)
      if (unlikely (!array[i].sanitize (c, base, user_data)))
        return TRACE_RETURN (false);
    return TRACE_RETURN (true);
  }

  private:
  inline bool sanitize_shallow (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return TRACE_RETURN (c->check_struct (this) && c->check_array (this, Type::static_size, len));
  }

  public:
  LenType len;
  Type array[VAR];
  public:
  DEFINE_SIZE_ARRAY (sizeof (LenType), array);
};


template <typename Type>
struct ArrayOf : GenericArrayOf<USHORT, Type> {};


template <typename Type>
struct LongArrayOf : GenericArrayOf<ULONG, Type> {};


template <typename Type>
struct OffsetArrayOf : ArrayOf<OffsetTo<Type> > {};


template <typename Type>
struct LongOffsetArrayOf : ArrayOf<LongOffsetTo<Type> > {};


template <typename Type>
struct LongOffsetLongArrayOf : LongArrayOf<LongOffsetTo<Type> > {};


template <typename Type>
struct OffsetListOf : OffsetArrayOf<Type>
{
  inline const Type& operator [] (unsigned int i) const
  {
    if (unlikely (i >= this->len)) return Null(Type);
    return this+this->array[i];
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return TRACE_RETURN (OffsetArrayOf<Type>::sanitize (c, this));
  }
  template <typename T>
  inline bool sanitize (hb_sanitize_context_t *c, T user_data) {
    TRACE_SANITIZE ();
    return TRACE_RETURN (OffsetArrayOf<Type>::sanitize (c, this, user_data));
  }
};




template <typename Type>
struct HeadlessArrayOf
{
  inline const Type& operator [] (unsigned int i) const
  {
    if (unlikely (i >= len || !i)) return Null(Type);
    return array[i-1];
  }
  inline unsigned int get_size (void) const
  { return len.static_size + (len ? len - 1 : 0) * Type::static_size; }

  inline bool serialize (hb_serialize_context_t *c,
			 Supplier<Type> &items,
			 unsigned int items_len)
  {
    TRACE_SERIALIZE ();
    if (unlikely (!c->extend_min (*this))) return TRACE_RETURN (false);
    len.set (items_len); 
    if (unlikely (!items_len)) return TRACE_RETURN (true);
    if (unlikely (!c->extend (*this))) return TRACE_RETURN (false);
    for (unsigned int i = 0; i < items_len - 1; i++)
      array[i] = items[i];
    items.advance (items_len - 1);
    return TRACE_RETURN (true);
  }

  inline bool sanitize_shallow (hb_sanitize_context_t *c) {
    return c->check_struct (this)
	&& c->check_array (this, Type::static_size, len);
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    if (unlikely (!sanitize_shallow (c))) return TRACE_RETURN (false);

    






    (void) (false && array[0].sanitize (c));

    return TRACE_RETURN (true);
  }

  USHORT len;
  Type array[VAR];
  public:
  DEFINE_SIZE_ARRAY (sizeof (USHORT), array);
};



template <typename Type>
struct SortedArrayOf : ArrayOf<Type> {

  template <typename SearchType>
  inline int search (const SearchType &x) const {
    unsigned int count = this->len;
    
    if (likely (count < 32)) {
      for (unsigned int i = 0; i < count; i++)
	if (this->array[i].cmp (x) == 0)
	  return i;
      return -1;
    } else {
      struct Cmp {
	static int cmp (const SearchType *a, const Type *b) { return b->cmp (*a); }
      };
      const Type *p = (const Type *) bsearch (&x, this->array, this->len, sizeof (this->array[0]), (hb_compare_func_t) Cmp::cmp);
      return p ? p - this->array : -1;
    }
  }
};


} 


#endif 
