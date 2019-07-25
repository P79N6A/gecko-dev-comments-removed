





void
nsTDependentString_CharT::Rebind( const char_type* data, size_type length )
  {
    
    Finalize();

    mData = const_cast<char_type*>(data);
    mLength = length;
    SetDataFlags(F_TERMINATED);
    AssertValid();
  }

void
nsTDependentString_CharT::Rebind( const string_type& str, uint32_t startPos )
  {
    
    Finalize();

    size_type strLength = str.Length();

    if (startPos > strLength)
      startPos = strLength;

    mData = const_cast<char_type*>(str.Data()) + startPos;
    mLength = strLength - startPos;

    SetDataFlags(F_TERMINATED);
  }
