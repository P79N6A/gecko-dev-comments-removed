



#ifndef BASE_TASK_H_
#define BASE_TASK_H_

#include "base/non_thread_safe.h"
#include "base/revocable_store.h"
#include "base/tracked.h"
#include "base/tuple.h"






class Task : public tracked_objects::Tracked {
 public:
  Task() {}
  virtual ~Task() {}

  
  virtual void Run() = 0;
};

class CancelableTask : public Task {
 public:
  
  virtual void Cancel() = 0;
};












































template<class TaskType>
class ScopedTaskFactory : public RevocableStore {
 public:
  ScopedTaskFactory() { }

  
  inline TaskType* NewTask() {
    return new TaskWrapper(this);
  }

  class TaskWrapper : public TaskType, public NonThreadSafe {
   public:
    explicit TaskWrapper(RevocableStore* store) : revocable_(store) { }

    virtual void Run() {
      if (!revocable_.revoked())
        TaskType::Run();
    }

   private:
    Revocable revocable_;

    DISALLOW_EVIL_CONSTRUCTORS(TaskWrapper);
  };

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ScopedTaskFactory);
};




template<class T>
class ScopedRunnableMethodFactory : public RevocableStore {
 public:
  explicit ScopedRunnableMethodFactory(T* object) : object_(object) { }

  template <class Method>
  inline Task* NewRunnableMethod(Method method) {
    typedef typename ScopedTaskFactory<RunnableMethod<
        Method, Tuple0> >::TaskWrapper TaskWrapper;

    TaskWrapper* task = new TaskWrapper(this);
    task->Init(object_, method, MakeTuple());
    return task;
  }

  template <class Method, class A>
  inline Task* NewRunnableMethod(Method method, const A& a) {
    typedef typename ScopedTaskFactory<RunnableMethod<
        Method, Tuple1<A> > >::TaskWrapper TaskWrapper;

    TaskWrapper* task = new TaskWrapper(this);
    task->Init(object_, method, MakeTuple(a));
    return task;
  }

  template <class Method, class A, class B>
  inline Task* NewRunnableMethod(Method method, const A& a, const B& b) {
    typedef typename ScopedTaskFactory<RunnableMethod<
        Method, Tuple2<A, B> > >::TaskWrapper TaskWrapper;

    TaskWrapper* task = new TaskWrapper(this);
    task->Init(object_, method, MakeTuple(a, b));
    return task;
  }

  template <class Method, class A, class B, class C>
  inline Task* NewRunnableMethod(Method method,
                                 const A& a,
                                 const B& b,
                                 const C& c) {
    typedef typename ScopedTaskFactory<RunnableMethod<
        Method, Tuple3<A, B, C> > >::TaskWrapper TaskWrapper;

    TaskWrapper* task = new TaskWrapper(this);
    task->Init(object_, method, MakeTuple(a, b, c));
    return task;
  }

  template <class Method, class A, class B, class C, class D>
  inline Task* NewRunnableMethod(Method method,
                                 const A& a,
                                 const B& b,
                                 const C& c,
                                 const D& d) {
    typedef typename ScopedTaskFactory<RunnableMethod<
        Method, Tuple4<A, B, C, D> > >::TaskWrapper TaskWrapper;

    TaskWrapper* task = new TaskWrapper(this);
    task->Init(object_, method, MakeTuple(a, b, c, d));
    return task;
  }

  template <class Method, class A, class B, class C, class D, class E>
  inline Task* NewRunnableMethod(Method method,
                                 const A& a,
                                 const B& b,
                                 const C& c,
                                 const D& d,
                                 const E& e) {
    typedef typename ScopedTaskFactory<RunnableMethod<
        Method, Tuple5<A, B, C, D, E> > >::TaskWrapper TaskWrapper;

    TaskWrapper* task = new TaskWrapper(this);
    task->Init(object_, method, MakeTuple(a, b, c, d, e));
    return task;
  }

 protected:
  template <class Method, class Params>
  class RunnableMethod : public Task {
   public:
    RunnableMethod() { }

    void Init(T* obj, Method meth, const Params& params) {
      obj_ = obj;
      meth_ = meth;
      params_ = params;
    }

    virtual void Run() { DispatchToMethod(obj_, meth_, params_); }

   private:
    T* obj_;
    Method meth_;
    Params params_;

    DISALLOW_EVIL_CONSTRUCTORS(RunnableMethod);
  };

 private:
  T* object_;

  DISALLOW_EVIL_CONSTRUCTORS(ScopedRunnableMethodFactory);
};




template<class T>
class DeleteTask : public CancelableTask {
 public:
  explicit DeleteTask(T* obj) : obj_(obj) {
  }
  virtual void Run() {
    delete obj_;
  }
  virtual void Cancel() {
    obj_ = NULL;
  }
 private:
  T* obj_;
};


template<class T>
class ReleaseTask : public CancelableTask {
 public:
  explicit ReleaseTask(T* obj) : obj_(obj) {
  }
  virtual void Run() {
    if (obj_)
      obj_->Release();
  }
  virtual void Cancel() {
    obj_ = NULL;
  }
 private:
  T* obj_;
};










template <class T>
struct RunnableMethodTraits {
  static void RetainCallee(T* obj) {
    obj->AddRef();
  }
  static void ReleaseCallee(T* obj) {
    obj->Release();
  }
};



























template <class T, class Method, class Params>
class RunnableMethod : public CancelableTask,
                       public RunnableMethodTraits<T> {
 public:
  RunnableMethod(T* obj, Method meth, const Params& params)
      : obj_(obj), meth_(meth), params_(params) {
    this->RetainCallee(obj_);
  }
  ~RunnableMethod() {
    ReleaseCallee();
  }

  virtual void Run() {
    if (obj_)
      DispatchToMethod(obj_, meth_, params_);
  }

  virtual void Cancel() {
    ReleaseCallee();
  }

 private:
  void ReleaseCallee() {
    if (obj_) {
      RunnableMethodTraits<T>::ReleaseCallee(obj_);
      obj_ = NULL;
    }
  }

  T* obj_;
  Method meth_;
  Params params_;
};

template <class T, class Method>
inline CancelableTask* NewRunnableMethod(T* object, Method method) {
  return new RunnableMethod<T, Method, Tuple0>(object, method, MakeTuple());
}

template <class T, class Method, class A>
inline CancelableTask* NewRunnableMethod(T* object, Method method, const A& a) {
  return new RunnableMethod<T, Method, Tuple1<A> >(object,
                                                   method,
                                                   MakeTuple(a));
}

template <class T, class Method, class A, class B>
inline CancelableTask* NewRunnableMethod(T* object, Method method,
const A& a, const B& b) {
  return new RunnableMethod<T, Method, Tuple2<A, B> >(object, method,
                                                      MakeTuple(a, b));
}

template <class T, class Method, class A, class B, class C>
inline CancelableTask* NewRunnableMethod(T* object, Method method,
                                          const A& a, const B& b, const C& c) {
  return new RunnableMethod<T, Method, Tuple3<A, B, C> >(object, method,
                                                         MakeTuple(a, b, c));
}

template <class T, class Method, class A, class B, class C, class D>
inline CancelableTask* NewRunnableMethod(T* object, Method method,
                                          const A& a, const B& b,
                                          const C& c, const D& d) {
  return new RunnableMethod<T, Method, Tuple4<A, B, C, D> >(object, method,
                                                            MakeTuple(a, b,
                                                                      c, d));
}

template <class T, class Method, class A, class B, class C, class D, class E>
inline CancelableTask* NewRunnableMethod(T* object, Method method,
                                          const A& a, const B& b,
                                          const C& c, const D& d, const E& e) {
  return new RunnableMethod<T,
                            Method,
                            Tuple5<A, B, C, D, E> >(object,
                                                    method,
                                                    MakeTuple(a, b, c, d, e));
}

template <class T, class Method, class A, class B, class C, class D, class E,
          class F>
inline CancelableTask* NewRunnableMethod(T* object, Method method,
                                          const A& a, const B& b,
                                          const C& c, const D& d, const E& e,
                                          const F& f) {
  return new RunnableMethod<T,
                            Method,
                            Tuple6<A, B, C, D, E, F> >(object,
                                                       method,
                                                       MakeTuple(a, b, c, d, e,
                                                                 f));
}

template <class T, class Method, class A, class B, class C, class D, class E,
          class F, class G>
inline CancelableTask* NewRunnableMethod(T* object, Method method,
                                         const A& a, const B& b,
                                         const C& c, const D& d, const E& e,
                                         const F& f, const G& g) {
  return new RunnableMethod<T,
                            Method,
                            Tuple7<A, B, C, D, E, F, G> >(object,
                                                          method,
                                                          MakeTuple(a, b, c, d,
                                                                    e, f, g));
}



template <class Function, class Params>
class RunnableFunction : public CancelableTask {
 public:
  RunnableFunction(Function function, const Params& params)
      : function_(function), params_(params) {
  }

  ~RunnableFunction() {
  }

  virtual void Run() {
    if (function_)
      DispatchToFunction(function_, params_);
  }

  virtual void Cancel() {
    function_ = NULL;
  }

 private:
  Function function_;
  Params params_;
};

template <class Function>
inline CancelableTask* NewRunnableFunction(Function function) {
  return new RunnableFunction<Function, Tuple0>(function, MakeTuple());
}

template <class Function, class A>
inline CancelableTask* NewRunnableFunction(Function function, const A& a) {
  return new RunnableFunction<Function, Tuple1<A> >(function, MakeTuple(a));
}

template <class Function, class A, class B>
inline CancelableTask* NewRunnableFunction(Function function,
                                           const A& a, const B& b) {
  return new RunnableFunction<Function, Tuple2<A, B> >(function,
                                                       MakeTuple(a, b));
}

template <class Function, class A, class B, class C>
inline CancelableTask* NewRunnableFunction(Function function,
                                           const A& a, const B& b,
                                           const C& c) {
  return new RunnableFunction<Function, Tuple3<A, B, C> >(function,
                                                          MakeTuple(a, b, c));
}

template <class Function, class A, class B, class C, class D>
inline CancelableTask* NewRunnableFunction(Function function,
                                           const A& a, const B& b,
                                           const C& c, const D& d) {
  return new RunnableFunction<Function, Tuple4<A, B, C, D> >(function,
                                                             MakeTuple(a, b,
                                                                       c, d));
}

template <class Function, class A, class B, class C, class D, class E>
inline CancelableTask* NewRunnableFunction(Function function,
                                           const A& a, const B& b,
                                           const C& c, const D& d,
                                           const E& e) {
  return new RunnableFunction<Function, Tuple5<A, B, C, D, E> >(function,
                                                                MakeTuple(a, b,
                                                                          c, d,
                                                                          e));
}






































template <class T, typename Method>
class CallbackStorage {
 public:
  CallbackStorage(T* obj, Method meth) : obj_(obj), meth_(meth) {
  }

 protected:
  T* obj_;
  Method meth_;
};



template <typename Params>
class CallbackRunner {
 public:
  typedef Params TupleType;

  virtual ~CallbackRunner() {}
  virtual void RunWithParams(const Params& params) = 0;

  
  inline void Run() {
    RunWithParams(Tuple0());
  }

  template <typename Arg1>
  inline void Run(const Arg1& a) {
    RunWithParams(Params(a));
  }

  template <typename Arg1, typename Arg2>
  inline void Run(const Arg1& a, const Arg2& b) {
    RunWithParams(Params(a, b));
  }

  template <typename Arg1, typename Arg2, typename Arg3>
  inline void Run(const Arg1& a, const Arg2& b, const Arg3& c) {
    RunWithParams(Params(a, b, c));
  }

  template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
  inline void Run(const Arg1& a, const Arg2& b, const Arg3& c, const Arg4& d) {
    RunWithParams(Params(a, b, c, d));
  }

  template <typename Arg1, typename Arg2, typename Arg3,
            typename Arg4, typename Arg5>
  inline void Run(const Arg1& a, const Arg2& b, const Arg3& c,
                  const Arg4& d, const Arg5& e) {
    RunWithParams(Params(a, b, c, d, e));
  }
};

template <class T, typename Method, typename Params>
class CallbackImpl : public CallbackStorage<T, Method>,
                     public CallbackRunner<Params> {
 public:
  CallbackImpl(T* obj, Method meth) : CallbackStorage<T, Method>(obj, meth) {
  }
  virtual void RunWithParams(const Params& params) {
    
    
    DispatchToMethod(this->obj_, this->meth_, params);
  }
};


struct Callback0 {
  typedef CallbackRunner<Tuple0> Type;
};

template <class T>
typename Callback0::Type* NewCallback(T* object, void (T::*method)()) {
  return new CallbackImpl<T, void (T::*)(), Tuple0 >(object, method);
}


template <typename Arg1>
struct Callback1 {
  typedef CallbackRunner<Tuple1<Arg1> > Type;
};

template <class T, typename Arg1>
typename Callback1<Arg1>::Type* NewCallback(T* object,
                                            void (T::*method)(Arg1)) {
  return new CallbackImpl<T, void (T::*)(Arg1), Tuple1<Arg1> >(object, method);
}


template <typename Arg1, typename Arg2>
struct Callback2 {
  typedef CallbackRunner<Tuple2<Arg1, Arg2> > Type;
};

template <class T, typename Arg1, typename Arg2>
typename Callback2<Arg1, Arg2>::Type* NewCallback(
    T* object,
    void (T::*method)(Arg1, Arg2)) {
  return new CallbackImpl<T, void (T::*)(Arg1, Arg2),
      Tuple2<Arg1, Arg2> >(object, method);
}


template <typename Arg1, typename Arg2, typename Arg3>
struct Callback3 {
  typedef CallbackRunner<Tuple3<Arg1, Arg2, Arg3> > Type;
};

template <class T, typename Arg1, typename Arg2, typename Arg3>
typename Callback3<Arg1, Arg2, Arg3>::Type* NewCallback(
    T* object,
    void (T::*method)(Arg1, Arg2, Arg3)) {
  return new CallbackImpl<T,  void (T::*)(Arg1, Arg2, Arg3),
      Tuple3<Arg1, Arg2, Arg3> >(object, method);
}


template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
struct Callback4 {
  typedef CallbackRunner<Tuple4<Arg1, Arg2, Arg3, Arg4> > Type;
};

template <class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4>
typename Callback4<Arg1, Arg2, Arg3, Arg4>::Type* NewCallback(
    T* object,
    void (T::*method)(Arg1, Arg2, Arg3, Arg4)) {
  return new CallbackImpl<T, void (T::*)(Arg1, Arg2, Arg3, Arg4),
      Tuple4<Arg1, Arg2, Arg3, Arg4> >(object, method);
}


template <typename Arg1, typename Arg2, typename Arg3,
          typename Arg4, typename Arg5>
struct Callback5 {
  typedef CallbackRunner<Tuple5<Arg1, Arg2, Arg3, Arg4, Arg5> > Type;
};

template <class T, typename Arg1, typename Arg2,
          typename Arg3, typename Arg4, typename Arg5>
typename Callback5<Arg1, Arg2, Arg3, Arg4, Arg5>::Type* NewCallback(
    T* object,
    void (T::*method)(Arg1, Arg2, Arg3, Arg4, Arg5)) {
  return new CallbackImpl<T, void (T::*)(Arg1, Arg2, Arg3, Arg4, Arg5),
      Tuple5<Arg1, Arg2, Arg3, Arg4, Arg5> >(object, method);
}



template <class T, class Method, class Params>
class UnboundMethod {
 public:
  UnboundMethod(Method m, Params p) : m_(m), p_(p) {}
  void Run(T* obj) const {
    DispatchToMethod(obj, m_, p_);
  }
 private:
  Method m_;
  Params p_;
};

#endif  
