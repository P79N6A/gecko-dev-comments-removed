



#ifndef BASE_ID_MAP_H__
#define BASE_ID_MAP_H__

#include "base/basictypes.h"
#include "base/hash_tables.h"
#include "base/logging.h"









template<class T>
class IDMap {
 private:
  typedef base::hash_map<int32, T*> HashTable;
  typedef typename HashTable::iterator iterator;

 public:
  
  
  typedef typename HashTable::const_iterator const_iterator;

  IDMap() : next_id_(1) {
  }
  IDMap(const IDMap& other) : next_id_(other.next_id_),
                                        data_(other.data_) {
  }

  const_iterator begin() const {
    return data_.begin();
  }
  const_iterator end() const {
    return data_.end();
  }

  
  int32 Add(T* data) {
    int32 this_id = next_id_;
    DCHECK(data_.find(this_id) == data_.end()) << "Inserting duplicate item";
    data_[this_id] = data;
    next_id_++;
    return this_id;
  }

  
  
  
  
  void AddWithID(T* data, int32 id) {
    DCHECK(data_.find(id) == data_.end()) << "Inserting duplicate item";
    data_[id] = data;
  }

  void Remove(int32 id) {
    iterator i = data_.find(id);
    if (i == data_.end()) {
      NOTREACHED() << "Attempting to remove an item not in the list";
      return;
    }
    data_.erase(i);
  }

  bool IsEmpty() const {
    return data_.empty();
  }

  T* Lookup(int32 id) const {
    const_iterator i = data_.find(id);
    if (i == data_.end())
      return NULL;
    return i->second;
  }

  size_t size() const {
    return data_.size();
  }

 protected:
  
  int32 next_id_;

  HashTable data_;
};

#endif  
