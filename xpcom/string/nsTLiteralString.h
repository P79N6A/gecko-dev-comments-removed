
















class nsTLiteralString_CharT : public nsTString_CharT
{
public:

  typedef nsTLiteralString_CharT self_type;

public:

  



  template<size_type N>
  nsTLiteralString_CharT(const char_type (&aStr)[N])
    : string_type(const_cast<char_type*>(aStr), N - 1, F_TERMINATED | F_LITERAL)
  {
  }

private:

  
  template<size_type N>
  nsTLiteralString_CharT(char_type (&aStr)[N]) MOZ_DELETE;
};
