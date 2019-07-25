





































NS_COM int NS_FASTCALL
Compare( const nsTSubstring_CharT::base_string_type& lhs, const nsTSubstring_CharT::base_string_type& rhs, const nsTStringComparator_CharT& comp )
  {
    typedef nsTSubstring_CharT::size_type size_type;

    if ( &lhs == &rhs )
      return 0;

    nsTSubstring_CharT::const_iterator leftIter, rightIter;
    lhs.BeginReading(leftIter);
    rhs.BeginReading(rightIter);

    size_type lLength = leftIter.size_forward();
    size_type rLength = rightIter.size_forward();
    size_type lengthToCompare = NS_MIN(lLength, rLength);

    int result;
    if ( (result = comp(leftIter.get(), rightIter.get(), lengthToCompare, lengthToCompare)) == 0 )
      {
        if ( lLength < rLength )
          result = -1;
        else if ( rLength < lLength )
          result = 1;
        else
          result = 0;
      }

    return result;
  }

int
nsTDefaultStringComparator_CharT::operator()( const char_type* lhs, const char_type* rhs, PRUint32 lLength, PRUint32 rLength) const
  {
    return (lLength == rLength) ? nsCharTraits<CharT>::compare(lhs, rhs, lLength) :
           (lLength > rLength) ? 1 : -1;
  }
