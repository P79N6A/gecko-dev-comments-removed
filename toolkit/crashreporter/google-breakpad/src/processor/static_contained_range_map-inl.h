



































#ifndef PROCESSOR_STATIC_CONTAINED_RANGE_MAP_INL_H__
#define PROCESSOR_STATIC_CONTAINED_RANGE_MAP_INL_H__

#include "processor/static_contained_range_map.h"
#include "common/logging.h"

namespace google_breakpad {

template<typename AddressType, typename EntryType>
StaticContainedRangeMap<AddressType, EntryType>::StaticContainedRangeMap(
    const char *base)
    : base_(*(reinterpret_cast<const AddressType*>(base))),
      entry_size_(*(reinterpret_cast<const uint32_t*>(base + sizeof(base_)))),
      entry_ptr_(reinterpret_cast<const EntryType *>(
          base + sizeof(base_) + sizeof(entry_size_))),
      map_(base + sizeof(base_) + sizeof(entry_size_) + entry_size_) {
  if (entry_size_ == 0)
    entry_ptr_ = NULL;
}


template<typename AddressType, typename EntryType>
bool StaticContainedRangeMap<AddressType, EntryType>::RetrieveRange(
    const AddressType &address, const EntryType *&entry) const {

  
  
  
  
  
  
  MapConstIterator iterator = map_.lower_bound(address);

  if (iterator == map_.end())
    return false;

  const char *memory_child =
      reinterpret_cast<const char*>(iterator.GetValuePtr());

  StaticContainedRangeMap child_map(memory_child);

  if (address < child_map.base_)
    return false;

  
  
  
  if (!child_map.RetrieveRange(address, entry))
    entry = child_map.entry_ptr_;

  return true;
}

}  

#endif  
