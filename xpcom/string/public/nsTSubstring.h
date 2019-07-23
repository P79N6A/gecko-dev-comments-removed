





































#ifndef MOZILLA_INTERNAL_API
#error Cannot use internal string classes without MOZILLA_INTERNAL_API defined. Use the frozen header nsStringAPI.h instead.
#endif

  


class NS_COM nsTStringComparator_CharT
  {
    public:
      typedef CharT char_type;

      nsTStringComparator_CharT() {}

      virtual int operator()( const char_type*, const char_type*, PRUint32 length ) const = 0;
      virtual int operator()( char_type, char_type ) const = 0;
  };


  


class NS_COM nsTDefaultStringComparator_CharT
    : public nsTStringComparator_CharT
  {
    public:
      typedef CharT char_type;

      nsTDefaultStringComparator_CharT() {}

      virtual int operator()( const char_type*, const char_type*, PRUint32 length ) const;
      virtual int operator()( char_type, char_type ) const;
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

        
      NS_COM NS_CONSTRUCTOR_FASTCALL ~nsTSubstring_CharT();

        



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

      PRBool IsEmpty() const
        {
          return mLength == 0;
        }

      PRBool IsVoid() const
        {
          return (mFlags & F_VOIDED) != 0;
        }

      PRBool IsTerminated() const
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

      NS_COM size_type NS_FASTCALL CountChar( char_type ) const;
      NS_COM PRInt32 NS_FASTCALL FindChar( char_type, index_type offset = 0 ) const;


        



      NS_COM PRBool NS_FASTCALL Equals( const self_type& ) const;
      NS_COM PRBool NS_FASTCALL Equals( const self_type&, const comparator_type& ) const;

      NS_COM PRBool NS_FASTCALL Equals( const char_type* data ) const;
      NS_COM PRBool NS_FASTCALL Equals( const char_type* data, const comparator_type& comp ) const;

        




      NS_COM PRBool NS_FASTCALL EqualsASCII( const char* data, size_type len ) const;
        




      NS_COM PRBool NS_FASTCALL EqualsASCII( const char* data ) const;

    
    
    
    
    
#ifdef NS_DISABLE_LITERAL_TEMPLATE
      inline PRBool EqualsLiteral( const char* str ) const
        {
          return EqualsASCII(str);
        }
#else
      template<int N>
      inline PRBool EqualsLiteral( const char (&str)[N] ) const
        {
          return EqualsASCII(str, N-1);
        }
      template<int N>
      inline PRBool EqualsLiteral( char (&str)[N] ) const
        {
          const char* s = str;
          return EqualsASCII(s, N-1);
        }
#endif

    
    
    
    
    
      NS_COM PRBool NS_FASTCALL LowerCaseEqualsASCII( const char* data, size_type len ) const;
      NS_COM PRBool NS_FASTCALL LowerCaseEqualsASCII( const char* data ) const;

    
    
    
    
#ifdef NS_DISABLE_LITERAL_TEMPLATE
      inline PRBool LowerCaseEqualsLiteral( const char* str ) const
        {
          return LowerCaseEqualsASCII(str);
        }
#else
      template<int N>
      inline PRBool LowerCaseEqualsLiteral( const char (&str)[N] ) const
        {
          return LowerCaseEqualsASCII(str, N-1);
        }
      template<int N>
      inline PRBool LowerCaseEqualsLiteral( char (&str)[N] ) const
        {
          const char* s = str;
          return LowerCaseEqualsASCII(s, N-1);
        }
#endif

        



      NS_COM void NS_FASTCALL Assign( char_type c );
      NS_COM void NS_FASTCALL Assign( const char_type* data, size_type length = size_type(-1) );
      NS_COM void NS_FASTCALL Assign( const self_type& );
      NS_COM void NS_FASTCALL Assign( const substring_tuple_type& );

      NS_COM void NS_FASTCALL AssignASCII( const char* data, size_type length );
      NS_COM void NS_FASTCALL AssignASCII( const char* data );

    
    
    
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

      NS_COM void NS_FASTCALL Adopt( char_type* data, size_type length = size_type(-1) );


        



      NS_COM void NS_FASTCALL Replace( index_type cutStart, size_type cutLength, char_type c );
      NS_COM void NS_FASTCALL Replace( index_type cutStart, size_type cutLength, const char_type* data, size_type length = size_type(-1) );
             void Replace( index_type cutStart, size_type cutLength, const self_type& str )      { Replace(cutStart, cutLength, str.Data(), str.Length()); }
      NS_COM void NS_FASTCALL Replace( index_type cutStart, size_type cutLength, const substring_tuple_type& tuple );

      NS_COM void NS_FASTCALL ReplaceASCII( index_type cutStart, size_type cutLength, const char* data, size_type length = size_type(-1) );

      void Append( char_type c )                                                                 { Replace(mLength, 0, c); }
      void Append( const char_type* data, size_type length = size_type(-1) )                     { Replace(mLength, 0, data, length); }
      void Append( const self_type& str )                                                        { Replace(mLength, 0, str); }
      void Append( const substring_tuple_type& tuple )                                           { Replace(mLength, 0, tuple); }

      void AppendASCII( const char* data, size_type length = size_type(-1) )                     { ReplaceASCII(mLength, 0, data, length); }

    
      NS_COM void AppendPrintf( const char* format, ... );
      void AppendInt( PRInt32 aInteger )
                 { AppendPrintf( "%d", aInteger ); }
      void AppendInt( PRInt32 aInteger, int aRadix )
        {
          const char *fmt = aRadix == 10 ? "%d" : aRadix == 8 ? "%o" : "%x";
          AppendPrintf( fmt, aInteger );
        }
      void AppendInt( PRUint32 aInteger )
                 { AppendPrintf( "%u", aInteger ); }
      void AppendInt( PRUint32 aInteger, int aRadix )
        {
          const char *fmt = aRadix == 10 ? "%u" : aRadix == 8 ? "%o" : "%x";
          AppendPrintf( fmt, aInteger );
        }
      void AppendInt( PRInt64 aInteger )
                 { AppendPrintf( "%lld", aInteger ); }
      void AppendInt( PRInt64 aInteger, int aRadix )
        {
          const char *fmt = aRadix == 10 ? "%lld" : aRadix == 8 ? "%llo" : "%llx";
          AppendPrintf( fmt, aInteger );
        }
      void AppendInt( PRUint64 aInteger )
                 { AppendPrintf( "%llu", aInteger ); }
      void AppendInt( PRUint64 aInteger, int aRadix )
        {
          const char *fmt = aRadix == 10 ? "%llu" : aRadix == 8 ? "%llo" : "%llx";
          AppendPrintf( fmt, aInteger );
        }

    
    
    
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


        



      NS_COM void NS_FASTCALL SetCapacity( size_type newCapacity );

      NS_COM void NS_FASTCALL SetLength( size_type newLength );

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


        




      NS_COM void NS_FASTCALL SetIsVoid( PRBool );

        






         
      NS_COM void StripChar( char_type aChar, PRInt32 aOffset=0 );


    public:

        



      NS_COM nsTSubstring_CharT(const substring_tuple_type& tuple);

        





#ifdef XP_OS2 
       nsTSubstring_CharT( char_type *data, size_type length, PRUint32 flags ) NS_COM;
#else
       NS_COM nsTSubstring_CharT( char_type *data, size_type length, PRUint32 flags );
#endif
    protected:

      friend class nsTObsoleteAStringThunk_CharT;
      friend class nsTSubstringTuple_CharT;

      
      friend class nsTPromiseFlatString_CharT;

      char_type*  mData;
      size_type   mLength;
      PRUint32    mFlags;

        
      NS_COM nsTSubstring_CharT();

        
      explicit
      NS_COM nsTSubstring_CharT( PRUint32 flags );

        
        
      NS_COM nsTSubstring_CharT( const self_type& str );

        




      void NS_FASTCALL Finalize();

        

















      PRBool NS_FASTCALL MutatePrep( size_type capacity, char_type** old_data, PRUint32* old_flags );

        



















      PRBool NS_FASTCALL ReplacePrep( index_type cutStart, size_type cutLength, size_type newLength );

        





      size_type NS_FASTCALL Capacity() const;

        



      NS_COM PRBool NS_FASTCALL EnsureMutable( size_type newLen = size_type(-1) );

        


      PRBool IsDependentOn( const char_type *start, const char_type *end ) const
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

NS_COM
int NS_FASTCALL Compare( const nsTSubstring_CharT::base_string_type& lhs, const nsTSubstring_CharT::base_string_type& rhs, const nsTStringComparator_CharT& = nsTDefaultStringComparator_CharT() );


inline
PRBool operator!=( const nsTSubstring_CharT::base_string_type& lhs, const nsTSubstring_CharT::base_string_type& rhs )
  {
    return !lhs.Equals(rhs);
  }

inline
PRBool operator< ( const nsTSubstring_CharT::base_string_type& lhs, const nsTSubstring_CharT::base_string_type& rhs )
  {
    return Compare(lhs, rhs)< 0;
  }

inline
PRBool operator<=( const nsTSubstring_CharT::base_string_type& lhs, const nsTSubstring_CharT::base_string_type& rhs )
  {
    return Compare(lhs, rhs)<=0;
  }

inline
PRBool operator==( const nsTSubstring_CharT::base_string_type& lhs, const nsTSubstring_CharT::base_string_type& rhs )
  {
    return lhs.Equals(rhs);
  }

inline
PRBool operator>=( const nsTSubstring_CharT::base_string_type& lhs, const nsTSubstring_CharT::base_string_type& rhs )
  {
    return Compare(lhs, rhs)>=0;
  }

inline
PRBool operator> ( const nsTSubstring_CharT::base_string_type& lhs, const nsTSubstring_CharT::base_string_type& rhs )
  {
    return Compare(lhs, rhs)> 0;
  }
