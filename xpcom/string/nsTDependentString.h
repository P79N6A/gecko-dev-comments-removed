

















class nsTDependentString_CharT : public nsTString_CharT
{
public:

  typedef nsTDependentString_CharT self_type;

public:

  



  nsTDependentString_CharT(const char_type* aStart, const char_type* aEnd)
    : string_type(const_cast<char_type*>(aStart),
                  uint32_t(aEnd - aStart), F_TERMINATED)
  {
    AssertValidDependentString();
  }

  nsTDependentString_CharT(const char_type* aData, uint32_t aLength)
    : string_type(const_cast<char_type*>(aData), aLength, F_TERMINATED)
  {
    AssertValidDependentString();
  }

#if defined(CharT_is_PRUnichar) && defined(MOZ_USE_CHAR16_WRAPPER)
  nsTDependentString_CharT(char16ptr_t aData, uint32_t aLength)
    : nsTDependentString_CharT(static_cast<const char16_t*>(aData), aLength)
  {
  }
#endif

  explicit
  nsTDependentString_CharT(const char_type* aData)
    : string_type(const_cast<char_type*>(aData),
                  uint32_t(char_traits::length(aData)), F_TERMINATED)
  {
    AssertValidDependentString();
  }

#if defined(CharT_is_PRUnichar) && defined(MOZ_USE_CHAR16_WRAPPER)
  explicit
  nsTDependentString_CharT(char16ptr_t aData)
    : nsTDependentString_CharT(static_cast<const char16_t*>(aData))
  {
  }
#endif

  nsTDependentString_CharT(const string_type& aStr, uint32_t aStartPos)
    : string_type()
  {
    Rebind(aStr, aStartPos);
  }

  
  nsTDependentString_CharT()
    : string_type()
  {
  }

  
  
  
  


  



  using nsTString_CharT::Rebind;
  void Rebind(const char_type* aData)
  {
    Rebind(aData, uint32_t(char_traits::length(aData)));
  }

  void Rebind(const char_type* aStart, const char_type* aEnd)
  {
    Rebind(aStart, uint32_t(aEnd - aStart));
  }

  void Rebind(const string_type&, uint32_t aStartPos);

private:

  
  nsTDependentString_CharT(const substring_tuple_type&) = delete;
};
