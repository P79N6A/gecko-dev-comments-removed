






































  



nsTSubstringTuple_CharT::size_type
nsTSubstringTuple_CharT::Length() const
  {
    PRUint32 len;
    if (mHead)
      len = mHead->Length();
    else
      len = TO_SUBSTRING(mFragA).Length();

    return len + TO_SUBSTRING(mFragB).Length();
  }


  





void
nsTSubstringTuple_CharT::WriteTo( char_type *buf, PRUint32 bufLen ) const
  {
    const substring_type& b = TO_SUBSTRING(mFragB);

    NS_ASSERTION(bufLen >= b.Length(), "buffer too small");
    PRUint32 headLen = bufLen - b.Length();
    if (mHead)
      {
        mHead->WriteTo(buf, headLen);
      }
    else
      {
        const substring_type& a = TO_SUBSTRING(mFragA);

        NS_ASSERTION(a.Length() == headLen, "buffer incorrectly sized");
        char_traits::copy(buf, a.Data(), a.Length());
      }

    char_traits::copy(buf + headLen, b.Data(), b.Length());

#if 0
    
    
    

    const substring_type& b = TO_SUBSTRING(mFragB);

    NS_ASSERTION(bufLen >= b.Length(), "buffer is too small");
    char_traits::copy(buf + bufLen - b.Length(), b.Data(), b.Length());

    bufLen -= b.Length();

    if (mHead)
      {
        mHead->WriteTo(buf, bufLen);
      }
    else
      {
        const substring_type& a = TO_SUBSTRING(mFragA);
        NS_ASSERTION(bufLen == a.Length(), "buffer is too small");
        char_traits::copy(buf, a.Data(), a.Length());
      }
#endif
  }


  




PRBool
nsTSubstringTuple_CharT::IsDependentOn( const char_type *start, const char_type *end ) const
  {
    

    if (TO_SUBSTRING(mFragB).IsDependentOn(start, end))
      return PR_TRUE;

    if (mHead)
      return mHead->IsDependentOn(start, end);

    return TO_SUBSTRING(mFragA).IsDependentOn(start, end);
  }
