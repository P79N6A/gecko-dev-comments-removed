

























#ifndef HB_CACHE_PRIVATE_HH
#define HB_CACHE_PRIVATE_HH

#include "hb-private.hh"




template <unsigned int key_bits, unsigned int value_bits, unsigned int cache_bits>
struct hb_cache_t
{
  ASSERT_STATIC (key_bits >= cache_bits);
  ASSERT_STATIC (key_bits + value_bits - cache_bits < 8 * sizeof (unsigned int));

  inline void clear (void)
  {
    memset (values, 255, sizeof (values));
  }

  inline bool get (unsigned int key, unsigned int *value)
  {
    unsigned int k = key & ((1<<cache_bits)-1);
    unsigned int v = values[k];
    if ((v >> value_bits) != (key >> cache_bits))
      return false;
    *value = v & ((1<<value_bits)-1);
    return true;
  }

  inline bool set (unsigned int key, unsigned int value)
  {
    if (unlikely ((key >> key_bits) || (value >> value_bits)))
      return false; 
    unsigned int k = key & ((1<<cache_bits)-1);
    unsigned int v = ((key>>cache_bits)<<value_bits) | value;
    values[k] = v;
    return true;
  }

  private:
  unsigned int values[1<<cache_bits];
};

typedef hb_cache_t<21, 16, 8> hb_cmap_cache_t;
typedef hb_cache_t<16, 24, 8> hb_advance_cache_t;


#endif 
