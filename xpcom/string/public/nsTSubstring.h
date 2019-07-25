





































#ifndef MOZILLA_INTERNAL_API
#error Cannot use internal string classes without MOZILLA_INTERNAL_API defined. Use the frozen header nsStringAPI.h instead.
#endif

  


class nsTStringComparator_CharT
  {
    public:
      typedef CharT char_type;

      nsTStringComparator_CharT() {}

      virtual int operator()( const char_type*, const char_type*, PRUint32, PRUint32 ) const = 0;
  };


  


class nsTDefaultStringComparator_CharT
    : public nsTStringComparator_CharT
  {
    public:
      typedef CharT char_type;

      nsTDefaultStringComparator_CharT() {}

      virtual int operator()( const char_type*, const char_type*, PRUint32, PRUint32 ) const;
  };

  











class nsTSubstring_CharT
  {
    public:
      typedef CharT                               char_type;

      typedef nsCharTraits<char_type>             char_traits;
      typedef char_traits::incompatible_char_type incompatible_char_type;

      typedef nsTSubstring_CharT                  self_type;
      typedef self_type                           abstract_string_type;
      typedef self_type                           base_string_type;

      typedef self_type                           substring_type;
      typedef nsTSubstringTuple_CharT             substring_tuple_type;
      typedef nsTString_CharT                     string_type;

      typedef nsReadingIterator<char_type>        const_iterator;
      typedef nsWritingIterator<char_type>        iterator;

      typedef nsTStringComparator_CharT           comparator_type;

      typedef char_type*                          char_iterator;
      typedef const char_type*                    const_char_iterator;

      typedef PRUint32                            size_type;
      typedef PRUint32                            index_type;

    public:

        
      ~nsTSubstring_CharT() { Finalize(); }

        



      const_char_iterator BeginReading() const { return mData; }
      const_char_iterator EndReading() const { return mData + mLength; }

        



      const_iterator& BeginReading( const_iterator& iter ) const
        {
          iter.mStart = mData;
          iter.mEnd = mData + mLength;
          iter.mPosition = iter.mStart;
          return iter;
        }

      const_iterator& EndReading( const_iterator& iter ) const
        {
          iter.mStart = mData;
          iter.mEnd = mData + mLength;
          iter.mPosition = iter.mEnd;
          return iter;
        }

      const_char_iterator& BeginReading( const_char_iterator& iter ) const
        {
          return iter = mData;
        }

      const_char_iterator& EndReading( const_char_iterator& iter ) const
        {
          return iter = mData + mLength;
        }


        


      
      char_iterator BeginWriting()
        {
          return EnsureMutable() ? mData : char_iterator(0);
        }

      char_iterator EndWriting()
        {
          return EnsureMutable() ? (mData + mLength) : char_iterator(0);
        }


        


      
      iterator& BeginWriting( iterator& iter )
        {
          char_type *data = EnsureMutable() ? mData : nsnull;
          iter.mStart = data;
          iter.mEnd = data + mLength;
          iter.mPosition = iter.mStart;
          return iter;
        }

      iterator& EndWriting( iterator& iter )
        {
          char_type *data = EnsureMutable() ? mData : nsnull;
          iter.mStart = data;
          iter.mEnd = data + mLength;
          iter.mPosition = iter.mEnd;
          return iter;
        }

      char_iterator& BeginWriting( char_iterator& iter )
        {
          return iter = EnsureMutable() ? mData : char_iterator(0);
        }

      char_iterator& EndWriting( char_iterator& iter )
        {
          return iter = EnsureMutable() ? (mData + mLength) : char_iterator(0);
        }


        



        
      const char_type *Data() const
        {
          return mData;
        }

      size_type Length() const
        {
          return mLength;
        }

      bool IsEmpty() const
        {
          return mLength == 0;
        }

      bool IsVoid() const
        {
          return (mFlags & F_VOIDED) != 0;
        }

      bool IsTerminated() const
        {
          return (mFlags & F_TERMINATED) != 0;
        }

      char_type CharAt( index_type i ) const
        {
          NS_ASSERTION(i < mLength, "index exceeds allowable range");
          return mData[i];
        }

      char_type operator[]( index_type i ) const
        {
          return CharAt(i);
        }

      char_type First() const
        {
          NS_ASSERTION(mLength > 0, "|First()| called on an empty string");
          return mData[0];
        }

      inline
      char_type Last() const
        {
          NS_ASSERTION(mLength > 0, "|Last()| called on an empty string");
          return mData[mLength - 1];
        }

      size_type NS_FASTCALL CountChar( char_type ) const;
      PRInt32 NS_FASTCALL FindChar( char_type, index_type offset = 0 ) const;


        



      bool NS_FASTCALL Equals( const self_type& ) const;
      bool NS_FASTCALL Equals( const self_type&, const comparator_type& ) const;

      bool NS_FASTCALL Equals( const char_type* data ) const;
      bool NS_FASTCALL Equals( const char_type* data, const comparator_type& comp ) const;

        




      bool NS_FASTCALL EqualsASCII( const char* data, size_type len ) const;
        




      bool NS_FASTCALL EqualsASCII( const char* data ) const;

    
    
    
    
    
#ifdef NS_DISABLE_LITERAL_TEMPLATE
      inline bool EqualsLiteral( const char* str ) const
        {
          return EqualsASCII(str);
        }
#else
      template<int N>
      inline bool EqualsLiteral( const char (&str)[N] ) const
        {
          return EqualsASCII(str, N-1);
        }
      template<int N>
      inline bool EqualsLiteral( char (&str)[N] ) const
        {
          const char* s = str;
          return EqualsASCII(s, N-1);
        }
#endif

    
    
    
    
    
      bool NS_FASTCALL LowerCaseEqualsASCII( const char* data, size_type len ) const;
      bool NS_FASTCALL LowerCaseEqualsASCII( const char* data ) const;

    
    
    
    
#ifdef NS_DISABLE_LITERAL_TEMPLATE
      inline bool LowerCaseEqualsLiteral( const char* str ) const
        {
          return LowerCaseEqualsASCII(str);
        }
#else
      template<int N>
      inline bool LowerCaseEqualsLiteral( const char (&str)[N] ) const
        {
          return LowerCaseEqualsASCII(str, N-1);
        }
      template<int N>
      inline bool LowerCaseEqualsLiteral( char (&str)[N] ) const
        {
          const char* s = str;
          return LowerCaseEqualsASCII(s, N-1);
        }
#endif

        



      void NS_FASTCALL Assign( char_type c );
      void NS_FASTCALL Assign( const char_type* data, size_type length = size_type(-1) );
      void NS_FASTCALL Assign( const self_type& );
      void NS_FASTCALL Assign( const substring_tuple_type& );

      void NS_FASTCALL AssignASCII( const char* data, size_type length );
      void NS_FASTCALL AssignASCII( const char* data );

    
    
    
#ifdef NS_DISABLE_LITERAL_TEMPLATE
      void AssignLiteral( const char* str )
                  { AssignASCII(str); }
#else
      template<int N>
      void AssignLiteral( const char (&str)[N] )
                  { AssignASCII(str, N-1); }
      template<int N>
      void AssignLiteral( char (&str)[N] )
                  { AssignASCII(str, N-1); }
#endif

      self_type& operator=( char_type c )                                                       { Assign(c);        return *this; }
      self_type& operator=( const char_type* data )                                             { Assign(data);     return *this; }
      self_type& operator=( const self_type& str )                                              { Assign(str);      return *this; }
      self_type& operator=( const substring_tuple_type& tuple )                                 { Assign(tuple);    return *this; }

      void NS_FASTCALL Adopt( char_type* data, size_type length = size_type(-1) );


        



      void NS_FASTCALL Replace( index_type cutStart, size_type cutLength, char_type c );
      void NS_FASTCALL Replace( index_type cutStart, size_type cutLength, const char_type* data, size_type length = size_type(-1) );
             void Replace( index_type cutStart, size_type cutLength, const self_type& str )      { Replace(cutStart, cutLength, str.Data(), str.Length()); }
      void NS_FASTCALL Replace( index_type cutStart, size_type cutLength, const substring_tuple_type& tuple );

      void NS_FASTCALL ReplaceASCII( index_type cutStart, size_type cutLength, const char* data, size_type length = size_type(-1) );

      void Append( char_type c )                                                                 { Replace(mLength, 0, c); }
      void Append( const char_type* data, size_type length = size_type(-1) )                     { Replace(mLength, 0, data, length); }
      void Append( const self_type& str )                                                        { Replace(mLength, 0, str); }
      void Append( const substring_tuple_type& tuple )                                           { Replace(mLength, 0, tuple); }

      void AppendASCII( const char* data, size_type length = size_type(-1) )                     { ReplaceASCII(mLength, 0, data, length); }

      void AppendPrintf( const char* format, ... );
      void AppendInt( PRInt32 aInteger )
                 { AppendPrintf31( "%d", aInteger ); }
      void AppendInt( PRInt32 aInteger, int aRadix )
        {
          const char *fmt = aRadix == 10 ? "%d" : aRadix == 8 ? "%o" : "%x";
          AppendPrintf31( fmt, aInteger );
        }
      void AppendInt( PRUint32 aInteger )
                 { AppendPrintf31( "%u", aInteger ); }
      void AppendInt( PRUint32 aInteger, int aRadix )
        {
          const char *fmt = aRadix == 10 ? "%u" : aRadix == 8 ? "%o" : "%x";
          AppendPrintf31( fmt, aInteger );
        }
      void AppendInt( PRInt64 aInteger )
                 { AppendPrintf31( "%lld", aInteger ); }
      void AppendInt( PRInt64 aInteger, int aRadix )
        {
          const char *fmt = aRadix == 10 ? "%lld" : aRadix == 8 ? "%llo" : "%llx";
          AppendPrintf31( fmt, aInteger );
        }
      void AppendInt( PRUint64 aInteger )
                 { AppendPrintf31( "%llu", aInteger ); }
      void AppendInt( PRUint64 aInteger, int aRadix )
        {
          const char *fmt = aRadix == 10 ? "%llu" : aRadix == 8 ? "%llo" : "%llx";
          AppendPrintf31( fmt, aInteger );
        }

      


      void AppendFloat( float aFloat )
                      { DoAppendFloat(aFloat, 6); }
      void AppendFloat( double aFloat )
                      { DoAppendFloat(aFloat, 15); }
  private:
      void NS_FASTCALL DoAppendFloat( double aFloat, int digits );
      
      void AppendPrintf31( const char* format, ... );
  public:

    
    
    
#ifdef NS_DISABLE_LITERAL_TEMPLATE
      void AppendLiteral( const char* str )
                  { AppendASCII(str); }
#else
      template<int N>
      void AppendLiteral( const char (&str)[N] )
                  { AppendASCII(str, N-1); }
      template<int N>
      void AppendLiteral( char (&str)[N] )
                  { AppendASCII(str, N-1); }
#endif

      self_type& operator+=( char_type c )                                                       { Append(c);        return *this; }
      self_type& operator+=( const char_type* data )                                             { Append(data);     return *this; }
      self_type& operator+=( const self_type& str )                                              { Append(str);      return *this; }
      self_type& operator+=( const substring_tuple_type& tuple )                                 { Append(tuple);    return *this; }

      void Insert( char_type c, index_type pos )                                                 { Replace(pos, 0, c); }
      void Insert( const char_type* data, index_type pos, size_type length = size_type(-1) )     { Replace(pos, 0, data, length); }
      void Insert( const self_type& str, index_type pos )                                        { Replace(pos, 0, str); }
      void Insert( const substring_tuple_type& tuple, index_type pos )                           { Replace(pos, 0, tuple); }

      void Cut( index_type cutStart, size_type cutLength )                                       { Replace(cutStart, cutLength, char_traits::sEmptyBuffer, 0); }


        



        







      bool NS_FASTCALL SetCapacity( size_type newCapacity );

      void NS_FASTCALL SetLength( size_type newLength );

      void Truncate( size_type newLength = 0 )
        {
          NS_ASSERTION(newLength <= mLength, "Truncate cannot make string longer");
          SetLength(newLength);
        }


        




        





      inline size_type GetData( const char_type** data ) const
        {
          *data = mData;
          return mLength;
        }
        
        









      inline size_type GetMutableData( char_type** data, size_type newLen = size_type(-1) )
        {
          if (!EnsureMutable(newLen))
            {
              *data = nsnull;
              return 0;
            }

          *data = mData;
          return mLength;
        }


        




      void NS_FASTCALL SetIsVoid( bool );

        






         
      void StripChar( char_type aChar, PRInt32 aOffset=0 );

        







      void StripChars( const char_type* aChars, PRUint32 aOffset=0 );

        



      void ForgetSharedBuffer()
      {
        if (mFlags & nsSubstring::F_SHARED)
          {
            mData = char_traits::sEmptyBuffer;
            mLength = 0;
            mFlags = F_TERMINATED;
          }
      }

    public:

        



      nsTSubstring_CharT(const substring_tuple_type& tuple)
        : mData(nsnull),
          mLength(0),
          mFlags(F_NONE)
        {
          Assign(tuple);
        }

        





        
#if defined(DEBUG) || defined(FORCE_BUILD_REFCNT_LOGGING)
#define XPCOM_STRING_CONSTRUCTOR_OUT_OF_LINE
       nsTSubstring_CharT( char_type *data, size_type length, PRUint32 flags );
#else
#undef XPCOM_STRING_CONSTRUCTOR_OUT_OF_LINE
       nsTSubstring_CharT( char_type *data, size_type length, PRUint32 flags )
         : mData(data),
           mLength(length),
           mFlags(flags) {}
#endif 

    protected:

      friend class nsTObsoleteAStringThunk_CharT;
      friend class nsTSubstringTuple_CharT;

      
      friend class nsTPromiseFlatString_CharT;

      char_type*  mData;
      size_type   mLength;
      PRUint32    mFlags;

        
      nsTSubstring_CharT()
        : mData(char_traits::sEmptyBuffer),
          mLength(0),
          mFlags(F_TERMINATED) {}

        
      explicit
      nsTSubstring_CharT( PRUint32 flags )
        : mFlags(flags) {}

        
        
      nsTSubstring_CharT( const self_type& str )
        : mData(str.mData),
          mLength(str.mLength),
          mFlags(str.mFlags & (F_TERMINATED | F_VOIDED)) {}

        




      void NS_FASTCALL Finalize();

        

















      bool NS_FASTCALL MutatePrep( size_type capacity, char_type** old_data, PRUint32* old_flags );

        



















      bool ReplacePrep(index_type cutStart, size_type cutLength,
                         size_type newLength)
      {
        cutLength = NS_MIN(cutLength, mLength - cutStart);
        PRUint32 newTotalLen = mLength - cutLength + newLength;
        if (cutStart == mLength && Capacity() > newTotalLen) {
          mFlags &= ~F_VOIDED;
          mData[newTotalLen] = char_type(0);
          mLength = newTotalLen;
          return true;
        }
        return ReplacePrepInternal(cutStart, cutLength, newLength, newTotalLen);
      }

      bool NS_FASTCALL ReplacePrepInternal(index_type cutStart,
                                             size_type cutLength,
                                             size_type newFragLength,
                                             size_type newTotalLength);
      
        






      size_type NS_FASTCALL Capacity() const;

        



      bool NS_FASTCALL EnsureMutable( size_type newLen = size_type(-1) );

        


      bool IsDependentOn( const char_type *start, const char_type *end ) const
        {
          







          return ( start < (mData + mLength) && end > mData );
        }

        


      void SetDataFlags(PRUint32 dataFlags)
        {
          NS_ASSERTION((dataFlags & 0xFFFF0000) == 0, "bad flags");
          mFlags = dataFlags | (mFlags & 0xFFFF0000);
        }

    public:

      
      
      
      
      
      
      enum
        {
          F_NONE         = 0,       

          
          F_TERMINATED   = 1 << 0,  
          F_VOIDED       = 1 << 1,  
          F_SHARED       = 1 << 2,  
          F_OWNED        = 1 << 3,  
          F_FIXED        = 1 << 4,  

          
          F_CLASS_FIXED  = 1 << 16   
        };

      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
  };

int NS_FASTCALL Compare( const nsTSubstring_CharT::base_string_type& lhs, const nsTSubstring_CharT::base_string_type& rhs, const nsTStringComparator_CharT& = nsTDefaultStringComparator_CharT() );


inline
bool operator!=( const nsTSubstring_CharT::base_string_type& lhs, const nsTSubstring_CharT::base_string_type& rhs )
  {
    return !lhs.Equals(rhs);
  }

inline
bool operator< ( const nsTSubstring_CharT::base_string_type& lhs, const nsTSubstring_CharT::base_string_type& rhs )
  {
    return Compare(lhs, rhs)< 0;
  }

inline
bool operator<=( const nsTSubstring_CharT::base_string_type& lhs, const nsTSubstring_CharT::base_string_type& rhs )
  {
    return Compare(lhs, rhs)<=0;
  }

inline
bool operator==( const nsTSubstring_CharT::base_string_type& lhs, const nsTSubstring_CharT::base_string_type& rhs )
  {
    return lhs.Equals(rhs);
  }

inline
bool operator>=( const nsTSubstring_CharT::base_string_type& lhs, const nsTSubstring_CharT::base_string_type& rhs )
  {
    return Compare(lhs, rhs)>=0;
  }

inline
bool operator> ( const nsTSubstring_CharT::base_string_type& lhs, const nsTSubstring_CharT::base_string_type& rhs )
  {
    return Compare(lhs, rhs)> 0;
  }
