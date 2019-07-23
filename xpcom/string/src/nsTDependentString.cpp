






































void
nsTDependentString_CharT::Rebind( const char_type* data, size_type length )
  {
    
    Finalize();

    mData = const_cast<char_type*>(data);
    mLength = length;
    SetDataFlags(F_TERMINATED);
    AssertValid();
  }
