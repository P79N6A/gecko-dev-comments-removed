



























































class nsTPromiseFlatString_CharT : public nsTString_CharT
{
public:

  typedef nsTPromiseFlatString_CharT    self_type;

private:

  void Init( const substring_type& );

  
  void operator=( const self_type& ) MOZ_DELETE;

  
  nsTPromiseFlatString_CharT() MOZ_DELETE;

  
  nsTPromiseFlatString_CharT( const string_type& str ) MOZ_DELETE;

public:

  explicit
  nsTPromiseFlatString_CharT( const substring_type& str )
    : string_type()
  {
    Init(str);
  }

  explicit
  nsTPromiseFlatString_CharT( const substring_tuple_type& tuple )
    : string_type()
  {
    
    
    Assign(tuple);
  }
};



template<class T>
const nsTPromiseFlatString_CharT
TPromiseFlatString_CharT( const T& string )
{
  return nsTPromiseFlatString_CharT(string);
}
