



#ifndef BASE_OBSERVER_LIST_THREADSAFE_H_
#define BASE_OBSERVER_LIST_THREADSAFE_H_

#include <algorithm>
#include <map>

#include "base/basictypes.h"
#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/message_loop/message_loop_proxy.h"
#include "base/observer_list.h"
#include "base/stl_util.h"
#include "base/threading/platform_thread.h"



































template <class ObserverType>
class ObserverListThreadSafe;



template <class T, class Method, class Params>
class UnboundMethod {
 public:
  UnboundMethod(Method m, const Params& p) : m_(m), p_(p) {
    COMPILE_ASSERT(
        (base::internal::ParamsUseScopedRefptrCorrectly<Params>::value),
        badunboundmethodparams);
  }
  void Run(T* obj) const {
    DispatchToMethod(obj, m_, p_);
  }
 private:
  Method m_;
  Params p_;
};










template <class T>
struct ObserverListThreadSafeTraits {
  static void Destruct(const ObserverListThreadSafe<T>* x) {
    delete x;
  }
};

template <class ObserverType>
class ObserverListThreadSafe
    : public base::RefCountedThreadSafe<
        ObserverListThreadSafe<ObserverType>,
        ObserverListThreadSafeTraits<ObserverType> > {
 public:
  typedef typename ObserverList<ObserverType>::NotificationType
      NotificationType;

  ObserverListThreadSafe()
      : type_(ObserverListBase<ObserverType>::NOTIFY_ALL) {}
  explicit ObserverListThreadSafe(NotificationType type) : type_(type) {}

  
  
  void AddObserver(ObserverType* obs) {
    
    
    if (!base::MessageLoop::current())
      return;

    ObserverList<ObserverType>* list = NULL;
    base::PlatformThreadId thread_id = base::PlatformThread::CurrentId();
    {
      base::AutoLock lock(list_lock_);
      if (observer_lists_.find(thread_id) == observer_lists_.end())
        observer_lists_[thread_id] = new ObserverListContext(type_);
      list = &(observer_lists_[thread_id]->list);
    }
    list->AddObserver(obs);
  }

  
  
  
  
  
  void RemoveObserver(ObserverType* obs) {
    ObserverListContext* context = NULL;
    ObserverList<ObserverType>* list = NULL;
    base::PlatformThreadId thread_id = base::PlatformThread::CurrentId();
    {
      base::AutoLock lock(list_lock_);
      typename ObserversListMap::iterator it = observer_lists_.find(thread_id);
      if (it == observer_lists_.end()) {
        
        
        return;
      }
      context = it->second;
      list = &context->list;

      
      
      if (list->HasObserver(obs) && list->size() == 1)
        observer_lists_.erase(it);
    }
    list->RemoveObserver(obs);

    
    
    
    if (list->size() == 0)
      delete context;
  }

  
  void AssertEmpty() const {
    base::AutoLock lock(list_lock_);
    DCHECK(observer_lists_.empty());
  }

  
  
  
  
  
  template <class Method>
  void Notify(Method m) {
    UnboundMethod<ObserverType, Method, Tuple0> method(m, MakeTuple());
    Notify<Method, Tuple0>(method);
  }

  template <class Method, class A>
  void Notify(Method m, const A& a) {
    UnboundMethod<ObserverType, Method, Tuple1<A> > method(m, MakeTuple(a));
    Notify<Method, Tuple1<A> >(method);
  }

  template <class Method, class A, class B>
  void Notify(Method m, const A& a, const B& b) {
    UnboundMethod<ObserverType, Method, Tuple2<A, B> > method(
        m, MakeTuple(a, b));
    Notify<Method, Tuple2<A, B> >(method);
  }

  template <class Method, class A, class B, class C>
  void Notify(Method m, const A& a, const B& b, const C& c) {
    UnboundMethod<ObserverType, Method, Tuple3<A, B, C> > method(
        m, MakeTuple(a, b, c));
    Notify<Method, Tuple3<A, B, C> >(method);
  }

  template <class Method, class A, class B, class C, class D>
  void Notify(Method m, const A& a, const B& b, const C& c, const D& d) {
    UnboundMethod<ObserverType, Method, Tuple4<A, B, C, D> > method(
        m, MakeTuple(a, b, c, d));
    Notify<Method, Tuple4<A, B, C, D> >(method);
  }

  

 private:
  
  friend struct ObserverListThreadSafeTraits<ObserverType>;

  struct ObserverListContext {
    explicit ObserverListContext(NotificationType type)
        : loop(base::MessageLoopProxy::current()),
          list(type) {
    }

    scoped_refptr<base::MessageLoopProxy> loop;
    ObserverList<ObserverType> list;

    DISALLOW_COPY_AND_ASSIGN(ObserverListContext);
  };

  ~ObserverListThreadSafe() {
    STLDeleteValues(&observer_lists_);
  }

  template <class Method, class Params>
  void Notify(const UnboundMethod<ObserverType, Method, Params>& method) {
    base::AutoLock lock(list_lock_);
    typename ObserversListMap::iterator it;
    for (it = observer_lists_.begin(); it != observer_lists_.end(); ++it) {
      ObserverListContext* context = (*it).second;
      context->loop->PostTask(
          FROM_HERE,
          base::Bind(&ObserverListThreadSafe<ObserverType>::
              template NotifyWrapper<Method, Params>, this, context, method));
    }
  }

  
  
  
  template <class Method, class Params>
  void NotifyWrapper(ObserverListContext* context,
      const UnboundMethod<ObserverType, Method, Params>& method) {

    
    {
      base::AutoLock lock(list_lock_);
      typename ObserversListMap::iterator it =
          observer_lists_.find(base::PlatformThread::CurrentId());

      
      
      
      
      if (it == observer_lists_.end() || it->second != context)
        return;
    }

    {
      typename ObserverList<ObserverType>::Iterator it(context->list);
      ObserverType* obs;
      while ((obs = it.GetNext()) != NULL)
        method.Run(obs);
    }

    
    if (context->list.size() == 0) {
      {
        base::AutoLock lock(list_lock_);
        
        
        
        typename ObserversListMap::iterator it =
            observer_lists_.find(base::PlatformThread::CurrentId());
        if (it != observer_lists_.end() && it->second == context)
          observer_lists_.erase(it);
      }
      delete context;
    }
  }

  
  
  
  typedef std::map<base::PlatformThreadId, ObserverListContext*>
      ObserversListMap;

  mutable base::Lock list_lock_;  
  ObserversListMap observer_lists_;
  const NotificationType type_;

  DISALLOW_COPY_AND_ASSIGN(ObserverListThreadSafe);
};

#endif  
