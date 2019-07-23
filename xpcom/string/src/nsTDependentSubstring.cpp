





































#ifdef MOZ_V1_STRING_ABI
void
nsTDependentSubstring_CharT::Rebind( const abstract_string_type& readable, PRUint32 startPos, PRUint32 length )
  {
    
    Finalize();

    size_type strLength = readable.GetReadableBuffer((const char_type**) &mData);

    if (startPos > strLength)
      startPos = strLength;

    mData += startPos;
    mLength = NS_MIN(length, strLength - startPos);

    SetDataFlags(F_NONE);
  }
#endif

void
nsTDependentSubstring_CharT::Rebind( const substring_type& str, PRUint32 startPos, PRUint32 length )
  {
    
    Finalize();

    size_type strLength = str.Length();

    if (startPos > strLength)
      startPos = strLength;

    mData = const_cast<char_type*>(str.Data()) + startPos;
    mLength = NS_MIN(length, strLength - startPos);

    SetDataFlags(F_NONE);
  }

void
nsTDependentSubstring_CharT::Rebind( const char_type* start, const char_type* end )
  {
    NS_ASSERTION(start && end, "nsTDependentSubstring must wrap a non-NULL buffer");

    
    Finalize();

    mData = const_cast<char_type*>(start);
    mLength = end - start;
    SetDataFlags(F_NONE);
  }
