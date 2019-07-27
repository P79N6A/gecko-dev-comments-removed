







#ifndef mozilla_UniquePtr_h
#define mozilla_UniquePtr_h

#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/Compiler.h"
#include "mozilla/Move.h"
#include "mozilla/NullPtr.h"
#include "mozilla/Pair.h"
#include "mozilla/TypeTraits.h"

namespace mozilla {

template<typename T> class DefaultDelete;
template<typename T, class D = DefaultDelete<T>> class UniquePtr;

} 

namespace mozilla {






























































































































template<typename T, class D>
class UniquePtr
{
  public:
    typedef T* Pointer;
    typedef T ElementType;
    typedef D DeleterType;

  private:
    Pair<Pointer, DeleterType> tuple;

    Pointer& ptr() { return tuple.first(); }
    const Pointer& ptr() const { return tuple.first(); }

    DeleterType& del() { return tuple.second(); }
    const DeleterType& del() const { return tuple.second(); }

  public:
    


    MOZ_CONSTEXPR UniquePtr()
      : tuple(static_cast<Pointer>(nullptr), DeleterType())
    {
      static_assert(!IsPointer<D>::value, "must provide a deleter instance");
      static_assert(!IsReference<D>::value, "must provide a deleter instance");
    }

    


    explicit UniquePtr(Pointer p)
      : tuple(p, DeleterType())
    {
      static_assert(!IsPointer<D>::value, "must provide a deleter instance");
      static_assert(!IsReference<D>::value, "must provide a deleter instance");
    }

    UniquePtr(Pointer p,
              typename Conditional<IsReference<D>::value,
                                   D,
                                   const D&>::Type d1)
      : tuple(p, d1)
    {}

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    UniquePtr(Pointer p,
              typename RemoveReference<D>::Type&& d2)
      : tuple(p, Move(d2))
    {
      static_assert(!IsReference<D>::value,
                    "rvalue deleter can't be stored by reference");
    }

    UniquePtr(UniquePtr&& other)
      : tuple(other.release(), Forward<DeleterType>(other.getDeleter()))
    {}

    template<typename N>
    UniquePtr(N,
              typename EnableIf<IsNullPointer<N>::value, int>::Type dummy = 0)
      : tuple(static_cast<Pointer>(nullptr), DeleterType())
    {
      static_assert(!IsPointer<D>::value, "must provide a deleter instance");
      static_assert(!IsReference<D>::value, "must provide a deleter instance");
    }

    template<typename U, class E>
    UniquePtr(UniquePtr<U, E>&& other,
              typename EnableIf<IsConvertible<typename UniquePtr<U, E>::Pointer,
                                              Pointer>::value &&
                                !IsArray<U>::value &&
                                (IsReference<D>::value
                                 ? IsSame<D, E>::value
                                 : IsConvertible<E, D>::value),
                                int>::Type dummy = 0)
      : tuple(other.release(), Forward<E>(other.getDeleter()))
    {
    }

    ~UniquePtr() {
      reset(nullptr);
    }

    UniquePtr& operator=(UniquePtr&& other) {
      reset(other.release());
      getDeleter() = Forward<DeleterType>(other.getDeleter());
      return *this;
    }

    template<typename U, typename E>
    UniquePtr& operator=(UniquePtr<U, E>&& other)
    {
      static_assert(IsConvertible<typename UniquePtr<U, E>::Pointer, Pointer>::value,
                    "incompatible UniquePtr pointees");
      static_assert(!IsArray<U>::value,
                    "can't assign from UniquePtr holding an array");

      reset(other.release());
      getDeleter() = Forward<E>(other.getDeleter());
      return *this;
    }

    UniquePtr& operator=(NullptrT n) {
      MOZ_ASSERT(n == nullptr);
      reset(nullptr);
      return *this;
    }

    T& operator*() const { return *get(); }
    Pointer operator->() const {
      MOZ_ASSERT(get(), "dereferencing a UniquePtr containing nullptr");
      return get();
    }

    Pointer get() const { return ptr(); }

    DeleterType& getDeleter() { return del(); }
    const DeleterType& getDeleter() const { return del(); }

  private:
    typedef void (UniquePtr::* ConvertibleToBool)(double, char);
    void nonNull(double, char) {}

  public:
    operator ConvertibleToBool() const {
      return get() != nullptr ? &UniquePtr::nonNull : nullptr;
    }

    Pointer release() {
      Pointer p = ptr();
      ptr() = nullptr;
      return p;
    }

    void reset(Pointer p = Pointer()) {
      Pointer old = ptr();
      ptr() = p;
      if (old != nullptr) {
        getDeleter()(old);
      }
    }

    void swap(UniquePtr& other) {
      tuple.swap(other.tuple);
    }

  private:
    UniquePtr(const UniquePtr& other) MOZ_DELETE; 
    void operator=(const UniquePtr& other) MOZ_DELETE; 
};





template<typename T, class D>
class UniquePtr<T[], D>
{
  public:
    typedef T* Pointer;
    typedef T ElementType;
    typedef D DeleterType;

  private:
    Pair<Pointer, DeleterType> tuple;

  public:
    


    MOZ_CONSTEXPR UniquePtr()
      : tuple(static_cast<Pointer>(nullptr), DeleterType())
    {
      static_assert(!IsPointer<D>::value, "must provide a deleter instance");
      static_assert(!IsReference<D>::value, "must provide a deleter instance");
    }

    


    explicit UniquePtr(Pointer p)
      : tuple(p, DeleterType())
    {
      static_assert(!IsPointer<D>::value, "must provide a deleter instance");
      static_assert(!IsReference<D>::value, "must provide a deleter instance");
    }

  private:
    
    
    
    
    
    template<typename U>
    UniquePtr(U&& u,
              typename EnableIf<IsPointer<U>::value &&
                                IsConvertible<U, Pointer>::value,
                                int>::Type dummy = 0)
    MOZ_DELETE;

  public:
    UniquePtr(Pointer p,
              typename Conditional<IsReference<D>::value,
                                   D,
                                   const D&>::Type d1)
      : tuple(p, d1)
    {}

    
    
    
    
    UniquePtr(Pointer p,
              typename RemoveReference<D>::Type&& d2)
      : tuple(p, Move(d2))
    {
      static_assert(!IsReference<D>::value,
                    "rvalue deleter can't be stored by reference");
    }

  private:
    
    template<typename U, typename V>
    UniquePtr(U&& u, V&& v,
              typename EnableIf<IsPointer<U>::value &&
                                IsConvertible<U, Pointer>::value,
                                int>::Type dummy = 0)
    MOZ_DELETE;

  public:
    UniquePtr(UniquePtr&& other)
      : tuple(other.release(), Forward<DeleterType>(other.getDeleter()))
    {}

    template<typename N>
    UniquePtr(N,
              typename EnableIf<IsNullPointer<N>::value, int>::Type dummy = 0)
      : tuple(static_cast<Pointer>(nullptr), DeleterType())
    {
      static_assert(!IsPointer<D>::value, "must provide a deleter instance");
      static_assert(!IsReference<D>::value, "must provide a deleter instance");
    }

    ~UniquePtr() {
      reset(nullptr);
    }

    UniquePtr& operator=(UniquePtr&& other) {
      reset(other.release());
      getDeleter() = Forward<DeleterType>(other.getDeleter());
      return *this;
    }

    UniquePtr& operator=(NullptrT) {
      reset();
      return *this;
    }

    T& operator[](decltype(sizeof(int)) i) const { return get()[i]; }
    Pointer get() const { return tuple.first(); }

    DeleterType& getDeleter() { return tuple.second(); }
    const DeleterType& getDeleter() const { return tuple.second(); }

  private:
    typedef void (UniquePtr::* ConvertibleToBool)(double, char);
    void nonNull(double, char) {}

  public:
    operator ConvertibleToBool() const {
      return get() != nullptr ? &UniquePtr::nonNull : nullptr;
    }

    Pointer release() {
      Pointer p = tuple.first();
      tuple.first() = nullptr;
      return p;
    }

    void reset(Pointer p = Pointer()) {
      Pointer old = tuple.first();
      tuple.first() = p;
      if (old != nullptr) {
        tuple.second()(old);
      }
    }

  private:
    
    
    
    template<typename U>
    void reset(U,
               typename EnableIf<!IsNullPointer<U>::value &&
                                 !IsSame<U,
                                         Conditional<(sizeof(int) == sizeof(void*)),
                                                     int,
                                                     long>::Type>::value,
                                 int>::Type dummy = 0)
    MOZ_DELETE;

  public:
    void swap(UniquePtr& other) {
      tuple.swap(other.tuple);
    }

  private:
    UniquePtr(const UniquePtr& other) MOZ_DELETE; 
    void operator=(const UniquePtr& other) MOZ_DELETE; 
};


template<typename T>
class DefaultDelete
{
  public:
    MOZ_CONSTEXPR DefaultDelete() {}

    template<typename U>
    DefaultDelete(const DefaultDelete<U>& other,
                  typename EnableIf<mozilla::IsConvertible<U*, T*>::value,
                                    int>::Type dummy = 0)
    {}

    void operator()(T* ptr) const {
      static_assert(sizeof(T) > 0, "T must be complete");
      delete ptr;
    }
};


template<typename T>
class DefaultDelete<T[]>
{
  public:
    MOZ_CONSTEXPR DefaultDelete() {}

    void operator()(T* ptr) const {
      static_assert(sizeof(T) > 0, "T must be complete");
      delete[] ptr;
    }

  private:
    template<typename U>
    void operator()(U* ptr) const MOZ_DELETE;
};

template<typename T, class D>
void
Swap(UniquePtr<T, D>& x, UniquePtr<T, D>& y)
{
  x.swap(y);
}

template<typename T, class D, typename U, class E>
bool
operator==(const UniquePtr<T, D>& x, const UniquePtr<U, E>& y)
{
  return x.get() == y.get();
}

template<typename T, class D, typename U, class E>
bool
operator!=(const UniquePtr<T, D>& x, const UniquePtr<U, E>& y)
{
  return x.get() != y.get();
}

template<typename T, class D>
bool
operator==(const UniquePtr<T, D>& x, NullptrT n)
{
  MOZ_ASSERT(n == nullptr);
  return !x;
}

template<typename T, class D>
bool
operator==(NullptrT n, const UniquePtr<T, D>& x)
{
  MOZ_ASSERT(n == nullptr);
  return !x;
}

template<typename T, class D>
bool
operator!=(const UniquePtr<T, D>& x, NullptrT n)
{
  MOZ_ASSERT(n == nullptr);
  return bool(x);
}

template<typename T, class D>
bool
operator!=(NullptrT n, const UniquePtr<T, D>& x)
{
  MOZ_ASSERT(n == nullptr);
  return bool(x);
}



namespace detail {

template<typename T>
struct UniqueSelector
{
    typedef UniquePtr<T> SingleObject;
};

template<typename T>
struct UniqueSelector<T[]>
{
    typedef UniquePtr<T[]> UnknownBound;
};

template<typename T, decltype(sizeof(int)) N>
struct UniqueSelector<T[N]>
{
    typedef UniquePtr<T[N]> KnownBound;
};

} 
































































template<typename T>
typename detail::UniqueSelector<T>::SingleObject
MakeUnique()
{
  return UniquePtr<T>(new T());
}

template<typename T, typename A1>
typename detail::UniqueSelector<T>::SingleObject
MakeUnique(A1&& a1)
{
  return UniquePtr<T>(new T(Forward<A1>(a1)));
}

template<typename T, typename A1, typename A2>
typename detail::UniqueSelector<T>::SingleObject
MakeUnique(A1&& a1, A2&& a2)
{
  return UniquePtr<T>(new T(Forward<A1>(a1), Forward<A2>(a2)));
}

template<typename T, typename A1, typename A2, typename A3>
typename detail::UniqueSelector<T>::SingleObject
MakeUnique(A1&& a1, A2&& a2, A3&& a3)
{
  return UniquePtr<T>(new T(Forward<A1>(a1), Forward<A2>(a2), Forward<A3>(a3)));
}

template<typename T, typename A1, typename A2, typename A3, typename A4>
typename detail::UniqueSelector<T>::SingleObject
MakeUnique(A1&& a1, A2&& a2, A3&& a3, A4&& a4)
{
  return UniquePtr<T>(new T(Forward<A1>(a1), Forward<A2>(a2), Forward<A3>(a3),
                            Forward<A4>(a4)));
}

template<typename T, typename A1, typename A2, typename A3, typename A4, typename A5>
typename detail::UniqueSelector<T>::SingleObject
MakeUnique(A1&& a1, A2&& a2, A3&& a3, A4&& a4, A5&& a5)
{
  return UniquePtr<T>(new T(Forward<A1>(a1), Forward<A2>(a2), Forward<A3>(a3), Forward<A4>(a4), Forward<A5>(a5)));
}

template<typename T>
typename detail::UniqueSelector<T>::UnknownBound
MakeUnique(decltype(sizeof(int)) n)
{
  typedef typename RemoveExtent<T>::Type ArrayType;
  return UniquePtr<T>(new ArrayType[n]());
}

template<typename T>
typename detail::UniqueSelector<T>::KnownBound
MakeUnique() MOZ_DELETE;

template<typename T, typename A1>
typename detail::UniqueSelector<T>::KnownBound
MakeUnique(A1&& a1) MOZ_DELETE;

template<typename T, typename A1, typename A2>
typename detail::UniqueSelector<T>::KnownBound
MakeUnique(A1&& a1, A2&& a2) MOZ_DELETE;

template<typename T, typename A1, typename A2, typename A3>
typename detail::UniqueSelector<T>::KnownBound
MakeUnique(A1&& a1, A2&& a2, A3&& a3) MOZ_DELETE;

template<typename T, typename A1, typename A2, typename A3, typename A4>
typename detail::UniqueSelector<T>::KnownBound
MakeUnique(A1&& a1, A2&& a2, A3&& a3, A4&& a4) MOZ_DELETE;

template<typename T, typename A1, typename A2, typename A3, typename A4, typename A5>
typename detail::UniqueSelector<T>::KnownBound
MakeUnique(A1&& a1, A2&& a2, A3&& a3, A4&& a4, A5&& a5) MOZ_DELETE;

} 

#endif 
