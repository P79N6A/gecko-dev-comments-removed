






























#ifndef HB_OBJECT_PRIVATE_HH
#define HB_OBJECT_PRIVATE_HH

#include "hb-private.hh"

#include "hb-atomic-private.hh"
#include "hb-mutex-private.hh"




#ifndef HB_DEBUG_OBJECT
#define HB_DEBUG_OBJECT (HB_DEBUG+0)
#endif




#define HB_REFERENCE_COUNT_INVALID_VALUE ((hb_atomic_int_t) -1)
#define HB_REFERENCE_COUNT_INVALID {HB_REFERENCE_COUNT_INVALID_VALUE}
struct hb_reference_count_t
{
  hb_atomic_int_t ref_count;

  inline void init (int v) { ref_count = v; }
  inline int inc (void) { return hb_atomic_int_add (const_cast<hb_atomic_int_t &> (ref_count),  1); }
  inline int dec (void) { return hb_atomic_int_add (const_cast<hb_atomic_int_t &> (ref_count), -1); }
  inline void finish (void) { ref_count = HB_REFERENCE_COUNT_INVALID_VALUE; }

  inline bool is_invalid (void) const { return ref_count == HB_REFERENCE_COUNT_INVALID_VALUE; }

};




#define HB_USER_DATA_ARRAY_INIT {HB_LOCKABLE_SET_INIT}
struct hb_user_data_array_t
{
  

  struct hb_user_data_item_t {
    hb_user_data_key_t *key;
    void *data;
    hb_destroy_func_t destroy;

    inline bool operator == (hb_user_data_key_t *other_key) const { return key == other_key; }
    inline bool operator == (hb_user_data_item_t &other) const { return key == other.key; }

    void finish (void) { if (destroy) destroy (data); }
  };

  hb_lockable_set_t<hb_user_data_item_t, hb_mutex_t> items;

  inline void init (void) { items.init (); }

  HB_INTERNAL bool set (hb_user_data_key_t *key,
			void *              data,
			hb_destroy_func_t   destroy,
			hb_bool_t           replace,
			hb_mutex_t         &lock);

  HB_INTERNAL void *get (hb_user_data_key_t *key,
			hb_mutex_t          &lock);

  HB_INTERNAL void finish (hb_mutex_t &lock);
};




struct hb_object_header_t
{
  hb_reference_count_t ref_count;
  hb_mutex_t mutex;
  hb_user_data_array_t user_data;

#define HB_OBJECT_HEADER_STATIC {HB_REFERENCE_COUNT_INVALID, HB_MUTEX_INIT, HB_USER_DATA_ARRAY_INIT}

  static inline void *create (unsigned int size) {
    hb_object_header_t *obj = (hb_object_header_t *) calloc (1, size);

    if (likely (obj))
      obj->init ();

    return obj;
  }

  inline void init (void) {
    ref_count.init (1);
    mutex.init ();
    user_data.init ();
  }

  inline bool is_inert (void) const {
    return unlikely (ref_count.is_invalid ());
  }

  inline void reference (void) {
    if (unlikely (!this || this->is_inert ()))
      return;
    ref_count.inc ();
  }

  inline bool destroy (void) {
    if (unlikely (!this || this->is_inert ()))
      return false;
    if (ref_count.dec () != 1)
      return false;

    ref_count.finish (); 
    user_data.finish (mutex);
    mutex.finish ();

    return true;
  }

  inline void lock (void) {
    mutex.lock ();
  }

  inline void unlock (void) {
    mutex.unlock ();
  }

  inline bool set_user_data (hb_user_data_key_t *key,
			     void *              data,
			     hb_destroy_func_t   destroy_func,
			     hb_bool_t           replace) {
    if (unlikely (!this || this->is_inert ()))
      return false;

    return user_data.set (key, data, destroy_func, replace, mutex);
  }

  inline void *get_user_data (hb_user_data_key_t *key) {
    if (unlikely (!this || this->is_inert ()))
      return NULL;

    return user_data.get (key, mutex);
  }

  inline void trace (const char *function) const {
    if (unlikely (!this)) return;
    

    DEBUG_MSG (OBJECT, (void *) this,
	       "%s refcount=%d",
	       function,
	       this ? ref_count.ref_count : 0);
  }

  private:
  ASSERT_POD ();
};




template <typename Type>
static inline void hb_object_trace (const Type *obj, const char *function)
{
  obj->header.trace (function);
}
template <typename Type>
static inline Type *hb_object_create (void)
{
  Type *obj = (Type *) hb_object_header_t::create (sizeof (Type));
  hb_object_trace (obj, HB_FUNC);
  return obj;
}
template <typename Type>
static inline bool hb_object_is_inert (const Type *obj)
{
  return unlikely (obj->header.is_inert ());
}
template <typename Type>
static inline Type *hb_object_reference (Type *obj)
{
  hb_object_trace (obj, HB_FUNC);
  obj->header.reference ();
  return obj;
}
template <typename Type>
static inline bool hb_object_destroy (Type *obj)
{
  hb_object_trace (obj, HB_FUNC);
  return obj->header.destroy ();
}
template <typename Type>
static inline void hb_object_lock (Type *obj)
{
  hb_object_trace (obj, HB_FUNC);
  return obj->header.lock ();
}
template <typename Type>
static inline void hb_object_unlock (Type *obj)
{
  hb_object_trace (obj, HB_FUNC);
  return obj->header.unlock ();
}
template <typename Type>
static inline bool hb_object_set_user_data (Type               *obj,
					    hb_user_data_key_t *key,
					    void *              data,
					    hb_destroy_func_t   destroy,
					    hb_bool_t           replace)
{
  return obj->header.set_user_data (key, data, destroy, replace);
}

template <typename Type>
static inline void *hb_object_get_user_data (Type               *obj,
					     hb_user_data_key_t *key)
{
  return obj->header.get_user_data (key);
}


#endif 
