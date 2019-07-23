


































#ifndef PROCESSOR_RANGE_MAP_INL_H__
#define PROCESSOR_RANGE_MAP_INL_H__


#include "processor/range_map.h"


namespace google_airbag {


template<typename AddressType, typename EntryType>
bool RangeMap<AddressType, EntryType>::StoreRange(const AddressType &base,
                                                  const AddressType &size,
                                                  const EntryType &entry) {
  AddressType high = base + size - 1;

  
  if (size <= 0 || high < base)
    return false;

  
  
  MapConstIterator iterator_base = map_.lower_bound(base);
  MapConstIterator iterator_high = map_.lower_bound(high);

  if (iterator_base != iterator_high) {
    
    
    
    return false;
  }

  if (iterator_high != map_.end()) {
    if (iterator_high->second.base() <= high) {
      
      
      
      return false;
    }
  }

  
  
  map_.insert(MapValue(high, Range(base, entry)));
  return true;
}


template<typename AddressType, typename EntryType>
bool RangeMap<AddressType, EntryType>::RetrieveRange(
    const AddressType &address, EntryType *entry,
    AddressType *entry_base, AddressType *entry_size) const {
  if (!entry)
    return false;

  MapConstIterator iterator = map_.lower_bound(address);
  if (iterator == map_.end())
    return false;

  
  
  
  
  
  if (address < iterator->second.base())
    return false;

  *entry = iterator->second.entry();
  if (entry_base)
    *entry_base = iterator->second.base();
  if (entry_size)
    *entry_size = iterator->first - iterator->second.base() + 1;

  return true;
}


template<typename AddressType, typename EntryType>
bool RangeMap<AddressType, EntryType>::RetrieveNearestRange(
    const AddressType &address, EntryType *entry,
    AddressType *entry_base, AddressType *entry_size) const {
  if (!entry)
    return false;

  
  if (RetrieveRange(address, entry, entry_base, entry_size))
    return true;

  
  
  
  
  
  MapConstIterator iterator = map_.upper_bound(address);
  if (iterator == map_.begin())
    return false;
  --iterator;

  *entry = iterator->second.entry();
  if (entry_base)
    *entry_base = iterator->first;
  if (entry_size)
    *entry_size = iterator->first - iterator->second.base() + 1;

  return true;
}


template<typename AddressType, typename EntryType>
void RangeMap<AddressType, EntryType>::Clear() {
  map_.clear();
}


}  


#endif  
