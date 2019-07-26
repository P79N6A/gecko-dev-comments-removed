


















class nsTString_CharT : public nsTSubstring_CharT
{
public:

  typedef nsTString_CharT    self_type;

public:

  



  nsTString_CharT()
    : substring_type() {}

  explicit
  nsTString_CharT( const char_type* data, size_type length = size_type(-1) )
    : substring_type()
  {
    Assign(data, length);
  }

#if defined(CharT_is_PRUnichar) && defined(MOZ_USE_CHAR16_WRAPPER)
  explicit
  nsTString_CharT( char16ptr_t data, size_type length = size_type(-1) )
    : substring_type()
  {
    Assign(static_cast<const char16_t*>(data), length);
  }
#endif

  nsTString_CharT( const self_type& str )
    : substring_type()
  {
    Assign(str);
  }

  MOZ_IMPLICIT nsTString_CharT( const substring_tuple_type& tuple )
    : substring_type()
  {
    Assign(tuple);
  }

  explicit
  nsTString_CharT( const substring_type& readable )
    : substring_type()
  {
    Assign(readable);
  }


  
  self_type& operator=( char_type c )                       { Assign(c);     return *this; }
  self_type& operator=( const char_type* data )             { Assign(data);  return *this; }
  self_type& operator=( const self_type& str )              { Assign(str);   return *this; }
#if defined(CharT_is_PRUnichar) && defined(MOZ_USE_CHAR16_WRAPPER)
  self_type& operator=( const char16ptr_t data )            { Assign(static_cast<const char16_t*>(data)); return *this; }
#endif
  self_type& operator=( const substring_type& str )         { Assign(str);   return *this; }
  self_type& operator=( const substring_tuple_type& tuple ) { Assign(tuple); return *this; }

  



#if defined(CharT_is_PRUnichar) && defined(MOZ_USE_CHAR16_WRAPPER)
  char16ptr_t get() const
#else
  const char_type* get() const
#endif
  {
    return mData;
  }


  






  char_type CharAt( index_type i ) const
  {
    NS_ASSERTION(i <= mLength, "index exceeds allowable range");
    return mData[i];
  }

  char_type operator[]( index_type i ) const
  {
    return CharAt(i);
  }


#if MOZ_STRING_WITH_OBSOLETE_API


  










  int32_t Find( const nsCString& aString, bool aIgnoreCase=false, int32_t aOffset=0, int32_t aCount=-1 ) const;
  int32_t Find( const char* aString, bool aIgnoreCase=false, int32_t aOffset=0, int32_t aCount=-1 ) const;

#ifdef CharT_is_PRUnichar
  int32_t Find( const nsAFlatString& aString, int32_t aOffset=0, int32_t aCount=-1 ) const;
  int32_t Find( const char16_t* aString, int32_t aOffset=0, int32_t aCount=-1 ) const;
#ifdef MOZ_USE_CHAR16_WRAPPER
  int32_t Find( char16ptr_t aString, int32_t aOffset=0, int32_t aCount=-1 ) const
  {
    return Find(static_cast<const char16_t*>(aString), aOffset, aCount);
  }
#endif
#endif


  











  int32_t RFind( const nsCString& aString, bool aIgnoreCase=false, int32_t aOffset=-1, int32_t aCount=-1 ) const;
  int32_t RFind( const char* aCString, bool aIgnoreCase=false, int32_t aOffset=-1, int32_t aCount=-1 ) const;

#ifdef CharT_is_PRUnichar
  int32_t RFind( const nsAFlatString& aString, int32_t aOffset=-1, int32_t aCount=-1 ) const;
  int32_t RFind( const char16_t* aString, int32_t aOffset=-1, int32_t aCount=-1 ) const;
#endif


  









  
  int32_t RFindChar( char16_t aChar, int32_t aOffset=-1, int32_t aCount=-1 ) const;


  









  int32_t FindCharInSet( const char* aString, int32_t aOffset=0 ) const;
  int32_t FindCharInSet( const self_type& aString, int32_t aOffset=0 ) const
  {
    return FindCharInSet(aString.get(), aOffset);
  }

#ifdef CharT_is_PRUnichar
  int32_t FindCharInSet( const char16_t* aString, int32_t aOffset=0 ) const;
#endif


  









  int32_t RFindCharInSet( const char_type* aString, int32_t aOffset=-1 ) const;
  int32_t RFindCharInSet( const self_type& aString, int32_t aOffset=-1 ) const
  {
    return RFindCharInSet(aString.get(), aOffset);
  }


  








#ifdef CharT_is_char
  int32_t Compare( const char* aString, bool aIgnoreCase=false, int32_t aCount=-1 ) const;
#endif


  







#ifdef CharT_is_char
  bool EqualsIgnoreCase( const char* aString, int32_t aCount=-1 ) const {
    return Compare(aString, true, aCount) == 0;
  }
#else
  bool EqualsIgnoreCase( const char* aString, int32_t aCount=-1 ) const;


#endif 

  





  double ToDouble( nsresult* aErrorCode ) const;

  





  float ToFloat( nsresult* aErrorCode ) const {
    return (float)ToDouble(aErrorCode);
  }


  





  int32_t ToInteger( nsresult* aErrorCode, uint32_t aRadix=kRadix10 ) const;

  





  int64_t ToInteger64( nsresult* aErrorCode, uint32_t aRadix=kRadix10 ) const;


  
















  size_type Mid( self_type& aResult, uint32_t aStartPos, uint32_t aCount ) const;

  size_type Left( self_type& aResult, size_type aCount ) const
  {
    return Mid(aResult, 0, aCount);
  }

  size_type Right( self_type& aResult, size_type aCount ) const
  {
    aCount = XPCOM_MIN(mLength, aCount);
    return Mid(aResult, mLength - aCount, aCount);
  }


  







  bool SetCharAt( char16_t aChar, uint32_t aIndex );


  





  void StripChars( const char* aSet );


  


  void StripWhitespace();


  



  void ReplaceChar( char_type aOldChar, char_type aNewChar );
  void ReplaceChar( const char* aSet, char_type aNewChar );
#ifdef CharT_is_PRUnichar
  void ReplaceChar( const char16_t* aSet, char16_t aNewChar );
#endif
  void ReplaceSubstring( const self_type& aTarget, const self_type& aNewValue);
  void ReplaceSubstring( const char_type* aTarget, const char_type* aNewValue);


  









  void Trim( const char* aSet, bool aEliminateLeading=true, bool aEliminateTrailing=true, bool aIgnoreQuotes=false );

  







  void CompressWhitespace( bool aEliminateLeading=true, bool aEliminateTrailing=true );


  



  void AssignWithConversion( const nsTAString_IncompatibleCharT& aString );
  void AssignWithConversion( const incompatible_char_type* aData, int32_t aLength=-1 );

#endif 

  




  void Rebind( const char_type* data, size_type length );

  


  void AssertValidDepedentString()
  {
    NS_ASSERTION(mData, "nsTDependentString must wrap a non-NULL buffer");
    NS_ASSERTION(mLength != size_type(-1), "nsTDependentString has bogus length");
    NS_ASSERTION(mData[mLength] == 0, "nsTDependentString must wrap only null-terminated strings. You are probably looking for nsTDependentSubstring.");
  }


protected:

  explicit
  nsTString_CharT( uint32_t flags )
    : substring_type(flags) {}

  
  nsTString_CharT( char_type* data, size_type length, uint32_t flags )
    : substring_type(data, length, flags) {}
};


class nsTFixedString_CharT : public nsTString_CharT
{
public:

  typedef nsTFixedString_CharT    self_type;
  typedef nsTFixedString_CharT    fixed_string_type;

public:

  









  nsTFixedString_CharT( char_type* data, size_type storageSize )
    : string_type(data, uint32_t(char_traits::length(data)), F_TERMINATED | F_FIXED | F_CLASS_FIXED)
    , mFixedCapacity(storageSize - 1)
    , mFixedBuf(data)
  {}

  nsTFixedString_CharT( char_type* data, size_type storageSize, size_type length )
    : string_type(data, length, F_TERMINATED | F_FIXED | F_CLASS_FIXED)
    , mFixedCapacity(storageSize - 1)
    , mFixedBuf(data)
  {
    
    mFixedBuf[length] = char_type(0);
  }

  
  self_type& operator=( char_type c )                       { Assign(c);     return *this; }
  self_type& operator=( const char_type* data )             { Assign(data);  return *this; }
  self_type& operator=( const substring_type& str )         { Assign(str);   return *this; }
  self_type& operator=( const substring_tuple_type& tuple ) { Assign(tuple); return *this; }

protected:

  friend class nsTSubstring_CharT;

  size_type  mFixedCapacity;
  char_type *mFixedBuf;
};














class nsTAutoString_CharT : public nsTFixedString_CharT
{
public:

  typedef nsTAutoString_CharT    self_type;

public:

  



  nsTAutoString_CharT()
    : fixed_string_type(mStorage, kDefaultStorageSize, 0)
  {}

  explicit
  nsTAutoString_CharT( char_type c )
    : fixed_string_type(mStorage, kDefaultStorageSize, 0)
  {
    Assign(c);
  }

  explicit
  nsTAutoString_CharT( const char_type* data, size_type length = size_type(-1) )
    : fixed_string_type(mStorage, kDefaultStorageSize, 0)
  {
    Assign(data, length);
  }

#if defined(CharT_is_PRUnichar) && defined(MOZ_USE_CHAR16_WRAPPER)
  explicit
  nsTAutoString_CharT( char16ptr_t data, size_type length = size_type(-1) )
    : nsTAutoString_CharT(static_cast<const char16_t*>(data), length)
  {}
#endif

  nsTAutoString_CharT( const self_type& str )
    : fixed_string_type(mStorage, kDefaultStorageSize, 0)
  {
    Assign(str);
  }

  explicit
  nsTAutoString_CharT( const substring_type& str )
    : fixed_string_type(mStorage, kDefaultStorageSize, 0)
  {
    Assign(str);
  }

  MOZ_IMPLICIT nsTAutoString_CharT( const substring_tuple_type& tuple )
    : fixed_string_type(mStorage, kDefaultStorageSize, 0)
  {
    Assign(tuple);
  }

  
  self_type& operator=( char_type c )                       { Assign(c);     return *this; }
  self_type& operator=( const char_type* data )             { Assign(data);  return *this; }
#if defined(CharT_is_PRUnichar) && defined(MOZ_USE_CHAR16_WRAPPER)
  self_type& operator=( char16ptr_t data )                  { Assign(data);  return *this; }
#endif
  self_type& operator=( const self_type& str )              { Assign(str);   return *this; }
  self_type& operator=( const substring_type& str )         { Assign(str);   return *this; }
  self_type& operator=( const substring_tuple_type& tuple ) { Assign(tuple); return *this; }

  enum { kDefaultStorageSize = 64 };

private:

  char_type mStorage[kDefaultStorageSize];
};







template<class E> class nsTArrayElementTraits;
template<>
class nsTArrayElementTraits<nsTAutoString_CharT> {
public:
  template<class A> struct Dont_Instantiate_nsTArray_of;
  template<class A> struct Instead_Use_nsTArray_of;

  static Dont_Instantiate_nsTArray_of<nsTAutoString_CharT> *
  Construct(Instead_Use_nsTArray_of<nsTString_CharT> *e) {
    return 0;
  }
  template<class A>
  static Dont_Instantiate_nsTArray_of<nsTAutoString_CharT> *
  Construct(Instead_Use_nsTArray_of<nsTString_CharT> *e,
            const A &arg) {
    return 0;
  }
  static Dont_Instantiate_nsTArray_of<nsTAutoString_CharT> *
  Destruct(Instead_Use_nsTArray_of<nsTString_CharT> *e) {
    return 0;
  }
};













class nsTXPIDLString_CharT : public nsTString_CharT
{
public:

  typedef nsTXPIDLString_CharT    self_type;

public:

  nsTXPIDLString_CharT()
    : string_type(char_traits::sEmptyBuffer, 0, F_TERMINATED | F_VOIDED) {}

  
  nsTXPIDLString_CharT( const self_type& str )
    : string_type(char_traits::sEmptyBuffer, 0, F_TERMINATED | F_VOIDED)
  {
    Assign(str);
  }

  
#if defined(CharT_is_PRUnichar) && defined(MOZ_USE_CHAR16_WRAPPER)
  char16ptr_t get() const
#else
  const char_type* get() const
#endif
  {
    return (mFlags & F_VOIDED) ? nullptr : mData;
  }

  
  
  operator const char_type*() const
  {
    return get();
  }

  
  char_type operator[]( int32_t i ) const
  {
    return CharAt(index_type(i));
  }

  
  self_type& operator=( char_type c )                       { Assign(c);     return *this; }
  self_type& operator=( const char_type* data )             { Assign(data);  return *this; }
  self_type& operator=( const self_type& str )              { Assign(str);   return *this; }
  self_type& operator=( const substring_type& str )         { Assign(str);   return *this; }
  self_type& operator=( const substring_tuple_type& tuple ) { Assign(tuple); return *this; }
};














class MOZ_STACK_CLASS nsTGetterCopies_CharT
{
public:
  typedef CharT char_type;

  explicit nsTGetterCopies_CharT(nsTSubstring_CharT& str)
    : mString(str), mData(nullptr) {}

  ~nsTGetterCopies_CharT()
  {
    mString.Adopt(mData); 
  }

  operator char_type**()
  {
    return &mData;
  }

private:
  nsTSubstring_CharT&      mString;
  char_type*            mData;
};

inline
nsTGetterCopies_CharT
getter_Copies( nsTSubstring_CharT& aString )
{
  return nsTGetterCopies_CharT(aString);
}










class nsTAdoptingString_CharT : public nsTXPIDLString_CharT
{
public:

  typedef nsTAdoptingString_CharT    self_type;

public:

  explicit nsTAdoptingString_CharT() {}
  explicit nsTAdoptingString_CharT(char_type* str, size_type length = size_type(-1))
  {
    Adopt(str, length);
  }

  
  
  
  
  nsTAdoptingString_CharT( const self_type& str )
  {
    *this = str;
  }

  
  self_type& operator=( const substring_type& str )         { Assign(str);   return *this; }
  self_type& operator=( const substring_tuple_type& tuple ) { Assign(tuple); return *this; }

  
  
  
  self_type& operator=( const self_type& str );

private:
  self_type& operator=( const char_type* data ) MOZ_DELETE;
  self_type& operator=( char_type* data ) MOZ_DELETE;
};

