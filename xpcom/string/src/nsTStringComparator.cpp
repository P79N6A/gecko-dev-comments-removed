





































NS_COM int NS_FASTCALL
Compare( const nsTSubstring_CharT::base_string_type& lhs, const nsTSubstring_CharT::base_string_type& rhs, const nsTStringComparator_CharT& comp )
  {
    typedef nsTAString_CharT::size_type size_type;

    if ( &lhs == &rhs )
      return 0;

    nsTAString_CharT::const_iterator leftIter, rightIter;
    lhs.BeginReading(leftIter);
    rhs.BeginReading(rightIter);

    size_type lLength = leftIter.size_forward();
    size_type rLength = rightIter.size_forward();
    size_type lengthToCompare = NS_MIN(lLength, rLength);

    int result;
    if ( (result = comp(leftIter.get(), rightIter.get(), lengthToCompare)) == 0 )
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
nsTDefaultStringComparator_CharT::operator()( const char_type* lhs, const char_type* rhs, PRUint32 aLength ) const
  {
    return nsCharTraits<CharT>::compare(lhs, rhs, aLength);
  }

int
nsTDefaultStringComparator_CharT::operator()( char_type lhs, char_type rhs) const
  {
    return lhs - rhs;
  } 
