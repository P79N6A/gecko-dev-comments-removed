
































#ifndef GOOGLE_PROTOBUF_STUBS_MAP_UTIL_H__
#define GOOGLE_PROTOBUF_STUBS_MAP_UTIL_H__

#include <google/protobuf/stubs/common.h>

namespace google {
namespace protobuf {




template <class Collection>
const typename Collection::value_type::second_type&
FindWithDefault(const Collection& collection,
                const typename Collection::value_type::first_type& key,
                const typename Collection::value_type::second_type& value) {
  typename Collection::const_iterator it = collection.find(key);
  if (it == collection.end()) {
    return value;
  }
  return it->second;
}




template <class Collection>
const typename Collection::value_type::second_type*
FindOrNull(const Collection& collection,
           const typename Collection::value_type::first_type& key) {
  typename Collection::const_iterator it = collection.find(key);
  if (it == collection.end()) {
    return 0;
  }
  return &it->second;
}






template <class Collection>
const typename Collection::value_type::second_type
FindPtrOrNull(const Collection& collection,
              const typename Collection::value_type::first_type& key) {
  typename Collection::const_iterator it = collection.find(key);
  if (it == collection.end()) {
    return 0;
  }
  return it->second;
}





template <class Collection, class Key, class Value>
bool InsertOrUpdate(Collection * const collection,
                   const Key& key, const Value& value) {
  pair<typename Collection::iterator, bool> ret =
    collection->insert(typename Collection::value_type(key, value));
  if (!ret.second) {
    
    ret.first->second = value;
    return false;
  }
  return true;
}





template <class Collection, class Key, class Value>
bool InsertIfNotPresent(Collection * const collection,
                        const Key& key, const Value& value) {
  pair<typename Collection::iterator, bool> ret =
    collection->insert(typename Collection::value_type(key, value));
  return ret.second;
}

}  
}  

#endif  
