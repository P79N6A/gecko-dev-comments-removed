



#ifndef BASE_OBSERVER_LIST_H__
#define BASE_OBSERVER_LIST_H__

#include <algorithm>
#include <limits>
#include <vector>

#include "base/basictypes.h"
#include "base/logging.h"













































template <class ObserverType, bool check_empty = false>
class ObserverList {
 public:
  
  enum NotificationType {
    
    
    NOTIFY_ALL,

    
    
    NOTIFY_EXISTING_ONLY
  };

  ObserverList() : notify_depth_(0), type_(NOTIFY_ALL) {}
  ObserverList(NotificationType type) : notify_depth_(0), type_(type) {}
  ~ObserverList() {
    
    if (check_empty) {
      Compact();
      DCHECK_EQ(observers_.size(), 0U);
    }
  }

  
  void AddObserver(ObserverType* obs) {
    DCHECK(find(observers_.begin(), observers_.end(), obs) == observers_.end())
        << "Observers can only be added once!";
    observers_.push_back(obs);
  }

  
  void RemoveObserver(ObserverType* obs) {
    typename ListType::iterator it =
      std::find(observers_.begin(), observers_.end(), obs);
    if (it != observers_.end()) {
      if (notify_depth_) {
        *it = 0;
      } else {
        observers_.erase(it);
      }
    }
  }

  size_t size() const {
    return observers_.size();
  }

  ObserverType* GetElementAt(int index) const {
    return observers_[index];
  }

  
  
  class Iterator {
   public:
    Iterator(const ObserverList<ObserverType>& list)
        : list_(list),
          index_(0),
          max_index_(list.type_ == NOTIFY_ALL ?
                     std::numeric_limits<size_t>::max() :
                     list.observers_.size()) {
      ++list_.notify_depth_;
    }

    ~Iterator() {
      if (--list_.notify_depth_ == 0)
        list_.Compact();
    }

    ObserverType* GetNext() {
      ListType& observers = list_.observers_;
      
      size_t max_index = std::min(max_index_, observers.size());
      while (index_ < max_index && !observers[index_])
        ++index_;
      return index_ < max_index ? observers[index_++] : NULL;
    }

   private:
    const ObserverList<ObserverType>& list_;
    size_t index_;
    size_t max_index_;
  };

 private:
  typedef std::vector<ObserverType*> ListType;

  void Compact() const {
    typename ListType::iterator it = observers_.begin();
    while (it != observers_.end()) {
      if (*it) {
        ++it;
      } else {
        it = observers_.erase(it);
      }
    }
  }

  
  mutable ListType observers_;
  mutable int notify_depth_;
  NotificationType type_;

  friend class ObserverList::Iterator;

  DISALLOW_EVIL_CONSTRUCTORS(ObserverList);
};

#define FOR_EACH_OBSERVER(ObserverType, observer_list, func)  \
  do {                                                        \
    ObserverList<ObserverType>::Iterator it(observer_list);   \
    ObserverType* obs;                                        \
    while ((obs = it.GetNext()) != NULL)                      \
      obs->func;                                              \
  } while (0)

#endif
