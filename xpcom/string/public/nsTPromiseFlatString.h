







































  




















































class nsTPromiseFlatString_CharT : public nsTString_CharT
  {
    public:

      typedef nsTPromiseFlatString_CharT    self_type;

    private:

      NS_COM void Init( const substring_type& );
#ifdef MOZ_V1_STRING_ABI
      NS_COM void Init( const abstract_string_type& );
#endif

        
      void operator=( const self_type& );

        
      nsTPromiseFlatString_CharT();

    public:

      explicit
      nsTPromiseFlatString_CharT( const substring_type& str )
        : string_type()
        {
          Init(str);
        }

#ifdef MOZ_V1_STRING_ABI
      explicit
      nsTPromiseFlatString_CharT( const abstract_string_type& readable )
        : string_type()
        {
          Init(readable);
        }
#endif

      explicit
      nsTPromiseFlatString_CharT( const substring_tuple_type& tuple )
        : string_type()
        {
          
          
          Assign(tuple);
        }
  };

#ifdef MOZ_V1_STRING_ABI
inline
const nsTPromiseFlatString_CharT
TPromiseFlatString_CharT( const nsTAString_CharT& str )
  {
    return nsTPromiseFlatString_CharT(str);
  }
#endif

  
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
