

































#ifndef PROCESSOR_STATIC_MAP_ITERATOR_INL_H__
#define PROCESSOR_STATIC_MAP_ITERATOR_INL_H__

#include "processor/static_map_iterator.h"

#include "processor/logging.h"

namespace google_breakpad {

template<typename Key, typename Value, typename Compare>
StaticMapIterator<Key, Value, Compare>::StaticMapIterator(const char* base,
                                                            const int &index):
      index_(index), base_(base) {
  
  
  num_nodes_ = *(reinterpret_cast<const int32_t*>(base_));
  offsets_ = reinterpret_cast<const uint32_t*>(base_ + sizeof(num_nodes_));
  keys_ = reinterpret_cast<const Key*>(
      base_ + (1 + num_nodes_) * sizeof(num_nodes_));
}


template<typename Key, typename Value, typename Compare>
StaticMapIterator<Key, Value, Compare>&
StaticMapIterator<Key, Value, Compare>::operator++() {
  if (!IsValid()) {
    BPLOG(ERROR) << "operator++ on invalid iterator";
    return *this;
  }
  if (++index_ > num_nodes_) index_ = num_nodes_;
  return *this;
}

template<typename Key, typename Value, typename Compare>
StaticMapIterator<Key, Value, Compare>
StaticMapIterator<Key, Value, Compare>::operator++(int postfix_operator) {
  if (!IsValid()) {
    BPLOG(ERROR) << "operator++ on invalid iterator";
    return *this;
  }
  StaticMapIterator<Key, Value, Compare> tmp = *this;
  if (++index_ > num_nodes_) index_ = num_nodes_;
  return tmp;
}

template<typename Key, typename Value, typename Compare>
StaticMapIterator<Key, Value, Compare>&
StaticMapIterator<Key, Value, Compare>::operator--() {
  if (!IsValid()) {
    BPLOG(ERROR) << "operator++ on invalid iterator";
    return *this;
  }

  if (--index_ < 0) index_ = 0;
  return *this;
}

template<typename Key, typename Value, typename Compare>
StaticMapIterator<Key, Value, Compare>
StaticMapIterator<Key, Value, Compare>::operator--(int postfix_operator) {
  if (!IsValid()) {
    BPLOG(ERROR) << "operator++ on invalid iterator";
    return *this;
  }
  StaticMapIterator<Key, Value, Compare> tmp = *this;

  if (--index_ < 0) index_ = 0;
  return tmp;
}

template<typename Key, typename Value, typename Compare>
const Key* StaticMapIterator<Key, Value, Compare>::GetKeyPtr() const {
  if (!IsValid()) {
    BPLOG(ERROR) << "call GetKeyPtr() on invalid iterator";
    return NULL;
  }
  return &(keys_[index_]);
}

template<typename Key, typename Value, typename Compare>
const char* StaticMapIterator<Key, Value, Compare>::GetValueRawPtr() const {
  if (!IsValid()) {
    BPLOG(ERROR) << "call GetValuePtr() on invalid iterator";
    return NULL;
  }
  return base_ + offsets_[index_];
}

template<typename Key, typename Value, typename Compare>
bool StaticMapIterator<Key, Value, Compare>::operator==(
    const StaticMapIterator<Key, Value, Compare>& x) const {
  return base_ == x.base_ && index_ == x.index_;
}

template<typename Key, typename Value, typename Compare>
bool StaticMapIterator<Key, Value, Compare>::operator!=(
    const StaticMapIterator<Key, Value, Compare>& x) const {
  
  
  return base_ != x.base_ || index_ != x.index_;
}

template<typename Key, typename Value, typename Compare>
bool StaticMapIterator<Key, Value, Compare>::IsValid() const {
  if (!base_ || index_ < 0 || index_ > num_nodes_)
    return false;

  return true;
}

}  

#endif  
