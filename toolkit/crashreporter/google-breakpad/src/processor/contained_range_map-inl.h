


































#ifndef PROCESSOR_CONTAINED_RANGE_MAP_INL_H__
#define PROCESSOR_CONTAINED_RANGE_MAP_INL_H__

#include "processor/contained_range_map.h"

#include <assert.h>

#include "common/logging.h"


namespace google_breakpad {


template<typename AddressType, typename EntryType>
ContainedRangeMap<AddressType, EntryType>::~ContainedRangeMap() {
  
  Clear();
}


template<typename AddressType, typename EntryType>
bool ContainedRangeMap<AddressType, EntryType>::StoreRange(
    const AddressType &base, const AddressType &size, const EntryType &entry) {
  AddressType high = base + size - 1;

  
  if (size <= 0 || high < base) {
    
    
    
    
    
    
    return false;
  }

  if (!map_)
    map_ = new AddressToRangeMap();

  MapIterator iterator_base = map_->lower_bound(base);
  MapIterator iterator_high = map_->lower_bound(high);
  MapIterator iterator_end = map_->end();

  if (iterator_base == iterator_high && iterator_base != iterator_end &&
      base >= iterator_base->second->base_) {
    

    
    
    
    
    if (iterator_base->second->base_ == base && iterator_base->first == high) {
      


      return false;
    }

    
    return iterator_base->second->StoreRange(base, size, entry);
  }

  
  
  
  
  bool contains_high = iterator_high != iterator_end &&
                       high >= iterator_high->second->base_;

  
  
  if ((iterator_base != iterator_end && base > iterator_base->second->base_) ||
      (contains_high && high < iterator_high->first)) {
    
    
    
    
    
    
    return false;
  }

  
  
  
  
  if (contains_high)
    ++iterator_high;

  
  
  
  AddressToRangeMap *child_map = NULL;

  if (iterator_base != iterator_high) {
    
    
    
    child_map = new AddressToRangeMap(iterator_base, iterator_high);

    
    map_->erase(iterator_base, iterator_high);
  }

  
  
  
  
  map_->insert(MapValue(high,
                        new ContainedRangeMap(base, entry, child_map)));
  return true;
}


template<typename AddressType, typename EntryType>
bool ContainedRangeMap<AddressType, EntryType>::RetrieveRange(
    const AddressType &address, EntryType *entry) const {
  BPLOG_IF(ERROR, !entry) << "ContainedRangeMap::RetrieveRange requires "
                             "|entry|";
  assert(entry);

  
  if (!map_)
    return false;

  
  
  
  
  
  
  MapConstIterator iterator = map_->lower_bound(address);
  if (iterator == map_->end() || address < iterator->second->base_)
    return false;

  
  
  
  if (!iterator->second->RetrieveRange(address, entry))
    *entry = iterator->second->entry_;

  return true;
}


template<typename AddressType, typename EntryType>
void ContainedRangeMap<AddressType, EntryType>::Clear() {
  if (map_) {
    MapConstIterator end = map_->end();
    for (MapConstIterator child = map_->begin(); child != end; ++child)
      delete child->second;

    delete map_;
    map_ = NULL;
  }
}


}  


#endif  
