















class nsTSubstringTuple_CharT
{
public:

  typedef CharT                      char_type;
  typedef nsCharTraits<char_type>    char_traits;

  typedef nsTSubstringTuple_CharT    self_type;
  typedef nsTSubstring_CharT         substring_type;
  typedef nsTSubstring_CharT         base_string_type;
  typedef uint32_t                   size_type;

public:

  nsTSubstringTuple_CharT(const base_string_type* aStrA,
                          const base_string_type* aStrB)
    : mHead(nullptr)
    , mFragA(aStrA)
    , mFragB(aStrB)
  {
  }

  nsTSubstringTuple_CharT(const self_type& aHead,
                          const base_string_type* aStrB)
    : mHead(&aHead)
    , mFragA(nullptr) 
    , mFragB(aStrB)
  {
  }

  


  size_type Length() const;

  




  void WriteTo(char_type* aBuf, uint32_t aBufLen) const;

  



  bool IsDependentOn(const char_type* aStart, const char_type* aEnd) const;

private:

  const self_type*        mHead;
  const base_string_type* mFragA;
  const base_string_type* mFragB;
};

inline const nsTSubstringTuple_CharT
operator+(const nsTSubstringTuple_CharT::base_string_type& aStrA,
          const nsTSubstringTuple_CharT::base_string_type& aStrB)
{
  return nsTSubstringTuple_CharT(&aStrA, &aStrB);
}

inline const nsTSubstringTuple_CharT
operator+(const nsTSubstringTuple_CharT& aHead,
          const nsTSubstringTuple_CharT::base_string_type& aStrB)
{
  return nsTSubstringTuple_CharT(aHead, &aStrB);
}
