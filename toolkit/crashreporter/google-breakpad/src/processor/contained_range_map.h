


























































#ifndef PROCESSOR_CONTAINED_RANGE_MAP_H__
#define PROCESSOR_CONTAINED_RANGE_MAP_H__


#include <map>


namespace google_airbag {


template<typename AddressType, typename EntryType>
class ContainedRangeMap {
 public:
  
  
  
  ContainedRangeMap() : base_(), entry_(), map_(NULL) {}

  ~ContainedRangeMap();

  
  
  
  
  
  
  
  bool StoreRange(const AddressType &base,
                  const AddressType &size,
                  const EntryType &entry);

  
  
  
  
  
  
  bool RetrieveRange(const AddressType &address, EntryType *entry) const;

  
  
  
  
  
  void Clear();

 private:
  
  
  typedef std::map<AddressType, ContainedRangeMap *> AddressToRangeMap;
  typedef typename AddressToRangeMap::const_iterator MapConstIterator;
  typedef typename AddressToRangeMap::iterator MapIterator;
  typedef typename AddressToRangeMap::value_type MapValue;

  
  
  
  ContainedRangeMap(const AddressType &base, const EntryType &entry,
                    AddressToRangeMap *map)
      : base_(base), entry_(entry), map_(map) {}

  
  
  
  
  
  
  
  const AddressType base_;

  
  
  
  
  const EntryType entry_;

  
  
  
  AddressToRangeMap *map_;
};


}  


#endif  
