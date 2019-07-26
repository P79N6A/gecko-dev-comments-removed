


































#ifndef PROCESSOR_RANGE_MAP_INL_H__
#define PROCESSOR_RANGE_MAP_INL_H__


#include <assert.h>

#include "processor/range_map.h"
#include "common/logging.h"


namespace google_breakpad {


template<typename AddressType, typename EntryType>
bool RangeMap<AddressType, EntryType>::StoreRange(const AddressType &base,
                                                  const AddressType &size,
                                                  const EntryType &entry) {
  AddressType high = base + size - 1;

  
  if (size <= 0 || high < base) {
    
    
    
    BPLOG_IF(INFO, size != 0) << "StoreRange failed, " << HexString(base) <<
                                 "+" << HexString(size) << ", " <<
                                 HexString(high);
    return false;
  }

  
  
  MapConstIterator iterator_base = map_.lower_bound(base);
  MapConstIterator iterator_high = map_.lower_bound(high);

  if (iterator_base != iterator_high) {
    
    
    
    AddressType other_base = iterator_base->second.base();
    AddressType other_size = iterator_base->first - other_base + 1;
    BPLOG(INFO) << "StoreRange failed, an existing range is contained by or "
                   "extends lower than the new range: new " <<
                   HexString(base) << "+" << HexString(size) << ", existing " <<
                   HexString(other_base) << "+" << HexString(other_size);
    return false;
  }

  if (iterator_high != map_.end()) {
    if (iterator_high->second.base() <= high) {
      
      
      
      AddressType other_base = iterator_high->second.base();
      AddressType other_size = iterator_high->first - other_base + 1;
      BPLOG(INFO) << "StoreRange failed, an existing range contains or "
                     "extends higher than the new range: new " <<
                     HexString(base) << "+" << HexString(size) <<
                     ", existing " <<
                     HexString(other_base) << "+" << HexString(other_size);
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
  BPLOG_IF(ERROR, !entry) << "RangeMap::RetrieveRange requires |entry|";
  assert(entry);

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
  BPLOG_IF(ERROR, !entry) << "RangeMap::RetrieveNearestRange requires |entry|";
  assert(entry);

  
  if (RetrieveRange(address, entry, entry_base, entry_size))
    return true;

  
  
  
  
  
  MapConstIterator iterator = map_.upper_bound(address);
  if (iterator == map_.begin())
    return false;
  --iterator;

  *entry = iterator->second.entry();
  if (entry_base)
    *entry_base = iterator->second.base();
  if (entry_size)
    *entry_size = iterator->first - iterator->second.base() + 1;

  return true;
}


template<typename AddressType, typename EntryType>
bool RangeMap<AddressType, EntryType>::RetrieveRangeAtIndex(
    int index, EntryType *entry,
    AddressType *entry_base, AddressType *entry_size) const {
  BPLOG_IF(ERROR, !entry) << "RangeMap::RetrieveRangeAtIndex requires |entry|";
  assert(entry);

  if (index >= GetCount()) {
    BPLOG(ERROR) << "Index out of range: " << index << "/" << GetCount();
    return false;
  }

  
  
  MapConstIterator iterator = map_.begin();
  for (int this_index = 0; this_index < index; ++this_index)
    ++iterator;

  *entry = iterator->second.entry();
  if (entry_base)
    *entry_base = iterator->second.base();
  if (entry_size)
    *entry_size = iterator->first - iterator->second.base() + 1;

  return true;
}


template<typename AddressType, typename EntryType>
int RangeMap<AddressType, EntryType>::GetCount() const {
  return map_.size();
}


template<typename AddressType, typename EntryType>
void RangeMap<AddressType, EntryType>::Clear() {
  map_.clear();
}


}  


#endif  
