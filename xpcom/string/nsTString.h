


















class nsTString_CharT : public nsTSubstring_CharT
{
public:

  typedef nsTString_CharT self_type;

public:

  



  nsTString_CharT()
    : substring_type()
  {
  }

  explicit
  nsTString_CharT(const char_type* aData, size_type aLength = size_type(-1))
    : substring_type()
  {
    Assign(aData, aLength);
  }

#if defined(CharT_is_PRUnichar) && defined(MOZ_USE_CHAR16_WRAPPER)
  explicit
  nsTString_CharT(char16ptr_t aStr, size_type aLength = size_type(-1))
    : substring_type()
  {
    Assign(static_cast<const char16_t*>(aStr), aLength);
  }
#endif

  nsTString_CharT(const self_type& aStr)
    : substring_type()
  {
    Assign(aStr);
  }

  MOZ_IMPLICIT nsTString_CharT(const substring_tuple_type& aTuple)
    : substring_type()
  {
    Assign(aTuple);
  }

  explicit
  nsTString_CharT(const substring_type& aReadable)
    : substring_type()
  {
    Assign(aReadable);
  }


  
  self_type& operator=(char_type aChar)
  {
    Assign(aChar);
    return *this;
  }
  self_type& operator=(const char_type* aData)
  {
    Assign(aData);
    return *this;
  }
  self_type& operator=(const self_type& aStr)
  {
    Assign(aStr);
    return *this;
  }
#if defined(CharT_is_PRUnichar) && defined(MOZ_USE_CHAR16_WRAPPER)
  self_type& operator=(const char16ptr_t aStr)
  {
    Assign(static_cast<const char16_t*>(aStr));
    return *this;
  }
#endif
  self_type& operator=(const substring_type& aStr)
  {
    Assign(aStr);
    return *this;
  }
  self_type& operator=(const substring_tuple_type& aTuple)
  {
    Assign(aTuple);
    return *this;
  }

  



#if defined(CharT_is_PRUnichar) && defined(MOZ_USE_CHAR16_WRAPPER)
  char16ptr_t get() const
#else
  const char_type* get() const
#endif
  {
    return mData;
  }


  






  char_type CharAt(index_type aIndex) const
  {
    NS_ASSERTION(aIndex <= mLength, "index exceeds allowable range");
    return mData[aIndex];
  }

  char_type operator[](index_type aIndex) const
  {
    return CharAt(aIndex);
  }


#if MOZ_STRING_WITH_OBSOLETE_API


  










  int32_t Find(const nsCString& aString, bool aIgnoreCase = false,
               int32_t aOffset = 0, int32_t aCount = -1) const;
  int32_t Find(const char* aString, bool aIgnoreCase = false,
               int32_t aOffset = 0, int32_t aCount = -1) const;

#ifdef CharT_is_PRUnichar
  int32_t Find(const nsAFlatString& aString, int32_t aOffset = 0,
               int32_t aCount = -1) const;
  int32_t Find(const char16_t* aString, int32_t aOffset = 0,
               int32_t aCount = -1) const;
#ifdef MOZ_USE_CHAR16_WRAPPER
  int32_t Find(char16ptr_t aString, int32_t aOffset = 0,
               int32_t aCount = -1) const
  {
    return Find(static_cast<const char16_t*>(aString), aOffset, aCount);
  }
#endif
#endif


  











  int32_t RFind(const nsCString& aString, bool aIgnoreCase = false,
                int32_t aOffset = -1, int32_t aCount = -1) const;
  int32_t RFind(const char* aCString, bool aIgnoreCase = false,
                int32_t aOffset = -1, int32_t aCount = -1) const;

#ifdef CharT_is_PRUnichar
  int32_t RFind(const nsAFlatString& aString, int32_t aOffset = -1,
                int32_t aCount = -1) const;
  int32_t RFind(const char16_t* aString, int32_t aOffset = -1,
                int32_t aCount = -1) const;
#endif


  









  
  int32_t RFindChar(char16_t aChar, int32_t aOffset = -1,
                    int32_t aCount = -1) const;


  









  int32_t FindCharInSet(const char* aString, int32_t aOffset = 0) const;
  int32_t FindCharInSet(const self_type& aString, int32_t aOffset = 0) const
  {
    return FindCharInSet(aString.get(), aOffset);
  }

#ifdef CharT_is_PRUnichar
  int32_t FindCharInSet(const char16_t* aString, int32_t aOffset = 0) const;
#endif


  









  int32_t RFindCharInSet(const char_type* aString, int32_t aOffset = -1) const;
  int32_t RFindCharInSet(const self_type& aString, int32_t aOffset = -1) const
  {
    return RFindCharInSet(aString.get(), aOffset);
  }


  








#ifdef CharT_is_char
  int32_t Compare(const char* aString, bool aIgnoreCase = false,
                  int32_t aCount = -1) const;
#endif


  







#ifdef CharT_is_char
  bool EqualsIgnoreCase(const char* aString, int32_t aCount = -1) const
  {
    return Compare(aString, true, aCount) == 0;
  }
#else
  bool EqualsIgnoreCase(const char* aString, int32_t aCount = -1) const;


#endif 

  





  double ToDouble(nsresult* aErrorCode) const;

  





  float ToFloat(nsresult* aErrorCode) const
  {
    return (float)ToDouble(aErrorCode);
  }


  





  int32_t ToInteger(nsresult* aErrorCode, uint32_t aRadix = kRadix10) const;

  





  int64_t ToInteger64(nsresult* aErrorCode, uint32_t aRadix = kRadix10) const;


  
















  size_type Mid(self_type& aResult, uint32_t aStartPos, uint32_t aCount) const;

  size_type Left(self_type& aResult, size_type aCount) const
  {
    return Mid(aResult, 0, aCount);
  }

  size_type Right(self_type& aResult, size_type aCount) const
  {
    aCount = XPCOM_MIN(mLength, aCount);
    return Mid(aResult, mLength - aCount, aCount);
  }


  







  bool SetCharAt(char16_t aChar, uint32_t aIndex);


  





  void StripChars(const char* aSet);


  


  void StripWhitespace();


  



  void ReplaceChar(char_type aOldChar, char_type aNewChar);
  void ReplaceChar(const char* aSet, char_type aNewChar);
#ifdef CharT_is_PRUnichar
  void ReplaceChar(const char16_t* aSet, char16_t aNewChar);
#endif
  




  void ReplaceSubstring(const self_type& aTarget, const self_type& aNewValue);
  void ReplaceSubstring(const char_type* aTarget, const char_type* aNewValue);
  MOZ_WARN_UNUSED_RESULT bool ReplaceSubstring(const self_type& aTarget,
                                               const self_type& aNewValue,
                                               const fallible_t&);
  MOZ_WARN_UNUSED_RESULT bool ReplaceSubstring(const char_type* aTarget,
                                               const char_type* aNewValue,
                                               const fallible_t&);


  









  void Trim(const char* aSet, bool aEliminateLeading = true,
            bool aEliminateTrailing = true, bool aIgnoreQuotes = false);

  







  void CompressWhitespace(bool aEliminateLeading = true,
                          bool aEliminateTrailing = true);


  



  void AssignWithConversion(const nsTAString_IncompatibleCharT& aString);
  void AssignWithConversion(const incompatible_char_type* aData,
                            int32_t aLength = -1);

#endif 

  




  void Rebind(const char_type* aData, size_type aLength);

  


  void AssertValidDependentString()
  {
    NS_ASSERTION(mData, "nsTDependentString must wrap a non-NULL buffer");
    NS_ASSERTION(mLength != size_type(-1), "nsTDependentString has bogus length");
    NS_ASSERTION(mData[mLength] == 0,
                 "nsTDependentString must wrap only null-terminated strings. "
                 "You are probably looking for nsTDependentSubstring.");
  }


protected:

  explicit
  nsTString_CharT(uint32_t aFlags)
    : substring_type(aFlags)
  {
  }

  
  nsTString_CharT(char_type* aData, size_type aLength, uint32_t aFlags)
    : substring_type(aData, aLength, aFlags)
  {
  }

  struct Segment {
    uint32_t mBegin, mLength;
    Segment(uint32_t aBegin, uint32_t aLength)
      : mBegin(aBegin)
      , mLength(aLength)
    {}
  };
};


class nsTFixedString_CharT : public nsTString_CharT
{
public:

  typedef nsTFixedString_CharT self_type;
  typedef nsTFixedString_CharT fixed_string_type;

public:

  









  nsTFixedString_CharT(char_type* aData, size_type aStorageSize)
    : string_type(aData, uint32_t(char_traits::length(aData)),
                  F_TERMINATED | F_FIXED | F_CLASS_FIXED)
    , mFixedCapacity(aStorageSize - 1)
    , mFixedBuf(aData)
  {
  }

  nsTFixedString_CharT(char_type* aData, size_type aStorageSize,
                       size_type aLength)
    : string_type(aData, aLength, F_TERMINATED | F_FIXED | F_CLASS_FIXED)
    , mFixedCapacity(aStorageSize - 1)
    , mFixedBuf(aData)
  {
    
    mFixedBuf[aLength] = char_type(0);
  }

  
  self_type& operator=(char_type aChar)
  {
    Assign(aChar);
    return *this;
  }
  self_type& operator=(const char_type* aData)
  {
    Assign(aData);
    return *this;
  }
  self_type& operator=(const substring_type& aStr)
  {
    Assign(aStr);
    return *this;
  }
  self_type& operator=(const substring_tuple_type& aTuple)
  {
    Assign(aTuple);
    return *this;
  }

protected:

  friend class nsTSubstring_CharT;

  size_type  mFixedCapacity;
  char_type* mFixedBuf;
};














class nsTAutoString_CharT : public nsTFixedString_CharT
{
public:

  typedef nsTAutoString_CharT self_type;

public:

  



  nsTAutoString_CharT()
    : fixed_string_type(mStorage, kDefaultStorageSize, 0)
  {
  }

  explicit
  nsTAutoString_CharT(char_type aChar)
    : fixed_string_type(mStorage, kDefaultStorageSize, 0)
  {
    Assign(aChar);
  }

  explicit
  nsTAutoString_CharT(const char_type* aData, size_type aLength = size_type(-1))
    : fixed_string_type(mStorage, kDefaultStorageSize, 0)
  {
    Assign(aData, aLength);
  }

#if defined(CharT_is_PRUnichar) && defined(MOZ_USE_CHAR16_WRAPPER)
  explicit
  nsTAutoString_CharT(char16ptr_t aData, size_type aLength = size_type(-1))
    : nsTAutoString_CharT(static_cast<const char16_t*>(aData), aLength)
  {
  }
#endif

  nsTAutoString_CharT(const self_type& aStr)
    : fixed_string_type(mStorage, kDefaultStorageSize, 0)
  {
    Assign(aStr);
  }

  explicit
  nsTAutoString_CharT(const substring_type& aStr)
    : fixed_string_type(mStorage, kDefaultStorageSize, 0)
  {
    Assign(aStr);
  }

  MOZ_IMPLICIT nsTAutoString_CharT(const substring_tuple_type& aTuple)
    : fixed_string_type(mStorage, kDefaultStorageSize, 0)
  {
    Assign(aTuple);
  }

  
  self_type& operator=(char_type aChar)
  {
    Assign(aChar);
    return *this;
  }
  self_type& operator=(const char_type* aData)
  {
    Assign(aData);
    return *this;
  }
#if defined(CharT_is_PRUnichar) && defined(MOZ_USE_CHAR16_WRAPPER)
  self_type& operator=(char16ptr_t aStr)
  {
    Assign(aStr);
    return *this;
  }
#endif
  self_type& operator=(const self_type& aStr)
  {
    Assign(aStr);
    return *this;
  }
  self_type& operator=(const substring_type& aStr)
  {
    Assign(aStr);
    return *this;
  }
  self_type& operator=(const substring_tuple_type& aTuple)
  {
    Assign(aTuple);
    return *this;
  }

  enum
  {
    kDefaultStorageSize = 64
  };

private:

  char_type mStorage[kDefaultStorageSize];
};







template<class E> class nsTArrayElementTraits;
template<>
class nsTArrayElementTraits<nsTAutoString_CharT>
{
public:
  template<class A> struct Dont_Instantiate_nsTArray_of;
  template<class A> struct Instead_Use_nsTArray_of;

  static Dont_Instantiate_nsTArray_of<nsTAutoString_CharT>*
  Construct(Instead_Use_nsTArray_of<nsTString_CharT>* aE)
  {
    return 0;
  }
  template<class A>
  static Dont_Instantiate_nsTArray_of<nsTAutoString_CharT>*
  Construct(Instead_Use_nsTArray_of<nsTString_CharT>* aE, const A& aArg)
  {
    return 0;
  }
  static Dont_Instantiate_nsTArray_of<nsTAutoString_CharT>*
  Destruct(Instead_Use_nsTArray_of<nsTString_CharT>* aE)
  {
    return 0;
  }
};













class nsTXPIDLString_CharT : public nsTString_CharT
{
public:

  typedef nsTXPIDLString_CharT self_type;

public:

  nsTXPIDLString_CharT()
    : string_type(char_traits::sEmptyBuffer, 0, F_TERMINATED | F_VOIDED)
  {
  }

  
  nsTXPIDLString_CharT(const self_type& aStr)
    : string_type(char_traits::sEmptyBuffer, 0, F_TERMINATED | F_VOIDED)
  {
    Assign(aStr);
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

  
  char_type operator[](int32_t aIndex) const
  {
    return CharAt(index_type(aIndex));
  }

  
  self_type& operator=(char_type aChar)
  {
    Assign(aChar);
    return *this;
  }
  self_type& operator=(const char_type* aStr)
  {
    Assign(aStr);
    return *this;
  }
  self_type& operator=(const self_type& aStr)
  {
    Assign(aStr);
    return *this;
  }
  self_type& operator=(const substring_type& aStr)
  {
    Assign(aStr);
    return *this;
  }
  self_type& operator=(const substring_tuple_type& aTuple)
  {
    Assign(aTuple);
    return *this;
  }
};














class MOZ_STACK_CLASS nsTGetterCopies_CharT
{
public:
  typedef CharT char_type;

  explicit nsTGetterCopies_CharT(nsTSubstring_CharT& aStr)
    : mString(aStr)
    , mData(nullptr)
  {
  }

  ~nsTGetterCopies_CharT()
  {
    mString.Adopt(mData); 
  }

  operator char_type**()
  {
    return &mData;
  }

private:
  nsTSubstring_CharT& mString;
  char_type* mData;
};

inline nsTGetterCopies_CharT
getter_Copies(nsTSubstring_CharT& aString)
{
  return nsTGetterCopies_CharT(aString);
}










class nsTAdoptingString_CharT : public nsTXPIDLString_CharT
{
public:

  typedef nsTAdoptingString_CharT self_type;

public:

  explicit nsTAdoptingString_CharT()
  {
  }
  explicit nsTAdoptingString_CharT(char_type* aStr,
                                   size_type aLength = size_type(-1))
  {
    Adopt(aStr, aLength);
  }

  
  
  
  
  nsTAdoptingString_CharT(const self_type& aStr)
  {
    *this = aStr;
  }

  
  self_type& operator=(const substring_type& aStr)
  {
    Assign(aStr);
    return *this;
  }
  self_type& operator=(const substring_tuple_type& aTuple)
  {
    Assign(aTuple);
    return *this;
  }

  
  
  
  self_type& operator=(const self_type& aStr);

private:
  self_type& operator=(const char_type* aData) = delete;
  self_type& operator=(char_type* aData) = delete;
};

