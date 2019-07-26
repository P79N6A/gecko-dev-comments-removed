





































#ifndef PROCESSOR_STATIC_RANGE_MAP_H__
#define PROCESSOR_STATIC_RANGE_MAP_H__


#include "processor/static_map-inl.h"

namespace google_breakpad {



template<typename AddressType, typename EntryType>
class StaticRangeMap {
 public:
  StaticRangeMap(): map_() { }
  explicit StaticRangeMap(const char *memory): map_(memory) { }

  
  
  
  bool RetrieveRange(const AddressType &address, const EntryType *&entry,
                     AddressType *entry_base, AddressType *entry_size) const;

  
  
  
  
  
  bool RetrieveNearestRange(const AddressType &address, const EntryType *&entry,
                            AddressType *entry_base, AddressType *entry_size)
                            const;

  
  
  
  
  
  
  
  bool RetrieveRangeAtIndex(int index, const EntryType *&entry,
                            AddressType *entry_base, AddressType *entry_size)
                            const;

  
  inline int GetCount() const { return map_.size(); }

 private:
  friend class ModuleComparer;
  class Range {
   public:
    AddressType base() const {
      return *(reinterpret_cast<const AddressType*>(this));
    }
    const EntryType* entryptr() const {
      return reinterpret_cast<const EntryType*>(this + sizeof(AddressType));
    }
  };

  
  typedef StaticRangeMap* SelfPtr;
  typedef StaticMap<AddressType, Range> AddressToRangeMap;
  typedef typename AddressToRangeMap::const_iterator MapConstIterator;

  AddressToRangeMap map_;
};

}  

#endif  
