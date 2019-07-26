


































#ifndef PROCESSOR_STATIC_ADDRESS_MAP_INL_H__
#define PROCESSOR_STATIC_ADDRESS_MAP_INL_H__

#include "processor/static_address_map.h"

#include "processor/logging.h"

namespace google_breakpad {

template<typename AddressType, typename EntryType>
bool StaticAddressMap<AddressType, EntryType>::Retrieve(
    const AddressType &address,
    const EntryType *&entry, AddressType *entry_address) const {

  
  
  
  
  

  MapConstIterator iterator = map_.upper_bound(address);
  if (iterator == map_.begin())
    return false;
  --iterator;

  entry = iterator.GetValuePtr();
  
  if (entry_address)
    *entry_address = iterator.GetKey();

  return true;
}

}  

#endif  
