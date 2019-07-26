








































#ifndef PROCESSOR_STATIC_ADDRESS_MAP_H__
#define PROCESSOR_STATIC_ADDRESS_MAP_H__

#include "processor/static_map-inl.h"

namespace google_breakpad {



template<typename AddressType, typename EntryType>
class StaticAddressMap {
 public:
  StaticAddressMap(): map_() { }
  explicit StaticAddressMap(const char *map_data): map_(map_data) { }

  
  
  
  
  
  bool Retrieve(const AddressType &address,
                const EntryType *&entry, AddressType *entry_address) const;

 private:
  friend class ModuleComparer;
  
  typedef StaticAddressMap* SelfPtr;
  typedef StaticMap<AddressType, EntryType> AddressToEntryMap;
  typedef typename AddressToEntryMap::const_iterator MapConstIterator;

  AddressToEntryMap map_;
};

}  

#endif  

