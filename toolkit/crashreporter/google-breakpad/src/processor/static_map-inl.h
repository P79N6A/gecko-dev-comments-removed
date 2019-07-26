


































#ifndef PROCESSOR_STATIC_MAP_INL_H__
#define PROCESSOR_STATIC_MAP_INL_H__

#include "processor/static_map.h"
#include "processor/static_map_iterator-inl.h"
#include "processor/logging.h"

namespace google_breakpad {

template<typename Key, typename Value, typename Compare>
StaticMap<Key, Value, Compare>::StaticMap(const char* raw_data)
    : raw_data_(raw_data),
      compare_() {
  
  num_nodes_ = *(reinterpret_cast<const uint32_t*>(raw_data_));

  offsets_ = reinterpret_cast<const uint32_t*>(
      raw_data_ + sizeof(num_nodes_));

  keys_ = reinterpret_cast<const Key*>(
      raw_data_ + (1 + num_nodes_) * sizeof(uint32_t));
}


template<typename Key, typename Value, typename Compare>
StaticMapIterator<Key, Value, Compare>
StaticMap<Key, Value, Compare>::find(const Key &key) const {
  int begin = 0;
  int end = num_nodes_;
  int middle;
  int compare_result;
  while (begin < end) {
    middle = begin + (end - begin) / 2;
    compare_result = compare_(key, GetKeyAtIndex(middle));
    if (compare_result == 0)
      return IteratorAtIndex(middle);
    if (compare_result < 0) {
      end = middle;
    } else {
      begin = middle + 1;
    }
  }
  return this->end();
}

template<typename Key, typename Value, typename Compare>
StaticMapIterator<Key, Value, Compare>
StaticMap<Key, Value, Compare>::lower_bound(const Key &key) const {
  int begin = 0;
  int end = num_nodes_;
  int middle;
  int comp_result;
  while (begin < end) {
    middle = begin + (end - begin) / 2;
    comp_result = compare_(key, GetKeyAtIndex(middle));
    if (comp_result == 0)
      return IteratorAtIndex(middle);
    if (comp_result < 0) {
      end = middle;
    } else {
      begin = middle + 1;
    }
  }
  return IteratorAtIndex(begin);
}

template<typename Key, typename Value, typename Compare>
StaticMapIterator<Key, Value, Compare>
StaticMap<Key, Value, Compare>::upper_bound(const Key &key) const {
  int begin = 0;
  int end = num_nodes_;
  int middle;
  int compare_result;
  while (begin < end) {
    middle = begin + (end - begin) / 2;
    compare_result = compare_(key, GetKeyAtIndex(middle));
    if (compare_result == 0)
      return IteratorAtIndex(middle + 1);
    if (compare_result < 0) {
      end = middle;
    } else {
      begin = middle + 1;
    }
  }
  return IteratorAtIndex(begin);
}

template<typename Key, typename Value, typename Compare>
bool StaticMap<Key, Value, Compare>::ValidateInMemoryStructure() const {
  
  if (!raw_data_) return false;
  int32_t num_nodes = *(reinterpret_cast<const int32_t*>(raw_data_));
  if (num_nodes < 0) {
    BPLOG(INFO) << "StaticMap check failed: negative number of nodes";
    return false;
  }

  int node_index = 0;
  if (num_nodes_) {
    uint64_t first_offset = sizeof(int32_t) * (num_nodes_ + 1)
                           + sizeof(Key) * num_nodes_;
    
    if (first_offset > 0xffffffffUL) {
      BPLOG(INFO) << "StaticMap check failed: size exceeds limit";
      return false;
    }
    if (offsets_[node_index] != static_cast<uint32_t>(first_offset)) {
      BPLOG(INFO) << "StaticMap check failed: first node offset is incorrect";
      return false;
    }
  }

  for (node_index = 1; node_index < num_nodes_; ++node_index) {
    
    if (offsets_[node_index] <= offsets_[node_index - 1]) {
      BPLOG(INFO) << "StaticMap check failed: node offsets non-increasing";
      return false;
    }
    
    if (compare_(GetKeyAtIndex(node_index),
                 GetKeyAtIndex(node_index - 1)) <= 0) {
      BPLOG(INFO) << "StaticMap check failed: node keys non-increasing";
      return false;
    }
  }
  return true;
}

template<typename Key, typename Value, typename Compare>
const Key StaticMap<Key, Value, Compare>::GetKeyAtIndex(int index) const {
  if (index < 0 || index >= num_nodes_) {
    BPLOG(ERROR) << "Key index out of range error";
    
    return 0;
  }
  return keys_[index];
}

}  

#endif  
