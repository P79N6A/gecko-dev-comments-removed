







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
  PairHelper(AArg&& aA, BArg&& aB)
    : mFirstA(Forward<AArg>(aA)),
      mSecondB(Forward<BArg>(aB))
  {}

  A& first() { return mFirstA; }
  const A& first() const { return mFirstA; }
  B& second() { return mSecondB; }
  const B& second() const { return mSecondB; }

  void swap(PairHelper& aOther)
  {
    Swap(mFirstA, aOther.mFirstA);
    Swap(mSecondB, aOther.mSecondB);
  }

private:
  A mFirstA;
  B mSecondB;
};

template<typename A, typename B>
struct PairHelper<A, B, AsMember, AsBase> : private B
{
protected:
  template<typename AArg, typename BArg>
  PairHelper(AArg&& aA, BArg&& aB)
    : B(Forward<BArg>(aB)),
      mFirstA(Forward<AArg>(aA))
  {}

  A& first() { return mFirstA; }
  const A& first() const { return mFirstA; }
  B& second() { return *this; }
  const B& second() const { return *this; }

  void swap(PairHelper& aOther)
  {
    Swap(mFirstA, aOther.mFirstA);
    Swap(static_cast<B&>(*this), static_cast<B&>(aOther));
  }

private:
  A mFirstA;
};

template<typename A, typename B>
struct PairHelper<A, B, AsBase, AsMember> : private A
{
protected:
  template<typename AArg, typename BArg>
  PairHelper(AArg&& aA, BArg&& aB)
    : A(Forward<AArg>(aA)),
      mSecondB(Forward<BArg>(aB))
  {}

  A& first() { return *this; }
  const A& first() const { return *this; }
  B& second() { return mSecondB; }
  const B& second() const { return mSecondB; }

  void swap(PairHelper& aOther)
  {
    Swap(static_cast<A&>(*this), static_cast<A&>(aOther));
    Swap(mSecondB, aOther.mSecondB);
  }

private:
  B mSecondB;
};

template<typename A, typename B>
struct PairHelper<A, B, AsBase, AsBase> : private A, private B
{
protected:
  template<typename AArg, typename BArg>
  PairHelper(AArg&& aA, BArg&& aB)
    : A(Forward<AArg>(aA)),
      B(Forward<BArg>(aB))
  {}

  A& first() { return static_cast<A&>(*this); }
  const A& first() const { return static_cast<A&>(*this); }
  B& second() { return static_cast<B&>(*this); }
  const B& second() const { return static_cast<B&>(*this); }

  void swap(PairHelper& aOther)
  {
    Swap(static_cast<A&>(*this), static_cast<A&>(aOther));
    Swap(static_cast<B&>(*this), static_cast<B&>(aOther));
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
  Pair(AArg&& aA, BArg&& aB)
    : Base(Forward<AArg>(aA), Forward<BArg>(aB))
  {}

  
  using Base::first;
  
  using Base::second;

  
  void swap(Pair& aOther) { Base::swap(aOther); }

private:
  Pair(const Pair&) MOZ_DELETE;
};

template<typename A, class B>
void
Swap(Pair<A, B>& aX, Pair<A, B>& aY)
{
  aX.swap(aY);
}

} 

#endif 
