






































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


  






























class nsTAString_CharT
  {
    public:

      typedef CharT                                  char_type;
      typedef nsCharTraits<char_type>                char_traits;

      typedef char_traits::incompatible_char_type    incompatible_char_type;

      typedef nsTAString_CharT                       self_type;
      typedef nsTAString_CharT                       abstract_string_type;
      typedef nsTObsoleteAString_CharT               obsolete_string_type;
      typedef nsTSubstring_CharT                     substring_type;
      typedef nsTSubstringTuple_CharT                substring_tuple_type;

      typedef nsReadingIterator<char_type>           const_iterator;
      typedef nsWritingIterator<char_type>           iterator;

      typedef nsTStringComparator_CharT              comparator_type;

      typedef PRUint32                               size_type;
      typedef PRUint32                               index_type;

#ifdef MOZ_V1_STRING_ABI
    public:

        
      NS_COM NS_CONSTRUCTOR_FASTCALL ~nsTAString_CharT();


        






      inline const_iterator& BeginReading( const_iterator& iter ) const
        {
          size_type len = GetReadableBuffer(&iter.mStart);
          iter.mEnd = iter.mStart + len;
          iter.mPosition = iter.mStart;
          return iter;
        }

      inline const_iterator& EndReading( const_iterator& iter ) const
        {
          size_type len = GetReadableBuffer(&iter.mStart);
          iter.mEnd = iter.mStart + len;
          iter.mPosition = iter.mEnd;
          return iter;
        }

      inline const char_type* BeginReading() const
        {
          const char_type *b;
          GetReadableBuffer(&b);
          return b;
        }

      inline const char_type* EndReading() const
        {
          const char_type *b;
          size_type len = GetReadableBuffer(&b);
          return b + len;
        }


        








      inline iterator& BeginWriting( iterator& iter )
        {
          size_type len = GetWritableBuffer(&iter.mStart);
          iter.mEnd = iter.mStart + len;
          iter.mPosition = iter.mStart;
          return iter;
        }

      inline iterator& EndWriting( iterator& iter )
        {
          size_type len = GetWritableBuffer(&iter.mStart);
          iter.mEnd = iter.mStart + len;
          iter.mPosition = iter.mEnd;
          return iter;
        }

      inline char_type* BeginWriting()
        {
          char_type *b;
          GetWritableBuffer(&b);
          return b;
        }

      inline char_type* EndWriting()
        {
          char_type *b;
          size_type len = GetWritableBuffer(&b);
          return b ? b + len : nsnull;
        }


        





      inline size_type GetData( const char_type** data ) const
        {
          return GetReadableBuffer(data);
        }

        









      inline size_type GetMutableData( char_type** data, size_type newLen = size_type(-1) )
        {
          return GetWritableBuffer(data, newLen);
        }


        




      NS_COM size_type NS_FASTCALL Length() const;
      PRBool IsEmpty() const { return Length() == 0; }


        





      NS_COM PRBool NS_FASTCALL Equals( const self_type& ) const;
      NS_COM PRBool NS_FASTCALL Equals( const self_type&, const comparator_type& ) const;
      NS_COM PRBool NS_FASTCALL Equals( const char_type* ) const;
      NS_COM PRBool NS_FASTCALL Equals( const char_type*, const comparator_type& ) const;

        




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

        







      NS_COM PRBool NS_FASTCALL IsVoid() const;
      NS_COM void NS_FASTCALL SetIsVoid( PRBool );


        






      NS_COM PRBool NS_FASTCALL IsTerminated() const;


        


      NS_COM char_type NS_FASTCALL First() const;
      NS_COM char_type NS_FASTCALL Last() const;


        


      NS_COM size_type NS_FASTCALL CountChar( char_type ) const;


        




      NS_COM PRInt32 NS_FASTCALL FindChar( char_type, index_type offset = 0 ) const;


        









      NS_COM void NS_FASTCALL SetCapacity( size_type newCapacity );


        









      NS_COM void NS_FASTCALL SetLength( size_type newLen );


        


      void Truncate( size_type newLen = 0 )
        {
          NS_ASSERTION(newLen <= Length(), "Truncate cannot make string longer");
          SetLength(newLen);
        }


        






      NS_COM void NS_FASTCALL Assign( const self_type& readable );
      NS_COM void NS_FASTCALL Assign( const substring_tuple_type& tuple );
      NS_COM void NS_FASTCALL Assign( const char_type* data );
      NS_COM void NS_FASTCALL Assign( const char_type* data, size_type length );
      NS_COM void NS_FASTCALL Assign( char_type c );

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

        
      self_type& operator=( const self_type& readable )                                             { Assign(readable); return *this; }
      self_type& operator=( const substring_tuple_type& tuple )                                     { Assign(tuple); return *this; }
      self_type& operator=( const char_type* data )                                                 { Assign(data); return *this; }
      self_type& operator=( char_type c )                                                           { Assign(c); return *this; }



        

 

      NS_COM void NS_FASTCALL Append( const self_type& readable );
      NS_COM void NS_FASTCALL Append( const substring_tuple_type& tuple );
      NS_COM void NS_FASTCALL Append( const char_type* data );
      NS_COM void NS_FASTCALL Append( const char_type* data, size_type length );
      NS_COM void NS_FASTCALL Append( char_type c );

      NS_COM void NS_FASTCALL AppendASCII( const char* data, size_type length );
      NS_COM void NS_FASTCALL AppendASCII( const char* data );

    
    
    
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

      self_type& operator+=( const self_type& readable )                                            { Append(readable); return *this; }
      self_type& operator+=( const substring_tuple_type& tuple )                                    { Append(tuple); return *this; }
      self_type& operator+=( const char_type* data )                                                { Append(data); return *this; }
      self_type& operator+=( char_type c )                                                          { Append(c); return *this; }


        


 

      NS_COM void NS_FASTCALL Insert( const self_type& readable, index_type pos );
      NS_COM void NS_FASTCALL Insert( const substring_tuple_type& tuple, index_type pos );
      NS_COM void NS_FASTCALL Insert( const char_type* data, index_type pos );
      NS_COM void NS_FASTCALL Insert( const char_type* data, index_type pos, size_type length );
      NS_COM void NS_FASTCALL Insert( char_type c, index_type pos );


        



      NS_COM void NS_FASTCALL Cut( index_type cutStart, size_type cutLength );

      
        



      NS_COM void NS_FASTCALL Replace( index_type cutStart, size_type cutLength, const self_type& readable );
      NS_COM void NS_FASTCALL Replace( index_type cutStart, size_type cutLength, const substring_tuple_type& readable );

      
        



      nsTAString_CharT(const substring_tuple_type& tuple)
        : mVTable(obsolete_string_type::sCanonicalVTable)
        , mData(nsnull)
        , mLength(0)
        , mFlags(0)
        {
          NS_ASSERTION(mVTable, "mVTable is null! Is this a static string instance?!");
          Assign(tuple);
        }

    protected:

      friend class nsTSubstringTuple_CharT;

      
      friend class nsTSubstring_CharT;
      friend class nsTDependentSubstring_CharT;
      friend class nsTPromiseFlatString_CharT;

        



      const void* mVTable;

        





      char_type*  mData;
      size_type   mLength;
      PRUint32    mFlags;

        


      nsTAString_CharT(char_type* data, size_type length, PRUint32 flags)
        : mVTable(obsolete_string_type::sCanonicalVTable)
        , mData(data)
        , mLength(length)
        , mFlags(flags)
        {
          NS_ASSERTION(mVTable, "mVTable is null! Is this a static string instance?!");
        }

        




      explicit
      nsTAString_CharT(PRUint32 flags)
        : mVTable(obsolete_string_type::sCanonicalVTable)
        , mFlags(flags)
        {
          NS_ASSERTION(mVTable, "mVTable is null! Is this a static string instance?!");
        }

        



      NS_COM size_type NS_FASTCALL GetReadableBuffer( const char_type **data ) const;
      NS_COM size_type NS_FASTCALL GetWritableBuffer(       char_type **data, size_type newLen = size_type(-1) );

        



      PRBool NS_FASTCALL IsDependentOn(const char_type *start, const char_type *end) const;

        


      const substring_type NS_FASTCALL ToSubstring() const;

        



      const obsolete_string_type* AsObsoleteString() const
        {
          return reinterpret_cast<const obsolete_string_type*>(this);
        }

      obsolete_string_type* AsObsoleteString()
        {
          return reinterpret_cast<obsolete_string_type*>(this);
        }

      const substring_type* AsSubstring() const
        {
          return reinterpret_cast<const substring_type*>(this);
        }

      substring_type* AsSubstring()
        {
          return reinterpret_cast<substring_type*>(this);
        }

    private:

        
        
        
        
        
        
#if !defined(__SUNPRO_CC) && \
   !(defined(_AIX) && defined(__IBMCPP__)) && \
   (!defined(__GNUC__) || __GNUC__ > 2 || __GNUC_MINOR__ > 95)

        
      nsTAString_CharT( const self_type& );

#endif

        
      void operator=     ( incompatible_char_type );
      void Assign        ( incompatible_char_type );
      void operator+=    ( incompatible_char_type );
      void Append        ( incompatible_char_type );
      void Insert        ( incompatible_char_type, index_type );
#endif
  };
