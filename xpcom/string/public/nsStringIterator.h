





































#ifndef nsStringIterator_h___
#define nsStringIterator_h___

#ifndef nsCharTraits_h___
#include "nsCharTraits.h"
#endif

#ifndef nsAlgorithm_h___
#include "nsAlgorithm.h"
#endif

#ifndef nsDebug_h___
#include "nsDebug.h"
#endif

  



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
#ifdef MOZ_V1_STRING_ABI
      friend class nsSubstring;
      friend class nsCSubstring;
#endif

        
        
        
        

      const CharT* mStart;
      const CharT* mEnd;
      const CharT* mPosition;

    public:
      nsReadingIterator() { }
      
      

      inline void normalize_forward() {}
      inline void normalize_backward() {}

      pointer
      start() const
        {
          return mStart;
        }

      pointer
      end() const
        {
          return mEnd;
        }

      pointer
      get() const
        {
          return mPosition;
        }
      
      CharT
      operator*() const
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

      self_type&
      operator++()
        {
          ++mPosition;
          return *this;
        }

      self_type
      operator++( int )
        {
          self_type result(*this);
          ++mPosition;
          return result;
        }

      self_type&
      operator--()
        {
          --mPosition;
          return *this;
        }

      self_type
      operator--( int )
        {
          self_type result(*this);
          --mPosition;
          return result;
        }

      difference_type
      size_forward() const
        {
          return mEnd - mPosition;
        }

      difference_type
      size_backward() const
        {
          return mPosition - mStart;
        }

      self_type&
      advance( difference_type n )
        {
          if (n > 0)
            {
              difference_type step = NS_MIN(n, size_forward());

              NS_ASSERTION(step>0, "can't advance a reading iterator beyond the end of a string");

              mPosition += step;
            }
          else if (n < 0)
            {
              difference_type step = NS_MAX(n, -size_backward());

              NS_ASSERTION(step<0, "can't advance (backward) a reading iterator beyond the end of a string");

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
#ifdef MOZ_V1_STRING_ABI
      friend class nsSubstring;
      friend class nsCSubstring;
#endif

        
        
        
        

      CharT* mStart;
      CharT* mEnd;
      CharT* mPosition;

    public:
      nsWritingIterator() { }
      
      

      inline void normalize_forward() {}
      inline void normalize_backward() {}

      pointer
      start() const
        {
          return mStart;
        }

      pointer
      end() const
        {
          return mEnd;
        }

      pointer
      get() const
        {
          return mPosition;
        }
      
      reference
      operator*() const
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

      self_type&
      operator++()
        {
          ++mPosition;
          return *this;
        }

      self_type
      operator++( int )
        {
          self_type result(*this);
          ++mPosition;
          return result;
        }

      self_type&
      operator--()
        {
          --mPosition;
          return *this;
        }

      self_type
      operator--( int )
        {
          self_type result(*this);
          --mPosition;
          return result;
        }

      difference_type
      size_forward() const
        {
          return mEnd - mPosition;
        }

      difference_type
      size_backward() const
        {
          return mPosition - mStart;
        }

      self_type&
      advance( difference_type n )
        {
          if (n > 0)
            {
              difference_type step = NS_MIN(n, size_forward());

              NS_ASSERTION(step>0, "can't advance a writing iterator beyond the end of a string");

              mPosition += step;
            }
          else if (n < 0)
            {
              difference_type step = NS_MAX(n, -size_backward());

              NS_ASSERTION(step<0, "can't advance (backward) a writing iterator beyond the end of a string");

              mPosition += step;
            }
          return *this;
        }

      PRUint32
      write( const value_type* s, PRUint32 n )
        {
          NS_ASSERTION(size_forward() > 0, "You can't |write| into an |nsWritingIterator| with no space!");

          nsCharTraits<value_type>::move(mPosition, s, n);
          advance( difference_type(n) );
          return n;
        }
  };

template <class CharT>
inline
PRBool
operator==( const nsReadingIterator<CharT>& lhs, const nsReadingIterator<CharT>& rhs )
  {
    return lhs.get() == rhs.get();
  }

template <class CharT>
inline
PRBool
operator!=( const nsReadingIterator<CharT>& lhs, const nsReadingIterator<CharT>& rhs )
  {
    return lhs.get() != rhs.get();
  }


  
  
  

template <class CharT>
inline
PRBool
operator==( const nsWritingIterator<CharT>& lhs, const nsWritingIterator<CharT>& rhs )
  {
    return lhs.get() == rhs.get();
  }

template <class CharT>
inline
PRBool
operator!=( const nsWritingIterator<CharT>& lhs, const nsWritingIterator<CharT>& rhs )
  {
    return lhs.get() != rhs.get();
  }

#endif 
