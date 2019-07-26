

















class nsTDependentSubstring_CharT : public nsTSubstring_CharT
{
public:

  typedef nsTDependentSubstring_CharT self_type;

public:

  void Rebind(const substring_type&, uint32_t aStartPos,
              uint32_t aLength = size_type(-1));

  void Rebind(const char_type* aData, size_type aLength);

  void Rebind(const char_type* aStart, const char_type* aEnd)
  {
    Rebind(aStart, size_type(aEnd - aStart));
  }

  nsTDependentSubstring_CharT(const substring_type& aStr, uint32_t aStartPos,
                              uint32_t aLength = size_type(-1))
    : substring_type()
  {
    Rebind(aStr, aStartPos, aLength);
  }

  nsTDependentSubstring_CharT(const char_type* aData, size_type aLength)
    : substring_type(const_cast<char_type*>(aData), aLength, F_NONE)
  {
  }

  nsTDependentSubstring_CharT(const char_type* aStart, const char_type* aEnd)
    : substring_type(const_cast<char_type*>(aStart), uint32_t(aEnd - aStart),
                     F_NONE)
  {
  }

#if defined(CharT_is_PRUnichar) && defined(MOZ_USE_CHAR16_WRAPPER)
  nsTDependentSubstring_CharT(char16ptr_t aData, size_type aLength)
    : nsTDependentSubstring_CharT(static_cast<const char16_t*>(aData), aLength)
  {
  }

  nsTDependentSubstring_CharT(char16ptr_t aStart, char16ptr_t aEnd)
    : nsTDependentSubstring_CharT(static_cast<const char16_t*>(aStart),
                                  static_cast<const char16_t*>(aEnd))
  {
  }
#endif

  nsTDependentSubstring_CharT(const const_iterator& aStart,
                              const const_iterator& aEnd)
    : substring_type(const_cast<char_type*>(aStart.get()),
                     uint32_t(aEnd.get() - aStart.get()), F_NONE)
  {
  }

  
  nsTDependentSubstring_CharT()
    : substring_type()
  {
  }

  

private:
  
  void operator=(const self_type&);  
};

inline const nsTDependentSubstring_CharT
Substring(const nsTSubstring_CharT& aStr, uint32_t aStartPos,
          uint32_t aLength = uint32_t(-1))
{
  return nsTDependentSubstring_CharT(aStr, aStartPos, aLength);
}

inline const nsTDependentSubstring_CharT
Substring(const nsReadingIterator<CharT>& aStart,
          const nsReadingIterator<CharT>& aEnd)
{
  return nsTDependentSubstring_CharT(aStart.get(), aEnd.get());
}

inline const nsTDependentSubstring_CharT
Substring(const CharT* aData, uint32_t aLength)
{
  return nsTDependentSubstring_CharT(aData, aLength);
}

inline const nsTDependentSubstring_CharT
Substring(const CharT* aStart, const CharT* aEnd)
{
  return nsTDependentSubstring_CharT(aStart, aEnd);
}

inline const nsTDependentSubstring_CharT
StringHead(const nsTSubstring_CharT& aStr, uint32_t aCount)
{
  return nsTDependentSubstring_CharT(aStr, 0, aCount);
}

inline const nsTDependentSubstring_CharT
StringTail(const nsTSubstring_CharT& aStr, uint32_t aCount)
{
  return nsTDependentSubstring_CharT(aStr, aStr.Length() - aCount, aCount);
}
