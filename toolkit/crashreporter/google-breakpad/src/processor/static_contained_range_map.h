








































#ifndef PROCESSOR_STATIC_CONTAINED_RANGE_MAP_H__
#define PROCESSOR_STATIC_CONTAINED_RANGE_MAP_H__

#include "processor/static_map-inl.h"

namespace google_breakpad {

template<typename AddressType, typename EntryType>
class StaticContainedRangeMap {
 public:
  StaticContainedRangeMap(): base_(), entry_size_(), entry_ptr_(), map_() { }
  explicit StaticContainedRangeMap(const char *base);

  
  
  
  
  
  bool RetrieveRange(const AddressType &address, const EntryType *&entry) const;

 private:
  friend class ModuleComparer;
  
  
  typedef StaticContainedRangeMap* SelfPtr;
  typedef
  StaticMap<AddressType, StaticContainedRangeMap> AddressToRangeMap;
  typedef typename AddressToRangeMap::const_iterator MapConstIterator;

  
  
  
  
  
  
  
  AddressType base_;

  
  
  
  
  uint32_t entry_size_;
  const EntryType *entry_ptr_;

  
  
  
  AddressToRangeMap map_;
};

}  


#endif  
