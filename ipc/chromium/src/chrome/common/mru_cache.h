














#ifndef CHROME_COMMON_MRU_CACHE_H__
#define CHROME_COMMON_MRU_CACHE_H__

#include <list>
#include <map>
#include <utility>

#include "base/basictypes.h"
#include "chrome/common/logging_chrome.h"






template <class KeyType, class PayloadType, class DeletorType>
class MRUCacheBase {
 public:
  
  
  typedef std::pair<KeyType, PayloadType> value_type;

 private:
  typedef std::list<value_type> PayloadList;
  typedef std::map<KeyType, typename PayloadList::iterator> KeyIndex;

 public:
  typedef typename PayloadList::size_type size_type;

  typedef typename PayloadList::iterator iterator;
  typedef typename PayloadList::const_iterator const_iterator;
  typedef typename PayloadList::reverse_iterator reverse_iterator;
  typedef typename PayloadList::const_reverse_iterator const_reverse_iterator;

  enum { NO_AUTO_EVICT = 0 };

  
  
  
  
  MRUCacheBase(size_type max_size) : max_size_(max_size) {
  }

  virtual ~MRUCacheBase() {
    iterator i = begin();
    while (i != end())
      i = Erase(i);
  }

  
  
  
  
  
  
  iterator Put(const KeyType& key, const PayloadType& payload) {
    
    typename KeyIndex::iterator index_iter = index_.find(key);
    if (index_iter != index_.end()) {
      
      
      Erase(index_iter->second);
    } else if (max_size_ != NO_AUTO_EVICT) {
      
      
      ShrinkToSize(max_size_ - 1);
    }

    ordering_.push_front(value_type(key, payload));
    index_[key] = ordering_.begin();
    return ordering_.begin();
  }

  
  
  
  
  
  iterator Get(const KeyType& key) {
    typename KeyIndex::iterator index_iter = index_.find(key);
    if (index_iter == index_.end())
      return end();
    typename PayloadList::iterator iter = index_iter->second;

    
    ordering_.splice(ordering_.begin(), ordering_, iter);
    return ordering_.begin();
  }

  
  
  
  
  iterator Peek(const KeyType& key) {
    typename KeyIndex::const_iterator index_iter = index_.find(key);
    if (index_iter == index_.end())
      return end();
    return index_iter->second;
  }

  
  
  iterator Erase(iterator pos) {
    deletor_(pos->second);
    index_.erase(pos->first);
    return ordering_.erase(pos);
  }

  
  
  reverse_iterator Erase(reverse_iterator pos) {
    
    
    
    return reverse_iterator(Erase((++pos).base()));
  }

  
  
  void ShrinkToSize(size_type new_size) {
    for (size_type i = size(); i > new_size; i--)
      Erase(rbegin());
  }

  
  size_type size() const {
    
    
    DCHECK(index_.size() == ordering_.size());
    return index_.size();
  }

  
  
  
  
  
  
  iterator begin() { return ordering_.begin(); }
  const_iterator begin() const { ordering_.begin(); }
  iterator end() { return ordering_.end(); }
  const_iterator end() const { return ordering_.end(); }

  reverse_iterator rbegin() { return ordering_.rbegin(); }
  const_reverse_iterator rbegin() const { ordering_.rbegin(); }
  reverse_iterator rend() { return ordering_.rend(); }
  const_reverse_iterator rend() const { return ordering_.rend(); }

  bool empty() const { return ordering_.empty(); }

 private:
  PayloadList ordering_;
  KeyIndex index_;

  size_type max_size_;

  DeletorType deletor_;

  DISALLOW_EVIL_CONSTRUCTORS(MRUCacheBase);
};




template<class PayloadType>
class MRUCacheNullDeletor {
 public:
  void operator()(PayloadType& payload) {
  }
};



template <class KeyType, class PayloadType>
class MRUCache : public MRUCacheBase<KeyType,
                                     PayloadType,
                                     MRUCacheNullDeletor<PayloadType> > {
 private:
  typedef MRUCacheBase<KeyType, PayloadType,
      MRUCacheNullDeletor<PayloadType> > ParentType;

 public:
  
  MRUCache(typename ParentType::size_type max_size)
      : ParentType(max_size) {
  }
  virtual ~MRUCache() {
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(MRUCache);
};



template<class PayloadType>
class MRUCachePointerDeletor {
 public:
  void operator()(PayloadType& payload) {
    delete payload;
  }
};




template <class KeyType, class PayloadType>
class OwningMRUCache
    : public MRUCacheBase<KeyType,
                          PayloadType,
                          MRUCachePointerDeletor<PayloadType> > {
 private:
  typedef MRUCacheBase<KeyType, PayloadType,
      MRUCachePointerDeletor<PayloadType> > ParentType;

 public:
  
  OwningMRUCache(typename ParentType::size_type max_size)
      : ParentType(max_size) {
  }
  virtual ~OwningMRUCache() {
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(OwningMRUCache);
};

#endif  
