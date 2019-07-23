




































#ifndef PROCESSOR_ADDRESS_MAP_H__
#define PROCESSOR_ADDRESS_MAP_H__

#include <map>

namespace google_breakpad {

template<typename AddressType, typename EntryType>
class AddressMap {
 public:
  AddressMap() : map_() {}

  
  
  
  bool Store(const AddressType &address, const EntryType &entry);

  
  
  
  
  
  bool Retrieve(const AddressType &address,
                EntryType *entry, AddressType *entry_address) const;

  
  
  void Clear();

 private:
  
  typedef std::map<AddressType, EntryType> AddressToEntryMap;
  typedef typename AddressToEntryMap::const_iterator MapConstIterator;
  typedef typename AddressToEntryMap::value_type MapValue;

  
  AddressToEntryMap map_;
};

}  

#endif  

