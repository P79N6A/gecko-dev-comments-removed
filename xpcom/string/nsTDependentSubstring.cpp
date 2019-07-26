





void
nsTDependentSubstring_CharT::Rebind(const substring_type& str,
                                    uint32_t startPos, uint32_t length)
{
  
  Finalize();

  size_type strLength = str.Length();

  if (startPos > strLength) {
    startPos = strLength;
  }

  mData = const_cast<char_type*>(static_cast<const char_type*>(str.Data())) + startPos;
  mLength = XPCOM_MIN(length, strLength - startPos);

  SetDataFlags(F_NONE);
}

void
nsTDependentSubstring_CharT::Rebind(const char_type* data, size_type length)
{
  NS_ASSERTION(data, "nsTDependentSubstring must wrap a non-NULL buffer");

  
  Finalize();

  mData = const_cast<char_type*>(static_cast<const char_type*>(data));
  mLength = length;
  SetDataFlags(F_NONE);
}
