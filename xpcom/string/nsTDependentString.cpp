





void
nsTDependentString_CharT::Rebind(const string_type& str, uint32_t startPos)
{
  MOZ_ASSERT(str.Flags() & F_TERMINATED, "Unterminated flat string");

  
  Finalize();

  size_type strLength = str.Length();

  if (startPos > strLength) {
    startPos = strLength;
  }

  mData = const_cast<char_type*>(static_cast<const char_type*>(str.Data())) + startPos;
  mLength = strLength - startPos;

  SetDataFlags(str.Flags() & (F_TERMINATED | F_LITERAL));
}
