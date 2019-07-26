


































#ifndef PROCESSOR_ADDRESS_MAP_INL_H__
#define PROCESSOR_ADDRESS_MAP_INL_H__

#include "processor/address_map.h"

#include <assert.h>

#include "common/logging.h"

namespace google_breakpad {

template<typename AddressType, typename EntryType>
bool AddressMap<AddressType, EntryType>::Store(const AddressType &address,
                                               const EntryType &entry) {
  
  
  if (map_.find(address) != map_.end()) {
    BPLOG(INFO) << "Store failed, address " << HexString(address) <<
                   " is already present";
    return false;
  }

  map_.insert(MapValue(address, entry));
  return true;
}

template<typename AddressType, typename EntryType>
bool AddressMap<AddressType, EntryType>::Retrieve(
    const AddressType &address,
    EntryType *entry, AddressType *entry_address) const {
  BPLOG_IF(ERROR, !entry) << "AddressMap::Retrieve requires |entry|";
  assert(entry);

  
  
  
  
  
  MapConstIterator iterator = map_.upper_bound(address);
  if (iterator == map_.begin())
    return false;
  --iterator;

  *entry = iterator->second;
  if (entry_address)
    *entry_address = iterator->first;

  return true;
}

template<typename AddressType, typename EntryType>
void AddressMap<AddressType, EntryType>::Clear() {
  map_.clear();
}

}  

#endif  
