







































  




















































class nsTPromiseFlatString_CharT : public nsTString_CharT
  {
    public:

      typedef nsTPromiseFlatString_CharT    self_type;

    private:

      NS_COM void Init( const substring_type& );

        
      void operator=( const self_type& );

        
      nsTPromiseFlatString_CharT();

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

  
inline
const nsTPromiseFlatString_CharT
TPromiseFlatString_CharT( const nsTSubstring_CharT& frag )
  {
    return nsTPromiseFlatString_CharT(frag);
  }

  
inline
const nsTPromiseFlatString_CharT
TPromiseFlatString_CharT( const nsTSubstringTuple_CharT& tuple )
  {
    return nsTPromiseFlatString_CharT(tuple);
  }
