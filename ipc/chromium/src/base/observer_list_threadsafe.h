



#ifndef BASE_OBSERVER_LIST_THREADSAFE_H_
#define BASE_OBSERVER_LIST_THREADSAFE_H_

#include <vector>
#include <algorithm>

#include "base/basictypes.h"
#include "base/logging.h"
#include "base/message_loop.h"
#include "base/observer_list.h"
#include "base/ref_counted.h"
#include "base/task.h"

































template <class ObserverType>
class ObserverListThreadSafe
    : public base::RefCountedThreadSafe<ObserverListThreadSafe<ObserverType> > {
 public:
  ObserverListThreadSafe() {}

  ~ObserverListThreadSafe() {
    typename ObserversListMap::const_iterator it;
    for (it = observer_lists_.begin(); it != observer_lists_.end(); ++it)
      delete (*it).second;
    observer_lists_.clear();
  }

  
  void AddObserver(ObserverType* obs) {
    ObserverList<ObserverType>* list = NULL;
    MessageLoop* loop = MessageLoop::current();
    
    
    
    if (!loop)
      return;  
    {
      AutoLock lock(list_lock_);
      if (observer_lists_.find(loop) == observer_lists_.end())
        observer_lists_[loop] = new ObserverList<ObserverType>();
      list = observer_lists_[loop];
    }
    list->AddObserver(obs);
  }

  
  
  
  
  
  void RemoveObserver(ObserverType* obs) {
    ObserverList<ObserverType>* list = NULL;
    MessageLoop* loop = MessageLoop::current();
    if (!loop)
      return;  
    {
      AutoLock lock(list_lock_);
      list = observer_lists_[loop];
      if (!list) {
        NOTREACHED() << "RemoveObserver called on for unknown thread";
        return;
      }

      
      
      if (list->size() == 1)
        observer_lists_.erase(loop);
    }
    list->RemoveObserver(obs);

    
    
    
    if (list->size() == 0)
      delete list;
  }

  
  
  
  
  
  template <class Method>
  void Notify(Method m) {
    UnboundMethod<ObserverType, Method, Tuple0> method(m, MakeTuple());
    Notify<Method, Tuple0>(method);
  }

  template <class Method, class A>
  void Notify(Method m, const A &a) {
    UnboundMethod<ObserverType, Method, Tuple1<A> > method(m, MakeTuple(a));
    Notify<Method, Tuple1<A> >(method);
  }

  

 private:
  template <class Method, class Params>
  void Notify(const UnboundMethod<ObserverType, Method, Params>& method) {
    AutoLock lock(list_lock_);
    typename ObserversListMap::iterator it;
    for (it = observer_lists_.begin(); it != observer_lists_.end(); ++it) {
      MessageLoop* loop = (*it).first;
      ObserverList<ObserverType>* list = (*it).second;
      loop->PostTask(FROM_HERE,
          NewRunnableMethod(this,
              &ObserverListThreadSafe<ObserverType>::
                 template NotifyWrapper<Method, Params>, list, method));
    }
  }

  
  
  
  template <class Method, class Params>
  void NotifyWrapper(ObserverList<ObserverType>* list,
      const UnboundMethod<ObserverType, Method, Params>& method) {

    
    {
      AutoLock lock(list_lock_);
      typename ObserversListMap::iterator it =
          observer_lists_.find(MessageLoop::current());

      
      
      
      
      if (it == observer_lists_.end() || it->second != list)
        return;
    }

    {
      typename ObserverList<ObserverType>::Iterator it(*list);
      ObserverType* obs;
      while ((obs = it.GetNext()) != NULL)
        method.Run(obs);
    }

    
    if (list->size() == 0) {
#ifndef NDEBUG
      {
        AutoLock lock(list_lock_);
        
        typename ObserversListMap::iterator it =
            observer_lists_.find(MessageLoop::current());
        DCHECK(it == observer_lists_.end() || it->second != list);
      }
#endif
      delete list;
    }
  }

  typedef std::map<MessageLoop*, ObserverList<ObserverType>*> ObserversListMap;

  
  Lock list_lock_;  
  ObserversListMap observer_lists_;

  DISALLOW_EVIL_CONSTRUCTORS(ObserverListThreadSafe);
};

#endif  
