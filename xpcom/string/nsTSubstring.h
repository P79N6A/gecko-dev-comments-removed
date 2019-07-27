






#include "mozilla/Casting.h"
#include "mozilla/MemoryReporting.h"

#ifndef MOZILLA_INTERNAL_API
#error Cannot use internal string classes without MOZILLA_INTERNAL_API defined. Use the frozen header nsStringAPI.h instead.
#endif




class nsTStringComparator_CharT
{
public:
  typedef CharT char_type;

  nsTStringComparator_CharT()
  {
  }

  virtual int operator()(const char_type*, const char_type*,
                         uint32_t, uint32_t) const = 0;
};





class nsTDefaultStringComparator_CharT
  : public nsTStringComparator_CharT
{
public:
  typedef CharT char_type;

  nsTDefaultStringComparator_CharT()
  {
  }

  virtual int operator()(const char_type*, const char_type*,
                         uint32_t, uint32_t) const override;
};













class nsTSubstring_CharT
{
public:
  typedef mozilla::fallible_t                 fallible_t;

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

  typedef uint32_t                            size_type;
  typedef uint32_t                            index_type;

public:

  
  ~nsTSubstring_CharT()
  {
    Finalize();
  }

  



  const_char_iterator BeginReading() const
  {
    return mData;
  }
  const_char_iterator EndReading() const
  {
    return mData + mLength;
  }

  



  const_iterator& BeginReading(const_iterator& aIter) const
  {
    aIter.mStart = mData;
    aIter.mEnd = mData + mLength;
    aIter.mPosition = aIter.mStart;
    return aIter;
  }

  const_iterator& EndReading(const_iterator& aIter) const
  {
    aIter.mStart = mData;
    aIter.mEnd = mData + mLength;
    aIter.mPosition = aIter.mEnd;
    return aIter;
  }

  const_char_iterator& BeginReading(const_char_iterator& aIter) const
  {
    return aIter = mData;
  }

  const_char_iterator& EndReading(const_char_iterator& aIter) const
  {
    return aIter = mData + mLength;
  }


  



  char_iterator BeginWriting()
  {
    if (!EnsureMutable()) {
      AllocFailed(mLength);
    }

    return mData;
  }

  char_iterator BeginWriting(const fallible_t&)
  {
    return EnsureMutable() ? mData : char_iterator(0);
  }

  char_iterator EndWriting()
  {
    if (!EnsureMutable()) {
      AllocFailed(mLength);
    }

    return mData + mLength;
  }

  char_iterator EndWriting(const fallible_t&)
  {
    return EnsureMutable() ? (mData + mLength) : char_iterator(0);
  }

  char_iterator& BeginWriting(char_iterator& aIter)
  {
    return aIter = BeginWriting();
  }

  char_iterator& BeginWriting(char_iterator& aIter, const fallible_t& aFallible)
  {
    return aIter = BeginWriting(aFallible);
  }

  char_iterator& EndWriting(char_iterator& aIter)
  {
    return aIter = EndWriting();
  }

  char_iterator& EndWriting(char_iterator& aIter, const fallible_t& aFallible)
  {
    return aIter = EndWriting(aFallible);
  }

  



  iterator& BeginWriting(iterator& aIter)
  {
    char_type* data = BeginWriting();
    aIter.mStart = data;
    aIter.mEnd = data + mLength;
    aIter.mPosition = aIter.mStart;
    return aIter;
  }

  iterator& EndWriting(iterator& aIter)
  {
    char_type* data = BeginWriting();
    aIter.mStart = data;
    aIter.mEnd = data + mLength;
    aIter.mPosition = aIter.mEnd;
    return aIter;
  }

  



  
#if defined(CharT_is_PRUnichar) && defined(MOZ_USE_CHAR16_WRAPPER)
  char16ptr_t Data() const
#else
  const char_type* Data() const
#endif
  {
    return mData;
  }

  size_type Length() const
  {
    return mLength;
  }

  uint32_t Flags() const
  {
    return mFlags;
  }

  bool IsEmpty() const
  {
    return mLength == 0;
  }

  bool IsLiteral() const
  {
    return (mFlags & F_LITERAL) != 0;
  }

  bool IsVoid() const
  {
    return (mFlags & F_VOIDED) != 0;
  }

  bool IsTerminated() const
  {
    return (mFlags & F_TERMINATED) != 0;
  }

  char_type CharAt(index_type aIndex) const
  {
    NS_ASSERTION(aIndex < mLength, "index exceeds allowable range");
    return mData[aIndex];
  }

  char_type operator[](index_type aIndex) const
  {
    return CharAt(aIndex);
  }

  char_type First() const
  {
    NS_ASSERTION(mLength > 0, "|First()| called on an empty string");
    return mData[0];
  }

  inline   char_type Last() const
  {
    NS_ASSERTION(mLength > 0, "|Last()| called on an empty string");
    return mData[mLength - 1];
  }

  size_type NS_FASTCALL CountChar(char_type) const;
  int32_t NS_FASTCALL FindChar(char_type, index_type aOffset = 0) const;


  



  bool NS_FASTCALL Equals(const self_type&) const;
  bool NS_FASTCALL Equals(const self_type&, const comparator_type&) const;

  bool NS_FASTCALL Equals(const char_type* aData) const;
  bool NS_FASTCALL Equals(const char_type* aData,
                          const comparator_type& aComp) const;

#if defined(CharT_is_PRUnichar) && defined(MOZ_USE_CHAR16_WRAPPER)
  bool NS_FASTCALL Equals(char16ptr_t aData) const
  {
    return Equals(static_cast<const char16_t*>(aData));
  }
  bool NS_FASTCALL Equals(char16ptr_t aData, const comparator_type& aComp) const
  {
    return Equals(static_cast<const char16_t*>(aData), aComp);
  }
#endif

  




  bool NS_FASTCALL EqualsASCII(const char* aData, size_type aLen) const;
  




  bool NS_FASTCALL EqualsASCII(const char* aData) const;

  
  
  
  
  
  
  template<int N>
  inline bool EqualsLiteral(const char (&aStr)[N]) const
  {
    return EqualsASCII(aStr, N - 1);
  }

  
  
  
  
  
  
  bool NS_FASTCALL LowerCaseEqualsASCII(const char* aData,
                                        size_type aLen) const;
  bool NS_FASTCALL LowerCaseEqualsASCII(const char* aData) const;

  
  
  
  
  
  template<int N>
  inline bool LowerCaseEqualsLiteral(const char (&aStr)[N]) const
  {
    return LowerCaseEqualsASCII(aStr, N - 1);
  }

  



  void NS_FASTCALL Assign(char_type aChar);
  MOZ_WARN_UNUSED_RESULT bool NS_FASTCALL Assign(char_type aChar,
                                                 const fallible_t&);

  void NS_FASTCALL Assign(const char_type* aData);
  void NS_FASTCALL Assign(const char_type* aData, size_type aLength);
  MOZ_WARN_UNUSED_RESULT bool NS_FASTCALL Assign(const char_type* aData,
                                                 size_type aLength,
                                                 const fallible_t&);

  void NS_FASTCALL Assign(const self_type&);
  MOZ_WARN_UNUSED_RESULT bool NS_FASTCALL Assign(const self_type&,
                                                 const fallible_t&);

  void NS_FASTCALL Assign(const substring_tuple_type&);
  MOZ_WARN_UNUSED_RESULT bool NS_FASTCALL Assign(const substring_tuple_type&,
                                                 const fallible_t&);

#if defined(CharT_is_PRUnichar) && defined(MOZ_USE_CHAR16_WRAPPER)
  void Assign(char16ptr_t aData)
  {
    Assign(static_cast<const char16_t*>(aData));
  }

  MOZ_WARN_UNUSED_RESULT bool Assign(char16ptr_t aData,
                                     const fallible_t& aFallible)
  {
    return Assign(static_cast<const char16_t*>(aData), aFallible);
  }

  void Assign(char16ptr_t aData, size_type aLength)
  {
    Assign(static_cast<const char16_t*>(aData), aLength);
  }

  MOZ_WARN_UNUSED_RESULT bool Assign(char16ptr_t aData, size_type aLength,
                                     const fallible_t& aFallible)
  {
    return Assign(static_cast<const char16_t*>(aData), aLength,
                  aFallible);
  }
#endif

  void NS_FASTCALL AssignASCII(const char* aData, size_type aLength);
  MOZ_WARN_UNUSED_RESULT bool NS_FASTCALL AssignASCII(const char* aData,
                                                      size_type aLength,
                                                      const fallible_t&);

  void NS_FASTCALL AssignASCII(const char* aData)
  {
    AssignASCII(aData, mozilla::AssertedCast<size_type, size_t>(strlen(aData)));
  }
  MOZ_WARN_UNUSED_RESULT bool NS_FASTCALL AssignASCII(const char* aData,
                                                      const fallible_t& aFallible)
  {
    return AssignASCII(aData,
                       mozilla::AssertedCast<size_type, size_t>(strlen(aData)),
                       aFallible);
  }

  
  
  
  
  
  
  template<int N>
  void AssignLiteral(const char_type (&aStr)[N])
  {
    AssignLiteral(aStr, N - 1);
  }
#ifdef CharT_is_PRUnichar
  template<int N>
  void AssignLiteral(const char (&aStr)[N])
  {
    AssignASCII(aStr, N - 1);
  }
#endif

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
  self_type& operator=(char16ptr_t aData)
  {
    Assign(aData);
    return *this;
  }
#endif
  self_type& operator=(const self_type& aStr)
  {
    Assign(aStr);
    return *this;
  }
  self_type& operator=(const substring_tuple_type& aTuple)
  {
    Assign(aTuple);
    return *this;
  }

  void NS_FASTCALL Adopt(char_type* aData, size_type aLength = size_type(-1));


  



  void NS_FASTCALL Replace(index_type aCutStart, size_type aCutLength,
                           char_type aChar);
  MOZ_WARN_UNUSED_RESULT bool NS_FASTCALL Replace(index_type aCutStart,
                                                  size_type aCutLength,
                                                  char_type aChar,
                                                  const fallible_t&);
  void NS_FASTCALL Replace(index_type aCutStart, size_type aCutLength,
                           const char_type* aData,
                           size_type aLength = size_type(-1));
  MOZ_WARN_UNUSED_RESULT bool NS_FASTCALL Replace(index_type aCutStart,
                                                  size_type aCutLength,
                                                  const char_type* aData,
                                                  size_type aLength,
                                                  const fallible_t&);
  void Replace(index_type aCutStart, size_type aCutLength,
               const self_type& aStr)
  {
    Replace(aCutStart, aCutLength, aStr.Data(), aStr.Length());
  }
  MOZ_WARN_UNUSED_RESULT bool Replace(index_type aCutStart,
                                      size_type aCutLength,
                                      const self_type& aStr,
                                      const fallible_t& aFallible)
  {
    return Replace(aCutStart, aCutLength, aStr.Data(), aStr.Length(),
                   aFallible);
  }
  void NS_FASTCALL Replace(index_type aCutStart, size_type aCutLength,
                           const substring_tuple_type& aTuple);

  void NS_FASTCALL ReplaceASCII(index_type aCutStart, size_type aCutLength,
                                const char* aData,
                                size_type aLength = size_type(-1));

  MOZ_WARN_UNUSED_RESULT bool NS_FASTCALL ReplaceASCII(index_type aCutStart, size_type aCutLength,
                                                       const char* aData,
                                                       size_type aLength,
                                                       const fallible_t&);

  
  
  
  template<int N>
  void ReplaceLiteral(index_type aCutStart, size_type aCutLength,
                      const char_type (&aStr)[N])
  {
    ReplaceLiteral(aCutStart, aCutLength, aStr, N - 1);
  }

  void Append(char_type aChar)
  {
    Replace(mLength, 0, aChar);
  }
  MOZ_WARN_UNUSED_RESULT bool Append(char_type aChar,
                                     const fallible_t& aFallible)
  {
    return Replace(mLength, 0, aChar, aFallible);
  }
  void Append(const char_type* aData, size_type aLength = size_type(-1))
  {
    Replace(mLength, 0, aData, aLength);
  }
  MOZ_WARN_UNUSED_RESULT bool Append(const char_type* aData, size_type aLength,
                                     const fallible_t& aFallible)
  {
    return Replace(mLength, 0, aData, aLength, aFallible);
  }

#if defined(CharT_is_PRUnichar) && defined(MOZ_USE_CHAR16_WRAPPER)
  void Append(char16ptr_t aData, size_type aLength = size_type(-1))
  {
    Append(static_cast<const char16_t*>(aData), aLength);
  }
#endif

  void Append(const self_type& aStr)
  {
    Replace(mLength, 0, aStr);
  }
  MOZ_WARN_UNUSED_RESULT bool Append(const self_type& aStr, const fallible_t& aFallible)
  {
    return Replace(mLength, 0, aStr, aFallible);
  }
  void Append(const substring_tuple_type& aTuple)
  {
    Replace(mLength, 0, aTuple);
  }

  void AppendASCII(const char* aData, size_type aLength = size_type(-1))
  {
    ReplaceASCII(mLength, 0, aData, aLength);
  }

  MOZ_WARN_UNUSED_RESULT bool AppendASCII(const char* aData, const fallible_t& aFallible)
  {
    return ReplaceASCII(mLength, 0, aData, size_type(-1), aFallible);
  }

  MOZ_WARN_UNUSED_RESULT bool AppendASCII(const char* aData, size_type aLength, const fallible_t& aFallible)
  {
    return ReplaceASCII(mLength, 0, aData, aLength, aFallible);
  }

  



  void AppendPrintf(const char* aFormat, ...);
  void AppendPrintf(const char* aFormat, va_list aAp);
  void AppendInt(int32_t aInteger)
  {
    AppendPrintf("%d", aInteger);
  }
  void AppendInt(int32_t aInteger, int aRadix)
  {
    const char* fmt = aRadix == 10 ? "%d" : aRadix == 8 ? "%o" : "%x";
    AppendPrintf(fmt, aInteger);
  }
  void AppendInt(uint32_t aInteger)
  {
    AppendPrintf("%u", aInteger);
  }
  void AppendInt(uint32_t aInteger, int aRadix)
  {
    const char* fmt = aRadix == 10 ? "%u" : aRadix == 8 ? "%o" : "%x";
    AppendPrintf(fmt, aInteger);
  }
  void AppendInt(int64_t aInteger)
  {
    AppendPrintf("%lld", aInteger);
  }
  void AppendInt(int64_t aInteger, int aRadix)
  {
    const char* fmt = aRadix == 10 ? "%lld" : aRadix == 8 ? "%llo" : "%llx";
    AppendPrintf(fmt, aInteger);
  }
  void AppendInt(uint64_t aInteger)
  {
    AppendPrintf("%llu", aInteger);
  }
  void AppendInt(uint64_t aInteger, int aRadix)
  {
    const char* fmt = aRadix == 10 ? "%llu" : aRadix == 8 ? "%llo" : "%llx";
    AppendPrintf(fmt, aInteger);
  }

  


  void NS_FASTCALL AppendFloat(float aFloat);
  void NS_FASTCALL AppendFloat(double aFloat);
public:

  
  
  
  template<int N>
  void AppendLiteral(const char_type (&aStr)[N])
  {
    ReplaceLiteral(mLength, 0, aStr, N - 1);
  }
#ifdef CharT_is_PRUnichar
  template<int N>
  void AppendLiteral(const char (&aStr)[N])
  {
    AppendASCII(aStr, N - 1);
  }

  template<int N>
  MOZ_WARN_UNUSED_RESULT     bool AppendLiteral(const char (&aStr)[N], const fallible_t& aFallible)
  {
    return AppendASCII(aStr, N - 1, aFallible);
  }
#endif

  self_type& operator+=(char_type aChar)
  {
    Append(aChar);
    return *this;
  }
  self_type& operator+=(const char_type* aData)
  {
    Append(aData);
    return *this;
  }
#if defined(CharT_is_PRUnichar) && defined(MOZ_USE_CHAR16_WRAPPER)
  self_type& operator+=(char16ptr_t aData)
  {
    Append(aData);
    return *this;
  }
#endif
  self_type& operator+=(const self_type& aStr)
  {
    Append(aStr);
    return *this;
  }
  self_type& operator+=(const substring_tuple_type& aTuple)
  {
    Append(aTuple);
    return *this;
  }

  void Insert(char_type aChar, index_type aPos)
  {
    Replace(aPos, 0, aChar);
  }
  void Insert(const char_type* aData, index_type aPos,
              size_type aLength = size_type(-1))
  {
    Replace(aPos, 0, aData, aLength);
  }
#if defined(CharT_is_PRUnichar) && defined(MOZ_USE_CHAR16_WRAPPER)
  void Insert(char16ptr_t aData, index_type aPos,
              size_type aLength = size_type(-1))
  {
    Insert(static_cast<const char16_t*>(aData), aPos, aLength);
  }
#endif
  void Insert(const self_type& aStr, index_type aPos)
  {
    Replace(aPos, 0, aStr);
  }
  void Insert(const substring_tuple_type& aTuple, index_type aPos)
  {
    Replace(aPos, 0, aTuple);
  }

  
  
  
  template<int N>
  void InsertLiteral(const char_type (&aStr)[N], index_type aPos)
  {
    ReplaceLiteral(aPos, 0, aStr, N - 1);
  }

  void Cut(index_type aCutStart, size_type aCutLength)
  {
    Replace(aCutStart, aCutLength, char_traits::sEmptyBuffer, 0);
  }


  



  






  void NS_FASTCALL SetCapacity(size_type aNewCapacity);
  MOZ_WARN_UNUSED_RESULT bool NS_FASTCALL SetCapacity(size_type aNewCapacity,
                                                      const fallible_t&);

  void NS_FASTCALL SetLength(size_type aNewLength);
  MOZ_WARN_UNUSED_RESULT bool NS_FASTCALL SetLength(size_type aNewLength,
                                                    const fallible_t&);

  void Truncate(size_type aNewLength = 0)
  {
    NS_ASSERTION(aNewLength <= mLength, "Truncate cannot make string longer");
    SetLength(aNewLength);
  }


  




  





  inline size_type GetData(const char_type** aData) const
  {
    *aData = mData;
    return mLength;
  }

  









  size_type GetMutableData(char_type** aData, size_type aNewLen = size_type(-1))
  {
    if (!EnsureMutable(aNewLen)) {
      AllocFailed(aNewLen == size_type(-1) ? mLength : aNewLen);
    }

    *aData = mData;
    return mLength;
  }

  size_type GetMutableData(char_type** aData, size_type aNewLen, const fallible_t&)
  {
    if (!EnsureMutable(aNewLen)) {
      *aData = nullptr;
      return 0;
    }

    *aData = mData;
    return mLength;
  }

#if defined(CharT_is_PRUnichar) && defined(MOZ_USE_CHAR16_WRAPPER)
  size_type GetMutableData(wchar_t** aData, size_type aNewLen = size_type(-1))
  {
    return GetMutableData(reinterpret_cast<char16_t**>(aData), aNewLen);
  }

  size_type GetMutableData(wchar_t** aData, size_type aNewLen,
                           const fallible_t& aFallible)
  {
    return GetMutableData(reinterpret_cast<char16_t**>(aData), aNewLen,
                          aFallible);
  }
#endif


  




  void NS_FASTCALL SetIsVoid(bool);

  







  void StripChar(char_type aChar, int32_t aOffset = 0);

  







  void StripChars(const char_type* aChars, uint32_t aOffset = 0);

  



  void ForgetSharedBuffer()
  {
    if (mFlags & nsSubstring::F_SHARED) {
      mData = char_traits::sEmptyBuffer;
      mLength = 0;
      mFlags = F_TERMINATED;
    }
  }

public:

  



  MOZ_IMPLICIT nsTSubstring_CharT(const substring_tuple_type& aTuple)
    : mData(nullptr)
    , mLength(0)
    , mFlags(F_NONE)
  {
    Assign(aTuple);
  }

  





  
#if defined(DEBUG) || defined(FORCE_BUILD_REFCNT_LOGGING)
#define XPCOM_STRING_CONSTRUCTOR_OUT_OF_LINE
  nsTSubstring_CharT(char_type* aData, size_type aLength, uint32_t aFlags);
#else
#undef XPCOM_STRING_CONSTRUCTOR_OUT_OF_LINE
  nsTSubstring_CharT(char_type* aData, size_type aLength, uint32_t aFlags)
    : mData(aData)
    , mLength(aLength)
    , mFlags(aFlags)
  {
  }
#endif 

  size_t SizeOfExcludingThisMustBeUnshared(mozilla::MallocSizeOf aMallocSizeOf)
  const;
  size_t SizeOfIncludingThisMustBeUnshared(mozilla::MallocSizeOf aMallocSizeOf)
  const;

  size_t SizeOfExcludingThisIfUnshared(mozilla::MallocSizeOf aMallocSizeOf)
  const;
  size_t SizeOfIncludingThisIfUnshared(mozilla::MallocSizeOf aMallocSizeOf)
  const;

  





  size_t SizeOfExcludingThisEvenIfShared(mozilla::MallocSizeOf aMallocSizeOf)
  const;
  size_t SizeOfIncludingThisEvenIfShared(mozilla::MallocSizeOf aMallocSizeOf)
  const;

  template<class T>
  void NS_ABORT_OOM(T)
  {
    struct never {}; 
    static_assert(mozilla::IsSame<T, never>::value,
      "In string classes, use AllocFailed to account for sizeof(char_type). "
      "Use the global ::NS_ABORT_OOM if you really have a count of bytes.");
  }

  MOZ_ALWAYS_INLINE void AllocFailed(size_t aLength)
  {
    ::NS_ABORT_OOM(aLength * sizeof(char_type));
  }

protected:

  friend class nsTObsoleteAStringThunk_CharT;
  friend class nsTSubstringTuple_CharT;

  
  friend class nsTPromiseFlatString_CharT;

  char_type*  mData;
  size_type   mLength;
  uint32_t    mFlags;

  
  nsTSubstring_CharT()
    : mData(char_traits::sEmptyBuffer)
    ,  mLength(0)
    ,  mFlags(F_TERMINATED)
  {
  }

  
  explicit
  nsTSubstring_CharT(uint32_t aFlags)
    : mFlags(aFlags)
  {
  }

  
  
  nsTSubstring_CharT(const self_type& aStr)
    : mData(aStr.mData)
    ,  mLength(aStr.mLength)
    ,  mFlags(aStr.mFlags & (F_TERMINATED | F_VOIDED))
  {
  }

  




  void NS_FASTCALL Finalize();

  

















  bool NS_FASTCALL MutatePrep(size_type aCapacity,
                              char_type** aOldData, uint32_t* aOldFlags);

  



















  MOZ_WARN_UNUSED_RESULT bool ReplacePrep(index_type aCutStart,
                                          size_type aCutLength,
                                          size_type aNewLength)
  {
    aCutLength = XPCOM_MIN(aCutLength, mLength - aCutStart);
    uint32_t newTotalLen = mLength - aCutLength + aNewLength;
    if (aCutStart == mLength && Capacity() > newTotalLen) {
      mFlags &= ~F_VOIDED;
      mData[newTotalLen] = char_type(0);
      mLength = newTotalLen;
      return true;
    }
    return ReplacePrepInternal(aCutStart, aCutLength, aNewLength, newTotalLen);
  }

  MOZ_WARN_UNUSED_RESULT bool NS_FASTCALL ReplacePrepInternal(
    index_type aCutStart,
    size_type aCutLength,
    size_type aNewFragLength,
    size_type aNewTotalLength);

  






  size_type NS_FASTCALL Capacity() const;

  



  MOZ_WARN_UNUSED_RESULT bool NS_FASTCALL EnsureMutable(
    size_type aNewLen = size_type(-1));

  


  bool IsDependentOn(const char_type* aStart, const char_type* aEnd) const
  {
    







    return (aStart < (mData + mLength) && aEnd > mData);
  }

  


  void SetDataFlags(uint32_t aDataFlags)
  {
    NS_ASSERTION((aDataFlags & 0xFFFF0000) == 0, "bad flags");
    mFlags = aDataFlags | (mFlags & 0xFFFF0000);
  }

  void NS_FASTCALL ReplaceLiteral(index_type aCutStart, size_type aCutLength,
                                  const char_type* aData, size_type aLength);

  static int AppendFunc(void* aArg, const char* aStr, uint32_t aLen);

public:

  
  
  void NS_FASTCALL AssignLiteral(const char_type* aData, size_type aLength);

  
  
  
  
  

  enum
  {
    F_NONE         = 0,       

    
    F_TERMINATED   = 1 << 0,  
    F_VOIDED       = 1 << 1,  
    F_SHARED       = 1 << 2,  
    F_OWNED        = 1 << 3,  
    F_FIXED        = 1 << 4,  
    F_LITERAL      = 1 << 5,  

    
    F_CLASS_FIXED  = 1 << 16   
  };

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
};

int NS_FASTCALL
Compare(const nsTSubstring_CharT::base_string_type& aLhs,
        const nsTSubstring_CharT::base_string_type& aRhs,
        const nsTStringComparator_CharT& = nsTDefaultStringComparator_CharT());


inline bool
operator!=(const nsTSubstring_CharT::base_string_type& aLhs,
           const nsTSubstring_CharT::base_string_type& aRhs)
{
  return !aLhs.Equals(aRhs);
}

inline bool
operator<(const nsTSubstring_CharT::base_string_type& aLhs,
          const nsTSubstring_CharT::base_string_type& aRhs)
{
  return Compare(aLhs, aRhs) < 0;
}

inline bool
operator<=(const nsTSubstring_CharT::base_string_type& aLhs,
           const nsTSubstring_CharT::base_string_type& aRhs)
{
  return Compare(aLhs, aRhs) <= 0;
}

inline bool
operator==(const nsTSubstring_CharT::base_string_type& aLhs,
           const nsTSubstring_CharT::base_string_type& aRhs)
{
  return aLhs.Equals(aRhs);
}

inline bool
operator==(const nsTSubstring_CharT::base_string_type& aLhs,
           const nsTSubstring_CharT::char_type* aRhs)
{
  return aLhs.Equals(aRhs);
}


inline bool
operator>=(const nsTSubstring_CharT::base_string_type& aLhs,
           const nsTSubstring_CharT::base_string_type& aRhs)
{
  return Compare(aLhs, aRhs) >= 0;
}

inline bool
operator>(const nsTSubstring_CharT::base_string_type& aLhs,
          const nsTSubstring_CharT::base_string_type& aRhs)
{
  return Compare(aLhs, aRhs) > 0;
}
