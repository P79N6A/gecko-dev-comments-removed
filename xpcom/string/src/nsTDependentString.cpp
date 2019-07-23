






































void
nsTDependentString_CharT::Rebind( const char_type* data, size_type length )
  {
    
    Finalize();

    mData = NS_CONST_CAST(char_type*, data);
    mLength = length;
    SetDataFlags(F_TERMINATED);
    AssertValid();
  }
