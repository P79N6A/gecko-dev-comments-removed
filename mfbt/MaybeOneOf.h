





#ifndef mozilla_MaybeOneOf_h
#define mozilla_MaybeOneOf_h

#include "mozilla/Alignment.h"
#include "mozilla/Assertions.h"
#include "mozilla/Move.h"
#include "mozilla/TemplateLib.h"

#include <new>    

namespace mozilla {










template<class T1, class T2>
class MaybeOneOf
{
  AlignedStorage<tl::Max<sizeof(T1), sizeof(T2)>::value> storage;

  enum State { None, SomeT1, SomeT2 } state;
  template <class T, class Ignored = void> struct Type2State {};

  template <class T>
  T& as()
  {
    MOZ_ASSERT(state == Type2State<T>::result);
    return *(T*)storage.addr();
  }

  template <class T>
  const T& as() const
  {
    MOZ_ASSERT(state == Type2State<T>::result);
    return *(T*)storage.addr();
  }

public:
  MaybeOneOf() : state(None) {}
  ~MaybeOneOf() { destroyIfConstructed(); }

  MaybeOneOf(MaybeOneOf&& rhs)
    : state(None)
  {
    if (!rhs.empty()) {
      if (rhs.constructed<T1>()) {
        construct<T1>(Move(rhs.as<T1>()));
        rhs.as<T1>().~T1();
      } else {
        construct<T2>(Move(rhs.as<T2>()));
        rhs.as<T2>().~T2();
      }
      rhs.state = None;
    }
  }

  MaybeOneOf &operator=(MaybeOneOf&& rhs)
  {
    MOZ_ASSERT(this != &rhs, "Self-move is prohibited");
    this->~MaybeOneOf();
    new(this) MaybeOneOf(Move(rhs));
    return *this;
  }

  bool empty() const { return state == None; }

  template <class T>
  bool constructed() const { return state == Type2State<T>::result; }

  template <class T, class... Args>
  void construct(Args&&... aArgs)
  {
    MOZ_ASSERT(state == None);
    state = Type2State<T>::result;
    ::new (storage.addr()) T(Forward<Args>(aArgs)...);
  }

  template <class T>
  T& ref()
  {
    return as<T>();
  }

  template <class T>
  const T& ref() const
  {
    return as<T>();
  }

  void destroy()
  {
    MOZ_ASSERT(state == SomeT1 || state == SomeT2);
    if (state == SomeT1) {
      as<T1>().~T1();
    } else if (state == SomeT2) {
      as<T2>().~T2();
    }
    state = None;
  }

  void destroyIfConstructed()
  {
    if (!empty()) {
      destroy();
    }
  }

private:
  MaybeOneOf(const MaybeOneOf& aOther) = delete;
  const MaybeOneOf& operator=(const MaybeOneOf& aOther) = delete;
};

template <class T1, class T2>
template <class Ignored>
struct MaybeOneOf<T1, T2>::Type2State<T1, Ignored>
{
  typedef MaybeOneOf<T1, T2> Enclosing;
  static const typename Enclosing::State result = Enclosing::SomeT1;
};

template <class T1, class T2>
template <class Ignored>
struct MaybeOneOf<T1, T2>::Type2State<T2, Ignored>
{
  typedef MaybeOneOf<T1, T2> Enclosing;
  static const typename Enclosing::State result = Enclosing::SomeT2;
};

} 

#endif 
