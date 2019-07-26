


































#ifndef PROCESSOR_STATIC_RANGE_MAP_INL_H__
#define PROCESSOR_STATIC_RANGE_MAP_INL_H__

#include "processor/static_range_map.h"
#include "common/logging.h"

namespace google_breakpad {

template<typename AddressType, typename EntryType>
bool StaticRangeMap<AddressType, EntryType>::RetrieveRange(
    const AddressType &address, const EntryType *&entry,
    AddressType *entry_base, AddressType *entry_size) const {
  MapConstIterator iterator = map_.lower_bound(address);
  if (iterator == map_.end())
    return false;

  
  
  
  
  

  const Range *range = iterator.GetValuePtr();

  
  
  if (address < range->base())
    return false;

  entry = range->entryptr();
  if (entry_base)
    *entry_base = range->base();
  if (entry_size)
    *entry_size = iterator.GetKey() - range->base() + 1;

  return true;
}


template<typename AddressType, typename EntryType>
bool StaticRangeMap<AddressType, EntryType>::RetrieveNearestRange(
    const AddressType &address, const EntryType *&entry,
    AddressType *entry_base, AddressType *entry_size) const {
  
  if (RetrieveRange(address, entry, entry_base, entry_size))
    return true;

  
  
  
  
  

  MapConstIterator iterator = map_.upper_bound(address);
  if (iterator == map_.begin())
    return false;
  --iterator;

  const Range *range = iterator.GetValuePtr();
  entry = range->entryptr();
  if (entry_base)
    *entry_base = range->base();
  if (entry_size)
    *entry_size = iterator.GetKey() - range->base() + 1;

  return true;
}

template<typename AddressType, typename EntryType>
bool StaticRangeMap<AddressType, EntryType>::RetrieveRangeAtIndex(
    int index, const EntryType *&entry,
    AddressType *entry_base, AddressType *entry_size) const {

  if (index >= GetCount()) {
    BPLOG(ERROR) << "Index out of range: " << index << "/" << GetCount();
    return false;
  }

  MapConstIterator iterator = map_.IteratorAtIndex(index);

  const Range *range = iterator.GetValuePtr();

  entry = range->entryptr();
  if (entry_base)
    *entry_base = range->base();
  if (entry_size)
    *entry_size = iterator.GetKey() - range->base() + 1;

  return true;
}

}  


#endif  
