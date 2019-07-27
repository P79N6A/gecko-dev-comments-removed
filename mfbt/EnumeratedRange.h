


















#ifndef mozilla_EnumeratedRange_h
#define mozilla_EnumeratedRange_h

#include "mozilla/IntegerTypeTraits.h"
#include "mozilla/ReverseIterator.h"

namespace mozilla {

namespace detail {

template<typename IntTypeT, typename EnumTypeT>
class EnumeratedIterator
{
public:
  typedef EnumTypeT ValueType;
  typedef typename MakeSigned<IntTypeT>::Type DifferenceType;

  template<typename EnumType>
  explicit EnumeratedIterator(EnumType aCurrent)
    : mCurrent(aCurrent) { }

  template<typename IntType, typename EnumType>
  EnumeratedIterator(const EnumeratedIterator<IntType, EnumType>& aOther)
    : mCurrent(aOther.mCurrent) { }

  EnumTypeT operator*() const { return mCurrent; }

  

  EnumeratedIterator& operator++()
  {
    mCurrent = EnumTypeT(IntTypeT(mCurrent) + IntTypeT(1));
    return *this;
  }
  EnumeratedIterator& operator--()
  {
    mCurrent = EnumTypeT(IntTypeT(mCurrent) - IntTypeT(1));
    return *this;
  }
  EnumeratedIterator operator++(int)
  {
    auto ret = *this;
    mCurrent = EnumTypeT(IntTypeT(mCurrent) + IntTypeT(1));
    return ret;
  }
  EnumeratedIterator operator--(int)
  {
    auto ret = *this;
    mCurrent = EnumTypeT(IntTypeT(mCurrent) - IntTypeT(1));
    return ret;
  }

  EnumeratedIterator operator+(DifferenceType aN) const
  {
    return EnumeratedIterator(EnumTypeT(IntTypeT(mCurrent) + aN));
  }
  EnumeratedIterator operator-(DifferenceType aN) const
  {
    return EnumeratedIterator(EnumTypeT(IntTypeT(mCurrent) - aN));
  }
  EnumeratedIterator& operator+=(DifferenceType aN)
  {
    mCurrent = EnumTypeT(IntTypeT(mCurrent) + aN);
    return *this;
  }
  EnumeratedIterator& operator-=(DifferenceType aN)
  {
    mCurrent = EnumTypeT(IntTypeT(mCurrent) - aN);
    return *this;
  }

  

  template<typename IntType, typename EnumType>
  friend bool operator==(const EnumeratedIterator<IntType, EnumType>& aIter1,
                         const EnumeratedIterator<IntType, EnumType>& aIter2);
  template<typename IntType, typename EnumType>
  friend bool operator!=(const EnumeratedIterator<IntType, EnumType>& aIter1,
                         const EnumeratedIterator<IntType, EnumType>& aIter2);
  template<typename IntType, typename EnumType>
  friend bool operator<(const EnumeratedIterator<IntType, EnumType>& aIter1,
                        const EnumeratedIterator<IntType, EnumType>& aIter2);
  template<typename IntType, typename EnumType>
  friend bool operator<=(const EnumeratedIterator<IntType, EnumType>& aIter1,
                         const EnumeratedIterator<IntType, EnumType>& aIter2);
  template<typename IntType, typename EnumType>
  friend bool operator>(const EnumeratedIterator<IntType, EnumType>& aIter1,
                        const EnumeratedIterator<IntType, EnumType>& aIter2);
  template<typename IntType, typename EnumType>
  friend bool operator>=(const EnumeratedIterator<IntType, EnumType>& aIter1,
                         const EnumeratedIterator<IntType, EnumType>& aIter2);

private:
  EnumTypeT mCurrent;
};

template<typename IntType, typename EnumType>
bool operator==(const EnumeratedIterator<IntType, EnumType>& aIter1,
                const EnumeratedIterator<IntType, EnumType>& aIter2)
{
  return aIter1.mCurrent == aIter2.mCurrent;
}

template<typename IntType, typename EnumType>
bool operator!=(const EnumeratedIterator<IntType, EnumType>& aIter1,
                const EnumeratedIterator<IntType, EnumType>& aIter2)
{
  return aIter1.mCurrent != aIter2.mCurrent;
}

template<typename IntType, typename EnumType>
bool operator<(const EnumeratedIterator<IntType, EnumType>& aIter1,
               const EnumeratedIterator<IntType, EnumType>& aIter2)
{
  return aIter1.mCurrent < aIter2.mCurrent;
}

template<typename IntType, typename EnumType>
bool operator<=(const EnumeratedIterator<IntType, EnumType>& aIter1,
                const EnumeratedIterator<IntType, EnumType>& aIter2)
{
  return aIter1.mCurrent <= aIter2.mCurrent;
}

template<typename IntType, typename EnumType>
bool operator>(const EnumeratedIterator<IntType, EnumType>& aIter1,
               const EnumeratedIterator<IntType, EnumType>& aIter2)
{
  return aIter1.mCurrent > aIter2.mCurrent;
}

template<typename IntType, typename EnumType>
bool operator>=(const EnumeratedIterator<IntType, EnumType>& aIter1,
                const EnumeratedIterator<IntType, EnumType>& aIter2)
{
  return aIter1.mCurrent >= aIter2.mCurrent;
}

template<typename IntTypeT, typename EnumTypeT>
class EnumeratedRange
{
public:
  typedef EnumeratedIterator<IntTypeT, EnumTypeT> iterator;
  typedef EnumeratedIterator<IntTypeT, EnumTypeT> const_iterator;
  typedef ReverseIterator<iterator> reverse_iterator;
  typedef ReverseIterator<const_iterator> const_reverse_iterator;

  template<typename EnumType>
  EnumeratedRange(EnumType aBegin, EnumType aEnd)
    : mBegin(aBegin), mEnd(aEnd) { }

  iterator begin() const { return iterator(mBegin); }
  const_iterator cbegin() const { return begin(); }
  iterator end() const { return iterator(mEnd); }
  const_iterator cend() const { return end(); }
  reverse_iterator rbegin() const { return reverse_iterator(mEnd); }
  const_reverse_iterator crbegin() const { return rbegin(); }
  reverse_iterator rend() const { return reverse_iterator(mBegin); }
  const_reverse_iterator crend() const { return rend(); }

private:
  EnumTypeT mBegin;
  EnumTypeT mEnd;
};

} 

#ifdef __GNUC__



#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wtype-limits"
#endif


template<typename IntType, typename EnumType>
inline detail::EnumeratedRange<IntType, EnumType>
MakeEnumeratedRange(EnumType aBegin, EnumType aEnd)
{
#ifdef DEBUG
  typedef typename MakeUnsigned<IntType>::Type UnsignedType;
#endif
  static_assert(sizeof(IntType) >= sizeof(EnumType),
                "IntType should be at least as big as EnumType!");
  MOZ_ASSERT(aBegin <= aEnd, "Cannot generate invalid, unbounded range!");
  MOZ_ASSERT_IF(aBegin < EnumType(0), IsSigned<IntType>::value);
  MOZ_ASSERT_IF(aBegin >= EnumType(0) && IsSigned<IntType>::value,
                UnsignedType(aEnd) <= UnsignedType(MaxValue<IntType>::value));
  return detail::EnumeratedRange<IntType, EnumType>(aBegin, aEnd);
}



template<typename IntType, typename EnumType>
inline detail::EnumeratedRange<IntType, EnumType>
MakeEnumeratedRange(EnumType aEnd)
{
  return MakeEnumeratedRange<IntType>(EnumType(0), aEnd);
}

#ifdef __GNUC__
#  pragma GCC diagnostic pop
#endif

} 

#endif 

