





#ifndef nsScannerString_h___
#define nsScannerString_h___

#include "nsString.h"
#include "nsUnicharUtils.h" 
#include "mozilla/LinkedList.h"
#include <algorithm>


  














class nsScannerIterator;
class nsScannerSubstring;
class nsScannerString;


  


















class nsScannerBufferList
  {
    public:

        




      class Buffer : public mozilla::LinkedListElement<Buffer>
        {
          public:

            void IncrementUsageCount() { ++mUsageCount; }
            void DecrementUsageCount() { --mUsageCount; }

            bool IsInUse() const { return mUsageCount != 0; }

            const char16_t* DataStart() const { return (const char16_t*) (this+1); }
                  char16_t* DataStart()       { return (      char16_t*) (this+1); }

            const char16_t* DataEnd() const { return mDataEnd; }
                  char16_t* DataEnd()       { return mDataEnd; }

            const Buffer* Next() const { return getNext(); }
                  Buffer* Next()       { return getNext(); }

            const Buffer* Prev() const { return getPrevious(); }
                  Buffer* Prev()       { return getPrevious(); }

            uint32_t DataLength() const { return mDataEnd - DataStart(); }
            void SetDataLength(uint32_t len) { mDataEnd = DataStart() + len; }

          private:

            friend class nsScannerBufferList;

            int32_t    mUsageCount;
            char16_t* mDataEnd;
        };

        




      class Position
        {
          public:

            Position() {}
            
            Position( Buffer* buffer, char16_t* position )
              : mBuffer(buffer)
              , mPosition(position)
              {}
            
            inline
            explicit Position( const nsScannerIterator& aIter );

            inline
            Position& operator=( const nsScannerIterator& aIter );

            static size_t Distance( const Position& p1, const Position& p2 );

            Buffer*    mBuffer;
            char16_t* mPosition;
        };

      static Buffer* AllocBufferFromString( const nsAString& );
      static Buffer* AllocBuffer( uint32_t capacity ); 

      explicit nsScannerBufferList( Buffer* buf )
        : mRefCnt(0)
        {
          mBuffers.insertBack(buf);
        }

      void  AddRef()  { ++mRefCnt; }
      void  Release() { if (--mRefCnt == 0) delete this; }

      void  Append( Buffer* buf ) { mBuffers.insertBack(buf); } 
      void  InsertAfter( Buffer* buf, Buffer* prev ) { prev->setNext(buf); }
      void  SplitBuffer( const Position& );
      void  DiscardUnreferencedPrefix( Buffer* );

            Buffer* Head()       { return mBuffers.getFirst(); }
      const Buffer* Head() const { return mBuffers.getFirst(); }

            Buffer* Tail()       { return mBuffers.getLast(); }
      const Buffer* Tail() const { return mBuffers.getLast(); }

    private:

      friend class nsScannerSubstring;

      ~nsScannerBufferList() { ReleaseAll(); }
      void ReleaseAll();

      int32_t mRefCnt;
      mozilla::LinkedList<Buffer> mBuffers;
  };


  


struct nsScannerFragment
  {
    typedef nsScannerBufferList::Buffer Buffer;

    const Buffer*    mBuffer;
    const char16_t* mFragmentStart;
    const char16_t* mFragmentEnd;
  };


  






class nsScannerSubstring
  {
    public:
      typedef nsScannerBufferList::Buffer      Buffer;
      typedef nsScannerBufferList::Position    Position;
      typedef uint32_t                         size_type;

      nsScannerSubstring();
      explicit nsScannerSubstring( const nsAString& s );

      ~nsScannerSubstring();

      nsScannerIterator& BeginReading( nsScannerIterator& iter ) const;
      nsScannerIterator& EndReading( nsScannerIterator& iter ) const;

      size_type Length() const { return mLength; }

      int32_t CountChar( char16_t ) const;

      void Rebind( const nsScannerSubstring&, const nsScannerIterator&, const nsScannerIterator& );
      void Rebind( const nsAString& );

      const nsSubstring& AsString() const;

      bool GetNextFragment( nsScannerFragment& ) const;
      bool GetPrevFragment( nsScannerFragment& ) const;

      static inline Buffer* AllocBufferFromString( const nsAString& aStr ) { return nsScannerBufferList::AllocBufferFromString(aStr); }
      static inline Buffer* AllocBuffer( size_type aCapacity )             { return nsScannerBufferList::AllocBuffer(aCapacity); }

    protected:

      void acquire_ownership_of_buffer_list() const
        {
          mBufferList->AddRef();
          mStart.mBuffer->IncrementUsageCount();
        }

      void release_ownership_of_buffer_list()
        {
          if (mBufferList)
            {
              mStart.mBuffer->DecrementUsageCount();
              mBufferList->DiscardUnreferencedPrefix(mStart.mBuffer);
              mBufferList->Release();
            }
        }
      
      void init_range_from_buffer_list()
        {
          mStart.mBuffer = mBufferList->Head();
          mStart.mPosition = mStart.mBuffer->DataStart();

          mEnd.mBuffer = mBufferList->Tail();
          mEnd.mPosition = mEnd.mBuffer->DataEnd();

          mLength = Position::Distance(mStart, mEnd);
        }

      Position             mStart;
      Position             mEnd;
      nsScannerBufferList *mBufferList;
      size_type            mLength;

      
      nsDependentSubstring mFlattenedRep;
      bool                 mIsDirty;

      friend class nsScannerSharedSubstring;
  };


  


class nsScannerString : public nsScannerSubstring
  {
    public:

      explicit nsScannerString( Buffer* );

        
        
        
        
      void AppendBuffer( Buffer* );

      void DiscardPrefix( const nsScannerIterator& );
        

      void UngetReadable(const nsAString& aReadable, const nsScannerIterator& aCurrentPosition);
      void ReplaceCharacter(nsScannerIterator& aPosition, char16_t aChar);
  };


  






class nsScannerSharedSubstring
  {
    public:
      nsScannerSharedSubstring()
        : mBuffer(nullptr), mBufferList(nullptr) { }

      ~nsScannerSharedSubstring()
        {
          if (mBufferList)
            ReleaseBuffer();
        }

        
      void Rebind(const nsScannerIterator& aStart,
                              const nsScannerIterator& aEnd);

       
      nsSubstring& writable()
        {
          if (mBufferList)
            MakeMutable();

          return mString;
        }

        
      const nsSubstring& str() const { return mString; }

    private:
      typedef nsScannerBufferList::Buffer Buffer;

      void ReleaseBuffer();
      void MakeMutable();

      nsDependentSubstring  mString;
      Buffer               *mBuffer;
      nsScannerBufferList  *mBufferList;
  };

  



class nsScannerIterator
  {
    public:
      typedef nsScannerIterator             self_type;
      typedef ptrdiff_t                     difference_type;
      typedef char16_t                     value_type;
      typedef const char16_t*              pointer;
      typedef const char16_t&              reference;
      typedef nsScannerSubstring::Buffer    Buffer;

    protected:

      nsScannerFragment         mFragment;
      const char16_t*          mPosition;
      const nsScannerSubstring* mOwner;

      friend class nsScannerSubstring;
      friend class nsScannerSharedSubstring;

    public:
      nsScannerIterator() {}
      
      

      inline void normalize_forward();
      inline void normalize_backward();

      pointer get() const
        {
          return mPosition;
        }
      
      char16_t operator*() const
        {
          return *get();
        }

      const nsScannerFragment& fragment() const
        {
          return mFragment;
        }

      const Buffer* buffer() const
        {
          return mFragment.mBuffer;
        }

      self_type& operator++()
        {
          ++mPosition;
          normalize_forward();
          return *this;
        }

      self_type operator++( int )
        {
          self_type result(*this);
          ++mPosition;
          normalize_forward();
          return result;
        }

      self_type& operator--()
        {
          normalize_backward();
          --mPosition;
          return *this;
        }

      self_type operator--( int )
        {
          self_type result(*this);
          normalize_backward();
          --mPosition;
          return result;
        }

      difference_type size_forward() const
        {
          return mFragment.mFragmentEnd - mPosition;
        }

      difference_type size_backward() const
        {
          return mPosition - mFragment.mFragmentStart;
        }

      self_type& advance( difference_type n )
        {
          while ( n > 0 )
            {
              difference_type one_hop = std::min(n, size_forward());

              NS_ASSERTION(one_hop>0, "Infinite loop: can't advance a reading iterator beyond the end of a string");
                

              mPosition += one_hop;
              normalize_forward();
              n -= one_hop;
            }

          while ( n < 0 )
            {
              normalize_backward();
              difference_type one_hop = std::max(n, -size_backward());

              NS_ASSERTION(one_hop<0, "Infinite loop: can't advance (backward) a reading iterator beyond the end of a string");
                

              mPosition += one_hop;
              n -= one_hop;
            }

          return *this;
        }
  };


inline
bool
SameFragment( const nsScannerIterator& a, const nsScannerIterator& b )
  {
    return a.fragment().mFragmentStart == b.fragment().mFragmentStart;
  }


  


template <>
struct nsCharSourceTraits<nsScannerIterator>
  {
    typedef nsScannerIterator::difference_type difference_type;

    static
    uint32_t
    readable_distance( const nsScannerIterator& first, const nsScannerIterator& last )
      {
        return uint32_t(SameFragment(first, last) ? last.get() - first.get() : first.size_forward());
      }

    static
    const nsScannerIterator::value_type*
    read( const nsScannerIterator& iter )
      {
        return iter.get();
      }
    
    static
    void
    advance( nsScannerIterator& s, difference_type n )
      {
        s.advance(n);
      }
  };


  



inline
void
nsScannerIterator::normalize_forward()
  {
    while (mPosition == mFragment.mFragmentEnd && mOwner->GetNextFragment(mFragment))
      mPosition = mFragment.mFragmentStart;
  }

inline
void
nsScannerIterator::normalize_backward()
  {
    while (mPosition == mFragment.mFragmentStart && mOwner->GetPrevFragment(mFragment))
      mPosition = mFragment.mFragmentEnd;
  }

inline
bool
operator==( const nsScannerIterator& lhs, const nsScannerIterator& rhs )
  {
    return lhs.get() == rhs.get();
  }

inline
bool
operator!=( const nsScannerIterator& lhs, const nsScannerIterator& rhs )
  {
    return lhs.get() != rhs.get();
  }


inline
nsScannerBufferList::Position::Position(const nsScannerIterator& aIter)
  : mBuffer(const_cast<Buffer*>(aIter.buffer()))
  , mPosition(const_cast<char16_t*>(aIter.get()))
  {}

inline
nsScannerBufferList::Position&
nsScannerBufferList::Position::operator=(const nsScannerIterator& aIter)
  {
    mBuffer   = const_cast<Buffer*>(aIter.buffer());
    mPosition = const_cast<char16_t*>(aIter.get());
    return *this;
  }


  






inline
size_t
Distance( const nsScannerIterator& aStart, const nsScannerIterator& aEnd )
  {
    typedef nsScannerBufferList::Position Position;
    return Position::Distance(Position(aStart), Position(aEnd));
  }

void
CopyUnicodeTo( const nsScannerIterator& aSrcStart,
               const nsScannerIterator& aSrcEnd,
               nsAString& aDest );

inline
void
CopyUnicodeTo( const nsScannerSubstring& aSrc, nsAString& aDest )
  {
    nsScannerIterator begin, end;
    CopyUnicodeTo(aSrc.BeginReading(begin), aSrc.EndReading(end), aDest);
  }

void
AppendUnicodeTo( const nsScannerIterator& aSrcStart,
                 const nsScannerIterator& aSrcEnd,
                 nsAString& aDest );

inline
void
AppendUnicodeTo( const nsScannerSubstring& aSrc, nsAString& aDest )
  {
    nsScannerIterator begin, end;
    AppendUnicodeTo(aSrc.BeginReading(begin), aSrc.EndReading(end), aDest);
  }

void
AppendUnicodeTo( const nsScannerIterator& aSrcStart,
                 const nsScannerIterator& aSrcEnd,
                 nsScannerSharedSubstring& aDest );

bool
FindCharInReadable( char16_t aChar,
                    nsScannerIterator& aStart,
                    const nsScannerIterator& aEnd );

bool
FindInReadable( const nsAString& aPattern,
                nsScannerIterator& aStart,
                nsScannerIterator& aEnd,
                const nsStringComparator& = nsDefaultStringComparator() );

bool
RFindInReadable( const nsAString& aPattern,
                 nsScannerIterator& aStart,
                 nsScannerIterator& aEnd,
                 const nsStringComparator& = nsDefaultStringComparator() );

inline
bool
CaseInsensitiveFindInReadable( const nsAString& aPattern, 
                               nsScannerIterator& aStart,
                               nsScannerIterator& aEnd )
  {
    return FindInReadable(aPattern, aStart, aEnd,
                          nsCaseInsensitiveStringComparator());
  }

#endif 
