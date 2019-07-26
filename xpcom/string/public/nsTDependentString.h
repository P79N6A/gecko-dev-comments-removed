

















class nsTDependentString_CharT : public nsTString_CharT
{
public:

  typedef nsTDependentString_CharT    self_type;

public:

  



  nsTDependentString_CharT( const char_type* start, const char_type* end )
    : string_type(const_cast<char_type*>(start), uint32_t(end - start), F_TERMINATED)
  {
    AssertValidDepedentString();
  }

  nsTDependentString_CharT( const char_type* data, uint32_t length )
    : string_type(const_cast<char_type*>(data), length, F_TERMINATED)
  {
    AssertValidDepedentString();
  }

#if defined(CharT_is_PRUnichar) && defined(MOZ_USE_CHAR16_WRAPPER)
  nsTDependentString_CharT( char16ptr_t data, uint32_t length )
    : nsTDependentString_CharT(static_cast<const char16_t*>(data), length) {}
#endif

  explicit
  nsTDependentString_CharT( const char_type* data )
    : string_type(const_cast<char_type*>(data), uint32_t(char_traits::length(data)), F_TERMINATED)
  {
    AssertValidDepedentString();
  }

#if defined(CharT_is_PRUnichar) && defined(MOZ_USE_CHAR16_WRAPPER)
  explicit
  nsTDependentString_CharT( char16ptr_t data )
    : nsTDependentString_CharT( static_cast<const char16_t*>(data)) {}
#endif

  nsTDependentString_CharT( const string_type& str, uint32_t startPos )
    : string_type()
  {
    Rebind(str, startPos);
  }

  
  nsTDependentString_CharT()
    : string_type() {}

  
  
  
  


  



  using nsTString_CharT::Rebind;
  void Rebind( const char_type* data )
  {
    Rebind(data, uint32_t(char_traits::length(data)));
  }

  void Rebind( const char_type* start, const char_type* end )
  {
    Rebind(start, uint32_t(end - start));
  }

  void Rebind( const string_type&, uint32_t startPos );

private:

  
  nsTDependentString_CharT( const substring_tuple_type& ) MOZ_DELETE;
};
