





#ifndef nsStringIterator_h___
#define nsStringIterator_h___

#include "nsCharTraits.h"
#include "nsAlgorithm.h"
#include "nsDebug.h"





template <class CharT>
class nsReadingIterator
{
public:
  typedef nsReadingIterator<CharT>    self_type;
  typedef ptrdiff_t                   difference_type;
  typedef CharT                       value_type;
  typedef const CharT*                pointer;
  typedef const CharT&                reference;

private:
  friend class nsAString;
  friend class nsACString;

  
  
  
  

  const CharT* mStart;
  const CharT* mEnd;
  const CharT* mPosition;

public:
  nsReadingIterator()
  {
  }
  
  

  inline void normalize_forward()
  {
  }
  inline void normalize_backward()
  {
  }

  pointer start() const
  {
    return mStart;
  }

  pointer end() const
  {
    return mEnd;
  }

  pointer get() const
  {
    return mPosition;
  }

  CharT operator*() const
  {
    return *get();
  }

#if 0
  
  
  pointer operator->() const
  {
    return get();
  }
#endif

  self_type& operator++()
  {
    ++mPosition;
    return *this;
  }

  self_type operator++(int)
  {
    self_type result(*this);
    ++mPosition;
    return result;
  }

  self_type& operator--()
  {
    --mPosition;
    return *this;
  }

  self_type operator--(int)
  {
    self_type result(*this);
    --mPosition;
    return result;
  }

  difference_type size_forward() const
  {
    return mEnd - mPosition;
  }

  difference_type size_backward() const
  {
    return mPosition - mStart;
  }

  self_type& advance(difference_type aN)
  {
    if (aN > 0) {
      difference_type step = XPCOM_MIN(aN, size_forward());

      NS_ASSERTION(step > 0,
                   "can't advance a reading iterator beyond the end of a string");

      mPosition += step;
    } else if (aN < 0) {
      difference_type step = XPCOM_MAX(aN, -size_backward());

      NS_ASSERTION(step < 0,
                   "can't advance (backward) a reading iterator beyond the end of a string");

      mPosition += step;
    }
    return *this;
  }
};





template <class CharT>
class nsWritingIterator
{
public:
  typedef nsWritingIterator<CharT>   self_type;
  typedef ptrdiff_t                  difference_type;
  typedef CharT                      value_type;
  typedef CharT*                     pointer;
  typedef CharT&                     reference;

private:
  friend class nsAString;
  friend class nsACString;

  
  
  
  

  CharT* mStart;
  CharT* mEnd;
  CharT* mPosition;

public:
  nsWritingIterator()
  {
  }
  
  

  inline void normalize_forward()
  {
  }
  inline void normalize_backward()
  {
  }

  pointer start() const
  {
    return mStart;
  }

  pointer end() const
  {
    return mEnd;
  }

  pointer get() const
  {
    return mPosition;
  }

  reference operator*() const
  {
    return *get();
  }

#if 0
  
  
  pointer
  operator->() const
  {
    return get();
  }
#endif

  self_type& operator++()
  {
    ++mPosition;
    return *this;
  }

  self_type operator++(int)
  {
    self_type result(*this);
    ++mPosition;
    return result;
  }

  self_type& operator--()
  {
    --mPosition;
    return *this;
  }

  self_type operator--(int)
  {
    self_type result(*this);
    --mPosition;
    return result;
  }

  difference_type size_forward() const
  {
    return mEnd - mPosition;
  }

  difference_type size_backward() const
  {
    return mPosition - mStart;
  }

  self_type& advance(difference_type aN)
  {
    if (aN > 0) {
      difference_type step = XPCOM_MIN(aN, size_forward());

      NS_ASSERTION(step > 0,
                   "can't advance a writing iterator beyond the end of a string");

      mPosition += step;
    } else if (aN < 0) {
      difference_type step = XPCOM_MAX(aN, -size_backward());

      NS_ASSERTION(step < 0,
                   "can't advance (backward) a writing iterator beyond the end of a string");

      mPosition += step;
    }
    return *this;
  }

  void write(const value_type* aS, uint32_t aN)
  {
    NS_ASSERTION(size_forward() > 0,
                 "You can't |write| into an |nsWritingIterator| with no space!");

    nsCharTraits<value_type>::move(mPosition, aS, aN);
    advance(difference_type(aN));
  }
};

template <class CharT>
inline bool
operator==(const nsReadingIterator<CharT>& aLhs,
           const nsReadingIterator<CharT>& aRhs)
{
  return aLhs.get() == aRhs.get();
}

template <class CharT>
inline bool
operator!=(const nsReadingIterator<CharT>& aLhs,
           const nsReadingIterator<CharT>& aRhs)
{
  return aLhs.get() != aRhs.get();
}






template <class CharT>
inline bool
operator==(const nsWritingIterator<CharT>& aLhs,
           const nsWritingIterator<CharT>& aRhs)
{
  return aLhs.get() == aRhs.get();
}

template <class CharT>
inline bool
operator!=(const nsWritingIterator<CharT>& aLhs,
           const nsWritingIterator<CharT>& aRhs)
{
  return aLhs.get() != aRhs.get();
}

#endif 
