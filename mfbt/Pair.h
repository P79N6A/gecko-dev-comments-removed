







#ifndef mozilla_Pair_h
#define mozilla_Pair_h

#include "mozilla/Attributes.h"
#include "mozilla/Move.h"
#include "mozilla/TypeTraits.h"

namespace mozilla {

namespace detail {

enum StorageType { AsBase, AsMember };








template<typename A, typename B,
         detail::StorageType =
           IsEmpty<A>::value ? detail::AsBase : detail::AsMember,
         detail::StorageType =
           IsEmpty<B>::value && !IsBaseOf<A, B>::value && !IsBaseOf<B, A>::value
           ? detail::AsBase
           : detail::AsMember>
struct PairHelper;

template<typename A, typename B>
struct PairHelper<A, B, AsMember, AsMember>
{
  protected:
    template<typename AArg, typename BArg>
    PairHelper(AArg&& a, BArg&& b)
      : firstA(Forward<AArg>(a)),
        secondB(Forward<BArg>(b))
    {}

    A& first() { return firstA; }
    const A& first() const { return firstA; }
    B& second() { return secondB; }
    const B& second() const { return secondB; }

    void swap(PairHelper& other) {
      Swap(firstA, other.firstA);
      Swap(secondB, other.secondB);
    }

  private:
    A firstA;
    B secondB;
};

template<typename A, typename B>
struct PairHelper<A, B, AsMember, AsBase> : private B
{
  protected:
    template<typename AArg, typename BArg>
    PairHelper(AArg&& a, BArg&& b)
      : B(Forward<BArg>(b)),
        firstA(Forward<AArg>(a))
    {}

    A& first() { return firstA; }
    const A& first() const { return firstA; }
    B& second() { return *this; }
    const B& second() const { return *this; }

    void swap(PairHelper& other) {
      Swap(firstA, other.firstA);
      Swap(static_cast<B&>(*this), static_cast<B&>(other));
    }

  private:
    A firstA;
};

template<typename A, typename B>
struct PairHelper<A, B, AsBase, AsMember> : private A
{
  protected:
    template<typename AArg, typename BArg>
    PairHelper(AArg&& a, BArg&& b)
      : A(Forward<AArg>(a)),
        secondB(Forward<BArg>(b))
    {}

    A& first() { return *this; }
    const A& first() const { return *this; }
    B& second() { return secondB; }
    const B& second() const { return secondB; }

    void swap(PairHelper& other) {
      Swap(static_cast<A&>(*this), static_cast<A&>(other));
      Swap(secondB, other.secondB);
    }

  private:
    B secondB;
};

template<typename A, typename B>
struct PairHelper<A, B, AsBase, AsBase> : private A, private B
{
  protected:
    template<typename AArg, typename BArg>
    PairHelper(AArg&& a, BArg&& b)
      : A(Forward<AArg>(a)),
        B(Forward<BArg>(b))
    {}

    A& first() { return static_cast<A&>(*this); }
    const A& first() const { return static_cast<A&>(*this); }
    B& second() { return static_cast<B&>(*this); }
    const B& second() const { return static_cast<B&>(*this); }

    void swap(PairHelper& other) {
      Swap(static_cast<A&>(*this), static_cast<A&>(other));
      Swap(static_cast<B&>(*this), static_cast<B&>(other));
    }
};

} 














template<typename A, typename B>
struct Pair
  : private detail::PairHelper<A, B>
{
    typedef typename detail::PairHelper<A, B> Base;

  public:
    template<typename AArg, typename BArg>
    Pair(AArg&& a, BArg&& b)
      : Base(Forward<AArg>(a), Forward<BArg>(b))
    {}

    
    using Base::first;
    
    using Base::second;

    
    void swap(Pair& other) {
      Base::swap(other);
    }

  private:
    Pair(const Pair&) MOZ_DELETE;
};

template<typename A, class B>
void
Swap(Pair<A, B>& x, Pair<A, B>& y)
{
  x.swap(y);
}

} 

#endif 
