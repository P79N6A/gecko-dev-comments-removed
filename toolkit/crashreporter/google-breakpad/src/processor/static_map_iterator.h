





































#ifndef PROCESSOR_STATIC_MAP_ITERATOR_H__
#define PROCESSOR_STATIC_MAP_ITERATOR_H__

#include "google_breakpad/common/breakpad_types.h"

namespace google_breakpad {


template<typename Key, typename Value, typename Compare> class StaticMap;



template<typename Key, typename Value, typename Compare>
class StaticMapIterator {
 public:
  
  StaticMapIterator(): index_(-1), base_(NULL) { }

  
  StaticMapIterator& operator++();
  StaticMapIterator operator++(int post_fix_operator);

  StaticMapIterator& operator--();
  StaticMapIterator operator--(int post_fix_operator);

  
  const Key* GetKeyPtr() const;

  
  inline const Key GetKey() const { return *GetKeyPtr(); }

  
  const char* GetValueRawPtr() const;

  
  inline const Value* GetValuePtr() const {
    return reinterpret_cast<const Value*>(GetValueRawPtr());
  }

  bool operator==(const StaticMapIterator& x) const;
  bool operator!=(const StaticMapIterator& x) const;

  
  
  
  bool IsValid() const;

 private:
  friend class StaticMap<Key, Value, Compare>;

  
  explicit StaticMapIterator(const char* base, const int32_t &index);

  
  int32_t index_;

  
  const char* base_;

  
  int32_t num_nodes_;

  
  
  
  const uint32_t* offsets_;

  
  const Key* keys_;
};

}  

#endif  
