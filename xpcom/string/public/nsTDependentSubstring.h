







































  










class nsTDependentSubstring_CharT : public nsTSubstring_CharT
  {
    public:

      typedef nsTDependentSubstring_CharT    self_type;

    public:

      void Rebind( const substring_type&, PRUint32 startPos, PRUint32 length = size_type(-1) );

      void Rebind( const char_type* data, size_type length );

      void Rebind( const char_type* start, const char_type* end )
        {
          Rebind(start, size_type(end - start));
        }

      nsTDependentSubstring_CharT( const substring_type& str, PRUint32 startPos, PRUint32 length = size_type(-1) )
        : substring_type()
        {
          Rebind(str, startPos, length);
        }

      nsTDependentSubstring_CharT( const char_type* data, size_type length )
        : substring_type(const_cast<char_type*>(data), length, F_NONE) {}

      nsTDependentSubstring_CharT( const char_type* start, const char_type* end )
        : substring_type(const_cast<char_type*>(start), PRUint32(end - start), F_NONE) {}

      nsTDependentSubstring_CharT( const const_iterator& start, const const_iterator& end )
        : substring_type(const_cast<char_type*>(start.get()), PRUint32(end.get() - start.get()), F_NONE) {}

      
      nsTDependentSubstring_CharT()
        : substring_type() {}

      

    private:
        
      void operator=( const self_type& );        
  };

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
Substring( const CharT* data, PRUint32 length )
  {
    return nsTDependentSubstring_CharT(data, length);
  }

inline
const nsTDependentSubstring_CharT
Substring( const CharT* start, const CharT* end )
  {
    return nsTDependentSubstring_CharT(start, end);
  }

inline
const nsTDependentSubstring_CharT
StringHead( const nsTSubstring_CharT& str, PRUint32 count )
  {
    return nsTDependentSubstring_CharT(str, 0, count);
  }

inline
const nsTDependentSubstring_CharT
StringTail( const nsTSubstring_CharT& str, PRUint32 count )
  {
    return nsTDependentSubstring_CharT(str, str.Length() - count, count);
  }
