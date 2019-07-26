






























































#ifndef PROCESSOR_STATIC_MAP_H__
#define PROCESSOR_STATIC_MAP_H__

#include "processor/static_map_iterator-inl.h"

namespace google_breakpad {


template<typename Key>
class DefaultCompare {
 public:
  int operator()(const Key &k1, const Key &k2) const {
    if (k1 < k2) return -1;
    if (k1 == k2) return 0;
    return 1;
  }
};

template<typename Key, typename Value, typename Compare = DefaultCompare<Key> >
class StaticMap {
 public:
  typedef StaticMapIterator<Key, Value, Compare> iterator;
  typedef StaticMapIterator<Key, Value, Compare> const_iterator;

  StaticMap() : raw_data_(0),
                num_nodes_(0),
                offsets_(0),
                compare_() { }

  explicit StaticMap(const char* raw_data);

  inline bool empty() const { return num_nodes_ == 0; }
  inline unsigned int size() const { return num_nodes_; }

  
  inline iterator begin() const { return IteratorAtIndex(0); }
  inline iterator last() const { return IteratorAtIndex(num_nodes_ - 1); }
  inline iterator end() const { return IteratorAtIndex(num_nodes_); }
  inline iterator IteratorAtIndex(int index) const {
    return iterator(raw_data_, index);
  }

  
  iterator find(const Key &k) const;

  
  
  iterator lower_bound(const Key &k) const;

  
  
  iterator upper_bound(const Key &k) const;

  
  
  
  bool ValidateInMemoryStructure() const;

 private:
  const Key GetKeyAtIndex(int i) const;

  
  const char* raw_data_;

  
  int32_t num_nodes_;

  
  
  
  const uint32_t* offsets_;

  
  const Key* keys_;

  Compare compare_;
};

}  

#endif  
