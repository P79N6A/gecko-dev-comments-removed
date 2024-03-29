









#ifndef mozilla_ReverseIterator_h
#define mozilla_ReverseIterator_h

#include "mozilla/Attributes.h"
#include "mozilla/TypeTraits.h"

namespace mozilla {

template<typename IteratorT>
class ReverseIterator
{
public:
  template<typename Iterator>
  explicit ReverseIterator(Iterator aIter)
    : mCurrent(aIter) { }

  template<typename Iterator>
  MOZ_IMPLICIT ReverseIterator(const ReverseIterator<Iterator>& aOther)
    : mCurrent(aOther.mCurrent) { }

  decltype(*DeclVal<IteratorT>()) operator*() const
  {
    IteratorT tmp = mCurrent;
    return *--tmp;
  }

  

  ReverseIterator& operator++() { --mCurrent; return *this; }
  ReverseIterator& operator--() { ++mCurrent; return *this; }
  ReverseIterator operator++(int) { auto ret = *this; mCurrent--; return ret; }
  ReverseIterator operator--(int) { auto ret = *this; mCurrent++; return ret; }

  

  template<typename Iterator1, typename Iterator2>
  friend bool operator==(const ReverseIterator<Iterator1>& aIter1,
                         const ReverseIterator<Iterator2>& aIter2);
  template<typename Iterator1, typename Iterator2>
  friend bool operator!=(const ReverseIterator<Iterator1>& aIter1,
                         const ReverseIterator<Iterator2>& aIter2);
  template<typename Iterator1, typename Iterator2>
  friend bool operator<(const ReverseIterator<Iterator1>& aIter1,
                        const ReverseIterator<Iterator2>& aIter2);
  template<typename Iterator1, typename Iterator2>
  friend bool operator<=(const ReverseIterator<Iterator1>& aIter1,
                         const ReverseIterator<Iterator2>& aIter2);
  template<typename Iterator1, typename Iterator2>
  friend bool operator>(const ReverseIterator<Iterator1>& aIter1,
                        const ReverseIterator<Iterator2>& aIter2);
  template<typename Iterator1, typename Iterator2>
  friend bool operator>=(const ReverseIterator<Iterator1>& aIter1,
                         const ReverseIterator<Iterator2>& aIter2);

private:
  IteratorT mCurrent;
};

template<typename Iterator1, typename Iterator2>
bool
operator==(const ReverseIterator<Iterator1>& aIter1,
           const ReverseIterator<Iterator2>& aIter2)
{
  return aIter1.mCurrent == aIter2.mCurrent;
}

template<typename Iterator1, typename Iterator2>
bool
operator!=(const ReverseIterator<Iterator1>& aIter1,
           const ReverseIterator<Iterator2>& aIter2)
{
  return aIter1.mCurrent != aIter2.mCurrent;
}

template<typename Iterator1, typename Iterator2>
bool
operator<(const ReverseIterator<Iterator1>& aIter1,
          const ReverseIterator<Iterator2>& aIter2)
{
  return aIter1.mCurrent > aIter2.mCurrent;
}

template<typename Iterator1, typename Iterator2>
bool
operator<=(const ReverseIterator<Iterator1>& aIter1,
           const ReverseIterator<Iterator2>& aIter2)
{
  return aIter1.mCurrent >= aIter2.mCurrent;
}

template<typename Iterator1, typename Iterator2>
bool
operator>(const ReverseIterator<Iterator1>& aIter1,
          const ReverseIterator<Iterator2>& aIter2)
{
  return aIter1.mCurrent < aIter2.mCurrent;
}

template<typename Iterator1, typename Iterator2>
bool
operator>=(const ReverseIterator<Iterator1>& aIter1,
           const ReverseIterator<Iterator2>& aIter2)
{
  return aIter1.mCurrent <= aIter2.mCurrent;
}

namespace detail {

template<typename IteratorT>
class IteratorRange
{
public:
  typedef IteratorT iterator;
  typedef IteratorT const_iterator;
  typedef ReverseIterator<IteratorT> reverse_iterator;
  typedef ReverseIterator<IteratorT> const_reverse_iterator;

  template<typename Iterator1, typename Iterator2>
  IteratorRange(Iterator1 aIterBegin, Iterator2 aIterEnd)
    : mIterBegin(aIterBegin), mIterEnd(aIterEnd) { }

  template<typename Iterator>
  IteratorRange(const IteratorRange<Iterator>& aOther)
    : mIterBegin(aOther.mIterBegin), mIterEnd(aOther.mIterEnd) { }

  iterator begin() const { return mIterBegin; }
  const_iterator cbegin() const { return begin(); }
  iterator end() const { return mIterEnd; }
  const_iterator cend() const { return end(); }
  reverse_iterator rbegin() const { return reverse_iterator(mIterEnd); }
  const_reverse_iterator crbegin() const { return rbegin(); }
  reverse_iterator rend() const { return reverse_iterator(mIterBegin); }
  const_reverse_iterator crend() const { return rend(); }

private:
  IteratorT mIterBegin;
  IteratorT mIterEnd;
};

} 

template<typename Range>
detail::IteratorRange<typename Range::reverse_iterator>
Reversed(Range& aRange)
{
  return {aRange.rbegin(), aRange.rend()};
}

template<typename Range>
detail::IteratorRange<typename Range::const_reverse_iterator>
Reversed(const Range& aRange)
{
  return {aRange.rbegin(), aRange.rend()};
}

} 

#endif
