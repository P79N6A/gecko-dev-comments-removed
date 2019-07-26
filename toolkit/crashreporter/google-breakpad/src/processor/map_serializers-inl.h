



































#ifndef PROCESSOR_MAP_SERIALIZERS_INL_H__
#define PROCESSOR_MAP_SERIALIZERS_INL_H__

#include <map>
#include <string>

#include "processor/map_serializers.h"
#include "processor/simple_serializer.h"

#include "processor/address_map-inl.h"
#include "processor/range_map-inl.h"
#include "processor/contained_range_map-inl.h"

#include "processor/logging.h"

namespace google_breakpad {

template<typename Key, typename Value>
size_t StdMapSerializer<Key, Value>::SizeOf(
    const std::map<Key, Value> &m) const {
  size_t size = 0;
  size_t header_size = (1 + m.size()) * sizeof(uint32_t);
  size += header_size;

  typename std::map<Key, Value>::const_iterator iter;
  for (iter = m.begin(); iter != m.end(); ++iter) {
    size += key_serializer_.SizeOf(iter->first);
    size += value_serializer_.SizeOf(iter->second);
  }
  return size;
}

template<typename Key, typename Value>
char *StdMapSerializer<Key, Value>::Write(const std::map<Key, Value> &m,
                                          char *dest) const {
  if (!dest) {
    BPLOG(ERROR) << "StdMapSerializer failed: write to NULL address.";
    return NULL;
  }
  char *start_address = dest;

  
  
  dest = SimpleSerializer<uint32_t>::Write(m.size(), dest);
  
  uint32_t *offsets = reinterpret_cast<uint32_t*>(dest);
  dest += sizeof(uint32_t) * m.size();

  char *key_address = dest;
  dest += sizeof(Key) * m.size();

  
  typename std::map<Key, Value>::const_iterator iter;
  int index = 0;
  for (iter = m.begin(); iter != m.end(); ++iter, ++index) {
    offsets[index] = static_cast<uint32_t>(dest - start_address);
    key_address = key_serializer_.Write(iter->first, key_address);
    dest = value_serializer_.Write(iter->second, dest);
  }
  return dest;
}

template<typename Key, typename Value>
char *StdMapSerializer<Key, Value>::Serialize(
    const std::map<Key, Value> &m, unsigned int *size) const {
  
  unsigned int size_to_alloc = SizeOf(m);
  
  char *serialized_data = new char[size_to_alloc];
  if (!serialized_data) {
    BPLOG(INFO) << "StdMapSerializer memory allocation failed.";
    if (size) *size = 0;
    return NULL;
  }
  
  Write(m, serialized_data);

  if (size) *size = size_to_alloc;
  return serialized_data;
}

template<typename Address, typename Entry>
size_t RangeMapSerializer<Address, Entry>::SizeOf(
    const RangeMap<Address, Entry> &m) const {
  size_t size = 0;
  size_t header_size = (1 + m.map_.size()) * sizeof(uint32_t);
  size += header_size;

  typename std::map<Address, Range>::const_iterator iter;
  for (iter = m.map_.begin(); iter != m.map_.end(); ++iter) {
    
    size += address_serializer_.SizeOf(iter->first);
    
    size += address_serializer_.SizeOf(iter->second.base());
    
    size += entry_serializer_.SizeOf(iter->second.entry());
  }
  return size;
}

template<typename Address, typename Entry>
char *RangeMapSerializer<Address, Entry>::Write(
    const RangeMap<Address, Entry> &m, char *dest) const {
  if (!dest) {
    BPLOG(ERROR) << "RangeMapSerializer failed: write to NULL address.";
    return NULL;
  }
  char *start_address = dest;

  
  
  dest = SimpleSerializer<uint32_t>::Write(m.map_.size(), dest);
  
  uint32_t *offsets = reinterpret_cast<uint32_t*>(dest);
  dest += sizeof(uint32_t) * m.map_.size();

  char *key_address = dest;
  dest += sizeof(Address) * m.map_.size();

  
  typename std::map<Address, Range>::const_iterator iter;
  int index = 0;
  for (iter = m.map_.begin(); iter != m.map_.end(); ++iter, ++index) {
    offsets[index] = static_cast<uint32_t>(dest - start_address);
    key_address = address_serializer_.Write(iter->first, key_address);
    dest = address_serializer_.Write(iter->second.base(), dest);
    dest = entry_serializer_.Write(iter->second.entry(), dest);
  }
  return dest;
}

template<typename Address, typename Entry>
char *RangeMapSerializer<Address, Entry>::Serialize(
    const RangeMap<Address, Entry> &m, unsigned int *size) const {
  
  unsigned int size_to_alloc = SizeOf(m);
  
  char *serialized_data = new char[size_to_alloc];
  if (!serialized_data) {
    BPLOG(INFO) << "RangeMapSerializer memory allocation failed.";
    if (size) *size = 0;
    return NULL;
  }

  
  Write(m, serialized_data);

  if (size) *size = size_to_alloc;
  return serialized_data;
}


template<class AddrType, class EntryType>
size_t ContainedRangeMapSerializer<AddrType, EntryType>::SizeOf(
    const ContainedRangeMap<AddrType, EntryType> *m) const {
  size_t size = 0;
  size_t header_size = addr_serializer_.SizeOf(m->base_)
                       + entry_serializer_.SizeOf(m->entry_)
                       + sizeof(uint32_t);
  size += header_size;
  
  size += sizeof(uint32_t);
  if (m->map_) {
    size += m->map_->size() * sizeof(uint32_t);
    typename Map::const_iterator iter;
    for (iter = m->map_->begin(); iter != m->map_->end(); ++iter) {
      size += addr_serializer_.SizeOf(iter->first);
      
      size += SizeOf(iter->second);
    }
  }
  return size;
}

template<class AddrType, class EntryType>
char *ContainedRangeMapSerializer<AddrType, EntryType>::Write(
    const ContainedRangeMap<AddrType, EntryType> *m, char *dest) const {
  if (!dest) {
    BPLOG(ERROR) << "StdMapSerializer failed: write to NULL address.";
    return NULL;
  }
  dest = addr_serializer_.Write(m->base_, dest);
  dest = SimpleSerializer<uint32_t>::Write(entry_serializer_.SizeOf(m->entry_),
                                            dest);
  dest = entry_serializer_.Write(m->entry_, dest);

  
  char *map_address = dest;
  if (m->map_ == NULL) {
    dest = SimpleSerializer<uint32_t>::Write(0, dest);
  } else {
    dest = SimpleSerializer<uint32_t>::Write(m->map_->size(), dest);
    uint32_t *offsets = reinterpret_cast<uint32_t*>(dest);
    dest += sizeof(uint32_t) * m->map_->size();

    char *key_address = dest;
    dest += sizeof(AddrType) * m->map_->size();

    
    typename Map::const_iterator iter;
    int index = 0;
    for (iter = m->map_->begin(); iter != m->map_->end(); ++iter, ++index) {
      offsets[index] = static_cast<uint32_t>(dest - map_address);
      key_address = addr_serializer_.Write(iter->first, key_address);
      
      dest = Write(iter->second, dest);
    }
  }
  return dest;
}

template<class AddrType, class EntryType>
char *ContainedRangeMapSerializer<AddrType, EntryType>::Serialize(
    const ContainedRangeMap<AddrType, EntryType> *m, unsigned int *size) const {
  unsigned int size_to_alloc = SizeOf(m);
  
  char *serialized_data = new char[size_to_alloc];
  if (!serialized_data) {
    BPLOG(INFO) << "ContainedRangeMapSerializer memory allocation failed.";
    if (size) *size = 0;
    return NULL;
  }
  Write(m, serialized_data);
  if (size) *size = size_to_alloc;
  return serialized_data;
}

}  

#endif  
