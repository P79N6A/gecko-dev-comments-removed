






































#ifndef PROCESSOR_RANGE_MAP_H__
#define PROCESSOR_RANGE_MAP_H__


#include <map>


namespace google_breakpad {


template<typename AddressType, typename EntryType>
class RangeMap {
 public:
  RangeMap() : map_() {}

  
  
  
  bool StoreRange(const AddressType &base,
                  const AddressType &size,
                  const EntryType &entry);

  
  
  
  bool RetrieveRange(const AddressType &address, EntryType *entry,
                     AddressType *entry_base, AddressType *entry_size) const;

  
  
  
  
  
  bool RetrieveNearestRange(const AddressType &address, EntryType *entry,
                            AddressType *entry_base, AddressType *entry_size)
                            const;

  
  
  
  
  
  
  
  bool RetrieveRangeAtIndex(int index, EntryType *entry,
                            AddressType *entry_base, AddressType *entry_size)
                            const;

  
  int GetCount() const;

  
  
  void Clear();

 private:
  class Range {
   public:
    Range(const AddressType &base, const EntryType &entry)
        : base_(base), entry_(entry) {}

    AddressType base() const { return base_; }
    EntryType entry() const { return entry_; }

   private:
    
    
    const AddressType base_;

    
    const EntryType entry_;
  };

  
  typedef std::map<AddressType, Range> AddressToRangeMap;
  typedef typename AddressToRangeMap::const_iterator MapConstIterator;
  typedef typename AddressToRangeMap::value_type MapValue;

  
  AddressToRangeMap map_;
};


}  


#endif  
