

































































class nsTPromiseFlatString_CharT : public nsTString_CharT
{
public:

  typedef nsTPromiseFlatString_CharT self_type;

private:

  void Init(const substring_type&);

  
  void operator=(const self_type&) = delete;

  
  nsTPromiseFlatString_CharT() = delete;

  
  nsTPromiseFlatString_CharT(const string_type& aStr) = delete;

public:

  explicit
  nsTPromiseFlatString_CharT(const substring_type& aStr)
    : string_type()
  {
    Init(aStr);
  }

  explicit
  nsTPromiseFlatString_CharT(const substring_tuple_type& aTuple)
    : string_type()
  {
    
    
    Assign(aTuple);
  }
};



template<class T>
const nsTPromiseFlatString_CharT
TPromiseFlatString_CharT(const T& aString)
{
  return nsTPromiseFlatString_CharT(aString);
}
