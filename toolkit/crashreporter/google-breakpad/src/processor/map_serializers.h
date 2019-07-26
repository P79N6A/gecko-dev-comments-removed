


































#ifndef PROCESSOR_MAP_SERIALIZERS_H__
#define PROCESSOR_MAP_SERIALIZERS_H__

#include <map>
#include <string>

#include "processor/simple_serializer.h"

#include "processor/address_map-inl.h"
#include "processor/range_map-inl.h"
#include "processor/contained_range_map-inl.h"

namespace google_breakpad {



template<typename Key, typename Value>
class StdMapSerializer {
 public:
  
  size_t SizeOf(const std::map<Key, Value> &m) const;

  
  
  
  
  char* Write(const std::map<Key, Value> &m, char* dest) const;

  
  
  
  
  
  char* Serialize(const std::map<Key, Value> &m, unsigned int *size) const;

 private:
  SimpleSerializer<Key> key_serializer_;
  SimpleSerializer<Value> value_serializer_;
};



template<typename Addr, typename Entry>
class AddressMapSerializer {
 public:
  
  size_t SizeOf(const AddressMap<Addr, Entry> &m) const {
    return std_map_serializer_.SizeOf(m.map_);
  }

  
  
  
  char* Write(const AddressMap<Addr, Entry> &m, char *dest) const {
    return std_map_serializer_.Write(m.map_, dest);
  }

  
  
  
  
  char* Serialize(const AddressMap<Addr, Entry> &m, unsigned int *size) const {
    return std_map_serializer_.Serialize(m.map_, size);
  }

 private:
  
  
  StdMapSerializer<Addr, Entry> std_map_serializer_;
};



template<typename Address, typename Entry>
class RangeMapSerializer {
 public:
  
  size_t SizeOf(const RangeMap<Address, Entry> &m) const;

  
  
  
  char* Write(const RangeMap<Address, Entry> &m, char* dest) const;

  
  
  
  
  char* Serialize(const RangeMap<Address, Entry> &m, unsigned int *size) const;

 private:
  
  typedef typename RangeMap<Address, Entry>::Range Range;

  
  SimpleSerializer<Address> address_serializer_;
  
  SimpleSerializer<Entry> entry_serializer_;
};



template<class AddrType, class EntryType>
class ContainedRangeMapSerializer {
 public:
  
  size_t SizeOf(const ContainedRangeMap<AddrType, EntryType> *m) const;

  
  
  
  char* Write(const ContainedRangeMap<AddrType, EntryType> *m,
              char* dest) const;

  
  
  
  
  char* Serialize(const ContainedRangeMap<AddrType, EntryType> *m,
                  unsigned int *size) const;

 private:
  
  typedef std::map<AddrType, ContainedRangeMap<AddrType, EntryType>*> Map;

  
  SimpleSerializer<AddrType> addr_serializer_;
  SimpleSerializer<EntryType> entry_serializer_;
};

}  

#endif  
