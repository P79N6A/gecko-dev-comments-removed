





int NS_FASTCALL
Compare(const nsTSubstring_CharT::base_string_type& aLhs,
        const nsTSubstring_CharT::base_string_type& aRhs,
        const nsTStringComparator_CharT& comp)
{
  typedef nsTSubstring_CharT::size_type size_type;

  if (&aLhs == &aRhs) {
    return 0;
  }

  nsTSubstring_CharT::const_iterator leftIter, rightIter;
  aLhs.BeginReading(leftIter);
  aRhs.BeginReading(rightIter);

  size_type lLength = leftIter.size_forward();
  size_type rLength = rightIter.size_forward();
  size_type lengthToCompare = XPCOM_MIN(lLength, rLength);

  int result;
  if ((result = comp(leftIter.get(), rightIter.get(),
                     lengthToCompare, lengthToCompare)) == 0) {
    if (lLength < rLength) {
      result = -1;
    } else if (rLength < lLength) {
      result = 1;
    } else {
      result = 0;
    }
  }

  return result;
}

int
nsTDefaultStringComparator_CharT::operator()(const char_type* aLhs,
                                             const char_type* aRhs,
                                             uint32_t aLLength,
                                             uint32_t aRLength) const
{
  return
    aLLength == aRLength ? nsCharTraits<CharT>::compare(aLhs, aRhs, aLLength) :
                            (aLLength > aRLength) ? 1 : -1;
}
