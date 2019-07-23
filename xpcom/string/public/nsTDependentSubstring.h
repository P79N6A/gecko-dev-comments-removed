







































  


class nsTDependentSubstring_CharT : public nsTSubstring_CharT
  {
    public:

      typedef nsTDependentSubstring_CharT    self_type;

    public:

#ifdef MOZ_V1_STRING_ABI
      NS_COM void Rebind( const abstract_string_type&, PRUint32 startPos, PRUint32 length = size_type(-1) );
#endif
      NS_COM void Rebind( const substring_type&, PRUint32 startPos, PRUint32 length = size_type(-1) );

      NS_COM void Rebind( const char_type* start, const char_type* end );

#ifdef MOZ_V1_STRING_ABI
      nsTDependentSubstring_CharT( const abstract_string_type& str, PRUint32 startPos, PRUint32 length = size_type(-1) )
        : substring_type()
        {
          Rebind(str, startPos, length);
        }
#endif

      nsTDependentSubstring_CharT( const substring_type& str, PRUint32 startPos, PRUint32 length = size_type(-1) )
        : substring_type()
        {
          Rebind(str, startPos, length);
        }

      nsTDependentSubstring_CharT( const char_type* start, const char_type* end )
        : substring_type(NS_CONST_CAST(char_type*, start), end - start, F_NONE) {}

      nsTDependentSubstring_CharT( const const_iterator& start, const const_iterator& end )
        : substring_type(NS_CONST_CAST(char_type*, start.get()), end.get() - start.get(), F_NONE) {}

      
      nsTDependentSubstring_CharT()
        : substring_type() {}

      

    private:
        
      void operator=( const self_type& );        
  };

#ifdef MOZ_V1_STRING_ABI
inline
const nsTDependentSubstring_CharT
Substring( const nsTAString_CharT& str, PRUint32 startPos, PRUint32 length = PRUint32(-1) )
  {
    return nsTDependentSubstring_CharT(str, startPos, length);
  }
#endif

inline
const nsTDependentSubstring_CharT
Substring( const nsTSubstring_CharT& str, PRUint32 startPos, PRUint32 length = PRUint32(-1) )
  {
    return nsTDependentSubstring_CharT(str, startPos, length);
  }

inline
const nsTDependentSubstring_CharT
Substring( const nsReadingIterator<CharT>& start, const nsReadingIterator<CharT>& end )
  {
    return nsTDependentSubstring_CharT(start.get(), end.get());
  }

inline
const nsTDependentSubstring_CharT
Substring( const CharT* start, const CharT* end )
  {
    return nsTDependentSubstring_CharT(start, end);
  }

#ifdef MOZ_V1_STRING_ABI
inline
const nsTDependentSubstring_CharT
StringHead( const nsTAString_CharT& str, PRUint32 count )
  {
    return nsTDependentSubstring_CharT(str, 0, count);
  }
#endif

inline
const nsTDependentSubstring_CharT
StringHead( const nsTSubstring_CharT& str, PRUint32 count )
  {
    return nsTDependentSubstring_CharT(str, 0, count);
  }

#ifdef MOZ_V1_STRING_ABI
inline
const nsTDependentSubstring_CharT
StringTail( const nsTAString_CharT& str, PRUint32 count )
  {
    return nsTDependentSubstring_CharT(str, str.Length() - count, count);
  }
#endif

inline
const nsTDependentSubstring_CharT
StringTail( const nsTSubstring_CharT& str, PRUint32 count )
  {
    return nsTDependentSubstring_CharT(str, str.Length() - count, count);
  }
