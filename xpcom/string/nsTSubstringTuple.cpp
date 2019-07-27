










nsTSubstringTuple_CharT::size_type
nsTSubstringTuple_CharT::Length() const
{
  uint32_t len;
  if (mHead) {
    len = mHead->Length();
  } else {
    len = TO_SUBSTRING(mFragA).Length();
  }

  return len + TO_SUBSTRING(mFragB).Length();
}








void
nsTSubstringTuple_CharT::WriteTo(char_type* aBuf, uint32_t aBufLen) const
{
  const substring_type& b = TO_SUBSTRING(mFragB);

  NS_ASSERTION(aBufLen >= b.Length(), "buffer too small");
  uint32_t headLen = aBufLen - b.Length();
  if (mHead) {
    mHead->WriteTo(aBuf, headLen);
  } else {
    const substring_type& a = TO_SUBSTRING(mFragA);

    NS_ASSERTION(a.Length() == headLen, "buffer incorrectly sized");
    char_traits::copy(aBuf, a.Data(), a.Length());
  }

  char_traits::copy(aBuf + headLen, b.Data(), b.Length());

#if 0
  
  
  

  const substring_type& b = TO_SUBSTRING(mFragB);

  NS_ASSERTION(aBufLen >= b.Length(), "buffer is too small");
  char_traits::copy(aBuf + aBufLen - b.Length(), b.Data(), b.Length());

  aBufLen -= b.Length();

  if (mHead) {
    mHead->WriteTo(aBuf, aBufLen);
  } else {
    const substring_type& a = TO_SUBSTRING(mFragA);
    NS_ASSERTION(aBufLen == a.Length(), "buffer is too small");
    char_traits::copy(aBuf, a.Data(), a.Length());
  }
#endif
}







bool
nsTSubstringTuple_CharT::IsDependentOn(const char_type* aStart,
                                       const char_type* aEnd) const
{
  

  if (TO_SUBSTRING(mFragB).IsDependentOn(aStart, aEnd)) {
    return true;
  }

  if (mHead) {
    return mHead->IsDependentOn(aStart, aEnd);
  }

  return TO_SUBSTRING(mFragA).IsDependentOn(aStart, aEnd);
}
