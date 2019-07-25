







































  










class nsTDependentString_CharT : public nsTString_CharT
  {
    public:

      typedef nsTDependentString_CharT    self_type;

    public:

        


      void AssertValid()
        {
          NS_ASSERTION(mData, "nsTDependentString must wrap a non-NULL buffer");
          NS_ASSERTION(mLength != size_type(-1), "nsTDependentString has bogus length");
          NS_ASSERTION(mData[mLength] == 0, "nsTDependentString must wrap only null-terminated strings");
        }


        



      nsTDependentString_CharT( const char_type* start, const char_type* end )
        : string_type(const_cast<char_type*>(start), PRUint32(end - start), F_TERMINATED)
        {
          AssertValid();
        }

      nsTDependentString_CharT( const char_type* data, PRUint32 length )
        : string_type(const_cast<char_type*>(data), length, F_TERMINATED)
        {
          AssertValid();
        }

      explicit
      nsTDependentString_CharT( const char_type* data )
        : string_type(const_cast<char_type*>(data), PRUint32(char_traits::length(data)), F_TERMINATED)
        {
          AssertValid();
        }

      explicit
      nsTDependentString_CharT( const substring_type& str )
        : string_type(const_cast<char_type*>(str.Data()), str.Length(), F_TERMINATED)
        {
          AssertValid();
        }

      
      nsTDependentString_CharT()
        : string_type() {}

      
      
      
      


        



      void Rebind( const char_type* data )
        {
          Rebind(data, PRUint32(char_traits::length(data)));
        }

      void Rebind( const char_type* data, size_type length );

      void Rebind( const char_type* start, const char_type* end )
        {
          Rebind(start, PRUint32(end - start));
        }

    private:
      
      
      nsTDependentString_CharT( const substring_tuple_type& );
  };
