






































  








class nsTSubstringTuple_CharT
  {
    public:

      typedef CharT                      char_type;
      typedef nsCharTraits<char_type>    char_traits;

      typedef nsTSubstringTuple_CharT    self_type;
      typedef nsTSubstring_CharT         substring_type;
#ifdef MOZ_V1_STRING_ABI
      typedef nsTAString_CharT           base_string_type;
      typedef nsTObsoleteAString_CharT   obsolete_string_type;
#else
      typedef nsTSubstring_CharT         base_string_type;
#endif
      typedef PRUint32                   size_type;

    public:

      nsTSubstringTuple_CharT(const base_string_type* a, const base_string_type* b)
        : mHead(nsnull)
        , mFragA(a)
        , mFragB(b) {}

      nsTSubstringTuple_CharT(const self_type& head, const base_string_type* b)
        : mHead(&head)
        , mFragA(nsnull) 
        , mFragB(b) {}

        


      NS_COM size_type Length() const;

        




      NS_COM void WriteTo(char_type *buf, PRUint32 bufLen) const;

        



      NS_COM PRBool IsDependentOn(const char_type *start, const char_type *end) const;

    private:

      const self_type*        mHead;
      const base_string_type* mFragA;
      const base_string_type* mFragB;
  };

inline
const nsTSubstringTuple_CharT
operator+(const nsTSubstringTuple_CharT::base_string_type& a, const nsTSubstringTuple_CharT::base_string_type& b)
  {
    return nsTSubstringTuple_CharT(&a, &b);
  }

inline
const nsTSubstringTuple_CharT
operator+(const nsTSubstringTuple_CharT& head, const nsTSubstringTuple_CharT::base_string_type& b)
  {
    return nsTSubstringTuple_CharT(head, &b);
  }
